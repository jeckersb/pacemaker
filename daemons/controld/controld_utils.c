/*
 * Copyright 2004-2019 the Pacemaker project contributors
 *
 * The version control history for this file may have further details.
 *
 * This source code is licensed under the GNU General Public License version 2
 * or later (GPLv2+) WITHOUT ANY WARRANTY.
 */

#include <crm_internal.h>

#include <stdlib.h>

#include <crm/crm.h>
#include <crm/cib.h>
#include <crm/msg_xml.h>
#include <crm/common/xml.h>

#include <pacemaker-controld.h>

const char *
fsa_input2string(enum crmd_fsa_input input)
{
    const char *inputAsText = NULL;

    switch (input) {
        case I_NULL:
            inputAsText = "I_NULL";
            break;
        case I_CIB_OP:
            inputAsText = "I_CIB_OP (unused)";
            break;
        case I_CIB_UPDATE:
            inputAsText = "I_CIB_UPDATE";
            break;
        case I_DC_TIMEOUT:
            inputAsText = "I_DC_TIMEOUT";
            break;
        case I_ELECTION:
            inputAsText = "I_ELECTION";
            break;
        case I_PE_CALC:
            inputAsText = "I_PE_CALC";
            break;
        case I_RELEASE_DC:
            inputAsText = "I_RELEASE_DC";
            break;
        case I_ELECTION_DC:
            inputAsText = "I_ELECTION_DC";
            break;
        case I_ERROR:
            inputAsText = "I_ERROR";
            break;
        case I_FAIL:
            inputAsText = "I_FAIL";
            break;
        case I_INTEGRATED:
            inputAsText = "I_INTEGRATED";
            break;
        case I_FINALIZED:
            inputAsText = "I_FINALIZED";
            break;
        case I_NODE_JOIN:
            inputAsText = "I_NODE_JOIN";
            break;
        case I_JOIN_OFFER:
            inputAsText = "I_JOIN_OFFER";
            break;
        case I_JOIN_REQUEST:
            inputAsText = "I_JOIN_REQUEST";
            break;
        case I_JOIN_RESULT:
            inputAsText = "I_JOIN_RESULT";
            break;
        case I_NOT_DC:
            inputAsText = "I_NOT_DC";
            break;
        case I_RECOVERED:
            inputAsText = "I_RECOVERED";
            break;
        case I_RELEASE_FAIL:
            inputAsText = "I_RELEASE_FAIL";
            break;
        case I_RELEASE_SUCCESS:
            inputAsText = "I_RELEASE_SUCCESS";
            break;
        case I_RESTART:
            inputAsText = "I_RESTART";
            break;
        case I_PE_SUCCESS:
            inputAsText = "I_PE_SUCCESS";
            break;
        case I_ROUTER:
            inputAsText = "I_ROUTER";
            break;
        case I_SHUTDOWN:
            inputAsText = "I_SHUTDOWN";
            break;
        case I_STARTUP:
            inputAsText = "I_STARTUP";
            break;
        case I_TE_SUCCESS:
            inputAsText = "I_TE_SUCCESS";
            break;
        case I_STOP:
            inputAsText = "I_STOP";
            break;
        case I_DC_HEARTBEAT:
            inputAsText = "I_DC_HEARTBEAT";
            break;
        case I_WAIT_FOR_EVENT:
            inputAsText = "I_WAIT_FOR_EVENT";
            break;
        case I_LRM_EVENT:
            inputAsText = "I_LRM_EVENT";
            break;
        case I_PENDING:
            inputAsText = "I_PENDING";
            break;
        case I_HALT:
            inputAsText = "I_HALT";
            break;
        case I_TERMINATE:
            inputAsText = "I_TERMINATE";
            break;
        case I_ILLEGAL:
            inputAsText = "I_ILLEGAL";
            break;
    }

    if (inputAsText == NULL) {
        crm_err("Input %d is unknown", input);
        inputAsText = "<UNKNOWN_INPUT>";
    }

    return inputAsText;
}

