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
    in {
      packages = forAllSystems (system:
        let
          pkgs = nixpkgsFor.${system};
          # Define a local overlay for duo-nix packages
          duoOverlay = self: super: {
            chip = "cv1800b";
            board = "milkv_duo_sd";
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
            duo-buildroot-sdk = duo-buildroot-sdk;
          };
          duoPkgs = (pkgs.extend duoOverlay);

          # xv6 build
          xv6 = pkgs.pkgsCross.riscv64-embedded.stdenv.mkDerivation {
            name = "xv6-milkv-duo";
            src = ./.;
            nativeBuildInputs = [
              pkgs.pkgsCross.riscv64-embedded.stdenv.cc
              pkgs.python3
            ];
            buildInputs = [
              duoPkgs.memmap
            ];
            CFLAGS = "-march=rv64imafdcv_xtheadsync_xtheadcmo_zicsr_zifencei";
            ASFLAGS = "-march=rv64imafdcv_xtheadsync_xtheadcmo_zicsr_zifencei";
            makeFlags = [
              "TOOLPREFIX=riscv64-none-elf-"
            ];
            installPhase = ''
              mkdir -p $out
              cp kernel/kernel $out/
            '';
          };

          # Final bootable image
          image = pkgs.runCommand "xv6-milkv-duo-image" {
            nativeBuildInputs = [ duoPkgs.tools ];
          } ''
            mkdir $out
            NOR_INFO=$(printf '%72s' | tr ' ' 'FF')
            NAND_INFO=00000000

            touch empty.bin

            fiptool.py -v genfip $out/fip.bin \
              --DDR_PARAM=${duoPkgs.duo-buildroot-sdk}/fsbl/test/cv181x/ddr_param.bin \
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
        }
      );
    };
} 
