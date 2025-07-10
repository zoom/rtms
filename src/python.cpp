#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include <thread>
#include <functional>
#include <memory>
#include <iostream>

#include "rtms.h"

using namespace std;
using namespace rtms;
namespace py = pybind11;

#define RTMS_PY_DEBUG 1

#if RTMS_PY_DEBUG
#define DEBUG_LOG(msg) std::cerr << "[RTMS-CORE-SDK] " << msg << std::endl
#else
#define DEBUG_LOG(msg)
#endif

class PyClient {
private:
    unique_ptr<Client> client_;
    int configuredMediaTypes_;
    bool isConfigured_;
    bool isShuttingDown_;  // NEW: Track shutdown state
    
    py::function joinConfirmCallback;
    py::function sessionUpdateCallback;
    py::function userUpdateCallback;
    py::function audioDataCallback;
    py::function videoDataCallback;
    py::function deskshareDataCallback;
    py::function transcriptDataCallback;
    py::function leaveCallback;

public:
    PyClient() : configuredMediaTypes_(0), isConfigured_(false), isShuttingDown_(false) {
        try {
            client_ = make_unique<Client>();
        } catch (const exception& e) {
            DEBUG_LOG("Error creating client instance: " << e.what());
            throw py::error_already_set();
        }
    }

    static void initialize(const string& ca_path) {
        try {
            Client::initialize(ca_path);
        } catch (const Exception& e) {
            DEBUG_LOG("RTMS initialization error: " << e.what() << " (code: " << e.code() << ")");
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        } catch (const exception& e) {
            DEBUG_LOG("Unknown error during initialization: " << e.what());
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        }
    }

    static void uninitialize() {
        try {
            Client::uninitialize();
        } catch (const Exception& e) {
            DEBUG_LOG("RTMS uninitialization error: " << e.what() << " (code: " << e.code() << ")");
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        } catch (const exception& e) {
            DEBUG_LOG("Unknown error during uninitialization: " << e.what());
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        }
    }

    void join(const string& uuid, const string& streamId, const string& signature, 
              const string& serverUrls, int timeout = -1) {
        try {
            client_->join(uuid, streamId, signature, serverUrls, timeout);
        } catch (const Exception& e) {
            DEBUG_LOG("RTMS join error: " << e.what() << " (code: " << e.code() << ")");
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        } catch (const exception& e) {
            DEBUG_LOG("Unknown error during join: " << e.what());
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        }
    }

    void joinWithOptions(py::dict options) {
        try {
            string uuid = options["meeting_uuid"].cast<string>();
            string streamId = options["rtms_stream_id"].cast<string>();
            string serverUrls = options["server_urls"].cast<string>();

            string signature = options.contains("signature") ? options["signature"].cast<string>() : "";
            string client = options.contains("client") ? options["client"].cast<string>() : "";
            string secret = options.contains("secret") ? options["secret"].cast<string>() : "";
            int timeout = options.contains("timeout") ? options["timeout"].cast<int>() : -1;
            
            join(uuid, streamId, signature, serverUrls, timeout);
        } catch (const Exception& e) {
            DEBUG_LOG("RTMS join with options error: " << e.what() << " (code: " << e.code() << ")");
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        } catch (const exception& e) {
            DEBUG_LOG("Unknown error during join with options: " << e.what());
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        }
    }

    void poll() {
        try {
            client_->poll();
        } catch (const Exception& e) {
            DEBUG_LOG("RTMS poll error: " << e.what() << " (code: " << e.code() << ")");
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        } catch (const exception& e) {
            DEBUG_LOG("Unknown error during poll: " << e.what());
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        }
    }

    void release() {
        isShuttingDown_ = true;  // Mark as shutting down
        
        try {
            if (client_) {
                // First set all callbacks to no-ops to prevent callback during release
                client_->setOnJoinConfirm([](int) {});
                client_->setOnSessionUpdate([](int, const Session&) {});
                client_->setOnUserUpdate([](int, const Participant&) {});
                client_->setOnAudioData([](const vector<uint8_t>&, uint64_t, const Metadata&) {});
                client_->setOnVideoData([](const vector<uint8_t>&, uint64_t, const Metadata&) {});
                client_->setOnDeskshareData([](const vector<uint8_t>&, uint64_t, const Metadata&) {});
                client_->setOnTranscriptData([](const vector<uint8_t>&, uint64_t, const Metadata&) {});
                client_->setOnLeave([](int) {});
                
                // Now release the client
                client_->release();
            }
        } catch (const Exception& e) {
            DEBUG_LOG("RTMS release error: " << e.what() << " (code: " << e.code() << ")");
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        } catch (const exception& e) {
            DEBUG_LOG("Unknown error during release: " << e.what());
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        }
    }

