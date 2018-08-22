//
// Created by matt on 8/22/18.
//

#ifndef INC_2050_LOG_HPP
#define INC_2050_LOG_HPP

#include <android/log.h>

#ifdef NDEBUG
#define LOG_DEBUG_WRITE(TAG, TXT)
#define LOG_DEBUG_PRINT(TAG, FMT, ...)
#else
#define LOG_DEBUG_WRITE(TAG, TXT) do {__android_log_write(ANDROID_LOG_DEBUG, TAG, TXT); } while(false)
#define LOG_DEBUG_PRINT(TAG, FMT, ...) do {__android_log_print(ANDROID_LOG_DEBUG, TAG, FMT, __VA_ARGS__); } while(false)
#endif

#define LOG_ERROR_WRITE(TAG, TXT) do {__android_log_write(ANDROID_LOG_ERROR, TAG, TXT); } while(false)
#define LOG_ERROR_PRINT(TAG, FMT, ...) do {__android_log_print(ANDROID_LOG_ERROR, TAG, FMT, __VA_ARGS__); } while(false)

#endif //INC_2050_LOG_HPP
