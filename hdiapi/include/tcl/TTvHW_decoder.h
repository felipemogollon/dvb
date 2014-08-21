////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2012-2020 TCL, Inc.
// All rights reserved.
//
//
////////////////////////////////////////////////////////////////////////////////
#ifndef _TTVHW_DECODER_H_
#define _TTVHW_DECODER_H_

#include "TTvHW_type.h"
#include "TTvHW_frontend_common.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
    /// None
    EN_3D_NONE,
    /// side by side half
    EN_3D_SIDE_BY_SIDE_HALF,
    /// top bottom
    EN_3D_TOP_BOTTOM,
    /// frame packing
    EN_3D_FRAME_PACKING,
    /// line alternative
    EN_3D_LINE_ALTERNATIVE,
    /// frame alernative
    EN_3D_FRAME_ALTERNATIVE,
    /// field alernative
    EN_3D_FIELD_ALTERNATIVE,
    /// check board
    EN_3D_CHECK_BORAD,
    /// 2DTo3D
    EN_3D_2DTO3D,
    /// 3D Dual view
    EN_3D_DUALVIEW,
    /// pixel alternative
    EN_3D_PIXEL_ALTERNATIVE,
    /// format number
    EN_3D_TYPE_NUM
}SRC_3D_TYPE_T;

typedef struct _VDEC_INFO
{
    TAPI_U32 u32FrameRate;
    TAPI_U16 u16Width;
    TAPI_U16 u16Height;
    TAPI_BOOL b_IsHD;  //HD --->TAPI_TRUE; SD--->TAPI_FALSE
    EN_ASPECT_RATIO_CODE  enAspectRate;
    TAPI_U8  u8Interlace; // 1---> I,  2 -->P
    SRC_3D_TYPE_T en_3D_type;//  //3D type
}VDEC_DISP_INFO;

typedef struct
{
    TAPI_U32 u4SrcWidth;
    TAPI_U32 u4SrcHeight;
    TAPI_U32 u4OutWidth;
    TAPI_U32 u4OutHeight;
    TAPI_U32 u4FrameRate;
    TAPI_U32 u4Interlace;
} MTVDO_SRC_INFO_T;

typedef enum
{
    ///failed
    E_VDEC_FAIL = 0,
    ///success
    E_VDEC_OK,
    ///access not allow
    E_VDEC_ILLEGAL_ACCESS,
    ///hardware abnormal
    E_VDEC_HARDWARE_BREAKDOWN,
     ///unsupported
    E_VDEC_UNSUPPORTED,
     ///timeout
    E_VDEC_TIMEOUT,
    ///not ready
    E_VDEC_NOT_READY,
    ///not initial
    E_VDEC_NOT_INIT,
} VDEC_STATUS;

/**Video decoder status.
* fgLock = TRUE, when the 1st picture decode ready.
* scramble channel  =>  fgLock = TAPI_FALSE
* non scramble =>  fgLock = TAPI_TRUE
* u4DecOkNs <= u4ReceiveNs
* When signal is weak, u4DecOkNs < u4ReceiveNs.
*   TAPI_BOOL  b_DisplayStatus;Frame dipslaying normally or not  normally => TAPI_TRUE ,else ==>TAPI_FALSE
*  TAPI_BOOL   b_DecodeError;  Frame decode normally or not  normally => TAPI_TRUE ,else ==>TAPI_FALSE

*/
typedef struct
{
    TAPI_BOOL b_IsLock;                        ///<video decoder is lock.
    TAPI_BOOL b_IsDisplaying;                  ///<video start displaying
    TAPI_U32   u4ReceiveCnt;                     ///<picture count received by video decoder.
    TAPI_U32   u4DecOkCnt;                       ///<picture count decoded ok by video decoder.
    TAPI_BOOL  b_DisplayStatus;               ///<frame dipslaying normally or not
    VDEC_STATUS u4Status;                         ///<current decode status,
} VDEC_STATUS_INFO_T;

typedef void (*VDEC_EventCb)(TAPI_U32 statu, void* udata);

