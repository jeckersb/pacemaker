/*
 * Copyright 2004-2019 the Pacemaker project contributors
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
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <time.h>

#include <sys/param.h>
#include <sys/types.h>

#include <crm/crm.h>
#include <crm/msg_xml.h>
#include <crm/common/xml.h>
#include <crm/common/ipc.h>
#include <crm/common/util.h>
#include <crm/cluster.h>

#include <crm/cib.h>
#include <crm/attrd.h>
#include <sys/utsname.h>

gboolean BE_QUIET = FALSE;
char command = 'G';

const char *dest_uname = NULL;
char *dest_node = NULL;
char *set_name = NULL;
char *attr_id = NULL;
char *attr_name = NULL;
char *attr_pattern = NULL;
const char *type = NULL;
const char *rsc_id = NULL;
const char *attr_value = NULL;
const char *attr_default = NULL;
const char *set_type = NULL;

/* *INDENT-OFF* */
static struct crm_option long_options[] = {
    /* Top-level Options */
    {"help",    0, 0, '?', "\tThis text"},
    {"version", 0, 0, '$', "\tVersion information"  },
    {"verbose", 0, 0, 'V', "\tIncrease debug output"},
    {"quiet",   0, 0, 'q', "\tPrint only the value on stdout\n"},

    {"name",    1, 0, 'n', "Name of the attribute/option to operate on"},
    {"pattern", 1, 0, 'P', "Pattern matching names of attributes (only with -v/-D and -l reboot)"},

    {"-spacer-",    0, 0, '-', "\nCommands:"},
    {"query",       0, 0, 'G', "\tQuery the current value of the attribute/option"},
    {"update",      1, 0, 'v', "Update the value of the attribute/option"},
    {"delete",      0, 0, 'D', "\tDelete the attribute/option"},

    {"-spacer-",    0, 0, '-', "\nAdditional Options:"},
    {"node",        1, 0, 'N', "Set an attribute for the named node (instead of a cluster option).  See also: -l"},
    {"type",        1, 0, 't', "Which part of the configuration to update/delete/query the option in"},
    {"-spacer-",    0, 0, '-', "\t\t\tValid values: crm_config, rsc_defaults, op_defaults, tickets"},
    {"lifetime",    1, 0, 'l', "Lifetime of the node attribute"},
    {"-spacer-",    0, 0, '-', "\t\t\tValid values: reboot, forever"},
    {"utilization", 0, 0, 'z', "Set an utilization attribute for the node."},
    {"set-name",    1, 0, 's', "(Advanced) The attribute set in which to place the value"},
    {"id",	    1, 0, 'i', "\t(Advanced) The ID used to identify the attribute"},
    {"default",     1, 0, 'd', "(Advanced) The default value to display if none is found in the configuration"},

    {"inhibit-policy-engine", 0, 0, '!', NULL, 1},

    /* legacy */
    {"quiet",       0, 0, 'Q', NULL, 1},
    {"node-uname",  1, 0, 'U', NULL, 1},
    {"get-value",   0, 0, 'G', NULL, 1},
    {"delete-attr", 0, 0, 'D', NULL, 1},
    {"attr-value",  1, 0, 'v', NULL, 1},
    {"attr-name",   1, 0, 'n', NULL, 1},
    {"attr-id",     1, 0, 'i', NULL, 1},

    {"-spacer-",	1, 0, '-', "\nExamples:", pcmk_option_paragraph},
    {"-spacer-",	1, 0, '-', "Add a new node attribute called 'location' with the value of 'office' for host 'myhost':", pcmk_option_paragraph},
    {"-spacer-",	1, 0, '-', " crm_attribute --node myhost --name location --update office", pcmk_option_example},
    {"-spacer-",	1, 0, '-', "Query the value of the 'location' node attribute for host 'myhost':", pcmk_option_paragraph},
    {"-spacer-",	1, 0, '-', " crm_attribute --node myhost --name location --query", pcmk_option_example},
    {"-spacer-",	1, 0, '-', "Change the value of the 'location' node attribute for host 'myhost':", pcmk_option_paragraph},
    {"-spacer-",	1, 0, '-', " crm_attribute --node myhost --name location --update backoffice", pcmk_option_example},
    {"-spacer-",	1, 0, '-', "Delete the 'location' node attribute for host 'myhost':", pcmk_option_paragraph},
    {"-spacer-",	1, 0, '-', " crm_attribute --node myhost --name location --delete", pcmk_option_example},
    {"-spacer-",	1, 0, '-', "Query the value of the cluster-delay cluster option:", pcmk_option_paragraph},
    {"-spacer-",	1, 0, '-', " crm_attribute --type crm_config --name cluster-delay --query", pcmk_option_example},
    {"-spacer-",	1, 0, '-', "Query the value of the cluster-delay cluster option. Only print the value:", pcmk_option_paragraph},
    {"-spacer-",	1, 0, '-', " crm_attribute --type crm_config --name cluster-delay --query --quiet", pcmk_option_example},

