#include "rtms.h"

RTMS::RTMS() {
    _mediaParam.audio_param = &_audioParam;
    _mediaParam.video_param = &_videoParam;
}

int RTMS::init(const string &ca_path) {
    if (_isInit) 
        return 0;

    auto ret = rtms_init(ca_path.c_str());
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

    rtms_set_callbacks(_sdk, &_options);
    checkErr(ret, "failed to set callbacks for RTMS CSDK");

    _isInit = ret;

    return ret;
}

int RTMS::join(const string &uuid, const string &session_id, const string &signature, const string &signal_url,
               const int &timeout) {

    auto ret = rtms_join(_sdk, uuid.c_str(), session_id.c_str(), signature.c_str(), signal_url.c_str(), timeout);

    _isRunning = checkErr(ret, "failed to join event " + uuid);

    while (_isRunning)
        checkErr(rtms_poll(_sdk), "unable to poll RTMS CSDK");

    return ret;
}

void RTMS::stop() {
    _isRunning = false;
}

bool RTMS::isInit() const
{
    return _isInit;
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

void RTMS::enableAudio(bool use_audio) {
    _useAudio = use_audio;
}

void RTMS::enableVideo(bool use_video) {
    _useVideo = use_video;
}

void RTMS::setAudioParam(const audio_parameters &param) {
    _audioParam = param;
}

void RTMS::setVideoParam(const video_parameters &param) {
    _videoParam = param;
}

void RTMS::setOnJoinConfirm(RTMS::onJoinConfirmFunc f) {
    _onJoinConfirm = f;
    _options.on_join_confirm = _onJoinConfirm;
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


