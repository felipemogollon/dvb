#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <android/log.h>
#include <assert.h>
#include <am_time.h>
#include <am_thread.h>

#include <unistd.h>

#include "am_dmx.h"

#include "../com/com_api.h"
#include "demux_core.h"
#include "../include/hdi_config.h"

#define LOG_TAG "demux_core"

#if 0
#define LOGD(...) 
#define LOGE(...) 

#else

#define LOGD(...) \
    if( log_cfg_get(CC_LOG_MODULE_DEMUX) & CC_LOG_CFG_DEBUG) { \
    __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__); } 

#define LOGE(...) \
    if( log_cfg_get(CC_LOG_MODULE_DEMUX) & CC_LOG_CFG_ERROR) { \
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__); } 

#endif

#define AM_ASSERT   assert

#define _NOT_USE_PES_FILTER_   1

#define DMX_DEV_NO (0)
#define AM_SI_PID_EIT	(0x12)
#define AM_DMX_BUFFER_SIZE    (32*1024)
#define AM_DMX_BUFFER_SIZE_EIT_SCHE   (128*1024)
#define AM_SI_PID_TDT	(0x14)
#define AM_SI_PID_TOT	(0x14)
#define AM_SI_TID_PMT			(0x2)
#define AM_SI_TID_TDT			(0x70)

#if 0
#define DEBUG(a...)\
    do{\
        LOGE(a);\
    }while(0)

#define DEBUG_TIME()\
    do{\
        int clk;\
        AM_TIME_GetClock(&clk);\
        DEBUG("%dms ",clk);\
    }while(0)
#else
#define DEBUG(a...)
#define DEBUG_TIME(a...)
#endif

//////////////////////////////////////////////////////////////
//   declare
//////////////////////////////////////////////////////////////
#define  SERIALNUMBER_MAX 256
#define  MAX_SAVE_EVENT_NUMBER 10
//////////////////////////////////////////////////////////////
//definiation a list struct to maintain filter buf queue to 
//satisfy customer use case
///////////////////////////////////////////////////////////////
/*cbuf*/
typedef struct {
    TAPI_U8 *data;
    ssize_t size;
    ssize_t pread;
    ssize_t pwrite;
    int error;

    pthread_cond_t cond;
} hdi_dmx_cbuf_t;

typedef struct am_demux_filter_psi_info_s {
    DMX_NOTIFY_INFO_PSI_T notify_info_psi;
    am_demux_filter_psi_info_s *p_next;
    TAPI_U8* buf; //buf         
} am_demux_filter_psi_info_t;

typedef struct {
    DMX_FILTER_TYPE type;
    TAPI_S16 pid;
    DMX_PID_TYPE_T pid_type;
    TAPI_S8 index; //user assigned index of filter
    DMX_FILTER_PARAM hdi_dmx_filter_param; //filter parameter for customer
    DMX_FILTER_STATUS hdi_dmx_filter_status;
//data used by am_demux_filter
    int fid; //filter handle get from demux
    int dmx_dev_id; //dmx dev ID which filter belong to
    TAPI_BOOL used; //flag indicated filter used or not
    TAPI_BOOL enable; //flag indicate filter enable or nor
    union {
        struct dmx_sct_filter_params sec; //section filter pamameter
        struct dmx_pes_filter_params pes; //pes filter pamameter
    } params;

    struct timespec timestamp; //timestamp for check timeout
    TAPI_BOOL timeout_triggered; //TRUE,trigger to check timeout
    am_demux_filter_psi_info_t *p_first_psi_info; //psi_info buf list
    TAPI_U8 serialnumread; //indicated index of buf list application program complete copy
    TAPI_U8 serialnumwrite; //indicated index of buf copy from demux driver ready to app read
    hdi_dmx_cbuf_t cbuf;
} am_demux_filter_t;

typedef struct {
    int dmx_dev_id;
    int open_count;
    pthread_mutex_t lock;
} am_demux_device_t;
typedef struct {
    uint8_t table_id; /**< table_id*/
    uint8_t syntax_indicator; /**< section_syntax_indicator*/
    uint8_t private_indicator; /**< private_indicator*/
    uint16_t length; /**< section_length*/
    uint16_t extension; /**< table_id_extension*/
    /**< transport_stream_id for a
     PAT section */
    uint8_t version; /**< version_number*/
    uint8_t cur_next_indicator; /**< current_next_indicator*/
    uint8_t sec_num; /**< section_number*/
    uint8_t last_sec_num; /**< last_section_number*/
} AM_SI_SectionHeader_t;
//////////////////////////////////////////////////////////////
//   definition
//////////////////////////////////////////////////////////////

static int dmx_dev_id = DMX_DEV_NO;
static am_demux_filter_t g_am_demux_filter[SECTION_FILTER_NUM]; //global filter array
static am_demux_device_t am_demux_dev;

static TAPI_BOOL get_filter_by_index(const TAPI_S8 index, am_demux_filter_t **p_am_filter);
static int am_demux_compare_timespec(const struct timespec *ts1, const struct timespec *ts2);
static TAPI_BOOL am_timeout_expire(const TAPI_S8 index);
static void am_set_demux_status(const TAPI_S8 index, DMX_FILTER_STATUS filter_status);
static void am_filter_local_callback(int dev_no, int fid, const uint8_t *data, int len, void *user_data);
EN_DEVICE_DEMOD_TYPE Demod_Get_Type(void);
//////////////////////////////////////////////////////////////
//   implement API
//////////////////////////////////////////////////////////////
/*channel buffer operations*/
int dmx_cbuf_init(hdi_dmx_cbuf_t *cbuf, size_t len) {
    cbuf->pread = cbuf->pwrite = 0;
    cbuf->data = (TAPI_U8*) malloc(len);
    if (cbuf->data == NULL)
        return -1;
    cbuf->size = len;
    cbuf->error = 0;

    pthread_cond_init(&cbuf->cond, NULL);

    return 0;
}

int dmx_cbuf_deinit(hdi_dmx_cbuf_t *cbuf) {
    cbuf->pread = cbuf->pwrite = 0;
    if (cbuf->data != NULL)
        free(cbuf->data);
    cbuf->size = 0;
    cbuf->error = 0;

    pthread_cond_destroy(&cbuf->cond);

    return 0;
}

int dmx_cbuf_set_buf_size(hdi_dmx_cbuf_t *cbuf, ssize_t size) {
    if (size > cbuf->size) {
        if (cbuf->data != NULL)
            free(cbuf->data);
        cbuf->pread = cbuf->pwrite = 0;
        cbuf->data = (TAPI_U8*) malloc(size);
        if (cbuf->data == NULL)
            return -1;
        cbuf->size = size;
        cbuf->error = 0;
    }

    return 0;
}

int dmx_cbuf_empty(hdi_dmx_cbuf_t *cbuf) {
    return (cbuf->pread == cbuf->pwrite);
}

ssize_t dmx_cbuf_free(hdi_dmx_cbuf_t *cbuf) {
    ssize_t free;

    free = cbuf->pread - cbuf->pwrite;
    if (free <= 0)
        free += cbuf->size;

    return free - 1;
}

ssize_t dmx_cbuf_avail(hdi_dmx_cbuf_t *cbuf) {
    ssize_t avail;

    avail = cbuf->pwrite - cbuf->pread;
    if (avail < 0)
        avail += cbuf->size;

    return avail;
}

ssize_t dmx_cbuf_avail_from_offset(hdi_dmx_cbuf_t *cbuf, size_t offset) {
    ssize_t avail;

    avail = cbuf->pwrite - cbuf->pread;
    if (avail < 0)
        avail += cbuf->size;

    avail -= offset;
    avail = (avail > 0) ? avail : 0;

    return avail;
}

void dmx_cbuf_flush(hdi_dmx_cbuf_t *cbuf) {
    cbuf->pread = cbuf->pwrite;
    cbuf->error = 0;
}

void dmx_cbuf_reset(hdi_dmx_cbuf_t *cbuf) {
    cbuf->pread = cbuf->pwrite = 0;
    cbuf->error = 0;
}

