// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_MACRO_UTIL_H
#define GG_MACRO_UTIL_H

/// Expands to first argument.
#define GG_MACRO_FIRST(first, ...) first

/// Expands to args, skipping first.
#define GG_MACRO_REST(first, ...) __VA_ARGS__

/// Combine two args, expanding them.
#define GG_MACRO_PASTE(left, right) GG_MACRO_PASTE2(left, right)
#define GG_MACRO_PASTE2(left, right) left##right

/// Macros for rescaning arguments for macro recursion
/// GG_MACRO_RESCAN_N rescans N times
#define GG_MACRO_RESCAN_2(...) __VA_ARGS__
#define GG_MACRO_RESCAN_4(...) GG_MACRO_RESCAN_2(GG_MACRO_RESCAN_2(__VA_ARGS__))
#define GG_MACRO_RESCAN_8(...) GG_MACRO_RESCAN_4(GG_MACRO_RESCAN_4(__VA_ARGS__))
#define GG_MACRO_RESCAN_16(...) \
    GG_MACRO_RESCAN_8(GG_MACRO_RESCAN_8(__VA_ARGS__))
#define GG_MACRO_RESCAN_32(...) \
    GG_MACRO_RESCAN_16(GG_MACRO_RESCAN_16(__VA_ARGS__))

/// Encloses args in parens
#define GG_MACRO_PARENS(...) (__VA_ARGS__)

/// Hides a macro beginning with GG_MACRO_ from preprocessor scan
#define GG_MACRO_HIDE(name) GG_MACRO_PASTE GG_MACRO_PARENS(GG_MACRO_, name)

/// Results in macro applied to each argument
#define GG_MACRO_FOREACH(macro, join, ...) \
    GG_MACRO_RESCAN_16(GG_MACRO_FOREACH__ITER(macro, join, __VA_ARGS__))
#define GG_MACRO_FOREACH__ITER(macro, join, arg, ...) \
    macro(arg) __VA_OPT__( \
        join GG_MACRO_HIDE(FOREACH__ITER)(macro, join, __VA_ARGS__) \
    )

#define GG_MACRO_ID(...) __VA_ARGS__

#endif
