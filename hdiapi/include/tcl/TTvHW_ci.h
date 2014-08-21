#ifndef _TTvHW_CI_H_
#define _TTvHW_CI_H_
//CI  return error code 与MTAL文档上错误码的定义一致
#include "TTvHW_type.h"
typedef enum{
//	CI_OK					=	0,
	CI0_NOT_INIT				=	-1,
	CI0_ALREADY_INIT			=	-2,
	CI0_INIT_FAILED				=	-3,
	CI0_INV_ARG				=	-4,
	CI0_MODULE_NOT_INSERTED			=	-5,
	CI0_CIS_ERROR				=	-6,
	CI0_REGISTER_ISR_FAILED			=	-7,
	CI0_CHANNEL_RESET_FAILED		=	-8,
	CI0_BUF_SIZE_NGO_FAILED			=	-9,
	CI0_DA_FR_INT_ENABLE_FAILED		=	-10,
	CI0_INVALID_ACCESS_MODE			=	-11,
	CI0_EXCEED_MAX_BUF_SIZE			=	-12,
	CI0_TIME_OUT				=	-13,	
	CI0_CMD_ERROR				=	-14,
	CI0_READ_ERROR				=	-15,
	CI0_WRITE_ERROR				=	-16,
	CI0_DATA_AVAILABLE			=	-17,
	CI0_POWER_CTRL_ERROR			=	-18,
	CI0_REG_TEST_ERROR			=	-19,
	CI0_CLI_ERROR				=	-20,
	CI0_DMA_BUF_NOT_ALLOC			=	-21,
	CI0_UNKNOWN_COND			=	-22,
}	TCLCI_Error_T;

#ifdef __cplusplus
extern "C" {
#endif

//-------------------------------------------------------------------------------------------------
/// @Function                         card drv init
/// @return                              
//-------------------------------------------------------------------------------------------------

TAPI_U8 Tapi_Ci_Init(void);

//-------------------------------------------------------------------------------------------------
/// @Function                            检测卡是否插入
/// @return                              1: 有卡插入, 0: 无卡.
//-------------------------------------------------------------------------------------------------

TAPI_U8 Tapi_Ci_PcmciaPolling( void );

//-------------------------------------------------------------------------------------------------
/// @Function                     整个cam卡的初始化，包括读取CIS，协商buffer
/// @param  buf                   IN: 主机CI协议栈所支持的最大传输缓存
/// @return                   		1: Ci_InitCam_ok
///																其他值：支持TCLCI_Error_T类型错误码
//-------------------------------------------------------------------------------------------------

TAPI_S8 Tapi_Ci_InitCam(TAPI_U16 buf);

//-------------------------------------------------------------------------------------------------
/// @Function                 	 使能TS流的流向，是直接流过DMX，还是经过大卡再流向DMX
/// @param  enable            	0：TS流直接流过DMX     非 0：TS流经过大卡再流向DMX
/// @return                   		1: 设置成功
///																其他值：支持TCLCI_Error_T类型错误码
//-------------------------------------------------------------------------------------------------

TAPI_S8 Tapi_Ci_TsBypass(TAPI_U8 enable);

//-------------------------------------------------------------------------------------------------
/// @Function                  读取cam卡状态，判断是否有数据可读或可写
/// @return                    1：表示cam卡有数据可读     2:表示cam卡状态可写 
///														 其他值：支持TCLCI_Error_T类型错误码      
//-------------------------------------------------------------------------------------------------

TAPI_S8 Tapi_Ci_ReadCamStatus(void);
//-------------------------------------------------------------------------------------------------
/// @Function                  从cam卡读取数据
/// @param  buf               IN: 读取到数据存放的起始地址
/// @param  len               IN: 协议栈所支持单次读取数据的最大的长度
/// @return                   1: Operation success, or 0: Operation failure.
//-------------------------------------------------------------------------------------------------

TAPI_U16 Tapi_Ci_ReadCamData( TAPI_U8* buf,TAPI_U16 len );

//-------------------------------------------------------------------------------------------------
/// @Function                 写入数据到cam卡
/// @param  buf               IN: 所要写入数据的起始地址
/// @param  len               IN: 所写数据的长度
/// @return                   1: Operation success, or 0: Operation failure.
//-------------------------------------------------------------------------------------------------

TAPI_U8 Tapi_Ci_WriteCamData( TAPI_U8* buf, TAPI_U16 len );

//-------------------------------------------------------------------------------------------------
/// @Function                     得到与cam协商后的buffer大小
/// @param  buf                   IN: 
/// @return                   		返回的是协商后的buffer大小
///																
//-------------------------------------------------------------------------------------------------

TAPI_U16 Tapi_Ci_GetCamBufSize(void);


//-------------------------------------------------------------------------------------------------
/// @Function                     开机初始化ci相关的驱动
/// @param  buf                   IN: 
/// @return                   		1: Operation success, or 0: Operation failure.
///																
//-------------------------------------------------------------------------------------------------

TAPI_U8 Tapi_Ci_InitPCMCIADriver(void);

#ifdef __cplusplus
}
#endif

#endif
