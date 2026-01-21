/*
 * =====================================================================================
 *
 *       Filename:  sfwifi_iface.h
 *
 *    Description:  Interface for wifi driver.
 *
 *        Version:  1.0
 *        Created:  2023年05月23日 20时34分05秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ming.guang (), ming.guang@siflower.com.cn
 *        Company:  Siflower
 *
 * =====================================================================================
 */

#ifndef _SIWIFI_IFACE_H_
#define _SIWIFI_IFACE_H_
#define ETH_ALEN 6
typedef unsigned char       uint8_t;
typedef   signed char        int8_t;
typedef unsigned short     uint16_t;
typedef   signed short      int16_t;
typedef unsigned int       uint32_t;
typedef   signed int        int32_t;
typedef unsigned long long uint64_t;
typedef   signed long long  int64_t;
typedef unsigned long        size_t;
typedef   signed long       ssize_t;
typedef     long long           s64;
typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;
typedef uint8_t     bool;

typedef u16 __bitwise be16;
typedef u16 __bitwise le16;
typedef u32 __bitwise be32;
typedef u32 __bitwise le32;

#define true  1
#define false 0
#define MAX_SCAN_BSS_CNT    64
#define DOT_OR_DOTDOT(s) ((s)[0] == '.' && (!(s)[1] || ((s)[1] == '.' && !(s)[2])))

/* Values for formatModTx */
#define FORMATMOD_NON_HT 0
#define FORMATMOD_NON_HT_DUP_OFDM 1
#define FORMATMOD_HT_MF 2
#define FORMATMOD_HT_GF 3
#define FORMATMOD_VHT 4
#define FORMATMOD_HE_SU 5
#define FORMATMOD_HE_MU 6
#define FORMATMOD_HE_ER 7
#define FORMATMOD_HE_TB 8

enum { COMMON_BUFSIZE = 1024 };
enum rate_info_flags {
    RATE_INFO_FLAGS_MCS         = 1U << (0),
    RATE_INFO_FLAGS_VHT_MCS     = 1U << (1),
    RATE_INFO_FLAGS_SHORT_GI    = 1U << (2),
    RATE_INFO_FLAGS_60G         = 1U << (3),
    RATE_INFO_FLAGS_HE_MCS      = 1U << (4),
};

enum rate_info_bw {
    RATE_INFO_BW_20 = 0,
    RATE_INFO_BW_5,
    RATE_INFO_BW_10,
    RATE_INFO_BW_40,
    RATE_INFO_BW_80,
    RATE_INFO_BW_160,
    RATE_INFO_BW_HE_RU,
};

enum nl80211_chan_width {
    NL80211_CHAN_WIDTH_20_NOHT,
    NL80211_CHAN_WIDTH_20,
    NL80211_CHAN_WIDTH_40,
    NL80211_CHAN_WIDTH_80,

    NL80211_CHAN_WIDTH_80P80,
    NL80211_CHAN_WIDTH_160,
    NL80211_CHAN_WIDTH_5,
    NL80211_CHAN_WIDTH_10,
};

enum nl80211_he_gi {
    NL80211_RATE_INFO_HE_GI_0_8,
    NL80211_RATE_INFO_HE_GI_1_6,
    NL80211_RATE_INFO_HE_GI_3_2,
};

enum nl80211_he_ru_alloc {
    NL80211_RATE_INFO_HE_RU_ALLOC_26,
    NL80211_RATE_INFO_HE_RU_ALLOC_52,
    NL80211_RATE_INFO_HE_RU_ALLOC_106,
    NL80211_RATE_INFO_HE_RU_ALLOC_242,
    NL80211_RATE_INFO_HE_RU_ALLOC_484,
    NL80211_RATE_INFO_HE_RU_ALLOC_996,
    NL80211_RATE_INFO_HE_RU_ALLOC_2x996,
};

enum siwifi_phy_mode {
    SF_PHY_MODE_11N,
    SF_PHY_MODE_11AC,
    SF_PHY_MODE_11AX,
    SF_PHY_MODE_UNSUPP,
};