void dmx_cbuf_read(hdi_dmx_cbuf_t *cbuf, TAPI_U8 *buf, size_t len) {
    size_t todo = len;
    size_t split;

    if (cbuf->data == NULL)
        return;
    split = ((ssize_t)(cbuf->pread + len) > cbuf->size) ? cbuf->size - cbuf->pread : 0;
    if (split > 0) {
        memcpy(buf, cbuf->data + cbuf->pread, split);
        buf += split;
        todo -= split;
        cbuf->pread = 0;
    }
    memcpy(buf, cbuf->data + cbuf->pread, todo);

    cbuf->pread = (cbuf->pread + todo) % cbuf->size;
}

void dmx_cbuf_copy_data(hdi_dmx_cbuf_t *cbuf, size_t offset, TAPI_U8 *buf, size_t len) {
    size_t todo = len;
    size_t split, read;

    if (cbuf->data == NULL)
        return;
    read = cbuf->pread + offset;
    split = ((ssize_t)(read + len) > cbuf->size) ? cbuf->size - read : 0;
    if (split > 0) {
        memcpy(buf, cbuf->data + read, split);
        buf += split;
        todo -= split;
        read = 0;
    }
    memcpy(buf, cbuf->data + read, todo);
}

ssize_t dmx_cbuf_write(hdi_dmx_cbuf_t *cbuf, const TAPI_U8 *buf, size_t len) {
    size_t todo = len;
    size_t split;

    if (cbuf->data == NULL)
        return 0;
    split = ((ssize_t)(cbuf->pwrite + len) > cbuf->size) ? cbuf->size - cbuf->pwrite : 0;

    if (split > 0) {
        memcpy(cbuf->data + cbuf->pwrite, buf, split);
        buf += split;
        todo -= split;
        cbuf->pwrite = 0;
    }
    memcpy(cbuf->data + cbuf->pwrite, buf, todo);
    cbuf->pwrite = (cbuf->pwrite + todo) % cbuf->size;
    if (len > 0)
        pthread_cond_signal(&cbuf->cond);

    return len;
}

static TAPI_BOOL get_filter_by_index(const TAPI_S8 index, am_demux_filter_t **p_am_filter) {
    am_demux_filter_t *tmp_am_filter;

    if (index < 0 || index >= SECTION_FILTER_NUM) {
        LOGE("fail to get  filter index=%d", index);
        return TAPI_FALSE;
    }

    if (g_am_demux_filter[index].used) {
        *p_am_filter = &g_am_demux_filter[index];
        return TAPI_TRUE;
    } else {
        LOGE("fail to get  filter%d used=%d", index, g_am_demux_filter[index].used);
        *p_am_filter = NULL;
        return TAPI_FALSE;
    }
}

static int am_demux_compare_timespec(const struct timespec *ts1, const struct timespec *ts2) {
    assert(ts1 && ts2);

    if ((ts1->tv_sec > ts2->tv_sec) || ((ts1->tv_sec == ts2->tv_sec) && (ts1->tv_nsec > ts2->tv_nsec))) {
        return 1;
    }

    if ((ts1->tv_sec == ts2->tv_sec) && (ts1->tv_nsec == ts2->tv_nsec)) {
        return 0;
    }

    return -1;
}

static AM_ErrorCode_t si_get_section_header(const uint8_t *buf, AM_SI_SectionHeader_t *sec_header) {
    assert(buf && sec_header);

    sec_header->table_id = buf[0];
    sec_header->syntax_indicator = (buf[1] & 0x80) >> 7;
    sec_header->private_indicator = (buf[1] & 0x40) >> 6;
    sec_header->length = (((uint16_t)(buf[1] & 0x0f)) << 8) | (uint16_t) buf[2];
    sec_header->extension = ((uint16_t) buf[3] << 8) | (uint16_t) buf[4];
    sec_header->version = (buf[5] & 0x3e) >> 1;
    sec_header->cur_next_indicator = buf[5] & 0x1;
    sec_header->sec_num = buf[6];
    sec_header->last_sec_num = buf[7];

    return AM_SUCCESS;
}
/////////////////////////////////////////////////////////////
// am_timeout_expire
//check fitler assigned whether hit timeout 
//return :TAPI_TRUE-> expire timeout,TAPI_FALSE->timeout not happen
/////////////////////////////////////////////////////////////
static TAPI_BOOL am_timeout_expire(const TAPI_S8 index) {
    struct timespec now;
    int rel;
    am_demux_filter_t *am_dmx_filter_ptr = NULL;

    if (TAPI_FALSE == get_filter_by_index(index, &am_dmx_filter_ptr)) {
        return TAPI_FALSE;
    }

    AM_TIME_GetTimeSpec(&now);
    //cehck filter whether hit timeout to recv data from demux driver     
    rel = am_demux_compare_timespec(&am_dmx_filter_ptr->timestamp, &now);
    if (rel <= 0) {
        LOGE("%d filter timeout", (int)index);
        return TAPI_TRUE;
    } else {
        if (am_dmx_filter_ptr->hdi_dmx_filter_param.timeOut > 0) {
            am_dmx_filter_ptr->timeout_triggered = TAPI_FALSE; //TAPI_TRUE;
            AM_TIME_GetTimeSpecTimeout(am_dmx_filter_ptr->hdi_dmx_filter_param.timeOut, &am_dmx_filter_ptr->timestamp);
            return TAPI_FALSE;
        }
    }

    return TAPI_FALSE;
}

static void am_set_demux_status(const TAPI_S8 index, DMX_FILTER_STATUS filter_status) {
    am_demux_filter_t *am_dmx_filter_ptr = NULL;

    if (TAPI_FALSE == get_filter_by_index(index, &am_dmx_filter_ptr)) {
        return;
    }

    if (NULL == am_dmx_filter_ptr) {
        LOGE("fail to get  filter_index=%d", index);
        return;
    }
    am_dmx_filter_ptr->hdi_dmx_filter_status = filter_status;
}

//dump section data aid to debug demux
static void dump_bytes(int dev_no, int fid, const uint8_t *data, int len, void *user_data) {
    int i;

    printf("\n==============section:=================\n");
    for (i = 0; i < len; i++) {
        printf("%02x ", data[i]);
        if (((i + 1) % 16) == 0) {
            printf("\n");
        }
    }

    if ((i % 16) != 0) {
        printf("\n================================\n");
    }
}

static void dumpFilterInputParam(const DMX_FILTER_PARAM param) {
    LOGE("\n===========dump Filter[%d]: input paramter==============\n", param.u1FilterIndex);
    LOGE("u2Pid :0x%x\n", param.u2Pid);
    LOGE("ePidType :0x%x\n", param.ePidType);
    LOGE("pvNotifyTag :0x%x\n", param.pvNotifyTag);
    LOGE("timeOut :%d\n", param.timeOut);
    LOGE("u1Offset :%d\n", param.u1Offset);
    LOGE("fgCheckCrc :%d\n", param.fgCheckCrc);
    LOGE("Filter:\n");
    LOGE(
            "0x%4x,0x%4x,0x%4x,0x%4x,0x%4x,0x%4x,0x%4x,0x%4x,0x%4x,\n", param.au1Data[0], param.au1Data[1], param.au1Data[2], param.au1Data[3], param.au1Data[4], param.au1Data[5], param.au1Data[6], param.au1Data[7]);
    LOGE("===========end dump Filter[] input paramter==============\n");
}
/////////////////////////////////////////////////////////////
// am_filter_local_callback
//wrap interface for filter callback and real invoke customer
//callback in here
/////////////////////////////////////////////////////////////

