cmake_minimum_required(VERSION 3.22)

project(aws-greengrass-sdk-lite-cpp CXX)

if(PROJECT_IS_TOP_LEVEL)

  option(ENABLE_WERROR "Compile warnings as errors")

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
  macro(add_cxxflags)
    add_compile_options(${ARGN})
    add_link_options(${ARGN})
  endmacro()

  include(CheckCXXCompilerFlag)
  include(CheckLinkerFlag)

  # Enable a compiler/linker option if supported with a condition
  macro(try_add_cxxflag_if name cond option)
    check_cxx_compiler_flag("${option}" compiler_has_${name})
    if(compiler_has_${name})
      add_cxxflags("$<${cond}:${option}>")
    endif()
  endmacro()

  # Enable a compiler/linker option if supported
  macro(try_add_cxxflag name option)
    try_add_cxxflag_if(${name} 1 ${option})
  endmacro()

  # Enable a linker option if supported with a condition
  macro(try_add_link_option_if name cond option)
    check_linker_flag(CXX "${option}" linker_has_${name})
    if(linker_has_${name})
      add_link_options("$<${cond}:${option}>")
    endif()
  endmacro()

  # Enable a linker option if supported
  macro(try_add_link_option name option)
    try_add_link_option_if(${name} 1 ${option})
  endmacro()

  # Clear CMake defaults
  set(CMAKE_CXX_FLAGS_DEBUG "")
  set(CMAKE_CXX_FLAGS_RELEASE "")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "")
  set(CMAKE_CXX_FLAGS_MINSIZEREL "")

  set(CMAKE_COLOR_DIAGNOSTICS ON)

  add_cxxflags($<$<CONFIG:Debug>:-O0>)
  add_cxxflags($<$<CONFIG:Release,RelWithDebInfo>:-O3>)
  try_add_cxxflag_if(Oz $<CONFIG:MinSizeRel> -Oz)
  if(NOT compiler_has_Oz)
    add_cxxflags($<$<CONFIG:MinSizeRel>:-Os>)
  endif()

  add_cxxflags($<$<CONFIG:Debug,RelWithDebInfo>:-ggdb3>)

  # These flags are required
  add_cxxflags(-pthread -fno-strict-aliasing)

  # Following flags are optional
  add_cxxflags(-std=gnu++20 -fvisibility=hidden -fno-semantic-interposition
               -fno-common)

  try_add_cxxflag(strict-flex-arrays -fstrict-flex-arrays=3)

  try_add_cxxflag(macro-prefix-map
                  "-fmacro-prefix-map=${CMAKE_CURRENT_LIST_DIR}/=")

  add_cxxflags($<$<CONFIG:Debug>:-fno-omit-frame-pointer>)
  try_add_cxxflag_if(trivial-auto-var-init-pattern $<CONFIG:Debug>
                     -ftrivial-auto-var-init=pattern)

  try_add_cxxflag_if(function-sections $<NOT:$<CONFIG:Debug>>
                     -ffunction-sections)
  try_add_cxxflag_if(data-sections $<NOT:$<CONFIG:Debug>> -fdata-sections)

  if(ENABLE_WERROR)
    add_cxxflags(-Werror)
  endif()

  add_cxxflags(
    -Wall
    -Wextra
    -Wwrite-strings
    -Wno-missing-braces
    -Wvla
    -Wformat
    -Wformat=2
    -Wunused
    -Wundef
    -Wconversion
    -Wsign-conversion
    -Wimplicit-fallthrough
    -Wredundant-decls
    -Wdate-time
    -Wstack-protector)

  try_add_cxxflag(Wenum-int-mismatch -Wenum-int-mismatch)
  try_add_cxxflag(Wtrampolines -Wtrampolines)
  try_add_cxxflag(Wbidi-chars -Wbidi-chars=any,ucn)

  add_cxxflags(-Werror=format-security)

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

file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS "src/*.cpp")

add_library(ggl-sdk++ STATIC ${SRCS})

set_property(TARGET ggl-sdk++ PROPERTY CXX_STANDARD 20)
target_include_directories(ggl-sdk++ PRIVATE include)
target_include_directories(ggl-sdk++ SYSTEM INTERFACE include)
