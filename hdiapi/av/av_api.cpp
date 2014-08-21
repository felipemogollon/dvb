#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <linux/kernel.h>
#include <cutils/properties.h>
#include <android/log.h>

#include "../include/std_defs.h"
#include "../version/version.h"
#include "av_core.h"
#include "../include/am_av.h"
#include "am_vout.h"
#include "am_aout.h"
#include "am_dmx.h"
#include "am_misc.h"
#include "am_util.h"
#include "am_time.h"
#include "am_debug.h"
#include "TTvHW_player.h"
#include "TTvHW_decoder.h"
#include "av_local.h"
#include "../include/hdi_config.h"

#define LOG_TAG "av_api"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define AV_HANDLE_COUNT 10

#define VOUT_DEV_NO     0
#define AOUT_DEV_NO     0
#define AV_DEV_NO       0
#define AOUT_VOL_MAX    100

#define CLIP_LEFT   30
#define CLIP_RIGHT  30
#define CLIP_TOP    30
#define CLIP_BOTTOM 30
#define SCREEN_W    1920
#define SCREEN_H    1080

#define DMX_SRC_FILE     "/sys/class/stb/demux0_source"
#define VID_AXIS_FILE       "/sys/class/video/axis"
#define _BLUE_COLOR_ "0x29f06e"
#define _BLACK_COLOR_ "0x108080"

#define AV_QUIT 1
#define AV_AUD_START 2
#define AV_VID_START 4
#define WARNING(msg...) \
	do{\
		fprintf(stderr, "\"%s\" %d: ", __FILE__, __LINE__);\
		fprintf(stderr, msg);\
		fprintf(stderr, "\n");\
	}while(0)

#if 1
#define DEBUG(a...)\
	do{\
		fprintf(stderr, "\"%s\" %d debug: ", __FILE__, __LINE__);\
		fprintf(stderr, a);\
		fprintf(stderr, "\n");\
	}while(0)

#define DEBUG_TIME(a...)\
    do{\
        int clk;\
        AM_TIME_GetClock(&clk);\
        fprintf(stderr, "%dms ",clk);\
        fprintf(stderr, a);\
        fprintf(stderr, "\n");\
    }while(0)
#else
#define DEBUG(a...)
#define DEBUG_TIME(a...)
#endif

struct AV_Handle_s {
    int used;
    //Injecter *inj;
    hdi_av_open_params_t params;
    hdi_av_settings_t av_settings;
    hdi_av_dispsettings_t disp_settings;
    hdi_av_default_dispsettings_t def_disp_settings;
    hdi_av_coord_t in_left;
    hdi_av_coord_t in_top;
    U32 in_width;
    U32 in_height;
    hdi_av_coord_t out_left;
    hdi_av_coord_t out_top;
    U32 out_width;
    U32 out_height;
    hdi_av_decoder_state_t a_state;
    hdi_av_decoder_state_t v_state;
    Tapi_av_evt_config_params_t events[E_TAPI_VDEC_EVENT_MAX];
    VDEC_STATUS vdec_status;
    int already_started;
    AM_AOUT_OutputMode_t AOUTmode;
    int need_start_ts;
    int only_update_audio;
};

typedef struct AV_Handle_s AV_Handle;

typedef struct AV_scambled_s {
    int video;
    int audio;
    int no_data;
}scambled_status_t;

static pthread_mutex_t scambled_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
static scambled_status_t scambled_status;

static AV_Handle handles[AV_HANDLE_COUNT];
static int init_ok = 0;
static int fl_mute;
static int fl_hide;
static int fl_suspend;
static int fl_blackout;
static int fl_enablev = 1;
static hdi_av_source_type_t a_source, v_source;
static int a_start_time, v_start_time;
static int aud_pid = 0x1FFF, vid_pid = 0x1FFF;
static hdi_av_init_params_t init_params;
static int fl_decode_vid_es;
static int fl_tuner_mode;
static int fl_inject_mode;
static pthread_mutex_t lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_t av_thread;
static pthread_t av_socket_thread = -1;
static handle_t gAVHandle = -1;
static int av_flags;
static U32 first_apts, first_vpts;

static int deinterlace_mode = -1;
static int ppmgr_mode = -1;
static int video_start = 0;
static int m_small_window = 0;
static int m_preview_init = -55;
static volatile int bDtvStartup = TAPI_FALSE;
static volatile int bDtvEPG = TAPI_FALSE;

handle_t get_avhandler(void) {
    return gAVHandle;
}

#if 0
static void test_and_write_dev(char *dev, int val) {
    AM_ErrorCode_t rc;
    char buf[32];
    int tmp;

    rc = AM_FileRead(dev, buf, sizeof(buf));
    if (rc < 0) {
        tmp = 0;
    } else {
        sscanf(buf, "%d", &tmp);
    }
    if (tmp != val) {
        char str[32];
        sprintf(str, "%d", val);
        AM_FileEcho(dev, str);
    }
}
#else
static void test_and_write_dev(char *dev, char * val) {
    AM_ErrorCode_t rc;
    char buf[32];

    rc = AM_FileRead(dev, buf, sizeof(buf));
    if (rc < 0) {
        buf[0] = 0;
    }
    if(strcmp(buf, val) != 0)
       AM_FileEcho(dev, val);
}
#endif

/*0: is black 1:blue*/
static char * am_get_colorForBackground(TAPI_BOOL bBlue) {
    char * cblue = _BLUE_COLOR_;
    char * cblack = _BLACK_COLOR_;

     if(bBlue && !bDtvEPG)
         return cblue;
     else
         return cblack;
}

#define VID_AXIS_FILE       "/sys/class/video/axis"
static TAPI_BOOL is_epg_mode(){
	int left, top, right, bottom;
	char buf[32];
	
	if(AM_FileRead(VID_AXIS_FILE, buf, sizeof(buf))==AM_SUCCESS)
	{
		if(sscanf(buf, "%d %d %d %d", &left, &top, &right, &bottom)==4)
		{
			if(right-left < 400 && bottom-top < 200)
			return TAPI_TRUE;
		}
	}
	return TAPI_FALSE;
}

static AM_AV_TSSource_t am_get_ts_source_by_dmx_id(int dmx_id) {
    AM_AV_TSSource_t ret = AM_AV_TS_SRC_DMX0;

    switch (dmx_id) {
    case 1: //HDI_DMX_ID_0:
        ret = AM_AV_TS_SRC_DMX0;
        break;
    case 2: //HDI_DMX_ID_1:
        ret = AM_AV_TS_SRC_DMX1;
        break;
    case 3: //HDI_DMX_ID_2:
        ret = AM_AV_TS_SRC_DMX2;
        break;
    default:
        break;
    }

    LOGD("get_ts_source_by_dmx_id, dmx_id %d, source %d", dmx_id, ret);

    return ret;
}

static AM_ErrorCode_t am_set_vpath(int di, int pp) {
    AM_ErrorCode_t rc = AM_SUCCESS;
    char prop_value[32];
    int flag_project = -1;

    memset(prop_value, '\0', 32);
    property_get("ro.build.product", prop_value, "e00ref");

    if (strcmp(prop_value, "e13ref") == 0 || strcmp(prop_value, "e15ref") == 0) {
        LOGD("set free scale for e13 and e15....");
        flag_project = 5;
    }
    LOGD("...begin set vpath di:%d pp:%d window:%d init:%d", di, pp, m_small_window, m_preview_init);

    LOGD(".111.......... dI: %d ppmgr: %d", deinterlace_mode, ppmgr_mode);
    if (di == -1) {
        di = deinterlace_mode;
    }
    if (pp == -1) {
        pp = ppmgr_mode;
    }

    if (di == -1) {
        di = AM_AV_DEINTERLACE_DISABLE;
    }
    if (pp == -1) {
        pp = AM_AV_PPMGR_DISABLE;
    }

    if (m_small_window) {
        di = AM_AV_DEINTERLACE_DISABLE;
    }

    //Joey for preview to force close DI
    if ((deinterlace_mode == -1) && (ppmgr_mode == -1) && (m_preview_init != 59)) {
        m_small_window = 1;
        di = AM_AV_DEINTERLACE_DISABLE;
    }

    //if hdi_av_leave_menu be called, then we must open DI
    if (m_preview_init == 59) {
        di = AM_AV_DEINTERLACE_ENABLE;
    }

    if (m_preview_init < 0) {
        di = AM_AV_DEINTERLACE_DISABLE;
    }

    LOGD("........... dI: %d ppmgr: %d, m_small_window:%d", deinterlace_mode, ppmgr_mode, m_small_window);

    if (di == deinterlace_mode && pp == ppmgr_mode) {
        return rc;
    }

    LOGD("........... real set vpath %d %d", di, pp);
    if (((m_small_window > 0) || (m_preview_init < 0)) && (flag_project > 0)) {
        rc = AM_AV_SetVPathPara(AV_DEV_NO, AM_AV_FREE_SCALE_ENABLE, (AM_AV_DeinterlacePara_t) di, (AM_AV_PPMGRPara_t) pp);
    } else {
        rc = AM_AV_SetVPathPara(AV_DEV_NO, AM_AV_FREE_SCALE_DISABLE, (AM_AV_DeinterlacePara_t) di, (AM_AV_PPMGRPara_t) pp);
    }

    if (rc == AM_SUCCESS) {
        deinterlace_mode = di;
        ppmgr_mode = pp;

        if (!m_small_window) {
            if (pp != AM_AV_PPMGR_DISABLE) {
                AM_AV_SetVideoWindow(AV_DEV_NO, 0, 0, SCREEN_W, SCREEN_H);
            } else {
                AM_AV_SetVideoWindow(AV_DEV_NO, -CLIP_LEFT, -CLIP_TOP, SCREEN_W + CLIP_LEFT + CLIP_RIGHT, SCREEN_H + CLIP_TOP + CLIP_BOTTOM);
            }
        }
    }

    return rc;
}

static void am_get_vid_status(AV_Handle *h, hdi_av_vid_status_t *p_status) {
    AM_AV_VideoStatus_t vs;
    AM_ErrorCode_t rc;
    hdi_av_format_t pfmt;
    U32 pts;
    char buf[64];

    memset(p_status, 0, sizeof(hdi_av_vid_status_t));
    memset(&vs, 0, sizeof(AM_AV_VideoStatus_t));

    rc = AM_AV_GetVideoStatus(AV_DEV_NO, &vs);
    LOGD("%s, src res: %d X %d, interlace = %d, fmt = %d.\n", __FUNCTION__, vs.src_w, vs.src_h, vs.interlaced, vs.vid_fmt);
    if (rc >= 0) {
        if (!vs.fps) {
            p_status->m_packet_count = 0;
            p_status->m_disp_pic_count = 0;
        } else {
            int now;

            AM_TIME_GetClock(&now);
            p_status->m_disp_pic_count = (now - a_start_time) * vs.fps / 1000;
            p_status->m_packet_count = p_status->m_disp_pic_count * 4096;
        }

        p_status->m_stream_type = vs.vid_fmt;

        if (vs.src_h <= 480) {
            pfmt = HDI_AV_FORMAT_NTSC;
        } else if (vs.src_h <= 576) {
            pfmt = vs.interlaced ? HDI_AV_FORMAT_PAL : HDI_AV_FORMAT_576P;
        } else if (vs.src_h <= 720) {
            pfmt = HDI_AV_FORMAT_HD_720P;
        } else {
            pfmt = vs.interlaced ? HDI_AV_FORMAT_HD_1080I : HDI_AV_FORMAT_HD_1080P;
        }
        p_status->m_stream_format = pfmt;

        p_status->m_frame_rate = (hdi_av_vid_frame_rate_t)(vs.fps);

        if (vs.src_w * 9 >= vs.src_h * 16) {
            p_status->m_aspect_ratio = HDI_AV_ASPECT_RATIO_16TO9;
        } else {
            p_status->m_aspect_ratio = HDI_AV_ASPECT_RATIO_4TO3;
        }

        p_status->m_pes_buffer_size = vs.vb_size;
        p_status->m_pes_buffer_free_size = vs.vb_free;
        p_status->m_es_buffer_size = vs.vb_size;
        p_status->m_es_buffer_free_size = vs.vb_free;
        p_status->m_source_width = vs.src_w;
        p_status->m_source_height = vs.src_h;
        p_status->m_is_interlaced = vs.interlaced;
    }

    p_status->m_decode_state = h->v_state;
    p_status->m_source_type = v_source;

    rc = AM_FileRead("/sys/class/tsync/pts_video", buf, sizeof(buf));
    if (rc < 0) {
        pts = 0;
    } else {
        sscanf(buf, "%lx", &pts);
    }
    p_status->m_stc = pts;
    p_status->m_pts = pts;
    p_status->m_first_pts = first_vpts;
    p_status->m_pid = vid_pid;

    LOGD("%s, m_pts: %lu , first_vpts = %lu, vid_pid = %d.src type= %d\n", __FUNCTION__, p_status->m_pts, p_status->m_first_pts, p_status->m_pid, v_source);
}

static void am_send_event(handle_t handle, EN_VDEC_EVENT_FLAG evt) {
    AV_Handle *h;

    TAPI_U32 statu;

    h = &handles[handle];

    if (h && h->used && h->events[evt].m_p_callback && h->events[evt].m_is_enable) {
        if (h->events[evt].m_notifications_to_skip) {
            h->events[evt].m_notifications_to_skip--;
        } else if (h->events[evt].m_evt == evt) {
            h->events[evt].m_p_callback(statu, h->events[evt].udata);
        }
    }
}

void am_send_all_event(EN_VDEC_EVENT_FLAG evt) {
    int i;

    pthread_mutex_lock(&lock);

    for (i = 0; i < AV_HANDLE_COUNT; i++) {
        if (handles[i].used) {
            am_send_event(i, evt);
        }
    }

    pthread_mutex_unlock(&lock);
}
static AV_Handle* av_get_handle(handle_t h) {
    int id = (int) h;

    if ((id < 0) || (id >= AV_HANDLE_COUNT)) {
        return NULL;
    }

    if (!handles[id].used) {
        return NULL;
    }

    return &handles[id];
}

