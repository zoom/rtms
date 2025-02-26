#include "rtms.h"

using namespace rtms;

Client::Client() {
    _mediaParam.audio_param = &_audioParam;
    _mediaParam.video_param = &_videoParam;
}

Client::~Client() {
    rtms_release(_sdk);
    rtms_uninit();
}

int Client::init(const string &ca) {
    if (_isInit) 
        return 0;

    auto ret = rtms_init(ca.c_str());
    checkErr(ret, "failed to init RTMS CSDK");

    _sdk = rtms_alloc();

    ret = rtms_config(_sdk, NULL, SDK_ALL, 0);
    checkErr(ret, "failed to configure RTMS CSDK");

    _options.on_join_confirm = _onJoinConfirm;
    _options.on_session_update = _onSessionUpdate;
    _options.on_user_update = _onUserUpdate;
    _options.on_audio_data = _onAudioData;
    _options.on_video_data = _onVideoData;
    _options.on_transcript_data = _onTranscriptData;
    _options.on_leave = _onLeave;

    ret = rtms_set_callbacks(_sdk, &_options);
    _isInit = checkErr(ret, "failed to set callbacks for RTMS CSDK");

    return 0;
}

int Client::join(const string &uuid, const string &streamId, const string &signature, const string &serverUrls, const int &timeout) {

    auto ret = rtms_join(_sdk, uuid.c_str(), streamId.c_str(), signature.c_str(), serverUrls.c_str(), timeout);

    _isRunning = checkErr(ret, "failed to join event " + uuid);

    while(_isRunning)
        _isRunning = checkErr(rtms_poll(_sdk), "failed to poll csdk");

    return 0;
}

bool Client::checkErr(const int &code, const string &message, bool except)
{
    auto isOk = code == RTMS_SDK_OK;
    ostringstream out;
    out << "(" << code << "): " << message << endl;

    if (!isOk && except)
        throw  runtime_error(out.str());

    return isOk;
}

void Client::stop() 
{
    _isRunning = false;
}

bool Client::isInit() const
{
    return _isInit;
}

bool Client::isRunning() const
{
    return _isRunning;
}

void Client::setMediaTypes(bool audio, bool video = false, bool transcript = false) {
    useAudio(audio);
    useVideo(video);
    useTranscript(transcript);
}

void Client::useTranscript(bool useTranscript) {
    _useTranscript = useTranscript;
}

void Client::useAudio(bool useAudio) {
    _useAudio = useAudio;
}

void Client::useVideo(bool useVideo) {
    _useVideo = useVideo;
}

void Client::setAudioParam(const audio_parameters &param) {
    _audioParam = param;
}

void Client::setVideoParam(const video_parameters &param) {
    _videoParam = param;
}

void Client::setOnJoinConfirm(fn_on_join_confirm f) {
    _onJoinConfirm = f;
    _options.on_join_confirm = f;
}

void Client::setOnSessionUpdate(fn_on_session_update f) {
    _onSessionUpdate = f;
    _options.on_session_update = _onSessionUpdate;
}

void Client::setOnUserUpdate(fn_on_user_update f)
{
    _onUserUpdate = f;
    _options.on_user_update = _onUserUpdate;
    
}

void Client::setOnAudioData(fn_on_audio_data f) {
    _onAudioData = f;
    _options.on_audio_data = _onAudioData;
}

void Client::setOnVideoData(fn_on_video_data f) {
    _onVideoData = f;
    _options.on_video_data = _onVideoData;
}

void Client::setOnTranscriptData(fn_on_transcript_data f) {
    _onTranscriptData = f;
    _options.on_transcript_data = _onTranscriptData;
}

void Client::setOnLeave(fn_on_leave f) {
    _onLeave = f;
    _options.on_leave = _onLeave;
}
