#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <cutils/properties.h>
#include <android/log.h>

#include "../com/com_api.h"
#include "demod_core.h"
#include "TTvHW_demodulator.h"

#include "am_fend.h"
#include "am_types.h"
#include "linux/dvb/frontend.h"
#include "am_misc.h"
#include "am_time.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <errno.h>

#define LOG_TAG "demod_api"

#define LOGD(...) \
    if( log_cfg_get(CC_LOG_MODULE_DEMOD) & CC_LOG_CFG_DEBUG) { \
    __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__); } \

#define LOGE(...) \
    if( log_cfg_get(CC_LOG_MODULE_DEMOD) & CC_LOG_CFG_ERROR) { \
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__); } \


#define DEBUG_TIME(a...)\
    do{\
        int clk;\
        AM_TIME_GetClock(&clk);\
        fprintf(stderr, "%dms ",clk);\
        fprintf(stderr, a);\
        fprintf(stderr, "\n");\
    }while(0)

#define FE_DEV_ID  0

#define HDI_INSTANCE_PER_DMD 2
#define HDI_DMD_CALLBACK_CNT 1
#define FE_TYPE_COUNT        2

#define _FE_UNKNOWN_      0
#define _FE_LOCK_      1
#define _FE_UNLOCK_      2

static pthread_mutex_t dmd_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
static TAPI_BOOL hdi_dmd_init = TAPI_FALSE;
static EN_DEVICE_DEMOD_TYPE demo_type = E_DEVICE_DEMOD_NULL;
static TAPI_BOOL demo_opened = AM_FALSE;
static int demo_fe_no = FE_DEV_ID;
static int demo_lock = _FE_UNKNOWN_;

static void dmd_fend_callback(int dev_no, int event_type, void *param, void *user_data) {
    struct dvb_frontend_event *evt = (struct dvb_frontend_event*) param;

    if (!evt || (evt->status == 0))
        return;
    LOGE("%s,evt->status=0x%x\n", __FUNCTION__, evt->status);
    pthread_mutex_lock(&dmd_lock);
    if (evt->status & FE_HAS_LOCK) {
        demo_lock = _FE_LOCK_;
        property_set("tvin.dtvLockStatus", "lock");
    } else if (evt->status & FE_TIMEDOUT) {
        demo_lock = _FE_UNLOCK_;
        property_set("tvin.dtvLockStatus", "unlock"); 
    }
    pthread_mutex_unlock(&dmd_lock);
    //dmd_notify_status(dev_no, (evt->status & FE_HAS_LOCK) ? SK_HDI_DMD_STATUS_LOCKED : SK_HDI_DMD_STATUS_UNLOCKED);
}

static fe_modulation_t QAMHdi2Local(EN_CAB_CONSTEL_TYPE eqam) {
    fe_modulation_t fe_mod = QAM_AUTO;
    switch (eqam) {
    case E_CAB_QAM16:
        fe_mod = QAM_16;
        break;
    case E_CAB_QAM32:
        fe_mod = QAM_32;
        break;
    case E_CAB_QAM64:
        fe_mod = QAM_64;
        break;
    case E_CAB_QAM128:
        fe_mod = QAM_128;
        break;
    case E_CAB_QAM256:
        fe_mod = QAM_256;
        break;
    }

    return fe_mod;
}

static fe_bandwidth_t BandWidthHdi2Local(RF_CHANNEL_BANDWIDTH bandwidth) {
    fe_bandwidth_t fe_bw = BANDWIDTH_AUTO;
    switch (bandwidth) {
    case E_RF_CH_BAND_6MHz:
        fe_bw = BANDWIDTH_6_MHZ;
        break;
    case E_RF_CH_BAND_7MHz:
        fe_bw = BANDWIDTH_7_MHZ;
        break;
    case E_RF_CH_BAND_8MHz:
        fe_bw = BANDWIDTH_8_MHZ;
        break;
    }

    return fe_bw;
}

