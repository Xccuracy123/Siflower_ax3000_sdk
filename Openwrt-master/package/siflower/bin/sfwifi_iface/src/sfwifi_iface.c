/*
 * =====================================================================================
 *
 *       Filename:  sfwifi_iface.c
 *
 *    Description:  Interface for wifi driver.
 *
 *        Version:  1.0
 *        Created:  2023年05月23日 11时29分58秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ming.guang , ming.guang@siflower.com.cn
 *        Company:  Siflower
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/wireless.h>
#include <sys/time.h>
#include <dirent.h>
#include <linux/errno.h>
#include <math.h>
#include "sfwifi_iface.h"

/* ======================================= Definition of relevant structures ====================================== */

/* ifname */
char ifname[16] = {"wlan0"};
/* soctet */
static int32_t ioctl_socket = -1;
typedef void* caddr_t;

struct sfcfghdr {
    uint32_t magic_no;
    uint32_t comand_type;
    uint32_t comand_id;
    uint16_t length;
    uint16_t sequence;
    uint32_t status;
    char  data[25600];
}__attribute__((packed));

#define SFCFG_MAGIC_IFACE           0x18182023
#define SFCFG_PRIV_IOCTL_IFACE      (SIOCIWFIRSTPRIV + 0x09)
#define SFCFG_BLACK_LIST_LB         "/lib/firmware/siwifi_blacklist_lb.ini"
#define SFCFG_BLACK_LIST_HB         "/lib/firmware/siwifi_blacklist_hb.ini"
#define SFCFG_WHITE_LIST_LB         "/lib/firmware/siwifi_whitelist_lb.ini"
#define SFCFG_WHITE_LIST_HB         "/lib/firmware/siwifi_whitelist_hb.ini"
#define SFCFG_BLACK_WHITE_LIST_TMP  "/lib/firmware/siwifi_bwlist_tmp.ini"
#define MAX_BLACK_WHITE_LIST_STA_CNT 130

/*  command id */
#define SF_VDR_CMD_MGMT_IE               0x0000
#define SF_VDR_CMD_BLACK_WHITE_LIST      0x0001
#define SF_VDR_CMD_STA_INFO              0x0002
#define SF_VDR_CMD_SCAN                  0x0003
#define SF_VDR_CMD_CON_CODE_INFO         0x0004
#define SF_VDR_CMD_CHAN_INFO             0x0005
#define SF_VDR_CMD_REPEATER_STATE        0x0006
#define SF_VDR_SET_RETRY_NUM             0x0007
#define SF_VDR_CMD_SET_CCA               0x0008
#define SF_VDR_CMD_SET_AGG_THRES         0x0009
#define SF_VDR_CMD_EDCA                  0x000a
#define SF_VDR_CMD_ACS                   0x000b
#define SF_VDR_CMD_DFS                   0x000c
#define SF_VDR_CMD_APSD                  0x000d
#define SF_VDR_CMD_GET_FRAME             0x000e
#define SF_VDR_CMD_SEND_DEBUG_FRAME      0x000f
#define SF_VDR_CMD_SET_POWER             0x0010
#define SF_VDR_CMD_SET_CALI_TABLE        0x0011
#define SF_VDR_CMD_SET_ANT               0x0012
#define SF_VDR_CMD_SET_THERMAL_ON        0x0013
#define SF_VDR_CMD_GET_VAP_STATS         0x0014
#define SF_VDR_CMD_GET_RF                0x0015
#define SF_VDR_CMD_GET_SCAN_RESULT       0x0016
#define SF_VDR_CMD_SET_POWER_LIMIT       0x0017
#define SF_VDR_CMD_GET_CHANNEL_SCORE     0x0018
#define SF_VDR_CMD_SET_CHAN              0x0019
#define SF_VDR_CMD_MON_STA               0x001a
#define SF_VDR_CMD_RADAR_STATUS          0X001b
#define SF_VDR_CMD_RADAR_THRESHOLD       0x001c
#define SF_VDR_CMD_SET_RTS_THRESHOLD     0x001d
#define SF_VDR_CMD_SET_RADAR_DETECT      0x001e
#define SF_VDR_CMD_GET_RADAR_DETECT      0x001f
#define SF_VDR_CMD_SET_SRRC              0x0020
#define SF_VDR_CMD_GET_WDS_STATS         0x0021
#define SF_VDR_CMD_SET_VLAN              0x0022
#define SF_VDR_CMD_SET_FEAT              0x0023

#define IS_TX 1
#define IS_RX 0

static char * format_enc_suites(int suites)
{
    static char str[64] = { 0 };
    char *pos = str;

    if (suites & KMGMT_PSK)
        pos += sprintf(pos, "PSK/");

    if (suites & KMGMT_8021x)
        pos += sprintf(pos, "802.1X/");

    if (suites & KMGMT_SAE)
        pos += sprintf(pos, "SAE/");

    if (suites & KMGMT_OWE)
        pos += sprintf(pos, "OWE/");

    if (!suites || (suites & KMGMT_NONE))
        pos += sprintf(pos, "NONE/");

    *(pos - 1) = 0;

    return str;
}

static char * format_enc_ciphers(int ciphers)
{
    static char str[128] = { 0 };
    char *pos = str;

    if (ciphers & CIPHER_WEP40)
        pos += sprintf(pos, "WEP-40, ");

    if (ciphers & CIPHER_WEP104)
        pos += sprintf(pos, "WEP-104, ");

    if (ciphers & CIPHER_TKIP)
        pos += sprintf(pos, "TKIP, ");

    if (ciphers & CIPHER_CCMP)
        pos += sprintf(pos, "CCMP, ");

    if (ciphers & CIPHER_GCMP)
        pos += sprintf(pos, "GCMP, ");

    if (ciphers & CIPHER_WRAP)
        pos += sprintf(pos, "WRAP, ");

    if (ciphers & CIPHER_AESOCB)
        pos += sprintf(pos, "AES-OCB, ");

    if (ciphers & CIPHER_CKIP)
        pos += sprintf(pos, "CKIP, ");

    if (!ciphers || (ciphers & CIPHER_NONE))
        pos += sprintf(pos, "NONE, ");

    *(pos - 2) = 0;

    return str;
}

static char *format_encryption(struct siwifi_crypto_entry *c)
{
    static char buf[512];
    char *pos = buf;
    int i, n;

    if (!c)
    {
        snprintf(buf, sizeof(buf), "unknown");
    }
    else if (c->enabled)
    {
        /* WEP */
        if (c->auth_algs && !c->wpa_version)
        {
            if ((c->auth_algs & AUTH_OPEN) &&
                (c->auth_algs & AUTH_SHARED))
            {
                snprintf(buf, sizeof(buf), "WEP Open/Shared (%s)",
                    format_enc_ciphers(c->pair_ciphers));
            }
            else if (c->auth_algs & AUTH_OPEN)
            {
                snprintf(buf, sizeof(buf), "WEP Open System (%s)",
                    format_enc_ciphers(c->pair_ciphers));
            }
            else if (c->auth_algs & AUTH_SHARED)
            {
                snprintf(buf, sizeof(buf), "WEP Shared Auth (%s)",
                    format_enc_ciphers(c->pair_ciphers));
            }
        }

        /* WPA */
        else if (c->wpa_version)
        {
            for (i = 0, n = 0; i < 3; i++)
                if (c->wpa_version & (1 << i))
                    n++;

            if (n > 1)
                pos += sprintf(pos, "mixed ");

            for (i = 0; i < 3; i++){
                if (c->wpa_version & (1 << i)){
                    if (i)
                        pos += sprintf(pos, "WPA%d/", i + 1);
                    else
                        pos += sprintf(pos, "WPA/");
                }
            }
            pos--;

            sprintf(pos, " %s (%s)",
                format_enc_suites(c->auth_suites),
                format_enc_ciphers(c->pair_ciphers | c->group_ciphers));
        }
        else
        {
            snprintf(buf, sizeof(buf), "none");
        }
    }
    else
    {
        snprintf(buf, sizeof(buf), "none");
    }

    return buf;
}

char *sf_channel_width_name(enum nl80211_chan_width width)
{
    switch (width) {
    case NL80211_CHAN_WIDTH_20_NOHT:
        return "20 MHz (no HT)";
    case NL80211_CHAN_WIDTH_20:
        return "20 MHz";
    case NL80211_CHAN_WIDTH_40:
        return "40 MHz";
    case NL80211_CHAN_WIDTH_80:
        return "80 MHz";
    case NL80211_CHAN_WIDTH_80P80:
        return "80+80 MHz";
    case NL80211_CHAN_WIDTH_160:
        return "160 MHz";
    case NL80211_CHAN_WIDTH_5:
        return "5 MHz";
    case NL80211_CHAN_WIDTH_10:
        return "10 MHz";
    default:
        return "unknown";
    }
}

uint16_t siwifi_freq_to_channel(uint16_t freq)
{
    if (freq == 2484)
        return 14;
    else if (freq < 2484)
        return (freq - 2407) / 5;
    else if (freq >= 4910 && freq <= 4980)
        return (freq - 4000) / 5;
    else
        return (freq - 5000) / 5;
}

/* Get the specific bandwidth for items in enum rate_info_bw */
uint32_t siwifi_get_bandwidth(uint8_t bw)
{
    uint32_t bandwidth;

    switch (bw) {
    case RATE_INFO_BW_5:
        bandwidth = 5;
        break;
    case RATE_INFO_BW_10:
        bandwidth = 10;
        break;
    case RATE_INFO_BW_20:
        bandwidth = 20;
        break;
    case RATE_INFO_BW_40:
        bandwidth = 40;
        break;
    case RATE_INFO_BW_80:
        bandwidth = 80;
        break;
    case RATE_INFO_BW_160:
        bandwidth = 160;
        break;
    case RATE_INFO_BW_HE_RU:
        /* bandwidth determined by HE RU allocation */
    default:
        bandwidth = 0;
    }

    return bandwidth;
}

/* Get the specific format_mod name */
const char *siwifi_get_format_mod(uint16_t format)
{
    const char *format_strings[] = { "NON_HT", "NON_HT_DUP_OFDM", "HT_MF", "HT_GF", "VHT",
                                     "HE_SU",  "HE_MU",           "HE_ER", "HE_TB" };
    return format_strings[format];
}

static uint32_t calculate_bitrate_ht(struct siwifi_station_info *info, bool is_tx)
{
    int modulation, streams, bitrate;
    int mcs;
    enum rate_info_bw bw;
    struct siwifi_sta_rate_info *rate;

    if(is_tx){
        mcs = info->tx_mcs;
        bw = info->tx_bw;
        rate = &info->txrate;
    }
    else{
        mcs = info->rx_mcs;
        bw = info->rx_bw;
        rate = &info->rxrate;
    }
    /* the formula below does only work for MCS values smaller than 32 */
    if (mcs >= 32)
        return 0;

    modulation = mcs & 7;
    streams = (mcs >> 3) + 1;

    bitrate = (bw == RATE_INFO_BW_40) ? 13500000 : 6500000;

    if (modulation < 4)
        bitrate *= (modulation + 1);
    else if (modulation == 4)
        bitrate *= (modulation + 2);
    else
        bitrate *= (modulation + 3);

    bitrate *= streams;

    if (rate->flags & RATE_INFO_FLAGS_SHORT_GI)
        bitrate = (bitrate / 9) * 10;

    /* do NOT round down here */
    return (bitrate + 50000) / 100000;
}

static uint32_t calculate_bitrate_vht(struct siwifi_station_info *info, bool is_tx)
{
    int mcs;
    u8 nss;
    enum rate_info_bw bw;
    struct siwifi_sta_rate_info *rate;

    if(is_tx){
        mcs = info->tx_mcs;
        nss = info->tx_nss;
        bw = info->tx_bw;
        rate = &info->txrate;
    }
    else{
        mcs = info->rx_mcs;
        nss = info->rx_nss;
        bw = info->rx_bw;
        rate = &info->rxrate;
    }
    static const uint32_t base[4][12] = {
        {   6500000,
            13000000,
            19500000,
            26000000,
            39000000,
            52000000,
            58500000,
            65000000,
            78000000,
        /* not in the spec, but some devices use this: */
            86700000,
            97500000,
            108300000,
        },
        {   13500000,
            27000000,
            40500000,
            54000000,
            81000000,
            108000000,
            121500000,
            135000000,
            162000000,
            180000000,
            202500000,
            225000000,
        },
        {   29300000,
            58500000,
            87800000,
            117000000,
            175500000,
            234000000,
            263300000,
            292500000,
            351000000,
            390000000,
            438800000,
            487500000,
        },
        {   58500000,
            117000000,
            175500000,
            234000000,
            351000000,
            468000000,
            526500000,
            585000000,
            702000000,
            780000000,
            877500000,
            975000000,
        },
    };
    uint32_t bitrate;
    int idx;

    if (mcs > 11)
        goto warn;

    switch (bw)
    {
    case RATE_INFO_BW_160:
        idx = 3;
        break;
    case RATE_INFO_BW_80:
        idx = 2;
        break;
    case RATE_INFO_BW_40:
        idx = 1;
        break;
    case RATE_INFO_BW_20:
        idx = 0;
        break;
    case RATE_INFO_BW_5:
    case RATE_INFO_BW_10:
    default:
        goto warn;
    }

    bitrate = base[idx][mcs];
    bitrate *= nss;

    if (rate->flags & RATE_INFO_FLAGS_SHORT_GI)
        bitrate = (bitrate / 9) * 10;

    /* do NOT round down here */
    return (bitrate + 50000) / 100000;

warn:
    printf("invalid rate bw=%d, mcs=%d, nss=%d\n", bw, mcs, nss);
    return 0;
}