static TAPI_BOOL get_out_video_window_para(int * x,int * y,int * w,int * h)
{
    int left, top, right, bottom;
    char buf[32];
    TAPI_BOOL  ret = TAPI_FALSE;

    if(AM_FileRead(VID_AXIS_FILE, buf, sizeof(buf))==AM_SUCCESS){
         if(sscanf(buf, "%d %d %d %d", &left, &top, &right, &bottom)==4){
            if(x)
              *x = left;
            if(y)
              *y = top;
            if(w)
              *w = right-left;
            if(h)
              *h = bottom-top;
            ret = TAPI_TRUE;
        }
    }
  return ret;
}

static TAPI_BOOL in_small_video_window(void)
{//exsample in EPG MENU
     int xx=0,yy=0,ww=0,hh=0;
     get_out_video_window_para(&xx,&yy,&ww,&hh);
     LOGD("%s x=%d y=%d  w=%d  h=%d ",__FUNCTION__,xx,yy,ww,hh);
     if ( ww < 500 && hh < 400)
       return TAPI_TRUE;
     else
       return TAPI_FALSE;
}

static TAPI_BOOL startup_SourceSwitch_finished(void) {
#define MAX_STARTUP_COUNT 126

    static int  check_count = 0;
    int i = 0;
    char prop_value[PROPERTY_VALUE_MAX];

    if(check_count >= MAX_STARTUP_COUNT || !bDtvStartup)//wait 6000ms/50ms=125
       return TAPI_TRUE;
    check_count++;
    memset(prop_value, '\0', PROPERTY_VALUE_MAX);
    property_get("Startup.SourceSwitchInput", prop_value, "null");
    if (strcmp(prop_value, "finished") == 0) {
       LOGE("%s, prop_value = %s, cn = %d\n", __FUNCTION__, prop_value, check_count);
       check_count = MAX_STARTUP_COUNT;
       return TAPI_TRUE;
     }
    //LOGE("%s, prop_value = %s, cn = %d\n", __FUNCTION__, prop_value, check_count);
    return TAPI_FALSE;
}

TAPI_BOOL Demod_GetLockStatus(void);
status_code_t am_av_start(AV_Handle *h, AM_Bool_t startv, AM_Bool_t starta);
static void* am_av_thread_entry(void *arg) {
    int flags = 0;
    int a_start = 0, a_first = 0;
    int v_start = 0, v_first = 0;
    char buf[64];
    AM_ErrorCode_t rc;
    int need_check_clear_vbuff = 0;
    int lastEpg = 0,curEpg=0;
    int signalStatus =0;
    TAPI_BOOL bLockStatus = TAPI_FALSE;
    U32 NolockCount = 0;

    AV_Handle *h;
    AM_AOUT_OutputMode_t old_mode = AM_AOUT_OUTPUT_STEREO;
    AM_AOUT_OutputMode_t new_mode = AM_AOUT_OUTPUT_STEREO;

    while (1) {
        struct timespec ts;

        pthread_mutex_lock(&lock);
        if (flags == av_flags) {
            AM_TIME_GetTimeSpecTimeout(50, &ts);
            pthread_cond_timedwait(&cond, &lock, &ts);

            bLockStatus = Demod_GetLockStatus();
            h = av_get_handle(gAVHandle);
            if (h) {
                status_code_t ret_s;
                if (h->need_start_ts && (bLockStatus && startup_SourceSwitch_finished())) {
                    LOGE("%s start ts", __FUNCTION__);
                    h->need_start_ts = 0;
                    ret_s = am_av_start(h, AM_TRUE, AM_TRUE);
                    if (ret_s == SUCCESS) {
                        h->v_state = HDI_AV_DECODER_STATE_RUNNING;
                        h->a_state = HDI_AV_DECODER_STATE_RUNNING;
                        h->vdec_status = E_VDEC_OK;
                    } else {
                        h->vdec_status = E_VDEC_FAIL;
                    }
                }
                new_mode = h->AOUTmode;

                property_get("tv.spdif.setting", buf, "null");
                if (strcmp(buf, "true") == 0) {
                    property_set("tv.spdif.setting", "false");
                    /* h->only_update_audio = 1;
                    am_av_start(h, h->v_state == HDI_AV_DECODER_STATE_RUNNING, h->a_state == HDI_AV_DECODER_STATE_RUNNING); */
                    if(h->a_state == HDI_AV_DECODER_STATE_RUNNING)
                        AM_AV_SwitchTSAudio(AV_DEV_NO, h->av_settings.m_aud_pid, h->av_settings.m_aud_stream_type);
                    LOGD("%s, %d, tv.spdif.setting\n", __FUNCTION__, __LINE__);
                }
            }
        }
        flags = av_flags;
        pthread_mutex_unlock(&lock);

        if(bLockStatus)
            NolockCount = 0;
        else
            NolockCount ++;

        pthread_mutex_lock(&scambled_lock);
        signalStatus = scambled_status.video ||
                         scambled_status.audio ||
                         scambled_status.no_data ||
                         (NolockCount > 120);//6S/50ms=120
        pthread_mutex_unlock(&scambled_lock);

        if (flags & AV_QUIT) {
            break;
        }

        if ((flags & AV_AUD_START) && !a_start) {
            a_start = 1;
            a_first = 1;
            //am_send_all_event(HDI_AV_AUD_EVT_DECODE_START);
        } else if (!(flags & AV_AUD_START) && a_start) {
            a_start = 0;
            //am_send_all_event(HDI_AV_AUD_EVT_DECODE_STOPPED);
        }

        if ((flags & AV_VID_START) && !v_start) {
            v_start = 1;
            v_first = 1;
            need_check_clear_vbuff = 1;
            //am_send_all_event(HDI_AV_VID_EVT_DECODE_START);
        } else if (!(flags & AV_VID_START) && v_start) {
            v_start = 0;
            //am_send_all_event(HDI_AV_VID_EVT_DECODE_STOPPED);
        }

        if (a_start) {
            //AM_AV_AudioStatus_t as;
            //AM_AV_GetAudioStatus(AV_DEV_NO, &as);
            if (a_first) {
                U32 pts;

                rc = AM_FileRead("/sys/class/tsync/pts_audio", buf, sizeof(buf));
                if (rc >= 0) {
                    if (sscanf(buf, "0x%lx", &pts) == 1) {
                        if (pts != 0xffffffff) {
                            pthread_mutex_lock(&lock);
                            first_apts = pts;
                            LOGD("%s, %d, first_apts = %ld\n", __FUNCTION__, __LINE__, first_apts);
                            pthread_mutex_unlock(&lock);

                            AM_TIME_GetClock(&a_start_time);

                            a_first = 0;
                            //am_send_all_event(HDI_AV_AUD_EVT_NEW_FRAME);

                            if (v_start && !v_first) {
                                //am_send_all_event(HDI_AV_AUD_EVT_FIRST_IN_SYNC);
                            }
                        }
                    }
                }
            }
        }

        if (v_start) {
            static int frame_rate = 0;
            static int source_ratio = 1;
            int ratio;
            AM_AV_VideoStatus_t vs;

            AM_AV_GetVideoStatus(AV_DEV_NO, &vs);

            if (v_first && vs.src_w) {
                U32 pts;

                AM_TIME_GetClock(&a_start_time);

                v_first = 0;
                //am_send_all_event(HDI_AV_VID_EVT_NEW_PICTURE_DECODED);
                //am_send_all_event(HDI_AV_VID_EVT_NEW_PICTURE_TO_BE_DISPLAYED);

                if (a_start && !a_first) {
                    am_send_all_event (E_TAPI_VDEC_EVENT_DEC_ONE);
                }

                property_set("tv.boot.complete", "completed"); //close boot animation
                LOGD("am_av_thread_entry vs.src_w=ox%x", vs.src_w);

                rc = AM_FileRead("/sys/class/tsync/pts_video", buf, sizeof(buf));
                if (rc >= 0) {
                    if (sscanf(buf, "0x%lx", &pts) == 1) {
                        pthread_mutex_lock(&lock);
                        first_vpts = pts;
                        LOGD("\n%s, %d, first_vpts = %ld\n", __FUNCTION__, __LINE__, first_vpts);

                        pthread_mutex_unlock(&lock);
                    }
                }
                property_set("tvin.dtvProgramPlaying", "true");
            }

            if (frame_rate != vs.fps) {
                //am_send_all_event(HDI_AV_VID_EVT_FRAME_RATE_CHANGE);
                frame_rate = vs.fps;
            }

            if (vs.src_w * 9 >= vs.src_h * 16) {
                ratio = 1;
            } else {
                ratio = 0;
            }
            /* no return when send E_TAPI_VDEC_EVENT_DISP_INFO_CHG event
             if (source_ratio != ratio) {
             am_send_all_event (E_TAPI_VDEC_EVENT_DISP_INFO_CHG);
             source_ratio = ratio;
             }*/
        }

/*black background when EPG shows*/
        curEpg = is_epg_mode();
        if(curEpg && !lastEpg || !curEpg && lastEpg){
            AM_FileEcho("/sys/class/video/edge_blue_patch", curEpg?"0":"1");
            bDtvEPG = curEpg;
			if(signalStatus || !Demod_GetLockStatus()){
				if(curEpg){
					 test_and_write_dev("/sys/class/video/test_screen",_BLACK_COLOR_);
				}else{
					 AM_AV_EnableVideoBlackout(AV_DEV_NO);
					 AM_FileEcho("/sys/class/video/test_screen", am_get_colorForBackground(TAPI_TRUE));
				}
			}
            lastEpg = curEpg;
        }

/*no signal check*/
         if(signalStatus){
              if(need_check_clear_vbuff){
                  need_check_clear_vbuff = 0;
                  AM_AV_EnableVideoBlackout(AV_DEV_NO);
                  AM_FileEcho("/sys/class/video/test_screen", am_get_colorForBackground(TAPI_TRUE));
              }
          }else{
              need_check_clear_vbuff = 1;
          }

        if (v_start || a_start) {
            rc = AM_AOUT_GetOutputMode(AOUT_DEV_NO, &old_mode);
            if (rc >= 0 && old_mode != new_mode) {
                rc = AM_AOUT_SetOutputMode(AOUT_DEV_NO, new_mode);
                if (rc >= 0) {
                    LOGD("%s old mode:%d curr mode:%d\n", __FUNCTION__, old_mode, new_mode);
                }
            }
        }
    }

    return NULL;
}

static void am_set_disp_settings(const hdi_av_dispsettings_t *p_settings) {
    AV_Handle *h;
    int val, nval;
    AM_ErrorCode_t rc;
    AM_Bool_t mute;
    AM_AV_VideoAspectMatchMode_t match, nmatch;

    switch (p_settings->m_aspect_ratio_conv) {
    case HDI_AV_AR_CONVERSION_IGNORE:
    default:
        nmatch = AM_AV_VIDEO_ASPECT_MATCH_IGNORE;
        break;
    case HDI_AV_AR_CONVERSION_LETTER_BOX:
        nmatch = AM_AV_VIDEO_ASPECT_MATCH_LETTER_BOX;
        break;
    case HDI_AV_AR_CONVERSION_PAN_SCAN:
        nmatch = AM_AV_VIDEO_ASPECT_MATCH_PAN_SCAN;
        break;
    case HDI_AV_AR_CONVERSION_COMBINED:
        nmatch = AM_AV_VIDEO_ASPECT_MATCH_COMBINED;
        break;
    }

    LOGD("set aspect match mode %d", nmatch);
    AM_AV_SetVideoAspectMatchMode(AV_DEV_NO, nmatch);

    if (!init_ok || (fl_hide && p_settings->m_is_enable_output) || (!fl_hide && !p_settings->m_is_enable_output)) {
        if (p_settings->m_is_enable_output) {
            LOGD("vout enable");
            rc = AM_VOUT_Enable(AV_DEV_NO);
        } else {
            LOGD("vout disable");
            rc = AM_VOUT_Disable(AV_DEV_NO);
        }
        if (rc >= 0) {
            fl_hide = !p_settings->m_is_enable_output;
        }
    }

    mute = (p_settings->m_aout_mute_device & HDI_AV_AOUT_DEVICE_HDMI);
    if (!init_ok || (fl_mute && !mute) || (!fl_mute && mute)) {
        LOGD("aout mute %d", mute);
        rc = AM_AOUT_SetMute(AOUT_DEV_NO, mute);
        if (rc >= 0) {
            fl_mute = mute;
        }
    }

    rc = AM_AV_GetVideoBrightness(AV_DEV_NO, &val);
    LOGD("\n%s, %d, brightness = %d\n", __FUNCTION__, __LINE__, val);
    if (rc >= 0) {
        nval = (((int) p_settings->m_brightness) - 50) * 1024 / 50;
        if (val != nval) {
            LOGD("set brightness %d", nval);
#ifdef ENABLE_VPARA_SETTING
            AM_AV_SetVideoBrightness(AV_DEV_NO, nval);
#endif
        }
    }

    rc = AM_AV_GetVideoContrast(AV_DEV_NO, &val);
    LOGD("\n%s, %d, contrast = %d\n", __FUNCTION__, __LINE__, val);
    if (rc >= 0) {
        nval = (((int) p_settings->m_contrast) - 50) * 1024 / 50;
        if (val != nval) {
            LOGD("set contrast %d", nval);
#ifdef ENABLE_VPARA_SETTING
            AM_AV_SetVideoContrast(AV_DEV_NO, nval);
#endif
        }
    }

    rc = AM_AV_GetVideoSaturation(AV_DEV_NO, &val);
    LOGD("\n%s, %d, saturation = %d\n", __FUNCTION__, __LINE__, val);
    if (rc >= 0) {
        nval = (((int) p_settings->m_saturation) - 50) * 1024 / 50;
        if (val != nval) {
            LOGD("set saturation %d", nval);
#ifdef ENABLE_VPARA_SETTING
            AM_AV_SetVideoSaturation(AV_DEV_NO, nval);
#endif
        }
    }
}

