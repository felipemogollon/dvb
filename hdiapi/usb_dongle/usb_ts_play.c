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
#include "am_av.h"
#include "am_vout.h"
#include "am_aout.h"
#include "am_dmx.h"
#include "am_misc.h"
#include "am_util.h"
#include "am_time.h"
#include "am_debug.h"
#include "TTvHW_player.h"
#include "TTvHW_decoder.h"
#include "../include/hdi_config.h"

#include "usb_ts_play.h"

#define LOG_TAG "usb_dongle_api"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define AV_DEV_NO       0
#define DMX_DEV_NO (0)
#define VOUT_DEV_NO     0
#define AOUT_DEV_NO     0
#define _BLUE_COLOR_ "0x29f06e"
#define _BLACK_COLOR_ "0x108080"
#define VID_RESOLUTION_FILE       "/sys/class/video/device_resolution"
#define VID_AXIS_FILE       "/sys/class/video/axis"


//#define _DUMP_TS_
//#define _USE_THREAD_
#define _RESTOR_WINDOW_

#ifdef _DUMP_TS_
#define __USB_PATH__ "/mnt/usb/667E-2F6B/"
FILE *fd_write = 0;
#endif

typedef struct ts_av_settings_s
{
	AM_AV_VFormat_t m_vid_stream_type;
	AM_AV_AFormat_t m_aud_stream_type;
	U16 m_aud_pid;
	U16 m_vid_pid;
#ifdef _RESTOR_WINDOW_
	int video_x;
	int video_y;
	int video_w;
	int video_h;
#endif
} ts_av_settings_t;

#ifdef _USE_THREAD_
typedef struct AV_scambled_s
{
	int video;
	int audio;
	int no_data;
	int already_cleared;
} scambled_status_t;

static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t scambled_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
static scambled_status_t scambled_status;
static pthread_t dongle_thread;
static volatile int thread_runing;
#endif

static pthread_mutex_t lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;

static ts_av_settings_t saved_av;

#ifdef _USE_THREAD_
static void * dongle_thread_entry(void *arg)
{
	int signalStatus = 0;

	pthread_mutex_lock(&lock);
	while (1)
	{
		struct timespec ts;
		int scambledStatus;

		AM_TIME_GetTimeSpecTimeout(200, &ts);
		pthread_cond_timedwait(&cond, &lock, &ts);
		if (!thread_runing)
			break;

		pthread_mutex_lock(&scambled_lock);
		scambledStatus = scambled_status.video
										|| scambled_status.audio
										|| scambled_status.no_data;
		pthread_mutex_unlock(&scambled_lock);
		if (scambledStatus && !scambled_status.already_cleared)
		{
			scambled_status.already_cleared = 1;
			AM_AV_EnableVideoBlackout(AV_DEV_NO); //clear freeze video
			AM_FileEcho("/sys/class/video/test_screen", _BLACK_COLOR_); //clear freeze video        
		}
	}
	pthread_mutex_unlock(&lock);
	return NULL;
}

static void v_scambled_callback(int dev_no, int event_type, void *param,
		void *user_data)
{
	pthread_mutex_lock(&scambled_lock);
	scambled_status.video = 1;
	pthread_mutex_unlock(&scambled_lock);
	LOGD("%s\n", __FUNCTION__);
}

static void a_scambled_callback(int dev_no, int event_type, void *param,
		void *user_data)
{
	pthread_mutex_lock(&scambled_lock);
	scambled_status.audio = 1;
	pthread_mutex_unlock(&scambled_lock);
	LOGD("%s\n", __FUNCTION__);
}

