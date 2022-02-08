// Copyright 2022 Matthew Chandler

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

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
