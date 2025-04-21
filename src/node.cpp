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

    unique_ptr<rtms::Client> client_;
    Napi::ThreadSafeFunction tsfn_join_confirm_;
    Napi::ThreadSafeFunction tsfn_session_update_;
    Napi::ThreadSafeFunction tsfn_user_update_;
    Napi::ThreadSafeFunction tsfn_audio_data_;
    Napi::ThreadSafeFunction tsfn_video_data_;
    Napi::ThreadSafeFunction tsfn_transcript_data_;
    Napi::ThreadSafeFunction tsfn_leave_;
    
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

    client_->setAudioParameters(audio_params);
    
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

    client_->setVideoParameters(video_params);
    
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
// Inside NodeClient class implementation in node.cpp

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
    : Napi::ObjectWrap<NodeClient>(info) {
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
        client_->join(meeting_uuid, rtms_stream_id, signature, server_url, timeout);
        return Napi::Boolean::New(env, true);
    } catch (const rtms::Exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Null();
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

    // ===== Audio Parameters =====

    // Audio Content Type
    Napi::Object audioContentType = Napi::Object::New(env);
    audioContentType.Set("UNDEFINED", Napi::Number::New(env, 0));
    audioContentType.Set("RTP", Napi::Number::New(env, 1));
    audioContentType.Set("RAW_AUDIO", Napi::Number::New(env, 2));
    audioContentType.Set("FILE_STREAM", Napi::Number::New(env, 4));
    audioContentType.Set("TEXT", Napi::Number::New(env, 5));
    exports.Set("AudioContentType", audioContentType);

    // Audio Codec
    Napi::Object audioCodec = Napi::Object::New(env);
    audioCodec.Set("UNDEFINED", Napi::Number::New(env, 0));
    audioCodec.Set("L16", Napi::Number::New(env, 1));
    audioCodec.Set("G711", Napi::Number::New(env, 2));
    audioCodec.Set("G722", Napi::Number::New(env, 3));
    audioCodec.Set("OPUS", Napi::Number::New(env, 4));
    exports.Set("AudioCodec", audioCodec);

    // Audio Sample Rate
    Napi::Object audioSampleRate = Napi::Object::New(env);
    audioSampleRate.Set("SR_8K", Napi::Number::New(env, 0));
    audioSampleRate.Set("SR_16K", Napi::Number::New(env, 1));
    audioSampleRate.Set("SR_32K", Napi::Number::New(env, 2));
    audioSampleRate.Set("SR_48K", Napi::Number::New(env, 3));
    exports.Set("AudioSampleRate", audioSampleRate);

    // Audio Channel
    Napi::Object audioChannel = Napi::Object::New(env);
    audioChannel.Set("MONO", Napi::Number::New(env, 1));
    audioChannel.Set("STEREO", Napi::Number::New(env, 2));
    exports.Set("AudioChannel", audioChannel);

    // Audio Data Option
    Napi::Object audioDataOption = Napi::Object::New(env);
    audioDataOption.Set("UNDEFINED", Napi::Number::New(env, 0));
    audioDataOption.Set("AUDIO_MIXED_STREAM", Napi::Number::New(env, 1));
    audioDataOption.Set("AUDIO_MULTI_STREAMS", Napi::Number::New(env, 2));
    exports.Set("AudioDataOption", audioDataOption);

    // ===== Video Parameters =====

    // Video Content Type
    Napi::Object videoContentType = Napi::Object::New(env);
    videoContentType.Set("UNDEFINED", Napi::Number::New(env, 0));
    videoContentType.Set("RTP", Napi::Number::New(env, 1));
    videoContentType.Set("RAW_VIDEO", Napi::Number::New(env, 3));
    videoContentType.Set("FILE_STREAM", Napi::Number::New(env, 4));
    videoContentType.Set("TEXT", Napi::Number::New(env, 5));
    exports.Set("VideoContentType", videoContentType);

    // Video Codec
    Napi::Object videoCodec = Napi::Object::New(env);
    videoCodec.Set("UNDEFINED", Napi::Number::New(env, 0));
    videoCodec.Set("JPG", Napi::Number::New(env, 5));
    videoCodec.Set("PNG", Napi::Number::New(env, 6));
    videoCodec.Set("H264", Napi::Number::New(env, 7));
    exports.Set("VideoCodec", videoCodec);

    // Video Resolution
    Napi::Object videoResolution = Napi::Object::New(env);
    videoResolution.Set("SD", Napi::Number::New(env, 1));
    videoResolution.Set("HD", Napi::Number::New(env, 2));
    videoResolution.Set("FHD", Napi::Number::New(env, 3));
    videoResolution.Set("QHD", Napi::Number::New(env, 4));
    exports.Set("VideoResolution", videoResolution);

    // Video Data Option
    Napi::Object videoDataOption = Napi::Object::New(env);
    videoDataOption.Set("UNDEFINED", Napi::Number::New(env, 0));
    videoDataOption.Set("VIDEO_SINGLE_ACTIVE_STREAM", Napi::Number::New(env, 3));
    videoDataOption.Set("VIDEO_MIXED_SPEAKER_VIEW", Napi::Number::New(env, 4));
    videoDataOption.Set("VIDEO_MIXED_GALLERY_VIEW", Napi::Number::New(env, 5));
    exports.Set("VideoDataOption", videoDataOption);

    // ===== Media Types =====

    // Media Data Type 
    Napi::Object mediaDataType = Napi::Object::New(env);
    mediaDataType.Set("UNDEFINED", Napi::Number::New(env, 0));
    mediaDataType.Set("AUDIO", Napi::Number::New(env, 1));
    mediaDataType.Set("VIDEO", Napi::Number::New(env, 2));
    mediaDataType.Set("DESKSHARE", Napi::Number::New(env, 4));
    mediaDataType.Set("TRANSCRIPT", Napi::Number::New(env, 8));
    mediaDataType.Set("CHAT", Napi::Number::New(env, 16));
    mediaDataType.Set("ALL", Napi::Number::New(env, 32));
    exports.Set("MediaDataType", mediaDataType);

    // ===== Session States =====

    // Session State
    Napi::Object sessionState = Napi::Object::New(env);
    sessionState.Set("INACTIVE", Napi::Number::New(env, 0));
    sessionState.Set("INITIALIZE", Napi::Number::New(env, 1));
    sessionState.Set("STARTED", Napi::Number::New(env, 2));
    sessionState.Set("PAUSED", Napi::Number::New(env, 3));
    sessionState.Set("RESUMED", Napi::Number::New(env, 4));
    sessionState.Set("STOPPED", Napi::Number::New(env, 5));
    exports.Set("SessionState", sessionState);

    // Stream State
    Napi::Object streamState = Napi::Object::New(env);
    streamState.Set("INACTIVE", Napi::Number::New(env, 0));
    streamState.Set("ACTIVE", Napi::Number::New(env, 1));
    streamState.Set("INTERRUPTED", Napi::Number::New(env, 2));
    streamState.Set("TERMINATING", Napi::Number::New(env, 3));
    streamState.Set("TERMINATED", Napi::Number::New(env, 4));
    exports.Set("StreamState", streamState);

    // Event Type
    Napi::Object eventType = Napi::Object::New(env);
    eventType.Set("UNDEFINED", Napi::Number::New(env, 0));
    eventType.Set("FIRST_PACKET_TIMESTAMP", Napi::Number::New(env, 1));
    eventType.Set("ACTIVE_SPEAKER_CHANGE", Napi::Number::New(env, 2));
    eventType.Set("PARTICIPANT_JOIN", Napi::Number::New(env, 3));
    eventType.Set("PARTICIPANT_LEAVE", Napi::Number::New(env, 4));
    exports.Set("EventType", eventType);

    // Message Type
    Napi::Object messageType = Napi::Object::New(env);
    messageType.Set("UNDEFINED", Napi::Number::New(env, 0));
    messageType.Set("SIGNALING_HAND_SHAKE_REQ", Napi::Number::New(env, 1));
    messageType.Set("SIGNALING_HAND_SHAKE_RESP", Napi::Number::New(env, 2));
    messageType.Set("DATA_HAND_SHAKE_REQ", Napi::Number::New(env, 3));
    messageType.Set("DATA_HAND_SHAKE_RESP", Napi::Number::New(env, 4));
    messageType.Set("EVENT_SUBSCRIPTION", Napi::Number::New(env, 5));
    messageType.Set("EVENT_UPDATE", Napi::Number::New(env, 6));
    messageType.Set("CLIENT_READY_ACK", Napi::Number::New(env, 7));
    messageType.Set("STREAM_STATE_UPDATE", Napi::Number::New(env, 8));
    messageType.Set("SESSION_STATE_UPDATE", Napi::Number::New(env, 9));
    messageType.Set("SESSION_STATE_REQ", Napi::Number::New(env, 10));
    messageType.Set("SESSION_STATE_RESP", Napi::Number::New(env, 11));
    messageType.Set("KEEP_ALIVE_REQ", Napi::Number::New(env, 12));
    messageType.Set("KEEP_ALIVE_RESP", Napi::Number::New(env, 13));
    messageType.Set("MEDIA_DATA_AUDIO", Napi::Number::New(env, 14));
    messageType.Set("MEDIA_DATA_VIDEO", Napi::Number::New(env, 15));
    messageType.Set("MEDIA_DATA_SHARE", Napi::Number::New(env, 16));
    messageType.Set("MEDIA_DATA_TRANSCRIPT", Napi::Number::New(env, 17));
    messageType.Set("MEDIA_DATA_CHAT", Napi::Number::New(env, 18));
    exports.Set("MessageType", messageType);

    // Stop Reason
    Napi::Object stopReason = Napi::Object::New(env);
    stopReason.Set("UNDEFINED", Napi::Number::New(env, 0));
    stopReason.Set("STOP_BC_HOST_TRIGGERED", Napi::Number::New(env, 1));
    stopReason.Set("STOP_BC_USER_TRIGGERED", Napi::Number::New(env, 2));
    stopReason.Set("STOP_BC_USER_LEFT", Napi::Number::New(env, 3));
    stopReason.Set("STOP_BC_USER_EJECTED", Napi::Number::New(env, 4));
    stopReason.Set("STOP_BC_APP_DISABLED_BY_HOST", Napi::Number::New(env, 5));
    stopReason.Set("STOP_BC_MEETING_ENDED", Napi::Number::New(env, 6));
    stopReason.Set("STOP_BC_STREAM_CANCELED", Napi::Number::New(env, 7));
    stopReason.Set("STOP_BC_STREAM_REVOKED", Napi::Number::New(env, 8));
    stopReason.Set("STOP_BC_ALL_APPS_DISABLED", Napi::Number::New(env, 9));
    stopReason.Set("STOP_BC_INTERNAL_EXCEPTION", Napi::Number::New(env, 10));
    stopReason.Set("STOP_BC_CONNECTION_TIMEOUT", Napi::Number::New(env, 11));
    stopReason.Set("STOP_BC_MEETING_CONNECTION_INTERRUPTED", Napi::Number::New(env, 12));
    stopReason.Set("STOP_BC_SIGNAL_CONNECTION_INTERRUPTED", Napi::Number::New(env, 13));
    stopReason.Set("STOP_BC_DATA_CONNECTION_INTERRUPTED", Napi::Number::New(env, 14));
    stopReason.Set("STOP_BC_SIGNAL_CONNECTION_CLOSED_ABNORMALLY", Napi::Number::New(env, 15));
    stopReason.Set("STOP_BC_DATA_CONNECTION_CLOSED_ABNORMALLY", Napi::Number::New(env, 16));
    stopReason.Set("STOP_BC_EXIT_SIGNAL", Napi::Number::New(env, 17));
    stopReason.Set("STOP_BC_AUTHENTICATION_FAILURE", Napi::Number::New(env, 18));
    exports.Set("StopReason", stopReason);

    return exports;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    return NodeClient::init(env, exports);
}

NODE_API_MODULE(rtms, Init)