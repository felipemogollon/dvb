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
#include "TTvHW_ci.h"

#include "../include/am_av.h"
#include "am_dmx.h"
#include "am_misc.h"
#include "../include/hdi_config.h"

#define LOG_TAG "ci_api"

#define LOGD(...) \
    if( log_cfg_get(CC_LOG_MODULE_CI) & CC_LOG_CFG_DEBUG) { \
    __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__); } \

#define LOGE(...) \
    if( log_cfg_get(CC_LOG_MODULE_CI) & CC_LOG_CFG_ERROR) { \
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__); } \

#define CI_GET_CNT      (300)/*test times is 33*/
#define DEFAULT_SLOT    (0)

#define DTV_DMX_DEV_NO (0)
#define DTV_AV_DEV_NO (0)

static int g_cam_fd = -1;
static int inited = 0;
static TAPI_U16 g_max_transbuf;
static pthread_mutex_t ci_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
EN_DEVICE_DEMOD_TYPE Demod_Get_Type(void);

TAPI_U8 Tapi_Ci_Init(void) {
    pthread_mutex_lock(&ci_lock);
    LOGD("%s %d be called", __FUNCTION__, __LINE__);
    if (!inited) {
        g_cam_fd = dvbca_open(0, 0);
        if (g_cam_fd < 0) {
            LOGE("%s, dvb ca device open error: %s.\n", __FUNCTION__, strerror(errno));
            pthread_mutex_unlock(&ci_lock);
            return CI0_NOT_INIT;
        }
        LOGD("%s be called fd=0x%x", __FUNCTION__, g_cam_fd);
        inited = 1;
    } else {
        LOGE("%s be called >= 2 times.\n", __FUNCTION__);
    }
    pthread_mutex_unlock(&ci_lock);
    return 0;
}
/*
 *===============================================================
 *  Function Name:
 *  Function Description:API will polling this function to check the card status
 *  Function Parameters:
 *===============================================================
 */
TAPI_U8 Tapi_Ci_PcmciaPolling(void) {
    int state = 0;

    pthread_mutex_lock(&ci_lock);

//    LOGD("%s %d be called.\n", __FUNCTION__, __LINE__);

    state = dvbca_get_cam_state(g_cam_fd, DEFAULT_SLOT);
//    LOGD("state is:%d.\n", state);

    pthread_mutex_unlock(&ci_lock);

    return state;
}

/*
 *===============================================================
 *  Function Name:
 *  Function Description:The Max trans buf length which the device can support
 *  Function Parameters:
 *===============================================================
 */
TAPI_S8 Tapi_Ci_InitCam(TAPI_U16 buf) {
    int idx = 0;

    pthread_mutex_lock(&ci_lock);

    //LOGD("%s %d be called", __FUNCTION__, __LINE__);

    if (dvbca_get_cam_state(g_cam_fd, DEFAULT_SLOT) == DVBCA_CAMSTATE_MISSING) {
        LOGE("%s, dvb ca device state missing.\n", __FUNCTION__);
        pthread_mutex_unlock(&ci_lock);
        return CI0_NOT_INIT;
    }
#if 0
    // reset it and wait
    dvbca_reset(g_cam_fd, DEFAULT_SLOT);

    //shall we using a thread to polling the state and then msg API?
    while (dvbca_get_cam_state(g_cam_fd, DEFAULT_SLOT) != DVBCA_CAMSTATE_READY) {
        if (idx++ > CI_GET_CNT) {
            break;
        }
        usleep(1000*100);
    }
#endif
    LOGD("%s CI_GET_CNT=%d", __FUNCTION__, idx);

    g_max_transbuf = buf;

    pthread_mutex_unlock(&ci_lock);

    return 1;
}

/*
 *===============================================================
 *  Function Name:
 *  Function Description:Set the data path of the TS, 
 *  Function Parameters:enable 0 CI should be work, while other value CI disabled
 *===============================================================
 */
TAPI_S8 Tapi_Ci_TsBypass(TAPI_U8 enable) {
    int ret1, ret2;
    EN_DEVICE_DEMOD_TYPE iDemoType = E_DEVICE_DEMOD_NULL;

    pthread_mutex_lock(&ci_lock);
    iDemoType = Demod_Get_Type();
    if (E_DEVICE_DEMOD_DTMB == iDemoType) {
#ifndef _USE_SERIAL_TS_PORT_ 
        ret1 = AM_DMX_SetSource(DTV_DMX_DEV_NO, AM_DMX_SRC_TS1);
        ret2 = AM_AV_SetTSSource(DTV_AV_DEV_NO, AM_AV_TS_SRC_TS1);
#else
        ret1 = AM_DMX_SetSource(DTV_DMX_DEV_NO, AM_DMX_SRC_TS0);
        ret2 = AM_AV_SetTSSource(DTV_AV_DEV_NO, AM_AV_TS_SRC_TS0);
#endif
    } else if (E_DEVICE_DEMOD_DVB_C == iDemoType) {
        if (enable) {
            AM_FileEcho("/sys/class/stb/tso_source", "ts2");
            ret1 = AM_DMX_SetSource(DTV_DMX_DEV_NO, AM_DMX_SRC_TS1);
            ret2 = AM_AV_SetTSSource(DTV_AV_DEV_NO, AM_AV_TS_SRC_TS1);
            LOGE("enable bypass.ret1 is %d ,ret2 is %d\n", ret1, ret2);
        } else {
            AM_FileEcho("/sys/class/stb/tso_source", "ts0");
            ret1 = AM_DMX_SetSource(DTV_DMX_DEV_NO, AM_DMX_SRC_TS2);
            ret2 = AM_AV_SetTSSource(DTV_AV_DEV_NO, AM_AV_TS_SRC_TS2);
            LOGE("disable bypass.ret1 is %d ,ret2 is %d\n", ret1, ret2);
        }
    }

    pthread_mutex_unlock(&ci_lock);
    return 0;
}

