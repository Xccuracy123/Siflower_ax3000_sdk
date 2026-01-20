#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/wireless.h>
#include <stdbool.h>

#include "easymesh_ioctl.h"

#define SFCFG_PRIV_IOCTL_AGENT (SIOCIWFIRSTPRIV + 0x0a)
#define SFCFG_PRIV_IOCTL_CONTROLLER (SIOCIWFIRSTPRIV + 0x0c)

char ifname[16];
static int32_t ioctl_socket = -1;

/* Get the ioctl socket fd. */
static int32_t get_ioctl_socket(void)
{
    /*  Prepare socket. */
    if (ioctl_socket == -1) {
        ioctl_socket = socket(AF_INET, SOCK_DGRAM, 0);
        fcntl(ioctl_socket, F_SETFD, fcntl(ioctl_socket, F_GETFD) | FD_CLOEXEC);
    }

    return ioctl_socket;
}

/* Close the ioctl socket fd. */
static void close_ioctl_socket(void)
{
    if (ioctl_socket > -1)
        close(ioctl_socket);
    ioctl_socket = -1;
}

/* Do the ioctl. */
static int32_t do_ioctl(int32_t cmd, struct iwreq *wrq)
{
    int32_t s = get_ioctl_socket();

    if (!wrq) {
        printf("%s error:wrq is NULL\n", __func__);
        return -1;
    }
    strncpy(wrq->ifr_name, ifname, IFNAMSIZ);
    if (s < 0) {
        printf("get socket failed!\n");
        return s;
    }

    return ioctl(s, cmd, wrq);
}

/*=============================================== Controller Commands ===================================================*/
/**
 * siwifi_easymesh_ioctl_controller_dump_info - Process "dump_info" command.
 *
 * @param al_mac: The MAC address of the device.
 *
 * This function sends SF_CONTROLLER_CMD_DUMP_INFO command to controller for dump dvice info.
 *
 * Return: 0 on success, otherwise error code.
 */
static int siwifi_easymesh_ioctl_controller_dump_info(char *al_mac)
{
    struct sfcfghdr hdr;
    struct iwreq wrq;

    /* Fill hdr. */
    memset(&hdr, 0, sizeof(struct sfcfghdr));
    hdr.comand_type = SF_CONTROLLER_CMD_DUMP_INFO;
    hdr.comand_id = SF_CONTROLLER_CMD_DUMP_INFO;
    hdr.sequence = 1;

    /* Fill wrq. */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&hdr;

    if (al_mac) {
        hdr.length = sprintf(hdr.data, "%s", al_mac);
        wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr.length;
    } else {
        hdr.length = 0;
        hdr.data[0] = '\0';
        wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr.length;
    }

    /* Send message to driver. */
    if (do_ioctl(SFCFG_PRIV_IOCTL_CONTROLLER, &wrq) < 0) {
        printf("IOCTL: Command SF_CONTROLLER_CMD_DUMP_INFO failed\n");
        return -1;
    }

    return 0;
}

/**
 * siwifi_easymesh_ioctl_renew - Process "renew" command.
 *
 * @param fh_ssid: Fronthaul SSID.
 * @param fh_key: Fronthaul key.
 * @param fh_encryption: Fronthaul encryption.
 *
 * This function sends SF_CONTROLLER_CMD_RENEW command to controller for updating fronthaul BSS encryption settings.
 *
 * Return: 0 on success, otherwise error code.
 */
