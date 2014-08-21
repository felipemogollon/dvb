////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2012-2020 TCL, Inc.
// All rights reserved.
//
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _TTVHW_PROXY_COMMON_H_
#define _TTVHW_PROXY_COMMON_H_

#include "TTvHW_type.h"

#ifdef __cplusplus 
extern "C" {
#endif

//-------------------------------------------------------------------------------------------------
/// Do MTK driver software initialize
/// @return              OUT: MAPI_TRUE or MAPI_FALSE
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Driver_Init(void); 

#ifdef __cplusplus 
}
#endif

#endif