static EN_CAB_CONSTEL_TYPE QAMLocal2hdi(fe_modulation_t fe_mod) {
    EN_CAB_CONSTEL_TYPE myeqam = E_CAB_INVALID;
    switch (fe_mod) {
    case QAM_16:
        myeqam = E_CAB_QAM16;
        break;
    case QAM_32:
        myeqam = E_CAB_QAM32;
        break;
    case QAM_64:
        myeqam = E_CAB_QAM64;
        break;
    case QAM_128:
        myeqam = E_CAB_QAM128;
        break;
    case QAM_256:
        myeqam = E_CAB_QAM256;
        break;
    }

    return myeqam;
}

static RF_CHANNEL_BANDWIDTH BandWidthLocal2hdi(fe_bandwidth_t fe_bw) {
    RF_CHANNEL_BANDWIDTH bandwidth = E_RF_CH_BAND_INVALID;
    switch (fe_bw) {
    case BANDWIDTH_6_MHZ:
        bandwidth = E_RF_CH_BAND_6MHz;
        break;
    case BANDWIDTH_7_MHZ:
        bandwidth = E_RF_CH_BAND_7MHz;
        break;
    case BANDWIDTH_8_MHZ:
        bandwidth = E_RF_CH_BAND_8MHz;
        break;
    }

    return bandwidth;
}

static fe_type_t DemodTypehdi2Local(EN_DEVICE_DEMOD_TYPE fe_demod_type) {
    /**default demod type is atsc.but is insignificance.*/
    fe_type_t my_type = FE_ATSC;

    if (fe_demod_type == E_DEVICE_DEMOD_DVB_T) {
        my_type = FE_OFDM;
    } else if (fe_demod_type == E_DEVICE_DEMOD_DVB_C) {
        my_type = FE_QAM;
    } else if (fe_demod_type == E_DEVICE_DEMOD_DTMB) {
        my_type = FE_DTMB;
    }

    return my_type;
}

//-------------------------------------------------------------------------------------------------
/// Connect to demod & set demod type.
/// @param  enDemodType  IN: Demod type.
/// @return                  TRUE: Connecting and setting success, FALSE: Connecting failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Demod_Connect(EN_DEVICE_DEMOD_TYPE enDemodType) {
    int ret = TAPI_TRUE;

    pthread_mutex_lock(&dmd_lock);
    LOGD("Switching fe_type from %d to %d.\n", demo_type, enDemodType);
    demo_type = enDemodType;
    pthread_mutex_unlock(&dmd_lock);

    return ret;
}

EN_DEVICE_DEMOD_TYPE Demod_Get_Type(void) {
    return demo_type;
}

//-------------------------------------------------------------------------------------------------
/// Disconnect from demod.
/// @param  None
/// @return                  TRUE: Disconnect success, FALSE: Disconnect failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Demod_Disconnect(void) {
    int ret = TAPI_TRUE;

    LOGD("%s, entering...\n", __FUNCTION__);

    pthread_mutex_lock(&dmd_lock);

    if (!hdi_dmd_init) {
        LOGE("%s, Not init yet.\n", __FUNCTION__);
        ret = TAPI_FALSE;
        goto finish;
    }

    if (demo_opened) {
        LOGD("%s, Close frontend %d OK.\n", __FUNCTION__, demo_fe_no);
        AM_FEND_Close(demo_fe_no);
        demo_opened = AM_FALSE;
    } else {
        LOGD("%s, not open frontend %d OK.\n", __FUNCTION__, demo_fe_no);
    }

    ret = AM_EVT_Unsubscribe(0, AM_FEND_EVT_STATUS_CHANGED, dmd_fend_callback, NULL);
    if (0 == ret) {
        LOGD("%s, AM_EVT_Unsubscribe OK\n", __FUNCTION__);
    }

    hdi_dmd_init = TAPI_FALSE;

    finish: pthread_mutex_unlock(&dmd_lock);

    return ret;
}

