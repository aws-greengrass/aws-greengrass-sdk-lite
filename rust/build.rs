// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

use std::env;
use std::path::PathBuf;

fn main() {
    let out_dir = env::var("OUT_DIR").unwrap();
    let manifest_dir = env::var("CARGO_MANIFEST_DIR").unwrap();
    let project_root =
        PathBuf::from(&manifest_dir).parent().unwrap().to_path_buf();

    let dst = cmake::Config::new(&project_root)
        .define("CMAKE_POSITION_INDEPENDENT_CODE", "ON")
        .define("BUILD_CPP", "OFF")
        .build_target("ggl-sdk")
        .build();

    println!("cargo:rustc-link-search=native={}/build", dst.display());
    println!("cargo:rustc-link-lib=static=ggl-sdk");

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

    cc::Build::new()
        .file(wrapper_path.with_extension("c"))
        .include(project_root.join("include"))
        .include(&manifest_dir)
        .compile("bindgen_wrapper");

    println!("cargo:rerun-if-changed=wrapper.h");
    println!("cargo:rerun-if-changed=../../CMakeLists.txt");
    println!("cargo:rerun-if-changed=../../src");
    println!("cargo:rerun-if-changed=../../include");
    println!("cargo:rerun-if-changed=../../priv_include");
}