//-------------------------------------------------------------------------------------------------
/// To init vdec device
/// @param  None
///
/// @return                 TRUE: success, FALSE: failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Vdec_Init(void);


//-------------------------------------------------------------------------------------------------
/// To finalize the vdec.
/// @param	None
/// @return 			   \b TRUE: success, FALSE: failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Vdec_Finalize(void);
//-------------------------------------------------------------------------------------------------
/// To get Decoder status.
/// @param                \b out: Vdec STATUS.
/// @return 			  \b TRUE: success, FALSE: failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Vdec_GetDecoderStatus(VDEC_STATUS_INFO_T*  );
//-------------------------------------------------------------------------------------------------
/// To Tapi_AV_start
/// @param                \b out: Vdec STATUS.
/// @return               \b TRUE: success, FALSE: failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_AV_start(void);
//-------------------------------------------------------------------------------------------------
/// To Tapi_AV_stop.
/// @param                \b out: Vdec STATUS.
/// @return               \b TRUE: success, FALSE: failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_AV_stop(void);
/// To register callback function and set user data for callback.
/// @param  vdec_event      \b IN: the vdec events want to be registerred.(refer to EN_VDEC_EVENT_FLAG)
/// @param  VDEC_EventCb      \b IN: function pointer of the callback function.
/// @return                \b TRUE: success, FALSE: failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Vdec_SetCallBack(TAPI_U32 evt, VDEC_EventCb callback, void* data);


//-------------------------------------------------------------------------------------------------
/// To Unregister callback function .
/// @param  vdec_event      \b IN: the vdec events want to be unregisterred.(refer to EN_VDEC_EVENT_FLAG)
/// @return                \b TRUE: success, FALSE: failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Vdec_UnSetCallBack(TAPI_U32 evt);


//-------------------------------------------------------------------------------------------------
/// To set vdec type and init vdec driver.(DTV)
/// @param  enType       IN: vdec type.
///
/// @return                 TRUE: success, FALSE: failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Vdec_SetType(CODEC_TYPE enType);

//lee
/// To set Auido pid to  av.
/// @param               None
/// @return                \b TRUE: success, FALSE: failure.
TAPI_BOOL Tapi_Adec_SetAudioPid(TAPI_U32 v_pid);

/// To set video pid to  av.
/// @param               None
/// @return                \b TRUE: success, FALSE: failure.
TAPI_BOOL Tapi_Vdec_SetVideoPid(TAPI_U32 v_pid);
//-------------------------------------------------------------------------------------------------
/// To set stop command of the vdec.
/// @param               None
/// @return                TRUE: success, FALSE: failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Vdec_Stop(void);

//-------------------------------------------------------------------------------------------------
/// To set play command of the vdec.
/// @param               None
/// @return                \b TRUE: success, FALSE: failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Vdec_Play(void);

//-------------------------------------------------------------------------------------------------
/// To get active vdec type of the vdec. (DTV)
/// @param  enType       OUT: Return acitve vdec type.
/// @return                 TRUE: success, FALSE: failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Vdec_GetActiveVdecType(CODEC_TYPE *enType);

//-------------------------------------------------------------------------------------------------
/// To set AV sync on/off of the vdec.
/// @param  bEnable     IN: enable AV sync or not. FALSE(off) / TRUE(on)
/// @return                 TRUE: success, FALSE: failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Vdec_SyncOn(TAPI_BOOL bEnable);

//-------------------------------------------------------------------------------------------------
/// To check if TS and STC is closed enough when enable AV sync
/// @param               None
/// @return                TRUE: success, FALSE: failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Vdec_IsSyncDone(void);

//-------------------------------------------------------------------------------------------------
/// To get display information of the vdec.
/// @param  pstInitPara       OUT: vdec display information.
/// @return                 TRUE: success, FALSE: failure.
//-------------------------------------------------------------------------------------------------
//TAPI_BOOL GetDispInfo(VDEC_DISP_INFO *pstDispInfo);
TAPI_BOOL Tapi_Vdec_GetDispInfo(VDEC_DISP_INFO *pstDispInfo);

