
#ifndef RTMS_JS_SDK_RTMS_H
#define RTMS_JS_SDK_RTMS_H

#include <functional>
#include <sstream>
#include <thread>
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

    bool _isInit;
    bool _isRunning;
    bool _useVideo;
    bool _useAudio;
    bool _useTranscript;

public:
    RTMS();
    ~RTMS();

    int init(const string& ca);

    int join(const string& uuid, const string& sessionId, const string& signature, const string& serverUrls, const int& timeout = -1);

    void stop();

    bool isInit() const;
    bool isRunning() const;

    void enableTranscript(bool useTranscript);
    void enableAudio(bool useAudio);
    void enableVideo(bool useVideo);

    void setMediaTypes(bool audio, bool video, bool transcript);
    void setAudioParam(const audio_parameters& param);
    void setVideoParam(const video_parameters& param);

    void setOnJoinConfirm(onJoinConfirmFunc f);
    void setOnSessionUpdate(onSessionUpdateFunc f);
    void setOnAudioData(onAudioDataFunc f);
    void setOnVideoData(onVideoDataFunc f);
    void setOnTranscriptData(onTranscriptDataFunc f);
    void setOnLeave(onLeaveFunc f);

    typedef function<void(int)> onJoinConfirmFn;
    typedef function<void(int, struct session_info* session)> onSessionUpdateFn;
    typedef function<void(unsigned char*, int, unsigned int, struct rtms_metadata*)> onAudioDataFn;
    typedef function<void(unsigned char*, int, unsigned int, const char*, struct rtms_metadata*)> onVideoDataFn;
    typedef function<void(unsigned char*, int, unsigned int, struct rtms_metadata*)> onTranscriptDataFn;
    typedef function<void(int)> onLeaveFn;

    static bool checkErr(const int& code, const string& message, bool except=true) {
        auto isOk = code == RTMS_SDK_OK;
        ostringstream out;
        out << "(" << code << "): " << message << endl;

        if (!isOk && except)
            throw logic_error(out.str());

        return isOk;
    };
};


#endif //RTMS_JS_SDK_RTMS_H
