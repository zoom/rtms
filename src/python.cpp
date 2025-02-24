#include <pybind11/pybind11.h>
#include <pybind11/functional.h>

#include <thread>
#include <functional>

#include "rtms.h"

using namespace std;
using namespace RTMS;
namespace py = pybind11;

RTMS rtms;

onJoinConfirmFunc fn_onJoinConfirm;
onUserUpdateFunc fn_onUserUpdate;
onSessionUpdateFunc fn_onSessionUpdate;
onAudioDataFunc fn_onAudioData;
onVideoDataFunc fn_onVideoData;
onTranscriptDataFunc fn_onTranscriptData;
onLeaveFunc fn_onLeave;


void onJoinConfirm(struct rtms_csdk* sdk, int reason) {
                fn_onJoinConfirm(reason);
}

void onSessionUpdate(struct rtms_csdk* sdk,int op, struct session_info* session) {
    fn_onSessionUpdate(op, session);
}

void setOnJoinConfirm(const onJoinConfirmFunc& fn) {
    fn_onJoinConfirm = fn;
    rtms.setOnJoinConfirm(onJoinConfirm);
}

void setOnSessionUpdate(const onSessionUpdateFunc& fn) {
    fn_onSessionUpdate = fn;
    rtms.setOnSessionUpdate(onSessionUpdate);
}

void setOnAudioData(const onAudioDataFunc& fn) {
    fn_onAudioData = fn;
}

void setOnVideoData(const onVideoDataFunc& fn) {
    fn_onVideoData = fn;
}

void setOnTranscriptData(const onTranscriptDataFunc& fn) {
    fn_onTranscriptData = fn;
}

void setOnLeave(const onLeaveFunc& fn) {
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