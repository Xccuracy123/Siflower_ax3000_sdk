/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  This is for the advance feature for wireless devices:
 *                  1. Record the sta sending probe/assoc req to us, and control the priority of 5g
 *                  connection according to whether to reply to assoc response.
 *                  2. Determine whether to switch STA to another band based on the signal strength.
 *                  3. This service will be acted as a UBUS server and expose interfaces to all client.
 *
 *        Version:  1.0
 *        Created:  2023年06月08日 09时33分28秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ming.guang , ming.guang@siflower.com.cn
 *        Company:  Siflower
 *
 * =====================================================================================
 */

#include <unistd.h>
#include <signal.h>
#include "libubus.h"
#include "band_steering.h"

struct ubus_context *ubus_ctx;

static void server_main()
{
    int ret;
    openlog("band_steering", 0, LOG_DAEMON);
    LOG("Start band_steering service.\n");

    ret = band_steering_server_init(ubus_ctx);
    if (ret) {
        LOG("band_steering service init failed!\n");
        return;
    }

    siwifi_sta_list_init();
    roam_timer_init();

    uloop_run();
}

int main()
{
    const char *ubus_socket = NULL;

    uloop_init();

    ubus_ctx = ubus_connect(ubus_socket);
    if (!ubus_ctx) {
        fprintf(stderr, "Failed to connect to ubus\n");
        return -1;
    }

    ubus_add_uloop(ubus_ctx);

    server_main();

    LOG("Stop band_steering service.\n");
    ubus_free(ubus_ctx);
    uloop_done();

    roam_timer_deinit();
    siwifi_sta_list_deinit();

    return 0;
}