static void am_filter_local_callback(int dev_no, int fid, const uint8_t *data, int len, void *user_data) {
    am_demux_filter_t *am_dmx_filter_ptr = (am_demux_filter_t*) user_data;
    am_demux_filter_psi_info_t *p_psi_info = NULL;
    int sec_len;
    DMX_NOTIFY_INFO_PSI_T notify_info_psi;

    if (!data) {
        LOGE("filter[%d]->local_callback: invalid section data get from filter[%d]", fid, fid);
        return;
    }

    pthread_mutex_lock(&am_demux_dev.lock);
    if (NULL == am_dmx_filter_ptr) {
        LOGE("filter[%d]->local_callback: Invalid param user_data in dmx callback", fid);
        goto done;
    }

    //check tid
    if ((am_dmx_filter_ptr->params.sec.filter.mask[0] & data[0]) != am_dmx_filter_ptr->params.sec.filter.filter[0]) {
        LOGE("filter[%d]->local_callback: tid is not right", fid);
        goto done;
    }
/*
    if (am_dmx_filter_ptr->pid == 0x12) {
        AM_SI_SectionHeader_t sh;

        DEBUG("filter[%d] callback srvid=%x srvmask=%x\n",
                am_dmx_filter_ptr->index,
                (__u16)(am_dmx_filter_ptr->params.sec.filter.filter[1]) << 8 | am_dmx_filter_ptr->params.sec.filter.filter[2],
                (__u16)(am_dmx_filter_ptr->params.sec.filter.mask[1]) << 8 | am_dmx_filter_ptr->params.sec.filter.mask[2]);
        si_get_section_header(data, &sh);
        DEBUG("tid:%2x sec_num:%2x last_sec_num:%2x version:%2x\n",sh.table_id,sh.sec_num,sh.last_sec_num,sh.version);DEBUG("\n");
    }
*/
    sec_len = ((uint16_t)(data[1] & 0x0f) << 8) | (uint16_t) data[2];
    sec_len += 3;
    if (sec_len != len) {
        LOGE("invalid section len get from filter[%d]", fid);
        am_set_demux_status(am_dmx_filter_ptr->index, DMX_FILTER_STATUS_ERROR);
        dmx_cbuf_reset(&am_dmx_filter_ptr->cbuf);
        goto done;
    }

    if (am_dmx_filter_ptr->params.sec.pid != AM_SI_PID_TDT) {
        if ((data[1] & 0x80) == 0) {
            LOGE("filter[%d]->local_callback: section_syntax_indicator=0", fid);
            goto done;
        }
        /*current section number > last section number*/
        if ((data[6] > data[7]) && (data[7] != 0)) {
           LOGE("filter[%d] callback: section number current > last", fid);
           goto done;
        }
    } else {
        /* for TDT/TOT section, the section_syntax_indicator bit must be 0 */
        if ((data[1] & 0x80) != 0) {
            LOGE("filter[%d]->local_callback: section_syntax_indicator=1", fid);
            goto done;
        }
    }

    if (NULL != am_dmx_filter_ptr->hdi_dmx_filter_param.pfnNotify) {
        //LOGE("filter[%d]->local_callback: invoking pid= 0x%x len=%d\n",fid,am_dmx_filter_ptr->pid,len);
        /*Copy data to cbuf*/
        int temp_size = dmx_cbuf_free(&am_dmx_filter_ptr->cbuf);
        if (temp_size < len) {
            LOGE("buffer overflow!!!!!,temp_size= %d  ,len=%d total=%d pid=%d", temp_size, len, am_dmx_filter_ptr->cbuf.size, am_dmx_filter_ptr->pid);
            am_set_demux_status(am_dmx_filter_ptr->index, DMX_FILTER_STATUS_DATA_OVER_FLOW);
            dmx_cbuf_reset(&am_dmx_filter_ptr->cbuf);
            goto done;
        }
        dmx_cbuf_write(&am_dmx_filter_ptr->cbuf, data, len);
        am_dmx_filter_ptr->serialnumwrite = (am_dmx_filter_ptr->serialnumwrite + 1) % SERIALNUMBER_MAX;

        notify_info_psi.u1SerialNumber = am_dmx_filter_ptr->serialnumwrite;
        notify_info_psi.u4SecAddr = (TAPI_U32) am_dmx_filter_ptr;
        notify_info_psi.u4SecLen = len;
        notify_info_psi.fgUpdateData = TAPI_FALSE;

        am_set_demux_status(am_dmx_filter_ptr->index, DMX_FILTER_STATUS_DATA_READY);
        if (am_dmx_filter_ptr->timeout_triggered == TAPI_TRUE) {
            if (TAPI_TRUE == am_timeout_expire(am_dmx_filter_ptr->index))
                am_set_demux_status(am_dmx_filter_ptr->index, DMX_FILTER_STATUS_STATUS_TIMEOUT); /*mark as timeout*/
        }
        am_dmx_filter_ptr->hdi_dmx_filter_param.pfnNotify(am_dmx_filter_ptr->index, am_dmx_filter_ptr->hdi_dmx_filter_status, (TAPI_S32)((void*) &notify_info_psi));
    }
    done: pthread_mutex_unlock(&am_demux_dev.lock);
}

/////////////////////////////////////////////////////////////////
//@Fuction name:am_demux_get_cuurentdevno
//@Description:get dmx dev number used at present
//@param <IN> :void
//@param <RET> :return current used dmx dev number
int am_demux_get_cuurentdevno(void) {
    return am_demux_dev.dmx_dev_id;
}

/////////////////////////////////////////////////////////////////
//@Fuction name:am_demux_set_currentdevno
//@Description:get dmx dev number used at present
//@param <IN> :dmx_devno-> dmx dev no to set
//@param <RET> :
void am_demux_set_currentdevno(const int dmx_devno) {
    am_demux_dev.dmx_dev_id = dmx_devno;
}

/////////////////////////////////////////////////////////////////
//@Fuction name:am_demux_init
//@Description:init demux,then we can set filter
//@param <IN> :void
//@param <RET>
// @ --0:sucess
// @ --other:fail
int am_demux_init(void) {
    pthread_mutexattr_t mta;
    static AM_DMX_OpenPara_t dmx_para;

    if (am_demux_dev.open_count > 0) {
        LOGE("AM_DMX_Open failed\n");
        return -1;
    }

    am_demux_set_currentdevno((int) DMX_DEV_NO);

    //open AM_DMX
    memset(&dmx_para, 0, sizeof(dmx_para));
    if (AM_DMX_Open(am_demux_get_cuurentdevno(), &dmx_para) != AM_SUCCESS) {
        LOGE("AM_DMX_Open failed\n");
        return -1;
    } else {

        EN_DEVICE_DEMOD_TYPE EDemodType = E_DEVICE_DEMOD_NULL;
        EDemodType = Demod_Get_Type();
        if (E_DEVICE_DEMOD_DTMB == EDemodType) {
#ifndef _USE_SERIAL_TS_PORT_ 
            AM_DMX_SetSource(DMX_DEV_NO, AM_DMX_SRC_TS1);
#else
            AM_DMX_SetSource(DMX_DEV_NO, AM_DMX_SRC_TS0);
#endif
        } else if (E_DEVICE_DEMOD_DVB_C == EDemodType) {
            AM_DMX_SetSource(DMX_DEV_NO, AM_DMX_SRC_TS2);
        }

    }

    //initialize the global filter array
    memset(&g_am_demux_filter, 0, sizeof(am_demux_filter_t) * SECTION_FILTER_NUM);
    for (int i = 0; i < SECTION_FILTER_NUM; i++) {
        g_am_demux_filter[i].type = DMX_FILTER_TYPE_VIDEO;
        g_am_demux_filter[i].pid_type = DMX_PID_TYPE_TS_RAW;
        g_am_demux_filter[i].hdi_dmx_filter_status = DMX_FILTER_STATUS_DATA_NOT_READY;
        g_am_demux_filter[i].hdi_dmx_filter_param.pfnNotify = NULL;
        g_am_demux_filter[i].hdi_dmx_filter_param.pvNotifyTag = NULL;
        g_am_demux_filter[i].fid = -1; //means the filter is not used
        g_am_demux_filter[i].dmx_dev_id = am_demux_get_cuurentdevno();
        g_am_demux_filter[i].p_first_psi_info = NULL;
        g_am_demux_filter[i].serialnumread = g_am_demux_filter[i].serialnumwrite = 0;
        g_am_demux_filter[i].index = i;
        g_am_demux_filter[i].used = TAPI_FALSE;
        g_am_demux_filter[i].enable = TAPI_FALSE;

        /*init cbuf*/
        dmx_cbuf_init(&g_am_demux_filter[i].cbuf, (size_t)AM_DMX_BUFFER_SIZE);
        if (g_am_demux_filter[i].cbuf.data == NULL) {
            LOGE("AM_DMX_Open init cbuf failed\n");
            return -1;
        }
    }

    pthread_mutexattr_init(&mta);
    pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&am_demux_dev.lock, &mta);
    pthread_mutexattr_destroy(&mta);

    am_demux_dev.open_count = 1;

    return 0;
}