    {0, 0, 0, 0}
};
/* *INDENT-ON* */

int
main(int argc, char **argv)
{
    cib_t *the_cib = NULL;
    int rc = pcmk_ok;

    int cib_opts = cib_sync_call;
    int argerr = 0;
    int flag;

    int option_index = 0;
    int is_remote_node = 0;

    bool try_attrd = true;

    crm_log_cli_init("crm_attribute");
    crm_set_options(NULL, "<command> -n <attribute> [options]", long_options,
                    "Manage node's attributes and cluster options."
                    "\n\nAllows node attributes and cluster options to be queried, modified and deleted.\n");

    if (argc < 2) {
        crm_help('?', CRM_EX_USAGE);
    }

    while (1) {
        flag = crm_get_option(argc, argv, &option_index);
        if (flag == -1)
            break;

        switch (flag) {
            case 'V':
                crm_bump_log_level(argc, argv);
                break;
            case '$':
            case '?':
                crm_help(flag, CRM_EX_OK);
                break;
            case 'G':
                command = flag;
                attr_value = optarg;
                break;
            case 'D':
            case 'v':
                command = flag;
                attr_value = optarg;
                crm_log_args(argc, argv);
                break;
            case 'q':
            case 'Q':
                BE_QUIET = TRUE;
                break;
            case 'U':
            case 'N':
                dest_uname = strdup(optarg);
                break;
            case 's':
                set_name = strdup(optarg);
                break;
            case 'l':
            case 't':
                type = optarg;
                break;
            case 'z':
                type = XML_CIB_TAG_NODES;
                set_type = XML_TAG_UTILIZATION;
                break;
            case 'n':
                attr_name = strdup(optarg);
                break;
            case 'P':
                attr_pattern = strdup(optarg);
                break;
            case 'i':
                attr_id = strdup(optarg);
                break;
            case 'r':
                rsc_id = optarg;
                break;
            case 'd':
                attr_default = optarg;
                break;
            case '!':
                crm_warn("Inhibiting notifications for this update");
                cib_opts |= cib_inhibit_notify;
                break;
            default:
                printf("Argument code 0%o (%c) is not (?yet?) supported\n", flag, flag);
                ++argerr;
                break;
        }
    }

    if (optind < argc) {
        printf("non-option ARGV-elements: ");
        while (optind < argc)
            printf("%s ", argv[optind++]);
        printf("\n");
    }

    if (optind > argc) {
        ++argerr;
    }

    if (argerr) {
        crm_help('?', CRM_EX_USAGE);
    }

    the_cib = cib_new();
    rc = the_cib->cmds->signon(the_cib, crm_system_name, cib_command);

    if (rc != pcmk_ok) {
        fprintf(stderr, "Error connecting to the CIB manager: %s\n",
                pcmk_strerror(rc));
        crm_exit(crm_errno2exit(rc));
    }

    if (type == NULL && dest_uname != NULL) {
	    type = "forever";
    }

    if (safe_str_eq(type, "reboot")) {
        type = XML_CIB_TAG_STATUS;

    } else if (safe_str_eq(type, "forever")) {
        type = XML_CIB_TAG_NODES;
    }

    if (type == NULL && dest_uname == NULL) {
        /* we're updating cluster options - don't populate dest_node */
        type = XML_CIB_TAG_CRMCONFIG;

    } else if (safe_str_eq(type, XML_CIB_TAG_CRMCONFIG)) {
    } else if (safe_str_neq(type, XML_CIB_TAG_TICKETS)) {
        /* If we are being called from a resource agent via the cluster,
         * the correct local node name will be passed as an environment
         * variable. Otherwise, we have to ask the cluster.
         */
        dest_uname = attrd_get_target(dest_uname);
        if (dest_uname == NULL) {
            dest_uname = get_local_node_name();
        }

        rc = query_node_uuid(the_cib, dest_uname, &dest_node, &is_remote_node);
        if (pcmk_ok != rc) {
            fprintf(stderr, "Could not map name=%s to a UUID\n", dest_uname);
            the_cib->cmds->signoff(the_cib);
            cib_delete(the_cib);
            crm_exit(crm_errno2exit(rc));
        }
    }

    if ((command == 'D') && (attr_name == NULL) && (attr_pattern == NULL)) {
        fprintf(stderr, "Error: must specify attribute name or pattern to delete\n");
        crm_exit(CRM_EX_USAGE);
    }

    if (attr_pattern) {
        if (((command != 'v') && (command != 'D'))
            || safe_str_neq(type, XML_CIB_TAG_STATUS)) {

            fprintf(stderr, "Error: pattern can only be used with till-reboot update or delete\n");
            crm_exit(CRM_EX_USAGE);
        }
        command = 'u';
        free(attr_name);
        attr_name = attr_pattern;
    }

    // Only go through attribute manager for transient attributes
    try_attrd = safe_str_eq(type, XML_CIB_TAG_STATUS);

    // Don't try to contact attribute manager if we're using a file as CIB
    if (getenv("CIB_file") || getenv("CIB_shadow")) {
        try_attrd = FALSE;
    }

    if (((command == 'v') || (command == 'D') || (command == 'u')) && try_attrd
        && pcmk_ok == attrd_update_delegate(NULL, command, dest_uname, attr_name,
                                            attr_value, type, set_name, NULL, NULL,
                                            is_remote_node?attrd_opt_remote:attrd_opt_none)) {
        crm_info("Update %s=%s sent via pacemaker-attrd",
                 attr_name, ((command == 'D')? "<none>" : attr_value));

    } else if (command == 'D') {
        rc = delete_attr_delegate(the_cib, cib_opts, type, dest_node, set_type, set_name,
                                  attr_id, attr_name, attr_value, TRUE, NULL);

        if (rc == -ENXIO) {
            /* Nothing to delete...
             * which means it's not there...
             * which is what the admin wanted
             */
            rc = pcmk_ok;
        }

    } else if (command == 'v') {
        CRM_LOG_ASSERT(type != NULL);
        CRM_LOG_ASSERT(attr_name != NULL);
        CRM_LOG_ASSERT(attr_value != NULL);

        rc = update_attr_delegate(the_cib, cib_opts, type, dest_node, set_type, set_name,
                                  attr_id, attr_name, attr_value, TRUE, NULL, is_remote_node ? "remote" : NULL);

    } else {                    /* query */

        char *read_value = NULL;

        rc = read_attr_delegate(the_cib, type, dest_node, set_type, set_name,
                                attr_id, attr_name, &read_value, TRUE, NULL);

        if (rc == -ENXIO && attr_default) {
            read_value = strdup(attr_default);
            rc = pcmk_ok;
        }

        crm_info("Read %s=%s %s%s",
                 attr_name, crm_str(read_value), set_name ? "in " : "", set_name ? set_name : "");

        if (rc == -ENOTUNIQ) {
            // Multiple matches (already displayed) are not error for queries
            rc = pcmk_ok;

        } else if (BE_QUIET == FALSE) {
            fprintf(stdout, "%s%s %s%s %s%s value=%s\n",
                    type ? "scope=" : "", type ? type : "",
                    attr_id ? "id=" : "", attr_id ? attr_id : "",
                    attr_name ? "name=" : "", attr_name ? attr_name : "",
                    read_value ? read_value : "(null)");

        } else if (read_value != NULL) {
            fprintf(stdout, "%s\n", read_value);
        }
        free(read_value);
    }

    if (rc == -ENOTUNIQ) {
        printf("Please choose from one of the matches above and supply the 'id' with --attr-id\n");

    } else if (rc != pcmk_ok) {
        fprintf(stderr, "Error performing operation: %s\n", pcmk_strerror(rc));
    }

    the_cib->cmds->signoff(the_cib);
    cib_delete(the_cib);
    crm_exit(crm_errno2exit(rc));
}
