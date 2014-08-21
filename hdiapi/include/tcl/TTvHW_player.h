////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2012-2020 TCL, Inc.
// All rights reserved.
//
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _TTvHW_PLAYER_H_
#define _TTvHW_PLAYER_H_

#include "TTvHW_type.h"
#include "TTvHW_frontend_common.h"

#ifdef __cplusplus
extern "C" {
#endif

//-------------------------------------------------------------------------------------------------
/// Set autio mute.
/// @param  eAudioMuteType       IN: mute type. headphone, speaker,spdif ,chip,etc mute_on ane mute off
/// @return                      OUT: TRUE: Operation success, or FALSE: Operation failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL SetAudioMute(AUDIOMUTETYPE_ eAudioMuteType );


//-------------------------------------------------------------------------------------------------
/// get autio mute flag.
/// @param  eAudioMuteType       IN: audio  mute PATH. headphone, speaker,spdif,chip ,etc
/// @return                      OUT: TRUE: mute, or FALSE: unmute.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL GetAudioMute(AudioPath_ eAudioMutePath);


//-------------------------------------------------------------------------------------------------
/// Set screen mute.
/// @param  bScreenMute          IN: mute or unmute screen.
/// @param  enColor              IN: mute screen color.
/// @param  u16ScreenUnMuteTime  IN: screen unmute time.
/// @return                      OUT: TRUE: Operation success, or FALSE: Operation failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL SetScreenMute(TAPI_BOOL bScreenMute ,TAPI_VIDEO_Screen_Mute_Color enColor ,TAPI_U16 u16ScreenUnMuteTime);



//------------------------------------------------------------------------------
/// Set window and aspec ration,include overscan setting
///
/// @param ptDispWin             IN: the setting of display window
/// @param ARCType             IN: the aspect ration type
/// @return None
//------------------------------------------------------------------------------

TAPI_BOOL SetWindow(VIDEO_WINDOW_TYPE *ptCropWin, VIDEO_WINDOW_TYPE *ptDispWin, const TAPI_VIDEO_ARC_Type ARCType);
TAPI_BOOL Tapi_Dtv_Startup(void);
//------------------------------------------------------------------------------
/// freeze_change channels
///
/// @param bfreeze             0:freeze  change channel 1:blue change channel
/// @return TAPI_FALSE/TAPI_TRUE
//------------------------------------------------------------------------------
TAPI_BOOL Tapi_Dtv_freeze_changingCH(TAPI_BOOL bfreeze);

#ifdef __cplusplus
}
#endif


#endif//_TTvHW_PLAYER_H_