    string uuid() {
        try {
            return client_->uuid();
        } catch (const Exception& e) {
            DEBUG_LOG("Error getting UUID: " << e.what() << " (code: " << e.code() << ")");
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        } catch (const exception& e) {
            DEBUG_LOG("Unknown error getting UUID: " << e.what());
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        }
    }

    string streamId() {
        try {
            return client_->streamId();
        } catch (const Exception& e) {
            DEBUG_LOG("Error getting stream ID: " << e.what() << " (code: " << e.code() << ")");
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        } catch (const exception& e) {
            DEBUG_LOG("Unknown error getting stream ID: " << e.what());
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        }
    }

    void enableAudio(bool enable) {
        try {
            client_->enableAudio(enable);
        } catch (const Exception& e) {
            DEBUG_LOG("Error enabling/disabling audio: " << e.what() << " (code: " << e.code() << ")");
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        } catch (const exception& e) {
            DEBUG_LOG("Unknown error enabling/disabling audio: " << e.what());
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        }
    }

    void enableVideo(bool enable) {
        try {
            client_->enableVideo(enable);
        } catch (const Exception& e) {
            DEBUG_LOG("Error enabling/disabling video: " << e.what() << " (code: " << e.code() << ")");
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        } catch (const exception& e) {
            DEBUG_LOG("Unknown error enabling/disabling video: " << e.what());
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        }
    }

    void enableTranscript(bool enable) {
        try {
            client_->enableTranscript(enable);
        } catch (const Exception& e) {
            DEBUG_LOG("Error enabling/disabling transcript: " << e.what() << " (code: " << e.code() << ")");
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        } catch (const exception& e) {
            DEBUG_LOG("Unknown error enabling/disabling transcript: " << e.what());
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        }
    }

    void enableDeskshare(bool enable) {
        try {
            client_->enableDeskshare(enable);
        } catch (const Exception& e) {
            DEBUG_LOG("Error enabling/disabling deskshare: " << e.what() << " (code: " << e.code() << ")");
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        } catch (const exception& e) {
            DEBUG_LOG("Unknown error enabling/disabling deskshare: " << e.what());
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        }
    }

    // Methods that return functions that can be used as decorators
    py::function onJoinConfirm() {
        // Create a decorator function that will capture 'this'
        return py::cpp_function([this](py::function func) {
            this->setJoinConfirmCallback(func);
            return func;
        });
    }
    
    py::function onSessionUpdate() {
        return py::cpp_function([this](py::function func) {
            this->setSessionUpdateCallback(func);
            return func;
        });
    }
    
    py::function onUserUpdate() {
        return py::cpp_function([this](py::function func) {
            this->setUserUpdateCallback(func);
            return func;
        });
    }
    
    py::function onAudioData() {
        return py::cpp_function([this](py::function func) {
            this->setAudioDataCallback(func);
            return func;
        });
    }
    
    py::function onVideoData() {
        return py::cpp_function([this](py::function func) {
            this->setVideoDataCallback(func);
            return func;
        });
    }
    
    py::function onDeskshareData() {
        return py::cpp_function([this](py::function func) {
            this->setDeskshareDataCallback(func);
            return func;
        });
    }
    
    py::function onTranscriptData() {
        return py::cpp_function([this](py::function func) {
            this->setTranscriptDataCallback(func);
            return func;
        });
    }
    
    py::function onLeave() {
        return py::cpp_function([this](py::function func) {
            this->setLeaveCallback(func);
            return func;
        });
    }
    
    // Enhanced callback validation helper
    bool isCallbackValid(const py::function& callback) {
        if (isShuttingDown_) {
            return false;  // Don't call callbacks during shutdown
        }
        
        try {
            return !callback.is_none() && PyCallable_Check(callback.ptr());
        } catch (...) {
            return false;  // If any exception occurs during validation, consider invalid
        }
    }

