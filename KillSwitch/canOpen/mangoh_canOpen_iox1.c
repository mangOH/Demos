#include "legato.h"
#include "can_defs.h"
#include "cop_api.h"
#include "cop_tcp.h"
#include "default.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <getopt.h>
#include <signal.h>
#include <errno.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <linux/can.h>

//#define __DEBUG__    1

#define NODE_ID            255
#define DEFAULT_NET        1
#define DEFAULT_NODE    1
#define BUFFER_LENGTH    1025
#define SEQUENCE_NO        1

#define DI0_DI7_CMD  "1 r 0x6000 1 u8"
#define DI8_DI15_CMD "1 r 0x6000 2 u8"
#define DO0_DO7_CMD  "1 w 0x6200 1 u8"

static int cnt = 1;


le_result_t mangoh_canOpenIox1_Init(void)
{
    long   baudrate = 3;
    long rc;
    struct _can_param can_param = {"can0", PF_CAN, SOCK_RAW, CAN_RAW};

    if((rc = cop_init(CAN_NETDEV, &can_param, (BYTE)baudrate)) != 0) {
        fprintf(stderr, "+++ error: cop_init = %li\n", rc);
        return LE_FAULT;
    }

    return LE_OK;
}

void mangoh_canOpenIox1_Free(void)
{
    cop_exit();
}

unsigned char mangoh_canOpenIox1_DigitalInput_DI0_DI7(void)
{
    unsigned char rt;
    char buf[64];
    struct _cop_tcp_settings settings = {DEFAULT_NET, DEFAULT_NODE, NODE_ID};

    snprintf(buf, sizeof(buf), "[%d] %s", cnt++, DI0_DI7_CMD);
#ifdef __DEBUG__
    printf("\t%s cmd: %s\n", __FUNCTION__, buf);
#endif
    cop_tcp_parse(&buf[0], &settings, &buf[0], sizeof(buf));
#ifdef __DEBUG__
    printf("\t%s: result:%s\n", __FUNCTION__, buf);
#endif
    rt = (unsigned char)strtol(buf+4, NULL, 0);

    return rt;
}

unsigned char mangoh_canOpenIox1_DigitalInput_DI8_DI15(void)
{
    int rt;
    char buf[64];
    struct _cop_tcp_settings settings = {DEFAULT_NET, DEFAULT_NODE, NODE_ID};

    snprintf(buf, sizeof(buf), "[%d] %s", cnt++, DI8_DI15_CMD);
#ifdef __DEBUG__
    printf("\t%s cmd: %s\n", __FUNCTION__, buf);
#endif
    cop_tcp_parse(&buf[0], &settings, &buf[0], sizeof(buf));
#ifdef __DEBUG__
    printf("\t%s: result:%s\n", __FUNCTION__, buf);
#endif
    rt = (unsigned char)strtol(buf+4, NULL, 0);

    return rt;
}

void mangoh_canOpenIox1_DigitalOutput_DO0_DO7(unsigned char value)
{
    char buf[64];
    struct _cop_tcp_settings settings = {DEFAULT_NET, DEFAULT_NODE, NODE_ID};

    snprintf(buf, sizeof(buf), "[%d] %s 0x%x", cnt++, DO0_DO7_CMD, value);
#ifdef __DEBUG__
    printf("\t%s cmd: %s\n", __FUNCTION__, buf);
#endif
    cop_tcp_parse(&buf[0], &settings, &buf[0], sizeof(buf));
#ifdef __DEBUG__
    printf("\t%s: result:%s\n", __FUNCTION__, buf);
#endif

    return;
}

COMPONENT_INIT
{
}
