# aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.22)
project(aws-greengrass-sdk-lite C)

set(GGL_LOG_LEVEL CACHE STRING "GGL log level")

if(PROJECT_IS_TOP_LEVEL)

  option(ENABLE_WERROR "Compile warnings as errors")

  option(BUILD_SAMPLES "Build sample components" ON)

  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

  include(GNUInstallDirs)

  # Put outputs in build/bin and build/lib
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
      "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
      "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")

  if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(WARNING "CMAKE_BUILD_TYPE not set, using MinSizeRel.")
    set(CMAKE_BUILD_TYPE MinSizeRel)
  endif()

  # Enable a compiler option for compile and link
  macro(add_cflags)
    add_compile_options(${ARGN})
    add_link_options(${ARGN})
  endmacro()

  include(CheckCCompilerFlag)
  include(CheckLinkerFlag)

  # Enable a compiler/linker option if supported with a condition
  macro(try_add_cflag_if name cond option)
    check_c_compiler_flag("${option}" compiler_has_${name})
    if(compiler_has_${name})
      add_cflags("$<${cond}:${option}>")
    endif()
  endmacro()

  # Enable a compiler/linker option if supported
  macro(try_add_cflag name option)
    try_add_cflag_if(${name} 1 ${option})
  endmacro()

  # Enable a linker option if supported with a condition
  macro(try_add_link_option_if name cond option)
    check_linker_flag(C "${option}" linker_has_${name})
    if(linker_has_${name})
      add_link_options("$<${cond}:${option}>")
    endif()
  endmacro()

  # Enable a linker option if supported
  macro(try_add_link_option name option)
    try_add_link_option_if(${name} 1 ${option})
  endmacro()

  # Clear CMake defaults
  set(CMAKE_C_FLAGS_DEBUG "")
  set(CMAKE_C_FLAGS_RELEASE "")
  set(CMAKE_C_FLAGS_RELWITHDEBINFO "")
  set(CMAKE_C_FLAGS_MINSIZEREL "")

  set(CMAKE_COLOR_DIAGNOSTICS ON)

  add_cflags($<$<CONFIG:Debug>:-O0>)
  add_cflags($<$<CONFIG:Release,RelWithDebInfo>:-O3>)
  try_add_cflag_if(Oz $<CONFIG:MinSizeRel> -Oz)
  if(NOT compiler_has_Oz)
    add_cflags($<$<CONFIG:MinSizeRel>:-Os>)
  endif()

  add_cflags($<$<CONFIG:Debug,RelWithDebInfo>:-ggdb3>)

  # These flags are required
  add_cflags(-pthread -fno-strict-aliasing)

  # Following flags are optional
  add_cflags(-std=gnu11 -fvisibility=hidden -fno-semantic-interposition
             -fno-common)

  try_add_cflag(strict-flex-arrays -fstrict-flex-arrays=3)

  try_add_cflag(macro-prefix-map
                "-fmacro-prefix-map=${CMAKE_CURRENT_LIST_DIR}/=")

  add_cflags($<$<CONFIG:Debug>:-fno-omit-frame-pointer>)
  try_add_cflag_if(trivial-auto-var-init-pattern $<CONFIG:Debug>
                   -ftrivial-auto-var-init=pattern)

  try_add_cflag_if(function-sections $<NOT:$<CONFIG:Debug>> -ffunction-sections)
  try_add_cflag_if(data-sections $<NOT:$<CONFIG:Debug>> -fdata-sections)

  if(ENABLE_WERROR)
    add_cflags(-Werror)
  endif()

  add_cflags(
    -Wall
    -Wextra
    -Wno-missing-braces
    -Wvla
    -Wshadow
    -Wformat
    -Wformat=2
    -Wmissing-prototypes
    -Wstrict-prototypes
    -Wold-style-definition
    -Wunused
    -Wundef
    -Wconversion
    -Wsign-conversion
    -Wimplicit-fallthrough
    -Wredundant-decls
    -Wdate-time
    -Wstack-protector)

  try_add_cflag(Wenum-int-mismatch -Wenum-int-mismatch)
  try_add_cflag(Wtrampolines -Wtrampolines)
  try_add_cflag(Wbidi-chars -Wbidi-chars=any,ucn)

  add_cflags(-Werror=format-security -Werror=implicit
             -Werror=incompatible-pointer-types -Werror=int-conversion)

  add_link_options(
    LINKER:-z,relro,-z,now,-z,noexecstack,-z,nodlopen LINKER:--as-needed
    LINKER:--no-copy-dt-needed-entries
    LINKER:--enable-new-dtags,--hash-style=gnu)
  try_add_link_option(compress-debug-sections-zlib
                      LINKER:--compress-debug-sections=zlib)

  try_add_link_option_if(gc-sections $<NOT:$<CONFIG:Debug>>
                         LINKER:--gc-sections)

  try_add_link_option_if(strip-all $<CONFIG:Release,MinSizeRel> LINKER:-s)

  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG FALSE)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

  set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
  set(CMAKE_INSTALL_RPATH "\$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")

  set(ENV{SOURCE_DATE_EPOCH} "0")
  set(ENV{ZERO_AR_DATE} "1")

endif()

file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS "src/*.c")
list(LENGTH SRCS SRCS_LEN)

foreach(src ${SRCS})
  set_property(
    SOURCE ${src}
    APPEND_STRING
    PROPERTY COMPILE_FLAGS "-frandom-seed=${src}")
endforeach()

add_library(ggl-sdk STATIC ${SRCS})
target_compile_options(ggl-sdk PRIVATE -pthread -fno-strict-aliasing -std=gnu11)
target_compile_definitions(ggl-sdk PRIVATE _GNU_SOURCE)
target_include_directories(ggl-sdk PRIVATE include priv_include)
target_include_directories(ggl-sdk SYSTEM INTERFACE include)

target_compile_definitions(ggl-sdk PRIVATE "GGL_MODULE=(\"ggl-sdk\")")
string(TOUPPER "${GGL_LOG_LEVEL}" log_level)
set(choose_level "$<IF:$<BOOL:${log_level}>,${log_level},DEBUG>")
target_compile_definitions(ggl-sdk PUBLIC GGL_LOG_LEVEL=GGL_LOG_${choose_level})

if(PROJECT_IS_TOP_LEVEL)
  install(TARGETS ggl-sdk)

  if(BUILD_SAMPLES)
    file(GLOB SAMPLE_DIRS CONFIGURE_DEPENDS "samples/*")
    foreach(sample_dir ${SAMPLE_DIRS})
      get_filename_component(sample_name ${sample_dir} NAME_WLE)
      set_property(
        SOURCE ${sample_dir}/main.c
        APPEND_STRING
        PROPERTY COMPILE_FLAGS "-frandom-seed=${sample_dir}/main.c")
      add_executable(sample_${sample_name} ${sample_dir}/main.c)
      target_link_libraries(sample_${sample_name} PRIVATE ggl-sdk)
      install(TARGETS sample_${sample_name})
    endforeach()
  endif()
endif()
