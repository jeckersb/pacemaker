/*
 * Copyright 2009-2019 the Pacemaker project contributors
 *
 * The version control history for this file may have further details.
 *
 * This source code is licensed under the GNU General Public License version 2
 * or later (GPLv2+) WITHOUT ANY WARRANTY.
 */

#include <crm_internal.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include <sys/stat.h>
#include <sys/param.h>
#include <sys/types.h>
#include <dirent.h>

#include <crm/crm.h>
#include <crm/cib.h>
#include <crm/common/util.h>
#include <crm/common/iso8601.h>
#include <crm/pengine/status.h>
#include <pacemaker-internal.h>

cib_t *global_cib = NULL;
GListPtr op_fail = NULL;
bool action_numbers = FALSE;
gboolean quiet = FALSE;
gboolean print_pending = TRUE;
char *temp_shadow = NULL;
extern gboolean bringing_nodes_online;

#define quiet_log(fmt, args...) do {		\
	if(quiet == FALSE) {			\
	    printf(fmt , ##args);		\
	}					\
    } while(0)

char *use_date = NULL;

static void
get_date(pe_working_set_t *data_set, bool print_original)
{
    time_t original_date = 0;

    crm_element_value_epoch(data_set->input, "execution-date", &original_date);

    if (use_date) {
        data_set->now = crm_time_new(use_date);
        quiet_log(" + Setting effective cluster time: %s", use_date);
        crm_time_log(LOG_NOTICE, "Pretending 'now' is", data_set->now,
                     crm_time_log_date | crm_time_log_timeofday);


    } else if (original_date) {

        data_set->now = crm_time_new(NULL);
        crm_time_set_timet(data_set->now, &original_date);

        if (print_original) {
            char *when = crm_time_as_string(data_set->now,
                            crm_time_log_date|crm_time_log_timeofday);

            printf("Using the original execution date of: %s\n", when);
            free(when);
        }
    }
}

static void
print_cluster_status(pe_working_set_t * data_set, long options)
{
    char *online_nodes = NULL;
    char *online_remote_nodes = NULL;
    char *online_guest_nodes = NULL;
    char *offline_nodes = NULL;
    char *offline_remote_nodes = NULL;

    GListPtr gIter = NULL;

    for (gIter = data_set->nodes; gIter != NULL; gIter = gIter->next) {
        node_t *node = (node_t *) gIter->data;
        const char *node_mode = NULL;
        char *node_name = NULL;

        if (pe__is_guest_node(node)) {
            node_name = crm_strdup_printf("%s:%s", node->details->uname, node->details->remote_rsc->container->id);
        } else {
            node_name = crm_strdup_printf("%s", node->details->uname);
        }

        if (node->details->unclean) {
            if (node->details->online && node->details->unclean) {
                node_mode = "UNCLEAN (online)";

            } else if (node->details->pending) {
                node_mode = "UNCLEAN (pending)";

            } else {
                node_mode = "UNCLEAN (offline)";
            }

        } else if (node->details->pending) {
            node_mode = "pending";

        } else if (node->details->standby_onfail && node->details->online) {
            node_mode = "standby (on-fail)";

        } else if (node->details->standby) {
            if (node->details->online) {
                node_mode = "standby";
            } else {
                node_mode = "OFFLINE (standby)";
            }

        } else if (node->details->maintenance) {
            if (node->details->online) {
                node_mode = "maintenance";
            } else {
                node_mode = "OFFLINE (maintenance)";
            }

        } else if (node->details->online) {
            if (pe__is_guest_node(node)) {
                online_guest_nodes = add_list_element(online_guest_nodes, node_name);
            } else if (pe__is_remote_node(node)) {
                online_remote_nodes = add_list_element(online_remote_nodes, node_name);
            } else {
                online_nodes = add_list_element(online_nodes, node_name);
            }
            free(node_name);
            continue;

        } else {
            if (pe__is_remote_node(node)) {
                offline_remote_nodes = add_list_element(offline_remote_nodes, node_name);
            } else if (pe__is_guest_node(node)) {
                /* ignore offline container nodes */
            } else {
                offline_nodes = add_list_element(offline_nodes, node_name);
            }
            free(node_name);
            continue;
        }

        if (pe__is_guest_node(node)) {
            printf("GuestNode %s: %s\n", node_name, node_mode);
        } else if (pe__is_remote_node(node)) {
            printf("RemoteNode %s: %s\n", node_name, node_mode);
        } else if (safe_str_eq(node->details->uname, node->details->id)) {
            printf("Node %s: %s\n", node_name, node_mode);
        } else {
            printf("Node %s (%s): %s\n", node_name, node->details->id, node_mode);
        }

        free(node_name);
    }

    if (online_nodes) {
        printf("Online: [%s ]\n", online_nodes);
        free(online_nodes);
    }
    if (offline_nodes) {
        printf("OFFLINE: [%s ]\n", offline_nodes);
        free(offline_nodes);
    }
    if (online_remote_nodes) {
        printf("RemoteOnline: [%s ]\n", online_remote_nodes);
        free(online_remote_nodes);
    }
    if (offline_remote_nodes) {
        printf("RemoteOFFLINE: [%s ]\n", offline_remote_nodes);
        free(offline_remote_nodes);
    }
    if (online_guest_nodes) {
        printf("GuestOnline: [%s ]\n", online_guest_nodes);
        free(online_guest_nodes);
    }

    fprintf(stdout, "\n");
    for (gIter = data_set->resources; gIter != NULL; gIter = gIter->next) {
        resource_t *rsc = (resource_t *) gIter->data;

        if (is_set(rsc->flags, pe_rsc_orphan)
            && rsc->role == RSC_ROLE_STOPPED) {
            continue;
        }
        rsc->fns->print(rsc, NULL, pe_print_printf | options, stdout);
    }
    fprintf(stdout, "\n");
}

