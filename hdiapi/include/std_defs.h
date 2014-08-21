/*!
 *@file std_defs.h	
 *@brief std_defs
 * Version:	 0.0.1
 * Description:  
 * Copyright (c) 2009, SkyworthDTV
 * All rights reserved.				
 *@author skyworth 
 *@date 2009-04-09 
 */

#ifndef STD_DEFS_H
#define STD_DEFS_H

#ifdef __cplusplus
extern "C" {
#endif

/*! Common unsigned types */
#ifndef DEFINED_U8
#define DEFINED_U8
typedef unsigned char U8;
#endif

#ifndef DEFINED_U16
#define DEFINED_U16
typedef unsigned short U16;
#endif

#ifndef DEFINED_U32
#define DEFINED_U32
typedef unsigned long U32;
#endif

#ifndef DEFINED_U64
#define DEFINED_U64
typedef unsigned long long U64;
#endif

/*! Common signed types */
#ifndef DEFINED_S8
#define DEFINED_S8
typedef signed char S8;
#endif

#ifndef DEFINED_S16
#define DEFINED_S16
typedef signed short S16;
#endif

#ifndef DEFINED_S32
#define DEFINED_S32
typedef signed long S32;
#endif

#ifndef DEFINED_S64
#define DEFINED_S64
typedef signed long long S64;
#endif

#ifndef		NULL
#define		NULL			0
#endif

#ifndef TRUE 
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 	0
#endif
typedef void * (*hdi_malloc_pfn_t)(U32 size);
typedef void (*hdi_free_pfn_t)(void * p_mem);

/*! ʱ�䶨�� */
typedef U32 am_clock_t;

/*! Revision structure */
typedef const char * revision_t;

/*! id */
typedef unsigned long am_id_t;

/*! handle */
typedef unsigned long handle_t;

typedef unsigned long key_code_t;

#define INVALID_ID			((U32)(-1))
#define INVALID_HANDLE		((U32)(-1))

#define OS_LINUX

/*! error code  */
typedef enum status_code_e {
    SUCCESS = 0,
    ERROR_BAD_PARAMETER, /* Bad parameter passed       */
    ERROR_NO_MEMORY, /* Memory allocation failed   */
    ERROR_UNKNOWN_DEVICE, /* Unknown device name        */
    ERROR_ALREADY_INITIALIZED, /* Device already initialized */
    ERROR_NO_FREE_HANDLES, /* Cannot open device again   */
    ERROR_INVALID_HANDLE, /* Handle is not valid        */
    ERROR_INVALID_ID, /* ID is not valid        */
    ERROR_FEATURE_NOT_SUPPORTED, /* Feature unavailable        */
    ERROR_INTERRUPT_INSTALL, /* Interrupt install failed   */
    ERROR_INTERRUPT_UNINSTALL, /* Interrupt uninstall failed */
    ERROR_TIMEOUT, /* Timeout occured            */
    ERROR_DEVICE_BUSY, /* Device is currently busy   */
    ERROR_NO_INIT, /* not init    */
    ERROR_MAX_COUNT, /* that more than max count    */
    FAILED, /* UNKNOWN ERROR   */
    STATUS_ENABLE,
    STATUS_DISABLE
} status_code_t;

/*! week days  */
typedef enum week_days_s {
    MONDAY = 1,
    TUESDAY,
    WEDNESDAY,
    THURSDAY,
    FRIDAY,
    SATURDAY,
    SUNDAY
} week_days_t;

/*! time  */
typedef struct time_s {
    U16 m_year; /*!< Decimal value*/
    U8 m_month; /*!< [1..12]*/
    U8 m_day; /*!< [1..31]*/
    U8 m_hour; /*!< [0..23]*/
    U8 m_minute; /*!< [0..59]*/
    U8 m_seconds; /*!< [0..59]*/
    week_days_t m_day_of_the_week; /*!@see week_days_t*/
} am_time_t;

/*! MULTI LANGUAGE  */
typedef enum lang_type_s {
    LANG_CHINA, /*!<"China"*/
    LANG_ENG, /*!<"English"*/
    LANG_FIN, /*!<"Finnish"*/
    LANG_FRAFRE, /*!<"French"*/
    LANG_DEUGER, /*!<"German"*/
    LANG_ELLGRE, /*!<"Greek"*/
    LANG_HUN, /*!<"Hungarian"*/
    LANG_ITA, /*!<"Italian"*/
    LANG_NOR, /*!<"Norwegian"*/
    LANG_POL, /*!<"Polish"*/
    LANG_POR, /*!<"Portuguese"*/
    LANG_RONRUM, /*!<"Romanian"*/
    LANG_RUS, /*!<"Russian"*/
    LANG_SLV, /*!<"Slovenian"*/
    LANG_ESLSPA, /*!<"Spanish"*/
    LANG_SVESWE, /*!<"Swedish"*/
    LANG_TUR, /*!<"Turkish"*/
    LANG_ARA, /*!<"Arabic"*/
    LANG_CHIZHO, /*!<"Chinese"*/
    LANG_CESCZE, /*!<"Czech"*/
    LANG_DAN, /*!<"Danish"*/
    LANG_DUTNLA, /*!<"Dutch"*/
    NB_OF_LANG
} lang_type_t;

#ifdef __cplusplus
}
#endif

#endif /*STD_DEFS_H*/

/*eof*/

