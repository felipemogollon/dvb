#ifndef __TUNER_API_H__
#define __TUNER_API_H__

#include "TTvHW_type.h"
#include "TTvHW_frontend_common.h"

#define CC_LOG_MODULE_MAX                       (32)
// log modules macro
#define CC_LOG_MODULE_CI                        (1)
#define CC_LOG_MODULE_DEMOD                     (2)
#define CC_LOG_MODULE_DEMUX                     (3)
#define CC_LOG_MODULE_AV                        (4)
#define CC_LOG_MODULE_TUNER                     (5)

// common log cfg declare
#define CC_LOG_CFG_DIABLE                       (0x00000000)
#define CC_LOG_CFG_ERROR                        (0x00000001)
#define CC_LOG_CFG_DEBUG                        (0x00000002)
#define CC_LOG_CFG_INFO                         (0x00000004)
#define CC_LOG_CFG_WARNING                      (0x00000008)
#define CC_LOG_CFG_VERBOSE                      (0x00000010)
#define CC_LOG_CFG_DEF                          (CC_LOG_CFG_ERROR | CC_LOG_CFG_DEBUG | CC_LOG_CFG_INFO | CC_LOG_CFG_WARNING | CC_LOG_CFG_VERBOSE)
#define CC_LOG_CFG_ALL                          (CC_LOG_CFG_ERROR | CC_LOG_CFG_DEBUG | CC_LOG_CFG_INFO | CC_LOG_CFG_WARNING | CC_LOG_CFG_VERBOSE)

#ifdef __cplusplus 
extern "C" {
#endif

int log_cfg_get(int log_cfg_module);
int log_cfg_set(int log_cfg_module, unsigned int cfg_val);

TAPI_BOOL Tapi_Driver_Init(void);

#ifdef __cplusplus
}
#endif

#endif //__TUNER_API_H__
