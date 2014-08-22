LOCAL_PATH:= $(call my-dir)

#build ffapi library

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES:=                \
                usb_dongle/usb_ts_play.c \
  
LOCAL_SHARED_LIBRARIES:= \
    libui \
    libutils \
    libcutils \
    libbinder \
    libmedia \
    libam_adp \

LOCAL_MODULE:= libhdiapi

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/include/tcl/ \
    $(LOCAL_PATH)/include \
    $(LOCAL_PATH)/../include/am_adp \
    $(LOCAL_PATH)/../android/ndk/include/ \
    $(LOCAL_PATH)/../../../vendor/amlogic/dvb/include/am_adp \
    $(LOCAL_PATH)/../../../vendor/amlogic/dvb/android/ndk/include \
    $(LOCAL_PATH)/../../../vendor/amlogic/dvb/android/ndk/include/linux

LOCAL_LDLIBS  += -L$(SYSROOT)/usr/lib -llog

LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)