static int siwifi_easymesh_ioctl_renew(char *fh_ssid, char *fh_key, char *fh_encryption)
{
    struct sfcfghdr hdr;
    struct iwreq wrq;

    /* Fill hdr. */
    memset(&hdr, 0, sizeof(struct sfcfghdr));
    hdr.comand_type = SF_CONTROLLER_CMD_RENEW;
    hdr.comand_id = SF_CONTROLLER_CMD_RENEW;
    hdr.sequence = 1;

    /* Fill wrq. */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&hdr;

    if (fh_ssid && fh_key && fh_encryption) {
        hdr.length = sprintf(hdr.data, "%s %s %s", fh_ssid, fh_key, fh_encryption);
        wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr.length;
    } else {
        hdr.length = 0;
        hdr.data[0] = '\0';
        wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr.length;
    }

    /* Send message to driver. */
    if (do_ioctl(SFCFG_PRIV_IOCTL_CONTROLLER, &wrq) < 0) {
        printf("IOCTL: Command SF_CONTROLLER_CMD_RENEW failed\n");
        return -1;
    }

    return 0;
}

/**
 * siwifi_easymesh_ioctl_get_topology - Process "get_topo" command.
 *
 * This function sends SF_CONTROLLER_CMD_TOPOLOGY command to controller for getting network topology.
 *
 * Return: 0 on success, otherwise error code.
 */
static int siwifi_easymesh_ioctl_get_topology(void)
{
    struct sfcfghdr hdr, *tmp;
    struct iwreq wrq;
    char *topology;

    /* Fill hdr. */
    memset(&hdr, 0, sizeof(struct sfcfghdr));
    hdr.comand_type = SF_CONTROLLER_CMD_TOPOLOGY;
    hdr.comand_id = SF_CONTROLLER_CMD_TOPOLOGY;
    hdr.length = 0;
    hdr.data[0] = '\0';
    hdr.sequence = 1;

    /* Fill wrq. */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&hdr;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr.length;

    /* Send message to driver. */
    if (do_ioctl(SFCFG_PRIV_IOCTL_CONTROLLER, &wrq) < 0) {
        printf("IOCTL: Command SF_CONTROLLER_CMD_TOPOLOGY failed\n");
        return -1;
    }

    /* Receive message from driver. */
    tmp = (struct sfcfghdr *)wrq.u.data.pointer;
    topology = malloc(tmp->length);
    if (!topology) {
        printf("Alloc space for topology map failed!\n");
        return -1;
    }

    memset(topology, 0, tmp->length);
    memcpy(topology, tmp->data, tmp->length);
    printf("%s\n", topology);

    free(topology);
    return 0;
}

/**
 * siwifi_easymesh_ioctl_send_ap_capability_query - Process "sendApCapaQuery" command.
 *
 * @param al_mac: The MAC address of the device.
 *
 * This function sends SF_CONTROLLER_CMD_AP_CAPA_QUERY command to controller to send AP Capability Query frame.
 *
 * Return: 0 on success, otherwise error code.
 */
static int siwifi_easymesh_ioctl_send_ap_capability_query(char *al_mac)
{
    struct sfcfghdr hdr;
    struct iwreq wrq;

    /* Fill hdr. */
    memset(&hdr, 0, sizeof(struct sfcfghdr));
    hdr.comand_type = SF_CONTROLLER_CMD_AP_CAPA_QUERY;
    hdr.comand_id = SF_CONTROLLER_CMD_AP_CAPA_QUERY;
    hdr.sequence = 1;

    /* Fill wrq. */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&hdr;

    hdr.length = sprintf(hdr.data, "%s", al_mac);
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr.length;

    /* Send message to driver. */
    if (do_ioctl(SFCFG_PRIV_IOCTL_CONTROLLER, &wrq) < 0) {
        printf("IOCTL: Command SF_CONTROLLER_CMD_AP_CAPA_QUERY failed\n");
        return -1;
    }

    return 0;
}

/**
 * siwifi_easymesh_ioctl_send_client_capability_query - Process "sendCliCapaQuery" command.
 *
 * @param client_mac: The MAC address of the client.
 *
 * This function sends SF_CONTROLLER_CMD_CLI_CAPA_QUERY command to controller to send Client Capability Query frame.
 *
 * Return: 0 on success, otherwise error code.
 */
