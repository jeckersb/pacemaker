/*
 * Copyright 2004-2019 the Pacemaker project contributors
 *
 * The version control history for this file may have further details.
 *
 * This source code is licensed under the GNU General Public License version 2
 * or later (GPLv2+) WITHOUT ANY WARRANTY.
 */

#include <crm_internal.h>

#include <crm/crm.h>

#include <crm/msg_xml.h>
#include <crm/common/xml.h>
#include <crm/cluster.h>

#include <pacemaker-controld.h>

char *max_epoch = NULL;
char *max_generation_from = NULL;
xmlNode *max_generation_xml = NULL;

void initialize_join(gboolean before);
void finalize_join_for(gpointer key, gpointer value, gpointer user_data);
void finalize_sync_callback(xmlNode * msg, int call_id, int rc, xmlNode * output, void *user_data);
gboolean check_join_state(enum crmd_fsa_state cur_state, const char *source);

static int current_join_id = 0;
unsigned long long saved_ccm_membership_id = 0;

void
crm_update_peer_join(const char *source, crm_node_t * node, enum crm_join_phase phase)
{
    enum crm_join_phase last = 0;

    if(node == NULL) {
        crm_err("Could not update join because node not specified"
                CRM_XS " join-%u source=%s phase=%s",
                current_join_id, source, crm_join_phase_str(phase));
        return;
    }

    /* Remote nodes do not participate in joins */
    if (is_set(node->flags, crm_remote_node)) {
        return;
    }

    last = node->join;

    if(phase == last) {
        crm_trace("%s: Node %s[%u] - join-%u phase still %s",
                  source, node->uname, node->id, current_join_id,
                  crm_join_phase_str(last));

    } else if ((phase <= crm_join_none) || (phase == (last + 1))) {
        node->join = phase;
        crm_info("%s: Node %s[%u] - join-%u phase %s -> %s",
                 source, node->uname, node->id, current_join_id,
                 crm_join_phase_str(last), crm_join_phase_str(phase));

    } else {
        crm_err("Could not update join for node %s because phase transition invalid "
                CRM_XS " join-%u source=%s node_id=%u last=%s new=%s",
                node->uname, current_join_id, source, node->id,
                crm_join_phase_str(last), crm_join_phase_str(phase));
    }
}

void
initialize_join(gboolean before)
{
    GHashTableIter iter;
    crm_node_t *peer = NULL;

    /* clear out/reset a bunch of stuff */
    crm_debug("join-%d: Initializing join data (flag=%s)",
              current_join_id, before ? "true" : "false");

    g_hash_table_iter_init(&iter, crm_peer_cache);
    while (g_hash_table_iter_next(&iter, NULL, (gpointer *) &peer)) {
        crm_update_peer_join(__FUNCTION__, peer, crm_join_none);
    }

    if (before) {
        if (max_generation_from != NULL) {
            free(max_generation_from);
            max_generation_from = NULL;
        }
        if (max_generation_xml != NULL) {
            free_xml(max_generation_xml);
            max_generation_xml = NULL;
        }
        clear_bit(fsa_input_register, R_HAVE_CIB);
        clear_bit(fsa_input_register, R_CIB_ASKED);
    }
}

/*!
 * \internal
 * \brief Create a join message from the DC
 *
 * \param[in] join_op  Join operation name
 * \param[in] host_to  Recipient of message
 */
static xmlNode *
create_dc_message(const char *join_op, const char *host_to)
{
    xmlNode *msg = create_request(join_op, NULL, host_to, CRM_SYSTEM_CRMD,
                                  CRM_SYSTEM_DC, NULL);

    /* Identify which election this is a part of */
    crm_xml_add_int(msg, F_CRM_JOIN_ID, current_join_id);

    /* Add a field specifying whether the DC is shutting down. This keeps the
     * joining node from fencing the old DC if it becomes the new DC.
     */
    crm_xml_add_boolean(msg, F_CRM_DC_LEAVING,
                        is_set(fsa_input_register, R_SHUTDOWN));
    return msg;
}

