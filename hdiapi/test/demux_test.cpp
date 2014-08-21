/*
 **  AM_DEMUX module test case
 **
 */

#define AM_DEBUG_LEVEL 5

#include <sys/types.h>
#include <am_debug.h>
#include <am_dmx.h>
#include <am_fend.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "TTvHW_demux.h"
#include "TTvHW_tuner.h"
#include "TTvHW_demodulator.h"

#define     Callback_Max_Number   64

//#include "TTvApi.h"
//----------------------------------------------
// Constant definitions
//----------------------------------------------

/* OS API return values */
#define OSR_THREAD_ACTIVE         ((TAPI_S32)
#define OSR_MEM_NOT_FREE          ((TAPI_S32)
#define OSR_WOULD_BLOCK           ((TAPI_S32)
#define OSR_OK                    ((TAPI_S32)
#define OSR_EXIST                 ((TAPI_S32)  -
#define OSR_INV_ARG               ((TAPI_S32)  -
#define OSR_TIMEOUT               ((TAPI_S32)  -
#define OSR_NO_RESOURCE           ((TAPI_S32)  -
#define OSR_NOT_EXIST             ((TAPI_S32)  -
#define OSR_NOT_FOUND             ((TAPI_S32)  -
#define OSR_INVALID               ((TAPI_S32)  -
#define OSR_NOT_INIT              ((TAPI_S32)  -
#define OSR_DELETED               ((TAPI_S32)  -
#define OSR_TOO_MANY              ((TAPI_S32) -1
#define OSR_TOO_BIG               ((TAPI_S32) -1
#define OSR_DUP_REG               ((TAPI_S32) -1
#define OSR_NO_MSG                ((TAPI_S32) -1
#define OSR_INV_HANDLE            ((TAPI_S32) -1
#define OSR_FAIL                  ((TAPI_S32) -1
#define OSR_OUT_BOUND             ((TAPI_S32) -1
#define OSR_NOT_SUPPORT           ((TAPI_S32) -1
#define OSR_BUSY                  ((TAPI_S32) -1
#define OSR_OUT_OF_HANDLES        ((TAPI_S32) -1
#define OSR_AEE_OUT_OF_RESOURCES  ((TAPI_S32) -2
#define OSR_AEE_NO_RIGHTS         ((TAPI_S32) -2
#define OSR_CANNOT_REG_WITH_CLI   ((TAPI_S32) -2
#define OSR_DUP_KEY               ((TAPI_S32) -2
#define OSR_KEY_NOT_FOUND         ((TAPI_S32) -2

#define ATSC_CH_NS            70  // 0~69
#define NTSC_RF_CH_NS         69

#define _STATE_INIT 0
#define _STATE_FILTER 1
#define _STATE_HIT 2
#define _STATE_FAIL 3

#define PSI_MAX_TABLE 8192

#define DMX_DEFAULT_ES_FIFO_SIZE
#define DMX_DEFAULT_PSI_FIFO_SIZE

#define X_SEMA_STATE_LOCK   ((TAPI_S32) 0)
#define X_SEMA_STATE_UNLOCK ((TAPI_S32) 1)

#define _MAX_PMT_NUM 256

#define SECT_HEADER_LEN 8

pthread_t thread_PMT_Data;

//----------------------------------------------
// Type definitions
//----------------------------------------------

typedef enum {
    AFORMAT_MPEG = 0,
    AFORMAT_PCM_S16LE = 1,
    AFORMAT_AAC = 2,
    AFORMAT_AC3 = 3,
    AFORMAT_ALAW = 4,
    AFORMAT_MULAW = 5,
    AFORMAT_DTS = 6,
    AFORMAT_PCM_S16BE = 7,
    AFORMAT_FLAC = 8,
    AFORMAT_COOK = 9,
    AFORMAT_PCM_U8 = 10,
    AFORMAT_ADPCM = 11,
    AFORMAT_AMR = 12,
    AFORMAT_RAAC = 13,
    AFORMAT_WMA = 14,
    AFORMAT_WMAPRO = 15,
    AFORMAT_PCM_BLURAY = 16,
    AFORMAT_ALAC = 17,
    AFORMAT_VORBIS = 18,
    AFORMAT_AAC_LATM = 19,
    AFORMAT_UNSUPPORT = 20,
    AFORMAT_MAX = 21
} aformat_t;

typedef enum {
    VFORMAT_MPEG12 = 0,
    VFORMAT_MPEG4,
    VFORMAT_H264,
    VFORMAT_MJPEG,
    VFORMAT_REAL,
    VFORMAT_JPEG,
    VFORMAT_VC1,
    VFORMAT_AVS,
    VFORMAT_YUV, // Use SW decoder
    VFORMAT_H264MVC,
    VFORMAT_MAX
} vformat_t;
typedef struct _UserEvent {
    pthread_cond_t cond;
    pthread_mutex_t mutex;
} UserEvent_t, *PUserEvent_t;

typedef struct _NAV_ATV_TBL_PROG_T {
    TAPI_S16 u2ChanIdx;
    TAPI_S32 u4TvSys;
    TAPI_S32 u4SoundSys;
} NAV_ATV_TBL_PROG_T;

typedef TAPI_S32 HANDLE_T;

typedef enum {
    X_SEMA_OPTION_WAIT = 1,
    X_SEMA_OPTION_NOWAIT
} SEMA_OPTION_T;

typedef enum {
    X_SEMA_TYPE_BINARY = 1,
    X_SEMA_TYPE_MUTEX,
    X_SEMA_TYPE_COUNTING
} SEMA_TYPE_T;

typedef struct _Sema {
    SEMA_TYPE_T eType;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
} Sema_t, *PSema_t;

typedef struct _PMTCallback {
    TAPI_BOOL used;
    TAPI_S8 index;
    DMX_FILTER_STATUS status;
    DMX_NOTIFY_INFO_PSI_T data;
} PMTCallback;

PMTCallback TclPMTCallback[Callback_Max_Number];

/*****************************************************************************
 * psi_descriptor_t
 *****************************************************************************/
typedef struct psi_descriptor_s {
    uint8_t i_tag; /*!< descriptor_tag */
    uint8_t i_length; /*!< descriptor_length */
    uint8_t * p_data; /*!< content */
    struct psi_descriptor_s * p_next; /*!< next element of the list */
    void * p_decoded; /*!< decoded descriptor */
} psi_descriptor_t;
/*****************************************************************************
 * psi_pmt_es_t
 *****************************************************************************/
typedef struct psi_pmt_es_s {
    TAPI_U8 i_type; /*!< stream_type */
    TAPI_U16 i_pid; /*!< elementary_PID */
    psi_descriptor_t * p_first_descriptor; /*!< descriptor list */
    struct psi_pmt_es_s * p_next; /*!< next element of  the list */
} psi_pmt_es_t;
/*****************************************************************************
 * psi_pmt_t
 *****************************************************************************/
