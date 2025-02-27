#include <napi.h>
#include "rtms.h"
#include <string>
#include <functional>
#include <memory>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <iostream>

using namespace Napi;
using namespace std;

// Global client instance for singleton mode
unique_ptr<rtms::Client> global_client = nullptr;
Napi::ThreadSafeFunction global_tsfn_join_confirm;
Napi::ThreadSafeFunction global_tsfn_session_update;
Napi::ThreadSafeFunction global_tsfn_user_update;
Napi::ThreadSafeFunction global_tsfn_audio_data;
Napi::ThreadSafeFunction global_tsfn_video_data;
Napi::ThreadSafeFunction global_tsfn_transcript_data;
Napi::ThreadSafeFunction global_tsfn_leave;
int global_configured_media_types = 0;
bool global_is_configured = false;
rtms::MediaParameters global_media_params;

// Function to ensure global client is initialized
void ensure_global_client() {
    if (!global_client) {
        global_client = make_unique<rtms::Client>();
    }
}

class NodeClient : public Napi::ObjectWrap<NodeClient> {
public:
    static Napi::Object init(Napi::Env env, Napi::Object exports);
    NodeClient(const Napi::CallbackInfo& info);
    ~NodeClient();

private:
    static Napi::Value initialize(const Napi::CallbackInfo& info);
    static Napi::Value uninitialize(const Napi::CallbackInfo& info);

    Napi::Value join(const Napi::CallbackInfo& info);
    Napi::Value configure(const Napi::CallbackInfo& info);
    Napi::Value poll(const Napi::CallbackInfo& info);
    Napi::Value release(const Napi::CallbackInfo& info);
    Napi::Value uuid(const Napi::CallbackInfo& info);
    Napi::Value streamId(const Napi::CallbackInfo& info);

    Napi::Value setAudioParameters(const Napi::CallbackInfo& info);
    Napi::Value setVideoParameters(const Napi::CallbackInfo& info);

    Napi::Value setOnJoinConfirm(const Napi::CallbackInfo& info);
    Napi::Value setOnSessionUpdate(const Napi::CallbackInfo& info);
    Napi::Value setOnUserUpdate(const Napi::CallbackInfo& info);
    Napi::Value setOnAudioData(const Napi::CallbackInfo& info);
    Napi::Value setOnVideoData(const Napi::CallbackInfo& info);
    Napi::Value setOnTranscriptData(const Napi::CallbackInfo& info);
    Napi::Value setOnLeave(const Napi::CallbackInfo& info);

    void reconfigureMediaTypes();

    unique_ptr<rtms::Client> client_;
    Napi::ThreadSafeFunction tsfn_join_confirm_;
    Napi::ThreadSafeFunction tsfn_session_update_;
    Napi::ThreadSafeFunction tsfn_user_update_;
    Napi::ThreadSafeFunction tsfn_audio_data_;
    Napi::ThreadSafeFunction tsfn_video_data_;
    Napi::ThreadSafeFunction tsfn_transcript_data_;
    Napi::ThreadSafeFunction tsfn_leave_;
    
    int configured_media_types_;
    bool is_configured_;
    rtms::MediaParameters media_params_;
};

