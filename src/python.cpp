#include <pybind11/pybind11.h>
#include <pybind11/functional.h>

#include <thread>
#include <functional>

#include "rtms.h"

namespace py = pybind11;
using namespace std;

RTMS rtms;

RTMS::onJoinConfirmFn fn_onJoinConfirm;
RTMS::onSessionUpdateFn fn_onSessionUpdate;
RTMS::onAudioDataFn fn_onAudioData;
RTMS::onVideoDataFn fn_onVideoData;
RTMS::onTranscriptDataFn fn_onTranscriptData;
RTMS::onLeaveFn fn_onLeave;


void onJoinConfirm(struct rtms_csdk* sdk, int reason) {
                fn_onJoinConfirm(reason);
}

void onSessionUpdate(struct rtms_csdk* sdk,int op, struct session_info* session) {
    fn_onSessionUpdate(op, session);
}

void setOnJoinConfirm(const RTMS::onJoinConfirmFn& fn) {
    fn_onJoinConfirm = fn;
    rtms.setOnJoinConfirm(onJoinConfirm);
}

void setOnSessionUpdate(const RTMS::onSessionUpdateFn& fn) {
    fn_onSessionUpdate = fn;
    rtms.setOnSessionUpdate(onSessionUpdate);
}

void setOnAudioData(const RTMS::onAudioDataFn& fn) {
    fn_onAudioData = fn;
}

void setOnVideoData(const RTMS::onVideoDataFn& fn) {
    fn_onVideoData = fn;
}

void setOnTranscriptData(const RTMS::onTranscriptDataFn& fn) {
    fn_onTranscriptData = fn;
}

void setOnLeave(const RTMS::onLeaveFn& fn) {
    fn_onLeave = fn;
}

int init (const string& ca_path) {
    rtms.setOnJoinConfirm(onJoinConfirm);
    rtms.setOnSessionUpdate(onSessionUpdate);
    
    return rtms.init(ca_path);
}

int join (const string& uuid, const string& streamId, const string& signature, const string& serverUrls, int timeout = -1) {
    return rtms.join(uuid, streamId, signature, serverUrls, timeout);
}

bool isInit() {
    return rtms.isInit();
}

PYBIND11_MODULE(_rtms, m) {
    m.doc() = "Zoom RTMS Python Bindings";

    m.def("_init", &init);
    m.def("_join", &join);
    m.def("_is_init", &isInit);

    m.def("on_join_confirm", &setOnJoinConfirm);
    m.def("on_session_update", &setOnSessionUpdate, py::call_guard<py::gil_scoped_release>());
    m.def("on_audio_data", &setOnAudioData, py::call_guard<py::gil_scoped_release>());
    m.def("on_video_data", &setOnVideoData, py::call_guard<py::gil_scoped_release>());
    m.def("on_transcript_data", &setOnTranscriptData, py::call_guard<py::gil_scoped_release>());
    m.def("on_leave", &setOnLeave, py::call_guard<py::gil_scoped_release>());
}