typedef struct psi_pmt_s {
    struct psi_pmt_s *p_next; /*!< pmt_list */
    TAPI_U8 i_table_id; /*!< table_id */
    TAPI_U16 i_program_number; /*!< program_number */
    TAPI_U8 i_version; /*!< version_number */
    TAPI_S32 b_current_next; /*!< current_next_indicator */
    TAPI_U16 i_pcr_pid; /*!< PCR_PID */
    psi_descriptor_t * p_first_descriptor; /*!< descriptor list */
    psi_pmt_es_t * p_first_es; /*!< ES list */
} psi_pmt_t;

/*****************************************************************************
 * psi_program_info_t
 *****************************************************************************/
typedef struct psi_program_info_s {
    TAPI_U16 i_program_number; /*!< program_number */
    TAPI_U16 i_pcr_pid; /*!< PCR_PID */
    TAPI_U8 v_type; /*!< Video stream_type */
    TAPI_U16 v_pid; /*!< Video elementary_PID */
    TAPI_U8 a_type; /*!< Video stream_type */
    TAPI_U16 a_pid; /*!< only support one  audio_pid at this test case code */
} psi_program_info_t;

static TAPI_S8 _u4PatIndex = 0;
static TAPI_S8 _u4PmtIndex = 0;
static TAPI_S8 _u4AllPFIndex = 0;
static TAPI_S8 _u4SchIndex = 0;
static TAPI_S8 _u4PatIndex_error = 0;

static TAPI_BOOL _uRecvDatafromDMX = TAPI_FALSE;
static psi_program_info_t g_psi_program_info_list[_MAX_PMT_NUM]; //storage program info for AV test
static TAPI_U8 psi_program_count;
//extern unsigned long u4MtalLogMask;
unsigned long u4MtalLogMask;

// UT Semaphore
static pthread_mutex_t _UtDemuxMutex;

static MW_RESULT_T _DmxPatCallback(TAPI_S8 u1Pidx, DMX_FILTER_STATUS status, TAPI_S32 u4Data);

static TAPI_BOOL _ut_demux_CreateSemaphore(void) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    if (pthread_mutex_init(&_UtDemuxMutex, &attr)) {
        printf("Mutex NOT created.\n");
        return TAPI_FALSE;
    } else {
        return TAPI_TRUE;
    }
}
static TAPI_BOOL _ut_demux_LockSemaphore(void) {
    if (pthread_mutex_lock(&_UtDemuxMutex)) {
        printf("Mutex LOCK ERROR.\n");
        return TAPI_FALSE;
    } else {
        return TAPI_TRUE;
    }
}
static TAPI_BOOL _ut_demux_UnlockSemaphore(void) {
    if (pthread_mutex_unlock(&_UtDemuxMutex)) {
        printf("Mutex UnlOCK ERROR.\n");
        return TAPI_FALSE;
    } else {
        return TAPI_TRUE;
    }
}
static TAPI_BOOL _ut_demux_DestroySemaphore(void) {
    if (pthread_mutex_destroy(&_UtDemuxMutex)) {
        printf("Mutex Destroy ERROR.\n");
        return TAPI_FALSE;
    } else {
        return TAPI_TRUE;
    }
}

static void dump_bytes(int dev_no, int fid, const uint8_t *data, int len, void *user_data) {
    int i;

    printf("section:\n");
    for (i = 0; i < len; i++) {
        printf("%02x ", data[i]);
        if (((i + 1) % 16) == 0)
            printf("\n");
    }

    if ((i % 16) != 0)
        printf("\n");
}

/*****************************************************************************
 * PSI_AddProgramInfo
 * Creation of a new psi_program_info_t  to list.
 *****************************************************************************/
TAPI_BOOL PSI_AddProgramInfo(psi_program_info_t *p_program_info) {
    if (psi_program_count + 1 < _MAX_PMT_NUM) {
        g_psi_program_info_list[psi_program_count].i_program_number = p_program_info->i_program_number;
        g_psi_program_info_list[psi_program_count].i_pcr_pid = p_program_info->i_pcr_pid;
        g_psi_program_info_list[psi_program_count].v_type = p_program_info->v_type;
        g_psi_program_info_list[psi_program_count].v_pid = p_program_info->v_pid;
        g_psi_program_info_list[psi_program_count].a_type = p_program_info->a_type;
        g_psi_program_info_list[psi_program_count].a_pid = p_program_info->a_pid;

        printf("\n*********add =======program_info_list[%d]**************\n", psi_program_count);
        printf("\program_info_list[%d].i_program_number = %x\n", psi_program_count, g_psi_program_info_list[psi_program_count].i_program_number);
        printf("\program_info_list[%d].i_pcr_pid= %x\n", psi_program_count, g_psi_program_info_list[psi_program_count].i_pcr_pid);
        printf("\program_info_list[%d].v_type= %x\n", psi_program_count, g_psi_program_info_list[psi_program_count].v_type);
        printf("\program_info_list[%d].v_pid= %x\n", psi_program_count, g_psi_program_info_list[psi_program_count].v_pid);
        printf("\program_info_list[%d].a_type= %x\n", psi_program_count, g_psi_program_info_list[psi_program_count].a_type);
        printf("\program_info_list[%d].a_pid= %x\n", psi_program_count, g_psi_program_info_list[psi_program_count].a_pid);

        psi_program_count++;
    } else {
        return TAPI_FALSE;
    }

    return TAPI_TRUE;
}

/*****************************************************************************
 * PSI_GetProgramInfo()
 * get ProgramInfo_list acquired from PMT table.
 *****************************************************************************/
void PSI_GetProgramInfo(TAPI_U8 *ProgramCount, psi_program_info_t **p_program_info_list) {
    *ProgramCount = psi_program_count;
    *p_program_info_list = (psi_program_info_t*) &g_psi_program_info_list[0];
    ;
}

/*****************************************************************************
 * PSI_GetProgramInfo()
 * Creation of a new psi_descriptor_t structure.
 *****************************************************************************/
