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

#include "band_steering.h"
#include "libubus.h"
#include <libubox/blobmsg_json.h>
#include <json-c/json_object.h>
#include <unistd.h>
#include <math.h>
#include <pthread.h>

extern struct ubus_context *ubus_ctx;
static struct blob_buf b;
uint32_t high_band_obj_id = 0;
uint32_t low_band_obj_id = 0;
/* sta table */
struct sta_list probe_list[WIFI_BAND_TYPE_MAX];
struct sta_list assoc_list[WIFI_BAND_TYPE_MAX];
uint64_t roam_sta_hash[STA_HASH_SIZE];
/* pthread rwlock list */
struct lock_list lock_list[WIFI_TABLE_TYPE_MAX];

static int wifi_recevie_probe_req(struct ubus_context *uctx, struct ubus_object *obj,
        struct ubus_request_data *req, const char *method,
        struct blob_attr *msg);

static int wifi_recevie_assoc(struct ubus_context *uctx, struct ubus_object *obj,
        struct ubus_request_data *req, const char *method,
        struct blob_attr *msg);


static const struct blobmsg_policy wifi_recevie_probe_req_policy[] = {
    [PROBE_REQ_STA_MAC] = { .name = "sta_mac", .type = BLOBMSG_TYPE_STRING },
    [PROBE_REQ_CHANNEL] = { .name = "channel", .type = BLOBMSG_TYPE_INT32 },
    [PROBE_REQ_RSSI] = { .name = "rssi", .type = BLOBMSG_TYPE_INT32 },
    [PROBE_REQ_OBJ_ID] = { .name = "obj_id", .type = BLOBMSG_TYPE_INT32 },
};

static const struct blobmsg_policy wifi_recevie_assoc_policy[] = {
    [ASSOC_REQ_STA_MAC] = { .name = "sta_mac", .type = BLOBMSG_TYPE_STRING },
    [ASSOC_REQ_CHANNEL] = { .name = "channel", .type = BLOBMSG_TYPE_INT32 },
    [ASSOC_REQ_RSSI] = { .name = "rssi", .type = BLOBMSG_TYPE_INT32 },
};

static const struct ubus_method band_steering_methods[] = {
    UBUS_METHOD("wifi_recevie_probe_req", wifi_recevie_probe_req, wifi_recevie_probe_req_policy),
    UBUS_METHOD("wifi_recevie_assoc", wifi_recevie_assoc, wifi_recevie_assoc_policy),
};

static struct ubus_object_type band_steering_type =
    UBUS_OBJECT_TYPE("sf_band_steering", band_steering_methods);

static struct ubus_object band_steering = {
    .name = "sf_band_steering",
    .type = &band_steering_type,
    .methods = band_steering_methods,
    .n_methods = ARRAY_SIZE(band_steering_methods),
};

static unsigned char char_to_value(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';

    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;

    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return '0';
}

static unsigned char twochar_to_byte(char h, char l)
{
    return (unsigned char)(char_to_value(h) * 16 + char_to_value(l));
}

static uint8_t *str_to_str16(uint8_t *data, char *str)
{
    int cnt = 0;
    while (cnt < 6) {
        data[cnt] = twochar_to_byte(str[3 * cnt], str[3 * cnt + 1]);
        cnt++;
    }

    return data;
}

static void dec_to_hexstr(uint8_t data, char *str, int len)
{
    int i;

    str[len] = 0;
    for (i = len - 1; i >= 0; i--, data >>= 4)
    {
        if ((data & 0xf) <= 9)
            str[i] = (data & 0xf) + '0';
        else
            str[i] = (data & 0xf) + 'A' - 0x0a;
    }
}

/**
 * Get current sec.
 */
static long siwifi_get_time()
{
    struct timeval t;

    /* Get current time */
    gettimeofday(&t, NULL);

    return t.tv_sec;
}

/**
 * Init the hash table.
 */
