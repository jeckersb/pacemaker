/*
 * Copyright 2004-2019 the Pacemaker project contributors
 *
 * The version control history for this file may have further details.
 *
 * This source code is licensed under the GNU Lesser General Public License
 * version 2.1 or later (LGPLv2.1+) WITHOUT ANY WARRANTY.
 */

#include <crm_internal.h>

#if defined(US_AUTH_PEERCRED_UCRED) || defined(US_AUTH_PEERCRED_SOCKPEERCRED)
#  ifdef US_AUTH_PEERCRED_UCRED
#    ifndef _GNU_SOURCE
#      define _GNU_SOURCE
#    endif
#  endif
#  include <sys/socket.h>
#elif defined(US_AUTH_GETPEERUCRED)
#  include <ucred.h>
#endif

#include <sys/param.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <grp.h>

#include <errno.h>
#include <fcntl.h>
#include <bzlib.h>

#include <crm/crm.h>   /* indirectly: pcmk_err_generic */
#include <crm/msg_xml.h>
#include <crm/common/ipc.h>
#include <crm/common/ipcs.h>

#include <crm/common/ipc_internal.h>  /* PCMK__SPECIAL_PID* */

#define PCMK_IPC_VERSION 1

/* Evict clients whose event queue grows this large (by default) */
#define PCMK_IPC_DEFAULT_QUEUE_MAX 500

struct crm_ipc_response_header {
    struct qb_ipc_response_header qb;
    uint32_t size_uncompressed;
    uint32_t size_compressed;
    uint32_t flags;
    uint8_t  version; /* Protect against version changes for anyone that might bother to statically link us */
};

static int hdr_offset = 0;
static unsigned int ipc_buffer_max = 0;
static unsigned int pick_ipc_buffer(unsigned int max);

static inline void
crm_ipc_init(void)
{
    if (hdr_offset == 0) {
        hdr_offset = sizeof(struct crm_ipc_response_header);
    }
    if (ipc_buffer_max == 0) {
        ipc_buffer_max = pick_ipc_buffer(0);
    }
}

unsigned int
crm_ipc_default_buffer_size(void)
{
    return pick_ipc_buffer(0);
}

static char *
generateReference(const char *custom1, const char *custom2)
{
    static uint ref_counter = 0;

    return crm_strdup_printf("%s-%s-%lld-%u",
                             (custom1? custom1 : "_empty_"),
                             (custom2? custom2 : "_empty_"),
                             (long long) time(NULL), ref_counter++);
}

xmlNode *
create_request_adv(const char *task, xmlNode * msg_data,
                   const char *host_to, const char *sys_to,
                   const char *sys_from, const char *uuid_from, const char *origin)
{
    char *true_from = NULL;
    xmlNode *request = NULL;
    char *reference = generateReference(task, sys_from);

    if (uuid_from != NULL) {
        true_from = generate_hash_key(sys_from, uuid_from);
    } else if (sys_from != NULL) {
        true_from = strdup(sys_from);
    } else {
        crm_err("No sys from specified");
    }

    // host_from will get set for us if necessary by the controller when routed
    request = create_xml_node(NULL, __FUNCTION__);
    crm_xml_add(request, F_CRM_ORIGIN, origin);
    crm_xml_add(request, F_TYPE, T_CRM);
    crm_xml_add(request, F_CRM_VERSION, CRM_FEATURE_SET);
    crm_xml_add(request, F_CRM_MSG_TYPE, XML_ATTR_REQUEST);
    crm_xml_add(request, F_CRM_REFERENCE, reference);
    crm_xml_add(request, F_CRM_TASK, task);
    crm_xml_add(request, F_CRM_SYS_TO, sys_to);
    crm_xml_add(request, F_CRM_SYS_FROM, true_from);

    /* HOSTTO will be ignored if it is to the DC anyway. */
    if (host_to != NULL && strlen(host_to) > 0) {
        crm_xml_add(request, F_CRM_HOST_TO, host_to);
    }

    if (msg_data != NULL) {
        add_message_xml(request, F_CRM_DATA, msg_data);
    }
    free(reference);
    free(true_from);

    return request;
}

/*
 * This method adds a copy of xml_response_data
 */
xmlNode *
create_reply_adv(xmlNode * original_request, xmlNode * xml_response_data, const char *origin)
{
    xmlNode *reply = NULL;

    const char *host_from = crm_element_value(original_request, F_CRM_HOST_FROM);
    const char *sys_from = crm_element_value(original_request, F_CRM_SYS_FROM);
    const char *sys_to = crm_element_value(original_request, F_CRM_SYS_TO);
    const char *type = crm_element_value(original_request, F_CRM_MSG_TYPE);
    const char *operation = crm_element_value(original_request, F_CRM_TASK);
    const char *crm_msg_reference = crm_element_value(original_request, F_CRM_REFERENCE);

    if (type == NULL) {
        crm_err("Cannot create new_message, no message type in original message");
        CRM_ASSERT(type != NULL);
        return NULL;
#if 0
    } else if (strcasecmp(XML_ATTR_REQUEST, type) != 0) {
        crm_err("Cannot create new_message, original message was not a request");
        return NULL;
#endif
    }
    reply = create_xml_node(NULL, __FUNCTION__);
    if (reply == NULL) {
        crm_err("Cannot create new_message, malloc failed");
        return NULL;
    }

    crm_xml_add(reply, F_CRM_ORIGIN, origin);
    crm_xml_add(reply, F_TYPE, T_CRM);
    crm_xml_add(reply, F_CRM_VERSION, CRM_FEATURE_SET);
    crm_xml_add(reply, F_CRM_MSG_TYPE, XML_ATTR_RESPONSE);
    crm_xml_add(reply, F_CRM_REFERENCE, crm_msg_reference);
    crm_xml_add(reply, F_CRM_TASK, operation);

    /* since this is a reply, we reverse the from and to */
    crm_xml_add(reply, F_CRM_SYS_TO, sys_from);
    crm_xml_add(reply, F_CRM_SYS_FROM, sys_to);

    /* HOSTTO will be ignored if it is to the DC anyway. */
    if (host_from != NULL && strlen(host_from) > 0) {
        crm_xml_add(reply, F_CRM_HOST_TO, host_from);
    }

    if (xml_response_data != NULL) {
        add_message_xml(reply, F_CRM_DATA, xml_response_data);
    }

    return reply;
}

