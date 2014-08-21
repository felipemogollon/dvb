/***********AV modules test case*******************/

#define AM_DEBUG_LEVEL 5

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/types.h>

#include <am_debug.h>
#include <am_dmx.h>
#include <am_fend.h>

#include "../include/std_defs.h"
#include "TTvHW_demux.h"
#include "TTvHW_tuner.h"
#include "TTvHW_demodulator.h"
#include "TTvHW_decoder.h"

#include "../include/am_av.h"
#include "am_aout.h"
#include "../av/av_local.h"


#define     Callback_Max_Number   64

TAPI_U16 Tapi_Vdec_GetFrameCnt(void);
handle_t get_avhandler(void);
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

//----------------------------------------------
// Type definitions
//----------------------------------------------
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

#define AV_TEST_CONST2STR(constant, str_msg)	 {case constant: str_msg =  # constant;break;}

static void dump_bytes(int dev_no, int fid, const uint8_t *data, int len, void *user_data) {
    int i;

    printf("section:\n");
    for (i = 0; i < len; i++) {
        printf("%02x ", data[i]);
        if (((i + 1) % 16) == 0) {
            printf("\n");
        }
    }

    if ((i % 16) != 0) {
        printf("\n");
    }
}

/*****************************************************************************
 * PSI_AddProgramInfo
 * Creation of a new psi_program_info_t  to list.
 *****************************************************************************/
static TAPI_BOOL PSI_AddProgramInfo(psi_program_info_t *p_program_info) {
    if (psi_program_count + 1 < _MAX_PMT_NUM) {
        g_psi_program_info_list[psi_program_count].i_program_number = p_program_info->i_program_number;
        g_psi_program_info_list[psi_program_count].i_pcr_pid = p_program_info->i_pcr_pid;
        g_psi_program_info_list[psi_program_count].v_type = p_program_info->v_type;
        g_psi_program_info_list[psi_program_count].v_pid = p_program_info->v_pid;
        g_psi_program_info_list[psi_program_count].a_type = p_program_info->a_type;
        g_psi_program_info_list[psi_program_count].a_pid = p_program_info->a_pid;

        printf("\n*********add =======program_info_list[%d]**************\n", psi_program_count);
        printf("program_info_list[%d].i_program_number = %x\n", psi_program_count, g_psi_program_info_list[psi_program_count].i_program_number);
        printf("program_info_list[%d].i_pcr_pid= %x\n", psi_program_count, g_psi_program_info_list[psi_program_count].i_pcr_pid);
        printf("program_info_list[%d].v_type= %x\n", psi_program_count, g_psi_program_info_list[psi_program_count].v_type);
        printf("program_info_list[%d].v_pid= %x\n", psi_program_count, g_psi_program_info_list[psi_program_count].v_pid);
        printf("program_info_list[%d].a_type= %x\n", psi_program_count, g_psi_program_info_list[psi_program_count].a_type);
        printf("program_info_list[%d].a_pid= %x\n", psi_program_count, g_psi_program_info_list[psi_program_count].a_pid);

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
static void PSI_GetProgramInfo(TAPI_U8 *ProgramCount, psi_program_info_t **p_program_info_list) {
    *ProgramCount = psi_program_count;
    *p_program_info_list = (psi_program_info_t*) &g_psi_program_info_list[0];
}

/*****************************************************************************
 * PSI_GetProgramInfo()
 * Creation of a new psi_descriptor_t structure.
 *****************************************************************************/
static void PSI_DumpProgramInfo_list() {
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
            printf("i_program_number = 0x%x \n", p_program_info->i_program_number);
            printf("i_pcr_pid = 0x%x \n", p_program_info->i_pcr_pid);
            printf("audio_type = 0x%x ,audio_pid = 0x%x\n", p_program_info->a_type, p_program_info->a_pid);
            printf("video_type = 0x%x ,video_pid = 0x%x\n", p_program_info->v_type, p_program_info->v_pid);
            //printf("\n=======================program[%d]  end===============\n",i);
            p_program_info = &p_program_info_list[++i];
        }
    }
}
/*****************************************************************************
 * PSI_NewDescriptor
 * Creation of a new psi_descriptor_t structure.
 *****************************************************************************/