static int siwifi_easymesh_ioctl_send_client_capability_query(char *client_mac)
{
    struct sfcfghdr hdr;
    struct iwreq wrq;

    /* Fill hdr. */
    memset(&hdr, 0, sizeof(struct sfcfghdr));
    hdr.comand_type = SF_CONTROLLER_CMD_CLI_CAPA_QUERY;
    hdr.comand_id = SF_CONTROLLER_CMD_CLI_CAPA_QUERY;
    hdr.sequence = 1;

    /* Fill wrq. */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&hdr;

    hdr.length = sprintf(hdr.data, "%s", client_mac);
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr.length;

    /* Send message to driver. */
    if (do_ioctl(SFCFG_PRIV_IOCTL_CONTROLLER, &wrq) < 0) {
        printf("IOCTL: Command SF_CONTROLLER_CMD_CLI_CAPA_QUERY failed\n");
        return -1;
    }

    return 0;
}

/**
 * siwifi_easymesh_ioctl_send_backhaul_steering_request - Process "sendBHSteerReq" command.
 *
 * @param al_mac: The MAC address of the device.
 * @param bssid: The bssid of BSS client associated with.
 *
 * This function sends SF_CONTROLLER_CMD_BACK_STEER_REQ command to controller to send Backhaul Steering frame.
 *
 * Return: 0 on success, otherwise error code.
 */
static int siwifi_easymesh_ioctl_send_backhaul_steering_request(char *al_mac, char *bssid)
{
    struct sfcfghdr hdr;
    struct iwreq wrq;

    /* Fill hdr. */
    memset(&hdr, 0, sizeof(struct sfcfghdr));
    hdr.comand_type = SF_CONTROLLER_CMD_BACK_STEER_REQ;
    hdr.comand_id = SF_CONTROLLER_CMD_BACK_STEER_REQ;
    hdr.sequence = 1;

    /* Fill wrq. */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&hdr;

    hdr.length = sprintf(hdr.data, "%s %s", al_mac, bssid);
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr.length;

    /* Send message to driver. */
    if (do_ioctl(SFCFG_PRIV_IOCTL_CONTROLLER, &wrq) < 0) {
        printf("IOCTL: Command SF_CONTROLLER_CMD_BACK_STEER_REQ failed\n");
        return -1;
    }

    return 0;
}

/**
 * siwifi_easymesh_ioctl_send_channel_preference_request - Process "sendChanPrefeReq" command.
 *
 * @param al_mac: The MAC address of the device.
 *
 * This function sends SF_CONTROLLER_CMD_CHAN_PREFR_REQ command to controller to send Channel Preference Request frame.
 *
 * Return: 0 on success, otherwise error code.
 */
static int siwifi_easymesh_ioctl_send_channel_preference_request(char *al_mac)
{
    struct sfcfghdr hdr;
    struct iwreq wrq;

    /* Fill hdr. */
    memset(&hdr, 0, sizeof(struct sfcfghdr));
    hdr.comand_type = SF_CONTROLLER_CMD_CHAN_PREFR_REQ;
    hdr.comand_id = SF_CONTROLLER_CMD_CHAN_PREFR_REQ;
    hdr.sequence = 1;

    /* Fill wrq. */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&hdr;

    hdr.length = sprintf(hdr.data, "%s", al_mac);
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr.length;

    /* Send message to driver. */
    if (do_ioctl(SFCFG_PRIV_IOCTL_CONTROLLER, &wrq) < 0) {
        printf("IOCTL: Command SF_CONTROLLER_CMD_CHAN_PREFR_REQ failed\n");
        return -1;
    }

    return 0;
}

/**
 * siwifi_easymesh_ioctl_send_channel_scan_request - Process "sendChanPrefeReq" command.
 *
 * @param argv: User input parameters starting from radio configurations.
 *
 * This function sends SF_CONTROLLER_CMD_CHAN_SCAN_REQ command to controller to send Channel Scan Request frame.
 *
 * Return: 0 on success, otherwise error code.
 */
