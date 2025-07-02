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
    int configured_media_types_;
    bool is_configured_;
    bool is_shutting_down_;  // NEW: Track shutdown state
    
    py::function join_confirm_callback;
    py::function session_update_callback;
    py::function user_update_callback;
    py::function audio_data_callback;
    py::function video_data_callback;
    py::function deskshare_data_callback;
    py::function transcript_data_callback;
    py::function leave_callback;

    void reconfigure_media_types() {
        if (!is_configured_) return;
        
        try {
            MediaParams params;
            client_->configure(params, configured_media_types_, false);
        } catch (const Exception& e) {
            DEBUG_LOG("Failed to reconfigure media types: " << e.what() << " (code: " << e.code() << ")");
            // Don't throw here since this is called from callback setters
        } catch (const exception& e) {
            DEBUG_LOG("C++ exception reconfiguring media types: " << e.what());
            // Don't throw here since this is called from callback setters
        } catch (...) {
            DEBUG_LOG("Unknown exception in reconfigure_media_types");
            // Don't throw 
    }
    }

public:
    PyClient() : configured_media_types_(0), is_configured_(false), is_shutting_down_(false) {
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

    void join(const string& uuid, const string& stream_id, const string& signature, 
              const string& server_urls, int timeout = -1) {
        try {
            client_->join(uuid, stream_id, signature, server_urls, timeout);
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

    void join_with_options(py::dict options) {
        try {
            string uuid = options["meeting_uuid"].cast<string>();
            string stream_id = options["rtms_stream_id"].cast<string>();
            string server_urls = options["server_urls"].cast<string>();

            string signature = options.contains("signature") ? options["signature"].cast<string>() : "";
            string client = options.contains("client") ? options["client"].cast<string>() : "";
            string secret = options.contains("secret") ? options["secret"].cast<string>() : "";
            int timeout = options.contains("timeout") ? options["timeout"].cast<int>() : -1;
            
            join(uuid, stream_id, signature, server_urls, timeout);
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
        is_shutting_down_ = true;  // Mark as shutting down
        
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

    string stream_id() {
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

    void enable_audio(bool enable) {
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

    void enable_video(bool enable) {
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

    void enable_transcript(bool enable) {
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

    void enable_deskshare(bool enable) {
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
    py::function on_join_confirm() {
        // Create a decorator function that will capture 'this'
        return py::cpp_function([this](py::function func) {
            this->set_join_confirm_callback(func);
            return func;
        });
    }
    
    py::function on_session_update() {
        return py::cpp_function([this](py::function func) {
            this->set_session_update_callback(func);
            return func;
        });
    }
    
    py::function on_user_update() {
        return py::cpp_function([this](py::function func) {
            this->set_user_update_callback(func);
            return func;
        });
    }
    
    py::function on_audio_data() {
        return py::cpp_function([this](py::function func) {
            this->set_audio_data_callback(func);
            return func;
        });
    }
    
    py::function on_video_data() {
        return py::cpp_function([this](py::function func) {
            this->set_video_data_callback(func);
            return func;
        });
    }
    
    py::function on_deskshare_data() {
        return py::cpp_function([this](py::function func) {
            this->set_deskshare_data_callback(func);
            return func;
        });
    }
    
    py::function on_transcript_data() {
        return py::cpp_function([this](py::function func) {
            this->set_transcript_data_callback(func);
            return func;
        });
    }
    
    py::function on_leave() {
        return py::cpp_function([this](py::function func) {
            this->set_leave_callback(func);
            return func;
        });
    }
    
        // Enhanced callback validation helper
    bool is_callback_valid(const py::function& callback) {
        if (is_shutting_down_) {
            return false;  // Don't call callbacks during shutdown
        }
        
        try {
            return !callback.is_none() && PyCallable_Check(callback.ptr());
        } catch (...) {
            return false;  // If any exception occurs during validation, consider invalid
        }
    }

    // Enhanced callback setters with improved error handling
    void set_join_confirm_callback(py::function callback) {
        join_confirm_callback = callback;
        
        client_->setOnJoinConfirm([this](int reason) {
            py::gil_scoped_acquire acquire;
            try {
                if (is_callback_valid(join_confirm_callback)) {
                    join_confirm_callback(reason);
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
    
    void set_session_update_callback(py::function callback) {
        session_update_callback = callback;
        
        client_->setOnSessionUpdate([this](int op, const Session& session) {
            py::gil_scoped_acquire acquire;
            try {
                if (is_callback_valid(session_update_callback)) {
                    session_update_callback(op, session);
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
    
    void set_user_update_callback(py::function callback) {
        user_update_callback = callback;
        
        client_->setOnUserUpdate([this](int op, const Participant& participant) {
            py::gil_scoped_acquire acquire;
            try {
                if (is_callback_valid(user_update_callback)) {
                    user_update_callback(op, participant);
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
    
    void set_audio_data_callback(py::function callback) {
        audio_data_callback = callback;
        
        client_->setOnAudioData([this](const vector<uint8_t>& data, uint64_t timestamp, const Metadata& metadata) {
            py::gil_scoped_acquire acquire;
            try {
                if (is_callback_valid(audio_data_callback)) {
                    py::bytes py_data(reinterpret_cast<const char*>(data.data()), data.size());
                    audio_data_callback(py_data, data.size(), timestamp, metadata);
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
        
        configured_media_types_ |= SDK_AUDIO;
        reconfigure_media_types();
    }
    
    void set_video_data_callback(py::function callback) {
        video_data_callback = callback;
        
        client_->setOnVideoData([this](const vector<uint8_t>& data, uint64_t timestamp, const Metadata& metadata) {
            py::gil_scoped_acquire acquire;
            try {
                if (is_callback_valid(video_data_callback)) {
                    py::bytes py_data(reinterpret_cast<const char*>(data.data()), data.size());
                    video_data_callback(py_data, data.size(), timestamp, metadata);
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
        
        configured_media_types_ |= SDK_VIDEO;
        reconfigure_media_types();
    }

    void set_deskshare_data_callback(py::function callback) {
        deskshare_data_callback = callback;
        
        client_->setOnDeskshareData([this](const vector<uint8_t>& data, uint64_t timestamp, const Metadata& metadata) {
            py::gil_scoped_acquire acquire;
            try {
                if (is_callback_valid(deskshare_data_callback)) {
                    py::bytes py_data(reinterpret_cast<const char*>(data.data()), data.size());
                    deskshare_data_callback(py_data, data.size(), timestamp, metadata);
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
        
        configured_media_types_ |= SDK_DESKSHARE;
        reconfigure_media_types();
    }
    
    void set_transcript_data_callback(py::function callback) {
        transcript_data_callback = callback;
        
        client_->setOnTranscriptData([this](const vector<uint8_t>& data, uint64_t timestamp, const Metadata& metadata) {
            py::gil_scoped_acquire acquire;
            try {
                if (is_callback_valid(transcript_data_callback)) {
                    py::bytes py_data(reinterpret_cast<const char*>(data.data()), data.size());
                    transcript_data_callback(py_data, data.size(), timestamp, metadata);
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
        
        configured_media_types_ |= SDK_TRANSCRIPT;
        reconfigure_media_types();
    }
    
    void set_leave_callback(py::function callback) {
        leave_callback = callback;
        
        client_->setOnLeave([this](int reason) {
            py::gil_scoped_acquire acquire;
            try {
                if (is_callback_valid(leave_callback)) {
                    leave_callback(reason);
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

    void set_audio_params(py::dict params) {
        try {
            AudioParams audio_params;
            
            if (params.contains("contentType"))
                audio_params.setContentType(params["contentType"].cast<int>());
            if (params.contains("codec"))
                audio_params.setCodec(params["codec"].cast<int>());
            if (params.contains("sampleRate"))
                audio_params.setSampleRate(params["sampleRate"].cast<int>());
            if (params.contains("channel"))
                audio_params.setChannel(params["channel"].cast<int>());
            if (params.contains("dataOpt"))
                audio_params.setDataOpt(params["dataOpt"].cast<int>());
            if (params.contains("duration"))
                audio_params.setDuration(params["duration"].cast<int>());
            if (params.contains("frameSize"))
                audio_params.setFrameSize(params["frameSize"].cast<int>());
            
            client_->setAudioParams(audio_params);
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

    void set_video_params(py::dict params) {
        try {
            VideoParams video_params;
            
            if (params.contains("contentType"))
                video_params.setContentType(params["contentType"].cast<int>());
            if (params.contains("codec"))
                video_params.setCodec(params["codec"].cast<int>());
            if (params.contains("resolution"))
                video_params.setResolution(params["resolution"].cast<int>());
            if (params.contains("dataOpt"))
                video_params.setDataOpt(params["dataOpt"].cast<int>());
            if (params.contains("fps"))
                video_params.setFps(params["fps"].cast<int>());
            
            client_->setVideoParams(video_params);
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

    void set_deskshare_params(py::dict params) {
        try {
            DeskshareParams deskshare_params;
            
            if (params.contains("contentType"))
                deskshare_params.setContentType(params["contentType"].cast<int>());
            if (params.contains("codec"))
                deskshare_params.setCodec(params["codec"].cast<int>());
            if (params.contains("resolution"))
                deskshare_params.setResolution(params["resolution"].cast<int>());
            if (params.contains("fps"))
                deskshare_params.setFps(params["fps"].cast<int>());
            
            client_->setDeskshareParams(deskshare_params);
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
static PyClient global_client;

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
        .def("join", &PyClient::join_with_options, "Join a meeting with a dictionary of options")
        .def("poll", &PyClient::poll, "Poll for new events")
        .def("release", &PyClient::release, "Release resources")
        .def("uuid", &PyClient::uuid, "Get the UUID of the current meeting")
        .def("stream_id", &PyClient::stream_id, "Get the stream ID of the current meeting")
        .def("enable_audio", &PyClient::enable_audio, "Enable or disable audio")
        .def("enable_video", &PyClient::enable_video, "Enable or disable video")
        .def("enable_transcript", &PyClient::enable_transcript, "Enable or disable transcript")
        .def("enable_deskshare", &PyClient::enable_deskshare, "Enable or disable deskshare")
        .def("set_audio_params", &PyClient::set_audio_params, "Set audio parameters")
        .def("set_video_params", &PyClient::set_video_params, "Set video parameters")
        .def("set_deskshare_params", &PyClient::set_deskshare_params, "Set deskshare parameters")

        // Decorator methods
        .def("on_join_confirm", &PyClient::on_join_confirm, "Get a decorator for join confirm events")
        .def("on_session_update", &PyClient::on_session_update, "Get a decorator for session update events")
        .def("on_user_update", &PyClient::on_user_update, "Get a decorator for participant update events")
        .def("on_audio_data", &PyClient::on_audio_data, "Get a decorator for receiving audio data")
        .def("on_video_data", &PyClient::on_video_data, "Get a decorator for receiving video data")
        .def("on_deskshare_data", &PyClient::on_deskshare_data, "Get a decorator for receiving deskshare data")
        .def("on_transcript_data", &PyClient::on_transcript_data, "Get a decorator for receiving transcript data") 
        .def("on_leave", &PyClient::on_leave, "Get a decorator for leave events")
        // Direct callback setting methods
        .def("set_join_confirm_callback", &PyClient::set_join_confirm_callback)
        .def("set_session_update_callback", &PyClient::set_session_update_callback)
        .def("set_user_update_callback", &PyClient::set_user_update_callback)
        .def("set_audio_data_callback", &PyClient::set_audio_data_callback)
        .def("set_video_data_callback", &PyClient::set_video_data_callback)
        .def("set_deskshare_data_callback", &PyClient::set_deskshare_data_callback)
        .def("set_transcript_data_callback", &PyClient::set_transcript_data_callback)
        .def("set_leave_callback", &PyClient::set_leave_callback);

    // Define module functions that operate on the global client for backward compatibility
    m.def("_initialize", [](const string& ca_path) {
        try {
            global_client.initialize(ca_path);
        } catch (const py::error_already_set& e) {
            DEBUG_LOG("Error in _initialize: " << e.what());
            throw;
        }
    }, "Initialize the RTMS SDK with the specified CA certificate path");
    
    m.def("_uninitialize", []() {
        try {
            global_client.uninitialize();
        } catch (const py::error_already_set& e) {
            DEBUG_LOG("Error in _uninitialize: " << e.what());
            throw;
        }
    }, "Uninitialize the RTMS SDK");
    
    m.def("_join", [](const string& uuid, const string& stream_id, const string& signature, 
                    const string& server_urls, int timeout) {
        try {
            global_client.join(uuid, stream_id, signature, server_urls, timeout);
        } catch (const py::error_already_set& e) {
            DEBUG_LOG("Error in global _join: " << e.what());
            throw;
        }
    }, "Join a meeting with the specified parameters", 
       py::arg("uuid"), py::arg("stream_id"), py::arg("signature"), 
       py::arg("server_urls"), py::arg("timeout") = -1);
    
    m.def("_poll", []() {
        global_client.poll();
    }, "Poll for new events");
    
    m.def("_release", []() {
        try {
            global_client.release();
        } catch (const py::error_already_set& e) {
            DEBUG_LOG("Error in global _release: " << e.what());
            throw;
        }
    }, "Release resources");
    
    m.def("_uuid", []() {
        try {
            return global_client.uuid();
        } catch (const py::error_already_set& e) {
            DEBUG_LOG("Error in global _uuid: " << e.what());
            throw;
        }
    }, "Get the UUID of the current meeting");
    
    m.def("_stream_id", []() {
        try {
            return global_client.stream_id();
        } catch (const py::error_already_set& e) {
            DEBUG_LOG("Error in global _stream_id: " << e.what());
            throw;
        }
    }, "Get the stream ID of the current meeting");

    m.def("_enable_audio", [](bool enable) {
        try {
            global_client.enable_audio(enable);
        } catch (const py::error_already_set& e) {
            DEBUG_LOG("Error in global _enable_audio: " << e.what());
            throw;
        }
    }, "Enable or disable audio");

    m.def("_enable_video", [](bool enable) {
        try {
            global_client.enable_video(enable);
        } catch (const py::error_already_set& e) {
            DEBUG_LOG("Error in global _enable_video: " << e.what());
            throw;
        }
    }, "Enable or disable video");

    m.def("_enable_transcript", [](bool enable) {
        try {
            global_client.enable_transcript(enable);
        } catch (const py::error_already_set& e) {
            DEBUG_LOG("Error in global _enable_transcript: " << e.what());
            throw;
        }
    }, "Enable or disable transcript");

    m.def("_enable_deskshare", [](bool enable) {
        try {
            global_client.enable_deskshare(enable);
        } catch (const py::error_already_set& e) {
            DEBUG_LOG("Error in global _enable_deskshare: " << e.what());
            throw;
        }
    }, "Enable or disable deskshare");
    
    // Register callback decorators for the global client
    m.def("on_join_confirm", [](py::function callback) {
        global_client.set_join_confirm_callback(callback);
        return callback;
    }, "Set callback for join confirmation events");
    
    m.def("on_session_update", [](py::function callback) {
        global_client.set_session_update_callback(callback);
        return callback;
    }, "Set callback for session update events");
    
    m.def("on_user_update", [](py::function callback) {
        global_client.set_user_update_callback(callback);
        return callback;
    }, "Set callback for participant update events");
    
    m.def("on_audio_data", [](py::function callback) {
        global_client.set_audio_data_callback(callback);
        return callback;
    }, "Set callback for receiving audio data");
    
    m.def("on_video_data", [](py::function callback) {
        global_client.set_video_data_callback(callback);
        return callback;
    }, "Set callback for receiving video data");

    m.def("on_deskshare_data", [](py::function callback) {
        global_client.set_deskshare_data_callback(callback);
        return callback;
    }, "Set callback for receiving deskshare data");
    
    m.def("on_transcript_data", [](py::function callback) {
        global_client.set_transcript_data_callback(callback);
        return callback;
    }, "Set callback for receiving transcript data");
    
    m.def("on_leave", [](py::function callback) {
        global_client.set_leave_callback(callback);
        return callback;
    }, "Set callback for leave events");

    m.def("set_audio_params", [](py::dict params) {
        try {
            global_client.set_audio_params(params);
        } catch (const py::error_already_set& e) {
            DEBUG_LOG("Error in global set_audio_params: " << e.what());
            throw;
        }
    }, "Set global audio parameters");

    m.def("set_video_params", [](py::dict params) {
        try {
            global_client.set_video_params(params);
        } catch (const py::error_already_set& e) {
            DEBUG_LOG("Error in global set_video_params: " << e.what());
            throw;
        }
    }, "Set global video parameters");

    m.def("set_deskshare_params", [](py::dict params) {
        try {
            global_client.set_deskshare_params(params);
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