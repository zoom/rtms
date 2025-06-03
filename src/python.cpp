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
#define DEBUG_LOG(msg) std::cerr << "[RTMS-PY] " << msg << std::endl
#else
#define DEBUG_LOG(msg)
#endif

// Python-friendly wrapper class for RTMS Client
class PyClient {
public:
    PyClient() : configured_media_types_(0), is_configured_(false) {
        DEBUG_LOG("Creating Python RTMS client");
        try {
            client_ = make_unique<Client>();
            DEBUG_LOG("Client instance created successfully");
        } catch (const exception& e) {
            DEBUG_LOG("Error creating client instance: " << e.what());
            throw py::error_already_set();
        }
    }
    
    ~PyClient() {
        DEBUG_LOG("Destroying Python RTMS client");
        // First clear all callbacks to avoid issues during destruction
        try {
            // Reset all callbacks to empty functions to avoid calling Python
            // during destruction when Python might be shutting down
            if (client_) {
                client_->setOnJoinConfirm([](int) {});
                client_->setOnSessionUpdate([](int, const Session&) {});
                client_->setOnUserUpdate([](int, const Participant&) {});
                client_->setOnAudioData([](const vector<uint8_t>&, uint32_t, const Metadata&) {});
                client_->setOnVideoData([](const vector<uint8_t>&, uint32_t, const string&, const Metadata&) {});
                client_->setOnTranscriptData([](const vector<uint8_t>&, uint32_t, const Metadata&) {});
                client_->setOnLeave([](int) {});
            }
        } catch (...) {
            // Ignore any errors during callback cleanup
        }
        
        join_confirm_callback = py::none();
        session_update_callback = py::none();
        user_update_callback = py::none();
        audio_data_callback = py::none();
        video_data_callback = py::none();
        transcript_data_callback = py::none();
        leave_callback = py::none();
    }

