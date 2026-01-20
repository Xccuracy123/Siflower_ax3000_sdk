#ifndef _SIWIFI_IFACE_H_
#define _SIWIFI_IFACE_H_

/*  Command ID */
/* Agent Commands */
#define SF_AGENT_CMD_DUMP_INFO 0X0000
#define SF_AGENT_CMD_UP_DOWN_NOTIFY 0x0001
#define SF_AGENT_CMD_BIND 0x0002
#define SF_AGENT_CMD_SET_DISCOVERY 0x0003
#define SF_AGENT_CMD_GET_STATUS 0x0004

/* Controller Commands */
#define SF_CONTROLLER_CMD_DUMP_INFO 0x1000
#define SF_CONTROLLER_CMD_RENEW 0x1001
#define SF_CONTROLLER_CMD_TOPOLOGY 0x1002
#define SF_CONTROLLER_CMD_AP_CAPA_QUERY 0x1003
#define SF_CONTROLLER_CMD_CLI_CAPA_QUERY 0x1004
#define SF_CONTROLLER_CMD_BACK_STEER_REQ 0x1005
#define SF_CONTROLLER_CMD_CHAN_PREFR_REQ 0x1006
#define SF_CONTROLLER_CMD_CHAN_SCAN_REQ 0x1007
#define SF_CONTROLLER_CMD_CHAN_SELECT_REQ 0x1008

#define SUPPORT_RADIO_MAX 2
#define SF_MAX_OPER_CLASS_NUM 20
#define SF_MAX_OPER_CHANNEL 24
#define SF_MAX_OPER_SCAN_CHANNEL 13
#define ETH_ALEN 6

/**
 * sf_channel_scan_request - Structure representing channel scan request parameters.
 *
 * @al_mac: The al mac address of device.
 * @fresh_scan: Boolean indicating whether a fresh scan is requested.
 *              If true, the request is to perform a fresh scan and return the results.
 *              If false, the request is to return stored results from the last successful scan.
 * @num_radios: Number of radios for which the channel scan is being requested.
 *              The maximum number of radios is defined by SUPPORT_RADIO_MAX.
 * @radio_requests: Array of structures, each containing scan request details for a specific radio.
 */
typedef struct {
    uint8_t al_mac[ETH_ALEN]; // The al mac address of device.
    bool fresh_scan;          // Indicator for a fresh scan.
    uint8_t num_radios;       // Number of radios for the scan request.

    struct {
        uint8_t radio_id[ETH_ALEN]; // Unique identifier of the radio.
        uint8_t num_oper_classes;   // Number of operating classes for the radio.

        struct {
            uint8_t operating_class;                            // Operating class value from Table E-4 in Annex E.
            uint8_t num_channels;                               // Number of channels in the channel list.
            uint8_t channel_list[SF_MAX_OPER_SCAN_CHANNEL];     // List of channel numbers for scanning.
        } operating_classes[SF_MAX_OPER_CLASS_NUM];             // Array of operating classes for the radio.
    } radio[SUPPORT_RADIO_MAX];                                 // Array of radio scan requests.
} sf_channel_scan_request;

/**
 * struct sfcfghdr - Structure for IOCTL command configuration.
 *
 * @magic_no: Magic number to identify the structure.
 * @command_type: Type of the command.
 * @command_id: Specific ID of the command.
 * @length: Length of the data.
 * @sequence: Sequence number for tracking the order of commands.
 * @status: Status of the command execution.
 * @data: Data buffer.
 */
struct sfcfghdr {
    uint32_t magic_no;
    uint32_t comand_type;
    uint32_t comand_id;
    uint16_t length;
    uint16_t sequence;
    uint32_t status;
    char data[4096];
} __attribute__((packed));

#endif /* _SIWIFI_IFACE_H_ */