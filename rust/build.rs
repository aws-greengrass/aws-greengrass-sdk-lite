// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//! Build script for ggl-sdk.

use std::env;
use std::path::PathBuf;

fn main() {
    unsafe {
        env::set_var("SOURCE_DATE_EPOCH", "0");
    }

    let manifest_dir = env::var("CARGO_MANIFEST_DIR").unwrap();
    let project_root =
        PathBuf::from(&manifest_dir).parent().unwrap().to_path_buf();
    let out_dir = env::var("OUT_DIR").unwrap();

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

    let wrapper_path = PathBuf::from(&out_dir).join("bindgen_wrapper");
    let bindings = bindgen::Builder::default()
        .header("wrapper.h")
        .clang_arg(format!("-I{}/include", project_root.display()))
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .default_enum_style(bindgen::EnumVariation::Rust {
            non_exhaustive: false,
        })
        .enable_function_attribute_detection()
        .disable_nested_struct_naming()
        .disable_header_comment()
        .derive_default(true)
        .use_core()
        .generate_cstr(true)
        .sort_semantically(true)
        .wrap_static_fns(true)
        .wrap_static_fns_path(&wrapper_path)
        .allowlist_function("ggl_.*")
        .allowlist_function("ggipc_.*")
        .allowlist_type("Ggl.*")
        .allowlist_type("GgIpc.*")
        .allowlist_var("GGL_.*")
        .generate()
        .expect("Failed to generate bindings");

    let out_path = PathBuf::from(&out_dir).join("bindings.rs");
    bindings
        .write_to_file(&out_path)
        .expect("Failed to write bindings");

    let mut build = cc::Build::new();
    for file in &src_files {
        build.flag(format!("-frandom-seed={}", file.display()));
    }

    build
        .compiler("clang")
        .files(&src_files)
        .file(wrapper_path.with_extension("c"))
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
        .define("GGL_MODULE", "\"ggl-sdk\"")
        .define("GGL_LOG_LEVEL", "GGL_LOG_DEBUG");

    if env::var("PROFILE").unwrap() == "release" {
        build.flag("-Oz");
        build.flag("-flto");
        build.flag("-ffat-lto-objects");
    }

    build.compile("ggl-sdk");

    println!("cargo:rerun-if-changed=wrapper.h");
    println!("cargo:rerun-if-changed=../src");
    println!("cargo:rerun-if-changed=../include");
    println!("cargo:rerun-if-changed=../priv_include");
}
