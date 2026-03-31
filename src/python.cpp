#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include "rtms.h"

namespace py = pybind11;
using namespace rtms;

// ============================================================================
// Python Client Wrapper
// ============================================================================

/**
 * PyClient wraps the C++ Client class and manages Python callback lifecycle.
 *
 * Key responsibilities:
 * - Manage C++ Client instance
 * - Store Python callbacks and ensure proper GIL management
 * - Handle conversion between C++ and Python types
 * - Provide Pythonic API matching Node.js implementation
 */
class PyClient {
public:
    PyClient() : client_(std::make_unique<Client>()) {
        // Client created successfully
    }

    ~PyClient() {
        // Clear all callbacks before destruction
        clearCallbacks();
    }

    // ========================================================================
    // Core Methods
    // ========================================================================

    void join(const std::string& uuid, const std::string& stream_id,
             const std::string& signature, const std::string& server_urls,
             int timeout = -1) {
        client_->join(uuid, stream_id, signature, server_urls, timeout);
    }

    void poll() {
        client_->poll();
    }

    void release() {
        stopCallbacks();
        client_->release();
    }

    std::string uuid() const {
        return client_->uuid();
    }

    std::string streamId() const {
        return client_->streamId();
    }

    // ========================================================================
    // Media Control Methods
    // ========================================================================

    void enableAudio(bool enable) {
        client_->enableAudio(enable);
    }

    void enableVideo(bool enable) {
        client_->enableVideo(enable);
    }

    void enableTranscript(bool enable) {
        client_->enableTranscript(enable);
    }

    void enableDeskshare(bool enable) {
        client_->enableDeskshare(enable);
    }

    // ========================================================================
    // Parameter Setting Methods
    // ========================================================================

    void setAudioParams(const AudioParams& params) {
        client_->setAudioParams(params);
    }

    void setVideoParams(const VideoParams& params) {
        client_->setVideoParams(params);
    }

    void setDeskshareParams(const DeskshareParams& params) {
        client_->setDeskshareParams(params);
    }

    void setTranscriptParams(const TranscriptParams& params) {
        client_->setTranscriptParams(params);
    }

    // ========================================================================
    // Callback Registration Methods
    // ========================================================================

    void onJoinConfirm(py::function callback) {
        join_confirm_callback_ = callback;
        client_->setOnJoinConfirm([this](int reason) {
            if (!join_confirm_callback_.is_none()) {
                py::gil_scoped_acquire acquire;
                try {
                    join_confirm_callback_(reason);
                } catch (const py::error_already_set& e) {
                    py::print("Error in join confirm callback:", e.what());
                }
            }
        });
    }

    void onSessionUpdate(py::function callback) {
        session_update_callback_ = callback;
        client_->setOnSessionUpdate([this](int op, const Session& session) {
            if (!session_update_callback_.is_none()) {
                py::gil_scoped_acquire acquire;
                try {
                    session_update_callback_(op, session);
                } catch (const py::error_already_set& e) {
                    py::print("Error in session update callback:", e.what());
                }
            }
        });
    }

    void onUserUpdate(py::function callback) {
        user_update_callback_ = callback;
        client_->setOnUserUpdate([this](int op, const Participant& participant) {
            if (!user_update_callback_.is_none()) {
                py::gil_scoped_acquire acquire;
                try {
                    user_update_callback_(op, participant);
                } catch (const py::error_already_set& e) {
                    py::print("Error in user update callback:", e.what());
                }
            }
        });
    }

    void onAudioData(py::function callback) {
        audio_data_callback_ = callback;
        client_->setOnAudioData([this](const std::vector<uint8_t>& data, uint64_t timestamp, const Metadata& metadata) {
            if (!audio_data_callback_.is_none()) {
                py::gil_scoped_acquire acquire;
                try {
                    py::bytes py_data(reinterpret_cast<const char*>(data.data()), data.size());
                    audio_data_callback_(py_data, data.size(), timestamp, metadata);
                } catch (const py::error_already_set& e) {
                    py::print("Error in audio data callback:", e.what());
                }
            }
        });
    }

    void onVideoData(py::function callback) {
        video_data_callback_ = callback;
        client_->setOnVideoData([this](const std::vector<uint8_t>& data, uint64_t timestamp, const Metadata& metadata) {
            if (!video_data_callback_.is_none()) {
                py::gil_scoped_acquire acquire;
                try {
                    py::bytes py_data(reinterpret_cast<const char*>(data.data()), data.size());
                    video_data_callback_(py_data, data.size(), timestamp, metadata);
                } catch (const py::error_already_set& e) {
                    py::print("Error in video data callback:", e.what());
                }
            }
        });
    }

    void onDeskshareData(py::function callback) {
        deskshare_data_callback_ = callback;
        client_->setOnDeskshareData([this](const std::vector<uint8_t>& data, uint64_t timestamp, const Metadata& metadata) {
            if (!deskshare_data_callback_.is_none()) {
                py::gil_scoped_acquire acquire;
                try {
                    py::bytes py_data(reinterpret_cast<const char*>(data.data()), data.size());
                    deskshare_data_callback_(py_data, data.size(), timestamp, metadata);
                } catch (const py::error_already_set& e) {
                    py::print("Error in deskshare data callback:", e.what());
                }
            }
        });
    }