const char *
fsa_state2string(enum crmd_fsa_state state)
{
    const char *stateAsText = NULL;

    switch (state) {
        case S_IDLE:
            stateAsText = "S_IDLE";
            break;
        case S_ELECTION:
            stateAsText = "S_ELECTION";
            break;
        case S_INTEGRATION:
            stateAsText = "S_INTEGRATION";
            break;
        case S_FINALIZE_JOIN:
            stateAsText = "S_FINALIZE_JOIN";
            break;
        case S_NOT_DC:
            stateAsText = "S_NOT_DC";
            break;
        case S_POLICY_ENGINE:
            stateAsText = "S_POLICY_ENGINE";
            break;
        case S_RECOVERY:
            stateAsText = "S_RECOVERY";
            break;
        case S_RELEASE_DC:
            stateAsText = "S_RELEASE_DC";
            break;
        case S_PENDING:
            stateAsText = "S_PENDING";
            break;
        case S_STOPPING:
            stateAsText = "S_STOPPING";
            break;
        case S_TERMINATE:
            stateAsText = "S_TERMINATE";
            break;
        case S_TRANSITION_ENGINE:
            stateAsText = "S_TRANSITION_ENGINE";
            break;
        case S_STARTING:
            stateAsText = "S_STARTING";
            break;
        case S_HALT:
            stateAsText = "S_HALT";
            break;
        case S_ILLEGAL:
            stateAsText = "S_ILLEGAL";
            break;
    }

    if (stateAsText == NULL) {
        crm_err("State %d is unknown", state);
        stateAsText = "<UNKNOWN_STATE>";
    }

    return stateAsText;
}

const char *
fsa_cause2string(enum crmd_fsa_cause cause)
{
    const char *causeAsText = NULL;

    switch (cause) {
        case C_UNKNOWN:
            causeAsText = "C_UNKNOWN";
            break;
        case C_STARTUP:
            causeAsText = "C_STARTUP";
            break;
        case C_IPC_MESSAGE:
            causeAsText = "C_IPC_MESSAGE";
            break;
        case C_HA_MESSAGE:
            causeAsText = "C_HA_MESSAGE";
            break;
        case C_TIMER_POPPED:
            causeAsText = "C_TIMER_POPPED";
            break;
        case C_SHUTDOWN:
            causeAsText = "C_SHUTDOWN";
            break;
        case C_LRM_OP_CALLBACK:
            causeAsText = "C_LRM_OP_CALLBACK";
            break;
        case C_CRMD_STATUS_CALLBACK:
            causeAsText = "C_CRMD_STATUS_CALLBACK";
            break;
        case C_FSA_INTERNAL:
            causeAsText = "C_FSA_INTERNAL";
            break;
    }

    if (causeAsText == NULL) {
        crm_err("Cause %d is unknown", cause);
        causeAsText = "<UNKNOWN_CAUSE>";
    }

    return causeAsText;
}