static char *
create_action_name(pe_action_t *action)
{
    char *action_name = NULL;
    const char *prefix = "";
    const char *action_host = NULL;
    const char *clone_name = NULL;
    const char *task = action->task;

    if (action->node) {
        action_host = action->node->details->uname;
    } else if (is_not_set(action->flags, pe_action_pseudo)) {
        action_host = "<none>";
    }

    if (safe_str_eq(action->task, RSC_CANCEL)) {
        prefix = "Cancel ";
        task = action->cancel_task;
    }

    if (action->rsc && action->rsc->clone_name) {
        clone_name = action->rsc->clone_name;
    }

    if (clone_name) {
        char *key = NULL;
        const char *interval_ms_s = NULL;
        guint interval_ms = 0;

        interval_ms_s = g_hash_table_lookup(action->meta,
                                            XML_LRM_ATTR_INTERVAL_MS);
        interval_ms = crm_parse_ms(interval_ms_s);

        if (safe_str_eq(action->task, RSC_NOTIFY)
            || safe_str_eq(action->task, RSC_NOTIFIED)) {
            const char *n_type = g_hash_table_lookup(action->meta, "notify_key_type");
            const char *n_task = g_hash_table_lookup(action->meta, "notify_key_operation");

            CRM_ASSERT(n_type != NULL);
            CRM_ASSERT(n_task != NULL);
            key = generate_notify_key(clone_name, n_type, n_task);

        } else {
            key = generate_op_key(clone_name, task, interval_ms);
        }

        if (action_host) {
            action_name = crm_strdup_printf("%s%s %s", prefix, key, action_host);
        } else {
            action_name = crm_strdup_printf("%s%s", prefix, key);
        }
        free(key);

    } else if (safe_str_eq(action->task, CRM_OP_FENCE)) {
        const char *op = g_hash_table_lookup(action->meta, "stonith_action");

        action_name = crm_strdup_printf("%s%s '%s' %s", prefix, action->task, op, action_host);

    } else if (action->rsc && action_host) {
        action_name = crm_strdup_printf("%s%s %s", prefix, action->uuid, action_host);

    } else if (action_host) {
        action_name = crm_strdup_printf("%s%s %s", prefix, action->task, action_host);

    } else {
        action_name = crm_strdup_printf("%s", action->uuid);
    }

    if (action_numbers) { // i.e. verbose
        char *with_id = crm_strdup_printf("%s (%d)", action_name, action->id);

        free(action_name);
        action_name = with_id;
    }
    return action_name;
}

