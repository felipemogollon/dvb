#ifndef __USB_TS_PLAY_H__
#define __USB_TS_PLAY_H__


/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 类型定义                                     *
 *----------------------------------------------*/
 
typedef  unsigned char  TAPI_BOOL;
typedef  unsigned short  TAPI_U16;
typedef  unsigned long  TAPI_U32;

#define TAPI_SUCCESS  0
#define TAPI_FAILURE  1

typedef enum DAL_VIDEO_CODEC_TYPE {
	EN_VID_CODE_TYPE_INVALID = 0,
	EN_VID_CODE_TYPE_MPEG2,			///< MPEG2
	EN_VID_CODE_TYPE_MPEG4,			///< MPEG4 DIVX4 DIVX5
	EN_VID_CODE_TYPE_AVS,			///< AVS
	EN_VID_CODE_TYPE_H263,			///< H263
	EN_VID_CODE_TYPE_H264,			///< H264
	EN_VID_CODE_TYPE_REAL8,			///< REAL8
	EN_VID_CODE_TYPE_REAL9,			///< REAL9
	EN_VID_CODE_TYPE_VC1,			///< VC1
	EN_VID_CODE_TYPE_VP6,			///< VP6
	EN_VID_CODE_TYPE_VP6F,			///< VP6F
	EN_VID_CODE_TYPE_VP6A,			///< VP6A
	EN_VID_CODE_TYPE_MJPEG,			///< MJPEG
	EN_VID_CODE_TYPE_SORENSON,		///< SORENSON SPARK
	EN_VID_CODE_TYPE_DIVX3,			///< DIVX3不支持
	EN_VID_CODE_TYPE_RAW,			///< RAW
	EN_VID_CODE_TYPE_JPEG,			///< JPEG add for Venc
	/*以下是客户新加暂时不接*/
	EN_VID_CODE_TYPE_MPEG2_IFRAME,	///< MPEG 1/2 IFrame
	EN_VID_CODE_TYPE_H264_IFRAME,	///< H264 IFrame

	EN_VID_CODE_TYPE_NUMBER
} EN_VID_CODE_TYPE;
	
typedef enum{
	EN_AUD_CODE_TYPE_INVALID = 0,
    EN_AUD_CODE_TYPE_PCM,			///< PCM
    EN_AUD_CODE_TYPE_MP2,			///< MP2
    EN_AUD_CODE_TYPE_AAC,			///< AAC
    EN_AUD_CODE_TYPE_MP3,			///< MP3
    EN_AUD_CODE_TYPE_AMRNB,			///< AMRNB
	EN_AUD_CODE_TYPE_AC3,			///< AC3
	EN_AUD_CODE_TYPE_EAC3,			///< EAC
	EN_AUD_CODE_TYPE_TRUEHD,		///< TRUEHD
	EN_AUD_CODE_TYPE_DTS,			///< DTS
	EN_AUD_CODE_TYPE_DOLBY_PLUS,	///< DOLBY PLUS
    EN_AUD_CODE_TYPE_MPEG,			///< MPEG
    /*以下是客户新加暂时不接*/
    EN_AUD_CODE_TYPE_SIF,			///< SIF
    EN_AUD_CODE_TYPE_AACP,			///< AACP
    EN_AUD_CODE_TYPE_MPEG_AD,		///< MPEG AD
	EN_AUD_CODE_TYPE_AC3_AD,		///< AC3 AD
    EN_AUD_CODE_TYPE_AC3P,			///< AC3P
    EN_AUD_CODE_TYPE_AC3P_AD,		///< AC3P AD
    EN_AUD_CODE_TYPE_AACP_AD,		///< AACP AD
    EN_AUD_CODE_TYPE_MS10_DDT,		///< MS10_DDT
    EN_AUD_CODE_TYPE_MS10_DDC,		///< MS10_DDC
	
    EN_AUD_CODE_TYPE_NUMBER 		//
}EN_AUD_CODE_TYPE;

typedef struct{
	TAPI_U32 PCR_Pid;
	TAPI_U32 Video_Pid;	
	EN_VID_CODE_TYPE Video_CodeType;
	TAPI_U32 Audio_Pid;
	EN_AUD_CODE_TYPE Audio_CodeType;
}ST_TS_PLAY_PARAM;

typedef enum{
    EN_AUD_SOUNDMODE_STEREO = 0,
    EN_AUD_SOUNDMODE_LEFT,
    EN_AUD_SOUNDMODE_RIGHT,
    EN_AUD_SOUNDMODE_MIXED
} EN_AUD_SOUNDMODE_TYPE;

typedef enum{
    EN_DOLBY_NONE = 0,			
	EN_DOLBY_AC3,  			///< AC3
	EN_DOLBY_EAC3, 			///< EAC3
	EN_DOLBY_DTS,			///< DTS
	
	EN_DOLBY_NUMBER
} EN_DOLBY_IDENTIFYIER;

typedef struct{
	TAPI_U16 x;
	TAPI_U16 y; 
	TAPI_U16 width;
	TAPI_U16 hight;
}ST_VIDEO_WINDOW_TYPE;

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

//初始化TS播放器
extern TAPI_BOOL  Tapi_TsPlay_Init( );
//销毁TS播放器
extern TAPI_BOOL  Tapi_TsPlay_Deinit( );
//设置ts播放参数
extern TAPI_BOOL  Tapi_TsPlay_SetParam( ST_TS_PLAY_PARAM* stTsPlayParam );
//往播放器推送TS数据
extern TAPI_BOOL  Tapi_TsPlay_PushData( void *data, TAPI_U32 data_len );
//播放TS
extern TAPI_BOOL  Tapi_TsPlay_Start( );
//停止播放
extern TAPI_BOOL  Tapi_TsPlay_Stop( );

/*以下暂时为空*/
extern TAPI_BOOL Tapi_TsPlay_GetDolbyIdentifier( EN_DOLBY_IDENTIFYIER* enDolbyIndent );
extern TAPI_BOOL  Tapi_TsPlay_SetSoundMode( EN_AUD_SOUNDMODE_TYPE* enAudModeType );
extern TAPI_BOOL  Tapi_TsPlay_GetSoundMode( EN_AUD_SOUNDMODE_TYPE* enAudModeType );

//停止播放时是否留最后一帧
TAPI_BOOL  Tapi_TsPlay_SetFreezingChangingChannel( TAPI_BOOL  bFreeze );

TAPI_BOOL  Tapi_TsPlay_ScreenClear();
//设置视频窗口大小及位置
extern TAPI_BOOL  Tapi_TsPlay_SetWindow( ST_VIDEO_WINDOW_TYPE* stVidWindowType );
//获得视频窗口大小及位置
extern TAPI_BOOL  Tapi_TsPlay_GetWindow( ST_VIDEO_WINDOW_TYPE* stVidWindowType );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __USB_TS_PLAY_H__ */

