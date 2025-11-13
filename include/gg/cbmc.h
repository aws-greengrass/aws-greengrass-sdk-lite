// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_CBMC_H
#define GG_CBMC_H

//! Macros for CBMC function contracts

#ifndef __CPROVER__
#define CBMC_CONTRACT(...) CBMC_CONTRACT__STRIP(__VA_ARGS__)
#define CBMC_CONTRACT__STRIP(...)
#else

#include <gg/macro_util.h>

#define CBMC_CONTRACT(...) GG_MACRO_FOREACH(CBMC_CONTRACT__STMT, , __VA_ARGS__)
#define CBMC_CONTRACT__STMT(arg) CBMC_CONTRACT__##arg

#define CBMC_CONTRACT__requires __CPROVER_requires
#define CBMC_CONTRACT__ensures __CPROVER_ensures
#define CBMC_CONTRACT__assigns __CPROVER_assigns
#define CBMC_CONTRACT__frees __CPROVER_frees
#define CBMC_CONTRACT__invariant __CPROVER_loop_invariant
#define CBMC_CONTRACT__decreases __CPROVER_decreases

#define cbmc_return __CPROVER_return_value

#define cbmc_old(v) __CPROVER_old(v)
#define cbmc_loop_entry(v) __CPROVER_loop_entry(v)

#define cbmc_object_whole __CPROVER_object_whole
#define cbmc_object_from __CPROVER_object_from
#define cbmc_object_upto __CPROVER_object_upto

#define cbmc_restrict(ptr, ...) \
    __CPROVER_is_fresh(ptr, sizeof(*ptr) __VA_OPT__(*(__VA_ARGS__)))
#define cbmc_ptr_eq(p, q) __CPROVER_pointer_equals(p, q)
#define cbmc_ptr_in_range(lb, p, ub) __CPROVER_pointer_in_range_dfcc(lb, p, ub)

#define cbmc_obeys_contract(f, c) __CPROVER_obeys_contract(f, c)

#define cbmc_enum_valid(e) __CPROVER_enum_is_in_range(e)

// clang-format off
#define cbmc_implies(c1, ...) \
    (c1) ==> GG_MACRO_FOREACH(GG_MACRO_PARENS, &&, __VA_ARGS__)
// clang-format on

#define cbmc_forall(type, id, lb, ub, exp) \
    __CPROVER_forall { \
        type id; \
        cbmc_implies(lb <= id && id < ub, exp) \
    }
#define cbmc_exists(type, id, lb, ub, exp) \
    __CPROVER_exists { \
        type id; \
        (lb <= id && id < ub) && (exp) \
    }

#endif

#endif