const char *
fsa_action2string(long long action)
{
    const char *actionAsText = NULL;

    switch (action) {

        case A_NOTHING:
            actionAsText = "A_NOTHING";
            break;
        case A_ELECTION_START:
            actionAsText = "A_ELECTION_START";
            break;
        case A_DC_JOIN_FINAL:
            actionAsText = "A_DC_JOIN_FINAL";
            break;
        case A_READCONFIG:
            actionAsText = "A_READCONFIG";
            break;
        case O_RELEASE:
            actionAsText = "O_RELEASE";
            break;
        case A_STARTUP:
            actionAsText = "A_STARTUP";
            break;
        case A_STARTED:
            actionAsText = "A_STARTED";
            break;
        case A_HA_CONNECT:
            actionAsText = "A_HA_CONNECT";
            break;
        case A_HA_DISCONNECT:
            actionAsText = "A_HA_DISCONNECT";
            break;
        case A_LRM_CONNECT:
            actionAsText = "A_LRM_CONNECT";
            break;
        case A_LRM_EVENT:
            actionAsText = "A_LRM_EVENT";
            break;
        case A_LRM_INVOKE:
            actionAsText = "A_LRM_INVOKE";
            break;
        case A_LRM_DISCONNECT:
            actionAsText = "A_LRM_DISCONNECT";
            break;
        case O_LRM_RECONNECT:
            actionAsText = "O_LRM_RECONNECT";
            break;
        case A_CL_JOIN_QUERY:
            actionAsText = "A_CL_JOIN_QUERY";
            break;
        case A_DC_TIMER_STOP:
            actionAsText = "A_DC_TIMER_STOP";
            break;
        case A_DC_TIMER_START:
            actionAsText = "A_DC_TIMER_START";
            break;
        case A_INTEGRATE_TIMER_START:
            actionAsText = "A_INTEGRATE_TIMER_START";
            break;
        case A_INTEGRATE_TIMER_STOP:
            actionAsText = "A_INTEGRATE_TIMER_STOP";
            break;
        case A_FINALIZE_TIMER_START:
            actionAsText = "A_FINALIZE_TIMER_START";
            break;
        case A_FINALIZE_TIMER_STOP:
            actionAsText = "A_FINALIZE_TIMER_STOP";
            break;
        case A_ELECTION_COUNT:
            actionAsText = "A_ELECTION_COUNT";
            break;
        case A_ELECTION_VOTE:
            actionAsText = "A_ELECTION_VOTE";
            break;
        case A_ELECTION_CHECK:
            actionAsText = "A_ELECTION_CHECK";
            break;
        case A_CL_JOIN_ANNOUNCE:
            actionAsText = "A_CL_JOIN_ANNOUNCE";
            break;
        case A_CL_JOIN_REQUEST:
            actionAsText = "A_CL_JOIN_REQUEST";
            break;
        case A_CL_JOIN_RESULT:
            actionAsText = "A_CL_JOIN_RESULT";
            break;
        case A_DC_JOIN_OFFER_ALL:
            actionAsText = "A_DC_JOIN_OFFER_ALL";
            break;
        case A_DC_JOIN_OFFER_ONE:
            actionAsText = "A_DC_JOIN_OFFER_ONE";
            break;
        case A_DC_JOIN_PROCESS_REQ:
            actionAsText = "A_DC_JOIN_PROCESS_REQ";
            break;
        case A_DC_JOIN_PROCESS_ACK:
            actionAsText = "A_DC_JOIN_PROCESS_ACK";
            break;
        case A_DC_JOIN_FINALIZE:
            actionAsText = "A_DC_JOIN_FINALIZE";
            break;
        case A_MSG_PROCESS:
            actionAsText = "A_MSG_PROCESS";
            break;
        case A_MSG_ROUTE:
            actionAsText = "A_MSG_ROUTE";
            break;
        case A_RECOVER:
            actionAsText = "A_RECOVER";
            break;
        case A_DC_RELEASE:
            actionAsText = "A_DC_RELEASE";
            break;
        case A_DC_RELEASED:
            actionAsText = "A_DC_RELEASED";
            break;
        case A_DC_TAKEOVER:
            actionAsText = "A_DC_TAKEOVER";
            break;
        case A_SHUTDOWN:
            actionAsText = "A_SHUTDOWN";
            break;
        case A_SHUTDOWN_REQ:
            actionAsText = "A_SHUTDOWN_REQ";
            break;
        case A_STOP:
            actionAsText = "A_STOP  ";
            break;
        case A_EXIT_0:
            actionAsText = "A_EXIT_0";
            break;
        case A_EXIT_1:
            actionAsText = "A_EXIT_1";
            break;
        case O_CIB_RESTART:
            actionAsText = "O_CIB_RESTART";
            break;
        case A_CIB_START:
            actionAsText = "A_CIB_START";
            break;
        case A_CIB_STOP:
            actionAsText = "A_CIB_STOP";
            break;
        case A_TE_INVOKE:
            actionAsText = "A_TE_INVOKE";
            break;
        case O_TE_RESTART:
            actionAsText = "O_TE_RESTART";
            break;
        case A_TE_START:
            actionAsText = "A_TE_START";
            break;
        case A_TE_STOP:
            actionAsText = "A_TE_STOP";
            break;
        case A_TE_HALT:
            actionAsText = "A_TE_HALT";
            break;
        case A_TE_CANCEL:
            actionAsText = "A_TE_CANCEL";
            break;
        case A_PE_INVOKE:
            actionAsText = "A_PE_INVOKE";
            break;
        case O_PE_RESTART:
            actionAsText = "O_PE_RESTART";
            break;
        case A_PE_START:
            actionAsText = "A_PE_START";
            break;
        case A_PE_STOP:
            actionAsText = "A_PE_STOP";
            break;
        case A_NODE_BLOCK:
            actionAsText = "A_NODE_BLOCK";
            break;
        case A_UPDATE_NODESTATUS:
            actionAsText = "A_UPDATE_NODESTATUS";
            break;
        case A_LOG:
            actionAsText = "A_LOG   ";
            break;
        case A_ERROR:
            actionAsText = "A_ERROR ";
            break;
        case A_WARN:
            actionAsText = "A_WARN  ";
            break;
            /* Composite actions */
        case A_DC_TIMER_START | A_CL_JOIN_QUERY:
            actionAsText = "A_DC_TIMER_START|A_CL_JOIN_QUERY";
            break;
    }

    if (actionAsText == NULL) {
        crm_err("Action %.16llx is unknown", action);
        actionAsText = "<UNKNOWN_ACTION>";
    }

    return actionAsText;
}