/////////////////////////////////////////////////////////////////
//@Fuction name:am_demux_exit
//@Description:free demux
//@param <IN> :void
//@param <RET>
//@  --0:sucess
// @ --other:fail
int am_demux_exit(void) {

    if (am_demux_dev.open_count == 1) {
        if (AM_DMX_Close(am_demux_get_cuurentdevno()) != AM_SUCCESS) {
            LOGE("AM_DMX_Close failed\n");
            return -1;
        }
//free resource allocated
        pthread_mutex_lock(&am_demux_dev.lock);
        for (int i = 0; i < SECTION_FILTER_NUM; i++) {
            g_am_demux_filter[i].type = DMX_FILTER_TYPE_VIDEO;
            g_am_demux_filter[i].pid_type = DMX_PID_TYPE_TS_RAW;
            g_am_demux_filter[i].hdi_dmx_filter_status = DMX_FILTER_STATUS_DATA_NOT_READY;
            g_am_demux_filter[i].hdi_dmx_filter_param.pfnNotify = NULL;
            g_am_demux_filter[i].hdi_dmx_filter_param.pvNotifyTag = NULL;
            g_am_demux_filter[i].fid = -1; //means the filter is not used
            g_am_demux_filter[i].used = TAPI_FALSE;
            g_am_demux_filter[i].enable = TAPI_FALSE;
            g_am_demux_filter[i].dmx_dev_id = am_demux_get_cuurentdevno();
            //free buf list used by filter
            dmx_cbuf_deinit(&g_am_demux_filter[i].cbuf);
            memset(&g_am_demux_filter[i], 0, sizeof(am_demux_filter_t));
        }
        pthread_mutex_unlock(&am_demux_dev.lock);
        pthread_mutex_destroy(&am_demux_dev.lock);
    }
    am_demux_dev.open_count--;

    return 0;
}

//=================================================================================
//////////////////////////////////////////////////////////////////////////
//@Fuction name:am_demux_allocatefilter
//@Description:allocate a new filter
//@param <IN>
//@ ---type:DMX_FILTER_TYPE
//@ ---index:the index of filter assigned by writer
//@ ---outindex:the index of filter assigned by vendor,if not requred,let it same as index
//@---u16id: pid
//@param <RET>
//@--TRUE:sucess
//@--FALSE:fail
TAPI_BOOL Demod_GetLockStatus(void);
TAPI_BOOL am_demux_allocatefilter(DMX_FILTER_TYPE type, TAPI_S16 u16id, TAPI_S8 index, TAPI_S8* outindex) {
    int ret = -1;
    int fid;
    am_demux_filter_t *am_dmx_filter_ptr = NULL;
    int indexId;

    if (Demod_GetLockStatus() != TAPI_TRUE && u16id != AM_SI_PID_EIT){
        LOGE("not lock,fail to get  filter_index=%d", index);
        return TAPI_FALSE;
    }

    pthread_mutex_lock(&am_demux_dev.lock);

    for (indexId = 0; indexId < SECTION_FILTER_NUM; indexId++) {
        if (!g_am_demux_filter[indexId].used)
            break;
    }

    if (indexId >= SECTION_FILTER_NUM) {
        pthread_mutex_unlock(&am_demux_dev.lock);
        LOGE("max index,fail to get  filter_index=%d", index);
        return TAPI_FALSE;
    }

    am_dmx_filter_ptr = &g_am_demux_filter[indexId];

    if (am_dmx_filter_ptr->fid != -1) {
        ret = AM_DMX_StopFilter(am_dmx_filter_ptr->dmx_dev_id, am_dmx_filter_ptr->fid);
        if (ret != AM_SUCCESS) {
            LOGE("stop filter failure!  filter_index=%d", am_dmx_filter_ptr->fid);
        }
        ret = AM_DMX_FreeFilter(am_dmx_filter_ptr->dmx_dev_id, am_dmx_filter_ptr->fid);
        if (ret != AM_SUCCESS) {
            LOGE("free filter failure!  filter_index=%d", am_dmx_filter_ptr->fid);
        }
        pthread_mutex_unlock(&am_demux_dev.lock);
        AM_DMX_Sync(am_dmx_filter_ptr->dmx_dev_id);
        pthread_mutex_lock(&am_demux_dev.lock);
    }

    ret = AM_DMX_AllocateFilter(am_dmx_filter_ptr->dmx_dev_id, &fid);
    if (ret == AM_SUCCESS) {
        *outindex = indexId; //am_dmx_filter_ptr->fid;//customer prefer used the index them pass
        am_dmx_filter_ptr->type = type;
        //am_dmx_filter_ptr->index = indexId;
        am_dmx_filter_ptr->pid = u16id;
        am_dmx_filter_ptr->serialnumread = am_dmx_filter_ptr->serialnumwrite = 0;
        am_dmx_filter_ptr->hdi_dmx_filter_status = DMX_FILTER_STATUS_DATA_READY;
        am_dmx_filter_ptr->fid = fid;
        am_dmx_filter_ptr->used = TAPI_TRUE;
        am_dmx_filter_ptr->enable = TAPI_FALSE;
        DEBUG_TIME();DEBUG("=====set filter->fid=%d,input type=%d index=%d\n",am_dmx_filter_ptr->fid,type,indexId);
    } else {
        LOGE("allocate filter failure!");
    }
    pthread_mutex_unlock(&am_demux_dev.lock);
    return ret == AM_SUCCESS ? TAPI_TRUE : TAPI_FALSE;
}

///////////////////////////////////////////
//@Fuction name:am_demux_setsectionfilterparam
//@Description:set param to a section filter,it will be started after call fuction demux_start
//@param <IN>
//@ ---param:as to MTDMX_FILTER_PARAM
//@param <RET>
//@--TRUE:sucess
//@--FALSE:fail
TAPI_BOOL am_demux_setsectionfilterparam(TAPI_S8 index, DMX_FILTER_PARAM param) {
    int ret = -1;
    am_demux_filter_t *am_dmx_filter_ptr = NULL;
    TAPI_S8 input_index = param.u1FilterIndex;

    pthread_mutex_lock(&am_demux_dev.lock);
    if (TAPI_FALSE == get_filter_by_index(input_index, &am_dmx_filter_ptr)) {
        pthread_mutex_unlock(&am_demux_dev.lock);
        LOGE("fail to get  filter[%d]", input_index);
        return TAPI_FALSE;
    }

    //LOGE("&&&&&& get  index=%d,find filterID =%ld,filter_type=%d\n", input_index,am_dmx_filter_ptr->fid,am_dmx_filter_ptr->type);

    if (am_dmx_filter_ptr->fid != -1 && !am_dmx_filter_ptr->enable) {
        if (true/*am_dmx_filter_ptr->type == DMX_FILTER_TYPE_SECTION*/) {
            memset(&am_dmx_filter_ptr->params.sec, 0, sizeof(am_dmx_filter_ptr->params.sec));
            am_dmx_filter_ptr->params.sec.pid = param.u2Pid;
            //map amlogic sec filter and mask
            am_dmx_filter_ptr->params.sec.filter.filter[0] = param.au1Data[0];
            am_dmx_filter_ptr->params.sec.filter.mask[0] = param.au1Mask[0];
            memcpy(&am_dmx_filter_ptr->params.sec.filter.filter[1], &param.au1Data[3], sizeof(param.au1Data) - 3);
            memcpy(&am_dmx_filter_ptr->params.sec.filter.mask[1], &param.au1Mask[3], sizeof(param.au1Mask) - 3);

            // am_dmx_filter_ptr->params.sec.flags = param.fgCheckCrc ? DMX_CHECK_CRC : 0;
            if (param.u2Pid != AM_SI_PID_TDT)
                am_dmx_filter_ptr->params.sec.flags = DMX_CHECK_CRC;
            else
                am_dmx_filter_ptr->params.sec.flags = 0;

            am_dmx_filter_ptr->params.sec.timeout = param.timeOut;
            //LOGD("&&&&&&type=%d params.sec.pid =%d,params.sec.filter.filter[0]=%d\n",
            //     am_dmx_filter_ptr->type,
            //     am_dmx_filter_ptr->params.sec.pid,
            //     am_dmx_filter_ptr->params.sec.filter.filter[0]);
            ret = AM_DMX_SetSecFilter(am_dmx_filter_ptr->dmx_dev_id, am_dmx_filter_ptr->fid, &(am_dmx_filter_ptr->params.sec));
            if (ret != AM_SUCCESS) {
                LOGE("set filter parameter failure!  filter_index=%d", am_dmx_filter_ptr->fid);
            }

            ret = AM_DMX_SetCallback(am_dmx_filter_ptr->dmx_dev_id, am_dmx_filter_ptr->fid, am_filter_local_callback, (void*) am_dmx_filter_ptr);
            if (ret != AM_SUCCESS) {
                LOGE("set filter callback failure!");
            } else {
                if (param.pfnNotify != NULL) {
                    am_dmx_filter_ptr->hdi_dmx_filter_param.pfnNotify = param.pfnNotify;
                    if (param.pvNotifyTag != NULL) {
                        am_dmx_filter_ptr->hdi_dmx_filter_param.pvNotifyTag = param.pvNotifyTag;
                    }
                }
            }
        }
    }
    // if(ret != AM_SUCCESS) dumpFilterInputParam(param);
    pthread_mutex_unlock(&am_demux_dev.lock);
    return ret == AM_SUCCESS ? TAPI_TRUE : TAPI_FALSE;
}

