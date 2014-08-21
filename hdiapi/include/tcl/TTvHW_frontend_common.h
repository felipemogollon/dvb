////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2012-2020 TCL, Inc.
// All rights reserved.
//
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _TTVHW_PROXY_FRONTEND_COMMON_H_
#define _TTVHW_PROXY_FRONTEND_COMMON_H_

#include "TTvHW_type.h"

/// the RF band
typedef enum
{
    /// VHF low
    E_RFBAND_VHF_LOW,
    /// VHF high
    E_RFBAND_VHF_HIGH,
    /// UHF
    E_RFBAND_UHF,
    /// invalid
    E_RFBAND_INVALID,
} RFBAND;

/// the RF channel bandwidth
typedef enum
{
    /// 6 MHz
    E_RF_CH_BAND_6MHz = 0x01,
    /// 7 MHz
    E_RF_CH_BAND_7MHz = 0x02,
    /// 8 MHz
    E_RF_CH_BAND_8MHz = 0x03,
    /// invalid
    E_RF_CH_BAND_INVALID
} RF_CHANNEL_BANDWIDTH;

/// the tuner mode
typedef enum
{
    /// DTV, DVBT
    E_TUNER_DTV_DVB_T_MODE = 0x00,
    /// DTV, DVBC
    E_TUNER_DTV_DVB_C_MODE ,
    /// DTV, DVBS
    E_TUNER_DTV_DVB_S_MODE ,
    /// DTV, DTMB
    E_TUNER_DTV_DTMB_MODE ,
    /// DTV, ATSC
    E_TUNER_DTV_ATSC_MODE ,
    /// ATV, PAL
    E_TUNER_ATV_PAL_MODE ,
    /// ATV, PAL AUTO
    E_TUNER_ATV_PAL_AUTOSCAN_MODE ,
    /// ATV, SECAM-L'
    E_TUNER_ATV_SECAM_L_PRIME_MODE ,
    /// ATV, SECAM-L' AUTO
    E_TUNER_ATV_SECAM_L_PRIME_AUTOSCAN_MODE ,
    /// ATV, NTSC
    E_TUNER_ATV_NTSC_MODE ,
    /// DTV, ISDB
    E_TUNER_DTV_ISDB_MODE ,
    /// invalid
    E_TUNER_INVALID
} EN_TUNER_MODE;


/// the demodulator type
typedef enum
{
    E_DEVICE_DEMOD_ATV = 0,
    E_DEVICE_DEMOD_DVB_T,
    E_DEVICE_DEMOD_DVB_C,
    E_DEVICE_DEMOD_DVB_S,
    E_DEVICE_DEMOD_DTMB,
    E_DEVICE_DEMOD_ATSC,
    E_DEVICE_DEMOD_ATSC_VSB,
    E_DEVICE_DEMOD_ATSC_QPSK,
    E_DEVICE_DEMOD_ATSC_16QAM,
    E_DEVICE_DEMOD_ATSC_64QAM,
    E_DEVICE_DEMOD_ATSC_256QAM,
    E_DEVICE_DEMOD_DVB_T2,
    E_DEVICE_DEMOD_ISDB,
    E_DEVICE_DEMOD_MAX,
    E_DEVICE_DEMOD_NULL = E_DEVICE_DEMOD_MAX,

} EN_DEVICE_DEMOD_TYPE;

/// the signal strength
typedef enum
{
    /// no signal
    E_FE_SIGNAL_NO = 0,
    /// weak signal
    E_FE_SIGNAL_WEAK,
    /// moderate signal
    E_FE_SIGNAL_MODERATE,
    /// strong signal
    E_FE_SIGNAL_STRONG,
    /// very strong signal
    E_FE_SIGNAL_VERY_STRONG,
} EN_FRONTEND_SIGNAL_CONDITION;


/// the demod lock status
typedef enum
{
    /// lock
    E_DEMOD_LOCK,
    /// is checking
    E_DEMOD_CHECKING,
    /// after checking
    E_DEMOD_CHECKEND,
    /// unlock
    E_DEMOD_UNLOCK,
    /// NULL state
    E_DEMOD_NULL,
} EN_LOCK_STATUS;

    /// DVBC QAM Type
typedef enum
{
    E_CAB_UNDEFINE ,
    ///< QAM 16
    E_CAB_QAM16 ,
    ///< QAM 32
    E_CAB_QAM32 ,
    ///< QAM 64
    E_CAB_QAM64 ,
    ///< QAM 128
    E_CAB_QAM128 ,
    ///< QAM 256
    E_CAB_QAM256 ,
    ///< Invalid value
    E_CAB_INVALID
} EN_CAB_CONSTEL_TYPE;

  typedef enum
{
    ///unsupported codec type
    CODEC_TYPE_NONE = 0,
    ///MPEG 1/2
    CODEC_TYPE_MPEG2,
    ///H264
    CODEC_TYPE_H264,
    ///AVS
    CODEC_TYPE_AVS,
    ///VC1
    CODEC_TYPE_VC1,
    ///For MPEG 1/2 IFrame
    CODEC_TYPE_MPEG2_IFRAME,
    ///For H264 IFrame
    CODEC_TYPE_H264_IFRAME
} CODEC_TYPE;

typedef enum{
	E_FORBIDDEN = 0,
	E_ASP_1TO1,       //    1 : 1
	E_ASP_4TO3,       //    4 : 3
	E_ASP_16TO9,      //   16 : 9
	E_ASP_221TO100,   // 2.21 : 1
	E_ASP_MAXNUM,
} EN_ASPECT_RATIO_CODE;

