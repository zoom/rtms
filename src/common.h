
#ifndef RTMS_JS_SDK_COMMON_H
#define RTMS_JS_SDK_COMMON_H

#include "napi.h"

class n_session_info: public Napi::ObjectWrap<n_session_info> {};

class n_audio_param : public Napi::ObjectWrap<n_audio_param> {};

class n_video_param : public Napi::ObjectWrap<n_video_param>{};


#endif //RTMS_JS_SDK_COMMON_H