//-------------------------------------------------------------------------------------------------
/// Reset demod status and state.
/// @param  None
/// @return  None
//-------------------------------------------------------------------------------------------------
void Tapi_Demod_Reset() {
    int ret = TAPI_TRUE;

    LOGD("%s, entering...\n", __FUNCTION__);

    AM_FEND_OpenPara_t para;
    memset(&para, 0, sizeof(para));

    pthread_mutex_lock(&dmd_lock);

    if (hdi_dmd_init) {
        LOGE("%s, Already initialized.\n", __FUNCTION__);
        ret = TAPI_FALSE;
        goto finish;
    }
    AM_FEND_Close(demo_fe_no);
    para.mode = DemodTypehdi2Local(demo_type);
    if (AM_FEND_Open(demo_fe_no, &para) != AM_SUCCESS) {
        LOGE("%s, ***frontend %d open failed***\n", __FUNCTION__, demo_fe_no);
        ret = TAPI_FALSE;
        goto finish;
    }

    /*register callback */
    AM_EVT_Subscribe(0, AM_FEND_EVT_STATUS_CHANGED, dmd_fend_callback, NULL);
    LOGD("%s, Init frontend device %d OK.\n", __FUNCTION__, demo_fe_no);

    finish: hdi_dmd_init = TAPI_TRUE;

    pthread_mutex_unlock(&dmd_lock);
}
//-------------------------------------------------------------------------------------------------
/// Set demod to bypass iic ,so controller can communcate with tuner.
/// @param  enable        IN: bypass or not.
/// @return                  TRUE: Bypass setting success, FALSE: Bypass setting done failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Demod_IIC_Bypass_Mode(TAPI_BOOL enable) {
    LOGD("%s, entering...\n", __FUNCTION__);
    return TAPI_FALSE;
}

//-------------------------------------------------------------------------------------------------
/// Init demod.
/// @param  None
/// @return                  TRUE: Init success, FALSE: Init failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Demod_Initialization(void) {
    int ret = TAPI_TRUE;

    LOGD("%s, entering...\n", __FUNCTION__);

    pthread_mutex_lock(&dmd_lock);

    if (hdi_dmd_init) {
        LOGE("%s,  Already initialized.\n", __FUNCTION__);
        ret = TAPI_FALSE;
        goto finish;
    }

    if (!demo_opened) {
        AM_FEND_OpenPara_t para;
        memset(&para, 0, sizeof(para));
        /*Mode changed*/
        para.mode = DemodTypehdi2Local(demo_type);
        if (AM_FEND_Open(demo_fe_no, &para) != AM_SUCCESS) {
            /*This case must Not happen*/
            LOGE("%s, open frontend %d for fe_type=%d failed.\n", __FUNCTION__, demo_fe_no, demo_type);
            ret = TAPI_FALSE;
            goto finish;
        }

        LOGD("%s, Switch fe_type OK.\n", __FUNCTION__);
        demo_opened = AM_TRUE;
    }

    /*register callback */
    AM_EVT_Subscribe(0, AM_FEND_EVT_STATUS_CHANGED, dmd_fend_callback, NULL);
    LOGD("%s, Init frontend device %d OK.\n", __FUNCTION__, demo_fe_no);
    property_set("tvin.dtvLockStatus", "unlock"); 
    finish: hdi_dmd_init = TAPI_TRUE;

    pthread_mutex_unlock(&dmd_lock);

    return ret;
}

//-------------------------------------------------------------------------------------------------
/// Set demod power on.
/// @param  None
/// @return                  TRUE: Set success, FALSE: Set failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Demod_Set_PowerOn(void) {
    LOGD("%s, entering...\n", __FUNCTION__);

    return TAPI_TRUE;
}

//-------------------------------------------------------------------------------------------------
/// Set demod power off.
/// @param  None
/// @return                  TRUE: Set success, FALSE: Set failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Demod_Set_PowerOff(void) {
    LOGD("%s, entering...\n", __FUNCTION__);

    return TAPI_TRUE;
}

//-------------------------------------------------------------------------------------------------
/// Get current demodulator type.
/// @param  None
/// @return  EN_DEVICE_DEMOD_TYPE      Current demodulator type.
//-------------------------------------------------------------------------------------------------
EN_DEVICE_DEMOD_TYPE Tapi_Demod_GetCurrentDemodulatorType(void) {
    LOGD("%s, entering...\n", __FUNCTION__);

    return demo_type;
}