static u32 cfg80211_calculate_bitrate_he(struct siwifi_station_info *info, bool is_tx)
{
    int mcs;
    u8 nss;
    enum rate_info_bw bw;
    struct siwifi_sta_rate_info *rate;

    if(is_tx){
        mcs = info->tx_mcs;
        nss = info->tx_nss;
        bw = info->tx_bw;
        rate = &info->txrate;
    }
    else{
        mcs = info->rx_mcs;
        nss = info->rx_nss;
        bw = info->rx_bw;
        rate = &info->rxrate;
    }
#define SCALE 6144
    u32 mcs_divisors[14] = {
        102399, /* 16.666666... */
         51201, /*  8.333333... */
         34134, /*  5.555555... */
         25599, /*  4.166666... */
         17067, /*  2.777777... */
         12801, /*  2.083333... */
         11769, /*  1.851851... */
         10239, /*  1.666666... */
          8532, /*  1.388888... */
          7680, /*  1.250000... */
          6828, /*  1.111111... */
          6144, /*  1.000000... */
          5690, /*  0.926106... */
          5120, /*  0.833333... */
    };
    u32 rates_160M[3] = { 960777777, 907400000, 816666666 };
    u32 rates_969[3] =  { 480388888, 453700000, 408333333 };
    u32 rates_484[3] =  { 229411111, 216666666, 195000000 };
    u32 rates_242[3] =  { 114711111, 108333333,  97500000 };
    u32 rates_106[3] =  {  40000000,  37777777,  34000000 };
    u32 rates_52[3]  =  {  18820000,  17777777,  16000000 };
    u32 rates_26[3]  =  {   9411111,   8888888,   8000000 };
    uint64_t tmp;
    u32 result;

    if (mcs > 13)
        return 0;

    if (rate->he_gi > NL80211_RATE_INFO_HE_GI_3_2)
        return 0;
    if (rate->he_ru_alloc >
             NL80211_RATE_INFO_HE_RU_ALLOC_2x996)
        return 0;
    if (nss < 1 || nss > 8)
        return 0;

    if (bw == RATE_INFO_BW_160)
        result = rates_160M[rate->he_gi];
    else if (bw == RATE_INFO_BW_80 ||
         (bw == RATE_INFO_BW_HE_RU &&
          rate->he_ru_alloc == NL80211_RATE_INFO_HE_RU_ALLOC_996))
        result = rates_969[rate->he_gi];
    else if (bw == RATE_INFO_BW_40 ||
         (bw == RATE_INFO_BW_HE_RU &&
          rate->he_ru_alloc == NL80211_RATE_INFO_HE_RU_ALLOC_484))
        result = rates_484[rate->he_gi];
    else if (bw == RATE_INFO_BW_20 ||
         (bw == RATE_INFO_BW_HE_RU &&
          rate->he_ru_alloc == NL80211_RATE_INFO_HE_RU_ALLOC_242))
        result = rates_242[rate->he_gi];
    else if (bw == RATE_INFO_BW_HE_RU &&
         rate->he_ru_alloc == NL80211_RATE_INFO_HE_RU_ALLOC_106)
        result = rates_106[rate->he_gi];
    else if (bw == RATE_INFO_BW_HE_RU &&
         rate->he_ru_alloc == NL80211_RATE_INFO_HE_RU_ALLOC_52)
        result = rates_52[rate->he_gi];
    else if (bw == RATE_INFO_BW_HE_RU &&
         rate->he_ru_alloc == NL80211_RATE_INFO_HE_RU_ALLOC_26)
        result = rates_26[rate->he_gi];
    else {
        printf("invalid HE MCS: bw:%d, ru:%d\n", bw, rate->he_ru_alloc);
        return 0;
    }

    /* now scale to the appropriate MCS */
    tmp = result;
    tmp *= SCALE;
    result = tmp / mcs_divisors[mcs];

    /* and take NSS, DCM into account */
    result = (result * nss) / 8;
    if (rate->he_dcm)
        result /= 2;

    return result / 10000;
}

/* Note: The outcomes computed by this function equals the bitrate multiplied by 10.
 * This design facilitates preserving one decimal place for subsequent operations. */
uint32_t calculate_bitrate(struct siwifi_station_info *info, bool is_tx)
{
    struct siwifi_sta_rate_info *rate;
    if(is_tx){
        rate = &info->txrate;
    }
    else{
        rate = &info->rxrate;
    }
    if (rate->flags & RATE_INFO_FLAGS_MCS)
        return calculate_bitrate_ht(info, is_tx);
    if (rate->flags & RATE_INFO_FLAGS_VHT_MCS)
        return calculate_bitrate_vht(info, is_tx);
    if (rate->flags & RATE_INFO_FLAGS_HE_MCS)
        return cfg80211_calculate_bitrate_he(info, is_tx);

    return rate->legacy;
}

/* ========================================= Interface implementation ============================================ */
/* create the socket fd to wext ioctl */
static int32_t get_ioctl_socket(void)
{
    /*  Prepare socket */
    if (ioctl_socket == -1) {
        ioctl_socket = socket(AF_INET, SOCK_DGRAM, 0);
        fcntl(ioctl_socket, F_SETFD, fcntl(ioctl_socket, F_GETFD) | FD_CLOEXEC);
    }

    return ioctl_socket;
}

/* close the ioctl socket fd */
static void close_ioctl_socket(void)
{
    if (ioctl_socket > -1)
        close(ioctl_socket);
    ioctl_socket = -1;
}

/* do the ioctl */
static int32_t do_ioctl(int32_t cmd, struct iwreq *wrq)
{
    int32_t s = get_ioctl_socket();
    if (!wrq) {
        printf("%s error:wrq is NULL\n",__func__);
        return -1;
    }
    strncpy(wrq->ifr_name, ifname, IFNAMSIZ);
    if (s < 0)
        return s;
    return ioctl(s, cmd, wrq);
}

/* test case mgmt_ie */
static int sf_vdr_add_mgmt_ie(char *test_str)
{
    struct iwreq wrq;
    struct sfcfghdr tmp;
    struct sf_vdr_common_data *test_info;

    /* Fill tmp */
    memset(&tmp, 0, sizeof(struct sfcfghdr));
    tmp.magic_no = SFCFG_MAGIC_IFACE;
    tmp.comand_type = SF_VDR_CMD_MGMT_IE;
    tmp.comand_id = SF_VDR_CMD_MGMT_IE;
    tmp.length = 52;
    tmp.sequence = 1;
    memcpy(tmp.data, test_str, 20);

    /* Fill wrq */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&tmp;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

    /* send msg to driver */
    if (do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_MGMT_IE failed\n");
        return -1;
    }

    /* Malloc memery space */
    test_info = (struct sf_vdr_common_data *)malloc(sizeof(struct sf_vdr_common_data));
    if (!test_info) {
        printf("memery alloc failed !\n");
        return -2;
    }

    /* Fill test info */
    memset(test_info, 0, sizeof(struct sf_vdr_common_data));
    memcpy(test_info, wrq.u.data.pointer, sizeof(struct sf_vdr_common_data));

    /* Print test info */
    printf("\t str:%s \n", test_info->str);
    free(test_info);

    return 0;
}

/**
 * sf_vdr_get_band - Get frequency band of the VAP
 *
 * @param ifname: Name of VAP.
 * @param band: Return the frequency band of VAP.
 *
*/
static int sf_vdr_get_band(char *ifname, char *band)
{
    char phy_name[5], tmp_filename[50];
    FILE *fp;

    sprintf(tmp_filename, "%s/%s/%s", "/sys/class/net", ifname, "phy80211/name");
    fp = fopen(tmp_filename, "r");
    if (!fp) {
        printf("Error opening file:%s\n", tmp_filename);
        return -1;
    }
    if (!fgets(phy_name, sizeof(phy_name), fp)) {
        printf("Error when get phy name!\n");
        fclose(fp);
        return -1;
    }
    fclose(fp);
    memset(tmp_filename, 0, sizeof(tmp_filename));

    sprintf(tmp_filename, "%s/%s/%s", "/sys/kernel/debug/ieee80211", phy_name, "siwifi/band_type");
    fp = fopen(tmp_filename, "r");
    if (!fp) {
        printf("Error opening file:%s\n", tmp_filename);
        return -1;
    }
    if (!fgets(band, sizeof(band), fp)) {
        printf("Error when get band of interface!\n");
        fclose(fp);
        return -1;
    }
    fclose(fp);

    return 0;
}

/**
 * sf_vdr_vap_bwlist_clear - Clear stations' MAC addresses in single file.
 *
 * @filename: Name of file store stations' MAC addresses list.
 * @ifname: Name of VAP.
 *
*/
static int sf_vdr_vap_clear_single_bwlist(char *filename, char *ifname)
{
    char *pos, tmp_buf[100];
    FILE *fp, *fp_tmp;
    int clear_flag = 0;

    if (!filename || !ifname) {
        printf("[%s]: Parameter NULL!\n", __func__);
        return -1;
    }

    fp = fopen(filename, "r+");
    if (!fp) {
        printf("[%s]:Failed to open file:%s\n", __func__, filename);
        return -1;
    }

    fp_tmp = fopen(SFCFG_BLACK_WHITE_LIST_TMP, "a+");
    if (fp_tmp == NULL) {
        printf("[%s]:Error opening file: %s.\n", __func__, SFCFG_BLACK_WHITE_LIST_TMP);
        fclose(fp);
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    /* If file empty*/
    if (ftell(fp) == 0) {
        fclose(fp);
        fclose(fp_tmp);
        return 0;
    } else {
        /* File isn't empty */
        fseek(fp, 0L, SEEK_SET);
        while (fgets(tmp_buf, sizeof(tmp_buf), fp) != NULL) {
            /* find the macaddr in file */
            if (!strncmp(tmp_buf, "VAP[", 4)) {
                pos = strstr(tmp_buf, "]");
                if (pos - (tmp_buf + 4) == strlen(ifname) &&!strncmp(tmp_buf + 4, ifname, strlen(ifname))) {
                    clear_flag = 1;
                }
                else {
                    clear_flag = 0;
                }
            }
            if (!clear_flag)
                fprintf(fp_tmp, "%s", tmp_buf);

            memset(tmp_buf, 0, sizeof(tmp_buf));
        }
        fclose(fp);
        fclose(fp_tmp);

        remove(filename);
        rename(SFCFG_BLACK_WHITE_LIST_TMP, filename);
    }

    return 0;
}

/**
 * sf_vdr_vap_clear_all_bwlist - Clear stations' MAC addresses of specified VAP.
 *
 * @band: The frequency band of the VAP.
 * @ifname: Name of VAP.
 *
*/
static int sf_vdr_vap_clear_all_bwlist(char *band, char *ifname)
{
    int i;

    if (!strncmp(band, "lb", 2)) {
        char *filenames[2] = {SFCFG_BLACK_LIST_LB, SFCFG_WHITE_LIST_LB};
        for (i = 0; i < 2; i++) {
            if (sf_vdr_vap_clear_single_bwlist(filenames[i], ifname))
            return -1;
        }
    } else {
        char *filenames[2] = {SFCFG_BLACK_LIST_HB, SFCFG_WHITE_LIST_HB};
        for (i = 0; i < 2; i++) {
            if (sf_vdr_vap_clear_single_bwlist(filenames[i], ifname))
                return -1;
        }
    }
    return 0;
}

/**
 * sf_vdr_wifi_black_white_list - Black and White List Management Interface
 * @param hdr: Store the data returned by the kernel.
 * @param opt: Indicate the different operations.
 * @param list_type: Indicate the type of list.
 * @param mac_addres: String of MAC addresses.
 * @param count: The number of MAC addresses.
 *
 * Return: 0 if success, non-zero otherwise.
*/
static int sf_vdr_wifi_black_white_list(struct sfcfghdr *hdr, char *opt, char *list_type, char *mac_addres, int count)
{
    char *filename, *pos, band[3], tmp_mac[18];
    int i;
    FILE *fp;
    struct iwreq wrq;
    uint8_t use_whitelist = 0;

    if (!hdr) {
        printf("[%s]: Error: The parameter of type struct sfcfghdr is NULL\n", __func__);
        return -1;
    }

    /* Fill tmp */
    memset(hdr, 0, sizeof(struct sfcfghdr));
    hdr->magic_no = SFCFG_MAGIC_IFACE;
    hdr->comand_type = SF_VDR_CMD_BLACK_WHITE_LIST;
    hdr->comand_id = SF_VDR_CMD_BLACK_WHITE_LIST;
    hdr->sequence = 1;

    /* Fill wrq */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)hdr;

    /* Get band of VAP */
    if (sf_vdr_get_band(ifname, band)) {
        printf("Get band failed!\n");
        return -1;
    }

    /* add the macaddr to black or white list */
    if (!strcmp(opt, "set")) {
        if (!list_type){
            printf("[%s]: Parameter [list_type] can't be NULL when set policy!\n", __func__);
            return -1;
        }

        /* Clear the list first */
        if (sf_vdr_vap_clear_all_bwlist(band, ifname)) {
            printf("Clear VAP[%s] black and white list failed\n", ifname);
            return -1;
        }

        if (!strcmp(list_type, "white") || !strcmp(list_type, "black")) {
            if (!mac_addres) {
                printf("[%s]: Parameter [mac_addres] can't be NULL when add MAC into list!\n", __func__);
                return -1;
            }

            /* Add MAC addresses into file */
            if (!strcmp(list_type, "white"))
                use_whitelist = 1;
            else if (!strcmp(list_type, "black"))
                use_whitelist = 0;
            else {
                printf("[%s]: Error format for parameter [list_type], please input again!\n", __func__);
                return -1;
            }

            if (strncmp(band, "lb", 2) == 0)
                filename = use_whitelist ? SFCFG_WHITE_LIST_LB : SFCFG_BLACK_LIST_LB;
            else
                filename = use_whitelist ? SFCFG_WHITE_LIST_HB : SFCFG_BLACK_LIST_HB;

            fp = fopen(filename, "a");
            if (fp == NULL) {
                printf("Error opening file: %s.\n", filename);
                return -1;
            }

            fseek(fp, 0L, SEEK_END);
            fprintf(fp, "VAP[%s]:\n", ifname);
            pos = mac_addres;
            for (i = 0; i < count; i++) {
                strncpy(tmp_mac, pos, 17);
                tmp_mac[17] = '\0';
                fprintf(fp, "\tmacaddr=%17s\n", tmp_mac);
                pos += 18;
            }
            fclose(fp);
            sprintf(hdr->data, "%s-%s-%d~%s", opt, list_type, count, mac_addres);
            hdr->length = strlen(hdr->data);
            wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr->length;
            /* Send msg to driver to update the list */
            if (do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
                printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_BLACK_WHITE_LIST failed\n");
                return -1;
            }
        } else if (!strcmp(list_type, "none")){
            sprintf(hdr->data, "%s-%s", opt, list_type);
            hdr->length = strlen(hdr->data);
            wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr->length;
            /* Send msg to driver to update the list */
            if (do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
                printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_BLACK_WHITE_LIST failed\n");
                return -1;
            }
        } else {
            printf("[%s]: Error list_type, input again!\n", __func__);
            return -1;
        }

        if (mac_addres)
            free(mac_addres);
    } else if (!strcmp(opt, "get")) {
        /* Get the macaddr form driver */
        struct sf_vdr_bwlist_info bwlist_info;
        /* Send msg to driver to update the list */
        sprintf(hdr->data, "%s", opt);
        hdr->length = strlen(hdr->data);
        wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr->length;
        if (do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
            printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_BLACK_WHITE_LIST failed\n");
            return -1;
        }

        pos = hdr->data;
        memcpy(&bwlist_info.count, pos, sizeof(bwlist_info.count));
        pos += sizeof(bwlist_info.count);
        memcpy(&bwlist_info.length, pos, sizeof(bwlist_info.length));
        pos += sizeof(bwlist_info.length);

        printf("\t--------list_count:%hu--------\n", bwlist_info.count);
        bwlist_info.mac_addres = malloc(bwlist_info.length);
        if (!bwlist_info.mac_addres) {
            printf("Alloc for information of station's MAC failed!\n");
            return -1;
        }
        memcpy(bwlist_info.mac_addres, pos, bwlist_info.length);
        for (i = 0; i < bwlist_info.count; i++) {
            memcpy(tmp_mac, &bwlist_info.mac_addres[i * 18], 17);
            tmp_mac[17] = '\0';
            printf("\tmac_addr:%17s\n", tmp_mac);
        }
        free(bwlist_info.mac_addres);
    } else if (!strcmp(opt, "clear")) {
        /* Clear the list */
        if (sf_vdr_vap_clear_all_bwlist(band, ifname)) {
            printf("Clear VAP[%s] black and white list failed\n", ifname);
            return -1;
        }

        /* Send msg to driver to update the list */
        sprintf(hdr->data, "%s", opt);
        hdr->length = strlen(hdr->data);
        wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr->length;
        if (do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
            printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_BLACK_WHITE_LIST failed\n");
            return -1;
        }
    } else {
        printf("\tinput error!, please input again!\n");
        return -1;
    }

    return 0;
}