static void d_scambled_callback(int dev_no, int event_type, void *param,
		void *user_data)
{
	pthread_mutex_lock(&scambled_lock);
	scambled_status.no_data = 1;
	pthread_mutex_unlock(&scambled_lock);
	LOGD("%s\n", __FUNCTION__);
}
#endif
/*初始化TS播放器*/
TAPI_BOOL  Tapi_TsPlay_Init( )
{
	static AM_DMX_OpenPara_t dmx_para;
	AM_AV_OpenPara_t para;
	AM_VOUT_OpenPara_t vo_para;
	AM_AOUT_OpenPara_t ao_para;
	char buf[64];
	int i;

	for(i = 0; i < 40; i++)
	{
		property_get("tv.vdec.used", buf, "null");
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
	property_set("usb.dongle.dvbc.used", "true");
	pthread_mutex_lock(&lock);
	memset(&vo_para, 0, sizeof(vo_para));
	AM_VOUT_Open(VOUT_DEV_NO, &vo_para);
	memset(&ao_para, 0, sizeof(ao_para));
	AM_AOUT_Open(AOUT_DEV_NO, &ao_para);
	para.vout_dev_no = VOUT_DEV_NO;
	AM_AV_Open(AV_DEV_NO, &para);
	AM_AV_EnableVideo(AV_DEV_NO);
	AM_VOUT_Disable(VOUT_DEV_NO);
	AM_FileEcho("/sys/module/amvdec_h264/parameters/dec_control", "3");
	AM_FileEcho("/sys/module/amvdec_mpeg12/parameters/dec_control", "62");
	AM_FileEcho("/sys/module/amvideo/parameters/smooth_sync_enable", "1"); //to enable video more smooth while reset PTS
	AM_FileEcho("/sys/module/amvdec_h264/parameters/fatal_error_reset", "0"); //avoid "fatal error happend" when playing h264 ts
	AM_FileEcho("/sys/module/di/parameters/bypass_dynamic", "0"); //need set 0 to avoid that subtile flutter
	AM_FileEcho("/sys/class/amaudio/enable_adjust_pts", "0"); //need set 0 for the subtitle smoothly  moving
	AM_FileEcho("/sys/module/amvdec_h264/parameters/toggle_drop_B_frame_on",
			"0"); //switch off drop B frame to avoid the video flickers when unplug and plug the cable for the HD signal
	memset(&dmx_para, 0, sizeof(dmx_para));
	if (AM_DMX_Open(DMX_DEV_NO, &dmx_para) != AM_SUCCESS)
	{
		LOGE("AM_DMX_Open failed\n");
		goto end;
	}
	else
	{
		AM_DMX_SetSource(DMX_DEV_NO, AM_DMX_SRC_HIU);
	}
	AM_AV_SetTSSource(AV_DEV_NO, AM_AV_TS_SRC_HIU);
	memset(&saved_av, 0, sizeof(saved_av));
	LOGD("%s %d", __FUNCTION__, __LINE__);
	AM_FileEcho("/sys/module/di/parameters/bypass_hd", "0");
	/* 
	 usleep(1000);
	 AM_FileEcho("/sys/class/vfm/map", "rm default");
	 usleep(1000);
	 AM_FileEcho("/sys/class/vfm/map", "rm tvpath");
	 usleep(1000);
	 AM_FileEcho("/sys/class/vfm/map", "add default decoder deinterlace d2d3 amvideo");
	 */
#ifdef _RESTOR_WINDOW_
	AM_AV_GetVideoWindow(AV_DEV_NO,&saved_av.video_x,&saved_av.video_y,
																&saved_av.video_w,&saved_av.video_h);
#endif
	if(AM_FileRead(VID_RESOLUTION_FILE, buf, sizeof(buf))==AM_SUCCESS)
	{/* restore*/
		if(!strncmp(buf, "1920x1080", 9))
		{
#ifdef _RESTOR_WINDOW_
				AM_AV_SetVideoWindow(AV_DEV_NO, 0, 0,1919,1079);
#endif
			AM_FileEcho("/sys/class/display/mode", "lvds1080p50hz");
		}
		else
		{
#ifdef _RESTOR_WINDOW_
			AM_AV_SetVideoWindow(AV_DEV_NO, 0, 0,1365,767);
#endif
			AM_FileEcho("/sys/class/display/mode", "lvds768p50hz");
		}
	}

	pthread_mutex_unlock(&lock);
#ifdef _USE_THREAD_
	AM_EVT_Subscribe(0, AM_AV_EVT_VIDEO_SCAMBLED, v_scambled_callback, NULL);
	AM_EVT_Subscribe(0, AM_AV_EVT_AUDIO_SCAMBLED, a_scambled_callback, NULL);
	AM_EVT_Subscribe(0, AM_AV_EVT_AV_NO_DATA, d_scambled_callback, NULL);

	pthread_mutex_lock(&scambled_lock);
	scambled_status.video = 0;
	scambled_status.audio = 0;
	scambled_status.no_data = 0;
	scambled_status.already_cleared = 0;
	pthread_mutex_unlock(&scambled_lock);

	thread_runing = 1;
	if (pthread_create(&dongle_thread, NULL, dongle_thread_entry, NULL))
	{
		LOGE("cant create dongle_thread!\n");
	}
#endif
	return TAPI_SUCCESS;
end:
	pthread_mutex_unlock(&lock);

	return TAPI_FAILURE;
}

static AM_AV_VFormat_t trans_video_type(EN_VID_CODE_TYPE inType)
{
	AM_AV_VFormat_t vfmt;

    switch (inType) 
	{
    case EN_VID_CODE_TYPE_MPEG2:
        vfmt = VFORMAT_MPEG12;
        break;
    case EN_VID_CODE_TYPE_MPEG4:
        vfmt = VFORMAT_MPEG4;
        break;
    case EN_VID_CODE_TYPE_H263:
    case EN_VID_CODE_TYPE_H264:
        vfmt = VFORMAT_H264;
        break;
    case EN_VID_CODE_TYPE_AVS:
        vfmt = VFORMAT_AVS;
        break;
    case EN_VID_CODE_TYPE_REAL8:
    case EN_VID_CODE_TYPE_REAL9:
        vfmt = VFORMAT_AVS;
        break;
        
    case EN_VID_CODE_TYPE_VC1:
        vfmt = VFORMAT_VC1;
        break;
    default:
        vfmt = VFORMAT_MPEG12;
        break;
    }
	return vfmt;
}

static AM_AV_AFormat_t trans_audio_type(EN_AUD_CODE_TYPE inType)
{
	AM_AV_AFormat_t afmt;

    switch (inType)
	{
    case EN_AUD_CODE_TYPE_MP2:
        afmt = AFORMAT_MPEG;
        break;

    case EN_AUD_CODE_TYPE_AC3:
        afmt = AFORMAT_AC3;
        break;

    case EN_AUD_CODE_TYPE_AAC:
        afmt = AFORMAT_AAC;
        break;
    case EN_AUD_CODE_TYPE_EAC3:
        afmt = AFORMAT_EAC3;
        break;
    case EN_AUD_CODE_TYPE_DTS:
        afmt = AFORMAT_DTS;
        break;
    default:
        afmt = AFORMAT_MPEG;
        break;
    }
	return afmt;
}

/*销毁TS播放器*/
TAPI_BOOL  Tapi_TsPlay_Deinit( )
{
#ifdef _USE_THREAD_
	thread_runing = 0;
	pthread_cond_broadcast(&cond);
	if (dongle_thread != -1)
	{
		pthread_join(dongle_thread, NULL);
		dongle_thread = -1;
	}
	AM_EVT_Unsubscribe(0, AM_AV_EVT_VIDEO_SCAMBLED, v_scambled_callback, NULL);
	AM_EVT_Unsubscribe(0, AM_AV_EVT_AUDIO_SCAMBLED, a_scambled_callback, NULL);
	AM_EVT_Unsubscribe(0, AM_AV_EVT_AV_NO_DATA, d_scambled_callback, NULL);
#endif
	pthread_mutex_lock(&lock);
	LOGD("%s %d", __FUNCTION__, __LINE__);
#ifdef _RESTOR_WINDOW_
	if(saved_av.video_w && saved_av.video_h)
				AM_AV_SetVideoWindow(AV_DEV_NO,saved_av.video_x,saved_av.video_y,
																			saved_av.video_w,saved_av.video_h);
#endif
	AM_AV_EnableVideoBlackout(AV_DEV_NO); //clear freeze video
	AM_FileEcho("/sys/class/video/test_screen", _BLACK_COLOR_); //clear freeze video
	AM_FileEcho("/sys/class/amaudio/enable_adjust_pts", "1"); //restore
	AM_FileEcho("/sys/module/amvdec_h264/parameters/fatal_error_reset", "1"); //restore
	AM_FileEcho("/sys/module/di/parameters/bypass_dynamic", "2"); //restore
	AM_AV_DisableVideo(AV_DEV_NO);
	AM_AOUT_Close(AOUT_DEV_NO);
	AM_VOUT_Close(VOUT_DEV_NO);
	AM_AV_Close(AV_DEV_NO);

	AM_DMX_Close(DMX_DEV_NO);
/*	usleep(50*1000);
	AM_FileEcho("/sys/class/display/mode", "lvds1080p"); //restore
	usleep(50*1000);*/
	pthread_mutex_unlock(&lock);
	property_set("usb.dongle.dvbc.used", "false");
	return TAPI_SUCCESS;
}

/*设置ts播放参数*/
TAPI_BOOL  Tapi_TsPlay_SetParam( ST_TS_PLAY_PARAM* stTsPlayParam )
{
	AM_AV_AFormat_t afmt;
	AM_AV_VFormat_t vfmt;

	if(!stTsPlayParam)
	{
		LOGE("%s, null poiter\n", __FUNCTION__);
		return TAPI_FAILURE;
	}

	pthread_mutex_lock(&lock);
	vfmt = trans_video_type(stTsPlayParam->Video_CodeType);
	afmt = trans_audio_type(stTsPlayParam->Audio_CodeType);
	saved_av.m_vid_stream_type = vfmt;
	saved_av.m_aud_stream_type = afmt;
	saved_av.m_vid_pid = stTsPlayParam->Video_Pid;
	saved_av.m_aud_pid = stTsPlayParam->Audio_Pid;
	
    pthread_mutex_unlock(&lock);
	LOGD("%s AudPid:%d AudType:%d VidPid:%d VidType:%d", 
			__FUNCTION__, stTsPlayParam->Audio_Pid, stTsPlayParam->Audio_CodeType, stTsPlayParam->Video_Pid, stTsPlayParam->Video_CodeType);
	return TAPI_SUCCESS;
}
/*往播放器推送TS数据*/
TAPI_BOOL Tapi_TsPlay_PushData( void *data, TAPI_U32 data_len )
{
	pthread_mutex_lock(&lock);
	// LOGD("%s %d", __FUNCTION__, __LINE__);
	AM_AV_InjectData(AV_DEV_NO, AM_AV_INJECT_MULTIPLEX, data, &data_len, -1);
	pthread_mutex_unlock(&lock);
#ifdef _DUMP_TS_
	if(fd_write > 0)
	{
		fwrite(data, sizeof(char), data_len, fd_write);
	}
#endif
	return TAPI_SUCCESS;
}
/*播放TS*/
TAPI_BOOL  Tapi_TsPlay_Start( )
{
	AM_AV_InjectPara_t para;
#ifdef _DUMP_TS_
	char n_buf[64];
	int i = 0;
#endif

	LOGD("%s %d", __FUNCTION__, __LINE__);
	pthread_mutex_lock(&lock);
	memset(&para, 0, sizeof(para));
	para.vid_fmt = saved_av.m_vid_stream_type;
	para.aud_fmt = saved_av.m_aud_stream_type;
	para.pkg_fmt = PFORMAT_TS;
	para.vid_id = saved_av.m_vid_pid;
	para.aud_id = saved_av.m_aud_pid;

	AM_AV_SetTSSource(AV_DEV_NO, AM_AV_TS_SRC_HIU);
	AM_AV_StartInject(AV_DEV_NO, &para);

	//AM_AV_StartTS(AV_DEV_NO, saved_av.m_vid_pid, saved_av.m_aud_pid, saved_av.m_vid_stream_type, saved_av.m_aud_stream_typ);

#ifdef _DUMP_TS_
	memset((void *) n_buf, 0, sizeof(n_buf));
	do
	{
		i++;
		sprintf(n_buf, "/mnt/usb/667E-2F6B/ts_dump%d.ts",i);
		LOGD("checking %s \n",n_buf);
	}while(access(n_buf,0) == 0);

	fd_write=fopen(n_buf, "wb");
	if(fd_write == NULL)
	LOGD("%s is not openned\n",n_buf);
	else
	LOGD("%s is openned\n",n_buf);
#endif
	pthread_mutex_unlock(&lock);
	return TAPI_SUCCESS;
}
/*停止播放*/
TAPI_BOOL  Tapi_TsPlay_Stop( )
{
	LOGD("%s %d", __FUNCTION__, __LINE__);
#ifdef _USE_THREAD_
	pthread_mutex_lock(&scambled_lock);
	scambled_status.video = 0;
	scambled_status.audio = 0;
	scambled_status.no_data = 0;
	scambled_status.already_cleared = 0;
	pthread_mutex_unlock(&scambled_lock);
#endif
	pthread_mutex_lock(&lock);
	AM_AV_DisableVideoBlackout(AV_DEV_NO); //freeze channel
	AM_AV_StopInject(AV_DEV_NO);

#ifdef _DUMP_TS_
	if(fd_write > 0)
	fclose(fd_write);
	system("sync");
#endif
	pthread_mutex_unlock(&lock);
	return TAPI_SUCCESS;
}
/*获取Dolby标识*/
TAPI_BOOL Tapi_TsPlay_GetDolbyIdentifier( EN_DOLBY_IDENTIFYIER* enDolbyIndent ){
	return TAPI_FAILURE;
}
/*设置左右声道*/
TAPI_BOOL  Tapi_TsPlay_SetSoundMode( EN_AUD_SOUNDMODE_TYPE* enAudModeType ){
	return TAPI_FAILURE;
}
/*获取声道模式*/
TAPI_BOOL  Tapi_TsPlay_GetSoundMode( EN_AUD_SOUNDMODE_TYPE* enAudModeType ){
	return TAPI_FAILURE;
}
/*设置静帧换台*/
TAPI_BOOL  Tapi_TsPlay_SetFreezingChangingChannel( TAPI_BOOL  bFreeze ){
	return TAPI_FAILURE;
}
/*清屏*/
TAPI_BOOL  Tapi_TsPlay_ScreenClear()
{
	pthread_mutex_lock(&lock);
	LOGD("%s %d", __FUNCTION__, __LINE__);
	AM_AV_EnableVideoBlackout(AV_DEV_NO); //clear freeze video
	AM_FileEcho("/sys/class/video/test_screen", _BLACK_COLOR_); //clear freeze video
	pthread_mutex_unlock(&lock);
	return TAPI_SUCCESS;
}
/*设置视频小窗口的位置和大小*/
TAPI_BOOL  Tapi_TsPlay_SetWindow( ST_VIDEO_WINDOW_TYPE* stVidWindowType ){
	char buf[32];
	AM_ErrorCode_t rc;

	if(!stVidWindowType)
	{
		LOGE("%s, null poiter\n", __FUNCTION__);
		return TAPI_FAILURE;
	}
	if(AM_FileRead(VID_RESOLUTION_FILE, buf, sizeof(buf))==AM_SUCCESS)
	{
		if(!strncmp(buf, "1366x768", 8))
		{
			stVidWindowType->x = stVidWindowType->x * 1366 / (1920 - 1);
			stVidWindowType->y = stVidWindowType->y * 768 / (1080 - 1);
			stVidWindowType->width = stVidWindowType->width * 1366 / (1920 - 1);
			stVidWindowType->hight = stVidWindowType->hight * 768 / (1080 - 1);
		}
	}
	LOGD("%s, x,y,w,h: %d, %d, %d, %d", __FUNCTION__, stVidWindowType->x, stVidWindowType->y,
																						stVidWindowType->width, stVidWindowType->hight);
	rc = AM_AV_SetVideoWindow(AV_DEV_NO,stVidWindowType->x,stVidWindowType->y,
															stVidWindowType->width,stVidWindowType->hight);
	if (rc < 0)
	{
		return TAPI_FAILURE;
	}
	return TAPI_SUCCESS;
}

/*获取视频小窗口的位置和大小*/
TAPI_BOOL  Tapi_TsPlay_GetWindow( ST_VIDEO_WINDOW_TYPE* stVidWindowType ){
	AM_ErrorCode_t rc;

	if(!stVidWindowType)
	{
		LOGE("%s, null poiter\n", __FUNCTION__);
		return TAPI_FAILURE;
	}
	rc = AM_AV_GetVideoWindow(AV_DEV_NO,(int*)&stVidWindowType->x,(int*)&stVidWindowType->y,
														(int*)&stVidWindowType->width,(int*)&stVidWindowType->hight);
	LOGD("%s, x,y,w,h: %d, %d, %d, %d", __FUNCTION__,stVidWindowType->x,stVidWindowType->y,
																					stVidWindowType->width,stVidWindowType->hight);
	if (rc < 0) {
		return TAPI_FAILURE;
	}
	return TAPI_SUCCESS;
}