void PSI_DumpProgramInfo_list() {
    psi_program_info_t *p_program_info_list;
    TAPI_U8 TotalProgramCount;
    int i = 0;

    //printf("\n=======================PSI_DumpProgramInfo_list===============\n");
    PSI_GetProgramInfo(&TotalProgramCount, &p_program_info_list);

    printf("TotalProgramCount = %d ,p_program_info_list = %8x", TotalProgramCount, p_program_info_list);
    if (TotalProgramCount > 0 && p_program_info_list != NULL) {
        psi_program_info_t *p_program_info = &p_program_info_list[0];
        while (p_program_info != NULL && i < TotalProgramCount) {
            printf("\n=======================program[%d] :===============\n", i);
            printf("i_program_number = %02x \n", p_program_info->i_program_number);
            printf("i_pcr_pid = %02x \n", p_program_info->i_pcr_pid);
            printf("audio_type = %02x ,audio_pid = %4x\n", p_program_info->a_type, p_program_info->a_pid);
            printf("video_type = %02x ,video_pid = %4x\n", p_program_info->v_type, p_program_info->v_pid);
            //printf("\n=======================program[%d]  end===============\n",i);
            p_program_info = &p_program_info_list[++i];
        }
    }
}
/*****************************************************************************
 * PSI_NewDescriptor
 * Creation of a new psi_descriptor_t structure.
 *****************************************************************************/
psi_descriptor_t* PSI_NewDescriptor(TAPI_U8 i_tag, TAPI_U8 i_length, TAPI_U8* p_data) {
    psi_descriptor_t* p_descriptor = (psi_descriptor_t*) malloc(sizeof(psi_descriptor_t));

    if (p_descriptor) {
        if (i_length == 0) {
            /* In some descriptors, this case is legacy */
            p_descriptor->p_data = NULL;
            p_descriptor->i_tag = i_tag;
            p_descriptor->i_length = i_length;
            p_descriptor->p_decoded = NULL;
            p_descriptor->p_next = NULL;
            return p_descriptor;
        }
        p_descriptor->p_data = (uint8_t*) malloc(i_length * sizeof(uint8_t));

        if (p_descriptor->p_data) {
            p_descriptor->i_tag = i_tag;
            p_descriptor->i_length = i_length;
            if (p_data)
                memcpy(p_descriptor->p_data, p_data, i_length);
            p_descriptor->p_decoded = NULL;
            p_descriptor->p_next = NULL;
            /*Decode it*/
        } else {
            free(p_descriptor);
            p_descriptor = NULL;
        }
    }

    return p_descriptor;
}

void PSI_EmptyPMT(psi_pmt_t *p_pmt) {
#define  DEL_DESCRIPTORS(p_descriptor) \
    do{\
        while(p_descriptor != NULL) {\
            psi_descriptor_t* p_next = p_descriptor->p_next;\
            if(p_descriptor->p_data != NULL)\
                free(p_descriptor->p_data);   \
            if(p_descriptor->p_decoded != NULL) \
                free(p_descriptor->p_decoded);\
            free(p_descriptor);\
            p_descriptor = p_next;\
        }\
    } while(0);

    psi_pmt_es_t *p_es = p_pmt->p_first_es;

    DEL_DESCRIPTORS(p_pmt->p_first_descriptor);

    while (p_es != NULL) {
        psi_pmt_es_t *p_tmp_es = p_es->p_next;
        DEL_DESCRIPTORS(p_es->p_first_descriptor);
        free(p_es);
        p_es = p_tmp_es;
    }

    p_pmt->p_first_descriptor = NULL;
    p_pmt->p_first_es = NULL;

    free(p_pmt);
}

void PSI_DUMP_VA_PID_INFO(psi_pmt_es_t *p_es, psi_program_info_t *program_info) {
    psi_descriptor_t *p_descriptor = NULL;

    /*override by parse descriptor*/
    switch (p_es->i_type) {
    case 0x6:
        p_descriptor = p_es->p_first_descriptor;
        while (p_descriptor != NULL) {
            switch (p_descriptor->i_tag) {
            case 0x6A: //DESCR_AC3
            case 0x7A: //DESCR_ENHANCED_AC3
                printf(" stream_type =%4x ->AC3,A_PID=%x\n", p_es->i_type, p_es->i_pid);
                program_info->a_type = AFORMAT_AC3;
                program_info->a_pid = p_es->i_pid;
                break;
            case 0x7C: //DESCR_AAC
                printf(" stream_type =%4x ->AAC,A_PID=%x\n", p_es->i_type, p_es->i_pid);
                program_info->a_type = AFORMAT_AAC;
                program_info->a_pid = p_es->i_pid;
                break;
            case 0x7B: //DESCR_DTS
                printf(" stream_type =%4x ->DTS,A_PID=%x\n", p_es->i_type, p_es->i_pid);
                program_info->a_type = AFORMAT_DTS;
                program_info->a_pid = p_es->i_pid;
                break;
            default:
                break;
            }
            p_descriptor = p_descriptor->p_next;
        }
        break;

        /*video pid and video format*/
    case 0x1:
    case 0x2:
        program_info->v_type = VFORMAT_MPEG12;
        program_info->v_pid = p_es->i_pid;
        printf(" stream_type =%4x ->VFORMAT_MPEG12,V_PID=%x\n", p_es->i_type, p_es->i_pid);
        break;

    case 0x10:
        program_info->v_type = VFORMAT_MPEG4;
        program_info->v_pid = p_es->i_pid;
        printf(" stream_type =%4x ->VFORMAT_MPEG4,V_PID=%x\n", p_es->i_type, p_es->i_pid);
        break;

    case 0x1b:
        program_info->v_type = VFORMAT_H264;
        program_info->v_pid = p_es->i_pid;
        printf(" stream_type =%4x ->VFORMAT_H264,V_PID=%x\n", p_es->i_type, p_es->i_pid);
        break;

    case 0xea:
        program_info->v_type = VFORMAT_VC1;
        program_info->v_pid = p_es->i_pid;
        printf(" stream_type =%4x ->VFORMAT_VC1,V_PID=%x\n", p_es->i_type, p_es->i_pid);
        break;

        /*audio pid and audio format*/
    case 0x3:
    case 0x4:
        program_info->a_type = AFORMAT_MPEG;
        program_info->a_pid = p_es->i_pid;
        printf(" stream_type =%4x ->AFORMAT_MPEG,A_PID=%x\n", p_es->i_type, p_es->i_pid);
        break;

    case 0x0f:
        program_info->a_type = AFORMAT_AAC;
        program_info->a_pid = p_es->i_pid;
        printf(" stream_type =%4x ->AFORMAT_AAC,A_PID=%x\n", p_es->i_type, p_es->i_pid);
        break;

    case 0x11:
        program_info->a_type = AFORMAT_AAC_LATM;
        program_info->a_pid = p_es->i_pid;
        printf(" stream_type =%4x ->AFORMAT_AAC_LATM,A_PID=%x\n", p_es->i_type, p_es->i_pid);
        break;

    case 0x81:
        program_info->a_type = AFORMAT_AC3;
        program_info->a_pid = p_es->i_pid;
        printf(" stream_type =%4x ->AFORMAT_AC3,A_PID=%x\n", p_es->i_type, p_es->i_pid);
        break;

    case 0x8A:
    case 0x82:
    case 0x85:
    case 0x86:
        program_info->a_type = AFORMAT_DTS;
        program_info->a_pid = p_es->i_pid;
        printf(" stream_type =%4x ->AFORMAT_DTS,A_PID=%x\n", p_es->i_type, p_es->i_pid);
        break;

    default:
        printf(" stream_type =%4x ->un-known type,PID=%x\n", p_es->i_type, p_es->i_pid);
        break;
    }
}