static int sf_vdr_sta_info()
{
    struct iwreq wrq;
    struct sfcfghdr tmp;
    char *buff = NULL;
    char *backup = NULL;
    struct siwifi_station_info *info[32];
    int sta_num, i;
    uint32_t rx_bitrate = 0;
    uint32_t tx_bitrate = 0;

    /* Fill tmp */
    memset(&tmp, 0, sizeof(struct sfcfghdr));
    tmp.magic_no = SFCFG_MAGIC_IFACE;
    tmp.comand_type = SF_VDR_CMD_STA_INFO;
    tmp.comand_id   = SF_VDR_CMD_STA_INFO;
    tmp.length      = 52;
    tmp.sequence    = 1;

    /* Fill wrq */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&tmp;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

    /* send msg to driver */
    if (do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_STA_INFO failed\n");
        return -1;
    }

    buff = malloc(wrq.u.data.length);
    backup = buff;
    if (!buff) {
        printf("memery alloc failed !\n");
        return -2;
    }

    sta_num = wrq.u.data.length / sizeof(struct siwifi_station_info);
    /* Fill test info */
    memset(buff, 0, wrq.u.data.length);
    memcpy(buff, wrq.u.data.pointer, wrq.u.data.length);

   for (i = 0; i < sta_num; i++) {
        info[i] = (struct siwifi_station_info *)buff;
        if (!info[i])
            goto DONE;
        printf("STA %d: %02x:%02x:%02x:%02x:%02x:%02x\n", i + 1, info[i]->bssid[0] & 0xff, info[i]->bssid[1] & 0xff,
                    info[i]->bssid[2] & 0xff, info[i]->bssid[3] & 0xff, info[i]->bssid[4] & 0xff, info[i]->bssid[5]);
        printf("\tlink_time\t\t%d s\n",info[i]->link_time);
        printf("\tquality\t\t\t%d/70\n", info[i]->quality);

        tx_bitrate = calculate_bitrate(info[i], IS_TX) * 1024; // from Mbps to kbps
        printf("\ttxrate: %7d.%d kbps, MCS %2d, %3dMHz, Nss %d,", tx_bitrate / 10, tx_bitrate % 10,
               info[i]->tx_mcs, siwifi_get_bandwidth(info[i]->tx_bw), info[i]->tx_nss);

        if (info[i]->txrate.format_mod > 4) // format_mod is HE
            printf(" GI: %4dns\n", 800 * (1 << info[i]->txrate.he_gi));
        else {
            if (info[i]->txrate.flags & RATE_INFO_FLAGS_SHORT_GI)
                printf(" GI:  400ns\n");
            else
                printf(" GI:  800ns\n");
        }

        rx_bitrate = calculate_bitrate(info[i], IS_RX) * 1024; // from Mbps to kbps
        printf("\trxrate: %7d.%d kbps, MCS %2d, %3dMHz, Nss %d,", rx_bitrate / 10, rx_bitrate % 10,
               info[i]->rx_mcs, siwifi_get_bandwidth(info[i]->rx_bw), info[i]->rx_nss);

        if (info[i]->rxrate.format_mod > 4) // format_mod is HE
            printf(" GI: %4dns\n", 800 * (1 << info[i]->rxrate.he_gi));
        else {
            if (info[i]->rxrate.flags & RATE_INFO_FLAGS_SHORT_GI)
                printf(" GI:  400ns\n");
            else
                printf(" GI:  800ns\n");
        }

        printf("\ttx_format_mod\t\t%s\n", siwifi_get_format_mod(info[i]->txrate.format_mod));
        printf("\trx_format_mod\t\t%s\n", siwifi_get_format_mod(info[i]->rxrate.format_mod));

        info[i]->tx_rate = tx_bitrate;
        info[i]->rx_rate = rx_bitrate;

        printf("\tlink_rate\t\t%lld.%lld kbps\n", (info[i]->link_rate) / 10, (info[i]->link_rate) % 10);
        printf("\tmax_tx_mcs\t\t%d\n", info[i]->mcs);
        printf("\ttx_mcs\t\t\t%d\n", info[i]->tx_mcs);
        printf("\trx_mcs\t\t\t%d\n", info[i]->rx_mcs);
        printf("\trssi\t\t\t%d dBm\n", info[i]->rssi);
        printf("\tnoise\t\t\t%d dBm\n", info[i]->noise);
        printf("\tsnr\t\t\t%d\n", info[i]->snr);
        printf("\ttx_rate\t\t\t%lld.%lld kbps\n", (info[i]->tx_rate) / 10, (info[i]->tx_rate) % 10);
        printf("\trx_rate\t\t\t%lld.%lld kbps\n", (info[i]->rx_rate) / 10, (info[i]->rx_rate) % 10);
        printf("\tpower_save\t\t%s\n", info[i]->power_save ? "true" : "false");
        printf("\tretransmissions\t\t%lld\n", info[i]->retransmissions);
        printf("\ttx_packets\t\t%llu\n", info[i]->tx_packets);
        printf("\trx_packets\t\t%llu\n", info[i]->rx_packets);
        printf("\ttx_bytes\t\t%llu\n", info[i]->tx_bytes);
        printf("\trx_bytes\t\t%llu\n", info[i]->rx_bytes);
        printf("\terrors_sent\t\t%lld\n", info[i]->errors_sent);
        printf("\tfailed_retrans_count\t%lld\n", info[i]->failed_retrans_count);
        printf("\tretrans_count\t\t%lld\n", info[i]->retrans_count);
        printf("\tmulti_retry_count\t%lld\n", info[i]->multi_retry_count);
        printf("\tretry_count\t\t%lld\n", info[i]->retry_count);
        printf("\ttx_kbps\t\t\t%lld.%lld kbps\n", (info[i]->tx_kbps) / 10, (info[i]->tx_kbps) % 10);
        printf("\trx_kbps\t\t\t%lld.%lld kbps\n", (info[i]->rx_kbps) / 10, (info[i]->rx_kbps) % 10);
        printf("\tdisassoc_reason\t\t%lld\n", info[i]->disassoc_reason);
        printf("\tantenna\t\t\t%lld\n", info[i]->antenna + 1);
        printf("\tbw\t\t\t%u MHz\n", siwifi_get_bandwidth(info[i]->bw));
        printf("\tbss_transition\t\t%s\n", info[i]->bss_transition ? "true" : "false");
        printf("\tbeacon_report_support\t%s\n", info[i]->beacon_report_support ? "true" : "false");
        printf("\tbeam_forming\t\t%s\n", info[i]->beam_forming ? "true" : "false");
        printf("\tMUMIMOS_up\t\t%s\n", info[i]->MUMIMOS_up ? "true" : "false");
        printf("\ttxPER\t\t\t%d%%\n", info[i]->txPER);
        printf("\trx_unicast_packets\t%lld\n", info[i]->rx_unicast_packets);
        printf("\ttx_multicast_packets\t%lld\n", info[i]->tx_multicast_packets);
        printf("\trx_multicast_packets\t%lld\n", info[i]->rx_multicast_packets);
        printf("\ttx_broadcast_packets\t%lld\n", info[i]->tx_broadcast_packets);
        printf("\trx_broadcast_packets\t%lld\n", info[i]->rx_broadcast_packets);
        printf("\ttx_mgmt_packets\t\t%lld\n", info[i]->tx_mgmt_packets);
        printf("\trx_mgmt_packets\t\t%lld\n", info[i]->rx_mgmt_packets);

        printf("\ttx_unicast_packets\t%d\n", info[i]->tx_unicast_packets);
        printf("\ttx_dropped\t\t%d\n", info[i]->tx_dropped);
        printf("\trx_dropped\t\t%d\n", info[i]->rx_dropped);
        printf("\ttx_unicast_bytes\t%lld\n", info[i]->tx_unicast_bytes);
        printf("\ttx_multicast_bytes\t%lld\n", info[i]->tx_multicast_bytes);
        printf("\ttx_broadcast_bytes\t%lld\n", info[i]->tx_broadcast_bytes);
        printf("\trx_unicast_bytes\t%lld\n", info[i]->rx_unicast_bytes);
        printf("\trx_multicast_bytes\t%lld\n", info[i]->rx_multicast_bytes);
        printf("\trx_broadcast_bytes\t%lld\n", info[i]->rx_broadcast_bytes);

        printf("\n\n");

        buff += sizeof(struct siwifi_station_info);
    }
DONE:
    free(backup);
    backup = NULL;
    return 0;
}

/**
 * siwifi_vdr_get_vap_info - Get AP information.
 *
 * @param hdr：Struct use in ioctl process.
 * @param ifname: Interface name of the vap.
 *
 * This function is used to get vap infomation.
 *
 * Return: 0 if success, non-zero otherwise, hdr->date save the struct siwifi_vap_info,
 * hdr->length save length.
 */