void
fsa_dump_inputs(int log_level, const char *text, long long input_register)
{
    if (input_register == A_NOTHING) {
        return;
    }
    if (text == NULL) {
        text = "Input register contents:";
    }

    if (is_set(input_register, R_THE_DC)) {
        crm_trace("%s %.16llx (R_THE_DC)", text, R_THE_DC);
    }
    if (is_set(input_register, R_STARTING)) {
        crm_trace("%s %.16llx (R_STARTING)", text, R_STARTING);
    }
    if (is_set(input_register, R_SHUTDOWN)) {
        crm_trace("%s %.16llx (R_SHUTDOWN)", text, R_SHUTDOWN);
    }
    if (is_set(input_register, R_STAYDOWN)) {
        crm_trace("%s %.16llx (R_STAYDOWN)", text, R_STAYDOWN);
    }
    if (is_set(input_register, R_JOIN_OK)) {
        crm_trace("%s %.16llx (R_JOIN_OK)", text, R_JOIN_OK);
    }
    if (is_set(input_register, R_READ_CONFIG)) {
        crm_trace("%s %.16llx (R_READ_CONFIG)", text, R_READ_CONFIG);
    }
    if (is_set(input_register, R_INVOKE_PE)) {
        crm_trace("%s %.16llx (R_INVOKE_PE)", text, R_INVOKE_PE);
    }
    if (is_set(input_register, R_CIB_CONNECTED)) {
        crm_trace("%s %.16llx (R_CIB_CONNECTED)", text, R_CIB_CONNECTED);
    }
    if (is_set(input_register, R_PE_CONNECTED)) {
        crm_trace("%s %.16llx (R_PE_CONNECTED)", text, R_PE_CONNECTED);
    }
    if (is_set(input_register, R_TE_CONNECTED)) {
        crm_trace("%s %.16llx (R_TE_CONNECTED)", text, R_TE_CONNECTED);
    }
    if (is_set(input_register, R_LRM_CONNECTED)) {
        crm_trace("%s %.16llx (R_LRM_CONNECTED)", text, R_LRM_CONNECTED);
    }
    if (is_set(input_register, R_CIB_REQUIRED)) {
        crm_trace("%s %.16llx (R_CIB_REQUIRED)", text, R_CIB_REQUIRED);
    }
    if (is_set(input_register, R_PE_REQUIRED)) {
        crm_trace("%s %.16llx (R_PE_REQUIRED)", text, R_PE_REQUIRED);
    }
    if (is_set(input_register, R_TE_REQUIRED)) {
        crm_trace("%s %.16llx (R_TE_REQUIRED)", text, R_TE_REQUIRED);
    }
    if (is_set(input_register, R_REQ_PEND)) {
        crm_trace("%s %.16llx (R_REQ_PEND)", text, R_REQ_PEND);
    }
    if (is_set(input_register, R_PE_PEND)) {
        crm_trace("%s %.16llx (R_PE_PEND)", text, R_PE_PEND);
    }
    if (is_set(input_register, R_TE_PEND)) {
        crm_trace("%s %.16llx (R_TE_PEND)", text, R_TE_PEND);
    }
    if (is_set(input_register, R_RESP_PEND)) {
        crm_trace("%s %.16llx (R_RESP_PEND)", text, R_RESP_PEND);
    }
    if (is_set(input_register, R_CIB_DONE)) {
        crm_trace("%s %.16llx (R_CIB_DONE)", text, R_CIB_DONE);
    }
    if (is_set(input_register, R_HAVE_CIB)) {
        crm_trace("%s %.16llx (R_HAVE_CIB)", text, R_HAVE_CIB);
    }
    if (is_set(input_register, R_CIB_ASKED)) {
        crm_trace("%s %.16llx (R_CIB_ASKED)", text, R_CIB_ASKED);
    }
    if (is_set(input_register, R_MEMBERSHIP)) {
        crm_trace("%s %.16llx (R_MEMBERSHIP)", text, R_MEMBERSHIP);
    }
    if (is_set(input_register, R_PEER_DATA)) {
        crm_trace("%s %.16llx (R_PEER_DATA)", text, R_PEER_DATA);
    }
    if (is_set(input_register, R_IN_RECOVERY)) {
        crm_trace("%s %.16llx (R_IN_RECOVERY)", text, R_IN_RECOVERY);
    }
}

