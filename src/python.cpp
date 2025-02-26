#include <pybind11/pybind11.h>
#include <pybind11/functional.h>

#include <thread>
#include <functional>

#include "rtms.h"

using namespace std;
using namespace rtms;
namespace py = pybind11;

Client client;

JoinConfirmFn onJoinConfirmFn;
SessionUpdateFn onSessionUpdateFn;
UserUpdateFn onUserUpdateFn;
AudioDataFn onAudioDataFn;
VideoDataFn onVideoDataFn;
TranscriptDataFn onTranscriptDataFn;
LeaveFn onLeaveFn;

void onJoinConfirm(struct rtms_csdk* sdk, int reason) {
    onJoinConfirmFn(reason);
}

void setOnJoinConfirm(const JoinConfirmFn fn) {
    onJoinConfirmFn = fn;
    client.setOnJoinConfirm(onJoinConfirm);
}

void onSessionUpdate(struct rtms_csdk* sdk, int op, struct session_info* session) {
    onSessionUpdateFn(op, session);
}

void setOnSessionUpdate(const SessionUpdateFn fn) {
    onSessionUpdateFn = fn;
    client.setOnSessionUpdate(onSessionUpdate);
}

void onUserUpdate(struct rtms_csdk* sdk, int op, struct participant_info* pi) {
    onUserUpdateFn(op, pi);
}

void setOnUserUpdate(const UserUpdateFn fn) {
    onUserUpdateFn = fn;
    client.setOnUserUpdate(onUserUpdate);
}

void onAudioData(struct rtms_csdk* sdk, unsigned char* buf, int size, unsigned int ts, struct rtms_metadata* md) {
    onAudioDataFn(buf, size, ts, md);
}

void setOnAudioData(const AudioDataFn fn) {
    onAudioDataFn = fn;
    client.setOnAudioData(onAudioData);
}

void onVideoData(struct rtms_csdk* sdk, unsigned char* buf, int size, unsigned int ts, const char* trackId, struct rtms_metadata* md) {
    onVideoDataFn(buf, size, ts, trackId, md);
}

void setOnVideoData(const VideoDataFn fn) {
    onVideoDataFn = fn;
    client.setOnVideoData(onVideoData);
}

void onTranscriptData(struct rtms_csdk* sdk, unsigned char* buf, int size, unsigned int ts, struct rtms_metadata* md) {
    onTranscriptDataFn(buf, size, ts, md);
}

void setOnTranscriptData(const TranscriptDataFn fn) {
    onTranscriptDataFn = fn;
    client.setOnTranscriptData(onTranscriptData);
}

void onLeave(struct rtms_csdk* sdk, int reason) {
    onLeaveFn(reason);
}

void setOnLeave(const LeaveFn fn) {
    onLeaveFn = fn;
    client.setOnLeave(onLeave);
}

int init(const string& ca_path) {
    return client.init(ca_path);
}

int join(const string& uuid, const string& streamId, const string& signature, const string& serverUrls, int timeout = -1) {
    return client.join(uuid, streamId, signature, serverUrls, timeout);
}

bool isInit() {
    return client.isInit();
}

PYBIND11_MODULE(_rtms, m) {
    m.doc() = "Zoom RTMS Python Bindings";

    m.def("_init", &init, "[_internal] Initialize the RTMS SDK with the specified CA certificate path");
    m.def("_join", &join, "[_internal] Join a meeting with the specified parameters");
    m.def("_is_init", &isInit, "[_internal] Check if the RTMS SDK is initialized");

    m.def("on_join_confirm", &setOnJoinConfirm, "Set callback for join confirmation events");
    m.def("on_session_update", &setOnSessionUpdate, "Set callback for session update events");
    m.def("on_user_update", &setOnUserUpdate, "Set callback for participant update events");
    m.def("on_audio_data", &setOnAudioData, "Set callback for receiving audio data");
    m.def("on_video_data", &setOnVideoData, "Set callback for receiving video data");
    m.def("on_transcript_data", &setOnTranscriptData, "Set callback for receiving transcript data");
    m.def("on_leave", &setOnLeave, "Set callback for leave events");
}