/* Libqb based IPC */

/* Server... */

GHashTable *client_connections = NULL;

crm_client_t *
crm_client_get(qb_ipcs_connection_t * c)
{
    if (client_connections) {
        return g_hash_table_lookup(client_connections, c);
    }

    crm_trace("No client found for %p", c);
    return NULL;
}

crm_client_t *
crm_client_get_by_id(const char *id)
{
    gpointer key;
    crm_client_t *client;
    GHashTableIter iter;

    if (client_connections && id) {
        g_hash_table_iter_init(&iter, client_connections);
        while (g_hash_table_iter_next(&iter, &key, (gpointer *) & client)) {
            if (strcmp(client->id, id) == 0) {
                return client;
            }
        }
    }

    crm_trace("No client found with id=%s", id);
    return NULL;
}

const char *
crm_client_name(crm_client_t * c)
{
    if (c == NULL) {
        return "null";
    } else if (c->name == NULL && c->id == NULL) {
        return "unknown";
    } else if (c->name == NULL) {
        return c->id;
    } else {
        return c->name;
    }
}

const char *
crm_client_type_text(enum client_type client_type)
{
    switch (client_type) {
        case CRM_CLIENT_IPC:
            return "IPC";
        case CRM_CLIENT_TCP:
            return "TCP";
#ifdef HAVE_GNUTLS_GNUTLS_H
        case CRM_CLIENT_TLS:
            return "TLS";
#endif
        default:
            return "unknown";
    }
}

void
crm_client_init(void)
{
    if (client_connections == NULL) {
        crm_trace("Creating client hash table");
        client_connections = g_hash_table_new(g_direct_hash, g_direct_equal);
    }
}

void
crm_client_cleanup(void)
{
    if (client_connections != NULL) {
        int active = g_hash_table_size(client_connections);

        if (active) {
            crm_err("Exiting with %d active connections", active);
        }
        g_hash_table_destroy(client_connections); client_connections = NULL;
    }
}

void
crm_client_disconnect_all(qb_ipcs_service_t *service)
{
    qb_ipcs_connection_t *c = NULL;

    if (service == NULL) {
        return;
    }

    c = qb_ipcs_connection_first_get(service);

    while (c != NULL) {
        qb_ipcs_connection_t *last = c;

        c = qb_ipcs_connection_next_get(service, last);

        /* There really shouldn't be anyone connected at this point */
        crm_notice("Disconnecting client %p, pid=%d...", last, crm_ipcs_client_pid(last));
        qb_ipcs_disconnect(last);
        qb_ipcs_connection_unref(last);
    }
}

/*!
 * \internal
 * \brief Allocate a new crm_client_t object based on an IPC connection
 *
 * \param[in] c           IPC connection (or NULL to allocate generic client)
 * \param[in] key         Connection table key (or NULL to use sane default)
 * \param[in] uid_client  UID corresponding to c (ignored if c is NULL)
 *
 * \return Pointer to new crm_client_t (or NULL on error)
 */
static crm_client_t *
client_from_connection(qb_ipcs_connection_t *c, void *key, uid_t uid_client)
{
    crm_client_t *client = calloc(1, sizeof(crm_client_t));

    if (client == NULL) {
        crm_perror(LOG_ERR, "Allocating client");
        return NULL;
    }

    if (c) {
#if ENABLE_ACL
        client->user = uid2username(uid_client);
        if (client->user == NULL) {
            client->user = strdup("#unprivileged");
            CRM_CHECK(client->user != NULL, free(client); return NULL);
            crm_err("Unable to enforce ACLs for user ID %d, assuming unprivileged",
                    uid_client);
        }
#endif
        client->ipcs = c;
        client->kind = CRM_CLIENT_IPC;
        client->pid = crm_ipcs_client_pid(c);
        if (key == NULL) {
            key = c;
        }
    }

    client->id = crm_generate_uuid();
    if (client->id == NULL) {
        crm_err("Could not generate UUID for client");
        free(client->user);
        free(client);
        return NULL;
    }
    if (key == NULL) {
        key = client->id;
    }
    g_hash_table_insert(client_connections, key, client);
    return client;
}

/*!
 * \brief Allocate a new crm_client_t object and generate its ID
 *
 * \param[in] key  What to use as connections hash table key (NULL to use ID)
 *
 * \return Pointer to new crm_client_t (asserts on failure)
 */
crm_client_t *
crm_client_alloc(void *key)
{
    crm_client_t *client = client_from_connection(NULL, key, 0);

    CRM_ASSERT(client != NULL);
    return client;
}