    static void initialize(const string& ca_path) {
        DEBUG_LOG("Initializing RTMS with CA path: " << ca_path);
        try {
            Client::initialize(ca_path);
            DEBUG_LOG("RTMS initialized successfully");
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
        DEBUG_LOG("Uninitializing RTMS");
        try {
            Client::uninitialize();
            DEBUG_LOG("RTMS uninitialized successfully");
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
        DEBUG_LOG("Joining RTMS session: uuid=" << uuid << ", stream_id=" << stream_id << 
                 ", server_url=" << server_url << ", timeout=" << timeout);
        try {
            // Configure if not already configured
            if (!is_configured_) {
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
                client_->configure(params, media_types, false);
                is_configured_ = true;
                DEBUG_LOG("Client configured successfully");
            }
            
            client_->join(uuid, stream_id, signature, server_url, timeout);
            DEBUG_LOG("Joined RTMS session successfully");
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
            
            // If signature is not provided, we'll need to rely on Python code to generate it
            // We're not implementing that here
            
            join(uuid, stream_id, signature, server_urls, timeout);
        } catch (const py::error_already_set& e) {
            // Just rethrow Python errors
            throw;
        } catch (const std::exception& e) {
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
            DEBUG_LOG("Unknown error during polling: " << e.what());
            return;
        } catch (...) {
            DEBUG_LOG("Unknown error during polling (caught ...)");
            return;
        }
    }

    void release() {
        DEBUG_LOG("Releasing RTMS client");
        try {
            client_->release();
            DEBUG_LOG("RTMS client released successfully");
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

    string streamId() const {
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

    // Decorator methods that return functions that can be used as decorators
    py::function on_join_confirm() {
        DEBUG_LOG("Creating on_join_confirm decorator");
        
        // Create a decorator function that will capture 'this'
        return py::cpp_function([this](py::function func) {
            DEBUG_LOG("on_join_confirm decorator called with function");
            this->set_join_confirm_callback(func);
            return func;
        });
    }
    
    py::function on_session_update() {
        DEBUG_LOG("Creating on_session_update decorator");
        
        return py::cpp_function([this](py::function func) {
            DEBUG_LOG("on_session_update decorator called with function");
            this->set_session_update_callback(func);
            return func;
        });
    }
    
    py::function on_user_update() {
        DEBUG_LOG("Creating on_user_update decorator");
        
        return py::cpp_function([this](py::function func) {
            DEBUG_LOG("on_user_update decorator called with function");
            this->set_user_update_callback(func);
            return func;
        });
    }
    
    py::function on_audio_data() {
        DEBUG_LOG("Creating on_audio_data decorator");
        
        return py::cpp_function([this](py::function func) {
            DEBUG_LOG("on_audio_data decorator called with function");
            this->set_audio_data_callback(func);
            return func;
        });
    }
    
    py::function on_video_data() {
        DEBUG_LOG("Creating on_video_data decorator");
        
        return py::cpp_function([this](py::function func) {
            DEBUG_LOG("on_video_data decorator called with function");
            this->set_video_data_callback(func);
            return func;
        });
    }
    
    py::function on_transcript_data() {
        DEBUG_LOG("Creating on_transcript_data decorator");
        
        return py::cpp_function([this](py::function func) {
            DEBUG_LOG("on_transcript_data decorator called with function");
            this->set_transcript_data_callback(func);
            return func;
        });
    }
    
    py::function on_leave() {
        DEBUG_LOG("Creating on_leave decorator");
        
        return py::cpp_function([this](py::function func) {
            DEBUG_LOG("on_leave decorator called with function");
            this->set_leave_callback(func);
            return func;
        });
    }
    
    // Direct callback setting methods
    void set_join_confirm_callback(py::function callback) {
        DEBUG_LOG("Setting join confirm callback");
        // Store Python callback
        join_confirm_callback = callback;
        
        client_->setOnJoinConfirm([this](int reason) {
            py::gil_scoped_acquire acquire;
            try {
                if (!join_confirm_callback.is_none() && PyCallable_Check(join_confirm_callback.ptr())) {
                    join_confirm_callback(reason);
                }
            } catch (py::error_already_set& e) {
                DEBUG_LOG("Error in join confirm callback: " << e.what());
                e.restore();  // This ensures the Python exception is properly restored
            } catch (std::exception& e) {
                DEBUG_LOG("C++ exception in join confirm callback: " << e.what());
            } catch (...) {
                DEBUG_LOG("Unknown exception in join confirm callback");
            }
        });
    }

    void set_session_update_callback(py::function callback) {
        DEBUG_LOG("Setting session update callback");
        session_update_callback = callback;
        
        client_->setOnSessionUpdate([this](int op, const Session& session) {
            py::gil_scoped_acquire acquire;
            try {
                if (!session_update_callback.is_none() && PyCallable_Check(session_update_callback.ptr())) {
                    session_update_callback(op, session);
                }
            } catch (py::error_already_set& e) {
                DEBUG_LOG("Error in session update callback: " << e.what());
                e.restore();
            } catch (std::exception& e) {
                DEBUG_LOG("C++ exception in session update callback: " << e.what());
            } catch (...) {
                DEBUG_LOG("Unknown exception in session update callback");
            }
        });
    }

    void set_user_update_callback(py::function callback) {
        DEBUG_LOG("Setting user update callback");
        user_update_callback = callback;
        
        client_->setOnUserUpdate([this](int op, const Participant& participant) {
            py::gil_scoped_acquire acquire;
            try {
                if (!user_update_callback.is_none() && PyCallable_Check(user_update_callback.ptr())) {
                    user_update_callback(op, participant);
                }
            } catch (py::error_already_set& e) {
                DEBUG_LOG("Error in user update callback: " << e.what());
                e.restore();
            } catch (std::exception& e) {
                DEBUG_LOG("C++ exception in user update callback: " << e.what());
            } catch (...) {
                DEBUG_LOG("Unknown exception in user update callback");
            }
        });
    }

    void set_audio_data_callback(py::function callback) {
        DEBUG_LOG("Setting audio data callback");
        audio_data_callback = callback;
        
        client_->setOnAudioData([this](const vector<uint8_t>& data, uint32_t timestamp, const Metadata& metadata) {
            py::gil_scoped_acquire acquire;
            try {
                if (!audio_data_callback.is_none() && PyCallable_Check(audio_data_callback.ptr())) {
                    py::bytes py_data(reinterpret_cast<const char*>(data.data()), data.size());
                    audio_data_callback(py_data, data.size(), timestamp, metadata);
                }
            } catch (py::error_already_set& e) {
                DEBUG_LOG("Error in audio data callback: " << e.what());
                e.restore();
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
        DEBUG_LOG("Setting video data callback");
        video_data_callback = callback;
        
        client_->setOnVideoData([this](const vector<uint8_t>& data, uint32_t timestamp, const string& track_id, const Metadata& metadata) {
            py::gil_scoped_acquire acquire;
            try {
                if (!video_data_callback.is_none() && PyCallable_Check(video_data_callback.ptr())) {
                    py::bytes py_data(reinterpret_cast<const char*>(data.data()), data.size());
                    video_data_callback(py_data, data.size(), timestamp, track_id, metadata);
                }
            } catch (py::error_already_set& e) {
                DEBUG_LOG("Error in video data callback: " << e.what());
                e.restore();
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
        DEBUG_LOG("Setting transcript data callback");
        transcript_data_callback = callback;
        
        client_->setOnTranscriptData([this](const vector<uint8_t>& data, uint32_t timestamp, const Metadata& metadata) {
            py::gil_scoped_acquire acquire;
            try {
                if (!transcript_data_callback.is_none() && PyCallable_Check(transcript_data_callback.ptr())) {
                    py::bytes py_data(reinterpret_cast<const char*>(data.data()), data.size());
                    transcript_data_callback(py_data, data.size(), timestamp, metadata);
                }
            } catch (py::error_already_set& e) {
                DEBUG_LOG("Error in transcript data callback: " << e.what());
                e.restore();
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
        DEBUG_LOG("Setting leave callback");
        leave_callback = callback;
        
        client_->setOnLeave([this](int reason) {
            py::gil_scoped_acquire acquire;
            try {
                if (!leave_callback.is_none() && PyCallable_Check(leave_callback.ptr())) {
                    leave_callback(reason);
                }
            } catch (py::error_already_set& e) {
                DEBUG_LOG("Error in leave callback: " << e.what());
                e.restore();
            } catch (std::exception& e) {
                DEBUG_LOG("C++ exception in leave callback: " << e.what());
            } catch (...) {
                DEBUG_LOG("Unknown exception in leave callback");
            }
        });
    }

private:
    void reconfigure_media_types() {
        if (!is_configured_) return;
        
        try {
            DEBUG_LOG("Reconfiguring media types: " << configured_media_types_);
            
            MediaParameters params;
            
            if (configured_media_types_ & static_cast<int>(SDK_AUDIO)) {
                DEBUG_LOG("Setting up full audio parameters for reconfiguration");
                AudioParameters audio;

                audio.setContentType(0);    // Default content type
                audio.setCodec(0);          // Default codec
                audio.setSampleRate(16000); // 16kHz is common for audio
                audio.setChannel(1);        // Mono
                audio.setDataOpt(0);        // Default data option
                audio.setDuration(20);      // 20ms frame duration
                audio.setFrameSize(320);    // 16000Hz * 20ms = 320 samples
                params.setAudioParameters(audio);
            }
            
            if (configured_media_types_ & static_cast<int>(SDK_VIDEO)) {
                DEBUG_LOG("Setting up full video parameters for reconfiguration");
                VideoParameters video;

                video.setContentType(0);   
                video.setCodec(0);          
                video.setResolution(0);     
                video.setDataOpt(0);       
                video.setFps(30);
                params.setVideoParameters(video);
            }
            
            DEBUG_LOG("Calling configure with media types: " << configured_media_types_);
            client_->configure(params, configured_media_types_, false);
            DEBUG_LOG("Media types reconfigured successfully");
        } catch (const Exception& e) {
            DEBUG_LOG("Failed to reconfigure media types: " << e.what() << " (code: " << e.code() << ")");
            // Don't throw here since this is called from callback setters
        } catch (const exception& e) {
            DEBUG_LOG("Unknown error reconfiguring media types: " << e.what());
            // Don't throw here since this is called from callback setters
        } catch (...) {
            DEBUG_LOG("Unknown error in reconfigure_media_types");
            // Don't throw 
        }
    }

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
};

// Global client for backward compatibility
static PyClient global_client;

PYBIND11_MODULE(_rtms, m) {
    m.doc() = "Zoom RTMS Python Bindings";

    // Expose SDK constants - using explicit static_cast to int
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
        .def("stream_id", &PyClient::streamId, "Get the stream ID of the current meeting")
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
        global_client.initialize(ca_path);
    }, "Initialize the RTMS SDK with the specified CA certificate path");
    
    m.def("_uninitialize", []() {
        global_client.uninitialize();
    }, "Uninitialize the RTMS SDK");
    
    m.def("_join", [](const string& uuid, const string& stream_id, const string& signature, 
                    const string& server_urls, int timeout) {
        DEBUG_LOG("Python _join called on global client");
        global_client.join(uuid, stream_id, signature, server_urls, timeout);
    }, "Join a meeting with the specified parameters", 
       py::arg("uuid"), py::arg("stream_id"), py::arg("signature"), 
       py::arg("server_urls"), py::arg("timeout") = -1);
    
    m.def("_poll", []() {
        global_client.poll();
    }, "Poll for new events");
    
    m.def("_release", []() {
        DEBUG_LOG("Python _release called on global client");
        global_client.release();
    }, "Release resources");
    
    m.def("_uuid", []() {
        return global_client.uuid();
    }, "Get the UUID of the current meeting");
    
    m.def("_stream_id", []() {
        return global_client.streamId();
    }, "Get the stream ID of the current meeting");
    
    // Register callback decorators for the global client
    m.def("on_join_confirm", [](py::function callback) {
        DEBUG_LOG("Python on_join_confirm called on global client");
        global_client.set_join_confirm_callback(callback);
        return callback;
    }, "Set callback for join confirmation events");
    
    m.def("on_session_update", [](py::function callback) {
        DEBUG_LOG("Python on_session_update called on global client");
        global_client.set_session_update_callback(callback);
        return callback;
    }, "Set callback for session update events");
    
    m.def("on_user_update", [](py::function callback) {
        DEBUG_LOG("Python on_user_update called on global client");
        global_client.set_user_update_callback(callback);
        return callback;
    }, "Set callback for participant update events");
    
    m.def("on_audio_data", [](py::function callback) {
        DEBUG_LOG("Python on_audio_data called on global client");
        global_client.set_audio_data_callback(callback);
        return callback;
    }, "Set callback for receiving audio data");
    
    m.def("on_video_data", [](py::function callback) {
        DEBUG_LOG("Python on_video_data called on global client");
        global_client.set_video_data_callback(callback);
        return callback;
    }, "Set callback for receiving video data");
    
    m.def("on_transcript_data", [](py::function callback) {
        DEBUG_LOG("Python on_transcript_data called on global client");
        global_client.set_transcript_data_callback(callback);
        return callback;
    }, "Set callback for receiving transcript data");
    
    m.def("on_leave", [](py::function callback) {
        DEBUG_LOG("Python on_leave called on global client");
        global_client.set_leave_callback(callback);
        return callback;
    }, "Set callback for leave events");
}