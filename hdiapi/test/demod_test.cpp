#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <time.h>

#include <android/log.h>

#include "am_dmx.h"

#include "TTvHW_demodulator.h"

#define LOG_TAG "demod_test"
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__);
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__);

static void *demod_thread_main(void * arg);
static void FrontEnd_Get_STATUS(void);
static void FrontEnd_Init(void);
static void FrontEnd_Close(void);
static void FrontEnd_FullProcess(void);

static pthread_t mDemodTestThreadID = 0;

void demod_test_main(int argc, char** argv) {
    AM_DMX_OpenPara_t para;

    ALOGD("%s, entering...\n", __FUNCTION__);

    memset(&para, 0, sizeof(para));

    AM_DMX_Open(0, &para);
    AM_DMX_SetSource(0, AM_DMX_SRC_TS2);

    if (pthread_create(&mDemodTestThreadID, NULL, demod_thread_main, NULL)) { // error
        printf("%s, create thread error.\n", __FUNCTION__);
    }
}

static void *demod_thread_main(void * arg) {
    while (1) {
        FrontEnd_FullProcess();
    }

    return NULL;
}

static void FrontEnd_Get_STATUS(void) {
    TAPI_BOOL ret = TAPI_FALSE;

   // EN_FRONTEND_SIGNAL_CONDITION sigCON = E_FE_SIGNAL_NO;
    //sigCON = Tapi_Demod_GetSNR();
    //ALOGD("%d Tapi_Demod_GetSNR", sigCON);

    TAPI_U32 bitErr = 0;
    bitErr = Tapi_Demod_GetBER();
    ALOGD(" %ld Tapi_Demod_GetBER", bitErr);

    SIGNAL_STATUS sigStatus;
    ret = Tapi_Demod_GetSignalStatus(&sigStatus);
    ALOGD("%d --quality=%d;strength=%d  Tapi_Demod_GetSignalStatus", ret, sigStatus.SignalQuality, sigStatus.SignalStrength);

    DEMOD_SETTING demodSet;
    ret = Tapi_Demod_DVB_GetFrequency(&demodSet);
    ALOGD("%d --u32Frequency=%ld Tapi_Demod_DVB_GetFrequency", ret, demodSet.u32Frequency);
    ALOGD("%d --eBandWidth=%d Tapi_Demod_DVB_GetFrequency", ret, demodSet.eBandWidth);
    ALOGD("%d --uSymRate=%ld Tapi_Demod_DVB_GetFrequency", ret, demodSet.uSymRate);
    ALOGD("%d --eQAM=%d Tapi_Demod_DVB_GetFrequency", ret, demodSet.eQAM);
}

static void FrontEnd_Init(void) {
    TAPI_BOOL ret = TAPI_FALSE;

//  ret = Tapi_Tuner_Init();
//	ALOGD(" %d Tapi_Tuner_Init",ret);

    ret = Tapi_Demod_Initialization();
    ALOGD("%d Tapi_Demod_Initialization", ret);

//	ret = Tapi_Tuner_Connect();
    // ALOGD(" %d Tapi_Tuner_Connect",ret);

    ret = Tapi_Demod_Connect(E_DEVICE_DEMOD_DVB_C);
    ALOGD("%d Tapi_Demod_Connect", ret);
}

static void FrontEnd_Close(void) {
    TAPI_BOOL ret = TAPI_FALSE;

    //ret = Tapi_Tuner_Disconnect();
    //ALOGD("%d Tapi_Tuner_Disconnect",ret);  

    ret = Tapi_Demod_Disconnect();
    ALOGD("%d\n Tapi_DEMOD_Disconnect", ret);
}

static void FrontEnd_FullProcess(void) {
    TAPI_BOOL ret = TAPI_FALSE;

    FrontEnd_Init();
    usleep(100000);

    //ret = Tapi_Tuner_DTV_SetTune(435000000,E_RF_CH_BAND_8MHz,E_TUNER_DTV_DVB_C_MODE);
    //ALOGD("%d Tapi_Tuner_DTV_SetTune",ret);

    DEMOD_SETTING demoSet;
    demoSet.u32Frequency = 435000;
    //demoSet.eBandWidth = E_RF_CH_BAND_8MHz;
    demoSet.uSymRate = 6875;
    demoSet.eQAM = E_CAB_QAM64;

    ret = Tapi_Demod_DVB_SetFrequency(demoSet);
    ALOGD("%d Tapi_Demod_DVB_SetFrequency", ret);
    usleep(20000);

    FrontEnd_Get_STATUS();
    usleep(20000);

    //reset Demod,then get status
    FrontEnd_Close();
    Tapi_Demod_Reset(); //MTK:no need to do demod reset befroe get status
    ALOGD("Tapi_Demod_Reset");
    usleep(20000);

    demoSet.u32Frequency = 435000;
    demoSet.eBandWidth = E_RF_CH_BAND_8MHz;
    demoSet.uSymRate = 6875;
    demoSet.eQAM = E_CAB_QAM64;
    ret = Tapi_Demod_DVB_SetFrequency(demoSet);
    ALOGD("%d Tapi_Demod_DVB_SetFrequency", ret);

    usleep(20000);

    FrontEnd_Get_STATUS(); //after reset to get status
    usleep(20000);
}