static void am_get_disp_settings(hdi_av_dispsettings_t *p_settings) {
    AM_ErrorCode_t rc;
    int val;
    AM_AV_VideoAspectMatchMode_t match;
    hdi_av_aspect_ratio_conversion_t conv;

    rc = AM_AV_GetVideoAspectMatchMode(AV_DEV_NO, &match);
    LOGD("%s, %d, AM_AV_GetVideoAspectMatchMode: %d", __FUNCTION__, __LINE__, match);
    if (rc >= 0) {
        switch (match) {
        case AM_AV_VIDEO_ASPECT_MATCH_IGNORE:
        default:
            conv = HDI_AV_AR_CONVERSION_IGNORE;
            break;
        case AM_AV_VIDEO_ASPECT_MATCH_LETTER_BOX:
            conv = HDI_AV_AR_CONVERSION_LETTER_BOX;
            break;
        case AM_AV_VIDEO_ASPECT_MATCH_PAN_SCAN:
            conv = HDI_AV_AR_CONVERSION_PAN_SCAN;
            break;
        case AM_AV_VIDEO_ASPECT_MATCH_COMBINED:
            conv = HDI_AV_AR_CONVERSION_COMBINED;
            break;
        }

        p_settings->m_aspect_ratio_conv = conv;
    }

    p_settings->m_is_enable_output = !fl_hide;
    p_settings->m_aout_mute_device = fl_mute ? HDI_AV_AOUT_DEVICE_HDMI : HDI_AV_AOUT_DEVICE_NONE;

#if 0
    rc = AM_AV_GetVideoBrightness(AV_DEV_NO, &val);
    if(rc>=0) {
        p_settings->m_brightness = val*50/1024+50;
    }

    rc = AM_AV_GetVideoContrast(AV_DEV_NO, &val);
    if(rc>=0) {
        p_settings->m_contrast = val*50/1024+50;
    }

    rc = AM_AV_GetVideoSaturation(AV_DEV_NO, &val);
    if(rc>=0) {
        p_settings->m_saturation = val*50/1024+50;
    }
#endif
}

status_code_t hdi_av_get(const handle_t av_handle, hdi_av_settings_t * const p_settings) {
    AV_Handle *h;
    status_code_t ret = SUCCESS;
    AM_AOUT_OutputMode_t ao_mode;
    int ao_vol;
    AM_ErrorCode_t rc;

    pthread_mutex_lock(&lock);

    if (!init_ok) {
        ret = ERROR_NO_INIT;
        goto end;
    }

    if (!p_settings) {
        ret = ERROR_BAD_PARAMETER;
        goto end;
    }

    h = av_get_handle(av_handle);
    if (!h) {
        ret = ERROR_INVALID_HANDLE;
        goto end;
    }

    *p_settings = h->av_settings;

    rc = AM_AOUT_GetOutputMode(AOUT_DEV_NO, &ao_mode);
    LOGD("%s, %d, AM_AOUT_GetOutputMode: %d", __FUNCTION__, __LINE__, ao_mode);
    if (rc >= 0) {
        hdi_av_aud_channel_t chan;

        switch (ao_mode) {
        case AM_AOUT_OUTPUT_STEREO:
        default:
            chan = HDI_AV_AUD_CHANNEL_STER;
            break;
        case AM_AOUT_OUTPUT_DUAL_LEFT:
            chan = HDI_AV_AUD_CHANNEL_LEFT;
            break;
        case AM_AOUT_OUTPUT_DUAL_RIGHT:
            chan = HDI_AV_AUD_CHANNEL_RIGHT;
            break;
        }

        p_settings->m_aud_channel = chan;
    }

    p_settings->m_is_mute = fl_mute;

    rc = AM_AOUT_GetVolume(AOUT_DEV_NO, &ao_vol);
    if (rc >= 0) {
        U32 vol;

        vol = ao_vol * HDI_AV_AUDIO_VOL_MAX / AOUT_VOL_MAX;
        p_settings->m_left_vol = vol;
        p_settings->m_right_vol = vol;
    }

    end:
    LOGD("%s %d, ret : %d", __FUNCTION__, __LINE__, ret);
    pthread_mutex_unlock(&lock);

    return ret;
}

status_code_t hdi_av_set(const handle_t av_handle, hdi_av_settings_t * const p_settings) {
    AV_Handle *h = NULL;
    status_code_t ret = SUCCESS;
    AM_ErrorCode_t rc;
    AM_AOUT_OutputMode_t ao_mode;
    int ao_vol;

    pthread_mutex_lock(&lock);

    if (!p_settings) {
        ret = ERROR_BAD_PARAMETER;
        goto end;
    }

    h = av_get_handle(av_handle);
    if (!h) {
        ret = ERROR_INVALID_HANDLE;
        goto end;
    }
    rc = AM_AOUT_GetOutputMode(AOUT_DEV_NO, &ao_mode);
    LOGD("%s %d, ao_mode : %d", __FUNCTION__, __LINE__, ao_mode);
    if (rc >= 0) {
        AM_AOUT_OutputMode_t nmode;

        switch (p_settings->m_aud_channel) {
        case HDI_AV_AUD_CHANNEL_RIGHT:
            nmode = AM_AOUT_OUTPUT_DUAL_RIGHT;
            break;
        case HDI_AV_AUD_CHANNEL_LEFT:
            nmode = AM_AOUT_OUTPUT_DUAL_LEFT;
            break;
        case HDI_AV_AUD_CHANNEL_STER:
        default:
            nmode = AM_AOUT_OUTPUT_STEREO;
            break;
        }

        if (ao_mode != nmode) {
            rc = AM_AOUT_SetOutputMode(AOUT_DEV_NO, nmode);
            if (rc < 0) {
                ret = FAILED;
                goto end;
            }
        }
    }

    if (fl_mute != (int) p_settings->m_is_mute) {
        rc = AM_AOUT_SetMute(AOUT_DEV_NO, p_settings->m_is_mute);
        if (rc < 0) {
            ret = FAILED;
            goto end;
        }
        fl_mute = p_settings->m_is_mute;
    }

    rc = AM_AOUT_GetVolume(AOUT_DEV_NO, &ao_vol);
    LOGD("%s %d, ao_vol : %d", __FUNCTION__, __LINE__, ao_vol);
    if (rc >= 0) {
        int nvol;
        nvol = p_settings->m_left_vol;
        nvol = AM_MIN(AM_MAX(0, nvol), 100);
        if (ao_vol != nvol) {
            rc = AM_AOUT_SetVolume(AOUT_DEV_NO, nvol);
            if (rc < 0) {
                ret = FAILED;
                goto end;
            }
        }
    }

    end: if (h) {
        h->av_settings = *p_settings;
    }
    LOGD("%s %d, ret : %d", __FUNCTION__, __LINE__, ret);

    pthread_mutex_unlock(&lock);

    return ret;
}
status_code_t am_av_evt_config(const handle_t av_handle, const Tapi_av_evt_config_params_t * const p_cfg) {
    AV_Handle *h;
    status_code_t ret = SUCCESS;

    pthread_mutex_lock(&lock);

    if (!init_ok) {
        ret = ERROR_NO_INIT;
        goto end;
    }

    if (!p_cfg) {
        ret = ERROR_BAD_PARAMETER;
        goto end;
    }

    h = av_get_handle(av_handle);
    if (!h) {
        ret = ERROR_INVALID_HANDLE;
        goto end;
    }

    if (p_cfg->m_evt >= E_TAPI_VDEC_EVENT_MAX) {
        ret = ERROR_BAD_PARAMETER;
        goto end;
    }

    h->events[p_cfg->m_evt] = *p_cfg;

    end:
    LOGD("%s %d, ret : %d", __FUNCTION__, __LINE__, ret);
    pthread_mutex_unlock(&lock);

    return ret;
}

#if 0
void* am_injecter_thread(void *arg)
{
    Injecter *inj = (Injecter*)arg;
    AM_AV_InjectType_t type;

    switch(inj->params.m_settings.m_data_type) {
        case HDI_AV_DATA_TYPE_TS:
        type = AM_AV_INJECT_MULTIPLEX;
        break;
        case HDI_AV_DATA_TYPE_PCM:
        type = AM_AV_INJECT_AUDIO;
        break;
        case HDI_AV_DATA_TYPE_IFRAME:
        type = AM_AV_INJECT_VIDEO;
        break;
        default:
        switch(inj->params.m_settings.m_inject_content) {
            case HDI_AV_INJECT_CONTENT_AUDIO:
            type = AM_AV_INJECT_AUDIO;
            break;
            case HDI_AV_INJECT_CONTENT_VIDEO:
            default:
            type = AM_AV_INJECT_VIDEO;
            break;
        }
        break;
    }

    while(inj->running) {
        uint8_t *data;
        int size;
        AM_ErrorCode_t rc;

        pthread_mutex_lock(&inj->lock);

        size = inj->buf_used;
        while(inj->running && !size) {
            pthread_cond_wait(&inj->cond, &inj->lock);
            size = inj->buf_used;
        }

        data = inj->buf + inj->buf_begin;

        pthread_mutex_unlock(&inj->lock);

        if(size && inj->running) {
            size = AM_MIN(size, inj->buf_size - inj->buf_begin);

            rc = AM_AV_InjectData(AV_DEV_NO, type, data, &size, 50);
            if(rc>=0) {
                pthread_mutex_lock(&inj->lock);
                inj->buf_begin = (inj->buf_begin + size) % inj->buf_size;
                inj->buf_used -= size;
                pthread_mutex_unlock(&inj->lock);
                pthread_cond_broadcast(&inj->cond);
            }
        }
    }

    return NULL;
}

void am_injecter_start(Injecter *inj)
{
    int size = inj->params.m_settings.m_buf_size;
    uint8_t *buf;
    int r;

    if(inj->running)
    return;

    if(!size)
    size = INJECT_BUF_SIZE;

    buf = realloc(inj->buf, size);
    if(!buf)
    return;

    inj->buf = buf;
    inj->buf_size = size;
    inj->buf_begin = 0;
    inj->buf_used = 0;
    inj->running = 1;

    r = pthread_create(&inj->th, NULL, am_injecter_thread, inj);
    if(r) {
        WARNING("pthread_create failed\n");
        inj->running = 0;
        return;
    }
}

void am_injecter_stop(Injecter *inj)
{
    if(!inj->running)
    return;

    inj->running = 0;
    pthread_cond_broadcast(&inj->cond);
    pthread_join(inj->th, NULL);

    if(inj->buf) {
        free(inj->buf);
        inj->buf = NULL;
    }
}
#endif
#if 0
status_code_t am_av_set_3d_mode(handle_t av_handle, hdi_av_3d_mode_t mode)
{
    status_code_t ret = SUCCESS;
    AV_Handle *h;
    AM_ErrorCode_t rc;

    if(!video_start)
    return ret;

    pthread_mutex_lock(&lock);

    if(!init_ok) {
        ret = ERROR_NO_INIT;
        goto end;
    }

    h = av_get_handle(av_handle);
    if(!h) {
        ret = ERROR_INVALID_HANDLE;
        goto end;
    }

    AM_AV_EnableVideoBlackout(AV_DEV_NO);

    if (mode == HDI_3D_DISABLE) {
        //rc = AM_AV_SetVPathPara(AV_DEV_NO, AM_AV_FREE_SCALE_DISABLE, AM_AV_DEINTERLACE_ENABLE, AM_AV_PPMGR_DISABLE);
        rc = am_set_vpath(AM_AV_DEINTERLACE_ENABLE, AM_AV_PPMGR_DISABLE);
    } else {
        //rc = AM_AV_SetVPathPara(AV_DEV_NO, AM_AV_FREE_SCALE_DISABLE, AM_AV_DEINTERLACE_DISABLE, mode);
        rc = am_set_vpath(AM_AV_DEINTERLACE_DISABLE, mode);
        if(rc<0) {
            ret = FAILED;
            goto end;
        }
    }

    end:
    pthread_mutex_unlock(&lock);

    return ret;

}

#define CS_DTV_SOCKET_FILE_NAME "/dev/socket/adtv_sock"

int am_setServer(const char* fileName) {
    int ret = -1, sock = -1;
    struct sockaddr_un srv_addr;

    sock = socket(PF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        LOGE("%s, socket create failed (errno = %d: %s).\n", __FUNCTION__, errno, strerror(errno));
        return -1;
    }

    //set server addr_param
    srv_addr.sun_family = AF_UNIX;
    strncpy(srv_addr.sun_path, CS_DTV_SOCKET_FILE_NAME, sizeof(srv_addr.sun_path) - 1);
    unlink(CS_DTV_SOCKET_FILE_NAME);

    //bind sockfd & addr
    ret = bind(sock, (struct sockaddr*) &srv_addr, sizeof(srv_addr));
    if (ret == -1) {
        LOGE("%s, cannot bind server socket.\n", __FUNCTION__);
        close(sock);
        unlink(CS_DTV_SOCKET_FILE_NAME);
        return -1;
    }

    //listen sockfd
    ret = listen(sock, 1);
    if (ret == -1) {
        LOGE("%s, cannot listen the client connect request.\n", __FUNCTION__);
        close(sock);
        unlink(CS_DTV_SOCKET_FILE_NAME);
        return -1;
    }

    return sock;
}

int am_acceptMessage(int listen_fd) {
    int ret, com_fd;
    socklen_t len;
    struct sockaddr_un clt_addr;

    //have connect request use accept
    len = sizeof(clt_addr);
    com_fd = accept(listen_fd, (struct sockaddr*) &clt_addr, &len);
    if (com_fd < 0) {
        LOGE("%s, cannot accept client connect request.\n", __FUNCTION__);
        close(listen_fd);
        unlink(CS_DTV_SOCKET_FILE_NAME);
        return -1;
    }

    LOGD("%s, com_fd = %d\n", __FUNCTION__, com_fd);

    return com_fd;
}

int am_parse_socket_message(char *msg_str, int *para_cnt, int para_buf[]) {
    int para_count = 0, set_mode = 0;
    char *token = NULL;

    set_mode = -1;

    token = strtok(msg_str, ",");
    if (token != NULL) {
        if (strcasecmp(token, "quit") == 0) {
            set_mode = 0;
        } else if (strcasecmp(token, "set3dmode") == 0) {
            set_mode = 1;
        } else if (strcasecmp(token, "setdisplaymode") == 0) {
            set_mode = 2;
        }
    }

    if (set_mode != 1 && set_mode != 2) {
        return set_mode;
    }

    para_count = 0;

    token = strtok(NULL, ",");
    while (token != NULL) {
        para_buf[para_count] = strtol(token, NULL, 10);
        para_count += 1;

        token = strtok(NULL, ",");
    }

    *para_cnt = para_count;

    return set_mode;
}