    void onTranscriptData(py::function callback) {
        transcript_data_callback_ = callback;
        client_->setOnTranscriptData([this](const std::vector<uint8_t>& data, uint64_t timestamp, const Metadata& metadata) {
            if (!transcript_data_callback_.is_none()) {
                py::gil_scoped_acquire acquire;
                try {
                    py::bytes py_data(reinterpret_cast<const char*>(data.data()), data.size());
                    transcript_data_callback_(py_data, data.size(), timestamp, metadata);
                } catch (const py::error_already_set& e) {
                    py::print("Error in transcript data callback:", e.what());
                }
            }
        });
    }

    void onLeave(py::function callback) {
        leave_callback_ = callback;
        client_->setOnLeave([this](int reason) {
            if (!leave_callback_.is_none()) {
                py::gil_scoped_acquire acquire;
                try {
                    leave_callback_(reason);
                } catch (const py::error_already_set& e) {
                    py::print("Error in leave callback:", e.what());
                }
            }
        });
    }

    void onEventEx(py::function callback) {
        event_ex_callback_ = callback;
        client_->setOnEventEx([this](const std::string& event_data) {
            if (!event_ex_callback_.is_none()) {
                py::gil_scoped_acquire acquire;
                try {
                    event_ex_callback_(event_data);
                } catch (const py::error_already_set& e) {
                    py::print("Error in event ex callback:", e.what());
                }
            }
        });
    }

    // ========================================================================
    // Event Subscription Methods
    // ========================================================================

    void subscribeEvent(const std::vector<int>& events) {
        client_->subscribeEvent(events);
    }

    void unsubscribeEvent(const std::vector<int>& events) {
        client_->unsubscribeEvent(events);
    }

private:
    std::unique_ptr<Client> client_;

    // Python callback storage
    py::object join_confirm_callback_ = py::none();
    py::object session_update_callback_ = py::none();
    py::object user_update_callback_ = py::none();
    py::object audio_data_callback_ = py::none();
    py::object video_data_callback_ = py::none();
    py::object deskshare_data_callback_ = py::none();
    py::object transcript_data_callback_ = py::none();
    py::object leave_callback_ = py::none();
    py::object event_ex_callback_ = py::none();

    void clearCallbacks() {
        join_confirm_callback_ = py::none();
        session_update_callback_ = py::none();
        user_update_callback_ = py::none();
        audio_data_callback_ = py::none();
        video_data_callback_ = py::none();
        deskshare_data_callback_ = py::none();
        transcript_data_callback_ = py::none();
        leave_callback_ = py::none();
        event_ex_callback_ = py::none();
    }

    void stopCallbacks() {
        // Replace callbacks with no-ops to prevent calling Python during shutdown
        if (client_) {
            client_->setOnJoinConfirm([](int) {});
            client_->setOnSessionUpdate([](int, const Session&) {});
            client_->setOnUserUpdate([](int, const Participant&) {});
            client_->setOnAudioData([](const std::vector<uint8_t>&, uint64_t, const Metadata&) {});
            client_->setOnVideoData([](const std::vector<uint8_t>&, uint64_t, const Metadata&) {});
            client_->setOnDeskshareData([](const std::vector<uint8_t>&, uint64_t, const Metadata&) {});
            client_->setOnTranscriptData([](const std::vector<uint8_t>&, uint64_t, const Metadata&) {});
            client_->setOnLeave([](int) {});
            client_->setOnEventEx([](const std::string&) {});
        }
    }
};

// ============================================================================
// Module Definition
// ============================================================================

