{
  description = "Self-contained xv6 RISC-V and Milk-V Duo development shell";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }:
    let
      systems = [ "x86_64-linux" "aarch64-linux" ];
      forAllSystems = nixpkgs.lib.genAttrs systems;
    in {
      devShells = forAllSystems (system:
        let
          pkgs = import nixpkgs { inherit system; };
          cross = pkgs.pkgsCross.riscv64-embedded;
        in {
          default = pkgs.mkShell {
            packages = [
              cross.buildPackages.gcc
              cross.buildPackages.binutils
              pkgs.gnumake
              pkgs.gnutar
              pkgs.gnused
              pkgs.lrzsz
              pkgs.perl
              pkgs.qemu
              pkgs.tio
              pkgs.picocom
            ];

            shellHook = ''
              echo "xv6 Milk-V Duo development shell"
            '';
          };
        });
    };
}