void* am_av_socket_thread_entry(void *arg) {
    int ret = 0, listen_fd = -1, com_fd = -1, rd_len = 0;
    int para_count = 0, set_mode = 0;
    int para_buf[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,};
    static char recv_buf[1024];

    listen_fd = am_setServer(CS_DTV_SOCKET_FILE_NAME);
    if (listen_fd < 0) {
        return NULL;
    }

    while (1) {
        com_fd = am_acceptMessage(listen_fd);

        if (com_fd >= 0) {
            //read message from client
            memset((void *) recv_buf, 0, sizeof(recv_buf));
            rd_len = read(com_fd, recv_buf, sizeof(recv_buf));
            LOGD("%s, message from client (%d)) : %s\n", __FUNCTION__, rd_len, recv_buf);

            set_mode = am_parse_socket_message(recv_buf, &para_count, para_buf);
            if (set_mode == 0) {
                LOGD("%s, receive quit message, starting to quit.\n", __FUNCTION__);
                sprintf(recv_buf, "%s", "quiting now...");
                write(com_fd, recv_buf, strlen(recv_buf) + 1);
                break;
            } else if (set_mode == 1) {
                ret = -1;

                if (para_count == 1) {
                    LOGD("%s, handle = %d, mode = %d\n", __FUNCTION__, gAVHandle, para_buf[0]);

                    ret = am_av_set_3d_mode(gAVHandle, (hdi_av_3d_mode_t)para_buf[0]);
                    if (ret == SUCCESS) {
                        LOGD("%s, hdi_av_set_3d_mode return sucess.\n", __FUNCTION__);
                    } else {
                        LOGD("%s, hdi_av_set_3d_mode return error(%d).\n", __FUNCTION__, ret);
                    }
                }

                sprintf(recv_buf, "%d", ret);
                write(com_fd, recv_buf, strlen(recv_buf) + 1);
            } else if (set_mode == 2) {
                ret = -1;

                if (para_count == 1) {
                    hdi_av_dispsettings_t tmpSet;
                    hdi_av_capability_t tmpcapability;

                    LOGD("%s, handle = %d, aspect_ratio = %d.\n", __FUNCTION__, gAVHandle, para_buf[0]);

                    ret = hdi_av_get_capability(&tmpcapability);
                    if (ret == SUCCESS) {
                        ret = hdi_av_display_get(gAVHandle, tmpcapability.m_channel, &tmpSet);
                        if (ret == SUCCESS) {
                            tmpSet.m_aspect_ratio = para_buf[0];

                            LOGD("%s, channel = %d\n", __FUNCTION__, tmpcapability.m_channel);

                            ret = hdi_av_display_set(gAVHandle, tmpcapability.m_channel, &tmpSet);
                            if (ret == SUCCESS) {
                                LOGD("%s, hdi_av_display_set return sucess.\n", __FUNCTION__);
                            } else {
                                LOGE("%s, hdi_av_display_set return error(%d).\n", __FUNCTION__, ret);
                            }
                        } else {
                            LOGE("%s, hdi_av_display_get return error(%d).\n", __FUNCTION__, ret);
                        }
                    } else {
                        LOGE("%s, hdi_av_get_capability return error(%d).\n", __FUNCTION__, ret);
                    }
                }

                sprintf(recv_buf, "%d", ret);
                write(com_fd, recv_buf, strlen(recv_buf) + 1);
            }

            close(com_fd);
            com_fd = -1;
        }
    }

    if (com_fd >= 0) {
        close(com_fd);
        com_fd = -1;
    }

    if (listen_fd >= 0) {
        close(listen_fd);
        listen_fd = -1;
    }

    unlink(CS_DTV_SOCKET_FILE_NAME);

    return NULL;
}

#if !defined(SUN_LEN)
#define SUN_LEN(su) (sizeof(*(su)) - sizeof((su)->sun_path) + strlen((su)->sun_path))
#endif 

int am_connectToServer(char *file_name) {
    int tmp_ret = 0, sock = -1;
    struct sockaddr_un addr;

    if (file_name == NULL) {
        LOGE("%s, file name is NULL\n", __FUNCTION__);
        return -1;
    }

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        LOGE("%s, socket create failed (errno = %d: %s)\n", __FUNCTION__, errno, strerror(errno));
        return -1;
    }

    /* connect to socket; fails if file doesn't exist */
    strcpy(addr.sun_path, file_name); // max 108 bytes
    addr.sun_family = AF_UNIX;
    tmp_ret = connect(sock, (struct sockaddr*) &addr, SUN_LEN(&addr));
    if (tmp_ret < 0) {
        // ENOENT means socket file doesn't exist
        // ECONNREFUSED means socket exists but nobody is listening
        LOGE("%s, AF_UNIX connect failed for '%s': %s\n", __FUNCTION__, file_name, strerror(errno));
        close(sock);
        return -1;
    }

    return sock;
}

int am_realSendSocketMsg(char *file_name, char *msg_str, char recv_buf[]) {
    int sock = -1, rd_len = 0;
    char tmp_buf[1024];

    if (file_name == NULL) {
        LOGE("%s, file name is NULL\n", __FUNCTION__);
        return -1;
    }

    if (msg_str == NULL) {
        LOGE("%s, msg string is NULL\n", __FUNCTION__);
        return -1;
    }

    LOGD("%s, message to server (%d)) : %s\n", __FUNCTION__, strlen(msg_str), msg_str);

    sock = am_connectToServer(file_name);

    if (sock >= 0) {
        write(sock, msg_str, strlen(msg_str) + 1);

        if (recv_buf == NULL) {
            memset((void *) tmp_buf, 0, sizeof(tmp_buf));
            rd_len = read(sock, tmp_buf, sizeof(tmp_buf));
            LOGD("%s, message from server (%d)) : %s\n", __FUNCTION__, rd_len, tmp_buf);
        } else {
            rd_len = read(sock, recv_buf, 1024);
            LOGD("%s, message from server (%d)) : %s\n", __FUNCTION__, rd_len, recv_buf);
        }

        close(sock);
        sock = -1;

        return 0;
    }

    return -1;
}
#endif
status_code_t am_av_open(handle_t * const p_av_handle, const hdi_av_decoder_id_t decoder_id, const hdi_av_open_params_t * const p_open_params) {
    AV_Handle *h;
    int id;
    status_code_t ret = SUCCESS;
    AM_ErrorCode_t rc;
    int x, y, width, height;

    pthread_mutex_lock(&lock);

    if (!init_ok) {
        ret = ERROR_NO_INIT;
        goto end;
    }

    if (!p_av_handle || !p_open_params) {
        ret = ERROR_BAD_PARAMETER;
        goto end;
    }

    if (HDI_AV_DECODER_MAIN != decoder_id) {
        ret = ERROR_FEATURE_NOT_SUPPORTED;
        goto end;
    }

    for (id = 1; id < AV_HANDLE_COUNT; id++) {
        if (!handles[id].used)
            break;
    }

    if (id >= AV_HANDLE_COUNT) {
        ret = ERROR_NO_FREE_HANDLES;
        goto end;
    }

    h = &handles[id];

    memset(h, 0, sizeof(AV_Handle));

    h->used = 1;
    h->a_state = HDI_AV_DECODER_STATE_STOPPED;
    h->v_state = HDI_AV_DECODER_STATE_STOPPED;

    h->av_settings.m_vid_stop_mode = HDI_AV_VID_STOP_MODE_BLACK; //black out

    h->in_left = h->in_top = 0;
    h->in_width = 1920;
    h->in_height = 1080;

    rc = AM_AV_GetVideoWindow(AV_DEV_NO, &x, &y, &width, &height);
    LOGD("%s %d, x,y,w,h: %d, %d, %d, %d", __FUNCTION__, __LINE__, x, y, width, height);
    if (rc < 0) {
        ret = FAILED;
        goto end;
    }

    h->out_left = x;
    h->out_top = y;
    h->out_width = width;
    h->out_height = height;

    h->def_disp_settings = init_params.m_default_dispsettings[0];
    h->disp_settings = init_params.m_disp_settings[0];

    memcpy(&h->params, p_open_params, sizeof(hdi_av_open_params_t));

    rc = AM_AV_SetTSSource(AV_DEV_NO, am_get_ts_source_by_dmx_id(p_open_params->m_source_params.m_demux_id));
    if (rc) {
        ret = FAILED;
        goto end;
    }

    *p_av_handle = (handle_t) id;
    gAVHandle = (handle_t) id;

    end:
    LOGD("%s %d, ret : %d", __FUNCTION__, __LINE__, ret);
    pthread_mutex_unlock(&lock);

    return ret;
}

TAPI_BOOL AM_DMX_GetScrambleStatus(int dev_no, AM_Bool_t dev_status[2]) {
    char buf[32];
    char class_file[64];
    TAPI_BOOL ret = TAPI_FALSE;
    int vflag, aflag;
    pthread_mutex_lock(&lock);

    snprintf(class_file, sizeof(class_file), "/sys/class/dmx/demux%d_scramble", dev_no);
    if (AM_FileRead(class_file, buf, sizeof(buf)) == AM_SUCCESS) {
        sscanf(buf, "%d %d", &vflag, &aflag);
        dev_status[0] = vflag ? AM_TRUE : AM_FALSE;
        dev_status[1] = aflag ? AM_TRUE : AM_FALSE;
        //LOGD("AM_DMX_GetScrambleStatus video scamble %d, audio scamble %d\n", vflag, aflag);
        ret = TAPI_TRUE;
    }

    //LOGD("%s,AM_DMX_GetScrambleStatus read scamble status failed\n", __FUNCTION__);
    pthread_mutex_unlock(&lock);

    return ret;
}

TAPI_BOOL am_av_enable_video(const handle_t av_handle, const hdi_av_channel_t channel) {
    AV_Handle *h;
    status_code_t ret = SUCCESS;
    TAPI_BOOL rst = TAPI_TRUE;
    AM_ErrorCode_t rc;

    pthread_mutex_lock(&lock);

    if (!init_ok) {
        ret = ERROR_NO_INIT;
        goto end;
    }

    if (!(channel & HDI_AV_CHANNEL_HD)) {
        ret = ERROR_FEATURE_NOT_SUPPORTED;
        goto end;
    }

    h = av_get_handle(av_handle);
    if (!h) {
        ret = ERROR_INVALID_HANDLE;
        goto end;
    }

    /*rc = AM_AV_EnableVideo(AV_DEV_NO);
     if(rc<0){
     ret = SK_FAILED;
     goto end;
     }*/

    fl_enablev = 1;

    end: if (ret != SUCCESS) {
        printf("%s,ret = %d\n", __FUNCTION__, ret);
        rst = TAPI_FALSE;
    }
    pthread_mutex_unlock(&lock);

    return rst;
}
TAPI_BOOL hdi_av_disable_video(const handle_t av_handle, const hdi_av_channel_t channel) {
    AV_Handle *h;
    status_code_t ret = SUCCESS;
    TAPI_BOOL rst = TAPI_TRUE;
    AM_ErrorCode_t rc;

    pthread_mutex_lock(&lock);

    if (!init_ok) {
        ret = ERROR_NO_INIT;
        goto end;
    }

    if (!(channel & HDI_AV_CHANNEL_HD)) {
        ret = ERROR_FEATURE_NOT_SUPPORTED;
        goto end;
    }

    h = av_get_handle(av_handle);
    if (!h) {
        ret = ERROR_INVALID_HANDLE;
        goto end;
    }

    /*rc = AM_AV_DisableVideo(AV_DEV_NO);
     if(rc<0){
     ret = SK_FAILED;
     goto end;
     }*/

    fl_enablev = 0;

    end: if (ret != SUCCESS) {
        rst = TAPI_FALSE;
    }
    pthread_mutex_unlock(&lock);

    return ret;
}

