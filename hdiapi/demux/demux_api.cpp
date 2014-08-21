#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <android/log.h>

#include "am_dmx.h"

#include "../com/com_api.h"
#include "demux_core.h"
#include "TTvHW_demux.h"

#define LOG_TAG "demux_api"

#if  1
#define LOGD(...) 
#define LOGE(...)
#else
#define LOGD(...) \
    if( log_cfg_get(CC_LOG_MODULE_DEMUX) & CC_LOG_CFG_DEBUG) { \
    __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__); } \

#define LOGE(...) \
    if( log_cfg_get(CC_LOG_MODULE_DEMUX) & CC_LOG_CFG_ERROR) { \
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__); } \

#endif

/////////////////////////////////////////////////////////////////
//@Fuction name:Tapi_Demux_Init
//@Description:init demux,then we can set filter
//@param <IN> :void
//@param <RET>
// @ --TRUE:sucess
// @ --FALSE:fail
TAPI_BOOL Tapi_Demux_Init(void) {
    AM_DMX_OpenPara_t dmx_para;
    LOGD(" : %d line %s calling\n", __LINE__, __FUNCTION__);
    if (!am_demux_init()) {
        return TAPI_TRUE;
    } else {
        LOGE("AM_DMX_Open failed\n");
        return TAPI_FALSE;
    }
}

/////////////////////////////////////////////////////////////////
//@Fuction name:TAPI_Demux_Exit
//@Description:free demux
//@param <IN> :void
//@param <RET>
//@  --TRUE:sucess
// @ --FALSE:fail
TAPI_BOOL Tapi_Demux_Exit(void) {
    LOGD(" : %d line %s calling\n", __LINE__, __FUNCTION__);
    return am_demux_exit() == 0 ? TAPI_TRUE : TAPI_FALSE;
}

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
TAPI_BOOL Tapi_Demux_AllocateFilter(DMX_FILTER_TYPE type, TAPI_S16 u16id, TAPI_S8 index, TAPI_S8* outindex) {
    TAPI_BOOL ret = am_demux_allocatefilter(type, u16id, index, outindex);
    LOGD(" : %d line %s calling index= %d\n", __LINE__, __FUNCTION__,* outindex);
    return ret;
}

///////////////////////////////////////////
//@Fuction name:Tapi_Demux_SetSectionFilterParam
//@Description:set param to a section filter,it will be started after call fuction demux_start
//@param <IN>
//@ ---param:as to MTDMX_FILTER_PARAM
//@param <RET>
//@--TRUE:sucess
//@--FALSE:fail
TAPI_BOOL Tapi_Demux_SetSectionFilterParam(TAPI_S8 index, DMX_FILTER_PARAM param) {
    LOGD(" : %d line %s calling index= %d\n", __LINE__, __FUNCTION__,index);
    return am_demux_setsectionfilterparam(index, param);
}

//////////////////////////////////////////////////////////
//@Fuction name :Tapi_Demux_Start
//@Description:start a filter after set param,
//@param <IN>
//@--index : same as outindex in the function demux_open(DMX_FILTER_TYPE type,U8 index,U8* outindex)
//@param <RET>
//@--TRUE:sucess
//@--FALSE:fail
TAPI_BOOL Tapi_Demux_Start(TAPI_S8 index) {
    LOGD(" : %d line %s calling index= %d\n", __LINE__, __FUNCTION__,index);
    return am_demux_start(index);
}

/////////////////////////////////////
//@Fuction name : Tapi_Demux_Stop(U8 index)
//@Description:stop a filter after set param,
//@param <IN>
//@--index : same as outindex in the function demux_open(DMX_FILTER_TYPE type,U8 index,U8* outindex)
//@param <RET>
//@--TRUE:sucess
//@--FALSE:fail
TAPI_BOOL Tapi_Demux_Stop(TAPI_S8 index) {
    LOGD(" : %d line %s calling  index= %d\n", __LINE__, __FUNCTION__, index);
    return am_demux_stop(index);
}

/////////////////////////////////////
//@Fuction name : Tapi_Demux_Reset(U8 index)
//@Description:reset a filter after set param and start
//@param <IN>
//--index : same as outindex in the function demux_open(DMX_FILTER_TYPE type,U8 index,U8* outindex)
//@param <RET>
//@--TRUE:sucess
//@--FALSE:fail
TAPI_BOOL Tapi_Demux_Reset(TAPI_S8 index) {
    LOGD(" : %d line %s calling index= %d\n", __LINE__, __FUNCTION__,index);
    return am_demux_reset(index);
}

//////////////////////////////////////////////////
//@Fuction name : Tapi_Demux_Restart(U8 index)
//@Description:restart a filter
//@param <IN>
//@--index : same as outindex in the function demux_open(DMX_FILTER_TYPE type,U8 index,U8* outindex)
//@param <RET>
//@--TRUE:sucess
//@--FALSE:fail
TAPI_BOOL Tapi_Demux_Restart(TAPI_S8 index) {
    LOGD(" : %d line %s calling index= %d\n", __LINE__, __FUNCTION__,index);
    return am_demux_restart(index);
}

