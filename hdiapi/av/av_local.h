#ifndef __AV_LOCAL_H__
#define __AV_LOCAL_H__

/*!������Χ: [0, 31]*/
#define HDI_AV_AUDIO_VOL_MAX      	32   
#define HDI_AV_AUDIO_VOL_MIN      	0   

/*!͸����ֵ[0,100]*/
#define HDI_AV_ALPHA_MAX      	100   
#define HDI_AV_ALPHA_MIN      	0   

/*!���ȷ�Χ[0,100]*/
#define HDI_AV_BRIGHTNESS_MAX      	100   
#define HDI_AV_BRIGHTNESS_MIN      	0   

/*!�Աȶȷ�Χ[0,100]*/
#define HDI_AV_CONTRAST_MAX      	100   
#define HDI_AV_CONTRAST_MIN      	0   

/*!���Ͷȷ�Χ[0,100]*/
#define HDI_AV_SATURATION_MAX      	100   
#define HDI_AV_SATURATION_MIN      	0   

/*!
 * decoder id, ֧�ֶ��decoder
 */
typedef enum hdi_av_decoder_id_e {
    HDI_AV_DECODER_MAIN = 1, /*!< main*/
    HDI_AV_DECODER_AUX = 2 /*!< aux*/
} hdi_av_decoder_id_t;

/*!decoder ����*/
#define   HDI_AV_NB_OF_DECODER 2  

/*!
 * decoder state ����
 */
typedef enum hdi_av_decoder_state_e {
    HDI_AV_DECODER_STATE_RUNNING = 0, /*!< ���ڽ���*/
    HDI_AV_DECODER_STATE_PAUSING, /*!< ��ͣ����, ʵʱ��������ͣ*/
    HDI_AV_DECODER_STATE_FREEZING, /*!< ����,����ʾ,����ʵʱ������ʹ�����������ͣ*/
    HDI_AV_DECODER_STATE_STOPPED, /*!< ֹͣ����*/
    HDI_AV_DECODER_STATE_UNKNOWN, /*!< δ֪״̬*/
    HDI_AV_NB_OF_DECODER_STATE
} hdi_av_decoder_state_t;

/*!
 * ��Ƶ�������ָ�ģʽ 
 */
typedef enum hdi_av_error_recovery_mode_e {
    HDI_AV_ERROR_RECOVERY_MODE_NONE = 0, /*!< �޾��?������*/
    HDI_AV_ERROR_RECOVERY_MODE_PARTIAL, /*!< ���־���*/
    HDI_AV_ERROR_RECOVERY_MODE_HIGH, /*!< ���־���*/
    HDI_AV_ERROR_RECOVERY_MODE_FULL, /*!< ȫ���?������*/
    HDI_AV_NB_OF_ERROR_RECOVERY_MODE
} hdi_av_error_recovery_mode_t;

/*!
 * ��� ����
 */
typedef enum hdi_av_aud_channel_e {
    HDI_AV_AUD_CHANNEL_MONO = 0, /*!<�����*/
    HDI_AV_AUD_CHANNEL_RIGHT, /*!<�����*/
    HDI_AV_AUD_CHANNEL_LEFT, /*!<�����*/
    HDI_AV_AUD_CHANNEL_STER, /*!<������*/
    HDI_AV_NB_OF_AUD_CHANNEL
} hdi_av_aud_channel_t;

/*!
 * ֡�� 
 *@see hdi_av_format_t
 */
typedef enum hdi_av_vid_frame_rate_e {
    HDI_AV_VID_FRAME_RATE_UNKNOWN = 0,
    HDI_AV_VID_FRAME_RATE_AUTO = 1, /*Ϊ�˱�����Ƶ��,�Ƽ�ʹ��HDI_AV_VID_FRAME_RATE_AUTO */
    HDI_AV_VID_FRAME_RATE_23_976 = 1 << 1,
    HDI_AV_VID_FRAME_RATE_24 = 1 << 2,
    HDI_AV_VID_FRAME_RATE_25 = 1 << 3,
    HDI_AV_VID_FRAME_RATE_29_97 = 1 << 4,
    HDI_AV_VID_FRAME_RATE_30 = 1 << 5,
    HDI_AV_VID_FRAME_RATE_50 = 1 << 6,
    HDI_AV_VID_FRAME_RATE_59_94 = 1 << 7,
    HDI_AV_VID_FRAME_RATE_60 = 1 << 8
} hdi_av_vid_frame_rate_t;

/*!
 * Spidf Mode  ����(ac3 & pcm)
 */
typedef enum hdi_av_spdif_mode_e {
    HDI_AV_SPDIF_MODE_PCM = 0,
    HDI_AV_SPDIF_MODE_AC3,
    HDI_AV_NB_OF_SPDIF_MODE
} hdi_av_spdif_mode_t;

/*!
 @brief ��Ƶ���ͨ������
 */
