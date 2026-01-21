define Device/pw231_demo
	BLOCKSIZE := 64k
	DEVICE_VENDOR := SIFLOWER
	DEVICE_MODEL := PW231_DEMO
	DEVICE_DTS := sf21a6826-fullmask-pw231_demo
	SUPPORTED_DEVICES := siflower,sf21a6826-pw231_demo
endef
TARGET_DEVICES += pw231_demo

define Device/6826sg
	BLOCKSIZE := 64k
	DEVICE_VENDOR := SIFLOWER
	DEVICE_MODEL := 6826SG
	DEVICE_DTS := sf21a6826-fullmask-6826sg
	SUPPORTED_DEVICES := siflower,sf21a6826-6826sg
endef
TARGET_DEVICES += 6826sg

define Device/6826sgsrb
	BLOCKSIZE := 64k
	DEVICE_VENDOR := SIFLOWER
	DEVICE_MODEL := 6826SGSRB
	DEVICE_DTS := sf21a6826-fullmask-6826sgsrb
	SUPPORTED_DEVICES := siflower,sf21a6826-6826sgsrb
endef
TARGET_DEVICES += 6826sgsrb

define Device/6826sgsrb-nand
	$(call Device/NAND_TAR)
	DEVICE_VENDOR := SIFLOWER
	DEVICE_MODEL := 6826SGSRB (Booting from NAND)
	DEVICE_DTS := sf21a6826-fullmask-6826sgsrb-nand
	SUPPORTED_DEVICES := siflower,sf21a6826-6826sgsrb-nand
endef
TARGET_DEVICES += 6826sgsrb-nand

define Device/6826sg_v4
	BLOCKSIZE := 64k
	DEVICE_VENDOR := SIFLOWER
	DEVICE_MODEL := 6826SG
	DEVICE_DTS := sf21a6826-fullmask-6826sg_v4
	SUPPORTED_DEVICES := siflower,sf21a6826-6826sg_v4
endef
TARGET_DEVICES += 6826sg_v4

define Device/6826sg2_v4
	BLOCKSIZE := 64k
	DEVICE_VENDOR := SIFLOWER
	DEVICE_MODEL := 6826SG
	DEVICE_DTS := sf21a6826-fullmask-6826sg2_v4
	SUPPORTED_DEVICES := siflower,sf21a6826-6826sg2_v4
endef
TARGET_DEVICES += 6826sg2_v4

define Device/6826sg_v4-nand
	$(call Device/NAND_TAR)
	DEVICE_VENDOR := SIFLOWER
	DEVICE_MODEL := 6826SG (Booting from NAND)
	DEVICE_DTS := sf21a6826-fullmask-6826sg_v4-nand
	SUPPORTED_DEVICES := siflower,sf21a6826-6826sg_v4-nand
endef
TARGET_DEVICES += 6826sg_v4-nand

define Device/6826sg2_v4-nand
	$(call Device/NAND_TAR)
	DEVICE_VENDOR := SIFLOWER
	DEVICE_MODEL := 6826SG (Booting from NAND)
	DEVICE_DTS := sf21a6826-fullmask-6826sg2_v4-nand
	SUPPORTED_DEVICES := siflower,sf21a6826-6826sg2_v4-nand
endef
TARGET_DEVICES += 6826sg2_v4-nand

define Device/6826sgsrb_v4
	BLOCKSIZE := 64k
	DEVICE_VENDOR := SIFLOWER
	DEVICE_MODEL := 6826SGSRB
	DEVICE_DTS := sf21a6826-fullmask-6826sgsrb_v4
	SUPPORTED_DEVICES := siflower,sf21a6826-6826sgsrb_v4
endef
TARGET_DEVICES += 6826sgsrb_v4

define Device/6826sgsrb_v4-nand
	$(call Device/NAND_TAR)
	DEVICE_VENDOR := SIFLOWER
	DEVICE_MODEL := 6826SGSRB (Booting from NAND)
	DEVICE_DTS := sf21a6826-fullmask-6826sgsrb_v4-nand
	SUPPORTED_DEVICES := siflower,sf21a6826-6826sgsrb_v4-nand
endef
TARGET_DEVICES += 6826sgsrb_v4-nand

define Device/pw231_demo-easymesh
	BLOCKSIZE := 64k
	DEVICE_VENDOR := SIFLOWER
	DEVICE_MODEL := PW231_DEMO for easymesh
	DEVICE_DTS := sf21a6826-fullmask-pw231_demo-easymesh
	SUPPORTED_DEVICES := siflower,sf21a6826-pw231_demo-easymesh
endef
TARGET_DEVICES += pw231_demo-easymesh

define Device/pw231_demo-wfa
	BLOCKSIZE := 64k
	DEVICE_VENDOR := SIFLOWER
	DEVICE_MODEL := PW231_DEMO for wfa
	DEVICE_DTS := sf21a6826-fullmask-pw231_demo
	SUPPORTED_DEVICES := siflower,sf21a6826-pw231_demo-wfa
endef
TARGET_DEVICES += pw231_demo-wfa

define Device/pw231_demo_switch
	BLOCKSIZE := 64k
	DEVICE_VENDOR := SIFLOWER
	DEVICE_MODEL := PW231_DEMO_SWITCH
	DEVICE_DTS := sf21a6826-fullmask-pw231_demo_switch
	SUPPORTED_DEVICES := siflower,sf21a6826-pw231_demo_switch
endef
TARGET_DEVICES += pw231_demo_switch

define Device/6826sg_ap
	$(call Device/NOR_FIT)
	DEVICE_VENDOR := SIFLOWER
	DEVICE_MODEL := 6826SG_AP
	DEVICE_DTS := sf21a6826-fullmask-6826sg_ap
	SUPPORTED_DEVICES := siflower,sf21a6826-6826sg_ap
endef
TARGET_DEVICES += 6826sg_ap

define Device/6826sg2_v4_zhaoyue
	BLOCKSIZE := 64k
	DEVICE_VENDOR := SIFLOWER
	DEVICE_MODEL := 6826SG2_V4_ZHAOYUE
	DEVICE_DTS := sf21a6826-fullmask-6826sg2_v4_zhaoyue
	SUPPORTED_DEVICES := siflower,sf21a6826_6826sg2_v4_zhaoyue
endef
TARGET_DEVICES += 6826sg2_v4_zhaoyue

define Device/6826sg2_v4_easymesh
	BLOCKSIZE := 64k
	DEVICE_VENDOR := SIFLOWER
	DEVICE_MODEL := 6826SG2_V4_EASYMESH
	DEVICE_DTS := sf21a6826-fullmask-6826sg2_v4_easymesh
	SUPPORTED_DEVICES := siflower,sf21a6826_6826sg2_v4_easymesh
endef
TARGET_DEVICES += 6826sg2_v4_easymesh