static int siwifi_easymesh_ioctl_send_channel_scan_request(char **argv)
{
    struct sfcfghdr hdr;
    struct iwreq wrq;
    size_t req_size;
    sf_channel_scan_request req;
    int i, j, k;

    /* Fill hdr. */
    memset(&hdr, 0, sizeof(struct sfcfghdr));
    hdr.comand_type = SF_CONTROLLER_CMD_CHAN_SCAN_REQ;
    hdr.comand_id = SF_CONTROLLER_CMD_CHAN_SCAN_REQ;
    hdr.sequence = 1;

    /* Fill wrq. */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&hdr;

    /* Initialize the structure with zeros. */
    memset(&req, 0, sizeof(req));

    /* Parse al_mac (argv[0]). */
    if (sscanf(argv[0], "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx", &req.al_mac[0], &req.al_mac[1], &req.al_mac[2],
               &req.al_mac[3], &req.al_mac[4], &req.al_mac[5]) != 6) {
        printf("Failed to parse the al mac.\n");
        return -1;
    }

    /* Parse fresh_scan (argv[1]). */
    if (argv[1] == NULL) {
        printf("Invalid input: fresh_scan parameter is missing.\n");
        return -1;
    }
    req.fresh_scan = (strcmp(argv[1], "true") == 0);

    /* Parse num_radios (argv[2]). */
    if (argv[2] == NULL) {
        printf("Invalid input: num_radios parameter is missing.\n");
        return -1;
    }
    req.num_radios = atoi(argv[2]);
    if (req.num_radios <= 0 || req.num_radios > SUPPORT_RADIO_MAX) {
        printf("Invalid number of radios: %d\n", req.num_radios);
        return -1;
    }

    /* Parse each radio's scan request. */
    for (i = 0; i < req.num_radios; i++) {
        char *radio_arg = argv[3 + i];
        if (radio_arg == NULL) {
            printf("Invalid input: radio argument %d is missing.\n", i + 1);
            return -1;
        }

        /* Proceed with parsing radio_arg. */
        char *token = strtok(radio_arg, ":");

        if (token == NULL || strcmp(token, "radio") != 0) {
            printf("Expected 'radio' but got '%s'\n", token);
            return -1;
        }

        /* Parse radio_id. */
        token = strtok(NULL, ",");
        if (token == NULL) {
            printf("Invalid input: radio_id parameter is missing.\n");
            return -1;
        }
        if (sscanf(token, "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx", &req.radio[i].radio_id[0], &req.radio[i].radio_id[1],
                   &req.radio[i].radio_id[2], &req.radio[i].radio_id[3], &req.radio[i].radio_id[4],
                   &req.radio[i].radio_id[5]) != 6) {
            printf("Failed to parse the radio id.\n");
            return -1;
        }

        /* Parse num_oper_classes. */
        token = strtok(NULL, ",");
        if (token == NULL) {
            printf("Invalid input: num_oper_classes is missing for radio %d.\n", i + 1);
            return -1;
        }
        req.radio[i].num_oper_classes = atoi(token);
        if (req.radio[i].num_oper_classes <= 0 || req.radio[i].num_oper_classes > SF_MAX_OPER_CLASS_NUM) {
            printf("Invalid num_oper_classes: %d\n", req.radio[i].num_oper_classes);
            return -1;
        }

        /* Parse each operating class. */
        for (j = 0; j < req.radio[i].num_oper_classes; j++) {
            /* Parse operating_class. */
            token = strtok(NULL, ",");
            if (token == NULL) {
                printf("Invalid input: operating_class is missing for radio %d, op class %d.\n", i + 1, j + 1);
                return -1;
            }
            req.radio[i].operating_classes[j].operating_class = atoi(token);

            /* Parse num_channels. */
            token = strtok(NULL, ",");
            if (token == NULL) {
                printf("Invalid input: num_channels is missing for radio %d, class %d.\n", i + 1, j + 1);
                return -1;
            }
            req.radio[i].operating_classes[j].num_channels = atoi(token);
            if (req.radio[i].operating_classes[j].num_channels <= 0 ||
                req.radio[i].operating_classes[j].num_channels > SF_MAX_OPER_CHANNEL) {
                printf("Invalid num_channels: %d\n", req.radio[i].operating_classes[j].num_channels);
                return -1;
            }

            /* Parse channel_list. */
            for (k = 0; k < req.radio[i].operating_classes[j].num_channels; k++) {
                token = strtok(NULL, ",");
                if (token == NULL) {
                    printf("Invalid input: channel number is missing for radio %d, class %d, channel %d.\n", i + 1,
                           j + 1, k + 1);
                    return -1;
                }
                req.radio[i].operating_classes[j].channel_list[k] = atoi(token);
            }
        }
    }

    req_size = sizeof(sf_channel_scan_request);
    memcpy(hdr.data, &req, req_size);
    hdr.length = req_size;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr.length;

    /* Send message to driver. */
    if (do_ioctl(SFCFG_PRIV_IOCTL_CONTROLLER, &wrq) < 0) {
        printf("IOCTL: Command SF_CONTROLLER_CMD_CHAN_SCAN_REQ failed\n");
        return -1;
    }

    return 0;
}

