# aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

{
  description = "Lightweight AWS IoT Greengrass SDK.";
  inputs.flakelight.url = "github:nix-community/flakelight";
  outputs = { flakelight, ... }@inputs: flakelight ./.
    ({ lib, config, ... }:
      let
        filteredSrc = lib.fileset.toSource {
          root = ./.;
          fileset = lib.fileset.unions [
            ./CMakeLists.txt
            ./include
            ./priv_include
            ./src
            ./.clang-tidy
            ./.clangd
            ./mock
            ./samples
            ./test
            ./cpp/CMakeLists.txt
            ./cpp/include
            ./cpp/priv_include
            ./cpp/src
            ./cpp/.clang-tidy
            ./cpp/samples
          ];
        };

        llvmStdenv = pkgs: pkgs.overrideCC pkgs.llvmPackages_latest.stdenv
          (pkgs.llvmPackages_latest.stdenv.cc.override
            { inherit (pkgs.llvmPackages_latest) bintools; });
      in
      {
        inherit inputs;
        systems = [ "x86_64-linux" "aarch64-linux" ];

        devShell = pkgs: {
          packages = with pkgs; [
            llvmPackages_latest.clang-tools
            clangd-tidy
            git
            cmake
            cbmc
            cargo
            pkg-config
            unity-test
            rustc
            rust-bindgen
            llvmPackages_latest.clang
            clippy
            rustfmt
            rust-analyzer-unwrapped
          ];
          env = { rustPlatform, ... }: {
            NIX_HARDENING_ENABLE = "";
            RUST_SRC_PATH = "${rustPlatform.rustLibSrc}";
          };
          shellHook = ''
            export MAKEFLAGS=-j
          '';
        };

        devShells.clang = { lib, pkgs, ... }: {
          imports = [ (config.devShell pkgs) ];
          stdenv = lib.mkForce (llvmStdenv pkgs);
        };

        apps.rust-bindgen = pkgs: ''
          export PATH=${lib.makeBinPath (with pkgs; [ rust-bindgen rustfmt ])}:$PATH
          ${./. + "/misc/run_bindgen.sh"}
        '';

        formatters =
          { llvmPackages_latest
          , cmake-format
          , rustfmt
          , nodePackages
          , yapf
          , ...
          }:
          let
            fmt-c = "${llvmPackages_latest.clang-unwrapped}/bin/clang-format -i";
            fmt-cmake = "${cmake-format}/bin/cmake-format -i";
            fmt-yaml =
              "${nodePackages.prettier}/bin/prettier --write --parser yaml";
          in
          {
            "*.c" = fmt-c;
            "*.h" = fmt-c;
            "*.cpp" = fmt-c;
            "*.hpp" = fmt-c;
            "CMakeLists.txt" = fmt-cmake;
            ".clang*" = fmt-yaml;
            "*.rs" = "${rustfmt}/bin/rustfmt";
            "*.py" = "${yapf}/bin/yapf -i";
          };

        pname = "gg-sdk";
        package = { stdenv, cmake, ninja, static ? true }:
          stdenv.mkDerivation {
            name = "gg-sdk";
            src = filteredSrc;
            nativeBuildInputs = [ cmake ninja ];
            cmakeBuildType = "MinSizeRel";
            cmakeFlags = [ "-DENABLE_WERROR=1" ]
              ++ lib.optional (!static) "-DBUILD_SHARED_LIBS=1";
            hardeningDisable = [ "all" ];
            dontStrip = true;
          };

        packages = {
          gg-sdk-static = { pkgsStatic }: pkgsStatic.gg-sdk;
          gg-sdk-static-aarch64 = { pkgsCross }:
            pkgsCross.aarch64-multiplatform-musl.gg-sdk-static;
          gg-sdk-static-armv7l = { pkgsCross }:
            pkgsCross.armv7l-hf-multiplatform.gg-sdk-static;
        };

        checks =
          let
            clangBuildDir = { pkgs, clang-tools, cmake, ... }:
              (llvmStdenv pkgs).mkDerivation {
                name = "clang-cmake-build-dir";
                nativeBuildInputs = [ clang-tools ];
                buildPhase = ''
                  ${cmake}/bin/cmake -B $out -S ${filteredSrc} \
                    -D CMAKE_BUILD_TYPE=Debug -D BUILD_CPP=OFF
                  rm $out/CMakeFiles/CMakeConfigureLog.yaml
                '';
                dontUnpack = true;
                dontPatch = true;
                dontConfigure = true;
                dontInstall = true;
                dontFixup = true;
                allowSubstitutes = false;
              };

            clangBuildDirCpp = { pkgs, clang-tools, cmake, ... }:
              (llvmStdenv pkgs).mkDerivation {
                name = "clang-cmake-build-dir";
                nativeBuildInputs = [ clang-tools ];
                buildPhase = ''
                  ${cmake}/bin/cmake -B $out -S ${filteredSrc} \
                    -D CMAKE_BUILD_TYPE=Debug
                  rm $out/CMakeFiles/CMakeConfigureLog.yaml
                '';
                dontUnpack = true;
                dontPatch = true;
                dontConfigure = true;
                dontInstall = true;
                dontFixup = true;
                allowSubstitutes = false;
              };

            shared-lib = pkgs: pkgs.gg-sdk.override { static = false; };
          in
          {
            build-clang = pkgs: pkgs.gg-sdk.override
              { stdenv = llvmStdenv pkgs; };

            build-shared = shared-lib;

            clang-tidy = pkgs: ''
              set -eo pipefail
              cd ${filteredSrc}
              PATH=${lib.makeBinPath
                (with pkgs; [clangd-tidy clang-tools fd])}:$PATH
              clangd-tidy -j$(nproc) --color=always \
                -p ${clangBuildDir pkgs} \
                $(fd . src include priv_include -e c -e h)
            '';

            clang-tidy-cpp = pkgs: ''
              set -eo pipefail
              cd ${filteredSrc}
              PATH=${lib.makeBinPath
                (with pkgs; [clangd-tidy clang-tools fd])}:$PATH
              clangd-tidy -j$(nproc) --color=always \
                -p ${clangBuildDirCpp pkgs} \
                $(fd . cpp/src cpp/include cpp/priv_include \
                  -e c -e h -e cpp -e hpp)
            '';

            namespacing = pkgs:
              let so = "${shared-lib pkgs}/lib/libgg-sdk.so"; in
              ''
                set -eo pipefail
                PATH=${lib.makeBinPath
                  (with pkgs; [gcc gnugrep])}:$PATH
                nm -D --defined-only ${so} | cut -d' ' -f3 > syms
                grep -v '^gg_\|^ggipc_' syms && exit 1 || true
              '';

            cbmc-contracts = { stdenv, cmake, cbmc, python3, ... }:
              stdenv.mkDerivation {
                name = "check-cbmc-contracts";
                src = filteredSrc;
                nativeBuildInputs = [ cbmc python3 ];
                buildPhase = ''
                  ${cmake}/bin/cmake -B build -D CMAKE_BUILD_TYPE=Debug \
                    -D GG_LOG_LEVEL=TRACE
                  python ${./misc/check_contracts.py} src
                  touch $out
                '';
                dontPatch = true;
                dontConfigure = true;
                dontInstall = true;
                dontFixup = true;
                allowSubstitutes = false;
              };

            unit-tests = { stdenv, unity-test, cmake, pkg-config, ninja, ... }:
              stdenv.mkDerivation {
                name = "check-unity-tests";
                src = filteredSrc;
                nativeBuildInputs = [ cmake pkg-config unity-test ninja ];
                cmakeBuildType = "MinSizeRel";
                cmakeFlags = [ "-DBUILD_TESTING=1" ];
                postBuild = ''
                  ctest --output-on-failure
                '';
                installPhase = "touch $out";
              };

            iwyu = pkgs: ''
              set -eo pipefail
              PATH=${lib.makeBinPath
                (with pkgs; [include-what-you-use fd])}:$PATH
              white=$(printf "\e[1;37m")
              red=$(printf "\e[1;31m")
              clear=$(printf "\e[0m")
              iwyu_tool.py -o clang -j $(nproc) -p ${clangBuildDirCpp pkgs} \
                $(fd . ${filteredSrc}/ -e c -e cpp) -- \
                -Xiwyu --error -Xiwyu --check_also="${filteredSrc}/*" \
                -Xiwyu --mapping_file=${./.}/misc/iwyu_mappings.yml |\
                { grep error: || true; } |\
                sed 's|\(.*\)error:\(.*\)|'$white'\1'$red'error:'$white'\2'$clear'|' |\
                sed 's|${filteredSrc}/||'
            '';

            cmake-lint = pkgs: ''
              ${pkgs.cmake-format}/bin/cmake-lint \
                $(${pkgs.fd}/bin/fd '.*\.cmake|CMakeLists.txt') \
                --suppress-decorations
            '';

            spelling = pkgs: ''
              ${pkgs.nodePackages.cspell}/bin/cspell "**" --quiet
              ${pkgs.coreutils}/bin/sort -cuf misc/dictionary.txt
            '';

            build-rust-crate =
              { rustPlatform
              , clippy
              , llvmPackages_latest
              , ...
              }:
              let
                meta = (fromTOML (builtins.readFile ./rust/Cargo.toml)).package;
              in
              rustPlatform.buildRustPackage {
                pname = "${meta.name}-rs";
                inherit (meta) version;
                nativeBuildInputs = [
                  clippy
                  llvmPackages_latest.clang
                ];
                dontUseCmakeConfigure = true;
                src = lib.fileset.toSource {
                  root = ./.;
                  fileset = lib.fileset.unions [
                    ./CMakeLists.txt
                    ./include
                    ./priv_include
                    ./src
                    ./rust
                  ];
                };
                cargoLock.lockFile = ./rust/Cargo.lock;
                cargoRoot = "rust";
                buildAndTestSubdir = "rust";
                postCheck = ''
                  pushd rust
                  cargo clippy --profile $cargoCheckType -- --deny warnings
                  popd
                '';
              };
            bindgen = { lib, rust-bindgen, rustfmt, diffutils, ... }: ''
              export PATH=${lib.makeBinPath [ rust-bindgen rustfmt ]}:$PATH
              ${./. + "/misc/run_bindgen.sh"}
              ${diffutils}/bin/diff -qr ${./.} .
            '';
          };

        withOverlays = final: prev: {
          clangd-tidy = final.callPackage
            ({ python3Packages }:
              python3Packages.buildPythonPackage rec {
                pname = "clangd_tidy";
                version = "1.1.0.post2";
                format = "pyproject";
                src = final.fetchPypi {
                  inherit pname version;
                  hash = "sha256-NyghLY+BeY9LAOstKEFcPLdA7l1jCdHLuyPms4bOyYE=";
                };
                buildInputs = with python3Packages; [ setuptools-scm ];
                propagatedBuildInputs = with python3Packages; [
                  attrs
                  cattrs
                  typing-extensions
                ];
              })
            { };
          cbmc = prev.cbmc.overrideAttrs {
            version = "6.7.1";
            src = final.fetchFromGitHub {
              owner = "diffblue";
              repo = "cbmc";
              # Includes commits after 6.7.1 needed for harness generation
              # Use tag when 6.7.2 is released
              rev = "062962c1da7149be418338b1f2220d51960e06f8";
              hash = "sha256-qcCiKv+AUoiIZdiCK955Bl5GBK+JHv0mDflQ4aAj4IQ=";
            };
          };
        };

        legacyPackages = pkgs: {
          _type = "pkgs";
          cached-paths = pkgs.stdenv.mkDerivation {
            name = "cached-paths";
            exportReferencesGraph =
              let
                getAttrSet = name: lib.mapAttrs'
                  (n: lib.nameValuePair ("${name}-" + n))
                  pkgs.outputs'.${name};
                cached-outputs = pkgs.linkFarm "cached-outputs" (
                  (getAttrSet "packages") //
                  (getAttrSet "devShells") //
                  (getAttrSet "checks") //
                  { "formatter" = pkgs.outputs'.formatter; }
                );
              in
              [ "cache-paths" cached-outputs.drvPath ];
            buildPhase =
              "grep '^/nix/store/' < cache-paths | sort | uniq > $out";
            dontUnpack = true;
            dontPatch = true;
            dontConfigure = true;
            dontInstall = true;
            dontFixup = true;
            allowSubstitutes = false;
          };
        };
      });
}
