/*
 * Copyright 2004-2019 the Pacemaker project contributors
 *
 * The version control history for this file may have further details.
 *
 * This source code is licensed under the GNU Lesser General Public License
 * version 2.1 or later (LGPLv2.1+) WITHOUT ANY WARRANTY.
 */

#include <crm_internal.h>

#include <glib.h>

#include <crm/crm.h>
#include <crm/services.h>
#include <crm/msg_xml.h>
#include <crm/common/xml.h>

#include <crm/common/util.h>
#include <crm/pengine/rules.h>
#include <crm/pengine/internal.h>
#include <unpack.h>
#include <pe_status_private.h>

CRM_TRACE_INIT_DATA(pe_status);

#define set_config_flag(data_set, option, flag) do {			\
	const char *tmp = pe_pref(data_set->config_hash, option);	\
	if(tmp) {							\
	    if(crm_is_true(tmp)) {					\
		set_bit(data_set->flags, flag);			\
	    } else {							\
		clear_bit(data_set->flags, flag);		\
	    }								\
	}								\
    } while(0)

static void unpack_rsc_op(pe_resource_t *rsc, pe_node_t *node, xmlNode *xml_op,
                          xmlNode **last_failure,
                          enum action_fail_response *failed,
                          pe_working_set_t *data_set);
static gboolean determine_remote_online_status(pe_working_set_t * data_set, node_t * this_node);
static void add_node_attrs(xmlNode *attrs, pe_node_t *node, bool overwrite,
                           pe_working_set_t *data_set);


// Bitmask for warnings we only want to print once
uint32_t pe_wo = 0;

static gboolean
is_dangling_guest_node(node_t *node)
{
    /* we are looking for a remote-node that was supposed to be mapped to a
     * container resource, but all traces of that container have disappeared 
     * from both the config and the status section. */
    if (pe__is_guest_or_remote_node(node) &&
        node->details->remote_rsc &&
        node->details->remote_rsc->container == NULL &&
        is_set(node->details->remote_rsc->flags, pe_rsc_orphan_container_filler)) {
        return TRUE;
    }

    return FALSE;
}


/*!
 * \brief Schedule a fence action for a node
 *
 * \param[in,out] data_set  Current working set of cluster
 * \param[in,out] node      Node to fence
 * \param[in]     reason    Text description of why fencing is needed
 */
void
pe_fence_node(pe_working_set_t * data_set, node_t * node, const char *reason)
{
    CRM_CHECK(node, return);

    /* A guest node is fenced by marking its container as failed */
    if (pe__is_guest_node(node)) {
        resource_t *rsc = node->details->remote_rsc->container;

        if (is_set(rsc->flags, pe_rsc_failed) == FALSE) {
            if (!is_set(rsc->flags, pe_rsc_managed)) {
                crm_notice("Not fencing guest node %s "
                           "(otherwise would because %s): "
                           "its guest resource %s is unmanaged",
                           node->details->uname, reason, rsc->id);
            } else {
                crm_warn("Guest node %s will be fenced "
                         "(by recovering its guest resource %s): %s",
                         node->details->uname, rsc->id, reason);

                /* We don't mark the node as unclean because that would prevent the
                 * node from running resources. We want to allow it to run resources
                 * in this transition if the recovery succeeds.
                 */
                node->details->remote_requires_reset = TRUE;
                set_bit(rsc->flags, pe_rsc_failed);
            }
        }

    } else if (is_dangling_guest_node(node)) {
        crm_info("Cleaning up dangling connection for guest node %s: "
                 "fencing was already done because %s, "
                 "and guest resource no longer exists",
                 node->details->uname, reason);
        set_bit(node->details->remote_rsc->flags, pe_rsc_failed);

    } else if (pe__is_remote_node(node)) {
        resource_t *rsc = node->details->remote_rsc;

        if (rsc && (!is_set(rsc->flags, pe_rsc_managed))) {
            crm_notice("Not fencing remote node %s "
                       "(otherwise would because %s): connection is unmanaged",
                       node->details->uname, reason);
        } else if(node->details->remote_requires_reset == FALSE) {
            node->details->remote_requires_reset = TRUE;
            crm_warn("Remote node %s %s: %s",
                     node->details->uname,
                     pe_can_fence(data_set, node)? "will be fenced" : "is unclean",
                     reason);
        }
        node->details->unclean = TRUE;
        pe_fence_op(node, NULL, TRUE, reason, data_set);

    } else if (node->details->unclean) {
        crm_trace("Cluster node %s %s because %s",
                  node->details->uname,
                  pe_can_fence(data_set, node)? "would also be fenced" : "also is unclean",
                  reason);

    } else {
        crm_warn("Cluster node %s %s: %s",
                 node->details->uname,
                 pe_can_fence(data_set, node)? "will be fenced" : "is unclean",
                 reason);
        node->details->unclean = TRUE;
        pe_fence_op(node, NULL, TRUE, reason, data_set);
    }
}

// @TODO xpaths can't handle templates, rules, or id-refs

// nvpair with provides or requires set to unfencing
#define XPATH_UNFENCING_NVPAIR XML_CIB_TAG_NVPAIR                \
    "[(@" XML_NVPAIR_ATTR_NAME "='" XML_RSC_ATTR_PROVIDES "'"    \
    "or @" XML_NVPAIR_ATTR_NAME "='" XML_RSC_ATTR_REQUIRES "') " \
    "and @" XML_NVPAIR_ATTR_VALUE "='unfencing']"

// unfencing in rsc_defaults or any resource
#define XPATH_ENABLE_UNFENCING \
    "/" XML_TAG_CIB "/" XML_CIB_TAG_CONFIGURATION "/" XML_CIB_TAG_RESOURCES   \
    "//" XML_TAG_META_SETS "/" XPATH_UNFENCING_NVPAIR                                               \
    "|/" XML_TAG_CIB "/" XML_CIB_TAG_CONFIGURATION "/" XML_CIB_TAG_RSCCONFIG  \
    "/" XML_TAG_META_SETS "/" XPATH_UNFENCING_NVPAIR

static
void set_if_xpath(unsigned long long flag, const char *xpath,
                  pe_working_set_t *data_set)
{
    xmlXPathObjectPtr result = NULL;

    if (is_not_set(data_set->flags, flag)) {
        result = xpath_search(data_set->input, xpath);
        if (result && (numXpathResults(result) > 0)) {
            set_bit(data_set->flags, flag);
        }
        freeXpathObject(result);
    }
}

gboolean
unpack_config(xmlNode * config, pe_working_set_t * data_set)
{
    const char *value = NULL;
    GHashTable *config_hash = crm_str_table_new();

    data_set->config_hash = config_hash;

    pe__unpack_dataset_nvpairs(config, XML_CIB_TAG_PROPSET, NULL, config_hash,
                               CIB_OPTIONS_FIRST, FALSE, data_set);

    verify_pe_options(data_set->config_hash);

    set_config_flag(data_set, "enable-startup-probes", pe_flag_startup_probes);
    if(is_not_set(data_set->flags, pe_flag_startup_probes)) {
        crm_info("Startup probes: disabled (dangerous)");
    }

    value = pe_pref(data_set->config_hash, XML_ATTR_HAVE_WATCHDOG);
    if (value && crm_is_true(value)) {
        crm_notice("Watchdog will be used via SBD if fencing is required");
        set_bit(data_set->flags, pe_flag_have_stonith_resource);
    }

    /* Set certain flags via xpath here, so they can be used before the relevant
     * configuration sections are unpacked.
     */
    set_if_xpath(pe_flag_enable_unfencing, XPATH_ENABLE_UNFENCING, data_set);

    value = pe_pref(data_set->config_hash, "stonith-timeout");
    data_set->stonith_timeout = (int) crm_parse_interval_spec(value);
    crm_debug("STONITH timeout: %d", data_set->stonith_timeout);

    set_config_flag(data_set, "stonith-enabled", pe_flag_stonith_enabled);
    crm_debug("STONITH of failed nodes is %s",
              is_set(data_set->flags, pe_flag_stonith_enabled) ? "enabled" : "disabled");

    data_set->stonith_action = pe_pref(data_set->config_hash, "stonith-action");
    if (!strcmp(data_set->stonith_action, "poweroff")) {
        pe_warn_once(pe_wo_poweroff,
                     "Support for stonith-action of 'poweroff' is deprecated "
                     "and will be removed in a future release (use 'off' instead)");
        data_set->stonith_action = "off";
    }
    crm_trace("STONITH will %s nodes", data_set->stonith_action);

    set_config_flag(data_set, "concurrent-fencing", pe_flag_concurrent_fencing);
    crm_debug("Concurrent fencing is %s",
              is_set(data_set->flags, pe_flag_concurrent_fencing) ? "enabled" : "disabled");

    set_config_flag(data_set, "stop-all-resources", pe_flag_stop_everything);
    crm_debug("Stop all active resources: %s",
              is_set(data_set->flags, pe_flag_stop_everything) ? "true" : "false");

    set_config_flag(data_set, "symmetric-cluster", pe_flag_symmetric_cluster);
    if (is_set(data_set->flags, pe_flag_symmetric_cluster)) {
        crm_debug("Cluster is symmetric" " - resources can run anywhere by default");
    }

    value = pe_pref(data_set->config_hash, "no-quorum-policy");

    if (safe_str_eq(value, "ignore")) {
        data_set->no_quorum_policy = no_quorum_ignore;

    } else if (safe_str_eq(value, "freeze")) {
        data_set->no_quorum_policy = no_quorum_freeze;

    } else if (safe_str_eq(value, "suicide")) {
        if (is_set(data_set->flags, pe_flag_stonith_enabled)) {
            int do_panic = 0;

            crm_element_value_int(data_set->input, XML_ATTR_QUORUM_PANIC,
                                  &do_panic);
            if (do_panic || is_set(data_set->flags, pe_flag_have_quorum)) {
                data_set->no_quorum_policy = no_quorum_suicide;
            } else {
                crm_notice("Resetting no-quorum-policy to 'stop': cluster has never had quorum");
                data_set->no_quorum_policy = no_quorum_stop;
            }
        } else {
            crm_config_err("Resetting no-quorum-policy to 'stop': stonith is not configured");
            data_set->no_quorum_policy = no_quorum_stop;
        }

    } else {
        data_set->no_quorum_policy = no_quorum_stop;
    }

    switch (data_set->no_quorum_policy) {
        case no_quorum_freeze:
            crm_debug("On loss of quorum: Freeze resources");
            break;
        case no_quorum_stop:
            crm_debug("On loss of quorum: Stop ALL resources");
            break;
        case no_quorum_suicide:
            crm_notice("On loss of quorum: Fence all remaining nodes");
            break;
        case no_quorum_ignore:
            crm_notice("On loss of quorum: Ignore");
            break;
    }

    set_config_flag(data_set, "stop-orphan-resources", pe_flag_stop_rsc_orphans);
    crm_trace("Orphan resources are %s",
              is_set(data_set->flags, pe_flag_stop_rsc_orphans) ? "stopped" : "ignored");

    set_config_flag(data_set, "stop-orphan-actions", pe_flag_stop_action_orphans);
    crm_trace("Orphan resource actions are %s",
              is_set(data_set->flags, pe_flag_stop_action_orphans) ? "stopped" : "ignored");

    set_config_flag(data_set, "remove-after-stop", pe_flag_remove_after_stop);
    crm_trace("Stopped resources are removed from the status section: %s",
              is_set(data_set->flags, pe_flag_remove_after_stop) ? "true" : "false");

    set_config_flag(data_set, "maintenance-mode", pe_flag_maintenance_mode);
    crm_trace("Maintenance mode: %s",
              is_set(data_set->flags, pe_flag_maintenance_mode) ? "true" : "false");

    set_config_flag(data_set, "start-failure-is-fatal", pe_flag_start_failure_fatal);
    crm_trace("Start failures are %s",
              is_set(data_set->flags,
                     pe_flag_start_failure_fatal) ? "always fatal" : "handled by failcount");

    if (is_set(data_set->flags, pe_flag_stonith_enabled)) {
        set_config_flag(data_set, "startup-fencing", pe_flag_startup_fencing);
    }
    if (is_set(data_set->flags, pe_flag_startup_fencing)) {
        crm_trace("Unseen nodes will be fenced");
    } else {
        pe_warn_once(pe_wo_blind, "Blind faith: not fencing unseen nodes");
    }

    node_score_red = char2score(pe_pref(data_set->config_hash, "node-health-red"));
    node_score_green = char2score(pe_pref(data_set->config_hash, "node-health-green"));
    node_score_yellow = char2score(pe_pref(data_set->config_hash, "node-health-yellow"));

    crm_debug("Node scores: 'red' = %s, 'yellow' = %s, 'green' = %s",
             pe_pref(data_set->config_hash, "node-health-red"),
             pe_pref(data_set->config_hash, "node-health-yellow"),
             pe_pref(data_set->config_hash, "node-health-green"));

    data_set->placement_strategy = pe_pref(data_set->config_hash, "placement-strategy");
    crm_trace("Placement strategy: %s", data_set->placement_strategy);

    return TRUE;
}

static void
destroy_digest_cache(gpointer ptr)
{
    op_digest_cache_t *data = ptr;

    free_xml(data->params_all);
    free_xml(data->params_secure);
    free_xml(data->params_restart);

    free(data->digest_all_calc);
    free(data->digest_restart_calc);
    free(data->digest_secure_calc);

    free(data);
}

node_t *
pe_create_node(const char *id, const char *uname, const char *type,
               const char *score, pe_working_set_t * data_set)
{
    node_t *new_node = NULL;

    if (pe_find_node(data_set->nodes, uname) != NULL) {
        crm_config_warn("Detected multiple node entries with uname=%s"
                        " - this is rarely intended", uname);
    }

    new_node = calloc(1, sizeof(node_t));
    if (new_node == NULL) {
        return NULL;
    }

    new_node->weight = char2score(score);
    new_node->fixed = FALSE;
    new_node->details = calloc(1, sizeof(struct pe_node_shared_s));

    if (new_node->details == NULL) {
        free(new_node);
        return NULL;
    }

    crm_trace("Creating node for entry %s/%s", uname, id);
    new_node->details->id = id;
    new_node->details->uname = uname;
    new_node->details->online = FALSE;
    new_node->details->shutdown = FALSE;
    new_node->details->rsc_discovery_enabled = TRUE;
    new_node->details->running_rsc = NULL;
    new_node->details->type = node_ping;

    if (safe_str_eq(type, "remote")) {
        new_node->details->type = node_remote;
        set_bit(data_set->flags, pe_flag_have_remote_nodes);
    } else if ((type == NULL) || safe_str_eq(type, "member")) {
        new_node->details->type = node_member;
    }

    new_node->details->attrs = crm_str_table_new();

    if (pe__is_guest_or_remote_node(new_node)) {
        g_hash_table_insert(new_node->details->attrs, strdup(CRM_ATTR_KIND),
                            strdup("remote"));
    } else {
        g_hash_table_insert(new_node->details->attrs, strdup(CRM_ATTR_KIND),
                            strdup("cluster"));
    }

    new_node->details->utilization = crm_str_table_new();

    new_node->details->digest_cache = g_hash_table_new_full(crm_str_hash,
                                                            g_str_equal, free,
                                                            destroy_digest_cache);

    data_set->nodes = g_list_insert_sorted(data_set->nodes, new_node, sort_node_uname);
    return new_node;
}

bool
remote_id_conflict(const char *remote_name, pe_working_set_t *data) 
{
    bool match = FALSE;
#if 1
    pe_find_resource(data->resources, remote_name);
#else
    if (data->name_check == NULL) {
        data->name_check = g_hash_table_new(crm_str_hash, g_str_equal);
        for (xml_rsc = __xml_first_child_element(parent); xml_rsc != NULL;
             xml_rsc = __xml_next_element(xml_rsc)) {

            const char *id = ID(xml_rsc);

            /* avoiding heap allocation here because we know the duration of this hashtable allows us to */
            g_hash_table_insert(data->name_check, (char *) id, (char *) id);
        }
    }
    if (g_hash_table_lookup(data->name_check, remote_name)) {
        match = TRUE;
    }
#endif
    if (match) {
        crm_err("Invalid remote-node name, a resource called '%s' already exists.", remote_name);
        return NULL;
    }

    return match;
}


static const char *
expand_remote_rsc_meta(xmlNode *xml_obj, xmlNode *parent, pe_working_set_t *data)
{
    xmlNode *attr_set = NULL;
    xmlNode *attr = NULL;

    const char *container_id = ID(xml_obj);
    const char *remote_name = NULL;
    const char *remote_server = NULL;
    const char *remote_port = NULL;
    const char *connect_timeout = "60s";
    const char *remote_allow_migrate=NULL;
    const char *is_managed = NULL;

    for (attr_set = __xml_first_child_element(xml_obj); attr_set != NULL;
         attr_set = __xml_next_element(attr_set)) {
        if (safe_str_neq((const char *)attr_set->name, XML_TAG_META_SETS)) {
            continue;
        }

        for (attr = __xml_first_child_element(attr_set); attr != NULL;
             attr = __xml_next_element(attr)) {
            const char *value = crm_element_value(attr, XML_NVPAIR_ATTR_VALUE);
            const char *name = crm_element_value(attr, XML_NVPAIR_ATTR_NAME);

            if (safe_str_eq(name, XML_RSC_ATTR_REMOTE_NODE)) {
                remote_name = value;
            } else if (safe_str_eq(name, "remote-addr")) {
                remote_server = value;
            } else if (safe_str_eq(name, "remote-port")) {
                remote_port = value;
            } else if (safe_str_eq(name, "remote-connect-timeout")) {
                connect_timeout = value;
            } else if (safe_str_eq(name, "remote-allow-migrate")) {
                remote_allow_migrate=value;
            } else if (safe_str_eq(name, XML_RSC_ATTR_MANAGED)) {
                is_managed = value;
            }
        }
    }

    if (remote_name == NULL) {
        return NULL;
    }

    if (remote_id_conflict(remote_name, data)) {
        return NULL;
    }

    pe_create_remote_xml(parent, remote_name, container_id,
                         remote_allow_migrate, is_managed,
                         connect_timeout, remote_server, remote_port);
    return remote_name;
}