//////////////////////////////////////////////////////////
//@Fuction name :am_demux_start
//@Description:start a filter after set param,
//@param <IN>
//@--index : same as outindex in the function demux_open(DMX_FILTER_TYPE type,U8 index,U8* outindex)
//@param <RET>
//@--TRUE:sucess
//@--FALSE:fail
TAPI_BOOL am_demux_start(TAPI_S8 index) {
    int ret = -1;
    am_demux_filter_t *am_dmx_filter_ptr = NULL;

    pthread_mutex_lock(&am_demux_dev.lock);
    if (TAPI_FALSE == get_filter_by_index(index, &am_dmx_filter_ptr)) {
        pthread_mutex_unlock(&am_demux_dev.lock);
        return TAPI_FALSE;
    }

    if (am_dmx_filter_ptr->fid != -1 && !am_dmx_filter_ptr->enable && am_dmx_filter_ptr->params.sec.pid <= 0x1fff) {
        if (am_dmx_filter_ptr->pid == AM_SI_PID_EIT && am_dmx_filter_ptr->params.sec.filter.filter[0] >= 0x50 && am_dmx_filter_ptr->params.sec.filter.filter[0] <= 0x6F) {
            dmx_cbuf_set_buf_size(&am_dmx_filter_ptr->cbuf, AM_DMX_BUFFER_SIZE_EIT_SCHE);
            ret = AM_DMX_SetBufferSize(am_dmx_filter_ptr->dmx_dev_id, am_dmx_filter_ptr->fid, AM_DMX_BUFFER_SIZE_EIT_SCHE);
        } else {
            dmx_cbuf_set_buf_size(&am_dmx_filter_ptr->cbuf, AM_DMX_BUFFER_SIZE);
            ret = AM_DMX_SetBufferSize(am_dmx_filter_ptr->dmx_dev_id, am_dmx_filter_ptr->fid, AM_DMX_BUFFER_SIZE);
        }
        if (ret != AM_SUCCESS) {
            LOGE("set demux buffer size failure!");
        }
        LOGD("filter[%d]start pid = %d tid = 0x%x\n",am_dmx_filter_ptr->index,am_dmx_filter_ptr->pid,am_dmx_filter_ptr->params.sec.filter.filter[0]);
        //LOGD("&&&&&& id = %d, fid = %d\n",am_dmx_filter_ptr->dmx_dev_id, am_dmx_filter_ptr->fid);
        ret = AM_DMX_StartFilter(am_dmx_filter_ptr->dmx_dev_id, am_dmx_filter_ptr->fid);
        if (ret != AM_SUCCESS) {
            LOGE("start demux filter failure!  filter_index=%d pid=%d", am_dmx_filter_ptr->fid, am_dmx_filter_ptr->pid);
        } else {
            am_dmx_filter_ptr->enable = TAPI_TRUE;
        }
        if (am_dmx_filter_ptr->hdi_dmx_filter_param.timeOut > 0) {
            am_dmx_filter_ptr->timeout_triggered = TAPI_FALSE; // TAPI_TRUE;
            ret = AM_TIME_GetTimeSpecTimeout(am_dmx_filter_ptr->hdi_dmx_filter_param.timeOut, &am_dmx_filter_ptr->timestamp);
        }
    } else {
        LOGE("am_demux_start  failed fid=%d enable=%d params.sec.pid =%d\n", am_dmx_filter_ptr->fid, am_dmx_filter_ptr->enable, am_dmx_filter_ptr->params.sec.pid);
    }
    pthread_mutex_unlock(&am_demux_dev.lock);
    return ret == AM_SUCCESS ? TAPI_TRUE : TAPI_FALSE;
}

/////////////////////////////////////
//@Fuction name : am_demux_stop(U8 index)
//@Description:stop a filter after set param,
//@param <IN>
//@--index : same as outindex in the function demux_open(DMX_FILTER_TYPE type,U8 index,U8* outindex)
//@param <RET>
//@--TRUE:sucess
//@--FALSE:fail
TAPI_BOOL am_demux_stop(TAPI_S8 index) {
    int ret = -1;
    am_demux_filter_t *am_dmx_filter_ptr = NULL;

    pthread_mutex_lock(&am_demux_dev.lock);
    if (TAPI_FALSE == get_filter_by_index(index, &am_dmx_filter_ptr)) {
        pthread_mutex_unlock(&am_demux_dev.lock);
        return TAPI_FALSE;
    }

    if (!am_dmx_filter_ptr->enable || !am_dmx_filter_ptr->used) {
        pthread_mutex_unlock(&am_demux_dev.lock);
        return TAPI_TRUE;
    }

    if (am_dmx_filter_ptr->fid != -1 && am_dmx_filter_ptr->enable) {
        DEBUG_TIME();DEBUG("filter[%d]stop pid = %d\n",am_dmx_filter_ptr->index,am_dmx_filter_ptr->pid);
        ret = AM_DMX_StopFilter(am_dmx_filter_ptr->dmx_dev_id, am_dmx_filter_ptr->fid);
        if (ret != AM_SUCCESS) {
            LOGE("stop filter failure!   filter_index=%d", am_dmx_filter_ptr->fid);
        } else {
            am_dmx_filter_ptr->enable = TAPI_FALSE;
        }
        if (am_dmx_filter_ptr->timeout_triggered == TAPI_TRUE) {
            am_dmx_filter_ptr->timeout_triggered = TAPI_FALSE;
        }
    }
    pthread_mutex_unlock(&am_demux_dev.lock);
    return ret == AM_SUCCESS ? TAPI_TRUE : TAPI_FALSE;;
}

/////////////////////////////////////
//@Fuction name : am_demux_Reset(U8 index)
//@Description:reset a filter after set param and start
//@param <IN>
//--index : same as outindex in the function demux_open(DMX_FILTER_TYPE type,U8 index,U8* outindex)
//@param <RET>
//@--TRUE:sucess
//@--FALSE:fail
TAPI_BOOL am_demux_reset(TAPI_S8 index) {
    int ret = 0;
    am_demux_filter_t *am_dmx_filter_ptr = NULL;

    pthread_mutex_lock(&am_demux_dev.lock);
    if (TAPI_FALSE == get_filter_by_index(index, &am_dmx_filter_ptr)) {
        pthread_mutex_unlock(&am_demux_dev.lock);
        return TAPI_FALSE;
    }

    if (am_dmx_filter_ptr->timeout_triggered == TAPI_TRUE) {
        am_dmx_filter_ptr->timeout_triggered = TAPI_FALSE;
    }

    dmx_cbuf_reset(&am_dmx_filter_ptr->cbuf);
    am_dmx_filter_ptr->serialnumread = am_dmx_filter_ptr->serialnumwrite = 0;

    pthread_mutex_unlock(&am_demux_dev.lock);
    return ret == 0 ? TAPI_TRUE : TAPI_FALSE;;
}

