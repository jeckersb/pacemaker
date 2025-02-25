/*
 * Copyright 2004-2019 the Pacemaker project contributors
 *
 * The version control history for this file may have further details.
 *
 * This source code is licensed under the GNU Lesser General Public License
 * version 2.1 or later (LGPLv2.1+) WITHOUT ANY WARRANTY.
 */

#include <controld_messages.h>
#include <controld_metadata.h>

extern gboolean verify_stopped(enum crmd_fsa_state cur_state, int log_level);
void lrm_clear_last_failure(const char *rsc_id, const char *node_name,
                            const char *operation, guint interval_ms);
void lrm_op_callback(lrmd_event_data_t * op);
lrmd_t *crmd_local_lrmd_conn(void);

typedef struct resource_history_s {
    char *id;
    uint32_t last_callid;
    lrmd_rsc_info_t rsc;
    lrmd_event_data_t *last;
    lrmd_event_data_t *failed;
    GList *recurring_op_list;

    /* Resources must be stopped using the same
     * parameters they were started with.  This hashtable
     * holds the parameters that should be used for the next stop
     * cmd on this resource. */
    GHashTable *stop_params;
} rsc_history_t;

void history_free(gpointer data);

/* TODO - Replace this with lrmd_event_data_t */
struct recurring_op_s {
    guint interval_ms;
    int call_id;
    gboolean remove;
    gboolean cancelled;
    time_t start_time;
    char *rsc_id;
    char *op_type;
    char *op_key;
    char *user_data;
    GHashTable *params;
};

typedef struct lrm_state_s {
    const char *node_name;
    void *conn;                 // Reserved for controld_execd_state.c usage
    void *remote_ra_data;       // Reserved for controld_remote_ra.c usage

    GHashTable *resource_history;
    GHashTable *pending_ops;
    GHashTable *deletion_ops;
    GHashTable *rsc_info_cache;
    GHashTable *metadata_cache; // key = class[:provider]:agent, value = ra_metadata_s

    int num_lrm_register_fails;
} lrm_state_t;

struct pending_deletion_op_s {
    char *rsc;
    ha_msg_input_t *input;
};

/*!
 * \brief Check whether this the local IPC connection to the executor
 */
gboolean
lrm_state_is_local(lrm_state_t *lrm_state);

/*!
 * \brief Clear all state information from a single state entry.
 * \note It sometimes useful to save metadata cache when it won't go stale.
 * \note This does not close the executor connection
 */
void lrm_state_reset_tables(lrm_state_t * lrm_state, gboolean reset_metadata);
GList *lrm_state_get_list(void);

/*!
 * \brief Initiate internal state tables
 */
gboolean lrm_state_init_local(void);

/*!
 * \brief Destroy all state entries and internal state tables
 */
void lrm_state_destroy_all(void);

/*!
 * \brief Create executor connection entry
 */
lrm_state_t *lrm_state_create(const char *node_name);

/*!
 * \brief Destroy executor connection by node name
 */
void lrm_state_destroy(const char *node_name);

/*!
 * \brief Find lrm_state data by node name
 */
lrm_state_t *lrm_state_find(const char *node_name);

/*!
 * \brief Either find or create a new entry
 */
lrm_state_t *lrm_state_find_or_create(const char *node_name);

/*!
 * The functions below are wrappers for the executor API the the controller
 * uses. These wrapper functions allow us to treat the controller's remote
 * executor connection resources the same as regular resources. Internally,
 * regular resources go to the executor, and remote connection resources are
 * handled locally in the controller.
 */
void lrm_state_disconnect_only(lrm_state_t * lrm_state);
void lrm_state_disconnect(lrm_state_t * lrm_state);
int lrm_state_ipc_connect(lrm_state_t * lrm_state);
int lrm_state_remote_connect_async(lrm_state_t * lrm_state, const char *server, int port,
                                   int timeout);
int lrm_state_is_connected(lrm_state_t * lrm_state);
int lrm_state_poke_connection(lrm_state_t * lrm_state);

int lrm_state_get_metadata(lrm_state_t * lrm_state,
                           const char *class,
                           const char *provider,
                           const char *agent, char **output, enum lrmd_call_options options);
int lrm_state_cancel(lrm_state_t *lrm_state, const char *rsc_id,
                     const char *action, guint interval_ms);
int lrm_state_exec(lrm_state_t *lrm_state, const char *rsc_id,
                   const char *action, const char *userdata, guint interval_ms,
                   int timeout, /* ms */
                   int start_delay,     /* ms */
                   lrmd_key_value_t * params);
lrmd_rsc_info_t *lrm_state_get_rsc_info(lrm_state_t * lrm_state,
                                        const char *rsc_id, enum lrmd_call_options options);
int lrm_state_register_rsc(lrm_state_t * lrm_state,
                           const char *rsc_id,
                           const char *class,
                           const char *provider, const char *agent, enum lrmd_call_options options);
int lrm_state_unregister_rsc(lrm_state_t * lrm_state,
                             const char *rsc_id, enum lrmd_call_options options);

// Functions used to manage remote executor connection resources
void remote_lrm_op_callback(lrmd_event_data_t * op);
gboolean is_remote_lrmd_ra(const char *agent, const char *provider, const char *id);
lrmd_rsc_info_t *remote_ra_get_rsc_info(lrm_state_t * lrm_state, const char *rsc_id);
int remote_ra_cancel(lrm_state_t *lrm_state, const char *rsc_id,
                     const char *action, guint interval_ms);
int remote_ra_exec(lrm_state_t *lrm_state, const char *rsc_id,
                   const char *action, const char *userdata, guint interval_ms,
                   int timeout, /* ms */
                   int start_delay,     /* ms */
                   lrmd_key_value_t * params);
void remote_ra_cleanup(lrm_state_t * lrm_state);
void remote_ra_fail(const char *node_name);
void remote_ra_process_pseudo(xmlNode *xml);
gboolean remote_ra_is_in_maintenance(lrm_state_t * lrm_state);
void remote_ra_process_maintenance_nodes(xmlNode *xml);
gboolean remote_ra_controlling_guest(lrm_state_t * lrm_state);

void process_lrm_event(lrm_state_t *lrm_state, lrmd_event_data_t *op,
                       struct recurring_op_s *pending, xmlNode *action_xml);
