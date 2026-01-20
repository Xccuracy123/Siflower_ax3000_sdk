include $(TOPDIR)/rules.mk

PKG_NAME:=agent
PKG_RELEASE:=1
PKG_MAINTAINER:= Ming Guang <ming.guang@siflower.com.cn>

include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/package.mk

INC_DIR:=$(shell pwd)/../include
NOSTDINC_FLAGS += \
	$(KERNEL_NOSTDINC_FLAGS) \
	-I$(INC_DIR) \
	-Werror

define KernelPackage/agent
	SUBMENU:=Kernel modules
	TITLE:= Agent module of EasyMesh.
	FILES:=$(PKG_BUILD_DIR)/agent.ko
	DEPENDS:=+kmod-1905_transport +kmod-sf_wifi
endef

define KernelPackage/agent/description
	Agent module of EasyMesh.
endef

KO_FILE_NAME="demo_ko"

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./$(KO_FILE_NAME)/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
endef

$(eval $(call KernelPackage,agent))
