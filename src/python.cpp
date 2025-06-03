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

// Python-friendly wrapper class for RTMS Client
class PyClient {
public:
    PyClient() : configured_media_types_(0), is_configured_(false) {
        try {
            client_ = make_unique<Client>();
        } catch (const exception& e) {
            DEBUG_LOG("Error creating client instance: " << e.what());
            throw py::error_already_set();
        }
    }
    
    ~PyClient() {
        // First clear all callbacks to avoid issues during destruction
        try {
            // Reset all callbacks to empty functions to avoid calling Python
            // during destruction when Python might be shutting down
            if (client_) {
                client_->setOnJoinConfirm([](int) {});
                client_->setOnSessionUpdate([](int, const Session&) {});
                client_->setOnUserUpdate([](int, const Participant&) {});
                client_->setOnAudioData([](const vector<uint8_t>&, uint32_t, const Metadata&) {});
                client_->setOnVideoData([](const vector<uint8_t>&, uint32_t, const Metadata&) {});
                client_->setOnTranscriptData([](const vector<uint8_t>&, uint32_t, const Metadata&) {});
                client_->setOnLeave([](int) {});
            }
        } catch (const Exception& e) {
            DEBUG_LOG("Error during callback cleanup: " << e.what() << " (error code: " << e.code() << ")");
        } catch (const exception& e) {
            DEBUG_LOG("C++ exception during callback cleanup: " << e.what());
        } catch (...) {
            DEBUG_LOG("Unknown exception during callback cleanup");
        }
        
        // Release any Python references
        join_confirm_callback = py::none();
        session_update_callback = py::none();
        user_update_callback = py::none();
        audio_data_callback = py::none();
        video_data_callback = py::none();
        transcript_data_callback = py::none();
        leave_callback = py::none();
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
             const string& server_url, int timeout = -1) {
        try {
            // Configure if not already configured
            if (!is_configured_ && configured_media_types_ > 0) {
                DEBUG_LOG("Configuring client with media types: " << configured_media_types_);
                
                MediaParams params;
                
                if (configured_media_types_ & static_cast<int>(SDK_AUDIO)) {
                    DEBUG_LOG("Setting up full audio parameters");
                    AudioParams audio;

                    audio.setContentType(0);    // Default content type
                    audio.setCodec(0);          // Default codec
                    audio.setSampleRate(16000); // 16kHz is common for audio
                    audio.setChannel(1);        // Mono
                    audio.setDataOpt(0);        // Default data option
                    audio.setDuration(20);      // 20ms frame duration
                    audio.setFrameSize(320);    // 16000Hz * 20ms = 320 samples
                    params.setAudioParams(audio);
                }
                
                if (configured_media_types_ & static_cast<int>(SDK_VIDEO)) {
                    DEBUG_LOG("Setting up full video parameters");
                    VideoParams video;

                    video.setContentType(0);    // Default content type
                    video.setCodec(0);          // Default codec
                    video.setResolution(0);     // Default resolution
                    video.setDataOpt(0);        // Default data option
                    video.setFps(30);           // 30 frames per second
                    params.setVideoParams(video);
                }
                
                int media_types = configured_media_types_ > 0 ? configured_media_types_ : static_cast<int>(SDK_ALL);
                
                DEBUG_LOG("Calling configure with media types: " << media_types);
                client_->configure(params, configured_media_types_, false);
                is_configured_ = true;
            }
            
            client_->join(uuid, stream_id, signature, server_url, timeout);
        } catch (const Exception& e) {
            DEBUG_LOG("Error joining RTMS session: " << e.what() << " (code: " << e.code() << ")");
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        } catch (const exception& e) {
            DEBUG_LOG("Unknown error during join: " << e.what());
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        }
    }

    // Support joining with a dictionary/kwargs
    void join_with_options(py::dict options) {
        try {
            string uuid = options["uuid"].cast<string>();
            string stream_id = options["stream_id"].cast<string>();
            string server_urls = options["server_urls"].cast<string>();
            
            // Get optional parameters with defaults
            string signature = options.contains("signature") ? options["signature"].cast<string>() : "";
            string client = options.contains("client") ? options["client"].cast<string>() : "";
            string secret = options.contains("secret") ? options["secret"].cast<string>() : "";
            int timeout = options.contains("timeout") ? options["timeout"].cast<int>() : -1;
            
            join(uuid, stream_id, signature, server_urls, timeout);
        } catch (const py::error_already_set& e) {
            // Just rethrow Python errors
            throw;
        } catch (const Exception& e) {
            DEBUG_LOG("RTMS error in join_with_options: " << e.what() << " (code: " << e.code() << ")");
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        } catch (const std::exception& e) {
            DEBUG_LOG("C++ exception in join_with_options: " << e.what());
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        }
    }

