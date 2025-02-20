#include "rtms.h"
#include <iostream>
#include <future>


RTMS::RTMS() {
    _mediaParam.audio_param = &_audioParam;
    _mediaParam.video_param = &_videoParam;
}

int RTMS::init(const string &ca) {
    if (_isInit) 
        return 0;

    auto ret = rtms_init(ca.c_str());
    checkErr(ret, "failed to init RTMS CSDK");

    _sdk = rtms_alloc();

    ret = rtms_config(_sdk, NULL, SDK_ALL, 0);
    checkErr(ret, "failed to configure RTMS CSDK");

    _options.on_join_confirm = _onJoinConfirm;
    _options.on_session_update = _onSessionUpdate;
    _options.on_audio_data = _onAudioData;
    _options.on_video_data = _onVideoData;
    _options.on_transcript_data = _onTranscriptData;
    _options.on_leave = _onLeave;

    ret = rtms_set_callbacks(_sdk, &_options);
    _isInit = checkErr(ret, "failed to set callbacks for RTMS CSDK");

    return 0;
}

int RTMS::join(const string &uuid, const string &sessionId, const string &signature, const string &serverUrls,
               const int &timeout) {

    auto ret = rtms_join(_sdk, uuid.c_str(), sessionId.c_str(), signature.c_str(), serverUrls.c_str(), timeout);

    _isRunning = checkErr(ret, "failed to join event " + uuid);

    while(_isRunning)
        _isRunning = checkErr(rtms_poll(_sdk), "failed to poll csdk");

    return 0;
}

void RTMS::stop() 
{
    _isRunning = false;
}

bool RTMS::isInit() const
{
    return _isInit;
}

bool RTMS::isRunning() const
{
    return _isRunning;
}

RTMS::~RTMS() {
    rtms_release(_sdk);
    rtms_uninit();
}

void RTMS::setMediaTypes(bool audio, bool video = false, bool transcript = false) {
    enableAudio(audio);
    enableVideo(video);
    enableTranscript(transcript);
}

void RTMS::enableTranscript(bool useTranscript) {
    _useTranscript = useTranscript;
}

void RTMS::enableAudio(bool useAudio) {
    _useAudio = useAudio;
}

void RTMS::enableVideo(bool useVideo) {
    _useVideo = useVideo;
}

void RTMS::setAudioParam(const audio_parameters &param) {
    _audioParam = param;
}

void RTMS::setVideoParam(const video_parameters &param) {
    _videoParam = param;
}

void RTMS::setOnJoinConfirm(RTMS::onJoinConfirmFunc f) {
    _onJoinConfirm = f;
    _options.on_join_confirm = f;
}

void RTMS::setOnSessionUpdate(RTMS::onSessionUpdateFunc f) {
    _onSessionUpdate = f;
    _options.on_session_update = _onSessionUpdate;
}

void RTMS::setOnAudioData(RTMS::onAudioDataFunc f) {
    _onAudioData = f;
    _options.on_audio_data = _onAudioData;
}

void RTMS::setOnVideoData(RTMS::onVideoDataFunc f) {
    _onVideoData = f;
    _options.on_video_data = _onVideoData;
}

void RTMS::setOnTranscriptData(RTMS::onTranscriptDataFunc f) {
    _onTranscriptData = f;
    _options.on_transcript_data = _onTranscriptData;
}

void RTMS::setOnLeave(RTMS::onLeaveFunc f) {
    _onLeave = f;
    _options.on_leave = _onLeave;
}


