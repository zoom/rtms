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
             "Register extended event callback");

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

    m.attr("USER_EVENT_JOIN") = py::int_(static_cast<int>(USER_JOIN));
    m.attr("USER_EVENT_LEAVE") = py::int_(static_cast<int>(USER_LEAVE));

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
    audioContentType["UNDEFINED"] = 0;
    audioContentType["RTP"] = 1;
    audioContentType["RAW_AUDIO"] = 2;
    audioContentType["FILE_STREAM"] = 4;
    audioContentType["TEXT"] = 5;
    m.attr("AudioContentType") = audioContentType;

    py::dict audioCodec;
    audioCodec["UNDEFINED"] = 0;
    audioCodec["L16"] = 1;
    audioCodec["G711"] = 2;
    audioCodec["G722"] = 3;
    audioCodec["OPUS"] = 4;
    m.attr("AudioCodec") = audioCodec;

    py::dict audioSampleRate;
    audioSampleRate["SR_8K"] = 0;
    audioSampleRate["SR_16K"] = 1;
    audioSampleRate["SR_32K"] = 2;
    audioSampleRate["SR_48K"] = 3;
    m.attr("AudioSampleRate") = audioSampleRate;

    py::dict audioChannel;
    audioChannel["MONO"] = 1;
    audioChannel["STEREO"] = 2;
    m.attr("AudioChannel") = audioChannel;

    py::dict audioDataOption;
    audioDataOption["UNDEFINED"] = 0;
    audioDataOption["AUDIO_MIXED_STREAM"] = 1;
    audioDataOption["AUDIO_MULTI_STREAMS"] = 2;
    m.attr("AudioDataOption") = audioDataOption;

    // ========================================================================
    // Constants - Video Parameters
    // ========================================================================

    py::dict videoContentType;
    videoContentType["UNDEFINED"] = 0;
    videoContentType["RTP"] = 1;
    videoContentType["RAW_VIDEO"] = 3;
    videoContentType["FILE_STREAM"] = 4;
    videoContentType["TEXT"] = 5;
    m.attr("VideoContentType") = videoContentType;

    py::dict videoCodec;
    videoCodec["UNDEFINED"] = 0;
    videoCodec["JPG"] = 5;
    videoCodec["PNG"] = 6;
    videoCodec["H264"] = 7;
    m.attr("VideoCodec") = videoCodec;

    py::dict videoResolution;
    videoResolution["SD"] = 1;
    videoResolution["HD"] = 2;
    videoResolution["FHD"] = 3;
    videoResolution["QHD"] = 4;
    m.attr("VideoResolution") = videoResolution;

    py::dict videoDataOption;
    videoDataOption["UNDEFINED"] = 0;
    videoDataOption["VIDEO_SINGLE_ACTIVE_STREAM"] = 3;
    videoDataOption["VIDEO_MIXED_SPEAKER_VIEW"] = 4;
    videoDataOption["VIDEO_MIXED_GALLERY_VIEW"] = 5;
    m.attr("VideoDataOption") = videoDataOption;

    // ========================================================================
    // Constants - Media Data Types
    // ========================================================================

    py::dict mediaDataType;
    mediaDataType["UNDEFINED"] = 0;
    mediaDataType["AUDIO"] = 1;
    mediaDataType["VIDEO"] = 2;
    mediaDataType["DESKSHARE"] = 4;
    mediaDataType["TRANSCRIPT"] = 8;
    mediaDataType["CHAT"] = 16;
    mediaDataType["ALL"] = 31;
    m.attr("MediaDataType") = mediaDataType;

    // ========================================================================
    // Constants - Session State
    // ========================================================================

    py::dict sessionState;
    sessionState["INACTIVE"] = 0;
    sessionState["INITIALIZE"] = 1;
    sessionState["STARTED"] = 2;
    sessionState["PAUSED"] = 3;
    sessionState["RESUMED"] = 4;
    sessionState["STOPPED"] = 5;
    m.attr("SessionState") = sessionState;

    // ========================================================================
    // Constants - Stream State
    // ========================================================================

    py::dict streamState;
    streamState["INACTIVE"] = 0;
    streamState["ACTIVE"] = 1;
    streamState["INTERRUPTED"] = 2;
    streamState["TERMINATING"] = 3;
    streamState["TERMINATED"] = 4;
    m.attr("StreamState") = streamState;

    // ========================================================================
    // Constants - Event Type
    // ========================================================================

    py::dict eventType;
    eventType["UNDEFINED"] = 0;
    eventType["FIRST_PACKET_TIMESTAMP"] = 1;
    eventType["ACTIVE_SPEAKER_CHANGE"] = 2;
    eventType["PARTICIPANT_JOIN"] = 3;
    eventType["PARTICIPANT_LEAVE"] = 4;
    m.attr("EventType") = eventType;

    // ========================================================================
    // Constants - Message Type
    // ========================================================================

    py::dict messageType;
    messageType["UNDEFINED"] = 0;
    messageType["SIGNALING_HAND_SHAKE_REQ"] = 1;
    messageType["SIGNALING_HAND_SHAKE_RESP"] = 2;
    messageType["DATA_HAND_SHAKE_REQ"] = 3;
    messageType["DATA_HAND_SHAKE_RESP"] = 4;
    messageType["EVENT_SUBSCRIPTION"] = 5;
    messageType["EVENT_UPDATE"] = 6;
    messageType["CLIENT_READY_ACK"] = 7;
    messageType["STREAM_STATE_UPDATE"] = 8;
    messageType["SESSION_STATE_UPDATE"] = 9;
    messageType["SESSION_STATE_REQ"] = 10;
    messageType["SESSION_STATE_RESP"] = 11;
    messageType["KEEP_ALIVE_REQ"] = 12;
    messageType["KEEP_ALIVE_RESP"] = 13;
    messageType["MEDIA_DATA_AUDIO"] = 14;
    messageType["MEDIA_DATA_VIDEO"] = 15;
    messageType["MEDIA_DATA_SHARE"] = 16;
    messageType["MEDIA_DATA_TRANSCRIPT"] = 17;
    messageType["MEDIA_DATA_CHAT"] = 18;
    m.attr("MessageType") = messageType;

    // ========================================================================
    // Constants - Stop Reason
    // ========================================================================

    py::dict stopReason;
    stopReason["UNDEFINED"] = 0;
    stopReason["STOP_BC_HOST_TRIGGERED"] = 1;
    stopReason["STOP_BC_USER_TRIGGERED"] = 2;
    stopReason["STOP_BC_USER_LEFT"] = 3;
    stopReason["STOP_BC_USER_EJECTED"] = 4;
    stopReason["STOP_BC_APP_DISABLED_BY_HOST"] = 5;
    stopReason["STOP_BC_MEETING_ENDED"] = 6;
    stopReason["STOP_BC_STREAM_CANCELED"] = 7;
    stopReason["STOP_BC_STREAM_REVOKED"] = 8;
    stopReason["STOP_BC_ALL_APPS_DISABLED"] = 9;
    stopReason["STOP_BC_INTERNAL_EXCEPTION"] = 10;
    stopReason["STOP_BC_CONNECTION_TIMEOUT"] = 11;
    stopReason["STOP_BC_MEETING_CONNECTION_INTERRUPTED"] = 12;
    stopReason["STOP_BC_SIGNAL_CONNECTION_INTERRUPTED"] = 13;
    stopReason["STOP_BC_DATA_CONNECTION_INTERRUPTED"] = 14;
    stopReason["STOP_BC_SIGNAL_CONNECTION_CLOSED_ABNORMALLY"] = 15;
    stopReason["STOP_BC_DATA_CONNECTION_CLOSED_ABNORMALLY"] = 16;
    stopReason["STOP_BC_EXIT_SIGNAL"] = 17;
    stopReason["STOP_BC_AUTHENTICATION_FAILURE"] = 18;
    m.attr("StopReason") = stopReason;
}