//////////////////////////////////////////////////
//@Fuction name : am_demux_restart(U8 index)
//@Description:restart a filter
//@param <IN>
//@--index : same as outindex in the function demux_open(DMX_FILTER_TYPE type,U8 index,U8* outindex)
//@param <RET>
//@--TRUE:sucess
//@--FALSE:fail
TAPI_BOOL am_demux_restart(TAPI_S8 index) {
    int ret = -1;
    am_demux_filter_t *am_dmx_filter_ptr = NULL;

    pthread_mutex_lock(&am_demux_dev.lock);
    if (TAPI_FALSE == get_filter_by_index(index, &am_dmx_filter_ptr)) {
        pthread_mutex_unlock(&am_demux_dev.lock);
        return TAPI_FALSE;
    }

    if (am_dmx_filter_ptr->fid != -1 && !am_dmx_filter_ptr->enable) {
        ret = AM_DMX_StopFilter(am_dmx_filter_ptr->dmx_dev_id, am_dmx_filter_ptr->fid);
        if (ret != AM_SUCCESS) {
            LOGE("stop filter failure!  filter_index=%d", am_dmx_filter_ptr->fid);
        } else {
            am_dmx_filter_ptr->enable = TAPI_FALSE;
        }
        dmx_cbuf_reset(&am_dmx_filter_ptr->cbuf);
        am_dmx_filter_ptr->serialnumread = am_dmx_filter_ptr->serialnumwrite = 0;
        //ret = AM_DMX_SetBufferSize(am_dmx_filter_ptr->dmx_dev_id,am_dmx_filter_ptr->fid,AM_DMX_BUFFER_SIZE);
        ret = AM_DMX_StartFilter(am_dmx_filter_ptr->dmx_dev_id, am_dmx_filter_ptr->fid);
        if (ret != AM_SUCCESS) {
            LOGE("re-start filter failure!   filter_index=%d", am_dmx_filter_ptr->fid);
        } else {
            am_dmx_filter_ptr->enable = TAPI_TRUE;
        }
        if (am_dmx_filter_ptr->hdi_dmx_filter_param.timeOut > 0) {
            am_dmx_filter_ptr->timeout_triggered = TAPI_FALSE; //TAPI_TRUE;
            ret = AM_TIME_GetTimeSpecTimeout(am_dmx_filter_ptr->hdi_dmx_filter_param.timeOut, &am_dmx_filter_ptr->timestamp);
        }
    }
    pthread_mutex_unlock(&am_demux_dev.lock);
    return ret == AM_SUCCESS ? TAPI_TRUE : TAPI_FALSE;
}

//////////////////////////////////////////////////
//@Fuction name : am_demux_close(U8 index)
//@Description:close filter
//@param <IN>
//@--index : same as outindex in the function demux_open(DMX_FILTER_TYPE type,U8 index,U8* outindex)
//@param <RET>
//@--TRUE:sucess
//@--FALSE:fail
TAPI_BOOL am_demux_close(TAPI_S8 index) {
    int ret = -1;
    am_demux_filter_t *am_dmx_filter_ptr = NULL;

    pthread_mutex_lock(&am_demux_dev.lock);
    if (TAPI_FALSE == get_filter_by_index(index, &am_dmx_filter_ptr)) {
        pthread_mutex_unlock(&am_demux_dev.lock);
        return TAPI_FALSE;
    }

    if (am_dmx_filter_ptr->fid != -1 && am_dmx_filter_ptr->used) {
        am_dmx_filter_ptr->enable = TAPI_FALSE;
        DEBUG_TIME();DEBUG("filter[%d]close pid = %d\n",am_dmx_filter_ptr->index,am_dmx_filter_ptr->pid);
        ret = AM_DMX_FreeFilter(am_dmx_filter_ptr->dmx_dev_id, am_dmx_filter_ptr->fid);
        if (ret != AM_SUCCESS) {
            LOGE("free filter failure!  filter_index=%d", am_dmx_filter_ptr->fid);
        }
        //free buf list used by filter
        dmx_cbuf_reset(&am_dmx_filter_ptr->cbuf);
        am_dmx_filter_ptr->used = TAPI_FALSE;
        am_dmx_filter_ptr->fid = -1;

        pthread_mutex_unlock(&am_demux_dev.lock);
        AM_DMX_Sync(am_dmx_filter_ptr->dmx_dev_id);
        pthread_mutex_lock(&am_demux_dev.lock);
        am_dmx_filter_ptr->serialnumread = am_dmx_filter_ptr->serialnumwrite = 0;
    }
    pthread_mutex_unlock(&am_demux_dev.lock);
    return ret == AM_SUCCESS ? TAPI_TRUE : TAPI_FALSE;
}

TAPI_BOOL am_demux_flushsectionbuffer(TAPI_S8 index) {
    am_demux_filter_t *am_dmx_filter_ptr = NULL;

    pthread_mutex_lock(&am_demux_dev.lock);
    if (TAPI_FALSE == get_filter_by_index(index, &am_dmx_filter_ptr)) {
        pthread_mutex_unlock(&am_demux_dev.lock);
        return TAPI_FALSE;
    }

    //free buf list used by filter
    dmx_cbuf_flush(&am_dmx_filter_ptr->cbuf);
    am_dmx_filter_ptr->serialnumread = am_dmx_filter_ptr->serialnumwrite = 0;
    pthread_mutex_unlock(&am_demux_dev.lock);
    return TAPI_TRUE;
}

////////////////////////////////////////////
//@Fuction name: am_demux_getfilterstatus(TAPI_S8 index)
//@Description:get the filter status
////@param <IN>
//@--index : same as outindex in the function demux_open(DMX_FILTER_TYPE type,U8 index,U8* outindex)
//@param <RET>
//@---DMX_FILTER_STATUS

DMX_FILTER_STATUS am_demux_getfilterstatus(TAPI_S8 index) {
    am_demux_filter_t *am_dmx_filter_ptr = NULL;
    DMX_FILTER_STATUS fs;
    pthread_mutex_lock(&am_demux_dev.lock);
    if (TAPI_FALSE == get_filter_by_index(index, &am_dmx_filter_ptr)) {
        pthread_mutex_unlock(&am_demux_dev.lock);
        return DMX_FILTER_STATUS_DATA_NOT_READY;
    }

    if (am_dmx_filter_ptr->fid != -1) {
        fs = am_dmx_filter_ptr->hdi_dmx_filter_status;
    } else {
        fs = DMX_FILTER_STATUS_DATA_NOT_READY;
    }

    pthread_mutex_unlock(&am_demux_dev.lock);
    return fs;
}

//////////////////////////////////////////////////
//@Fuction name : am_demux_isfiltertimeoutU8 index)
//@Description: is filter out?
//@param <IN>
//@--index : same as outindex in the function demux_open(DMX_FILTER_TYPE type,U8 index,U8* outindex)
//@param <RET>
//@--TRUE:time out
//@--FALSE:time not out
TAPI_BOOL am_demux_isfiltertimeout(TAPI_S8 index) {
    am_demux_filter_t *am_dmx_filter_ptr = NULL;
    TAPI_BOOL ret;

    pthread_mutex_lock(&am_demux_dev.lock);
    if (TAPI_FALSE == get_filter_by_index(index, &am_dmx_filter_ptr)) {
        pthread_mutex_unlock(&am_demux_dev.lock);
        return TAPI_FALSE;
    }
    ret = am_dmx_filter_ptr->hdi_dmx_filter_status == DMX_FILTER_STATUS_STATUS_TIMEOUT ? TAPI_TRUE : TAPI_FALSE;
    pthread_mutex_unlock(&am_demux_dev.lock);
    return ret;
}