    // Enhanced callback setters with improved error handling
    void setJoinConfirmCallback(py::function callback) {
        joinConfirmCallback = callback;
        
        client_->setOnJoinConfirm([this](int reason) {
            py::gil_scoped_acquire acquire;
            try {
                if (isCallbackValid(joinConfirmCallback)) {
                    joinConfirmCallback(reason);
                }
            } catch (py::error_already_set& e) {
                DEBUG_LOG("Python exception in join confirm callback: " << e.what());
                e.restore();
            } catch (const Exception& e) {
                DEBUG_LOG("RTMS exception in join confirm callback: " << e.what() << " (code: " << e.code() << ")");
            } catch (std::exception& e) {
                DEBUG_LOG("C++ exception in join confirm callback: " << e.what());
            } catch (...) {
                DEBUG_LOG("Unknown exception in join confirm callback");
            }
        });
    }
    
    void setSessionUpdateCallback(py::function callback) {
        sessionUpdateCallback = callback;
        
        client_->setOnSessionUpdate([this](int op, const Session& session) {
            py::gil_scoped_acquire acquire;
            try {
                if (isCallbackValid(sessionUpdateCallback)) {
                    sessionUpdateCallback(op, session);
                }
            } catch (py::error_already_set& e) {
                DEBUG_LOG("Python exception in session update callback: " << e.what());
                e.restore();
            } catch (const Exception& e) {
                DEBUG_LOG("RTMS exception in session update callback: " << e.what() << " (code: " << e.code() << ")");
            } catch (std::exception& e) {
                DEBUG_LOG("C++ exception in session update callback: " << e.what());
            } catch (...) {
                DEBUG_LOG("Unknown exception in session update callback");
            }
        });
    }
    
    void setUserUpdateCallback(py::function callback) {
        userUpdateCallback = callback;
        
        client_->setOnUserUpdate([this](int op, const Participant& participant) {
            py::gil_scoped_acquire acquire;
            try {
                if (isCallbackValid(userUpdateCallback)) {
                    userUpdateCallback(op, participant);
                }
            } catch (py::error_already_set& e) {
                DEBUG_LOG("Python exception in user update callback: " << e.what());
                e.restore();
            } catch (const Exception& e) {
                DEBUG_LOG("RTMS exception in user update callback: " << e.what() << " (code: " << e.code() << ")");
            } catch (std::exception& e) {
                DEBUG_LOG("C++ exception in user update callback: " << e.what());
            } catch (...) {
                DEBUG_LOG("Unknown exception in user update callback");
            }
        });
    }
    
    void setAudioDataCallback(py::function callback) {
        audioDataCallback = callback;
        cout << "setAudioDataCallback called" << endl;

        client_->setOnAudioData([this](const vector<uint8_t>& data, uint64_t timestamp, const Metadata& metadata) {
            cout << "OnAudioData callback triggered" << endl;
            py::gil_scoped_acquire acquire;
            try {
                if (isCallbackValid(audioDataCallback)) {
                    py::bytes py_data(reinterpret_cast<const char*>(data.data()), data.size());
                    audioDataCallback(py_data, data.size(), timestamp, metadata);
                }
            } catch (py::error_already_set& e) {
                DEBUG_LOG("Python exception in audio data callback: " << e.what());
                e.restore();
            } catch (const Exception& e) {
                DEBUG_LOG("RTMS exception in audio data callback: " << e.what() << " (code: " << e.code() << ")");
            } catch (std::exception& e) {
                DEBUG_LOG("C++ exception in audio data callback: " << e.what());
            } catch (...) {
                DEBUG_LOG("Unknown exception in audio data callback");
            }
        });
        
        client_->enableAudio(true);
    }
    