void PSI_DumpPMT(psi_pmt_t* p_pmt, psi_program_info_t *program_info) {
#define DUMP_DESCRIPTORS(p_descriptor) \
        do{\
            while(p_descriptor != NULL) {\
                psi_descriptor_t *p_next = p_descriptor->p_next;\
                printf("descriptor : %x  tag =%d,descriptor_len=%d\n",p_descriptor,p_descriptor->i_tag,p_descriptor->i_length);\
                printf("\n=======================descriptor data :===============\n");\
                for(int i=0;i<p_descriptor->i_length;i++) {\
                    printf("%02x ",p_descriptor->p_data[i]);\
                    if(((i+1)%16)==0) printf("\n");\
                }\
                printf("\n=======================descriptor data end===============\n");\
                p_descriptor = p_next;\
            }\
        } while(0);

    if (p_pmt) {
        psi_descriptor_t *p_descriptor = p_pmt->p_first_descriptor;

        printf("\n=======================dump PMT data===============\n");
        printf("current porgamnum=%4x,pcr_pid=%x\n", p_pmt->i_program_number, p_pmt->i_pcr_pid);
        printf("******** PMT descriptors************\n");
        DUMP_DESCRIPTORS(p_descriptor);

        psi_pmt_es_t *p_es = p_pmt->p_first_es;
        while (p_es != NULL) {
            psi_pmt_es_t *p_tmp = p_es->p_next;
            PSI_DUMP_VA_PID_INFO(p_es, program_info);
            p_descriptor = p_es->p_first_descriptor;
            printf("******** PMT ES descriptors************\n");
            DUMP_DESCRIPTORS(p_descriptor);

            p_es = p_tmp;
        }
    }
}
/*****************************************************************************
 * PSI_DecodePMTSections
 *****************************************************************************
 * PMT decoder.
 *****************************************************************************/
void PSI_DecodePMTSections(psi_pmt_t* p_pmt, void *p_section) {
#define Add_DESC_2_PMT(type ,tag,length,data) \
        do{\
            psi_descriptor_t* p_descriptor  = PSI_NewDescriptor(i_tag, length, data);\
            if(p_descriptor) {\
                if(p_pmt->p_first_descriptor == NULL) {\
                    p_pmt->p_first_descriptor = p_descriptor;\
                } else {\
                    psi_descriptor_t* p_last_descriptor = type->p_first_descriptor;\
                    while(p_last_descriptor->p_next != NULL)\
                        p_last_descriptor = p_last_descriptor->p_next;\
                        p_last_descriptor->p_next = p_descriptor;\
                }\
            } \
        } while(0);

    TAPI_U8 *p_byte, *p_end;
    TAPI_U8 *p_data = (TAPI_U8*) p_section;
    TAPI_U8 *p_payload_end, *p_payload_start;
    TAPI_U8 i_tag;
    TAPI_U8 i_length;
    TAPI_U16 section_length, useddata_len;

    section_length = (((TAPI_U16)(p_data[1] & 0x0f)) << 8) | (TAPI_U16) p_data[2];
    p_payload_start = &p_data[SECT_HEADER_LEN];
    p_payload_end = &p_data[3 + section_length];
    useddata_len = 5;

    printf("\nuseddata_len =%d ,section_length= %d\n", useddata_len, section_length);
    while (useddata_len < section_length) {
        /* - PMT descriptors */
        p_byte = p_payload_start + 4;
        p_end = p_byte + (((TAPI_U16)(p_payload_start[2] & 0x0f) << 8) | p_payload_start[3]);
        useddata_len += 4;

        while (p_byte + 2 <= p_end) {
            i_tag = p_byte[0];
            i_length = p_byte[1];

            if (i_length + 2 <= p_end - p_byte) {
                //add descript to PMT
                Add_DESC_2_PMT(p_pmt, i_tag, i_length, p_byte + 2);
            }
            p_byte += 2 + i_length;
            useddata_len += 2 + i_length;
        }

        /* - ESs */
        //useddata_len = (((TAPI_U16)(p_payload_start[2] & 0x0f) << 8) | p_payload_start[3]);
        for (p_byte = p_end; p_byte + 5 <= p_payload_end;) {
            TAPI_U8 i_type = p_byte[0];
            TAPI_U16 i_pid = ((TAPI_U16)(p_byte[1] & 0x1f) << 8) | p_byte[2];
            TAPI_U16 i_es_length = ((TAPI_U16)(p_byte[3] & 0x0f) << 8) | p_byte[4];

            //add ES info to PMT
            psi_pmt_es_t* p_es = (psi_pmt_es_t*) malloc(sizeof(psi_pmt_es_t));
            if (p_es) {
                p_es->i_type = i_type;
                p_es->i_pid = i_pid;
                p_es->p_first_descriptor = NULL;
                p_es->p_next = NULL;

                if (p_pmt->p_first_es == NULL) {
                    p_pmt->p_first_es = p_es;
                } else {
                    psi_pmt_es_t* p_last_es = p_pmt->p_first_es;
                    while (p_last_es->p_next != NULL)
                        p_last_es = p_last_es->p_next;
                    p_last_es->p_next = p_es;
                }
            }

            /* - ES descriptors */
            p_byte += 5;
            p_end = p_byte + i_es_length;
            useddata_len += 5;
            if (p_end > p_payload_end) {
                p_end = p_payload_end;
            }

            while (p_byte + 2 <= p_end) {
                i_tag = p_byte[0];
                i_length = p_byte[1];
                if (i_length + 2 <= p_end - p_byte) {
                    //add ES descriptor
                    Add_DESC_2_PMT(p_es, i_tag, i_length, p_byte + 2);
                }
                p_byte += 2 + i_length;
                useddata_len += 2 + i_length;
            }
        }

        useddata_len += 4; //caculate CRC byte length
        //printf("\n decoding PMTuseddata_len =%d ,section_length= %d\n",useddata_len,section_length);
    }
}
#define TSGetU16(ptr)                       ((((const TAPI_U8 *)ptr)[0] << 8) + ((const TAPI_U8 *)ptr)[1])
#define TSGetBitsFromU8(ptr, lsb, mask)     (((*(const TAPI_U8 *)ptr) >> lsb) & mask)
#define TSGetBitsFromU16(ptr, lsb, mask)    ((TSGetU16(ptr) >> lsb) & mask)