//////////////////////////////////////////////////
//@Fuction name : Tapi_Demux_Close(U8 index)
//@Description:close filter
//@param <IN>
//@--index : same as outindex in the function demux_open(DMX_FILTER_TYPE type,U8 index,U8* outindex)
//@param <RET>
//@--TRUE:sucess
//@--FALSE:fail
TAPI_BOOL Tapi_Demux_Close(TAPI_S8 index) {
    LOGD(" : %d line %s calling  index= %d\n", __LINE__, __FUNCTION__, index);
    return am_demux_close(index);
}

TAPI_BOOL Tapi_Demux_FlushSectionBuffer(TAPI_S8 index) {
    LOGD(" : %d line %s calling index= %d\n", __LINE__, __FUNCTION__,index);
    return am_demux_flushsectionbuffer(index);
}

////////////////////////////////////////////
//@Fuction name: Tapi_Demux_GetFilterStatus(TAPI_S8 index)
//@Description:get the filter status
////@param <IN>
//@--index : same as outindex in the function demux_open(DMX_FILTER_TYPE type,U8 index,U8* outindex)
//@param <RET>
//@---DMX_FILTER_STATUS

DMX_FILTER_STATUS Tapi_Demux_GetFilterStatus(TAPI_S8 index) {
    LOGD(" : %d line %s calling\n", __LINE__, __FUNCTION__);
    return am_demux_getfilterstatus(index);
}

//////////////////////////////////////////////////
//@Fuction name : Tapi_Demux_IsFilterTimeOut(U8 index)
//@Description: is filter out?
//@param <IN>
//@--index : same as outindex in the function demux_open(DMX_FILTER_TYPE type,U8 index,U8* outindex)
//@param <RET>
//@--TRUE:time out
//@--FALSE:time not out
TAPI_BOOL Tapi_Demux_IsFilterTimeOut(TAPI_S8 index) {
    LOGD(" : %d line %s calling\n", __LINE__, __FUNCTION__);
    return am_demux_isfiltertimeout(index);
}

///////////////////////////////////////////////////////////////////
//Fuction name :  Tapi_Demux_SetPID
//Description : set Video,Audio pid to demux,then call start,play automatically
//@param <IN>
//@--u16id : pid of audio,video
//@param <RET>
//@--TRUE:success
//@--FALSE:fail

TAPI_BOOL Tapi_Demux_GetData(TAPI_U8 index, TAPI_U8 serialnum, TAPI_U32 addr, TAPI_U32 len, TAPI_BOOL fgUpdateData, TAPI_U8* buf) {
    LOGD(" : %d line %s calling index= %d\n", __LINE__, __FUNCTION__, index);
    return am_demux_getdata(index, serialnum, addr, len, fgUpdateData, buf);
}

/////**************************************************************//////////
/*******************PID FILLTER***************************************/

//allocate a pid filter 
TAPI_BOOL Tapi_Demux_AllocatePidFilter(DMX_FILTER_TYPE type, TAPI_S8 index, TAPI_S8* outindex) {
    LOGD(" : %d line %s calling index= %d\n", __LINE__, __FUNCTION__,index);
    return am_demux_allocatepidfilter(type, index, outindex);
}

//set pid to filter
TAPI_BOOL Tapi_Demux_SetPID(TAPI_S8 index, TAPI_S16 u16id) {
    LOGD(" : %d line %s calling index= %d\n", __LINE__, __FUNCTION__,index);
    return am_demux_setpid(index, u16id);
}

TAPI_BOOL Tapi_PidFilter_Start(TAPI_S8 index) {
    LOGD(" : %d line %s calling index= %d\n", __LINE__, __FUNCTION__,index);
    return am_demux_pidfilter_start(index);
}

TAPI_BOOL Tapi_PidFilter_Stop(TAPI_S8 index) {
    LOGD(" : %d line %s calling index= %d\n", __LINE__, __FUNCTION__,index);
    return am_demux_pidfilter_stop(index);
}

TAPI_BOOL Tapi_PidFilter_Reset(TAPI_S8 index) {
    LOGD(" : %d line %s calling index= %d\n", __LINE__, __FUNCTION__,index);
    return am_demux_pidfilter_reset(index);
}

TAPI_BOOL Tapi_PidFilter_Restart(TAPI_S8 index) {
    LOGD(" : %d line %s calling index= %d\n", __LINE__, __FUNCTION__,index);
    return am_demux_pidfilter_restart(index);
}

TAPI_BOOL Tapi_PidFilter_Close(TAPI_S8 index) {
    LOGD(" : %d line %s calling index= %d\n", __LINE__, __FUNCTION__,index);
    return am_demux_pidfilter_close(index);
}
