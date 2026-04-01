/**
 * Mock implementations of rtms_sdk, rtms_sdk_provider, and g_agent.
 * Linked in place of the real Zoom RTMS shared library for unit tests.
 */

#include "mock_sdk.h"
#include <string>
#include <mutex>

// Required by rtms_sdk.h extern declaration
std::string g_agent;

// Global mock state — reset in each test
MockSdkState g_mock_state;

// ============================================================================
// rtms_sdk stubs
// rtms_sdk_impl is only forward-declared; m_impl is never used in the mock.
// ============================================================================

rtms_sdk::rtms_sdk() : m_impl(nullptr) {}
rtms_sdk::~rtms_sdk() {}

int rtms_sdk::open(rtms_sdk_sink* sink) {
    ++g_mock_state.open_calls;
    g_mock_state.last_sink = sink;
    return g_mock_state.open_result;
}

int rtms_sdk::config(struct media_parameters* params, int media_types, int ale, int feature_ids) {
    ++g_mock_state.config_calls;
    g_mock_state.last_media_types = media_types;
    g_mock_state.last_ale         = ale;
    return g_mock_state.config_result;
}

int rtms_sdk::set_proxy(std::string proxy_type, std::string proxy_url) {
    ++g_mock_state.proxy_calls;
    g_mock_state.last_proxy_type = std::move(proxy_type);
    g_mock_state.last_proxy_url  = std::move(proxy_url);
    return g_mock_state.proxy_result;
}

int rtms_sdk::join(const char* meeting_uuid, const char* rtms_stream_id,
                   const char* signature, const char* server_url, int timeout) {
    ++g_mock_state.join_calls;
    g_mock_state.last_meeting_uuid = meeting_uuid  ? meeting_uuid  : "";
    g_mock_state.last_stream_id    = rtms_stream_id ? rtms_stream_id : "";
    g_mock_state.last_signature    = signature     ? signature     : "";
    g_mock_state.last_server_url   = server_url    ? server_url    : "";
    g_mock_state.last_timeout      = timeout;
    return g_mock_state.join_result;
}

int rtms_sdk::send_data(int, unsigned char*, int, uint64_t, void*) {
    return RTMS_SDK_OK;
}

int rtms_sdk::leave(int) {
    ++g_mock_state.leave_calls;
    return g_mock_state.leave_result;
}

int rtms_sdk::poll() {
    ++g_mock_state.poll_calls;
    return g_mock_state.poll_result;
}

int rtms_sdk::subscribe_event(int events[], int len) {
    ++g_mock_state.subscribe_calls;
    g_mock_state.last_subscribed_events.assign(events, events + len);
    return g_mock_state.subscribe_result;
}

int rtms_sdk::unsubscribe_event(int events[], int len) {
    ++g_mock_state.unsubscribe_calls;
    g_mock_state.last_unsubscribed_events.assign(events, events + len);
    return g_mock_state.unsubscribe_result;
}

int rtms_sdk::send_subscript_video(int user_id, bool is_sub) {
    ++g_mock_state.subscript_video_calls;
    g_mock_state.last_subscript_user_id = user_id;
    g_mock_state.last_subscript_is_sub  = is_sub;
    return g_mock_state.subscript_video_result;
}

// ============================================================================
// rtms_sdk_provider stubs
// ============================================================================

rtms_sdk_provider* rtms_sdk_provider::s_inst = nullptr;
std::mutex         rtms_sdk_provider::mtx;

rtms_sdk_provider::rtms_sdk_provider()  {}
rtms_sdk_provider::~rtms_sdk_provider() {}

rtms_sdk_provider* rtms_sdk_provider::instance() {
    std::lock_guard<std::mutex> lock(mtx);
    if (!s_inst) s_inst = new rtms_sdk_provider();
    return s_inst;
}

int rtms_sdk_provider::init(const char* ca_path, bool is_verify_cert) {
    ++g_mock_state.init_calls;
    g_mock_state.last_ca_path      = ca_path ? ca_path : "";
    g_mock_state.last_verify_cert  = is_verify_cert;
    return g_mock_state.init_result;
}

rtms_sdk* rtms_sdk_provider::create_sdk() {
    ++g_mock_state.create_calls;
    if (g_mock_state.create_result != 0) return nullptr;
    return new rtms_sdk();
}

int rtms_sdk_provider::release_sdk(rtms_sdk* sdk) {
    ++g_mock_state.release_calls;
    delete sdk;
    return RTMS_SDK_OK;
}

void rtms_sdk_provider::uninit() {
    ++g_mock_state.uninit_calls;
}

// ============================================================================
// Callback trigger helpers
// ============================================================================

void mock_trigger_join_confirm(int reason) {
    if (g_mock_state.last_sink)
        g_mock_state.last_sink->on_join_confirm(reason);
}

void mock_trigger_session_update(int op, session_info* sess) {
    if (g_mock_state.last_sink)
        g_mock_state.last_sink->on_session_update(op, sess);
}

void mock_trigger_user_update(int op, participant_info* pi) {
    if (g_mock_state.last_sink)
        g_mock_state.last_sink->on_user_update(op, pi);
}

void mock_trigger_audio_data(unsigned char* buf, int size, uint64_t ts, rtms_metadata* md) {
    if (g_mock_state.last_sink)
        g_mock_state.last_sink->on_audio_data(buf, size, ts, md);
}

void mock_trigger_video_data(unsigned char* buf, int size, uint64_t ts, rtms_metadata* md) {
    if (g_mock_state.last_sink)
        g_mock_state.last_sink->on_video_data(buf, size, ts, md);
}

void mock_trigger_ds_data(unsigned char* buf, int size, uint64_t ts, rtms_metadata* md) {
    if (g_mock_state.last_sink)
        g_mock_state.last_sink->on_ds_data(buf, size, ts, md);
}

void mock_trigger_transcript_data(unsigned char* buf, int size, uint64_t ts, rtms_metadata* md) {
    if (g_mock_state.last_sink)
        g_mock_state.last_sink->on_transcript_data(buf, size, ts, md);
}

void mock_trigger_leave(int reason) {
    if (g_mock_state.last_sink)
        g_mock_state.last_sink->on_leave(reason);
}

void mock_trigger_event_ex(const std::string& compact_str) {
    if (g_mock_state.last_sink)
        g_mock_state.last_sink->on_event_ex(compact_str);
}

void mock_trigger_participant_video(std::vector<int> users, bool is_on) {
    if (g_mock_state.last_sink)
        g_mock_state.last_sink->on_participant_video(std::move(users), is_on);
}

void mock_trigger_video_subscript_resp(int user_id, int status, std::string error) {
    if (g_mock_state.last_sink)
        g_mock_state.last_sink->on_video_subscript_resp(user_id, status, std::move(error));
}