//add parse NIT table case
static MW_RESULT_T _DmxNITCallback(TAPI_S8 index, DMX_FILTER_STATUS status, TAPI_S32 u4Data) {
    //printf("start nit callcack######################################\n");
    TAPI_U32 i;
    DMX_NOTIFY_INFO_PSI_T* prInfo = (DMX_NOTIFY_INFO_PSI_T*) u4Data;
    TAPI_U8* pu8BUF = (TAPI_U8*) malloc(1025 * 4);
    TAPI_U8 *pu8Section, *recvBuf;

    psi_pmt_t *p_pmt;
    TAPI_U16 sect_payloadlen;
    printf("PMT: Parser:\n");
    printf("==============NIT recv data from callback=================\n");
    recvBuf = (TAPI_U8*) prInfo->u4SecAddr;

    for (i = 0; i < prInfo->u4SecLen; i++) {
        printf("%02x ", recvBuf[i]);
        if (((i + 1) % 16) == 0)
            printf("\n");
    }
    printf("\n");

    memset(pu8BUF, 0, 1025 * 4);

    Tapi_Demux_GetData(index, prInfo->u1SerialNumber, prInfo->u4SecAddr, prInfo->u4SecLen, prInfo->fgUpdateData, pu8BUF);
    Tapi_Demux_Stop(index);

    pu8Section = pu8BUF;
    //printf("u4SecLen=%d,pu8Buff[0]=0x%x\n",prInfo->u4SecLen,pu8Section[0]);
    printf("\nNIT: Parser:");
    printf("\n==============NIT recv data from GetData=================\n");
    for (i = 0; i < prInfo->u4SecLen; i++) {
        printf("%02x ", pu8Section[i]);
        if (((i + 1) % 16) == 0)
            printf("\n");
    }
    Tapi_Demux_Close(index);

    free(pu8BUF);
    printf("\n************get NIT data sucess...................\n");

    //request PAT
    DMX_FILTER_PARAM rFilter;

    if (AM_FALSE == Tapi_Demux_AllocateFilter(DMX_FILTER_TYPE_SECTION, 0, 0, &_u4PatIndex)) {
        printf("AllocateFilter failed!!!!!!!!!!!!!\n");
        return MW_OK;
    } else {
        printf("[%s:%d] :Allocate Filter[%d] successfully \n", __FUNCTION__, __LINE__, (int) _u4PatIndex);
    }
    // rFilter.u2Pid     = 0; //PID_EIT_CIT;
    rFilter.u2Pid = 0; //PID_EIT_CIT;
    rFilter.ePidType = DMX_PID_TYPE_PSI;
    rFilter.pfnNotify = _DmxPatCallback;
    rFilter.pvNotifyTag = NULL;
    rFilter.fgCheckCrc = 1;
    rFilter.u1FilterIndex = _u4PatIndex;
    rFilter.u1Offset = 1;
    memset(rFilter.au1Data, 0, 16);
    memset(rFilter.au1Mask, 0, 16);
    rFilter.au1Data[0] = 0;
    rFilter.au1Mask[0] = 0xFF;
    rFilter.timeOut = 20000;

    if (AM_FALSE == Tapi_Demux_SetSectionFilterParam(_u4PatIndex, rFilter)) {
        printf("setSectionFilter error........\n");
        return MW_OK;
    } else {
        printf("[%s:%d] :set Filter[%d] successfully \n", __FUNCTION__, __LINE__, (int) _u4PatIndex);
    }

    if (AM_FALSE == Tapi_Demux_Start(_u4PatIndex)) {
        printf("Tapi_Demux_Start(_u4PatIndex) failed!!!!!!!!!!!!!!!!!!!!!\n");
        return MW_OK;
    } else {
        printf("[%s:%d] :start Filter[%d]successfully \n", __FUNCTION__, __LINE__, (int) _u4PatIndex);
    }

    return MW_OK;
}

static TAPI_BOOL _DmxGetNITTbl(TAPI_U16 NIT_PID, TAPI_U8 TableId, TAPI_S8 fid) {
    printf("start nit###############################\n");
    DMX_FILTER_PARAM rFilter;
    TAPI_S8 _u4PmtIndex_temp = 0;
    static TAPI_BOOL fgTestMulFilter = TAPI_TRUE;
    TAPI_U8 nit_fid = fid;

    if (TAPI_FALSE == Tapi_Demux_AllocateFilter(DMX_FILTER_TYPE_SECTION, NIT_PID, nit_fid, &_u4PmtIndex_temp)) {
        printf("AllocateFilter failed!!!!!!!!!!!!!\n");
        return TAPI_FALSE;
    }

    rFilter.u2Pid = NIT_PID; //PID_EIT_CIT;
    rFilter.ePidType = DMX_PID_TYPE_PSI;
    rFilter.pfnNotify = _DmxNITCallback;
    rFilter.pvNotifyTag = NULL;
    rFilter.fgCheckCrc = 1;
    rFilter.u1FilterIndex = _u4PmtIndex_temp;
    rFilter.u1Offset = 3;
    memset(rFilter.au1Data, 0, 16);
    memset(rFilter.au1Mask, 0, 16);
    rFilter.au1Data[0] = TableId;
    rFilter.au1Mask[0] = 0xFF;
    rFilter.au1Data[4] = 0; //(TAPI_U8)(ServiceId);   // stVirtualCh.wService_ID)
    rFilter.au1Data[3] = 0; //(TAPI_U8)((ServiceId>>8) & 0xff);    // stVirtualCh.wService_ID);
    rFilter.au1Mask[3] = 0xFF;
    rFilter.au1Mask[4] = 0xFF;
    rFilter.timeOut = 10000;
    //printf("_DmxGetPMTTbl1: PMT_PID=0x%x,ServiceId=0x%x,au1Data[3]=0x%x,au1Data[4]=0x%x,_u4PmtIndex=0x%x,\n",
    //    PMT_PID,ServiceId, rFilter.au1Data[3],rFilter.au1Data[4],_u4PmtIndex_temp);

    if (TAPI_FALSE == Tapi_Demux_SetSectionFilterParam(_u4PmtIndex_temp, rFilter)) {
        printf("setSectionFilter error........\n");
        return TAPI_FALSE;
    }

    if (TAPI_FALSE == Tapi_Demux_Start(_u4PmtIndex_temp)) {
        printf("Tapi_Demux_Start(_u4PmtIndex_temp) failed!!!!!!!!!!!!!!!!!!!!!\n");
        return TAPI_FALSE;
    }

    return TAPI_TRUE;
}

