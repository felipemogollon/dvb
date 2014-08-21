#ifndef _TTvHW_DEMUX__H_
#define _TTvHW_DEMUX__H_

#include "TTvHW_type.h"

typedef enum
{
    DMX_FILTER_TYPE_VIDEO=0,
	DMX_FILTER_TYPE_AUDIO,
	DMX_FILTER_TYPE_AUDIO2,
	DMX_FILTER_TYPE_PCR,
	DMX_FILTER_TYPE_SECTION
}DMX_FILTER_TYPE;

/// PID types
typedef enum
{
    DMX_PID_TYPE_NONE = 0,      ///< None
    DMX_PID_TYPE_PSI,           ///< PSI
    DMX_PID_TYPE_PES,           ///< PES
    DMX_PID_TYPE_PES_TIME,      ///< PES with time information
    DMX_PID_TYPE_ES_VIDEO,      ///< Video ES
    DMX_PID_TYPE_ES_AUDIO,      ///< Audio ES
    DMX_PID_TYPE_ES_VIDEOCLIP,  ///< Video clip ES
    DMX_PID_TYPE_ES_OTHER,       ///< Other ES
    DMX_PID_TYPE_ES_DTCP,
    DMX_PID_TYPE_TS_RAW
} DMX_PID_TYPE_T;

typedef enum
{
    DMX_NOTIFY_CODE_PSI,                ///< PSI notification
    DMX_NOTIFY_CODE_ES,                 ///< ES notification
    DMX_NOTIFY_CODE_PES,                ///< PES notification
    DMX_NOTIFY_CODE_PES_TIME,           ///< PES notification with time information
    DMX_NOTIFY_CODE_SCRAMBLE_STATE,     ///< Scramble state change notification
    DMX_NOTIFY_CODE_OVERFLOW,           ///< Overflow notification
    DMX_NOTIFY_CODE_STREAM_ID,          ///< Report pre-parsed Stream IDs
    DMX_NOTIFY_CODE_RAW_TS              ///< Rwa TS notification
} DMX_NOTIFY_CODE_T;

typedef enum
{
   MW_OK                    = 0,   ///< Success
   MW_NOT_OK                = 1,   ///< Not ok
   MW_ERR_TIMEDOUT          = 2,    ///< Timeout occured
}MW_RESULT_T;

typedef enum 
{
	DMX_FILTER_STATUS_DATA_READY=1,
	DMX_FILTER_STATUS_DATA_OVER_FLOW=2,
	DMX_FILTER_STATUS_ERROR=3,
	DMX_FILTER_STATUS_DATA_NOT_READY=4,
	DMX_FILTER_STATUS_STATUS_TIMEOUT=5
}DMX_FILTER_STATUS;

#define SECTION_FILTER_NUM     32//64 //amlogic SOC only 32 filter
#define BITMAP_CNT         	   (SECTION_FILTER_NUM+31)/32
typedef struct
{
     TAPI_U32  u4SecAddr;
	 TAPI_U32  u4SecLen;
	 TAPI_U8   u1SerialNumber;
	 TAPI_BOOL fgUpdateData;
	 TAPI_U32  au4MatchBitmap[BITMAP_CNT];
}DMX_NOTIFY_INFO_PSI_T;

//////////////////////////////////
//Description:callback of setfilter
//@u1Pidx:index of filter
//@status:filter status
//@ecode:MTDMX_NOTIFY_CODE_T.kind of notify
//@u4Data:address of data ,we can get data from this address
//@infomation plus , not use?

typedef MW_RESULT_T (*CallBack)(TAPI_S8 u1Pidx,DMX_FILTER_STATUS status,TAPI_S32 u4Data);