    void setVideoDataCallback(py::function callback) {
        videoDataCallback = callback;
        
        client_->setOnVideoData([this](const vector<uint8_t>& data, uint64_t timestamp, const Metadata& metadata) {
            py::gil_scoped_acquire acquire;
            try {
                if (isCallbackValid(videoDataCallback)) {
                    py::bytes py_data(reinterpret_cast<const char*>(data.data()), data.size());
                    videoDataCallback(py_data, data.size(), timestamp, metadata);
                }
            } catch (py::error_already_set& e) {
                DEBUG_LOG("Python exception in video data callback: " << e.what());
                e.restore();
            } catch (const Exception& e) {
                DEBUG_LOG("RTMS exception in video data callback: " << e.what() << " (code: " << e.code() << ")");
            } catch (std::exception& e) {
                DEBUG_LOG("C++ exception in video data callback: " << e.what());
            } catch (...) {
                DEBUG_LOG("Unknown exception in video data callback");
            }
        });
        
        client_->enableVideo(true);
    }

    void setDeskshareDataCallback(py::function callback) {
        deskshareDataCallback = callback;
        
        client_->setOnDeskshareData([this](const vector<uint8_t>& data, uint64_t timestamp, const Metadata& metadata) {
            py::gil_scoped_acquire acquire;
            try {
                if (isCallbackValid(deskshareDataCallback)) {
                    py::bytes py_data(reinterpret_cast<const char*>(data.data()), data.size());
                    deskshareDataCallback(py_data, data.size(), timestamp, metadata);
                }
            } catch (py::error_already_set& e) {
                DEBUG_LOG("Python exception in deskshare data callback: " << e.what());
                e.restore();
            } catch (const Exception& e) {
                DEBUG_LOG("RTMS exception in deskshare data callback: " << e.what() << " (code: " << e.code() << ")");
            } catch (std::exception& e) {
                DEBUG_LOG("C++ exception in deskshare data callback: " << e.what());
            } catch (...) {
                DEBUG_LOG("Unknown exception in deskshare data callback");
            }
        });
        
        client_->enableDeskshare(true);
    }
    
    void setTranscriptDataCallback(py::function callback) {
        transcriptDataCallback = callback;
        
        client_->setOnTranscriptData([this](const vector<uint8_t>& data, uint64_t timestamp, const Metadata& metadata) {
            py::gil_scoped_acquire acquire;
            try {
                if (isCallbackValid(transcriptDataCallback)) {
                    py::bytes py_data(reinterpret_cast<const char*>(data.data()), data.size());
                    transcriptDataCallback(py_data, data.size(), timestamp, metadata);
                }
            } catch (py::error_already_set& e) {
                DEBUG_LOG("Python exception in transcript data callback: " << e.what());
                e.restore();
            } catch (const Exception& e) {
                DEBUG_LOG("RTMS exception in transcript data callback: " << e.what() << " (code: " << e.code() << ")");
            } catch (std::exception& e) {
                DEBUG_LOG("C++ exception in transcript data callback: " << e.what());
            } catch (...) {
                DEBUG_LOG("Unknown exception in transcript data callback");
            }
        });
        
        client_->enableTranscript(true);
    }
    
    void setLeaveCallback(py::function callback) {
        leaveCallback = callback;
        
        client_->setOnLeave([this](int reason) {
            py::gil_scoped_acquire acquire;
            try {
                if (isCallbackValid(leaveCallback)) {
                    leaveCallback(reason);
                }
            } catch (py::error_already_set& e) {
                DEBUG_LOG("Python exception in leave callback: " << e.what());
                e.restore();
            } catch (const Exception& e) {
                DEBUG_LOG("RTMS exception in leave callback: " << e.what() << " (code: " << e.code() << ")");
            } catch (std::exception& e) {
                DEBUG_LOG("C++ exception in leave callback: " << e.what());
            } catch (...) {
                DEBUG_LOG("Unknown exception in leave callback");
            }
        });
    }

    void setAudioParams(py::dict params) {
        try {
            AudioParams audioParams;
            
            if (params.contains("contentType"))
                audioParams.setContentType(params["contentType"].cast<int>());
            if (params.contains("codec"))
                audioParams.setCodec(params["codec"].cast<int>());
            if (params.contains("sampleRate"))
                audioParams.setSampleRate(params["sampleRate"].cast<int>());
            if (params.contains("channel"))
                audioParams.setChannel(params["channel"].cast<int>());
            if (params.contains("dataOpt"))
                audioParams.setDataOpt(params["dataOpt"].cast<int>());
            if (params.contains("duration"))
                audioParams.setDuration(params["duration"].cast<int>());
            if (params.contains("frameSize"))
                audioParams.setFrameSize(params["frameSize"].cast<int>());
            
            client_->setAudioParams(audioParams);
        } catch (const Exception& e) {
            DEBUG_LOG("Error setting audio parameters: " << e.what() << " (code: " << e.code() << ")");
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        } catch (const exception& e) {
            DEBUG_LOG("Unknown error setting audio parameters: " << e.what());
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        }
    }