static MW_RESULT_T _DmxPmtCallback(TAPI_S8 index, DMX_FILTER_STATUS status, TAPI_S32 u4Data) {
    //printf("start pmt callcack######################################\n");
    TAPI_U32 i;
    DMX_NOTIFY_INFO_PSI_T* prInfo = (DMX_NOTIFY_INFO_PSI_T*) u4Data;
    TAPI_U8* pu8BUF = (TAPI_U8*) malloc(1025 * 4);
    TAPI_U8 *pu8Section, *pu8SecIter, *recvBuf;
    TAPI_U8 u8SecCnt, u8Version;
    TAPI_U16 j, u16SecSize, u16NumOfItem, u16ServiceIndex = 0, u16TsId, m_u16SecLengthCount, temp1, temp2, temp3, temp4, temp5;
    static TAPI_U16 u16ServiceID[120], u16PmtPID[120];
    psi_pmt_t *p_pmt;
    TAPI_U16 sect_payloadlen;
    printf("PMT: Parser:\n");
    printf("==============PMT recv data from callback=================\n");
    recvBuf = (TAPI_U8*) prInfo->u4SecAddr;

    for (i = 0; i < prInfo->u4SecLen; i++) {
        printf("%02x ", recvBuf[i]);
        if (((i + 1) % 16) == 0)
            printf("\n");
    }
    printf("\n");

    m_u16SecLengthCount = 0;
    memset(pu8BUF, 0, 1025 * 4);

    Tapi_Demux_GetData(index, prInfo->u1SerialNumber, prInfo->u4SecAddr, prInfo->u4SecLen, prInfo->fgUpdateData, pu8BUF);
    Tapi_Demux_Stop(index);

    pu8Section = pu8BUF;
    //printf("u4SecLen=%d,pu8Buff[0]=0x%x\n",prInfo->u4SecLen,pu8Section[0]);
    printf("\n\nPMT: Parser:");
    printf("\n==============PMT recv data from GetData=================\n");
    for (i = 0; i < prInfo->u4SecLen; i++) {
        printf("%02x ", pu8Section[i]);
        if (((i + 1) % 16) == 0)
            printf("\n");
    }
    Tapi_Demux_Close(index);

    //parse PMT table to get detail vid,aid vfmt,audfmt,ect
    /*Allocate a new table*/
    p_pmt = (psi_pmt_t*) malloc(sizeof(psi_pmt_t));
    if (p_pmt) {

        //init PMT table
        psi_program_info_t tmp_program_info;
        p_pmt->i_table_id = pu8Section[0];
        sect_payloadlen = (((TAPI_U16)(pu8Section[1] & 0x0f)) << 8) | (TAPI_U16) pu8Section[2];
        p_pmt->i_program_number = ((TAPI_U16) pu8Section[3] << 8) | (TAPI_U16) pu8Section[4];
        p_pmt->i_version = (pu8Section[5] & 0x3e) >> 1;
        p_pmt->b_current_next = pu8Section[5] & 0x1;
        ;
        p_pmt->i_pcr_pid = (TAPI_U16)((pu8Section[SECT_HEADER_LEN + 0] & 0x1f) << 8) | (TAPI_U16) pu8Section[SECT_HEADER_LEN + 1];
        p_pmt->p_first_descriptor = NULL;
        p_pmt->p_first_es = NULL;

        //
        memset(&tmp_program_info, 0, sizeof(psi_program_info_t));
        tmp_program_info.i_program_number = p_pmt->i_program_number;
        tmp_program_info.i_pcr_pid = p_pmt->i_pcr_pid;

        //decode PMT
        printf("\n*********start=======decode PMT data**************\n");
        PSI_DecodePMTSections(p_pmt, pu8Section);
        //dump PMT data
        PSI_DumpPMT(p_pmt, &tmp_program_info);
        //add program info to list
        PSI_AddProgramInfo(&tmp_program_info);
        //dump program info list
        //printf("\n*********dump program info list**************\n");
        PSI_DumpProgramInfo_list();
        //free PMT resource
        PSI_EmptyPMT(p_pmt);
    }
    free(pu8BUF);
    printf("\n************get PMT data sucess...................\n");

    return MW_OK;
}