void
fsa_dump_actions(long long action, const char *text)
{
    if (is_set(action, A_READCONFIG)) {
        crm_trace("Action %.16llx (A_READCONFIG) %s", A_READCONFIG, text);
    }
    if (is_set(action, A_STARTUP)) {
        crm_trace("Action %.16llx (A_STARTUP) %s", A_STARTUP, text);
    }
    if (is_set(action, A_STARTED)) {
        crm_trace("Action %.16llx (A_STARTED) %s", A_STARTED, text);
    }
    if (is_set(action, A_HA_CONNECT)) {
        crm_trace("Action %.16llx (A_CONNECT) %s", A_HA_CONNECT, text);
    }
    if (is_set(action, A_HA_DISCONNECT)) {
        crm_trace("Action %.16llx (A_DISCONNECT) %s", A_HA_DISCONNECT, text);
    }
    if (is_set(action, A_LRM_CONNECT)) {
        crm_trace("Action %.16llx (A_LRM_CONNECT) %s", A_LRM_CONNECT, text);
    }
    if (is_set(action, A_LRM_EVENT)) {
        crm_trace("Action %.16llx (A_LRM_EVENT) %s", A_LRM_EVENT, text);
    }
    if (is_set(action, A_LRM_INVOKE)) {
        crm_trace("Action %.16llx (A_LRM_INVOKE) %s", A_LRM_INVOKE, text);
    }
    if (is_set(action, A_LRM_DISCONNECT)) {
        crm_trace("Action %.16llx (A_LRM_DISCONNECT) %s", A_LRM_DISCONNECT, text);
    }
    if (is_set(action, A_DC_TIMER_STOP)) {
        crm_trace("Action %.16llx (A_DC_TIMER_STOP) %s", A_DC_TIMER_STOP, text);
    }
    if (is_set(action, A_DC_TIMER_START)) {
        crm_trace("Action %.16llx (A_DC_TIMER_START) %s", A_DC_TIMER_START, text);
    }
    if (is_set(action, A_INTEGRATE_TIMER_START)) {
        crm_trace("Action %.16llx (A_INTEGRATE_TIMER_START) %s", A_INTEGRATE_TIMER_START, text);
    }
    if (is_set(action, A_INTEGRATE_TIMER_STOP)) {
        crm_trace("Action %.16llx (A_INTEGRATE_TIMER_STOP) %s", A_INTEGRATE_TIMER_STOP, text);
    }
    if (is_set(action, A_FINALIZE_TIMER_START)) {
        crm_trace("Action %.16llx (A_FINALIZE_TIMER_START) %s", A_FINALIZE_TIMER_START, text);
    }
    if (is_set(action, A_FINALIZE_TIMER_STOP)) {
        crm_trace("Action %.16llx (A_FINALIZE_TIMER_STOP) %s", A_FINALIZE_TIMER_STOP, text);
    }
    if (is_set(action, A_ELECTION_COUNT)) {
        crm_trace("Action %.16llx (A_ELECTION_COUNT) %s", A_ELECTION_COUNT, text);
    }
    if (is_set(action, A_ELECTION_VOTE)) {
        crm_trace("Action %.16llx (A_ELECTION_VOTE) %s", A_ELECTION_VOTE, text);
    }
    if (is_set(action, A_ELECTION_CHECK)) {
        crm_trace("Action %.16llx (A_ELECTION_CHECK) %s", A_ELECTION_CHECK, text);
    }
    if (is_set(action, A_CL_JOIN_ANNOUNCE)) {
        crm_trace("Action %.16llx (A_CL_JOIN_ANNOUNCE) %s", A_CL_JOIN_ANNOUNCE, text);
    }
    if (is_set(action, A_CL_JOIN_REQUEST)) {
        crm_trace("Action %.16llx (A_CL_JOIN_REQUEST) %s", A_CL_JOIN_REQUEST, text);
    }
    if (is_set(action, A_CL_JOIN_RESULT)) {
        crm_trace("Action %.16llx (A_CL_JOIN_RESULT) %s", A_CL_JOIN_RESULT, text);
    }
    if (is_set(action, A_DC_JOIN_OFFER_ALL)) {
        crm_trace("Action %.16llx (A_DC_JOIN_OFFER_ALL) %s", A_DC_JOIN_OFFER_ALL, text);
    }
    if (is_set(action, A_DC_JOIN_OFFER_ONE)) {
        crm_trace("Action %.16llx (A_DC_JOIN_OFFER_ONE) %s", A_DC_JOIN_OFFER_ONE, text);
    }
    if (is_set(action, A_DC_JOIN_PROCESS_REQ)) {
        crm_trace("Action %.16llx (A_DC_JOIN_PROCESS_REQ) %s", A_DC_JOIN_PROCESS_REQ, text);
    }
    if (is_set(action, A_DC_JOIN_PROCESS_ACK)) {
        crm_trace("Action %.16llx (A_DC_JOIN_PROCESS_ACK) %s", A_DC_JOIN_PROCESS_ACK, text);
    }
    if (is_set(action, A_DC_JOIN_FINALIZE)) {
        crm_trace("Action %.16llx (A_DC_JOIN_FINALIZE) %s", A_DC_JOIN_FINALIZE, text);
    }
    if (is_set(action, A_MSG_PROCESS)) {
        crm_trace("Action %.16llx (A_MSG_PROCESS) %s", A_MSG_PROCESS, text);
    }
    if (is_set(action, A_MSG_ROUTE)) {
        crm_trace("Action %.16llx (A_MSG_ROUTE) %s", A_MSG_ROUTE, text);
    }
    if (is_set(action, A_RECOVER)) {
        crm_trace("Action %.16llx (A_RECOVER) %s", A_RECOVER, text);
    }
    if (is_set(action, A_DC_RELEASE)) {
        crm_trace("Action %.16llx (A_DC_RELEASE) %s", A_DC_RELEASE, text);
    }
    if (is_set(action, A_DC_RELEASED)) {
        crm_trace("Action %.16llx (A_DC_RELEASED) %s", A_DC_RELEASED, text);
    }
    if (is_set(action, A_DC_TAKEOVER)) {
        crm_trace("Action %.16llx (A_DC_TAKEOVER) %s", A_DC_TAKEOVER, text);
    }
    if (is_set(action, A_SHUTDOWN)) {
        crm_trace("Action %.16llx (A_SHUTDOWN) %s", A_SHUTDOWN, text);
    }
    if (is_set(action, A_SHUTDOWN_REQ)) {
        crm_trace("Action %.16llx (A_SHUTDOWN_REQ) %s", A_SHUTDOWN_REQ, text);
    }
    if (is_set(action, A_STOP)) {
        crm_trace("Action %.16llx (A_STOP  ) %s", A_STOP, text);
    }
    if (is_set(action, A_EXIT_0)) {
        crm_trace("Action %.16llx (A_EXIT_0) %s", A_EXIT_0, text);
    }
    if (is_set(action, A_EXIT_1)) {
        crm_trace("Action %.16llx (A_EXIT_1) %s", A_EXIT_1, text);
    }
    if (is_set(action, A_CIB_START)) {
        crm_trace("Action %.16llx (A_CIB_START) %s", A_CIB_START, text);
    }
    if (is_set(action, A_CIB_STOP)) {
        crm_trace("Action %.16llx (A_CIB_STOP) %s", A_CIB_STOP, text);
    }
    if (is_set(action, A_TE_INVOKE)) {
        crm_trace("Action %.16llx (A_TE_INVOKE) %s", A_TE_INVOKE, text);
    }
    if (is_set(action, A_TE_START)) {
        crm_trace("Action %.16llx (A_TE_START) %s", A_TE_START, text);
    }
    if (is_set(action, A_TE_STOP)) {
        crm_trace("Action %.16llx (A_TE_STOP) %s", A_TE_STOP, text);
    }
    if (is_set(action, A_TE_CANCEL)) {
        crm_trace("Action %.16llx (A_TE_CANCEL) %s", A_TE_CANCEL, text);
    }
    if (is_set(action, A_PE_INVOKE)) {
        crm_trace("Action %.16llx (A_PE_INVOKE) %s", A_PE_INVOKE, text);
    }
    if (is_set(action, A_PE_START)) {
        crm_trace("Action %.16llx (A_PE_START) %s", A_PE_START, text);
    }
    if (is_set(action, A_PE_STOP)) {
        crm_trace("Action %.16llx (A_PE_STOP) %s", A_PE_STOP, text);
    }
    if (is_set(action, A_NODE_BLOCK)) {
        crm_trace("Action %.16llx (A_NODE_BLOCK) %s", A_NODE_BLOCK, text);
    }
    if (is_set(action, A_UPDATE_NODESTATUS)) {
        crm_trace("Action %.16llx (A_UPDATE_NODESTATUS) %s", A_UPDATE_NODESTATUS, text);
    }
    if (is_set(action, A_LOG)) {
        crm_trace("Action %.16llx (A_LOG   ) %s", A_LOG, text);
    }
    if (is_set(action, A_ERROR)) {
        crm_trace("Action %.16llx (A_ERROR ) %s", A_ERROR, text);
    }
    if (is_set(action, A_WARN)) {
        crm_trace("Action %.16llx (A_WARN  ) %s", A_WARN, text);
    }
}

