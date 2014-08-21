#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include <android/log.h>

#include "TTvHW_ci.h"

#define LOG_TAG "ci_test"
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__);
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__);

#define MW_TRUE 1
#define MW_FALSE 0
#define INIT_OK 1
#define ENABLE_READ 1
#define ENABLE_WRITE 2

#define  Creat_T_C 4303487233

typedef void (*delay_fun)(int);

static unsigned char buf[256];
static unsigned char send_buf[100];

/* [0]: Total length
 [1]: Transport connection ID (LPDU)
 [2]: More/Last indicator (LPDU)
 [3]: TPDU tag (TPDU)
 [4]: TPDU length field (TPDU)
 [5]: t_c_id (TPDU)
 [6]: session_number_tag (SPDU)
 [7]: length_field (SPDU)
 [8]: session_nb (MS) (SPDU)
 [9]: session_nb (LS) (SPDU) */
static unsigned char au1MsgXmt[5][20] = {
//
        { 5, 0x01, 0x00, 0x82, 0x01, 0x01 }, //
        { 5, 0x01, 0x00, 0x81, 0x01, 0x01 }, //
        { 14, 0x01, 0x00, 0xA0, 0x0A, 0x01, 0x92, 0x07, 0x00, 0x00, 0x01, 0x00, 0x41, 0x00, 0x01 }, //
        { 13, 0x01, 0x00, 0xA0, 0x09, 0x01, 0x90, 0x02, 0x00, 0x01, 0x9F, 0x80, 0x10, 0x00 }, //
        { 5, 0x01, 0x00, 0x81, 0x01, 0x01 }, //
        };

static void *ci_thread_main(void * arg);
static void DelayMs(int ms);

static pthread_t mCiTestThreadID = 0;

void ci_test_main(int argc, char** argv) {
    ALOGD("%s, entering...\n", __FUNCTION__);

    if (pthread_create(&mCiTestThreadID, NULL, ci_thread_main, NULL)) { // error
        ALOGD("%s, create thread error.\n", __FUNCTION__);
    }
}

static void DelayMs(int ms) {
    usleep(ms * 1000);
}

static void *ci_thread_main(void * arg) {
    int card_insert_news = 0;
    int card_insert = 0;
    int start_protocol = 0;
    int mw_ci_init_ok = MW_FALSE;
    int state = 0;
    int CIDAStatus;
    int i = 0, j = 0;
    int tmp;
    int *pRSizeLen;
    int len = 0;
    int buf_sz = 1024;
    int ret = 100;
    int idx = 0;

    ALOGD("%s, Test begin...\n", __FUNCTION__);
    for (i = 0; i < 256; i++) {
        buf[i] = 0;
    }

    ALOGD("%s, card_insert is:%d, card_insert_news is:%d.\n", __FUNCTION__, card_insert, card_insert_news);
    ret = Tapi_Ci_InitCam(100);
    ALOGD("%s, The init cam result is:%d.\n", __FUNCTION__, ret);

    while (j < 5) {
        DelayMs(10);

        // check card if insert
        ret = Tapi_Ci_PcmciaPolling();
        ALOGD("%s, The polling ret is:%d.\n", __FUNCTION__, ret);
        if (ret == 2) {
            card_insert_news = 1;
        } else {
            card_insert_news = 0;
        }

        ALOGD("%s, card_insert_news = %u\n", __FUNCTION__, card_insert_news);

        if (card_insert != card_insert_news) {
            card_insert = card_insert_news;
            if (card_insert) {
                if (1) //(INIT_OK == Tapi_Ci_InitCam(buf_sz))
                {
                    ALOGD("%s, test cam init ok.\n", __FUNCTION__);
                    mw_ci_init_ok = MW_TRUE;
                    start_protocol = 1;
                } else {
                    ALOGD("%s, cam init fail.\n", __FUNCTION__);
                }
            } else { // card remove
                ALOGD("%s, test cam remove.\n", __FUNCTION__);

                mw_ci_init_ok = MW_FALSE;
                start_protocol = 0;
            }
        }

        if (MW_FALSE == mw_ci_init_ok) {
            continue;
        }

        int i4Return;
        ALOGD("%s, test1 start_protocol = %u \n", __FUNCTION__, start_protocol);
        if (start_protocol) {
            if (1 == start_protocol) {
                if (ENABLE_WRITE == Tapi_Ci_ReadCamStatus()) {
                    // create a transport connect
                    for (idx = 0; idx < 100; idx++) {
                        send_buf[idx] = 0;
                    }
                    ALOGD("%s, ========= we have to send %d data ================\n", __FUNCTION__, au1MsgXmt[j][0]);
                    for (idx = 0; idx < au1MsgXmt[j][0]; idx++) {
                        send_buf[idx] = au1MsgXmt[j][idx + 1];
                        ALOGD("%s, 0x%02x  ", __FUNCTION__, send_buf[idx]);
                        if (idx % 7 == 0 && idx != 0) {
                            ALOGD("%s, \n", __FUNCTION__);
                        }
                    }
                    state = Tapi_Ci_WriteCamData(send_buf, (int) au1MsgXmt[j][0]);
                    if (0 == state) {
                        ALOGD("%s, Write Cam Data fail.\n", __FUNCTION__);
                    } else {
                        ++j;
                        ALOGD("%s, Write Cam Data sucess.\n", __FUNCTION__);
                    }
                }
                //start_protocol = 2;
            } else {
                start_protocol = 0;
                continue;
            }
        }

        ALOGD("..............test2 start_protocol = %u \n", start_protocol);
        DelayMs(20);
        if (ENABLE_READ == Tapi_Ci_ReadCamStatus()) {
            len = Tapi_Ci_GetCamBufSize();
            ALOGD("%s, Tapi_Ci_GetCamBufSize size of  Buf = %hu.\n", __FUNCTION__, len);
            state = Tapi_Ci_ReadCamData(&buf[0], 0x200);

            tmp = state;
            if (0 == state) {
                ALOGD("%s, Read Cam Data fail.\n", __FUNCTION__);
            } else {
                ALOGD("%s, ========= we have received %d data ================\n", __FUNCTION__, tmp);
                for (i = 0; i < tmp; ++i) {
                    ALOGD("%02x ", buf[i]);
                    if (i % 7 == 0 && i != 0)
                        ALOGD("\n");
                }

                ALOGD("%s, Read Cam Data sucess.\n", __FUNCTION__);
            }
        }
    }

    ALOGD("%s, test success.\n", __FUNCTION__);

    return NULL;
}
