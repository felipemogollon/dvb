#ifndef __DEMUX_CORE_H__
#define __DEMUX_CORE_H__

#include "TTvHW_demux.h"

/////////////////////////////////////////////////////////////////
//@Fuction name:am_demux_init
//@Description:init demux,then we can set filter
//@param <IN> :void
//@param <RET>
// @ --0:sucess
// @ --other:fail
int am_demux_init(void);

/////////////////////////////////////////////////////////////////
//@Fuction name:am_demux_exit
//@Description:free demux
//@param <IN> :void
//@param <RET>
//@  --0:sucess
// @ --other:fail
int am_demux_exit(void);

/////////////////////////////////////////////////////////////////
//@Fuction name:am_demux_get_currentdevno
//@Description:get dmx dev number used at present
//@param <IN> :void
//@param <RET> :return current used dmx dev number
int am_demux_get_currentdevno(void);

/////////////////////////////////////////////////////////////////
//@Fuction name:am_demux_set_currentdevno
//@Description:get dmx dev number used at present
//@param <IN> :dmx_devno-> dmx dev no to set
//@param <RET> :
void am_demux_set_currentdevno(const int dmx_devno);

//////////////////////////////////////////////////////////////////////////
//@Fuction name:am_demux_allocatefilter
//@Description:allocate a new filter
//@param <IN>
//@ ---type:DMX_FILTER_TYPE
//@ ---index:the index of filter assigned by writer
//@ ---outindex:the index of filter assigned by vendor,if not requred,let it same as index
//@---u16id: pid
//@param <RET>
//@--TRUE:sucess
//@--FALSE:fail
TAPI_BOOL am_demux_allocatefilter(DMX_FILTER_TYPE type, TAPI_S16 u16id, TAPI_S8 index, TAPI_S8* outindex);

///////////////////////////////////////////
//@Fuction name:am_demux_setsectionfilterparam
//@Description:set param to a section filter,it will be started after call fuction demux_start
//@param <IN>
//@ ---param:as to MTDMX_FILTER_PARAM
//@param <RET>
//@--TRUE:sucess
//@--FALSE:fail
TAPI_BOOL am_demux_setsectionfilterparam(TAPI_S8 index, DMX_FILTER_PARAM param);

//////////////////////////////////////////////////////////
//@Fuction name :am_demux_start
//@Description:start a filter after set param,
//@param <IN>
//@--index : same as outindex in the function demux_open(DMX_FILTER_TYPE type,U8 index,U8* outindex)
//@param <RET>
//@--TRUE:sucess
//@--FALSE:fail
TAPI_BOOL am_demux_start(TAPI_S8 index);

/////////////////////////////////////
//@Fuction name : am_demux_stop(U8 index)
//@Description:stop a filter after set param,
//@param <IN>
//@--index : same as outindex in the function demux_open(DMX_FILTER_TYPE type,U8 index,U8* outindex)
//@param <RET>
//@--TRUE:sucess
//@--FALSE:fail
TAPI_BOOL am_demux_stop(TAPI_S8 index);

/////////////////////////////////////
//@Fuction name : am_demux_reset(U8 index)
//@Description:reset a filter after set param and start
//@param <IN>
//--index : same as outindex in the function demux_open(DMX_FILTER_TYPE type,U8 index,U8* outindex)
//@param <RET>
//@--TRUE:sucess
//@--FALSE:fail
TAPI_BOOL am_demux_reset(TAPI_S8 index);

//////////////////////////////////////////////////
//@Fuction name : am_demux_restart(U8 index)
//@Description:restart a filter
//@param <IN>
//@--index : same as outindex in the function demux_open(DMX_FILTER_TYPE type,U8 index,U8* outindex)
//@param <RET>
//@--TRUE:sucess
//@--FALSE:fail
TAPI_BOOL am_demux_restart(TAPI_S8 index);

//////////////////////////////////////////////////
//@Fuction name : am_demux_close(U8 index)
//@Description:close filter
//@param <IN>
//@--index : same as outindex in the function demux_open(DMX_FILTER_TYPE type,U8 index,U8* outindex)
//@param <RET>
//@--TRUE:sucess
//@--FALSE:fail
TAPI_BOOL am_demux_close(TAPI_S8 index);

TAPI_BOOL am_demux_flushsectionbuffer(TAPI_S8 index);

////////////////////////////////////////////
//@Fuction name: am_demux_getfilterstatus(TAPI_S8 index)
//@Description:get the filter status
////@param <IN>
//@--index : same as outindex in the function demux_open(DMX_FILTER_TYPE type,U8 index,U8* outindex)
//@param <RET>
//@---DMX_FILTER_STATUS

DMX_FILTER_STATUS am_demux_getfilterstatus(TAPI_S8 index);

//////////////////////////////////////////////////
//@Fuction name : am_demux_isfiltertimeout(U8 index)
//@Description: is filter out?
//@param <IN>
//@--index : same as outindex in the function demux_open(DMX_FILTER_TYPE type,U8 index,U8* outindex)
//@param <RET>
//@--TRUE:time out
//@--FALSE:time not out
TAPI_BOOL am_demux_isfiltertimeout(TAPI_S8 index);

///////////////////////////////////////////////////////////////////
//Fuction name :  am_demux_getdata
//Description : 
//@param <IN>
//@--u16id : pid of audio,video
//@param <RET>
//@--TRUE:success
//@--FALSE:fail

TAPI_BOOL am_demux_getdata(TAPI_U8 index, TAPI_U8 serialnum, TAPI_U32 addr, TAPI_U32 len, TAPI_BOOL fgUpdateData, TAPI_U8* buf);

/////**************************************************************//////////
/*******************PID FILLTER***************************************/

//allocate a pid filter 
TAPI_BOOL am_demux_allocatepidfilter(DMX_FILTER_TYPE type, TAPI_S8 index, TAPI_S8* outindex);

//set pid to filter
TAPI_BOOL am_demux_setpid(TAPI_S8 index, TAPI_S16 u16id);

TAPI_BOOL am_demux_pidfilter_start(TAPI_S8 index);

TAPI_BOOL am_demux_pidfilter_stop(TAPI_S8 index);

TAPI_BOOL am_demux_pidfilter_reset(TAPI_S8 index);

TAPI_BOOL am_demux_pidfilter_restart(TAPI_S8 index);

TAPI_BOOL am_demux_pidfilter_close(TAPI_S8 index);

#endif //__DEMUX_CORE_H__