typedef enum hdi_av_aout_device_e {
    HDI_AV_AOUT_DEVICE_NONE = 0,
    HDI_AV_AOUT_DEVICE_RCA = 1, /*!< ��Ƶ������������*/
    HDI_AV_AOUT_DEVICE_SPDIF = 1 << 1, /*!< ��ƵSPDIF�����*/
    HDI_AV_AOUT_DEVICE_HDMI = 1 << 2 /*!< HDMI��Ƶ���*/
} hdi_av_aout_device_t;

/*!
 @brief ��Ƶ���ͨ������,�������spdif 
 */
typedef enum hdi_av_aud_op_mode_e {
    HDI_AUD_OP_MODE_DECODE, /*!<��Ƶ������Ҫ������룬Ȼ�����*/
    HDI_AUD_OP_MODE_BYPASS /*!<��Ƶ��������Ҫ�������ֱ�����*/
} hdi_av_aud_op_mode_t;

/*!
 * Av Event 
 */
typedef enum hdi_av_evt_e {
    HDI_AV_EVT_BASE = 0,
    /*video event*/
    HDI_AV_VID_EVT_BASE = HDI_AV_EVT_BASE,
    HDI_AV_VID_EVT_DECODE_START = HDI_AV_VID_EVT_BASE, /*!< This event passes a (hdi_av_vid_status_t *) as parameter */
    HDI_AV_VID_EVT_DECODE_STOPPED, /*!< This event passes a (hdi_av_vid_status_t *) as parameter */
    HDI_AV_VID_EVT_NEW_PICTURE_DECODED, /*!< This event passes a (hdi_av_vid_status_t *) as parameter */
    HDI_AV_VID_EVT_NEW_PICTURE_TO_BE_DISPLAYED, /*!< This event passes a (hdi_av_vid_status_t *) as parameter */
    HDI_AV_VID_EVT_SOURCE_CHANGED, /*!< This event passes a (hdi_av_vid_status_t *) as parameter */
    HDI_AV_VID_EVT_ASPECT_RATIO_CHANGE, /*!< This event passes a (hdi_av_vid_status_t *) as parameter */
    HDI_AV_VID_EVT_FRAME_RATE_CHANGE, /*!< This event passes a (hdi_av_vid_status_t *) as parameter */
    HDI_AV_VID_EVT_STREAM_FORMAT_CHANGE, /*!< This event passes a (hdi_av_vid_status_t *) as parameter */
    HDI_AV_VID_EVT_OUT_OF_SYNC, /*!< This event passes a (hdi_av_vid_status_t *) as parameter */
    HDI_AV_VID_EVT_BACK_TO_SYNC, /*!< This event passes a (hdi_av_vid_status_t *) as parameter */
    HDI_AV_VID_EVT_DATA_ERROR, /*!< This event passes a (hdi_av_vid_status_t *) as parameter */
    HDI_AV_VID_EVT_DATA_OVERFLOW, /*!< This event passes a (hdi_av_vid_status_t *) as parameter */
    HDI_AV_VID_EVT_DATA_UNDERFLOW, /*!< This event passes a (hdi_av_vid_status_t *) as parameter */
    HDI_AV_VID_EVT_IMPOSSIBLE_WITH_MEM_PROFILE, /*!< This event passes a (hdi_av_vid_status_t *) as parameter */
    HDI_AV_VID_EVT_PICTURE_DECODING_ERROR, /*!< This event passes a (hdi_av_vid_status_t *) as parameter */
    HDI_AV_NB_OF_VID_EVT,

    /*audio event*/
    HDI_AV_AUD_EVT_BASE = HDI_AV_NB_OF_VID_EVT,
    HDI_AV_AUD_EVT_DECODE_START = HDI_AV_AUD_EVT_BASE, /*!< This event passes a ( hdi_av_aud_status_t*) as parameter */
    HDI_AV_AUD_EVT_DECODE_STOPPED, /*!< This event passes a ( hdi_av_aud_status_t*) as parameter */
    HDI_AV_AUD_EVT_NEW_FRAME, /*!< This event passes a ( hdi_av_aud_status_t*) as parameter */
    HDI_AV_AUD_EVT_SOURCE_CHANGED, /*!< This event passes a ( hdi_av_aud_status_t*) as parameter */
    HDI_AV_AUD_EVT_DATA_ERROR, /*!< This event passes a ( hdi_av_aud_status_t*) as parameter */
    HDI_AV_AUD_EVT_PCM_UNDERFLOW, /*!< This event passes a ( hdi_av_aud_status_t*) as parameter */
    HDI_AV_AUD_EVT_FIFO_OVERF, /*!< This event passes a ( hdi_av_aud_status_t*) as parameter */
    HDI_AV_AUD_EVT_LOW_DATA_LEVEL, /*!< This event passes a ( hdi_av_aud_status_t*) as parameter */
    HDI_AV_AUD_EVT_OUT_OF_SYNC, /*!< This event passes a ( hdi_av_aud_status_t*) as parameter */
    HDI_AV_AUD_EVT_IN_SYNC, /*!< This event passes a ( hdi_av_aud_status_t*) as parameter */
    HDI_AV_AUD_EVT_FIRST_IN_SYNC, /*!< This event passes a ( hdi_av_aud_status_t*) as parameter */
    HDI_AV_NB_OF_AUD_EVT,
#if 0
    /*inject event*/
    HDI_AV_INJECT_EVT_BASE = HDI_AV_NB_OF_AUD_EVT ,
    HDI_AV_INJECT_EVT_DATA_UNDERFLOW = HDI_AV_INJECT_EVT_BASE , /*!< This event passes a ( hdi_av_inject_status_t*) as parameter */
    HDI_AV_INJECT_EVT_DATA_OVERFLOW, /*!< This event passes a ( hdi_av_inject_status_t*) as parameter */
    HDI_AV_INJECT_EVT_IMPOSSIBLE_WITH_MEM_PROFILE, /*!< This event passes a ( hdi_av_inject_status_t*) as parameter */
    HDI_AV_NB_OF_INJECT_EVT,
#endif
    HDI_AV_NB_OF_EVT = HDI_AV_NB_OF_AUD_EVT
} hdi_av_evt_t;