static void
create_dotfile(pe_working_set_t * data_set, const char *dot_file, gboolean all_actions)
{
    GListPtr gIter = NULL;
    FILE *dot_strm = fopen(dot_file, "w");

    if (dot_strm == NULL) {
        crm_perror(LOG_ERR, "Could not open %s for writing", dot_file);
        return;
    }

    fprintf(dot_strm, " digraph \"g\" {\n");
    for (gIter = data_set->actions; gIter != NULL; gIter = gIter->next) {
        action_t *action = (action_t *) gIter->data;
        const char *style = "dashed";
        const char *font = "black";
        const char *color = "black";
        char *action_name = create_action_name(action);

        crm_trace("Action %d: %s %s %p", action->id, action_name, action->uuid, action);

        if (is_set(action->flags, pe_action_pseudo)) {
            font = "orange";
        }

        if (is_set(action->flags, pe_action_dumped)) {
            style = "bold";
            color = "green";

        } else if (action->rsc != NULL && is_not_set(action->rsc->flags, pe_rsc_managed)) {
            color = "red";
            font = "purple";
            if (all_actions == FALSE) {
                goto do_not_write;
            }

        } else if (is_set(action->flags, pe_action_optional)) {
            color = "blue";
            if (all_actions == FALSE) {
                goto do_not_write;
            }

        } else {
            color = "red";
            CRM_CHECK(is_set(action->flags, pe_action_runnable) == FALSE,;
                );
        }

        set_bit(action->flags, pe_action_dumped);
        crm_trace("\"%s\" [ style=%s color=\"%s\" fontcolor=\"%s\"]",
                action_name, style, color, font);
        fprintf(dot_strm, "\"%s\" [ style=%s color=\"%s\" fontcolor=\"%s\"]\n",
                action_name, style, color, font);
  do_not_write:
        free(action_name);
    }

    for (gIter = data_set->actions; gIter != NULL; gIter = gIter->next) {
        action_t *action = (action_t *) gIter->data;

        GListPtr gIter2 = NULL;

        for (gIter2 = action->actions_before; gIter2 != NULL; gIter2 = gIter2->next) {
            action_wrapper_t *before = (action_wrapper_t *) gIter2->data;

            char *before_name = NULL;
            char *after_name = NULL;
            const char *style = "dashed";
            gboolean optional = TRUE;

            if (before->state == pe_link_dumped) {
                optional = FALSE;
                style = "bold";
            } else if (is_set(action->flags, pe_action_pseudo)
                       && (before->type & pe_order_stonith_stop)) {
                continue;
            } else if (before->type == pe_order_none) {
                continue;
            } else if (is_set(before->action->flags, pe_action_dumped)
                       && is_set(action->flags, pe_action_dumped)
                       && before->type != pe_order_load) {
                optional = FALSE;
            }

            if (all_actions || optional == FALSE) {
                before_name = create_action_name(before->action);
                after_name = create_action_name(action);
                crm_trace("\"%s\" -> \"%s\" [ style = %s]",
                        before_name, after_name, style);
                fprintf(dot_strm, "\"%s\" -> \"%s\" [ style = %s]\n",
                        before_name, after_name, style);
                free(before_name);
                free(after_name);
            }
        }
    }

    fprintf(dot_strm, "}\n");
    fflush(dot_strm);
    fclose(dot_strm);
}

