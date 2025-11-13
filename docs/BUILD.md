# Building `aws-greengrass-component-sdk` and its samples

This document will walk you through building the SDK, as well as the included
sample Greengrass v2 components.

The SDK has no third-party library dependencies. Currently, only Linux targets
using Glibc or Musl are supported.

This repo provides a CMake project that can be built standalone or included in
your CMake project. It should be simple to add to another build system of your
choice if you do not use CMake.

## Build tools

To build the SDK and samples, you will need the following build dependencies:

- GCC or Clang
- CMake (at least version 3.22)
- Make or Ninja

On Ubuntu, these can be installed with:

```sh
sudo apt install build-essential cmake
```

## Building examples

To build the examples, clone the repo. The following commands assume you are in
the project root directory. Run the following to build:

```sh
cmake -B build -D CMAKE_BUILD_TYPE=MinSizeRel -D BUILD_SAMPLES=ON
make -C build -j$(nproc)
```

You will find the sample binaries in `./build/bin`.

## Building just the SDK

If you want to build just the SDK to link into your non-CMake project, you can
run the following:

```sh
cmake -B build -D CMAKE_BUILD_TYPE=MinSizeRel -D BUILD_SAMPLES=OFF
make -C build -j$(nproc)
```

The library will be available at `./build/libgg-sdk.a`.

## Adding to a CMake project

To include the SDK in your CMake project, you can obtain the repo with a git
submodule or CMake FetchContent and then call `add_subdirectory` on it. A
library target named `gg-sdk` will be available in your project to link against.