status_code_t am_av_start(AV_Handle *h, AM_Bool_t startv, AM_Bool_t starta) {
    AM_AV_AFormat_t afmt;
    AM_AV_VFormat_t vfmt;
    int apid, vpid;
    AM_ErrorCode_t rc;
    status_code_t ret;
    int apkg, vpkg;
    //handle_t av_handle = h - handles;

#define PKG_NONE  0
#define PKG_TUNER 1
#define PKG_TS    2
#define PKG_ES    3
#define PKG_PS    4

    if (fl_decode_vid_es && startv)
        return ERROR_DEVICE_BUSY;

    aud_pid = h->av_settings.m_aud_pid;
    apid = aud_pid;
    a_source = HDI_AV_SOURCE_TUNER;
    apkg = PKG_TUNER;

    vid_pid = h->av_settings.m_vid_pid;
    vpid = vid_pid;
    v_source = HDI_AV_SOURCE_TUNER;
    vpkg = PKG_TUNER;

    //afmt = h->av_settings.m_atype;
    //vfmt = h->av_settings.m_vtype;
    afmt = h->av_settings.m_aud_stream_type;
    vfmt = h->av_settings.m_vid_stream_type;

    if (!starta) {
        apkg = PKG_NONE;
        apid = -1;
    }

    if (!startv) {
        vpkg = PKG_NONE;
        vpid = -1;
    }

    if (!apid)
        apid = -1;
    if (!vpid)
        vpid = -1;

    if (apkg && vpkg && (apkg != vpkg)) {
        LOGD("%s ,%d, %d %d %d %d\n", __FUNCTION__, __LINE__, vpid, apid, vfmt, afmt);
        return ERROR_FEATURE_NOT_SUPPORTED;
    }

    if ((apkg == PKG_NONE) && (vpkg == PKG_NONE)) {
        if (fl_tuner_mode) {
            if (h->av_settings.m_vid_stop_mode == HDI_AV_VID_STOP_MODE_FREEZE && !bDtvEPG) {
                AM_AV_DisableVideoBlackout(AV_DEV_NO);
            } else {
                AM_AV_EnableVideoBlackout(AV_DEV_NO);
            }

            rc = AM_AV_StopTS(AV_DEV_NO);

            if (rc < 0)
                return FAILED;
            LOGD("stop ts");
        }
        video_start = 0;
    } else if ((apkg == PKG_TUNER) || (vpkg == PKG_TUNER)) {
         rc = AM_AV_SetTSSource(AV_DEV_NO, am_get_ts_source_by_dmx_id(h->params.m_source_params.m_demux_id));
         if (rc) {
               return FAILED;
         }
          if (vpid > 0 && vpid < 0x1fff) {
          int di = AM_AV_DEINTERLACE_ENABLE;
          if (ppmgr_mode > AM_AV_PPMGR_ENABLE)
          di = AM_AV_DEINTERLACE_DISABLE;
          // am_set_vpath(di, ppmgr_mode);
          video_start = 1;
       }

        if (vpid == -1){ //for radio channel
                 AM_AV_EnableVideoBlackout(AV_DEV_NO);
                 AM_FileEcho("/sys/class/video/test_screen", am_get_colorForBackground(TAPI_TRUE));
        }
        LOGD("start ts b %d %d %d %d\n", vpid, apid, vfmt, afmt);
        rc = AM_AV_StartTS(AV_DEV_NO, vpid, apid, vfmt, afmt);
        fl_tuner_mode = 1;
        LOGD("start ts a %d %d %d %d\n", vpid, apid, vfmt, afmt);

        if (rc < 0)
            return FAILED;
    }

    if ((h->av_settings.m_av_sync_mode == HDI_AV_SYNC_MODE_DISABLE) || (apid == -1) || (vpid == -1)) {
        AM_FileEcho("/sys/class/tsync/enable", "0");
    } else {
        AM_FileEcho("/sys/class/tsync/enable", "1");
    }

    if (starta) {
        av_flags |= AV_AUD_START;
    } else {
        av_flags &= ~AV_AUD_START;
    }

    if (startv) {
        av_flags |= AV_VID_START;
        AM_TIME_GetClock(&a_start_time);
    } else {
        av_flags &= ~AV_VID_START;
    }

    pthread_cond_broadcast(&cond);

    return SUCCESS;
}
void audio_SetAbsoluteVolume(TAPI_U8 uPercent, AudioPath_ uAudioPath) {

    hdi_av_settings_t set;
    status_code_t ret;
    //char buf[256];

    pthread_mutex_lock(&lock);
    if (uPercent > 100) {
        LOGE("%s set volume beyone 0~100 !", __FUNCTION__);
        goto end;
    }
    if (uAudioPath < 0x0 || uAudioPath > 0x04) {
        LOGE("%s set Audio error !", __FUNCTION__);
        goto end;
    }
    //sscanf(buf+3, "%d", &uPercent);
    ret = hdi_av_get(gAVHandle, &set);
    if (ret != SUCCESS) {
        fprintf(stderr, "hdi_av_get: %d\n", ret);
    }
    set.m_left_vol = uPercent;
    set.m_right_vol = uPercent;
    ret = hdi_av_set(gAVHandle, &set);
    if (ret != SUCCESS) {
        fprintf(stderr, "hdi_av_set: %d\n", ret);
    }

    end: pthread_mutex_unlock(&lock);

}

status_code_t Audio_Stop() {
    AV_Handle *h;
    status_code_t ret = SUCCESS;
    AM_ErrorCode_t rc;

    pthread_mutex_lock(&lock);

    h = av_get_handle(gAVHandle);
    if (!h) {
        ret = ERROR_INVALID_HANDLE;
        goto end;
    }

    ret = am_av_start(h, (h->v_state == HDI_AV_DECODER_STATE_RUNNING), AM_FALSE);
    if (ret == SUCCESS) {
        h->a_state = HDI_AV_DECODER_STATE_STOPPED;
    }

    end:
    LOGD("%s %d, ret : %d", __FUNCTION__, __LINE__, ret);
    pthread_mutex_unlock(&lock);

    return ret;
}

status_code_t Audio_Play() {
    AV_Handle *h;
    status_code_t ret = SUCCESS;
    AM_ErrorCode_t rc;

    pthread_mutex_lock(&lock);

    h = av_get_handle(gAVHandle);
    if (!h) {
        ret = ERROR_INVALID_HANDLE;
        goto end;
    }

    ret = am_av_start(h, (h->v_state == HDI_AV_DECODER_STATE_RUNNING), AM_TRUE);
    if (ret == SUCCESS) {
        h->a_state = HDI_AV_DECODER_STATE_RUNNING;
    }
    end:
    LOGD("%s %d, ret : %d", __FUNCTION__, __LINE__, ret);
    pthread_mutex_unlock(&lock);

    return ret;
}

status_code_t Audio_SetOutPutMode(En_DVB_soundModeType_ const aud_channel) {
    status_code_t ret = SUCCESS;
    AM_ErrorCode_t rc;
    AM_AOUT_OutputMode_t ao_mode;
    AM_AOUT_OutputMode_t nmode;
    AV_Handle *h;

    pthread_mutex_lock(&lock);

    //rc = AM_AOUT_GetOutputMode(AOUT_DEV_NO, &ao_mode);
    //LOGD("%s curr mode:%d\n", __FUNCTION__,ao_mode);
    //if (rc >= 0) {

    switch (aud_channel) {
    case TAPI_AUD_DVB_SOUNDMODE_RIGHT_:
        nmode = AM_AOUT_OUTPUT_DUAL_RIGHT;
        break;
    case TAPI_AUD_DVB_SOUNDMODE_LEFT_:
        nmode = AM_AOUT_OUTPUT_DUAL_LEFT;
        break;
    case TAPI_AUD_DVB_SOUNDMODE_MIXED_:
        nmode = AM_AOUT_OUTPUT_SWAP;
        break;
    case TAPI_AUD_DVB_SOUNDMODE_STEREO_:
    default:
        nmode = AM_AOUT_OUTPUT_STEREO;
        break;
    }
    /*
     if (ao_mode != nmode) {
     rc = AM_AOUT_SetOutputMode(AOUT_DEV_NO, nmode);
     if (rc < 0) {
     ret = FAILED;
     goto end;
     }
     }
     }
     */
    h = av_get_handle(gAVHandle);
    if (!h) {
        LOGE("%s failed\n", __FUNCTION__);
        ret = FAILED;
        goto end;
    }

    h->AOUTmode = nmode;
    LOGD("%s  h->AOUTmode=%d\n", __FUNCTION__, h->AOUTmode);
    end: pthread_mutex_unlock(&lock);

    return ret;
}

static void v_scambled_callback(int dev_no, int event_type, void *param, void *user_data) {
    pthread_mutex_lock(&scambled_lock);
    scambled_status.video = 1;
    pthread_mutex_unlock(&scambled_lock);
    LOGD("%s\n", __FUNCTION__);
}

static void a_scambled_callback(int dev_no, int event_type, void *param, void *user_data) {
    pthread_mutex_lock(&scambled_lock);
    scambled_status.audio = 1;
    pthread_mutex_unlock(&scambled_lock);
    LOGD("%s\n", __FUNCTION__);
}

static void d_scambled_callback(int dev_no, int event_type, void *param, void *user_data) {
    pthread_mutex_lock(&scambled_lock);
    scambled_status.no_data = 1;
    pthread_mutex_unlock(&scambled_lock);
    LOGD("%s\n", __FUNCTION__);
}

static void d_unscambled_callback(int dev_no, int event_type, void *param, void *user_data) {
    pthread_mutex_lock(&scambled_lock);
    scambled_status.video = 0;
    scambled_status.audio = 0;
    scambled_status.no_data = 0;
    pthread_mutex_unlock(&scambled_lock);
    LOGD("%s\n", __FUNCTION__);
}

extern int am_av_restart_pts_repeat_count;
TAPI_BOOL Tapi_Vdec_Init(void) {
    hdi_av_init_params_t av_para;
    AM_AV_OpenPara_t para;
    AM_VOUT_OpenPara_t vo_para;
    AM_AOUT_OpenPara_t ao_para;
    status_code_t ret = SUCCESS;
    TAPI_BOOL rst = TAPI_TRUE;
    AM_ErrorCode_t rc;
    hdi_av_open_params_t av_open_params;
    int x, y, width, height;
    char buf[64];
    int i;

    LOGD("%s %d\n", __FUNCTION__, __LINE__);
    print_version_info();

    for(i = 0; i < 40; i++)
    {
      property_get("usb.dongle.dvbc.used", buf, "null");
      if (strcmp(buf, "true") == 0)
      {
        usleep(100*1000);
      }
      else
      {
        LOGD("%s,wait%dms", __FUNCTION__,i*100);
        break;
      }
    }

 		property_set("tv.vdec.used", "true");
    pthread_mutex_lock(&lock);
    memset(&av_para, 0, sizeof(av_para));
    av_para.m_disp_settings[0].m_is_enable_output = 1;
    av_para.m_disp_settings[0].m_aout_mute_device = HDI_AV_AOUT_DEVICE_RCA;
    av_para.m_disp_settings[0].m_brightness = 50;
    av_para.m_disp_settings[0].m_contrast = 50;
    av_para.m_disp_settings[0].m_saturation = 50;

    if (init_ok) {
        ret = ERROR_ALREADY_INITIALIZED;
        goto end;
    }

    memset(&vo_para, 0, sizeof(vo_para));
    rc = AM_VOUT_Open(VOUT_DEV_NO, &vo_para);
    if (rc != AM_SUCCESS) {
        LOGD("%s %d, ret : %d\n", __FUNCTION__, __LINE__, rc);
        ret = FAILED;
        goto end;
    }

    memset(&ao_para, 0, sizeof(ao_para));
    rc = AM_AOUT_Open(AOUT_DEV_NO, &ao_para);
    if (rc != AM_SUCCESS) {
        AM_VOUT_Close(VOUT_DEV_NO);
        LOGD("%s %d, ret : %d\n", __FUNCTION__, __LINE__, rc);
        ret = FAILED;
        goto end;
    }
    //DEBUG("AM_AV_Close before");
    //AM_AV_Close(AV_DEV_NO);
    //DEBUG("AM_AV_Close after");

    para.vout_dev_no = VOUT_DEV_NO;
    rc = AM_AV_Open(AV_DEV_NO, &para);
    if (rc != AM_SUCCESS) {
        AM_AOUT_Close(AOUT_DEV_NO);
        AM_VOUT_Close(VOUT_DEV_NO);
        LOGD("%s %d, ret : %d\n", __FUNCTION__, __LINE__, rc);
        ret = FAILED;
        goto end;
    }
    AM_AV_EnableVideo(AV_DEV_NO);
    AM_AV_EnableVideoBlackout(AV_DEV_NO);
    memcpy(&init_params, &av_para, sizeof(hdi_av_init_params_t));
    am_set_disp_settings(&init_params.m_disp_settings[0]);

    if (init_params.m_is_enable_disp) {
        AM_VOUT_Enable(VOUT_DEV_NO);
    } else {
        AM_VOUT_Disable(VOUT_DEV_NO);
    }

    deinterlace_mode = -1;
    ppmgr_mode = -1;
    /*
     rc = AM_AV_SetVPathPara(AV_DEV_NO, AM_AV_FREE_SCALE_DISABLE, AM_AV_DEINTERLACE_ENABLE, AM_AV_PPMGR_DISABLE);
     if (rc < 0) {
     AM_AOUT_Close(AOUT_DEV_NO);
     AM_VOUT_Close(VOUT_DEV_NO);
     ret = FAILED;
     goto end;
     }
     */
    av_flags = 0;
    rc = pthread_create(&av_thread, NULL, am_av_thread_entry, NULL);
    if (rc) {
        AM_AOUT_Close(AOUT_DEV_NO);
        AM_VOUT_Close(VOUT_DEV_NO);
        ret = FAILED;
        goto end;
    }

    init_ok = 1;

    memset(&av_open_params, 0x00, sizeof(av_open_params));
    av_open_params.m_source_params.m_demux_id = 0;
    rc = am_av_open(&gAVHandle, HDI_AV_DECODER_MAIN, &av_open_params);
    if (rc != AM_SUCCESS) {
        LOGD("%s %d, ret : %d\n", __FUNCTION__, __LINE__, rc);
        ret = FAILED;
        goto end;
    }

    rc = AM_AV_GetVideoWindow(AV_DEV_NO, &x, &y, &width, &height);
    if (rc < 0) {
        ret = FAILED;
        goto end;
    }
    AM_FileEcho("/sys/module/amvdec_h264/parameters/dec_control", "3");
    AM_FileEcho("/sys/module/amvdec_mpeg12/parameters/dec_control", "62"); 
    AM_FileEcho("/sys/module/amvideo/parameters/smooth_sync_enable", "1"); //to enable video more smooth while reset PTS
    AM_FileEcho("/sys/module/amvdec_h264/parameters/fatal_error_reset", "0"); //avoid "fatal error happend" when playing h264 ts
    AM_FileEcho("/sys/class/video/edge_blue_patch", "1"); //need set 1 to avoid blue edge for 4b3 mode,if using blue screen for changing channels
    AM_FileEcho("/sys/module/di/parameters/bypass_dynamic", "0"); //need set 0 to avoid that subtile flutter
    AM_FileEcho("/sys/class/amaudio/enable_adjust_pts", "0"); //need set 0 for the subtitle smoothly  moving
    AM_FileEcho("/sys/class/si2176/si2176_autoLoadFirmware", "1"); //save time for changing source
    AM_FileEcho("/sys/module/amvdec_h264/parameters/toggle_drop_B_frame_on", "0"); //switch off drop B frame to avoid the video flickers when unplug and plug the cable for the HD signal
    test_and_write_dev("/sys/class/video/test_screen",_BLUE_COLOR_); //avoid black screen
    property_set("tvin.dtvProgramPlaying","false");
    end:
    LOGD("%s %d, ret : %d gAVHandle =%lu\n", __FUNCTION__, __LINE__, ret, gAVHandle);
    if (ret != SUCCESS) {
        rst = TAPI_FALSE;
    }

    AM_EVT_Subscribe(0, AM_AV_EVT_VIDEO_SCAMBLED, v_scambled_callback, NULL);
    AM_EVT_Subscribe(0, AM_AV_EVT_AUDIO_SCAMBLED, a_scambled_callback, NULL);
    AM_EVT_Subscribe(0, AM_AV_EVT_AV_NO_DATA, d_scambled_callback, NULL);
    AM_EVT_Subscribe(0, AM_AV_EVT_AV_DATA_RESUME, d_unscambled_callback, NULL);

    am_av_restart_pts_repeat_count = 5;//avoid blue screen when inserting ci card
    pthread_mutex_unlock(&lock);

    pthread_mutex_lock(&scambled_lock);
    scambled_status.video = 0;
    scambled_status.audio = 0;
    scambled_status.no_data = 0;
    pthread_mutex_unlock(&scambled_lock);

    return rst;
}