static void
setup_input(const char *input, const char *output)
{
    int rc = pcmk_ok;
    cib_t *cib_conn = NULL;
    xmlNode *cib_object = NULL;
    char *local_output = NULL;

    if (input == NULL) {
        /* Use live CIB */
        cib_conn = cib_new();
        rc = cib_conn->cmds->signon(cib_conn, crm_system_name, cib_command);

        if (rc == pcmk_ok) {
            rc = cib_conn->cmds->query(cib_conn, NULL, &cib_object, cib_scope_local | cib_sync_call);
        }

        cib_conn->cmds->signoff(cib_conn);
        cib_delete(cib_conn);
        cib_conn = NULL;

        if (rc != pcmk_ok) {
            fprintf(stderr, "Live CIB query failed: %s (%d)\n", pcmk_strerror(rc), rc);
            crm_exit(crm_errno2exit(rc));

        } else if (cib_object == NULL) {
            fprintf(stderr, "Live CIB query failed: empty result\n");
            crm_exit(CRM_EX_NOINPUT);
        }

    } else if (safe_str_eq(input, "-")) {
        cib_object = filename2xml(NULL);

    } else {
        cib_object = filename2xml(input);
    }

    if (get_object_root(XML_CIB_TAG_STATUS, cib_object) == NULL) {
        create_xml_node(cib_object, XML_CIB_TAG_STATUS);
    }

    if (cli_config_update(&cib_object, NULL, FALSE) == FALSE) {
        free_xml(cib_object);
        crm_exit(CRM_EX_CONFIG);
    }

    if (validate_xml(cib_object, NULL, FALSE) != TRUE) {
        free_xml(cib_object);
        crm_exit(CRM_EX_CONFIG);
    }

    if (output == NULL) {
        char *pid = crm_getpid_s();

        local_output = get_shadow_file(pid);
        temp_shadow = strdup(local_output);
        output = local_output;
        free(pid);
    }

    rc = write_xml_file(cib_object, output, FALSE);
    free_xml(cib_object);
    cib_object = NULL;

    if (rc < 0) {
        fprintf(stderr, "Could not create '%s': %s\n",
                output, pcmk_strerror(rc));
        crm_exit(CRM_EX_CANTCREAT);
    }
    setenv("CIB_file", output, 1);
    free(local_output);
}


/* *INDENT-OFF* */
static struct crm_option long_options[] = {
    /* Top-level Options */
    {"help",    0, 0, '?', "\tThis text"},
    {"version", 0, 0, '$', "\tVersion information"  },
    {"quiet",   0, 0, 'Q', "\tDisplay only essentialoutput"},
    {"verbose", 0, 0, 'V', "\tIncrease debug output"},

    {"-spacer-",      0, 0, '-', "\nOperations:"},
    {"run",           0, 0, 'R', "\tDetermine the cluster's response to the given configuration and status"},
    {"simulate",      0, 0, 'S', "Simulate the transition's execution and display the resulting cluster status"},
    {"in-place",      0, 0, 'X', "Simulate the transition's execution and store the result back to the input file"},
    {"show-scores",   0, 0, 's', "Show allocation scores"},
    {"show-utilization",   0, 0, 'U', "Show utilization information"},
    {"profile",       1, 0, 'P', "Run all tests in the named directory to create profiling data"},
    {"repeat",        1, 0, 'N', "With --profile, repeat each test N times and print timings"},
    {"pending",       0, 0, 'j', "\tDisplay pending state if 'record-pending' is enabled", pcmk_option_hidden},