static void
join_make_offer(gpointer key, gpointer value, gpointer user_data)
{
    xmlNode *offer = NULL;
    crm_node_t *member = (crm_node_t *)value;

    CRM_ASSERT(member != NULL);
    if (crm_is_peer_active(member) == FALSE) {
        crm_info("Not making an offer to %s: not active (%s)", member->uname, member->state);
        if(member->expected == NULL && safe_str_eq(member->state, CRM_NODE_LOST)) {
            /* You would think this unsafe, but in fact this plus an
             * active resource is what causes it to be fenced.
             *
             * Yes, this does mean that any node that dies at the same
             * time as the old DC and is not running resource (still)
             * won't be fenced.
             *
             * I'm not happy about this either.
             */
            crm_update_peer_expected(__FUNCTION__, member, CRMD_JOINSTATE_DOWN);
        }
        return;
    }

    if (member->uname == NULL) {
        crm_info("No recipient for welcome message.(Node uuid:%s)", member->uuid);
        return;
    }

    if (saved_ccm_membership_id != crm_peer_seq) {
        saved_ccm_membership_id = crm_peer_seq;
        crm_info("Making join offers based on membership %llu", crm_peer_seq);
    }

    if(user_data && member->join > crm_join_none) {
        crm_info("Skipping %s: already known %d", member->uname, member->join);
        return;
    }

    crm_update_peer_join(__FUNCTION__, (crm_node_t*)member, crm_join_none);

    offer = create_dc_message(CRM_OP_JOIN_OFFER, member->uname);

    // Advertise our feature set so the joining node can bail if not compatible
    crm_xml_add(offer, XML_ATTR_CRM_VERSION, CRM_FEATURE_SET);

    /* send the welcome */
    crm_info("join-%d: Sending offer to %s", current_join_id, member->uname);

    send_cluster_message(member, crm_msg_crmd, offer, TRUE);
    free_xml(offer);

    crm_update_peer_join(__FUNCTION__, member, crm_join_welcomed);
    /* crm_update_peer_expected(__FUNCTION__, member, CRMD_JOINSTATE_PENDING); */
}

/*	 A_DC_JOIN_OFFER_ALL	*/
void
do_dc_join_offer_all(long long action,
                     enum crmd_fsa_cause cause,
                     enum crmd_fsa_state cur_state,
                     enum crmd_fsa_input current_input, fsa_data_t * msg_data)
{
    /* Reset everyone's status back to down or in_ccm in the CIB.
     * Any nodes that are active in the CIB but not in the cluster membership
     * will be seen as offline by the scheduler anyway.
     */
    current_join_id++;
    initialize_join(TRUE);
/* 	do_update_cib_nodes(TRUE, __FUNCTION__); */

    update_dc(NULL);
    if (cause == C_HA_MESSAGE && current_input == I_NODE_JOIN) {
        crm_info("A new node joined the cluster");
    }
    g_hash_table_foreach(crm_peer_cache, join_make_offer, NULL);

    // Don't waste time by invoking the scheduler yet
    crm_info("join-%d: Waiting on %d outstanding join acks",
             current_join_id, crmd_join_phase_count(crm_join_welcomed));
}

/*	 A_DC_JOIN_OFFER_ONE	*/
void
do_dc_join_offer_one(long long action,
                     enum crmd_fsa_cause cause,
                     enum crmd_fsa_state cur_state,
                     enum crmd_fsa_input current_input, fsa_data_t * msg_data)
{
    crm_node_t *member;
    ha_msg_input_t *welcome = NULL;

    const char *op = NULL;
    const char *join_to = NULL;

    if (msg_data->data) {
        welcome = fsa_typed_data(fsa_dt_ha_msg);

    } else {
        crm_info("An unknown node joined - (re-)offer to any unconfirmed nodes");
        g_hash_table_foreach(crm_peer_cache, join_make_offer, &member);
        check_join_state(cur_state, __FUNCTION__);
        return;
    }

