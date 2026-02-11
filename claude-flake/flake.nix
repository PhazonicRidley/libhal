{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixpkgs-unstable";
    nixpkgs-libxml2.url = "github:nixos/nixpkgs/c4d0026e7346ad2006c2ba730d5a712c18195aab"; # required for older libxml2 used for clang

    flake-parts.url = "github:hercules-ci/flake-parts";
  };

  outputs =
    { flake-parts, ... }@inputs:
    let
      libhalModule =
        { lib, config, ... }:
        let
          inherit (lib)
            mkOption
            mkEnableOption
            types
            mkIf
            mkMerge
            ;

          topConfig = config; # Capture top-level config before perSystem shadows it
        in
        {
          options.libhal = {
            enable = mkEnableOption "libhal development environment";

            type = mkOption {
              type = types.enum [
                "fhs"
                "llvm"
                "gcc"
              ];
              default = "fhs";
              description = "Type of devshell to use as default";
            };

            exposeAllVariants = mkEnableOption "expose all shell variants (fhs, llvm, gcc)";

            extraPackages = mkOption {
              type = types.functionTo (types.listOf types.package);
              default = pkgs: [ ];
              description = "Extra packages to include (function from pkgs to list)";
            };

            shellHook = mkOption {
              type = types.lines;
              default = "";
              description = "Extra shell hook commands";
            };
          };

          config = {
            perSystem =
              {
                pkgs,
                system,
                ...
              }:
              let
                cfg = topConfig.libhal;
                pkgs-libxml2 = inputs.nixpkgs-libxml2.legacyPackages.${system};

                buildBaseLibs = [
                  pkgs.zlib
                  pkgs.zstd
                  pkgs-libxml2.libxml2
                ];

                fhsBuildLibs = with pkgs; [
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
                    llvmPackages_21.clang-tools
                    stm32loader
                  ]
                  ++ buildBaseLibs
                  ++ (cfg.extraPackages pkgs);

                baseShellHook = ''
                  conan config install https://github.com/libhal/conan-config2.git
                  conan hal setup
                  ${cfg.shellHook}
                '';

                mkFhsShell =
                  (pkgs.buildFHSEnv {
                    name = "libhal-fhs";
                    targetPkgs = pkgs: devshellBasePkgs ++ fhsBuildLibs;
                    runScript = "bash";
                    profile = baseShellHook;
                  }).env;

                mkLlvmShell = pkgs.mkShell.override { stdenv = pkgs.llvmPackages_20.stdenv; } {
                  name = "libhal-native-llvm";
                  buildInputs =
                    devshellBasePkgs
                    ++ (with pkgs; [
                      llvmPackages_20.libcxx
                      llvmPackages_20.libunwind
                    ]);
                  shellHook = baseShellHook;
                };

                mkGccShell = pkgs.mkShell {
                  name = "libhal-native-gcc";
                  buildInputs = devshellBasePkgs;
                  shellHook = baseShellHook;
                };

                shellByType = {
                  fhs = mkFhsShell;
                  llvm = mkLlvmShell;
                  gcc = mkGccShell;
                };
              in
              mkIf cfg.enable {
                devShells = mkMerge [
                  { default = shellByType.${cfg.type}; }
                  (mkIf cfg.exposeAllVariants {
                    libhal-fhs = mkFhsShell;
                    libhal-llvm = mkLlvmShell;
                    libhal-gcc = mkGccShell;
                  })
                ];
              };
          };
        };
    in
    flake-parts.lib.mkFlake { inherit inputs; } {
      imports = [
        inputs.flake-parts.flakeModules.flakeModules
        libhalModule
      ];

      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "aarch64-darwin"
        "x86_64-darwin"
      ];

      flake.flakeModules.default = libhalModule;

      libhal = {
        enable = true;
        exposeAllVariants = true;
      };
    };
}