void siwifi_sta_list_init()
{
    struct sta_info *sta = NULL, *sta_tmp = NULL;
    int i, j;

    /* Traverse the hash table and initialize to Empty */
    /* Init roam sta table */
    for (i = 0; i < STA_HASH_SIZE; i++)
        roam_sta_hash[i] = NULL;
    pthread_rwlock_init(&(lock_list[WIFI_AP_STA_ROAM].sta_lock[0]), NULL);

    /* Init probe sta table */
    for (i = 0; i < STA_HASH_SIZE; i++)
        for (j = 0; j < WIFI_BAND_TYPE_MAX; j++)
            probe_list[j].sta_hash[i] = NULL;
    pthread_rwlock_init(&(lock_list[WIFI_AP_REC_PROBE].sta_lock[WIFI_AP_BAND_24G]), NULL);
    pthread_rwlock_init(&(lock_list[WIFI_AP_REC_PROBE].sta_lock[WIFI_AP_BAND_5G]), NULL);

    /* Init assoc sta table */
    for (i = 0; i < STA_HASH_SIZE; i++)
        for (j = 0; j < WIFI_BAND_TYPE_MAX; j++)
            assoc_list[j].sta_hash[i] = NULL;
    pthread_rwlock_init(&(lock_list[WIFI_AP_REC_ASSOC].sta_lock[WIFI_AP_BAND_24G]), NULL);
    pthread_rwlock_init(&(lock_list[WIFI_AP_REC_ASSOC].sta_lock[WIFI_AP_BAND_5G]), NULL);
}

/**
 * Deinit the hash table.
 */
void siwifi_sta_list_deinit()
{
    struct sta_info *sta = NULL, *sta_tmp = NULL;
    int i, j;

    /* Traverse the hash table and release it one by one */
    /* Free roam sta list */
    for (i = 0; i < STA_HASH_SIZE; i++) {
        sta = (struct sta_info *)(roam_sta_hash[i]);;
        while (sta != NULL) {
            sta_tmp = sta;
            sta = sta->next;
            free(sta_tmp);
        }
    }
    pthread_rwlock_destroy(&(lock_list[WIFI_AP_STA_ROAM].sta_lock[0]));

    /* Free probe sta table */
    for (i = 0; i < STA_HASH_SIZE; i++) {
        for (j = 0; j < WIFI_BAND_TYPE_MAX; j++) {
            sta = (struct sta_info *)(probe_list[j].sta_hash[i]);;
            while (sta != NULL) {
                sta_tmp = sta;
                sta = sta->next;
                free(sta_tmp);
            }
        }
    }
    pthread_rwlock_destroy(&(lock_list[WIFI_AP_REC_PROBE].sta_lock[WIFI_AP_BAND_24G]));
    pthread_rwlock_destroy(&(lock_list[WIFI_AP_REC_PROBE].sta_lock[WIFI_AP_BAND_5G]));

    /* Free assoc sta table */
    for (i = 0; i < STA_HASH_SIZE; i++) {
        for (j = 0; j < WIFI_BAND_TYPE_MAX; j++) {
            sta = (struct sta_info *)(assoc_list[j].sta_hash[i]);;
            while (sta != NULL) {
                sta_tmp = sta;
                sta = sta->next;
                free(sta_tmp);
            }
        }
    }
    pthread_rwlock_destroy(&(lock_list[WIFI_AP_REC_ASSOC].sta_lock[WIFI_AP_BAND_24G]));
    pthread_rwlock_destroy(&(lock_list[WIFI_AP_REC_ASSOC].sta_lock[WIFI_AP_BAND_5G]));
}

/**
 * Get the sta of the corresponding Hash table according to macaddr, table_type and band_type.
 * Return NULL if not found.
 */
static struct sta_info *siwifi_sta_get(uint8_t *mac, int table_type, int band_type)
{
    struct sta_info *sta = NULL;
    int hash_idx = STA_HASH(mac);

    /* Get the header of the corresponding hash table by table_type */
    switch(table_type) {
        case WIFI_AP_STA_ROAM:
            sta = (struct sta_info *)(roam_sta_hash[hash_idx]);
            break;
        case WIFI_AP_REC_PROBE:
            sta = (struct sta_info *)(probe_list[band_type].sta_hash[hash_idx]);
            break;
        case WIFI_AP_REC_ASSOC:
            sta = (struct sta_info *)(assoc_list[band_type].sta_hash[hash_idx]);
            break;
        default:
            LOG("table_type error !\n");
            break;
    }

    /* Find out if STA is in the hash table */
    while (sta != NULL && memcmp(sta->sta_mac, mac, ETH_ALEN))
        sta = sta->next;

    return sta;
}

/**
 * Add sta to the sta list in the corresponding Hash table according to table_type and band_type.
 */