    if (welcome == NULL) {
        crm_err("Attempt to send welcome message without a message to reply to!");
        return;
    }

    join_to = crm_element_value(welcome->msg, F_CRM_HOST_FROM);
    if (join_to == NULL) {
        crm_err("Attempt to send welcome message without a host to reply to!");
        return;
    }

    member = crm_get_peer(0, join_to);
    op = crm_element_value(welcome->msg, F_CRM_TASK);
    if (join_to != NULL && (cur_state == S_INTEGRATION || cur_state == S_FINALIZE_JOIN)) {
        /* note: it _is_ possible that a node will have been
         *  sick or starting up when the original offer was made.
         *  however, it will either re-announce itself in due course
         *  _or_ we can re-store the original offer on the client.
         */
        crm_trace("(Re-)offering membership to %s...", join_to);
    }

    crm_info("join-%d: Processing %s request from %s in state %s",
             current_join_id, op, join_to, fsa_state2string(cur_state));

    crm_update_peer_join(__FUNCTION__, member, crm_join_none);
    join_make_offer(NULL, member, NULL);

    /* always offer to the DC (ourselves)
     * this ensures the correct value for max_generation_from
     */
    if (strcmp(join_to, fsa_our_uname) != 0) {
        member = crm_get_peer(0, fsa_our_uname);
        join_make_offer(NULL, member, NULL);
    }

    /* this was a genuine join request, cancel any existing
     * transition and invoke the PE
     */
    abort_transition(INFINITY, tg_restart, "Node join", NULL);

    // Don't waste time by invoking the scheduler yet
    crm_debug("Waiting on %d outstanding join acks for join-%d",
              crmd_join_phase_count(crm_join_welcomed), current_join_id);
}

static int
compare_int_fields(xmlNode * left, xmlNode * right, const char *field)
{
    const char *elem_l = crm_element_value(left, field);
    const char *elem_r = crm_element_value(right, field);

    int int_elem_l = crm_int_helper(elem_l, NULL);
    int int_elem_r = crm_int_helper(elem_r, NULL);

    if (int_elem_l < int_elem_r) {
        return -1;

    } else if (int_elem_l > int_elem_r) {
        return 1;
    }

    return 0;
}

/*	 A_DC_JOIN_PROCESS_REQ	*/
void
do_dc_join_filter_offer(long long action,
                        enum crmd_fsa_cause cause,
                        enum crmd_fsa_state cur_state,
                        enum crmd_fsa_input current_input, fsa_data_t * msg_data)
{
    xmlNode *generation = NULL;

    int cmp = 0;
    int join_id = -1;
    gboolean ack_nack_bool = TRUE;
    const char *ack_nack = CRMD_JOINSTATE_MEMBER;
    ha_msg_input_t *join_ack = fsa_typed_data(fsa_dt_ha_msg);

    const char *join_from = crm_element_value(join_ack->msg, F_CRM_HOST_FROM);
    const char *ref = crm_element_value(join_ack->msg, F_CRM_REFERENCE);
    const char *join_version = crm_element_value(join_ack->msg,
                                                 XML_ATTR_CRM_VERSION);

    crm_node_t *join_node = crm_get_peer(0, join_from);

    crm_debug("Processing req from %s", join_from);

    generation = join_ack->xml;
    crm_element_value_int(join_ack->msg, F_CRM_JOIN_ID, &join_id);

    if (max_generation_xml != NULL && generation != NULL) {
        int lpc = 0;

        const char *attributes[] = {
            XML_ATTR_GENERATION_ADMIN,
            XML_ATTR_GENERATION,
            XML_ATTR_NUMUPDATES,
        };

        for (lpc = 0; cmp == 0 && lpc < DIMOF(attributes); lpc++) {
            cmp = compare_int_fields(max_generation_xml, generation, attributes[lpc]);
        }
    }

