////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2012-2020 TCL, Inc.
// All rights reserved.
//
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _TTVHW_TNUER_H_
#define _TTVHW_TNUER_H_

#include "TTvHW_type.h"
#include "TTvHW_frontend_common.h"


#ifdef __cplusplus 
extern "C" {
#endif

//-------------------------------------------------------------------------------------------------
/// Some Vendor Tuner need to Do Special Init flow
/// @return              OUT: MAPI_TRUE or MAPI_FALSE
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Tuner_Init(void) ;

//-------------------------------------------------------------------------------------------------
/// To connect this tuner module
/// @return              OUT: MAPI_TRUE or MAPI_FALSE
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Tuner_Connect(void) ;

//-------------------------------------------------------------------------------------------------
/// To disconnect this tuner module
/// @return              OUT: MAPI_TRUE or MAPI_FALSE
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Tuner_Disconnect(void) ;

//-------------------------------------------------------------------------------------------------
/// Set the frequency to the tuner module for ATV
/// @param u32FreqKHz     IN: Input the frequency with the unit = KHz
/// @param eBand            IN: Input the band (E_RFBAND_VHF_LOW, E_RFBAND_VHF_HIGH, or E_RFBAND_UHF)
/// @param eMode           IN: the tuner mode(TV system)
/// @param OtherMode     IN: for other audio mode
/// @return              OUT: MAPI_TRUE or MAPI_FALSE
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Tuner_ATV_SetTune(TAPI_U32 u32FreqKHz, RFBAND eBand, EN_TUNER_MODE eMode,TAPI_U8 OtherMode) ;

//-------------------------------------------------------------------------------------------------
/// Set the frequency to the tuner module for DTV
/// @param Freq          IN: Input the frequency with the unit = KHz
/// @param eBandWidth    IN: Input the BandWidth (E_RF_CH_BAND_6MHz E_RF_CH_BAND_7MHz, or E_RF_CH_BAND_8MHz)
/// @param eMode         IN: the tuner mode(TV system)
/// @return              OUT: MAPI_TRUE or MAPI_FALSE
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Tuner_DTV_SetTune(double Freq, RF_CHANNEL_BANDWIDTH eBandWidth, EN_TUNER_MODE eMode);

//-------------------------------------------------------------------------------------------------
/// Reserve extend command for customer in future.
/// @param u8SubCmd      IN: Commad defined by the customer.
/// @param u32Param1     IN: Defined by the customer.
/// @param u32Param2     IN: Defined by the customer.
/// @param pvoidParam3     IN: Defined by the customer.
/// @return              OUT: MAPI_TRUE or MAPI_FALSE
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Tuner_ExtendCommand(TAPI_U8 u8SubCmd, TAPI_U32 u32Param1, TAPI_U32 u32Param2, void* pvoidParam3) ;


#ifdef __cplusplus 
}
#endif

#endif

