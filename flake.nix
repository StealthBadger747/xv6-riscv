{
  description = "xv6 for Milk-V Duo";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    duo-nix = {
      url = "path:/home/erikp/Documents/fun-coding-projects/duo-nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, duo-nix }:
    let
      supportedSystems = [ "x86_64-linux" "aarch64-darwin" ];
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;
      nixpkgsFor = forAllSystems (system: import nixpkgs { inherit system; });
    in {
      packages = forAllSystems (system:
        let
          pkgs = nixpkgsFor.${system};
          duoPkgs = duo-nix.packages.${system};
          pkgsCross = pkgs.pkgsCross.riscv64;

          # xv6 build
          xv6 = pkgsCross.stdenv.mkDerivation {
            name = "xv6-milkv-duo";
            src = ./.;
            nativeBuildInputs = [
              pkgsCross.stdenv.cc
              pkgs.python3
            ];
            buildInputs = [
              duoPkgs.memmap
            ];
            CFLAGS = "-march=rv64imafdcv_xtheadsync_xtheadcmo_zicsr_zifencei";
            ASFLAGS = "-march=rv64imafdcv_xtheadsync_xtheadcmo_zicsr_zifencei";
            makeFlags = [
              "TOOLPREFIX=riscv64-unknown-linux-gnu-"
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
            # Create a temporary directory for building the image
            mkdir -p $out
            cd $out

            # Copy the xv6 kernel
            cp ${xv6}/kernel kernel.img

            # Create the FIP (Firmware Image Package)
            mkdir -p fip
            cp ${duoPkgs.opensbi}/fw_dynamic.bin fip/
            cp ${duoPkgs.uboot}/u-boot.bin fip/
            cp kernel.img fip/

            # Generate the FIP image
            fiptool.py create --align 4096 \
              --opensbi fip/fw_dynamic.bin \
              --uboot fip/u-boot.bin \
              --kernel fip/kernel.img \
              fip.bin

            # Create the final boot image
            mkdir -p boot
            cp ${duoPkgs.fsbl}/bl2.bin boot/
            cp fip.bin boot/
            
            # Create the final image
            mkcvipart.py ${duoPkgs.memmap}/include/partition.xml boot/
            mk_imgHeader.py ${duoPkgs.memmap}/include/partition.xml boot/

            # Create the final bootable image
            cat boot/bl2.bin boot/fip.bin > $out/xv6-milkv-duo.img
          '';
        in {
          inherit xv6 image;
          default = image;
        }
      );
    };
} 