//-------------------------------------------------------------------------------------------------
/// Get current demodulator type.
/// @param  enDemodType  IN: demodulator type
/// @return       TRUE: Set success, FALSE: Set failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Demod_SetCurrentDemodulatorType(EN_DEVICE_DEMOD_TYPE enDemodType) {
    LOGD("%s, entering...\n", __FUNCTION__);

    if (enDemodType == E_DEVICE_DEMOD_DVB_T || enDemodType == E_DEVICE_DEMOD_DVB_C || enDemodType == E_DEVICE_DEMOD_DTMB) {
        demo_type = enDemodType;
        return TAPI_TRUE;
    }

    return TAPI_FALSE;
}

//-------------------------------------------------------------------------------------------------
/// For user defined command. for extending
/// @param  SubCmd      IN: sub command.
/// @param  u32Param1      IN: command parameter1.
/// @param  u32Param2      IN: command parameter2.
/// @param  pvParam3      IN: command parameter3 point.
/// @return       TRUE: Set success, FALSE: Set failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Demod_ExtendCmd(TAPI_U8 SubCmd, TAPI_U32 u32Param1, TAPI_U32 u32Param2, void *pvParam3) {
    LOGD("%s, entering...\n", __FUNCTION__);

    return TAPI_TRUE;
}

//-------------------------------------------------------------------------------------------------
/// Get signal SNR.
/// @param  None
/// @return        Signal condition.
//-------------------------------------------------------------------------------------------------
TAPI_U16 Tapi_Demod_GetSNR(void) {
    int Strength = 0;
    int data = 0;
    int verifydata = 0;
    EN_FRONTEND_SIGNAL_CONDITION signal_condition = E_FE_SIGNAL_NO;

    LOGD("%s, entering...\n", __FUNCTION__);

    pthread_mutex_lock(&dmd_lock);

    AM_FEND_GetStrength(FE_DEV_ID, &Strength);

    LOGD("%s, Strength = %d.\n", __FUNCTION__, Strength);
#if 0
    /*adjust signal strength data which get from driver */
    data = Strength ^ 65535;
    verifydata = (data + 1) * (-1);

    if (verifydata > -89 && verifydata < -85) {
        signal_condition = E_FE_SIGNAL_NO;
    } else if (verifydata >= -85 && verifydata < -75) {
        signal_condition = E_FE_SIGNAL_WEAK;
    } else if (verifydata >= -75 && verifydata < -40) {
        signal_condition = E_FE_SIGNAL_STRONG;
    } else if (verifydata >= -40 && verifydata < 5) {
        signal_condition = E_FE_SIGNAL_VERY_STRONG;
    }
#endif
//    pthread_mutex_unlock(&dmd_lock);
    return (TAPI_U16) Strength;
}

//-------------------------------------------------------------------------------------------------
/// Get signal BER.
/// @param  None
/// @return        Bit error rate.
//-------------------------------------------------------------------------------------------------
TAPI_U32 Tapi_Demod_GetBER(void) {
    int ber = 0;

    LOGD("%s, entering...\n", __FUNCTION__);

    pthread_mutex_lock(&dmd_lock);

    AM_FEND_GetBER(FE_DEV_ID, &ber);
    LOGD("%s, ber = %d.\n", __FUNCTION__, ber);

    pthread_mutex_unlock(&dmd_lock);

    return ber;
}