    void poll() {
        try {
            // Don't use gil_scoped_release in poll - this is dangerous if
            // called from a background thread that didn't properly acquire GIL
            client_->poll();
        } catch (const Exception& e) {
            DEBUG_LOG("Error during polling: " << e.what() << " (code: " << e.code() << ")");
            // Don't throw exceptions from poll - this could crash if called from a background thread
            return;
        } catch (const exception& e) {
            DEBUG_LOG("C++ exception during polling: " << e.what());
            return;
        } catch (...) {
            DEBUG_LOG("Unknown exception during polling");
            return;
        }
    }

    void release() {
        try {
            client_->release();
        } catch (const Exception& e) {
            DEBUG_LOG("Error releasing RTMS client: " << e.what() << " (code: " << e.code() << ")");
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        } catch (const exception& e) {
            DEBUG_LOG("Unknown error during release: " << e.what());
            PyErr_SetString(PyExc_RuntimeError, e.what());
            throw py::error_already_set();
        }
    }

    string uuid() const {
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

    string stream_id() const {
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
    
    // Direct callback setting methods
    void set_join_confirm_callback(py::function callback) {
        // Store Python callback
        join_confirm_callback = callback;
        
        client_->setOnJoinConfirm([this](int reason) {
            py::gil_scoped_acquire acquire;
            try {
                if (!join_confirm_callback.is_none() && PyCallable_Check(join_confirm_callback.ptr())) {
                    join_confirm_callback(reason);
                }
            } catch (py::error_already_set& e) {
                DEBUG_LOG("Python exception in join confirm callback: " << e.what());
                e.restore();  // This ensures the Python exception is properly restored
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
                if (!session_update_callback.is_none() && PyCallable_Check(session_update_callback.ptr())) {
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
                if (!user_update_callback.is_none() && PyCallable_Check(user_update_callback.ptr())) {
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
        
        client_->setOnAudioData([this](const vector<uint8_t>& data, uint32_t timestamp, const Metadata& metadata) {
            py::gil_scoped_acquire acquire;
            try {
                if (!audio_data_callback.is_none() && PyCallable_Check(audio_data_callback.ptr())) {
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
        
        // Enable audio media type
        configured_media_types_ |= static_cast<int>(SDK_AUDIO);
        reconfigure_media_types();
    }

    void set_video_data_callback(py::function callback) {
        video_data_callback = callback;
        
        client_->setOnVideoData([this](const vector<uint8_t>& data, uint32_t timestamp, const Metadata& metadata) {
            py::gil_scoped_acquire acquire;
            try {
                if (!video_data_callback.is_none() && PyCallable_Check(video_data_callback.ptr())) {
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
        
        // Enable video media type
        configured_media_types_ |= static_cast<int>(SDK_VIDEO);
        reconfigure_media_types();
    }

    void set_transcript_data_callback(py::function callback) {
        transcript_data_callback = callback;
        
        client_->setOnTranscriptData([this](const vector<uint8_t>& data, uint32_t timestamp, const Metadata& metadata) {
            py::gil_scoped_acquire acquire;
            try {
                if (!transcript_data_callback.is_none() && PyCallable_Check(transcript_data_callback.ptr())) {
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
        
        // Enable transcript media type
        configured_media_types_ |= static_cast<int>(SDK_TRANSCRIPT);
        reconfigure_media_types();
    }

    void set_leave_callback(py::function callback) {
        leave_callback = callback;
        
        client_->setOnLeave([this](int reason) {
            py::gil_scoped_acquire acquire;
            try {
                if (!leave_callback.is_none() && PyCallable_Check(leave_callback.ptr())) {
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

    void set_audio_parameters(py::dict params) {
        try {
            AudioParameters audio_params;
            
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
            
            client_->setAudioParameters(audio_params);
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

    void set_video_parameters(py::dict params) {
        try {
            VideoParameters video_params;
            
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
            
            client_->setVideoParameters(video_params);
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

private:
    unique_ptr<Client> client_;
    int configured_media_types_;
    bool is_configured_;
    
    py::function join_confirm_callback;
    py::function session_update_callback;
    py::function user_update_callback;
    py::function audio_data_callback;
    py::function video_data_callback;
    py::function transcript_data_callback;
    py::function leave_callback;

    void reconfigure_media_types() {
        if (!is_configured_) return;
        
        try {
            MediaParameters params;
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
        .def("set_audio_parameters", &PyClient::set_audio_parameters, "Set audio parameters")
        .def("set_video_parameters", &PyClient::set_video_parameters, "Set video parameters")

        // Decorator methods
        .def("on_join_confirm", &PyClient::on_join_confirm, "Get a decorator for join confirm events")
        .def("on_session_update", &PyClient::on_session_update, "Get a decorator for session update events")
        .def("on_user_update", &PyClient::on_user_update, "Get a decorator for participant update events")
        .def("on_audio_data", &PyClient::on_audio_data, "Get a decorator for receiving audio data")
        .def("on_video_data", &PyClient::on_video_data, "Get a decorator for receiving video data")
        .def("on_transcript_data", &PyClient::on_transcript_data, "Get a decorator for receiving transcript data") 
        .def("on_leave", &PyClient::on_leave, "Get a decorator for leave events")
        // Direct callback setting methods
        .def("set_join_confirm_callback", &PyClient::set_join_confirm_callback)
        .def("set_session_update_callback", &PyClient::set_session_update_callback)
        .def("set_user_update_callback", &PyClient::set_user_update_callback)
        .def("set_audio_data_callback", &PyClient::set_audio_data_callback)
        .def("set_video_data_callback", &PyClient::set_video_data_callback)
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
    
    m.def("on_transcript_data", [](py::function callback) {
        global_client.set_transcript_data_callback(callback);
        return callback;
    }, "Set callback for receiving transcript data");
    
    m.def("on_leave", [](py::function callback) {
        global_client.set_leave_callback(callback);
        return callback;
    }, "Set callback for leave events");

    m.def("set_audio_parameters", [](py::dict params) {
        try {
            global_client.set_audio_parameters(params);
        } catch (const py::error_already_set& e) {
            DEBUG_LOG("Error in global set_audio_parameters: " << e.what());
            throw;
        }
    }, "Set global audio parameters");

    m.def("set_video_parameters", [](py::dict params) {
        try {
            global_client.set_video_parameters(params);
        } catch (const py::error_already_set& e) {
            DEBUG_LOG("Error in global set_video_parameters: " << e.what());
            throw;
        }
    }, "Set global video parameters");

        // Expose SDK constants
        m.attr("SDK_AUDIO") = py::int_(static_cast<int>(SDK_AUDIO));
        m.attr("SDK_VIDEO") = py::int_(static_cast<int>(SDK_VIDEO));
        m.attr("SDK_TRANSCRIPT") = py::int_(static_cast<int>(SDK_TRANSCRIPT));
        m.attr("SDK_ALL") = py::int_(static_cast<int>(SDK_ALL));
        
        m.attr("SESSION_ADD") = py::int_(static_cast<int>(SESSION_ADD));
        m.attr("SESSION_STOP") = py::int_(static_cast<int>(SESSION_STOP));
        m.attr("SESSION_PAUSE") = py::int_(static_cast<int>(SESSION_PAUSE));
        m.attr("SESSION_RESUME") = py::int_(static_cast<int>(SESSION_RESUME));
        
        m.attr("USER_JOIN") = py::int_(static_cast<int>(USER_JOIN));
        m.attr("USER_LEAVE") = py::int_(static_cast<int>(USER_LEAVE));
        
        m.attr("RTMS_SDK_FAILURE") = py::int_(static_cast<int>(RTMS_SDK_FAILURE));
        m.attr("RTMS_SDK_OK") = py::int_(static_cast<int>(RTMS_SDK_OK));
        m.attr("RTMS_SDK_TIMEOUT") = py::int_(static_cast<int>(RTMS_SDK_TIMEOUT));
        m.attr("RTMS_SDK_NOT_EXIST") = py::int_(static_cast<int>(RTMS_SDK_NOT_EXIST));
        m.attr("RTMS_SDK_WRONG_TYPE") = py::int_(static_cast<int>(RTMS_SDK_WRONG_TYPE));
        m.attr("RTMS_SDK_INVALID_STATUS") = py::int_(static_cast<int>(RTMS_SDK_INVALID_STATUS));
        m.attr("RTMS_SDK_INVALID_ARGS") = py::int_(static_cast<int>(RTMS_SDK_INVALID_ARGS));
        
        m.attr("SESS_STATUS_ACTIVE") = py::int_(static_cast<int>(SESS_STATUS_ACTIVE));
        m.attr("SESS_STATUS_PAUSED") = py::int_(static_cast<int>(SESS_STATUS_PAUSED));
    
        // ===== Audio Parameter Constants =====
        
        // Audio Content Type
        py::dict audioContentType = py::dict();
        audioContentType["UNDEFINED"] = py::int_(0);
        audioContentType["RTP"] = py::int_(1);
        audioContentType["RAW_AUDIO"] = py::int_(2);
        audioContentType["FILE_STREAM"] = py::int_(4);
        audioContentType["TEXT"] = py::int_(5);
        m.attr("AudioContentType") = audioContentType;
        
        // Audio Codec
        py::dict audioCodec = py::dict();
        audioCodec["UNDEFINED"] = py::int_(0);
        audioCodec["L16"] = py::int_(1);
        audioCodec["G711"] = py::int_(2);
        audioCodec["G722"] = py::int_(3);
        audioCodec["OPUS"] = py::int_(4);
        m.attr("AudioCodec") = audioCodec;
        
        // Audio Sample Rate
        py::dict audioSampleRate = py::dict();
        audioSampleRate["SR_8K"] = py::int_(0);
        audioSampleRate["SR_16K"] = py::int_(1);
        audioSampleRate["SR_32K"] = py::int_(2);
        audioSampleRate["SR_48K"] = py::int_(3);
        m.attr("AudioSampleRate") = audioSampleRate;
        
        // Audio Channel
        py::dict audioChannel = py::dict();
        audioChannel["MONO"] = py::int_(1);
        audioChannel["STEREO"] = py::int_(2);
        m.attr("AudioChannel") = audioChannel;
        
        // Audio Data Option
        py::dict audioDataOption = py::dict();
        audioDataOption["UNDEFINED"] = py::int_(0);
        audioDataOption["AUDIO_MIXED_STREAM"] = py::int_(1);
        audioDataOption["AUDIO_MULTI_STREAMS"] = py::int_(2);
        m.attr("AudioDataOption") = audioDataOption;
        
        // ===== Video Parameter Constants =====
        
        // Video Content Type
        py::dict videoContentType = py::dict();
        videoContentType["UNDEFINED"] = py::int_(0);
        videoContentType["RTP"] = py::int_(1);
        videoContentType["RAW_VIDEO"] = py::int_(3);
        videoContentType["FILE_STREAM"] = py::int_(4);
        videoContentType["TEXT"] = py::int_(5);
        m.attr("VideoContentType") = videoContentType;
        
        // Video Codec
        py::dict videoCodec = py::dict();
        videoCodec["UNDEFINED"] = py::int_(0);
        videoCodec["JPG"] = py::int_(5);
        videoCodec["PNG"] = py::int_(6);
        videoCodec["H264"] = py::int_(7);
        m.attr("VideoCodec") = videoCodec;
        
        // Video Resolution
        py::dict videoResolution = py::dict();
        videoResolution["SD"] = py::int_(1);
        videoResolution["HD"] = py::int_(2);
        videoResolution["FHD"] = py::int_(3);
        videoResolution["QHD"] = py::int_(4);
        m.attr("VideoResolution") = videoResolution;
        
        // Video Data Option
        py::dict videoDataOption = py::dict();
        videoDataOption["UNDEFINED"] = py::int_(0);
        videoDataOption["VIDEO_SINGLE_ACTIVE_STREAM"] = py::int_(3);
        videoDataOption["VIDEO_MIXED_SPEAKER_VIEW"] = py::int_(4);
        videoDataOption["VIDEO_MIXED_GALLERY_VIEW"] = py::int_(5);
        m.attr("VideoDataOption") = videoDataOption;
        
        // ===== Other Constant Dictionaries =====
        
        // Media Data Type
        py::dict mediaDataType = py::dict();
        mediaDataType["UNDEFINED"] = py::int_(0);
        mediaDataType["AUDIO"] = py::int_(1);
        mediaDataType["VIDEO"] = py::int_(2);
        mediaDataType["DESKSHARE"] = py::int_(4);
        mediaDataType["TRANSCRIPT"] = py::int_(8);
        mediaDataType["CHAT"] = py::int_(16);
        mediaDataType["ALL"] = py::int_(31);
        m.attr("MediaDataType") = mediaDataType;
        
        // Session State
        py::dict sessionState = py::dict();
        sessionState["INACTIVE"] = py::int_(0);
        sessionState["INITIALIZE"] = py::int_(1);
        sessionState["STARTED"] = py::int_(2);
        sessionState["PAUSED"] = py::int_(3);
        sessionState["RESUMED"] = py::int_(4);
        sessionState["STOPPED"] = py::int_(5);
        m.attr("SessionState") = sessionState;
        
        // Stream State
        py::dict streamState = py::dict();
        streamState["INACTIVE"] = py::int_(0);
        streamState["ACTIVE"] = py::int_(1);
        streamState["INTERRUPTED"] = py::int_(2);
        streamState["TERMINATING"] = py::int_(3);
        streamState["TERMINATED"] = py::int_(4);
        m.attr("StreamState") = streamState;
        
        // Event Type
        py::dict eventType = py::dict();
        eventType["UNDEFINED"] = py::int_(0);
        eventType["FIRST_PACKET_TIMESTAMP"] = py::int_(1);
        eventType["ACTIVE_SPEAKER_CHANGE"] = py::int_(2);
        eventType["PARTICIPANT_JOIN"] = py::int_(3);
        eventType["PARTICIPANT_LEAVE"] = py::int_(4);
        m.attr("EventType") = eventType;
        
        // Message Type
        py::dict messageType = py::dict();
        messageType["UNDEFINED"] = py::int_(0);
        messageType["SIGNALING_HAND_SHAKE_REQ"] = py::int_(1);
        messageType["SIGNALING_HAND_SHAKE_RESP"] = py::int_(2);
        messageType["DATA_HAND_SHAKE_REQ"] = py::int_(3);
        messageType["DATA_HAND_SHAKE_RESP"] = py::int_(4);
        messageType["EVENT_SUBSCRIPTION"] = py::int_(5);
        messageType["EVENT_UPDATE"] = py::int_(6);
        messageType["CLIENT_READY_ACK"] = py::int_(7);
        messageType["STREAM_STATE_UPDATE"] = py::int_(8);
        messageType["SESSION_STATE_UPDATE"] = py::int_(9);
        messageType["SESSION_STATE_REQ"] = py::int_(10);
        messageType["SESSION_STATE_RESP"] = py::int_(11);
        messageType["KEEP_ALIVE_REQ"] = py::int_(12);
        messageType["KEEP_ALIVE_RESP"] = py::int_(13);
        messageType["MEDIA_DATA_AUDIO"] = py::int_(14);
        messageType["MEDIA_DATA_VIDEO"] = py::int_(15);
        messageType["MEDIA_DATA_SHARE"] = py::int_(16);
        messageType["MEDIA_DATA_TRANSCRIPT"] = py::int_(17);
        messageType["MEDIA_DATA_CHAT"] = py::int_(18);
        m.attr("MessageType") = messageType;
        
        // Stop Reason
        py::dict stopReason = py::dict();
        stopReason["UNDEFINED"] = py::int_(0);
        stopReason["STOP_BC_HOST_TRIGGERED"] = py::int_(1);
        stopReason["STOP_BC_USER_TRIGGERED"] = py::int_(2);
        stopReason["STOP_BC_USER_LEFT"] = py::int_(3);
        stopReason["STOP_BC_USER_EJECTED"] = py::int_(4);
        stopReason["STOP_BC_APP_DISABLED_BY_HOST"] = py::int_(5);
        stopReason["STOP_BC_MEETING_ENDED"] = py::int_(6);
        stopReason["STOP_BC_STREAM_CANCELED"] = py::int_(7);
        stopReason["STOP_BC_STREAM_REVOKED"] = py::int_(8);
        stopReason["STOP_BC_ALL_APPS_DISABLED"] = py::int_(9);
        stopReason["STOP_BC_INTERNAL_EXCEPTION"] = py::int_(10);
        stopReason["STOP_BC_CONNECTION_TIMEOUT"] = py::int_(11);
        stopReason["STOP_BC_MEETING_CONNECTION_INTERRUPTED"] = py::int_(12);
        stopReason["STOP_BC_SIGNAL_CONNECTION_INTERRUPTED"] = py::int_(13);
        stopReason["STOP_BC_DATA_CONNECTION_INTERRUPTED"] = py::int_(14);
        stopReason["STOP_BC_SIGNAL_CONNECTION_CLOSED_ABNORMALLY"] = py::int_(15);
        stopReason["STOP_BC_DATA_CONNECTION_CLOSED_ABNORMALLY"] = py::int_(16);
        stopReason["STOP_BC_EXIT_SIGNAL"] = py::int_(17);
        stopReason["STOP_BC_AUTHENTICATION_FAILURE"] = py::int_(18);
        m.attr("StopReason") = stopReason;
        
}