//-------------------------------------------------------------------------------------------------
/// To finalize the vdec.
/// @param  None
/// @return                \b TRUE: success, FALSE: failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Vdec_Finalize(void) {
    TAPI_BOOL ret = TAPI_TRUE;
    int id = 0;

    AM_FileEcho("/sys/class/amaudio/enable_adjust_pts", "1"); //restore
    AM_FileEcho("/sys/module/amvdec_h264/parameters/fatal_error_reset", "1"); //restore
    AM_FileEcho("/sys/class/video/edge_blue_patch", "0"); //restore
    test_and_write_dev("/sys/class/video/test_screen",_BLUE_COLOR_); //avoid the black screen flicker and must before AM_AV_DisableVideo
    AM_FileEcho("/sys/module/di/parameters/bypass_dynamic", "2"); //restore
    usleep(1000);

    av_flags |= AV_QUIT;
    pthread_cond_broadcast(&cond);
    if (av_thread != -1) {
        pthread_join(av_thread, NULL);
        av_thread = -1;
    }

    pthread_mutex_lock(&lock);

    AM_EVT_Unsubscribe(0, AM_AV_EVT_VIDEO_SCAMBLED, v_scambled_callback, NULL);
    AM_EVT_Unsubscribe(0, AM_AV_EVT_AUDIO_SCAMBLED, a_scambled_callback, NULL);
    AM_EVT_Unsubscribe(0, AM_AV_EVT_AV_NO_DATA, d_scambled_callback, NULL);
    AM_EVT_Unsubscribe(0, AM_AV_EVT_AV_DATA_RESUME, d_unscambled_callback, NULL);

    AM_AOUT_SetOutputMode(AOUT_DEV_NO, AM_AOUT_OUTPUT_STEREO); //restore

    AM_AV_DisableVideo(AV_DEV_NO);
    AM_AOUT_Close(AOUT_DEV_NO);
    AM_VOUT_Close(VOUT_DEV_NO);
    AM_AV_Close(AV_DEV_NO);

    for (id = 0; id < AV_HANDLE_COUNT; id++)
        handles[id].used = 0;

    gAVHandle = -1;
    init_ok = 0;
    LOGD("%s %d, ret : %d", __FUNCTION__, __LINE__, ret);
    pthread_mutex_unlock(&lock);
    property_set("tv.vdec.used", "false");
    return ret;
}

/*
 * @brief, start av decoder
 * using the parameters set before
 */
TAPI_BOOL Tapi_AV_start(void) {
    AV_Handle *h;
    TAPI_BOOL ret = TAPI_TRUE;
    status_code_t ret_s;

    pthread_mutex_lock(&lock);
    LOGD("%s %d be called.", __FUNCTION__, __LINE__);

    if (!init_ok) {
        ret = TAPI_FALSE;
        goto end;
    }
    h = av_get_handle(gAVHandle);
    if (!h) {
        ret = TAPI_FALSE;
        goto end;
    }

    if (h->already_started == 1) {
        AM_ErrorCode_t rc;

        if (h->only_update_audio) {
            /* ret_s = am_av_start(h, h->v_state == HDI_AV_DECODER_STATE_RUNNING, h->a_state == HDI_AV_DECODER_STATE_RUNNING);*/
            rc = AM_AV_SwitchTSAudio(AV_DEV_NO, h->av_settings.m_aud_pid, h->av_settings.m_aud_stream_type);
            if (rc == AM_SUCCESS) {
                LOGE("%s have update audio %d %d\n", __FUNCTION__,h->av_settings.m_aud_pid,h->av_settings.m_aud_stream_type);
                ret = TAPI_TRUE;
            }
            else{
                ret = TAPI_FALSE;
            }
        }
        goto end;
    }
    h->already_started = 1;
    h->only_update_audio = 0;
    if(!Demod_GetLockStatus() || !startup_SourceSwitch_finished()) {
        LOGE("%s demod not lock!!!!!!", __FUNCTION__);
        h->need_start_ts = 1;
        ret = TAPI_FALSE;
        goto end;
    }
    ret_s = am_av_start(h, AM_TRUE, AM_TRUE);
    if (ret_s == SUCCESS) {
        h->v_state = HDI_AV_DECODER_STATE_RUNNING;
        h->a_state = HDI_AV_DECODER_STATE_RUNNING;
        h->vdec_status = E_VDEC_OK;
    } else {
        h->vdec_status = E_VDEC_FAIL;
    }
    end: pthread_mutex_unlock(&lock);

    return ret;
}

/*
 * @brief, stop av decoder
 * using the parameters set before
 */
TAPI_BOOL Tapi_AV_stop(void) {
    AV_Handle *h;
    TAPI_BOOL ret = TAPI_TRUE;
    status_code_t ret_s;

    pthread_mutex_lock(&scambled_lock);
    scambled_status.video = 0;
    scambled_status.audio = 0;
    scambled_status.no_data = 0;
    pthread_mutex_unlock(&scambled_lock);

    pthread_mutex_lock(&lock);
    LOGD("%s %d be called.", __FUNCTION__, __LINE__);

    if (!init_ok) {
        ret = TAPI_FALSE;
        goto end;
    }

    h = av_get_handle(gAVHandle);
    if (!h) {
        ret = TAPI_FALSE;
        goto end;
    }

    if (h->already_started == 0) {
        ret = TAPI_FALSE;
        goto end;
    }
    h->already_started = 0;
    h->need_start_ts = 0;
    ret_s = am_av_start(h, AM_FALSE, AM_FALSE);
    if (ret_s == SUCCESS) {
        h->v_state = HDI_AV_DECODER_STATE_STOPPED;
        h->a_state = HDI_AV_DECODER_STATE_STOPPED;
        h->vdec_status = E_VDEC_OK;
    } else {
        h->vdec_status = E_VDEC_FAIL;
    }
//patch for radio channel
    h->av_settings.m_aud_pid = 0;
    h->av_settings.m_vid_pid = 0;

    h->only_update_audio = 0;
    property_set("tvin.dtvProgramPlaying", "false");
end:
   pthread_mutex_unlock(&lock);

    return ret;
}

//-------------------------------------------------------------------------------------------------
/// To get Decoder status.
/// @param                \b out: Vdec STATUS.
/// @return               \b TRUE: success, FALSE: failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Vdec_GetDecoderStatus(VDEC_STATUS_INFO_T * sta) {
    AV_Handle *h;
    TAPI_BOOL rst = TAPI_TRUE;
    VDEC_STATUS vstatus = E_VDEC_OK;
    AM_AV_VideoStatus_t vs;
    int now;

    if (!sta)
        return TAPI_FALSE;

    memset(sta, 0, sizeof(VDEC_STATUS_INFO_T));
    pthread_mutex_lock(&lock);
    if (!init_ok) {
        sta->u4Status = E_VDEC_NOT_INIT;
        goto end;
    }

    h = av_get_handle(gAVHandle);
    if (!h) {
        rst = TAPI_FALSE;
        goto end;
    }
    sta->u4Status = h->vdec_status;
    if (h->v_state == HDI_AV_DECODER_STATE_RUNNING) {
        sta->b_IsLock = TAPI_TRUE;
        sta->b_IsDisplaying = TAPI_TRUE;
        sta->b_DisplayStatus = TAPI_TRUE;
    } else {
        sta->b_IsLock = TAPI_FALSE;
        sta->b_IsDisplaying = TAPI_FALSE;
        sta->b_DisplayStatus = TAPI_FALSE;
    }

    memset(&vs, 0, sizeof(AM_AV_VideoStatus_t));
    AM_AV_GetVideoStatus(AV_DEV_NO, &vs);
    AM_TIME_GetClock(&now);
    sta->u4DecOkCnt = sta->u4ReceiveCnt = (now - a_start_time) * vs.fps / 1000;
    LOGD("%s u4DecOkCnt=%u,vs.frames=%d", __FUNCTION__, sta->u4DecOkCnt, vs.frames);
    end: pthread_mutex_unlock(&lock);
    return rst;
}
//-------------------------------------------------------------------------------------------------
/// To register callback function and set user data for callback.
/// @param  vdec_event      \b IN: the vdec events want to be registerred.(refer to EN_VDEC_EVENT_FLAG)
/// @param  VDEC_EventCb      \b IN: function pointer of the callback function.
/// @return                \b TRUE: success, FALSE: failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Vdec_SetCallBack(TAPI_U32 evt, VDEC_EventCb callback, void* data) {
    Tapi_av_evt_config_params_t evt_cfg;
    status_code_t ret = SUCCESS;
    TAPI_BOOL rst = TAPI_TRUE;
    pthread_mutex_lock(&lock);

    evt_cfg.m_evt = (EN_VDEC_EVENT_FLAG) evt;
    evt_cfg.m_is_enable = TAPI_TRUE;
    evt_cfg.m_notifications_to_skip = 0;
    evt_cfg.m_p_callback = callback;
    evt_cfg.udata = data;

    ret = am_av_evt_config(gAVHandle, &evt_cfg);
    LOGD("%s %d, ret : %d", __FUNCTION__, __LINE__, ret);

    if (ret != SUCCESS)
        rst = TAPI_FALSE;
    pthread_mutex_unlock(&lock);
    return rst;
}

//-------------------------------------------------------------------------------------------------
/// To Unregister callback function .
/// @param  vdec_event      \b IN: the vdec events want to be unregisterred.(refer to EN_VDEC_EVENT_FLAG)
/// @return                \b TRUE: success, FALSE: failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Vdec_UnSetCallBack(TAPI_U32 event) {
    AV_Handle *h;
    EN_VDEC_EVENT_FLAG evt = (EN_VDEC_EVENT_FLAG) event;
    Tapi_av_evt_config_params_t evt_cfg;
    status_code_t ret = SUCCESS;
    TAPI_BOOL rst = TAPI_TRUE;

    pthread_mutex_lock(&lock);

    if (!init_ok) {
        ret = ERROR_NO_INIT;
        goto end;
    }

    h = av_get_handle(gAVHandle);
    if (!h) {
        ret = ERROR_INVALID_HANDLE;
        goto end;
    }

    if (evt >= E_TAPI_VDEC_EVENT_MAX) {
        ret = ERROR_BAD_PARAMETER;
        goto end;
    }

    if (h->events[evt].m_evt == evt) {
        LOGD("%s %d, ret : %d\n", __FUNCTION__, __LINE__, ret);
        evt_cfg.m_evt = E_TAPI_VDEC_EVENT_MAX;
        evt_cfg.m_is_enable = TAPI_FALSE;
        evt_cfg.m_notifications_to_skip = 0;
        evt_cfg.m_p_callback = NULL;
        evt_cfg.udata = NULL;
        h->events[evt] = evt_cfg;
    }

end:
    LOGD("%s %d, ret : %d", __FUNCTION__, __LINE__, ret);
    if (ret != SUCCESS)
        rst = TAPI_FALSE;

    pthread_mutex_unlock(&lock);

    return rst;

}

//-------------------------------------------------------------------------------------------------
/// To set vdec type and init vdec driver.(DTV)
/// @param  enType       IN: vdec type.
///
/// @return                 TRUE: success, FALSE: failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Vdec_SetType(CODEC_TYPE enType) {
    AV_Handle *h;
    status_code_t ret = SUCCESS;
    TAPI_BOOL rst = TAPI_TRUE;

    pthread_mutex_lock(&lock);
    if (!init_ok) {
        ret = ERROR_NO_INIT;
        goto end;
    }

    h = av_get_handle(gAVHandle);
    if (!h) {
        ret = ERROR_INVALID_HANDLE;
        goto end;
    }
    switch (enType) {
    case CODEC_TYPE_MPEG2_IFRAME:
    case CODEC_TYPE_MPEG2:
        h->av_settings.m_vid_stream_type = VFORMAT_MPEG12;
        break;
        ///For H264 IFrame
    case CODEC_TYPE_H264_IFRAME:
    case CODEC_TYPE_H264:
        h->av_settings.m_vid_stream_type = VFORMAT_H264;
        break;
    case CODEC_TYPE_AVS:
        h->av_settings.m_vid_stream_type = VFORMAT_AVS;
        break;
    case CODEC_TYPE_VC1:
        h->av_settings.m_vid_stream_type = VFORMAT_VC1;
        break;
    default:
        h->av_settings.m_vid_stream_type = VFORMAT_MPEG12;
        break;
    }
    end: if (ret != SUCCESS) {
        rst = TAPI_FALSE;
    }
    LOGD("%s %d, enType : %d", __FUNCTION__, __LINE__, enType);
    pthread_mutex_unlock(&lock);
    return rst;
}
//-------------------------------------------------------------------------------------------------
/// To set vid of the vdec.
/// @param               vid
/// @return                TRUE: success, FALSE: failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Vdec_SetVideoPid(TAPI_U32 v_pid) {
    AV_Handle *h;
    status_code_t ret = SUCCESS;
    TAPI_BOOL rst = TAPI_TRUE;

    pthread_mutex_lock(&lock);
    if (!init_ok) {
        ret = ERROR_NO_INIT;
        goto end;
    }

    h = av_get_handle(gAVHandle);
    if (!h) {
        ret = ERROR_INVALID_HANDLE;
        goto end;
    }
    h->av_settings.m_vid_pid = (U16) v_pid;
    end:
    LOGD("%s %d, ret : %d, vid: %lu", __FUNCTION__, __LINE__, ret, v_pid);
    if (ret != SUCCESS)
        rst = TAPI_FALSE;
    pthread_mutex_unlock(&lock);
    return rst;

}
//-------------------------------------------------------------------------------------------------
/// To set aud of the vdec.
/// @param               vid
/// @return                TRUE: success, FALSE: failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Adec_SetAudioPid(TAPI_U32 a_pid) {
    AV_Handle *h;
    status_code_t ret = SUCCESS;
    TAPI_BOOL rst = TAPI_TRUE;
    pthread_mutex_lock(&lock);
    if (!init_ok) {
        ret = ERROR_NO_INIT;
        goto end;
    }

    h = av_get_handle(gAVHandle);
    if (!h) {
        ret = ERROR_INVALID_HANDLE;
        goto end;
    }

    if (h->already_started && h->av_settings.m_aud_pid != (U16) a_pid) {
        h->only_update_audio = 1;
        LOGD("%s %d, update aid: %lu", __FUNCTION__, __LINE__, a_pid);
    }
    h->av_settings.m_aud_pid = (U16) a_pid;
    end:
    LOGD("%s %d, ret : %d, aid: %lu", __FUNCTION__, __LINE__, ret, a_pid);
    if (ret != SUCCESS)
        rst = TAPI_FALSE;
    pthread_mutex_unlock(&lock);
    return rst;

}
//-------------------------------------------------------------------------------------------------
/// To set stop command of the vdec.
/// @param               None
/// @return                TRUE: success, FALSE: failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Vdec_Stop(void) {
    AV_Handle *h;
    status_code_t ret = SUCCESS;
    TAPI_BOOL rst = TAPI_TRUE;