    if (join_id != current_join_id) {
        crm_debug("Invalid response from %s: join-%d vs. join-%d",
                  join_from, join_id, current_join_id);
        check_join_state(cur_state, __FUNCTION__);
        return;

    } else if (join_node == NULL || crm_is_peer_active(join_node) == FALSE) {
        crm_err("Node %s is not a member", join_from);
        ack_nack_bool = FALSE;

    } else if (generation == NULL) {
        crm_err("Generation was NULL");
        ack_nack_bool = FALSE;

    } else if ((join_version == NULL)
               || !feature_set_compatible(CRM_FEATURE_SET, join_version)) {
        crm_err("Node %s feature set (%s) is incompatible with ours (%s)",
                join_from, (join_version? join_version : "pre-3.1.0"),
                CRM_FEATURE_SET);
        ack_nack_bool = FALSE;

    } else if (max_generation_xml == NULL) {
        max_generation_xml = copy_xml(generation);
        max_generation_from = strdup(join_from);

    } else if (cmp < 0 || (cmp == 0 && safe_str_eq(join_from, fsa_our_uname))) {
        crm_debug("%s has a better generation number than"
                  " the current max %s", join_from, max_generation_from);
        if (max_generation_xml) {
            crm_log_xml_debug(max_generation_xml, "Max generation");
        }
        crm_log_xml_debug(generation, "Their generation");

        free(max_generation_from);
        free_xml(max_generation_xml);

        max_generation_from = strdup(join_from);
        max_generation_xml = copy_xml(join_ack->xml);
    }

    if (ack_nack_bool == FALSE) {
        /* NACK this client */
        ack_nack = CRMD_JOINSTATE_NACK;
        crm_update_peer_join(__FUNCTION__, join_node, crm_join_nack);
        crm_err("Rejecting cluster join request from %s " CRM_XS
                " NACK join-%d ref=%s", join_from, join_id, ref);

    } else {
        crm_debug("join-%d: Welcoming node %s (ref %s)", join_id, join_from, ref);
        crm_update_peer_join(__FUNCTION__, join_node, crm_join_integrated);
    }

    crm_update_peer_expected(__FUNCTION__, join_node, ack_nack);

    crm_debug("%u nodes have been integrated into join-%d",
              crmd_join_phase_count(crm_join_integrated), join_id);


    if (check_join_state(cur_state, __FUNCTION__) == FALSE) {
        // Don't waste time by invoking the scheduler yet
        crm_debug("join-%d: Still waiting on %d outstanding offers",
                  join_id, crmd_join_phase_count(crm_join_welcomed));
    }
}

/*	A_DC_JOIN_FINALIZE	*/
void
do_dc_join_finalize(long long action,
                    enum crmd_fsa_cause cause,
                    enum crmd_fsa_state cur_state,
                    enum crmd_fsa_input current_input, fsa_data_t * msg_data)
{
    char *sync_from = NULL;
    int rc = pcmk_ok;

    /* This we can do straight away and avoid clients timing us out
     *  while we compute the latest CIB
     */
    crm_debug("Finalizing join-%d for %d clients",
              current_join_id, crmd_join_phase_count(crm_join_integrated));

    crmd_join_phase_log(LOG_INFO);
    if (crmd_join_phase_count(crm_join_welcomed) != 0) {
        crm_info("Waiting for %d more nodes", crmd_join_phase_count(crm_join_welcomed));
        /* crmd_fsa_stall(FALSE); Needed? */
        return;

    } else if (crmd_join_phase_count(crm_join_integrated) == 0) {
        /* Nothing to do */
        check_join_state(fsa_state, __FUNCTION__);
        return;
    }

    clear_bit(fsa_input_register, R_HAVE_CIB);
    if (max_generation_from == NULL || safe_str_eq(max_generation_from, fsa_our_uname)) {
        set_bit(fsa_input_register, R_HAVE_CIB);
    }