///////////////////////////////////////////////////////////////////
//Fuction name :  am_demux_getData
//Description : 
//@param <IN>
//@--u16id : 
//@param <RET>
//@--TRUE:success
//@--FALSE:fail

TAPI_BOOL am_demux_getdata(TAPI_U8 index, TAPI_U8 serialnum, TAPI_U32 addr, TAPI_U32 len, TAPI_BOOL fgUpdateData, TAPI_U8* buf) {
    int ret = -1;
    am_demux_filter_psi_info_t *p_psi_info = NULL;
    am_demux_filter_t *am_dmx_filter_ptr = NULL;
    hdi_dmx_cbuf_t *cbuf;
    TAPI_U32 avail_len;

    pthread_mutex_lock(&am_demux_dev.lock);
    if (buf == NULL || len == 0 || addr == 0) {
        pthread_mutex_unlock(&am_demux_dev.lock);
        LOGE("invalidate data buf or buf length");
        return TAPI_FALSE;
    }

    if (TAPI_FALSE == get_filter_by_index(index, &am_dmx_filter_ptr)) {
        pthread_mutex_unlock(&am_demux_dev.lock);
        LOGE("can not find the assigned filter[%d]", index);
        return TAPI_FALSE;
    }

    if (!am_dmx_filter_ptr->used || !am_dmx_filter_ptr->enable) {
        pthread_mutex_unlock(&am_demux_dev.lock);
        LOGE("filter index =%d used=%d,enble=%d\n", am_dmx_filter_ptr->index, am_dmx_filter_ptr->used, am_dmx_filter_ptr->enable);
        return TAPI_FALSE;
    }
    cbuf = &am_dmx_filter_ptr->cbuf;
    if (cbuf->data == NULL) {
        pthread_mutex_unlock(&am_demux_dev.lock);
        LOGE("please alloc cbuf->data");
        return TAPI_FALSE;
    }
    //LOGE("%s,in len = %d pid=0x%x tid=%d\n",__FUNCTION__, len,am_dmx_filter_ptr->pid,am_dmx_filter_ptr->params.sec.filter.filter[0]);
    //LOGE("%s,filter[%d].serial=%d\n",__FUNCTION__,am_dmx_filter_ptr->fid,serialnum);
    /*Now read from cbuf*/
    avail_len = dmx_cbuf_avail(cbuf);
    if (avail_len <= 0) {
#if 0
        struct timespec rt;
        int rv;
        AM_TIME_GetTimeSpecTimeout(500, &rt); //wait 500ms
        rv = pthread_cond_timedwait(&cbuf->cond, &am_demux_dev.lock, &rt);
        if (rv == ETIMEDOUT) {
            pthread_mutex_unlock(&am_demux_dev.lock);
            LOGE("not avail data,timeout");
            return TAPI_FALSE;
        }
#else
        pthread_mutex_unlock(&am_demux_dev.lock);
        LOGE("not avail data");
        return TAPI_FALSE;
#endif
    } else {
        TAPI_U16 sec_len;
        if (avail_len >= 4) {
            dmx_cbuf_read(cbuf, buf, 3);
            sec_len = ((TAPI_U16)(buf[1] & 0x0f) << 8) | (TAPI_U16) buf[2];
            if ((TAPI_U32)(sec_len + 3) != len) {
                LOGE("length is not right,sec_len=%d,len=%d,filter[%d].pid=%d ", sec_len, len, am_dmx_filter_ptr->index, am_dmx_filter_ptr->pid);
                dmx_cbuf_reset(&am_dmx_filter_ptr->cbuf);
            } else {
                if (sec_len > 0) {
                    dmx_cbuf_read(cbuf, buf + 3, sec_len);
#if 0
                    if (am_dmx_filter_ptr->pid == 0) {
                        int j;
                        DEBUG_TIME();DEBUG("PAT DATA :\n");
                        for (j = 0; j < len; j++) {
                            DEBUG("%2x ",buf[j]);
                            if ((j + 1) % 8 == 0)
                                DEBUG("\n");
                        }DEBUG("\n");
                    }
                    if (am_dmx_filter_ptr->params.sec.filter.filter[0] == AM_SI_TID_PMT) {
                        int j;
                        DEBUG_TIME();DEBUG("PMT DATA :\n");
                        for (j = 0; j < len; j++) {
                            DEBUG("%2x ",buf[j]);
                            if ((j + 1) % 8 == 0)
                                DEBUG("\n");
                        }DEBUG("\n");
                    }
                    if (am_dmx_filter_ptr->params.sec.filter.filter[0] == AM_SI_TID_TDT) {
                      int j;
                      DEBUG_TIME();DEBUG("filter[%d] TDT DATA :\n",am_dmx_filter_ptr->index);
                      for (j = 0; j < len; j++) {
                         DEBUG("%2x ",buf[j]);
                         if ((j + 1) % 8 == 0)
                             DEBUG("\n");
                      }DEBUG("\n");
                   }
#endif
                    ret = 0;
                }
            }

        } else {
            if (avail_len != 0) {
                dmx_cbuf_reset(cbuf);

            }
        }
    }
    pthread_mutex_unlock(&am_demux_dev.lock);

    return ret == 0 ? TAPI_TRUE : TAPI_FALSE;
}

///////////////////////////////////////////////////////////////////////
//                   PID FILLTER
///////////////////////////////////////////////////////////////////////

//allocate a pid filter 
TAPI_BOOL am_demux_allocatepidfilter(DMX_FILTER_TYPE type, TAPI_S8 index, TAPI_S8* outindex) {
#if _NOT_USE_PES_FILTER_
    *outindex = index;
    return TAPI_TRUE;
#else
    int ret = -1;
    am_demux_filter_t *am_dmx_filter_ptr = NULL;
    int indexId;
    pthread_mutex_lock(&am_demux_dev.lock);

    for(indexId=0; indexId<SECTION_FILTER_NUM; indexId++)
    {
        if(!g_am_demux_filter[indexId].used)
        break;
    }

    if(indexId>=SECTION_FILTER_NUM)
    {
        pthread_mutex_unlock(&am_demux_dev.lock);
        LOGE("fail to get  filter_index=%d", index);
        return TAPI_FALSE;
    }

    am_dmx_filter_ptr = &g_am_demux_filter[indexId];

    if (am_dmx_filter_ptr->fid != -1) {
        ret = AM_DMX_StopFilter(am_dmx_filter_ptr->dmx_dev_id, am_dmx_filter_ptr->fid);
        if (ret != AM_SUCCESS) {
            LOGE("stop filter failure!");
        }
        ret = AM_DMX_FreeFilter(am_dmx_filter_ptr->dmx_dev_id, am_dmx_filter_ptr->fid);
        if (ret != AM_SUCCESS) {
            LOGE("free filter failure!");
        }
        pthread_mutex_unlock(&am_demux_dev.lock);
        AM_DMX_Sync(am_dmx_filter_ptr->dmx_dev_id);
        pthread_mutex_lock(&am_demux_dev.lock);
    }

    ret = AM_DMX_AllocateFilter(am_dmx_filter_ptr->dmx_dev_id, &(am_dmx_filter_ptr->fid));
    if (ret != AM_SUCCESS) {
        LOGE("allocate filter failure!");
    } else {
        *outindex = indexId; //am_dmx_filter_ptr->fid;
        am_dmx_filter_ptr->type = type;
        //am_dmx_filter_ptr->index = indexId;
        am_dmx_filter_ptr->hdi_dmx_filter_status = DMX_FILTER_STATUS_DATA_READY;
        am_dmx_filter_ptr->used = TAPI_TRUE;
        am_dmx_filter_ptr->enable = TAPI_FALSE;
        ret = AM_DMX_SetCallback(am_dmx_filter_ptr->dmx_dev_id,am_dmx_filter_ptr->fid,NULL, NULL);
    }
    pthread_mutex_unlock(&am_demux_dev.lock);
    return ret == AM_SUCCESS ? TAPI_TRUE : TAPI_FALSE;
#endif
}