static int siwifi_vdr_get_vap_info(struct sfcfghdr *hdr, char *ifname)
{
    struct iwreq wrq;
    char *buff = NULL;
    char *backup = NULL;
    struct siwifi_vap_info *info = NULL;

    /* Fill tmp */
    memset(hdr, 0, sizeof(struct sfcfghdr));
    hdr->magic_no = SFCFG_MAGIC_IFACE;
    hdr->comand_type = SF_VDR_CMD_GET_VAP_STATS;
    hdr->comand_id   = SF_VDR_CMD_GET_VAP_STATS;
    hdr->length      = sizeof(struct siwifi_vap_info);
    hdr->sequence    = 1;

    sprintf(hdr->data, "%s", ifname);

    /* Fill wrq */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)hdr;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr->length;

    /* send msg to driver */
    if (do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_GET_VAP_STATS failed\n");
        return -1;
    }

    buff = malloc(wrq.u.data.length);
    backup = buff;
    if (!buff) {
        printf("memery alloc failed !\n");
        return -2;
    }

    /* Fill test info */
    memset(buff, 0, wrq.u.data.length);
    memcpy(buff, wrq.u.data.pointer, wrq.u.data.length);

    /* Print */
    info = (struct siwifi_vap_info *)buff;

    /* LOG for debugging */
    printf("BSSID = %02x:%02x:%02x:%02x:%02x:%02x\n", info->bssid[0], info->bssid[1], info->bssid[2]
                                        , info->bssid[3], info->bssid[4], info->bssid[5]);
    printf("SSID = %s\n", info->ssid);
    printf("Rx_packets =%lld\n", info->rx_packets);
    printf("Tx_packets =%lld\n", info->tx_packets);
    printf("Rx_unicast_packets =%lld\n", info->rx_unicast_packets);
    printf("Tx_unicast_packets =%lld\n", info->tx_unicast_packets);
    printf("Rx_multicast_packets =%lld\n", info->rx_multicast_packets);
    printf("Tx_multicast_packets =%lld\n", info->tx_multicast_packets);
    printf("Rx_broadcast_packets =%lld\n", info->rx_broadcast_packets);
    printf("Tx_broadcast_packets =%lld\n", info->tx_broadcast_packets);
    printf("Rx_non_unicast_packets =%lld\n", info->rx_non_unicast_packets);
    printf("Rx_bytes =%lld\n", info->rx_bytes);
    printf("Tx_bytes =%lld\n", info->tx_bytes);
    printf("Rx_dropped =%lld\n", info->rx_dropped);
    printf("Tx_dropped =%lld\n", info->tx_dropped);
    printf("Rx_error =%lld\n", info->rx_errors);
    printf("Tx_error =%lld\n", info->tx_errors);

    printf("Psk_Failures =%lld\n", info->psk_failures);
    printf("Integrity_Failures =%lld\n", info->integrity_failures);
    printf("Rx_unknown_packets =%lld\n", info->rx_unknown_packets);
    printf("Tx_mgmt_packets =%lld\n", info->tx_mgmt_packets);
    printf("Rx_mgmt_packets =%lld\n", info->rx_mgmt_packets);
    printf("Ack_failure_count =%d\n", info->ack_failure_count);
    printf("Aggregated_packet_count =%d\n", info->aggregated_packet_count);
    printf("Failed_retrans_count =%d\n", info->failed_retrans_count);
    printf("Multiple_retry_count =%d\n", info->multiple_retry_count);
    printf("Retry_count =%d\n", info->retry_count);
    printf("Retrans_count =%d\n", info->retrans_count);
    printf("Unknow_proto_packet =%lld\n", info->rx_unknown_packets);
    printf("Tx_throughput =%d\n", info->tx_throughput);
    printf("Rx_throughput =%d\n", info->rx_throughput);

    free(backup);
    backup = NULL;
    return 0;
}

/**
 * sf_vdr_get_radar_status - get radar status.
 *
 * @param hdr：Struct use in ioctl process.
 * @param chan: channel number.
 *
 * This function is used to get if channel detected radar.
 *
 * Return: 0 if success, non-zero otherwiss.
 */
static int sf_vdr_get_radar_status(struct sfcfghdr *hdr, char *chan)
{
    struct iwreq wrq;
    char *buff = NULL;
    int *channel_status;

    /* Fill tmp */
    memset(hdr, 0, sizeof(struct sfcfghdr));
    hdr->magic_no = SFCFG_MAGIC_IFACE;
    hdr->comand_type = SF_VDR_CMD_RADAR_STATUS;
    hdr->comand_id   = SF_VDR_CMD_RADAR_STATUS;
    hdr->length      = 52;
    hdr->sequence    = 1;

    sprintf(hdr->data, "%s", chan);

    /* Fill wrq */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)hdr;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr->length;
    /* send msg to driver */
    if (do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_RADAR_STATUS failed\n");
        return -1;
    }

    buff = malloc(wrq.u.data.length);
    if (!buff) {
        printf("memery alloc failed !\n");
        return -2;
    }

    /* Fill test info */
    memset(buff, 0, wrq.u.data.length);
    memcpy(buff, wrq.u.data.pointer, wrq.u.data.length);

    channel_status = (int *)buff;
    sscanf(buff, "%d", &channel_status);


    printf("\nchannel %s is %s\n\n", chan, *channel_status ? "available" : "unavailable");

    return 0;
}

/**
 * sf_vdr_set_radar_threshold - set radar detect sensitivity.
 *
 * @param hdr：Struct use in ioctl process.
 * @param param: param of sensitivity.
 *
 * This function is used to set param of radar detect sensitivity.
 *
 * Return: 0 if success, non-zero otherwiss.
 */
static int sf_vdr_set_radar_threshold(struct sfcfghdr *hdr, char *param)
{
    struct iwreq wrq;

    memset(hdr, 0, sizeof(struct sfcfghdr));
    hdr->magic_no = SFCFG_MAGIC_IFACE;
    hdr->comand_type = SF_VDR_CMD_RADAR_THRESHOLD;
    hdr->comand_id   = SF_VDR_CMD_RADAR_THRESHOLD;
    hdr->length      = 52;
    hdr->sequence    = 1;

    sprintf(hdr->data, "%s", param);

    /* Fill wrq */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)hdr;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr->length;
    /* send msg to driver */
    if (do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_RADAR_THRESHOLD failed\n");
        return -1;
    }

    return 0;
}

/**
 * sf_vdr_set_rts_threshold - set rts sensitivity.
 *
 * @param hdr：Struct use in ioctl process.
 * @param param: param of rts threshold.
 *
 * This function is used to set param of rts threshold.
 *
 * Return: 0 if success, non-zero otherwiss.
 */
static int sf_vdr_set_rts_threshold(struct sfcfghdr *hdr, char *param)
{
    struct iwreq wrq;

    memset(hdr, 0, sizeof(struct sfcfghdr));
    hdr->magic_no = SFCFG_MAGIC_IFACE;
    hdr->comand_type = SF_VDR_CMD_SET_RTS_THRESHOLD;
    hdr->comand_id   = SF_VDR_CMD_SET_RTS_THRESHOLD;
    hdr->length      = 52;
    hdr->sequence    = 1;

    sprintf(hdr->data, "%s", param);

    /* Fill wrq */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)hdr;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr->length;
    /* send msg to driver */
    if (do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_SET_RTS_THRESHOLD failed\n");
        return -1;
    }

    return 0;
}

/**
 * sf_vdr_set_radar_detect - radar detect trigger.
 *
 * @param hdr：Struct use in ioctl process.
 * @param param: param of trigger.
 *
 * This function is used to trigger the radar detect.
 *
 * Return: 0 if success, non-zero otherwiss.
 */
static int sf_vdr_set_radar_detect(struct sfcfghdr *hdr, char *param)
{
    struct iwreq wrq;

    memset(hdr, 0, sizeof(struct sfcfghdr));
    hdr->magic_no = SFCFG_MAGIC_IFACE;
    hdr->comand_type = SF_VDR_CMD_SET_RADAR_DETECT;
    hdr->comand_id   = SF_VDR_CMD_SET_RADAR_DETECT;
    hdr->length      = 52;
    hdr->sequence    = 1;

    sprintf(hdr->data, "%s", param);

    /* Fill wrq */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)hdr;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr->length;
    /* send msg to driver */
    if (do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_SET_RADAR_DETECT failed\n");
        return -1;
    }

    return 0;
}

/**
 * sf_vdr_get_radar_detect - radar detect trigger.
 *
 * @param hdr：Struct use in ioctl process.
 *
 * This function is used to trigger the radar detect.
 *
 * Return: 0 if success, non-zero otherwiss.
 */
static int sf_vdr_get_radar_detect(struct sfcfghdr *hdr)
{
    struct iwreq wrq;
    char *buff = NULL;
    int *status;

    memset(hdr, 0, sizeof(struct sfcfghdr));
    hdr->magic_no = SFCFG_MAGIC_IFACE;
    hdr->comand_type = SF_VDR_CMD_GET_RADAR_DETECT;
    hdr->comand_id   = SF_VDR_CMD_GET_RADAR_DETECT;
    hdr->length      = 52;
    hdr->sequence    = 1;

    /* Fill wrq */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)hdr;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr->length;
    /* send msg to driver */
    if (do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_GET_RADAR_DETECT failed\n");
        return -1;
    }

    buff = malloc(wrq.u.data.length);
    if (!buff) {
        printf("memery alloc failed !\n");
        return -2;
    }

    /* Fill test info */
    memset(buff, 0, wrq.u.data.length);
    memcpy(buff, wrq.u.data.pointer, wrq.u.data.length);

    status = (int *)buff;
    sscanf(buff, "%d", &status);

    printf("\nCurrent radar trigger: %d\n\n", *status);

    return 0;
}

/**
 * sf_vdr_set_srrc - set srrc.
 *
 * @param hdr：Struct use in ioctl process.
 *
 * This function is used to set the srrc flag.
 *
 * Return: 0 if success, non-zero otherwiss.
 */
static int sf_vdr_set_srrc(struct sfcfghdr *hdr, char *param)
{
    struct iwreq wrq;

    memset(hdr, 0, sizeof(struct sfcfghdr));
    hdr->magic_no = SFCFG_MAGIC_IFACE;
    hdr->comand_type = SF_VDR_CMD_SET_SRRC;
    hdr->comand_id   = SF_VDR_CMD_SET_SRRC;
    hdr->length      = 52;
    hdr->sequence    = 1;

    sprintf(hdr->data, "%s", param);

    /* Fill wrq */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)hdr;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr->length;
    /* send msg to driver */
    if (do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_SET_SRRC failed\n");
        return -1;
    }

    return 0;
}

static int sf_vdr_get_wds_stats(struct sfcfghdr *hdr)
{
    struct iwreq wrq;
    char *buff = NULL;
    char *backup = NULL;
    struct sf_vdr_wds_info *info = NULL;

    /* Fill tmp */
    memset(hdr, 0, sizeof(struct sfcfghdr));
    hdr->magic_no = SFCFG_MAGIC_IFACE;
    hdr->comand_type = SF_VDR_CMD_GET_WDS_STATS;
    hdr->comand_id   = SF_VDR_CMD_GET_WDS_STATS;
    hdr->length      = sizeof(struct sf_vdr_wds_info);
    hdr->sequence    = 1;

    /* Fill wrq */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)hdr;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr->length;

    /* send msg to driver */
    if (do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_GET_WDS_STATS failed\n");
        return -1;
    }

    buff = malloc(wrq.u.data.length);
    backup = buff;
    if (!buff) {
        printf("memery alloc failed !\n");
        return -2;
    }

    /* Fill test info */
    memset(buff, 0, wrq.u.data.length);
    memcpy(buff, wrq.u.data.pointer, wrq.u.data.length);

    /* Print */
    info = (struct sf_vdr_wds_info *)buff;

    info->last_data_downlink_rate = calculate_bitrate(&info->ap_info, IS_TX) * 1024;
    info->last_data_uplink_rate = calculate_bitrate(&info->ap_info, IS_RX) * 1024;

    sscanf(format_encryption(&info->crypto), "%[^()] (%[^)])", info->auth_mode, info->encryp_type);

    printf("SSID        : %s\n", info->ssid);
    printf("Channel     : %d\n", info->channel);
    printf("BSSID       : %02x:%02x:%02x:%02x:%02x:%02x\n",
                                          info->bssid[0], info->bssid[1], info->bssid[2]
                                        , info->bssid[3], info->bssid[4], info->bssid[5]);
    printf("StaMac      : %02x:%02x:%02x:%02x:%02x:%02x\n",
                                          info->sta_mac[0], info->sta_mac[1], info->sta_mac[2]
                                        , info->sta_mac[3], info->sta_mac[4], info->sta_mac[5]);

    printf("AuthMode    : %s\n", info->auth_mode);
    printf("EncrypType  : %s\n", info->encryp_type);
    printf("WlanStandard: %s\n", siwifi_get_format_mod(info->wireless_mode));

    printf("Signal      : %d\n", info->signal);
    printf("SNR         : %d\n", info->snr);
    printf("Noise       : %d\n", info->noise);

    printf("AssocRate   : %d.%dKbps\n", info->assoc_rate / 10, info->assoc_rate % 10);
    printf("DownlinkRate: %d.%dKbps\n", info->last_data_downlink_rate / 10, info->last_data_downlink_rate % 10);
    printf("UplinkRate  : %d.%dKbps\n", info->last_data_uplink_rate / 10, info->last_data_uplink_rate % 10);
    printf("TxKbps      : %d.%dKbps\n", info->txkbps / 10, info->txkbps % 10);
    printf("RxKbps      : %d.%dKbps\n", info->rxkbps / 10, info->rxkbps % 10);

    printf("TxError     : %d\n", info->tx_error_count);
    printf("RxError     : %d\n", info->rx_error_count);
    printf("TxCount     : %d\n", info->tx_count);
    printf("RxCount     : %d\n", info->rx_count);
    printf("BandWidth   : %d\n", info->band_width);
    printf("TxByte      : %d\n", info->tx_bytes);
    printf("RxByte      : %lld\n", info->rx_bytes);
    return 0;
}

/**
 * sf_vdr_set_vlan - Set vlan.
 *
 * @hdr: Struct use in ioctl process.
 * @operation: VLAN operation to perform, such as "add", "del", "passthrough", or "close".
 * @vlan_id: VLAN ID to be added or deleted. Can be NULL for certain operations.
 *
 * Return: 0 if successful, -1 on error.
 */