    if (is_set(fsa_input_register, R_IN_TRANSITION)) {
        crm_warn("Delaying response to cluster join offer while transition in progress "
                 CRM_XS " join-%d", current_join_id);
        crmd_fsa_stall(FALSE);
        return;
    }

    if (max_generation_from && is_set(fsa_input_register, R_HAVE_CIB) == FALSE) {
        /* ask for the agreed best CIB */
        sync_from = strdup(max_generation_from);
        set_bit(fsa_input_register, R_CIB_ASKED);
        crm_notice("Syncing the Cluster Information Base from %s to rest of cluster "
                   CRM_XS " join-%d", sync_from, current_join_id);
        crm_log_xml_notice(max_generation_xml, "Requested version");

    } else {
        /* Send _our_ CIB out to everyone */
        sync_from = strdup(fsa_our_uname);
        crm_info("join-%d: Syncing our CIB to the rest of the cluster",
                 current_join_id);
        crm_log_xml_debug(max_generation_xml, "Requested version");
    }


    rc = fsa_cib_conn->cmds->sync_from(fsa_cib_conn, sync_from, NULL, cib_quorum_override);
    fsa_register_cib_callback(rc, FALSE, sync_from, finalize_sync_callback);
}

void
finalize_sync_callback(xmlNode * msg, int call_id, int rc, xmlNode * output, void *user_data)
{
    CRM_LOG_ASSERT(-EPERM != rc);
    clear_bit(fsa_input_register, R_CIB_ASKED);
    if (rc != pcmk_ok) {
        do_crm_log((rc == -pcmk_err_old_data ? LOG_WARNING : LOG_ERR),
                   "Sync from %s failed: %s", (char *)user_data, pcmk_strerror(rc));

        /* restart the whole join process */
        register_fsa_error_adv(C_FSA_INTERNAL, I_ELECTION_DC, NULL, NULL, __FUNCTION__);

    } else if (AM_I_DC && fsa_state == S_FINALIZE_JOIN) {
        set_bit(fsa_input_register, R_HAVE_CIB);
        clear_bit(fsa_input_register, R_CIB_ASKED);

        /* make sure dc_uuid is re-set to us */
        if (check_join_state(fsa_state, __FUNCTION__) == FALSE) {
            crm_debug("Notifying %d clients of join-%d results",
                      crmd_join_phase_count(crm_join_integrated), current_join_id);
            g_hash_table_foreach(crm_peer_cache, finalize_join_for, NULL);
        }

    } else {
        crm_debug("No longer the DC in S_FINALIZE_JOIN: %s in %s",
                  AM_I_DC ? "DC" : "controller", fsa_state2string(fsa_state));
    }
}

static void
join_update_complete_callback(xmlNode * msg, int call_id, int rc, xmlNode * output, void *user_data)
{
    fsa_data_t *msg_data = NULL;

    if (rc == pcmk_ok) {
        crm_debug("Join update %d complete", call_id);
        check_join_state(fsa_state, __FUNCTION__);

    } else {
        crm_err("Join update %d failed", call_id);
        crm_log_xml_debug(msg, "failed");
        register_fsa_error(C_FSA_INTERNAL, I_ERROR, NULL);
    }
}

/*	A_DC_JOIN_PROCESS_ACK	*/
void
do_dc_join_ack(long long action,
               enum crmd_fsa_cause cause,
               enum crmd_fsa_state cur_state,
               enum crmd_fsa_input current_input, fsa_data_t * msg_data)
{
    int join_id = -1;
    int call_id = 0;
    ha_msg_input_t *join_ack = fsa_typed_data(fsa_dt_ha_msg);

    const char *op = crm_element_value(join_ack->msg, F_CRM_TASK);
    const char *join_from = crm_element_value(join_ack->msg, F_CRM_HOST_FROM);
    crm_node_t *peer = crm_get_peer(0, join_from);

    if (safe_str_neq(op, CRM_OP_JOIN_CONFIRM) || peer == NULL) {
        crm_debug("Ignoring op=%s message from %s", op, join_from);
        return;
    }