//set pid to filter
TAPI_BOOL am_demux_setpid(TAPI_S8 index, TAPI_S16 u16id) {
#if _NOT_USE_PES_FILTER_
    return TAPI_TRUE;
#else
    int ret = -1;
    am_demux_filter_t *am_dmx_filter_ptr = NULL;

    pthread_mutex_lock(&am_demux_dev.lock);
    if (TAPI_FALSE == get_filter_by_index(index, &am_dmx_filter_ptr)) {
        pthread_mutex_unlock(&am_demux_dev.lock);
        return TAPI_FALSE;
    }

    if (am_dmx_filter_ptr->fid != -1 && !am_dmx_filter_ptr->enable) {
        memset(&am_dmx_filter_ptr->params.pes, 0, sizeof(am_dmx_filter_ptr->params.pes));
        am_dmx_filter_ptr->params.pes.pid = u16id;
        am_dmx_filter_ptr->params.pes.input = DMX_IN_FRONTEND;
        am_dmx_filter_ptr->params.pes.output = DMX_OUT_TS_TAP;
        am_dmx_filter_ptr->params.pes.pes_type = DMX_PES_OTHER;
//memcpy(&am_dmx_filter_ptr->params.pes.filter.filter[0],&param.au1Data[0],sizeof(param.au1Data));
//memcpy(&am_dmx_filter_ptr->params.pes.filter.mask[0],&param.au1Mask[0],sizeof(param.au1Mask));
        ret = AM_DMX_SetPesFilter(am_dmx_filter_ptr->dmx_dev_id, am_dmx_filter_ptr->fid, &(am_dmx_filter_ptr->params.pes));
        if (ret != AM_SUCCESS) {
            LOGE("set Pesfilter failure!");
        }
    }

    pthread_mutex_unlock(&am_demux_dev.lock);
    return ret == AM_SUCCESS ? TAPI_TRUE : TAPI_FALSE;
#endif
}

TAPI_BOOL am_demux_pidfilter_start(TAPI_S8 index) {
#if _NOT_USE_PES_FILTER_
    return TAPI_TRUE;
#else
    int ret = -1;
    am_demux_filter_t *am_dmx_filter_ptr = NULL;

    pthread_mutex_lock(&am_demux_dev.lock);
    if (TAPI_FALSE == get_filter_by_index(index, &am_dmx_filter_ptr)) {
        pthread_mutex_unlock(&am_demux_dev.lock);
        return TAPI_FALSE;
    }

    if (am_dmx_filter_ptr->fid != -1 && !am_dmx_filter_ptr->enable) {
        LOGD("%s,id = %d, fid = %d\n",__FUNCTION__, am_dmx_filter_ptr->dmx_dev_id, am_dmx_filter_ptr->fid);
        ret = AM_DMX_StartFilter(am_dmx_filter_ptr->dmx_dev_id, am_dmx_filter_ptr->fid);
        if (ret != AM_SUCCESS) {
            LOGE("start Pesfilter failure!");
        } else {
            am_dmx_filter_ptr->enable = TAPI_TRUE;
        }
    }

    pthread_mutex_unlock(&am_demux_dev.lock);
    return ret == AM_SUCCESS ? TAPI_TRUE : TAPI_FALSE;
#endif
}

TAPI_BOOL am_demux_pidfilter_stop(TAPI_S8 index) {
#if _NOT_USE_PES_FILTER_
    return TAPI_TRUE;
#else
    int ret = -1;
    am_demux_filter_t *am_dmx_filter_ptr = NULL;

    pthread_mutex_lock(&am_demux_dev.lock);
    if (TAPI_FALSE == get_filter_by_index(index, &am_dmx_filter_ptr)) {
        pthread_mutex_unlock(&am_demux_dev.lock);
        return TAPI_FALSE;
    }

    if (!am_dmx_filter_ptr->enable || !am_dmx_filter_ptr->used) {
        pthread_mutex_unlock(&am_demux_dev.lock);
        return TAPI_TRUE;
    }

    if (am_dmx_filter_ptr->fid != -1) {
        ret = AM_DMX_StopFilter(am_dmx_filter_ptr->dmx_dev_id, am_dmx_filter_ptr->fid);
        if (ret != AM_SUCCESS) {
            LOGE("stop Pesfilter failure!");
        } else {
            am_dmx_filter_ptr->enable = TAPI_FALSE;
        }
    }

    pthread_mutex_unlock(&am_demux_dev.lock);
    return ret == AM_SUCCESS ? TAPI_TRUE : TAPI_FALSE;
#endif
}

TAPI_BOOL am_demux_pidfilter_reset(TAPI_S8 index) {
#if _NOT_USE_PES_FILTER_
    return TAPI_TRUE;
#else
    int ret = 0;
    am_demux_filter_t *am_dmx_filter_ptr = NULL;

    pthread_mutex_lock(&am_demux_dev.lock);
    if (TAPI_FALSE == get_filter_by_index(index, &am_dmx_filter_ptr)) {
        pthread_mutex_unlock(&am_demux_dev.lock);
        return TAPI_FALSE;
    }

    pthread_mutex_unlock(&am_demux_dev.lock);
    return ret == 0 ? TAPI_TRUE : TAPI_FALSE;
#endif
}

TAPI_BOOL am_demux_pidfilter_restart(TAPI_S8 index) {
#if _NOT_USE_PES_FILTER_
    return TAPI_TRUE;
#else
    int ret = -1;
    am_demux_filter_t *am_dmx_filter_ptr = NULL;

    pthread_mutex_lock(&am_demux_dev.lock);
    if (TAPI_FALSE == get_filter_by_index(index, &am_dmx_filter_ptr)) {
        pthread_mutex_unlock(&am_demux_dev.lock);
        return TAPI_FALSE;
    }

    if (am_dmx_filter_ptr->fid != -1 && !am_dmx_filter_ptr->enable) {
        ret = AM_DMX_StartFilter(am_dmx_filter_ptr->dmx_dev_id, am_dmx_filter_ptr->fid);
        if (ret != AM_SUCCESS) {
            LOGE("re-start Pesfilter failure!");
        } else {
            am_dmx_filter_ptr->enable = TAPI_TRUE;
        }
    }
    pthread_mutex_unlock(&am_demux_dev.lock);
    return ret == AM_SUCCESS ? TAPI_TRUE : TAPI_FALSE;
#endif
}

TAPI_BOOL am_demux_pidfilter_close(TAPI_S8 index) {
#if _NOT_USE_PES_FILTER_
    return TAPI_TRUE;
#else
    int ret = -1;
    am_demux_filter_t *am_dmx_filter_ptr = NULL;

    pthread_mutex_lock(&am_demux_dev.lock);
    if (TAPI_FALSE == get_filter_by_index(index, &am_dmx_filter_ptr)) {
        pthread_mutex_unlock(&am_demux_dev.lock);
        return TAPI_FALSE;
    }

    if (am_dmx_filter_ptr->fid != -1) {
        ret = AM_DMX_StopFilter(am_dmx_filter_ptr->dmx_dev_id, am_dmx_filter_ptr->fid);
        if (ret != AM_SUCCESS) {
            LOGE("stop Pesfilter failure!");
        } else {
            am_dmx_filter_ptr->enable = TAPI_FALSE;
        }

        ret = AM_DMX_FreeFilter(am_dmx_filter_ptr->dmx_dev_id, am_dmx_filter_ptr->fid);
        if (ret != AM_SUCCESS) {
            LOGE("free Pesfilter failure!");
        } else {
            am_dmx_filter_ptr->used = TAPI_FALSE;
        }
        pthread_mutex_unlock(&am_demux_dev.lock);
        AM_DMX_Sync(am_dmx_filter_ptr->dmx_dev_id);
        pthread_mutex_lock(&am_demux_dev.lock);
        am_dmx_filter_ptr->fid = -1;
    }
    pthread_mutex_unlock(&am_demux_dev.lock);
    return ret == AM_SUCCESS ? TAPI_TRUE : TAPI_FALSE;
#endif
}