static int sf_vdr_set_vlan(struct sfcfghdr *hdr, char *operation, char *vlan_id)
{
    struct iwreq wrq;
    uint16_t length = 0;

    if (!strcmp(operation, "add") && !vlan_id) {
        printf("[%s]: vlan_id can't be NULL when add vlan!\n", __func__);
        return -1;
    }

    memset(hdr, 0, sizeof(struct sfcfghdr));
    hdr->magic_no = SFCFG_MAGIC_IFACE;
    hdr->comand_type = SF_VDR_CMD_SET_VLAN;
    hdr->comand_id   = SF_VDR_CMD_SET_VLAN;
    hdr->sequence    = 1;
    if (!strcmp(operation, "add"))
        length = sprintf(hdr->data, "%s,%s,%s", ifname, operation, vlan_id);
    else if (!strcmp(operation, "del"))
        length = sprintf(hdr->data, "%s,%s,%s", ifname, operation, vlan_id ? vlan_id : "0");
    else if (!strcmp(operation, "passthrough"))
        length = sprintf(hdr->data, "%s,%s", ifname, operation);
    else if (!strcmp(operation, "close"))
        length = sprintf(hdr->data, "%s,%s", ifname, operation);
    else {
        printf("[%s]: Invaild operation:%s, retry!\n", __func__, operation);
        return -1;
    }

    /* Fill wrq */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)hdr;
    hdr->length = length;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr->length;
    /* send msg to driver */
    if (do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_SET_VLAN failed\n");
        return -1;
    }

    return 0;
}

/**
 * sf_vdr_set_feat - Set 802.11ax feat.
 *
 * @hdr: Struct use in ioctl process.
 * @feat: Feat to be set(OFDMA/MU_MIMO).
 * @state: State to be set(on/off).
 *
 * Return: 0 if successful, -1 on error.
 */
static int sf_vdr_set_feat(struct sfcfghdr *hdr, char *feat, char *state)
{
    struct iwreq wrq;
    uint16_t length = 0;

    if (!(!strcmp(feat, "OFDMA") || !strcmp(feat, "MU_MIMO"))) {
        printf("[%s]: feat should be OFDMA or MU_MIMO!\n", __func__);
        return -1;
    }

    if (!(!strcmp(state, "on") || !strcmp(state, "off"))) {
        printf("[%s]: state should be on or off!\n", __func__);
        return -1;
    }

    memset(hdr, 0, sizeof(struct sfcfghdr));
    length = sprintf(hdr->data, "%s,%s", feat, state);
    hdr->magic_no = SFCFG_MAGIC_IFACE;
    hdr->comand_type = SF_VDR_CMD_SET_FEAT;
    hdr->comand_id   = SF_VDR_CMD_SET_FEAT;
    hdr->sequence    = 1;
    hdr->length = length;

    /* Fill wrq */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)hdr;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr->length;

    /* send msg to driver */
    if (do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_SET_FEAT failed\n");
        return -1;
    }

    return 0;
}

/**
 * sf_vdr_trigger_scan - Trigger scan.
 *
 * @param hdr：Struct use in ioctl process.
 * @param ifname: Interface name of the vap.
 * @param list: List of the scan channels.
 *
 * This function is used to scan channels.
 *
 * Return: 0 if success, non-zero otherwiss.
 */
static int sf_vdr_trigger_scan(struct sfcfghdr *hdr, char *ifname,
                               struct sf_vdr_scan_chan_list list)
{
    struct iwreq wrq;

    /* Fill hdr */
    memset(hdr, 0, sizeof(struct sfcfghdr));
    hdr->magic_no = SFCFG_MAGIC_IFACE;
    hdr->comand_type = SF_VDR_CMD_SCAN;
    hdr->comand_id   = SF_VDR_CMD_SCAN;
    hdr->length      = sizeof(struct sf_vdr_scan_chan_list);
    hdr->sequence    = 1;
    memcpy(&hdr->data, &list, sizeof(struct sf_vdr_scan_chan_list));

    /* Fill wrq */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)hdr;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr->length;

    /* send msg to driver */
    if (do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_SCAN failed\n");
        return -1;
    }

    return 0;
}

/**
 * sf_vdr_get_scan_result - Get the scan result.
 *
 * @param hdr：Struct use in ioctl process and save result.
 * @param ifname: Interface name of the vap.
 *
 * This function is used to get scan result.
 *
 * Return: 0 if success, non-zero otherwise, hdr->date save the struct siwifi_vdr_scan_result,
 * hdr->length is the data length.
 */
static int sf_vdr_get_scan_result(struct sfcfghdr *hdr, char *ifname)
{
    struct iwreq wrq;
    int i, channel, offset;
    struct siwifi_vdr_scan_result result;

    /* Fill hdr */
    memset(hdr, 0, sizeof(struct sfcfghdr));
    hdr->magic_no = SFCFG_MAGIC_IFACE;
    hdr->comand_type = SF_VDR_CMD_GET_SCAN_RESULT;
    hdr->comand_id   = SF_VDR_CMD_GET_SCAN_RESULT;
    hdr->length      = 1024;
    hdr->sequence    = 1;

    /* Fill wrq */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)hdr;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr->length;

    /* send msg to driver */
    if (do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_GET_SCAN_RESULT failed\n");
        return -1;
    }

    offset = sizeof(result.scan_done) + sizeof(result.result_number);
    memcpy(&result.scan_done, hdr->data, sizeof(result.scan_done));
    memcpy(&result.result_number, hdr->data + sizeof(result.scan_done), sizeof(result.result_number));

    if (!result.scan_done) {
        printf("%s: Scan is not finish.\n", __func__);
        return -1;
    }

    result.scan_result = (struct sf_vdr_scan_result *)malloc(result.result_number * sizeof(struct sf_vdr_scan_result));
    for (i = 0; i < result.result_number; i++) {
        memcpy(&result.scan_result[i], hdr->data + offset + i * sizeof(struct sf_vdr_scan_result), sizeof(struct sf_vdr_scan_result));
        channel = siwifi_freq_to_channel(result.scan_result[i].freq);
        printf("\nSSID = %s \n", result.scan_result[i].ssid);
        printf("\tmac = %02x:%02x:%02x:%02x:%02x:%02x\n", result.scan_result[i].bssid[0], result.scan_result[i].bssid[1],
                                                          result.scan_result[i].bssid[2], result.scan_result[i].bssid[3],
                                                          result.scan_result[i].bssid[4], result.scan_result[i].bssid[5]);
        printf("\tsignal = %d dBm\n", result.scan_result[i].rssi);
        printf("\tnoise = %d dBm\n", result.scan_result[i].noise);
        printf("\twork_mode = %s \n", result.scan_result[i].nettype);
        printf("\tencryption = %s \n", format_encryption(&result.scan_result[i].crypto));
        printf("\tband_width = %s \n", result.scan_result[i].bandwidth);
        printf("\tcen_freq = %d MHz\n", result.scan_result[i].freq);
        printf("\tbeacon_interval = %d TUs\n", result.scan_result[i].beacon_interval);
        printf("\tdtim_period = %d\n", result.scan_result[i].dtim_period);
        printf("\tnss = %d\n", result.scan_result[i].nss);
        printf("\tside_band = %d\n", result.scan_result[i].side_band);
        printf("\tchannel = %d\n", channel);
    }

    free(result.scan_result);
    return 0;
}

/**
 * sf_vdr_scan - Scan with default list.
 *
 * This function is used to scan channel with default list.
 *
 * Return: 0 if success, non-zero otherwise.
 */
static int sf_vdr_scan()
{
    struct sfcfghdr hdr;
    struct sf_vdr_scan_chan_list list = {
        .scan_chan_num = SCAN_CHANNEL_MAX,
        .scan_chan_freq_list = { 2412, 2417, 2422, 2427,
                                 2432, 2437, 2442, 2447,
                                 2452, 2457, 2462, 2467, 2472, 2484,
                                 5180, 5200, 5220, 5240,
                                 5260, 5280, 5300, 5320,
                                 5500, 5520, 5540, 5560,
                                 5580, 5600, 5620, 5640,
                                 5660, 5680, 5700, 5720,
                                 5745, 5765, 5785, 5805,
                                 5825, 5845, 5865, 5885 },
        .duration_mandatory = false,
        .duration = 0,
        .n_ssids = 0
    };
    memset(list.ssids, 0, sizeof(list.ssids));
    memset(&hdr, 0, sizeof(struct sfcfghdr));

    /**
     * duration_mandatory: true: use duration time for scan,
     *                     false: 110ms (passive scan), 10ms (active scan).
     * n_ssids: 0: passive scan, 1~SCAN_SSID_MAX: active scan.
     *
     * Example: the following shows to set atcive scan, duration 100 TUs,
     *          one SSID element in probe request is wildcard ssid,
     *          another SSID element in probe request is SiWiFi.
     *
     * list.duration_mandatory = true;
     * list.duration = 100;
     * list.n_ssids = 2;
     * list.ssids[0].ssid_len = 0;
     * memset(list.ssids[0].ssid, 0, MAX_SSID_LEN);
     * list.ssids[1].ssid_len = 6;
     * memcpy(list.ssids[1].ssid, "SiWiFi", 6);
     */

    return sf_vdr_trigger_scan(&hdr, ifname, list);
}

/**
 * sf_vdr_get_chan_score - Get the channel score after scan.
 *
 * @param hdr：Struct use in ioctl process and save result.
 * @param ifname: Interface name of the vap.
 * @param info: Information of channel.
 * @param auto_chan: True: wait driver scan finish,
 *                   Flase: not wait scan finish.
 *
 * This function is used to get channel score.
 *
 * Return: 0 if success, non-zero otherwise, hdr->date save the struct siwifi_vdr_chan_info,
 * hdr->length is the data length.
 */
static int sf_vdr_get_chan_score(struct sfcfghdr *hdr, char *ifname, struct siwifi_vdr_chan_info *info,
                                 bool auto_chan)
{
    struct iwreq wrq;
    int i, channel;

    /* Fill hdr */
    memset(hdr, 0, sizeof(struct sfcfghdr));
    hdr->magic_no = SFCFG_MAGIC_IFACE;
    hdr->comand_type = SF_VDR_CMD_GET_CHANNEL_SCORE;
    hdr->comand_id   = SF_VDR_CMD_GET_CHANNEL_SCORE;
    hdr->length      = 1024;
    hdr->sequence    = 1;
    hdr->data[0]     = auto_chan;

    /* Fill wrq */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)hdr;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr->length;

    /* send msg to driver */
    if (do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_GET_CHANNEL_SCORE failed\n");
        return -1;
    }

    /* Get channel score data */
    memcpy(info, hdr->data, hdr->length);

    /* Judge scan if finished */
    if (!info->scan_done) {
        printf("%s: Scan is not finish.\n", __func__);
        return -1;
    }

    /* Print channel information */
    for (i = 0; i < info->info_number; i++) {
        channel = siwifi_freq_to_channel(info->chan_info[i].center_freq);
        printf("\nChannel = %d \n", channel);
        printf("\tcenter_freq = %d MHz\n", info->chan_info[i].center_freq);
        printf("\tscore = %d\n", info->chan_info[i].score);
        printf("\tbss_num = %d\n", info->chan_info[i].bss_num);
        printf("\tchan_time_busy_ms = %d ms\n", info->chan_info[i].chan_time_busy_ms);
        printf("\tchan_time_ms = %d ms\n", info->chan_info[i].chan_time_ms);
        printf("\tnoise_dbm = %d dBm\n", info->chan_info[i].noise_dbm);
    }

    return 0;
}

/**
 * sf_vdr_set_chan - Set radio channel.
 *
 * @param hdr：Struct use in ioctl process and save result.
 * @param ifname: Interface name of the vap.
 * @param info: Information of channel set.
 *
 * This function is used to set channel.
 *
 * Return: 0 if success, non-zero otherwise, hdr->date save the struct sf_vdr_set_chan_info,
 * hdr->length is the data length.
 */
static int sf_vdr_set_chan(struct sfcfghdr *hdr, char *ifname, struct sf_vdr_set_chan_info *info)
{
    struct iwreq wrq;

    /* Fill hdr */
    memset(hdr, 0, sizeof(struct sfcfghdr));
    hdr->magic_no = SFCFG_MAGIC_IFACE;
    hdr->comand_type = SF_VDR_CMD_SET_CHAN;
    hdr->comand_id   = SF_VDR_CMD_SET_CHAN;
    hdr->length      = sizeof(struct sf_vdr_set_chan_info);
    hdr->sequence    = 1;
    memcpy(&hdr->data, info, sizeof(struct sf_vdr_set_chan_info));

    /* Fill wrq */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)hdr;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr->length;

    /* send msg to driver */
    if (do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_SET_CHAN failed\n");
        return -1;
    }

    /* Check set if success */
    memcpy(info, hdr->data, hdr->length);
    if (!info->set_done) {
        printf("%s: Set channel failed.\n", __func__);
        return -1;
    }

    return 0;
}

/**
 * sf_vdr_set_auto_chan - Select the best channel to set.
 *
 * This function is used to choose the best channel.
 *
 * Return: 0 if success, non-zero otherwise.
 */
static int sf_vdr_set_auto_chan()
{
    struct siwifi_vdr_chan_info info;
    struct sf_vdr_set_chan_info set_info;
    struct sfcfghdr hdr;
    int  ret, i;

    /* Scan */
    if ((ret = sf_vdr_scan())) {
        printf("%s: Scan failed.\n", __func__);
        return ret;
    }

    /* Get channel score */
    if ((ret = sf_vdr_get_chan_score(&hdr, ifname, &info, true))) {
        printf("%s: Get channel score failed.\n", __func__);
        return ret;
    }

    /* Fill channel set information */
    for (i = 0; i < info.info_number; i++) {
        if (info.chan_info[i].score == 100)
            break;
    }
    set_info.prim20_freq = info.chan_info[i].center_freq;
    set_info.center1_freq = set_info.prim20_freq;
    set_info.center2_freq = 0;
    set_info.width = NL80211_CHAN_WIDTH_20;

    /* Set channel */
    if ((ret = sf_vdr_set_chan(&hdr, ifname, &set_info))) {
        printf("%s: Set channel failed.\n", __func__);
        return ret;
    }

    return ret;
}