static TAPI_BOOL _DmxGetPMTTbl(TAPI_U16 PMT_PID, TAPI_U16 ServiceId, TAPI_S8 fid) {
    printf("start pmt###############################\n");
    DMX_FILTER_PARAM rFilter;
    TAPI_S8 _u4PmtIndex_temp = 0;
    static TAPI_BOOL fgTestMulFilter = TAPI_TRUE;
    TAPI_U8 pmt_fid = fid;

#if 1

    if (TAPI_FALSE == Tapi_Demux_AllocateFilter(DMX_FILTER_TYPE_SECTION, PMT_PID, pmt_fid, &_u4PmtIndex_temp)) {
        printf("AllocateFilter failed!!!!!!!!!!!!!\n");
        return TAPI_FALSE;
    }

    rFilter.u2Pid = PMT_PID; //PID_EIT_CIT;
    rFilter.ePidType = DMX_PID_TYPE_PSI;
    rFilter.pfnNotify = _DmxPmtCallback;
    rFilter.pvNotifyTag = NULL;
    rFilter.fgCheckCrc = 1;
    rFilter.u1FilterIndex = _u4PmtIndex_temp;
    rFilter.u1Offset = 1;
    memset(rFilter.au1Data, 0, 16);
    memset(rFilter.au1Mask, 0, 16);
    rFilter.au1Data[0] = 0x2;
    rFilter.au1Mask[0] = 0xFF;
    rFilter.au1Data[4] = (TAPI_U8)(ServiceId); // stVirtualCh.wService_ID)
    rFilter.au1Data[3] = (TAPI_U8)((ServiceId >> 8) & 0xff); // stVirtualCh.wService_ID);
    rFilter.au1Mask[3] = 0xFF;
    rFilter.au1Mask[4] = 0xFF;
    rFilter.timeOut = 2000;
    //printf("_DmxGetPMTTbl1: PMT_PID=0x%x,ServiceId=0x%x,au1Data[3]=0x%x,au1Data[4]=0x%x,_u4PmtIndex=0x%x,\n",
    //    PMT_PID,ServiceId, rFilter.au1Data[3],rFilter.au1Data[4],_u4PmtIndex_temp);

    if (TAPI_FALSE == Tapi_Demux_SetSectionFilterParam(_u4PmtIndex_temp, rFilter)) {
        printf("setSectionFilter error........\n");
        return TAPI_FALSE;
    }

    if (TAPI_FALSE == Tapi_Demux_Start(_u4PmtIndex_temp)) {
        printf("Tapi_Demux_Start(_u4PmtIndex_temp) failed!!!!!!!!!!!!!!!!!!!!!\n");
        return TAPI_FALSE;
    }

#else
    //////////////////////////////////////
    //Test multiple section filters with same pid
    if (fgTestMulFilter) {
        DMX_FILTER_PARAM rFilter2;
        TAPI_S8 _u4FilterIndex2= 0;

        if(TAPI_FALSE==Tapi_Demux_AllocateFilter(DMX_FILTER_TYPE_SECTION,PMT_PID,0,&_u4FilterIndex2)) {
            printf("AllocateFilter failed!!!!!!!!!!!!!\n");
            return TAPI_FALSE;
        }

        rFilter2.u2Pid = PMT_PID; //PID_EIT_CIT;
        rFilter2.ePidType = DMX_PID_TYPE_PSI;
        rFilter2.pfnNotify = _DmxPmtCallback;
        rFilter2.pvNotifyTag = NULL;
        rFilter2.fgCheckCrc = 1;
        rFilter2.u1FilterIndex = _u4FilterIndex2;
        rFilter2.u1Offset = 1;
        memset(rFilter2.au1Data, 0, 16);
        memset(rFilter2.au1Mask, 0, 16);

        rFilter2.au1Data[0] = 2;
        rFilter2.au1Mask[0] = 0xFF;
        rFilter2.au1Data[4] = (TAPI_U8)(ServiceId);// stVirtualCh.wService_ID)
        rFilter2.au1Data[3] = (TAPI_U8)(ServiceId>>8);// stVirtualCh.wService_ID);
        rFilter2.au1Mask[3] = 0xFF;
        rFilter2.au1Mask[4] = 0xFF;

        rFilter2.timeOut = 1000;
        printf("_DmxGetPMTTbl1: PMT_PID=0x%x,ServiceId=0x%x,au1Data[3]=0x%x,au1Data[4]=0x%x,_u4PmtIndex=0x%x,\n",PMT_PID,ServiceId, rFilter.au1Data[3],rFilter.au1Data[4],_u4FilterIndex2);
        if(TAPI_FALSE==Tapi_Demux_SetSectionFilterParam(_u4FilterIndex2,rFilter2))
        {
            printf("setSectionFilter error........\n");
            return TAPI_FALSE;
        }

        if(TAPI_FALSE==Tapi_Demux_Start(_u4FilterIndex2))
        {
            printf("Tapi_Demux_Start(_u4FilterIndex2) failed!!!!!!!!!!!!!!!!!!!!!\n");
            return TAPI_FALSE;
        }
        printf("///////////////////////////////////////////////////////\n");
        printf("Test mul section filter: index=(%d)\n", _u4FilterIndex2);
        printf("///////////////////////////////////////////////////////\n");
        fgTestMulFilter = TAPI_FALSE;
    }
#endif
    return TAPI_TRUE;
}

static MW_RESULT_T _DmxPatCallback(TAPI_S8 u1Pidx, DMX_FILTER_STATUS status, TAPI_S32 u4Data) {
    TAPI_U32 i;
    DMX_NOTIFY_INFO_PSI_T* prInfo = (DMX_NOTIFY_INFO_PSI_T*) u4Data;
    TAPI_U8* pu8BUF = (TAPI_U8*) malloc(1025 * 4);
    TAPI_U8 *pu8Section, *pu8SecIter;
    TAPI_U8 u8SecCnt, u8Version;
    TAPI_U16 u16SecSize, u16NumOfItem, u16ServiceIndex = 0, u16TsId, m_u16SecLengthCount, temp1, temp2, temp3, temp4, temp5;
    TAPI_S8 j;
    static TAPI_U16 u16ServiceID[120], u16PmtPID[120];
    TAPI_U8 *recvBuf;
    TAPI_U32 usedSecLen;
    printf("PAT: Parser:\n");
    printf("\n==============PAT recv data from callback=================\n");
    recvBuf = (TAPI_U8*) prInfo->u4SecAddr;

    for (i = 0; i < prInfo->u4SecLen; i++) {
        printf("%02x ", recvBuf[i]);
        if (((i + 1) % 16) == 0)
            printf("\n");
    }
    printf("\n");

    m_u16SecLengthCount = 0;
    memset(pu8BUF, 0, 1025 * 4);

    Tapi_Demux_GetData(_u4PatIndex, prInfo->u1SerialNumber, prInfo->u4SecAddr, prInfo->u4SecLen, prInfo->fgUpdateData, pu8BUF);
    Tapi_Demux_Stop(_u4PatIndex);

    pu8Section = pu8BUF;
    printf("get data buf[%d]===========,buf_len=%ld,pu8Buff[0]=0x%x\n", prInfo->u1SerialNumber, prInfo->u4SecLen, pu8Section[0]);
    printf("\n==============PAT recv data from Demux_GetData=================\n");
    for (i = 0; i < prInfo->u4SecLen; i++) {
        printf("%02x ", pu8Section[i]);
        if (((i + 1) % 16) == 0)
            printf("\n");
    }
    printf("\n");

    Tapi_Demux_Close(_u4PatIndex);

    /*##########################START TEST PMT#####################################
     #############################################################################*/
#if 1
    printf("prInfo->u4SecLen = %ld\n", prInfo->u4SecLen);
    usedSecLen = 0;
    for (u8SecCnt = 0; usedSecLen < prInfo->u4SecLen; u8SecCnt++) {
        if (prInfo->u4SecLen < 1)
            break;

        pu8Section += m_u16SecLengthCount;
        u16SecSize = (pu8Section[1] & 0x0f) << 8 | pu8Section[2];
        u16SecSize |= pu8Section[2];
        printf("pu8Section[2] = %x, pu8Section[2] & 0x0f = %x, u16SecSize = %x\n", pu8Section[2], pu8Section[2] & 0x0f, u16SecSize);
        if (u16SecSize < 9)
            break;

        u16NumOfItem = (u16SecSize - 9) >> 2;
        u16TsId = TSGetU16(&pu8Section[3]);
        u8Version = TSGetBitsFromU8(&pu8Section[5], 1, 0x1f);

        printf("\n PAT parse - Size = 0x%x\n", u16SecSize);
        printf("...................    TSID = %u\n", u16TsId);
        printf("Ver = %u,u16NumOfItem = %u\n", u8Version, u16NumOfItem);

        pu8SecIter = pu8Section + SECT_HEADER_LEN;
        u16ServiceIndex = 0;

        for (j = 0; j < u16NumOfItem; j++) {
            temp1 = pu8SecIter[0];
            temp2 = pu8SecIter[1];
            temp3 = pu8SecIter[2];
            temp4 = pu8SecIter[3];
            //printf("temp1=0x%x,temp2=0x%x,temp3=0x%x,temp4=0x%x,\n ",temp1,temp2,temp3,temp4);
            if (((temp1 << 8) | temp2) && (u16ServiceIndex < 120)) {
                u16ServiceID[u16ServiceIndex] = (temp1 << 8) + temp2;
                temp5 = (temp1 << 8) + temp2;
                //printf("u16ServiceID=0x%x,temp5=0x%x,\n ", u16ServiceID[u16ServiceIndex],temp5);
                u16PmtPID[u16ServiceIndex] = ((temp3 << 8) + temp4) & 0x1fff;
                printf("PMT PID = 0x%x, Program ID = 0x%x\n", u16PmtPID[u16ServiceIndex], u16ServiceID[u16ServiceIndex]);

                _DmxGetPMTTbl(u16PmtPID[u16ServiceIndex], u16ServiceID[u16ServiceIndex], j);

                u16ServiceIndex++;
            }
            pu8SecIter += 4;
        }
        usedSecLen += m_u16SecLengthCount;
        m_u16SecLengthCount = TSGetBitsFromU16(&pu8Section[1], 0, 0x0fff) + 3;
    }
    /*##########################END TEST PMT######################################
     #############################################################################*/

#endif
    free(pu8BUF);
    printf("get pf data sucess...................\n");
    _uRecvDatafromDMX = AM_TRUE;

    return MW_OK;
}

