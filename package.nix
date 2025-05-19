{ runCommand, xv6, opensbi, uboot, tools, memmap, duo-buildroot-sdk }:

runCommand "xv6-milkv-duo-image" {
  nativeBuildInputs = [ tools ];
} ''
  # Create a temporary directory for building the image
  mkdir -p $out
  cd $out

  # Copy the xv6 kernel
  cp ${xv6}/kernel kernel.img

  # Create the FIP (Firmware Image Package)
  mkdir -p fip
  cp ${opensbi}/fw_dynamic.bin fip/
  cp ${uboot}/u-boot.bin fip/
  cp kernel.img fip/

  # Generate the FIP image
  fiptool.py create --align 4096 \
    --opensbi fip/fw_dynamic.bin \
    --uboot fip/u-boot.bin \
    --kernel fip/kernel.img \
    fip.bin

  # Create the final boot image
  mkdir -p boot
  cp ${duo-buildroot-sdk}/fsbl/build/bl2.bin boot/
  cp fip.bin boot/
  
  # Create the final image
  mkcvipart.py ${duo-buildroot-sdk}/build/boards/cv180x/cv1800b_milkv_duo/partition/partition_sd.xml boot/
  mk_imgHeader.py ${duo-buildroot-sdk}/build/boards/cv180x/cv1800b_milkv_duo/partition/partition_sd.xml boot/

  # Create the final bootable image
  cat boot/bl2.bin boot/fip.bin > $out/xv6-milkv-duo.img
'' 