static psi_descriptor_t* PSI_NewDescriptor(TAPI_U8 i_tag, TAPI_U8 i_length, TAPI_U8* p_data) {
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

static void PSI_EmptyPMT(psi_pmt_t *p_pmt) {
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

static void PSI_DUMP_VA_PID_INFO(psi_pmt_es_t *p_es, psi_program_info_t *program_info) {
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

static void PSI_DumpPMT(psi_pmt_t* p_pmt, psi_program_info_t *program_info) {
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
static void PSI_DecodePMTSections(psi_pmt_t* p_pmt, void *p_section) {
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
    rFilter.au1Data[4] = 0xd8; //(TAPI_U8)(ServiceId);   // stVirtualCh.wService_ID)
    rFilter.au1Data[3] = 0; //(TAPI_U8)((ServiceId>>8) & 0xff);    // stVirtualCh.wService_ID);
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
    printf("[%s:%d] :successfully \n", __FUNCTION__, __LINE__);

    usleep(5000 * 1000);

    return AM_TRUE;
}

void Getprograminfo(hdi_av_settings_t *set, TAPI_U16 ProgramCount) {
    psi_program_info_t *p_program_info_list = NULL;
    TAPI_U8 TotalProgramCount = 0;
    psi_program_info_t *p_program_info = NULL;
    int i = 0;

    //printf("\n=======================PSI_DumpProgramInfo_list===============\n");
    PSI_GetProgramInfo(&TotalProgramCount, &p_program_info_list);

    printf("TotalProgramCount = %d ,p_program_info_list = %8x\n", TotalProgramCount, p_program_info_list);
    if (TotalProgramCount > 0 && p_program_info_list != NULL) {
        p_program_info = &p_program_info_list[0];
        while (p_program_info != NULL && i < TotalProgramCount) {
            if (ProgramCount == p_program_info->i_program_number)
                break;
            p_program_info = &p_program_info_list[++i];
        }
        set->m_vtype = (AM_AV_VFormat_t)(p_program_info->v_type);
        set->m_atype = (AM_AV_AFormat_t)(p_program_info->a_type);
        set->m_vid_pid = p_program_info->v_pid;
        set->m_aud_pid = p_program_info->a_pid;
    }

    printf("[%s:%d] :vtype : %d, atype :%d, vid : %x, aid: %x \n", __FUNCTION__, __LINE__, p_program_info->v_type, p_program_info->a_type, set->m_vid_pid, set->m_aud_pid);

}
static void av_test_evt2str(TAPI_U32 evt, void* udata) {
    char * str_msg = "UNKNOWN AV EVT";

    switch (evt) {
    AV_TEST_CONST2STR(E_TAPI_VDEC_EVENT_DEC_ONE, str_msg);
    AV_TEST_CONST2STR(E_TAPI_VDEC_EVENT_DISP_INFO_CHG, str_msg);
default:
    break;
    }

    printf("av_test_evt2str, %s\n", str_msg);

}
static void Test_Callback() {
    EN_VDEC_EVENT_FLAG evt1 = E_TAPI_VDEC_EVENT_DEC_ONE;
    EN_VDEC_EVENT_FLAG evt2 = E_TAPI_VDEC_EVENT_DISP_INFO_CHG;

    Tapi_Vdec_SetCallBack(E_TAPI_VDEC_EVENT_DEC_ONE, av_test_evt2str, &evt1);
    Tapi_Vdec_SetCallBack(E_TAPI_VDEC_EVENT_DISP_INFO_CHG, av_test_evt2str, &evt2);
}
static void normal_help(void) {
    printf("*1 quit\n");
    printf("*2 Tapi_Vdec_Finalize\n");
    printf("*3 Tapi_Vdec_SetType\n");
    printf("*4 Tapi_Vdec_Stop\n");
    printf("*5 Tapi_Vdec_Play\n");
    printf("*6 Tapi_Vdec_GetActiveVdecType\n");
    printf("*7 Tapi_Vdec_SyncOn\n");
    printf("*8 Tapi_Vdec_GetDispInfo\n");
    printf("*9 Tapi_Vdec_GetScrambleState\n");
    printf("*10 Tapi_Adec_GetScrambleState\n");
    printf("*11 Tapi_Vdec_GetFrameCnt\n");
    printf("*12 Tapi_Adec_Init\n");
    printf("*13 Tapi_Adec_SetAbsoluteVolume\n");
    printf("*14 Tapi_Adec_SetType\n");
    printf("*15 Tapi_Adec_Stop\n");
    printf("*16 Tapi_Adec_Play\n");
    printf("*17 Tapi_Adec_SetOutputMode\n");
    printf("*18 Tapi_Vdec_SetCallBack\n");
    printf("*19 Test callback\n");
    printf("*20 Tapi_Vdec_UnSetCallBack\n");
}

static int normal_cmd(const int cmd) {
    TAPI_BOOL ret = TAPI_FALSE;

    printf("cmd:%d\n", cmd);

    if (cmd == 2) {
        ret = Tapi_Vdec_Finalize();
        if (ret != TAPI_TRUE) {
            fprintf(stderr, "Tapi_Vdec_Finalize failed: %d\n", ret);
            return 1;
        }
    } else if (cmd == 3) {
        int codec_type;
        printf("input vd type: 1~4\n");
        printf("(1: MPEG2, 2: H264, 3: AVS, 4: VC1)\n");
        scanf("%d", &codec_type);
        ret = Tapi_Vdec_SetType((CODEC_TYPE) codec_type);
        if (ret != TAPI_TRUE) {
            fprintf(stderr, "Tapi_Vdec_SetType failed: %d\n", ret);
            return 1;
        }
    } else if (cmd == 4) {
        ret = Tapi_Vdec_Stop();
        if (ret != TAPI_TRUE) {
            fprintf(stderr, "Tapi_Vdec_Stop failed: %d\n", ret);
            return 1;
        }
    } else if (cmd == 5) {
        ret = Tapi_Vdec_Play();
        if (ret != TAPI_TRUE) {
            fprintf(stderr, "Tapi_Vdec_Play failed: %d\n", ret);
            return 1;
        }
    } else if (cmd == 6) {
        CODEC_TYPE vtype;
        ret = Tapi_Vdec_GetActiveVdecType(&vtype);
        if (ret != TAPI_TRUE) {
            fprintf(stderr, "Tapi_Vdec_GetActiveVdecType failed: %d\n", ret);
            return 1;
        }
        printf("Tapi_Vdec_GetActiveVdecType vtype: %d\n", vtype);

    } else if (cmd == 7) {
        ret = Tapi_Vdec_SyncOn(1);
        if (ret != TAPI_TRUE) {
            fprintf(stderr, "Tapi_Vdec_SyncOn failed: %d\n", ret);
            return 1;
        }
    } else if (cmd == 8) {
        VDEC_DISP_INFO pstDispInfo;

        memset(&pstDispInfo, 0, sizeof(VDEC_DISP_INFO));
        ret = Tapi_Vdec_GetDispInfo(&pstDispInfo);
        if (ret != TAPI_TRUE) {
            fprintf(stderr, "Tapi_Vdec_GetDispInfo failed: %d\n", ret);
            return 1;
        }
        printf(" u32FrameRate=%ld\n", pstDispInfo.u32FrameRate);
        printf(" u16Width=%d\n", pstDispInfo.u16Width);
        printf(" u16Height=%d\n", pstDispInfo.u16Height);
        printf(" enAspectRate=%d\n", pstDispInfo.enAspectRate);
        printf(" u8Interlace=%d\n", pstDispInfo.u8Interlace);
        printf(" IsHD =%d\n", pstDispInfo.b_IsHD);
    } else if (cmd == 9) {
        En_DVB_SCRAMBLE_STATE peScramble_State;
        ret = Tapi_Vdec_GetScrambleState(1, &peScramble_State);
        if (ret != TAPI_TRUE) {
            fprintf(stderr, "Tapi_Adec_GetScrambleState failed: %d\n", ret);
            return 1;
        }
        printf("\nVD peScramble_State=%d\n", peScramble_State);
    } else if (cmd == 10) {
        En_DVB_SCRAMBLE_STATE peScramble_State;
        ret = Tapi_Adec_GetScrambleState(1, &peScramble_State);
        if (ret != TAPI_TRUE) {
            fprintf(stderr, "Tapi_Adec_GetScrambleState failed: %d\n", ret);
            return 1;
        }
        printf("\naud peScramble_State=%d\n", peScramble_State);
    } else if (cmd == 11) {
        TAPI_U16 frmcnt = Tapi_Vdec_GetFrameCnt();
        printf("Tapi_Vdec_GetFrameCnt()=%d\n", frmcnt);
        return 1;
    } else if (cmd == 12) {
        if (Tapi_Adec_Init() != TAPI_TRUE) {
            fprintf(stderr, "Tapi_Adec_Init failed: %d\n", ret);
            return 1;
        }
    } else if (cmd == 13) {
        TAPI_U8 uPercent, audiopath;
        printf("input audio audiopath: : (0: BYUSER, 1: BYUSER, 2: SPEAKER, 3: HP, 4: SPDIF)\n");
        printf("audiopath(0/1/2/3/4): ");
        scanf("%d", &audiopath);

        printf("input audio uPercent:");
        scanf(" %d", &uPercent);

        Tapi_Adec_SetAbsoluteVolume(uPercent, (AudioPath_) audiopath);
    } else if (cmd == 14) {
        int codec_type;
        printf("input aud type: 0~9\n");
        printf("(0: MPEG, 1: AC3, 2: AC3_AD, 3: SIF, 4: AAC, 5: AACP,)\n");
        printf("(6: MPEG_AD, 7: AC3P, 8: AC3P_AD, 9: AACP_AD)\n");
        scanf("%d", &codec_type);
        ret = Tapi_Adec_SetType((AUDIO_DSP_SYSTEM_) codec_type);
        if (ret != TAPI_TRUE) {
            fprintf(stderr, "Tapi_Adec_SetType failed: %d\n", ret);
            return 1;
        }
    } else if (cmd == 15) {
        if (Tapi_Adec_Stop() != TAPI_TRUE) {
            fprintf(stderr, "Tapi_Adec_Stop failed: %d\n", ret);
            return 1;
        }
    } else if (cmd == 16) {
        if (Tapi_Adec_Play() != TAPI_TRUE) {
            fprintf(stderr, "Tapi_Adec_Play failed: %d\n", ret);
            return 1;
        }
    } else if (cmd == 17) {
        int aud_channel;
        printf("input output mode: 0~3\n");
        printf("(0: STEREO, 1: LEFT, 2: RIGHT, 3: MIXED)\n");
        scanf("%d", &aud_channel);
        if (Tapi_Adec_SetOutputMode((En_DVB_soundModeType_) aud_channel) != TAPI_TRUE) {
            fprintf(stderr, "Tapi_Adec_SetOutputMode failed: %d\n", ret);
            return 1;
        }
    } else if (cmd == 18) {
        Test_Callback();
        return 1;
    } else if (cmd == 19) {
        am_send_all_event (E_TAPI_VDEC_EVENT_DEC_ONE);
        am_send_all_event (E_TAPI_VDEC_EVENT_DISP_INFO_CHG);
        return 1;
    } else if (cmd == 20) {
        Tapi_Vdec_UnSetCallBack (E_TAPI_VDEC_EVENT_DEC_ONE);
        Tapi_Vdec_UnSetCallBack (E_TAPI_VDEC_EVENT_DISP_INFO_CHG);
        return 1;
    }

    return 0;
}

int av_test_main(int argc, char **argv) {
    AM_DMX_OpenPara_t para;
    int freq = 0;
    hdi_av_settings_t set;
    handle_t handle;
    status_code_t rst = SUCCESS;
    TAPI_BOOL ret = TAPI_FALSE;
    DEMOD_SETTING demoSet;
    int rate;
    EN_CAB_CONSTEL_TYPE qam;
    RF_CHANNEL_BANDWIDTH bandwith;
    EN_LOCK_STATUS lockStatus = E_DEMOD_NULL;
    TAPI_BOOL running = TAPI_TRUE;
    int cmd = 0;
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
    freq = 427000; //259;
    bandwith = E_RF_CH_BAND_8MHz;
    rate = 6875 * 1000;
    qam = E_CAB_QAM64;

    Tapi_Tuner_DTV_SetTune(freq * 1000, bandwith, E_TUNER_DTV_DVB_C_MODE);
    printf(" %s :%d\n", "Tapi_Tuner_DTV_SetTune", ret);

    demoSet.u32Frequency = freq * 1000;
    demoSet.eBandWidth = (RF_CHANNEL_BANDWIDTH) bandwith;
    demoSet.uSymRate = rate;
    demoSet.eQAM = qam;
    ret = Tapi_Demod_DVB_SetFrequency(demoSet);
    printf(" %s :%d\n", "Tapi_Demod_DVB_SetFrequency", ret);
    usleep(2000 * 1000);

    for (int i = 0; i < 500; i++) {
        lockStatus = Tapi_Demod_GetLockStatus();
        if (lockStatus == E_DEMOD_LOCK) {
            printf(" %s \n", "Tapi_Demod_GetLockStatus  LOCK ");
            break;
        } else if (lockStatus == E_DEMOD_UNLOCK) {
            printf(" %s \n", "Tapi_Demod_GetLockStatus  UNLOCK");
            break;
        } else { //unknown, polling again
            usleep(1000);
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
    get_section();

    ret = Tapi_Vdec_Init();
    printf(" %s :%d\n", "Tapi_Vdec_Init", ret);
    handle = get_avhandler();

    ret = am_av_enable_video(handle, HDI_AV_CHANNEL_HD);
    if (ret != TAPI_TRUE) {
        fprintf(stderr, "hdi_av_enable_video failed: %d\n", ret);
        return 1;
    }

    memset(&set, 0, sizeof(set));
    Getprograminfo(&set, 0xd8); //tianwei is no 5

    set.m_av_sync_mode = HDI_AV_SYNC_MODE_AUTO;
    set.m_vid_stop_mode = HDI_AV_VID_STOP_MODE_FREEZE;
    set.m_is_vid_decode_once = 0;
    set.m_is_mute = 0;
    set.m_left_vol = 100;
    set.m_right_vol = 100;
    //set.m_vid_stream_type = HDI_AV_VID_STREAM_TYPE_MPEG2;
    //set.m_aud_stream_type = HDI_AV_AUD_STREAM_TYPE_MPEG;
    set.m_aud_channel = HDI_AV_AUD_CHANNEL_STER;
    //set.m_vid_pid = 1921;//640;
    //set.m_aud_pid = 1922;//641;

    rst = hdi_av_set(handle, &set);
    if (rst != SUCCESS) {
        fprintf(stderr, "hdi_av_set failed: %d\n", ret);
        return 1;
    }
    //Tapi_Vdec_Play();
    hdi_av_start(handle);

    while (running) {
        printf("********************\n");
        normal_help();
        scanf("%d", &cmd);
        //if(gets(buf))
        {
            if (cmd == 1) {
                running = 0;
                printf("cmd:%d\n", cmd);
            } else {
                normal_cmd(cmd);
            }
        }

    }
    Tapi_Demod_Disconnect();
    Tapi_Vdec_Finalize();
    printf("=========exit av success!!!!!!!!==============\n");
    return 0;
}