/*!
 *ֹͣ��Ƶ����ʱ���Ǳ������һ֡���Ǻ��� 
 */
typedef enum hdi_av_vid_stop_mode_e {
    HDI_AV_VID_STOP_MODE_FREEZE = 0, /*!< show the last frame   */
    HDI_AV_VID_STOP_MODE_BLACK, /*!< show the black screen */
    HDI_AV_VID_STOP_MODE_FADE_INOUT, /*!< fade in out */
    HDI_AV_NB_OF_VID_STOP_MODE
} hdi_av_vid_stop_mode_t;

/*!
 * ������ʽ �͸���ֱ���
 * hdi_av_format_t & hdi_av_vid_frame_rate_t ��ϳ���������
 * HDI_AV_FORMAT_PAL , 
 * HDI_AV_FORMAT_NTSC,
 * HDI_AV_FORMAT_PALN,
 * HDI_AV_FORMAT_PALM,
 * HDI_AV_FORMAT_576P ,
 * HDI_AV_FORMAT_HD_720P_24HZ  ,
 * HDI_AV_FORMAT_HD_720P_30HZ  ,
 * HDI_AV_FORMAT_HD_720P_50HZ  ,
 * HDI_AV_FORMAT_HD_720P_60HZ  ,
 * HDI_AV_FORMAT_HD_1080I_50HZ ,
 * HDI_AV_FORMAT_HD_1080I_60HZ ,    
 * HDI_AV_FORMAT_HD_1080P_24HZ ,
 * HDI_AV_FORMAT_HD_1080P_25HZ ,
 * HDI_AV_FORMAT_HD_1080P_30HZ ,
 * HDI_AV_FORMAT_HD_1080P_50HZ ,
 * HDI_AV_FORMAT_HD_1080P_60HZ ,
 */
typedef enum hdi_av_format_e {
    HDI_AV_FORMAT_AUTO = 0,
    HDI_AV_FORMAT_PAL,
    HDI_AV_FORMAT_NTSC,
    HDI_AV_FORMAT_PALN,
    HDI_AV_FORMAT_PALM,
    HDI_AV_FORMAT_576P,
    HDI_AV_FORMAT_HD_720P,
    HDI_AV_FORMAT_HD_1080I,
    HDI_AV_FORMAT_HD_1080P,
    HDI_AV_NB_OF_FORMAT
} hdi_av_format_t;

/*!
 *@brief ��Ŀ��Դ���� 
 * �����Դ����
 * ע��:��ʱ��audio & video�� �����ǲ�ͬ��
 * ԭ����:iframe, pcm �Ȳ��þ���dmxֱ�ӽ���decoder.
 * ��:����ע��iframe��ͬʱ��������tuner��audio
 *        ���߲���ĳ��tuner����file��Դ��video��ͬʱע��pcm����
 */
typedef enum hdi_av_source_type_e {
    HDI_AV_SOURCE_TUNER = 1, /*!< tuner*/
    HDI_AV_SOURCE_FILE = 1 << 1, /*!< file(pvr)*/
    HDI_AV_SOURCE_URL = 1 << 2, /*!< url*/
    HDI_AV_SOURCE_MEM = 1 << 3 /*!< mem ts, ָ����buf,��Ҫ����dmx��ts & mpg����*/
} hdi_av_source_type_t;

/*!
 *@brief  �ڴ�ע���������
 */
typedef enum hdi_av_data_type_e {
    HDI_AV_DATA_TYPE_NONE = 0,
    HDI_AV_DATA_TYPE_TS = 1, /* ���뾭��dmx*/
    HDI_AV_DATA_TYPE_PES = 1 << 1, /*!< mem PES */
    HDI_AV_DATA_TYPE_ES = 1 << 2, /*!< mem ES*/
    HDI_AV_DATA_TYPE_PCM = 1 << 3, /*!< mem PCM */
    HDI_AV_DATA_TYPE_IFRAME = 1 << 4 /*!< mem IFRAME */
} hdi_av_data_type_t;