    void setVideoParams(py::dict params) {
        try {
            VideoParams videoParams;
            
            if (params.contains("contentType"))
                videoParams.setContentType(params["contentType"].cast<int>());
            if (params.contains("codec"))
                videoParams.setCodec(params["codec"].cast<int>());
            if (params.contains("resolution"))
                videoParams.setResolution(params["resolution"].cast<int>());
            if (params.contains("dataOpt"))
                videoParams.setDataOpt(params["dataOpt"].cast<int>());
            if (params.contains("fps"))
                videoParams.setFps(params["fps"].cast<int>());
            
            client_->setVideoParams(videoParams);
        } catch (const Exception& e) {
            DEBUG_LOG("Error setting video parameters: " << e.what() << " (code: " << e.code() << ")");
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        } catch (const exception& e) {
            DEBUG_LOG("Unknown error setting video parameters: " << e.what());
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        }
    }

    void setDeskshareParams(py::dict params) {
        try {
            DeskshareParams deskshareParams;
            
            if (params.contains("contentType"))
                deskshareParams.setContentType(params["contentType"].cast<int>());
            if (params.contains("codec"))
                deskshareParams.setCodec(params["codec"].cast<int>());
            if (params.contains("resolution"))
                deskshareParams.setResolution(params["resolution"].cast<int>());
            if (params.contains("fps"))
                deskshareParams.setFps(params["fps"].cast<int>());
            
            client_->setDeskshareParams(deskshareParams);
        } catch (const Exception& e) {
            DEBUG_LOG("Error setting deskshare parameters: " << e.what() << " (code: " << e.code() << ")");
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        } catch (const exception& e) {
            DEBUG_LOG("Unknown error setting deskshare parameters: " << e.what());
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        }
    }
};

// Global client for backward compatibility
static PyClient globalClient;

