// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//! Build script for gg-sdk.

use std::env;
use std::path::PathBuf;

fn main() {
    unsafe {
        env::set_var("SOURCE_DATE_EPOCH", "0");
    }

    let manifest_dir = env::var("CARGO_MANIFEST_DIR").unwrap();
    let project_root =
        PathBuf::from(&manifest_dir).parent().unwrap().to_path_buf();

    let mut src_files = Vec::new();
    let mut dirs = vec![project_root.join("src")];
    while let Some(dir) = dirs.pop() {
        for entry in std::fs::read_dir(dir).unwrap() {
            let entry = entry.unwrap();
            let path = entry.path();
            if path.is_dir() {
                dirs.push(path);
            } else if path.extension().and_then(|s| s.to_str()) == Some("c") {
                src_files.push(path);
            }
        }
    }

    let mut build = cc::Build::new();
    for file in &src_files {
        build.flag(format!("-frandom-seed={}", file.display()));
    }

    build
        .compiler("clang")
        .files(&src_files)
        .include(project_root.join("include"))
        .include(project_root.join("priv_include"))
        .include(&manifest_dir)
        .flag("-pthread")
        .flag("-fno-strict-aliasing")
        .flag("-std=gnu11")
        .flag("-Wno-missing-braces")
        .flag("-fno-semantic-interposition")
        .flag("-fno-unwind-tables")
        .flag("-fno-asynchronous-unwind-tables")
        .flag("-fstrict-flex-arrays=3")
        .define("_GNU_SOURCE", None)
        .define("GG_MODULE", "\"gg-sdk\"")
        .define("GG_LOG_LEVEL", "GG_LOG_DEBUG");

    if env::var("PROFILE").unwrap() == "release" {
        build.flag("-Oz");
        build.flag("-flto");
        build.flag("-ffat-lto-objects");
    }

    build.compile("gg-sdk");

    println!("cargo:rerun-if-changed=../src");
    println!("cargo:rerun-if-changed=../include");
    println!("cargo:rerun-if-changed=../priv_include");
}
