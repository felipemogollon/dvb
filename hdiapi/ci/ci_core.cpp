#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <android/log.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "../com/com_api.h"
#include "ci_core.h"
#include "../include/hdi_config.h"

#define LOG_TAG "ci_core"

#define LOGD(...) \
    if( log_cfg_get(CC_LOG_MODULE_CI) & CC_LOG_CFG_DEBUG) { \
    __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__); } \

#define LOGE(...) \
    if( log_cfg_get(CC_LOG_MODULE_CI) & CC_LOG_CFG_ERROR) { \
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__); } \

/*
 *===============================================================
 *  Function Name:
 *  Function Description:
 *  Function Parameters:
 *===============================================================
 */
int dvbca_open(int adapter, int cadevice) {
    int fd = -1;
#ifndef _DISABLE_CI_
    char filename[100];

    sprintf(filename, "/dev/dvb/adapter%i/ca%i", adapter, cadevice);
    if ((fd = open(filename, O_RDWR)) < 0) {
        // if that failed, try a flat /dev structure
        sprintf(filename, "/dev/dvb%i.ca%i", adapter, cadevice);
        fd = open(filename, O_RDWR);
    }
#endif
    return fd;
}

/*
 *===============================================================
 *  Function Name:
 *  Function Description:
 *  Function Parameters:
 *===============================================================
 */
int dvbca_reset(int fd, int slot) {
#ifndef _DISABLE_CI_
    return ioctl(fd, CA_RESET, (1 << slot));
#else
    return -1;
#endif
}

/*
 *===============================================================
 *  Function Name:
 *  Function Description:
 *  Function Parameters:
 *===============================================================
 */
int dvbca_get_interface_type(int fd, int slot) {
#ifndef _DISABLE_CI_
    ca_slot_info_t info;

    info.num = slot;
    if (ioctl(fd, CA_GET_SLOT_INFO, &info)) {
        return -1;
    }

    if (info.type & CA_CI_LINK) {
        return DVBCA_INTERFACE_LINK;
    } else if (info.type & CA_CI) {
        return DVBCA_INTERFACE_HLCI;
    }
#endif
    return -1;
}

/*
 *===============================================================
 *  Function Name:
 *  Function Description:
 *  Function Parameters:
 *===============================================================
 */
int dvbca_get_cam_state(int fd, int slot) {
#ifndef _DISABLE_CI_
    ca_slot_info_t info;

    info.num = slot;
    if (ioctl(fd, CA_GET_SLOT_INFO, &info)) {
        LOGE("%s,error %s fd=0x%x\n", __FUNCTION__, strerror(errno), fd);
        return 0;
    }
    // LOGE("%s,info.flags = %d\n", __FUNCTION__, info.flags);
    if ((info.flags & CA_CI_MODULE_READY) == CA_CI_MODULE_READY) {
        return DVBCA_CAMSTATE_READY;
    } else {
        return DVBCA_CAMSTATE_MISSING;
    }
#else
    return 0;
#endif
}

/*
 *===============================================================
 *  Function Name:
 *  Function Description:
 *  Function Parameters:
 *===============================================================
 */
int dvbca_link_write(int fd, uint8_t slot, uint8_t connection_id, uint8_t *data, uint16_t data_length) {
#ifndef _DISABLE_CI_
#if 0
    uint8_t *buf = (uint8_t *) malloc(data_length);
    int idx = 0;
    if (buf == NULL) {
        return -1;
    }

    memcpy(buf, data, data_length);
    for (idx = 0; idx < data_length; idx++) {
        LOGD("0x%02x", buf[idx]);
    }

    int result = write(fd, buf, data_length);
    free(buf);
    return result;
#else
    uint8_t *buf = (uint8_t *) malloc(data_length + 1);
    ca_lpdu_write_t write_data;
    if (buf == NULL) {
        return -1;
    }

    write_data.wlen = data_length + 1;
    buf[0] = slot;
    //buf[1] = connection_id;
    memcpy(buf + 1, data, data_length);

    //memcpy(write_data.wdata, buf, data_length+1);
    write_data.wdata = buf;

    int result = ioctl(fd, CA_LPDU_WRITE, &write_data);
    //int result = write(fd, buf, data_length+1);
    free(buf);
    return result - 1;
#endif
#else
    return -1;
#endif
}

/*
 *===============================================================
 *  Function Name:
 *  Function Description:
 *  Function Parameters:
 *===============================================================
 */
int dvbca_link_read(int fd, uint8_t *slot, uint8_t *connection_id, uint8_t *data, uint16_t data_length) {
#ifndef _DISABLE_CI_
    int size;

    uint8_t *buf = (uint8_t *) malloc(data_length + 1);
    ca_lpdu_read_t read_data;
    if (buf == NULL) {
        return -1;
    }
    read_data.rlen = data_length;
    read_data.rdata = buf;
    int result = ioctl(fd, CA_LPDU_READ, &read_data);

    // if ((size = read(fd, buf, data_length + 1)) < 2) {
    //return -1;
    //}

    *slot = read_data.rdata[0];
    //*connection_id = buf[1];
    memcpy(data, read_data.rdata + 1, result - 1);
    free(buf);

    return (result - 1);
#else
    return -1;
#endif
}