gboolean
update_dc(xmlNode * msg)
{
    char *last_dc = fsa_our_dc;
    const char *dc_version = NULL;
    const char *welcome_from = NULL;

    if (msg != NULL) {
        gboolean invalid = FALSE;

        dc_version = crm_element_value(msg, F_CRM_VERSION);
        welcome_from = crm_element_value(msg, F_CRM_HOST_FROM);

        CRM_CHECK(dc_version != NULL, return FALSE);
        CRM_CHECK(welcome_from != NULL, return FALSE);

        if (AM_I_DC && safe_str_neq(welcome_from, fsa_our_uname)) {
            invalid = TRUE;

        } else if (fsa_our_dc && safe_str_neq(welcome_from, fsa_our_dc)) {
            invalid = TRUE;
        }

        if (invalid) {
            CRM_CHECK(fsa_our_dc != NULL, crm_err("We have no DC"));
            if (AM_I_DC) {
                crm_err("Not updating DC to %s (%s): we are also a DC", welcome_from, dc_version);
            } else {
                crm_warn("New DC %s is not %s", welcome_from, fsa_our_dc);
            }

            register_fsa_action(A_CL_JOIN_QUERY | A_DC_TIMER_START);
            return FALSE;
        }
    }

    free(fsa_our_dc_version);
    fsa_our_dc_version = NULL;

    fsa_our_dc = NULL;          /* Free'd as last_dc */

    if (welcome_from != NULL) {
        fsa_our_dc = strdup(welcome_from);
    }
    if (dc_version != NULL) {
        fsa_our_dc_version = strdup(dc_version);
    }

    if (safe_str_eq(fsa_our_dc, last_dc)) {
        /* do nothing */

    } else if (fsa_our_dc != NULL) {
        crm_node_t *dc_node = crm_get_peer(0, fsa_our_dc);

        crm_info("Set DC to %s (%s)", crm_str(fsa_our_dc), crm_str(fsa_our_dc_version));
        crm_update_peer_expected(__FUNCTION__, dc_node, CRMD_JOINSTATE_MEMBER);

    } else if (last_dc != NULL) {
        crm_info("Unset DC. Was %s", crm_str(last_dc));
    }

    free(last_dc);
    return TRUE;
}

