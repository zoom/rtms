
#ifndef RTMS_JS_SDK_RTMS_H
#define RTMS_JS_SDK_RTMS_H

#include "napi.h"
#include <functional>
#include "rtms_csdk.h"

using namespace std;



class RTMS {
public:
    RTMS(const string& ca_path);
    ~RTMS();

    int join(const string& uuid, const string& session_id, const string& signature, const string& signal_url, const int& timeout = -1);

    void enableTranscript(bool useTranscript);
    void enableAudio(bool use_audio);
    void enableVideo(bool use_video);

    void setMediaTypes(bool audio, bool video, bool transcript);
    void setAudioParam(const audio_parameters& param);
    void setVideoParam(const video_parameters& param);

    typedef void (*onJoinConfirmFunc)(struct rtms_csdk *sdk, int reason);
    typedef void (*onSessionUpdateFunc)(struct rtms_csdk *sdk, int op, struct session_info *sess);
    typedef void (*onAudioDataFunc)(struct rtms_csdk *sdk, unsigned char *buf, int size, unsigned int timestamp, const char *user_name, int user_id);
    typedef void (*onVideoDataFunc)(struct rtms_csdk *sdk, unsigned char *buf, int size, unsigned int timestamp, const char *user_name, int user_id, const char *rtms_session_id);
    typedef void (*onTranscriptDataFunc)(struct rtms_csdk *sdk, unsigned char *buf, int size, unsigned int timestamp, const char *user_name, int user_id);
    typedef void (*onLeaveFunc)(struct rtms_csdk *sdk, int reason);

    [[nodiscard]] onJoinConfirmFunc getOnJoinConfirm() const;
    void setOnJoinConfirm(onJoinConfirmFunc f);

    [[nodiscard]] onSessionUpdateFunc getSessionUpdate() const;
    void setOnSessionUpdate(onSessionUpdateFunc f);

private:
    rtms_csdk *m_sdk;
    media_parameters m_mediaParam;
    audio_parameters m_audioParam;
    video_parameters m_videoParam;

    rtms_csdk_ops m_options;

    onJoinConfirmFunc m_onJoinConfirm;
    onSessionUpdateFunc m_onSessionUpdate;
    onAudioDataFunc m_onAudioData;
    onVideoDataFunc m_onVideoData;
    onTranscriptDataFunc m_onTranscriptData;
    onLeaveFunc m_onLeave;

    bool m_isRunning;
    bool m_useVideo;
    bool m_useAudio;
    bool m_useTranscript;
};


#endif //RTMS_JS_SDK_RTMS_H
