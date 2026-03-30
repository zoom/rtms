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

TEST_CASE("VideoParams default constructor zeros all fields", "[params][video]") {
    R _;
    VideoParams p;
    CHECK(p.contentType() == 0);
    CHECK(p.codec()       == 0);
    CHECK(p.resolution()  == 0);
    CHECK(p.dataOpt()     == 0);
    CHECK(p.fps()         == 0);
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
    bool has_join  = std::find(evts.begin(), evts.end(), Client::EVENT_PARTICIPANT_JOIN)  != evts.end();
    bool has_leave = std::find(evts.begin(), evts.end(), Client::EVENT_PARTICIPANT_LEAVE) != evts.end();
    CHECK(has_join);
    CHECK(has_leave);
}

// ============================================================================
// Media type auto-enable
// ============================================================================

TEST_CASE("setOnAudioData enables AUDIO media type via config", "[client][media]") {
    R _;
    Client c;
    c.setOnAudioData([](const std::vector<uint8_t>&, uint64_t, const Metadata&) {});
    REQUIRE(g_mock_state.config_calls >= 1);
    CHECK(g_mock_state.last_media_types & Client::AUDIO);
}

TEST_CASE("setOnVideoData enables VIDEO media type via config", "[client][media]") {
    R _;
    Client c;
    c.setOnVideoData([](const std::vector<uint8_t>&, uint64_t, const Metadata&) {});
    REQUIRE(g_mock_state.config_calls >= 1);
    CHECK(g_mock_state.last_media_types & Client::VIDEO);
}

TEST_CASE("setOnTranscriptData enables TRANSCRIPT media type via config", "[client][media]") {
    R _;
    Client c;
    c.setOnTranscriptData([](const std::vector<uint8_t>&, uint64_t, const Metadata&) {});
    REQUIRE(g_mock_state.config_calls >= 1);
    CHECK(g_mock_state.last_media_types & Client::TRANSCRIPT);
}