/// PID data structure
typedef struct
{
  //  TAPI_BOOL fgEnable;                  ///< Enable or disable
  //  TAPI_BOOL fgAllocateBuffer;          ///< Allocate buffer
    TAPI_S16 u2Pid;                   ///< PID //pmt id
  //  UINT32 u4BufAddr;               ///< Buffer address, kernel address
  //  UINT32 u4BufSize;               ///< Buffer size
    DMX_PID_TYPE_T ePidType;      ///< PID type //psi,si,..
    CallBack pfnNotify;     ///< Callback function
    void* pvNotifyTag;              ///< Tag value of callback function
  //  PFN_MTDMX_NOTIFY pfnScramble;   ///< Callback function of scramble state
   // void* pvScrambleTag;            ///< Tag value of scramble callback function
   // UINT8 u1DeviceId;               ///< Decoder ID
   // UINT8 u1ChannelId;              ///< Channel ID
   // UINT8 u1TsIndex;                ///< TS index
   // TAPI_BOOL fgDisableOutput;           ///< No output
    TAPI_BOOL fgCheckCrc;                ///< Check CRC or not
    //TAPI_S8 u1Pidx;                   ///< PID index, 0 ~ 31 //for writer
    TAPI_S8 u1FilterIndex;     ///< filter index 
    TAPI_S8 u1Offset;                 ///< Offset
    TAPI_S8 au1Data[16];              ///< Pattern
    TAPI_S8 au1Mask[16];              ///< Mask
    TAPI_S32 timeOut;                 //ms   //if timeout,then callback

} DMX_FILTER_PARAM;