static int sf_vdr_country_code_info(char *con_code)
{
    struct iwreq wrq;
    struct sfcfghdr tmp;
    struct country_chan_info *con_info[32];
    char *buff = NULL, *backup = NULL;
    int i, num;

    /* Fill tmp */
    memset(&tmp, 0, sizeof(struct sfcfghdr));
    tmp.magic_no = SFCFG_MAGIC_IFACE;
    tmp.comand_type = SF_VDR_CMD_CON_CODE_INFO;
    tmp.comand_id   = SF_VDR_CMD_CON_CODE_INFO;
    tmp.length      = 52;
    tmp.sequence    = 1;
    memcpy(tmp.data, con_code, 20);

    /* Fill wrq */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&tmp;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

    /* send msg to driver */
    if (do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_CON_CODE_INFO failed\n");
        return -1;
    }

    /* Malloc memery space */
    buff = malloc(wrq.u.data.length);
    backup = buff;
    if (!buff) {
        printf("memery alloc failed !\n");
        return -2;
    }

    num = wrq.u.data.length / sizeof(struct country_chan_info);

    memset(buff, 0, wrq.u.data.length);
    memcpy(buff, wrq.u.data.pointer, wrq.u.data.length);

    for (i = 0; i < num; i++) {
        con_info[i] = (struct country_chan_info *)buff;
        printf("\t\t* %d MHz  @ %d (%d dBm)", con_info[i]->center_freq, con_info[i]->max_bw, con_info[i]->max_dBm);
        if (con_info[i]->dfs)
            printf(" (radar detection)");
        printf("\n");
        buff += sizeof(struct country_chan_info);
    }

    free(backup);
    backup = NULL;
    return 0;
}

static int sf_vdr_chan_info()
{
    struct iwreq wrq;
    struct sfcfghdr tmp;
    char *buff = NULL;
    char *backup = NULL;
    struct siwifi_chan_info *chinfo[48];
    int chan_num, i, channel;

    /* Fill tmp */
    memset(&tmp, 0, sizeof(struct sfcfghdr));
    tmp.magic_no = SFCFG_MAGIC_IFACE;
    tmp.comand_type = SF_VDR_CMD_CHAN_INFO;
    tmp.comand_id   = SF_VDR_CMD_CHAN_INFO;
    tmp.length      = 52;
    tmp.sequence    = 1;

    /* Fill wrq */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&tmp;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

    /* send msg to driver */
    if(do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_CHAN_INFO failed\n");
        return -1;
    }

    buff = malloc(wrq.u.data.length);
    backup = buff;
    if (!buff) {
        printf("memery alloc failed !\n");
        return -2;
    }
    chan_num = wrq.u.data.length / sizeof(struct siwifi_chan_info);

    /* Fill test info */
    memset(buff, 0, wrq.u.data.length);
    memcpy(buff, wrq.u.data.pointer, wrq.u.data.length);
    for (i = 0; i < chan_num; i++) {
        chinfo[i] = (struct siwifi_chan_info *)buff;
        channel = siwifi_freq_to_channel(chinfo[i]->freq);
        printf("channel: %d", channel);
        if (chinfo[i]->bandwidth != 0) {
            printf(" *[in used]\n");
            printf("\tbandwidth: %s", sf_channel_width_name(chinfo[i]->bandwidth));
            switch (chinfo[i]->side_flag) {
            case 1:
                printf("\tside_flag: LOWER\n");
                break;
            case 2:
                printf("\tside_flag: UPPER\n");
                break;
            default:
                printf("\tside_flag: NONE\n");
                break;
            }
        } else {
            printf("\n");
        }
        buff += sizeof(struct siwifi_chan_info);
    }

    free(backup);
    backup = NULL;
    return 0;
}

static int sf_vdr_get_repeater_status()
{
    struct iwreq wrq;
    struct sfcfghdr tmp;

    memset(&tmp, 0, sizeof(struct sfcfghdr));
    tmp.magic_no = SFCFG_MAGIC_IFACE;
    tmp.comand_type = SF_VDR_CMD_REPEATER_STATE;
    tmp.comand_id   = SF_VDR_CMD_REPEATER_STATE;
    tmp.length      = 52;
    tmp.sequence    = 1;

    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&tmp;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

    if(do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_REPEATER_STATE failed\n");
        return -1;
    }
    return 0;
}

static int sf_vdr_set_retry_num(char *type, char *num)
{
    struct iwreq wrq;
    struct sfcfghdr tmp;

    memset(&tmp, 0, sizeof(struct sfcfghdr));
    tmp.magic_no = SFCFG_MAGIC_IFACE;
    tmp.comand_type = SF_VDR_SET_RETRY_NUM;
    tmp.comand_id   = SF_VDR_SET_RETRY_NUM;
    tmp.length      = 52;
    tmp.sequence    = 1;
    memcpy(tmp.data, type, 10);
    memcpy(&tmp.data[10], num, 10);

    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&tmp;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

    if (do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_SET_RETRY_NUM failed\n");
        return -1;
    }

    return 0;
}

static int sf_vdr_set_cca(char *pd, char *val)
{
    struct iwreq wrq;
    struct sfcfghdr tmp;

    memset(&tmp, 0, sizeof(struct sfcfghdr));
    tmp.magic_no = SFCFG_MAGIC_IFACE;
    tmp.comand_type = SF_VDR_CMD_SET_CCA;
    tmp.comand_id   = SF_VDR_CMD_SET_CCA;
    tmp.length      = 52;
    tmp.sequence    = 1;
    memcpy(tmp.data, pd, 10);
    memcpy(&tmp.data[10], val, 10);

    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&tmp;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

    if (do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_SET_CCA failed\n");
        return -1;
    }
    return 0;
}

static int sf_vdr_set_agg_threshold(char *type, char *val)
{
    struct iwreq wrq;
    struct sfcfghdr tmp;

    memset(&tmp, 0, sizeof(struct sfcfghdr));
    tmp.magic_no = SFCFG_MAGIC_IFACE;
    tmp.comand_type = SF_VDR_CMD_SET_AGG_THRES;
    tmp.comand_id   = SF_VDR_CMD_SET_AGG_THRES;
    tmp.length      = 52;
    tmp.sequence    = 1;
    memcpy(tmp.data, type, 10);
    memcpy(&tmp.data[10], val, 10);

    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&tmp;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

    if (do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_SET_AGG_THRES failed\n");
        return -1;
    }
    return 0;
}

static int sf_vdr_set_edca(char *set_data)
{
    struct sf_vdr_common_data *test_info;
    struct iwreq wrq;
    struct sfcfghdr tmp;

    memset(&tmp, 0, sizeof(struct sfcfghdr));
    tmp.magic_no = SFCFG_MAGIC_IFACE;
    tmp.comand_type = SF_VDR_CMD_EDCA;
    tmp.comand_id   = SF_VDR_CMD_EDCA;
    tmp.length      = 52;
    tmp.sequence    = 1;

    memcpy(tmp.data, set_data, 16);
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&tmp;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

    if (do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_REPEATER_STATE failed\n");
        return -1;
    }

    /* Malloc memery space */
    test_info = (struct sf_vdr_common_data *)malloc(sizeof(struct sf_vdr_common_data));
    if (!test_info) {
        printf("memery alloc failed !\n");
        return -2;
    }

    /* Fill test info */
    memset(test_info, 0, sizeof(struct sf_vdr_common_data));
    memcpy(test_info, wrq.u.data.pointer, sizeof(struct sf_vdr_common_data));

    /* Print test info */
    printf("\t str:%s \n", test_info->str);
    free(test_info);

    return 0;
}

static int sf_vdr_get_frame_num()
{
    struct iwreq wrq;
    struct sfcfghdr tmp;

    memset(&tmp, 0, sizeof(struct sfcfghdr));
    tmp.magic_no = SFCFG_MAGIC_IFACE;
    tmp.comand_type = SF_VDR_CMD_GET_FRAME;
    tmp.comand_id   = SF_VDR_CMD_GET_FRAME;
    tmp.length      = 52;
    tmp.sequence    = 1;

    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&tmp;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

    if (do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_GET_FRAME failed\n");
        return -1;
    }
    return 0;
}

static int sf_vdr_set_acs(char *set_data1, char *set_data2, char *set_data3)
{
    struct sf_vdr_common_data *test_info;
    struct iwreq wrq;
    struct sfcfghdr tmp;
    int i = 0;

    memset(&tmp, 0, sizeof(struct sfcfghdr));
    tmp.magic_no = SFCFG_MAGIC_IFACE;
    tmp.comand_type = SF_VDR_CMD_ACS;
    tmp.comand_id   = SF_VDR_CMD_ACS;
    tmp.length      = 52;
    tmp.sequence    = 1;

    if ((strlen(set_data3) > 39) || (strlen(set_data3) % 3 != 0) || (strlen(set_data2) != 6) || (strlen(set_data1) != 1)) {
        printf("params is error, please try again!\n");
        return -1;
    }

    sprintf(tmp.data, "%s%s%s", set_data1, set_data2, set_data3);

    for (i = 0; i < (strlen(set_data3) + 7); i++) {
        if (tmp.data[i] >= '0' && tmp.data[i] <= '9') {
            continue;
        } else {
            printf("params is error, please try again!\n");
            return -1;
        }
    }

    printf("tmp.data = %s \n",tmp.data);
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&tmp;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

    if(do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_ACS failed\n");
        return -1;
    }

    /* Malloc memery space */
    test_info = (struct sf_vdr_common_data *)malloc(sizeof(struct sf_vdr_common_data));
    if (!test_info) {
        printf("memery alloc failed !\n");
        return -2;
    }

    /* Fill test info */
    memset(test_info, 0, sizeof(struct sf_vdr_common_data));
    memcpy(test_info, wrq.u.data.pointer, sizeof(struct sf_vdr_common_data));

    /* Print test info */
    printf("\t str:%s \n", test_info->str);
    free(test_info);

    return 0;
}

static int sf_vdr_set_dfs(char *set_data1)
{
    struct sf_vdr_common_data *test_info;
    struct iwreq wrq;
    struct sfcfghdr tmp;
    int i = 0;

    memset(&tmp, 0, sizeof(struct sfcfghdr));
    tmp.magic_no = SFCFG_MAGIC_IFACE;
    tmp.comand_type = SF_VDR_CMD_DFS;
    tmp.comand_id   = SF_VDR_CMD_DFS;
    tmp.length      = 52;
    tmp.sequence    = 1;

    if ((strlen(set_data1) != 2)) {
        printf("params is error, please try again!\n");
        return -1;
    }

    sprintf(tmp.data, "%s", set_data1);

    for (i = 0; i < (strlen(set_data1)); i++) {
        if (tmp.data[i] >= '0' && tmp.data[i] <= '1') {
            continue;
        } else {
            printf("params is error, please try again!\n");
            return -1;
        }
    }

    printf("tmp.data = %s \n",tmp.data);
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&tmp;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

    if(do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_DFS failed\n");
        return -1;
    }

    /* Malloc memery space */
    test_info = (struct sf_vdr_common_data *)malloc(sizeof(struct sf_vdr_common_data));
    if (!test_info) {
        printf("memery alloc failed !\n");
        return -2;
    }

    /* Fill test info */
    memset(test_info, 0, sizeof(struct sf_vdr_common_data));
    memcpy(test_info, wrq.u.data.pointer, sizeof(struct sf_vdr_common_data));

    /* Print test info */
    printf("\t str:%s \n", test_info->str);
    free(test_info);

    return 0;
}

static int sf_vdr_set_apsd(char *set_data1)
{
    struct sf_vdr_common_data *test_info;
    struct iwreq wrq;
    struct sfcfghdr tmp;
    int i = 0;

    memset(&tmp, 0, sizeof(struct sfcfghdr));
    tmp.magic_no = SFCFG_MAGIC_IFACE;
    tmp.comand_type = SF_VDR_CMD_APSD;
    tmp.comand_id   = SF_VDR_CMD_APSD;
    tmp.length      = 52;
    tmp.sequence    = 1;

    if (strlen(set_data1) != 4) {
        printf("params is error, please try again!\n");
        return -1;
    }

    sprintf(tmp.data, "%s", set_data1);
    for (i = 0; i < (strlen(set_data1)); i++) {
        if (tmp.data[i] >= '0' && tmp.data[i] <= '9') {
            continue;
        } else {
            printf("params is error, please try again!\n");
            return -1;
        }
    }

    printf("tmp.data = %s \n",tmp.data);
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&tmp;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

    if(do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_APSD failed\n");
        return -1;
    }

    /* Malloc memery space */
    test_info = (struct sf_vdr_common_data *)malloc(sizeof(struct sf_vdr_common_data));
    if (!test_info) {
        printf("memery alloc failed !\n");
        return -2;
    }

    /* Fill test info */
    memset(test_info, 0, sizeof(struct sf_vdr_common_data));
    memcpy(test_info, wrq.u.data.pointer, sizeof(struct sf_vdr_common_data));

    /* Print test info */
    printf("\t str:%s \n", test_info->str);
    free(test_info);

    return 0;
}