crm_client_t *
crm_client_new(qb_ipcs_connection_t * c, uid_t uid_client, gid_t gid_client)
{
    static gid_t uid_cluster = 0;
    static gid_t gid_cluster = 0;

    crm_client_t *client = NULL;

    CRM_CHECK(c != NULL, return NULL);

    if (uid_cluster == 0) {
        if (crm_user_lookup(CRM_DAEMON_USER, &uid_cluster, &gid_cluster) < 0) {
            static bool need_log = TRUE;

            if (need_log) {
                crm_warn("Could not find user and group IDs for user %s",
                         CRM_DAEMON_USER);
                need_log = FALSE;
            }
        }
    }

    if (uid_client != 0) {
        crm_trace("Giving access to group %u", gid_cluster);
        /* Passing -1 to chown(2) means don't change */
        qb_ipcs_connection_auth_set(c, -1, gid_cluster, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    }

    crm_client_init();

    /* TODO: Do our own auth checking, return NULL if unauthorized */
    client = client_from_connection(c, NULL, uid_client);
    if (client == NULL) {
        return NULL;
    }

    if ((uid_client == 0) || (uid_client == uid_cluster)) {
        /* Remember when a connection came from root or hacluster */
        set_bit(client->flags, crm_client_flag_ipc_privileged);
    }

    crm_debug("Connecting %p for uid=%d gid=%d pid=%u id=%s", c, uid_client, gid_client, client->pid, client->id);

    return client;
}

static struct iovec *
pcmk__new_ipc_event()
{
    struct iovec *iov = calloc(2, sizeof(struct iovec));

    CRM_ASSERT(iov != NULL);
    return iov;
}

/*!
 * \brief Free an I/O vector created by crm_ipc_prepare()
 *
 * \param[in] event  I/O vector to free
 */
void
pcmk_free_ipc_event(struct iovec *event)
{
    if (event != NULL) {
        free(event[0].iov_base);
        free(event[1].iov_base);
        free(event);
    }
}

static void
free_event(gpointer data)
{
    pcmk_free_ipc_event((struct iovec *) data);
}

static void
add_event(crm_client_t *c, struct iovec *iov)
{
    if (c->event_queue == NULL) {
        c->event_queue = g_queue_new();
    }
    g_queue_push_tail(c->event_queue, iov);
}

void
crm_client_destroy(crm_client_t * c)
{
    if (c == NULL) {
        return;
    }

    if (client_connections) {
        if (c->ipcs) {
            crm_trace("Destroying %p/%p (%d remaining)",
                      c, c->ipcs, crm_hash_table_size(client_connections) - 1);
            g_hash_table_remove(client_connections, c->ipcs);

        } else {
            crm_trace("Destroying remote connection %p (%d remaining)",
                      c, crm_hash_table_size(client_connections) - 1);
            g_hash_table_remove(client_connections, c->id);
        }
    }

    if (c->event_timer) {
        g_source_remove(c->event_timer);
    }

    if (c->event_queue) {
        crm_debug("Destroying %d events", g_queue_get_length(c->event_queue));
        g_queue_free_full(c->event_queue, free_event);
    }

    free(c->id);
    free(c->name);
    free(c->user);
    if (c->remote) {
        if (c->remote->auth_timeout) {
            g_source_remove(c->remote->auth_timeout);
        }
        free(c->remote->buffer);
        free(c->remote);
    }
    free(c);
}

/*!
 * \brief Raise IPC eviction threshold for a client, if allowed
 *
 * \param[in,out] client     Client to modify
 * \param[in]     queue_max  New threshold (as string)
 *
 * \return TRUE if change was allowed, FALSE otherwise
 */
bool
crm_set_client_queue_max(crm_client_t *client, const char *qmax)
{
    if (is_set(client->flags, crm_client_flag_ipc_privileged)) {
        int qmax_int = crm_int_helper(qmax, NULL);

        if ((errno == 0) && (qmax_int > 0)) {
            client->queue_max = qmax_int;
            return TRUE;
        }
    }
    return FALSE;
}

int
crm_ipcs_client_pid(qb_ipcs_connection_t * c)
{
    struct qb_ipcs_connection_stats stats;

    stats.client_pid = 0;
    qb_ipcs_connection_stats_get(c, &stats, 0);
    return stats.client_pid;
}

xmlNode *
crm_ipcs_recv(crm_client_t * c, void *data, size_t size, uint32_t * id, uint32_t * flags)
{
    xmlNode *xml = NULL;
    char *uncompressed = NULL;
    char *text = ((char *)data) + sizeof(struct crm_ipc_response_header);
    struct crm_ipc_response_header *header = data;

    if (id) {
        *id = ((struct qb_ipc_response_header *)data)->id;
    }
    if (flags) {
        *flags = header->flags;
    }

    if (is_set(header->flags, crm_ipc_proxied)) {
        /* Mark this client as being the endpoint of a proxy connection.
         * Proxy connections responses are sent on the event channel, to avoid
         * blocking the controller serving as proxy.
         */
        c->flags |= crm_client_flag_ipc_proxied;
    }

    if(header->version > PCMK_IPC_VERSION) {
        crm_err("Filtering incompatible v%d IPC message, we only support versions <= %d",
                header->version, PCMK_IPC_VERSION);
        return NULL;
    }

    if (header->size_compressed) {
        int rc = 0;
        unsigned int size_u = 1 + header->size_uncompressed;
        uncompressed = calloc(1, size_u);

        crm_trace("Decompressing message data %u bytes into %u bytes",
                  header->size_compressed, size_u);

        rc = BZ2_bzBuffToBuffDecompress(uncompressed, &size_u, text, header->size_compressed, 1, 0);
        text = uncompressed;

        if (rc != BZ_OK) {
            crm_err("Decompression failed: %s " CRM_XS " bzerror=%d",
                    bz2_strerror(rc), rc);
            free(uncompressed);
            return NULL;
        }
    }

    CRM_ASSERT(text[header->size_uncompressed - 1] == 0);

    crm_trace("Received %.200s", text);
    xml = string2xml(text);

    free(uncompressed);
    return xml;
}

ssize_t crm_ipcs_flush_events(crm_client_t * c);

static gboolean
crm_ipcs_flush_events_cb(gpointer data)
{
    crm_client_t *c = data;

    c->event_timer = 0;
    crm_ipcs_flush_events(c);
    return FALSE;
}

/*!
 * \internal
 * \brief Add progressive delay before next event queue flush
 *
 * \param[in,out] c          Client connection to add delay to
 * \param[in]     queue_len  Current event queue length
 */
static inline void
delay_next_flush(crm_client_t *c, unsigned int queue_len)
{
    /* Delay a maximum of 1.5 seconds */
    guint delay = (queue_len < 5)? (1000 + 100 * queue_len) : 1500;

    c->event_timer = g_timeout_add(delay, crm_ipcs_flush_events_cb, c);
}

ssize_t
crm_ipcs_flush_events(crm_client_t * c)
{
    ssize_t rc = 0;
    unsigned int sent = 0;
    unsigned int queue_len = 0;

    if (c == NULL) {
        return pcmk_ok;

    } else if (c->event_timer) {
        /* There is already a timer, wait until it goes off */
        crm_trace("Timer active for %p - %d", c->ipcs, c->event_timer);
        return pcmk_ok;
    }

    if (c->event_queue) {
        queue_len = g_queue_get_length(c->event_queue);
    }
    while (sent < 100) {
        struct crm_ipc_response_header *header = NULL;
        struct iovec *event = NULL;

        if (c->event_queue) {
            // We don't pop unless send is successful
            event = g_queue_peek_head(c->event_queue);
        }
        if (event == NULL) { // Queue is empty
            break;
        }

        rc = qb_ipcs_event_sendv(c->ipcs, event, 2);
        if (rc < 0) {
            break;
        }
        event = g_queue_pop_head(c->event_queue);

        sent++;
        header = event[0].iov_base;
        if (header->size_compressed) {
            crm_trace("Event %d to %p[%d] (%lld compressed bytes) sent",
                      header->qb.id, c->ipcs, c->pid, (long long) rc);
        } else {
            crm_trace("Event %d to %p[%d] (%lld bytes) sent: %.120s",
                      header->qb.id, c->ipcs, c->pid, (long long) rc,
                      (char *) (event[1].iov_base));
        }
        pcmk_free_ipc_event(event);
    }

    queue_len -= sent;
    if (sent > 0 || queue_len) {
        crm_trace("Sent %d events (%d remaining) for %p[%d]: %s (%lld)",
                  sent, queue_len, c->ipcs, c->pid,
                  pcmk_strerror(rc < 0 ? rc : 0), (long long) rc);
    }

    if (queue_len) {

        /* Allow clients to briefly fall behind on processing incoming messages,
         * but drop completely unresponsive clients so the connection doesn't
         * consume resources indefinitely.
         */
        if (queue_len > QB_MAX(c->queue_max, PCMK_IPC_DEFAULT_QUEUE_MAX)) {
            if ((c->queue_backlog <= 1) || (queue_len < c->queue_backlog)) {
                /* Don't evict for a new or shrinking backlog */
                crm_warn("Client with process ID %u has a backlog of %u messages "
                         CRM_XS " %p", c->pid, queue_len, c->ipcs);
            } else {
                crm_err("Evicting client with process ID %u due to backlog of %u messages "
                         CRM_XS " %p", c->pid, queue_len, c->ipcs);
                c->queue_backlog = 0;
                qb_ipcs_disconnect(c->ipcs);
                return rc;
            }
        }

        c->queue_backlog = queue_len;
        delay_next_flush(c, queue_len);

    } else {
        /* Event queue is empty, there is no backlog */
        c->queue_backlog = 0;
    }

    return rc;
}

ssize_t
crm_ipc_prepare(uint32_t request, xmlNode * message, struct iovec ** result, uint32_t max_send_size)
{
    static unsigned int biggest = 0;
    struct iovec *iov;
    unsigned int total = 0;
    char *compressed = NULL;
    char *buffer = dump_xml_unformatted(message);
    struct crm_ipc_response_header *header = calloc(1, sizeof(struct crm_ipc_response_header));

    CRM_ASSERT(result != NULL);

    crm_ipc_init();

    if (max_send_size == 0) {
        max_send_size = ipc_buffer_max;
    }

    CRM_LOG_ASSERT(max_send_size != 0);

    *result = NULL;
    iov = pcmk__new_ipc_event();
    iov[0].iov_len = hdr_offset;
    iov[0].iov_base = header;

    header->version = PCMK_IPC_VERSION;
    header->size_uncompressed = 1 + strlen(buffer);
    total = iov[0].iov_len + header->size_uncompressed;

    if (total < max_send_size) {
        iov[1].iov_base = buffer;
        iov[1].iov_len = header->size_uncompressed;

    } else {
        unsigned int new_size = 0;

        if (crm_compress_string
            (buffer, header->size_uncompressed, max_send_size, &compressed, &new_size)) {

            header->flags |= crm_ipc_compressed;
            header->size_compressed = new_size;

            iov[1].iov_len = header->size_compressed;
            iov[1].iov_base = compressed;

            free(buffer);

            biggest = QB_MAX(header->size_compressed, biggest);

        } else {
            ssize_t rc = -EMSGSIZE;

            crm_log_xml_trace(message, "EMSGSIZE");
            biggest = QB_MAX(header->size_uncompressed, biggest);

            crm_err
                ("Could not compress the message (%u bytes) into less than the configured ipc limit (%u bytes). "
                 "Set PCMK_ipc_buffer to a higher value (%u bytes suggested)",
                 header->size_uncompressed, max_send_size, 4 * biggest);

            free(compressed);
            pcmk_free_ipc_event(iov);
            return rc;
        }
    }

    header->qb.size = iov[0].iov_len + iov[1].iov_len;
    header->qb.id = (int32_t)request;    /* Replying to a specific request */

    *result = iov;
    CRM_ASSERT(header->qb.size > 0);
    return header->qb.size;
}

ssize_t
crm_ipcs_sendv(crm_client_t * c, struct iovec * iov, enum crm_ipc_flags flags)
{
    ssize_t rc;
    static uint32_t id = 1;
    struct crm_ipc_response_header *header = iov[0].iov_base;

    if (c->flags & crm_client_flag_ipc_proxied) {
        /* _ALL_ replies to proxied connections need to be sent as events */
        if (is_not_set(flags, crm_ipc_server_event)) {
            flags |= crm_ipc_server_event;
            /* this flag lets us know this was originally meant to be a response.
             * even though we're sending it over the event channel. */
            flags |= crm_ipc_proxied_relay_response;
        }
    }

    header->flags |= flags;
    if (flags & crm_ipc_server_event) {
        header->qb.id = id++;   /* We don't really use it, but doesn't hurt to set one */

        if (flags & crm_ipc_server_free) {
            crm_trace("Sending the original to %p[%d]", c->ipcs, c->pid);
            add_event(c, iov);

        } else {
            struct iovec *iov_copy = pcmk__new_ipc_event();

            crm_trace("Sending a copy to %p[%d]", c->ipcs, c->pid);
            iov_copy[0].iov_len = iov[0].iov_len;
            iov_copy[0].iov_base = malloc(iov[0].iov_len);
            memcpy(iov_copy[0].iov_base, iov[0].iov_base, iov[0].iov_len);

            iov_copy[1].iov_len = iov[1].iov_len;
            iov_copy[1].iov_base = malloc(iov[1].iov_len);
            memcpy(iov_copy[1].iov_base, iov[1].iov_base, iov[1].iov_len);

            add_event(c, iov_copy);
        }

    } else {
        CRM_LOG_ASSERT(header->qb.id != 0);     /* Replying to a specific request */

        rc = qb_ipcs_response_sendv(c->ipcs, iov, 2);
        if (rc < header->qb.size) {
            crm_notice("Response %d to pid %d failed: %s "
                       CRM_XS " bytes=%u rc=%lld ipcs=%p",
                       header->qb.id, c->pid, pcmk_strerror(rc),
                       header->qb.size, (long long) rc, c->ipcs);

        } else {
            crm_trace("Response %d sent, %lld bytes to %p[%d]",
                      header->qb.id, (long long) rc, c->ipcs, c->pid);
        }

        if (flags & crm_ipc_server_free) {
            pcmk_free_ipc_event(iov);
        }
    }

    if (flags & crm_ipc_server_event) {
        rc = crm_ipcs_flush_events(c);
    } else {
        crm_ipcs_flush_events(c);
    }

    if (rc == -EPIPE || rc == -ENOTCONN) {
        crm_trace("Client %p disconnected", c->ipcs);
    }

    return rc;
}

ssize_t
crm_ipcs_send(crm_client_t * c, uint32_t request, xmlNode * message,
              enum crm_ipc_flags flags)
{
    struct iovec *iov = NULL;
    ssize_t rc = 0;

    if(c == NULL) {
        return -EDESTADDRREQ;
    }
    crm_ipc_init();

    rc = crm_ipc_prepare(request, message, &iov, ipc_buffer_max);
    if (rc > 0) {
        rc = crm_ipcs_sendv(c, iov, flags | crm_ipc_server_free);
    } else {
        pcmk_free_ipc_event(iov);
        crm_notice("Message to pid %d failed: %s " CRM_XS " rc=%lld ipcs=%p",
                   c->pid, pcmk_strerror(rc), (long long) rc, c->ipcs);
    }
    return rc;
}

void
crm_ipcs_send_ack(crm_client_t * c, uint32_t request, uint32_t flags, const char *tag, const char *function,
                  int line)
{
    if (flags & crm_ipc_client_response) {
        xmlNode *ack = create_xml_node(NULL, tag);

        crm_trace("Ack'ing msg from %s (%p)", crm_client_name(c), c);
        c->request_id = 0;
        crm_xml_add(ack, "function", function);
        crm_xml_add_int(ack, "line", line);
        crm_ipcs_send(c, request, ack, flags);
        free_xml(ack);
    }
}

/* Client... */

#define MIN_MSG_SIZE    12336   /* sizeof(struct qb_ipc_connection_response) */
#define MAX_MSG_SIZE    128*1024 /* 128k default */

struct crm_ipc_s {
    struct pollfd pfd;

    /* the max size we can send/receive over ipc */
    unsigned int max_buf_size;
    /* Size of the allocated 'buffer' */
    unsigned int buf_size;
    int msg_size;
    int need_reply;
    char *buffer;
    char *name;

    qb_ipcc_connection_t *ipc;

};

static unsigned int
pick_ipc_buffer(unsigned int max)
{
    static unsigned int global_max = 0;

    if (global_max == 0) {
        const char *env = getenv("PCMK_ipc_buffer");

        if (env) {
            int env_max = crm_parse_int(env, "0");

            global_max = (env_max > 0)? QB_MAX(MIN_MSG_SIZE, env_max) : MAX_MSG_SIZE;

        } else {
            global_max = MAX_MSG_SIZE;
        }
    }

    return QB_MAX(max, global_max);
}

crm_ipc_t *
crm_ipc_new(const char *name, size_t max_size)
{
    crm_ipc_t *client = NULL;

    client = calloc(1, sizeof(crm_ipc_t));

    client->name = strdup(name);
    client->buf_size = pick_ipc_buffer(max_size);
    client->buffer = malloc(client->buf_size);

    /* Clients initiating connection pick the max buf size */
    client->max_buf_size = client->buf_size;

    client->pfd.fd = -1;
    client->pfd.events = POLLIN;
    client->pfd.revents = 0;

    return client;
}

/*!
 * \brief Establish an IPC connection to a Pacemaker component
 *
 * \param[in] client  Connection instance obtained from crm_ipc_new()
 *
 * \return TRUE on success, FALSE otherwise (in which case errno will be set;
 *         specifically, in case of discovering the remote side is not
 *         authentic, its value is set to ECONNABORTED).
 */
bool
crm_ipc_connect(crm_ipc_t * client)
{
    static uid_t cl_uid = 0;
    static gid_t cl_gid = 0;
    pid_t found_pid = 0; uid_t found_uid = 0; gid_t found_gid = 0;
    int rv;

    client->need_reply = FALSE;
    client->ipc = qb_ipcc_connect(client->name, client->buf_size);

    if (client->ipc == NULL) {
        crm_debug("Could not establish %s connection: %s (%d)", client->name, pcmk_strerror(errno), errno);
        return FALSE;
    }

    client->pfd.fd = crm_ipc_get_fd(client);
    if (client->pfd.fd < 0) {
        rv = errno;
        /* message already omitted */
        crm_ipc_close(client);
        errno = rv;
        return FALSE;
    }

    if (!cl_uid && !cl_gid
            && (rv = crm_user_lookup(CRM_DAEMON_USER, &cl_uid, &cl_gid)) < 0) {
        errno = -rv;
        /* message already omitted */
        crm_ipc_close(client);
        errno = -rv;
        return FALSE;
    }

    if (!(rv = crm_ipc_is_authentic_process(client->pfd.fd, cl_uid, cl_gid,
                                            &found_pid, &found_uid,
                                            &found_gid))) {
        crm_err("Daemon (IPC %s) is not authentic:"
                " process %lld (uid: %lld, gid: %lld)",
                client->name,  (long long) PCMK__SPECIAL_PID_AS_0(found_pid),
                (long long) found_uid, (long long) found_gid);
        crm_ipc_close(client);
        errno = ECONNABORTED;
        return FALSE;

    } else if (rv < 0) {
        errno = -rv;
        crm_perror(LOG_ERR, "Could not verify authenticity of daemon (IPC %s)",
                   client->name);
        crm_ipc_close(client);
        errno = -rv;
        return FALSE;
    }

    qb_ipcc_context_set(client->ipc, client);

#ifdef HAVE_IPCS_GET_BUFFER_SIZE
    client->max_buf_size = qb_ipcc_get_buffer_size(client->ipc);
    if (client->max_buf_size > client->buf_size) {
        free(client->buffer);
        client->buffer = calloc(1, client->max_buf_size);
        client->buf_size = client->max_buf_size;
    }
#endif

    return TRUE;
}

void
crm_ipc_close(crm_ipc_t * client)
{
    if (client) {
        crm_trace("Disconnecting %s IPC connection %p (%p)", client->name, client, client->ipc);

        if (client->ipc) {
            qb_ipcc_connection_t *ipc = client->ipc;

            client->ipc = NULL;
            qb_ipcc_disconnect(ipc);
        }
    }
}

void
crm_ipc_destroy(crm_ipc_t * client)
{
    if (client) {
        if (client->ipc && qb_ipcc_is_connected(client->ipc)) {
            crm_notice("Destroying an active IPC connection to %s", client->name);
            /* The next line is basically unsafe
             *
             * If this connection was attached to mainloop and mainloop is active,
             *   the 'disconnected' callback will end up back here and we'll end
             *   up free'ing the memory twice - something that can still happen
             *   even without this if we destroy a connection and it closes before
             *   we call exit
             */
            /* crm_ipc_close(client); */
        }
        crm_trace("Destroying IPC connection to %s: %p", client->name, client);
        free(client->buffer);
        free(client->name);
        free(client);
    }
}

int
crm_ipc_get_fd(crm_ipc_t * client)
{
    int fd = 0;

    if (client && client->ipc && (qb_ipcc_fd_get(client->ipc, &fd) == 0)) {
        return fd;
    }
    errno = EINVAL;
    crm_perror(LOG_ERR, "Could not obtain file IPC descriptor for %s",
               (client? client->name : "unspecified client"));
    return -errno;
}

bool
crm_ipc_connected(crm_ipc_t * client)
{
    bool rc = FALSE;

    if (client == NULL) {
        crm_trace("No client");
        return FALSE;

    } else if (client->ipc == NULL) {
        crm_trace("No connection");
        return FALSE;

    } else if (client->pfd.fd < 0) {
        crm_trace("Bad descriptor");
        return FALSE;
    }

    rc = qb_ipcc_is_connected(client->ipc);
    if (rc == FALSE) {
        client->pfd.fd = -EINVAL;
    }
    return rc;
}

/*!
 * \brief Check whether an IPC connection is ready to be read
 *
 * \param[in] client  Connection to check
 *
 * \return Positive value if ready to be read, 0 if not ready, -errno on error
 */
int
crm_ipc_ready(crm_ipc_t *client)
{
    int rc;

    CRM_ASSERT(client != NULL);

    if (crm_ipc_connected(client) == FALSE) {
        return -ENOTCONN;
    }

    client->pfd.revents = 0;
    rc = poll(&(client->pfd), 1, 0);
    return (rc < 0)? -errno : rc;
}

static int
crm_ipc_decompress(crm_ipc_t * client)
{
    struct crm_ipc_response_header *header = (struct crm_ipc_response_header *)(void*)client->buffer;

    if (header->size_compressed) {
        int rc = 0;
        unsigned int size_u = 1 + header->size_uncompressed;
        /* never let buf size fall below our max size required for ipc reads. */
        unsigned int new_buf_size = QB_MAX((hdr_offset + size_u), client->max_buf_size);
        char *uncompressed = calloc(1, new_buf_size);

        crm_trace("Decompressing message data %u bytes into %u bytes",
                 header->size_compressed, size_u);

        rc = BZ2_bzBuffToBuffDecompress(uncompressed + hdr_offset, &size_u,
                                        client->buffer + hdr_offset, header->size_compressed, 1, 0);

        if (rc != BZ_OK) {
            crm_err("Decompression failed: %s " CRM_XS " bzerror=%d",
                    bz2_strerror(rc), rc);
            free(uncompressed);
            return -EILSEQ;
        }

        /*
         * This assert no longer holds true.  For an identical msg, some clients may
         * require compression, and others may not. If that same msg (event) is sent
         * to multiple clients, it could result in some clients receiving a compressed
         * msg even though compression was not explicitly required for them.
         *
         * CRM_ASSERT((header->size_uncompressed + hdr_offset) >= ipc_buffer_max);
         */
        CRM_ASSERT(size_u == header->size_uncompressed);

        memcpy(uncompressed, client->buffer, hdr_offset);       /* Preserve the header */
        header = (struct crm_ipc_response_header *)(void*)uncompressed;

        free(client->buffer);
        client->buf_size = new_buf_size;
        client->buffer = uncompressed;
    }

    CRM_ASSERT(client->buffer[hdr_offset + header->size_uncompressed - 1] == 0);
    return pcmk_ok;
}

long
crm_ipc_read(crm_ipc_t * client)
{
    struct crm_ipc_response_header *header = NULL;

    CRM_ASSERT(client != NULL);
    CRM_ASSERT(client->ipc != NULL);
    CRM_ASSERT(client->buffer != NULL);

    crm_ipc_init();

    client->buffer[0] = 0;
    client->msg_size = qb_ipcc_event_recv(client->ipc, client->buffer,
                                          client->buf_size, 0);
    if (client->msg_size >= 0) {
        int rc = crm_ipc_decompress(client);

        if (rc != pcmk_ok) {
            return rc;
        }

        header = (struct crm_ipc_response_header *)(void*)client->buffer;
        if(header->version > PCMK_IPC_VERSION) {
            crm_err("Filtering incompatible v%d IPC message, we only support versions <= %d",
                    header->version, PCMK_IPC_VERSION);
            return -EBADMSG;
        }

        crm_trace("Received %s event %d, size=%u, rc=%d, text: %.100s",
                  client->name, header->qb.id, header->qb.size, client->msg_size,
                  client->buffer + hdr_offset);

    } else {
        crm_trace("No message from %s received: %s", client->name, pcmk_strerror(client->msg_size));
    }

    if (crm_ipc_connected(client) == FALSE || client->msg_size == -ENOTCONN) {
        crm_err("Connection to %s failed", client->name);
    }

    if (header) {
        /* Data excluding the header */
        return header->size_uncompressed;
    }
    return -ENOMSG;
}

const char *
crm_ipc_buffer(crm_ipc_t * client)
{
    CRM_ASSERT(client != NULL);
    return client->buffer + sizeof(struct crm_ipc_response_header);
}

uint32_t
crm_ipc_buffer_flags(crm_ipc_t * client)
{
    struct crm_ipc_response_header *header = NULL;

    CRM_ASSERT(client != NULL);
    if (client->buffer == NULL) {
        return 0;
    }

    header = (struct crm_ipc_response_header *)(void*)client->buffer;
    return header->flags;
}

const char *
crm_ipc_name(crm_ipc_t * client)
{
    CRM_ASSERT(client != NULL);
    return client->name;
}

static int
internal_ipc_send_recv(crm_ipc_t * client, const void *iov)
{
    int rc = 0;

    do {
        rc = qb_ipcc_sendv_recv(client->ipc, iov, 2, client->buffer, client->buf_size, -1);
    } while (rc == -EAGAIN && crm_ipc_connected(client));

    return rc;
}

static int
internal_ipc_send_request(crm_ipc_t * client, const void *iov, int ms_timeout)
{
    int rc = 0;
    time_t timeout = time(NULL) + 1 + (ms_timeout / 1000);

    do {
        rc = qb_ipcc_sendv(client->ipc, iov, 2);
    } while (rc == -EAGAIN && time(NULL) < timeout && crm_ipc_connected(client));

    return rc;
}

static int
internal_ipc_get_reply(crm_ipc_t * client, int request_id, int ms_timeout)
{
    time_t timeout = time(NULL) + 1 + (ms_timeout / 1000);
    int rc = 0;

    crm_ipc_init();

    /* get the reply */
    crm_trace("client %s waiting on reply to msg id %d", client->name, request_id);
    do {

        rc = qb_ipcc_recv(client->ipc, client->buffer, client->buf_size, 1000);
        if (rc > 0) {
            struct crm_ipc_response_header *hdr = NULL;

            int rc = crm_ipc_decompress(client);

            if (rc != pcmk_ok) {
                return rc;
            }

            hdr = (struct crm_ipc_response_header *)(void*)client->buffer;
            if (hdr->qb.id == request_id) {
                /* Got it */
                break;
            } else if (hdr->qb.id < request_id) {
                xmlNode *bad = string2xml(crm_ipc_buffer(client));

                crm_err("Discarding old reply %d (need %d)", hdr->qb.id, request_id);
                crm_log_xml_notice(bad, "OldIpcReply");

            } else {
                xmlNode *bad = string2xml(crm_ipc_buffer(client));

                crm_err("Discarding newer reply %d (need %d)", hdr->qb.id, request_id);
                crm_log_xml_notice(bad, "ImpossibleReply");
                CRM_ASSERT(hdr->qb.id <= request_id);
            }
        } else if (crm_ipc_connected(client) == FALSE) {
            crm_err("Server disconnected client %s while waiting for msg id %d", client->name,
                    request_id);
            break;
        }

    } while (time(NULL) < timeout);

    return rc;
}

int
crm_ipc_send(crm_ipc_t * client, xmlNode * message, enum crm_ipc_flags flags, int32_t ms_timeout,
             xmlNode ** reply)
{
    long rc = 0;
    struct iovec *iov;
    static uint32_t id = 0;
    static int factor = 8;
    struct crm_ipc_response_header *header;

    crm_ipc_init();

    if (client == NULL) {
        crm_notice("Invalid connection");
        return -ENOTCONN;

    } else if (crm_ipc_connected(client) == FALSE) {
        /* Don't even bother */
        crm_notice("Connection to %s closed", client->name);
        return -ENOTCONN;
    }

    if (ms_timeout == 0) {
        ms_timeout = 5000;
    }

    if (client->need_reply) {
        crm_trace("Trying again to obtain pending reply from %s", client->name);
        rc = qb_ipcc_recv(client->ipc, client->buffer, client->buf_size, ms_timeout);
        if (rc < 0) {
            crm_warn("Sending to %s (%p) is disabled until pending reply is received", client->name,
                     client->ipc);
            return -EALREADY;

        } else {
            crm_notice("Lost reply from %s (%p) finally arrived, sending re-enabled", client->name,
                       client->ipc);
            client->need_reply = FALSE;
        }
    }

    id++;
    CRM_LOG_ASSERT(id != 0); /* Crude wrap-around detection */
    rc = crm_ipc_prepare(id, message, &iov, client->max_buf_size);
    if(rc < 0) {
        return rc;
    }

    header = iov[0].iov_base;
    header->flags |= flags;

    if(is_set(flags, crm_ipc_proxied)) {
        /* Don't look for a synchronous response */
        clear_bit(flags, crm_ipc_client_response);
    }

    if(header->size_compressed) {
        if(factor < 10 && (client->max_buf_size / 10) < (rc / factor)) {
            crm_notice("Compressed message exceeds %d0%% of the configured ipc limit (%u bytes), "
                       "consider setting PCMK_ipc_buffer to %u or higher",
                       factor, client->max_buf_size, 2 * client->max_buf_size);
            factor++;
        }
    }

    crm_trace("Sending from client: %s request id: %d bytes: %u timeout:%d msg...",
              client->name, header->qb.id, header->qb.size, ms_timeout);

    if (ms_timeout > 0 || is_not_set(flags, crm_ipc_client_response)) {

        rc = internal_ipc_send_request(client, iov, ms_timeout);

        if (rc <= 0) {
            crm_trace("Failed to send from client %s request %d with %u bytes...",
                      client->name, header->qb.id, header->qb.size);
            goto send_cleanup;

        } else if (is_not_set(flags, crm_ipc_client_response)) {
            crm_trace("Message sent, not waiting for reply to %d from %s to %u bytes...",
                      header->qb.id, client->name, header->qb.size);

            goto send_cleanup;
        }

        rc = internal_ipc_get_reply(client, header->qb.id, ms_timeout);
        if (rc < 0) {
            /* No reply, for now, disable sending
             *
             * The alternative is to close the connection since we don't know
             * how to detect and discard out-of-sequence replies
             *
             * TODO - implement the above
             */
            client->need_reply = TRUE;
        }

    } else {
        rc = internal_ipc_send_recv(client, iov);
    }

    if (rc > 0) {
        struct crm_ipc_response_header *hdr = (struct crm_ipc_response_header *)(void*)client->buffer;

        crm_trace("Received response %d, size=%u, rc=%ld, text: %.200s", hdr->qb.id, hdr->qb.size,
                  rc, crm_ipc_buffer(client));

        if (reply) {
            *reply = string2xml(crm_ipc_buffer(client));
        }

    } else {
        crm_trace("Response not received: rc=%ld, errno=%d", rc, errno);
    }

  send_cleanup:
    if (crm_ipc_connected(client) == FALSE) {
        crm_notice("Connection to %s closed: %s (%ld)", client->name, pcmk_strerror(rc), rc);

    } else if (rc == -ETIMEDOUT) {
        crm_warn("Request %d to %s (%p) failed: %s (%ld) after %dms",
                 header->qb.id, client->name, client->ipc, pcmk_strerror(rc), rc, ms_timeout);
        crm_write_blackbox(0, NULL);

    } else if (rc <= 0) {
        crm_warn("Request %d to %s (%p) failed: %s (%ld)",
                 header->qb.id, client->name, client->ipc, pcmk_strerror(rc), rc);
    }

    pcmk_free_ipc_event(iov);
    return rc;
}

int
crm_ipc_is_authentic_process(int sock, uid_t refuid, gid_t refgid,
                             pid_t *gotpid, uid_t *gotuid, gid_t *gotgid) {
    int ret = 0;
    pid_t found_pid = 0; uid_t found_uid = 0; gid_t found_gid = 0;
#if defined(US_AUTH_PEERCRED_UCRED)
    struct ucred ucred;
    socklen_t ucred_len = sizeof(ucred);

    if (!getsockopt(sock, SOL_SOCKET, SO_PEERCRED,
                    &ucred, &ucred_len)
                && ucred_len == sizeof(ucred)) {
        found_pid = ucred.pid; found_uid = ucred.uid; found_gid = ucred.gid;

#elif defined(US_AUTH_PEERCRED_SOCKPEERCRED)
    struct sockpeercred sockpeercred;
    socklen_t sockpeercred_len = sizeof(sockpeercred);

    if (!getsockopt(sock, SOL_SOCKET, SO_PEERCRED,
                    &sockpeercred, &sockpeercred_len)
                && sockpeercred_len == sizeof(sockpeercred_len)) {
        found_pid = sockpeercred.pid;
        found_uid = sockpeercred.uid; found_gid = sockpeercred.gid;

#elif defined(US_AUTH_GETPEEREID)
    if (!getpeereid(sock, &found_uid, &found_gid)) {
        found_pid = PCMK__SPECIAL_PID;  /* cannot obtain PID (FreeBSD) */

#elif defined(US_AUTH_GETPEERUCRED)
    ucred_t *ucred;
    if (!getpeerucred(sock, &ucred)) {
        errno = 0;
        found_pid = ucred_getpid(ucred);
        found_uid = ucred_geteuid(ucred); found_gid = ucred_getegid(ucred);
        ret = -errno;
        ucred_free(ucred);
        if (ret) {
            return (ret < 0) ? ret : -pcmk_err_generic;
        }

#else
#  error "No way to authenticate a Unix socket peer"
    errno = 0;
    if (0) {
#endif
        if (gotpid != NULL) {
            *gotpid = found_pid;
        }
        if (gotuid != NULL) {
            *gotuid = found_uid;
        }
        if (gotgid != NULL) {
            *gotgid = found_gid;
        }
        ret = (found_uid == 0 || found_uid == refuid || found_gid == refgid);
    } else {
        ret = (errno > 0) ? -errno : -pcmk_err_generic;
    }

    return ret;
}

int
pcmk__ipc_is_authentic_process_active(const char *name, uid_t refuid,
                                      gid_t refgid, pid_t *gotpid) {
    static char last_asked_name[PATH_MAX / 2] = "";  /* log spam prevention */
    int fd, ret = 0;
    pid_t found_pid = 0; uid_t found_uid = 0; gid_t found_gid = 0;
    qb_ipcc_connection_t *c;

    if ((c = qb_ipcc_connect(name, 0)) == NULL) {
        crm_info("Could not connect to %s IPC: %s", name, strerror(errno));

    } else if ((ret = qb_ipcc_fd_get(c, &fd))) {
        crm_err("Could not get fd from %s IPC: %s (%d)", name,
                strerror(-ret), -ret);
        ret = -1;

    } else if ((ret = crm_ipc_is_authentic_process(fd, refuid, refgid,
                                                   &found_pid, &found_uid,
                                                   &found_gid)) < 0) {
        if (ret == -pcmk_err_generic) {
            crm_err("Could not get peer credentials from %s IPC", name);
        } else {
            crm_err("Could not get peer credentials from %s IPC: %s (%d)",
                    name, strerror(-ret), -ret);
        }
        ret = -1;

    } else {
        if (gotpid != NULL) {
            *gotpid = found_pid;
        }

        if (!ret) {
            crm_err("Daemon (IPC %s) effectively blocked with unauthorized"
                    " process %lld (uid: %lld, gid: %lld)",
                    name, (long long) PCMK__SPECIAL_PID_AS_0(found_pid),
                    (long long) found_uid, (long long) found_gid);
            ret = -2;
        } else if ((found_uid != refuid || found_gid != refgid)
                && strncmp(last_asked_name, name, sizeof(last_asked_name))) {
            if (!found_uid && refuid) {
                crm_warn("Daemon (IPC %s) runs as root, whereas the expected"
                         " credentials are %lld:%lld, hazard of violating"
                         " the least privilege principle",
                         name, (long long) refuid, (long long) refgid);
            } else {
                crm_notice("Daemon (IPC %s) runs as %lld:%lld, whereas the"
                           " expected credentials are %lld:%lld, which may"
                           " mean a different set of privileges than expected",
                           name, (long long) found_uid, (long long) found_gid,
                           (long long) refuid, (long long) refgid);
            }
            memccpy(last_asked_name, name, '\0', sizeof(last_asked_name));
        }
    }

    if (ret) {  /* here, !ret only when we could not initially connect */
        qb_ipcc_disconnect(c);
    }

    return ret;
}


/* Utils */

xmlNode *
create_hello_message(const char *uuid,
                     const char *client_name, const char *major_version, const char *minor_version)
{
    xmlNode *hello_node = NULL;
    xmlNode *hello = NULL;

    if (uuid == NULL || strlen(uuid) == 0
        || client_name == NULL || strlen(client_name) == 0
        || major_version == NULL || strlen(major_version) == 0
        || minor_version == NULL || strlen(minor_version) == 0) {
        crm_err("Missing fields, Hello message will not be valid.");
        return NULL;
    }

    hello_node = create_xml_node(NULL, XML_TAG_OPTIONS);
    crm_xml_add(hello_node, "major_version", major_version);
    crm_xml_add(hello_node, "minor_version", minor_version);
    crm_xml_add(hello_node, "client_name", client_name);
    crm_xml_add(hello_node, "client_uuid", uuid);

    crm_trace("creating hello message");
    hello = create_request(CRM_OP_HELLO, hello_node, NULL, NULL, client_name, uuid);
    free_xml(hello_node);

    return hello;
}