struct sf_vdr_common_data {
    char str[64];
};

/**
 * struct siwifi_sta_rate_info - Struct of station rate information.
 *
 * @flags: Flags of the station.
 * @mcs: MCS of station.
 * @legacy:
 * @format_mod: Format_mod of the station.
 * @nss: NSS of the station.
 * @bw:  Bandwidth of the station.
 * @he_gi: GI of the station.
 * @he_ru_alloc: RU Size.
 * @he_dcm: Dual Carrier Modulation (for for HE modulation).
 */
struct siwifi_sta_rate_info {
    u8 flags;
    // u8 mcs;
    u16 legacy;
    u16 format_mod;
    // u8 nss;
    // u8 bw;
    u8 he_gi;
    u8 he_ru_alloc;
    u8 he_dcm;
};

/// Maximum number of SSIDs in a scan request
#define SCAN_SSID_MAX 2

/// Maximum SSID length in a scan request
#define MAX_SSID_LEN 32

/// Max number of channels in the 2.4 GHZ band
#define MAC_DOMAINCHANNEL_24G_MAX 14

/// Max number of channels in the 5 GHZ band
#define MAC_DOMAINCHANNEL_5G_MAX 28

/// Maximum number of channels in a scan request
#define SCAN_CHANNEL_MAX (MAC_DOMAINCHANNEL_24G_MAX + MAC_DOMAINCHANNEL_5G_MAX)

/// Maximum number of WDS PSK len
#define SF_VDR_WDS_PSK_MAX_LEN 68

/// Maximum number of SSID len
#define IEEE80211_MAX_SSID_LEN 32

/// Interface type
enum nl80211_iftype {
    NL80211_IFTYPE_UNSPECIFIED,
    NL80211_IFTYPE_ADHOC,
    NL80211_IFTYPE_STATION,
    NL80211_IFTYPE_AP,
    NL80211_IFTYPE_AP_VLAN,
    NL80211_IFTYPE_WDS,
    NL80211_IFTYPE_MONITOR,
    NL80211_IFTYPE_MESH_POINT,
    NL80211_IFTYPE_P2P_CLIENT,
    NL80211_IFTYPE_P2P_GO,
    NL80211_IFTYPE_P2P_DEVICE,
    NL80211_IFTYPE_OCB,
    NL80211_IFTYPE_NAN,

    /* keep last */
    NUM_NL80211_IFTYPES,
    NL80211_IFTYPE_MAX = NUM_NL80211_IFTYPES - 1
};

/**
 * struct cfg80211_ssid - SSID description.
 * @ssid: The SSID.
 * @ssid_len: Length of the ssid.
 */
struct cfg80211_ssid {
	u8 ssid[MAX_SSID_LEN];
	u8 ssid_len;
};

/**
 * struct sf_vdr_scan_chan_list - Struct of the scan request information.
 *
 * @param scan_chan_num: The number of scan channels.
 * @param scan_chan_freq_list: List of the scan channels' freq.
 * @duration_mandatory: If not set, the scan duration use default value.
 * @duration: How long to listen on each channel, in TUs.
 * @ssids: SSIDs to scan for (passed in the probe_reqs in active scans).
 * @n_ssids: Number of SSIDs.
 *
 */
struct sf_vdr_scan_chan_list {
    u32 scan_chan_num;
    u32 scan_chan_freq_list[SCAN_CHANNEL_MAX];
    bool duration_mandatory;
    u16 duration;
    struct cfg80211_ssid ssids[SCAN_SSID_MAX];
    int n_ssids;
};

#define CIPHER_NONE   (1 << 0)
#define CIPHER_WEP40  (1 << 1)
#define CIPHER_TKIP   (1 << 2)
#define CIPHER_WRAP   (1 << 3)
#define CIPHER_CCMP   (1 << 4)
#define CIPHER_WEP104 (1 << 5)
#define CIPHER_AESOCB (1 << 6)
#define CIPHER_CKIP   (1 << 7)
#define CIPHER_GCMP   (1 << 8)
#define CIPHER_COUNT   9