PYBIND11_MODULE(_rtms, m) {
    m.doc() = "Zoom RTMS Python Bindings";

    // Expose classes
    py::class_<Session>(m, "Session")
        .def_property_readonly("session_id", &Session::sessionId)
        .def_property_readonly("stat_time", &Session::statTime)
        .def_property_readonly("status", &Session::status)
        .def_property_readonly("is_active", &Session::isActive)
        .def_property_readonly("is_paused", &Session::isPaused);

    py::class_<Participant>(m, "Participant")
        .def_property_readonly("id", &Participant::id)
        .def_property_readonly("name", &Participant::name);

    py::class_<Metadata>(m, "Metadata")
        .def_property_readonly("user_name", &Metadata::userName)
        .def_property_readonly("user_id", &Metadata::userId);

    // Expose PyClient as the Client class
    py::class_<PyClient>(m, "Client")
        .def(py::init<>())
        .def_static("initialize", &PyClient::initialize, "Initialize the RTMS SDK with the specified CA certificate path")
        .def_static("uninitialize", &PyClient::uninitialize, "Uninitialize the RTMS SDK")
        .def("join", &PyClient::join, "Join a meeting with the specified parameters", 
            py::arg("uuid"), py::arg("stream_id"), py::arg("signature"), 
            py::arg("server_urls"), py::arg("timeout") = -1)
        .def("join", &PyClient::joinWithOptions, "Join a meeting with a dictionary of options")
        .def("poll", &PyClient::poll, "Poll for new events")
        .def("release", &PyClient::release, "Release resources")
        .def("uuid", &PyClient::uuid, "Get the UUID of the current meeting")
        .def("stream_id", &PyClient::streamId, "Get the stream ID of the current meeting")
        .def("enable_audio", &PyClient::enableAudio, "Enable or disable audio")
        .def("enable_video", &PyClient::enableVideo, "Enable or disable video")
        .def("enable_transcript", &PyClient::enableTranscript, "Enable or disable transcript")
        .def("enable_deskshare", &PyClient::enableDeskshare, "Enable or disable deskshare")
        .def("set_audio_params", &PyClient::setAudioParams, "Set audio parameters")
        .def("set_video_params", &PyClient::setVideoParams, "Set video parameters")
        .def("set_deskshare_params", &PyClient::setDeskshareParams, "Set deskshare parameters")

        // Decorator methods (exported with underscores for Python)
        .def("on_join_confirm", &PyClient::onJoinConfirm, "Get a decorator for join confirm events")
        .def("on_session_update", &PyClient::onSessionUpdate, "Get a decorator for session update events")
        .def("on_user_update", &PyClient::onUserUpdate, "Get a decorator for participant update events")
        .def("on_audio_data", &PyClient::onAudioData, "Get a decorator for receiving audio data")
        .def("on_video_data", &PyClient::onVideoData, "Get a decorator for receiving video data")
        .def("on_deskshare_data", &PyClient::onDeskshareData, "Get a decorator for receiving deskshare data")
        .def("on_transcript_data", &PyClient::onTranscriptData, "Get a decorator for receiving transcript data") 
        .def("on_leave", &PyClient::onLeave, "Get a decorator for leave events")
        // Direct callback setting methods (exported with underscores for Python)
        .def("set_join_confirm_callback", &PyClient::setJoinConfirmCallback)
        .def("set_session_update_callback", &PyClient::setSessionUpdateCallback)
        .def("set_user_update_callback", &PyClient::setUserUpdateCallback)
        .def("set_audio_data_callback", &PyClient::setAudioDataCallback)
        .def("set_video_data_callback", &PyClient::setVideoDataCallback)
        .def("set_deskshare_data_callback", &PyClient::setDeskshareDataCallback)
        .def("set_transcript_data_callback", &PyClient::setTranscriptDataCallback)
        .def("set_leave_callback", &PyClient::setLeaveCallback);

    // Define module functions that operate on the global client for backward compatibility
    m.def("_initialize", [](const string& ca_path) {
        try {
            globalClient.initialize(ca_path);
        } catch (const py::error_already_set& e) {
            DEBUG_LOG("Error in _initialize: " << e.what());
            throw;
        }
    }, "Initialize the RTMS SDK with the specified CA certificate path");
    
    m.def("_uninitialize", []() {
        try {
            globalClient.uninitialize();
        } catch (const py::error_already_set& e) {
            DEBUG_LOG("Error in _uninitialize: " << e.what());
            throw;
        }
    }, "Uninitialize the RTMS SDK");
    
    m.def("_join", [](const string& uuid, const string& streamId, const string& signature, 
                    const string& serverUrls, int timeout) {
        try {
            globalClient.join(uuid, streamId, signature, serverUrls, timeout);
        } catch (const py::error_already_set& e) {
            DEBUG_LOG("Error in global _join: " << e.what());
            throw;
        }
    }, "Join a meeting with the specified parameters", 
       py::arg("uuid"), py::arg("stream_id"), py::arg("signature"), 
       py::arg("server_urls"), py::arg("timeout") = -1);
    
    m.def("_poll", []() {
        globalClient.poll();
    }, "Poll for new events");
    
    m.def("_release", []() {
        try {
            globalClient.release();
        } catch (const py::error_already_set& e) {
            DEBUG_LOG("Error in global _release: " << e.what());
            throw;
        }
    }, "Release resources");
    
    m.def("_uuid", []() {
        try {
            return globalClient.uuid();
        } catch (const py::error_already_set& e) {
            DEBUG_LOG("Error in global _uuid: " << e.what());
            throw;
        }
    }, "Get the UUID of the current meeting");
    
    m.def("_stream_id", []() {
        try {
            return globalClient.streamId();
        } catch (const py::error_already_set& e) {
            DEBUG_LOG("Error in global _stream_id: " << e.what());
            throw;
        }
    }, "Get the stream ID of the current meeting");

    m.def("_enable_audio", [](bool enable) {
        try {
            globalClient.enableAudio(enable);
        } catch (const py::error_already_set& e) {
            DEBUG_LOG("Error in global _enable_audio: " << e.what());
            throw;
        }
    }, "Enable or disable audio");

    m.def("_enable_video", [](bool enable) {
        try {
            globalClient.enableVideo(enable);
        } catch (const py::error_already_set& e) {
            DEBUG_LOG("Error in global _enable_video: " << e.what());
            throw;
        }
    }, "Enable or disable video");

    m.def("_enable_transcript", [](bool enable) {
        try {
            globalClient.enableTranscript(enable);
        } catch (const py::error_already_set& e) {
            DEBUG_LOG("Error in global _enable_transcript: " << e.what());
            throw;
        }
    }, "Enable or disable transcript");

    m.def("_enable_deskshare", [](bool enable) {
        try {
            globalClient.enableDeskshare(enable);
        } catch (const py::error_already_set& e) {
            DEBUG_LOG("Error in global _enable_deskshare: " << e.what());
            throw;
        }
    }, "Enable or disable deskshare");
    
    // Register callback decorators for the global client
    m.def("on_join_confirm", [](py::function callback) {
        globalClient.setJoinConfirmCallback(callback);
        return callback;
    }, "Set callback for join confirmation events");
    
    m.def("on_session_update", [](py::function callback) {
        globalClient.setSessionUpdateCallback(callback);
        return callback;
    }, "Set callback for session update events");
    
    m.def("on_user_update", [](py::function callback) {
        globalClient.setUserUpdateCallback(callback);
        return callback;
    }, "Set callback for participant update events");
    
    m.def("on_audio_data", [](py::function callback) {
        globalClient.setAudioDataCallback(callback);
        return callback;
    }, "Set callback for receiving audio data");
    
    m.def("on_video_data", [](py::function callback) {
        globalClient.setVideoDataCallback(callback);
        return callback;
    }, "Set callback for receiving video data");

    m.def("on_deskshare_data", [](py::function callback) {
        globalClient.setDeskshareDataCallback(callback);
        return callback;
    }, "Set callback for receiving deskshare data");
    
    m.def("on_transcript_data", [](py::function callback) {
        globalClient.setTranscriptDataCallback(callback);
        return callback;
    }, "Set callback for receiving transcript data");
    
    m.def("on_leave", [](py::function callback) {
        globalClient.setLeaveCallback(callback);
        return callback;
    }, "Set callback for leave events");

    m.def("set_audio_params", [](py::dict params) {
        try {
            globalClient.setAudioParams(params);
        } catch (const py::error_already_set& e) {
            DEBUG_LOG("Error in global set_audio_params: " << e.what());
            throw;
        }
    }, "Set global audio parameters");

    m.def("set_video_params", [](py::dict params) {
        try {
            globalClient.setVideoParams(params);
        } catch (const py::error_already_set& e) {
            DEBUG_LOG("Error in global set_video_params: " << e.what());
            throw;
        }
    }, "Set global video parameters");

    m.def("set_deskshare_params", [](py::dict params) {
        try {
            globalClient.setDeskshareParams(params);
        } catch (const py::error_already_set& e) {
            DEBUG_LOG("Error in global set_deskshare_params: " << e.what());
            throw;
        }
    }, "Set global deskshare parameters");

    // Expose SDK constants
    m.attr("SDK_AUDIO") = py::int_(static_cast<int>(SDK_AUDIO));
    m.attr("SDK_VIDEO") = py::int_(static_cast<int>(SDK_VIDEO));
    m.attr("SDK_DESKSHARE") = py::int_(static_cast<int>(SDK_DESKSHARE));
    m.attr("SDK_TRANSCRIPT") = py::int_(static_cast<int>(SDK_TRANSCRIPT));
    m.attr("SDK_ALL") = py::int_(static_cast<int>(SDK_ALL));
    
    m.attr("SESSION_ADD") = py::int_(static_cast<int>(SESSION_ADD));
    m.attr("SESSION_STOP") = py::int_(static_cast<int>(SESSION_STOP));
    m.attr("SESSION_PAUSE") = py::int_(static_cast<int>(SESSION_PAUSE));
    m.attr("SESSION_RESUME") = py::int_(static_cast<int>(SESSION_RESUME));
    
    m.attr("USER_JOIN") = py::int_(static_cast<int>(USER_JOIN));
    m.attr("USER_LEAVE") = py::int_(static_cast<int>(USER_LEAVE));
}