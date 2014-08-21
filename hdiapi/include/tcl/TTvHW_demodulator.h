////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2012-2020 TCL, Inc.
// All rights reserved.
//
//
////////////////////////////////////////////////////////////////////////////////
#ifndef _TTVHW_DEMODULATOR_H_
#define _TTVHW_DEMODULATOR_H_

#include "TTvHW_type.h"
#include "TTvHW_tuner.h"
#include "TTvHW_frontend_common.h"

typedef struct
{
    TAPI_U32 u32Frequency;         ///<Frequency
    RF_CHANNEL_BANDWIDTH eBandWidth;         ///<BandWidth
    TAPI_U32 uSymRate;     ///<Symbol rate.
    EN_CAB_CONSTEL_TYPE eQAM;    ///<QAM type
    TAPI_U32 u32State; /// u32State 0:SCAN 1  /u32State 1:play
} DEMOD_SETTING;

typedef struct
{
    TAPI_U16 SignalQuality;         ///<Signal Quality
    TAPI_U16 SignalStrength;         ///<Signal Strength
} SIGNAL_STATUS;



#ifdef __cplusplus 
extern "C" {
#endif

//-------------------------------------------------------------------------------------------------
/// Connect to demod & set demod type.
/// @param  enDemodType  IN: Demod type.
/// @return                  TRUE: Connecting and setting success, FALSE: Connecting failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Demod_Connect(EN_DEVICE_DEMOD_TYPE enDemodType);

//-------------------------------------------------------------------------------------------------
/// Disconnect from demod.
/// @param  None
/// @return                  TRUE: Disconnect success, FALSE: Disconnect failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Demod_Disconnect(void);

//-------------------------------------------------------------------------------------------------
/// Reset demod status and state.
/// @param  None
/// @return  None
//-------------------------------------------------------------------------------------------------
void Tapi_Demod_Reset();

//-------------------------------------------------------------------------------------------------
/// Set demod to bypass iic ,so controller can communcate with tuner.
/// @param  enable        IN: bypass or not.
/// @return                  TRUE: Bypass setting success, FALSE: Bypass setting done failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Demod_IIC_Bypass_Mode(TAPI_BOOL enable);

//-------------------------------------------------------------------------------------------------
/// Init demod.
/// @param  None
/// @return                  TRUE: Init success, FALSE: Init failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Demod_Initialization(void);

//-------------------------------------------------------------------------------------------------
/// Set demod power on.
/// @param  None
/// @return                  TRUE: Set success, FALSE: Set failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Demod_Set_PowerOn(void);

//-------------------------------------------------------------------------------------------------
/// Set demod power off.
/// @param  None
/// @return                  TRUE: Set success, FALSE: Set failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Demod_Set_PowerOff(void);

//-------------------------------------------------------------------------------------------------
/// Get current demodulator type.
/// @param  None
/// @return  EN_DEVICE_DEMOD_TYPE      Current demodulator type.
//-------------------------------------------------------------------------------------------------
EN_DEVICE_DEMOD_TYPE Tapi_Demod_GetCurrentDemodulatorType(void);

//-------------------------------------------------------------------------------------------------
/// Get current demodulator type.
/// @param  enDemodType  IN: demodulator type
/// @return       TRUE: Set success, FALSE: Set failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Demod_SetCurrentDemodulatorType(EN_DEVICE_DEMOD_TYPE enDemodType);

//-------------------------------------------------------------------------------------------------
/// For user defined command. for extending
/// @param  SubCmd      IN: sub command.
/// @param  u32Param1      IN: command parameter1.
/// @param  u32Param2      IN: command parameter2.
/// @param  pvParam3      IN: command parameter3 point.
/// @return       TRUE: Set success, FALSE: Set failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Demod_ExtendCmd(TAPI_U8 SubCmd, TAPI_U32 u32Param1, TAPI_U32 u32Param2, void *pvParam3);

//-------------------------------------------------------------------------------------------------
/// Get signal SNR.
/// @param  None
/// @return        Signal condition.
//-------------------------------------------------------------------------------------------------
TAPI_U16 Tapi_Demod_GetSNR(void) ;

//-------------------------------------------------------------------------------------------------
/// Get signal BER.
/// @param  None
/// @return        Bit error rate.
//-------------------------------------------------------------------------------------------------
TAPI_U32 Tapi_Demod_GetBER(void)  ;

//-------------------------------------------------------------------------------------------------
///Tapi_Demod_CheckDtmbMoudle
/// @param  None
/// @return                  TRUE:
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Demod_CheckDtmbMoudle(void);

//-------------------------------------------------------------------------------------------------
/// Get Packet error of DVBT and DVBC.
/// @param  None
/// @return        Packet error.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL DTV_GetPacketErr(TAPI_U16 *pKtErr);

//-------------------------------------------------------------------------------------------------
/// Get signal status.
/// @param  None
/// @return       TRUE: Get success, FALSE: Get failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Demod_GetSignalStatus(SIGNAL_STATUS *sSignalStatus);

//-------------------------------------------------------------------------------------------------
/// Set demod to output serial or parallel.
/// @param  bEnable  IN: True : serial . False : parallel.
/// @return       TRUE: Set success, FALSE: Set failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Demod_Serial_Control( TAPI_BOOL bEnable) ;

//-------------------------------------------------------------------------------------------------
/// Config demod.(DVBC)
/// @param  sDemodSetting      IN:  demod setting ,include Frequency,BandWidth,Symbol rate, QAM type
/// @return       TRUE: Set success, FALSE: Set failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Demod_DVB_SetFrequency(DEMOD_SETTING sDemodSetting);


//-------------------------------------------------------------------------------------------------
/// get demod config.(DVBC)
/// @param  sDemodSetting     OUT:  demod setting ,include Frequency,BandWidth,Symbol rate, QAM type
/// @return       TRUE: Get success, FALSE: Get failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Demod_DVB_GetFrequency(DEMOD_SETTING *sDemodSetting);

//-------------------------------------------------------------------------------------------------
/// Set TS output enable/disable.(DVBC)
/// @param  bEnable TRUE:enable TS output, FALSE:disable TS output
/// @return  TRUE: Set success, FALSE: Set failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Demod_SetTsOutput(TAPI_BOOL bEnable);

//-------------------------------------------------------------------------------------------------
/// Get Lock Status.(DVBC)
/// @param  None
/// @return       Lock Status.
//-------------------------------------------------------------------------------------------------
EN_LOCK_STATUS Tapi_Demod_GetLockStatus(void);
  //------------------------------------------------------------------------------
 /// To get frequency offset
 /// @param None
 /// @return value of offset
 //------------------------------------------------------------------------------
TAPI_U32 Tapi_Demod_DVB_GetFrequencyOffset(void);
//------------------------------------------------------------------------------
/// To get startup type
/// @param None
/// @offsetE_DEVICE_DEMOD_DVB_C or E_DEVICE_DEMOD_DTMB
//------------------------------------------------------------------------------
EN_DEVICE_DEMOD_TYPE Tapi_Demod_Get_StartupType(void);

#ifdef __cplusplus 
}
#endif

#endif