/* Select audio output mode type */
typedef enum
{
    TAPI_AUD_DVB_SOUNDMODE_STEREO_,
    TAPI_AUD_DVB_SOUNDMODE_LEFT_,
    TAPI_AUD_DVB_SOUNDMODE_RIGHT_,
    TAPI_AUD_DVB_SOUNDMODE_MIXED_,
} En_DVB_soundModeType_;


/* Audio DSP system type */
typedef enum
{
	TAPI_DVB_SCRAMBLE_STATE_DESCRAMBLED,           ///descrambled data
	TAPI_DVB_SCRAMBLE_STATE_SCRAMBLED,       ///scrambled data
	TAPI_DVB_SCRAMBLE_STATE_INVALID          ///invalid data
} En_DVB_SCRAMBLE_STATE;


/* Audio DSP system type */
typedef enum
{
    E_AUDIO_DSP_MPEG_,
    E_AUDIO_DSP_AC3_,
    E_AUDIO_DSP_AC3_AD_,
    E_AUDIO_DSP_SIF_,
    E_AUDIO_DSP_AAC_,
    E_AUDIO_DSP_AACP_,
    E_AUDIO_DSP_MPEG_AD_,
    E_AUDIO_DSP_AC3P_,
    E_AUDIO_DSP_AC3P_AD_,
    E_AUDIO_DSP_AACP_AD_,
    E_AUDIO_DSP_DTS_,
    E_AUDIO_DSP_MS10_DDT_,
    E_AUDIO_DSP_MS10_DDC_,
    E_AUDIO_DSP_RESERVE6_,
    E_AUDIO_DSP_RESERVE5_,
    E_AUDIO_DSP_RESERVE4_,
    E_AUDIO_DSP_RESERVE3_,
    E_AUDIO_DSP_RESERVE2_,
    E_AUDIO_DSP_RESERVE1_,
    E_AUDIO_DSP_MAX_,
    E_AUDIO_DSP_INVALID_                    = 0xFF
} AUDIO_DSP_SYSTEM_;

    /// decoder event enumerator.
    typedef enum
    {
        /// display information is changed
        E_TAPI_VDEC_EVENT_DISP_INFO_CHG      = 0x00,
         /// decode one frame
        E_TAPI_VDEC_EVENT_DEC_ONE            =  0x01,

         E_TAPI_VDEC_EVENT_MAX

    } EN_VDEC_EVENT_FLAG;

    typedef enum
{
    /// Default
    E_AR_DEFAULT = 0,   // 0
    /// 16x9
    E_AR_16x9,          // 1
    /// 4x3
    E_AR_4x3,           // 2
    /// Auto
    E_AR_AUTO,          // 3
    /// Panorama
    E_AR_Panorama,      // 4
    /// Just Scan
    E_AR_JustScan,      // 5
    /// Zoom 1
    E_AR_Zoom1,         // 6
    /// Zoom 2
    E_AR_Zoom2,         // 7
    /// subtile
    E_AR_Subtitle,      // 8
    /// Dot by Dot
    E_AR_DotByDot,      // 9
    /// Maximum value of this enum
    E_AR_MAX,

}TAPI_VIDEO_ARC_Type;

/// the display color when screen mute
typedef enum
{
    E_SCREEN_MUTE_OFF=0,
    /// black
    E_SCREEN_MUTE_BLACK,
    /// white
    E_SCREEN_MUTE_WHITE,
    /// red
    E_SCREEN_MUTE_RED,
    /// blue
    E_SCREEN_MUTE_BLUE,
    /// green
    E_SCREEN_MUTE_GREEN,
    /// yellow
    E_SCREEN_MUTE_YELLOW,
    /// gray
    E_SCREEN_MUTE_GRAY,
    /// pink
    E_SCREEN_MUTE_PINK,
    /// counts of this enum
    E_SCREEN_MUTE_NUMBER,
} TAPI_VIDEO_Screen_Mute_Color;

typedef enum
{
    ///< Mute_OFF for IR key
    E_AUDIO_BYUSER_MUTEOFF_                 = 0x0,
    ///< Mute_OFF for IR key
    E_AUDIO_BYUSER_MUTEON_                  = 0x1,
    ///< Mute_OFF for DTV Chip
    E_AUDIO_BYVCHIP_MUTEOFF_                = 0x2,
    ///< Mute_ON for DTV Chip
    E_AUDIO_BYVCHIP_MUTEON_                 = 0x3,

///< Not use currently, for speaker only
    E_AUDIO_USER_SPEAKER_MUTEOFF_           = 0x4,
    E_AUDIO_USER_SPEAKER_MUTEON_            = 0x5,
///< Not use currently, for headphone olny
    E_AUDIO_USER_HP_MUTEOFF_                = 0x6,
    E_AUDIO_USER_HP_MUTEON_                 = 0x7,
///< Not use currently, for spdif olny
    E_AUDIO_USER_SPDIF_MUTEOFF_             = 0x8,
    E_AUDIO_USER_SPDIF_MUTEON_              = 0x9,

} AUDIOMUTETYPE_;

typedef enum
{
    E_AUDIO_BYUSER                 = 0x0,
    E_AUDIO_BYVCHIP                = 0x1,
    E_AUDIO_USER_SPEAKER           = 0x2,
    E_AUDIO_USER_HP                = 0x3,
    E_AUDIO_USER_SPDIF             = 0x4,
} AudioPath_;

typedef struct
{
    TAPI_U16 x;         ///<start x of the window
    TAPI_U16 y;         ///<start y of the window
    TAPI_U16 width;     ///<width of the window
    TAPI_U16 height;    ///<height of the window
} VIDEO_WINDOW_TYPE;

#endif