#define STATUS_PATH_MAX 512
static void
erase_xpath_callback(xmlNode * msg, int call_id, int rc, xmlNode * output, void *user_data)
{
    char *xpath = user_data;

    do_crm_log_unlikely(rc == 0 ? LOG_DEBUG : LOG_NOTICE,
                        "Deletion of \"%s\": %s (rc=%d)", xpath, pcmk_strerror(rc), rc);
}

#define XPATH_STATUS_TAG "//node_state[@uname='%s']/%s"

void
erase_status_tag(const char *uname, const char *tag, int options)
{
    if (fsa_cib_conn && uname) {
        int call_id;
        char *xpath = crm_strdup_printf(XPATH_STATUS_TAG, uname, tag);

        crm_info("Deleting %s status entries for %s " CRM_XS " xpath=%s",
                 tag, uname, xpath);
        call_id = fsa_cib_conn->cmds->remove(fsa_cib_conn, xpath, NULL,
                                             cib_quorum_override | cib_xpath | options);
        fsa_register_cib_callback(call_id, FALSE, xpath, erase_xpath_callback);
        // CIB library handles freeing xpath
    }
}

void crmd_peer_down(crm_node_t *peer, bool full) 
{
    if(full && peer->state == NULL) {
        crm_update_peer_state(__FUNCTION__, peer, CRM_NODE_LOST, 0);
        crm_update_peer_proc(__FUNCTION__, peer, crm_proc_none, NULL);
    }
    crm_update_peer_join(__FUNCTION__, peer, crm_join_none);
    crm_update_peer_expected(__FUNCTION__, peer, CRMD_JOINSTATE_DOWN);
}