    crm_trace("Processing ack from %s", join_from);
    crm_element_value_int(join_ack->msg, F_CRM_JOIN_ID, &join_id);

    if (peer->join != crm_join_finalized) {
        crm_info("Join not in progress: ignoring join-%d from %s (phase = %d)",
                 join_id, join_from, peer->join);
        return;

    } else if (join_id != current_join_id) {
        crm_err("Invalid response from %s: join-%d vs. join-%d",
                join_from, join_id, current_join_id);
        crm_update_peer_join(__FUNCTION__, peer, crm_join_nack);
        return;
    }

    crm_update_peer_join(__FUNCTION__, peer, crm_join_confirmed);

    crm_info("join-%d: Updating node state to %s for %s",
             join_id, CRMD_JOINSTATE_MEMBER, join_from);

    /* update CIB with the current LRM status from the node
     * We don't need to notify the TE of these updates, a transition will
     *   be started in due time
     */
    erase_status_tag(join_from, XML_CIB_TAG_LRM, cib_scope_local);

    if (safe_str_eq(join_from, fsa_our_uname)) {
        xmlNode *now_dc_lrmd_state = do_lrm_query(TRUE, fsa_our_uname);

        if (now_dc_lrmd_state != NULL) {
            crm_debug("Local executor state updated from query");
            fsa_cib_update(XML_CIB_TAG_STATUS, now_dc_lrmd_state,
                cib_scope_local | cib_quorum_override | cib_can_create, call_id, NULL);
            free_xml(now_dc_lrmd_state);
        } else {
            crm_warn("Local executor state updated from join acknowledgement because query failed");
            fsa_cib_update(XML_CIB_TAG_STATUS, join_ack->xml,
                cib_scope_local | cib_quorum_override | cib_can_create, call_id, NULL);
        }
    } else {
        crm_debug("Executor state for %s updated from join acknowledgement",
                  join_from);
        fsa_cib_update(XML_CIB_TAG_STATUS, join_ack->xml,
           cib_scope_local | cib_quorum_override | cib_can_create, call_id, NULL);
    }

    fsa_register_cib_callback(call_id, FALSE, NULL, join_update_complete_callback);
    crm_debug("join-%d: Registered callback for CIB status update %d", join_id, call_id);
}

void
finalize_join_for(gpointer key, gpointer value, gpointer user_data)
{
    xmlNode *acknak = NULL;
    xmlNode *tmp1 = NULL;
    crm_node_t *join_node = value;
    const char *join_to = join_node->uname;

    if(join_node->join != crm_join_integrated) {
        crm_trace("Skipping %s in state %d", join_to, join_node->join);
        return;
    }

    /* make sure a node entry exists for the new node */
    crm_trace("Creating node entry for %s", join_to);

    tmp1 = create_xml_node(NULL, XML_CIB_TAG_NODE);
    set_uuid(tmp1, XML_ATTR_UUID, join_node);
    crm_xml_add(tmp1, XML_ATTR_UNAME, join_to);

    fsa_cib_anon_update(XML_CIB_TAG_NODES, tmp1);
    free_xml(tmp1);

    join_node = crm_get_peer(0, join_to);
    if (crm_is_peer_active(join_node) == FALSE) {
        /*
         * NACK'ing nodes that the membership layer doesn't know about yet
         * simply creates more churn
         *
         * Better to leave them waiting and let the join restart when
         * the new membership event comes in
         *
         * All other NACKs (due to versions etc) should still be processed
         */
        crm_update_peer_expected(__FUNCTION__, join_node, CRMD_JOINSTATE_PENDING);
        return;
    }

    /* send the ack/nack to the node */
    acknak = create_dc_message(CRM_OP_JOIN_ACKNAK, join_to);

    crm_debug("join-%d: ACK'ing join request from %s",
              current_join_id, join_to);
    crm_xml_add(acknak, CRM_OP_JOIN_ACKNAK, XML_BOOLEAN_TRUE);
    crm_update_peer_join(__FUNCTION__, join_node, crm_join_finalized);
    crm_update_peer_expected(__FUNCTION__, join_node, CRMD_JOINSTATE_MEMBER);

    send_cluster_message(crm_get_peer(0, join_to), crm_msg_crmd, acknak, TRUE);
    free_xml(acknak);
    return;
}

