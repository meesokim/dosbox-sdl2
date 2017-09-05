
# Uncomment this if you're using STL in your project
# See CPLUSPLUS-SUPPORT.html in the NDK documentation for more information
# APP_STL := stlport_static 
APP_STL := stlport_shared
APP_ABI := armeabi-v7a
ANDROID_ABI := "armeabi-v7a with NEON"
#ANDROID_ABI := arm64-v8a
#TARGET_ARCH_ABI := arm64-v8a
ANDROID_ARM_NEON := true
LOCAL_ARM_NEON := true
ANDROID_ARM_VFP := hard
LOCAL_CXXFLAGS  := -O2 -mhard-float -mfloat-abi=hard -mfpu=neon
LOCAL_CFLAGS    := -O2 -mhard-float -mfloat-abi=hard -mfpu=neon
LOCAL_LDFLAGS   := -lm_hard