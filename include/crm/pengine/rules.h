/*
 * Copyright 2004-2019 the Pacemaker project contributors
 *
 * The version control history for this file may have further details.
 *
 * This source code is licensed under the GNU Lesser General Public License
 * version 2.1 or later (LGPLv2.1+) WITHOUT ANY WARRANTY.
 */

#ifndef PENGINE_RULES__H
#  define PENGINE_RULES__H

#ifdef __cplusplus
extern "C" {
#endif

#  include <glib.h>
#  include <regex.h>

#  include <crm/crm.h>
#  include <crm/common/iso8601.h>
#  include <crm/pengine/common.h>

enum expression_type {
    not_expr,
    nested_rule,
    attr_expr,
    loc_expr,
    role_expr,
    time_expr,
    version_expr
};

typedef struct pe_re_match_data {
    char *string;
    int nregs;
    regmatch_t *pmatch;
} pe_re_match_data_t;

typedef struct pe_match_data {
    pe_re_match_data_t *re;
    GHashTable *params;
    GHashTable *meta;
} pe_match_data_t;

enum expression_type find_expression_type(xmlNode * expr);

gboolean pe_evaluate_rules(xmlNode *ruleset, GHashTable *node_hash,
                           crm_time_t *now, crm_time_t *next_change);

gboolean pe_test_rule(xmlNode *rule, GHashTable *node_hash,
                      enum rsc_role_e role, crm_time_t *now,
                      crm_time_t *next_change, pe_match_data_t *match_data);

gboolean pe_test_expression(xmlNode *expr, GHashTable *node_hash,
                            enum rsc_role_e role, crm_time_t *now,
                            crm_time_t *next_change,
                            pe_match_data_t *match_data);

void pe_unpack_nvpairs(xmlNode *top, xmlNode *xml_obj, const char *set_name,
                       GHashTable *node_hash, GHashTable *hash,
                       const char *always_first, gboolean overwrite,
                       crm_time_t *now, crm_time_t *next_change);

#if ENABLE_VERSIONED_ATTRS
void pe_unpack_versioned_attributes(xmlNode *top, xmlNode *xml_obj,
                                    const char *set_name, GHashTable *node_hash,
                                    xmlNode *hash, crm_time_t *now,
                                    crm_time_t *next_change);
GHashTable *pe_unpack_versioned_parameters(xmlNode *versioned_params, const char *ra_version);
#endif

char *pe_expand_re_matches(const char *string, pe_re_match_data_t * match_data);

//! \deprecated Use pe_evaluate_rules() instead
gboolean test_ruleset(xmlNode *ruleset, GHashTable *node_hash, crm_time_t *now);

//! \deprecated Use pe_test_rule() instead
gboolean test_rule(xmlNode *rule, GHashTable *node_hash, enum rsc_role_e role,
                   crm_time_t *now);

//! \deprecated Use pe_test_rule() instead
gboolean pe_test_rule_re(xmlNode *rule, GHashTable *node_hash,
                         enum rsc_role_e role, crm_time_t *now,
                         pe_re_match_data_t *re_match_data);

//! \deprecated Use pe_test_rule() instead
gboolean pe_test_rule_full(xmlNode *rule, GHashTable *node_hash,
                           enum rsc_role_e role, crm_time_t *now,
                           pe_match_data_t *match_data);

//! \deprecated Use pe_test_expression() instead
gboolean test_expression(xmlNode *expr, GHashTable *node_hash,
                         enum rsc_role_e role, crm_time_t *now);

//! \deprecated Use pe_test_expression() instead
gboolean pe_test_expression_re(xmlNode *expr, GHashTable *node_hash,
                         enum rsc_role_e role, crm_time_t *now,
                         pe_re_match_data_t *re_match_data);

//! \deprecated Use pe_test_expression() instead
gboolean pe_test_expression_full(xmlNode *expr, GHashTable *node_hash,
                                 enum rsc_role_e role,
                                 crm_time_t *now, pe_match_data_t *match_data);

//! \deprecated Use pe_unpack_nvpairs() instead
void unpack_instance_attributes(xmlNode *top, xmlNode *xml_obj,
                                const char *set_name, GHashTable *node_hash,
                                GHashTable *hash, const char *always_first,
                                gboolean overwrite, crm_time_t *now);

#ifdef __cplusplus
}
#endif

#endif