static int sf_vdr_send_debug_frame(char *vif_idx, char *ac, char *bw, char *mcs,
                                    char *nss, char *format_mod, char *gi, char *he_ltf,
                                    char *ldpc, char *expect_ack, char *file_path)
{
    struct sf_vdr_common_data *test_info;
    struct iwreq wrq;
    struct sfcfghdr tmp;
    int i = 0;
    int date_len = strlen(vif_idx) + strlen(ac) + strlen(bw) +  strlen(mcs) +  strlen(nss) +
                    strlen(format_mod) + strlen(gi) + strlen(he_ltf) + strlen(ldpc) + strlen(expect_ack);

    memset(&tmp, 0, sizeof(struct sfcfghdr));
    tmp.magic_no = SFCFG_MAGIC_IFACE;
    tmp.comand_type = SF_VDR_CMD_SEND_DEBUG_FRAME;
    tmp.comand_id   = SF_VDR_CMD_SEND_DEBUG_FRAME;
    tmp.length      = 52;
    tmp.sequence    = 1;

    sprintf(tmp.data, "%s%s%s%s%s%s%s%s%s%s%s", vif_idx, ac, bw, mcs,
            nss, format_mod, gi, he_ltf, ldpc, expect_ack, file_path);

    for (i = 0; i < date_len; i++) {
        if (tmp.data[i] >= '0' && tmp.data[i] <= '9') {
            continue;
        } else {
            printf("params is error, please try again!\n");
            return -1;
        }
    }

    printf("tmp.data = %s \n",tmp.data);
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&tmp;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

    if(do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_APSD failed\n");
        return -1;
    }

    /* Malloc memery space */
    test_info = (struct sf_vdr_common_data *)malloc(sizeof(struct sf_vdr_common_data));
    if (!test_info) {
        printf("memery alloc failed !\n");
        return -2;
    }

    /* Fill test info */
    memset(test_info, 0, sizeof(struct sf_vdr_common_data));
    memcpy(test_info, wrq.u.data.pointer, sizeof(struct sf_vdr_common_data));

    /* Print test info */
    printf("\t str:%s \n", test_info->str);
    free(test_info);

    return 0;
}

static int sf_vdr_set_power(char *sign, char *value)
{
    struct sf_vdr_common_data *test_info;
    struct iwreq wrq;
    struct sfcfghdr tmp;
    int i = 0;

    memset(&tmp, 0, sizeof(struct sfcfghdr));
    tmp.magic_no = SFCFG_MAGIC_IFACE;
    tmp.comand_type = SF_VDR_CMD_SET_POWER;
    tmp.comand_id   = SF_VDR_CMD_SET_POWER;
    tmp.length      = 52;
    tmp.sequence    = 1;

    sprintf(tmp.data, "%s%s", sign, value);

    if(strcmp(sign, "inc") != 0 && strcmp(sign, "dec") != 0) {
        printf("params sign is error, please try again!\n");
        return -1;
    }

    for (i = 0; i < strlen(value); i++) {
        if (tmp.data[i + 3] >= '0' && tmp.data[i + 3] <= '9') {
            continue;
        } else {
            printf("params value is error, please try again!\n");
            return -1;
        }
    }

    printf("tmp.data = %s \n",tmp.data);
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&tmp;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

    if(do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_APSD failed\n");
        return -1;
    }

    /* Malloc memery space */
    test_info = (struct sf_vdr_common_data *)malloc(sizeof(struct sf_vdr_common_data));
    if (!test_info) {
        printf("memery alloc failed !\n");
        return -2;
    }

    /* Fill test info */
    memset(test_info, 0, sizeof(struct sf_vdr_common_data));
    memcpy(test_info, wrq.u.data.pointer, sizeof(struct sf_vdr_common_data));

    /* Print test info */
    printf("\t str:%s \n", test_info->str);
    free(test_info);

    return 0;
}

static int sf_vdr_set_cali_table(char *table_id){
    struct sf_vdr_common_data *test_info;
    struct iwreq wrq;
    struct sfcfghdr tmp;

    memset(&tmp, 0, sizeof(struct sfcfghdr));
    tmp.magic_no = SFCFG_MAGIC_IFACE;
    tmp.comand_type = SF_VDR_CMD_SET_CALI_TABLE;
    tmp.comand_id   = SF_VDR_CMD_SET_CALI_TABLE;
    tmp.length      = 52;
    tmp.sequence    = 1;

    sprintf(tmp.data, "%s", table_id);

    if(strlen(table_id) != 1 || tmp.data[0] < '0' || tmp.data[0] > '2') {
        printf("params table_id(0/1/2) is error, please try again!\n");
        return -1;
    }

    printf("tmp.data = %s \n",tmp.data);
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&tmp;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

    if(do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_APSD failed\n");
        return -1;
    }

    /* Malloc memery space */
    test_info = (struct sf_vdr_common_data *)malloc(sizeof(struct sf_vdr_common_data));
    if (!test_info) {
        printf("memery alloc failed !\n");
        return -2;
    }

    /* Fill test info */
    memset(test_info, 0, sizeof(struct sf_vdr_common_data));
    memcpy(test_info, wrq.u.data.pointer, sizeof(struct sf_vdr_common_data));

    /* Print test info */
    printf("\t str:%s \n", test_info->str);
    free(test_info);

    return 0;
}

static int sf_vdr_set_ant(char *hb, char *lb, char *dir){
    struct sf_vdr_common_data *test_info;
    struct iwreq wrq;
    struct sfcfghdr tmp;

    memset(&tmp, 0, sizeof(struct sfcfghdr));
    tmp.magic_no = SFCFG_MAGIC_IFACE;
    tmp.comand_type = SF_VDR_CMD_SET_ANT;
    tmp.comand_id   = SF_VDR_CMD_SET_ANT;
    tmp.length      = 52;
    tmp.sequence    = 1;

    if(strcmp(hb, "hb") == 0) {
        hb = "hb ";
    }
    else if(strcmp(hb, "hb1") != 0 && strcmp(hb, "hb2") != 0) {
        printf("params hb(hb/hb1/hb2) is error, please try again!\n");
        return -1;
    }

    if(strcmp(lb, "lb") == 0) {
        lb = "lb ";
    }
    else if(strcmp(lb, "lb1") != 0 && strcmp(lb, "lb2") != 0) {
        printf("params lb(lb/lb1/lb2) is error, please try again!\n");
        return -1;
    }

    if(strcmp(dir, "tx") == 0) {
        dir = "tx ";
    }
    else if(strcmp(dir, "rx") == 0) {
        dir = "rx ";
    }
    else if(strcmp(dir, "all") != 0 && strcmp(dir, "no") != 0) {
        printf("params dir(tx/rx/all/no) is error, please try again!\n");
        return -1;
    }

    sprintf(tmp.data, "%3s%3s%3s", hb, lb, dir);

    printf("tmp.data = %s \n",tmp.data);
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&tmp;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

    if(do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_APSD failed\n");
        return -1;
    }

    /* Malloc memery space */
    test_info = (struct sf_vdr_common_data *)malloc(sizeof(struct sf_vdr_common_data));
    if (!test_info) {
        printf("memery alloc failed !\n");
        return -2;
    }

    /* Fill test info */
    memset(test_info, 0, sizeof(struct sf_vdr_common_data));
    memcpy(test_info, wrq.u.data.pointer, sizeof(struct sf_vdr_common_data));

    /* Print test info */
    printf("\t str:%s \n", test_info->str);
    free(test_info);

    return 0;
}

static int sf_vdr_set_thermal_on(char *tempcomp_en){
    struct sf_vdr_common_data *test_info;
    struct iwreq wrq;
    struct sfcfghdr tmp;

    memset(&tmp, 0, sizeof(struct sfcfghdr));
    tmp.magic_no = SFCFG_MAGIC_IFACE;
    tmp.comand_type = SF_VDR_CMD_SET_THERMAL_ON;
    tmp.comand_id   = SF_VDR_CMD_SET_THERMAL_ON;
    tmp.length      = 52;
    tmp.sequence    = 1;

    sprintf(tmp.data, "%s", tempcomp_en);

    if(strlen(tempcomp_en) != 1 || (tempcomp_en[0] != '0' && tempcomp_en[0] != '1')) {
        printf("params tempcomp_en(0/1) is error, please try again!\n");
        return -1;
    }

    printf("tmp.data = %s \n",tmp.data);
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&tmp;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

    if(do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_APSD failed\n");
        return -1;
    }

    /* Malloc memery space */
    test_info = (struct sf_vdr_common_data *)malloc(sizeof(struct sf_vdr_common_data));
    if (!test_info) {
        printf("memery alloc failed !\n");
        return -2;
    }

    /* Fill test info */
    memset(test_info, 0, sizeof(struct sf_vdr_common_data));
    memcpy(test_info, wrq.u.data.pointer, sizeof(struct sf_vdr_common_data));

    /* Print test info */
    printf("\t str:%s \n", test_info->str);
    free(test_info);

    return 0;
}

static int siwifi_vdr_get_rf(struct sfcfghdr *hdr, char *ifname)
{
    struct iwreq wrq;
    struct sf_vdr_phy_info *phy_info = NULL;
    u32 rxcount;

    memset(hdr, 0, sizeof(struct sfcfghdr));
    hdr->magic_no = SFCFG_MAGIC_IFACE;
    hdr->comand_type = SF_VDR_CMD_GET_RF;
    hdr->comand_id   = SF_VDR_CMD_GET_RF;
    hdr->length      = 52;
    hdr->sequence    = 1;

    sprintf(hdr->data, "%s", ifname);

    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)hdr;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr->length;

    if(do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_GET_RF failed\n");
        return -1;
    }
    phy_info = (struct sf_vdr_phy_info *)malloc(sizeof(struct sf_vdr_phy_info));
    if (!phy_info) {
        printf("memery alloc failed !\n");
        return -2;
    }
    memset(phy_info, 0, sizeof(struct sf_vdr_phy_info));
    memcpy(phy_info, wrq.u.data.pointer, sizeof(struct sf_vdr_phy_info));

    printf(" channel     = %d \n", phy_info->channel);
    printf(" center_freq = %d\n",phy_info->center_freq);
    switch (phy_info->width) {
    case NL80211_CHAN_WIDTH_20:
        printf(" band_width  : 20MHz\n");
        break;
    case NL80211_CHAN_WIDTH_40:
        printf(" band_width  : 40MHz\n");
        break;
    case NL80211_CHAN_WIDTH_80:
        printf(" band_width  : 80MHz\n");
        break;
    case NL80211_CHAN_WIDTH_160:
        printf(" band_width  : 160MHz\n");
        break;
    default:
        printf(" Unsupported width [%d]\n",phy_info->width);
        break;
    }
    switch (phy_info->mode) {
    case SF_PHY_MODE_11N:
        printf(" mode        : 80211N\n");
        break;
    case SF_PHY_MODE_11AC:
        printf(" mode        : 80211AC\n");
        break;
    case SF_PHY_MODE_11AX:
        printf(" mode        : 80211AX\n");
        break;
    default:
        printf(" mode        : UNSUPPORT MODE\n");
        break;
    }
    rxcount = phy_info->rx_sucess + phy_info->rx_dropped;
    printf(" Tx_success       : %d\n", phy_info->tx_sucess);
    printf(" Tx_fail          : %d\n", phy_info->tx_fail);
    printf(" Rx_success       : %d\n", phy_info->rx_sucess);
    printf(" Rx_CRC           : %lld\n", phy_info->rx_crc);
    printf(" False_CCA        : %lld\n", phy_info->false_cca);
    printf(" Beacon_count     : %lld\n", phy_info->beacon_count);
    printf(" Tx_Retry         : %d\n", phy_info->tx_retry);
    printf(" PLCP_Error_Count : %d\n", phy_info->PLCP_error_count);
    printf(" FCS_Error_Count  : %d\n", phy_info->FCS_error_count);
    printf(" Txper            : %d%%\n", phy_info->tx_packets ?
                                        (phy_info->tx_fail * 100 / phy_info->tx_packets) : 0);
    printf(" Rxper            : %d%%\n", rxcount ?
                                        (phy_info->rx_dropped * 100 / rxcount) : 0);

    printf(" Noise            : %d\n", phy_info->noise);
    printf(" InterferenceLevel: %.2f%%\n", phy_info->chan_time ?
                                         (double)(phy_info->cca_busy * 100) / phy_info->chan_time : 0);
    printf(" ChanUtil         : %.2f%%\n", phy_info->swcfgtime ?
                                         (double)((phy_info->obss_chlutil + phy_info->bss_chlutil) * 100) / phy_info->swcfgtime : 0);
    printf(" OBSSChlUtil      : %.2f%%\n", phy_info->swcfgtime ?
                                         (double)(phy_info->obss_chlutil * 100) / phy_info->swcfgtime : 0);
    printf(" CoChlOBSSUtil    : %.2f%%\n", phy_info->swcfgtime ?
                                         (double)(phy_info->bss_chlutil * 100) / phy_info->swcfgtime : 0);
    printf(" RxInradioUtil    : %.2f%%\n", phy_info->chan_time ?
                                         (double)(phy_info->rx_frame_time * 100) / phy_info->chan_time : 0);
    printf(" TxInradioUtil    : %.2f%%\n", phy_info->chan_time ?
                                         (double)(phy_info->tx_frame_time * 100) / phy_info->chan_time : 0);
    printf(" NonWifiInterfUtil: %.2f%%\n", phy_info->swcfgtime ?
                                         (double)(phy_info->nonwifi_intf_util * 100) / phy_info->swcfgtime : 0);
    printf(" iSNR             : %d\n", phy_info->isnr);
    printf(" Temperature      : %d\n", phy_info->temperature);
    printf(" Interrupt        : %lld\n", phy_info->interrupt);
    printf(" ProbeRespone     : %lld\n", phy_info->probe_respone);
    printf(" ProbeRequest     : %lld\n", phy_info->probe_request);
    free(phy_info);
    return 0;
}