static void siwifi_sta_add(uint8_t *mac, int rssi, int table_type, int band_type)
{
    struct sta_info *sta = NULL, *sta_tmp = NULL;
    int hash_idx = STA_HASH(mac);

    /* Request memory and initialize STA information */
    sta = (struct sta_info *)malloc(sizeof(struct sta_info));
    if (sta) {
        sta->rx_sec = siwifi_get_time();
        sta->rx_rssi = rssi;
        memcpy(sta->sta_mac, mac, ETH_ALEN);
    } else {
        LOG("Failed to request memory.\n");
        return;
    }

    /* Add sta to the corresponding hash table by table_type */
    switch(table_type) {
        case WIFI_AP_STA_ROAM:
            {
                LOG("======Add roam station:%02x:%02x:%02x:%02x:%02x:%02x, rx_sec:%ld\n",
                                                MAC2STR(sta->sta_mac), sta->rx_sec);
                sta_tmp = (struct sta_info *)(roam_sta_hash[hash_idx]);
                if (sta_tmp) {
                    /* Header exists, add to header */
                    roam_sta_hash[hash_idx] = (uint64_t)sta;
                    sta->next = sta_tmp;
                } else {
                    /* The header does not exist, create a new header */
                    roam_sta_hash[hash_idx] = (uint64_t)sta;
                    sta->next = NULL;
                }
                break;
            }
        case WIFI_AP_REC_PROBE:
            {
                sta_tmp = (struct sta_info *)(probe_list[band_type].sta_hash[hash_idx]);
                if (sta_tmp) {
                    /* Header exists, add to header */
                    probe_list[band_type].sta_hash[hash_idx] = (uint64_t)sta;
                    sta->next = sta_tmp;
                } else {
                    /* The header does not exist, create a new header */
                    probe_list[band_type].sta_hash[hash_idx] = (uint64_t)sta;
                    sta->next = NULL;
                }
                break;
            }
        case WIFI_AP_REC_ASSOC:
            {
                sta_tmp = (struct sta_info *)(assoc_list[band_type].sta_hash[hash_idx]);
                if (sta_tmp) {
                    /* Header exists, add to header */
                    assoc_list[band_type].sta_hash[hash_idx] = (uint64_t)sta;
                    sta->next = sta_tmp;
                } else {
                    /* The header does not exist, create a new header */
                    assoc_list[band_type].sta_hash[hash_idx] = (uint64_t)sta;
                    sta->next = NULL;
                }
                break;
            }
        default:
            {
                LOG("table_type error !\n");
                break;
            }
    }

    return;
}

/**
 * Delete the sta in the corresponding Hash table according to macaddr, table_type and band_type.
 */
static void siwifi_sta_del(uint8_t *mac, int table_type, int band_type)
{
    struct sta_info *sta = NULL, *sta_tmp = NULL, *sta_next = NULL;
    int hash_idx = STA_HASH(mac);

    /* Find the hash header based on the table type, and if the header is the STA to be deleted, delete it and return */
    switch(table_type) {
        case WIFI_AP_STA_ROAM:
            {
                sta = (struct sta_info *)(roam_sta_hash[hash_idx]);
                /* The header is the STA to be deleted, delete it */
                if (!memcmp(mac, sta->sta_mac, ETH_ALEN)) {
                    roam_sta_hash[hash_idx] = (uint64_t)sta->next;
                    free(sta);
                    return;
                }
                break;
            }
        case WIFI_AP_REC_PROBE:
            {
                sta = (struct sta_info *)(probe_list[band_type].sta_hash[hash_idx]);
                /* The header is the STA to be deleted, delete it */
                if (!memcmp(mac, sta->sta_mac, ETH_ALEN)) {
                    probe_list[band_type].sta_hash[hash_idx] = (uint64_t)sta->next;
                    free(sta);
                    return;
                }
                break;
            }
        case WIFI_AP_REC_ASSOC:
            {
                sta = (struct sta_info *)(assoc_list[band_type].sta_hash[hash_idx]);
                /* The header is the STA to be deleted, delete it */
                if (!memcmp(mac, sta->sta_mac, ETH_ALEN)) {
                    assoc_list[band_type].sta_hash[hash_idx] = (uint64_t)sta->next;
                    free(sta);
                    return;
                }
                break;
            }
        default:
            {
                LOG("table_type error !\n");
                break;
            }
    }

    /* Remove sta from the list and return */
    while (sta->next != NULL && memcmp(mac, sta->next->sta_mac, ETH_ALEN))
        sta = sta->next;
    if (sta->next != NULL) {
        sta_tmp = sta->next;
        sta->next = sta->next->next;
        free(sta_tmp);
        return;
    }

    LOG("can't found sta to delete \n");
    return;
}

/**
 * Fulush the aging sta in the corresponding Hash table according to table_type, band_type and aging_time.
 */