PYBIND11_MODULE(_rtms, m) {
    m.doc() = "Zoom RTMS Python Bindings - Real-Time Media Streaming SDK";

    // ========================================================================
    // Exception Class
    // ========================================================================

    py::register_exception<Exception>(m, "RTMSException");

    // ========================================================================
    // Data Classes
    // ========================================================================

    py::class_<Session>(m, "Session")
        .def_property_readonly("sessionId", &Session::sessionId)
        .def_property_readonly("streamId", &Session::streamId)
        .def_property_readonly("meetingId", &Session::meetingId)
        .def_property_readonly("statTime", &Session::statTime)
        .def_property_readonly("status", &Session::status)
        .def_property_readonly("isActive", &Session::isActive)
        .def_property_readonly("isPaused", &Session::isPaused);

    py::class_<Participant>(m, "Participant")
        .def_property_readonly("id", &Participant::id)
        .def_property_readonly("name", &Participant::name);

    py::class_<Metadata>(m, "Metadata")
        .def_property_readonly("userName", &Metadata::userName)
        .def_property_readonly("userId", &Metadata::userId);

    // ========================================================================
    // Parameter Classes
    // ========================================================================

    py::class_<AudioParams>(m, "AudioParams")
        .def(py::init<>())
        .def(py::init<int, int, int, int, int, int, int>(),
             py::arg("content_type"), py::arg("codec"), py::arg("sample_rate"),
             py::arg("channel"), py::arg("data_opt"), py::arg("duration"),
             py::arg("frame_size"))
        .def_property("contentType", &AudioParams::contentType, &AudioParams::setContentType)
        .def_property("codec", &AudioParams::codec, &AudioParams::setCodec)
        .def_property("sampleRate", &AudioParams::sampleRate, &AudioParams::setSampleRate)
        .def_property("channel", &AudioParams::channel, &AudioParams::setChannel)
        .def_property("dataOpt", &AudioParams::dataOpt, &AudioParams::setDataOpt)
        .def_property("duration", &AudioParams::duration, &AudioParams::setDuration)
        .def_property("frameSize", &AudioParams::frameSize, &AudioParams::setFrameSize);

    py::class_<VideoParams>(m, "VideoParams")
        .def(py::init<>())
        .def(py::init<int, int, int, int, int>(),
             py::arg("content_type"), py::arg("codec"), py::arg("resolution"),
             py::arg("data_opt"), py::arg("fps"))
        .def_property("contentType", &VideoParams::contentType, &VideoParams::setContentType)
        .def_property("codec", &VideoParams::codec, &VideoParams::setCodec)
        .def_property("resolution", &VideoParams::resolution, &VideoParams::setResolution)
        .def_property("dataOpt", &VideoParams::dataOpt, &VideoParams::setDataOpt)
        .def_property("fps", &VideoParams::fps, &VideoParams::setFps);

    py::class_<DeskshareParams>(m, "DeskshareParams")
        .def(py::init<>())
        .def(py::init<int, int, int, int>(),
             py::arg("content_type"), py::arg("codec"), py::arg("resolution"),
             py::arg("fps"))
        .def_property("contentType", &DeskshareParams::contentType, &DeskshareParams::setContentType)
        .def_property("codec", &DeskshareParams::codec, &DeskshareParams::setCodec)
        .def_property("resolution", &DeskshareParams::resolution, &DeskshareParams::setResolution)
        .def_property("fps", &DeskshareParams::fps, &DeskshareParams::setFps);

    py::class_<TranscriptParams>(m, "TranscriptParams")
        .def(py::init<>())
        .def_property("content_type", &TranscriptParams::contentType, &TranscriptParams::setContentType)
        .def_property("src_language", &TranscriptParams::srcLanguage, &TranscriptParams::setSrcLanguage)
        .def_property("enable_lid", &TranscriptParams::enableLid, &TranscriptParams::setEnableLid);

    // TranscriptLanguage constants dict (matches pattern of AudioCodec, VideoCodec, etc.)
    // Values sourced from the TranscriptLanguageId enum in rtms.h
    py::dict transcriptLanguage;
    transcriptLanguage["NONE"]                = (int)LANGUAGE_ID_NONE;
    transcriptLanguage["ARABIC"]              = (int)LANGUAGE_ID_ARABIC;
    transcriptLanguage["BENGALI"]             = (int)LANGUAGE_ID_BENGALI;
    transcriptLanguage["CANTONESE"]           = (int)LANGUAGE_ID_CANTONESE;
    transcriptLanguage["CATALAN"]             = (int)LANGUAGE_ID_CATALAN;
    transcriptLanguage["CHINESE_SIMPLIFIED"]  = (int)LANGUAGE_ID_CHINESE_SIMPLIFIED;
    transcriptLanguage["CHINESE_TRADITIONAL"] = (int)LANGUAGE_ID_CHINESE_TRADITIONAL;
    transcriptLanguage["CZECH"]               = (int)LANGUAGE_ID_CZECH;
    transcriptLanguage["DANISH"]              = (int)LANGUAGE_ID_DANISH;
    transcriptLanguage["DUTCH"]               = (int)LANGUAGE_ID_DUTCH;
    transcriptLanguage["ENGLISH"]             = (int)LANGUAGE_ID_ENGLISH;
    transcriptLanguage["ESTONIAN"]            = (int)LANGUAGE_ID_ESTONIAN;
    transcriptLanguage["FINNISH"]             = (int)LANGUAGE_ID_FINNISH;
    transcriptLanguage["FRENCH_CANADA"]       = (int)LANGUAGE_ID_FRENCH_CANADA;
    transcriptLanguage["FRENCH_FRANCE"]       = (int)LANGUAGE_ID_FRENCH_FRANCE;
    transcriptLanguage["GERMAN"]              = (int)LANGUAGE_ID_GERMAN;
    transcriptLanguage["HEBREW"]              = (int)LANGUAGE_ID_HEBREW;
    transcriptLanguage["HINDI"]               = (int)LANGUAGE_ID_HINDI;
    transcriptLanguage["HUNGARIAN"]           = (int)LANGUAGE_ID_HUNGARIAN;
    transcriptLanguage["INDONESIAN"]          = (int)LANGUAGE_ID_INDONESIAN;
    transcriptLanguage["ITALIAN"]             = (int)LANGUAGE_ID_ITALIAN;
    transcriptLanguage["JAPANESE"]            = (int)LANGUAGE_ID_JAPANESE;
    transcriptLanguage["KOREAN"]              = (int)LANGUAGE_ID_KOREAN;
    transcriptLanguage["MALAY"]               = (int)LANGUAGE_ID_MALAY;
    transcriptLanguage["PERSIAN"]             = (int)LANGUAGE_ID_PERSIAN;
    transcriptLanguage["POLISH"]              = (int)LANGUAGE_ID_POLISH;
    transcriptLanguage["PORTUGUESE"]          = (int)LANGUAGE_ID_PORTUGUESE;
    transcriptLanguage["ROMANIAN"]            = (int)LANGUAGE_ID_ROMANIAN;
    transcriptLanguage["RUSSIAN"]             = (int)LANGUAGE_ID_RUSSIAN;
    transcriptLanguage["SPANISH"]             = (int)LANGUAGE_ID_SPANISH;
    transcriptLanguage["SWEDISH"]             = (int)LANGUAGE_ID_SWEDISH;
    transcriptLanguage["TAGALOG"]             = (int)LANGUAGE_ID_TAGALOG;
    transcriptLanguage["TAMIL"]               = (int)LANGUAGE_ID_TAMIL;
    transcriptLanguage["TELUGU"]              = (int)LANGUAGE_ID_TELUGU;
    transcriptLanguage["THAI"]                = (int)LANGUAGE_ID_THAI;
    transcriptLanguage["TURKISH"]             = (int)LANGUAGE_ID_TURKISH;
    transcriptLanguage["UKRAINIAN"]           = (int)LANGUAGE_ID_UKRAINIAN;
    transcriptLanguage["VIETNAMESE"]          = (int)LANGUAGE_ID_VIETNAMESE;
    m.attr("TranscriptLanguage") = transcriptLanguage;

    // ========================================================================
    // Client Class
    // ========================================================================

    py::class_<PyClient>(m, "Client")
        .def(py::init<>())
        .def_static("initialize", [](const std::string& ca_path, int is_verify_cert = 1, const char* agent = nullptr) {
            Client::initialize(ca_path, is_verify_cert, agent);
        },
             "Initialize the RTMS SDK",
             py::arg("ca_path"), py::arg("is_verify_cert") = 1, py::arg("agent") = nullptr)
        .def_static("uninitialize", &Client::uninitialize,
             "Uninitialize the RTMS SDK")
        .def("join", &PyClient::join,
             "Join an RTMS session",
             py::arg("uuid"), py::arg("stream_id"), py::arg("signature"),
             py::arg("server_urls"), py::arg("timeout") = -1)
        .def("poll", &PyClient::poll,
             "Poll for events (call periodically)")
        .def("release", &PyClient::release,
             "Release client resources")
        .def("uuid", &PyClient::uuid,
             "Get meeting UUID")
        .def("streamId", &PyClient::streamId,
             "Get stream ID")
        .def("enableAudio", &PyClient::enableAudio,
             "Enable/disable audio streaming")
        .def("enableVideo", &PyClient::enableVideo,
             "Enable/disable video streaming")
        .def("enableTranscript", &PyClient::enableTranscript,
             "Enable/disable transcript streaming")
        .def("enableDeskshare", &PyClient::enableDeskshare,
             "Enable/disable deskshare streaming")
        .def("setAudioParams", &PyClient::setAudioParams,
             "Set audio parameters")
        .def("setVideoParams", &PyClient::setVideoParams,
             "Set video parameters")
        .def("setDeskshareParams", &PyClient::setDeskshareParams,
             "Set deskshare parameters")
        .def("setTranscriptParams", &PyClient::setTranscriptParams,
             "Set transcript parameters")
        .def("onJoinConfirm", &PyClient::onJoinConfirm,
             "Register join confirm callback")
        .def("onSessionUpdate", &PyClient::onSessionUpdate,
             "Register session update callback")
        .def("onUserUpdate", &PyClient::onUserUpdate,
             "Register user update callback")
        .def("onAudioData", &PyClient::onAudioData,
             "Register audio data callback")
        .def("onVideoData", &PyClient::onVideoData,
             "Register video data callback")
        .def("onDeskshareData", &PyClient::onDeskshareData,
             "Register deskshare data callback")
        .def("onTranscriptData", &PyClient::onTranscriptData,
             "Register transcript data callback")
        .def("onLeave", &PyClient::onLeave,
             "Register leave callback")
        .def("onEventEx", &PyClient::onEventEx,
             "Register extended event callback")
        .def("subscribeEvent", &PyClient::subscribeEvent,
             "Subscribe to receive specific event types",
             py::arg("events"))
        .def("unsubscribeEvent", &PyClient::unsubscribeEvent,
             "Unsubscribe from specific event types",
             py::arg("events"));

    // ========================================================================
    // Constants - Media Types
    // ========================================================================

    m.attr("MEDIA_TYPE_AUDIO") = py::int_(static_cast<int>(SDK_AUDIO));
    m.attr("MEDIA_TYPE_VIDEO") = py::int_(static_cast<int>(SDK_VIDEO));
    m.attr("MEDIA_TYPE_DESKSHARE") = py::int_(static_cast<int>(SDK_DESKSHARE));
    m.attr("MEDIA_TYPE_TRANSCRIPT") = py::int_(static_cast<int>(SDK_TRANSCRIPT));
    m.attr("MEDIA_TYPE_CHAT") = py::int_(static_cast<int>(SDK_CHAT));
    m.attr("MEDIA_TYPE_ALL") = py::int_(static_cast<int>(SDK_ALL));

    // ========================================================================
    // Constants - Session Events
    // ========================================================================

    m.attr("SESSION_EVENT_ADD") = py::int_(static_cast<int>(SESSION_ADD));
    m.attr("SESSION_EVENT_STOP") = py::int_(static_cast<int>(SESSION_STOP));
    m.attr("SESSION_EVENT_PAUSE") = py::int_(static_cast<int>(SESSION_PAUSE));
    m.attr("SESSION_EVENT_RESUME") = py::int_(static_cast<int>(SESSION_RESUME));

    // ========================================================================
    // Constants - User Events
    // ========================================================================

    m.attr("USER_JOIN") = py::int_(static_cast<int>(USER_JOIN));
    m.attr("USER_LEAVE") = py::int_(static_cast<int>(USER_LEAVE));

    // ========================================================================
    // Constants - Event Types
    // ========================================================================
    // Event Types (for subscribeEvent/unsubscribeEvent - used with onEventEx callback)
    // These match RTMS_EVENT_TYPE from Zoom's C SDK
    // ========================================================================

    m.attr("EVENT_UNDEFINED") = py::int_(static_cast<int>(Client::EVENT_UNDEFINED));
    m.attr("EVENT_FIRST_PACKET_TIMESTAMP") = py::int_(static_cast<int>(Client::EVENT_FIRST_PACKET_TIMESTAMP));
    m.attr("EVENT_ACTIVE_SPEAKER_CHANGE") = py::int_(static_cast<int>(Client::EVENT_ACTIVE_SPEAKER_CHANGE));
    m.attr("EVENT_PARTICIPANT_JOIN") = py::int_(static_cast<int>(Client::EVENT_PARTICIPANT_JOIN));
    m.attr("EVENT_PARTICIPANT_LEAVE") = py::int_(static_cast<int>(Client::EVENT_PARTICIPANT_LEAVE));
    m.attr("EVENT_SHARING_START") = py::int_(static_cast<int>(Client::EVENT_SHARING_START));
    m.attr("EVENT_SHARING_STOP") = py::int_(static_cast<int>(Client::EVENT_SHARING_STOP));
    m.attr("EVENT_MEDIA_CONNECTION_INTERRUPTED") = py::int_(static_cast<int>(Client::EVENT_MEDIA_CONNECTION_INTERRUPTED));
    m.attr("EVENT_CONSUMER_ANSWERED") = py::int_(static_cast<int>(Client::EVENT_CONSUMER_ANSWERED));
    m.attr("EVENT_CONSUMER_END") = py::int_(static_cast<int>(Client::EVENT_CONSUMER_END));
    m.attr("EVENT_USER_ANSWERED") = py::int_(static_cast<int>(Client::EVENT_USER_ANSWERED));
    m.attr("EVENT_USER_END") = py::int_(static_cast<int>(Client::EVENT_USER_END));
    m.attr("EVENT_USER_HOLD") = py::int_(static_cast<int>(Client::EVENT_USER_HOLD));
    m.attr("EVENT_USER_UNHOLD") = py::int_(static_cast<int>(Client::EVENT_USER_UNHOLD));

    // ========================================================================
    // Constants - Error Codes
    // ========================================================================

    m.attr("RTMS_SDK_OK") = py::int_(static_cast<int>(RTMS_SDK_OK));
    m.attr("RTMS_SDK_FAILURE") = py::int_(static_cast<int>(RTMS_SDK_FAILURE));
    m.attr("RTMS_SDK_TIMEOUT") = py::int_(static_cast<int>(RTMS_SDK_TIMEOUT));
    m.attr("RTMS_SDK_NOT_EXIST") = py::int_(static_cast<int>(RTMS_SDK_NOT_EXIST));
    m.attr("RTMS_SDK_WRONG_TYPE") = py::int_(static_cast<int>(RTMS_SDK_WRONG_TYPE));
    m.attr("RTMS_SDK_INVALID_STATUS") = py::int_(static_cast<int>(RTMS_SDK_INVALID_STATUS));
    m.attr("RTMS_SDK_INVALID_ARGS") = py::int_(static_cast<int>(RTMS_SDK_INVALID_ARGS));

    // ========================================================================
    // Constants - Session Status
    // ========================================================================

    m.attr("SESS_STATUS_ACTIVE") = py::int_(static_cast<int>(SESS_STATUS_ACTIVE));
    m.attr("SESS_STATUS_PAUSED") = py::int_(static_cast<int>(SESS_STATUS_PAUSED));

    // ========================================================================
    // Constants - Audio Parameters
    // ========================================================================

    py::dict audioContentType;
    audioContentType["UNDEFINED"]   = (int)CONTENT_TYPE_UNDEFINED;
    audioContentType["RTP"]         = (int)CONTENT_TYPE_RTP;
    audioContentType["RAW_AUDIO"]   = (int)CONTENT_TYPE_RAW_AUDIO;
    audioContentType["FILE_STREAM"] = (int)CONTENT_TYPE_FILE_STREAM;
    audioContentType["TEXT"]        = (int)CONTENT_TYPE_TEXT;
    m.attr("AudioContentType") = audioContentType;

    py::dict audioCodec;
    audioCodec["UNDEFINED"] = (int)CODEC_UNDEFINED;
    audioCodec["L16"]       = (int)CODEC_L16;
    audioCodec["G711"]      = (int)CODEC_G711;
    audioCodec["G722"]      = (int)CODEC_G722;
    audioCodec["OPUS"]      = (int)CODEC_OPUS;
    m.attr("AudioCodec") = audioCodec;

    py::dict audioSampleRate;
    audioSampleRate["SR_8K"]  = (int)SAMPLE_RATE_8K;
    audioSampleRate["SR_16K"] = (int)SAMPLE_RATE_16K;
    audioSampleRate["SR_32K"] = (int)SAMPLE_RATE_32K;
    audioSampleRate["SR_48K"] = (int)SAMPLE_RATE_48K;
    m.attr("AudioSampleRate") = audioSampleRate;

    py::dict audioChannel;
    audioChannel["MONO"]   = (int)CHANNEL_MONO;
    audioChannel["STEREO"] = (int)CHANNEL_STEREO;
    m.attr("AudioChannel") = audioChannel;

    py::dict audioDataOption;
    audioDataOption["UNDEFINED"]           = (int)DATA_OPT_UNDEFINED;
    audioDataOption["AUDIO_MIXED_STREAM"]  = (int)DATA_OPT_AUDIO_MIXED_STREAM;
    audioDataOption["AUDIO_MULTI_STREAMS"] = (int)DATA_OPT_AUDIO_MULTI_STREAMS;
    m.attr("AudioDataOption") = audioDataOption;

    // ========================================================================
    // Constants - Video Parameters
    // ========================================================================

    py::dict videoContentType;
    videoContentType["UNDEFINED"]   = (int)CONTENT_TYPE_UNDEFINED;
    videoContentType["RTP"]         = (int)CONTENT_TYPE_RTP;
    videoContentType["RAW_VIDEO"]   = (int)CONTENT_TYPE_RAW_VIDEO;
    videoContentType["FILE_STREAM"] = (int)CONTENT_TYPE_FILE_STREAM;
    videoContentType["TEXT"]        = (int)CONTENT_TYPE_TEXT;
    m.attr("VideoContentType") = videoContentType;

    py::dict videoCodec;
    videoCodec["UNDEFINED"] = (int)CODEC_UNDEFINED;
    videoCodec["JPG"]       = (int)CODEC_JPG;
    videoCodec["PNG"]       = (int)CODEC_PNG;
    videoCodec["H264"]      = (int)CODEC_H264;
    m.attr("VideoCodec") = videoCodec;

    py::dict videoResolution;
    videoResolution["SD"]  = (int)RESOLUTION_SD;
    videoResolution["HD"]  = (int)RESOLUTION_HD;
    videoResolution["FHD"] = (int)RESOLUTION_FHD;
    videoResolution["QHD"] = (int)RESOLUTION_QHD;
    m.attr("VideoResolution") = videoResolution;

    py::dict videoDataOption;
    videoDataOption["UNDEFINED"]                  = (int)DATA_OPT_UNDEFINED;
    videoDataOption["VIDEO_SINGLE_ACTIVE_STREAM"] = (int)DATA_OPT_VIDEO_SINGLE_ACTIVE_STREAM;
    videoDataOption["VIDEO_MIXED_SPEAKER_VIEW"]   = (int)DATA_OPT_VIDEO_SINGLE_INDIVIDUAL_STREAM;
    videoDataOption["VIDEO_MIXED_GALLERY_VIEW"]   = (int)DATA_OPT_VIDEO_MIXED_GALLERY_VIEW;
    m.attr("VideoDataOption") = videoDataOption;

    // ========================================================================
    // Constants - Media Data Types
    // ========================================================================

    py::dict mediaDataType;
    mediaDataType["UNDEFINED"]  = (int)MEDIA_DATA_UNDEFINED;
    mediaDataType["AUDIO"]      = (int)MEDIA_DATA_AUDIO;
    mediaDataType["VIDEO"]      = (int)MEDIA_DATA_VIDEO;
    mediaDataType["DESKSHARE"]  = (int)MEDIA_DATA_DESKSHARE;
    mediaDataType["TRANSCRIPT"] = (int)MEDIA_DATA_TRANSCRIPT;
    mediaDataType["CHAT"]       = (int)MEDIA_DATA_CHAT;
    mediaDataType["ALL"]        = (int)MEDIA_DATA_ALL;
    m.attr("MediaDataType") = mediaDataType;

    // ========================================================================
    // Constants - Session State
    // ========================================================================

    py::dict sessionState;
    sessionState["INACTIVE"]   = (int)SESSION_STATE_INACTIVE;
    sessionState["INITIALIZE"] = (int)SESSION_STATE_INITIALIZE;
    sessionState["STARTED"]    = (int)SESSION_STATE_STARTED;
    sessionState["PAUSED"]     = (int)SESSION_STATE_PAUSED;
    sessionState["RESUMED"]    = (int)SESSION_STATE_RESUMED;
    sessionState["STOPPED"]    = (int)SESSION_STATE_STOPPED;
    m.attr("SessionState") = sessionState;

    // ========================================================================
    // Constants - Stream State
    // ========================================================================

    py::dict streamState;
    streamState["INACTIVE"]    = (int)STREAM_STATE_INACTIVE;
    streamState["ACTIVE"]      = (int)STREAM_STATE_ACTIVE;
    streamState["INTERRUPTED"] = (int)STREAM_STATE_INTERRUPTED;
    streamState["TERMINATING"] = (int)STREAM_STATE_TERMINATING;
    streamState["TERMINATED"]  = (int)STREAM_STATE_TERMINATED;
    streamState["PAUSED"]      = (int)STREAM_STATE_PAUSED;
    streamState["RESUMED"]     = (int)STREAM_STATE_RESUMED;
    m.attr("StreamState") = streamState;

    // ========================================================================
    // Constants - Event Type (matches RTMS_EVENT_TYPE from Zoom's C SDK)
    // ========================================================================

    py::dict eventType;
    eventType["UNDEFINED"]                    = (int)RTMS_EVENT_UNDEFINED;
    eventType["FIRST_PACKET_TIMESTAMP"]       = (int)RTMS_EVENT_FIRST_PACKET_TIMESTAMP;
    eventType["ACTIVE_SPEAKER_CHANGE"]        = (int)RTMS_EVENT_ACTIVE_SPEAKER_CHANGE;
    eventType["PARTICIPANT_JOIN"]             = (int)RTMS_EVENT_PARTICIPANT_JOIN;
    eventType["PARTICIPANT_LEAVE"]            = (int)RTMS_EVENT_PARTICIPANT_LEAVE;
    eventType["SHARING_START"]               = (int)RTMS_EVENT_SHARING_START;
    eventType["SHARING_STOP"]               = (int)RTMS_EVENT_SHARING_STOP;
    eventType["MEDIA_CONNECTION_INTERRUPTED"] = (int)RTMS_EVENT_MEDIA_CONNECTION_INTERRUPTED;
    eventType["PARTICIPANT_VIDEO_ON"]         = (int)RTMS_EVENT_PARTICIPANT_VIDEO_ON;
    eventType["PARTICIPANT_VIDEO_OFF"]        = (int)RTMS_EVENT_PARTICIPANT_VIDEO_OFF;
    // ZCC voice events
    eventType["CONSUMER_ANSWERED"] = (int)ZCC_EVENT_CONSUMER_ANSWERED;
    eventType["CONSUMER_END"]      = (int)ZCC_EVENT_CONSUMER_END;
    eventType["USER_ANSWERED"]     = (int)ZCC_EVENT_USER_ANSWERED;
    eventType["USER_END"]          = (int)ZCC_EVENT_USER_END;
    eventType["USER_HOLD"]         = (int)ZCC_EVENT_USER_HOLD;
    eventType["USER_UNHOLD"]       = (int)ZCC_EVENT_USER_UNHOLD;
    m.attr("EventType") = eventType;

    // ========================================================================
    // Constants - Message Type
    // ========================================================================

    py::dict messageType;
    messageType["UNDEFINED"]                 = (int)MSG_UNDEFINED;
    messageType["SIGNALING_HAND_SHAKE_REQ"]  = (int)MSG_SIGNALING_HAND_SHAKE_REQ;
    messageType["SIGNALING_HAND_SHAKE_RESP"] = (int)MSG_SIGNALING_HAND_SHAKE_RESP;
    messageType["DATA_HAND_SHAKE_REQ"]       = (int)MSG_DATA_HAND_SHAKE_REQ;
    messageType["DATA_HAND_SHAKE_RESP"]      = (int)MSG_DATA_HAND_SHAKE_RESP;
    messageType["EVENT_SUBSCRIPTION"]        = (int)MSG_EVENT_SUBSCRIPTION;
    messageType["EVENT_UPDATE"]              = (int)MSG_EVENT_UPDATE;
    messageType["CLIENT_READY_ACK"]          = (int)MSG_CLIENT_READY_ACK;
    messageType["STREAM_STATE_UPDATE"]       = (int)MSG_STREAM_STATE_UPDATE;
    messageType["SESSION_STATE_UPDATE"]      = (int)MSG_SESSION_STATE_UPDATE;
    messageType["SESSION_STATE_REQ"]         = (int)MSG_SESSION_STATE_REQ;
    messageType["SESSION_STATE_RESP"]        = (int)MSG_SESSION_STATE_RESP;
    messageType["KEEP_ALIVE_REQ"]            = (int)MSG_KEEP_ALIVE_REQ;
    messageType["KEEP_ALIVE_RESP"]           = (int)MSG_KEEP_ALIVE_RESP;
    messageType["MEDIA_DATA_AUDIO"]          = (int)MSG_MEDIA_DATA_AUDIO;
    messageType["MEDIA_DATA_VIDEO"]          = (int)MSG_MEDIA_DATA_VIDEO;
    messageType["MEDIA_DATA_SHARE"]          = (int)MSG_MEDIA_DATA_SHARE;
    messageType["MEDIA_DATA_TRANSCRIPT"]     = (int)MSG_MEDIA_DATA_TRANSCRIPT;
    messageType["MEDIA_DATA_CHAT"]           = (int)MSG_MEDIA_DATA_CHAT;
    messageType["STREAM_STATE_REQ"]          = (int)MSG_STREAM_STATE_REQ;
    messageType["STREAM_STATE_RESP"]         = (int)MSG_STREAM_STATE_RESP;
    messageType["STREAM_CLOSE_REQ"]          = (int)MSG_STREAM_CLOSE_REQ;
    messageType["STREAM_CLOSE_RESP"]         = (int)MSG_STREAM_CLOSE_RESP;
    messageType["META_DATA_AUDIO"]           = (int)MSG_META_DATA_AUDIO;
    messageType["META_DATA_VIDEO"]           = (int)MSG_META_DATA_VIDEO;
    messageType["META_DATA_SHARE"]           = (int)MSG_META_DATA_SHARE;
    messageType["META_DATA_TRANSCRIPT"]      = (int)MSG_META_DATA_TRANSCRIPT;
    messageType["META_DATA_CHAT"]            = (int)MSG_META_DATA_CHAT;
    messageType["VIDEO_SUBSCRIPTION_REQ"]    = (int)MSG_VIDEO_SUBSCRIPTION_REQ;
    messageType["VIDEO_SUBSCRIPTION_RESP"]   = (int)MSG_VIDEO_SUBSCRIPTION_RESP;
    m.attr("MessageType") = messageType;

    // ========================================================================
    // Constants - Stop Reason
    // ========================================================================

    py::dict stopReason;
    stopReason["UNDEFINED"]                               = (int)STOP_UNDEFINED;
    stopReason["STOP_BC_HOST_TRIGGERED"]                  = (int)STOP_BC_HOST_TRIGGERED;
    stopReason["STOP_BC_USER_TRIGGERED"]                  = (int)STOP_BC_USER_TRIGGERED;
    stopReason["STOP_BC_USER_LEFT"]                       = (int)STOP_BC_USER_LEFT;
    stopReason["STOP_BC_USER_EJECTED"]                    = (int)STOP_BC_USER_EJECTED;
    stopReason["STOP_BC_HOST_DISABLED_APP"]               = (int)STOP_BC_HOST_DISABLED_APP;
    stopReason["STOP_BC_MEETING_ENDED"]                   = (int)STOP_BC_MEETING_ENDED;
    stopReason["STOP_BC_STREAM_CANCELED"]                 = (int)STOP_BC_STREAM_CANCELED;
    stopReason["STOP_BC_STREAM_REVOKED"]                  = (int)STOP_BC_STREAM_REVOKED;
    stopReason["STOP_BC_ALL_APPS_DISABLED"]               = (int)STOP_BC_ALL_APPS_DISABLED;
    stopReason["STOP_BC_INTERNAL_EXCEPTION"]              = (int)STOP_BC_INTERNAL_EXCEPTION;
    stopReason["STOP_BC_CONNECTION_TIMEOUT"]              = (int)STOP_BC_CONNECTION_TIMEOUT;
    stopReason["STOP_BC_INSTANCE_CONNECTION_INTERRUPTED"] = (int)STOP_BC_INSTANCE_CONNECTION_INTERRUPTED;
    stopReason["STOP_BC_SIGNAL_CONNECTION_INTERRUPTED"]   = (int)STOP_BC_SIGNAL_CONNECTION_INTERRUPTED;
    stopReason["STOP_BC_DATA_CONNECTION_INTERRUPTED"]     = (int)STOP_BC_DATA_CONNECTION_INTERRUPTED;
    stopReason["STOP_BC_SIGNAL_CONNECTION_CLOSED_ABNORMALLY"] = (int)STOP_BC_SIGNAL_CONNECTION_CLOSED_ABNORMALLY;
    stopReason["STOP_BC_DATA_CONNECTION_CLOSED_ABNORMALLY"]   = (int)STOP_BC_DATA_CONNECTION_CLOSED_ABNORMALLY;
    stopReason["STOP_BC_EXIT_SIGNAL"]                     = (int)STOP_BC_EXIT_SIGNAL;
    stopReason["STOP_BC_AUTHENTICATION_FAILURE"]          = (int)STOP_BC_AUTHENTICATION_FAILURE;
    stopReason["STOP_BC_AWAIT_RECONNECTION_TIMEOUT"]      = (int)STOP_BC_AWAIT_RECONNECTION_TIMEOUT;
    stopReason["STOP_BC_RECEIVER_REQUEST_CLOSE"]          = (int)STOP_BC_RECEIVER_REQUEST_CLOSE;
    stopReason["STOP_BC_CUSTOMER_DISCONNECTED"]           = (int)STOP_BC_CUSTOMER_DISCONNECTED;
    stopReason["STOP_BC_AGENT_DISCONNECTED"]              = (int)STOP_BC_AGENT_DISCONNECTED;
    stopReason["STOP_BC_ADMIN_DISABLED_APP"]              = (int)STOP_BC_ADMIN_DISABLED_APP;
    stopReason["STOP_BC_KEEP_ALIVE_TIMEOUT"]              = (int)STOP_BC_KEEP_ALIVE_TIMEOUT;
    stopReason["STOP_BC_MANUAL_API_TRIGGERED"]            = (int)STOP_BC_MANUAL_API_TRIGGERED;
    stopReason["STOP_BC_STREAMING_NOT_SUPPORTED"]         = (int)STOP_BC_STREAMING_NOT_SUPPORTED;
    m.attr("StopReason") = stopReason;
}