//-------------------------------------------------------------------------------------------------
///Tapi_Demod_CheckDtmbMoudle
/// @param  None
/// @return                  TRUE:
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Demod_CheckDtmbMoudle(void)
{
	TAPI_BOOL iRetExist = true;
	int fd =-1,ret =-1;
	TAPI_U8 chipId = 0x00;
	TAPI_U8 chipIdReg[2] = {0};
	struct i2c_rdwr_ioctl_data data;
    	struct i2c_msg msg[2];

	do{
		LOGD("%s, entering...\n", __FUNCTION__);
		pthread_mutex_lock(&dmd_lock);
		fd = open("/dev/i2c-0", O_RDWR);
		if(fd < 0)
		{
			iRetExist = false;
			break;
		}

		ret = ioctl(fd,I2C_TENBIT,0);
		if(-1 == ret)
		{
			iRetExist = false;
			break;
		}
		
		ret = ioctl(fd,I2C_SLAVE,0x40);
		if(ret == -1)
		{
			LOGE("ioctl I2C_SLVE %s\n", strerror(errno));
			iRetExist = false;
			break;
		}

		data.nmsgs = 2;
    		data.msgs  = msg;
		msg[0].addr  = 0x40;
    		msg[0].flags = 0;
    		msg[0].len   = 2;
    		msg[0].buf   = chipIdReg;

		msg[1].addr  = 0x40;
    		msg[1].flags = I2C_M_RD;
    		msg[1].len   = 1;
    		msg[1].buf   = &chipId;

		ret = ioctl(fd, I2C_RDWR, (unsigned long)&data);
		if (ret < 0) {
			LOGE("ERR:DTMB  READ => %d %s\n", ret,strerror(errno));
			iRetExist = false;
			break;
		}
		LOGE("chipId ============= %02x\n",chipId);
		
	}while(false);
	
	if(fd > 0)
	{
		close(fd);
	}
	
	pthread_mutex_unlock(&dmd_lock);
	return iRetExist;
}

//-------------------------------------------------------------------------------------------------
/// Get Packet error of DVBT and DVBC.
/// @param  None
/// @return        Packet error.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL DTV_GetPacketErr(TAPI_U16 *pKtErr) {
    LOGD("%s, entering...\n", __FUNCTION__);

    return TAPI_TRUE;
}
//per
//-------------------------------------------------------------------------------------------------
/// Get signal status.
/// @param  None
/// @return       TRUE: Get success, FALSE: Get failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Demod_GetSignalStatus(SIGNAL_STATUS *sSignalStatus) {
    int quality;
    int strength;

    pthread_mutex_lock(&dmd_lock);

    AM_FEND_GetSNR(FE_DEV_ID, &quality);
    sSignalStatus->SignalQuality = quality;

    AM_FEND_GetStrength(FE_DEV_ID, &strength);
    sSignalStatus->SignalStrength = strength;

    pthread_mutex_unlock(&dmd_lock);
	
    LOGD("%s, entering...quality=%d strength=%d %d\n", __FUNCTION__, quality, strength);

    return TAPI_TRUE;
}

//-------------------------------------------------------------------------------------------------
/// Set demod to output serial or parallel.
/// @param  bEnable  IN: True : serial . False : parallel.
/// @return       TRUE: Set success, FALSE: Set failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Demod_Serial_Control(TAPI_BOOL bEnable) {
    LOGD("%s, entering...\n", __FUNCTION__);

    return TAPI_TRUE;
}

