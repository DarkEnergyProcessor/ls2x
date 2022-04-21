LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE    := ls2xlib
LOCAL_MODULE_FILENAME := ls2xlib

LOCAL_CFLAGS    := -DNOMINMAX -fopenmp -DLS2X_USE_KISSFFT -Dkiss_fft_scalar=double
LOCAL_CPPFLAGS  := -std=c++11
LOCAL_LDFLAGS   := -fopenmp

LOCAL_ARM_NEON := true

LOCAL_C_INCLUDES := \
	${LOCAL_PATH}/src

LOCAL_SRC_FILES := \
	src/main.cpp \
	src/audiomix.cpp \
	src/fft.cpp \
	src/kissfft/kiss_fft.c \
	src/kissfft/kiss_fftr.c

LOCAL_SHARED_LIBRARIES := liblove

include $(BUILD_SHARED_LIBRARY)
