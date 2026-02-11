{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixpkgs-unstable";
    nixpkgs-libxml2.url = "github:nixos/nixpkgs/c4d0026e7346ad2006c2ba730d5a712c18195aab"; # required for older libxml2 used for clang

    flake-parts.url = "github:hercules-ci/flake-parts";
  };

  outputs =
    { flake-parts, ... }@inputs:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "aarch64-darwin"
        "x86_64-darwin"
      ];
      perSystem =
        { pkgs, system, ... }:
        let
          pkgs-libxml2 = inputs.nixpkgs-libxml2.legacyPackages.${system};
          buildBaseLibs = [
            # Base requirements needed to build libhal with a given standard lib
            pkgs.zlib
            pkgs.zstd
            pkgs-libxml2.libxml2
          ];

          fhsBuildLibs = with pkgs; [
            # To be used with libhal's llvm-toolchain conan package, requirements for including standard libraries
            gcc.cc.lib
            gcc-unwrapped
            libgcc
            glibc
            glibc.dev
          ];

          devshellBasePkgs =
            with pkgs;
            [
              conan
              python313
              cmake
              ninja
              llvmPackages_20.clang-tools
              stm32loader

            ]
            ++ buildBaseLibs;
        in
        {
          devShells = {
            default =
              (pkgs.buildFHSEnv {
                name = "libhal-fhs";
                targetPkgs = pkgs: devshellBasePkgs ++ fhsBuildLibs;

                runScript = "bash";
                profile = ''
                  conan config install https://github.com/libhal/conan-config2.git
                  conan hal setup
                '';
              }).env;

            "llvm" = pkgs.mkShell.override { stdenv = pkgs.llvmPackages_20.stdenv; } {
              name = "libhal-native-llvm";
              buildInputs =
                devshellBasePkgs
                ++ (with pkgs; [
                  llvmPackages_20.libcxx
                  llvmPackages_20.libunwind
                ]);
              shellHook = ''
                conan config install https://github.com/libhal/conan-config2.git
                conan hal setup
              '';
            };

            "gcc" = pkgs.mkShell {
              name = "libhal-native-gcc";
              buildInputs = devshellBasePkgs;
              shellHook = ''
                conan config install https://github.com/libhal/conan-config2.git
                conan hal setup
              '';
            };

          };

        };
    };
}