//-------------------------------------------------------------------------------------------------
/// Config demod.(DVBC)
/// @param  sDemodSetting      IN:  demod setting ,include Frequency,BandWidth,Symbol rate, QAM type
/// @return       TRUE: Set success, FALSE: Set failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Demod_DVB_SetFrequency(DEMOD_SETTING sDemodSetting) {
    int ret = -1;
    int sync = 1;
    fe_status_t status = (fe_status_t) 0;
    AM_FENDCTRL_DVBFrontendParameters_t para;

    LOGD("%s, is para %d %d %d %d\n", __FUNCTION__, sDemodSetting.u32Frequency, sDemodSetting.eQAM, sDemodSetting.eBandWidth, sDemodSetting.uSymRate);

    pthread_mutex_lock(&dmd_lock);
    property_set("tvin.dtvLockStatus", "unlock"); 
    demo_lock = _FE_UNKNOWN_;
    memset(&para, 0, sizeof(AM_FENDCTRL_DVBFrontendParameters_t));
    para.m_type = DemodTypehdi2Local(demo_type);
    LOGE("%s, demod type is %d.\n", __FUNCTION__, para.m_type);

    if (demo_type == E_DEVICE_DEMOD_NULL || !demo_opened) {
        LOGE("%s, fontend is not init ok.\n", __FUNCTION__);

        pthread_mutex_unlock(&dmd_lock);
        return TAPI_FALSE;
    }

    if (demo_type == E_DEVICE_DEMOD_DVB_T) {
        LOGD("%s, E_DEVICE_DEMOD_DVB_T is support.\n", __FUNCTION__);
        para.terrestrial.para.frequency = sDemodSetting.u32Frequency * 1000;
        para.terrestrial.para.u.ofdm.bandwidth = BandWidthHdi2Local(sDemodSetting.eBandWidth);
    }

    if (demo_type == E_DEVICE_DEMOD_DVB_C) {
        LOGD("%s, E_DEVICE_DEMOD_DVB_C u32State=%d\n", __FUNCTION__, sDemodSetting.u32State);
        para.cable.para.frequency = sDemodSetting.u32Frequency * 1000;
        para.cable.para.u.qam.symbol_rate = sDemodSetting.uSymRate * 1000;
        fe_modulation_t fe_mod = QAMHdi2Local(sDemodSetting.eQAM);
        para.cable.para.u.qam.modulation = fe_mod;
        if (sDemodSetting.u32State) {
            AM_FileEcho("/sys/class/m6_demod/auto_sym", "1");
        } else {
            AM_FileEcho("/sys/class/m6_demod/auto_sym", "0");
        }
    }

    if (demo_type == E_DEVICE_DEMOD_DTMB) {
        LOGD("%s, E_DEVICE_DEMOD_DTMB u32State=%d\n", __FUNCTION__, sDemodSetting.u32State);
        para.terrestrial.para.frequency = sDemodSetting.u32Frequency * 1000;
        para.terrestrial.para.u.ofdm.bandwidth = BANDWIDTH_8_MHZ;
    }

    if (sync == 0) {
        ret = AM_FENDCTRL_Lock(FE_DEV_ID, &para, &status);
    } else {
        ret = AM_FENDCTRL_SetPara(FE_DEV_ID, &para);
    }

    pthread_mutex_unlock(&dmd_lock);

    if (status & FE_HAS_LOCK) {
        return TAPI_TRUE;
    }

    if (ret == 0) {
        return AM_TRUE;
    } else {
        return AM_FALSE;
    }

    return TAPI_FALSE;
}

//-------------------------------------------------------------------------------------------------
/// get demod config.(DVBC)
/// @param  sDemodSetting     OUT:  demod setting ,include Frequency,BandWidth,Symbol rate, QAM type
/// @return       TRUE: Get success, FALSE: Get failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Demod_DVB_GetFrequency(DEMOD_SETTING *sDemodSetting) {
    int ret = 0;
    struct dvb_frontend_parameters para;

    LOGD("%s, entering...\n", __FUNCTION__);

    pthread_mutex_lock(&dmd_lock);

    memset(&para, 0, sizeof(struct dvb_frontend_parameters));
    //sDemodSetting = (DEMOD_SETTING *) malloc  (sizeof(struct DEMOD_SETTING));

    if (demo_type == E_DEVICE_DEMOD_NULL) {
        LOGE("%s, fontend type is E_DEVICE_DEMOD_NULL.", __FUNCTION__);

        pthread_mutex_unlock(&dmd_lock);
        return TAPI_FALSE;
    }

    if ((ret = AM_FEND_GetPara(FE_DEV_ID, &para)) < 0) {
        LOGE("%s, getPara faiture.", __FUNCTION__);

        pthread_mutex_unlock(&dmd_lock);
        return TAPI_FALSE;
    }

    pthread_mutex_unlock(&dmd_lock);

    if (demo_type == E_DEVICE_DEMOD_DVB_T) {
        sDemodSetting->u32Frequency = para.frequency;
        sDemodSetting->eQAM = E_CAB_INVALID;
        sDemodSetting->uSymRate = 0;
        sDemodSetting->eBandWidth = BandWidthLocal2hdi(para.u.ofdm.bandwidth);
    } else if (demo_type == E_DEVICE_DEMOD_DVB_C) {
        sDemodSetting->u32Frequency = para.frequency;
        sDemodSetting->eBandWidth = E_RF_CH_BAND_INVALID;
        sDemodSetting->uSymRate = para.u.qam.symbol_rate / 1000;
        sDemodSetting->eQAM = QAMLocal2hdi(para.u.qam.modulation);
    } else if (demo_type == E_DEVICE_DEMOD_DTMB) {
        sDemodSetting->u32Frequency = para.frequency;
        sDemodSetting->eBandWidth = E_RF_CH_BAND_8MHz;
        sDemodSetting->uSymRate = 0;
        sDemodSetting->eQAM = E_CAB_INVALID;
    }

    LOGD("%s is para %d %d %d %d.\n", __FUNCTION__, sDemodSetting->u32Frequency, sDemodSetting->eQAM, sDemodSetting->eBandWidth, sDemodSetting->uSymRate);

    return TAPI_TRUE;
}