//-------------------------------------------------------------------------------------------------
/// @ Function	Description: get Vedio Scramble state
/// @param	  out : scramble state :scramble or  Discramble
/// @return 		success TAPI_TRUE /faile TAPI_FALSE
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Vdec_GetScrambleState(TAPI_U8 u1Pidx, En_DVB_SCRAMBLE_STATE* peScramble_State);

    //-------------------------------------------------------------------------------------------------
    /// To init  Adec device.
    /// @param               None
    /// @return                \b TRUE: success, FALSE: failure.
    //-------------------------------------------------------------------------------------------------
    TAPI_BOOL Tapi_Adec_Init(void);




	//-------------------------------------------------------------------------------------------------
	/// To finalize the adec.
	/// @param	None
	/// @return 			   \b TRUE: success, FALSE: failure.
	//-------------------------------------------------------------------------------------------------
	TAPI_BOOL Tapi_Adec_Finalize(void);

	//-------------------------------------------------------------------------------------------------
	/// @brief \b Function \b Name: SetAbsoluteVolume()
	/// @brief \b Function \b Description:
	/// @param <IN> 	   \b uPercent: (0~100 percentage)
	/// 						   \b uAudioPath:
	/// @param <OUT>	   \b NONE	  :
	/// @param <GLOBAL>    \b NONE	  :
	//-------------------------------------------------------------------------------------------------
	void Tapi_Adec_SetAbsoluteVolume( TAPI_U8 uPercent, AudioPath_ uAudioPath);



    //-------------------------------------------------------------------------------------------------
    /// To set  Adec type.
    /// @param               None
    /// @return                \b TRUE: success, FALSE: failure.
    //-------------------------------------------------------------------------------------------------
    TAPI_BOOL Tapi_Adec_SetType(AUDIO_DSP_SYSTEM_  eAudioDSPSystem);

    //-------------------------------------------------------------------------------------------------
/// To set stop command of the Adec.
/// @param               None
/// @return                \b TRUE: success, FALSE: failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Adec_Stop(void);

//-------------------------------------------------------------------------------------------------
/// To set play command of the Adec.
/// @param               None
/// @return                TRUE: success, FALSE: failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Adec_Play(void);
//-------------------------------------------------------------------------------------------------
/// To set decoder av-sync
/// @param  bEnable     IN: enable AV sync or not. FALSE(off) / TRUE(on)
/// @return                 TRUE: success, FALSE: failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Adec_AVSync(void);

//-------------------------------------------------------------------------------------------------
/// To set decoder av-sync
/// @param  bEnable     \b IN: enable AV sync or not. FALSE(off) / TRUE(on)
/// @return                \b TRUE: success, FALSE: failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Adec_FreeRun(void);

//-------------------------------------------------------------------------------------------------
/// @ Function  Description: Set decoder output to Stereo/LL/RR/Mixed LR
/// @param    En_DVB_soundModeType_ <IN>: Stereo - MSAPI_AUD_DVB_SOUNDMODE_STEREO_,
///                                             : LL     - MSAPI_AUD_DVB_SOUNDMODE_LEFT_,
///                                             : RR     - MSAPI_AUD_DVB_SOUNDMODE_RIGHT_,
///                                             : Mixed  - MSAPI_AUD_DVB_SOUNDMODE_MIXED_,
/// @return         NONE
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Adec_SetOutputMode(En_DVB_soundModeType_ mode);

//-------------------------------------------------------------------------------------------------
/// @ Function  Description: get audio Scramble state
/// @param    out : scramble state :scramble or  Discramble
/// @return         success TAPI_TRUE /faile TAPI_FALSE
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Adec_GetScrambleState(TAPI_U8 u1Pidx, En_DVB_SCRAMBLE_STATE* peScramble_State);

//-------------------------------------------------------------------------------------------------
/// @ Function  Description: Clear Video Buffer after stop ts,use before seartch channel
/// @return         success TAPI_TRUE /faile TAPI_FALSE
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Dtv_ClearVideoBuffer(void);

#ifdef __cplusplus
}
#endif

#endif

