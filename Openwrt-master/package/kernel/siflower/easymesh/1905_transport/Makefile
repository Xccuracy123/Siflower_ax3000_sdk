include $(TOPDIR)/rules.mk

PKG_NAME:=1905_transport
PKG_RELEASE:=1
PKG_MAINTAINER:= Ming Guang <ming.guang@siflower.com.cn>

include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/package.mk

INC_DIR:=$(shell pwd)/../include
NOSTDINC_FLAGS += \
	$(KERNEL_NOSTDINC_FLAGS) \
	-I$(INC_DIR) \
	-Werror

define KernelPackage/1905_transport
	SUBMENU:=Kernel modules
	TITLE:= 1905 transport module of EasyMesh.
	FILES:=$(PKG_BUILD_DIR)/1905_transport.ko
endef

define KernelPackage/1905_transport/description
	1905 transport module of EasyMesh.
endef

KO_FILE_NAME="demo_ko"

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./$(KO_FILE_NAME)/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
endef

$(eval $(call KernelPackage,1905_transport))