/**
 * siwifi_easymesh_ioctl_send_channel_selection_request - Process "sendChanPrefeReq" command.
 *
 * @param al_mac: The MAC address of the device.
 * @param radio_mac: The radio identifier.
 *
 * This function sends SF_CONTROLLER_CMD_CHAN_SELECT_REQ command to controller to send Channel Selection Request frame.
 *
 * Return: 0 on success, otherwise error code.
 */
static int siwifi_easymesh_ioctl_send_channel_selection_request(char *al_mac, char *radio_id)
{
    struct sfcfghdr hdr;
    struct iwreq wrq;

    /* Fill hdr. */
    memset(&hdr, 0, sizeof(struct sfcfghdr));
    hdr.comand_type = SF_CONTROLLER_CMD_CHAN_SELECT_REQ;
    hdr.comand_id = SF_CONTROLLER_CMD_CHAN_SELECT_REQ;
    hdr.sequence = 1;

    /* Fill wrq. */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&hdr;

    hdr.length = sprintf(hdr.data, "%s %s", al_mac, radio_id);
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr.length;

    /* Send message to driver. */
    if (do_ioctl(SFCFG_PRIV_IOCTL_CONTROLLER, &wrq) < 0) {
        printf("IOCTL: Command SF_CONTROLLER_CMD_CHAN_SELECT_REQ failed\n");
        return -1;
    }

    return 0;
}

/*================================================= Agent Commands ======================================================*/
/**
 * siwifi_easymesh_ioctl_agent_dump_info - Process "dump_info" command.
 *
 * This function sends SF_AGENT_CMD_DUMP_INFO command to agent for dump dvice info.
 *
 * Return: 0 on success, otherwise error code.
 */
static int siwifi_easymesh_ioctl_agent_dump_info(void)
{
    struct sfcfghdr hdr;
    struct iwreq wrq;

    /* Fill hdr. */
    memset(&hdr, 0, sizeof(struct sfcfghdr));
    hdr.comand_type = SF_AGENT_CMD_DUMP_INFO;
    hdr.comand_id = SF_AGENT_CMD_DUMP_INFO;
    hdr.length = 0;
    hdr.sequence = 1;

    /* Fill wrq. */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&hdr;
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr.length;

    /* Send message to driver. */
    if (do_ioctl(SFCFG_PRIV_IOCTL_AGENT, &wrq) < 0) {
        printf("IOCTL: Command SF_AGENT_CMD_DUMP_INFO failed\n");
        return -1;
    }

    return 0;
}

/**
 * siwifi_easymesh_ioctl_up_down_notify - Process "up_down_notify" command.
 *
 * @param wan_lan: Specifies whether the event is related to the WAN or LAN interface.
 * @param updown: Indicates whether the interface is coming up or going down.
 * @param mac: The MAC address associated with the event.
 * @param ip: The IP address associated with the event.
 * @param port: The name of the Ethernet interface related to the event.
 *
 * This function sends SF_AGENT_CMD_UP_DOWN_NOTIFY command for notifying network up/down events.
 *
 * Return: 0 on success, otherwise error code.
 */
