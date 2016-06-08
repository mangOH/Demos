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
#ifdef __DEBUG__
#define PRINT_DEBUG(_fs_, ...) printf(_fs_, __VA_ARGS__)
#else
#define PRINT_DEBUG(_fs_, ...)
#endif

#define NODE_ID            255
#define DEFAULT_NET        1
#define DEFAULT_NODE    1
#define BUFFER_LENGTH    1025
#define SEQUENCE_NO        1

#define DI0_DI7_CMD  "1 r 0x6000 1 u8"
#define DI8_DI15_CMD "1 r 0x6000 2 u8"
#define DO0_DO7_CMD  "1 w 0x6200 1 u8"

static int cnt = 1;

static unsigned char genericDigitalInput(const char* cmd);


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
    return genericDigitalInput(DI0_DI7_CMD);
}

unsigned char mangoh_canOpenIox1_DigitalInput_DI8_DI15(void)
{
    return genericDigitalInput(DI8_DI15_CMD);
}

void mangoh_canOpenIox1_DigitalOutput_DO0_DO7(unsigned char value)
{
    char buf[64];
    struct _cop_tcp_settings settings = {DEFAULT_NET, DEFAULT_NODE, NODE_ID};

    snprintf(buf, sizeof(buf), "[%d] %s 0x%x", cnt++, DO0_DO7_CMD, value);
    PRINT_DEBUG("\t%s cmd: %s\n", __FUNCTION__, buf);
    cop_tcp_parse(&buf[0], &settings, &buf[0], sizeof(buf));
    PRINT_DEBUG("\t%s: result:%s\n", __FUNCTION__, buf);

    return;
}

static unsigned char genericDigitalInput(const char* cmd)
{
    unsigned char rt;
    char* token;
    const char* search = " ";
    char buf[32];
    struct _cop_tcp_settings settings = {DEFAULT_NET, DEFAULT_NODE, NODE_ID};

    while (true)
    {
        snprintf(buf, sizeof(buf), "[%d] %s", cnt++, cmd);
        PRINT_DEBUG("\t%s cmd: %s\n", __FUNCTION__, buf);
        cop_tcp_parse(&buf[0], &settings, &buf[0], sizeof(buf));
        PRINT_DEBUG("\t%s: result:%s\n", __FUNCTION__, buf);
        token = strtok(buf, search);
        PRINT_DEBUG("token 1 = %s\n", token);
        token = strtok(NULL, search);
        PRINT_DEBUG("token 2 = %s, %c\n", token, token[0]);
        if (token[0] == 'E')
        {
            PRINT_DEBUG("Got error result, try read again!\n");
        }
        else
        {
            break;
        }
    }
    rt = (unsigned char)strtol(token, NULL, 0);

    return rt;
}

COMPONENT_INIT
{
}
