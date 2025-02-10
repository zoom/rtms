#include "rtms.h"

#include <utility>


RTMS::RTMS() {
    m_mediaParam.audio_param = &m_audioParam;
    m_mediaParam.video_param = &m_videoParam;
}

int RTMS::init(const string &ca_path) {
    if (ca_path.empty())
        throw invalid_argument("CA path cannot be empty");

    auto ret = rtms_init(ca_path.c_str());

    if (ret != RTMS_SDK_OK)
        throw runtime_error("failed to initialize RTMS SDK");

    m_sdk = rtms_alloc();

    return ret;
}

int RTMS::join(const string &uuid, const string &session_id, const string &signature, const string &signal_url,
               const int &timeout) {

    auto _uuid = uuid.c_str();
    auto _session = session_id.c_str();
    auto _signature = signature.c_str();
    auto _signal = signal_url.c_str();

    auto ret = rtms_join(m_sdk, _uuid, _session, _signature, _signal, timeout);
    auto status = ret == RTMS_SDK_OK;

    while (status && m_isRunning)
        rtms_poll(m_sdk);

    return ret;
}

RTMS::~RTMS() {
    rtms_release(m_sdk);
    rtms_uninit();
}

void RTMS::setMediaTypes(bool audio, bool video = false, bool transcript = false) {
    enableAudio(audio);
    enableVideo(video);
    enableTranscript(transcript);
}

void RTMS::enableTranscript(bool useTranscript) {
    m_useTranscript = useTranscript;
}

void RTMS::enableAudio(bool use_audio) {
    m_useAudio = use_audio;
}

void RTMS::enableVideo(bool use_video) {
    m_useVideo = use_video;
}

void RTMS::setAudioParam(const audio_parameters &param) {
    m_audioParam = param;
}

void RTMS::setVideoParam(const video_parameters &param) {
    m_videoParam = param;
}

RTMS::onJoinConfirmFunc RTMS::getOnJoinConfirm() const {
    return m_onJoinConfirm;
}

void RTMS::setOnJoinConfirm(RTMS::onJoinConfirmFunc f) {
    m_onJoinConfirm = f;
}

RTMS::onSessionUpdateFunc RTMS::getSessionUpdate() const {
    return m_onSessionUpdate;
}

void RTMS::setOnSessionUpdate(RTMS::onSessionUpdateFunc f) {
    m_onSessionUpdate = f;
}


