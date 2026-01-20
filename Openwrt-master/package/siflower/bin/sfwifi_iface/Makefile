#
# Copyright (C) 2015-2018 ming.guang@siflower.com.cn
#
# sifiower Software.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=sfwifi_iface
PKG_VERSION:=1
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define Package/sfwifi_iface
SECTION:=utils
CATEGORY:=Utilities
TITLE:=sfwifi_iface
MAINTAINER:=ming.guang <ming.guang@siflower.com.cn>
DEPENDS:=+libc +libpthread +librt +libubus
endef

define Package/sfwifi_iface/description
interface for wifi driver.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
endef

define Build/Compile
    $(MAKE) -C $(PKG_BUILD_DIR) \
        CC="$(TARGET_CC)" \
        CFLAGS="$(TARGET_CFLAGS) -Wall " \
        LDFLAGS="$(TARGET_LDFLAGS) -lpthread -lc -lm"
endef

define Package/sfwifi_iface/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/sfwifi_iface
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/sfwifi_iface $(1)/usr/bin
endef

$(eval $(call BuildPackage,sfwifi_iface))
