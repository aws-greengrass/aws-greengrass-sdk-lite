file(GLOB_RECURSE UNITY_SRCS CONFIGURE_DEPENDS "${unity_SOURCE_DIR}/src/*.c")
file(GLOB_RECURSE UNITY_CFG_SRCS CONFIGURE_DEPENDS "unity/unity_config/*.c")
add_library(unity EXCLUDE_FROM_ALL ${UNITY_SRCS} ${UNITY_CFG_SRCS})

# Unity includes the config as "unity_config.h" inside of <unity.h>
target_include_directories(unity PUBLIC unity/unity_config)
target_include_directories(unity SYSTEM INTERFACE "${unity_SOURCE_DIR}/src")
target_include_directories(unity PRIVATE "${unity_SOURCE_DIR}/src" priv_include)

target_compile_definitions(unity PUBLIC "UNITY_INCLUDE_CONFIG_H=1")
target_compile_definitions(unity PRIVATE "GG_MODULE=(\"gg-unity\")")

target_link_libraries(unity PRIVATE gg-sdk)

file(GLOB_RECURSE GG_TEST_SRCS CONFIGURE_DEPENDS "unity/gg_test/*.c")
add_library(gg-test EXCLUDE_FROM_ALL ${GG_TEST_SRCS})

target_include_directories(gg-test SYSTEM INTERFACE unity/gg_test/include)
target_include_directories(gg-test PRIVATE unity/gg_test/include priv_include)
target_compile_definitions(gg-test PRIVATE "GG_MODULE=(\"gg-test\")")

target_link_libraries(gg-test PRIVATE gg-sdk unity)

# Contains boilerplate main files for linking test executables
set(aws-greengrass-component-sdk_C_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test)

if(NOT BUILD_SHARED_LIBS)
  file(GLOB_RECURSE MOCKS CONFIGURE_DEPENDS "mock/*.c")
  add_library(gg-ipc-mock EXCLUDE_FROM_ALL ${MOCKS})

  target_compile_options(gg-ipc-mock PRIVATE -pthread -fno-strict-aliasing
                                             -std=gnu11 -Wno-missing-braces)
  target_compile_definitions(gg-ipc-mock PRIVATE _GNU_SOURCE)
  target_include_directories(gg-ipc-mock PRIVATE mock include priv_include)
  target_include_directories(gg-ipc-mock SYSTEM INTERFACE mock priv_include)
  target_compile_definitions(gg-ipc-mock PRIVATE "GG_MODULE=(\"gg-test\")")
  target_link_libraries(gg-ipc-mock PUBLIC gg-sdk gg-test)
  target_link_libraries(gg-ipc-mock PRIVATE m)
endif()