/*!
 * ����Ƶͬ��ģʽ 
 */
typedef enum hdi_av_sync_mode_e {
    HDI_AV_SYNC_MODE_ENABLE = 0, /*!< enable*/
    HDI_AV_SYNC_MODE_DISABLE, /*!< disable*/
    HDI_AV_SYNC_MODE_AUTO, /*!<auto*/
    HDI_AV_SYNC_MODE_VID, /*!< ��ƵPTSΪʱ�ӻ�׼��ͬ��ģʽ*/
    HDI_AV_SYNC_MODE_AUD, /*!<��ƵPTSΪʱ�ӻ�׼��ͬ��ģʽ*/
    HDI_AV_NB_OF_SYNC_MODE
} hdi_av_sync_mode_t;

/*!
 * �߱���ͨ������
 * ÿ��decoder�ĸ߱���ͨ�����ܶ�������
 */
typedef enum hdi_av_channel_e {
    HDI_AV_CHANNEL_HD = 1, /*!<HD*/
    HDI_AV_CHANNEL_SD = 2 /*!<SD*/
} hdi_av_channel_t;

typedef enum hdi_av_channel_id_e {
    HDI_AV_CHANNEL_ID_HD = 0,
    HDI_AV_CHANNEL_ID_SD,
    HDI_AV_NB_OF_CHANNEL_ID
} hdi_av_channel_id_t;

/*!
 * ��Ƶ��߱�
 * auto�����Ǹ������ʵ��������Զ�����ȷ������
 */
typedef enum hdi_av_aspect_ratio_e {
    HDI_AV_ASPECT_RATIO_AUTO = 0, /*!<auto*/
    HDI_AV_ASPECT_RATIO_16TO9, /*!<16:9*/
    HDI_AV_ASPECT_RATIO_4TO3, /*!<4:3*/
    HDI_AV_NB_OF_ASPECT_RATIO
} hdi_av_aspect_ratio_t;

/*!
 * ����Ӧģʽ
 */
typedef enum hdi_av_aspect_ratio_conversion_e {
    HDI_AV_AR_CONVERSION_PAN_SCAN = 0, /*!<PAN_SCAN*/
    HDI_AV_AR_CONVERSION_LETTER_BOX, /*!<LETTER_BOX*/
    HDI_AV_AR_CONVERSION_COMBINED, /*!<COMBINED*/
    HDI_AV_AR_CONVERSION_IGNORE, /*!<IGNORE*/
    HDI_AV_NB_OF_AR_CONVERSION
} hdi_av_aspect_ratio_conversion_t;

/*!  
 *@brief �������
 * �������Ҫ֧����Ϸ�ʽ
 */
typedef enum hdi_av_output_type_e {
    HDI_AV_OUTPUT_TYPE_NONE = 0x00,
    HDI_AV_OUTPUT_TYPE_COMPOSITE = 0x01, /*!<cvbs*/
    HDI_AV_OUTPUT_TYPE_YPBPR = 0x02, /*!<ypbpr*/
    HDI_AV_OUTPUT_TYPE_SVIDEO = 0x04, /*!<svideo*/
    HDI_AV_OUTPUT_TYPE_DVI = 0x08, /*!<hdmi*/
    HDI_AV_OUTPUT_TYPE_SCART = 0x10, /*!<scart*/
    HDI_AV_OUTPUT_TYPE_VGA = 0x20, /*!<vga*/
    HDI_AV_OUTPUT_TYPE_RF = 0x40 /*!<rf*/
} hdi_av_output_type_t;

/*!
 *@brief ͸���� 
 * @see HDI_AV_ALPHA_MAX & HDI_AV_ALPHA_MIN
 */
typedef U8 hdi_av_alpha_t;

/*!
 *@brief ���(coordinate)
 * Ŀǰʹ�þ�����
 */
typedef U32 hdi_av_coord_t;

typedef enum hdi_av_inject_content_e {
    HDI_AV_INJECT_CONTENT_DEFAULT = 0, /*!<  ȱʡ���� ,��ts���*/
    HDI_AV_INJECT_CONTENT_AUDIO, /*!<  ��Ƶ���*/
    HDI_AV_INJECT_CONTENT_VIDEO, /*!<   ��Ƶ��� */
    HDI_AV_INJECT_CONTENT_SUBTITLE, /*!<   subtitle��� */
    HDI_AV_INJECT_CONTENT_TELETEXT, /*!<  teletext��� */
    HDI_AV_NB_OF_INJECT_CONTENT
} hdi_av_inject_content_t;

typedef enum hdi_av_inject_channel_e {
    HDI_AV_INJECT_CHANNEL_AUD_DECODER = 0,
    HDI_AV_INJECT_CHANNEL_VID_DECODER,
    HDI_AV_INJECT_CHANNEL_DMX,
    HDI_AV_NB_OF_INJECT_CHANNEL
} hdi_av_inject_channel_t;

/*!
 *@CGMS type
 */