    {"-spacer-",     0, 0, '-', "\nSynthetic Cluster Events:"},
    {"node-up",      1, 0, 'u', "\tBring a node online"},
    {"node-down",    1, 0, 'd', "\tTake a node offline"},
    {"node-fail",    1, 0, 'f', "\tMark a node as failed"},
    {"op-inject",    1, 0, 'i', "\tGenerate a failure for the cluster to react to in the simulation"},
    {"-spacer-",     0, 0, '-', "\t\tValue is of the form ${resource}_${task}_${interval_in_ms}@${node}=${rc}."},
    {"-spacer-",     0, 0, '-', "\t\tEg. memcached_monitor_20000@bart.example.com=7"},
    {"-spacer-",     0, 0, '-', "\t\tFor more information on OCF return codes, refer to: https://clusterlabs.org/pacemaker/doc/en-US/Pacemaker/2.0/html/Pacemaker_Administration/s-ocf-return-codes.html"},
    {"op-fail",      1, 0, 'F', "\tIf the specified task occurs during the simulation, have it fail with return code ${rc}"},
    {"-spacer-",     0, 0, '-', "\t\tValue is of the form ${resource}_${task}_${interval_in_ms}@${node}=${rc}."},
    {"-spacer-",     0, 0, '-', "\t\tEg. memcached_stop_0@bart.example.com=1\n"},
    {"-spacer-",     0, 0, '-', "\t\tThe transition will normally stop at the failed action.  Save the result with --save-output and re-run with --xml-file"},
    {   "set-datetime", required_argument, NULL, 't',
        "Set date/time (ISO 8601 format, see https://en.wikipedia.org/wiki/ISO_8601)"
    },
    {"quorum",       1, 0, 'q', "\tSpecify a value for quorum"},
    {"watchdog",     1, 0, 'w', "\tAssume a watchdog device is active"},
    {"ticket-grant",     1, 0, 'g', "Grant a ticket"},
    {"ticket-revoke",    1, 0, 'r', "Revoke a ticket"},
    {"ticket-standby",   1, 0, 'b', "Make a ticket standby"},
    {"ticket-activate",  1, 0, 'e', "Activate a ticket"},

    {"-spacer-",     0, 0, '-', "\nOutput Options:"},

    {"save-input",   1, 0, 'I', "\tSave the input configuration to the named file"},
    {"save-output",  1, 0, 'O', "Save the output configuration to the named file"},
    {"save-graph",   1, 0, 'G', "\tSave the transition graph (XML format) to the named file"},
    {"save-dotfile", 1, 0, 'D', "Save the transition graph (DOT format) to the named file"},
    {"all-actions",  0, 0, 'a', "\tDisplay all possible actions in the DOT graph - even ones not part of the transition"},

    {"-spacer-",    0, 0, '-', "\nData Source:"},
    {"live-check",  0, 0, 'L', "\tConnect to the CIB mamager and use the current CIB contents as input"},
    {"xml-file",    1, 0, 'x', "\tRetrieve XML from the named file"},
    {"xml-pipe",    0, 0, 'p', "\tRetrieve XML from stdin"},

    {"-spacer-",    0, 0, '-', "\nExamples:\n"},
    {"-spacer-",    0, 0, '-', "Pretend a recurring monitor action found memcached stopped on node fred.example.com and, during recovery, that the memcached stop action failed", pcmk_option_paragraph},
    {"-spacer-",    0, 0, '-', " crm_simulate -LS --op-inject memcached:0_monitor_20000@bart.example.com=7 --op-fail memcached:0_stop_0@fred.example.com=1 --save-output /tmp/memcached-test.xml", pcmk_option_example},
    {"-spacer-",    0, 0, '-', "Now see what the reaction to the stop failure would be", pcmk_option_paragraph},
    {"-spacer-",    0, 0, '-', " crm_simulate -S --xml-file /tmp/memcached-test.xml", pcmk_option_example},

    {0, 0, 0, 0}
};
/* *INDENT-ON* */

