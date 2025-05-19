{
  inputs = {
    duo-buildroot-sdk.url = "github:milkv-duo/duo-buildroot-sdk";
    duo-buildroot-sdk.flake = false;
  };
  outputs = { self, nixpkgs, duo-buildroot-sdk }:
  let
    supportedSystems = [ "x86_64-linux" "aarch64-darwin" ];
    forAllSystems = nixpkgs.lib.genAttrs supportedSystems;
    nixpkgsFor = forAllSystems (system: import nixpkgs { inherit system; });
  in
  {
    packages = forAllSystems (system:
      let
        pkgs = nixpkgsFor.${system};
        ov = _self: super: {
          inherit self duo-buildroot-sdk;
        };
        customPkgs = (pkgs.extend ov).extend (import ./overlay.nix);
      in {
        inherit (customPkgs) tools chip_conf fip-simple memmap fsbl opensbi uboot fipinfo freertos fip;
        default = customPkgs.fip;
      }
    );
 
    defaultPackage = forAllSystems (system: self.packages.${system}.default);
  };
}