typedef enum hdi_av_vbi_cgms_type_s {
    HDI_VBI_CGMS_A = 0,
    HDI_VBI_CGMS_B = 1,
    HDI_VBI_CGMS_MAX_TYPE
} hdi_av_vbi_cgms_type_t;

/*!
 *@CGMS-A permission type
 */
typedef enum hdi_av_vbi_cgms_a_copy_s {
    HDI_VBI_CGMS_A_COPY_PERMITTED = 0, /*copying is permitted without restriction*/
    HDI_VBI_CGMS_A_COPY_ONE_TIME_BEEN_MADE = 1, /*no more copies, one generation copy has been made*/
    HDI_VBI_CGMS_A_COPY_ONE_TIME = 2, /*one generation of copied may be made*/
    HDI_VBI_CGMS_A_COPY_FORBIDDEN = 3, /*no copying is permitted*/
    HDI_VBI_CGMS_A_MAX
} hdi_av_vbi_cgms_a_copy_t;

/*!
 *@ 3D commands
 */
typedef enum hdi_av_3d_mode_s {
    HDI_3D_DISABLE, /*Donot use 3D function*/
    HDI_3D_ENABLE, /*only enable 3D function, no mode is set*/
    /*Set the following modes will cause the 3D function enabled*/
    HDI_3D_MODE_2D_TO_3D,
    HDI_3D_MODE_LR,
    HDI_3D_MODE_BT,
    HDI_3D_MODE_OFF_LR_SWITCH,
    HDI_3D_MODE_ON_LR_SWITCH,
    HDI_3D_MODE_FIELD_DEPTH,
    HDI_3D_MODE_OFF_3D_TO_2D,
    HDI_3D_MODE_L_3D_TO_2D,
    HDI_3D_MODE_R_3D_TO_2D,
    HDI_3D_MODE_OFF_LR_SWITCH_BT,
    HDI_3D_MODE_ON_LR_SWITCH_BT,
    HDI_3D_MODE_OFF_3D_TO_2D_BT,
    HDI_3D_MODE_L_3D_TO_2D_BT,
    HDI_3D_MODE_R_3D_TO_2D_BT,
    HDI_3D_MODE_MAX,
} hdi_av_3d_mode_t;

typedef struct hdi_av_inject_settings_s {
    hdi_av_data_type_t m_data_type;
    hdi_av_inject_content_t m_inject_content; /*!< ��Щ������Ĭ�ϵ�,��pcm=>audio, ���Բ�Ҫ��m_data_type��ͻ*/
    hdi_av_inject_channel_t m_inject_channel; /*!< ��Щͨ����Ĭ�ϵ�,��ò�Ҫ��m_data_type��ͻ,������ЩӦ��hdi�����Լ�����*/
    U32 m_buf_size; /*!< Ϊ��ʱ,hdi�Լ�����*/
    U32 m_inject_min_len; /*!< Ϊ��ʱ,hdi�Լ�����*/
} hdi_av_inject_settings_t;

/*!
 *@brief �ڴ�״̬��Ϣ
 *@see hdi_av_source_params_t
 */
typedef struct hdi_av_buf_status_s {
    hdi_av_data_type_t m_data_type;
    U32 m_size;
    U32 m_free;
    U32 m_used;
    U32 m_inject_min_len;
} hdi_av_buf_status_t;

/*!
 *@brief hdi_av_inject_status_t
 *@see hdi_av_inject_settings_t , hdi_av_buf_status_t
 */
typedef struct hdi_av_inject_status_s {
    handle_t m_inject_handle;
    handle_t m_av_handle;
    hdi_av_buf_status_t m_buf_status;
    id_t m_source_id; /*!<TSע��ʱ,��Ӧ��TS��ͨ��,
     * ��ͬƽ̨����������岻һ��, 
     * Ҫ��hdi_dmx ģ��Լ����
     * ��Ϊdmx��Ҫ������ȡ���*/
} hdi_av_inject_status_t;

typedef struct hdi_av_injecter_open_params_s {
    hdi_av_inject_settings_t m_settings;
} hdi_av_injecter_open_params_t;

typedef struct hdi_av_injecter_close_params_s {
    U8 m_dummy; /*!<  �Ժ���չ�� */
} hdi_av_injecter_close_params_t;

/*!
 *@brief ��Ŀ��Դ���� 
 */
typedef struct hdi_av_source_params_s {
    //hdi_dmx_id_t 					m_demux_id;		/*!< ֻ�е���ݱ��뾭��dmx����Ч,������Щҵ��hdi��Ȩ�Լ�����*/
    U8 m_demux_id;
} hdi_av_source_params_t;

/*!
 *@brief ��Ƶ״̬ 
 * ��ǰ��,������,DAC��״̬,
 */