//-------------------------------------------------------------------------------------------------
/// Set TS output enable/disable.(DVBC)
/// @param  bEnable TRUE:enable TS output, FALSE:disable TS output
/// @return  TRUE: Set success, FALSE: Set failure.
//-------------------------------------------------------------------------------------------------
TAPI_BOOL Tapi_Demod_SetTsOutput(TAPI_BOOL bEnable) {
    LOGD("%s, entering...\n", __FUNCTION__);

    return TAPI_TRUE;
}

//-------------------------------------------------------------------------------------------------
/// Get Lock Status.(DVBC)
/// @param  None
/// @return       Lock Status.
//-------------------------------------------------------------------------------------------------ze
EN_LOCK_STATUS Tapi_Demod_GetLockStatus(void) {
#if 0
    fe_status_t status;
    //LOGD("%s, entering...\n", __FUNCTION__);

    pthread_mutex_lock(&dmd_lock);

    AM_FEND_GetStatus(FE_DEV_ID, &status);

    pthread_mutex_unlock(&dmd_lock);

    LOGE("%s,status=0x%x\n", __FUNCTION__,status);

    if (status & FE_HAS_LOCK) {
        return E_DEMOD_LOCK;
    }
    else if (status & FE_TIMEDOUT)
    return E_DEMOD_UNLOCK;
    else
    return E_DEMOD_CHECKING;
#else
    EN_LOCK_STATUS enlock;
    pthread_mutex_lock(&dmd_lock);
    LOGE("%s,demo_lock=0x%x\n", __FUNCTION__, demo_lock);
    if (demo_lock == _FE_UNKNOWN_)
        enlock = E_DEMOD_CHECKING;
    else if (demo_lock == _FE_LOCK_)
        enlock = E_DEMOD_LOCK;
    else
        enlock = E_DEMOD_UNLOCK;
    pthread_mutex_unlock(&dmd_lock);
    return enlock;
#endif
}

//------------------------------------------------------------------------------
/// To get frequency offset
/// @param None
/// @return value of offset
//------------------------------------------------------------------------------
TAPI_U32 Tapi_Demod_DVB_GetFrequencyOffset(void) {
    LOGD("%s, entering...\n", __FUNCTION__);

    return TAPI_TRUE;
}

TAPI_BOOL Demod_GetLockStatus(void) {
    TAPI_BOOL ret = TAPI_FALSE;
    pthread_mutex_lock(&dmd_lock);
    if (demo_lock == _FE_LOCK_)
        ret = TAPI_TRUE;
    pthread_mutex_unlock(&dmd_lock);
    return ret;
}

EN_DEVICE_DEMOD_TYPE Tapi_Demod_Get_StartupType(void) {
    int i = 0;
    char prop_value[PROPERTY_VALUE_MAX];
    EN_DEVICE_DEMOD_TYPE  demo_type = E_DEVICE_DEMOD_DVB_C;

    memset(prop_value, '\0', PROPERTY_VALUE_MAX);
    for (i = 0; i < 30; i++) {
        property_get("tvin.dtvtype", prop_value, "null");
        if (strcmp(prop_value, "DTV_TYPE_DVBC") == 0) {
            demo_type = E_DEVICE_DEMOD_DVB_C;
            break;
        }
        if (strcmp(prop_value, "DTV_TYPE_DTMB") == 0) {
             demo_type = E_DEVICE_DEMOD_DTMB;
             break;
        } else {
            usleep(100 * 1000);
        }
    }
    LOGD("%s, prop_value = %s, i = %d true\n", __FUNCTION__, prop_value, i);
    return demo_type;
}