static int get_section() {
    DMX_FILTER_PARAM rFilter;

    if (AM_FALSE == Tapi_Demux_AllocateFilter(DMX_FILTER_TYPE_SECTION, 0, 0, &_u4PatIndex)) {
        printf("AllocateFilter failed!!!!!!!!!!!!!\n");
        return AM_FALSE;
    } else {
        printf("[%s:%d] :Allocate Filter[%d] successfully \n", __FUNCTION__, __LINE__, (int) _u4PatIndex);
    }

#if 0
    //request NIT table
    _DmxGetNITTbl(0x12,0x40,_u4PatIndex);

#else
    // rFilter.u2Pid     = 0; //PID_EIT_CIT;
    rFilter.u2Pid = 0; //PID_EIT_CIT;
    rFilter.ePidType = DMX_PID_TYPE_PSI;
    rFilter.pfnNotify = _DmxPatCallback;
    rFilter.pvNotifyTag = NULL;
    rFilter.fgCheckCrc = 1;
    rFilter.u1FilterIndex = _u4PatIndex;
    rFilter.u1Offset = 1;
    memset(rFilter.au1Data, 0, 16);
    memset(rFilter.au1Mask, 0, 16);
    rFilter.au1Data[0] = 0;
    rFilter.au1Mask[0] = 0xFF;
    rFilter.timeOut = 20000;

    if (AM_FALSE == Tapi_Demux_SetSectionFilterParam(_u4PatIndex, rFilter)) {
        printf("setSectionFilter error........\n");
        return AM_FALSE;
    } else {
        printf("[%s:%d] :set Filter[%d] successfully \n", __FUNCTION__, __LINE__, (int) _u4PatIndex);
    }

    if (AM_FALSE == Tapi_Demux_Start(_u4PatIndex)) {
        printf("Tapi_Demux_Start(_u4PatIndex) failed!!!!!!!!!!!!!!!!!!!!!\n");
        return AM_FALSE;
    } else {
        printf("[%s:%d] :start Filter[%d]successfully \n", __FUNCTION__, __LINE__, (int) _u4PatIndex);
    }

#endif
    sleep(20000);

    return AM_TRUE;
}

int demux_test_main(int argc, char **argv) {
    AM_DMX_OpenPara_t para;
    int freq = 0;

    TAPI_BOOL ret = TAPI_FALSE;
    DEMOD_SETTING demoSet;
    int rate;
    EN_CAB_CONSTEL_TYPE qam;
    RF_CHANNEL_BANDWIDTH bandwith;
    EN_LOCK_STATUS lockStatus = E_DEMOD_NULL;

    // initialize demod
    ret = Tapi_Tuner_Connect();
    printf(" %s :%d\n", "Tapi_Tuner_Connect", ret);
    ret = Tapi_Tuner_Init();
    printf(" %s :%d\n", "Tapi_Tuner_Init", ret);

    ret = Tapi_Demod_Initialization();
    printf(" %s :%d\n", "Tapi_Demod_Initialization", ret);
    ret = Tapi_Demod_Connect(E_DEVICE_DEMOD_DVB_C);
    printf(" %s :%d\n", "Tapi_Demod_Connect", ret);
    ret = Tapi_Demod_IIC_Bypass_Mode(TAPI_FALSE);
    printf(" %s :%d\n", "Tapi_Demod_Initialization", ret);

    //set FrontEnd
    freq = 427000; //435000;//259;
    bandwith = E_RF_CH_BAND_8MHz;
    rate = 6875;
    qam = E_CAB_QAM64;

    Tapi_Tuner_DTV_SetTune(freq * 1000, bandwith, E_TUNER_DTV_DVB_C_MODE);
    printf(" %s :%d\n", "Tapi_Tuner_DTV_SetTune", ret);

    demoSet.u32Frequency = freq;
    demoSet.eBandWidth = (RF_CHANNEL_BANDWIDTH) bandwith;
    demoSet.uSymRate = rate;
    demoSet.eQAM = qam;
    ret = Tapi_Demod_DVB_SetFrequency(demoSet);
    printf(" %s :%d\n", "Tapi_Demod_DVB_SetFrequency", ret);
    usleep(2000 * 1000);

    for (int i = 0; i < 50; i++) {
        lockStatus = Tapi_Demod_GetLockStatus();
        if (lockStatus == E_DEMOD_LOCK) {
            printf(" %s \n", "Tapi_Demod_GetLockStatus  LOCK ");
            break;
        } else if (lockStatus == E_DEMOD_UNLOCK) {
            printf(" %s \n", "Tapi_Demod_GetLockStatus  UNLOCK");
            break;
        } else { //unknown, polling again
            usleep(20000);
        }
    }

    if (lockStatus != E_DEMOD_LOCK) {
        printf("lock faild............\n");
        return 0;
    }

    if (AM_FALSE == Tapi_Demux_Init()) {
        printf("init demux faild............\n");
        return 0;
    }

    //Create ut demux semaphore
    _ut_demux_CreateSemaphore();
    memset(TclPMTCallback, 0, Callback_Max_Number * sizeof(PMTCallback));
    u4MtalLogMask = (1 << 3); // set mtdmx debug

    get_section();
    usleep(100000);
    if (AM_FALSE == Tapi_Demux_Exit()) {
        printf("exit demux fail.................\n");
    } else {
        printf("=========exit demux success!!!!!!!!==============\n");
    }

    return 0;
}