typedef struct hdi_av_vid_status_s {
    U32 m_packet_count; /*!<ͨ�����Ƿ�����Ƶ�����ȷ����û����Ƶ
     * ��ʹû������CҲ�ܼ�⵽
     */
    U32 m_disp_pic_count; /*!<ͨ������ʾ��Ƶ֡����Ŀ��ȷ����û����Ƶ
     * ��ʱ��ҪCA����,������"��̨ʱֻʹ�ü��ŵ�ES PIDs����,
     * ���ǲ�������PMT(CAû������)" ������������Ȼ�ܼ�⵽�Ƿ�����Ƶ
     */
    hdi_av_decoder_state_t m_decode_state; /*!< decode state*/
    AM_AV_VFormat_t m_stream_type; /*!<stream type*/
    hdi_av_format_t m_stream_format; /*!<stream format*/
    hdi_av_vid_frame_rate_t m_frame_rate; /*!<frame rate*/
    hdi_av_aspect_ratio_t m_aspect_ratio; /*!<aspect ratio*/
    hdi_av_source_type_t m_source_type;
    U32 m_pes_buffer_size; /* pes buffer size             */
    U32 m_pes_buffer_free_size; /* pes buffer size not used    */
    U32 m_es_buffer_size; /* pes buffer size             */
    U32 m_es_buffer_free_size; /* pes buffer size not used    */
    U32 m_stc; /* current STC used by the video decoder */
    U32 m_pts; /* current PTS of the video decoder */
    U32 m_first_pts;
    U32 m_is_interlaced; /*!<is interlaced*/
    U16 m_source_width; /*!<width*/
    U16 m_source_height; /*!<height*/
    U16 m_pid;
} hdi_av_vid_status_t;

/*!
 *@brief av���ò���
 * av���ò�������Ӧ��get��set��������ȡ������
 * ע��:
 * 1. ��Ҫ�޸���Щ����ʱ,����get����,�޸���Ҫ�޸ĵĳ�Ա��set��ȥ
 * set�����������û�иı�ĳ�Ա�����޸�
 *  2.:���ǵ����������,get�����Ĳ�����set��ȥ֮ǰ,�����������Ѿ���set����
 * Ҳ���Ǵ�ʱ��set�п��ܻḲ����������set���������
 */
typedef struct hdi_av_settings_s {
    hdi_av_source_params_t m_source_params;
    hdi_av_sync_mode_t m_av_sync_mode;
    hdi_av_error_recovery_mode_t m_err_recovery_mode;
    hdi_av_vid_stop_mode_t m_vid_stop_mode;
    hdi_av_spdif_mode_t m_spdif_mode; /*!< Ϊ�˼����ϴ���,���ȼ���m_aud_op_mode��*/
    AM_AV_VFormat_t m_vid_stream_type;
    AM_AV_AFormat_t m_aud_stream_type;
    AM_AV_VFormat_t m_vtype;
    AM_AV_AFormat_t m_atype;
    hdi_av_aud_channel_t m_aud_channel;
    hdi_av_aud_op_mode_t m_aud_op_mode; /*!< ���m_aud_op_mode == HDI_AUD_OP_MODE_BYPASS,m_spdif_mode ģʽ���Զ���ΪHDI_AV_SPDIF_MODE_AC3 */
    U32 m_is_mute; /*!<is mute*/
    U32 m_is_vid_decode_once; /*!< FALSE : ������,���������������; TRUE :ֻ���һ��ͼƬ,��Ҫ����Ԥ��*/
    U32 m_left_vol; /*!<left vol*/
    U32 m_right_vol; /*!<right vol*/
    S32 m_speed; /*!<(%), The trick modes consist in rewind, forward at slow and fast speed.*/
    U16 m_pcr_pid; /*!<if(pid==HDI_INVALID_PID)���ƽ̨֧��,��clear��ͨ����pid*/
    U16 m_aud_pid; /*!<if(pid==HDI_INVALID_PID)���ƽ̨֧��,��clear��ͨ����pid*/
    U16 m_vid_pid; /*!<if(pid==HDI_INVALID_PID)���ƽ̨֧��,��clear��ͨ����pid*/
    S16 m_aud_sync_offset_ms;
    S16 m_vid_sync_offset_ms;
} hdi_av_settings_t;

/*!
 *@brief Ĭ����ʾ����
 * �߱���Ҫ�ܶ�������
 *@see hdi_av_format_t, hdi_av_aspect_ratio_t,  hdi_av_vid_frame_rate_t
 */
typedef struct hdi_av_default_dispsettings_s {
    hdi_av_format_t m_disp_fmt; /*!<��Ҫ��Ϊ�Զ�ģʽ*/
    hdi_av_vid_frame_rate_t m_frame_rate; /*!<��Ҫ��Ϊ�Զ�ģʽ*/
    hdi_av_aspect_ratio_t m_aspect_ratio; /*!<��Ҫ��Ϊ�Զ�ģʽ*/
} hdi_av_default_dispsettings_t;

