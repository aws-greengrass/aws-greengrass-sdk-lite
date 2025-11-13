# aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

project(aws-greengrass-sdk-lite++ C CXX)

if(aws-greengrass-sdk-lite_IS_TOP_LEVEL)
  set(CMAKE_CXX_FLAGS_DEBUG "")
  set(CMAKE_CXX_FLAGS_RELEASE "")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "")
  set(CMAKE_CXX_FLAGS_MINSIZEREL "")

  add_cflags($<$<COMPILE_LANGUAGE:CXX>:-std=gnu++20>)
endif()

file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS "src/*.cpp")

foreach(src ${SRCS})
  set_property(
    SOURCE ${src}
    APPEND_STRING
    PROPERTY COMPILE_FLAGS "-frandom-seed=${src}")
endforeach()

if(aws-greengrass-sdk-lite_IS_TOP_LEVEL)
  add_library(gg-sdk++ STATIC ${SRCS})
else()
  add_library(gg-sdk++ STATIC EXCLUDE_FROM_ALL ${SRCS})
endif()

target_compile_options(gg-sdk++ PRIVATE -pthread -fno-strict-aliasing
                                        -std=gnu++20)
target_include_directories(gg-sdk++ PRIVATE include priv_include)
target_include_directories(gg-sdk++ SYSTEM INTERFACE include)
target_compile_definitions(gg-sdk++ PRIVATE "GG_MODULE=(\"gg-sdk++\")")
target_link_libraries(gg-sdk++ PRIVATE gg-sdk)

if(aws-greengrass-sdk-lite_IS_TOP_LEVEL)
  install(TARGETS gg-sdk++)

  if(BUILD_SAMPLES)
    file(GLOB SAMPLE_DIRS CONFIGURE_DEPENDS "samples/*")
    foreach(sample_dir ${SAMPLE_DIRS})
      if(NOT IS_DIRECTORY ${sample_dir})
        continue()
      endif()
      get_filename_component(sample_name ${sample_dir} NAME_WLE)
      set_property(
        SOURCE ${sample_dir}/main.cpp
        APPEND_STRING
        PROPERTY COMPILE_FLAGS "-frandom-seed=${sample_dir}/main.cpp")
      add_executable(sample_cpp_${sample_name} ${sample_dir}/main.cpp)
      target_link_libraries(sample_cpp_${sample_name} PRIVATE gg-sdk++)
      install(TARGETS sample_cpp_${sample_name})
    endforeach()
  endif()

  if(BUILD_TESTING)
    file(GLOB TEST_DIRS CONFIGURE_DEPENDS "test/*")
    foreach(test_dir ${TEST_DIRS})
      if(NOT IS_DIRECTORY ${test_dir})
        continue()
      endif()
      get_filename_component(test_name ${test_dir} NAME_WLE)
      set_property(
        SOURCE ${test_dir}/main.cpp
        APPEND_STRING
        PROPERTY COMPILE_FLAGS "-frandom-seed=${test_dir}/main.cpp")
      add_executable(test_cpp_${test_name} ${test_dir}/main.cpp)
      target_link_libraries(test_cpp_${test_name} PRIVATE gg-sdk++ gg-ipc-mock
                                                          PkgConfig::unity)
      install(TARGETS test_cpp_${test_name})
    endforeach()
  endif()
endif()