static int siwifi_vdr_set_power_limit(struct sfcfghdr *hdr, char *limit_pwr, char *limit_pwr_lvl)
{
    struct iwreq wrq;
    int limit_pwr_tmp;
    int limit_pwr_lvl_tmp;
    int adjust_txpower_level;

    limit_pwr_tmp = atoi(limit_pwr);
    limit_pwr_lvl_tmp = atoi(limit_pwr_lvl);

    if (limit_pwr_tmp < -1 || limit_pwr_tmp > 26) {
        printf("limit_pwr:%d is out of range [-1, 26].\n", limit_pwr_tmp);
        return -1;
    }

    if (limit_pwr_lvl_tmp <= 0 || limit_pwr_lvl_tmp > 100) {
        printf("limit_pwr_lvl:%d is out of range (0, 100].\n", limit_pwr_lvl_tmp);
        return -1;
    }

    memset(hdr, 0, sizeof(struct sfcfghdr));
    hdr->magic_no = SFCFG_MAGIC_IFACE;
    hdr->comand_type = SF_VDR_CMD_SET_POWER_LIMIT;
    hdr->comand_id   = SF_VDR_CMD_SET_POWER_LIMIT;
    hdr->length      = 30;
    hdr->sequence    = 1;

    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)hdr;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr->length;

    adjust_txpower_level = (int)round(10 * log10(100 / (double)limit_pwr_lvl_tmp) * 4);
    sprintf(hdr->data, "%d_%d_%d", limit_pwr_tmp, limit_pwr_lvl_tmp, adjust_txpower_level);
    if (do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_SET_POWER_LIMIT failed\n");
        return -1;
    }

    return 0;
}


static int sf_vdr_mon_sta(char *mon_num, char *mon_mac_addr, char* mon_sta_enable) {
    struct sf_vdr_common_data *test_info;
    struct iwreq wrq;
    struct sfcfghdr tmp;

    memset(&tmp, 0, sizeof(struct sfcfghdr));
    tmp.magic_no = SFCFG_MAGIC_IFACE;
    tmp.comand_type = SF_VDR_CMD_MON_STA;
    tmp.comand_id   = SF_VDR_CMD_MON_STA;
    tmp.length      = 195;
    tmp.sequence    = 1;

    sprintf(tmp.data, "%s %s %s",mon_num, mon_mac_addr, mon_sta_enable);

    if(!strlen(mon_num)) {
        printf("params mon_mac_addr is error, please try again!\n");
        return -1;
    }

    if(!strlen(mon_mac_addr)) {
        printf("params mon_mac_addr is error, please try again!\n");
        return -1;
    }

    if(!strlen(mon_sta_enable)) {
        printf("params mon_sta_enable is error, please try again!\n");
        return -1;
    }

    printf("tmp.data = %s \n",tmp.data);
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&tmp;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

    if(do_ioctl(SFCFG_PRIV_IOCTL_IFACE, &wrq) < 0) {
        printf("SFCFG_PRIV_IOCTL_IFACE ioctl SF_VDR_CMD_MON_STA failed\n");
        return -1;
    }

    /* Malloc memery space */
    test_info = (struct sf_vdr_common_data *)malloc(sizeof(struct sf_vdr_common_data));
    if (!test_info) {
        printf("memery alloc failed !\n");
        return -2;
    }

    /* Fill test info */
    memset(test_info, 0, sizeof(struct sf_vdr_common_data));
    memcpy(test_info, wrq.u.data.pointer, sizeof(struct sf_vdr_common_data));

    /* Print test info */
    printf("\t str:%s \n", test_info->str);
    free(test_info);

    return 0;
}

/* THe usage of siwifi_iface */
static void usage()
{
    printf("\nUsage:\n"
        "\tsfwifi_iface <ifname> test_mgmt_ie test_str\n"
        "\tsfwifi_iface <ifname> bwlist operation list_type mac_addres\n"
        "\tsfwifi_iface <ifname> sta_info\n"
        "\tsfwifi_iface <ifname> scan\n"
        "\tsfwifi_iface <ifname> get_scan_result\n"
        "\tsfwifi_iface <ifname> get_chan_score\n"
        "\tsfwifi_iface <ifname> set_auto_chan\n"
        "\tsfwifi_iface <ifname> con_code country_str\n"
        "\tsfwifi_iface <ifname> chan_info\n"
        "\tsfwifi_iface <ifname> repeater_status\n"
        "\tsfwifi_iface <ifname> set_retry mgmt/ctrl/data set_data(007004)\n"
        "\tsfwifi_iface <ifname> set_cca rise/fall/off val\n"
        "\tsfwifi_iface <ifname> set_ampdu/set_amsdu/amsdu_th set_data\n"
        "\tsfwifi_iface <ifname> set_edca set_data\n"
        "\tsfwifi_iface <ifname> get_frame_num\n"
        "\tsfwifi_iface <ifname> set_acs set_data0 set_data1 set_data2\n"
        "\tsfwifi_iface <ifname> set_dfs set_dfs\n"
        "\tsfwifi_iface <ifname> set_apsd param\n"
        "\tsfwifi_iface <ifname> send_debug_frame vif_idx ac bw mcs nss format_mod gi he_ltf ldpc expect_ack file_path\n"
        "\tsfwifi_iface <ifname> set_power inc/dec value\n"
        "\tsfwifi_iface <ifname> set_cali_table table_id\n"
        "\tsfwifi_iface <ifname> set_ant hb lb dir\n"
        "\tsfwifi_iface <ifname> set_thermal_on tempcomp_en(0/1)\n"
        "\tsfwifi_iface <ifname> get_vap_stats\n"
        "\tsfwifi_iface <ifname> get_rf\n"
        "\tsfwifi_iface <ifname> set_power_limit limit_pwr limit_pwr_lvl\n"
        "\tsfwifi_iface <ifname> mon_sta mon_num mon_mac_addr mon_sta_enable\n"
        "\tsfwifi_iface <ifname> radar_status chan_num\n"
        "\tsfwifi_iface <ifname> radar_threshold param[0x00~0xff]\n"
        "\tsfwifi_iface <ifname> set_rts_threshold param[1~2347]\n"
        "\tsfwifi_iface <ifname> set_radar_detect (0/1)\n"
        "\tsfwifi_iface <ifname> get_radar_detect\n"
        "\tsfwifi_iface <ifname> set_srrc (0/1)\n"
        "\tsfwifi_iface <ifname> get_wds_stats\n"
        "\tsfwifi_iface <ifname> set_vlan operation vlan_id\n"
        "\tsfwifi_iface <ifname> set_feat <OFDMA/MU_MIMO> <on/off>\n\n"
    );
}

extern unsigned int if_nametoindex(const char *ifname);
int32_t main(int32_t argc, char *argv[])
{
    struct timeval start;
    struct timeval end;
    struct sfcfghdr hdr = { 0 };
    float use_time;

    /* get cmd start time */
    gettimeofday(&start, NULL);
    if (argc < 3) {
        usage();
        return -1;
    }

    /* get ifname */
    if (if_nametoindex(argv[1])) {
        strcpy(ifname, argv[1]);
    } else {
        usage();
        return -1;
    }

    /* Identify cmd and execute it */
    if ((strcmp(argv[2], "test_mgmt_ie") == 0 ) && argv[3] != NULL) {
        if (sf_vdr_add_mgmt_ie(argv[3]))
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "bwlist") == 0) && argv[3] != NULL) {
        char *pos, *end, *list_type = NULL, *mac_addres = NULL;
        int i, mac_count = 0;

        if (!strcmp(argv[3], "set")) {
            if (!argv[4]) {
                usage();
                return -1;
            }

            list_type = argv[4];
            if (strcmp(list_type, "none")) {
                mac_count = argc -5;
                if (argc - 5 > MAX_BLACK_WHITE_LIST_STA_CNT) {
                    printf("The count of MAC reach limit, just operate the stations before %s",
                                                        argv[MAX_BLACK_WHITE_LIST_STA_CNT + 5]);
                    mac_count = MAX_BLACK_WHITE_LIST_STA_CNT;
                }

                mac_addres = malloc(mac_count * 18);
                if (!mac_addres) {
                    printf("Space alloc failed!\n");
                    return -1;
                    goto DONE;
                }
                pos = mac_addres;
                end = mac_addres + mac_count * 18;
                memset(mac_addres, 0, end - pos);
                for (i = 0; i < mac_count; i++) {
                    snprintf(pos, end - pos, "%17s ", argv[i + 5]);
                    pos += 18;
                }
            }
        }

        if (sf_vdr_wifi_black_white_list(&hdr, argv[3], list_type, mac_addres, mac_count))
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "sta_info") == 0)) {
        if (sf_vdr_sta_info())
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "scan") == 0)) {
        if (sf_vdr_scan())
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "get_scan_result") == 0)) {
        if (sf_vdr_get_scan_result(&hdr, ifname))
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "get_chan_score") == 0)) {
        struct siwifi_vdr_chan_info info;
        if (sf_vdr_get_chan_score(&hdr, ifname, &info, false))
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "set_auto_chan") == 0)) {
        if (sf_vdr_set_auto_chan(&hdr, ifname))
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "con_code") == 0) && argv[3] != NULL ) {
        if (sf_vdr_country_code_info(argv[3]))
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "chan_info") == 0)) {
        if (sf_vdr_chan_info())
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "repeater_status") == 0)) {
        if (sf_vdr_get_repeater_status())
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "set_retry") == 0) && argv[3] != NULL && argv[4] != NULL) {
        if (sf_vdr_set_retry_num(argv[3], argv[4]))
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "set_cca") == 0) && argv[3] != NULL && argv[4] != NULL) {
        if (sf_vdr_set_cca(argv[3], argv[4]))
            return -1;
        goto DONE;
    } else if (((strcmp(argv[2], "set_ampdu") == 0) || (strcmp(argv[2], "set_amsdu") == 0) ||
                (strcmp(argv[2], "amsdu_th") == 0)) && argv[3] != NULL) {
        if (sf_vdr_set_agg_threshold(argv[2], argv[3]))
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "set_edca") == 0) && argv[3] != NULL) {
        if (sf_vdr_set_edca(argv[3]))
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "get_frame_num") == 0)) {
        if (sf_vdr_get_frame_num())
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "set_acs") == 0) && argv[3] != NULL && argv[4] != NULL && argv[5] != NULL) {
        if (sf_vdr_set_acs(argv[3], argv[4], argv[5]))
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "set_dfs") == 0) && argv[3] != NULL) {
        if (sf_vdr_set_dfs(argv[3]))
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "set_apsd") == 0) && argv[3] != NULL) {
        if (sf_vdr_set_apsd(argv[3]))
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "send_debug_frame") == 0) && argv[3] != NULL && argv[4] != NULL &&
                argv[5] != NULL && argv[6] != NULL && argv[7] != NULL && argv[8] != NULL &&
                argv[9] != NULL && argv[10] != NULL && argv[11] != NULL && argv[12] != NULL &&
                argv[13] != NULL) {
        if (sf_vdr_send_debug_frame(argv[3], argv[4], argv[5], argv[6],
                                    argv[7], argv[8], argv[9], argv[10],
                                    argv[11], argv[12], argv[13]))
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "set_power") == 0) && argv[3] != NULL && argv[4] != NULL) {
        if (sf_vdr_set_power(argv[3], argv[4]))
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "set_cali_table") == 0) && argv[3] != NULL) {
        if (sf_vdr_set_cali_table(argv[3]))
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "set_ant") == 0) && argv[3] != NULL && argv[4] != NULL && argv[5] != NULL) {
        if (sf_vdr_set_ant(argv[3], argv[4], argv[5]))
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "set_thermal_on") == 0) && argv[3] != NULL) {
        if (sf_vdr_set_thermal_on(argv[3]))
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "get_vap_stats") == 0)) {
        if (siwifi_vdr_get_vap_info(&hdr, argv[1]))
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "get_rf") == 0)) {
        if (siwifi_vdr_get_rf(&hdr, argv[1]))
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "set_power_limit") == 0) && argv[3] != NULL && argv[4] != NULL) {
        if (siwifi_vdr_set_power_limit(&hdr, argv[3], argv[4]))
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "mon_sta") == 0) && argv[3] != NULL && argv[4] != NULL && argv[5] != NULL) {
        if (sf_vdr_mon_sta(argv[3], argv[4], argv[5]))
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "radar_status") == 0) && argv[3] != NULL) {
        if (sf_vdr_get_radar_status(&hdr, argv[3]))
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "radar_threshold") == 0) && argv[3] != NULL) {
        if (sf_vdr_set_radar_threshold(&hdr, argv[3]))
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "set_rts_threshold") == 0) && argv[3] != NULL) {
        if (sf_vdr_set_rts_threshold(&hdr, argv[3]))
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "set_radar_detect") == 0) && argv[3] != NULL) {
        if (sf_vdr_set_radar_detect(&hdr, argv[3]))
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "get_radar_detect") == 0)) {
        if (sf_vdr_get_radar_detect(&hdr))
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "set_srrc") == 0)) {
        if (sf_vdr_set_srrc(&hdr, argv[3]))
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "get_wds_stats") == 0)) {
        if (sf_vdr_get_wds_stats(&hdr))
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "set_vlan") == 0) && argv[3] != NULL) {
        if (sf_vdr_set_vlan(&hdr, argv[3], argv[4]))
            return -1;
        goto DONE;
    } else if ((strcmp(argv[2], "set_feat") == 0) && argv[3] != NULL && argv[4] != NULL) {
        if (sf_vdr_set_feat(&hdr, argv[3], argv[4]))
            return -1;
        goto DONE;
    } else {
        usage();
        return -1;
    }
DONE:
    close_ioctl_socket();
    /* get cmd end time and print use time */
    gettimeofday(&end,NULL);
    use_time = (end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec);
    return 0;
}
