#include "rtms.h"


RTMS::RTMS(const string& ca_path) {
    m_mediaParam.audio_param = &m_audioParam;
    m_mediaParam.video_param = &m_videoParam;

    char* ca = const_cast<char*>(ca_path.c_str());
    rtms_init(ca);

    //m_sdk = rtms_alloc();
}

int RTMS::join(const string& uuid, const string& session_id, const string& signature, const string& signal_url,
               const int& timeout) {

    char* c_uuid = const_cast<char*>(uuid.c_str());
    char* c_session = const_cast<char*>(session_id.c_str());
    char* c_signature = const_cast<char*>(signature.c_str());
    char* c_signal = const_cast<char*>(signal_url.c_str());

    const int ret = rtms_join(m_sdk, c_uuid, c_session, c_signature, c_signal, timeout);

    while (m_isRunning)
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

void RTMS::setAudioParam(const audio_parameters& param) {
    m_audioParam = param;
}

void RTMS::setVideoParam(const video_parameters& param) {
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