static int siwifi_easymesh_ioctl_up_down_notify(char *wan_lan, char *updown, char *mac,
                                                char *ip, char *port)
{
    struct sfcfghdr hdr;
    struct iwreq wrq;

    /* Fill hdr. */
    memset(&hdr, 0, sizeof(struct sfcfghdr));
    hdr.comand_type = SF_AGENT_CMD_UP_DOWN_NOTIFY;
    hdr.comand_id = SF_AGENT_CMD_UP_DOWN_NOTIFY;
    hdr.sequence = 1;

    /* Fill wrq. */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&hdr;

    hdr.length = sprintf(hdr.data, "%s %s %s %s %s", wan_lan, updown, mac, ip ? ip : "", port);
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr.length;

    /* Send message to driver. */
    if (do_ioctl(SFCFG_PRIV_IOCTL_AGENT, &wrq) < 0) {
        printf("IOCTL: Command SF_AGENT_CMD_UP_DOWN_NOTIFY failed\n");
        return -1;
    }

    return 0;
}

/**
 * siwifi_easymesh_ioctl_set_bind_status - Process "bind" command.
 *
 * @param bind_status: The status will be set.
 *
 * This function sends SF_AGENT_CMD_BIND command for updating the bind status of netlink.
 *
 * Return: 0 on success, otherwise error code.
 */
static int siwifi_easymesh_ioctl_set_bind_status(char *bind_status)
{
    struct sfcfghdr hdr;
    struct iwreq wrq;

    /* Fill hdr. */
    memset(&hdr, 0, sizeof(struct sfcfghdr));
    hdr.comand_type = SF_AGENT_CMD_BIND;
    hdr.comand_id = SF_AGENT_CMD_BIND;
    hdr.sequence = 1;

    /* Fill wrq. */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&hdr;

    hdr.length = sprintf(hdr.data, "%s", bind_status);
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr.length;

    /* Send message to driver. */
    if (do_ioctl(SFCFG_PRIV_IOCTL_AGENT, &wrq) < 0) {
        printf("IOCTL: Command SF_AGENT_CMD_BIND failed\n");
        return -1;
    }

    return 0;
}

/**
 * siwifi_easymesh_ioctl_set_discovery_status - Process "set_discovery" command.
 *
 * @param band: The band on which discovery should be set.
 * @param mode: The mode for discovery.
 *
 * This function sends SF_AGENT_CMD_SET_DISCOVERY command to enable or disable
 * discovery on the specified band.
 *
 * Return: 0 on success, otherwise error code.
 */
static int siwifi_easymesh_ioctl_set_discovery_status(char *band, char *status)
{
    struct sfcfghdr hdr;
    struct iwreq wrq;

    /* Fill hdr. */
    memset(&hdr, 0, sizeof(struct sfcfghdr));
    hdr.comand_type = SF_AGENT_CMD_SET_DISCOVERY;
    hdr.comand_id = SF_AGENT_CMD_SET_DISCOVERY;
    hdr.sequence = 1;

    /* Fill wrq. */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&hdr;

    hdr.length = sprintf(hdr.data, "%s %s", band, status);
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr.length;

    /* Send message to driver. */
    if (do_ioctl(SFCFG_PRIV_IOCTL_AGENT, &wrq) < 0) {
        printf("IOCTL: Command SF_AGENT_CMD_SET_DISCOVERY failed\n");
        return -1;
    }

    return 0;
}

/**
 * siwifi_easymesh_ioctl_get_status - Process "get_status" command.
 *
 * @param flag: The flag specifying which status to get.
 *
 * This function sends SF_AGENT_CMD_SET_DISCOVERY command to retrieve the
 * status based on the specified flag.
 *
 * Return: 0 on success, otherwise error code.
 */
