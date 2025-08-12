// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_ATTR_H
#define GGL_ATTR_H

#ifdef __has_c_attribute
#if __has_c_attribute(nodiscard)
#define NODISCARD [[nodiscard]]
#endif
#endif

#ifndef NODISCARD
#define NODISCARD
#endif

#ifdef __has_attribute
#if __has_attribute(visibility)
#define VISIBILITY(v) __attribute__((visibility(#v)))
#endif
#endif

#ifndef VISIBILITY
#define VISIBILITY(v)
#endif

#ifdef GGL_SDK_EXPORT_API
#define GGL_EXPORT VISIBILITY(protected)
#else
#define GGL_EXPORT VISIBILITY(hidden)
#endif

#ifdef __has_attribute
#if __has_attribute(format)
#define FORMAT(...) __attribute__((format(__VA_ARGS__)))
#endif
#endif

#ifndef FORMAT
#define FORMAT(...)
#endif

#ifdef __has_attribute
#if __has_attribute(unused)
#define UNUSED __attribute__((unused))
#endif
#endif

#ifndef UNUSED
#define UNUSED
#endif

#ifdef __has_attribute
#if __has_attribute(counted_by)
#define COUNTED_BY(field) __attribute__((counted_by(field)))
#endif
#endif

#ifndef COUNTED_BY
#define COUNTED_BY(field)
#endif

#ifdef __has_attribute
#if __has_attribute(designated_init)
#define DESIGNATED_INIT __attribute__((designated_init))
#endif
#endif

#ifndef DESIGNATED_INIT
#define DESIGNATED_INIT
#endif

#ifdef __has_attribute
#if __has_attribute(alloc_size)
#define ALLOC_SIZE(...) __attribute__((alloc_size(__VA_ARGS__)))
#endif
#endif

#ifndef ALLOC_SIZE
#define ALLOC_SIZE(...)
#endif

#ifdef __has_attribute
#if __has_attribute(alloc_align)
#define ALLOC_ALIGN(pos) __attribute__((alloc_align(pos)))
#endif
#endif

#ifndef ALLOC_ALIGN
#define ALLOC_ALIGN(pos)
#endif

#ifdef __has_attribute
#if __has_attribute(nonnull)
#define NONNULL(...) __attribute__((nonnull(__VA_ARGS__)))
#endif
#endif

#ifndef NONNULL
#define NONNULL(...)
#endif

#ifdef __has_attribute
#if __has_attribute(nonnull_if_nonzero)
#define NONNULL_IF_NONZERO(...) __attribute__((nonnull_if_nonzero(__VA_ARGS__)))
#endif
#endif

#ifndef NONNULL_IF_NONZERO
#define NONNULL_IF_NONZERO(...)
#endif

#ifdef __has_attribute
#if __has_attribute(null_terminated_string_arg)
#define NULL_TERMINATED_STRING_ARG(pos) \
    __attribute__((null_terminated_string_arg(pos)))
#endif
#endif

#ifndef NULL_TERMINATED_STRING_ARG
#define NULL_TERMINATED_STRING_ARG(pos)
#endif

#ifdef __has_attribute
#if __has_attribute(access)
#define ACCESS(...) __attribute__((access(__VA_ARGS__)))
#endif
#endif

#ifndef ACCESS
#define ACCESS(...)
#endif

#ifdef __has_attribute
#if __has_attribute(cold)
#define COLD __attribute__((cold))
#endif
#endif

#ifndef COLD
#define COLD
#endif

#ifdef __has_attribute
#if __has_attribute(const)
#define CONST __attribute__((const))
#endif
#endif

#ifndef CONST
#define CONST
#endif

#ifdef __has_attribute
#if __has_attribute(pure)
#define PURE __attribute__((pure))
#endif
#endif

#ifndef PURE
#define PURE
#endif

#ifdef __has_attribute
#if __has_attribute(unsequenced)
#define UNSEQUENCED __attribute__((unsequenced))
#endif
#endif

#ifndef UNSEQUENCED
#define UNSEQUENCED
#endif

#ifdef __has_attribute
#if __has_attribute(reproducible)
#define REPRODUCIBLE __attribute__((reproducible))
#endif
#endif

#ifndef REPRODUCIBLE
#define REPRODUCIBLE
#endif

#ifdef __has_attribute
#if __has_attribute(flag_enum)
#define FLAG_ENUM __attribute__((flag_enum))
#endif
#endif

#ifndef FLAG_ENUM
#define FLAG_ENUM
#endif

#ifdef __has_attribute
#if __has_attribute(enum_extensibility)
#define ENUM_EXTENSIBILITY(type) __attribute__((enum_extensibility(type)))
#endif
#endif

#ifndef ENUM_EXTENSIBILITY
#define ENUM_EXTENSIBILITY(type)
#endif

#endif
