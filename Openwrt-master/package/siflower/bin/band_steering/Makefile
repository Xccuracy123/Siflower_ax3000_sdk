include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=band_steering
PKG_VERSION:=1
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

define Package/band_steering
	SECTION:=utils
	CATEGORY:=Utilities
	TITLE:=band_steering
	MAINTAINER:=ming.guang <ming.guang@siflower.com.cn>
	DEPENDS:=+hostapd-common +libblobmsg-json +libubus +libjson-c
endef

define Package/band_steering/description
1. Record the sta sending probei/assoc req to us, and control the priority
of 5g connection according to whether to reply to assoc response.
2. Determine whether to switch STA to another band based on the signal strength.
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
		CFLAGS="$(TARGET_CFLAGS) -w" \
		LDFLAGS="$(TARGET_LDFLAGS)"
endef

define Package/band_steering/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_DIR) $(1)/lib/firmware
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/band_steering $(1)/usr/bin
	$(INSTALL_BIN) ./files/band_steering $(1)/etc/init.d
endef

$(eval $(call BuildPackage,band_steering))