#define MIN_CIB_OP_TIMEOUT (30)

unsigned int
cib_op_timeout()
{
    static int env_timeout = -1;
    unsigned int calculated_timeout = 0;

    if (env_timeout == -1) {
        const char *env = getenv("PCMK_cib_timeout");

        if (env) {
            env_timeout = crm_parse_int(env, "0");
        }
        env_timeout = QB_MAX(env_timeout, MIN_CIB_OP_TIMEOUT);
        crm_trace("Minimum CIB op timeout: %us (environment: %s)",
                  env_timeout, (env? env : "none"));
    }

    calculated_timeout = 1 + crm_active_peers();
    if (crm_remote_peer_cache) {
        calculated_timeout += g_hash_table_size(crm_remote_peer_cache);
    }
    calculated_timeout *= 10;

    calculated_timeout = QB_MAX(calculated_timeout, env_timeout);
    crm_trace("Calculated timeout: %us", calculated_timeout);

    if (fsa_cib_conn) {
        fsa_cib_conn->call_timeout = calculated_timeout;
    }
    return calculated_timeout;
}

/*!
 * \internal
 * \brief Check feature set compatibility of DC and joining node
 *
 * Return true if a joining node's CRM feature set is compatible with the
 * current DC's. The feature sets are compatible if they have the same major
 * version number, and the DC's minor version number is the same or older than
 * the joining node's. The minor-minor version is intended solely to allow
 * resource agents to detect feature support, and so is ignored.
 *
 * \param[in] dc_version    DC's feature set
 * \param[in] join_version  Joining node's version
 */
bool
feature_set_compatible(const char *dc_version, const char *join_version)
{
    char *dc_minor = NULL;
    char *join_minor = NULL;
    long dc_v = 0;
    long join_v = 0;

    // Get DC's major version
    errno = 0;
    dc_v = strtol(dc_version, &dc_minor, 10);
    if (errno) {
        return FALSE;
    }

    // Get joining node's major version
    errno = 0;
    join_v = strtol(join_version, &join_minor, 10);
    if (errno) {
        return FALSE;
    }

    // Major version component must be identical
    if (dc_v != join_v) {
        return FALSE;
    }

    // Get DC's minor version
    if (*dc_minor == '.') {
        ++dc_minor;
    }
    errno = 0;
    dc_v = strtol(dc_minor, NULL, 10);
    if (errno) {
        return FALSE;
    }

    // Get joining node's minor version
    if (*join_minor == '.') {
        ++join_minor;
    }
    errno = 0;
    join_v = strtol(join_minor, NULL, 10);
    if (errno) {
        return FALSE;
    }

    // DC's minor version must be the same or older
    return dc_v <= join_v;
}

const char *
get_node_id(xmlNode *lrm_rsc_op)
{
    xmlNode *node = lrm_rsc_op;

    while (node != NULL && safe_str_neq(XML_CIB_TAG_STATE, TYPE(node))) {
        node = node->parent;
    }

    CRM_CHECK(node != NULL, return NULL);
    return ID(node);
}
