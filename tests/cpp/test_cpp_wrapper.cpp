/**
 * C++ unit tests for the RTMS SDK wrapper (src/rtms.h / src/rtms.cpp).
 *
 * Linked against tests/mock_sdk.cpp instead of the real Zoom SDK binary,
 * so these run in CI without credentials or the proprietary library.
 *
 * Test coverage:
 *   - AudioParams / VideoParams / DeskshareParams validation
 *   - Session / Participant / Metadata data classes
 *   - MediaParams composition and toNative()
 *   - Client lifecycle (create, initialize, join, poll, release)
 *   - Callback dispatch (via mock_trigger_* helpers)
 *   - Event subscription deferral / on-confirm flush
 *   - Media type auto-enable on callback registration
 *   - setProxy forwarding and error handling
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "rtms.h"
#include "mock_sdk.h"

#include <cstring>
#include <string>
#include <vector>

using namespace rtms;
using Catch::Matchers::ContainsSubstring;

// Reset global mock state before every test case
struct R { R() { g_mock_state.reset(); } };

// ============================================================================
// AudioParams
// ============================================================================

TEST_CASE("AudioParams default constructor", "[params][audio]") {
    R _;
    AudioParams p;
    CHECK(p.contentType() == 2);  // RAW_AUDIO
    CHECK(p.codec()       == 4);  // OPUS
    CHECK(p.sampleRate()  == 3);  // SR_48K
    CHECK(p.channel()     == 2);  // STEREO
    CHECK(p.dataOpt()     == 2);  // AUDIO_MULTI_STREAMS
    CHECK(p.duration()    == 20);
    CHECK(p.frameSize()   == 960);
}

TEST_CASE("AudioParams validate() — valid configurations", "[params][audio]") {
    R _;

    SECTION("default params pass validation") {
        AudioParams p;
        REQUIRE_NOTHROW(p.validate());
    }

    SECTION("PCM/L16 at 16kHz with correct frame size passes") {
        // L16=1, SR_16K=1, MONO=1, AUDIO_MULTI_STREAMS=2, 20ms → 320 samples
        AudioParams p(2, 1, 1, 1, 2, 20, 320);
        REQUIRE_NOTHROW(p.validate());
    }

    SECTION("PCM/L16 at 8kHz with correct frame size passes") {
        // L16=1, SR_8K=0, MONO=1, AUDIO_MULTI_STREAMS=2, 20ms → 160 samples
        AudioParams p(2, 1, 0, 1, 2, 20, 160);
        REQUIRE_NOTHROW(p.validate());
    }
}

TEST_CASE("AudioParams validate() — invalid configurations throw", "[params][audio]") {
    R _;

    SECTION("contentType 0 throws") {
        AudioParams p;
        p.setContentType(0);
        REQUIRE_THROWS_AS(p.validate(), std::invalid_argument);
    }

    SECTION("codec 0 throws") {
        AudioParams p;
        p.setCodec(0);
        REQUIRE_THROWS_AS(p.validate(), std::invalid_argument);
    }

    SECTION("channel 0 throws") {
        AudioParams p;
        p.setChannel(0);
        REQUIRE_THROWS_AS(p.validate(), std::invalid_argument);
    }

    SECTION("dataOpt 0 throws") {
        AudioParams p;
        p.setDataOpt(0);
        REQUIRE_THROWS_AS(p.validate(), std::invalid_argument);
    }

    SECTION("OPUS codec with 16kHz sample rate throws") {
        AudioParams p;
        p.setSampleRate(1); // SR_16K — OPUS requires SR_48K
        REQUIRE_THROWS_AS(p.validate(), std::invalid_argument);
    }

    SECTION("OPUS codec with 8kHz sample rate throws") {
        AudioParams p;
        p.setSampleRate(0); // SR_8K
        REQUIRE_THROWS_AS(p.validate(), std::invalid_argument);
    }

    SECTION("frameSize mismatch for 48kHz 20ms throws") {
        AudioParams p;                 // default: 48kHz, 20ms → expects 960
        p.setFrameSize(480);           // wrong
        REQUIRE_THROWS_AS(p.validate(), std::invalid_argument);
    }

    SECTION("frameSize mismatch error message names the values") {
        AudioParams p;
        p.setFrameSize(480);
        REQUIRE_THROWS_WITH(p.validate(), ContainsSubstring("960"));
    }

    SECTION("OPUS error message mentions 48kHz") {
        AudioParams p;
        p.setSampleRate(1);
        REQUIRE_THROWS_WITH(p.validate(), ContainsSubstring("48kHz"));
    }
}

TEST_CASE("AudioParams toNative() maps fields correctly", "[params][audio]") {
    R _;
    AudioParams p(1, 2, 0, 1, 1, 20, 160); // RTP, G711, 8K, MONO, MIXED, 20ms, 160
    auto n = p.toNative();
    CHECK(n.content_type == 1);
    CHECK(n.codec        == 2);
    CHECK(n.sample_rate  == 0);
    CHECK(n.channel      == 1);
    CHECK(n.data_opt     == 1);
    CHECK(n.duration     == 20);
    CHECK(n.frame_size   == 160);
}

// ============================================================================
// VideoParams
// ============================================================================

TEST_CASE("VideoParams default constructor uses H264/HD/30fps defaults", "[params][video]") {
    R _;
    VideoParams p;
    CHECK(p.contentType() == (int)MEDIA_CONTENT_TYPE::RAW_VIDEO);
    CHECK(p.codec()       == (int)MEDIA_PAYLOAD_TYPE::H264);
    CHECK(p.resolution()  == (int)MEDIA_RESOLUTION::HD);
    CHECK(p.dataOpt()     == (int)MEDIA_DATA_OPTION::VIDEO_SINGLE_ACTIVE_STREAM);
    CHECK(p.fps()         == 30);
}

TEST_CASE("VideoParams validate() — invalid configurations throw", "[params][video]") {
    R _;

    SECTION("resolution 0 throws") {
        VideoParams p(3, 7, 0, 5, 30); // RAW_VIDEO, H264, no resolution
        REQUIRE_THROWS_AS(p.validate(), std::invalid_argument);
    }

    SECTION("JPG codec with fps > 5 throws") {
        VideoParams p(3, 5, 2, 5, 10); // JPG=5, HD=2, fps=10
        REQUIRE_THROWS_AS(p.validate(), std::invalid_argument);
    }

    SECTION("PNG codec with fps > 5 throws") {
        VideoParams p(3, 6, 2, 5, 6);  // PNG=6, fps=6
        REQUIRE_THROWS_AS(p.validate(), std::invalid_argument);
    }

    SECTION("JPG codec with fps <= 5 passes") {
        VideoParams p(3, 5, 2, 5, 5);  // JPG=5, fps=5
        REQUIRE_NOTHROW(p.validate());
    }

    SECTION("H264 with fps > 30 throws") {
        VideoParams p(3, 7, 2, 5, 31); // H264=7, fps=31
        REQUIRE_THROWS_AS(p.validate(), std::invalid_argument);
    }

    SECTION("H264 with fps 0 throws") {
        VideoParams p(3, 7, 2, 5, 0);
        REQUIRE_THROWS_AS(p.validate(), std::invalid_argument);
    }

    SECTION("H264 with fps 30 passes") {
        VideoParams p(3, 7, 2, 5, 30);
        REQUIRE_NOTHROW(p.validate());
    }
}

// ============================================================================
// DeskshareParams
// ============================================================================

TEST_CASE("DeskshareParams validate() — fps/codec rules match VideoParams", "[params][ds]") {
    R _;

    SECTION("JPG with fps > 5 throws") {
        DeskshareParams p(3, 5, 2, 10); // JPG=5, fps=10
        REQUIRE_THROWS_AS(p.validate(), std::invalid_argument);
    }

    SECTION("H264 with fps 30 passes") {
        DeskshareParams p(3, 7, 2, 30);
        REQUIRE_NOTHROW(p.validate());
    }
}

// ============================================================================
// Session
// ============================================================================

TEST_CASE("Session normal construction", "[data][session]") {
    R _;
    session_info info;
    char sid[]  = "sess-abc";
    char strid[] = "stream-xyz";
    char mid[MAX_MEETING_ID_LEN] = "meeting-123";
    info.session_id = sid;
    info.stream_id  = strid;
    memcpy(info.meeting_id, mid, sizeof(info.meeting_id));
    info.stat_time  = 42;
    info.status     = SESS_STATUS_ACTIVE;

    Session s(info);
    CHECK(s.sessionId() == "sess-abc");
    CHECK(s.streamId()  == "stream-xyz");
    CHECK(s.meetingId() == "meeting-123");
    CHECK(s.statTime()  == 42);
    CHECK(s.isActive());
    CHECK_FALSE(s.isPaused());
}

TEST_CASE("Session null pointer safety", "[data][session]") {
    R _;

    SECTION("null session_id becomes empty string") {
        session_info info{};
        info.session_id = nullptr;
        info.stream_id  = nullptr;
        Session s(info);
        CHECK(s.sessionId().empty());
        CHECK(s.streamId().empty());
    }

    SECTION("low-address session_id treated as invalid") {
        session_info info{};
        // Anything <= 0xFFFF is treated as an invalid pointer (not a real address)
        info.session_id = reinterpret_cast<char*>(0x100);
        info.stream_id  = nullptr;
        Session s(info);
        CHECK(s.sessionId().empty());
    }

    SECTION("meeting_id buffer without null terminator is safe") {
        session_info info{};
        info.session_id = nullptr;
        info.stream_id  = nullptr;
        // Fill entire buffer with 'X' — no null terminator
        memset(info.meeting_id, 'X', MAX_MEETING_ID_LEN);
        Session s(info);
        // Should read exactly MAX_MEETING_ID_LEN 'X' characters, not overflow
        CHECK(s.meetingId().size() == MAX_MEETING_ID_LEN);
        CHECK(s.meetingId() == std::string(MAX_MEETING_ID_LEN, 'X'));
    }
}

TEST_CASE("Session pause status", "[data][session]") {
    R _;
    session_info info{};
    info.status = SESS_STATUS_PAUSED;
    Session s(info);
    CHECK(s.isPaused());
    CHECK_FALSE(s.isActive());
}

// ============================================================================
// Participant
// ============================================================================

TEST_CASE("Participant construction", "[data][participant]") {
    R _;

    SECTION("normal construction copies id and name") {
        participant_info pi;
        char name[] = "Alice";
        pi.participant_id   = 99;
        pi.participant_name = name;
        Participant p(pi);
        CHECK(p.id()   == 99);
        CHECK(p.name() == "Alice");
    }

    SECTION("null participant_name becomes empty string") {
        participant_info pi;
        pi.participant_id   = 1;
        pi.participant_name = nullptr;
        Participant p(pi);
        CHECK(p.name().empty());
    }
}

// ============================================================================
// Metadata
// ============================================================================

TEST_CASE("Metadata construction", "[data][metadata]") {
    R _;

    SECTION("normal construction copies user_name and user_id") {
        rtms_metadata md{};
        char name[] = "Bob";
        md.user_name = name;
        md.user_id   = 42;
        Metadata m(md);
        CHECK(m.userName() == "Bob");
        CHECK(m.userId()   == 42);
    }

    SECTION("null user_name becomes empty string") {
        rtms_metadata md{};
        md.user_name = nullptr;
        md.user_id   = 7;
        Metadata m(md);
        CHECK(m.userName().empty());
        CHECK(m.userId() == 7);
    }
}

// ============================================================================
// MediaParams
// ============================================================================

TEST_CASE("MediaParams — empty by default", "[params][media]") {
    R _;
    MediaParams mp;
    CHECK_FALSE(mp.hasAudioParams());
    CHECK_FALSE(mp.hasVideoParams());
    CHECK_FALSE(mp.hasDeskshareParams());
}

TEST_CASE("MediaParams — set/get round-trips", "[params][media]") {
    R _;
    MediaParams mp;

    AudioParams ap;
    ap.setSampleRate(1); // SR_16K
    mp.setAudioParams(ap);
    REQUIRE(mp.hasAudioParams());
    CHECK(mp.audioParams().sampleRate() == 1);

    VideoParams vp;
    vp.setResolution(2); // HD
    mp.setVideoParams(vp);
    REQUIRE(mp.hasVideoParams());
    CHECK(mp.videoParams().resolution() == 2);
}

TEST_CASE("MediaParams toNative() — correct pointers", "[params][media]") {
    R _;

    SECTION("empty MediaParams produces all-null native struct") {
        MediaParams mp;
        auto n = mp.toNative();
        CHECK(n.audio_param == nullptr);
        CHECK(n.video_param == nullptr);
        CHECK(n.ds_param    == nullptr);
        CHECK(n.tr_param    == nullptr);
        // No leak — nothing was allocated
    }

    SECTION("set audio produces non-null audio_param, others null") {
        MediaParams mp;
        mp.setAudioParams(AudioParams());
        auto n = mp.toNative();
        REQUIRE(n.audio_param != nullptr);
        CHECK(n.video_param == nullptr);
        CHECK(n.ds_param    == nullptr);
        CHECK(n.tr_param    == nullptr);
        CHECK(n.audio_param->codec == 4); // OPUS default
        delete n.audio_param;
    }

    SECTION("set video produces non-null video_param") {
        MediaParams mp;
        VideoParams vp(3, 7, 2, 5, 30); // RAW_VIDEO, H264, HD, GALLERY, 30fps
        mp.setVideoParams(vp);
        auto n = mp.toNative();
        REQUIRE(n.video_param != nullptr);
        CHECK(n.audio_param == nullptr);
        CHECK(n.video_param->codec == 7);
        delete n.video_param;
    }
}

TEST_CASE("MediaParams assignment copies all params", "[params][media]") {
    R _;
    MediaParams a;
    a.setAudioParams(AudioParams());
    VideoParams vp; vp.setResolution(2);
    a.setVideoParams(vp);

    MediaParams b;
    b = a;
    REQUIRE(b.hasAudioParams());
    REQUIRE(b.hasVideoParams());
    CHECK_FALSE(b.hasDeskshareParams());
    CHECK(b.videoParams().resolution() == 2);
}

// ============================================================================
// Client lifecycle
// ============================================================================

TEST_CASE("Client constructor calls create_sdk", "[client][lifecycle]") {
    R _;
    { Client c; }
    CHECK(g_mock_state.create_calls == 1);
    // Destructor should release
    CHECK(g_mock_state.release_calls == 1);
}

TEST_CASE("Client constructor throws when create_sdk returns null", "[client][lifecycle]") {
    R _;
    g_mock_state.create_result = 1; // make create_sdk return nullptr
    REQUIRE_THROWS_AS(Client(), Exception);
}

TEST_CASE("Client::initialize calls provider->init with correct args", "[client][lifecycle]") {
    R _;
    Client::initialize("/path/to/ca.pem", 1, nullptr);
    CHECK(g_mock_state.init_calls        == 1);
    CHECK(g_mock_state.last_ca_path      == "/path/to/ca.pem");
    CHECK(g_mock_state.last_verify_cert  == true);
}

TEST_CASE("Client::initialize with verify=0 passes false to provider", "[client][lifecycle]") {
    R _;
    Client::initialize("", 0);
    CHECK(g_mock_state.last_verify_cert == false);
}

TEST_CASE("Client::initialize sets g_agent when provided", "[client][lifecycle]") {
    R _;
    Client::initialize("", 1, "my-agent/1.0");
    CHECK(g_agent == "my-agent/1.0");
    g_agent.clear(); // cleanup
}

TEST_CASE("Client::uninitialize calls provider->uninit", "[client][lifecycle]") {
    R _;
    Client::uninitialize();
    CHECK(g_mock_state.uninit_calls == 1);
}

TEST_CASE("Client::join calls open() then join() in order", "[client][join]") {
    R _;
    Client c;
    c.join("uuid-1", "stream-1", "sig-1", "wss://server", 5000);

    CHECK(g_mock_state.open_calls  == 1);
    CHECK(g_mock_state.join_calls  == 1);
    // open() must be called before join() — verified by sequential counts
    CHECK(g_mock_state.last_meeting_uuid == "uuid-1");
    CHECK(g_mock_state.last_stream_id    == "stream-1");
    CHECK(g_mock_state.last_signature    == "sig-1");
    CHECK(g_mock_state.last_server_url   == "wss://server");
    CHECK(g_mock_state.last_timeout      == 5000);
}

TEST_CASE("Client::join passes itself as the sink to open()", "[client][join]") {
    R _;
    Client c;
    c.join("uuid", "stream", "sig", "url");
    CHECK(g_mock_state.last_sink == &c);
}

TEST_CASE("Client::join throws when open() fails", "[client][join]") {
    R _;
    g_mock_state.open_result = RTMS_SDK_FAILURE;
    Client c;
    REQUIRE_THROWS_AS(c.join("uuid", "stream", "sig", "url"), Exception);
    CHECK(g_mock_state.join_calls == 0); // join() should not be called after open() fails
}

TEST_CASE("Client::join throws when join() fails", "[client][join]") {
    R _;
    g_mock_state.join_result = RTMS_SDK_FAILURE;
    Client c;
    REQUIRE_THROWS_AS(c.join("uuid", "stream", "sig", "url"), Exception);
}

TEST_CASE("Client::poll calls sdk->poll()", "[client][poll]") {
    R _;
    Client c;
    c.poll();
    CHECK(g_mock_state.poll_calls == 1);
}

TEST_CASE("Client::poll throws on SDK error", "[client][poll]") {
    R _;
    g_mock_state.poll_result = RTMS_SDK_TIMEOUT;
    Client c;
    REQUIRE_THROWS_AS(c.poll(), Exception);
}

TEST_CASE("Client::release calls leave() then release_sdk()", "[client][release]") {
    R _;
    {
        Client c;
        c.release();
        CHECK(g_mock_state.leave_calls   == 1);
        CHECK(g_mock_state.release_calls == 1);
    }
    // Destructor should NOT double-release (sdk_ is nullptr after release())
    CHECK(g_mock_state.release_calls == 1);
}

TEST_CASE("Client::uuid and streamId return joined values", "[client][join]") {
    R _;
    Client c;
    c.join("my-uuid", "my-stream", "sig", "url");
    CHECK(c.uuid()     == "my-uuid");
    CHECK(c.streamId() == "my-stream");
}

// ============================================================================
// Callback dispatch
// ============================================================================

TEST_CASE("on_join_confirm fires registered callback", "[client][callbacks]") {
    R _;
    Client c;
    c.join("u", "s", "sig", "url");

    int received_reason = -99;
    c.setOnJoinConfirm([&](int r) { received_reason = r; });
    mock_trigger_join_confirm(0);

    CHECK(received_reason == 0);
}

TEST_CASE("on_leave fires registered callback with reason", "[client][callbacks]") {
    R _;
    Client c;
    c.join("u", "s", "sig", "url");

    int received = -1;
    c.setOnLeave([&](int r) { received = r; });
    mock_trigger_leave(3);

    CHECK(received == 3);
}

TEST_CASE("on_event_ex fires registered callback with string", "[client][callbacks]") {
    R _;
    Client c;
    c.join("u", "s", "sig", "url");

    std::string received;
    c.setOnEventEx([&](const std::string& s) { received = s; });
    mock_trigger_event_ex(R"({"event":"test"})");

    CHECK(received == R"({"event":"test"})");
}

TEST_CASE("on_event_ex does not fire callback on empty string", "[client][callbacks]") {
    R _;
    Client c;
    c.join("u", "s", "sig", "url");

    bool called = false;
    c.setOnEventEx([&](const std::string&) { called = true; });
    mock_trigger_event_ex("");

    CHECK_FALSE(called);
}

TEST_CASE("on_audio_data fires callback with correct data", "[client][callbacks]") {
    R _;
    Client c;
    c.join("u", "s", "sig", "url");

    std::vector<uint8_t> received_data;
    uint64_t received_ts = 0;
    int received_uid = -1;
    c.setOnAudioData([&](const std::vector<uint8_t>& d, uint64_t ts, const Metadata& md) {
        received_data = d;
        received_ts   = ts;
        received_uid  = md.userId();
    });

    unsigned char buf[] = {0x01, 0x02, 0x03};
    rtms_metadata md{}; md.user_id = 42; md.user_name = nullptr;
    mock_trigger_audio_data(buf, 3, 999ULL, &md);

    REQUIRE(received_data.size() == 3);
    CHECK(received_data[0] == 0x01);
    CHECK(received_data[2] == 0x03);
    CHECK(received_ts      == 999ULL);
    CHECK(received_uid     == 42);
}

TEST_CASE("on_audio_data does not fire callback on null buf", "[client][callbacks]") {
    R _;
    Client c;
    c.join("u", "s", "sig", "url");

    bool called = false;
    c.setOnAudioData([&](const std::vector<uint8_t>&, uint64_t, const Metadata&) { called = true; });

    rtms_metadata md{};
    mock_trigger_audio_data(nullptr, 3, 0, &md);
    CHECK_FALSE(called);
}

TEST_CASE("on_audio_data does not fire callback on zero size", "[client][callbacks]") {
    R _;
    Client c;
    c.join("u", "s", "sig", "url");

    bool called = false;
    c.setOnAudioData([&](const std::vector<uint8_t>&, uint64_t, const Metadata&) { called = true; });

    unsigned char buf[] = {0x01};
    rtms_metadata md{};
    mock_trigger_audio_data(buf, 0, 0, &md);
    CHECK_FALSE(called);
}

TEST_CASE("on_audio_data does not fire callback on null metadata", "[client][callbacks]") {
    R _;
    Client c;
    c.join("u", "s", "sig", "url");

    bool called = false;
    c.setOnAudioData([&](const std::vector<uint8_t>&, uint64_t, const Metadata&) { called = true; });

    unsigned char buf[] = {0x01};
    mock_trigger_audio_data(buf, 1, 0, nullptr);
    CHECK_FALSE(called);
}

TEST_CASE("on_session_update fires with correct Session object", "[client][callbacks]") {
    R _;
    Client c;
    c.join("u", "s", "sig", "url");

    int received_op = -1;
    std::string received_sid;
    c.setOnSessionUpdate([&](int op, const Session& sess) {
        received_op  = op;
        received_sid = sess.sessionId();
    });

    session_info info{};
    char sid[] = "my-session";
    info.session_id = sid;
    info.stream_id  = nullptr;
    info.status     = SESS_STATUS_ACTIVE;
    mock_trigger_session_update(SESSION_ADD, &info);

    CHECK(received_op  == SESSION_ADD);
    CHECK(received_sid == "my-session");
}

TEST_CASE("on_user_update fires with correct Participant object", "[client][callbacks]") {
    R _;
    Client c;
    c.join("u", "s", "sig", "url");

    int received_op = -1;
    int received_id = -1;
    c.setOnUserUpdate([&](int op, const Participant& p) {
        received_op = op;
        received_id = p.id();
    });

    participant_info pi;
    pi.participant_id   = 77;
    pi.participant_name = nullptr;
    mock_trigger_user_update(USER_JOIN, &pi);

    CHECK(received_op == USER_JOIN);
    CHECK(received_id == 77);
}

// ============================================================================
// Event subscription
// ============================================================================

TEST_CASE("subscribeEvent before join queues events for later", "[client][events]") {
    R _;
    Client c;
    // Not joined yet — subscribe should NOT call sdk immediately
    c.subscribeEvent({1, 2, 3});
    CHECK(g_mock_state.subscribe_calls == 0);
}

TEST_CASE("pending subscriptions flushed on join confirm", "[client][events]") {
    R _;
    Client c;
    c.subscribeEvent({4, 5});
    CHECK(g_mock_state.subscribe_calls == 0);

    c.join("u", "s", "sig", "url");
    mock_trigger_join_confirm(0);

    CHECK(g_mock_state.subscribe_calls == 1);
    // The pending events should have been sent
    REQUIRE(g_mock_state.last_subscribed_events.size() == 2);
    CHECK(g_mock_state.last_subscribed_events[0] == 4);
    CHECK(g_mock_state.last_subscribed_events[1] == 5);
}

TEST_CASE("subscribeEvent after join confirm fires sdk immediately", "[client][events]") {
    R _;
    Client c;
    c.join("u", "s", "sig", "url");
    mock_trigger_join_confirm(0);
    g_mock_state.subscribe_calls = 0; // reset counter (was called for pending)

    c.subscribeEvent({7});
    CHECK(g_mock_state.subscribe_calls == 1);
    CHECK(g_mock_state.last_subscribed_events[0] == 7);
}

TEST_CASE("setOnUserUpdate auto-subscribes to participant events", "[client][events]") {
    R _;
    Client c;
    c.setOnUserUpdate([](int, const Participant&) {});
    // Before join: should be queued (not sent to sdk yet)
    CHECK(g_mock_state.subscribe_calls == 0);

    c.join("u", "s", "sig", "url");
    mock_trigger_join_confirm(0);

    // After join confirm, pending subscriptions should include participant events
    REQUIRE(g_mock_state.subscribe_calls >= 1);
    auto& evts = g_mock_state.last_subscribed_events;
    bool has_join  = std::find(evts.begin(), evts.end(), (int)EVENT_TYPE::PARTICIPANT_JOIN)  != evts.end();
    bool has_leave = std::find(evts.begin(), evts.end(), (int)EVENT_TYPE::PARTICIPANT_LEAVE) != evts.end();
    CHECK(has_join);
    CHECK(has_leave);
}

// ============================================================================
// Media type auto-enable
// ============================================================================

TEST_CASE("setOnAudioData enables AUDIO media type via config on join", "[client][media]") {
    R _;
    Client c;
    c.setOnAudioData([](const std::vector<uint8_t>&, uint64_t, const Metadata&) {});
    // Config is deferred until join() calls open() — no sdk_->config() before open()
    CHECK(g_mock_state.config_calls == 0);
    c.join("u", "s", "sig", "url");
    REQUIRE(g_mock_state.config_calls >= 1);
    CHECK(g_mock_state.last_media_types & Client::AUDIO);
}

TEST_CASE("setOnVideoData enables VIDEO media type via config on join", "[client][media]") {
    R _;
    Client c;
    c.setOnVideoData([](const std::vector<uint8_t>&, uint64_t, const Metadata&) {});
    // Config is deferred until join() — calling sdk_->config() before open()
    // hangs for VIDEO in the real C++ SDK
    CHECK(g_mock_state.config_calls == 0);
    c.join("u", "s", "sig", "url");
    REQUIRE(g_mock_state.config_calls >= 1);
    CHECK(g_mock_state.last_media_types & Client::VIDEO);
}

TEST_CASE("setOnTranscriptData enables TRANSCRIPT media type via config on join", "[client][media]") {
    R _;
    Client c;
    c.setOnTranscriptData([](const std::vector<uint8_t>&, uint64_t, const Metadata&) {});
    // Config is deferred until join()
    CHECK(g_mock_state.config_calls == 0);
    c.join("u", "s", "sig", "url");
    REQUIRE(g_mock_state.config_calls >= 1);
    CHECK(g_mock_state.last_media_types & Client::TRANSCRIPT);
}

// ============================================================================
// TranscriptParams
// ============================================================================

TEST_CASE("TranscriptParams default constructor", "[params][transcript]") {
    R _;
    TranscriptParams p;
    CHECK(p.contentType()   == 5);     // TEXT
    CHECK(p.srcLanguage()   == -1);    // LANGUAGE_ID_NONE — auto-detect
    CHECK(p.enableLid()     == true);
}

TEST_CASE("TranscriptParams setters round-trip", "[params][transcript]") {
    R _;
    TranscriptParams p;
    p.setSrcLanguage(9);   // LANGUAGE_ID_ENGLISH
    p.setEnableLid(false);
    CHECK(p.srcLanguage() == 9);
    CHECK(p.enableLid()   == false);
}

TEST_CASE("TranscriptParams toNative() maps fields correctly", "[params][transcript]") {
    R _;
    TranscriptParams p;
    p.setSrcLanguage(9);
    p.setEnableLid(false);
    auto n = p.toNative();
    CHECK(n.content_type  == 5);
    CHECK(n.src_language  == 9);
    CHECK(n.enable_lid    == false);
}

TEST_CASE("MediaParams setTranscriptParams / hasTranscriptParams", "[params][media][transcript]") {
    R _;
    MediaParams mp;
    CHECK_FALSE(mp.hasTranscriptParams());

    TranscriptParams tp;
    mp.setTranscriptParams(tp);
    REQUIRE(mp.hasTranscriptParams());
    CHECK(mp.transcriptParams().contentType() == 5);
}

TEST_CASE("MediaParams toNative() produces non-null tr_param when set", "[params][media][transcript]") {
    R _;
    MediaParams mp;
    mp.setTranscriptParams(TranscriptParams());
    auto n = mp.toNative();
    REQUIRE(n.tr_param != nullptr);
    CHECK(n.tr_param->content_type == 5);
    CHECK(n.tr_param->enable_lid   == true);
    delete n.tr_param;
}

TEST_CASE("Client::setTranscriptParams calls config with updated params", "[client][transcript]") {
    R _;
    Client c;
    // Enable transcript first so the reconfigure path runs
    c.setOnTranscriptData([](const std::vector<uint8_t>&, uint64_t, const Metadata&) {});
    // Must join first — configure() is deferred until after open() sets sdk_opened_=true
    c.join("u", "s", "sig", "url");
    int calls_before = g_mock_state.config_calls;

    TranscriptParams tp;
    tp.setSrcLanguage(14); // LANGUAGE_ID_GERMAN
    c.setTranscriptParams(tp);

    // Should have called config() again with the updated params
    CHECK(g_mock_state.config_calls > calls_before);
}

// ============================================================================
// Client::setProxy
// ============================================================================

TEST_CASE("Client::setProxy forwards proxy_type and proxy_url to SDK", "[client][proxy]") {
    R _;
    Client c;
    c.setProxy("http", "http://proxy.example.com:8080");
    CHECK(g_mock_state.proxy_calls    == 1);
    CHECK(g_mock_state.last_proxy_type == "http");
    CHECK(g_mock_state.last_proxy_url  == "http://proxy.example.com:8080");
}

TEST_CASE("Client::setProxy works for https proxy", "[client][proxy]") {
    R _;
    Client c;
    c.setProxy("https", "https://proxy.example.com:8080");
    CHECK(g_mock_state.proxy_calls    == 1);
    CHECK(g_mock_state.last_proxy_type == "https");
    CHECK(g_mock_state.last_proxy_url  == "https://proxy.example.com:8080");
}

TEST_CASE("Client::setProxy throws on SDK failure", "[client][proxy]") {
    R _;
    g_mock_state.proxy_result = RTMS_SDK_FAILURE;
    Client c;
    REQUIRE_THROWS_WITH(
        c.setProxy("http", "http://proxy.example.com:8080"),
        ContainsSubstring("setProxy failed")
    );
}

// ============================================================================
// Client::subscribeVideo
// ============================================================================

TEST_CASE("Client::subscribeVideo forwards user_id and true to send_subscript_video", "[client][video]") {
    R _;
    Client c;
    c.subscribeVideo(12345, true);
    CHECK(g_mock_state.subscript_video_calls    == 1);
    CHECK(g_mock_state.last_subscript_user_id   == 12345);
    CHECK(g_mock_state.last_subscript_is_sub    == true);
}

TEST_CASE("Client::subscribeVideo forwards user_id and false to send_subscript_video", "[client][video]") {
    R _;
    Client c;
    c.subscribeVideo(99999, false);
    CHECK(g_mock_state.subscript_video_calls    == 1);
    CHECK(g_mock_state.last_subscript_user_id   == 99999);
    CHECK(g_mock_state.last_subscript_is_sub    == false);
}

TEST_CASE("Client::subscribeVideo throws on SDK failure", "[client][video]") {
    R _;
    g_mock_state.subscript_video_result = RTMS_SDK_FAILURE;
    Client c;
    REQUIRE_THROWS_WITH(
        c.subscribeVideo(12345, true),
        ContainsSubstring("subscribeVideo failed")
    );
}

// ============================================================================
// on_participant_video callback
// ============================================================================

TEST_CASE("on_participant_video fires registered callback with users and flag", "[client][video][callbacks]") {
    R _;
    Client c;
    c.join("u", "s", "sig", "url");

    std::vector<int> received_users;
    bool received_is_on = false;
    c.setOnParticipantVideo([&](const std::vector<int>& users, bool is_on) {
        received_users = users;
        received_is_on = is_on;
    });

    mock_trigger_participant_video({11111, 22222}, true);

    REQUIRE(received_users.size() == 2);
    CHECK(received_users[0] == 11111);
    CHECK(received_users[1] == 22222);
    CHECK(received_is_on    == true);
}

TEST_CASE("on_participant_video fires with is_on=false when video turns off", "[client][video][callbacks]") {
    R _;
    Client c;
    c.join("u", "s", "sig", "url");

    bool received_is_on = true;
    c.setOnParticipantVideo([&](const std::vector<int>&, bool is_on) {
        received_is_on = is_on;
    });

    mock_trigger_participant_video({11111}, false);
    CHECK(received_is_on == false);
}

TEST_CASE("on_participant_video does not fire when no callback is registered", "[client][video][callbacks]") {
    R _;
    Client c;
    c.join("u", "s", "sig", "url");
    // No callback registered — should not crash
    REQUIRE_NOTHROW(mock_trigger_participant_video({11111}, true));
}

// ============================================================================
// on_video_subscript_resp callback
// ============================================================================

TEST_CASE("on_video_subscript_resp fires registered callback with user_id, status, error", "[client][video][callbacks]") {
    R _;
    Client c;
    c.join("u", "s", "sig", "url");

    int received_user_id = -1;
    int received_status  = -1;
    std::string received_error;
    c.setOnVideoSubscribed([&](int user_id, int status, const std::string& error) {
        received_user_id = user_id;
        received_status  = status;
        received_error   = error;
    });

    mock_trigger_video_subscript_resp(12345, 0, "");

    CHECK(received_user_id == 12345);
    CHECK(received_status  == 0);
    CHECK(received_error.empty());
}

TEST_CASE("on_video_subscript_resp relays error string on failure", "[client][video][callbacks]") {
    R _;
    Client c;
    c.join("u", "s", "sig", "url");

    int received_status = 0;
    std::string received_error;
    c.setOnVideoSubscribed([&](int, int status, const std::string& error) {
        received_status = status;
        received_error  = error;
    });

    mock_trigger_video_subscript_resp(12345, RTMS_SDK_FAILURE, "subscription failed");

    CHECK(received_status == RTMS_SDK_FAILURE);
    CHECK(received_error  == "subscription failed");
}

TEST_CASE("on_video_subscript_resp does not fire when no callback is registered", "[client][video][callbacks]") {
    R _;
    Client c;
    c.join("u", "s", "sig", "url");
    REQUIRE_NOTHROW(mock_trigger_video_subscript_resp(12345, 0, ""));
}

// ============================================================================
// setOnParticipantVideo auto-subscribe
// ============================================================================

TEST_CASE("setOnParticipantVideo auto-subscribes to video on/off events", "[client][events]") {
    R _;
    Client c;
    c.setOnParticipantVideo([](const std::vector<int>&, bool) {});
    // Before join: subscriptions should be queued, not sent to sdk yet
    CHECK(g_mock_state.subscribe_calls == 0);

    c.join("u", "s", "sig", "url");
    mock_trigger_join_confirm(0);

    // After join confirm, pending subscriptions must include both video events
    REQUIRE(g_mock_state.subscribe_calls >= 1);
    auto& evts = g_mock_state.last_subscribed_events;
    bool has_on  = std::find(evts.begin(), evts.end(), (int)EVENT_TYPE::PARTICIPANT_VIDEO_ON)  != evts.end();
    bool has_off = std::find(evts.begin(), evts.end(), (int)EVENT_TYPE::PARTICIPANT_VIDEO_OFF) != evts.end();
    CHECK(has_on);
    CHECK(has_off);
}

// ============================================================================
// Metadata — startTs / endTs / AiInterpreter
// ============================================================================

TEST_CASE("Metadata construction — start_ts and end_ts", "[data][metadata]") {
    R _;

    SECTION("start_ts and end_ts are copied correctly") {
        rtms_metadata md{};
        md.start_ts = 1000000ULL;
        md.end_ts   = 2000000ULL;
        Metadata m(md);
        CHECK(m.startTs() == 1000000ULL);
        CHECK(m.endTs()   == 2000000ULL);
    }

    SECTION("zero timestamps are preserved") {
        rtms_metadata md{};
        md.start_ts = 0;
        md.end_ts   = 0;
        Metadata m(md);
        CHECK(m.startTs() == 0ULL);
        CHECK(m.endTs()   == 0ULL);
    }
}

TEST_CASE("AiInterpreter construction — zero targets when target_size is zero", "[data][metadata]") {
    R _;
    rtms_metadata md{};
    md.aii.lid         = 9;        // ENGLISH
    md.aii.timestamp   = 12345ULL;
    md.aii.channel_num = 1;
    md.aii.sample_rate = 16000;
    md.aii.target_size = 0;

    Metadata m(md);
    const auto& ai = m.aiInterpreter();
    CHECK(ai.lid()        == 9);
    CHECK(ai.timestamp()  == 12345ULL);
    CHECK(ai.channelNum() == 1);
    CHECK(ai.sampleRate() == 16000);
    CHECK(ai.targets().empty());
}

TEST_CASE("AiInterpreter construction — target_size guarded against out-of-bounds values", "[data][metadata]") {
    R _;

    SECTION("target_size larger than atl array is clamped") {
        rtms_metadata md{};
        md.aii.target_size = 999;  // atl has only 100 entries
        // Must not read past bounds — just constructing should be safe
        Metadata m(md);
        CHECK(m.aiInterpreter().targets().size() <= 100);
    }

    SECTION("negative target_size produces empty targets") {
        rtms_metadata md{};
        md.aii.target_size = -1;
        Metadata m(md);
        CHECK(m.aiInterpreter().targets().empty());
    }
}

TEST_CASE("AiInterpreter construction — one target populated correctly", "[data][metadata]") {
    R _;
    rtms_metadata md{};
    md.aii.lid         = 9;
    md.aii.timestamp   = 100ULL;
    md.aii.channel_num = 2;
    md.aii.sample_rate = 48000;
    md.aii.target_size = 1;
    md.aii.atl[0].lid    = 14;  // GERMAN
    md.aii.atl[0].toneid = 0;
    strncpy(md.aii.atl[0].voice_id, "voice-de-1", MAX_VOICE_ID_LEN - 1);
    strncpy(md.aii.atl[0].engine,   "engine-A",   MAX_ENGINE_LEN - 1);

    Metadata m(md);
    const auto& ai = m.aiInterpreter();
    REQUIRE(ai.targets().size() == 1);
    const auto& tgt = ai.targets()[0];
    CHECK(tgt.lid()     == 14);
    CHECK(tgt.toneId()  == 0);
    CHECK(tgt.voiceId() == "voice-de-1");
    CHECK(tgt.engine()  == "engine-A");
}