static void
handle_startup_fencing(pe_working_set_t *data_set, node_t *new_node)
{
    if ((new_node->details->type == node_remote) && (new_node->details->remote_rsc == NULL)) {
        /* Ignore fencing for remote nodes that don't have a connection resource
         * associated with them. This happens when remote node entries get left
         * in the nodes section after the connection resource is removed.
         */
        return;
    }

    if (is_set(data_set->flags, pe_flag_startup_fencing)) {
        // All nodes are unclean until we've seen their status entry
        new_node->details->unclean = TRUE;

    } else {
        // Blind faith ...
        new_node->details->unclean = FALSE;
    }

    /* We need to be able to determine if a node's status section
     * exists or not separate from whether the node is unclean. */
    new_node->details->unseen = TRUE;
}

gboolean
unpack_nodes(xmlNode * xml_nodes, pe_working_set_t * data_set)
{
    xmlNode *xml_obj = NULL;
    node_t *new_node = NULL;
    const char *id = NULL;
    const char *uname = NULL;
    const char *type = NULL;
    const char *score = NULL;

    for (xml_obj = __xml_first_child_element(xml_nodes); xml_obj != NULL;
         xml_obj = __xml_next_element(xml_obj)) {

        if (crm_str_eq((const char *)xml_obj->name, XML_CIB_TAG_NODE, TRUE)) {
            new_node = NULL;

            id = crm_element_value(xml_obj, XML_ATTR_ID);
            uname = crm_element_value(xml_obj, XML_ATTR_UNAME);
            type = crm_element_value(xml_obj, XML_ATTR_TYPE);
            score = crm_element_value(xml_obj, XML_RULE_ATTR_SCORE);
            crm_trace("Processing node %s/%s", uname, id);

            if (id == NULL) {
                crm_config_err("Must specify id tag in <node>");
                continue;
            }
            new_node = pe_create_node(id, uname, type, score, data_set);

            if (new_node == NULL) {
                return FALSE;
            }

/* 		if(data_set->have_quorum == FALSE */
/* 		   && data_set->no_quorum_policy == no_quorum_stop) { */
/* 			/\* start shutting resources down *\/ */
/* 			new_node->weight = -INFINITY; */
/* 		} */

            handle_startup_fencing(data_set, new_node);

            add_node_attrs(xml_obj, new_node, FALSE, data_set);
            pe__unpack_dataset_nvpairs(xml_obj, XML_TAG_UTILIZATION, NULL,
                                       new_node->details->utilization, NULL,
                                       FALSE, data_set);

            crm_trace("Done with node %s", crm_element_value(xml_obj, XML_ATTR_UNAME));
        }
    }

    if (data_set->localhost && pe_find_node(data_set->nodes, data_set->localhost) == NULL) {
        crm_info("Creating a fake local node");
        pe_create_node(data_set->localhost, data_set->localhost, NULL, 0,
                       data_set);
    }

    return TRUE;
}

static void
setup_container(resource_t * rsc, pe_working_set_t * data_set)
{
    const char *container_id = NULL;

    if (rsc->children) {
        GListPtr gIter = rsc->children;

        for (; gIter != NULL; gIter = gIter->next) {
            resource_t *child_rsc = (resource_t *) gIter->data;

            setup_container(child_rsc, data_set);
        }
        return;
    }

    container_id = g_hash_table_lookup(rsc->meta, XML_RSC_ATTR_CONTAINER);
    if (container_id && safe_str_neq(container_id, rsc->id)) {
        resource_t *container = pe_find_resource(data_set->resources, container_id);

        if (container) {
            rsc->container = container;
            set_bit(container->flags, pe_rsc_is_container);
            container->fillers = g_list_append(container->fillers, rsc);
            pe_rsc_trace(rsc, "Resource %s's container is %s", rsc->id, container_id);
        } else {
            pe_err("Resource %s: Unknown resource container (%s)", rsc->id, container_id);
        }
    }
}

gboolean
unpack_remote_nodes(xmlNode * xml_resources, pe_working_set_t * data_set)
{
    xmlNode *xml_obj = NULL;

    /* Create remote nodes and guest nodes from the resource configuration
     * before unpacking resources.
     */
    for (xml_obj = __xml_first_child_element(xml_resources); xml_obj != NULL;
         xml_obj = __xml_next_element(xml_obj)) {

        const char *new_node_id = NULL;

        /* Check for remote nodes, which are defined by ocf:pacemaker:remote
         * primitives.
         */
        if (xml_contains_remote_node(xml_obj)) {
            new_node_id = ID(xml_obj);
            /* The "pe_find_node" check is here to make sure we don't iterate over
             * an expanded node that has already been added to the node list. */
            if (new_node_id && pe_find_node(data_set->nodes, new_node_id) == NULL) {
                crm_trace("Found remote node %s defined by resource %s",
                          new_node_id, ID(xml_obj));
                pe_create_node(new_node_id, new_node_id, "remote", NULL,
                               data_set);
            }
            continue;
        }

        /* Check for guest nodes, which are defined by special meta-attributes
         * of a primitive of any type (for example, VirtualDomain or Xen).
         */
        if (crm_str_eq((const char *)xml_obj->name, XML_CIB_TAG_RESOURCE, TRUE)) {
            /* This will add an ocf:pacemaker:remote primitive to the
             * configuration for the guest node's connection, to be unpacked
             * later.
             */
            new_node_id = expand_remote_rsc_meta(xml_obj, xml_resources, data_set);
            if (new_node_id && pe_find_node(data_set->nodes, new_node_id) == NULL) {
                crm_trace("Found guest node %s in resource %s",
                          new_node_id, ID(xml_obj));
                pe_create_node(new_node_id, new_node_id, "remote", NULL,
                               data_set);
            }
            continue;
        }

        /* Check for guest nodes inside a group. Clones are currently not
         * supported as guest nodes.
         */
        if (crm_str_eq((const char *)xml_obj->name, XML_CIB_TAG_GROUP, TRUE)) {
            xmlNode *xml_obj2 = NULL;
            for (xml_obj2 = __xml_first_child_element(xml_obj); xml_obj2 != NULL;
                 xml_obj2 = __xml_next_element(xml_obj2)) {

                new_node_id = expand_remote_rsc_meta(xml_obj2, xml_resources, data_set);

                if (new_node_id && pe_find_node(data_set->nodes, new_node_id) == NULL) {
                    crm_trace("Found guest node %s in resource %s inside group %s",
                              new_node_id, ID(xml_obj2), ID(xml_obj));
                    pe_create_node(new_node_id, new_node_id, "remote", NULL,
                                   data_set);
                }
            }
        }
    }
    return TRUE;
}

/* Call this after all the nodes and resources have been
 * unpacked, but before the status section is read.
 *
 * A remote node's online status is reflected by the state
 * of the remote node's connection resource. We need to link
 * the remote node to this connection resource so we can have
 * easy access to the connection resource during the PE calculations.
 */
static void
link_rsc2remotenode(pe_working_set_t *data_set, resource_t *new_rsc)
{
    node_t *remote_node = NULL;

    if (new_rsc->is_remote_node == FALSE) {
        return;
    }

    if (is_set(data_set->flags, pe_flag_quick_location)) {
        /* remote_nodes and remote_resources are not linked in quick location calculations */
        return;
    }

    print_resource(LOG_TRACE, "Linking remote-node connection resource, ", new_rsc, FALSE);

    remote_node = pe_find_node(data_set->nodes, new_rsc->id);
    CRM_CHECK(remote_node != NULL, return;);

    remote_node->details->remote_rsc = new_rsc;

    if (new_rsc->container == NULL) {
        /* Handle start-up fencing for remote nodes (as opposed to guest nodes)
         * the same as is done for cluster nodes.
         */
        handle_startup_fencing(data_set, remote_node);

    } else {
        /* pe_create_node() marks the new node as "remote" or "cluster"; now
         * that we know the node is a guest node, update it correctly.
         */
        g_hash_table_replace(remote_node->details->attrs, strdup(CRM_ATTR_KIND),
                             strdup("container"));
    }
}

static void
destroy_tag(gpointer data)
{
    tag_t *tag = data;

    if (tag) {
        free(tag->id);
        g_list_free_full(tag->refs, free);
        free(tag);
    }
}

/*!
 * \internal
 * \brief Parse configuration XML for resource information
 *
 * \param[in]     xml_resources  Top of resource configuration XML
 * \param[in,out] data_set       Where to put resource information
 *
 * \return TRUE
 *
 * \note unpack_remote_nodes() MUST be called before this, so that the nodes can
 *       be used when common_unpack() calls resource_location()
 */
gboolean
unpack_resources(xmlNode * xml_resources, pe_working_set_t * data_set)
{
    xmlNode *xml_obj = NULL;
    GListPtr gIter = NULL;

    data_set->template_rsc_sets = g_hash_table_new_full(crm_str_hash,
                                                        g_str_equal, free,
                                                        destroy_tag);

    for (xml_obj = __xml_first_child_element(xml_resources); xml_obj != NULL;
         xml_obj = __xml_next_element(xml_obj)) {

        resource_t *new_rsc = NULL;

        if (crm_str_eq((const char *)xml_obj->name, XML_CIB_TAG_RSC_TEMPLATE, TRUE)) {
            const char *template_id = ID(xml_obj);

            if (template_id && g_hash_table_lookup_extended(data_set->template_rsc_sets,
                                                            template_id, NULL, NULL) == FALSE) {
                /* Record the template's ID for the knowledge of its existence anyway. */
                g_hash_table_insert(data_set->template_rsc_sets, strdup(template_id), NULL);
            }
            continue;
        }

        crm_trace("Beginning unpack... <%s id=%s... >", crm_element_name(xml_obj), ID(xml_obj));
        if (common_unpack(xml_obj, &new_rsc, NULL, data_set)) {
            data_set->resources = g_list_append(data_set->resources, new_rsc);
            print_resource(LOG_TRACE, "Added ", new_rsc, FALSE);

        } else {
            crm_config_err("Failed unpacking %s %s",
                           crm_element_name(xml_obj), crm_element_value(xml_obj, XML_ATTR_ID));
            if (new_rsc != NULL && new_rsc->fns != NULL) {
                new_rsc->fns->free(new_rsc);
            }
        }
    }

    for (gIter = data_set->resources; gIter != NULL; gIter = gIter->next) {
        resource_t *rsc = (resource_t *) gIter->data;

        setup_container(rsc, data_set);
        link_rsc2remotenode(data_set, rsc);
    }

    data_set->resources = g_list_sort(data_set->resources, sort_rsc_priority);
    if (is_set(data_set->flags, pe_flag_quick_location)) {
        /* Ignore */

    } else if (is_set(data_set->flags, pe_flag_stonith_enabled)
               && is_set(data_set->flags, pe_flag_have_stonith_resource) == FALSE) {

        crm_config_err("Resource start-up disabled since no STONITH resources have been defined");
        crm_config_err("Either configure some or disable STONITH with the stonith-enabled option");
        crm_config_err("NOTE: Clusters with shared data need STONITH to ensure data integrity");
    }

    return TRUE;
}