void siwifi_sta_flush_aged(int table_type, int band_type, int aging_time)
{
    struct sta_info *sta = NULL, *sta_tmp = NULL;
    int i;
    long cur_sec;

    pthread_rwlock_wrlock(&lock_list[table_type].sta_lock[band_type]);
    /* Delete aging STA according to table_type,band_type and aging_time */
    for (i = 0; i < STA_HASH_SIZE; i++) {
        /* Get the header of the corresponding hash table by table_type */
        cur_sec = siwifi_get_time();
        switch(table_type) {
            case WIFI_AP_STA_ROAM:
                {
                    sta = (struct sta_info *)(roam_sta_hash[i]);
                    /* Check the sta of the hash header */
                    while (sta && cur_sec >= sta->rx_sec + aging_time) {
                        roam_sta_hash[i] = (uint64_t)sta->next;
                        LOG("======flush roam station:%02X:%02X:%02X:%02X:%02X:%02X, rx_sec:%ld, cur_sec:%ld",
                                                            MAC2STR(sta->sta_mac), sta->rx_sec, cur_sec);
                        free(sta);
                        sta = (struct sta_info *)(roam_sta_hash[i]);
                    }
                    break;
                }
            case WIFI_AP_REC_PROBE:
                {
                    sta = (struct sta_info *)(probe_list[band_type].sta_hash[i]);
                    /* Check the sta of the hash header */
                    while (sta && cur_sec >= sta->rx_sec + aging_time) {
                        probe_list[band_type].sta_hash[i] = (uint64_t)sta->next;
                        free(sta);
                        sta = (struct sta_info *)(probe_list[band_type].sta_hash[i]);
                    }
                    break;
                }
            case WIFI_AP_REC_ASSOC:
                {
                    sta = (struct sta_info *)(assoc_list[band_type].sta_hash[i]);
                    /* Check the sta of the hash header */
                    while (sta && cur_sec >= sta->rx_sec + aging_time) {
                        assoc_list[band_type].sta_hash[i] = (uint64_t)sta->next;
                        free(sta);
                        sta = (struct sta_info *)(assoc_list[band_type].sta_hash[i]);
                    }
                    break;
                }
            default:
                LOG("table_type error !\n");
                break;
        }
        /* Check the sta in the hash table */
        while (sta && sta->next != NULL) {
            if (sta->next && cur_sec >= sta->next->rx_sec + aging_time) {
                sta_tmp = sta->next;
                sta->next = sta->next->next;
                free(sta_tmp);
            } else {
                sta = sta->next;
            }
        }
    }
    pthread_rwlock_unlock(&lock_list[table_type].sta_lock[band_type]);
}

/**
 * Update the sta information in the corresponding Hash table according to table_type and band_type.
 * If there is no sta, create a new one.
 */
static void siwifi_sta_update(uint8_t *mac, int rssi, int table_type, int band_type, int aging_time)
{
    struct sta_info *sta = NULL, *sta_tmp = NULL;
    int i = 0;

    /* Determine if STA is in the hash table. If so, update the information. Otherwise, create a new one. */
    sta = siwifi_sta_get(mac, table_type, band_type);
    if (sta) {
        /* Exists, update information */
        sta->rx_rssi = rssi;
        sta->rx_sec = siwifi_get_time();
        if (table_type == WIFI_AP_STA_ROAM) {
            LOG("======Update roam station:%02x:%02x:%02x:%02x:%02x:%02x, rx_sec:%ld\n",
                                                MAC2STR(sta->sta_mac), sta->rx_sec);
        }
    } else {
        /* No, add new one */
        siwifi_sta_add(mac, rssi, table_type, band_type);
    }
}

enum {
    BSS_STATUS,
    BSS_BSSID,
    BSS_SSID,
    BSS_FREQ,
    BSS_CHANNEL,
    BSS_OP_CLASS,
    __BSS_INFO_MAX
};