Napi::Value NodeClient::poll(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    try {
        client_->poll();
        return Napi::Boolean::New(env, true);
    } catch (const rtms::Exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
}

Napi::Value NodeClient::release(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    try {
        client_->release();
        return Napi::Boolean::New(env, true);
    } catch (const rtms::Exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
}

Napi::Value NodeClient::uuid(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    try {
        string uuid = client_->uuid();
        return Napi::String::New(env, uuid);
    } catch (const rtms::Exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
}

Napi::Value NodeClient::streamId(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    try {
        string stream_id = client_->streamId();
        return Napi::String::New(env, stream_id);
    } catch (const rtms::Exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
}

Napi::Value NodeClient::setAudioParameters(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "Object argument expected").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Object params = info[0].As<Napi::Object>();
    rtms::AudioParameters audio_params;

    if (params.Has("contentType") && params.Get("contentType").IsNumber()) {
        audio_params.setContentType(params.Get("contentType").As<Napi::Number>().Int32Value());
    }
    
    if (params.Has("codec") && params.Get("codec").IsNumber()) {
        audio_params.setCodec(params.Get("codec").As<Napi::Number>().Int32Value());
    }
    
    if (params.Has("sampleRate") && params.Get("sampleRate").IsNumber()) {
        audio_params.setSampleRate(params.Get("sampleRate").As<Napi::Number>().Int32Value());
    }
    
    if (params.Has("channel") && params.Get("channel").IsNumber()) {
        audio_params.setChannel(params.Get("channel").As<Napi::Number>().Int32Value());
    }
    
    if (params.Has("dataOpt") && params.Get("dataOpt").IsNumber()) {
        audio_params.setDataOpt(params.Get("dataOpt").As<Napi::Number>().Int32Value());
    }
    
    if (params.Has("duration") && params.Get("duration").IsNumber()) {
        audio_params.setDuration(params.Get("duration").As<Napi::Number>().Int32Value());
    }
    
    if (params.Has("frameSize") && params.Get("frameSize").IsNumber()) {
        audio_params.setFrameSize(params.Get("frameSize").As<Napi::Number>().Int32Value());
    }

    media_params_.setAudioParameters(audio_params);
    configured_media_types_ |= SDK_AUDIO;
    
    if (is_configured_) {
        reconfigureMediaTypes();
    }

    return Napi::Boolean::New(env, true);
}

Napi::Value NodeClient::setVideoParameters(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "Object argument expected").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Object params = info[0].As<Napi::Object>();
    rtms::VideoParameters video_params;

    if (params.Has("contentType") && params.Get("contentType").IsNumber()) {
        video_params.setContentType(params.Get("contentType").As<Napi::Number>().Int32Value());
    }
    
    if (params.Has("codec") && params.Get("codec").IsNumber()) {
        video_params.setCodec(params.Get("codec").As<Napi::Number>().Int32Value());
    }
    
    if (params.Has("resolution") && params.Get("resolution").IsNumber()) {
        video_params.setResolution(params.Get("resolution").As<Napi::Number>().Int32Value());
    }
    
    if (params.Has("dataOpt") && params.Get("dataOpt").IsNumber()) {
        video_params.setDataOpt(params.Get("dataOpt").As<Napi::Number>().Int32Value());
    }
    
    if (params.Has("fps") && params.Get("fps").IsNumber()) {
        video_params.setFps(params.Get("fps").As<Napi::Number>().Int32Value());
    }

    media_params_.setVideoParameters(video_params);
    configured_media_types_ |= SDK_VIDEO;
    
    if (is_configured_) {
        reconfigureMediaTypes();
    }

    return Napi::Boolean::New(env, true);
}

Napi::Value NodeClient::setOnJoinConfirm(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Function argument expected").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Function callback = info[0].As<Napi::Function>();
    
    if (tsfn_join_confirm_) {
        tsfn_join_confirm_.Release();
    }

    tsfn_join_confirm_ = Napi::ThreadSafeFunction::New(
        env, callback, "JoinConfirmCallback", 0, 1
    );

    client_->setOnJoinConfirm([this](int reason) {
        auto callback = [reason](Napi::Env env, Napi::Function jsCallback) {
            jsCallback.Call({Napi::Number::New(env, reason)});
        };
        tsfn_join_confirm_.BlockingCall(callback);
    });

    return Napi::Boolean::New(env, true);
}

Napi::Value NodeClient::setOnSessionUpdate(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Function argument expected").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Function callback = info[0].As<Napi::Function>();
    
    if (tsfn_session_update_) {
        tsfn_session_update_.Release();
    }

    tsfn_session_update_ = Napi::ThreadSafeFunction::New(
        env, callback, "SessionUpdateCallback", 0, 1
    );

    client_->setOnSessionUpdate([this](int op, const rtms::Session& session) {
        auto callback = [op, sessionId = session.sessionId(), statTime = session.statTime(), status = session.status()]
                        (Napi::Env env, Napi::Function jsCallback) {
            Napi::Object sessionObj = Napi::Object::New(env);
            sessionObj.Set("sessionId", Napi::String::New(env, sessionId));
            sessionObj.Set("statTime", Napi::Number::New(env, statTime));
            sessionObj.Set("status", Napi::Number::New(env, status));
            sessionObj.Set("isActive", Napi::Boolean::New(env, status == SESS_STATUS_ACTIVE));
            sessionObj.Set("isPaused", Napi::Boolean::New(env, status == SESS_STATUS_PAUSED));

            jsCallback.Call({Napi::Number::New(env, op), sessionObj});
        };
        tsfn_session_update_.BlockingCall(callback);
    });

    return Napi::Boolean::New(env, true);
}

Napi::Value NodeClient::setOnUserUpdate(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Function argument expected").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Function callback = info[0].As<Napi::Function>();
    
    if (tsfn_user_update_) {
        tsfn_user_update_.Release();
    }

    tsfn_user_update_ = Napi::ThreadSafeFunction::New(
        env, callback, "UserUpdateCallback", 0, 1
    );

    client_->setOnUserUpdate([this](int op, const rtms::Participant& participant) {
        auto callback = [op, id = participant.id(), name = participant.name()]
                        (Napi::Env env, Napi::Function jsCallback) {
            Napi::Object participantObj = Napi::Object::New(env);
            participantObj.Set("id", Napi::Number::New(env, id));
            participantObj.Set("name", Napi::String::New(env, name));

            jsCallback.Call({Napi::Number::New(env, op), participantObj});
        };
        tsfn_user_update_.BlockingCall(callback);
    });

    return Napi::Boolean::New(env, true);
}

Napi::Value NodeClient::setOnAudioData(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Function argument expected").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Function callback = info[0].As<Napi::Function>();
    
    if (tsfn_audio_data_) {
        tsfn_audio_data_.Release();
    }

    tsfn_audio_data_ = Napi::ThreadSafeFunction::New(
        env, callback, "AudioDataCallback", 0, 1
    );

    client_->setOnAudioData([this](const vector<uint8_t>& data, uint32_t timestamp, const rtms::Metadata& metadata) {
        auto callback = [data, timestamp, userName = metadata.userName(), userId = metadata.userId()]
                       (Napi::Env env, Napi::Function jsCallback) {
            Napi::Buffer<uint8_t> buffer = Napi::Buffer<uint8_t>::Copy(env, data.data(), data.size());
            
            Napi::Object metadataObj = Napi::Object::New(env);
            metadataObj.Set("userName", Napi::String::New(env, userName));
            metadataObj.Set("userId", Napi::Number::New(env, userId));

            jsCallback.Call({buffer, Napi::Number::New(env, data.size()), Napi::Number::New(env, timestamp), metadataObj});
        };
        tsfn_audio_data_.BlockingCall(callback);
    });

    // Auto-configure for audio
    configured_media_types_ |= SDK_AUDIO;
    if (is_configured_) {
        reconfigureMediaTypes();
    }

    return Napi::Boolean::New(env, true);
}

Napi::Value NodeClient::setOnVideoData(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Function argument expected").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Function callback = info[0].As<Napi::Function>();
    
    if (tsfn_video_data_) {
        tsfn_video_data_.Release();
    }

    tsfn_video_data_ = Napi::ThreadSafeFunction::New(
        env, callback, "VideoDataCallback", 0, 1
    );

    client_->setOnVideoData([this](const vector<uint8_t>& data, uint32_t timestamp, const string& session_id, const rtms::Metadata& metadata) {
        auto callback = [data, timestamp, session_id, userName = metadata.userName(), userId = metadata.userId()]
                       (Napi::Env env, Napi::Function jsCallback) {
            Napi::Buffer<uint8_t> buffer = Napi::Buffer<uint8_t>::Copy(env, data.data(), data.size());
            
            Napi::Object metadataObj = Napi::Object::New(env);
            metadataObj.Set("userName", Napi::String::New(env, userName));
            metadataObj.Set("userId", Napi::Number::New(env, userId));

            jsCallback.Call({buffer, Napi::Number::New(env, data.size()), Napi::Number::New(env, timestamp), Napi::String::New(env, session_id), metadataObj});
        };
        tsfn_video_data_.BlockingCall(callback);
    });

    // Auto-configure for video
    configured_media_types_ |= SDK_VIDEO;
    if (is_configured_) {
        reconfigureMediaTypes();
    }

    return Napi::Boolean::New(env, true);
}

Napi::Value NodeClient::setOnTranscriptData(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Function argument expected").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Function callback = info[0].As<Napi::Function>();
    
    if (tsfn_transcript_data_) {
        tsfn_transcript_data_.Release();
    }

    tsfn_transcript_data_ = Napi::ThreadSafeFunction::New(
        env, callback, "TranscriptDataCallback", 0, 1
    );

    client_->setOnTranscriptData([this](const vector<uint8_t>& data, uint32_t timestamp, const rtms::Metadata& metadata) {
        auto callback = [data, timestamp, userName = metadata.userName(), userId = metadata.userId()]
                       (Napi::Env env, Napi::Function jsCallback) {
            Napi::Buffer<uint8_t> buffer = Napi::Buffer<uint8_t>::Copy(env, data.data(), data.size());
            
            Napi::Object metadataObj = Napi::Object::New(env);
            metadataObj.Set("userName", Napi::String::New(env, userName));
            metadataObj.Set("userId", Napi::Number::New(env, userId));

            jsCallback.Call({buffer, Napi::Number::New(env, data.size()), Napi::Number::New(env, timestamp), metadataObj});
        };
        tsfn_transcript_data_.BlockingCall(callback);
    });

    // Auto-configure for transcript
    configured_media_types_ |= SDK_TRANSCRIPT;
    if (is_configured_) {
        reconfigureMediaTypes();
    }

    return Napi::Boolean::New(env, true);
}

Napi::Value NodeClient::setOnLeave(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Function argument expected").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Function callback = info[0].As<Napi::Function>();
    
    if (tsfn_leave_) {
        tsfn_leave_.Release();
    }

    tsfn_leave_ = Napi::ThreadSafeFunction::New(
        env, callback, "LeaveCallback", 0, 1
    );

    client_->setOnLeave([this](int reason) {
        auto callback = [reason](Napi::Env env, Napi::Function jsCallback) {
            jsCallback.Call({Napi::Number::New(env, reason)});
        };
        tsfn_leave_.BlockingCall(callback);
    });

    return Napi::Boolean::New(env, true);
}

NodeClient::NodeClient(const Napi::CallbackInfo& info) 
    : Napi::ObjectWrap<NodeClient>(info), 
      configured_media_types_(0),
      is_configured_(false) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    try {
        client_ = make_unique<rtms::Client>();
    } catch (const rtms::Exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
    }
}

NodeClient::~NodeClient() {
    if (tsfn_join_confirm_) tsfn_join_confirm_.Release();
    if (tsfn_session_update_) tsfn_session_update_.Release();
    if (tsfn_user_update_) tsfn_user_update_.Release();
    if (tsfn_audio_data_) tsfn_audio_data_.Release();
    if (tsfn_video_data_) tsfn_video_data_.Release();
    if (tsfn_transcript_data_) tsfn_transcript_data_.Release();
    if (tsfn_leave_) tsfn_leave_.Release();
}

Napi::Value NodeClient::initialize(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    try {
        string ca_path = "";
        if (info.Length() > 0 && info[0].IsString()) {
            ca_path = info[0].As<Napi::String>();
        }
        
        rtms::Client::initialize(ca_path);
        return Napi::Boolean::New(env, true);
    } catch (const rtms::Exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
}

Napi::Value NodeClient::uninitialize(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    rtms::Client::uninitialize();
    return Napi::Boolean::New(env, true);
}

void NodeClient::reconfigureMediaTypes() {
    if (!client_ || !is_configured_) return;
    
    try {
        client_->configure(media_params_, configured_media_types_, false);
    } catch (const rtms::Exception& e) {
        // Log error but don't throw - this is called from callback setters
        cerr << "Failed to reconfigure media types: " << e.what() << endl;
    }
}

Napi::Value NodeClient::configure(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    int media_types = info[0].As<Napi::Number>().Int32Value();
    bool enable_ale = false;
    
    if (info.Length() > 1 && info[1].IsBoolean()) {
        enable_ale = info[1].As<Napi::Boolean>().Value();
    }
    
    try {
        configured_media_types_ = media_types;
        client_->configure(media_params_, media_types, enable_ale);
        is_configured_ = true;
        return Napi::Boolean::New(env, true);
    } catch (const rtms::Exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
}

Napi::Value NodeClient::join(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 4) {
        Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!info[0].IsString() || !info[1].IsString() || !info[2].IsString() || !info[3].IsString()) {
        Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    string meeting_uuid = info[0].As<Napi::String>();
    string rtms_stream_id = info[1].As<Napi::String>();
    string signature = info[2].As<Napi::String>();
    string server_url = info[3].As<Napi::String>();
    
    int timeout = -1;
    if (info.Length() > 4 && info[4].IsNumber()) {
        timeout = info[4].As<Napi::Number>().Int32Value();
    }

    try {
        if (!is_configured_) {
            media_params_ = rtms::MediaParameters();
            client_->configure(media_params_, configured_media_types_ > 0 ? configured_media_types_ : SDK_ALL, false);
            is_configured_ = true;
        }

        client_->join(meeting_uuid, rtms_stream_id, signature, server_url, timeout);
        return Napi::Boolean::New(env, true);
    } catch (const rtms::Exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
    
}

// Global client API implementations
void reconfigureGlobalMediaTypes() {
    if (!global_client || !global_is_configured) return;
    
    try {
        global_client->configure(global_media_params, global_configured_media_types, false);
    } catch (const rtms::Exception& e) {
        // Log error but don't throw - this is called from callback setters
        cerr << "Failed to reconfigure global media types: " << e.what() << endl;
    }
}

Napi::Value globalJoin(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    // Support both object form and parameter form
    if (info.Length() >= 1 && info[0].IsObject() && !info[0].IsString()) {
        // Object form: join({meeting_uuid: '...', rtms_stream_id: '...', ...})
        Napi::Object options = info[0].As<Napi::Object>();
        
        if (!options.Has("meeting_uuid") || !options.Get("meeting_uuid").IsString() ||
            !options.Has("rtms_stream_id") || !options.Get("rtms_stream_id").IsString() ||
            !options.Has("server_urls") || !options.Get("server_urls").IsString()) {
            Napi::TypeError::New(env, "Options object must contain meeting_uuid, rtms_stream_id, and server_urls as strings").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        string meeting_uuid = options.Get("meeting_uuid").As<Napi::String>();
        string rtms_stream_id = options.Get("rtms_stream_id").As<Napi::String>();
        string server_url = options.Get("server_urls").As<Napi::String>();
        
        string signature = "";
        if (options.Has("signature") && options.Get("signature").IsString()) {
            signature = options.Get("signature").As<Napi::String>();
        }
        
        int timeout = -1;
        if (options.Has("timeout") && options.Get("timeout").IsNumber()) {
            timeout = options.Get("timeout").As<Napi::Number>().Int32Value();
        }
        
        ensure_global_client();
        
        try {
            if (!global_is_configured) {
                global_media_params = rtms::MediaParameters();
                global_client->configure(global_media_params, global_configured_media_types > 0 ? global_configured_media_types : SDK_ALL, false);
                global_is_configured = true;
            }

            global_client->join(meeting_uuid, rtms_stream_id, signature, server_url, timeout);
            return Napi::Boolean::New(env, true);
        } catch (const rtms::Exception& e) {
            Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
            return env.Null();
        }
    }
    else if (info.Length() >= 4) {
        // Parameter form: join(meeting_uuid, rtms_stream_id, signature, server_url, [timeout])
        if (!info[0].IsString() || !info[1].IsString() || !info[2].IsString() || !info[3].IsString()) {
            Napi::TypeError::New(env, "Arguments must be strings").ThrowAsJavaScriptException();
            return env.Null();
        }

        string meeting_uuid = info[0].As<Napi::String>();
        string rtms_stream_id = info[1].As<Napi::String>();
        string signature = info[2].As<Napi::String>();
        string server_url = info[3].As<Napi::String>();
        
        int timeout = -1;
        if (info.Length() > 4 && info[4].IsNumber()) {
            timeout = info[4].As<Napi::Number>().Int32Value();
        }
        
        ensure_global_client();
        
        try {
            if (!global_is_configured) {
                global_media_params = rtms::MediaParameters();
                global_client->configure(global_media_params, global_configured_media_types > 0 ? global_configured_media_types : SDK_ALL, false);
                global_is_configured = true;
            }

            global_client->join(meeting_uuid, rtms_stream_id, signature, server_url, timeout);
            return Napi::Boolean::New(env, true);
        } catch (const rtms::Exception& e) {
            Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
            return env.Null();
        }
    }
    else {
        Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
        return env.Null();
    }
}

Napi::Value globalPoll(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (!global_client) {
        Napi::Error::New(env, "Global client not initialized").ThrowAsJavaScriptException();
        return env.Null();
    }

    try {
        global_client->poll();
        return Napi::Boolean::New(env, true);
    } catch (const rtms::Exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
}

Napi::Value globalRelease(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (!global_client) {
        return Napi::Boolean::New(env, true); // Already released
    }

    try {
        global_client->release();
        global_client.reset();
        return Napi::Boolean::New(env, true);
    } catch (const rtms::Exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
}

Napi::Value globalUuid(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (!global_client) {
        Napi::Error::New(env, "Global client not initialized").ThrowAsJavaScriptException();
        return env.Null();
    }

    try {
        string uuid = global_client->uuid();
        return Napi::String::New(env, uuid);
    } catch (const rtms::Exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
}

Napi::Value globalStreamId(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (!global_client) {
        Napi::Error::New(env, "Global client not initialized").ThrowAsJavaScriptException();
        return env.Null();
    }

    try {
        string stream_id = global_client->streamId();
        return Napi::String::New(env, stream_id);
    } catch (const rtms::Exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
}

Napi::Value globalSetOnJoinConfirm(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Function argument expected").ThrowAsJavaScriptException();
        return env.Null();
    }

    ensure_global_client();
    Napi::Function callback = info[0].As<Napi::Function>();
    
    if (global_tsfn_join_confirm) {
        global_tsfn_join_confirm.Release();
    }

    global_tsfn_join_confirm = Napi::ThreadSafeFunction::New(
        env, callback, "GlobalJoinConfirmCallback", 0, 1
    );

    global_client->setOnJoinConfirm([](int reason) {
        auto callback = [reason](Napi::Env env, Napi::Function jsCallback) {
            jsCallback.Call({Napi::Number::New(env, reason)});
        };
        global_tsfn_join_confirm.BlockingCall(callback);
    });

    return Napi::Boolean::New(env, true);
}

Napi::Value globalSetOnSessionUpdate(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Function argument expected").ThrowAsJavaScriptException();
        return env.Null();
    }

    ensure_global_client();
    Napi::Function callback = info[0].As<Napi::Function>();
    
    if (global_tsfn_session_update) {
        global_tsfn_session_update.Release();
    }

    global_tsfn_session_update = Napi::ThreadSafeFunction::New(
        env, callback, "GlobalSessionUpdateCallback", 0, 1
    );

    global_client->setOnSessionUpdate([](int op, const rtms::Session& session) {
        auto callback = [op, sessionId = session.sessionId(), statTime = session.statTime(), status = session.status()]
                        (Napi::Env env, Napi::Function jsCallback) {
            Napi::Object sessionObj = Napi::Object::New(env);
            sessionObj.Set("sessionId", Napi::String::New(env, sessionId));
            sessionObj.Set("statTime", Napi::Number::New(env, statTime));
            sessionObj.Set("status", Napi::Number::New(env, status));
            sessionObj.Set("isActive", Napi::Boolean::New(env, status == SESS_STATUS_ACTIVE));
            sessionObj.Set("isPaused", Napi::Boolean::New(env, status == SESS_STATUS_PAUSED));

            jsCallback.Call({Napi::Number::New(env, op), sessionObj});
        };
        global_tsfn_session_update.BlockingCall(callback);
    });

    return Napi::Boolean::New(env, true);
}

Napi::Value globalSetOnUserUpdate(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Function argument expected").ThrowAsJavaScriptException();
        return env.Null();
    }

    ensure_global_client();
    Napi::Function callback = info[0].As<Napi::Function>();
    
    if (global_tsfn_user_update) {
        global_tsfn_user_update.Release();
    }

    global_tsfn_user_update = Napi::ThreadSafeFunction::New(
        env, callback, "GlobalUserUpdateCallback", 0, 1
    );

    global_client->setOnUserUpdate([](int op, const rtms::Participant& participant) {
        auto callback = [op, id = participant.id(), name = participant.name()]
                        (Napi::Env env, Napi::Function jsCallback) {
            Napi::Object participantObj = Napi::Object::New(env);
            participantObj.Set("id", Napi::Number::New(env, id));
            participantObj.Set("name", Napi::String::New(env, name));

            jsCallback.Call({Napi::Number::New(env, op), participantObj});
        };
        global_tsfn_user_update.BlockingCall(callback);
    });

    return Napi::Boolean::New(env, true);
}

Napi::Value globalSetOnAudioData(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Function argument expected").ThrowAsJavaScriptException();
        return env.Null();
    }

    ensure_global_client();
    Napi::Function callback = info[0].As<Napi::Function>();
    
    if (global_tsfn_audio_data) {
        global_tsfn_audio_data.Release();
    }

    global_tsfn_audio_data = Napi::ThreadSafeFunction::New(
        env, callback, "GlobalAudioDataCallback", 0, 1
    );

    global_client->setOnAudioData([](const vector<uint8_t>& data, uint32_t timestamp, const rtms::Metadata& metadata) {
        auto callback = [data, timestamp, userName = metadata.userName(), userId = metadata.userId()]
                       (Napi::Env env, Napi::Function jsCallback) {
            Napi::Buffer<uint8_t> buffer = Napi::Buffer<uint8_t>::Copy(env, data.data(), data.size());
            
            Napi::Object metadataObj = Napi::Object::New(env);
            metadataObj.Set("userName", Napi::String::New(env, userName));
            metadataObj.Set("userId", Napi::Number::New(env, userId));

            jsCallback.Call({buffer, Napi::Number::New(env, data.size()), Napi::Number::New(env, timestamp), metadataObj});
        };
        global_tsfn_audio_data.BlockingCall(callback);
    });

    // Auto-configure for audio
    global_configured_media_types |= SDK_AUDIO;
    if (global_is_configured) {
        reconfigureGlobalMediaTypes();
    }

    return Napi::Boolean::New(env, true);
}

Napi::Value globalSetOnVideoData(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Function argument expected").ThrowAsJavaScriptException();
        return env.Null();
    }

    ensure_global_client();
    Napi::Function callback = info[0].As<Napi::Function>();
    
    if (global_tsfn_video_data) {
        global_tsfn_video_data.Release();
    }

    global_tsfn_video_data = Napi::ThreadSafeFunction::New(
        env, callback, "GlobalVideoDataCallback", 0, 1
    );

    global_client->setOnVideoData([](const vector<uint8_t>& data, uint32_t timestamp, const string& session_id, const rtms::Metadata& metadata) {
        auto callback = [data, timestamp, session_id, userName = metadata.userName(), userId = metadata.userId()]
                       (Napi::Env env, Napi::Function jsCallback) {
            Napi::Buffer<uint8_t> buffer = Napi::Buffer<uint8_t>::Copy(env, data.data(), data.size());
            
            Napi::Object metadataObj = Napi::Object::New(env);
            metadataObj.Set("userName", Napi::String::New(env, userName));
            metadataObj.Set("userId", Napi::Number::New(env, userId));

            jsCallback.Call({buffer, Napi::Number::New(env, data.size()), Napi::Number::New(env, timestamp), Napi::String::New(env, session_id), metadataObj});
        };
        global_tsfn_video_data.BlockingCall(callback);
    });

    // Auto-configure for video
    global_configured_media_types |= SDK_VIDEO;
    if (global_is_configured) {
        reconfigureGlobalMediaTypes();
    }

    return Napi::Boolean::New(env, true);
}

Napi::Value globalSetOnTranscriptData(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Function argument expected").ThrowAsJavaScriptException();
        return env.Null();
    }

    ensure_global_client();
    Napi::Function callback = info[0].As<Napi::Function>();
    
    if (global_tsfn_transcript_data) {
        global_tsfn_transcript_data.Release();
    }

    global_tsfn_transcript_data = Napi::ThreadSafeFunction::New(
        env, callback, "GlobalTranscriptDataCallback", 0, 1
    );

    global_client->setOnTranscriptData([](const vector<uint8_t>& data, uint32_t timestamp, const rtms::Metadata& metadata) {
        auto callback = [data, timestamp, userName = metadata.userName(), userId = metadata.userId()]
                       (Napi::Env env, Napi::Function jsCallback) {
            Napi::Buffer<uint8_t> buffer = Napi::Buffer<uint8_t>::Copy(env, data.data(), data.size());
            
            Napi::Object metadataObj = Napi::Object::New(env);
            metadataObj.Set("userName", Napi::String::New(env, userName));
            metadataObj.Set("userId", Napi::Number::New(env, userId));

            jsCallback.Call({buffer, Napi::Number::New(env, data.size()), Napi::Number::New(env, timestamp), metadataObj});
        };
        global_tsfn_transcript_data.BlockingCall(callback);
    });

    // Auto-configure for transcript
    global_configured_media_types |= SDK_TRANSCRIPT;
    if (global_is_configured) {
        reconfigureGlobalMediaTypes();
    }

    return Napi::Boolean::New(env, true);
}

Napi::Value globalSetOnLeave(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Function argument expected").ThrowAsJavaScriptException();
        return env.Null();
    }

    ensure_global_client();
    Napi::Function callback = info[0].As<Napi::Function>();
    
    if (global_tsfn_leave) {
        global_tsfn_leave.Release();
    }

    global_tsfn_leave = Napi::ThreadSafeFunction::New(
        env, callback, "GlobalLeaveCallback", 0, 1
    );

    global_client->setOnLeave([](int reason) {
        auto callback = [reason](Napi::Env env, Napi::Function jsCallback) {
            jsCallback.Call({Napi::Number::New(env, reason)});
        };
        global_tsfn_leave.BlockingCall(callback);
    });

    return Napi::Boolean::New(env, true);
}

Napi::Object NodeClient::init(Napi::Env env, Napi::Object exports) {
    Napi::HandleScope scope(env);

    Napi::Function func = DefineClass(env, "Client", {
        StaticMethod("initialize", &NodeClient::initialize),
        StaticMethod("uninitialize", &NodeClient::uninitialize),
        InstanceMethod("join", &NodeClient::join),
        InstanceMethod("configure", &NodeClient::configure),
        InstanceMethod("poll", &NodeClient::poll),
        InstanceMethod("release", &NodeClient::release),
        InstanceMethod("uuid", &NodeClient::uuid),
        InstanceMethod("streamId", &NodeClient::streamId),
        InstanceMethod("setAudioParameters", &NodeClient::setAudioParameters),
        InstanceMethod("setVideoParameters", &NodeClient::setVideoParameters),
        InstanceMethod("onJoinConfirm", &NodeClient::setOnJoinConfirm),
        InstanceMethod("onSessionUpdate", &NodeClient::setOnSessionUpdate),
        InstanceMethod("onUserUpdate", &NodeClient::setOnUserUpdate),
        InstanceMethod("onAudioData", &NodeClient::setOnAudioData),
        InstanceMethod("onVideoData", &NodeClient::setOnVideoData),
        InstanceMethod("onTranscriptData", &NodeClient::setOnTranscriptData),
        InstanceMethod("onLeave", &NodeClient::setOnLeave),
    });

    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    env.SetInstanceData(constructor);

    exports.Set("Client", func);
    
    // Module-level functions for global client (singleton mode)
    exports.Set("join", Napi::Function::New(env, globalJoin));
    exports.Set("poll", Napi::Function::New(env, globalPoll));
    exports.Set("release", Napi::Function::New(env, globalRelease));
    exports.Set("uuid", Napi::Function::New(env, globalUuid));
    exports.Set("streamId", Napi::Function::New(env, globalStreamId));
    exports.Set("onJoinConfirm", Napi::Function::New(env, globalSetOnJoinConfirm));
    exports.Set("onSessionUpdate", Napi::Function::New(env, globalSetOnSessionUpdate));
    exports.Set("onUserUpdate", Napi::Function::New(env, globalSetOnUserUpdate));
    exports.Set("onAudioData", Napi::Function::New(env, globalSetOnAudioData));
    exports.Set("onVideoData", Napi::Function::New(env, globalSetOnVideoData));
    exports.Set("onTranscriptData", Napi::Function::New(env, globalSetOnTranscriptData));
    exports.Set("onLeave", Napi::Function::New(env, globalSetOnLeave));
    
    // Export constants
    exports.Set(Napi::String::New(env, "MEDIA_TYPE_AUDIO"), Napi::Number::New(env, SDK_AUDIO));
    exports.Set(Napi::String::New(env, "MEDIA_TYPE_VIDEO"), Napi::Number::New(env, SDK_VIDEO));
    exports.Set(Napi::String::New(env, "MEDIA_TYPE_DESKSHARE"), Napi::Number::New(env, SDK_DESKSHARE));
    exports.Set(Napi::String::New(env, "MEDIA_TYPE_TRANSCRIPT"), Napi::Number::New(env, SDK_TRANSCRIPT));
    exports.Set(Napi::String::New(env, "MEDIA_TYPE_CHAT"), Napi::Number::New(env, SDK_CHAT));
    exports.Set(Napi::String::New(env, "MEDIA_TYPE_ALL"), Napi::Number::New(env, SDK_ALL));

    exports.Set(Napi::String::New(env, "SESSION_EVENT_ADD"), Napi::Number::New(env, SESSION_ADD));
    exports.Set(Napi::String::New(env, "SESSION_EVENT_STOP"), Napi::Number::New(env, SESSION_STOP));
    exports.Set(Napi::String::New(env, "SESSION_EVENT_PAUSE"), Napi::Number::New(env, SESSION_PAUSE));
    exports.Set(Napi::String::New(env, "SESSION_EVENT_RESUME"), Napi::Number::New(env, SESSION_RESUME));

    exports.Set(Napi::String::New(env, "USER_EVENT_JOIN"), Napi::Number::New(env, USER_JOIN));
    exports.Set(Napi::String::New(env, "USER_EVENT_LEAVE"), Napi::Number::New(env, USER_LEAVE));

    exports.Set(Napi::String::New(env, "RTMS_SDK_FAILURE"), Napi::Number::New(env, RTMS_SDK_FAILURE));
    exports.Set(Napi::String::New(env, "RTMS_SDK_OK"), Napi::Number::New(env, RTMS_SDK_OK));
    exports.Set(Napi::String::New(env, "RTMS_SDK_TIMEOUT"), Napi::Number::New(env, RTMS_SDK_TIMEOUT));
    exports.Set(Napi::String::New(env, "RTMS_SDK_NOT_EXIST"), Napi::Number::New(env, RTMS_SDK_NOT_EXIST));
    exports.Set(Napi::String::New(env, "RTMS_SDK_WRONG_TYPE"), Napi::Number::New(env, RTMS_SDK_WRONG_TYPE));
    exports.Set(Napi::String::New(env, "RTMS_SDK_INVALID_STATUS"), Napi::Number::New(env, RTMS_SDK_INVALID_STATUS));
    exports.Set(Napi::String::New(env, "RTMS_SDK_INVALID_ARGS"), Napi::Number::New(env, RTMS_SDK_INVALID_ARGS));

    exports.Set(Napi::String::New(env, "SESS_STATUS_ACTIVE"), Napi::Number::New(env, SESS_STATUS_ACTIVE));
    exports.Set(Napi::String::New(env, "SESS_STATUS_PAUSED"), Napi::Number::New(env, SESS_STATUS_PAUSED));

    return exports;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    return NodeClient::init(env, exports);
}

NODE_API_MODULE(rtms, Init)