static void
profile_one(const char *xml_file, long long repeat, pe_working_set_t *data_set)
{
    xmlNode *cib_object = NULL;
    clock_t start = 0;

    printf("* Testing %s ...", xml_file);
    fflush(stdout);

    cib_object = filename2xml(xml_file);
    start = clock();

    if (get_object_root(XML_CIB_TAG_STATUS, cib_object) == NULL) {
        create_xml_node(cib_object, XML_CIB_TAG_STATUS);
    }


    if (cli_config_update(&cib_object, NULL, FALSE) == FALSE) {
        free_xml(cib_object);
        return;
    }

    if (validate_xml(cib_object, NULL, FALSE) != TRUE) {
        free_xml(cib_object);
        return;
    }

    for (int i = 0; i < repeat; ++i) {
        xmlNode *input = (repeat == 1)? cib_object : copy_xml(cib_object);

        data_set->input = input;
        get_date(data_set, false);
        pcmk__schedule_actions(data_set, input, NULL);
        pe_reset_working_set(data_set);
    }
    printf(" %.2f secs\n", (clock() - start) / (float) CLOCKS_PER_SEC);
}

#ifndef FILENAME_MAX
#  define FILENAME_MAX 512
#endif

static void
profile_all(const char *dir, long long repeat, pe_working_set_t *data_set)
{
    struct dirent **namelist;

    int file_num = scandir(dir, &namelist, 0, alphasort);

    if (file_num > 0) {
        struct stat prop;
        char buffer[FILENAME_MAX];

        while (file_num--) {
            if ('.' == namelist[file_num]->d_name[0]) {
                free(namelist[file_num]);
                continue;

            } else if (!crm_ends_with_ext(namelist[file_num]->d_name, ".xml")) {
                free(namelist[file_num]);
                continue;
            }
            snprintf(buffer, sizeof(buffer), "%s/%s", dir, namelist[file_num]->d_name);
            if (stat(buffer, &prop) == 0 && S_ISREG(prop.st_mode)) {
                profile_one(buffer, repeat, data_set);
            }
            free(namelist[file_num]);
        }
        free(namelist);
    }
}

static int
count_resources(pe_working_set_t * data_set, resource_t * rsc)
{
    int count = 0;
    GListPtr gIter = NULL;

    if (rsc == NULL) {
        gIter = data_set->resources;
    } else if (rsc->children) {
        gIter = rsc->children;
    } else {
        return is_not_set(rsc->flags, pe_rsc_orphan);
    }

    for (; gIter != NULL; gIter = gIter->next) {
        count += count_resources(data_set, gIter->data);
    }
    return count;
}

