{
  description = "xv6 for Milk-V Duo";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    duo-buildroot-sdk.url = "github:milkv-duo/duo-buildroot-sdk";
    duo-buildroot-sdk.flake = false;
  };

  outputs = { self, nixpkgs, duo-buildroot-sdk }:
    let
      supportedSystems = [ "x86_64-linux" "aarch64-darwin" ];
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;
      nixpkgsFor = forAllSystems (system: import nixpkgs { inherit system; });
      
      # Define supported chips and their configurations
      supportedChips = {
        cv1800b = {
          board = "milkv_duo_sd";
          ddr_param = "cv181x/ddr_param.bin";
        };
        sg2002 = {
          board = "milkv_duo_256m";
          ddr_param = "sg2002/ddr_param.bin";
        };
      };

      # Define a local overlay for duo-nix packages
      mkDuoOverlay = chip: self: super: {
        inherit chip;
        board = supportedChips.${chip}.board;
        tools = self.callPackage ./duo-nix/tools.nix {};
        chip_conf = self.callPackage ./duo-nix/chipconf.nix {};
        fip-simple = self.callPackage ./duo-nix/fip-simple.nix {};
        memmap = self.callPackage ./duo-nix/memmap.nix {};
        fsbl = self.callPackage ./duo-nix/fsbl.nix {};
        opensbi = self.callPackage ./duo-nix/opensbi.nix {};
        uboot = self.callPackage ./duo-nix/uboot.nix {};
        fipinfo = self.callPackage ./duo-nix/fipinfo.nix {};
        freertos = self.callPackage ./duo-nix/freertos.nix {};
        fip = self.callPackage ./duo-nix/fip.nix {};
        sg_boot = self.callPackage ./duo-nix/sg_boot.nix {};
        duo-buildroot-sdk = duo-buildroot-sdk;
      };
    in {
      packages = forAllSystems (system:
        let
          pkgs = nixpkgsFor.${system};
          duoPkgs = pkgs.extend (mkDuoOverlay "cv1800b");
          # Create packages for each supported chip
          mkChipPackages = chip:
            let
              duoPkgs = (pkgs.extend (mkDuoOverlay chip));
              
              # xv6 build
              xv6 = pkgs.pkgsCross.riscv64-embedded.stdenv.mkDerivation {
                name = "xv6-milkv-duo-${chip}";
                src = ./.;
                nativeBuildInputs = [
                  pkgs.pkgsCross.riscv64-embedded.stdenv.cc
                  pkgs.python3
                ];
                buildInputs = [
                  duoPkgs.memmap
                ];
                CFLAGS = "-march=rv64imafdcv_xtheadsync_xtheadcmo_zicsr_zifencei ${if chip == "sg2002" then "-DCHIP_SG2002" else ""}";
                ASFLAGS = "-march=rv64imafdcv_xtheadsync_xtheadcmo_zicsr_zifencei ${if chip == "sg2002" then "-DCHIP_SG2002" else ""}";
                makeFlags = [
                  "TOOLPREFIX=riscv64-none-elf-"
                ];
                installPhase = ''
                  mkdir -p $out
                  cp kernel/kernel $out/
                '';
              };

              # Final bootable image
              image = pkgs.runCommand "xv6-milkv-duo-${chip}-image" {
                nativeBuildInputs = [ duoPkgs.tools ];
              } ''
                mkdir $out
                NOR_INFO=$(printf '%72s' | tr ' ' 'FF')
                NAND_INFO=00000000

                touch empty.bin

                fiptool.py -v genfip $out/fip.bin \
                  --DDR_PARAM=${duoPkgs.duo-buildroot-sdk}/fsbl/test/${supportedChips.${chip}.ddr_param} \
                  --MONITOR_RUNADDR=0x80000000 --MONITOR=${duoPkgs.opensbi}/fw_dynamic.bin \
                  --BLCP_2ND_RUNADDR=0x83f40000 --BLCP_2ND=${./duo-nix/buildroot-unpacked/blcp_2nd.bin} \
                  --CHIP_CONF=${duoPkgs.chip_conf} --NOR_INFO=$NOR_INFO --NAND_INFO=$NAND_INFO \
                  --LOADER_2ND=${duoPkgs.uboot}/u-boot.bin --compress=lzma \
                  --BLCP=empty.bin --BLCP_IMG_RUNADDR=0x05200200 \
                  --BL2=${duoPkgs.fsbl}/bl2.bin
              '';
            in {
              inherit xv6 image;
              default = image;
            };
        in
          {
            cv1800b = mkChipPackages "cv1800b";
            sg2002 = mkChipPackages "sg2002";
            default = mkChipPackages "cv1800b";  # Default to CV1800B
          }
      );

      devShells = forAllSystems (system:
        let
          pkgs = nixpkgsFor.${system};
        in {
          default = pkgs.mkShell {
            buildInputs = with pkgs; [
              minicom
              screen
              # Build sg_boot from source
              (rustPlatform.buildRustPackage {
                pname = "sg_boot";
                version = "0.1.0";
                src = fetchFromGitHub {
                  owner = "platform-system-interface";
                  repo = "sg_boot";
                  rev = "988fbd0e95b647d0ef5240756ecb92898e8c8e40";
                  hash = "sha256-Tn1yHkKz4oBKbMd7z8xOfdzQtORqKocXruTGbRjzNbs=";
                };
                cargoHash = "sha256-mUAko3/T+2NDSdhJ97xKlVdBXX5D7BiIrZ9DRHArNHM=";
                nativeBuildInputs = [
                  pkg-config
                  rustPlatform.bindgenHook
                ];
                buildInputs = [
                  libudev-zero
                ];
              })
              # Python package
              (python3Packages.buildPythonPackage rec {
                pname = "yoctools";
                version = "2.1.11";
                src = python3Packages.fetchPypi {
                  pname = pname;
                  version = version;
                  sha256 = "sha256-2VtdH4WxObqsc3Vbu/NmtxDie6AO15fwDuic4gw8WHg=";
                };
                propagatedBuildInputs = with python3Packages; [ 
                  pyserial 
                  click 
                  scons
                  ruamel-yaml
                  threadpool
                  setuptools
                  wheel
                  pip
                  GitPython
                  gitdb
                  smmap
                  xlsxwriter
                  requests-toolbelt
                  configparser
                ];
                meta = with lib; {
                  description = "YoC tools";
                  homepage = "https://pypi.org/project/yoctools/";
                  license = licenses.bsd3;
                };
              })
            ];
          };
        }
      );
    };
} 