#define KMGMT_NONE    (1 << 0)
#define KMGMT_8021x   (1 << 1)
#define KMGMT_PSK     (1 << 2)
#define KMGMT_SAE     (1 << 3)
#define KMGMT_OWE     (1 << 4)
#define KMGMT_COUNT    5

#define AUTH_OPEN     (1 << 0)
#define AUTH_SHARED   (1 << 1)
#define AUTH_COUNT     2

/**
 * stuct sf_vdr_bwlist_info - Struct of the Black and White list.
 *
 * @count: Number of Mac address in list.
 * @length: Length of Mac addresses.
 * @mac_addres: The Mac addresses in list.
 *
*/
struct sf_vdr_bwlist_info {
    u16 count;
    u16 length;
    char *mac_addres;
};

/**
 * siwifi_crypto_entry - Encryption struct of BSS.
 *
 * @enabled: Privacy or not in capabilities information of beacon/probe frame.
 * @wpa_version: Indicate WPA version.
 * @group_ciphers: Indicate group cipher suite.
 * @pair_ciphers: Indicate pairwise cipher suite.
 * @auth_suites: Indicate authentication key management (AKM) suite.
 * @auth_algs: Indicate open system authentication or shared key authentication.
 */
struct siwifi_crypto_entry {
    uint8_t enabled;
    uint8_t wpa_version;
    uint16_t group_ciphers;
    uint16_t pair_ciphers;
    uint8_t auth_suites;
    uint8_t auth_algs;
};

/**
 * struct sf_vdr_scan_result - Struct of the scanned BSS information.
 *
 * @bssid: BSSID of the BSS.
 * @ssid: SSID of the BSS.
 * @rssi: RSSI of the BSS.
 * @nettype: Phy mode of the BSS.
 * @bandwidth: Bandwidth of the BSS.
 * @freq: Freq of the BSS.
 * @beacon_interval: The number of time units (TUs) between target
                     beacon transmission times (TBTTs).
 * @dtim_period: The number of beacon intervals between successive DTIMs.
 * @nss: Number of spatial streams.
 * @side_band:0: no secondary channel, 1: secondary channel is above,
 *            3: secondary channel is below.
 * @noise: Noise of the channel.
 * @crypto: Encryption method of the BSS.
 */
struct sf_vdr_scan_result {
    u8 bssid[ETH_ALEN];
    char ssid[33];
    int rssi;
    char nettype[8];
    char bandwidth[16];
    u32 freq;
    u16 beacon_interval;
    u8 dtim_period;
    u8 nss;
    u8 side_band;
    int noise;
    struct siwifi_crypto_entry crypto;
};

/**
 * struct siwifi_vdr_scan_result - Struct of the scan result.
 *
 * @scan_done: True: scan finished, False: scan unfinish.
 * @result_number: The number of scan result.
 * @scan_result: Pointer to the first scan result.
 */
struct siwifi_vdr_scan_result {
    bool scan_done;
    int result_number;
    struct sf_vdr_scan_result *scan_result;
};

/**
 * struct siwifi_vdr_chan_info - Struct of the channel info.
 *
 * @center_freq: Center frequence of the channel.
 * @bss_num: Number of the BSS on the channel.
 * @chan_time_ms: Amount of time in ms the radio spent on the channel.
 * @chan_time_busy_ms: Amount of time in ms the primary channel was sensed busy.
 * @noise_dbm: Noise in dbm.
 * @score: Socre of the channel quality.
 */
struct sf_vdr_chan_info {
    u32 center_freq;
    u32 bss_num;
    u32 chan_time_ms;
    u32 chan_time_busy_ms;
    int noise_dbm;
    u8  score;
};