int
main(int argc, char **argv)
{
    int rc = pcmk_ok;
    guint modified = 0;

    gboolean store = FALSE;
    gboolean process = FALSE;
    gboolean simulate = FALSE;
    gboolean all_actions = FALSE;
    gboolean have_stdout = FALSE;

    pe_working_set_t *data_set = NULL;

    const char *xml_file = "-";
    const char *quorum = NULL;
    const char *watchdog = NULL;
    const char *test_dir = NULL;
    const char *dot_file = NULL;
    const char *graph_file = NULL;
    const char *input_file = NULL;
    const char *output_file = NULL;
    const char *repeat_s = NULL;

    int flag = 0;
    int index = 0;
    int argerr = 0;
    long long repeat = 1;

    GListPtr node_up = NULL;
    GListPtr node_down = NULL;
    GListPtr node_fail = NULL;
    GListPtr op_inject = NULL;
    GListPtr ticket_grant = NULL;
    GListPtr ticket_revoke = NULL;
    GListPtr ticket_standby = NULL;
    GListPtr ticket_activate = NULL;

    xmlNode *input = NULL;

    crm_log_cli_init("crm_simulate");
    crm_set_options(NULL, "datasource operation [additional options]",
                    long_options, "Tool for simulating the cluster's response to events");

    if (argc < 2) {
        crm_help('?', CRM_EX_USAGE);
    }

    while (1) {
        flag = crm_get_option(argc, argv, &index);
        if (flag == -1)
            break;

        switch (flag) {
            case 'V':
                if (have_stdout == FALSE) {
                    /* Redirect stderr to stdout so we can grep the output */
                    have_stdout = TRUE;
                    close(STDERR_FILENO);
                    dup2(STDOUT_FILENO, STDERR_FILENO);
                }

                crm_bump_log_level(argc, argv);
                action_numbers = TRUE;
                break;
            case '?':
            case '$':
                crm_help(flag, CRM_EX_OK);
                break;
            case 'p':
                xml_file = "-";
                break;
            case 'Q':
                quiet = TRUE;
                break;
            case 'L':
                xml_file = NULL;
                break;
            case 'x':
                xml_file = optarg;
                break;
            case 'u':
                modified++;
                bringing_nodes_online = TRUE;
                node_up = g_list_append(node_up, optarg);
                break;
            case 'd':
                modified++;
                node_down = g_list_append(node_down, optarg);
                break;
            case 'f':
                modified++;
                node_fail = g_list_append(node_fail, optarg);
                break;
            case 't':
                use_date = strdup(optarg);
                break;
            case 'i':
                modified++;
                op_inject = g_list_append(op_inject, optarg);
                break;
            case 'F':
                process = TRUE;
                simulate = TRUE;
                op_fail = g_list_append(op_fail, optarg);
                break;
            case 'w':
                modified++;
                watchdog = optarg;
                break;
            case 'q':
                modified++;
                quorum = optarg;
                break;
            case 'g':
                modified++;
                ticket_grant = g_list_append(ticket_grant, optarg);
                break;
            case 'r':
                modified++;
                ticket_revoke = g_list_append(ticket_revoke, optarg);
                break;
            case 'b':
                modified++;
                ticket_standby = g_list_append(ticket_standby, optarg);
                break;
            case 'e':
                modified++;
                ticket_activate = g_list_append(ticket_activate, optarg);
                break;
            case 'a':
                all_actions = TRUE;
                break;
            case 's':
                process = TRUE;
                show_scores = TRUE;
                break;
            case 'U':
                process = TRUE;
                show_utilization = TRUE;
                break;
            case 'j':
                print_pending = TRUE;
                break;
            case 'S':
                process = TRUE;
                simulate = TRUE;
                break;
            case 'X':
                store = TRUE;
                process = TRUE;
                simulate = TRUE;
                break;
            case 'R':
                process = TRUE;
                break;
            case 'D':
                process = TRUE;
                dot_file = optarg;
                break;
            case 'G':
                process = TRUE;
                graph_file = optarg;
                break;
            case 'I':
                input_file = optarg;
                break;
            case 'O':
                output_file = optarg;
                break;
            case 'P':
                test_dir = optarg;
                break;
            case 'N':
                repeat_s = optarg;
                break;
            default:
                ++argerr;
                break;
        }
    }

    if (optind > argc) {
        ++argerr;
    }

    if (argerr) {
        crm_help('?', CRM_EX_USAGE);
    }

    data_set = pe_new_working_set();
    if (data_set == NULL) {
        crm_perror(LOG_ERR, "Could not allocate working set");
        rc = -ENOMEM;
        goto done;
    }

    if (test_dir != NULL) {
        if (repeat_s != NULL) {
            repeat = crm_int_helper(repeat_s, NULL);
            if (errno || (repeat < 1)) {
                fprintf(stderr, "--repeat must be positive integer, not '%s' -- using 1",
                        repeat_s);
                repeat = 1;
            }
        }
        profile_all(test_dir, repeat, data_set);
        return CRM_EX_OK;
    }

    setup_input(xml_file, store ? xml_file : output_file);

    global_cib = cib_new();
    rc = global_cib->cmds->signon(global_cib, crm_system_name, cib_command);
    if (rc != pcmk_ok) {
        fprintf(stderr, "Could not connect to the CIB manager: %s\n",
                pcmk_strerror(rc));
        goto done;
    }

    rc = global_cib->cmds->query(global_cib, NULL, &input, cib_sync_call | cib_scope_local);
    if (rc != pcmk_ok) {
        fprintf(stderr, "Could not get local CIB: %s\n", pcmk_strerror(rc));
        goto done;
    }

    data_set->input = input;
    get_date(data_set, true);
    if(xml_file) {
        set_bit(data_set->flags, pe_flag_sanitized);
    }
    set_bit(data_set->flags, pe_flag_stdout);
    cluster_status(data_set);

    if (quiet == FALSE) {
        int options = print_pending ? pe_print_pending : 0;

        if (is_set(data_set->flags, pe_flag_maintenance_mode)) {
            quiet_log("\n              *** Resource management is DISABLED ***");
            quiet_log("\n  The cluster will not attempt to start, stop or recover services");
            quiet_log("\n");
        }

        if (data_set->disabled_resources || data_set->blocked_resources) {
            quiet_log("%d of %d resources DISABLED and %d BLOCKED from being started due to failures\n",
                      data_set->disabled_resources,
                      count_resources(data_set, NULL),
                      data_set->blocked_resources);
        }

        quiet_log("\nCurrent cluster status:\n");
        print_cluster_status(data_set, options);
    }

    if (modified) {
        quiet_log("Performing requested modifications\n");
        modify_configuration(data_set, global_cib, quorum, watchdog, node_up, node_down, node_fail, op_inject,
                             ticket_grant, ticket_revoke, ticket_standby, ticket_activate);

        rc = global_cib->cmds->query(global_cib, NULL, &input, cib_sync_call);
        if (rc != pcmk_ok) {
            fprintf(stderr, "Could not get modified CIB: %s\n", pcmk_strerror(rc));
            goto done;
        }

        cleanup_calculations(data_set);
        data_set->input = input;
        get_date(data_set, true);

        if(xml_file) {
            set_bit(data_set->flags, pe_flag_sanitized);
        }
        set_bit(data_set->flags, pe_flag_stdout);
        cluster_status(data_set);
    }

    if (input_file != NULL) {
        rc = write_xml_file(input, input_file, FALSE);
        if (rc < 0) {
            fprintf(stderr, "Could not create '%s': %s\n",
                    input_file, pcmk_strerror(rc));
            goto done;
        }
    }

    if (process || simulate) {
        crm_time_t *local_date = NULL;

        if (show_scores && show_utilization) {
            printf("Allocation scores and utilization information:\n");
        } else if (show_scores) {
            fprintf(stdout, "Allocation scores:\n");
        } else if (show_utilization) {
            printf("Utilization information:\n");
        }

        pcmk__schedule_actions(data_set, input, local_date);
        input = NULL;           /* Don't try and free it twice */

        if (graph_file != NULL) {
            write_xml_file(data_set->graph, graph_file, FALSE);
        }

        if (dot_file != NULL) {
            create_dotfile(data_set, dot_file, all_actions);
        }

        if (quiet == FALSE) {
            GListPtr gIter = NULL;

            quiet_log("%sTransition Summary:\n", show_scores || show_utilization
                      || modified ? "\n" : "");
            fflush(stdout);

            LogNodeActions(data_set, TRUE);
            for (gIter = data_set->resources; gIter != NULL; gIter = gIter->next) {
                resource_t *rsc = (resource_t *) gIter->data;

                LogActions(rsc, data_set, TRUE);
            }
        }
    }

    rc = pcmk_ok;

    if (simulate) {
        if (run_simulation(data_set, global_cib, op_fail, quiet) != pcmk_ok) {
            rc = pcmk_err_generic;
        }
        if(quiet == FALSE) {
            get_date(data_set, true);

            quiet_log("\nRevised cluster status:\n");
            set_bit(data_set->flags, pe_flag_stdout);
            cluster_status(data_set);
            print_cluster_status(data_set, 0);
        }
    }

  done:
    pe_free_working_set(data_set);
    global_cib->cmds->signoff(global_cib);
    cib_delete(global_cib);
    free(use_date);
    fflush(stderr);

    if (temp_shadow) {
        unlink(temp_shadow);
        free(temp_shadow);
    }
    crm_exit(crm_errno2exit(rc));
}
