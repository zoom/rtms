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
    // Phase 1: safe to call from any thread — no C SDK interaction.
    PyClient() : client_(nullptr) {}

    ~PyClient() {
        clearCallbacks();
    }

    // Phase 2: allocates the C SDK handle. Must be called from the thread that
    // will own this client for its entire lifetime (join/poll/release must all
    // run on the same OS thread as alloc).
    //
    // Replays all callbacks and params that were buffered before alloc() so
    // that the caller can set up the client fully before calling alloc()+join().
    void alloc() {
        if (client_) return;  // idempotent
        client_ = std::make_unique<Client>();

        // Replay buffered callbacks
        if (!join_confirm_callback_.is_none())   _registerJoinConfirm();
        if (!session_update_callback_.is_none()) _registerSessionUpdate();
        if (!user_update_callback_.is_none())    _registerUserUpdate();
        if (!audio_data_callback_.is_none())     _registerAudioData();
        if (!video_data_callback_.is_none())     _registerVideoData();
        if (!deskshare_data_callback_.is_none()) _registerDeskshareData();
        if (!transcript_data_callback_.is_none()) _registerTranscriptData();
        if (!leave_callback_.is_none())          _registerLeave();
        if (!event_ex_callback_.is_none())       _registerEventEx();
        if (!participant_video_callback_.is_none()) _registerParticipantVideo();
        if (!video_subscribed_callback_.is_none())  _registerVideoSubscribed();

        // Replay buffered params
        if (pending_audio_params_)      client_->setAudioParams(*pending_audio_params_);
        if (pending_video_params_)      client_->setVideoParams(*pending_video_params_);
        if (pending_deskshare_params_)  client_->setDeskshareParams(*pending_deskshare_params_);
        if (pending_transcript_params_) client_->setTranscriptParams(*pending_transcript_params_);
        if (!pending_proxy_type_.empty()) client_->setProxy(pending_proxy_type_, pending_proxy_url_);
    }

    bool isAllocated() const { return client_ != nullptr; }

    // ========================================================================
    // Core Methods
    // ========================================================================

    void join(const std::string& uuid, const std::string& stream_id,
             const std::string& signature, const std::string& server_urls,
             int timeout = -1) {
        if (!client_) throw std::runtime_error("alloc() must be called before join()");
        client_->join(uuid, stream_id, signature, server_urls, timeout);
    }

    void poll() {
        // Release the GIL *before* acquiring poll_mutex_ to prevent deadlock:
        // release() holds poll_mutex_ while the webhook thread holds the GIL.
        // If poll() tried to acquire poll_mutex_ while holding the GIL, the two
        // threads would deadlock. Releasing the GIL first breaks the cycle.
        py::gil_scoped_release release;
        std::lock_guard<std::mutex> lk(poll_mutex_);
        if (client_) client_->poll();
    }

    void release() {
        if (!client_) return;
        // Hold poll_mutex_ for the entire release sequence so that any in-flight
        // poll() completes before we tear down the C SDK handle.
        std::lock_guard<std::mutex> lk(poll_mutex_);
        // markClosed() sets sdk_opened_=false so that stopCallbacks() calls
        // setOnAudioData/Video/etc. with empty lambdas without triggering
        // configure() on an already-dead session (avoids 4 spurious warnings).
        client_->markClosed();
        stopCallbacks();
        client_->release();
        client_.reset();  // prevent subsequent poll() from calling into released SDK
    }

    std::string uuid() const {
        return client_ ? client_->uuid() : "";
    }

    std::string streamId() const {
        return client_ ? client_->streamId() : "";
    }

    // ========================================================================
    // Media Control Methods
    // ========================================================================

    void enableAudio(bool enable) {
        if (client_) client_->enableAudio(enable);
    }

    void enableVideo(bool enable) {
        if (client_) client_->enableVideo(enable);
    }

    void enableTranscript(bool enable) {
        if (client_) client_->enableTranscript(enable);
    }

    void enableDeskshare(bool enable) {
        if (client_) client_->enableDeskshare(enable);
    }

    // ========================================================================
    // Parameter Setting Methods
    // Buffer params pre-alloc; apply immediately post-alloc.
    // ========================================================================

    void setAudioParams(const AudioParams& params) {
        pending_audio_params_ = std::make_unique<AudioParams>(params);
        if (client_) client_->setAudioParams(params);
    }

    void setVideoParams(const VideoParams& params) {
        pending_video_params_ = std::make_unique<VideoParams>(params);
        if (client_) client_->setVideoParams(params);
    }

    void setDeskshareParams(const DeskshareParams& params) {
        pending_deskshare_params_ = std::make_unique<DeskshareParams>(params);
        if (client_) client_->setDeskshareParams(params);
    }

    void setTranscriptParams(const TranscriptParams& params) {
        pending_transcript_params_ = std::make_unique<TranscriptParams>(params);
        if (client_) client_->setTranscriptParams(params);
    }

    void setProxy(const std::string& proxy_type, const std::string& proxy_url) {
        pending_proxy_type_ = proxy_type;
        pending_proxy_url_ = proxy_url;
        if (client_) client_->setProxy(proxy_type, proxy_url);
    }

    // ========================================================================
    // Callback Registration Methods
    // Buffer callback pre-alloc; register with C SDK immediately post-alloc.
    // ========================================================================

    void onJoinConfirm(py::function callback) {
        join_confirm_callback_ = callback;
        if (client_) _registerJoinConfirm();
    }

    void onSessionUpdate(py::function callback) {
        session_update_callback_ = callback;
        if (client_) _registerSessionUpdate();
    }

    void onUserUpdate(py::function callback) {
        user_update_callback_ = callback;
        if (client_) _registerUserUpdate();
    }

    void onAudioData(py::function callback) {
        audio_data_callback_ = callback;
        if (client_) _registerAudioData();
    }

    void onVideoData(py::function callback) {
        video_data_callback_ = callback;
        if (client_) _registerVideoData();
    }

    void onDeskshareData(py::function callback) {
        deskshare_data_callback_ = callback;
        if (client_) _registerDeskshareData();
    }

    void onTranscriptData(py::function callback) {
        transcript_data_callback_ = callback;
        if (client_) _registerTranscriptData();
    }

    void onLeave(py::function callback) {
        leave_callback_ = callback;
        if (client_) _registerLeave();
    }

    void onEventEx(py::function callback) {
        event_ex_callback_ = callback;
        if (client_) _registerEventEx();
    }

    // ========================================================================
    // Individual Video Subscription
    // ========================================================================

    void subscribeVideo(int user_id, bool subscribe) {
        if (!client_) throw std::runtime_error("alloc() must be called before subscribeVideo()");
        client_->subscribeVideo(user_id, subscribe);
    }

    void onParticipantVideo(py::function callback) {
        participant_video_callback_ = callback;
        if (client_) _registerParticipantVideo();
    }

    void onVideoSubscribed(py::function callback) {
        video_subscribed_callback_ = callback;
        if (client_) _registerVideoSubscribed();
    }

    // ========================================================================
    // Event Subscription Methods
    // ========================================================================

    void subscribeEvent(const std::vector<int>& events) {
        if (!client_) {
            // Buffer for replay after alloc
            pending_subscriptions_.insert(pending_subscriptions_.end(), events.begin(), events.end());
            return;
        }
        client_->subscribeEvent(events);
    }

    void unsubscribeEvent(const std::vector<int>& events) {
        if (client_) client_->unsubscribeEvent(events);
    }