#ifdef __cplusplus 
extern "C" {
#endif


/////////////////////////////////////////////////////////////////
//@Fuction name:Tapi_Demux_Init
//@Description:init demux,then we can set filter
//@param <IN> :void
//@param <RET>
// @ --TRUE:sucess
// @ --FALSE:fail
TAPI_BOOL Tapi_Demux_Init(void);

/////////////////////////////////////////////////////////////////
//@Fuction name:TAPI_Demux_Exit
//@Description:free demux
//@param <IN> :void
//@param <RET>
//@  --TRUE:sucess
// @ --FALSE:fail
TAPI_BOOL Tapi_Demux_Exit(void);

//////////////////////////////////////////////////////////////////////////
//@Fuction name:Tapi_Demux_AllocateFilter
//@Description:allocate a new filter
//@param <IN>
//@ ---type:DMX_FILTER_TYPE
//@ ---index:the index of filter assigned by writer
//@ ---outindex:the index of filter assigned by vendor,if not requred,let it same as index
//@---u16id: pid
//@param <RET>
//@--TRUE:sucess
//@--FALSE:fail
TAPI_BOOL Tapi_Demux_AllocateFilter(DMX_FILTER_TYPE type,TAPI_S16 u16id,TAPI_S8 index,TAPI_S8* outindex);

///////////////////////////////////////////
//@Fuction name:Tapi_Demux_SetSectionFilterParam
//@Description:set param to a section filter,it will be started after call fuction demux_start
//@param <IN>
//@ ---param:as to MTDMX_FILTER_PARAM
//@param <RET>
//@--TRUE:sucess
//@--FALSE:fail
TAPI_BOOL Tapi_Demux_SetSectionFilterParam(TAPI_S8 index,DMX_FILTER_PARAM param);



//////////////////////////////////////////////////////////
//@Fuction name :Tapi_Demux_Start
//@Description:start a filter after set param,
//@param <IN>
//@--index : same as outindex in the function demux_open(DMX_FILTER_TYPE type,U8 index,U8* outindex)
//@param <RET>
//@--TRUE:sucess
//@--FALSE:fail
TAPI_BOOL Tapi_Demux_Start(TAPI_S8 index);

/////////////////////////////////////
//@Fuction name : Tapi_Demux_Stop(U8 index)
//@Description:stop a filter after set param,
//@param <IN>
//@--index : same as outindex in the function demux_open(DMX_FILTER_TYPE type,U8 index,U8* outindex)
//@param <RET>
//@--TRUE:sucess
//@--FALSE:fail
TAPI_BOOL Tapi_Demux_Stop(TAPI_S8 index);


/////////////////////////////////////
//@Fuction name : Tapi_Demux_Reset(U8 index)
//@Description:reset a filter after set param and start
//@param <IN>
//--index : same as outindex in the function demux_open(DMX_FILTER_TYPE type,U8 index,U8* outindex)
//@param <RET>
//@--TRUE:sucess
//@--FALSE:fail
TAPI_BOOL Tapi_Demux_Reset(TAPI_S8 index);

//////////////////////////////////////////////////
//@Fuction name : Tapi_Demux_Restart(U8 index)
//@Description:restart a filter
//@param <IN>
//@--index : same as outindex in the function demux_open(DMX_FILTER_TYPE type,U8 index,U8* outindex)
//@param <RET>
//@--TRUE:sucess
//@--FALSE:fail
TAPI_BOOL Tapi_Demux_Restart(TAPI_S8 index);

//////////////////////////////////////////////////
//@Fuction name : Tapi_Demux_Close(U8 index)
//@Description:close filter
//@param <IN>
//@--index : same as outindex in the function demux_open(DMX_FILTER_TYPE type,U8 index,U8* outindex)
//@param <RET>
//@--TRUE:sucess
//@--FALSE:fail
TAPI_BOOL Tapi_Demux_Close(TAPI_S8 index);

TAPI_BOOL Tapi_Demux_FlushSectionBuffer(TAPI_S8 index);


////////////////////////////////////////////
//@Fuction name: Tapi_Demux_GetFilterStatus(TAPI_S8 index)
//@Description:get the filter status
////@param <IN>
//@--index : same as outindex in the function demux_open(DMX_FILTER_TYPE type,U8 index,U8* outindex)
//@param <RET>
//@---DMX_FILTER_STATUS

DMX_FILTER_STATUS Tapi_Demux_GetFilterStatus(TAPI_S8 index);

//////////////////////////////////////////////////
//@Fuction name : Tapi_Demux_IsFilterTimeOut(U8 index)
//@Description: is filter out?
//@param <IN>
//@--index : same as outindex in the function demux_open(DMX_FILTER_TYPE type,U8 index,U8* outindex)
//@param <RET>
//@--TRUE:time out
//@--FALSE:time not out
TAPI_BOOL Tapi_Demux_IsFilterTimeOut(TAPI_S8 index);

///////////////////////////////////////////////////////////////////
//Fuction name :  Tapi_Demux_SetPID
//Description : set Video,Audio pid to demux,then call start,play automatically
//@param <IN>
//@--u16id : pid of audio,video
//@param <RET>
//@--TRUE:success
//@--FALSE:fail


TAPI_BOOL Tapi_Demux_GetData(TAPI_U8 index,TAPI_U8 serialnum,TAPI_U32 addr,
	TAPI_U32 len, TAPI_BOOL fgUpdateData, TAPI_U8* buf );



/////**************************************************************//////////
/*******************PID FILLTER***************************************/

//allocate a pid filter 
TAPI_BOOL Tapi_Demux_AllocatePidFilter(DMX_FILTER_TYPE type,TAPI_S8 index,TAPI_S8* outindex);


//set pid to filter
TAPI_BOOL Tapi_Demux_SetPID(TAPI_S8 index,TAPI_S16 u16id);



TAPI_BOOL Tapi_PidFilter_Start(TAPI_S8 index);

TAPI_BOOL Tapi_PidFilter_Stop(TAPI_S8 index);

TAPI_BOOL Tapi_PidFilter_Reset(TAPI_S8 index);

TAPI_BOOL Tapi_PidFilter_Restart(TAPI_S8 index);

TAPI_BOOL Tapi_PidFilter_Close(TAPI_S8 index);

#ifdef __cplusplus 
}
#endif

#endif


