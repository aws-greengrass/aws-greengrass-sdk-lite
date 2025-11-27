// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
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

    bindgen::Builder::default()
        .header(
            PathBuf::from(&manifest_dir)
                .join("wrapper.h")
                .to_str()
                .unwrap(),
        )
        .clang_arg(format!("-I{}", project_root.join("include").display()))
        .default_enum_style(bindgen::EnumVariation::Rust {
            non_exhaustive: false,
        })
        .generate_inline_functions(true)
        .enable_function_attribute_detection()
        .disable_nested_struct_naming()
        .derive_default(true)
        .generate_comments(false)
        .use_core()
        .generate_cstr(true)
        .sort_semantically(true)
        .allowlist_function("gg_.*")
        .allowlist_function("ggipc_.*")
        .allowlist_type("Gg.*")
        .allowlist_type("GgIpc.*")
        .allowlist_var("GG_.*")
        .raw_line("#![allow(non_upper_case_globals)]")
        .raw_line("#![allow(non_camel_case_types)]")
        .raw_line("#![allow(non_snake_case)]")
        .raw_line("#![allow(dead_code)]")
        .raw_line("#![allow(clippy::pedantic)]")
        .generate()
        .unwrap()
        .write_to_file(PathBuf::from(&manifest_dir).join("src/c.rs"))
        .unwrap();

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

    println!("cargo:rerun-if-changed=wrapper.h");
    println!("cargo:rerun-if-changed=../src");
    println!("cargo:rerun-if-changed=../include");
    println!("cargo:rerun-if-changed=../priv_include");
}