private:
    std::unique_ptr<Client> client_;
    std::mutex poll_mutex_;  // guards poll() vs release() race

    // Python callback storage (buffered pre-alloc, registered post-alloc)
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

    // Param buffers (applied on alloc)
    std::unique_ptr<AudioParams>      pending_audio_params_;
    std::unique_ptr<VideoParams>      pending_video_params_;
    std::unique_ptr<DeskshareParams>  pending_deskshare_params_;
    std::unique_ptr<TranscriptParams> pending_transcript_params_;
    std::string pending_proxy_type_;
    std::string pending_proxy_url_;
    std::vector<int> pending_subscriptions_;

    // ── Private registration helpers ────────────────────────────────────────
    // Each helper wires one stored py::object into the C++ Client.
    // Called from alloc() (replay) and from the public setter (live update).

    void _registerJoinConfirm() {
        client_->setOnJoinConfirm([this](int reason) {
            if (!join_confirm_callback_.is_none()) {
                py::gil_scoped_acquire acquire;
                try { join_confirm_callback_(reason); }
                catch (const py::error_already_set& e) { py::print("Error in join_confirm callback:", e.what()); }
            }
        });
    }

    void _registerSessionUpdate() {
        client_->setOnSessionUpdate([this](int op, const Session& session) {
            if (!session_update_callback_.is_none()) {
                py::gil_scoped_acquire acquire;
                try { session_update_callback_(op, session); }
                catch (const py::error_already_set& e) { py::print("Error in session_update callback:", e.what()); }
            }
        });
    }

    void _registerUserUpdate() {
        client_->setOnUserUpdate([this](int op, const Participant& participant) {
            if (!user_update_callback_.is_none()) {
                py::gil_scoped_acquire acquire;
                try { user_update_callback_(op, participant); }
                catch (const py::error_already_set& e) { py::print("Error in user_update callback:", e.what()); }
            }
        });
    }

    void _registerAudioData() {
        client_->setOnAudioData([this](const std::vector<uint8_t>& data, uint64_t timestamp, const Metadata& metadata) {
            if (!audio_data_callback_.is_none()) {
                py::gil_scoped_acquire acquire;
                try {
                    py::bytes py_data(reinterpret_cast<const char*>(data.data()), data.size());
                    audio_data_callback_(py_data, data.size(), timestamp, metadata);
                } catch (const py::error_already_set& e) { py::print("Error in audio_data callback:", e.what()); }
            }
        });
    }

    void _registerVideoData() {
        client_->setOnVideoData([this](const std::vector<uint8_t>& data, uint64_t timestamp, const Metadata& metadata) {
            if (!video_data_callback_.is_none()) {
                py::gil_scoped_acquire acquire;
                try {
                    py::bytes py_data(reinterpret_cast<const char*>(data.data()), data.size());
                    video_data_callback_(py_data, data.size(), timestamp, metadata);
                } catch (const py::error_already_set& e) { py::print("Error in video_data callback:", e.what()); }
            }
        });
    }

    void _registerDeskshareData() {
        client_->setOnDeskshareData([this](const std::vector<uint8_t>& data, uint64_t timestamp, const Metadata& metadata) {
            if (!deskshare_data_callback_.is_none()) {
                py::gil_scoped_acquire acquire;
                try {
                    py::bytes py_data(reinterpret_cast<const char*>(data.data()), data.size());
                    deskshare_data_callback_(py_data, data.size(), timestamp, metadata);
                } catch (const py::error_already_set& e) { py::print("Error in deskshare_data callback:", e.what()); }
            }
        });
    }

    void _registerTranscriptData() {
        client_->setOnTranscriptData([this](const std::vector<uint8_t>& data, uint64_t timestamp, const Metadata& metadata) {
            if (!transcript_data_callback_.is_none()) {
                py::gil_scoped_acquire acquire;
                try {
                    py::bytes py_data(reinterpret_cast<const char*>(data.data()), data.size());
                    transcript_data_callback_(py_data, data.size(), timestamp, metadata);
                } catch (const py::error_already_set& e) { py::print("Error in transcript_data callback:", e.what()); }
            }
        });
    }

    void _registerLeave() {
        client_->setOnLeave([this](int reason) {
            if (!leave_callback_.is_none()) {
                py::gil_scoped_acquire acquire;
                try { leave_callback_(reason); }
                catch (const py::error_already_set& e) { py::print("Error in leave callback:", e.what()); }
            }
        });
    }

    void _registerEventEx() {
        client_->setOnEventEx([this](const std::string& event_data) {
            if (!event_ex_callback_.is_none()) {
                py::gil_scoped_acquire acquire;
                try { event_ex_callback_(event_data); }
                catch (const py::error_already_set& e) { py::print("Error in event_ex callback:", e.what()); }
            }
        });
    }

    void _registerParticipantVideo() {
        client_->setOnParticipantVideo([this](const std::vector<int>& users, bool is_on) {
            if (!participant_video_callback_.is_none()) {
                py::gil_scoped_acquire acquire;
                try { participant_video_callback_(users, is_on); }
                catch (const py::error_already_set& e) { py::print("Error in participant_video callback:", e.what()); }
            }
        });
    }

    void _registerVideoSubscribed() {
        client_->setOnVideoSubscribed([this](int user_id, int status, const std::string& error) {
            if (!video_subscribed_callback_.is_none()) {
                py::gil_scoped_acquire acquire;
                try { video_subscribed_callback_(user_id, status, error); }
                catch (const py::error_already_set& e) { py::print("Error in video_subscribed callback:", e.what()); }
            }
        });
    }

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

    py::class_<AiTargetLanguage>(m, "AiTargetLanguage")
        .def_property_readonly("lid", &AiTargetLanguage::lid)
        .def_property_readonly("toneId", &AiTargetLanguage::toneId)
        .def_property_readonly("voiceId", &AiTargetLanguage::voiceId)
        .def_property_readonly("engine", &AiTargetLanguage::engine);

    py::class_<AiInterpreter>(m, "AiInterpreter")
        .def_property_readonly("lid", &AiInterpreter::lid)
        .def_property_readonly("timestamp", &AiInterpreter::timestamp)
        .def_property_readonly("channelNum", &AiInterpreter::channelNum)
        .def_property_readonly("sampleRate", &AiInterpreter::sampleRate)
        .def_property_readonly("targets", &AiInterpreter::targets);

    py::class_<Metadata>(m, "Metadata")
        .def_property_readonly("userName", &Metadata::userName)
        .def_property_readonly("userId", &Metadata::userId)
        .def_property_readonly("startTs", &Metadata::startTs)
        .def_property_readonly("endTs", &Metadata::endTs)
        .def_property_readonly("aiInterpreter", &Metadata::aiInterpreter);

    // ========================================================================
    // Parameter Classes
    // ========================================================================

    py::class_<AudioParams>(m, "AudioParams")
        .def(py::init<>())
        .def(py::init<int, int, int, int, int, int, int>(),
             py::arg("content_type"), py::arg("codec"), py::arg("sample_rate"),
             py::arg("channel"), py::arg("data_opt"), py::arg("duration"),
             py::arg("frame_size"))
        // camelCase (primary — kept for backwards compat)
        .def_property("contentType", &AudioParams::contentType, &AudioParams::setContentType)
        .def_property("codec",       &AudioParams::codec,       &AudioParams::setCodec)
        .def_property("sampleRate",  &AudioParams::sampleRate,  &AudioParams::setSampleRate)
        .def_property("channel",     &AudioParams::channel,     &AudioParams::setChannel)
        .def_property("dataOpt",     &AudioParams::dataOpt,     &AudioParams::setDataOpt)
        .def_property("duration",    &AudioParams::duration,    &AudioParams::setDuration)
        .def_property("frameSize",   &AudioParams::frameSize,   &AudioParams::setFrameSize)
        // snake_case aliases
        .def_property("content_type", &AudioParams::contentType, &AudioParams::setContentType)
        .def_property("sample_rate",  &AudioParams::sampleRate,  &AudioParams::setSampleRate)
        .def_property("data_opt",     &AudioParams::dataOpt,     &AudioParams::setDataOpt)
        .def_property("frame_size",   &AudioParams::frameSize,   &AudioParams::setFrameSize);

    py::class_<VideoParams>(m, "VideoParams")
        .def(py::init<>())
        .def(py::init<int, int, int, int, int>(),
             py::arg("content_type"), py::arg("codec"), py::arg("resolution"),
             py::arg("data_opt"), py::arg("fps"))
        // camelCase (primary — kept for backwards compat)
        .def_property("contentType", &VideoParams::contentType, &VideoParams::setContentType)
        .def_property("codec",       &VideoParams::codec,       &VideoParams::setCodec)
        .def_property("resolution",  &VideoParams::resolution,  &VideoParams::setResolution)
        .def_property("dataOpt",     &VideoParams::dataOpt,     &VideoParams::setDataOpt)
        .def_property("fps",         &VideoParams::fps,         &VideoParams::setFps)
        // snake_case aliases
        .def_property("content_type", &VideoParams::contentType, &VideoParams::setContentType)
        .def_property("data_opt",     &VideoParams::dataOpt,     &VideoParams::setDataOpt);

    py::class_<DeskshareParams>(m, "DeskshareParams")
        .def(py::init<>())
        .def(py::init<int, int, int, int>(),
             py::arg("content_type"), py::arg("codec"), py::arg("resolution"),
             py::arg("fps"))
        // camelCase (primary — kept for backwards compat)
        .def_property("contentType", &DeskshareParams::contentType, &DeskshareParams::setContentType)
        .def_property("codec",       &DeskshareParams::codec,       &DeskshareParams::setCodec)
        .def_property("resolution",  &DeskshareParams::resolution,  &DeskshareParams::setResolution)
        .def_property("fps",         &DeskshareParams::fps,         &DeskshareParams::setFps)
        // snake_case aliases
        .def_property("content_type", &DeskshareParams::contentType, &DeskshareParams::setContentType)
        .def_property("data_opt",     &DeskshareParams::dataOpt,     &DeskshareParams::setDataOpt);

    py::class_<TranscriptParams>(m, "TranscriptParams")
        .def(py::init<>())
        // snake_case (primary)
        .def_property("content_type", &TranscriptParams::contentType,  &TranscriptParams::setContentType)
        .def_property("src_language", &TranscriptParams::srcLanguage,  &TranscriptParams::setSrcLanguage)
        .def_property("enable_lid",   &TranscriptParams::enableLid,    &TranscriptParams::setEnableLid)
        // camelCase aliases
        .def_property("contentType",  &TranscriptParams::contentType,  &TranscriptParams::setContentType)
        .def_property("srcLanguage",  &TranscriptParams::srcLanguage,  &TranscriptParams::setSrcLanguage)
        .def_property("enableLid",    &TranscriptParams::enableLid,    &TranscriptParams::setEnableLid);

    // TranscriptLanguage constants dict (matches pattern of AudioCodec, VideoCodec, etc.)
    // Values sourced from the TRANSCRIPT_LANGUAGE enum in rtms.h
    py::dict transcriptLanguage;
    transcriptLanguage["NONE"]                = (int)TRANSCRIPT_LANGUAGE::NONE;
    transcriptLanguage["ARABIC"]              = (int)TRANSCRIPT_LANGUAGE::ARABIC;
    transcriptLanguage["BENGALI"]             = (int)TRANSCRIPT_LANGUAGE::BENGALI;
    transcriptLanguage["CANTONESE"]           = (int)TRANSCRIPT_LANGUAGE::CANTONESE;
    transcriptLanguage["CATALAN"]             = (int)TRANSCRIPT_LANGUAGE::CATALAN;
    transcriptLanguage["CHINESE_SIMPLIFIED"]  = (int)TRANSCRIPT_LANGUAGE::CHINESE_SIMPLIFIED;
    transcriptLanguage["CHINESE_TRADITIONAL"] = (int)TRANSCRIPT_LANGUAGE::CHINESE_TRADITIONAL;
    transcriptLanguage["CZECH"]               = (int)TRANSCRIPT_LANGUAGE::CZECH;
    transcriptLanguage["DANISH"]              = (int)TRANSCRIPT_LANGUAGE::DANISH;
    transcriptLanguage["DUTCH"]               = (int)TRANSCRIPT_LANGUAGE::DUTCH;
    transcriptLanguage["ENGLISH"]             = (int)TRANSCRIPT_LANGUAGE::ENGLISH;
    transcriptLanguage["ESTONIAN"]            = (int)TRANSCRIPT_LANGUAGE::ESTONIAN;
    transcriptLanguage["FINNISH"]             = (int)TRANSCRIPT_LANGUAGE::FINNISH;
    transcriptLanguage["FRENCH_CANADA"]       = (int)TRANSCRIPT_LANGUAGE::FRENCH_CANADA;
    transcriptLanguage["FRENCH_FRANCE"]       = (int)TRANSCRIPT_LANGUAGE::FRENCH_FRANCE;
    transcriptLanguage["GERMAN"]              = (int)TRANSCRIPT_LANGUAGE::GERMAN;
    transcriptLanguage["HEBREW"]              = (int)TRANSCRIPT_LANGUAGE::HEBREW;
    transcriptLanguage["HINDI"]               = (int)TRANSCRIPT_LANGUAGE::HINDI;
    transcriptLanguage["HUNGARIAN"]           = (int)TRANSCRIPT_LANGUAGE::HUNGARIAN;
    transcriptLanguage["INDONESIAN"]          = (int)TRANSCRIPT_LANGUAGE::INDONESIAN;
    transcriptLanguage["ITALIAN"]             = (int)TRANSCRIPT_LANGUAGE::ITALIAN;
    transcriptLanguage["JAPANESE"]            = (int)TRANSCRIPT_LANGUAGE::JAPANESE;
    transcriptLanguage["KOREAN"]              = (int)TRANSCRIPT_LANGUAGE::KOREAN;
    transcriptLanguage["MALAY"]               = (int)TRANSCRIPT_LANGUAGE::MALAY;
    transcriptLanguage["PERSIAN"]             = (int)TRANSCRIPT_LANGUAGE::PERSIAN;
    transcriptLanguage["POLISH"]              = (int)TRANSCRIPT_LANGUAGE::POLISH;
    transcriptLanguage["PORTUGUESE"]          = (int)TRANSCRIPT_LANGUAGE::PORTUGUESE;
    transcriptLanguage["ROMANIAN"]            = (int)TRANSCRIPT_LANGUAGE::ROMANIAN;
    transcriptLanguage["RUSSIAN"]             = (int)TRANSCRIPT_LANGUAGE::RUSSIAN;
    transcriptLanguage["SPANISH"]             = (int)TRANSCRIPT_LANGUAGE::SPANISH;
    transcriptLanguage["SWEDISH"]             = (int)TRANSCRIPT_LANGUAGE::SWEDISH;
    transcriptLanguage["TAGALOG"]             = (int)TRANSCRIPT_LANGUAGE::TAGALOG;
    transcriptLanguage["TAMIL"]               = (int)TRANSCRIPT_LANGUAGE::TAMIL;
    transcriptLanguage["TELUGU"]              = (int)TRANSCRIPT_LANGUAGE::TELUGU;
    transcriptLanguage["THAI"]                = (int)TRANSCRIPT_LANGUAGE::THAI;
    transcriptLanguage["TURKISH"]             = (int)TRANSCRIPT_LANGUAGE::TURKISH;
    transcriptLanguage["UKRAINIAN"]           = (int)TRANSCRIPT_LANGUAGE::UKRAINIAN;
    transcriptLanguage["VIETNAMESE"]          = (int)TRANSCRIPT_LANGUAGE::VIETNAMESE;
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
        .def("alloc", &PyClient::alloc,
             "Allocate the C SDK handle. Must be called from the thread that will own this "
             "client (same thread must call join/poll/release). Replays any callbacks and "
             "params registered before alloc().")
        .def("is_allocated", &PyClient::isAllocated,
             "Return True if alloc() has been called for this client")
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
    // These match EVENT_TYPE from Zoom's C SDK
    // ========================================================================

    m.attr("EVENT_UNDEFINED") = py::int_(static_cast<int>(EVENT_TYPE::UNDEFINED));
    m.attr("EVENT_FIRST_PACKET_TIMESTAMP") = py::int_(static_cast<int>(EVENT_TYPE::FIRST_PACKET_TIMESTAMP));
    m.attr("EVENT_ACTIVE_SPEAKER_CHANGE") = py::int_(static_cast<int>(EVENT_TYPE::ACTIVE_SPEAKER_CHANGE));
    m.attr("EVENT_PARTICIPANT_JOIN") = py::int_(static_cast<int>(EVENT_TYPE::PARTICIPANT_JOIN));
    m.attr("EVENT_PARTICIPANT_LEAVE") = py::int_(static_cast<int>(EVENT_TYPE::PARTICIPANT_LEAVE));
    m.attr("EVENT_SHARING_START") = py::int_(static_cast<int>(EVENT_TYPE::SHARING_START));
    m.attr("EVENT_SHARING_STOP") = py::int_(static_cast<int>(EVENT_TYPE::SHARING_STOP));
    m.attr("EVENT_MEDIA_CONNECTION_INTERRUPTED") = py::int_(static_cast<int>(EVENT_TYPE::MEDIA_CONNECTION_INTERRUPTED));
    m.attr("EVENT_PARTICIPANT_VIDEO_ON")  = py::int_(static_cast<int>(EVENT_TYPE::PARTICIPANT_VIDEO_ON));
    m.attr("EVENT_PARTICIPANT_VIDEO_OFF") = py::int_(static_cast<int>(EVENT_TYPE::PARTICIPANT_VIDEO_OFF));
    m.attr("EVENT_CONSUMER_ANSWERED") = py::int_(static_cast<int>(ZCC_VOICE_EVENT_TYPE::CONSUMER_ANSWERED));
    m.attr("EVENT_CONSUMER_END") = py::int_(static_cast<int>(ZCC_VOICE_EVENT_TYPE::CONSUMER_END));
    m.attr("EVENT_USER_ANSWERED") = py::int_(static_cast<int>(ZCC_VOICE_EVENT_TYPE::USER_ANSWERED));
    m.attr("EVENT_USER_END") = py::int_(static_cast<int>(ZCC_VOICE_EVENT_TYPE::USER_END));
    m.attr("EVENT_USER_HOLD") = py::int_(static_cast<int>(ZCC_VOICE_EVENT_TYPE::USER_HOLD));
    m.attr("EVENT_USER_UNHOLD") = py::int_(static_cast<int>(ZCC_VOICE_EVENT_TYPE::USER_UNHOLD));

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

    // DataOption — unified dict for all audio and video stream delivery modes.
    // Mirrors the C++ MEDIA_DATA_OPTION enum directly.
    // AudioDataOption and VideoDataOption are kept as backward-compat aliases.
    py::dict dataOption;
    dataOption["UNDEFINED"]                    = (int)MEDIA_DATA_OPTION::UNDEFINED;
    dataOption["AUDIO_MIXED_STREAM"]           = (int)MEDIA_DATA_OPTION::AUDIO_MIXED_STREAM;
    dataOption["AUDIO_MULTI_STREAMS"]          = (int)MEDIA_DATA_OPTION::AUDIO_MULTI_STREAMS;
    dataOption["VIDEO_SINGLE_ACTIVE_STREAM"]   = (int)MEDIA_DATA_OPTION::VIDEO_SINGLE_ACTIVE_STREAM;
    dataOption["VIDEO_SINGLE_INDIVIDUAL_STREAM"] = (int)MEDIA_DATA_OPTION::VIDEO_SINGLE_INDIVIDUAL_STREAM;
    dataOption["VIDEO_MIXED_SPEAKER_VIEW"]     = (int)MEDIA_DATA_OPTION::VIDEO_SINGLE_INDIVIDUAL_STREAM;  // legacy alias
    dataOption["VIDEO_MIXED_GALLERY_VIEW"]     = (int)MEDIA_DATA_OPTION::VIDEO_MIXED_GALLERY_VIEW;
    m.attr("DataOption")      = dataOption;
    m.attr("AudioDataOption") = dataOption;  // legacy alias
    m.attr("VideoDataOption") = dataOption;  // legacy alias

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
    sessionState["INACTIVE"]   = (int)SESSION_STATE::INACTIVE;
    sessionState["INITIALIZE"] = (int)SESSION_STATE::INITIALIZE;
    sessionState["STARTED"]    = (int)SESSION_STATE::STARTED;
    sessionState["PAUSED"]     = (int)SESSION_STATE::PAUSED;
    sessionState["RESUMED"]    = (int)SESSION_STATE::RESUMED;
    sessionState["STOPPED"]    = (int)SESSION_STATE::STOPPED;
    m.attr("SessionState") = sessionState;

    // ========================================================================
    // Constants - Stream State
    // ========================================================================

    py::dict streamState;
    streamState["INACTIVE"]    = (int)STREAM_STATE::INACTIVE;
    streamState["ACTIVE"]      = (int)STREAM_STATE::ACTIVE;
    streamState["INTERRUPTED"] = (int)STREAM_STATE::INTERRUPTED;
    streamState["TERMINATING"] = (int)STREAM_STATE::TERMINATING;
    streamState["TERMINATED"]  = (int)STREAM_STATE::TERMINATED;
    streamState["PAUSED"]      = (int)STREAM_STATE::PAUSED;
    streamState["RESUMED"]     = (int)STREAM_STATE::RESUMED;
    m.attr("StreamState") = streamState;

    // ========================================================================
    // Constants - Event Type (matches EVENT_TYPE from Zoom's C SDK)
    // ========================================================================

    py::dict eventType;
    eventType["UNDEFINED"]                    = (int)EVENT_TYPE::UNDEFINED;
    eventType["FIRST_PACKET_TIMESTAMP"]       = (int)EVENT_TYPE::FIRST_PACKET_TIMESTAMP;
    eventType["ACTIVE_SPEAKER_CHANGE"]        = (int)EVENT_TYPE::ACTIVE_SPEAKER_CHANGE;
    eventType["PARTICIPANT_JOIN"]             = (int)EVENT_TYPE::PARTICIPANT_JOIN;
    eventType["PARTICIPANT_LEAVE"]            = (int)EVENT_TYPE::PARTICIPANT_LEAVE;
    eventType["SHARING_START"]                = (int)EVENT_TYPE::SHARING_START;
    eventType["SHARING_STOP"]                 = (int)EVENT_TYPE::SHARING_STOP;
    eventType["MEDIA_CONNECTION_INTERRUPTED"] = (int)EVENT_TYPE::MEDIA_CONNECTION_INTERRUPTED;
    eventType["PARTICIPANT_VIDEO_ON"]         = (int)EVENT_TYPE::PARTICIPANT_VIDEO_ON;
    eventType["PARTICIPANT_VIDEO_OFF"]        = (int)EVENT_TYPE::PARTICIPANT_VIDEO_OFF;
    // ZCC voice events
    eventType["CONSUMER_ANSWERED"] = (int)ZCC_VOICE_EVENT_TYPE::CONSUMER_ANSWERED;
    eventType["CONSUMER_END"]      = (int)ZCC_VOICE_EVENT_TYPE::CONSUMER_END;
    eventType["USER_ANSWERED"]     = (int)ZCC_VOICE_EVENT_TYPE::USER_ANSWERED;
    eventType["USER_END"]          = (int)ZCC_VOICE_EVENT_TYPE::USER_END;
    eventType["USER_HOLD"]         = (int)ZCC_VOICE_EVENT_TYPE::USER_HOLD;
    eventType["USER_UNHOLD"]       = (int)ZCC_VOICE_EVENT_TYPE::USER_UNHOLD;
    m.attr("EventType") = eventType;

    // ========================================================================
    // Constants - Message Type
    // ========================================================================

    py::dict messageType;
    messageType["UNDEFINED"]                 = (int)MESSAGE_TYPE::UNDEFINED;
    messageType["SIGNALING_HAND_SHAKE_REQ"]  = (int)MESSAGE_TYPE::SIGNALING_HAND_SHAKE_REQ;
    messageType["SIGNALING_HAND_SHAKE_RESP"] = (int)MESSAGE_TYPE::SIGNALING_HAND_SHAKE_RESP;
    messageType["DATA_HAND_SHAKE_REQ"]       = (int)MESSAGE_TYPE::DATA_HAND_SHAKE_REQ;
    messageType["DATA_HAND_SHAKE_RESP"]      = (int)MESSAGE_TYPE::DATA_HAND_SHAKE_RESP;
    messageType["EVENT_SUBSCRIPTION"]        = (int)MESSAGE_TYPE::EVENT_SUBSCRIPTION;
    messageType["EVENT_UPDATE"]              = (int)MESSAGE_TYPE::EVENT_UPDATE;
    messageType["CLIENT_READY_ACK"]          = (int)MESSAGE_TYPE::CLIENT_READY_ACK;
    messageType["STREAM_STATE_UPDATE"]       = (int)MESSAGE_TYPE::STREAM_STATE_UPDATE;
    messageType["SESSION_STATE_UPDATE"]      = (int)MESSAGE_TYPE::SESSION_STATE_UPDATE;
    messageType["SESSION_STATE_REQ"]         = (int)MESSAGE_TYPE::SESSION_STATE_REQ;
    messageType["SESSION_STATE_RESP"]        = (int)MESSAGE_TYPE::SESSION_STATE_RESP;
    messageType["KEEP_ALIVE_REQ"]            = (int)MESSAGE_TYPE::KEEP_ALIVE_REQ;
    messageType["KEEP_ALIVE_RESP"]           = (int)MESSAGE_TYPE::KEEP_ALIVE_RESP;
    messageType["MEDIA_DATA_AUDIO"]          = (int)MESSAGE_TYPE::MEDIA_DATA_AUDIO;
    messageType["MEDIA_DATA_VIDEO"]          = (int)MESSAGE_TYPE::MEDIA_DATA_VIDEO;
    messageType["MEDIA_DATA_SHARE"]          = (int)MESSAGE_TYPE::MEDIA_DATA_SHARE;
    messageType["MEDIA_DATA_TRANSCRIPT"]     = (int)MESSAGE_TYPE::MEDIA_DATA_TRANSCRIPT;
    messageType["MEDIA_DATA_CHAT"]           = (int)MESSAGE_TYPE::MEDIA_DATA_CHAT;
    messageType["STREAM_STATE_REQ"]          = (int)MESSAGE_TYPE::STREAM_STATE_REQ;
    messageType["STREAM_STATE_RESP"]         = (int)MESSAGE_TYPE::STREAM_STATE_RESP;
    messageType["STREAM_CLOSE_REQ"]          = (int)MESSAGE_TYPE::STREAM_CLOSE_REQ;
    messageType["STREAM_CLOSE_RESP"]         = (int)MESSAGE_TYPE::STREAM_CLOSE_RESP;
    messageType["META_DATA_AUDIO"]           = (int)MESSAGE_TYPE::META_DATA_AUDIO;
    messageType["META_DATA_VIDEO"]           = (int)MESSAGE_TYPE::META_DATA_VIDEO;
    messageType["META_DATA_SHARE"]           = (int)MESSAGE_TYPE::META_DATA_SHARE;
    messageType["META_DATA_TRANSCRIPT"]      = (int)MESSAGE_TYPE::META_DATA_TRANSCRIPT;
    messageType["META_DATA_CHAT"]            = (int)MESSAGE_TYPE::META_DATA_CHAT;
    messageType["VIDEO_SUBSCRIPTION_REQ"]    = (int)MESSAGE_TYPE::VIDEO_SUBSCRIPTION_REQ;
    messageType["VIDEO_SUBSCRIPTION_RESP"]   = (int)MESSAGE_TYPE::VIDEO_SUBSCRIPTION_RESP;
    m.attr("MessageType") = messageType;

    // ========================================================================
    // Constants - Stop Reason
    // ========================================================================

    py::dict stopReason;
    stopReason["UNDEFINED"]                               = (int)STOP_REASON::UNDEFINED;
    stopReason["HOST_TRIGGERED"]                          = (int)STOP_REASON::HOST_TRIGGERED;
    stopReason["USER_TRIGGERED"]                          = (int)STOP_REASON::USER_TRIGGERED;
    stopReason["USER_LEFT"]                               = (int)STOP_REASON::USER_LEFT;
    stopReason["USER_EJECTED"]                            = (int)STOP_REASON::USER_EJECTED;
    stopReason["HOST_DISABLED_APP"]                       = (int)STOP_REASON::HOST_DISABLED_APP;
    stopReason["MEETING_ENDED"]                           = (int)STOP_REASON::MEETING_ENDED;
    stopReason["STREAM_CANCELED"]                         = (int)STOP_REASON::STREAM_CANCELED;
    stopReason["STREAM_REVOKED"]                          = (int)STOP_REASON::STREAM_REVOKED;
    stopReason["ALL_APPS_DISABLED"]                       = (int)STOP_REASON::ALL_APPS_DISABLED;
    stopReason["INTERNAL_EXCEPTION"]                      = (int)STOP_REASON::INTERNAL_EXCEPTION;
    stopReason["CONNECTION_TIMEOUT"]                      = (int)STOP_REASON::CONNECTION_TIMEOUT;
    stopReason["INSTANCE_CONNECTION_INTERRUPTED"]         = (int)STOP_REASON::INSTANCE_CONNECTION_INTERRUPTED;
    stopReason["SIGNAL_CONNECTION_INTERRUPTED"]           = (int)STOP_REASON::SIGNAL_CONNECTION_INTERRUPTED;
    stopReason["DATA_CONNECTION_INTERRUPTED"]             = (int)STOP_REASON::DATA_CONNECTION_INTERRUPTED;
    stopReason["SIGNAL_CONNECTION_CLOSED_ABNORMALLY"]     = (int)STOP_REASON::SIGNAL_CONNECTION_CLOSED_ABNORMALLY;
    stopReason["DATA_CONNECTION_CLOSED_ABNORMALLY"]       = (int)STOP_REASON::DATA_CONNECTION_CLOSED_ABNORMALLY;
    stopReason["EXIT_SIGNAL"]                             = (int)STOP_REASON::EXIT_SIGNAL;
    stopReason["AUTHENTICATION_FAILURE"]                  = (int)STOP_REASON::AUTHENTICATION_FAILURE;
    stopReason["AWAIT_RECONNECTION_TIMEOUT"]              = (int)STOP_REASON::AWAIT_RECONNECTION_TIMEOUT;
    stopReason["RECEIVER_REQUEST_CLOSE"]                  = (int)STOP_REASON::RECEIVER_REQUEST_CLOSE;
    stopReason["CUSTOMER_DISCONNECTED"]                   = (int)STOP_REASON::CUSTOMER_DISCONNECTED;
    stopReason["AGENT_DISCONNECTED"]                      = (int)STOP_REASON::AGENT_DISCONNECTED;
    stopReason["ADMIN_DISABLED_APP"]                      = (int)STOP_REASON::ADMIN_DISABLED_APP;
    stopReason["KEEP_ALIVE_TIMEOUT"]                      = (int)STOP_REASON::KEEP_ALIVE_TIMEOUT;
    stopReason["MANUAL_API_TRIGGERED"]                    = (int)STOP_REASON::MANUAL_API_TRIGGERED;
    stopReason["STREAMING_NOT_SUPPORTED"]                 = (int)STOP_REASON::STREAMING_NOT_SUPPORTED;
    m.attr("StopReason") = stopReason;
}