gboolean
unpack_tags(xmlNode * xml_tags, pe_working_set_t * data_set)
{
    xmlNode *xml_tag = NULL;

    data_set->tags = g_hash_table_new_full(crm_str_hash, g_str_equal, free,
                                           destroy_tag);

    for (xml_tag = __xml_first_child_element(xml_tags); xml_tag != NULL;
         xml_tag = __xml_next_element(xml_tag)) {

        xmlNode *xml_obj_ref = NULL;
        const char *tag_id = ID(xml_tag);

        if (crm_str_eq((const char *)xml_tag->name, XML_CIB_TAG_TAG, TRUE) == FALSE) {
            continue;
        }

        if (tag_id == NULL) {
            crm_config_err("Failed unpacking %s: %s should be specified",
                           crm_element_name(xml_tag), XML_ATTR_ID);
            continue;
        }

        for (xml_obj_ref = __xml_first_child_element(xml_tag); xml_obj_ref != NULL;
             xml_obj_ref = __xml_next_element(xml_obj_ref)) {

            const char *obj_ref = ID(xml_obj_ref);

            if (crm_str_eq((const char *)xml_obj_ref->name, XML_CIB_TAG_OBJ_REF, TRUE) == FALSE) {
                continue;
            }

            if (obj_ref == NULL) {
                crm_config_err("Failed unpacking %s for tag %s: %s should be specified",
                               crm_element_name(xml_obj_ref), tag_id, XML_ATTR_ID);
                continue;
            }

            if (add_tag_ref(data_set->tags, tag_id, obj_ref) == FALSE) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

/* The ticket state section:
 * "/cib/status/tickets/ticket_state" */
static gboolean
unpack_ticket_state(xmlNode * xml_ticket, pe_working_set_t * data_set)
{
    const char *ticket_id = NULL;
    const char *granted = NULL;
    const char *last_granted = NULL;
    const char *standby = NULL;
    xmlAttrPtr xIter = NULL;

    ticket_t *ticket = NULL;

    ticket_id = ID(xml_ticket);
    if (ticket_id == NULL || strlen(ticket_id) == 0) {
        return FALSE;
    }

    crm_trace("Processing ticket state for %s", ticket_id);

    ticket = g_hash_table_lookup(data_set->tickets, ticket_id);
    if (ticket == NULL) {
        ticket = ticket_new(ticket_id, data_set);
        if (ticket == NULL) {
            return FALSE;
        }
    }

    for (xIter = xml_ticket->properties; xIter; xIter = xIter->next) {
        const char *prop_name = (const char *)xIter->name;
        const char *prop_value = crm_element_value(xml_ticket, prop_name);

        if (crm_str_eq(prop_name, XML_ATTR_ID, TRUE)) {
            continue;
        }
        g_hash_table_replace(ticket->state, strdup(prop_name), strdup(prop_value));
    }

    granted = g_hash_table_lookup(ticket->state, "granted");
    if (granted && crm_is_true(granted)) {
        ticket->granted = TRUE;
        crm_info("We have ticket '%s'", ticket->id);
    } else {
        ticket->granted = FALSE;
        crm_info("We do not have ticket '%s'", ticket->id);
    }

    last_granted = g_hash_table_lookup(ticket->state, "last-granted");
    if (last_granted) {
        ticket->last_granted = crm_parse_int(last_granted, 0);
    }

    standby = g_hash_table_lookup(ticket->state, "standby");
    if (standby && crm_is_true(standby)) {
        ticket->standby = TRUE;
        if (ticket->granted) {
            crm_info("Granted ticket '%s' is in standby-mode", ticket->id);
        }
    } else {
        ticket->standby = FALSE;
    }

    crm_trace("Done with ticket state for %s", ticket_id);

    return TRUE;
}

static gboolean
unpack_tickets_state(xmlNode * xml_tickets, pe_working_set_t * data_set)
{
    xmlNode *xml_obj = NULL;

    for (xml_obj = __xml_first_child_element(xml_tickets); xml_obj != NULL;
         xml_obj = __xml_next_element(xml_obj)) {

        if (crm_str_eq((const char *)xml_obj->name, XML_CIB_TAG_TICKET_STATE, TRUE) == FALSE) {
            continue;
        }
        unpack_ticket_state(xml_obj, data_set);
    }

    return TRUE;
}

static void
unpack_handle_remote_attrs(node_t *this_node, xmlNode *state, pe_working_set_t * data_set) 
{
    const char *resource_discovery_enabled = NULL;
    xmlNode *attrs = NULL;
    resource_t *rsc = NULL;

    if (crm_str_eq((const char *)state->name, XML_CIB_TAG_STATE, TRUE) == FALSE) {
        return;
    }

    if ((this_node == NULL) || !pe__is_guest_or_remote_node(this_node)) {
        return;
    }
    crm_trace("Processing remote node id=%s, uname=%s", this_node->details->id, this_node->details->uname);

    this_node->details->remote_maintenance =
        crm_atoi(crm_element_value(state, XML_NODE_IS_MAINTENANCE), "0");

    rsc = this_node->details->remote_rsc;
    if (this_node->details->remote_requires_reset == FALSE) {
        this_node->details->unclean = FALSE;
        this_node->details->unseen = FALSE;
    }
    attrs = find_xml_node(state, XML_TAG_TRANSIENT_NODEATTRS, FALSE);
    add_node_attrs(attrs, this_node, TRUE, data_set);

    if (pe__shutdown_requested(this_node)) {
        crm_info("Node %s is shutting down", this_node->details->uname);
        this_node->details->shutdown = TRUE;
        if (rsc) {
            rsc->next_role = RSC_ROLE_STOPPED;
        }
    }
 
    if (crm_is_true(pe_node_attribute_raw(this_node, "standby"))) {
        crm_info("Node %s is in standby-mode", this_node->details->uname);
        this_node->details->standby = TRUE;
    }

    if (crm_is_true(pe_node_attribute_raw(this_node, "maintenance")) ||
        (rsc && !is_set(rsc->flags, pe_rsc_managed))) {
        crm_info("Node %s is in maintenance-mode", this_node->details->uname);
        this_node->details->maintenance = TRUE;
    }

    resource_discovery_enabled = pe_node_attribute_raw(this_node, XML_NODE_ATTR_RSC_DISCOVERY);
    if (resource_discovery_enabled && !crm_is_true(resource_discovery_enabled)) {
        if (pe__is_remote_node(this_node)
            && is_not_set(data_set->flags, pe_flag_stonith_enabled)) {
            crm_warn("Ignoring %s attribute on remote node %s because stonith is disabled",
                     XML_NODE_ATTR_RSC_DISCOVERY, this_node->details->uname);
        } else {
            /* This is either a remote node with fencing enabled, or a guest
             * node. We don't care whether fencing is enabled when fencing guest
             * nodes, because they are "fenced" by recovering their containing
             * resource.
             */
            crm_info("Node %s has resource discovery disabled", this_node->details->uname);
            this_node->details->rsc_discovery_enabled = FALSE;
        }
    }
}

static bool
unpack_node_loop(xmlNode * status, bool fence, pe_working_set_t * data_set) 
{
    bool changed = false;
    xmlNode *lrm_rsc = NULL;

    for (xmlNode *state = __xml_first_child_element(status); state != NULL;
         state = __xml_next_element(state)) {

        const char *id = NULL;
        const char *uname = NULL;
        node_t *this_node = NULL;
        bool process = FALSE;

        if (crm_str_eq((const char *)state->name, XML_CIB_TAG_STATE, TRUE) == FALSE) {
            continue;
        }

        id = crm_element_value(state, XML_ATTR_ID);
        uname = crm_element_value(state, XML_ATTR_UNAME);
        this_node = pe_find_node_any(data_set->nodes, id, uname);

        if (this_node == NULL) {
            crm_info("Node %s is unknown", id);
            continue;

        } else if (this_node->details->unpacked) {
            crm_info("Node %s is already processed", id);
            continue;

        } else if (!pe__is_guest_or_remote_node(this_node)
                   && is_set(data_set->flags, pe_flag_stonith_enabled)) {
            // A redundant test, but preserves the order for regression tests
            process = TRUE;

        } else if (pe__is_guest_or_remote_node(this_node)) {
            bool check = FALSE;
            resource_t *rsc = this_node->details->remote_rsc;

            if(fence) {
                check = TRUE;

            } else if(rsc == NULL) {
                /* Not ready yet */

            } else if (pe__is_guest_node(this_node)
                       && rsc->role == RSC_ROLE_STARTED
                       && rsc->container->role == RSC_ROLE_STARTED) {
                /* Both the connection and its containing resource need to be
                 * known to be up before we process resources running in it.
                 */
                check = TRUE;
                crm_trace("Checking node %s/%s/%s status %d/%d/%d", id, rsc->id, rsc->container->id, fence, rsc->role, RSC_ROLE_STARTED);

            } else if (!pe__is_guest_node(this_node)
                       && rsc->role == RSC_ROLE_STARTED) {
                check = TRUE;
                crm_trace("Checking node %s/%s status %d/%d/%d", id, rsc->id, fence, rsc->role, RSC_ROLE_STARTED);
            }

            if (check) {
                determine_remote_online_status(data_set, this_node);
                unpack_handle_remote_attrs(this_node, state, data_set);
                process = TRUE;
            }

        } else if (this_node->details->online) {
            process = TRUE;

        } else if (fence) {
            process = TRUE;
        }

        if(process) {
            crm_trace("Processing lrm resource entries on %shealthy%s node: %s",
                      fence?"un":"",
                      (pe__is_guest_or_remote_node(this_node)? " remote" : ""),
                      this_node->details->uname);
            changed = TRUE;
            this_node->details->unpacked = TRUE;

            lrm_rsc = find_xml_node(state, XML_CIB_TAG_LRM, FALSE);
            lrm_rsc = find_xml_node(lrm_rsc, XML_LRM_TAG_RESOURCES, FALSE);
            unpack_lrm_resources(this_node, lrm_rsc, data_set);
        }
    }
    return changed;
}

/* remove nodes that are down, stopping */
/* create positive rsc_to_node constraints between resources and the nodes they are running on */
/* anything else? */
gboolean
unpack_status(xmlNode * status, pe_working_set_t * data_set)
{
    const char *id = NULL;
    const char *uname = NULL;

    xmlNode *state = NULL;
    node_t *this_node = NULL;

    crm_trace("Beginning unpack");

    if (data_set->tickets == NULL) {
        data_set->tickets = g_hash_table_new_full(crm_str_hash, g_str_equal,
                                                  free, destroy_ticket);
    }

    for (state = __xml_first_child_element(status); state != NULL;
         state = __xml_next_element(state)) {

        if (crm_str_eq((const char *)state->name, XML_CIB_TAG_TICKETS, TRUE)) {
            unpack_tickets_state((xmlNode *) state, data_set);

        } else if (crm_str_eq((const char *)state->name, XML_CIB_TAG_STATE, TRUE)) {
            xmlNode *attrs = NULL;
            const char *resource_discovery_enabled = NULL;

            id = crm_element_value(state, XML_ATTR_ID);
            uname = crm_element_value(state, XML_ATTR_UNAME);
            this_node = pe_find_node_any(data_set->nodes, id, uname);

            if (uname == NULL) {
                /* error */
                continue;

            } else if (this_node == NULL) {
                crm_config_warn("Node %s in status section no longer exists", uname);
                continue;

            } else if (pe__is_guest_or_remote_node(this_node)) {
                /* online state for remote nodes is determined by the
                 * rsc state after all the unpacking is done. we do however
                 * need to mark whether or not the node has been fenced as this plays
                 * a role during unpacking cluster node resource state */
                this_node->details->remote_was_fenced = 
                    crm_atoi(crm_element_value(state, XML_NODE_IS_FENCED), "0");
                continue;
            }

            crm_trace("Processing node id=%s, uname=%s", id, uname);

            /* Mark the node as provisionally clean
             * - at least we have seen it in the current cluster's lifetime
             */
            this_node->details->unclean = FALSE;
            this_node->details->unseen = FALSE;
            attrs = find_xml_node(state, XML_TAG_TRANSIENT_NODEATTRS, FALSE);
            add_node_attrs(attrs, this_node, TRUE, data_set);

            if (crm_is_true(pe_node_attribute_raw(this_node, "standby"))) {
                crm_info("Node %s is in standby-mode", this_node->details->uname);
                this_node->details->standby = TRUE;
            }

            if (crm_is_true(pe_node_attribute_raw(this_node, "maintenance"))) {
                crm_info("Node %s is in maintenance-mode", this_node->details->uname);
                this_node->details->maintenance = TRUE;
            }

            resource_discovery_enabled = pe_node_attribute_raw(this_node, XML_NODE_ATTR_RSC_DISCOVERY);
            if (resource_discovery_enabled && !crm_is_true(resource_discovery_enabled)) {
                crm_warn("ignoring %s attribute on node %s, disabling resource discovery is not allowed on cluster nodes",
                    XML_NODE_ATTR_RSC_DISCOVERY, this_node->details->uname);
            }

            crm_trace("determining node state");
            determine_online_status(state, this_node, data_set);

            if (is_not_set(data_set->flags, pe_flag_have_quorum)
                && this_node->details->online
                && (data_set->no_quorum_policy == no_quorum_suicide)) {
                /* Everything else should flow from this automatically
                 * At least until the PE becomes able to migrate off healthy resources
                 */
                pe_fence_node(data_set, this_node, "cluster does not have quorum");
            }
        }
    }


    while(unpack_node_loop(status, FALSE, data_set)) {
        crm_trace("Start another loop");
    }

    // Now catch any nodes we didn't see
    unpack_node_loop(status, is_set(data_set->flags, pe_flag_stonith_enabled), data_set);

    /* Now that we know where resources are, we can schedule stops of containers
     * with failed bundle connections
     */
    if (data_set->stop_needed != NULL) {
        for (GList *item = data_set->stop_needed; item; item = item->next) {
            pe_resource_t *container = item->data;
            pe_node_t *node = pe__current_node(container);

            if (node) {
                stop_action(container, node, FALSE);
            }
        }
        g_list_free(data_set->stop_needed);
        data_set->stop_needed = NULL;
    }

    for (GListPtr gIter = data_set->nodes; gIter != NULL; gIter = gIter->next) {
        node_t *this_node = gIter->data;

        if (this_node == NULL) {
            continue;
        } else if (!pe__is_guest_or_remote_node(this_node)) {
            continue;
        } else if(this_node->details->unpacked) {
            continue;
        }
        determine_remote_online_status(data_set, this_node);
    }

    return TRUE;
}

static gboolean
determine_online_status_no_fencing(pe_working_set_t * data_set, xmlNode * node_state,
                                   node_t * this_node)
{
    gboolean online = FALSE;
    const char *join = crm_element_value(node_state, XML_NODE_JOIN_STATE);
    const char *is_peer = crm_element_value(node_state, XML_NODE_IS_PEER);
    const char *in_cluster = crm_element_value(node_state, XML_NODE_IN_CLUSTER);
    const char *exp_state = crm_element_value(node_state, XML_NODE_EXPECTED);

    if (!crm_is_true(in_cluster)) {
        crm_trace("Node is down: in_cluster=%s", crm_str(in_cluster));

    } else if (safe_str_eq(is_peer, ONLINESTATUS)) {
        if (safe_str_eq(join, CRMD_JOINSTATE_MEMBER)) {
            online = TRUE;
        } else {
            crm_debug("Node is not ready to run resources: %s", join);
        }

    } else if (this_node->details->expected_up == FALSE) {
        crm_trace("Controller is down: in_cluster=%s", crm_str(in_cluster));
        crm_trace("\tis_peer=%s, join=%s, expected=%s",
                  crm_str(is_peer), crm_str(join), crm_str(exp_state));

    } else {
        /* mark it unclean */
        pe_fence_node(data_set, this_node, "peer is unexpectedly down");
        crm_info("\tin_cluster=%s, is_peer=%s, join=%s, expected=%s",
                 crm_str(in_cluster), crm_str(is_peer), crm_str(join), crm_str(exp_state));
    }
    return online;
}

static gboolean
determine_online_status_fencing(pe_working_set_t * data_set, xmlNode * node_state,
                                node_t * this_node)
{
    gboolean online = FALSE;
    gboolean do_terminate = FALSE;
    bool crmd_online = FALSE;
    const char *join = crm_element_value(node_state, XML_NODE_JOIN_STATE);
    const char *is_peer = crm_element_value(node_state, XML_NODE_IS_PEER);
    const char *in_cluster = crm_element_value(node_state, XML_NODE_IN_CLUSTER);
    const char *exp_state = crm_element_value(node_state, XML_NODE_EXPECTED);
    const char *terminate = pe_node_attribute_raw(this_node, "terminate");

/*
  - XML_NODE_IN_CLUSTER    ::= true|false
  - XML_NODE_IS_PEER       ::= online|offline
  - XML_NODE_JOIN_STATE    ::= member|down|pending|banned
  - XML_NODE_EXPECTED      ::= member|down
*/

    if (crm_is_true(terminate)) {
        do_terminate = TRUE;

    } else if (terminate != NULL && strlen(terminate) > 0) {
        /* could be a time() value */
        char t = terminate[0];

        if (t != '0' && isdigit(t)) {
            do_terminate = TRUE;
        }
    }

    crm_trace("%s: in_cluster=%s, is_peer=%s, join=%s, expected=%s, term=%d",
              this_node->details->uname, crm_str(in_cluster), crm_str(is_peer),
              crm_str(join), crm_str(exp_state), do_terminate);

    online = crm_is_true(in_cluster);
    crmd_online = safe_str_eq(is_peer, ONLINESTATUS);
    if (exp_state == NULL) {
        exp_state = CRMD_JOINSTATE_DOWN;
    }

    if (this_node->details->shutdown) {
        crm_debug("%s is shutting down", this_node->details->uname);

        /* Slightly different criteria since we can't shut down a dead peer */
        online = crmd_online;

    } else if (in_cluster == NULL) {
        pe_fence_node(data_set, this_node, "peer has not been seen by the cluster");

    } else if (safe_str_eq(join, CRMD_JOINSTATE_NACK)) {
        pe_fence_node(data_set, this_node, "peer failed the pacemaker membership criteria");

    } else if (do_terminate == FALSE && safe_str_eq(exp_state, CRMD_JOINSTATE_DOWN)) {

        if (crm_is_true(in_cluster) || crmd_online) {
            crm_info("- Node %s is not ready to run resources", this_node->details->uname);
            this_node->details->standby = TRUE;
            this_node->details->pending = TRUE;

        } else {
            crm_trace("%s is down or still coming up", this_node->details->uname);
        }

    } else if (do_terminate && safe_str_eq(join, CRMD_JOINSTATE_DOWN)
               && crm_is_true(in_cluster) == FALSE && !crmd_online) {
        crm_info("Node %s was just shot", this_node->details->uname);
        online = FALSE;

    } else if (crm_is_true(in_cluster) == FALSE) {
        pe_fence_node(data_set, this_node, "peer is no longer part of the cluster");

    } else if (!crmd_online) {
        pe_fence_node(data_set, this_node, "peer process is no longer available");

        /* Everything is running at this point, now check join state */
    } else if (do_terminate) {
        pe_fence_node(data_set, this_node, "termination was requested");

    } else if (safe_str_eq(join, CRMD_JOINSTATE_MEMBER)) {
        crm_info("Node %s is active", this_node->details->uname);

    } else if (safe_str_eq(join, CRMD_JOINSTATE_PENDING)
               || safe_str_eq(join, CRMD_JOINSTATE_DOWN)) {
        crm_info("Node %s is not ready to run resources", this_node->details->uname);
        this_node->details->standby = TRUE;
        this_node->details->pending = TRUE;

    } else {
        pe_fence_node(data_set, this_node, "peer was in an unknown state");
        crm_warn("%s: in-cluster=%s, is-peer=%s, join=%s, expected=%s, term=%d, shutdown=%d",
                 this_node->details->uname, crm_str(in_cluster), crm_str(is_peer),
                 crm_str(join), crm_str(exp_state), do_terminate, this_node->details->shutdown);
    }

    return online;
}

static gboolean
determine_remote_online_status(pe_working_set_t * data_set, node_t * this_node)
{
    resource_t *rsc = this_node->details->remote_rsc;
    resource_t *container = NULL;
    pe_node_t *host = NULL;

    /* If there is a node state entry for a (former) Pacemaker Remote node
     * but no resource creating that node, the node's connection resource will
     * be NULL. Consider it an offline remote node in that case.
     */
    if (rsc == NULL) {
        this_node->details->online = FALSE;
        goto remote_online_done;
    }

    container = rsc->container;

    if (container && (g_list_length(rsc->running_on) == 1)) {
        host = rsc->running_on->data;
    }

    /* If the resource is currently started, mark it online. */
    if (rsc->role == RSC_ROLE_STARTED) {
        crm_trace("%s node %s presumed ONLINE because connection resource is started",
                  (container? "Guest" : "Remote"), this_node->details->id);
        this_node->details->online = TRUE;
    }

    /* consider this node shutting down if transitioning start->stop */
    if (rsc->role == RSC_ROLE_STARTED && rsc->next_role == RSC_ROLE_STOPPED) {
        crm_trace("%s node %s shutting down because connection resource is stopping",
                  (container? "Guest" : "Remote"), this_node->details->id);
        this_node->details->shutdown = TRUE;
    }

    /* Now check all the failure conditions. */
    if(container && is_set(container->flags, pe_rsc_failed)) {
        crm_trace("Guest node %s UNCLEAN because guest resource failed",
                  this_node->details->id);
        this_node->details->online = FALSE;
        this_node->details->remote_requires_reset = TRUE;

    } else if(is_set(rsc->flags, pe_rsc_failed)) {
        crm_trace("%s node %s OFFLINE because connection resource failed",
                  (container? "Guest" : "Remote"), this_node->details->id);
        this_node->details->online = FALSE;

    } else if (rsc->role == RSC_ROLE_STOPPED
        || (container && container->role == RSC_ROLE_STOPPED)) {

        crm_trace("%s node %s OFFLINE because its resource is stopped",
                  (container? "Guest" : "Remote"), this_node->details->id);
        this_node->details->online = FALSE;
        this_node->details->remote_requires_reset = FALSE;

    } else if (host && (host->details->online == FALSE)
               && host->details->unclean) {
        crm_trace("Guest node %s UNCLEAN because host is unclean",
                  this_node->details->id);
        this_node->details->online = FALSE;
        this_node->details->remote_requires_reset = TRUE;
    }

remote_online_done:
    crm_trace("Remote node %s online=%s",
        this_node->details->id, this_node->details->online ? "TRUE" : "FALSE");
    return this_node->details->online;
}

gboolean
determine_online_status(xmlNode * node_state, node_t * this_node, pe_working_set_t * data_set)
{
    gboolean online = FALSE;
    const char *exp_state = crm_element_value(node_state, XML_NODE_EXPECTED);

    if (this_node == NULL) {
        crm_config_err("No node to check");
        return online;
    }

    this_node->details->shutdown = FALSE;
    this_node->details->expected_up = FALSE;

    if (pe__shutdown_requested(this_node)) {
        this_node->details->shutdown = TRUE;

    } else if (safe_str_eq(exp_state, CRMD_JOINSTATE_MEMBER)) {
        this_node->details->expected_up = TRUE;
    }

    if (this_node->details->type == node_ping) {
        this_node->details->unclean = FALSE;
        online = FALSE;         /* As far as resource management is concerned,
                                 * the node is safely offline.
                                 * Anyone caught abusing this logic will be shot
                                 */

    } else if (is_set(data_set->flags, pe_flag_stonith_enabled) == FALSE) {
        online = determine_online_status_no_fencing(data_set, node_state, this_node);

    } else {
        online = determine_online_status_fencing(data_set, node_state, this_node);
    }

    if (online) {
        this_node->details->online = TRUE;

    } else {
        /* remove node from contention */
        this_node->fixed = TRUE;
        this_node->weight = -INFINITY;
    }

    if (online && this_node->details->shutdown) {
        /* don't run resources here */
        this_node->fixed = TRUE;
        this_node->weight = -INFINITY;
    }

    if (this_node->details->type == node_ping) {
        crm_info("Node %s is not a pacemaker node", this_node->details->uname);

    } else if (this_node->details->unclean) {
        pe_proc_warn("Node %s is unclean", this_node->details->uname);

    } else if (this_node->details->online) {
        crm_info("Node %s is %s", this_node->details->uname,
                 this_node->details->shutdown ? "shutting down" :
                 this_node->details->pending ? "pending" :
                 this_node->details->standby ? "standby" :
                 this_node->details->maintenance ? "maintenance" : "online");

    } else {
        crm_trace("Node %s is offline", this_node->details->uname);
    }

    return online;
}

/*!
 * \internal
 * \brief Find the end of a resource's name, excluding any clone suffix
 *
 * \param[in] id  Resource ID to check
 *
 * \return Pointer to last character of resource's base name
 */
const char *
pe_base_name_end(const char *id)
{
    if (!crm_strlen_zero(id)) {
        const char *end = id + strlen(id) - 1;

        for (const char *s = end; s > id; --s) {
            switch (*s) {
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    break;
                case ':':
                    return (s == end)? s : (s - 1);
                default:
                    return end;
            }
        }
        return end;
    }
    return NULL;
}

/*!
 * \internal
 * \brief Get a resource name excluding any clone suffix
 *
 * \param[in] last_rsc_id  Resource ID to check
 *
 * \return Pointer to newly allocated string with resource's base name
 * \note It is the caller's responsibility to free() the result.
 *       This asserts on error, so callers can assume result is not NULL.
 */
char *
clone_strip(const char *last_rsc_id)
{
    const char *end = pe_base_name_end(last_rsc_id);
    char *basename = NULL;

    CRM_ASSERT(end);
    basename = strndup(last_rsc_id, end - last_rsc_id + 1);
    CRM_ASSERT(basename);
    return basename;
}

/*!
 * \internal
 * \brief Get the name of the first instance of a cloned resource
 *
 * \param[in] last_rsc_id  Resource ID to check
 *
 * \return Pointer to newly allocated string with resource's base name plus :0
 * \note It is the caller's responsibility to free() the result.
 *       This asserts on error, so callers can assume result is not NULL.
 */
char *
clone_zero(const char *last_rsc_id)
{
    const char *end = pe_base_name_end(last_rsc_id);
    size_t base_name_len = end - last_rsc_id + 1;
    char *zero = NULL;

    CRM_ASSERT(end);
    zero = calloc(base_name_len + 3, sizeof(char));
    CRM_ASSERT(zero);
    memcpy(zero, last_rsc_id, base_name_len);
    zero[base_name_len] = ':';
    zero[base_name_len + 1] = '0';
    return zero;
}

static resource_t *
create_fake_resource(const char *rsc_id, xmlNode * rsc_entry, pe_working_set_t * data_set)
{
    resource_t *rsc = NULL;
    xmlNode *xml_rsc = create_xml_node(NULL, XML_CIB_TAG_RESOURCE);

    copy_in_properties(xml_rsc, rsc_entry);
    crm_xml_add(xml_rsc, XML_ATTR_ID, rsc_id);
    crm_log_xml_debug(xml_rsc, "Orphan resource");

    if (!common_unpack(xml_rsc, &rsc, NULL, data_set)) {
        return NULL;
    }

    if (xml_contains_remote_node(xml_rsc)) {
        node_t *node;

        crm_debug("Detected orphaned remote node %s", rsc_id);
        node = pe_find_node(data_set->nodes, rsc_id);
        if (node == NULL) {
	        node = pe_create_node(rsc_id, rsc_id, "remote", NULL, data_set);
        }
        link_rsc2remotenode(data_set, rsc);

        if (node) {
            crm_trace("Setting node %s as shutting down due to orphaned connection resource", rsc_id);
            node->details->shutdown = TRUE;
        }
    }

    if (crm_element_value(rsc_entry, XML_RSC_ATTR_CONTAINER)) {
        /* This orphaned rsc needs to be mapped to a container. */
        crm_trace("Detected orphaned container filler %s", rsc_id);
        set_bit(rsc->flags, pe_rsc_orphan_container_filler);
    }
    set_bit(rsc->flags, pe_rsc_orphan);
    data_set->resources = g_list_append(data_set->resources, rsc);
    return rsc;
}

/*!
 * \internal
 * \brief Create orphan instance for anonymous clone resource history
 */
static pe_resource_t *
create_anonymous_orphan(pe_resource_t *parent, const char *rsc_id,
                        pe_node_t *node, pe_working_set_t *data_set)
{
    pe_resource_t *top = pe__create_clone_child(parent, data_set);

    // find_rsc() because we might be a cloned group
    pe_resource_t *orphan = top->fns->find_rsc(top, rsc_id, NULL, pe_find_clone);

    pe_rsc_debug(parent, "Created orphan %s for %s: %s on %s",
                 top->id, parent->id, rsc_id, node->details->uname);
    return orphan;
}

/*!
 * \internal
 * \brief Check a node for an instance of an anonymous clone
 *
 * Return a child instance of the specified anonymous clone, in order of
 * preference: (1) the instance running on the specified node, if any;
 * (2) an inactive instance (i.e. within the total of clone-max instances);
 * (3) a newly created orphan (i.e. clone-max instances are already active).
 *
 * \param[in] data_set  Cluster information
 * \param[in] node      Node on which to check for instance
 * \param[in] parent    Clone to check
 * \param[in] rsc_id    Name of cloned resource in history (without instance)
 */
static resource_t *
find_anonymous_clone(pe_working_set_t * data_set, node_t * node, resource_t * parent,
                     const char *rsc_id)
{
    GListPtr rIter = NULL;
    pe_resource_t *rsc = NULL;
    pe_resource_t *inactive_instance = NULL;
    gboolean skip_inactive = FALSE;

    CRM_ASSERT(parent != NULL);
    CRM_ASSERT(pe_rsc_is_clone(parent));
    CRM_ASSERT(is_not_set(parent->flags, pe_rsc_unique));

    // Check for active (or partially active, for cloned groups) instance
    pe_rsc_trace(parent, "Looking for %s on %s in %s", rsc_id, node->details->uname, parent->id);
    for (rIter = parent->children; rsc == NULL && rIter; rIter = rIter->next) {
        GListPtr locations = NULL;
        resource_t *child = rIter->data;

        /* Check whether this instance is already known to be active or pending
         * anywhere, at this stage of unpacking. Because this function is called
         * for a resource before the resource's individual operation history
         * entries are unpacked, locations will generally not contain the
         * desired node.
         *
         * However, there are three exceptions:
         * (1) when child is a cloned group and we have already unpacked the
         *     history of another member of the group on the same node;
         * (2) when we've already unpacked the history of another numbered
         *     instance on the same node (which can happen if globally-unique
         *     was flipped from true to false); and
         * (3) when we re-run calculations on the same data set as part of a
         *     simulation.
         */
        child->fns->location(child, &locations, 2);
        if (locations) {
            /* We should never associate the same numbered anonymous clone
             * instance with multiple nodes, and clone instances can't migrate,
             * so there must be only one location, regardless of history.
             */
            CRM_LOG_ASSERT(locations->next == NULL);

            if (((pe_node_t *)locations->data)->details == node->details) {
                /* This child instance is active on the requested node, so check
                 * for a corresponding configured resource. We use find_rsc()
                 * instead of child because child may be a cloned group, and we
                 * need the particular member corresponding to rsc_id.
                 *
                 * If the history entry is orphaned, rsc will be NULL.
                 */
                rsc = parent->fns->find_rsc(child, rsc_id, NULL, pe_find_clone);
                if (rsc) {
                    /* If there are multiple instance history entries for an
                     * anonymous clone in a single node's history (which can
                     * happen if globally-unique is switched from true to
                     * false), we want to consider the instances beyond the
                     * first as orphans, even if there are inactive instance
                     * numbers available.
                     */
                    if (rsc->running_on) {
                        crm_notice("Active (now-)anonymous clone %s has "
                                   "multiple (orphan) instance histories on %s",
                                   parent->id, node->details->uname);
                        skip_inactive = TRUE;
                        rsc = NULL;
                    } else {
                        pe_rsc_trace(parent, "Resource %s, active", rsc->id);
                    }
                }
            }
            g_list_free(locations);

        } else {
            pe_rsc_trace(parent, "Resource %s, skip inactive", child->id);
            if (!skip_inactive && !inactive_instance
                && is_not_set(child->flags, pe_rsc_block)) {
                // Remember one inactive instance in case we don't find active
                inactive_instance = parent->fns->find_rsc(child, rsc_id, NULL,
                                                          pe_find_clone);

                /* ... but don't use it if it was already associated with a
                 * pending action on another node
                 */
                if (inactive_instance && inactive_instance->pending_node
                    && (inactive_instance->pending_node->details != node->details)) {
                    inactive_instance = NULL;
                }
            }
        }
    }

    if ((rsc == NULL) && !skip_inactive && (inactive_instance != NULL)) {
        pe_rsc_trace(parent, "Resource %s, empty slot", inactive_instance->id);
        rsc = inactive_instance;
    }

    /* If the resource has "requires" set to "quorum" or "nothing", and we don't
     * have a clone instance for every node, we don't want to consume a valid
     * instance number for unclean nodes. Such instances may appear to be active
     * according to the history, but should be considered inactive, so we can
     * start an instance elsewhere. Treat such instances as orphans.
     *
     * An exception is instances running on guest nodes -- since guest node
     * "fencing" is actually just a resource stop, requires shouldn't apply.
     *
     * @TODO Ideally, we'd use an inactive instance number if it is not needed
     * for any clean instances. However, we don't know that at this point.
     */
    if ((rsc != NULL) && is_not_set(rsc->flags, pe_rsc_needs_fencing)
        && (!node->details->online || node->details->unclean)
        && !pe__is_guest_node(node)
        && !pe__is_universal_clone(parent, data_set)) {

        rsc = NULL;
    }

    if (rsc == NULL) {
        rsc = create_anonymous_orphan(parent, rsc_id, node, data_set);
        pe_rsc_trace(parent, "Resource %s, orphan", rsc->id);
    }
    return rsc;
}

static resource_t *
unpack_find_resource(pe_working_set_t * data_set, node_t * node, const char *rsc_id,
                     xmlNode * rsc_entry)
{
    resource_t *rsc = NULL;
    resource_t *parent = NULL;

    crm_trace("looking for %s", rsc_id);
    rsc = pe_find_resource(data_set->resources, rsc_id);

    if (rsc == NULL) {
        /* If we didn't find the resource by its name in the operation history,
         * check it again as a clone instance. Even when clone-max=0, we create
         * a single :0 orphan to match against here.
         */
        char *clone0_id = clone_zero(rsc_id);
        resource_t *clone0 = pe_find_resource(data_set->resources, clone0_id);

        if (clone0 && is_not_set(clone0->flags, pe_rsc_unique)) {
            rsc = clone0;
            parent = uber_parent(clone0);
            crm_trace("%s found as %s (%s)", rsc_id, clone0_id, parent->id);
        } else {
            crm_trace("%s is not known as %s either (orphan)",
                      rsc_id, clone0_id);
        }
        free(clone0_id);

    } else if (rsc->variant > pe_native) {
        crm_trace("Resource history for %s is orphaned because it is no longer primitive",
                  rsc_id);
        return NULL;

    } else {
        parent = uber_parent(rsc);
    }

    if (pe_rsc_is_anon_clone(parent)) {

        if (pe_rsc_is_bundled(parent)) {
            rsc = pe__find_bundle_replica(parent->parent, node);
        } else {
            char *base = clone_strip(rsc_id);

            rsc = find_anonymous_clone(data_set, node, parent, base);
            free(base);
            CRM_ASSERT(rsc != NULL);
        }
    }

    if (rsc && safe_str_neq(rsc_id, rsc->id)
        && safe_str_neq(rsc_id, rsc->clone_name)) {

        free(rsc->clone_name);
        rsc->clone_name = strdup(rsc_id);
        pe_rsc_debug(rsc, "Internally renamed %s on %s to %s%s",
                     rsc_id, node->details->uname, rsc->id,
                     (is_set(rsc->flags, pe_rsc_orphan)? " (ORPHAN)" : ""));
    }
    return rsc;
}

static resource_t *
process_orphan_resource(xmlNode * rsc_entry, node_t * node, pe_working_set_t * data_set)
{
    resource_t *rsc = NULL;
    const char *rsc_id = crm_element_value(rsc_entry, XML_ATTR_ID);

    crm_debug("Detected orphan resource %s on %s", rsc_id, node->details->uname);
    rsc = create_fake_resource(rsc_id, rsc_entry, data_set);

    if (is_set(data_set->flags, pe_flag_stop_rsc_orphans) == FALSE) {
        clear_bit(rsc->flags, pe_rsc_managed);

    } else {
        print_resource(LOG_TRACE, "Added orphan", rsc, FALSE);

        CRM_CHECK(rsc != NULL, return NULL);
        resource_location(rsc, NULL, -INFINITY, "__orphan_do_not_run__", data_set);
    }
    return rsc;
}

static void
process_rsc_state(resource_t * rsc, node_t * node,
                  enum action_fail_response on_fail,
                  xmlNode * migrate_op, pe_working_set_t * data_set)
{
    node_t *tmpnode = NULL;
    char *reason = NULL;

    CRM_ASSERT(rsc);
    pe_rsc_trace(rsc, "Resource %s is %s on %s: on_fail=%s",
                 rsc->id, role2text(rsc->role), node->details->uname, fail2text(on_fail));

    /* process current state */
    if (rsc->role != RSC_ROLE_UNKNOWN) {
        resource_t *iter = rsc;

        while (iter) {
            if (g_hash_table_lookup(iter->known_on, node->details->id) == NULL) {
                node_t *n = node_copy(node);

                pe_rsc_trace(rsc, "%s (aka. %s) known on %s", rsc->id, rsc->clone_name,
                             n->details->uname);
                g_hash_table_insert(iter->known_on, (gpointer) n->details->id, n);
            }
            if (is_set(iter->flags, pe_rsc_unique)) {
                break;
            }
            iter = iter->parent;
        }
    }

    /* If a managed resource is believed to be running, but node is down ... */
    if (rsc->role > RSC_ROLE_STOPPED
        && node->details->online == FALSE
        && node->details->maintenance == FALSE
        && is_set(rsc->flags, pe_rsc_managed)) {

        gboolean should_fence = FALSE;

        /* If this is a guest node, fence it (regardless of whether fencing is
         * enabled, because guest node fencing is done by recovery of the
         * container resource rather than by the fencer). Mark the resource
         * we're processing as failed. When the guest comes back up, its
         * operation history in the CIB will be cleared, freeing the affected
         * resource to run again once we are sure we know its state.
         */
        if (pe__is_guest_node(node)) {
            set_bit(rsc->flags, pe_rsc_failed);
            should_fence = TRUE;

        } else if (is_set(data_set->flags, pe_flag_stonith_enabled)) {
            if (pe__is_remote_node(node) && node->details->remote_rsc
                && is_not_set(node->details->remote_rsc->flags, pe_rsc_failed)) {

                /* Setting unseen means that fencing of the remote node will
                 * occur only if the connection resource is not going to start
                 * somewhere. This allows connection resources on a failed
                 * cluster node to move to another node without requiring the
                 * remote nodes to be fenced as well.
                 */
                node->details->unseen = TRUE;
                reason = crm_strdup_printf("%s is active there (fencing will be"
                                           " revoked if remote connection can "
                                           "be re-established elsewhere)",
                                           rsc->id);
            }
            should_fence = TRUE;
        }

        if (should_fence) {
            if (reason == NULL) {
               reason = crm_strdup_printf("%s is thought to be active there", rsc->id);
            }
            pe_fence_node(data_set, node, reason);
        }
        free(reason);
    }

    if (node->details->unclean) {
        /* No extra processing needed
         * Also allows resources to be started again after a node is shot
         */
        on_fail = action_fail_ignore;
    }

    switch (on_fail) {
        case action_fail_ignore:
            /* nothing to do */
            break;

        case action_fail_fence:
            /* treat it as if it is still running
             * but also mark the node as unclean
             */
            reason = crm_strdup_printf("%s failed there", rsc->id);
            pe_fence_node(data_set, node, reason);
            free(reason);
            break;

        case action_fail_standby:
            node->details->standby = TRUE;
            node->details->standby_onfail = TRUE;
            break;

        case action_fail_block:
            /* is_managed == FALSE will prevent any
             * actions being sent for the resource
             */
            clear_bit(rsc->flags, pe_rsc_managed);
            set_bit(rsc->flags, pe_rsc_block);
            break;

        case action_fail_migrate:
            /* make sure it comes up somewhere else
             * or not at all
             */
            resource_location(rsc, node, -INFINITY, "__action_migration_auto__", data_set);
            break;

        case action_fail_stop:
            rsc->next_role = RSC_ROLE_STOPPED;
            break;

        case action_fail_recover:
            if (rsc->role != RSC_ROLE_STOPPED && rsc->role != RSC_ROLE_UNKNOWN) {
                set_bit(rsc->flags, pe_rsc_failed);
                stop_action(rsc, node, FALSE);
            }
            break;

        case action_fail_restart_container:
            set_bit(rsc->flags, pe_rsc_failed);

            if (rsc->container && pe_rsc_is_bundled(rsc)) {
                /* A bundle's remote connection can run on a different node than
                 * the bundle's container. We don't necessarily know where the
                 * container is running yet, so remember it and add a stop
                 * action for it later.
                 */
                data_set->stop_needed = g_list_prepend(data_set->stop_needed,
                                                       rsc->container);
            } else if (rsc->container) {
                stop_action(rsc->container, node, FALSE);
            } else if (rsc->role != RSC_ROLE_STOPPED && rsc->role != RSC_ROLE_UNKNOWN) {
                stop_action(rsc, node, FALSE);
            }
            break;

        case action_fail_reset_remote:
            set_bit(rsc->flags, pe_rsc_failed);
            if (is_set(data_set->flags, pe_flag_stonith_enabled)) {
                tmpnode = NULL;
                if (rsc->is_remote_node) {
                    tmpnode = pe_find_node(data_set->nodes, rsc->id);
                }
                if (tmpnode &&
                    pe__is_remote_node(tmpnode) &&
                    tmpnode->details->remote_was_fenced == 0) {

                    /* The remote connection resource failed in a way that
                     * should result in fencing the remote node.
                     */
                    pe_fence_node(data_set, tmpnode,
                                  "remote connection is unrecoverable");
                }
            }

            /* require the stop action regardless if fencing is occurring or not. */
            if (rsc->role > RSC_ROLE_STOPPED) {
                stop_action(rsc, node, FALSE);
            }

            /* if reconnect delay is in use, prevent the connection from exiting the
             * "STOPPED" role until the failure is cleared by the delay timeout. */
            if (rsc->remote_reconnect_ms) {
                rsc->next_role = RSC_ROLE_STOPPED;
            }
            break;
    }

    /* ensure a remote-node connection failure forces an unclean remote-node
     * to be fenced. By setting unseen = FALSE, the remote-node failure will
     * result in a fencing operation regardless if we're going to attempt to 
     * reconnect to the remote-node in this transition or not. */
    if (is_set(rsc->flags, pe_rsc_failed) && rsc->is_remote_node) {
        tmpnode = pe_find_node(data_set->nodes, rsc->id);
        if (tmpnode && tmpnode->details->unclean) {
            tmpnode->details->unseen = FALSE;
        }
    }

    if (rsc->role != RSC_ROLE_STOPPED && rsc->role != RSC_ROLE_UNKNOWN) {
        if (is_set(rsc->flags, pe_rsc_orphan)) {
            if (is_set(rsc->flags, pe_rsc_managed)) {
                crm_config_warn("Detected active orphan %s running on %s",
                                rsc->id, node->details->uname);
            } else {
                crm_config_warn("Cluster configured not to stop active orphans."
                                " %s must be stopped manually on %s",
                                rsc->id, node->details->uname);
            }
        }

        native_add_running(rsc, node, data_set);
        if (on_fail != action_fail_ignore) {
            set_bit(rsc->flags, pe_rsc_failed);
        }

    } else if (rsc->clone_name && strchr(rsc->clone_name, ':') != NULL) {
        /* Only do this for older status sections that included instance numbers
         * Otherwise stopped instances will appear as orphans
         */
        pe_rsc_trace(rsc, "Resetting clone_name %s for %s (stopped)", rsc->clone_name, rsc->id);
        free(rsc->clone_name);
        rsc->clone_name = NULL;

    } else {
        GList *possible_matches = pe__resource_actions(rsc, node, RSC_STOP,
                                                       FALSE);
        GListPtr gIter = possible_matches;

        for (; gIter != NULL; gIter = gIter->next) {
            action_t *stop = (action_t *) gIter->data;

            stop->flags |= pe_action_optional;
        }

        g_list_free(possible_matches);
    }
}

/* create active recurring operations as optional */
static void
process_recurring(node_t * node, resource_t * rsc,
                  int start_index, int stop_index,
                  GListPtr sorted_op_list, pe_working_set_t * data_set)
{
    int counter = -1;
    const char *task = NULL;
    const char *status = NULL;
    GListPtr gIter = sorted_op_list;

    CRM_ASSERT(rsc);
    pe_rsc_trace(rsc, "%s: Start index %d, stop index = %d", rsc->id, start_index, stop_index);

    for (; gIter != NULL; gIter = gIter->next) {
        xmlNode *rsc_op = (xmlNode *) gIter->data;

        guint interval_ms = 0;
        char *key = NULL;
        const char *id = ID(rsc_op);
        const char *interval_ms_s = NULL;

        counter++;

        if (node->details->online == FALSE) {
            pe_rsc_trace(rsc, "Skipping %s/%s: node is offline", rsc->id, node->details->uname);
            break;

            /* Need to check if there's a monitor for role="Stopped" */
        } else if (start_index < stop_index && counter <= stop_index) {
            pe_rsc_trace(rsc, "Skipping %s/%s: resource is not active", id, node->details->uname);
            continue;

        } else if (counter < start_index) {
            pe_rsc_trace(rsc, "Skipping %s/%s: old %d", id, node->details->uname, counter);
            continue;
        }

        interval_ms_s = crm_element_value(rsc_op, XML_LRM_ATTR_INTERVAL_MS);
        interval_ms = crm_parse_ms(interval_ms_s);
        if (interval_ms == 0) {
            pe_rsc_trace(rsc, "Skipping %s/%s: non-recurring", id, node->details->uname);
            continue;
        }

        status = crm_element_value(rsc_op, XML_LRM_ATTR_OPSTATUS);
        if (safe_str_eq(status, "-1")) {
            pe_rsc_trace(rsc, "Skipping %s/%s: status", id, node->details->uname);
            continue;
        }
        task = crm_element_value(rsc_op, XML_LRM_ATTR_TASK);
        /* create the action */
        key = generate_op_key(rsc->id, task, interval_ms);
        pe_rsc_trace(rsc, "Creating %s/%s", key, node->details->uname);
        custom_action(rsc, key, task, node, TRUE, TRUE, data_set);
    }
}

void
calculate_active_ops(GListPtr sorted_op_list, int *start_index, int *stop_index)
{
    int counter = -1;
    int implied_monitor_start = -1;
    int implied_clone_start = -1;
    const char *task = NULL;
    const char *status = NULL;
    GListPtr gIter = sorted_op_list;

    *stop_index = -1;
    *start_index = -1;

    for (; gIter != NULL; gIter = gIter->next) {
        xmlNode *rsc_op = (xmlNode *) gIter->data;

        counter++;

        task = crm_element_value(rsc_op, XML_LRM_ATTR_TASK);
        status = crm_element_value(rsc_op, XML_LRM_ATTR_OPSTATUS);

        if (safe_str_eq(task, CRMD_ACTION_STOP)
            && safe_str_eq(status, "0")) {
            *stop_index = counter;

        } else if (safe_str_eq(task, CRMD_ACTION_START) || safe_str_eq(task, CRMD_ACTION_MIGRATED)) {
            *start_index = counter;

        } else if ((implied_monitor_start <= *stop_index) && safe_str_eq(task, CRMD_ACTION_STATUS)) {
            const char *rc = crm_element_value(rsc_op, XML_LRM_ATTR_RC);

            if (safe_str_eq(rc, "0") || safe_str_eq(rc, "8")) {
                implied_monitor_start = counter;
            }
        } else if (safe_str_eq(task, CRMD_ACTION_PROMOTE) || safe_str_eq(task, CRMD_ACTION_DEMOTE)) {
            implied_clone_start = counter;
        }
    }

    if (*start_index == -1) {
        if (implied_clone_start != -1) {
            *start_index = implied_clone_start;
        } else if (implied_monitor_start != -1) {
            *start_index = implied_monitor_start;
        }
    }
}

static resource_t *
unpack_lrm_rsc_state(node_t * node, xmlNode * rsc_entry, pe_working_set_t * data_set)
{
    GListPtr gIter = NULL;
    int stop_index = -1;
    int start_index = -1;
    enum rsc_role_e req_role = RSC_ROLE_UNKNOWN;

    const char *task = NULL;
    const char *rsc_id = crm_element_value(rsc_entry, XML_ATTR_ID);

    resource_t *rsc = NULL;
    GListPtr op_list = NULL;
    GListPtr sorted_op_list = NULL;

    xmlNode *migrate_op = NULL;
    xmlNode *rsc_op = NULL;
    xmlNode *last_failure = NULL;

    enum action_fail_response on_fail = FALSE;
    enum rsc_role_e saved_role = RSC_ROLE_UNKNOWN;

    crm_trace("[%s] Processing %s on %s",
              crm_element_name(rsc_entry), rsc_id, node->details->uname);

    /* extract operations */
    op_list = NULL;
    sorted_op_list = NULL;

    for (rsc_op = __xml_first_child_element(rsc_entry); rsc_op != NULL;
         rsc_op = __xml_next_element(rsc_op)) {
        if (crm_str_eq((const char *)rsc_op->name, XML_LRM_TAG_RSC_OP, TRUE)) {
            op_list = g_list_prepend(op_list, rsc_op);
        }
    }

    if (op_list == NULL) {
        /* if there are no operations, there is nothing to do */
        return NULL;
    }

    /* find the resource */
    rsc = unpack_find_resource(data_set, node, rsc_id, rsc_entry);
    if (rsc == NULL) {
        rsc = process_orphan_resource(rsc_entry, node, data_set);
    }
    CRM_ASSERT(rsc != NULL);

    /* process operations */
    saved_role = rsc->role;
    on_fail = action_fail_ignore;
    rsc->role = RSC_ROLE_UNKNOWN;
    sorted_op_list = g_list_sort(op_list, sort_op_by_callid);

    for (gIter = sorted_op_list; gIter != NULL; gIter = gIter->next) {
        xmlNode *rsc_op = (xmlNode *) gIter->data;

        task = crm_element_value(rsc_op, XML_LRM_ATTR_TASK);
        if (safe_str_eq(task, CRMD_ACTION_MIGRATED)) {
            migrate_op = rsc_op;
        }

        unpack_rsc_op(rsc, node, rsc_op, &last_failure, &on_fail, data_set);
    }

    /* create active recurring operations as optional */
    calculate_active_ops(sorted_op_list, &start_index, &stop_index);
    process_recurring(node, rsc, start_index, stop_index, sorted_op_list, data_set);

    /* no need to free the contents */
    g_list_free(sorted_op_list);

    process_rsc_state(rsc, node, on_fail, migrate_op, data_set);

    if (get_target_role(rsc, &req_role)) {
        if (rsc->next_role == RSC_ROLE_UNKNOWN || req_role < rsc->next_role) {
            pe_rsc_debug(rsc, "%s: Overwriting calculated next role %s"
                         " with requested next role %s",
                         rsc->id, role2text(rsc->next_role), role2text(req_role));
            rsc->next_role = req_role;

        } else if (req_role > rsc->next_role) {
            pe_rsc_info(rsc, "%s: Not overwriting calculated next role %s"
                        " with requested next role %s",
                        rsc->id, role2text(rsc->next_role), role2text(req_role));
        }
    }

    if (saved_role > rsc->role) {
        rsc->role = saved_role;
    }

    return rsc;
}

static void
handle_orphaned_container_fillers(xmlNode * lrm_rsc_list, pe_working_set_t * data_set)
{
    xmlNode *rsc_entry = NULL;
    for (rsc_entry = __xml_first_child_element(lrm_rsc_list); rsc_entry != NULL;
         rsc_entry = __xml_next_element(rsc_entry)) {

        resource_t *rsc;
        resource_t *container;
        const char *rsc_id;
        const char *container_id;

        if (safe_str_neq((const char *)rsc_entry->name, XML_LRM_TAG_RESOURCE)) {
            continue;
        }

        container_id = crm_element_value(rsc_entry, XML_RSC_ATTR_CONTAINER);
        rsc_id = crm_element_value(rsc_entry, XML_ATTR_ID);
        if (container_id == NULL || rsc_id == NULL) {
            continue;
        }

        container = pe_find_resource(data_set->resources, container_id);
        if (container == NULL) {
            continue;
        }

        rsc = pe_find_resource(data_set->resources, rsc_id);
        if (rsc == NULL ||
            is_set(rsc->flags, pe_rsc_orphan_container_filler) == FALSE ||
            rsc->container != NULL) {
            continue;
        }

        pe_rsc_trace(rsc, "Mapped container of orphaned resource %s to %s",
                     rsc->id, container_id);
        rsc->container = container;
        container->fillers = g_list_append(container->fillers, rsc);
    }
}

gboolean
unpack_lrm_resources(node_t * node, xmlNode * lrm_rsc_list, pe_working_set_t * data_set)
{
    xmlNode *rsc_entry = NULL;
    gboolean found_orphaned_container_filler = FALSE;

    CRM_CHECK(node != NULL, return FALSE);

    crm_trace("Unpacking resources on %s", node->details->uname);

    for (rsc_entry = __xml_first_child_element(lrm_rsc_list); rsc_entry != NULL;
         rsc_entry = __xml_next_element(rsc_entry)) {

        if (crm_str_eq((const char *)rsc_entry->name, XML_LRM_TAG_RESOURCE, TRUE)) {
            resource_t *rsc = unpack_lrm_rsc_state(node, rsc_entry, data_set);
            if (!rsc) {
                continue;
            }
            if (is_set(rsc->flags, pe_rsc_orphan_container_filler)) {
                found_orphaned_container_filler = TRUE;
            }
        }
    }

    /* now that all the resource state has been unpacked for this node
     * we have to go back and map any orphaned container fillers to their
     * container resource */
    if (found_orphaned_container_filler) {
        handle_orphaned_container_fillers(lrm_rsc_list, data_set);
    }
    return TRUE;
}

static void
set_active(resource_t * rsc)
{
    resource_t *top = uber_parent(rsc);

    if (top && is_set(top->flags, pe_rsc_promotable)) {
        rsc->role = RSC_ROLE_SLAVE;
    } else {
        rsc->role = RSC_ROLE_STARTED;
    }
}

static void
set_node_score(gpointer key, gpointer value, gpointer user_data)
{
    node_t *node = value;
    int *score = user_data;

    node->weight = *score;
}

#define STATUS_PATH_MAX 1024
static xmlNode *
find_lrm_op(const char *resource, const char *op, const char *node, const char *source,
            bool success_only, pe_working_set_t *data_set)
{
    int offset = 0;
    char xpath[STATUS_PATH_MAX];
    xmlNode *xml = NULL;

    offset += snprintf(xpath + offset, STATUS_PATH_MAX - offset, "//node_state[@uname='%s']", node);
    offset +=
        snprintf(xpath + offset, STATUS_PATH_MAX - offset, "//" XML_LRM_TAG_RESOURCE "[@id='%s']",
                 resource);

    /* Need to check against transition_magic too? */
    if (source && safe_str_eq(op, CRMD_ACTION_MIGRATE)) {
        offset +=
            snprintf(xpath + offset, STATUS_PATH_MAX - offset,
                     "/" XML_LRM_TAG_RSC_OP "[@operation='%s' and @migrate_target='%s']", op,
                     source);
    } else if (source && safe_str_eq(op, CRMD_ACTION_MIGRATED)) {
        offset +=
            snprintf(xpath + offset, STATUS_PATH_MAX - offset,
                     "/" XML_LRM_TAG_RSC_OP "[@operation='%s' and @migrate_source='%s']", op,
                     source);
    } else {
        offset +=
            snprintf(xpath + offset, STATUS_PATH_MAX - offset,
                     "/" XML_LRM_TAG_RSC_OP "[@operation='%s']", op);
    }

    CRM_LOG_ASSERT(offset > 0);
    xml = get_xpath_object(xpath, data_set->input, LOG_DEBUG);

    if (xml && success_only) {
        int rc = PCMK_OCF_UNKNOWN_ERROR;
        int status = PCMK_LRM_OP_ERROR;

        crm_element_value_int(xml, XML_LRM_ATTR_RC, &rc);
        crm_element_value_int(xml, XML_LRM_ATTR_OPSTATUS, &status);
        if ((rc != PCMK_OCF_OK) || (status != PCMK_LRM_OP_DONE)) {
            return NULL;
        }
    }
    return xml;
}

static int
pe__call_id(xmlNode *op_xml)
{
    int id = 0;

    if (op_xml) {
        crm_element_value_int(op_xml, XML_LRM_ATTR_CALLID, &id);
    }
    return id;
}

/*!
 * \brief Check whether a stop happened on the same node after some event
 *
 * \param[in] rsc       Resource being checked
 * \param[in] node      Node being checked
 * \param[in] xml_op    Event that stop is being compared to
 * \param[in] data_set  Cluster working set
 *
 * \return TRUE if stop happened after event, FALSE otherwise
 *
 * \note This is really unnecessary, but kept as a safety mechanism. We
 *       currently don't save more than one successful event in history, so this
 *       only matters when processing really old CIB files that we don't
 *       technically support anymore, or as preparation for logging an extended
 *       history in the future.
 */
static bool
stop_happened_after(pe_resource_t *rsc, pe_node_t *node, xmlNode *xml_op,
                    pe_working_set_t *data_set)
{
    xmlNode *stop_op = find_lrm_op(rsc->id, CRMD_ACTION_STOP,
                                   node->details->uname, NULL, TRUE, data_set);

    return (stop_op && (pe__call_id(stop_op) > pe__call_id(xml_op)));
}

static void
unpack_migrate_to_success(pe_resource_t *rsc, pe_node_t *node, xmlNode *xml_op,
                          pe_working_set_t *data_set)
{
    /* A successful migration sequence is:
     *    migrate_to on source node
     *    migrate_from on target node
     *    stop on source node
     *
     * If a migrate_to is followed by a stop, the entire migration (successful
     * or failed) is complete, and we don't care what happened on the target.
     *
     * If no migrate_from has happened, the migration is considered to be
     * "partial". If the migrate_from failed, make sure the resource gets
     * stopped on both source and target (if up).
     *
     * If the migrate_to and migrate_from both succeeded (which also implies the
     * resource is no longer running on the source), but there is no stop, the
     * migration is considered to be "dangling". Schedule a stop on the source
     * in this case.
     */
    int from_rc = 0;
    int from_status = 0;
    pe_node_t *target_node = NULL;
    pe_node_t *source_node = NULL;
    xmlNode *migrate_from = NULL;
    const char *source = crm_element_value(xml_op, XML_LRM_ATTR_MIGRATE_SOURCE);
    const char *target = crm_element_value(xml_op, XML_LRM_ATTR_MIGRATE_TARGET);

    // Sanity check
    CRM_CHECK(source && target && !strcmp(source, node->details->uname), return);

    if (stop_happened_after(rsc, node, xml_op, data_set)) {
        return;
    }

    // Clones are not allowed to migrate, so role can't be master
    rsc->role = RSC_ROLE_STARTED;

    target_node = pe_find_node(data_set->nodes, target);
    source_node = pe_find_node(data_set->nodes, source);

    // Check whether there was a migrate_from action on the target
    migrate_from = find_lrm_op(rsc->id, CRMD_ACTION_MIGRATED, target,
                               source, FALSE, data_set);
    if (migrate_from) {
        crm_element_value_int(migrate_from, XML_LRM_ATTR_RC, &from_rc);
        crm_element_value_int(migrate_from, XML_LRM_ATTR_OPSTATUS, &from_status);
        pe_rsc_trace(rsc, "%s op on %s exited with status=%d, rc=%d",
                     ID(migrate_from), target, from_status, from_rc);
    }

    if (migrate_from && from_rc == PCMK_OCF_OK
        && from_status == PCMK_LRM_OP_DONE) {
        /* The migrate_to and migrate_from both succeeded, so mark the migration
         * as "dangling". This will be used to schedule a stop action on the
         * source without affecting the target.
         */
        pe_rsc_trace(rsc, "Detected dangling migration op: %s on %s", ID(xml_op),
                     source);
        rsc->role = RSC_ROLE_STOPPED;
        rsc->dangling_migrations = g_list_prepend(rsc->dangling_migrations, node);

    } else if (migrate_from && (from_status != PCMK_LRM_OP_PENDING)) { // Failed
        if (target_node && target_node->details->online) {
            pe_rsc_trace(rsc, "Marking active on %s %p %d", target, target_node,
                         target_node->details->online);
            native_add_running(rsc, target_node, data_set);
        }

    } else { // Pending, or complete but erased
        if (target_node && target_node->details->online) {
            pe_rsc_trace(rsc, "Marking active on %s %p %d", target, target_node,
                         target_node->details->online);

            native_add_running(rsc, target_node, data_set);
            if (source_node && source_node->details->online) {
                /* This is a partial migration: the migrate_to completed
                 * successfully on the source, but the migrate_from has not
                 * completed. Remember the source and target; if the newly
                 * chosen target remains the same when we schedule actions
                 * later, we may continue with the migration.
                 */
                rsc->partial_migration_target = target_node;
                rsc->partial_migration_source = source_node;
            }
        } else {
            /* Consider it failed here - forces a restart, prevents migration */
            set_bit(rsc->flags, pe_rsc_failed);
            clear_bit(rsc->flags, pe_rsc_allow_migrate);
        }
    }
}

static void
unpack_migrate_to_failure(pe_resource_t *rsc, pe_node_t *node, xmlNode *xml_op,
                          pe_working_set_t *data_set)
{
    int target_stop_id = 0;
    int target_migrate_from_id = 0;
    xmlNode *target_stop = NULL;
    xmlNode *target_migrate_from = NULL;
    const char *source = crm_element_value(xml_op, XML_LRM_ATTR_MIGRATE_SOURCE);
    const char *target = crm_element_value(xml_op, XML_LRM_ATTR_MIGRATE_TARGET);

    // Sanity check
    CRM_CHECK(source && target && !strcmp(source, node->details->uname), return);

    /* If a migration failed, we have to assume the resource is active. Clones
     * are not allowed to migrate, so role can't be master.
     */
    rsc->role = RSC_ROLE_STARTED;

    // Check for stop on the target
    target_stop = find_lrm_op(rsc->id, CRMD_ACTION_STOP, target, NULL,
                              TRUE, data_set);
    target_stop_id = pe__call_id(target_stop);

    // Check for migrate_from on the target
    target_migrate_from = find_lrm_op(rsc->id, CRMD_ACTION_MIGRATED, target,
                                      source, TRUE, data_set);
    target_migrate_from_id = pe__call_id(target_migrate_from);

    if ((target_stop == NULL) || (target_stop_id < target_migrate_from_id)) {
        /* There was no stop on the source, or a stop that happened before a
         * migrate_from, so assume the resource is still active on the target
         * (if it is up).
         */
        node_t *target_node = pe_find_node(data_set->nodes, target);

        pe_rsc_trace(rsc, "stop (%d) + migrate_from (%d)",
                     target_stop_id, target_migrate_from_id);
        if (target_node && target_node->details->online) {
            native_add_running(rsc, target_node, data_set);
        }

    } else if (target_migrate_from == NULL) {
        /* We know there was a stop on the target, but there may not have been a
         * migrate_from (the stop could have happened before migrate_from was
         * scheduled or attempted).
         *
         * That means this could be a "dangling" migration. But first, check
         * whether there is a newer migrate_from or start on the source node --
         * it's possible the failed migration was followed by a successful
         * full restart or migration in the reverse direction, in which case we
         * don't want to force it to stop.
         */
        xmlNode *source_migrate_from = NULL;
        xmlNode *source_start = NULL;
        int source_migrate_to_id = pe__call_id(xml_op);

        source_migrate_from = find_lrm_op(rsc->id, CRMD_ACTION_MIGRATED, source,
                                          NULL, TRUE, data_set);
        if (pe__call_id(source_migrate_from) > source_migrate_to_id) {
            return;
        }

        source_start = find_lrm_op(rsc->id, CRMD_ACTION_START, source, NULL,
                                   TRUE, data_set);
        if (pe__call_id(source_start) > source_migrate_to_id) {
            return;
        }

        // Mark node as having dangling migration so we can force a stop later
        rsc->dangling_migrations = g_list_prepend(rsc->dangling_migrations, node);
    }
}

static void
unpack_migrate_from_failure(pe_resource_t *rsc, pe_node_t *node,
                            xmlNode *xml_op, pe_working_set_t *data_set)
{
    xmlNode *source_stop = NULL;
    xmlNode *source_migrate_to = NULL;
    const char *source = crm_element_value(xml_op, XML_LRM_ATTR_MIGRATE_SOURCE);
    const char *target = crm_element_value(xml_op, XML_LRM_ATTR_MIGRATE_TARGET);

    // Sanity check
    CRM_CHECK(source && target && !strcmp(target, node->details->uname), return);

    /* If a migration failed, we have to assume the resource is active. Clones
     * are not allowed to migrate, so role can't be master.
     */
    rsc->role = RSC_ROLE_STARTED;

    // Check for a stop on the source
    source_stop = find_lrm_op(rsc->id, CRMD_ACTION_STOP, source, NULL,
                              TRUE, data_set);

    // Check for a migrate_to on the source
    source_migrate_to = find_lrm_op(rsc->id, CRMD_ACTION_MIGRATE,
                                    source, target, TRUE, data_set);

    if ((source_stop == NULL)
        || (pe__call_id(source_stop) < pe__call_id(source_migrate_to))) {
        /* There was no stop on the source, or a stop that happened before
         * migrate_to, so assume the resource is still active on the source (if
         * it is up).
         */
        pe_node_t *source_node = pe_find_node(data_set->nodes, source);

        if (source_node && source_node->details->online) {
            native_add_running(rsc, source_node, data_set);
        }
    }
}

static void
record_failed_op(xmlNode *op, const pe_node_t *node,
                 const pe_resource_t *rsc, pe_working_set_t *data_set)
{
    xmlNode *xIter = NULL;
    const char *op_key = crm_element_value(op, XML_LRM_ATTR_TASK_KEY);

    if (node->details->online == FALSE) {
        return;
    }

    for (xIter = data_set->failed->children; xIter; xIter = xIter->next) {
        const char *key = crm_element_value(xIter, XML_LRM_ATTR_TASK_KEY);
        const char *uname = crm_element_value(xIter, XML_ATTR_UNAME);

        if(safe_str_eq(op_key, key) && safe_str_eq(uname, node->details->uname)) {
            crm_trace("Skipping duplicate entry %s on %s", op_key, node->details->uname);
            return;
        }
    }

    crm_trace("Adding entry %s on %s", op_key, node->details->uname);
    crm_xml_add(op, XML_ATTR_UNAME, node->details->uname);
    crm_xml_add(op, XML_LRM_ATTR_RSCID, rsc->id);
    add_node_copy(data_set->failed, op);
}

static const char *get_op_key(xmlNode *xml_op)
{
    const char *key = crm_element_value(xml_op, XML_LRM_ATTR_TASK_KEY);
    if(key == NULL) {
        key = ID(xml_op);
    }
    return key;
}

static void
unpack_rsc_op_failure(resource_t * rsc, node_t * node, int rc, xmlNode * xml_op, xmlNode ** last_failure,
                      enum action_fail_response * on_fail, pe_working_set_t * data_set)
{
    guint interval_ms = 0;
    bool is_probe = FALSE;
    action_t *action = NULL;

    const char *key = get_op_key(xml_op);
    const char *task = crm_element_value(xml_op, XML_LRM_ATTR_TASK);

    CRM_ASSERT(rsc);

    *last_failure = xml_op;

    crm_element_value_ms(xml_op, XML_LRM_ATTR_INTERVAL_MS, &interval_ms);
    if ((interval_ms == 0) && safe_str_eq(task, CRMD_ACTION_STATUS)) {
        is_probe = TRUE;
        pe_rsc_trace(rsc, "is a probe: %s", key);
    }

    if (rc != PCMK_OCF_NOT_INSTALLED || is_set(data_set->flags, pe_flag_symmetric_cluster)) {
        crm_warn("Processing failed %s of %s on %s: %s " CRM_XS " rc=%d",
                 (is_probe? "probe" : task), rsc->id, node->details->uname,
                 services_ocf_exitcode_str(rc), rc);

        if (is_probe && (rc != PCMK_OCF_OK)
            && (rc != PCMK_OCF_NOT_RUNNING)
            && (rc != PCMK_OCF_RUNNING_MASTER)) {

            /* A failed (not just unexpected) probe result could mean the user
             * didn't know resources will be probed even where they can't run.
             */
            crm_notice("If it is not possible for %s to run on %s, see "
                       "the resource-discovery option for location constraints",
                       rsc->id, node->details->uname);
        }

        record_failed_op(xml_op, node, rsc, data_set);

    } else {
        crm_trace("Processing failed op %s for %s on %s: %s (%d)",
                 task, rsc->id, node->details->uname, services_ocf_exitcode_str(rc),
                 rc);
    }

    action = custom_action(rsc, strdup(key), task, NULL, TRUE, FALSE, data_set);
    if ((action->on_fail <= action_fail_fence && *on_fail < action->on_fail) ||
        (action->on_fail == action_fail_reset_remote && *on_fail <= action_fail_recover) ||
        (action->on_fail == action_fail_restart_container && *on_fail <= action_fail_recover) ||
        (*on_fail == action_fail_restart_container && action->on_fail >= action_fail_migrate)) {
        pe_rsc_trace(rsc, "on-fail %s -> %s for %s (%s)", fail2text(*on_fail),
                     fail2text(action->on_fail), action->uuid, key);
        *on_fail = action->on_fail;
    }

    if (safe_str_eq(task, CRMD_ACTION_STOP)) {
        resource_location(rsc, node, -INFINITY, "__stop_fail__", data_set);

    } else if (safe_str_eq(task, CRMD_ACTION_MIGRATE)) {
        unpack_migrate_to_failure(rsc, node, xml_op, data_set);

    } else if (safe_str_eq(task, CRMD_ACTION_MIGRATED)) {
        unpack_migrate_from_failure(rsc, node, xml_op, data_set);

    } else if (safe_str_eq(task, CRMD_ACTION_PROMOTE)) {
        rsc->role = RSC_ROLE_MASTER;

    } else if (safe_str_eq(task, CRMD_ACTION_DEMOTE)) {
        if (action->on_fail == action_fail_block) {
            rsc->role = RSC_ROLE_MASTER;
            rsc->next_role = RSC_ROLE_STOPPED;

        } else if(rc == PCMK_OCF_NOT_RUNNING) {
            rsc->role = RSC_ROLE_STOPPED;

        } else {
            /*
             * Staying in master role would put the PE/TE into a loop. Setting
             * slave role is not dangerous because the resource will be stopped
             * as part of recovery, and any master promotion will be ordered
             * after that stop.
             */
            rsc->role = RSC_ROLE_SLAVE;
        }
    }

    if(is_probe && rc == PCMK_OCF_NOT_INSTALLED) {
        /* leave stopped */
        pe_rsc_trace(rsc, "Leaving %s stopped", rsc->id);
        rsc->role = RSC_ROLE_STOPPED;

    } else if (rsc->role < RSC_ROLE_STARTED) {
        pe_rsc_trace(rsc, "Setting %s active", rsc->id);
        set_active(rsc);
    }

    pe_rsc_trace(rsc, "Resource %s: role=%s, unclean=%s, on_fail=%s, fail_role=%s",
                 rsc->id, role2text(rsc->role),
                 node->details->unclean ? "true" : "false",
                 fail2text(action->on_fail), role2text(action->fail_role));

    if (action->fail_role != RSC_ROLE_STARTED && rsc->next_role < action->fail_role) {
        rsc->next_role = action->fail_role;
    }

    if (action->fail_role == RSC_ROLE_STOPPED) {
        int score = -INFINITY;

        resource_t *fail_rsc = rsc;

        if (fail_rsc->parent) {
            resource_t *parent = uber_parent(fail_rsc);

            if (pe_rsc_is_clone(parent)
                && is_not_set(parent->flags, pe_rsc_unique)) {
                /* For clone resources, if a child fails on an operation
                 * with on-fail = stop, all the resources fail.  Do this by preventing
                 * the parent from coming up again. */
                fail_rsc = parent;
            }
        }
        crm_warn("Making sure %s doesn't come up again", fail_rsc->id);
        /* make sure it doesn't come up again */
        if (fail_rsc->allowed_nodes != NULL) {
            g_hash_table_destroy(fail_rsc->allowed_nodes);
        }
        fail_rsc->allowed_nodes = node_hash_from_list(data_set->nodes);
        g_hash_table_foreach(fail_rsc->allowed_nodes, set_node_score, &score);
    }

    pe_free_action(action);
}

/*!
 * \internal
 * \brief Remap operation status based on action result
 *
 * Given an action result, determine an appropriate operation status for the
 * purposes of responding to the action (the status provided by the executor is
 * not directly usable since the executor does not know what was expected).
 *
 * \param[in,out] rsc        Resource that operation history entry is for
 * \param[in]     rc         Actual return code of operation
 * \param[in]     target_rc  Expected return code of operation
 * \param[in]     node       Node where operation was executed
 * \param[in]     xml_op     Operation history entry XML from CIB status
 * \param[in,out] on_fail    What should be done about the result
 * \param[in]     data_set   Current cluster working set
 *
 * \return Operation status based on return code and action info
 * \note This may update the resource's current and next role.
 */
static int
determine_op_status(
    resource_t *rsc, int rc, int target_rc, node_t * node, xmlNode * xml_op, enum action_fail_response * on_fail, pe_working_set_t * data_set) 
{
    guint interval_ms = 0;
    int result = PCMK_LRM_OP_DONE;

    const char *key = get_op_key(xml_op);
    const char *task = crm_element_value(xml_op, XML_LRM_ATTR_TASK);

    bool is_probe = FALSE;

    CRM_ASSERT(rsc);

    crm_element_value_ms(xml_op, XML_LRM_ATTR_INTERVAL_MS, &interval_ms);
    if ((interval_ms == 0) && safe_str_eq(task, CRMD_ACTION_STATUS)) {
        is_probe = TRUE;
    }

    if (target_rc < 0) {
        /* Pre-1.0 Pacemaker versions, and Pacemaker 1.1.6 or earlier with
         * Heartbeat 2.0.7 or earlier as the cluster layer, did not include the
         * target_rc in the transition key, which (along with the similar case
         * of a corrupted transition key in the CIB) will be reported to this
         * function as -1. Pacemaker 2.0+ does not support rolling upgrades from
         * those versions or processing of saved CIB files from those versions,
         * so we do not need to care much about this case.
         */
        result = PCMK_LRM_OP_ERROR;
        crm_warn("Expected result not found for %s on %s (corrupt or obsolete CIB?)",
                 key, node->details->uname);

    } else if (target_rc != rc) {
        result = PCMK_LRM_OP_ERROR;
        pe_rsc_debug(rsc, "%s on %s returned '%s' (%d) instead of the expected value: '%s' (%d)",
                     key, node->details->uname,
                     services_ocf_exitcode_str(rc), rc,
                     services_ocf_exitcode_str(target_rc), target_rc);
    }

    switch (rc) {
        case PCMK_OCF_OK:
            // @TODO Should this be (rc != target_rc)?
            if (is_probe && (target_rc == PCMK_OCF_NOT_RUNNING)) {
                result = PCMK_LRM_OP_DONE;
                pe_rsc_info(rsc, "Operation %s found resource %s active on %s",
                            task, rsc->id, node->details->uname);
            }
            break;

        case PCMK_OCF_NOT_RUNNING:
            if (is_probe || target_rc == rc || is_not_set(rsc->flags, pe_rsc_managed)) {
                result = PCMK_LRM_OP_DONE;
                rsc->role = RSC_ROLE_STOPPED;

                /* clear any previous failure actions */
                *on_fail = action_fail_ignore;
                rsc->next_role = RSC_ROLE_UNKNOWN;
            }
            break;

        case PCMK_OCF_RUNNING_MASTER:
            if (is_probe && (rc != target_rc)) {
                result = PCMK_LRM_OP_DONE;
                pe_rsc_info(rsc, "Operation %s found resource %s active in master mode on %s",
                            task, rsc->id, node->details->uname);
            }
            rsc->role = RSC_ROLE_MASTER;
            break;

        case PCMK_OCF_DEGRADED_MASTER:
        case PCMK_OCF_FAILED_MASTER:
            rsc->role = RSC_ROLE_MASTER;
            result = PCMK_LRM_OP_ERROR;
            break;

        case PCMK_OCF_NOT_CONFIGURED:
            result = PCMK_LRM_OP_ERROR_FATAL;
            break;

        case PCMK_OCF_UNIMPLEMENT_FEATURE:
            if (interval_ms > 0) {
                result = PCMK_LRM_OP_NOTSUPPORTED;
                break;
            }
            // fall through
        case PCMK_OCF_NOT_INSTALLED:
        case PCMK_OCF_INVALID_PARAM:
        case PCMK_OCF_INSUFFICIENT_PRIV:
            if (!pe_can_fence(data_set, node)
                && safe_str_eq(task, CRMD_ACTION_STOP)) {
                /* If a stop fails and we can't fence, there's nothing else we can do */
                pe_proc_err("No further recovery can be attempted for %s: %s action failed with '%s' (%d)",
                            rsc->id, task, services_ocf_exitcode_str(rc), rc);
                clear_bit(rsc->flags, pe_rsc_managed);
                set_bit(rsc->flags, pe_rsc_block);
            }
            result = PCMK_LRM_OP_ERROR_HARD;
            break;

        default:
            if (result == PCMK_LRM_OP_DONE) {
                crm_info("Treating unknown return code %d for %s on %s as failure",
                         rc, key, node->details->uname);
                result = PCMK_LRM_OP_ERROR;
            }
            break;
    }
    return result;
}

// return TRUE if start or monitor last failure but parameters changed
static bool
should_clear_for_param_change(xmlNode *xml_op, const char *task,
                              pe_resource_t *rsc, pe_node_t *node,
                              pe_working_set_t *data_set)
{
    if (!strcmp(task, "start") || !strcmp(task, "monitor")) {

        if (pe__bundle_needs_remote_name(rsc)) {
            /* We haven't allocated resources yet, so we can't reliably
             * substitute addr parameters for the REMOTE_CONTAINER_HACK.
             * When that's needed, defer the check until later.
             */
            pe__add_param_check(xml_op, rsc, node, pe_check_last_failure,
                                data_set);

        } else {
            op_digest_cache_t *digest_data = NULL;

            digest_data = rsc_action_digest_cmp(rsc, xml_op, node, data_set);
            switch (digest_data->rc) {
                case RSC_DIGEST_UNKNOWN:
                    crm_trace("Resource %s history entry %s on %s"
                              " has no digest to compare",
                              rsc->id, get_op_key(xml_op), node->details->id);
                    break;
                case RSC_DIGEST_MATCH:
                    break;
                default:
                    return TRUE;
            }
        }
    }
    return FALSE;
}

// Order action after fencing of remote node, given connection rsc
static void
order_after_remote_fencing(pe_action_t *action, pe_resource_t *remote_conn,
                           pe_working_set_t *data_set)
{
    pe_node_t *remote_node = pe_find_node(data_set->nodes, remote_conn->id);

    if (remote_node) {
        pe_action_t *fence = pe_fence_op(remote_node, NULL, TRUE, NULL,
                                         data_set);

        order_actions(fence, action, pe_order_implies_then);
    }
}

static bool
should_ignore_failure_timeout(pe_resource_t *rsc, xmlNode *xml_op,
                              const char *task, guint interval_ms,
                              bool is_last_failure, pe_working_set_t *data_set)
{
    /* Clearing failures of recurring monitors has special concerns. The
     * executor reports only changes in the monitor result, so if the
     * monitor is still active and still getting the same failure result,
     * that will go undetected after the failure is cleared.
     *
     * Also, the operation history will have the time when the recurring
     * monitor result changed to the given code, not the time when the
     * result last happened.
     *
     * @TODO We probably should clear such failures only when the failure
     * timeout has passed since the last occurrence of the failed result.
     * However we don't record that information. We could maybe approximate
     * that by clearing only if there is a more recent successful monitor or
     * stop result, but we don't even have that information at this point
     * since we are still unpacking the resource's operation history.
     *
     * This is especially important for remote connection resources with a
     * reconnect interval, so in that case, we skip clearing failures
     * if the remote node hasn't been fenced.
     */
    if (rsc->remote_reconnect_ms
        && is_set(data_set->flags, pe_flag_stonith_enabled)
        && (interval_ms != 0) && safe_str_eq(task, CRMD_ACTION_STATUS)) {

        pe_node_t *remote_node = pe_find_node(data_set->nodes, rsc->id);

        if (remote_node && !remote_node->details->remote_was_fenced) {
            if (is_last_failure) {
                crm_info("Waiting to clear monitor failure for remote node %s"
                         " until fencing has occurred", rsc->id);
            }
            return TRUE;
        }
    }
    return FALSE;
}

/*!
 * \internal
 * \brief Check operation age and schedule failure clearing when appropriate
 *
 * This function has two distinct purposes. The first is to check whether an
 * operation history entry is expired (i.e. the resource has a failure timeout,
 * the entry is older than the timeout, and the resource either has no fail
 * count or its fail count is entirely older than the timeout). The second is to
 * schedule fail count clearing when appropriate (i.e. the operation is expired
 * and either the resource has an expired fail count or the operation is a
 * last_failure for a remote connection resource with a reconnect interval,
 * or the operation is a last_failure for a start or monitor operation and the
 * resource's parameters have changed since the operation).
 *
 * \param[in] rsc       Resource that operation happened to
 * \param[in] node      Node that operation happened on
 * \param[in] rc        Actual result of operation
 * \param[in] xml_op    Operation history entry XML
 * \param[in] data_set  Current working set
 *
 * \return TRUE if operation history entry is expired, FALSE otherwise
 */
static bool
check_operation_expiry(pe_resource_t *rsc, pe_node_t *node, int rc,
                       xmlNode *xml_op, pe_working_set_t *data_set)
{
    bool expired = FALSE;
    bool is_last_failure = crm_ends_with(ID(xml_op), "_last_failure_0");
    time_t last_run = 0;
    guint interval_ms = 0;
    int unexpired_fail_count = 0;
    const char *task = crm_element_value(xml_op, XML_LRM_ATTR_TASK);
    const char *clear_reason = NULL;

    crm_element_value_ms(xml_op, XML_LRM_ATTR_INTERVAL_MS, &interval_ms);

    if ((rsc->failure_timeout > 0)
        && (crm_element_value_epoch(xml_op, XML_RSC_OP_LAST_CHANGE,
                                    &last_run) == 0)) {

        // Resource has a failure-timeout, and history entry has a timestamp

        time_t now = get_effective_time(data_set);
        time_t last_failure = 0;

        // Is this particular operation history older than the failure timeout?
        if ((now >= (last_run + rsc->failure_timeout))
            && !should_ignore_failure_timeout(rsc, xml_op, task, interval_ms,
                                              is_last_failure, data_set)) {
            expired = TRUE;
        }

        // Does the resource as a whole have an unexpired fail count?
        unexpired_fail_count = pe_get_failcount(node, rsc, &last_failure,
                                                pe_fc_effective, xml_op,
                                                data_set);

        // Update scheduler recheck time according to *last* failure
        crm_trace("%s@%lld is %sexpired @%lld with unexpired_failures=%d timeout=%ds"
                  " last-failure@%lld",
                  ID(xml_op), (long long) last_run, (expired? "" : "not "),
                  (long long) now, unexpired_fail_count, rsc->failure_timeout,
                  (long long) last_failure);
        last_failure += rsc->failure_timeout + 1;
        if (unexpired_fail_count && (now < last_failure)) {
            pe__update_recheck_time(last_failure, data_set);
        }
    }

    if (expired) {
        if (pe_get_failcount(node, rsc, NULL, pe_fc_default, xml_op, data_set)) {

            // There is a fail count ignoring timeout

            if (unexpired_fail_count == 0) {
                // There is no fail count considering timeout
                clear_reason = "it expired";

            } else {
                /* This operation is old, but there is an unexpired fail count.
                 * In a properly functioning cluster, this should only be
                 * possible if this operation is not a failure (otherwise the
                 * fail count should be expired too), so this is really just a
                 * failsafe.
                 */
                expired = FALSE;
            }

        } else if (is_last_failure && rsc->remote_reconnect_ms) {
            /* Clear any expired last failure when reconnect interval is set,
             * even if there is no fail count.
             */
            clear_reason = "reconnect interval is set";
        }
    }

    if (!expired && is_last_failure
        && should_clear_for_param_change(xml_op, task, rsc, node, data_set)) {
        clear_reason = "resource parameters have changed";
    }

    if (clear_reason != NULL) {
        // Schedule clearing of the fail count
        pe_action_t *clear_op = pe__clear_failcount(rsc, node, clear_reason,
                                                    data_set);

        if (is_set(data_set->flags, pe_flag_stonith_enabled)
            && rsc->remote_reconnect_ms) {
            /* If we're clearing a remote connection due to a reconnect
             * interval, we want to wait until any scheduled fencing
             * completes.
             *
             * We could limit this to remote_node->details->unclean, but at
             * this point, that's always true (it won't be reliable until
             * after unpack_node_loop() is done).
             */
            crm_info("Clearing %s failure will wait until any scheduled "
                     "fencing of %s completes", task, rsc->id);
            order_after_remote_fencing(clear_op, rsc, data_set);
        }
    }

    if (expired && (interval_ms == 0) && safe_str_eq(task, CRMD_ACTION_STATUS)) {
        switch(rc) {
            case PCMK_OCF_OK:
            case PCMK_OCF_NOT_RUNNING:
            case PCMK_OCF_RUNNING_MASTER:
            case PCMK_OCF_DEGRADED:
            case PCMK_OCF_DEGRADED_MASTER:
                // Don't expire probes that return these values
                expired = FALSE;
                break;
        }
    }

    return expired;
}

int pe__target_rc_from_xml(xmlNode *xml_op)
{
    int target_rc = 0;
    const char *key = crm_element_value(xml_op, XML_ATTR_TRANSITION_KEY);

    if (key == NULL) {
        return -1;
    }
    decode_transition_key(key, NULL, NULL, NULL, &target_rc);
    return target_rc;
}

static enum action_fail_response
get_action_on_fail(resource_t *rsc, const char *key, const char *task, pe_working_set_t * data_set) 
{
    int result = action_fail_recover;
    action_t *action = custom_action(rsc, strdup(key), task, NULL, TRUE, FALSE, data_set);

    result = action->on_fail;
    pe_free_action(action);

    return result;
}

static void
update_resource_state(resource_t * rsc, node_t * node, xmlNode * xml_op, const char * task, int rc,
                      xmlNode * last_failure, enum action_fail_response * on_fail, pe_working_set_t * data_set)
{
    gboolean clear_past_failure = FALSE;

    CRM_ASSERT(rsc);
    CRM_ASSERT(xml_op);

    if (rc == PCMK_OCF_NOT_RUNNING) {
        clear_past_failure = TRUE;

    } else if (rc == PCMK_OCF_NOT_INSTALLED) {
        rsc->role = RSC_ROLE_STOPPED;

    } else if (safe_str_eq(task, CRMD_ACTION_STATUS)) {
        if (last_failure) {
            const char *op_key = get_op_key(xml_op);
            const char *last_failure_key = get_op_key(last_failure);

            if (safe_str_eq(op_key, last_failure_key)) {
                clear_past_failure = TRUE;
            }
        }

        if (rsc->role < RSC_ROLE_STARTED) {
            set_active(rsc);
        }

    } else if (safe_str_eq(task, CRMD_ACTION_START)) {
        rsc->role = RSC_ROLE_STARTED;
        clear_past_failure = TRUE;

    } else if (safe_str_eq(task, CRMD_ACTION_STOP)) {
        rsc->role = RSC_ROLE_STOPPED;
        clear_past_failure = TRUE;

    } else if (safe_str_eq(task, CRMD_ACTION_PROMOTE)) {
        rsc->role = RSC_ROLE_MASTER;
        clear_past_failure = TRUE;

    } else if (safe_str_eq(task, CRMD_ACTION_DEMOTE)) {
        /* Demote from Master does not clear an error */
        rsc->role = RSC_ROLE_SLAVE;

    } else if (safe_str_eq(task, CRMD_ACTION_MIGRATED)) {
        rsc->role = RSC_ROLE_STARTED;
        clear_past_failure = TRUE;

    } else if (safe_str_eq(task, CRMD_ACTION_MIGRATE)) {
        unpack_migrate_to_success(rsc, node, xml_op, data_set);

    } else if (rsc->role < RSC_ROLE_STARTED) {
        pe_rsc_trace(rsc, "%s active on %s", rsc->id, node->details->uname);
        set_active(rsc);
    }

    /* clear any previous failure actions */
    if (clear_past_failure) {
        switch (*on_fail) {
            case action_fail_stop:
            case action_fail_fence:
            case action_fail_migrate:
            case action_fail_standby:
                pe_rsc_trace(rsc, "%s.%s is not cleared by a completed stop",
                             rsc->id, fail2text(*on_fail));
                break;

            case action_fail_block:
            case action_fail_ignore:
            case action_fail_recover:
            case action_fail_restart_container:
                *on_fail = action_fail_ignore;
                rsc->next_role = RSC_ROLE_UNKNOWN;
                break;
            case action_fail_reset_remote:
                if (rsc->remote_reconnect_ms == 0) {
                    /* With no reconnect interval, the connection is allowed to
                     * start again after the remote node is fenced and
                     * completely stopped. (With a reconnect interval, we wait
                     * for the failure to be cleared entirely before attempting
                     * to reconnect.)
                     */
                    *on_fail = action_fail_ignore;
                    rsc->next_role = RSC_ROLE_UNKNOWN;
                }
                break;
        }
    }
}

/*!
 * \internal
 * \brief Remap informational monitor results to usual values
 *
 * Certain OCF result codes are for providing extended information to the
 * user about services that aren't yet failed but not entirely healthy either.
 * These must be treated as the "normal" result by pacemaker.
 *
 * \param[in] rc        Actual result of a monitor action
 * \param[in] xml_op    Operation history XML
 * \param[in] node      Node that operation happened on
 * \param[in] rsc       Resource that operation happened to
 * \param[in] data_set  Cluster working set
 *
 * \return Result code that pacemaker should use
 *
 * \note If the result is remapped, and the node is not shutting down or failed,
 *       the operation will be recorded in the data set's list of failed
 *       operations, to highlight it for the user.
 */
static int
remap_monitor_rc(int rc, xmlNode *xml_op, const pe_node_t *node,
                 const pe_resource_t *rsc, pe_working_set_t *data_set)
{
    int remapped_rc = rc;

    switch (rc) {
        case PCMK_OCF_DEGRADED:
            remapped_rc = PCMK_OCF_OK;
            break;

        case PCMK_OCF_DEGRADED_MASTER:
            remapped_rc = PCMK_OCF_RUNNING_MASTER;
            break;

        default:
            break;
    }

    if (rc != remapped_rc) {
        crm_trace("Remapping monitor result %d to %d", rc, remapped_rc);
        if (!node->details->shutdown || node->details->online) {
            record_failed_op(xml_op, node, rsc, data_set);
        }
    }
    return remapped_rc;
}

static void
unpack_rsc_op(pe_resource_t *rsc, pe_node_t *node, xmlNode *xml_op,
              xmlNode **last_failure, enum action_fail_response *on_fail,
              pe_working_set_t *data_set)
{
    int task_id = 0;

    const char *task = NULL;
    const char *task_key = NULL;

    int rc = 0;
    int status = PCMK_LRM_OP_UNKNOWN;
    int target_rc = pe__target_rc_from_xml(xml_op);
    guint interval_ms = 0;

    bool expired = FALSE;
    resource_t *parent = rsc;
    enum action_fail_response failure_strategy = action_fail_recover;

    CRM_CHECK(rsc != NULL, return);
    CRM_CHECK(node != NULL, return);
    CRM_CHECK(xml_op != NULL, return);

    task_key = get_op_key(xml_op);

    task = crm_element_value(xml_op, XML_LRM_ATTR_TASK);

    crm_element_value_int(xml_op, XML_LRM_ATTR_RC, &rc);
    crm_element_value_int(xml_op, XML_LRM_ATTR_CALLID, &task_id);
    crm_element_value_int(xml_op, XML_LRM_ATTR_OPSTATUS, &status);
    crm_element_value_ms(xml_op, XML_LRM_ATTR_INTERVAL_MS, &interval_ms);

    CRM_CHECK(task != NULL, return);
    CRM_CHECK(status <= PCMK_LRM_OP_INVALID, return);
    CRM_CHECK(status >= PCMK_LRM_OP_PENDING, return);

    if (safe_str_eq(task, CRMD_ACTION_NOTIFY) ||
        safe_str_eq(task, CRMD_ACTION_METADATA)) {
        /* safe to ignore these */
        return;
    }

    if (is_not_set(rsc->flags, pe_rsc_unique)) {
        parent = uber_parent(rsc);
    }

    pe_rsc_trace(rsc, "Unpacking task %s/%s (call_id=%d, status=%d, rc=%d) on %s (role=%s)",
                 task_key, task, task_id, status, rc, node->details->uname, role2text(rsc->role));

    if (node->details->unclean) {
        pe_rsc_trace(rsc, "Node %s (where %s is running) is unclean."
                     " Further action depends on the value of the stop's on-fail attribute",
                     node->details->uname, rsc->id);
    }

    /* It should be possible to call remap_monitor_rc() first then call
     * check_operation_expiry() only if rc != target_rc, because there should
     * never be a fail count without at least one unexpected result in the
     * resource history. That would be more efficient by avoiding having to call
     * check_operation_expiry() for expected results.
     *
     * However, we do have such configurations in the scheduler regression
     * tests, even if it shouldn't be possible with the current code. It's
     * probably a good idea anyway, but that would require updating the test
     * inputs to something currently possible.
     */

    if ((status != PCMK_LRM_OP_NOT_INSTALLED)
        && check_operation_expiry(rsc, node, rc, xml_op, data_set)) {
        expired = TRUE;
    }

    if (!strcmp(task, CRMD_ACTION_STATUS)) {
        rc = remap_monitor_rc(rc, xml_op, node, rsc, data_set);
    }

    if (expired && (rc != target_rc)) {
        const char *magic = crm_element_value(xml_op, XML_ATTR_TRANSITION_MAGIC);

        if (interval_ms == 0) {
            crm_notice("Ignoring expired %s failure on %s "
                       CRM_XS " actual=%d expected=%d magic=%s",
                       task_key, node->details->uname, rc, target_rc, magic);
            goto done;

        } else if(node->details->online && node->details->unclean == FALSE) {
            /* Reschedule the recurring monitor. CancelXmlOp() won't work at
             * this stage, so as a hacky workaround, forcibly change the restart
             * digest so check_action_definition() does what we want later.
             *
             * @TODO We should skip this if there is a newer successful monitor.
             *       Also, this causes rescheduling only if the history entry
             *       has an op-digest (which the expire-non-blocked-failure
             *       scheduler regression test doesn't, but that may not be a
             *       realistic scenario in production).
             */
            crm_notice("Rescheduling %s after failure expired on %s "
                       CRM_XS " actual=%d expected=%d magic=%s",
                       task_key, node->details->uname, rc, target_rc, magic);
            crm_xml_add(xml_op, XML_LRM_ATTR_RESTART_DIGEST, "calculated-failure-timeout");
            goto done;
        }
    }

    /* If the executor reported an operation status of anything but done or
     * error, consider that final. But for done or error, we know better whether
     * it should be treated as a failure or not, because we know the expected
     * result.
     */
    if(status == PCMK_LRM_OP_DONE || status == PCMK_LRM_OP_ERROR) {
        status = determine_op_status(rsc, rc, target_rc, node, xml_op, on_fail, data_set);
    }

    pe_rsc_trace(rsc, "Handling status: %d", status);
    switch (status) {
        case PCMK_LRM_OP_CANCELLED:
            /* do nothing?? */
            pe_err("Don't know what to do for cancelled ops yet");
            break;

        case PCMK_LRM_OP_PENDING:
            if (safe_str_eq(task, CRMD_ACTION_START)) {
                set_bit(rsc->flags, pe_rsc_start_pending);
                set_active(rsc);

            } else if (safe_str_eq(task, CRMD_ACTION_PROMOTE)) {
                rsc->role = RSC_ROLE_MASTER;

            } else if (safe_str_eq(task, CRMD_ACTION_MIGRATE) && node->details->unclean) {
                /* If a pending migrate_to action is out on a unclean node,
                 * we have to force the stop action on the target. */
                const char *migrate_target = crm_element_value(xml_op, XML_LRM_ATTR_MIGRATE_TARGET);
                node_t *target = pe_find_node(data_set->nodes, migrate_target);
                if (target) {
                    stop_action(rsc, target, FALSE);
                }
            }

            if (rsc->pending_task == NULL) {
                if (safe_str_eq(task, CRMD_ACTION_STATUS) && (interval_ms == 0)) {
                    /* Pending probes are not printed, even if pending
                     * operations are requested. If someone ever requests that
                     * behavior, uncomment this and the corresponding part of
                     * native.c:native_pending_task().
                     */
                    /*rsc->pending_task = strdup("probe");*/
                    /*rsc->pending_node = node;*/
                } else {
                    rsc->pending_task = strdup(task);
                    rsc->pending_node = node;
                }
            }
            break;

        case PCMK_LRM_OP_DONE:
            pe_rsc_trace(rsc, "%s/%s completed on %s", rsc->id, task, node->details->uname);
            update_resource_state(rsc, node, xml_op, task, rc, *last_failure, on_fail, data_set);
            break;

        case PCMK_LRM_OP_NOT_INSTALLED:
            failure_strategy = get_action_on_fail(rsc, task_key, task, data_set);
            if (failure_strategy == action_fail_ignore) {
                crm_warn("Cannot ignore failed %s (status=%d, rc=%d) on %s: "
                         "Resource agent doesn't exist",
                         task_key, status, rc, node->details->uname);
                /* Also for printing it as "FAILED" by marking it as pe_rsc_failed later */
                *on_fail = action_fail_migrate;
            }
            resource_location(parent, node, -INFINITY, "hard-error", data_set);
            unpack_rsc_op_failure(rsc, node, rc, xml_op, last_failure, on_fail, data_set);
            break;

        case PCMK_LRM_OP_NOT_CONNECTED:
            if (pe__is_guest_or_remote_node(node)
                && is_set(node->details->remote_rsc->flags, pe_rsc_managed)) {
                /* We should never get into a situation where a managed remote
                 * connection resource is considered OK but a resource action
                 * behind the connection gets a "not connected" status. But as a
                 * fail-safe in case a bug or unusual circumstances do lead to
                 * that, ensure the remote connection is considered failed.
                 */
                set_bit(node->details->remote_rsc->flags, pe_rsc_failed);
            }

            // fall through

        case PCMK_LRM_OP_ERROR:
        case PCMK_LRM_OP_ERROR_HARD:
        case PCMK_LRM_OP_ERROR_FATAL:
        case PCMK_LRM_OP_TIMEOUT:
        case PCMK_LRM_OP_NOTSUPPORTED:
        case PCMK_LRM_OP_INVALID:

            failure_strategy = get_action_on_fail(rsc, task_key, task, data_set);
            if ((failure_strategy == action_fail_ignore)
                || (failure_strategy == action_fail_restart_container
                    && safe_str_eq(task, CRMD_ACTION_STOP))) {

                crm_warn("Pretending the failure of %s (rc=%d) on %s succeeded",
                         task_key, rc, node->details->uname);

                update_resource_state(rsc, node, xml_op, task, target_rc, *last_failure, on_fail, data_set);
                crm_xml_add(xml_op, XML_ATTR_UNAME, node->details->uname);
                set_bit(rsc->flags, pe_rsc_failure_ignored);

                record_failed_op(xml_op, node, rsc, data_set);

                if (failure_strategy == action_fail_restart_container && *on_fail <= action_fail_recover) {
                    *on_fail = failure_strategy;
                }

            } else {
                unpack_rsc_op_failure(rsc, node, rc, xml_op, last_failure, on_fail, data_set);

                if(status == PCMK_LRM_OP_ERROR_HARD) {
                    do_crm_log(rc != PCMK_OCF_NOT_INSTALLED?LOG_ERR:LOG_NOTICE,
                               "Preventing %s from re-starting on %s: operation %s failed '%s' (%d)",
                               parent->id, node->details->uname,
                               task, services_ocf_exitcode_str(rc), rc);

                    resource_location(parent, node, -INFINITY, "hard-error", data_set);

                } else if(status == PCMK_LRM_OP_ERROR_FATAL) {
                    crm_err("Preventing %s from re-starting anywhere: operation %s failed '%s' (%d)",
                            parent->id, task, services_ocf_exitcode_str(rc), rc);

                    resource_location(parent, NULL, -INFINITY, "fatal-error", data_set);
                }
            }
            break;
    }

  done:
    pe_rsc_trace(rsc, "Resource %s after %s: role=%s, next=%s",
                 rsc->id, task, role2text(rsc->role),
                 role2text(rsc->next_role));
}

static void
add_node_attrs(xmlNode *xml_obj, pe_node_t *node, bool overwrite,
               pe_working_set_t *data_set)
{
    const char *cluster_name = NULL;

    g_hash_table_insert(node->details->attrs,
                        strdup(CRM_ATTR_UNAME), strdup(node->details->uname));

    g_hash_table_insert(node->details->attrs, strdup(CRM_ATTR_ID),
                        strdup(node->details->id));
    if (safe_str_eq(node->details->id, data_set->dc_uuid)) {
        data_set->dc_node = node;
        node->details->is_dc = TRUE;
        g_hash_table_insert(node->details->attrs,
                            strdup(CRM_ATTR_IS_DC), strdup(XML_BOOLEAN_TRUE));
    } else {
        g_hash_table_insert(node->details->attrs,
                            strdup(CRM_ATTR_IS_DC), strdup(XML_BOOLEAN_FALSE));
    }

    cluster_name = g_hash_table_lookup(data_set->config_hash, "cluster-name");
    if (cluster_name) {
        g_hash_table_insert(node->details->attrs, strdup(CRM_ATTR_CLUSTER_NAME),
                            strdup(cluster_name));
    }

    pe__unpack_dataset_nvpairs(xml_obj, XML_TAG_ATTR_SETS, NULL,
                               node->details->attrs, NULL, overwrite, data_set);

    if (pe_node_attribute_raw(node, CRM_ATTR_SITE_NAME) == NULL) {
        const char *site_name = pe_node_attribute_raw(node, "site-name");

        if (site_name) {
            g_hash_table_insert(node->details->attrs,
                                strdup(CRM_ATTR_SITE_NAME),
                                strdup(site_name));

        } else if (cluster_name) {
            /* Default to cluster-name if unset */
            g_hash_table_insert(node->details->attrs,
                                strdup(CRM_ATTR_SITE_NAME),
                                strdup(cluster_name));
        }
    }
}

static GListPtr
extract_operations(const char *node, const char *rsc, xmlNode * rsc_entry, gboolean active_filter)
{
    int counter = -1;
    int stop_index = -1;
    int start_index = -1;

    xmlNode *rsc_op = NULL;

    GListPtr gIter = NULL;
    GListPtr op_list = NULL;
    GListPtr sorted_op_list = NULL;

    /* extract operations */
    op_list = NULL;
    sorted_op_list = NULL;

    for (rsc_op = __xml_first_child_element(rsc_entry);
         rsc_op != NULL; rsc_op = __xml_next_element(rsc_op)) {
        if (crm_str_eq((const char *)rsc_op->name, XML_LRM_TAG_RSC_OP, TRUE)) {
            crm_xml_add(rsc_op, "resource", rsc);
            crm_xml_add(rsc_op, XML_ATTR_UNAME, node);
            op_list = g_list_prepend(op_list, rsc_op);
        }
    }

    if (op_list == NULL) {
        /* if there are no operations, there is nothing to do */
        return NULL;
    }

    sorted_op_list = g_list_sort(op_list, sort_op_by_callid);

    /* create active recurring operations as optional */
    if (active_filter == FALSE) {
        return sorted_op_list;
    }

    op_list = NULL;

    calculate_active_ops(sorted_op_list, &start_index, &stop_index);

    for (gIter = sorted_op_list; gIter != NULL; gIter = gIter->next) {
        xmlNode *rsc_op = (xmlNode *) gIter->data;

        counter++;

        if (start_index < stop_index) {
            crm_trace("Skipping %s: not active", ID(rsc_entry));
            break;

        } else if (counter < start_index) {
            crm_trace("Skipping %s: old", ID(rsc_op));
            continue;
        }
        op_list = g_list_append(op_list, rsc_op);
    }

    g_list_free(sorted_op_list);
    return op_list;
}

GListPtr
find_operations(const char *rsc, const char *node, gboolean active_filter,
                pe_working_set_t * data_set)
{
    GListPtr output = NULL;
    GListPtr intermediate = NULL;

    xmlNode *tmp = NULL;
    xmlNode *status = find_xml_node(data_set->input, XML_CIB_TAG_STATUS, TRUE);

    node_t *this_node = NULL;

    xmlNode *node_state = NULL;

    for (node_state = __xml_first_child_element(status); node_state != NULL;
         node_state = __xml_next_element(node_state)) {

        if (crm_str_eq((const char *)node_state->name, XML_CIB_TAG_STATE, TRUE)) {
            const char *uname = crm_element_value(node_state, XML_ATTR_UNAME);

            if (node != NULL && safe_str_neq(uname, node)) {
                continue;
            }

            this_node = pe_find_node(data_set->nodes, uname);
            if(this_node == NULL) {
                CRM_LOG_ASSERT(this_node != NULL);
                continue;

            } else if (pe__is_guest_or_remote_node(this_node)) {
                determine_remote_online_status(data_set, this_node);

            } else {
                determine_online_status(node_state, this_node, data_set);
            }

            if (this_node->details->online || is_set(data_set->flags, pe_flag_stonith_enabled)) {
                /* offline nodes run no resources...
                 * unless stonith is enabled in which case we need to
                 *   make sure rsc start events happen after the stonith
                 */
                xmlNode *lrm_rsc = NULL;

                tmp = find_xml_node(node_state, XML_CIB_TAG_LRM, FALSE);
                tmp = find_xml_node(tmp, XML_LRM_TAG_RESOURCES, FALSE);

                for (lrm_rsc = __xml_first_child_element(tmp); lrm_rsc != NULL;
                     lrm_rsc = __xml_next_element(lrm_rsc)) {
                    if (crm_str_eq((const char *)lrm_rsc->name, XML_LRM_TAG_RESOURCE, TRUE)) {

                        const char *rsc_id = crm_element_value(lrm_rsc, XML_ATTR_ID);

                        if (rsc != NULL && safe_str_neq(rsc_id, rsc)) {
                            continue;
                        }

                        intermediate = extract_operations(uname, rsc_id, lrm_rsc, active_filter);
                        output = g_list_concat(output, intermediate);
                    }
                }
            }
        }
    }

    return output;
}
