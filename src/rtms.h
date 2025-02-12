
#ifndef RTMS_JS_SDK_RTMS_H
#define RTMS_JS_SDK_RTMS_H

#include <napi.h>
#include <functional>
#include <sstream>
#include "rtms_csdk.h"

using namespace std;

class RTMS {

    typedef void (*onJoinConfirmFunc)(struct rtms_csdk *sdk, int reason);
    typedef void (*onSessionUpdateFunc)(struct rtms_csdk *sdk, int op, struct session_info *sess);
    typedef void (*onAudioDataFunc)(struct rtms_csdk *sdk, unsigned char *buf, int size, unsigned int timestamp, struct rtms_metadata *md);
    typedef void (*onVideoDataFunc)(struct rtms_csdk *sdk, unsigned char *buf, int size, unsigned int timestamp, const char *rtms_session_id, struct rtms_metadata *md);
    typedef void (*onTranscriptDataFunc)(struct rtms_csdk *sdk, unsigned char *buf, int size, unsigned int timestamp, struct rtms_metadata *md);
    typedef void (*onLeaveFunc)(struct rtms_csdk *sdk, int reason);

    rtms_csdk *_sdk;
    media_parameters _mediaParam;
    audio_parameters _audioParam;
    video_parameters _videoParam;

    rtms_csdk_ops _options;

    onJoinConfirmFunc _onJoinConfirm;
    onSessionUpdateFunc _onSessionUpdate;
    onAudioDataFunc _onAudioData;
    onVideoDataFunc _onVideoData;
    onTranscriptDataFunc _onTranscriptData;
    onLeaveFunc _onLeave;

    bool _isRunning;
    bool _useVideo;
    bool _useAudio;
    bool _useTranscript;

public:
    RTMS();
    ~RTMS();

    int init(const string& ca_path);

    int join(const string& uuid, const string& session_id, const string& signature, const string& signal_url, const int& timeout = -1);

    void stop();

    void enableTranscript(bool useTranscript);
    void enableAudio(bool use_audio);
    void enableVideo(bool use_video);

    void setMediaTypes(bool audio, bool video, bool transcript);
    void setAudioParam(const audio_parameters& param);
    void setVideoParam(const video_parameters& param);

    void setOnJoinConfirm(onJoinConfirmFunc f);
    void setOnSessionUpdate(onSessionUpdateFunc f);
    void setOnAudioData(onAudioDataFunc f);
    void setOnVideoData(onVideoDataFunc f);
    void setOnTranscriptData(onTranscriptDataFunc f);
    void setOnLeave(onLeaveFunc f);

    static bool checkErr(const int& code, const string& message, bool except=true) {
        auto isOk = code == RTMS_SDK_OK;
        ostringstream out;
        out << "error (" << code << "): " << message << endl;

        if (!isOk && except)
            throw logic_error(out.str());

        return isOk;
    };
};


#endif //RTMS_JS_SDK_RTMS_H