/**
 * siwifi_vdr_chan_info - Struct of all channel info after scan.
 *
 * @scan_done: True: scan finished, False: scan unfinish.
 * @info_number: The number of channel information.
 * @chan_info: Pointer to the first channel information.
 */
struct siwifi_vdr_chan_info {
    bool scan_done;
    int  info_number;
    struct sf_vdr_chan_info chan_info[28];
};

/**
 * sf_vdr_set_chan_info - Struct for set radio channel.
 *
 * @set_done: True: set success, False: scan failed.
 * @prim20_freq: Frequency for Primary 20MHz channel (in MHz).
 * @center1_freq: Frequency center of the contiguous channel or center of Primary 80+80 (in MHz).
 * @center2_freq: Frequency center of the non-contiguous secondary 80+80 (in MHz)
 * @width: Channel width.
 */
struct sf_vdr_set_chan_info {
    bool set_done;
    u16 prim20_freq;
    u16 center1_freq;
    u16 center2_freq;
    enum nl80211_chan_width width;
};

/**
 * struct siwifi_vdr_trx_stats - Information of tx rx packet statistics.
 *
 * @rx_packets: Packets number received in rx path.
 * @rx_unicast_packets: Unicast packets number received in rx path.
 * @rx_multicast_packets: Multicast packets number received in rx path.
 * @rx_broadcast_packets: Broadcastpackets number received in rx path.
 * @rx_dropped: Packets dropped number in rx path.
 * @tx_packets: Packets number transmited in tx path.
 * @tx_unicast_packets: Unicast packets number transmited in tx path.
 * @tx_multicast_packets: Multicast packets number transmited in tx path.
 * @tx_broadcast_packets: Broadcast packets number transmited in tx path.
 * @tx_dropped: Packets dropped number in tx path.
 * @rx_bytes: Total bytes received in rx path.
 * @rx_unicast_bytes: Unicast bytes received in rx path.
 * @rx_multicast_bytes: Multicast bytes received in rx path.
 * @rx_broadcast_bytes: Broadcast bytes received in rx path.
 * @tx_bytes: Total bytes transmited in tx path.
 * @tx_unicast_bytes: Unicast bytes transmited in tx path.
 * @tx_multicast_bytes: Multicast bytes transmited in tx path.
 * @tx_broadcast_bytes: Broadcast bytes transmited in tx path.
 */
struct siwifi_vdr_trx_stats {
    /* Packets */
    u32 rx_packets;
    u32 rx_unicast_packets;
    u32 rx_multicast_packets;
    u32 rx_broadcast_packets;
    u32 rx_dropped;
    u32 tx_packets;
    u32 tx_unicast_packets;
    u32 tx_multicast_packets;
    u32 tx_broadcast_packets;
    u32 tx_dropped;

    /* Bytes */
    u64 rx_bytes;
    u64 rx_unicast_bytes;
    u64 rx_multicast_bytes;
    u64 rx_broadcast_bytes;
    u64 tx_bytes;
    u64 tx_unicast_bytes;
    u64 tx_multicast_bytes;
    u64 tx_broadcast_bytes;
};