gboolean
check_join_state(enum crmd_fsa_state cur_state, const char *source)
{
    static unsigned long long highest_seq = 0;

    crm_debug("Invoked by %s in state: %s", source, fsa_state2string(cur_state));

    if (saved_ccm_membership_id != crm_peer_seq) {
        crm_debug("%s: Membership changed since join started: %llu -> %llu (%llu)",
                  source, saved_ccm_membership_id, crm_peer_seq, highest_seq);
        if(highest_seq < crm_peer_seq) {
            /* Don't spam the FSA with duplicates */
            highest_seq = crm_peer_seq;
            register_fsa_input_before(C_FSA_INTERNAL, I_NODE_JOIN, NULL);
        }

    } else if (cur_state == S_INTEGRATION) {
        if (crmd_join_phase_count(crm_join_welcomed) == 0) {
            crm_debug("join-%d: Integration of %d peers complete: %s",
                      current_join_id, crmd_join_phase_count(crm_join_integrated), source);
            register_fsa_input_before(C_FSA_INTERNAL, I_INTEGRATED, NULL);
            return TRUE;
        }

    } else if (cur_state == S_FINALIZE_JOIN) {
        if (is_set(fsa_input_register, R_HAVE_CIB) == FALSE) {
            crm_debug("join-%d: Delaying I_FINALIZED until we have the CIB", current_join_id);
            return TRUE;

        } else if (crmd_join_phase_count(crm_join_welcomed) != 0) {
            crm_debug("join-%d: Still waiting on %d welcomed nodes",
                      current_join_id, crmd_join_phase_count(crm_join_welcomed));
            crmd_join_phase_log(LOG_DEBUG);

        } else if (crmd_join_phase_count(crm_join_integrated) != 0) {
            crm_debug("join-%d: Still waiting on %d integrated nodes",
                      current_join_id, crmd_join_phase_count(crm_join_integrated));
            crmd_join_phase_log(LOG_DEBUG);

        } else if (crmd_join_phase_count(crm_join_finalized) != 0) {
            crm_debug("join-%d: Still waiting on %d finalized nodes",
                      current_join_id, crmd_join_phase_count(crm_join_finalized));
            crmd_join_phase_log(LOG_DEBUG);

        } else {
            crm_debug("join-%d complete: %s", current_join_id, source);
            register_fsa_input_later(C_FSA_INTERNAL, I_FINALIZED, NULL);
            return TRUE;
        }
    }

    return FALSE;
}

void
do_dc_join_final(long long action,
                 enum crmd_fsa_cause cause,
                 enum crmd_fsa_state cur_state,
                 enum crmd_fsa_input current_input, fsa_data_t * msg_data)
{
    crm_debug("Ensuring DC, quorum and node attributes are up-to-date");
    crm_update_quorum(crm_have_quorum, TRUE);
}

int crmd_join_phase_count(enum crm_join_phase phase)
{
    int count = 0;
    crm_node_t *peer;
    GHashTableIter iter;

    g_hash_table_iter_init(&iter, crm_peer_cache);
    while (g_hash_table_iter_next(&iter, NULL, (gpointer *) &peer)) {
        if(peer->join == phase) {
            count++;
        }
    }
    return count;
}

void crmd_join_phase_log(int level)
{
    crm_node_t *peer;
    GHashTableIter iter;

    g_hash_table_iter_init(&iter, crm_peer_cache);
    while (g_hash_table_iter_next(&iter, NULL, (gpointer *) &peer)) {
        do_crm_log(level, "join-%d: %s=%s", current_join_id, peer->uname,
                   crm_join_phase_str(peer->join));
    }
}
