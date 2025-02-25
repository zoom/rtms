#ifndef RTMS_H
#define RTMS_H

#include <functional>
#include <sstream>
#include <thread>
#include "rtms_csdk.h"

using namespace std;

class RTMS
{
    rtms_csdk* _sdk;
    media_parameters _mediaParam;
    audio_parameters _audioParam;
    video_parameters _videoParam;

    rtms_csdk_ops _options;

    fn_on_join_confirm _onJoinConfirm;
    fn_on_session_update _onSessionUpdate;
    fn_on_user_update _onUserUpdate;
    fn_on_audio_data _onAudioData;
    fn_on_video_data _onVideoData;
    fn_on_transcript_data _onTranscriptData;
    fn_on_leave _onLeave;

    bool _isInit;
    bool _isRunning;
    bool _useVideo;
    bool _useAudio;
    bool _useTranscript;
public:
    RTMS();
    ~RTMS();

    int init(const string &ca);

    int join(const string &uuid, const string &streamId, const string &signature, const string &serverUrls, const int &timeout = -1);

    void stop();

    bool isInit() const;
    bool isRunning() const;

    void useTranscript(bool useTranscript);
    void useAudio(bool useAudio);
    void useVideo(bool useVideo);

    void setMediaTypes(bool audio, bool video, bool transcript);
    void setAudioParam(const audio_parameters &param);
    void setVideoParam(const video_parameters &param);

    void setOnJoinConfirm(fn_on_join_confirm f);
    void setOnSessionUpdate(fn_on_session_update f);
    void setOnUserUpdate(fn_on_user_update f);
    void setOnAudioData(fn_on_audio_data f);
    void setOnVideoData(fn_on_video_data f);
    void setOnTranscriptData(fn_on_transcript_data f);
    void setOnLeave(fn_on_leave f);

    static bool checkErr(const int &code, const string &message, bool except = true);
};

#endif // RTMS_H
