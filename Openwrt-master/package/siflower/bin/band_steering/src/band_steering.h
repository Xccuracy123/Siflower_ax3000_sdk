/*
 * =====================================================================================
 *
 *       Filename:  band_steering.c
 *
 *    Description:  This is for the advance feature for wireless devices:
 *                  1. Record the sta sending probe/assoc req to us, and control the priority of 5g
 *                  connection according to whether to reply to assoc response.
 *                  2. Determine whether to switch STA to another band based on the signal strength.
 *                  3. This service will be acted as a UBUS server and expose interfaces to all client.
 *
 *        Version:  1.0
 *        Created:  2023年06月08日 09时38分56秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ming.guang , ming.guang@siflower.com.cn
 *        Company:  Siflower
 *
 * =====================================================================================
 */

#ifndef __BAND_STEERING_H
#define __BAND_STEERING_H

#include <time.h>
#include <syslog.h>
#include "libubus.h"

#define ROAM_AGING_TIME 30
#define PROBE_AGING_TIME 10
#define ASSOC_AGING_TIME 2
#define ROAM_CHECK_TIME 1000
#define HIGH_BAND_ROAM_RSSI -78
#define LOW_BAND_ROAM_RSSI -50
#define ETH_ALEN 6
#define STA_HASH_SIZE 256
#define STA_HASH(sta) (sta[3])

#define LOG(X,...) syslog(LOG_CRIT, X, ##__VA_ARGS__)
#define MACSTR "%02X:%02X:%02X:%02X:%02X:%02X"
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]

enum {
    WIFI_AP_BAND_24G,
    WIFI_AP_BAND_5G,
    WIFI_BAND_TYPE_MAX
};

enum {
    WIFI_AP_STA_ROAM,
    WIFI_AP_REC_PROBE,
    WIFI_AP_REC_ASSOC,
    WIFI_TABLE_TYPE_MAX
};

enum {
    PROBE_REQ_STA_MAC,
    PROBE_REQ_CHANNEL,
    PROBE_REQ_RSSI,
    PROBE_REQ_OBJ_ID,
    __PROBE_REQ_MAX
};

enum {
    ASSOC_REQ_STA_MAC,
    ASSOC_REQ_CHANNEL,
    ASSOC_REQ_RSSI,
    __ASSOC_REQ_MAX
};

struct bss_info {
    char sta_mac[32];
    uint32_t channel;
    uint32_t op_class;
};

struct sta_info {
    uint8_t sta_mac[ETH_ALEN];
    int rx_rssi;
    long rx_sec;
    struct sta_info *next;
};

struct sta_list {
    uint64_t sta_hash[STA_HASH_SIZE];
};

struct lock_list {
    pthread_rwlock_t sta_lock[WIFI_BAND_TYPE_MAX];
};

int band_steering_server_init(struct ubus_context *ctx);
void roam_timer_init(void);
void roam_timer_deinit(void);
void siwifi_sta_list_init();
void siwifi_sta_list_deinit();

#endif /* _BAND_STEERING_H */
