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

    void setProxy(const std::string& proxy_type, const std::string& proxy_url) {
        client_->setProxy(proxy_type, proxy_url);
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
    // Individual Video Subscription
    // ========================================================================

    void subscribeVideo(int user_id, bool subscribe) {
        client_->subscribeVideo(user_id, subscribe);
    }

    void onParticipantVideo(py::function callback) {
        participant_video_callback_ = callback;
        client_->setOnParticipantVideo([this](const std::vector<int>& users, bool is_on) {
            if (!participant_video_callback_.is_none()) {
                py::gil_scoped_acquire acquire;
                try {
                    participant_video_callback_(users, is_on);
                } catch (const py::error_already_set& e) {
                    py::print("Error in participant video callback:", e.what());
                }
            }
        });
    }

    void onVideoSubscribed(py::function callback) {
        video_subscribed_callback_ = callback;
        client_->setOnVideoSubscribed([this](int user_id, int status, const std::string& error) {
            if (!video_subscribed_callback_.is_none()) {
                py::gil_scoped_acquire acquire;
                try {
                    video_subscribed_callback_(user_id, status, error);
                } catch (const py::error_already_set& e) {
                    py::print("Error in video subscribed callback:", e.what());
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
    py::object participant_video_callback_ = py::none();
    py::object video_subscribed_callback_ = py::none();

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
        participant_video_callback_ = py::none();
        video_subscribed_callback_ = py::none();
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
            client_->setOnParticipantVideo([](const std::vector<int>&, bool) {});
            client_->setOnVideoSubscribed([](int, int, const std::string&) {});
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
    // Values sourced from the RTMS_TRANSCRIPT_LANGUAGE enum in rtms.h
    py::dict transcriptLanguage;
    transcriptLanguage["NONE"]                = (int)RTMS_TRANSCRIPT_LANGUAGE::NONE;
    transcriptLanguage["ARABIC"]              = (int)RTMS_TRANSCRIPT_LANGUAGE::ARABIC;
    transcriptLanguage["BENGALI"]             = (int)RTMS_TRANSCRIPT_LANGUAGE::BENGALI;
    transcriptLanguage["CANTONESE"]           = (int)RTMS_TRANSCRIPT_LANGUAGE::CANTONESE;
    transcriptLanguage["CATALAN"]             = (int)RTMS_TRANSCRIPT_LANGUAGE::CATALAN;
    transcriptLanguage["CHINESE_SIMPLIFIED"]  = (int)RTMS_TRANSCRIPT_LANGUAGE::CHINESE_SIMPLIFIED;
    transcriptLanguage["CHINESE_TRADITIONAL"] = (int)RTMS_TRANSCRIPT_LANGUAGE::CHINESE_TRADITIONAL;
    transcriptLanguage["CZECH"]               = (int)RTMS_TRANSCRIPT_LANGUAGE::CZECH;
    transcriptLanguage["DANISH"]              = (int)RTMS_TRANSCRIPT_LANGUAGE::DANISH;
    transcriptLanguage["DUTCH"]               = (int)RTMS_TRANSCRIPT_LANGUAGE::DUTCH;
    transcriptLanguage["ENGLISH"]             = (int)RTMS_TRANSCRIPT_LANGUAGE::ENGLISH;
    transcriptLanguage["ESTONIAN"]            = (int)RTMS_TRANSCRIPT_LANGUAGE::ESTONIAN;
    transcriptLanguage["FINNISH"]             = (int)RTMS_TRANSCRIPT_LANGUAGE::FINNISH;
    transcriptLanguage["FRENCH_CANADA"]       = (int)RTMS_TRANSCRIPT_LANGUAGE::FRENCH_CANADA;
    transcriptLanguage["FRENCH_FRANCE"]       = (int)RTMS_TRANSCRIPT_LANGUAGE::FRENCH_FRANCE;
    transcriptLanguage["GERMAN"]              = (int)RTMS_TRANSCRIPT_LANGUAGE::GERMAN;
    transcriptLanguage["HEBREW"]              = (int)RTMS_TRANSCRIPT_LANGUAGE::HEBREW;
    transcriptLanguage["HINDI"]               = (int)RTMS_TRANSCRIPT_LANGUAGE::HINDI;
    transcriptLanguage["HUNGARIAN"]           = (int)RTMS_TRANSCRIPT_LANGUAGE::HUNGARIAN;
    transcriptLanguage["INDONESIAN"]          = (int)RTMS_TRANSCRIPT_LANGUAGE::INDONESIAN;
    transcriptLanguage["ITALIAN"]             = (int)RTMS_TRANSCRIPT_LANGUAGE::ITALIAN;
    transcriptLanguage["JAPANESE"]            = (int)RTMS_TRANSCRIPT_LANGUAGE::JAPANESE;
    transcriptLanguage["KOREAN"]              = (int)RTMS_TRANSCRIPT_LANGUAGE::KOREAN;
    transcriptLanguage["MALAY"]               = (int)RTMS_TRANSCRIPT_LANGUAGE::MALAY;
    transcriptLanguage["PERSIAN"]             = (int)RTMS_TRANSCRIPT_LANGUAGE::PERSIAN;
    transcriptLanguage["POLISH"]              = (int)RTMS_TRANSCRIPT_LANGUAGE::POLISH;
    transcriptLanguage["PORTUGUESE"]          = (int)RTMS_TRANSCRIPT_LANGUAGE::PORTUGUESE;
    transcriptLanguage["ROMANIAN"]            = (int)RTMS_TRANSCRIPT_LANGUAGE::ROMANIAN;
    transcriptLanguage["RUSSIAN"]             = (int)RTMS_TRANSCRIPT_LANGUAGE::RUSSIAN;
    transcriptLanguage["SPANISH"]             = (int)RTMS_TRANSCRIPT_LANGUAGE::SPANISH;
    transcriptLanguage["SWEDISH"]             = (int)RTMS_TRANSCRIPT_LANGUAGE::SWEDISH;
    transcriptLanguage["TAGALOG"]             = (int)RTMS_TRANSCRIPT_LANGUAGE::TAGALOG;
    transcriptLanguage["TAMIL"]               = (int)RTMS_TRANSCRIPT_LANGUAGE::TAMIL;
    transcriptLanguage["TELUGU"]              = (int)RTMS_TRANSCRIPT_LANGUAGE::TELUGU;
    transcriptLanguage["THAI"]                = (int)RTMS_TRANSCRIPT_LANGUAGE::THAI;
    transcriptLanguage["TURKISH"]             = (int)RTMS_TRANSCRIPT_LANGUAGE::TURKISH;
    transcriptLanguage["UKRAINIAN"]           = (int)RTMS_TRANSCRIPT_LANGUAGE::UKRAINIAN;
    transcriptLanguage["VIETNAMESE"]          = (int)RTMS_TRANSCRIPT_LANGUAGE::VIETNAMESE;
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
        .def("stream_id", &PyClient::streamId,
             "Get stream ID")
        .def("streamId", &PyClient::streamId,
             "Get stream ID")
        .def("enable_audio", &PyClient::enableAudio,
             "Enable/disable audio streaming")
        .def("enableAudio", &PyClient::enableAudio,
             "Enable/disable audio streaming")
        .def("enable_video", &PyClient::enableVideo,
             "Enable/disable video streaming")
        .def("enableVideo", &PyClient::enableVideo,
             "Enable/disable video streaming")
        .def("enable_transcript", &PyClient::enableTranscript,
             "Enable/disable transcript streaming")
        .def("enableTranscript", &PyClient::enableTranscript,
             "Enable/disable transcript streaming")
        .def("enable_deskshare", &PyClient::enableDeskshare,
             "Enable/disable deskshare streaming")
        .def("enableDeskshare", &PyClient::enableDeskshare,
             "Enable/disable deskshare streaming")
        .def("set_audio_params", &PyClient::setAudioParams,
             "Set audio parameters")
        .def("setAudioParams", &PyClient::setAudioParams,
             "Set audio parameters")
        .def("set_video_params", &PyClient::setVideoParams,
             "Set video parameters")
        .def("setVideoParams", &PyClient::setVideoParams,
             "Set video parameters")
        .def("set_deskshare_params", &PyClient::setDeskshareParams,
             "Set deskshare parameters")
        .def("setDeskshareParams", &PyClient::setDeskshareParams,
             "Set deskshare parameters")
        .def("set_transcript_params", &PyClient::setTranscriptParams,
             "Set transcript parameters")
        .def("setTranscriptParams", &PyClient::setTranscriptParams,
             "Set transcript parameters")
        .def("set_proxy", &PyClient::setProxy,
             "Set proxy for SDK connections",
             py::arg("proxy_type"), py::arg("proxy_url"))
        .def("setProxy", &PyClient::setProxy,
             "Set proxy for SDK connections",
             py::arg("proxy_type"), py::arg("proxy_url"))
        .def("on_join_confirm", &PyClient::onJoinConfirm,
             "Register join confirm callback")
        .def("onJoinConfirm", &PyClient::onJoinConfirm,
             "Register join confirm callback")
        .def("on_session_update", &PyClient::onSessionUpdate,
             "Register session update callback")
        .def("onSessionUpdate", &PyClient::onSessionUpdate,
             "Register session update callback")
        .def("on_user_update", &PyClient::onUserUpdate,
             "Register user update callback")
        .def("onUserUpdate", &PyClient::onUserUpdate,
             "Register user update callback")
        .def("on_audio_data", &PyClient::onAudioData,
             "Register audio data callback")
        .def("onAudioData", &PyClient::onAudioData,
             "Register audio data callback")
        .def("on_video_data", &PyClient::onVideoData,
             "Register video data callback")
        .def("onVideoData", &PyClient::onVideoData,
             "Register video data callback")
        .def("on_deskshare_data", &PyClient::onDeskshareData,
             "Register deskshare data callback")
        .def("onDeskshareData", &PyClient::onDeskshareData,
             "Register deskshare data callback")
        .def("on_transcript_data", &PyClient::onTranscriptData,
             "Register transcript data callback")
        .def("onTranscriptData", &PyClient::onTranscriptData,
             "Register transcript data callback")
        .def("on_leave", &PyClient::onLeave,
             "Register leave callback")
        .def("onLeave", &PyClient::onLeave,
             "Register leave callback")
        .def("on_event_ex", &PyClient::onEventEx,
             "Register extended event callback")
        .def("onEventEx", &PyClient::onEventEx,
             "Register extended event callback")
        .def("subscribe_video", &PyClient::subscribeVideo,
             "Subscribe or unsubscribe from an individual participant's video stream",
             py::arg("user_id"), py::arg("subscribe"))
        .def("subscribeVideo", &PyClient::subscribeVideo,
             "Subscribe or unsubscribe from an individual participant's video stream",
             py::arg("user_id"), py::arg("subscribe"))
        .def("on_participant_video", &PyClient::onParticipantVideo,
             "Register callback for participant video state changes")
        .def("onParticipantVideo", &PyClient::onParticipantVideo,
             "Register callback for participant video state changes")
        .def("on_video_subscribed", &PyClient::onVideoSubscribed,
             "Register callback for video subscription responses")
        .def("onVideoSubscribed", &PyClient::onVideoSubscribed,
             "Register callback for video subscription responses")
        .def("subscribe_event", &PyClient::subscribeEvent,
             "Subscribe to receive specific event types",
             py::arg("events"))
        .def("subscribeEvent", &PyClient::subscribeEvent,
             "Subscribe to receive specific event types",
             py::arg("events"))
        .def("unsubscribe_event", &PyClient::unsubscribeEvent,
             "Unsubscribe from specific event types",
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

    m.attr("EVENT_UNDEFINED") = py::int_(static_cast<int>(RTMS_EVENT_TYPE::UNDEFINED));
    m.attr("EVENT_FIRST_PACKET_TIMESTAMP") = py::int_(static_cast<int>(RTMS_EVENT_TYPE::FIRST_PACKET_TIMESTAMP));
    m.attr("EVENT_ACTIVE_SPEAKER_CHANGE") = py::int_(static_cast<int>(RTMS_EVENT_TYPE::ACTIVE_SPEAKER_CHANGE));
    m.attr("EVENT_PARTICIPANT_JOIN") = py::int_(static_cast<int>(RTMS_EVENT_TYPE::PARTICIPANT_JOIN));
    m.attr("EVENT_PARTICIPANT_LEAVE") = py::int_(static_cast<int>(RTMS_EVENT_TYPE::PARTICIPANT_LEAVE));
    m.attr("EVENT_SHARING_START") = py::int_(static_cast<int>(RTMS_EVENT_TYPE::SHARING_START));
    m.attr("EVENT_SHARING_STOP") = py::int_(static_cast<int>(RTMS_EVENT_TYPE::SHARING_STOP));
    m.attr("EVENT_MEDIA_CONNECTION_INTERRUPTED") = py::int_(static_cast<int>(RTMS_EVENT_TYPE::MEDIA_CONNECTION_INTERRUPTED));
    m.attr("EVENT_CONSUMER_ANSWERED") = py::int_(static_cast<int>(RTMS_ZCC_VOICE_EVENT_TYPE::CONSUMER_ANSWERED));
    m.attr("EVENT_CONSUMER_END") = py::int_(static_cast<int>(RTMS_ZCC_VOICE_EVENT_TYPE::CONSUMER_END));
    m.attr("EVENT_USER_ANSWERED") = py::int_(static_cast<int>(RTMS_ZCC_VOICE_EVENT_TYPE::USER_ANSWERED));
    m.attr("EVENT_USER_END") = py::int_(static_cast<int>(RTMS_ZCC_VOICE_EVENT_TYPE::USER_END));
    m.attr("EVENT_USER_HOLD") = py::int_(static_cast<int>(RTMS_ZCC_VOICE_EVENT_TYPE::USER_HOLD));
    m.attr("EVENT_USER_UNHOLD") = py::int_(static_cast<int>(RTMS_ZCC_VOICE_EVENT_TYPE::USER_UNHOLD));

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
    audioContentType["UNDEFINED"]   = (int)MEDIA_CONTENT_TYPE::UNDEFINED;
    audioContentType["RTP"]         = (int)MEDIA_CONTENT_TYPE::RTP;
    audioContentType["RAW_AUDIO"]   = (int)MEDIA_CONTENT_TYPE::RAW_AUDIO;
    audioContentType["FILE_STREAM"] = (int)MEDIA_CONTENT_TYPE::FILE_STREAM;
    audioContentType["TEXT"]        = (int)MEDIA_CONTENT_TYPE::TEXT;
    m.attr("AudioContentType") = audioContentType;

    py::dict audioCodec;
    audioCodec["UNDEFINED"] = (int)MEDIA_PAYLOAD_TYPE::UNDEFINED;
    audioCodec["L16"]       = (int)MEDIA_PAYLOAD_TYPE::L16;
    audioCodec["G711"]      = (int)MEDIA_PAYLOAD_TYPE::G711;
    audioCodec["G722"]      = (int)MEDIA_PAYLOAD_TYPE::G722;
    audioCodec["OPUS"]      = (int)MEDIA_PAYLOAD_TYPE::OPUS;
    m.attr("AudioCodec") = audioCodec;

    py::dict audioSampleRate;
    audioSampleRate["SR_8K"]  = (int)AUDIO_SAMPLE_RATE::SR_8K;
    audioSampleRate["SR_16K"] = (int)AUDIO_SAMPLE_RATE::SR_16K;
    audioSampleRate["SR_32K"] = (int)AUDIO_SAMPLE_RATE::SR_32K;
    audioSampleRate["SR_48K"] = (int)AUDIO_SAMPLE_RATE::SR_48K;
    m.attr("AudioSampleRate") = audioSampleRate;

    py::dict audioChannel;
    audioChannel["MONO"]   = (int)AUDIO_CHANNEL::MONO;
    audioChannel["STEREO"] = (int)AUDIO_CHANNEL::STEREO;
    m.attr("AudioChannel") = audioChannel;

    py::dict audioDataOption;
    audioDataOption["UNDEFINED"]           = (int)MEDIA_DATA_OPTION::UNDEFINED;
    audioDataOption["AUDIO_MIXED_STREAM"]  = (int)MEDIA_DATA_OPTION::AUDIO_MIXED_STREAM;
    audioDataOption["AUDIO_MULTI_STREAMS"] = (int)MEDIA_DATA_OPTION::AUDIO_MULTI_STREAMS;
    m.attr("AudioDataOption") = audioDataOption;

    // ========================================================================
    // Constants - Video Parameters
    // ========================================================================

    py::dict videoContentType;
    videoContentType["UNDEFINED"]   = (int)MEDIA_CONTENT_TYPE::UNDEFINED;
    videoContentType["RTP"]         = (int)MEDIA_CONTENT_TYPE::RTP;
    videoContentType["RAW_VIDEO"]   = (int)MEDIA_CONTENT_TYPE::RAW_VIDEO;
    videoContentType["FILE_STREAM"] = (int)MEDIA_CONTENT_TYPE::FILE_STREAM;
    videoContentType["TEXT"]        = (int)MEDIA_CONTENT_TYPE::TEXT;
    m.attr("VideoContentType") = videoContentType;

    py::dict videoCodec;
    videoCodec["UNDEFINED"] = (int)MEDIA_PAYLOAD_TYPE::UNDEFINED;
    videoCodec["JPG"]       = (int)MEDIA_PAYLOAD_TYPE::JPG;
    videoCodec["PNG"]       = (int)MEDIA_PAYLOAD_TYPE::PNG;
    videoCodec["H264"]      = (int)MEDIA_PAYLOAD_TYPE::H264;
    m.attr("VideoCodec") = videoCodec;

    py::dict videoResolution;
    videoResolution["SD"]  = (int)MEDIA_RESOLUTION::SD;
    videoResolution["HD"]  = (int)MEDIA_RESOLUTION::HD;
    videoResolution["FHD"] = (int)MEDIA_RESOLUTION::FHD;
    videoResolution["QHD"] = (int)MEDIA_RESOLUTION::QHD;
    m.attr("VideoResolution") = videoResolution;

    py::dict videoDataOption;
    videoDataOption["UNDEFINED"]                  = (int)MEDIA_DATA_OPTION::UNDEFINED;
    videoDataOption["VIDEO_SINGLE_ACTIVE_STREAM"] = (int)MEDIA_DATA_OPTION::VIDEO_SINGLE_ACTIVE_STREAM;
    videoDataOption["VIDEO_MIXED_SPEAKER_VIEW"]   = (int)MEDIA_DATA_OPTION::VIDEO_SINGLE_INDIVIDUAL_STREAM;
    videoDataOption["VIDEO_MIXED_GALLERY_VIEW"]   = (int)MEDIA_DATA_OPTION::VIDEO_MIXED_GALLERY_VIEW;
    m.attr("VideoDataOption") = videoDataOption;

    // ========================================================================
    // Constants - Media Data Types
    // ========================================================================

    py::dict mediaDataType;
    mediaDataType["UNDEFINED"]  = (int)MEDIA_DATA_TYPE::UNDEFINED;
    mediaDataType["AUDIO"]      = (int)MEDIA_DATA_TYPE::AUDIO;
    mediaDataType["VIDEO"]      = (int)MEDIA_DATA_TYPE::VIDEO;
    mediaDataType["DESKSHARE"]  = (int)MEDIA_DATA_TYPE::DESKSHARE;
    mediaDataType["TRANSCRIPT"] = (int)MEDIA_DATA_TYPE::TRANSCRIPT;
    mediaDataType["CHAT"]       = (int)MEDIA_DATA_TYPE::CHAT;
    mediaDataType["ALL"]        = (int)MEDIA_DATA_TYPE::ALL;
    m.attr("MediaDataType") = mediaDataType;

    // ========================================================================
    // Constants - Session State
    // ========================================================================

    py::dict sessionState;
    sessionState["INACTIVE"]   = (int)RTMS_SESSION_STATE::INACTIVE;
    sessionState["INITIALIZE"] = (int)RTMS_SESSION_STATE::INITIALIZE;
    sessionState["STARTED"]    = (int)RTMS_SESSION_STATE::STARTED;
    sessionState["PAUSED"]     = (int)RTMS_SESSION_STATE::PAUSED;
    sessionState["RESUMED"]    = (int)RTMS_SESSION_STATE::RESUMED;
    sessionState["STOPPED"]    = (int)RTMS_SESSION_STATE::STOPPED;
    m.attr("SessionState") = sessionState;

    // ========================================================================
    // Constants - Stream State
    // ========================================================================

    py::dict streamState;
    streamState["INACTIVE"]    = (int)RTMS_STREAM_STATE::INACTIVE;
    streamState["ACTIVE"]      = (int)RTMS_STREAM_STATE::ACTIVE;
    streamState["INTERRUPTED"] = (int)RTMS_STREAM_STATE::INTERRUPTED;
    streamState["TERMINATING"] = (int)RTMS_STREAM_STATE::TERMINATING;
    streamState["TERMINATED"]  = (int)RTMS_STREAM_STATE::TERMINATED;
    streamState["PAUSED"]      = (int)RTMS_STREAM_STATE::PAUSED;
    streamState["RESUMED"]     = (int)RTMS_STREAM_STATE::RESUMED;
    m.attr("StreamState") = streamState;

    // ========================================================================
    // Constants - Event Type (matches RTMS_EVENT_TYPE from Zoom's C SDK)
    // ========================================================================

    py::dict eventType;
    eventType["UNDEFINED"]                    = (int)RTMS_EVENT_TYPE::UNDEFINED;
    eventType["FIRST_PACKET_TIMESTAMP"]       = (int)RTMS_EVENT_TYPE::FIRST_PACKET_TIMESTAMP;
    eventType["ACTIVE_SPEAKER_CHANGE"]        = (int)RTMS_EVENT_TYPE::ACTIVE_SPEAKER_CHANGE;
    eventType["PARTICIPANT_JOIN"]             = (int)RTMS_EVENT_TYPE::PARTICIPANT_JOIN;
    eventType["PARTICIPANT_LEAVE"]            = (int)RTMS_EVENT_TYPE::PARTICIPANT_LEAVE;
    eventType["SHARING_START"]                = (int)RTMS_EVENT_TYPE::SHARING_START;
    eventType["SHARING_STOP"]                 = (int)RTMS_EVENT_TYPE::SHARING_STOP;
    eventType["MEDIA_CONNECTION_INTERRUPTED"] = (int)RTMS_EVENT_TYPE::MEDIA_CONNECTION_INTERRUPTED;
    eventType["PARTICIPANT_VIDEO_ON"]         = (int)RTMS_EVENT_TYPE::PARTICIPANT_VIDEO_ON;
    eventType["PARTICIPANT_VIDEO_OFF"]        = (int)RTMS_EVENT_TYPE::PARTICIPANT_VIDEO_OFF;
    // ZCC voice events
    eventType["CONSUMER_ANSWERED"] = (int)RTMS_ZCC_VOICE_EVENT_TYPE::CONSUMER_ANSWERED;
    eventType["CONSUMER_END"]      = (int)RTMS_ZCC_VOICE_EVENT_TYPE::CONSUMER_END;
    eventType["USER_ANSWERED"]     = (int)RTMS_ZCC_VOICE_EVENT_TYPE::USER_ANSWERED;
    eventType["USER_END"]          = (int)RTMS_ZCC_VOICE_EVENT_TYPE::USER_END;
    eventType["USER_HOLD"]         = (int)RTMS_ZCC_VOICE_EVENT_TYPE::USER_HOLD;
    eventType["USER_UNHOLD"]       = (int)RTMS_ZCC_VOICE_EVENT_TYPE::USER_UNHOLD;
    m.attr("EventType") = eventType;

    // ========================================================================
    // Constants - Message Type
    // ========================================================================

    py::dict messageType;
    messageType["UNDEFINED"]                 = (int)RTMS_MESSAGE_TYPE::UNDEFINED;
    messageType["SIGNALING_HAND_SHAKE_REQ"]  = (int)RTMS_MESSAGE_TYPE::SIGNALING_HAND_SHAKE_REQ;
    messageType["SIGNALING_HAND_SHAKE_RESP"] = (int)RTMS_MESSAGE_TYPE::SIGNALING_HAND_SHAKE_RESP;
    messageType["DATA_HAND_SHAKE_REQ"]       = (int)RTMS_MESSAGE_TYPE::DATA_HAND_SHAKE_REQ;
    messageType["DATA_HAND_SHAKE_RESP"]      = (int)RTMS_MESSAGE_TYPE::DATA_HAND_SHAKE_RESP;
    messageType["EVENT_SUBSCRIPTION"]        = (int)RTMS_MESSAGE_TYPE::EVENT_SUBSCRIPTION;
    messageType["EVENT_UPDATE"]              = (int)RTMS_MESSAGE_TYPE::EVENT_UPDATE;
    messageType["CLIENT_READY_ACK"]          = (int)RTMS_MESSAGE_TYPE::CLIENT_READY_ACK;
    messageType["STREAM_STATE_UPDATE"]       = (int)RTMS_MESSAGE_TYPE::STREAM_STATE_UPDATE;
    messageType["SESSION_STATE_UPDATE"]      = (int)RTMS_MESSAGE_TYPE::SESSION_STATE_UPDATE;
    messageType["SESSION_STATE_REQ"]         = (int)RTMS_MESSAGE_TYPE::SESSION_STATE_REQ;
    messageType["SESSION_STATE_RESP"]        = (int)RTMS_MESSAGE_TYPE::SESSION_STATE_RESP;
    messageType["KEEP_ALIVE_REQ"]            = (int)RTMS_MESSAGE_TYPE::KEEP_ALIVE_REQ;
    messageType["KEEP_ALIVE_RESP"]           = (int)RTMS_MESSAGE_TYPE::KEEP_ALIVE_RESP;
    messageType["MEDIA_DATA_AUDIO"]          = (int)RTMS_MESSAGE_TYPE::MEDIA_DATA_AUDIO;
    messageType["MEDIA_DATA_VIDEO"]          = (int)RTMS_MESSAGE_TYPE::MEDIA_DATA_VIDEO;
    messageType["MEDIA_DATA_SHARE"]          = (int)RTMS_MESSAGE_TYPE::MEDIA_DATA_SHARE;
    messageType["MEDIA_DATA_TRANSCRIPT"]     = (int)RTMS_MESSAGE_TYPE::MEDIA_DATA_TRANSCRIPT;
    messageType["MEDIA_DATA_CHAT"]           = (int)RTMS_MESSAGE_TYPE::MEDIA_DATA_CHAT;
    messageType["STREAM_STATE_REQ"]          = (int)RTMS_MESSAGE_TYPE::STREAM_STATE_REQ;
    messageType["STREAM_STATE_RESP"]         = (int)RTMS_MESSAGE_TYPE::STREAM_STATE_RESP;
    messageType["STREAM_CLOSE_REQ"]          = (int)RTMS_MESSAGE_TYPE::STREAM_CLOSE_REQ;
    messageType["STREAM_CLOSE_RESP"]         = (int)RTMS_MESSAGE_TYPE::STREAM_CLOSE_RESP;
    messageType["META_DATA_AUDIO"]           = (int)RTMS_MESSAGE_TYPE::META_DATA_AUDIO;
    messageType["META_DATA_VIDEO"]           = (int)RTMS_MESSAGE_TYPE::META_DATA_VIDEO;
    messageType["META_DATA_SHARE"]           = (int)RTMS_MESSAGE_TYPE::META_DATA_SHARE;
    messageType["META_DATA_TRANSCRIPT"]      = (int)RTMS_MESSAGE_TYPE::META_DATA_TRANSCRIPT;
    messageType["META_DATA_CHAT"]            = (int)RTMS_MESSAGE_TYPE::META_DATA_CHAT;
    messageType["VIDEO_SUBSCRIPTION_REQ"]    = (int)RTMS_MESSAGE_TYPE::VIDEO_SUBSCRIPTION_REQ;
    messageType["VIDEO_SUBSCRIPTION_RESP"]   = (int)RTMS_MESSAGE_TYPE::VIDEO_SUBSCRIPTION_RESP;
    m.attr("MessageType") = messageType;

    // ========================================================================
    // Constants - Stop Reason
    // ========================================================================

    py::dict stopReason;
    stopReason["UNDEFINED"]                               = (int)RTMS_STOP_REASON::UNDEFINED;
    stopReason["HOST_TRIGGERED"]                          = (int)RTMS_STOP_REASON::HOST_TRIGGERED;
    stopReason["USER_TRIGGERED"]                          = (int)RTMS_STOP_REASON::USER_TRIGGERED;
    stopReason["USER_LEFT"]                               = (int)RTMS_STOP_REASON::USER_LEFT;
    stopReason["USER_EJECTED"]                            = (int)RTMS_STOP_REASON::USER_EJECTED;
    stopReason["HOST_DISABLED_APP"]                       = (int)RTMS_STOP_REASON::HOST_DISABLED_APP;
    stopReason["MEETING_ENDED"]                           = (int)RTMS_STOP_REASON::MEETING_ENDED;
    stopReason["STREAM_CANCELED"]                         = (int)RTMS_STOP_REASON::STREAM_CANCELED;
    stopReason["STREAM_REVOKED"]                          = (int)RTMS_STOP_REASON::STREAM_REVOKED;
    stopReason["ALL_APPS_DISABLED"]                       = (int)RTMS_STOP_REASON::ALL_APPS_DISABLED;
    stopReason["INTERNAL_EXCEPTION"]                      = (int)RTMS_STOP_REASON::INTERNAL_EXCEPTION;
    stopReason["CONNECTION_TIMEOUT"]                      = (int)RTMS_STOP_REASON::CONNECTION_TIMEOUT;
    stopReason["INSTANCE_CONNECTION_INTERRUPTED"]         = (int)RTMS_STOP_REASON::INSTANCE_CONNECTION_INTERRUPTED;
    stopReason["SIGNAL_CONNECTION_INTERRUPTED"]           = (int)RTMS_STOP_REASON::SIGNAL_CONNECTION_INTERRUPTED;
    stopReason["DATA_CONNECTION_INTERRUPTED"]             = (int)RTMS_STOP_REASON::DATA_CONNECTION_INTERRUPTED;
    stopReason["SIGNAL_CONNECTION_CLOSED_ABNORMALLY"]     = (int)RTMS_STOP_REASON::SIGNAL_CONNECTION_CLOSED_ABNORMALLY;
    stopReason["DATA_CONNECTION_CLOSED_ABNORMALLY"]       = (int)RTMS_STOP_REASON::DATA_CONNECTION_CLOSED_ABNORMALLY;
    stopReason["EXIT_SIGNAL"]                             = (int)RTMS_STOP_REASON::EXIT_SIGNAL;
    stopReason["AUTHENTICATION_FAILURE"]                  = (int)RTMS_STOP_REASON::AUTHENTICATION_FAILURE;
    stopReason["AWAIT_RECONNECTION_TIMEOUT"]              = (int)RTMS_STOP_REASON::AWAIT_RECONNECTION_TIMEOUT;
    stopReason["RECEIVER_REQUEST_CLOSE"]                  = (int)RTMS_STOP_REASON::RECEIVER_REQUEST_CLOSE;
    stopReason["CUSTOMER_DISCONNECTED"]                   = (int)RTMS_STOP_REASON::CUSTOMER_DISCONNECTED;
    stopReason["AGENT_DISCONNECTED"]                      = (int)RTMS_STOP_REASON::AGENT_DISCONNECTED;
    stopReason["ADMIN_DISABLED_APP"]                      = (int)RTMS_STOP_REASON::ADMIN_DISABLED_APP;
    stopReason["KEEP_ALIVE_TIMEOUT"]                      = (int)RTMS_STOP_REASON::KEEP_ALIVE_TIMEOUT;
    stopReason["MANUAL_API_TRIGGERED"]                    = (int)RTMS_STOP_REASON::MANUAL_API_TRIGGERED;
    stopReason["STREAMING_NOT_SUPPORTED"]                 = (int)RTMS_STOP_REASON::STREAMING_NOT_SUPPORTED;
    m.attr("StopReason") = stopReason;
}