/*!
 *@brief ��ʾ����,
 * �߱���Ҫ�ܶ�������
 * ��Щ��������Ӧ��get��set��������ȡ������
 * ע��,��Ҫ�޸���Щ����ʱ,����get����,�޸���Ҫ�޸ĵĳ�Ա��set��ȥ
 * set�����������û�иı�ĳ�Ա�����޸�,
 * ע��:���ǵ����������,get�����Ĳ�����set��ȥ֮ǰ,�����������Ѿ���set����
 * Ҳ���Ǵ�ʱ��set�п��ܻḲ����������set���������
 * ע��:brightness,contrast,saturation����Щƽ̨�ϻ�Ӱ�쵽��Ƶָ��,�����޸��Ժ�������Ĭ��ֵ
 * Ϊ�˲�Ӱ�쵽��Ƶָ��,hdi_av_init()��������洫������m_brightness,m_contrast,m_saturation,������
 * �����get��������ϵͳ��Ĭ��ֵ,���鱣���ָ�����������.
 * hdi_av_display_set()����Żᴦ�����洫������m_brightness,m_contrast,m_saturation,�޸�brightness,contrast,saturation.
 *@see hdi_av_format_t, hdi_av_aspect_ratio_t, hdi_av_aspect_ratio_conversion_t, hdi_av_output_type_t, hdi_av_alpha_t
 *@see HDI_AV_SATURATION_MAX, HDI_AV_SATURATION_MIN;HDI_AV_CONTRAST_MAX,HDI_AV_CONTRAST_MIN;
 *@see HDI_AV_BRIGHTNESS_MAX, HDI_AV_BRIGHTNESS_MIN;HDI_AV_ALPHA_MAX, HDI_AV_ALPHA_MIN
 */
typedef struct hdi_av_dispsettings_s {
    hdi_av_format_t m_disp_fmt; /*!<Ĭ��ΪHDI_AV_FORMAT_AUTO*/
    hdi_av_vid_frame_rate_t m_frame_rate; /*!<frame rate, Ĭ��ΪHDI_AV_VID_FRAME_RATE_AUTO*/
    hdi_av_aspect_ratio_t m_aspect_ratio; /*!<Ĭ��ΪHDI_AV_ASPECT_RATIO_AUTO*/
    hdi_av_aspect_ratio_conversion_t m_aspect_ratio_conv; /*!<Ĭ��ΪHDI_AV_AR_CONVERSION_LETTER_BOX*/
    hdi_av_output_type_t m_output_type;
    U32 m_is_enable_output; /*!<�Ƿ������,ֻ�Ե�ǰͨ����Ч, Ĭ��Ϊtrue*/
    hdi_av_alpha_t m_alpha;
    hdi_av_aout_device_t m_aout_mute_device; /*!< ָ����Щ��Ƶ���ͨ��Ϊ����(֧�ֻ����)*/
    U8 m_brightness; /*!<To adjust the luminance intensity of the display video image.This value is unsigned and scaled between 0 to 100.Default value are 50.*/
    U8 m_contrast; /*!<To adjust the relative difference between higher and lower intensity luminance values of the display image.This value is unsigned and scaled between 0 to 100.Default value are 50.*/
    U8 m_saturation; /*!<To adjust the color intensity of the displayed video image.This value is unsigned and scaled between 0 to 100.Default value are 50.*/
} hdi_av_dispsettings_t;

/*!
 * iframe ����(��ݼ��䳤��)  
 */
typedef struct hdi_av_iframe_paras_s {
    U32 m_data_length; /*!<��ݳ���*/
    void * m_p_iframe_data; /*!<���*/
} hdi_av_iframe_paras_t;

/*!
 *@brief IFrameDecode 
 *@see hdi_av_iframe_paras_t
 */
typedef struct hdi_av_iframe_decode_params_s {
    hdi_av_iframe_paras_t m_iframe_params;
} hdi_av_iframe_decode_params_t;

/*!
 *@brief  av �¼��ص���������˵��
 * ע��:�˺���������Ե��ñ�ģ���api
 * Ҳ����˵���ڻص�������ʱ��Ҫͬʱ���ǵ������������������
 *@return void
 *@param av_handle ʵ����,��ʶ���ĸ�ʵ���ϵ��¼�
 *@param  evt: hdi_av_evt_t 
 *@param p_data: const void * const ��ο� hdi_av_evt_t ��˵��
 *@see hdi_av_evt_t
 */
//typedef void (* hdi_av_callback_pfn_t)(const handle_t  av_handle, const hdi_av_evt_t evt, const void * const p_data);
/*!
 *@brief hdi_av_evt_config_params_t 
 *@see hdi_av_evt_t, hdi_av_callback_pfn_t
 */
typedef struct Tapi_av_evt_config_params_s {
    EN_VDEC_EVENT_FLAG m_evt; /*!< av �¼�,��ʾ�����ö��ĸ��¼���Ч*/
    VDEC_EventCb m_p_callback; /*!< ��m_p_callbackΪ��ʱ,����Ѿ�ע���˻ص��������Ƴ�,ȡ���ע��*/
    U32 m_is_enable; /*!< ��ʾ�Ƿ�ʹ�ܸ��¼��ص�*/
    U32 m_notifications_to_skip; /*!< ��ʾ�ڵ���ע��Ļص�����֮ǰ,��Ҫ���η���ĸ��¼�*/
    void * udata;
} Tapi_av_evt_config_params_t;

