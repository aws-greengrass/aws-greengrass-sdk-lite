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

add_library(ggl-sdk++ STATIC ${SRCS})

target_compile_options(ggl-sdk++ PRIVATE -pthread -fno-strict-aliasing
                                         -std=gnu++20)
target_include_directories(ggl-sdk++ PRIVATE include priv_include)
target_include_directories(ggl-sdk++ SYSTEM INTERFACE include)
target_compile_definitions(ggl-sdk++ PRIVATE "GGL_MODULE=(\"ggl-sdk++\")")
target_link_libraries(ggl-sdk++ PRIVATE ggl-sdk)

if(aws-greengrass-sdk-lite_IS_TOP_LEVEL)
  install(TARGETS ggl-sdk++)

  if(BUILD_SAMPLES)
    file(GLOB SAMPLE_DIRS CONFIGURE_DEPENDS "samples/*")
    foreach(sample_dir ${SAMPLE_DIRS})
      get_filename_component(sample_name ${sample_dir} NAME_WLE)
      set_property(
        SOURCE ${sample_dir}/main.cpp
        APPEND_STRING
        PROPERTY COMPILE_FLAGS "-frandom-seed=${sample_dir}/main.cpp")
      add_executable(sample_${sample_name} ${sample_dir}/main.cpp)
      target_link_libraries(sample_${sample_name} PRIVATE ggl-sdk++)
      install(TARGETS sample_${sample_name})
    endforeach()
  endif()
endif()