/*
 *===============================================================
 *  Function Name:
 *  Function Description:To check whether data availble or device can be write
 *  Function Parameters:return 1 data available to read, 2 device can be write
 *===============================================================
 */
TAPI_S8 Tapi_Ci_ReadCamStatus(void) {
    pthread_mutex_lock(&ci_lock);
    struct pollfd pfd;
    int s_read = -1, s_write = -1, ret;

//    LOGD("%s %d be called.\n", __FUNCTION__, __LINE__);

    if (g_cam_fd < 0) {
        LOGE("%s, g_cam_fd is < 0.\n", __FUNCTION__);
        pthread_mutex_unlock(&ci_lock);
        return -1;
    }

    pfd.fd = g_cam_fd;
    pfd.events = POLLIN;

    s_read = poll(&pfd, 1, 10); //block or nun-block?
    if (s_read != 1) {
        s_write = dvbca_get_cam_state(g_cam_fd, DEFAULT_SLOT);
    }

//    LOGD("%s %d, s_read:%d, s_write:%d.\n", __FUNCTION__, __LINE__, s_read, s_write);

    pthread_mutex_unlock(&ci_lock);

    if (s_read == 1) {
        return 1;
    }

    if (s_write == DVBCA_CAMSTATE_READY) {
        return 2;
    }

    return -1;
}

/*
 *===============================================================
 *  Function Name:
 *  Function Description:Read data from device
 *  Function Parameters:buf, data to be stored, len, the max length which device support
 *===============================================================
 */
TAPI_U16 Tapi_Ci_ReadCamData(TAPI_U8* buf, TAPI_U16 len) {
    int ret = -1;
    uint8_t slot, connection_id;
    pthread_mutex_lock(&ci_lock);

//    LOGD("%s %d, len:%d.\n", __FUNCTION__, __LINE__, len);

    if (g_cam_fd < 0) {
        LOGE("%s, g_cam_fd is < 0.\n", __FUNCTION__);
        pthread_mutex_unlock(&ci_lock);
        return -1;
    }

    ret = dvbca_link_read(g_cam_fd, &slot, &connection_id, buf, len);

    //LOGD("%s %d ret is:%d, slot:%d, connection_id:%d.\n", __FUNCTION__, __LINE__, ret, slot, connection_id);

    pthread_mutex_unlock(&ci_lock);
    return ret;
}

/*
 *===============================================================
 *  Function Name:
 *  Function Description:
 *  Function Parameters:
 *===============================================================
 */
TAPI_U8 Tapi_Ci_WriteCamData(TAPI_U8* buf, TAPI_U16 len) {
    int ret = -1, idx = 0;
    pthread_mutex_lock(&ci_lock);

//    LOGD("%s %d be called, len:%d", __FUNCTION__, __LINE__, len);

    if (g_cam_fd < 0) {
        LOGE("%s, g_cam_fd is < 0.\n", __FUNCTION__);
        pthread_mutex_unlock(&ci_lock);
        return -1;
    }

//    for (idx = 0; idx < len; idx++) {
//        LOGD("0x%02x", buf[idx]);
//    }

    ret = dvbca_link_write(g_cam_fd, DEFAULT_SLOT, buf[2], buf, len);

    //LOGD("%s %d be called ret is:%d.", __FUNCTION__, __LINE__, ret);

    pthread_mutex_unlock(&ci_lock);
    return ret;
}

/*
 *===============================================================
 *  Function Name:
 *  Function Description:
 *  Function Parameters:
 *===============================================================
 */
TAPI_U16 Tapi_Ci_GetCamBufSize(void) {
    TAPI_U16 tmp_ret = 0;

    pthread_mutex_lock(&ci_lock);

    tmp_ret = 0x200; //this is the size defined in 50221 in which will frag the data to packets

    pthread_mutex_unlock(&ci_lock);

    return tmp_ret;
}

/*
 *===============================================================
 *  Function Name:
 *  Function Description:
 *  Function Parameters:
 *===============================================================
 */
TAPI_U8 Tapi_Ci_InitPCMCIADriver(void) {
    pthread_mutex_lock(&ci_lock);

    LOGD("%s %d be called", __FUNCTION__, __LINE__);

    pthread_mutex_unlock(&ci_lock);
    return 0;
}

/*
 *===============================================================
 *  Function Name:
 *  Function Description:
 *  Function Parameters:
 *===============================================================
 */
TAPI_U8 Tapi_Ci_DetectCiModeleStatus(void) {
    pthread_mutex_lock(&ci_lock);

    LOGD("%s %d be called", __FUNCTION__, __LINE__);

    pthread_mutex_unlock(&ci_lock);
    return 0;
}

/*
 *===============================================================
 *  Function Name:
 *  Function Description:
 *  Function Parameters:
 *===============================================================
 */
TAPI_U8 Tapi_Ci_ReInitCiDriver(void) {
    pthread_mutex_lock(&ci_lock);

    LOGD("%s %d be called", __FUNCTION__, __LINE__);

    pthread_mutex_unlock(&ci_lock);
    return 0;
}