/*!
 * ��Ƶģ���ʼ������ 
 */
typedef struct hdi_av_init_params_s {
    hdi_malloc_pfn_t m_p_malloc; /*!<  ��ģ���ڴ涯̬���亯��ָ��*/
    hdi_free_pfn_t m_p_free; /*!<  ��̬�ڴ��ͷź���ָ��*/
    /*!<  ֻҪ����������һ��Ϊ����ʹ��malloc & free */
    U32 m_tapriority; /*!<  av �������ȼ�����ƽ̨������ͳһ����*/
    U32 m_is_enable_disp; /*!< ture:��ʼ��ͬʱenable disp���(av, osd ,still, etc.); false: ��ʼ����û�����(av, osd, still, etc.)
     *���Ǵ����, ���״̬������û�����
     */
    hdi_av_dispsettings_t m_disp_settings[HDI_AV_NB_OF_CHANNEL_ID]; /*!<����ֵ,�Ժ���Զ�̬�޸�@see hdi_av_channel_id_t */
    hdi_av_default_dispsettings_t m_default_dispsettings[HDI_AV_NB_OF_CHANNEL_ID]; /*!<����ֵ,�Ժ���Զ�̬�޸�@see hdi_av_channel_id_t */
} hdi_av_init_params_t;

/*!
 * ��һ��ʵ������� 
 */
typedef struct hdi_av_open_params_s {
    hdi_av_source_params_t m_source_params; /*!<����ֵ,�Ժ���Զ�̬�޸�*/
} hdi_av_open_params_t;

/*!
 * �ر�ʵ����� 
 */
typedef struct hdi_av_close_params_s {
    U8 m_dummy; /*!<  �Ժ���չ�� */
} hdi_av_close_params_t;

/*!
 * AVģ����ֹ���� 
 */
typedef struct hdi_av_term_params_s {
    U8 m_dummy; /*!< �Ժ���չ�� */
} hdi_av_term_params_t;

/*!
 *@brief ����PCM��ݲ���Ľṹ����
 */
typedef struct hdi_av_pcm_param_s {
    U32 m_sample_rate; /*!<��Ƶ������ (32000,44100,48000 )*/
    U32 m_bit_width; /*!< ��Ƶ����λ��(8 or 16 ) */
    hdi_av_aud_channel_t m_aud_channel;
    U32 m_is_big_endian;
} hdi_av_pcm_param_t;

typedef struct hdi_av_disp_capability_s {
    hdi_av_output_type_t m_vout_type; /*!<  OR of supported ones */
    hdi_av_aout_device_t m_aout_device; /*!<  OR of supported ones */
    U32 m_input_window_height_min; /*!<  value in pixels */
    U32 m_input_window_width_min; /*!<  value in pixels */
    U32 m_output_window_height_min; /*!<  value in pixels */
    U32 m_output_window_width_min; /*!<  value in pixels */
    hdi_av_format_t m_av_format_arr[HDI_AV_NB_OF_FORMAT]; /*!< ��hdi_av_format_tΪ����,��ֵ��֧�ֵ�hdi_av_vid_frame_rate_t�����������ֵ,
     * ��ֵΪ0.���ʾ��֧�����ֱַ���*/
} hdi_av_disp_capability_t;

typedef struct hdi_av_es_param_s {
    U32 m_sample_rate; /*!<������ */
} hdi_av_es_param_t;

typedef struct hdi_av_es_data_s {
    U32 m_timestamp; /* !< ʱ��� */
    void * m_p_buf; /* !< ��ݻ������ַ */
    U32 m_len; /* !< ������� */
    hdi_free_pfn_t m_p_free; /*!<  m_p_buf �ڴ��ͷź���ָ��*/
    /*!<  ע��:���m_p_buf �ڴ治�Ƕ�̬�����*/
    /*!<  ������m_p_buf �ڴ治��ҪHDI�ͷ�*/
    /*!<  ���m_p_free�ÿ�*/
    /*!<  ���hdi_av_inject_es_data()���غ�,m_p_buf��Ϊ��,*/
    /*!<  ����Ҫapp��m_p_buf �ͷ�,�����ڴ�й©*/
    /*!<  ͬʱHDI ���뱣֤�ͷ���m_p_buf �ڴ��,�����m_p_buf �ÿ�,*/
    /*!<  ���⵼��m_p_buf ���ͷ�����*/
} hdi_av_es_data_t;

void am_send_all_event(EN_VDEC_EVENT_FLAG evt);



status_code_t hdi_av_start(const handle_t av_handle);
TAPI_BOOL am_av_enable_video(const handle_t av_handle, const hdi_av_channel_t channel);
TAPI_BOOL hdi_av_disable_video(const handle_t av_handle, const hdi_av_channel_t channel);
status_code_t hdi_av_set(const handle_t av_handle, hdi_av_settings_t * const p_settings);
TAPI_U16 Tapi_Vdec_GetFrameCnt(void);
#endif//__AV_LOCAL_H__