static void siwifi_get_status_cb(struct ubus_request *req, int type, struct blob_attr *msg)
{
    struct blob_attr *tb[__BSS_INFO_MAX], *cur;
    int rem;

    const struct blobmsg_policy bss_info_policy[] = {
        { .name = "status", .type = BLOBMSG_TYPE_STRING },
        { .name = "bssid", .type = BLOBMSG_TYPE_STRING },
        { .name = "ssid", .type = BLOBMSG_TYPE_STRING },
        { .name = "freq", .type = BLOBMSG_TYPE_INT32 },
        { .name = "channel", .type = BLOBMSG_TYPE_INT32 },
        { .name = "op_class", .type = BLOBMSG_TYPE_INT32 },
    };

    if (!msg)
        return;

    /* Parse msg and put it into tb */
    blobmsg_parse(bss_info_policy, ARRAY_SIZE(bss_info_policy), tb, blob_data(msg), blob_len(msg));

    /* Get bss info from tb */
    strcpy(((struct bss_info *)req->priv)->sta_mac, blobmsg_get_string(tb[BSS_BSSID]));
    ((struct bss_info *)req->priv)->channel = blobmsg_get_u32(tb[BSS_CHANNEL]);
    ((struct bss_info *)req->priv)->op_class = blobmsg_get_u32(tb[BSS_OP_CLASS]);
}

/**
 * Trigger STA to roam to another band.
 */
