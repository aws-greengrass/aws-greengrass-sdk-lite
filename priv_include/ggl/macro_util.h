// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_MACRO_UTIL_H
#define GGL_MACRO_UTIL_H

/// Expands to first argument.
#define GGL_MACRO_FIRST(first, ...) first

/// Expands to args, skipping first.
#define GGL_MACRO_REST(first, ...) __VA_ARGS__

/// Combine two args, expanding them.
#define GGL_MACRO_PASTE(left, right) GGL_MACRO_PASTE2(left, right)
#define GGL_MACRO_PASTE2(left, right) left##right

/// Macros for rescaning arguments for macro recursion
/// GGL_MACRO_RESCAN_N rescans N times
#define GGL_MACRO_RESCAN_2(...) __VA_ARGS__
#define GGL_MACRO_RESCAN_4(...) \
    GGL_MACRO_RESCAN_2(GGL_MACRO_RESCAN_2(__VA_ARGS__))
#define GGL_MACRO_RESCAN_8(...) \
    GGL_MACRO_RESCAN_4(GGL_MACRO_RESCAN_4(__VA_ARGS__))
#define GGL_MACRO_RESCAN_16(...) \
    GGL_MACRO_RESCAN_8(GGL_MACRO_RESCAN_8(__VA_ARGS__))
#define GGL_MACRO_RESCAN_32(...) \
    GGL_MACRO_RESCAN_16(GGL_MACRO_RESCAN_16(__VA_ARGS__))

/// Encloses args in parens
#define GGL_MACRO_PARENS(...) (__VA_ARGS__)

/// Hides a macro beginning with GGL_MACRO_ from preprocessor scan
#define GGL_MACRO_HIDE(name) GGL_MACRO_PASTE GGL_MACRO_PARENS(GGL_MACRO_, name)

/// Results in macro applied to each argument
#define GGL_MACRO_FOREACH(macro, join, ...) \
    GGL_MACRO_RESCAN_16(GGL_MACRO_FOREACH__ITER(macro, join, __VA_ARGS__))
#define GGL_MACRO_FOREACH__ITER(macro, join, arg, ...) \
    macro(arg \
    ) __VA_OPT__(join GGL_MACRO_HIDE(FOREACH__ITER)(macro, join, __VA_ARGS__))

#define GGL_MACRO_ID(...) __VA_ARGS__

#endif