/**
 * struct siwifi_vap_info - Struct of AP info.
 *
 * @bssid: BSSID of the access point.
 * @ssid: SSID (Service Set Identifier) of the access point.
 * @tx_bytes: Total number of transmitted bytes.
 * @rx_bytes: Total number of received bytes.
 * @tx_packets: Total number of transmitted packets.
 * @rx_packets: Total number of received packets.
 * @tx_errors: Number of packets that failed to send after being pushed to the firmware.
 * @rx_errors: Number of errors detected, including dot11_fcs_error_count and nx_rx_phy_error_count from MIB.
 * @rx_non_unicast_packets: Number of received non-unicast packets.
 * @tx_unicast_packets: Number of transmitted unicast packets.
 * @rx_unicast_packets: Number of received unicast packets.
 * @tx_dropped: Number of packets dropped by the MAC layer.
 * @rx_dropped: Number of packets dropped by the MAC layer.
 * @psk_failures: Number of PSK (Pre-Shared Key) authentication failures. TODO
 * @integrity_failures: Number of integrity failures.
 * @tx_multicast_packets: Number of transmitted multicast packets.
 * @rx_multicast_packets: Number of received multicast packets.
 * @tx_broadcast_packets: Number of transmitted broadcast packets.
 * @rx_broadcast_packets: Number of received broadcast packets.
 * @rx_unknown_packets: Number of received unknown packets.
 * @tx_mgmt_packets: Number of transmitted management packets.
 * @rx_mgmt_packets: Number of received management packets.
 * @ack_failure_count: Number of ACK (Acknowledgment) failures, corresponding to nx_qos_ack_failure_count in MIB.
 * @aggregated_packet_count: Number of aggregated packets (AMPDU count).
 * @failed_retrans_count: Number of failed retransmissions, equal to tx_dropped.
 * @multiple_retry_count: Number of multiple retries for multicast packets in the MAC layer.
 * @retry_count: Number of retries in the MAC layer.
 * @retrans_count: Number of retransmissions, corresponding to dot11_qos_retry_count in MIB.
 * @tx_throughput: Transmission throughput in Kbps (Kilobits per second).
 * @rx_throughput: Reception throughput in Kbps (Kilobits per second).
 * @bssid: BSSID (Basic Service Set Identifier) of the access point.
 * @ssid: SSID (Service Set Identifier) of the access point.
 */
struct siwifi_vap_info {
    u64 tx_bytes;
    u64 rx_bytes;
    u64 tx_packets;
    u64 rx_packets;
    u64 tx_errors;               /* Packets fail to send after push to fw */
    u64 rx_errors;               /* dot11_fcs_error_count and nx_rx_phy_error_count in MIB */
    u64 rx_non_unicast_packets;
    u64 tx_unicast_packets;
    u64 rx_unicast_packets;
    u64 tx_dropped;              /* Packets dropped by umac */
    u64 rx_dropped;

    u64 psk_failures; /* TODO */

    u64 integrity_failures;

    u64 tx_multicast_packets;
    u64 rx_multicast_packets;
    u64 tx_broadcast_packets;
    u64 rx_broadcast_packets;
    u64 rx_unknown_packets;
    u64 tx_mgmt_packets;
    u64 rx_mgmt_packets;
    u32 ack_failure_count;       /* nx_qos_ack_failure_count in MIB */
    u32 aggregated_packet_count; /* ampdu count */
    u32 failed_retrans_count;    /* equal to tx_dropped */
    u32 multiple_retry_count;    /* Multicast retry number in umac */
    u32 retry_count;             /* Retry number in umac */
    u32 retrans_count;           /* dot11_qos_retry_count in MIB */
    u32 tx_throughput; /* in Kbps */
    u32 rx_throughput; /* in Kbps */
    u8     bssid[ETH_ALEN];
    char   ssid[32];
};