static void siwifi_trigger_roam(char *mac, int band_type)
{
    struct bss_info info;
    uint8_t mac_addr[ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    char neighbors[33], channel[2], op_class[2];
    void *array;

    LOG("======Start Roam======band_type:%s======\n", band_type ? "5G" : "2.4G");
    /* get other band's bss info and trigger roam */
    if (band_type == WIFI_AP_BAND_24G && high_band_obj_id != 0)
        ubus_invoke(ubus_ctx, high_band_obj_id, "get_status", b.head, siwifi_get_status_cb, &info, 1000);
    else if (band_type == WIFI_AP_BAND_5G && low_band_obj_id != 0)
        ubus_invoke(ubus_ctx, low_band_obj_id, "get_status", b.head, siwifi_get_status_cb, &info, 1000);

    /* Prepare to send information for btm_req */
    blob_buf_init(&b, 0);
    blobmsg_add_string(&b, "addr", mac);
    blobmsg_add_u8(&b, "disassociation_imminent", 1);
    blobmsg_add_u32(&b, "disassociation_timer", 0);
    blobmsg_add_u32(&b, "validity_period", 100);
    array = blobmsg_open_array(&b, "neighbors");
    /* bssid */
    neighbors[0] = info.sta_mac[0];
    neighbors[1] = info.sta_mac[1];
    neighbors[2] = info.sta_mac[3];
    neighbors[3] = info.sta_mac[4];
    neighbors[4] = info.sta_mac[6];
    neighbors[5] = info.sta_mac[7];
    neighbors[6] = info.sta_mac[9];
    neighbors[7] = info.sta_mac[10];
    neighbors[8] = info.sta_mac[12];
    neighbors[9] = info.sta_mac[13];
    neighbors[10] = info.sta_mac[15];
    neighbors[11] = info.sta_mac[16];
    /* bss info */
    memcpy(&neighbors[12], "00000000", 8);
    /* op_class */
    dec_to_hexstr(info.op_class, op_class, 2);
    memcpy(&neighbors[20], op_class, 2);
    /* channel */
    dec_to_hexstr(info.channel, channel, 2);
    memcpy(&neighbors[22], channel, 2);
    /* phy */
    memcpy(&neighbors[24], band_type ? "01" : "00", 2);
    /* BSS Transition Candidate Preference subelement */
    memcpy(&neighbors[26], "0301ff", 6);
    /* end */
    neighbors[32] = '\0';
    blobmsg_add_string(&b, NULL, neighbors);
    blobmsg_close_array(&b, array);
    blobmsg_add_u8(&b, "abridged", 1);
    blobmsg_add_u32(&b, "dialog_token", 1);

    /* send btm_req */
    LOG("Send BSS Transition Request to station[%s], Direction:%s\n", mac, band_type ? "5G->2.4G" : "2.4G->5G");
    if (band_type == WIFI_AP_BAND_5G && high_band_obj_id != 0)
        ubus_invoke(ubus_ctx, high_band_obj_id, "bss_transition_request", b.head, NULL, 0, 1000);
    else if (band_type == WIFI_AP_BAND_24G && low_band_obj_id != 0)
        ubus_invoke(ubus_ctx, low_band_obj_id, "bss_transition_request", b.head, NULL, 0, 1000);

    /* Update the hash table */
    str_to_str16(mac_addr, mac);
    LOG(MACSTR, MAC2STR(mac_addr));
    pthread_rwlock_wrlock(&lock_list[WIFI_AP_STA_ROAM].sta_lock[0]);
    siwifi_sta_update(mac_addr, 0, WIFI_AP_STA_ROAM, 0, ROAM_AGING_TIME);
    pthread_rwlock_unlock(&lock_list[WIFI_AP_STA_ROAM].sta_lock[0]);
}

/**
 * Judge whether to reply assoc response to STA.
 */
static int siwifi_assoc_judgement(uint8_t *mac, int rssi, int band_type)
{
    struct sta_info *sta = NULL;
    int ret;
    long cur_sec;

    /* Check if STA is on the roaming list. If it is, reply with an assoc response */
    pthread_rwlock_rdlock(&lock_list[WIFI_AP_STA_ROAM].sta_lock[0]);
    if (sta = siwifi_sta_get(mac, WIFI_AP_STA_ROAM, 0)) {
        pthread_rwlock_unlock(&lock_list[WIFI_AP_STA_ROAM].sta_lock[0]);
        return 0;
    }
    pthread_rwlock_unlock(&lock_list[WIFI_AP_STA_ROAM].sta_lock[0]);

    /* Check high band */
    if (band_type == WIFI_AP_BAND_5G) {
        pthread_rwlock_wrlock(&lock_list[WIFI_AP_REC_ASSOC].sta_lock[WIFI_AP_BAND_5G]);
        /* Check if the current signal strength meets the threshold */
        if (rssi > HIGH_BAND_ROAM_RSSI) {
            pthread_rwlock_unlock(&lock_list[WIFI_AP_REC_ASSOC].sta_lock[WIFI_AP_BAND_5G]);
            return 0;
        }

        /* Check if sta is in the current band's assoc list, reply assoc response
         * if yes, otherwise reject and add it to the current assoc list */
        if (sta = siwifi_sta_get(mac, WIFI_AP_REC_ASSOC, band_type)) {
            pthread_rwlock_unlock(&lock_list[WIFI_AP_REC_ASSOC].sta_lock[WIFI_AP_BAND_5G]);
            return 0;
        } else {
            siwifi_sta_add(mac, rssi, WIFI_AP_REC_ASSOC, band_type);
            pthread_rwlock_unlock(&lock_list[WIFI_AP_REC_ASSOC].sta_lock[WIFI_AP_BAND_5G]);
            return 1;
        }
    }

    /* Check low band */
    if (band_type == WIFI_AP_BAND_24G) {
        pthread_rwlock_wrlock(&lock_list[WIFI_AP_REC_ASSOC].sta_lock[WIFI_AP_BAND_24G]);
        /* Check if the current signal strength meets the threshold */
        if (rssi < LOW_BAND_ROAM_RSSI) {
            pthread_rwlock_unlock(&lock_list[WIFI_AP_REC_ASSOC].sta_lock[WIFI_AP_BAND_24G]);
            return 0;
        }

        /* Check if sta is in the current band's assoc list, reply assoc response
         * if yes, otherwise reject and add it to the current assoc list */
        if (sta = siwifi_sta_get(mac, WIFI_AP_REC_ASSOC, band_type)) {
            pthread_rwlock_unlock(&lock_list[WIFI_AP_REC_ASSOC].sta_lock[WIFI_AP_BAND_24G]);
            return 0;
        } else {
            siwifi_sta_add(mac, rssi, WIFI_AP_REC_ASSOC, band_type);
            pthread_rwlock_unlock(&lock_list[WIFI_AP_REC_ASSOC].sta_lock[WIFI_AP_BAND_24G]);
            return 1;
        }
    }
#if 0
    /* Check if STA is on the probe list of another band.
     * If so, compare the signal strength.
     * If the curent signal strength is high, reply assco response, otherwise refuse */
    if (sta = siwifi_sta_get(mac, WIFI_AP_REC_PROBE, band_type ? WIFI_AP_BAND_24G : WIFI_AP_BAND_5G )) {
        LOG("=%s %d new:%d old:%d =\n", __func__, __LINE__, rssi, sta->rx_rssi);
        if (sta->rx_rssi < rssi) {
            LOG("=%s %d=\n", __func__, __LINE__);
            return 0;
        } else {
            LOG("=%s %d=\n", __func__, __LINE__);
            return 1;
        }
    }
#endif
}

/**
 * Judge whether to roaming to another band, return 1 means yes.
 */
static int siwifi_roam_judgement(char *mac, int rssi, int band_type)
{
    struct sta_info *sta = NULL;
    int ret = 0;
    uint8_t mac_addr[ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    /* Check if STA is on the roaming list. If it is, return 0 */
    pthread_rwlock_rdlock(&lock_list[WIFI_AP_STA_ROAM].sta_lock[0]);
    /* Conversion between different types */
    str_to_str16(mac_addr, mac);
    if (sta = siwifi_sta_get(mac_addr, WIFI_AP_STA_ROAM, 0)) {
        LOG("======[%s]======station exist!======\n", __func__);
        pthread_rwlock_unlock(&lock_list[WIFI_AP_STA_ROAM].sta_lock[0]);
        return ret;
    }
    pthread_rwlock_unlock(&lock_list[WIFI_AP_STA_ROAM].sta_lock[0]);

    /* Check if the current signal strength meets the roaming threshold */
    switch (band_type)
    {
        case WIFI_AP_BAND_24G:
            if (rssi > LOW_BAND_ROAM_RSSI) {
                siwifi_trigger_roam(mac, WIFI_AP_BAND_24G);
                ret = 1;
            }
            break;
        case WIFI_AP_BAND_5G:
            if (rssi < HIGH_BAND_ROAM_RSSI && rssi != -255) {
                siwifi_trigger_roam(mac, WIFI_AP_BAND_5G);
                ret = 1;
            }
            break;
        default:
            LOG("band type error!\n");
            break;
    }

    return ret;
}


/**
 * After receiving the ASSOC REQ, it will be called to determine whether to respond to STA.
 */
static int wifi_recevie_assoc(struct ubus_context *uctx, struct ubus_object *obj,
        struct ubus_request_data *req, const char *method,
        struct blob_attr *msg)
{
    struct blob_attr *tb[__ASSOC_REQ_MAX];
    int ret, rssi, band_type;
    uint8_t mac_addr[ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    char *str, sta_mac[32];

    str = blobmsg_format_json(msg, true);
    LOG("====Received notification '%s' : %s\n", method, str);
    free(str);

    /* Parse msg and put it into tb */
    blobmsg_parse(wifi_recevie_assoc_policy, ARRAY_SIZE(wifi_recevie_assoc_policy), tb, blob_data(msg), blob_len(msg));

    /* Get info from tb */
    strcpy(sta_mac, blobmsg_get_string(tb[ASSOC_REQ_STA_MAC]));
    band_type = (blobmsg_get_u32(tb[ASSOC_REQ_CHANNEL]) < 14) ? WIFI_AP_BAND_24G : WIFI_AP_BAND_5G;
    rssi = blobmsg_get_u32(tb[ASSOC_REQ_RSSI]);

    /* Conversion between different types */
    str_to_str16(mac_addr, sta_mac);
    LOG(MACSTR, MAC2STR(mac_addr));

    /* Determine whether to respond to STA, 0 means respond and 1 means not */
    ret = siwifi_assoc_judgement(mac_addr, rssi, band_type);

    /* Ubus response, fill ret into result */
    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "result", ret);
    ubus_send_reply(uctx, req, b.head);

    /* Update the hash table */
    pthread_rwlock_wrlock(&lock_list[WIFI_AP_REC_ASSOC].sta_lock[band_type]);
    siwifi_sta_update(mac_addr, rssi, WIFI_AP_REC_ASSOC, band_type, ASSOC_AGING_TIME);
    pthread_rwlock_unlock(&lock_list[WIFI_AP_REC_ASSOC].sta_lock[band_type]);

    return 0;
}

/**
 * After receiving the Probe REQ, it will be called to record STA.
 */
static int wifi_recevie_probe_req(struct ubus_context *uctx, struct ubus_object *obj,
        struct ubus_request_data *req, const char *method,
        struct blob_attr *msg)
{
    struct blob_attr *tb[__PROBE_REQ_MAX];
    int rssi, band_type;
    uint8_t mac_addr[ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    char *str, sta_mac[32];

    str = blobmsg_format_json(msg, true);
    //LOG("Received notification '%s' : %s\n", method, str);
    free(str);

    /* Parse msg and put it into tb */
    blobmsg_parse(wifi_recevie_probe_req_policy, ARRAY_SIZE(wifi_recevie_probe_req_policy), tb, blob_data(msg), blob_len(msg));
    /* Get info from tb */
    strcpy(sta_mac, blobmsg_get_string(tb[PROBE_REQ_STA_MAC]));
    band_type = (blobmsg_get_u32(tb[PROBE_REQ_CHANNEL]) < 14) ? WIFI_AP_BAND_24G : WIFI_AP_BAND_5G;
    rssi = blobmsg_get_u32(tb[PROBE_REQ_RSSI]);
    if (band_type == WIFI_AP_BAND_5G)
        high_band_obj_id = blobmsg_get_u32(tb[PROBE_REQ_OBJ_ID]);
    if (band_type == WIFI_AP_BAND_24G)
        low_band_obj_id = blobmsg_get_u32(tb[PROBE_REQ_OBJ_ID]);

    /* Conversion between different types */
    str_to_str16(mac_addr, sta_mac);

#if 0
    /* Update the hash table */
    pthread_rwlock_wrlock(&lock_list[WIFI_AP_REC_PROBE].sta_lock[band_type]);
    siwifi_sta_update(mac_addr, rssi, WIFI_AP_REC_PROBE, band_type, PROBE_AGING_TIME);
    pthread_rwlock_unlock(&lock_list[WIFI_AP_REC_PROBE].sta_lock[band_type]);
#endif

    return 0;
}

static void siwifi_get_client_cb(struct ubus_request *req, int type, struct blob_attr *msg)
{
    struct blob_attr *tb[2], *cur, *cur_tmp;
    char sta_mac[32];
    int rem, rem_tmp, band_type = -1, rssi = -255, ret = 0;
    uint32_t freq;

    const struct blobmsg_policy sta_info_policy[] = {
        { .name = "freq", .type = BLOBMSG_TYPE_INT32 },
        { .name = "clients", .type = BLOBMSG_TYPE_TABLE },
    };

    if (!msg)
        return;

    /* Parse msg and put it into tb */
    blobmsg_parse(sta_info_policy, ARRAY_SIZE(sta_info_policy), tb, blob_data(msg), blob_len(msg));

    /* get current band type */
    if (tb[0] && blobmsg_get_u32(tb[0]) < 5180)
        band_type = WIFI_AP_BAND_24G;
    else
        band_type = WIFI_AP_BAND_5G;

    /* Traverse every sta */
    blobmsg_for_each_attr(cur, tb[1], rem) {
        /* Get mac addr */
        strcpy(sta_mac, blobmsg_name(cur));
        /* Get rssi */
        blobmsg_for_each_attr(cur_tmp, cur, rem_tmp) {
            if (strcmp(blobmsg_name(cur_tmp), "signal") == 0)
                rssi = blobmsg_get_u32(cur_tmp);
        }
        if (rssi == -255)
            return;
        /* Determine if roaming to another frequency band is necessary,
         * 0 means roaming. if roam, need to be return,*/
        if (ret = siwifi_roam_judgement(sta_mac, rssi, band_type)) {
            return;
        }
    }
}

static void sta_roam_timer_cb(struct uloop_timeout *timeout);
static struct uloop_timeout sta_roam_timer = {
    .cb = sta_roam_timer_cb,
};

static void sta_roam_timer_cb(struct uloop_timeout *timeout)
{
    /* get high band's clients */
    if (high_band_obj_id != 0)
        ubus_invoke(ubus_ctx, high_band_obj_id, "get_clients", b.head, siwifi_get_client_cb, NULL, 1000);
    /* get low band's clients */
    if (low_band_obj_id != 0)
        ubus_invoke(ubus_ctx, low_band_obj_id, "get_clients", b.head, siwifi_get_client_cb, NULL, 1000);

    /* Delete the aged STA from table */
    siwifi_sta_flush_aged(WIFI_AP_STA_ROAM, 0, ROAM_AGING_TIME);
#if 0
    siwifi_sta_flush_aged(WIFI_AP_REC_PROBE, WIFI_AP_BAND_24G, PROBE_AGING_TIME);
    siwifi_sta_flush_aged(WIFI_AP_REC_PROBE, WIFI_AP_BAND_5G, PROBE_AGING_TIME);
#endif
    siwifi_sta_flush_aged(WIFI_AP_REC_ASSOC, WIFI_AP_BAND_24G, ASSOC_AGING_TIME);
    siwifi_sta_flush_aged(WIFI_AP_REC_ASSOC, WIFI_AP_BAND_5G, ASSOC_AGING_TIME);

    uloop_timeout_set(&sta_roam_timer, ROAM_CHECK_TIME);
}

/**
 * Set up timer update high band sta list.
 */
void roam_timer_init(void)
{
    uloop_timeout_set(&sta_roam_timer, ROAM_CHECK_TIME);
}

/**
 * Cancel update sta list timer.
 */
void roam_timer_deinit(void)
{
    uloop_timeout_cancel(&sta_roam_timer);
}

int band_steering_server_init(struct ubus_context *ctx)
{
    /* Add object */
    return ubus_add_object(ctx, &band_steering);
}