#if 0
    pthread_mutex_lock(&lock);

    if (!init_ok) {
        ret = ERROR_NO_INIT;
        goto end;
    }

    h = av_get_handle(gAVHandle);
    if (!h) {
        ret = ERROR_INVALID_HANDLE;
        goto end;
    }

    ret = am_av_start(h, AM_FALSE, (h->a_state == HDI_AV_DECODER_STATE_RUNNING));
    if (ret == SUCCESS) {
        h->v_state = HDI_AV_DECODER_STATE_STOPPED;
        h->vdec_status = E_VDEC_OK;
    } else {
        h->vdec_status = E_VDEC_FAIL;
    }

    end:
    LOGD("%s %d, ret : %d", __FUNCTION__, __LINE__, ret);
    if (ret != SUCCESS)
    rst = TAPI_FALSE;
    pthread_mutex_unlock(&lock);
#else
    Tapi_AV_stop();
#endif
    return rst;
}

//-------------------------------------------------------------------------------------------------
/// To set play command of the vdec.
/// @param               None
/// @return                \b TRUE: success, FALSE: failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Vdec_Play(void) {
    TAPI_BOOL rst = TAPI_TRUE;
#if 0
    AV_Handle *h;
    status_code_t ret = SUCCESS;

    pthread_mutex_lock(&lock);

    if (!init_ok) {
        ret = ERROR_NO_INIT;
        goto end;
    }

    h = av_get_handle(gAVHandle);
    if (!h) {
        ret = ERROR_INVALID_HANDLE;
        goto end;
    }

    ret = am_av_start(h, AM_TRUE, (h->a_state == HDI_AV_DECODER_STATE_RUNNING));
    if (ret == SUCCESS) {
        h->v_state = HDI_AV_DECODER_STATE_RUNNING;
        h->vdec_status = E_VDEC_OK;
    } else {
        h->vdec_status = E_VDEC_FAIL;
    }

    end:
    LOGD("%s %d, ret : %d", __FUNCTION__, __LINE__, ret);
    if (ret != SUCCESS)
    rst = TAPI_FALSE;
    pthread_mutex_unlock(&lock);
#endif
    return rst;
}
status_code_t hdi_av_start(const handle_t av_handle) {
    AV_Handle *h;
    status_code_t ret = SUCCESS;

    pthread_mutex_lock(&lock);

    if (!init_ok) {
        ret = ERROR_NO_INIT;
        goto end;
    }

    h = av_get_handle(av_handle);
    if (!h) {
        ret = ERROR_INVALID_HANDLE;
        goto end;
    }

    ret = am_av_start(h, AM_TRUE, AM_TRUE);
    if (ret == SUCCESS) {
        h->v_state = HDI_AV_DECODER_STATE_RUNNING;
        h->a_state = HDI_AV_DECODER_STATE_RUNNING;
    }

    end: pthread_mutex_unlock(&lock);

    return ret;
}

//-------------------------------------------------------------------------------------------------
/// To get active vdec type of the vdec. (DTV)
/// @param  enType       OUT: Return acitve vdec type.
/// @return                 TRUE: success, FALSE: failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Vdec_GetActiveVdecType(CODEC_TYPE *enType) {
    AV_Handle *h;
    status_code_t ret = SUCCESS;
    TAPI_BOOL rst = TAPI_TRUE;
    CODEC_TYPE vfmt;

    pthread_mutex_lock(&lock);

    if (!init_ok) {
        ret = ERROR_NO_INIT;
        goto end;
    }

    h = av_get_handle(gAVHandle);
    if (!h) {
        ret = ERROR_INVALID_HANDLE;
        goto end;
    }

    switch (h->av_settings.m_vid_stream_type) {
    case VFORMAT_MPEG12:
        vfmt = CODEC_TYPE_MPEG2;
        break;
    case VFORMAT_MPEG4:
        vfmt = CODEC_TYPE_AVS;
        break;
    case VFORMAT_H264:
        vfmt = CODEC_TYPE_H264;
        break;
    case VFORMAT_VC1:
        vfmt = CODEC_TYPE_VC1;
        break;
    default:
        vfmt = CODEC_TYPE_MPEG2;
        break;
    }
    *enType = vfmt;
    end:
    LOGD("%s %d, ret : %d,*enType = %d", __FUNCTION__, __LINE__, ret, *enType);
    if (ret != SUCCESS)
        rst = TAPI_FALSE;
    pthread_mutex_unlock(&lock);

    return rst;
}

//-------------------------------------------------------------------------------------------------
/// To set AV sync on/off of the vdec.
/// @param  bEnable     IN: enable AV sync or not. FALSE(off) / TRUE(on)
/// @return                 TRUE: success, FALSE: failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Vdec_SyncOn(TAPI_BOOL bEnable) {
    AV_Handle *h;
    status_code_t ret = SUCCESS;
    TAPI_BOOL rst = TAPI_TRUE;
#if 0
    AM_ErrorCode_t rc;
    CODEC_TYPE vfmt;

    pthread_mutex_lock(&lock);

    if (!init_ok) {
        ret = ERROR_NO_INIT;
        goto end;
    }

    h = av_get_handle(gAVHandle);
    if (!h) {
        ret = ERROR_INVALID_HANDLE;
        goto end;
    }

    if (bEnable) {
        rc = AM_FileEcho("/sys/class/tsync/enable", "1");
        //h->av_settings.m_av_sync_mode == HDI_AV_SYNC_MODE_VID;
    } else {
        rc = AM_FileEcho("/sys/class/tsync/enable", "0");
        //h->av_settings.m_av_sync_mode == HDI_AV_SYNC_MODE_DISABLE;
    }
    if (rc < 0)
    ret != FAILED;
    end:
    LOGD("%s %d, ret : %d", __FUNCTION__, __LINE__, ret);
    if (ret != SUCCESS)
    rst = TAPI_FALSE;
    pthread_mutex_unlock(&lock);
#endif
    return rst;
}

//-------------------------------------------------------------------------------------------------
/// To check if TS and STC is closed enough when enable AV sync
/// @param               None
/// @return                TRUE: success, FALSE: failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Vdec_IsSyncDone(void) {
    return TAPI_TRUE;
}
EN_ASPECT_RATIO_CODE Local2Aspect(hdi_av_aspect_ratio_t ratio) {
    EN_ASPECT_RATIO_CODE aspec = E_ASP_1TO1;
    if (ratio == HDI_AV_ASPECT_RATIO_16TO9)
        aspec = E_ASP_16TO9;
    else if (ratio == HDI_AV_ASPECT_RATIO_4TO3)
        aspec = E_ASP_4TO3;
    else
        aspec = E_ASP_1TO1;
    return aspec;
}
//-------------------------------------------------------------------------------------------------
/// To get display information of the vdec.
/// @param  pstInitPara       OUT: vdec display information.
/// @return                 TRUE: success, FALSE: failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Vdec_GetDispInfo(VDEC_DISP_INFO *pstDispInfo) {
    AV_Handle *h;
    AM_AV_VideoAspectRatio_t aspe;
    hdi_av_vid_status_t p_status;
    status_code_t ret = SUCCESS;
    TAPI_BOOL rst = TAPI_TRUE;

    pthread_mutex_lock(&lock);

    if (!init_ok) {
        ret = ERROR_NO_INIT;
        goto end;
    }

    h = av_get_handle(gAVHandle);
    if (!h) {
        ret = ERROR_INVALID_HANDLE;
        goto end;
    }

    memset(&p_status, 0, sizeof(hdi_av_vid_status_t));
    am_get_vid_status(h, &p_status);

    pstDispInfo->u16Height = p_status.m_source_height;
    pstDispInfo->u16Width = p_status.m_source_width;
    pstDispInfo->u8Interlace = p_status.m_is_interlaced;
    pstDispInfo->u32FrameRate = p_status.m_frame_rate;
    //AM_AV_GetVideoAspectRatio(AV_DEV_NO, &aspe);
    pstDispInfo->enAspectRate = Local2Aspect(p_status.m_aspect_ratio);
    if (pstDispInfo->u16Height >= 720 && pstDispInfo->u16Width >= 1280)
        pstDispInfo->b_IsHD = TAPI_TRUE;
    else
        pstDispInfo->b_IsHD = TAPI_FALSE;
    //pstDispInfo->en_3D_type= h->disp_settings->m_aspect_ratio;
    LOGD(" u32FrameRate=%ld\n", pstDispInfo->u32FrameRate);
    LOGD(" u16Width=%d\n", pstDispInfo->u16Width);
    LOGD(" u16Height=%d\n", pstDispInfo->u16Height);
    LOGD(" enAspectRate=%d\n", pstDispInfo->enAspectRate);
    LOGD(" u8Interlace=%d\n", pstDispInfo->u8Interlace);
    LOGD(" IsHD =%d\n", pstDispInfo->b_IsHD);

    end: if (ret != SUCCESS)
        rst = TAPI_FALSE;
    pthread_mutex_unlock(&lock);

    return rst;
}

//-------------------------------------------------------------------------------------------------
/// @ Function  Description: get Vedio Scramble state
/// @param    out : scramble state :scramble or  Discramble
/// @return         success TAPI_TRUE /faile TAPI_FALSE
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Vdec_GetScrambleState(TAPI_U8 u1Pidx, En_DVB_SCRAMBLE_STATE* peScramble_State) {
    TAPI_BOOL ret = TAPI_TRUE;
    pthread_mutex_lock(&scambled_lock);
    if (scambled_status.video || scambled_status.no_data) {
        *peScramble_State = TAPI_DVB_SCRAMBLE_STATE_SCRAMBLED;
         //LOGD("%s scambled", __FUNCTION__);
    } else{
        *peScramble_State = TAPI_DVB_SCRAMBLE_STATE_DESCRAMBLED;
         //LOGD("%s no scambled", __FUNCTION__);
    }
    //LOGD("%s scambled_status.video=%d,audio=%d,no_data=%d\n", __FUNCTION__,
    //scambled_status.video, 
    //scambled_status.audio, 
    //scambled_status.no_data);
    pthread_mutex_unlock(&scambled_lock);
    return ret;
}

static void am_wait_vid_es_end_cb(int dev_no, int event_type, void *param, void *data) {
    if (event_type == AM_AV_EVT_VIDEO_ES_END) {
        pthread_mutex_lock(&lock);
        fl_decode_vid_es = 0;
        pthread_mutex_unlock(&lock);
        LOGD("%s %d", __FUNCTION__, __LINE__);
        pthread_cond_broadcast(&cond);
    }
}

