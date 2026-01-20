ARCH:=riscv64
SUBTARGET:=sf21a6826
BOARDNAME:=Siflower SF21A6826 based boards
CPU_TYPE:=c908
KERNELNAME:=Image
DEFAULT_PACKAGES += opensbi_siflower

define Target/Description
  Build firmware images for Siflower SF21A6826 SoCs
endef
