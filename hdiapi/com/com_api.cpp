#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <android/log.h>

#include "com_api.h"

#define LOG_TAG "com_api"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__);
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__);

TAPI_BOOL Tapi_Driver_Init(void) {
    return TAPI_FALSE;
}

static int gHdiLogCfgBuf[CC_LOG_MODULE_MAX] = {
//
        CC_LOG_CFG_DEF, CC_LOG_CFG_DEF, CC_LOG_CFG_DEF, CC_LOG_CFG_DEF, ///
        CC_LOG_CFG_DEF, CC_LOG_CFG_DEF, CC_LOG_CFG_DEF, CC_LOG_CFG_DEF, //
        CC_LOG_CFG_DEF, CC_LOG_CFG_DEF, CC_LOG_CFG_DEF, CC_LOG_CFG_DEF, //
        CC_LOG_CFG_DEF, CC_LOG_CFG_DEF, CC_LOG_CFG_DEF, CC_LOG_CFG_DEF, //
        CC_LOG_CFG_DEF, CC_LOG_CFG_DEF, CC_LOG_CFG_DEF, CC_LOG_CFG_DEF, ///
        CC_LOG_CFG_DEF, CC_LOG_CFG_DEF, CC_LOG_CFG_DEF, CC_LOG_CFG_DEF, //
        CC_LOG_CFG_DEF, CC_LOG_CFG_DEF, CC_LOG_CFG_DEF, CC_LOG_CFG_DEF, //
        CC_LOG_CFG_DEF, CC_LOG_CFG_DEF, CC_LOG_CFG_DEF, CC_LOG_CFG_DEF, //
        };

static pthread_mutex_t logcfg_set_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t logcfg_get_mutex = PTHREAD_MUTEX_INITIALIZER;

int log_cfg_get(int log_cfg_module) {
    int tmp_log_cfg_val = CC_LOG_CFG_ALL;

    pthread_mutex_lock(&logcfg_set_mutex);

    if (log_cfg_module >= 0 && log_cfg_module < CC_LOG_MODULE_MAX) {
        tmp_log_cfg_val = gHdiLogCfgBuf[log_cfg_module];
    }

    pthread_mutex_unlock(&logcfg_set_mutex);

    return tmp_log_cfg_val;
}

int log_cfg_set(int log_cfg_module, unsigned int cfg_val) {
    int tmp_log_cfg_val = CC_LOG_CFG_ALL;

    pthread_mutex_lock(&logcfg_get_mutex);

    tmp_log_cfg_val = gHdiLogCfgBuf[log_cfg_module];

    if (log_cfg_module >= 0 && log_cfg_module < CC_LOG_MODULE_MAX) {
        gHdiLogCfgBuf[log_cfg_module] = cfg_val;
    }

    pthread_mutex_unlock(&logcfg_get_mutex);

    return tmp_log_cfg_val;
}
