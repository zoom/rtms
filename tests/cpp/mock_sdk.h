/**
 * Mock SDK state for C++ unit tests.
 *
 * Provides:
 *   - g_mock_state  — per-test configurable return values and call records
 *   - mock_trigger_*  — helpers to fire SDK callbacks on the registered sink
 *
 * Usage in tests:
 *   g_mock_state.reset();           // called in each test fixture
 *   g_mock_state.join_result = RTMS_SDK_FAILURE;   // inject failure
 *   mock_trigger_join_confirm(0);   // simulate SDK firing a callback
 */

#pragma once

#include "rtms_sdk.h"
#include <string>
#include <vector>
#include <cstdint>

struct MockSdkState {
    // --- Configurable return values ---
    int create_result   = 0;        // 0 = return real sdk, non-zero = return nullptr
    int init_result     = RTMS_SDK_OK;
    int open_result     = RTMS_SDK_OK;
    int config_result   = RTMS_SDK_OK;
    int join_result     = RTMS_SDK_OK;
    int poll_result     = RTMS_SDK_OK;
    int leave_result    = RTMS_SDK_OK;
    int subscribe_result   = RTMS_SDK_OK;
    int unsubscribe_result = RTMS_SDK_OK;
    int proxy_result       = RTMS_SDK_OK;
    int subscript_video_result = RTMS_SDK_OK;

    // --- Call counters ---
    int init_calls       = 0;
    int uninit_calls     = 0;
    int create_calls     = 0;
    int release_calls    = 0;
    int open_calls       = 0;
    int config_calls     = 0;
    int join_calls       = 0;
    int poll_calls       = 0;
    int leave_calls      = 0;
    int subscribe_calls  = 0;
    int unsubscribe_calls = 0;
    int proxy_calls      = 0;
    int subscript_video_calls = 0;

    // --- Recorded arguments ---
    rtms_sdk_sink* last_sink = nullptr;

    // init()
    std::string last_ca_path;
    bool        last_verify_cert = true;

    // join()
    std::string last_meeting_uuid;
    std::string last_stream_id;
    std::string last_signature;
    std::string last_server_url;
    int         last_timeout = 0;

    // config()
    int last_media_types = 0;
    int last_ale         = 0;           // application layer encryption flag

    // set_proxy()
    std::string last_proxy_type;
    std::string last_proxy_url;

    // subscribe_event() / unsubscribe_event()
    std::vector<int> last_subscribed_events;
    std::vector<int> last_unsubscribed_events;

    // send_subscript_video()
    int  last_subscript_user_id = 0;
    bool last_subscript_is_sub  = false;

    void reset() { *this = MockSdkState{}; }
};

extern MockSdkState g_mock_state;

// ============================================================================
// Callback trigger helpers — simulate the SDK firing events on the Client
// ============================================================================

void mock_trigger_join_confirm(int reason);
void mock_trigger_session_update(int op, session_info* sess);
void mock_trigger_user_update(int op, participant_info* pi);
void mock_trigger_audio_data(unsigned char* buf, int size, uint64_t ts, rtms_metadata* md);
void mock_trigger_video_data(unsigned char* buf, int size, uint64_t ts, rtms_metadata* md);
void mock_trigger_ds_data(unsigned char* buf, int size, uint64_t ts, rtms_metadata* md);
void mock_trigger_transcript_data(unsigned char* buf, int size, uint64_t ts, rtms_metadata* md);
void mock_trigger_leave(int reason);
void mock_trigger_event_ex(const std::string& compact_str);
void mock_trigger_participant_video(std::vector<int> users, bool is_on);
void mock_trigger_video_subscript_resp(int user_id, int status, std::string error);
