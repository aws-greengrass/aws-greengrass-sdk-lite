# aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.22)
project(aws-greengrass-component-sdk C)

set(GG_LOG_LEVEL CACHE STRING "GG log level")

if(PROJECT_IS_TOP_LEVEL)

  option(ENABLE_WERROR "Compile warnings as errors")

  option(BUILD_SAMPLES "Build sample components" ON)

  option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

  option(BUILD_CPP "Build C++ library" ON)

  option(BUILD_TESTING "Build C/C++ testing" OFF)

  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

  include(GNUInstallDirs)

  if(BUILD_TESTING)
    find_package(PkgConfig REQUIRED)
    pkg_search_module(unity REQUIRED IMPORTED_TARGET unity)
  endif()

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

  add_cflags($<$<COMPILE_LANGUAGE:C>:-std=gnu11>)

  add_cflags(-fno-semantic-interposition -fno-common)

  # This stops GCC from generating unstrippable inline DWARF debug info
  add_cflags(-fno-unwind-tables -fno-asynchronous-unwind-tables)

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
    -Wwrite-strings
    -Wno-missing-braces
    -Wvla
    $<$<COMPILE_LANGUAGE:C>:-Wshadow>
    -Wformat
    -Wformat=2
    $<$<COMPILE_LANGUAGE:C>:-Wmissing-prototypes>
    $<$<COMPILE_LANGUAGE:C>:-Wstrict-prototypes>
    $<$<COMPILE_LANGUAGE:C>:-Wold-style-definition>
    -Wunused
    -Wundef
    -Wconversion
    -Wsign-conversion
    -Wimplicit-fallthrough
    -Wredundant-decls
    -Wdate-time
    -Wstack-protector)

  try_add_cflag_if(Wenum-int-mismatch $<COMPILE_LANGUAGE:C> -Wenum-int-mismatch)
  try_add_cflag(Wtrampolines -Wtrampolines)
  try_add_cflag(Wbidi-chars -Wbidi-chars=any,ucn)

  add_cflags(
    -Werror=format-security $<$<COMPILE_LANGUAGE:C>:-Werror=implicit>
    $<$<COMPILE_LANGUAGE:C>:-Werror=incompatible-pointer-types>
    $<$<COMPILE_LANGUAGE:C>:-Werror=int-conversion>)

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
  if(NOT DEFINED CMAKE_INTERPROCEDURAL_OPTIMIZATION)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
  endif()

  set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
  set(CMAKE_INSTALL_RPATH "\$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")

  set(ENV{SOURCE_DATE_EPOCH} "0")
  set(ENV{ZERO_AR_DATE} "1")

endif()

file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS "src/*.c")

foreach(src ${SRCS})
  set_property(
    SOURCE ${src}
    APPEND_STRING
    PROPERTY COMPILE_FLAGS "-frandom-seed=${src}")
endforeach()

if(PROJECT_IS_TOP_LEVEL)
  add_library(gg-sdk ${SRCS})
else()
  add_library(gg-sdk EXCLUDE_FROM_ALL ${SRCS})
endif()

target_compile_options(gg-sdk PRIVATE -pthread -fno-strict-aliasing -std=gnu11
                                      -Wno-missing-braces)
target_compile_definitions(gg-sdk PRIVATE _GNU_SOURCE)
target_include_directories(gg-sdk PRIVATE include priv_include)
target_include_directories(gg-sdk SYSTEM INTERFACE include)

target_compile_definitions(gg-sdk PRIVATE "GG_MODULE=(\"gg-sdk\")")

string(TOUPPER "${GG_LOG_LEVEL}" log_level)
set(choose_level "$<IF:$<BOOL:${log_level}>,${log_level},DEBUG>")
target_compile_definitions(gg-sdk PUBLIC GG_LOG_LEVEL=GG_LOG_${choose_level})

if(PROJECT_IS_TOP_LEVEL)
  install(TARGETS gg-sdk)

  if(BUILD_SAMPLES)
    file(GLOB SAMPLE_DIRS CONFIGURE_DEPENDS "samples/*")
    foreach(sample_dir ${SAMPLE_DIRS})
      if(NOT IS_DIRECTORY ${sample_dir})
        continue()
      endif()
      get_filename_component(sample_name ${sample_dir} NAME_WLE)
      set_property(
        SOURCE ${sample_dir}/main.c
        APPEND_STRING
        PROPERTY COMPILE_FLAGS "-frandom-seed=${sample_dir}/main.c")
      add_executable(sample_${sample_name} ${sample_dir}/main.c)
      target_link_libraries(sample_${sample_name} PRIVATE gg-sdk)
      install(TARGETS sample_${sample_name})
    endforeach()
  endif()

  if(BUILD_TESTING)
    include(CTest)
    file(GLOB_RECURSE MOCKS CONFIGURE_DEPENDS "mock/*.c")
    add_library(gg-ipc-mock ${MOCKS})

    target_compile_options(gg-ipc-mock PRIVATE -pthread -fno-strict-aliasing
                                               -std=gnu11 -Wno-missing-braces)
    target_compile_definitions(gg-ipc-mock PRIVATE _GNU_SOURCE)
    target_include_directories(gg-ipc-mock PRIVATE mock include priv_include)
    target_include_directories(gg-ipc-mock SYSTEM INTERFACE mock priv_include)
    target_compile_definitions(gg-ipc-mock PUBLIC "GG_MODULE=(\"gg-test\")")
    target_link_libraries(gg-ipc-mock PRIVATE gg-sdk m)

    file(GLOB TEST_DIRS CONFIGURE_DEPENDS "test/*")
    foreach(test_dir ${TEST_DIRS})
      if(NOT IS_DIRECTORY ${test_dir})
        continue()
      endif()
      get_filename_component(test_name ${test_dir} NAME_WLE)
      set_property(
        SOURCE ${test_dir}/main.c
        APPEND_STRING
        PROPERTY COMPILE_FLAGS "-frandom-seed=${test_dir}/main.c")
      add_executable(test_${test_name} ${test_dir}/main.c)
      target_link_libraries(test_${test_name} PRIVATE gg-sdk gg-ipc-mock
                                                      PkgConfig::unity)
      target_compile_options(test_${test_name} PRIVATE)
      target_link_options(test_${test_name} PUBLIC)
      install(TARGETS test_${test_name})
      add_test(test_${test_name} ${CMAKE_INSTALL_BINDIR}/test_${test_name})
    endforeach()
  endif()
endif()

if(BUILD_CPP)
  add_subdirectory(cpp)
endif()