static int siwifi_easymesh_ioctl_get_status(char *flag)
{
    struct sfcfghdr hdr, *tmp;
    struct iwreq wrq;
    uint8_t status;

    /* Fill hdr. */
    memset(&hdr, 0, sizeof(struct sfcfghdr));
    hdr.comand_type = SF_AGENT_CMD_GET_STATUS;
    hdr.comand_id = SF_AGENT_CMD_GET_STATUS;
    hdr.sequence = 1;

    /* Fill wrq. */
    memset(&wrq, 0, sizeof(struct iwreq));
    memcpy(&wrq.ifr_ifrn.ifrn_name, &ifname, sizeof(ifname));
    wrq.u.data.pointer = (caddr_t)&hdr;

    hdr.length = sprintf(hdr.data, "%s", flag);
    wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + hdr.length;

    /* Send message to driver. */
    if (do_ioctl(SFCFG_PRIV_IOCTL_AGENT, &wrq) < 0) {
        printf("IOCTL: Command SF_AGENT_CMD_GET_STATUS failed\n");
        return -1;
    }

    tmp = (struct sfcfghdr *)wrq.u.data.pointer;
    status = tmp->data[0];
    printf("status: %d\n", status);

    return 0;
}

static void usage(void)
{
    printf(
        "\nUsage:\n"
        "\tAgent commands:\n"
        "\t\teasymesh_ioctl Agent dump_info\n"
        "\t\teasymesh_ioctl Agent up_down_notify <wan/lan> <up/down> <mac> <ip> <eth name>\n"
        "\t\teasymesh_ioctl Agent bind <bind/unbind>\n"
        "\t\teasymesh_ioctl Agent set_discovery <lb/hb> <enable/disable>\n"
        "\t\teasymesh_ioctl Agent get_status <flag>\n"
        "\t\t\tAvailable flags:\n"
        "\t\t\t\t- discovery_enable_lb\n"
        "\t\t\t\t- discovery_enable_hb\n"
        "\t\t\t\t- autoconfig_completed_lb\n"
        "\t\t\t\t- autoconfig_completed_hb\n"
        "\t\t\t\t- agent_init_completed\n"
        "\t\t\t\t- nl_status\n\n"

        "\tController commands:\n"
        "\t\teasymesh_ioctl Controller dump_info [al_mac]\n"
        "\t\teasymesh_ioctl Controller renew [<fronthual ssid> <fronthaul key> <fronthaul encryption>]\n"
        "\t\teasymesh_ioctl Controller get_topo\n"
        "\t\teasymesh_ioctl Controller sendApCapaQuery <al_mac>\n"
        "\t\teasymesh_ioctl Controller sendCliCapaQuery <client_mac>\n"
        "\t\teasymesh_ioctl Controller sendBHSteerReq <al_mac> <bssid>\n"
        "\t\teasymesh_ioctl Controller sendChanPrefeReq <al_mac>\n"
        "\t\teasymesh_ioctl Controller sendChanScanReq <al_mac> <fresh_scan> <radio_num> radio:<radio_id>,<op_class_num>,"
        "<op_class1>,<chan_list_num>,<chan_list>,<op_class2>,<chan_list_num>,<chan_list>.. radio:<radio_id>..\n"
        "\t\teasymesh_ioctl Controller sendChanSelectReq <al_mac> <radio_mac>\n\n");
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        usage();
        return -1;
    }

    /* Fill interface name. */
    strcpy(ifname, argv[1]);

    if (!strcmp(argv[1], "Controller")) {
        if (!strcmp(argv[2], "dump_info")) {
            if (siwifi_easymesh_ioctl_controller_dump_info(argv[3])) {
                printf("siwifi_easymesh_ioctl_controller_dump_info() failed!\n");
                return -1;
            }
        } else if (!strcmp(argv[2], "renew")) {
            if (siwifi_easymesh_ioctl_renew(argv[3], argv[4], argv[5])) {
                printf("siwifi_easymesh_ioctl_renew() failed!\n");
                return -1;
            }
        } else if (!strcmp(argv[2], "get_topo")) {
            if (siwifi_easymesh_ioctl_get_topology()) {
                printf("siwifi_easymesh_ioctl_get_topology() failed!\n");
                return -1;
            }
        } else if (!strcmp(argv[2], "sendApCapaQuery") && argv[3]) {
            if (siwifi_easymesh_ioctl_send_ap_capability_query(argv[3])) {
                printf("siwifi_easymesh_ioctl_send_ap_capability_query() failed!\n");
                return -1;
            }
        } else if (!strcmp(argv[2], "sendCliCapaQuery") && argv[3]) {
            if (siwifi_easymesh_ioctl_send_client_capability_query(argv[3])) {
                printf("siwifi_easymesh_ioctl_send_client_capability_query() failed!\n");
                return -1;
            }
        } else if (!strcmp(argv[2], "sendBHSteerReq") && argv[3] && argv[4]) {
            if (siwifi_easymesh_ioctl_send_backhaul_steering_request(argv[3], argv[4])) {
                printf("siwifi_easymesh_ioctl_send_backhaul_steering_request() failed!\n");
                return -1;
            }
        } else if (!strcmp(argv[2], "sendChanPrefeReq") && argv[3]) {
            if (siwifi_easymesh_ioctl_send_channel_preference_request(argv[3])) {
                printf("siwifi_easymesh_ioctl_send_channel_preference_request() failed!\n");
                return -1;
            }
        } else if (!strcmp(argv[2], "sendChanScanReq") && argv[3]) {
            if (siwifi_easymesh_ioctl_send_channel_scan_request(argv + 3)) {
                printf("siwifi_easymesh_ioctl_send_channel_scan_request() failed!\n");
                return -1;
            }
        } else if (!strcmp(argv[2], "sendChanSelectReq") && argv[3] && argv[4]) {
            if (siwifi_easymesh_ioctl_send_channel_selection_request(argv[3], argv[4])) {
                printf("siwifi_easymesh_ioctl_send_channel_selection_request() failed!\n");
                return -1;
            }
        } else {
            printf("Incorrect usage\n");
            usage();
            return -1;
        }
    } else if (!strcmp(argv[1], "Agent")) {
        if (!strcmp(argv[2], "dump_info")) {
            if (siwifi_easymesh_ioctl_agent_dump_info()) {
                printf("siwifi_easymesh_ioctl_agent_dump_info() failed!\n");
                return -1;
            }
        } else if (!strcmp(argv[2], "up_down_notify") && argv[3] && argv[4] && argv[5] && argv[6] && argv[7]) {
            if (siwifi_easymesh_ioctl_up_down_notify(argv[3], argv[4], argv[5], argv[6], argv[7])) {
                printf("siwifi_easymesh_ioctl_up_down_notify() failed!\n");
                return -1;
            }
        } else if (!strcmp(argv[2], "bind") && argv[3]) {
            if (siwifi_easymesh_ioctl_set_bind_status(argv[3])) {
                printf("siwifi_easymesh_ioctl_set_bind_status() failed!\n");
                return -1;
            }
        } else if (!strcmp(argv[2], "set_discovery") && argv[3] && argv[4]) {
            if (siwifi_easymesh_ioctl_set_discovery_status(argv[3], argv[4])) {
                printf("siwifi_easymesh_ioctl_set_discovery_status() failed!\n");
                return -1;
            }
        } else if (!strcmp(argv[2], "get_status") && argv[3]) {
            if (siwifi_easymesh_ioctl_get_status(argv[3])) {
                printf("siwifi_easymesh_ioctl_get_status() failed!\n");
                return -1;
            }
        } else {
            printf("Incorrect usage\n");
            usage();
            return -1;
        }
    } else {
        printf("Invalid interface name %s\n", argv[1]);
        usage();
        return -1;
    }

    close_ioctl_socket();
    return 0;
}