TAPI_BOOL Tapi_Vdec_DecodeIframe(CODEC_TYPE type, TAPI_U32* iframeSrcAddr, TAPI_U32 iframeSize) {
    AV_Handle *h;
    status_code_t ret = SUCCESS;
    TAPI_BOOL rst = TAPI_TRUE;
    AM_ErrorCode_t rc;
    vformat_t vf = VFORMAT_MPEG12;
    pthread_mutex_lock(&lock);

    if (!init_ok) {
        ret = ERROR_NO_INIT;
        goto end;
    }

    if (!iframeSrcAddr || iframeSize == 0) {
        ret = ERROR_BAD_PARAMETER;
        goto end;
    }

    h = av_get_handle(gAVHandle);
    if (!h) {
        ret = ERROR_INVALID_HANDLE;
        goto end;
    }

    if (h->v_state == HDI_AV_DECODER_STATE_RUNNING) {
        ret = ERROR_DEVICE_BUSY;
        goto end;
    }

    fl_decode_vid_es = 1;
    switch (type) {
    case CODEC_TYPE_MPEG2:
        vf = VFORMAT_MPEG12;
        break;
        ///H264
    case CODEC_TYPE_H264:
        vf = VFORMAT_H264;
        break;
        ///AVS
    case CODEC_TYPE_AVS:
        vf = VFORMAT_AVS;
        break;
        ///VC1
    case CODEC_TYPE_VC1:
        vf = VFORMAT_VC1;
        break;
        ///For MPEG 1/2 IFrame
    case CODEC_TYPE_MPEG2_IFRAME:
        vf = VFORMAT_MPEG12;
        break;
        ///For H264 IFrame
    case CODEC_TYPE_H264_IFRAME:
        vf = VFORMAT_H264MVC;
        break;
    default:
        vf = VFORMAT_MPEG12;
        break;

    }
    rc = AM_EVT_Subscribe(AV_DEV_NO, AM_AV_EVT_VIDEO_ES_END, am_wait_vid_es_end_cb, NULL);
    if (rc < 0) {
        ret = FAILED;
        goto end;
    }
    m_preview_init = 99;
    //AM_AV_SetVPathPara(AV_DEV_NO, AM_AV_FREE_SCALE_DISABLE, AM_AV_DEINTERLACE_DISABLE, AM_AV_PPMGR_DISABLE);
    am_set_vpath(AM_AV_DEINTERLACE_DISABLE, AM_AV_PPMGR_DISABLE);
    h->v_state = HDI_AV_DECODER_STATE_RUNNING;

    AM_AV_DisableVideoBlackout(AV_DEV_NO);

    rc = AM_AV_StartVideoESData(AV_DEV_NO, vf, (const uint8_t *) iframeSrcAddr, (int) iframeSize);
    if (rc < 0) {
        AM_EVT_Unsubscribe(AV_DEV_NO, AM_AV_EVT_VIDEO_ES_END, am_wait_vid_es_end_cb, NULL);
        ret = FAILED;
        //AM_AV_SetVPathPara(AV_DEV_NO, AM_AV_FREE_SCALE_DISABLE, AM_AV_DEINTERLACE_ENABLE, AM_AV_PPMGR_DISABLE);
        goto end;
    }

    v_source = HDI_AV_SOURCE_MEM;

    av_flags |= AV_VID_START;
    pthread_cond_broadcast(&cond);

    if (fl_decode_vid_es) {
        struct timespec ts;

        AM_TIME_GetTimeSpecTimeout(1000, &ts);
        pthread_cond_timedwait(&cond, &lock, &ts);
    }

    AM_EVT_Unsubscribe(AV_DEV_NO, AM_AV_EVT_VIDEO_ES_END, am_wait_vid_es_end_cb, NULL);
    h->v_state = HDI_AV_DECODER_STATE_STOPPED;
    fl_decode_vid_es = 0;

    usleep(200 * 1000);
    //AM_AV_SetVPathPara(AV_DEV_NO, AM_AV_FREE_SCALE_DISABLE, AM_AV_DEINTERLACE_ENABLE, AM_AV_PPMGR_DISABLE);

    av_flags &= ~AV_VID_START;
    pthread_cond_broadcast(&cond);

    end:
    LOGD("%s %d, ret : %d", __FUNCTION__, __LINE__, ret);
    if (ret != SUCCESS) {
        rst = TAPI_FALSE;
    }
    pthread_mutex_unlock(&lock);

    return rst;
}
TAPI_U16 Tapi_Vdec_GetFrameCnt(void) {
    AV_Handle *h;
    hdi_av_vid_status_t p_status;
    status_code_t ret = SUCCESS;
    TAPI_BOOL rst = TAPI_TRUE;

    pthread_mutex_lock(&lock);

    if (!init_ok) {
        ret = ERROR_NO_INIT;
        goto end;
    }

    h = av_get_handle(gAVHandle);
    if (!h) {
        ret = ERROR_INVALID_HANDLE;
        goto end;
    }

    memset(&p_status, 0, sizeof(hdi_av_vid_status_t));
    am_get_vid_status(h, &p_status);

    end:
    LOGD("%s %d, ret : %d", __FUNCTION__, __LINE__, ret);
    if (ret != SUCCESS) {
        rst = TAPI_FALSE;
    }
    pthread_mutex_unlock(&lock);

    return p_status.m_disp_pic_count;

}
TAPI_BOOL Tapi_Adec_Init(void) {
#if 0
    AM_AOUT_OpenPara_t aout_para;
    AM_ErrorCode_t rc;

    memset(&aout_para, 0, sizeof(aout_para));
    rc = AM_AOUT_Open(AOUT_DEV_NO, &aout_para);
    if (rc != AM_SUCCESS) {
        AM_AOUT_Close(AOUT_DEV_NO);
        printf("%s %d, ret : %d", __FUNCTION__, __LINE__, rc);
        return TAPI_FALSE;
    }
#endif
    return TAPI_TRUE;

}

TAPI_BOOL Tapi_Adec_Finalize(void) {

    return TAPI_TRUE;
}

void Tapi_Adec_SetAbsoluteVolume(TAPI_U8 uPercent, AudioPath_ uAudioPath) {

    audio_SetAbsoluteVolume(uPercent, uAudioPath);

}

TAPI_BOOL Tapi_Adec_SetType(AUDIO_DSP_SYSTEM_ eAudioDSPSystem) {
    AV_Handle *h;
    status_code_t ret = SUCCESS;
    TAPI_BOOL rst = TAPI_TRUE;
    AM_AV_AFormat_t oldType;

    pthread_mutex_lock(&lock);
    if (!init_ok) {
        ret = ERROR_NO_INIT;
        goto end;
    }

    h = av_get_handle(gAVHandle);
    if (!h) {
        ret = ERROR_INVALID_HANDLE;
        goto end;
    }

    oldType = h->av_settings.m_aud_stream_type;

    switch (eAudioDSPSystem) {
    case E_AUDIO_DSP_MPEG_:
    case E_AUDIO_DSP_MPEG_AD_:
        h->av_settings.m_aud_stream_type = AFORMAT_MPEG;
        break;

    case E_AUDIO_DSP_AC3_:
    case E_AUDIO_DSP_AC3_AD_:
    case E_AUDIO_DSP_AC3P_:
    case E_AUDIO_DSP_AC3P_AD_:
        h->av_settings.m_aud_stream_type = AFORMAT_AC3;
        break;

    case E_AUDIO_DSP_AAC_:
    case E_AUDIO_DSP_AACP_:
    case E_AUDIO_DSP_AACP_AD_:
        h->av_settings.m_aud_stream_type = AFORMAT_AAC;
        break;
    case E_AUDIO_DSP_DTS_:
        h->av_settings.m_aud_stream_type = AFORMAT_DTS;
        break;
    default:
        h->av_settings.m_aud_stream_type = AFORMAT_MPEG;
        break;
    }

    if (h->already_started && oldType != h->av_settings.m_aud_stream_type) {
        h->only_update_audio = 1;
        LOGD("%s %d, update audio fmt:%d", __FUNCTION__, __LINE__, h->av_settings.m_aud_stream_type);
    }

    end: if (ret != SUCCESS) {
        rst = TAPI_FALSE;
    }
    LOGD("%s %d, ret : %d", __FUNCTION__, __LINE__, ret);
    pthread_mutex_unlock(&lock);
    return rst;
}

TAPI_BOOL Tapi_Adec_Stop(void) {

    TAPI_BOOL ret = TAPI_TRUE;

    //if (Audio_Stop() != SUCCESS)
    //    ret = TAPI_FALSE;
    return ret;
}

TAPI_BOOL Tapi_Adec_Play(void) {
    LOGD("%s %d", __FUNCTION__, __LINE__);
    return Tapi_AV_start();
}

TAPI_BOOL Tapi_Adec_AVSync(void) {

    return TAPI_TRUE;
}

TAPI_BOOL Tapi_Adec_FreeRun(void) {
    AV_Handle *h;
    TAPI_BOOL ret = TAPI_TRUE;

    pthread_mutex_lock(&lock);

    h = av_get_handle(gAVHandle);
    if (!h) {
        ret = TAPI_TRUE;
        goto end;
    }
    if (h->a_state == HDI_AV_DECODER_STATE_RUNNING) {
        ret = TAPI_FALSE;
    }
    end: pthread_mutex_unlock(&lock);

    return ret;
}
TAPI_BOOL Tapi_Adec_SetOutputMode(En_DVB_soundModeType_ aud_channel) {
    status_code_t ret = SUCCESS;

    if (aud_channel < TAPI_AUD_DVB_SOUNDMODE_STEREO_ || aud_channel > TAPI_AUD_DVB_SOUNDMODE_MIXED_)
        return TAPI_FALSE;
    ret = Audio_SetOutPutMode(aud_channel);
    if (ret != SUCCESS)
        return TAPI_FALSE;

    return TAPI_TRUE;
}

TAPI_BOOL Tapi_Adec_GetScrambleState(TAPI_U8 u1Pidx, En_DVB_SCRAMBLE_STATE* peScramble_State) {
    TAPI_BOOL ret = TAPI_TRUE;
    pthread_mutex_lock(&scambled_lock);
    if (scambled_status.audio) {
            *peScramble_State = TAPI_DVB_SCRAMBLE_STATE_SCRAMBLED;
            //LOGD("%s scambled", __FUNCTION__);
     } else{
            *peScramble_State = TAPI_DVB_SCRAMBLE_STATE_DESCRAMBLED;
            //LOGD("%s no scambled", __FUNCTION__);
     }
    //LOGD("%s scambled_status.video=%d,audio=%d,no_data=%d\n", __FUNCTION__,
    //scambled_status.video, 
    //scambled_status.audio, 
    //scambled_status.no_data);
    pthread_mutex_unlock(&scambled_lock);
    return ret;
}

TAPI_BOOL SetAudioMute(AUDIOMUTETYPE_ eAudioMuteType) {

    return TAPI_TRUE;
}

TAPI_BOOL SetScreenMute(TAPI_BOOL bScreenMute, TAPI_VIDEO_Screen_Mute_Color enColor, TAPI_U16 u16ScreenUnMuteTime) {
#if 0
    AM_ErrorCode_t rc;
    if(bScreenMute)
    rc = AM_AV_EnableVideo(AV_DEV_NO);
    else
    rc = AM_AV_DisableVideo(AV_DEV_NO);

    printf("%s %d\n", __FUNCTION__, __LINE__);

    if(rc == AM_SUCCESS)
    return TAPI_TRUE;
    else
    return TAPI_FALSE;
#endif
    return TAPI_TRUE;
}

TAPI_BOOL SetWindow(VIDEO_WINDOW_TYPE *ptCropWin, VIDEO_WINDOW_TYPE *ptDispWin, const TAPI_VIDEO_ARC_Type ARCType) {
    AV_Handle *h;
    TAPI_BOOL ret = TAPI_TRUE;
    int nx, ny, nw, nh;
    AM_ErrorCode_t rc;
    /*
     pthread_mutex_lock(&lock);

     if (!init_ok) {
     ret = TAPI_FALSE;
     goto end;
     }

     h = av_get_handle(gAVHandle);
     if (!h) {
     ret = TAPI_FALSE;
     goto end;
     }

     if(ptCropWin){
     h->in_left = ptCropWin->x;
     h->in_top = ptCropWin->y;
     h->in_width = ptCropWin->width ? ptCropWin->width : 1920;
     h->in_height = ptCropWin->height ? ptCropWin->height : 1080;
     }
     else
     {
     LOGE("%s,ptCropWin is NULL\n",__FUNCTION__);
     }

     if(ptDispWin){
     h->out_left = ptDispWin->x;
     h->out_top = ptDispWin->y;
     h->out_width = ptDispWin->width ? ptDispWin->width : 1920;
     h->out_height = ptDispWin->height ? ptDispWin->height : 1080;
     }
     else
     {
     LOGE("%s,ptDispWin is NULL\n",__FUNCTION__);
     }

     nx = 1920 * h->out_left / h->in_width;
     ny = 1080 * h->out_top / h->in_height;
     nw = 1920 * h->out_width / h->in_width;
     nh = 1080 * h->out_height / h->in_height;

     rc = AM_AV_SetVideoWindow(AV_DEV_NO, nx, ny, nw, nh);
     if (rc < 0) {
     ret = TAPI_FALSE;
     goto end;
     }

     if (ARCType == E_AR_AUTO) {
     rc = AM_AV_SetVideoAspectRatio(AV_DEV_NO, AM_AV_VIDEO_ASPECT_AUTO);
     } else if (ARCType == E_AR_4x3) {
     rc = AM_AV_SetVideoAspectRatio(AV_DEV_NO, AM_AV_VIDEO_ASPECT_4_3);
     } else {
     rc = AM_AV_SetVideoAspectRatio(AV_DEV_NO, AM_AV_VIDEO_ASPECT_16_9);
     }
     if (rc < 0) {
     ret = TAPI_FALSE;
     }
     end: pthread_mutex_unlock(&lock);
     */
    return ret;
}

TAPI_BOOL Tapi_Dtv_Startup(void) {
    int i = 0;
    char prop_value[PROPERTY_VALUE_MAX];

    memset(prop_value, '\0', PROPERTY_VALUE_MAX);
    for (i = 0; i < 30; i++) {
        property_get("tvin.lastselectsource", prop_value, "null");
        if (strcmp(prop_value, "SOURCE_DTV") == 0) {
            bDtvStartup = TAPI_TRUE;
            LOGD("%s, prop_value = %s, i = %d true\n", __FUNCTION__, prop_value, i);
            return TAPI_TRUE;
        } else {
            usleep(100 * 1000);
        }
    }

    LOGD("%s, prop_value = %s, i = %d true\n", __FUNCTION__, prop_value, i);
    return TAPI_FALSE;
}

TAPI_BOOL Tapi_Dtv_freeze_changingCH(TAPI_BOOL bfreeze) {
    AV_Handle *h;
    TAPI_BOOL ret = TAPI_TRUE;
    
    pthread_mutex_lock(&lock);
    h = av_get_handle(gAVHandle);
    if (h) {
          if(bfreeze){
             h->av_settings.m_vid_stop_mode = HDI_AV_VID_STOP_MODE_FREEZE;
         }
         else{
          h->av_settings.m_vid_stop_mode = HDI_AV_VID_STOP_MODE_BLACK;
        }
    }
    else{
      ret = TAPI_FALSE;
    }
    pthread_mutex_unlock(&lock);
    LOGD("%s bfreeze =%d\n", __FUNCTION__,bfreeze);
    return ret;
}

TAPI_BOOL Tapi_Dtv_ClearVideoBuffer(void)
{
  AV_Handle *h;
  pthread_mutex_lock(&lock);
  h = av_get_handle(gAVHandle);
  if (h && h->av_settings.m_vid_stop_mode == HDI_AV_VID_STOP_MODE_FREEZE) {
       AM_AV_EnableVideoBlackout(AV_DEV_NO);
       AM_FileEcho("/sys/class/video/test_screen",_BLUE_COLOR_);
  }
  pthread_mutex_unlock(&lock);
  LOGD("%s\n", __FUNCTION__);
  return TAPI_TRUE;
}

