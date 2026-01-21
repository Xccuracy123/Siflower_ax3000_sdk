include $(TOPDIR)/rules.mk

PKG_NAME:=controller
PKG_RELEASE:=1
PKG_MAINTAINER:= Ming Guang <ming.guang@siflower.com.cn>

include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/package.mk

INC_DIR:=$(shell pwd)/../include
NOSTDINC_FLAGS += \
	$(KERNEL_NOSTDINC_FLAGS) \
	-I$(INC_DIR) \
	-Werror

define KernelPackage/controller
	SUBMENU:=Kernel modules
	TITLE:= Controller module of EasyMesh.
	FILES:=$(PKG_BUILD_DIR)/controller.ko
	DEPENDS:=+kmod-1905_transport
endef

define KernelPackage/controller/description
	Controller module of EasyMesh.
endef

KO_FILE_NAME="demo_ko"

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./$(KO_FILE_NAME)/* $(PKG_BUILD_DIR)/
	$(CP) -r ./config $(PKG_BUILD_DIR)/
endef

define Build/Compile
endef

define KernelPackage/controller/install
	$(INSTALL_DIR) $(1)/opt/easymesh
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_DIR) $(1)/etc/config
	cp $(PKG_BUILD_DIR)/config/easymesh $(1)/etc/config/easymesh
	cp $(PKG_BUILD_DIR)/config/sf_easymesh $(1)/bin/sf_easymesh
	cp $(PKG_BUILD_DIR)/config/easymesh $(1)/opt/easymesh/easymesh
	cp $(PKG_BUILD_DIR)/config/wireless $(1)/opt/easymesh/wireless
	cp $(PKG_BUILD_DIR)/config/sf_easymesh $(1)/opt/easymesh/sf_easymesh
	cp $(PKG_BUILD_DIR)/config/device_list $(1)/opt/easymesh/device_list
endef

$(eval $(call KernelPackage,controller))