/**
 * struct siwifi_station_info - Struct of STA info.
 *
 * @link_rate: Link rate(association rate) of the station, unit: kbps.
 * @mcs: Max MCS Capability Supported by STA.
 * @tx_mcs: Transmission MCS.
 * @rx_mcs: Reception MCS.
 * @rssi: RSSI of the station, unit: dBm.
 * @noise: Noise of the station.
 * @snr: Signal-to-Noise Ratio: rssi divided by noise.
 * @tx_rate: Tx rate information, unit: kbps.
 * @rx_rate: Rx rate information, unit: kbps.
 * @power_save: Whether the station is in PS mode.
 * @retransmissions:
 * @tx_packets: Total number of transmitted packets.
 * @rx_packets: Total number of received packets.
 * @tx_bytes: Total number of transmitted bytes.
 * @rx_bytes: Total number of received bytes.
 * @errors_sent: Number of packets that failed to send after being pushed to the firmware.
 * @failed_retrans_count:
 * @retrans_count: The same as retransmissions.
 * @multi_retry_count: Number of retry transmitted multicast frames.
 * @tx_kbps: Number of bits within the last 1 second(AP->STA), unit: kbps.
 * @rx_kbps: Number of bits within the last 1 second(STA->AP), unit: kbps.
 * @disassoc_reason: Disassociation reason code for station disconnect.
 * @antenna: Max number of spatial streams supported minus 1.
 * @retry_count: Number of retry transmitted frames.
 * @bss_transition: Whether the station supports BSS transition.
 * @beacon_report_support: Whether the station supports beacon report generation.
 * @beam_forming: Whether the station supports beamforming technology.
 * @MUMIMOS_up: Whether the station supports Multi-User MIMO (MU-MIMO) uplink transmission.
 * @txPER: Packet Error Rate, errors_sent divided by tx_packets.
 * @rx_unicast_packets: Number of received unicast packets.
 * @tx_multicast_packets: Number of transmitted multicast packets.
 * @rx_multicast_packets: Number of received multicast packets.
 * @tx_broadcast_packets: Number of transmitted broadcast packets.
 * @rx_broadcast_packets: Number of received broadcast packets.
 * @bssid: BSSID of the access point.
 * @quality: Quality of the station, unit: percent(%).
 * @link_time: Link time of the station, unit: second.
 * @sta_trx_stat: Tx Rx packet statistics of STA.
 * @tx_unicast_packets: Unicast packets number transmited in tx path.
 * @tx_dropped: Packets dropped number in tx path.
 * @rx_dropped: Packets dropped number in rx path.
 * @tx_unicast_bytes: Unicast bytes transmited in tx path.
 * @tx_multicast_bytes: Multicast bytes transmited in tx path.
 * @tx_broadcast_bytes: Broadcast bytes transmited in tx path.
 * @rx_unicast_bytes: Unicast bytes received in rx path.
 * @rx_multicast_bytes: Multicast bytes received in rx path.
 * @rx_broadcast_bytes: Broadcast bytes received in rx path.
 * @tx_nss: Transmission nss.
 * @rx_nss: Reception nss.
 */
struct siwifi_station_info {
    s64 link_rate;
    int mcs;
    int tx_mcs;
    int rx_mcs;
    int rssi;
    int noise;
    int snr;
    s64 tx_rate;
    s64 rx_rate;
    bool power_save;
    s64 retransmissions;
    u64 tx_packets;
    u64 rx_packets;
    u64 tx_bytes;
    u64 rx_bytes;
    s64 errors_sent;
    s64 failed_retrans_count;
    s64 retrans_count;
    s64 multi_retry_count;
    s64 retry_count;
    s64 tx_kbps;
    s64 rx_kbps;
    s64 disassoc_reason;
    s64 antenna;
    enum rate_info_bw bw;
    bool bss_transition;
    bool beacon_report_support;
    bool beam_forming;
    bool MUMIMOS_up;
    int txPER;
    s64 rx_unicast_packets;
    s64 tx_multicast_packets;
    s64 rx_multicast_packets;
    s64 tx_broadcast_packets;
    s64 rx_broadcast_packets;
    s64 tx_mgmt_packets;
    s64 rx_mgmt_packets;

    /* Extra part of siflower */
    u8 bssid[ETH_ALEN];
    u8 tx_nss;
    u8 rx_nss;
    u8 tx_bw;
    u8 rx_bw;
    u8 format_mod;
    u8 short_gi;
    u8 dcm_mcs_max;
    char quality;
    u32 link_time;

    u32 tx_unicast_packets;
    u32 tx_dropped;
    u32 rx_dropped;
    u64 tx_unicast_bytes;
    u64 tx_multicast_bytes;
    u64 tx_broadcast_bytes;
    u64 rx_unicast_bytes;
    u64 rx_multicast_bytes;
    u64 rx_broadcast_bytes;
    struct siwifi_sta_rate_info txrate;
    struct siwifi_sta_rate_info rxrate;
};

