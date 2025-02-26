#include <pybind11/pybind11.h>
#include <pybind11/functional.h>

#include <thread>
#include <functional>

#include "rtms.h"

using namespace std;
namespace py = pybind11;


RTMS rtms;

void setOnJoinConfirm(const fn_on_join_confirm fn) {
    rtms.setOnJoinConfirm(fn);
}

void setOnSessionUpdate(const fn_on_session_update fn) {
    rtms.setOnSessionUpdate(fn);
}

void setOnAudioData(const fn_on_audio_data fn) {
    rtms.setOnAudioData(fn);
}

void setOnVideoData(const fn_on_video_data fn) {
    rtms.setOnVideoData(fn);
}

void setOnTranscriptData(const fn_on_transcript_data fn) {
    rtms.setOnTranscriptData(fn);
}

void setOnLeave(const fn_on_leave fn) {
    rtms.setOnLeave(fn);
} 

int init (const string& ca_path) {
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