struct country_chan_info
{
    uint32_t center_freq;
    int max_bw;
    int max_dBm;
    int dfs;
};

struct siwifi_chan_info
{
    u16 freq;
    u8 bandwidth;
    u8 side_flag;
};

/**
 * sf_vdr_phy_info - Structure for RF-related data
 *
 * @channel: Channel of the phy.
 * @center_freq: Center frequency in MHz.
 * @width: Channel width.
 * @mode: Hardware mode of the phy.
 * @tx_packets: The number of TX data frames.
 * @tx_sucess: The number of TX data frames successfully.
 * @tx_fail: The number of TX data frames failed.
 * @rx_dropped: The number of RX data frames failed.
 * @rx_sucess: The number of RX data frames successfully.
 * @rx_crc: The number of CRC errors in MPDU delimeter of and A-MPDU.
 * @false_cca: The number of unsuccessful RTS Frame transmission.
 * @beacon_count: The number of successful Beacon Frame transmission.
 * @tx_retry: The number of A-MSDU's transmitted successfully with retry.
 * @PLCP_error_count: The number of PHY Errors reported during a receive transaction.
 * @FCS_error_count: The receive FCS errors.
 * @swcfgtime: The statistical period.
 * @obss_chlutil: The time occupied by interference from other APs
 * @bss_chlutil: The time occupied by AP interference on this channel.
 * @nonwifi_intf_util: The time occupied by non-WiFi interference.
 * @temperature: wifif Current Temperature.
 * @probe_request: The number of RX probe_request.
 * @probe_respone: The number of TX probe_respone.
 * @cca_busy: cca edca busy time(us) in last period, used to calculate duty cycle..
 * @chan_time: While cal period, used to calculate duty cycle..
 * @rx_frame_time: Whole rx time in last period, used to calculate RxInradioUtil.
 * @tx_frame_time: Whole tx time in last period, used to calculate TxInradioUtil.
 * @isnr: Signal-to-noise ratio (SNR)
 *
 */
struct sf_vdr_phy_info {
    u32 channel;
    u32 center_freq;
    enum nl80211_chan_width width;
    enum siwifi_phy_mode mode;
    u32 tx_packets;
    u32 tx_sucess;
    u32 tx_fail;
    u32 rx_dropped;
    u32 rx_sucess;
    u64 rx_crc;
    u64 false_cca;
    u64 beacon_count;
    u32 tx_retry;
    u32 PLCP_error_count;
    u32 FCS_error_count;
    u32 swcfgtime;
    u32 obss_chlutil;
    u32 bss_chlutil;
    u32 nonwifi_intf_util;
    u32 temperature;
    u64 interrupt;
    u64 probe_request;
    u64 probe_respone;
    /*TODO:  Retrieve these parameters after lmac supports mpinfo.*/
    int noise;
    u32 cca_busy;
    u32 chan_time;
    u32 rx_frame_time;
    u32 tx_frame_time;
    int isnr;
};

struct sf_vdr_wds_info {
    u8 ssid[IEEE80211_MAX_SSID_LEN + 1];
    int channel;
    u8 bssid[ETH_ALEN];
    u8 sta_mac[ETH_ALEN];
    u8 wpa_psk[SF_VDR_WDS_PSK_MAX_LEN];
    int psklen;
    u8 auth_mode[32];
    u8 encryp_type[32];
    bool supported_wps;
    int signal;
    int snr;
    int noise;
    u32 assoc_rate;
    u32 last_data_downlink_rate;
    u32 last_data_uplink_rate;
    u32 txkbps;
    u32 rxkbps;
    u16 wireless_mode;
    u32 tx_error_count;
    u32 rx_error_count;
    u32 tx_count;
    u32 rx_count;
    u32 band_width;
    u64 rx_bytes;
    u32 tx_bytes;
    struct siwifi_station_info ap_info;
    struct siwifi_crypto_entry crypto;
};
#endif /* _SIWIFI_IFACE_H_ */
