#include <napi.h>
#include "rtms.h"
#include <string>
#include <functional>
#include <memory>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <iostream>

class NodeClient : public Napi::ObjectWrap<NodeClient> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    NodeClient(const Napi::CallbackInfo& info);
    ~NodeClient();

private:
    static Napi::Value Initialize(const Napi::CallbackInfo& info);
    static Napi::Value Uninitialize(const Napi::CallbackInfo& info);

    Napi::Value Join(const Napi::CallbackInfo& info);
    Napi::Value Configure(const Napi::CallbackInfo& info);
    Napi::Value Poll(const Napi::CallbackInfo& info);
    Napi::Value Release(const Napi::CallbackInfo& info);
    Napi::Value GetMeetingUuid(const Napi::CallbackInfo& info);
    Napi::Value GetRtmsStreamId(const Napi::CallbackInfo& info);

    Napi::Value SetAudioParameters(const Napi::CallbackInfo& info);
    Napi::Value SetVideoParameters(const Napi::CallbackInfo& info);

    Napi::Value SetJoinConfirm(const Napi::CallbackInfo& info);
    Napi::Value SetSessionUpdate(const Napi::CallbackInfo& info);
    Napi::Value SetUserUpdate(const Napi::CallbackInfo& info);
    Napi::Value SetAudioData(const Napi::CallbackInfo& info);
    Napi::Value SetVideoData(const Napi::CallbackInfo& info);
    Napi::Value SetTranscriptData(const Napi::CallbackInfo& info);
    Napi::Value SetLeave(const Napi::CallbackInfo& info);

    Napi::Value StartPolling(const Napi::CallbackInfo& info);
    Napi::Value StopPolling(const Napi::CallbackInfo& info);

    void ReconfigureMediaTypes();

    std::unique_ptr<rtms::Client> client_;
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

    std::unique_ptr<std::thread> polling_thread_;
    bool stop_polling_;
};

Napi::Value NodeClient::Poll(const Napi::CallbackInfo& info) {
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

Napi::Value NodeClient::Release(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    try {
        if (polling_thread_) {
            stop_polling_ = true;
            polling_thread_->join();
            polling_thread_.reset();
        }
        
        client_->release();
        return Napi::Boolean::New(env, true);
    } catch (const rtms::Exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
}

Napi::Value NodeClient::GetMeetingUuid(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    try {
        std::string uuid = client_->uuid();
        return Napi::String::New(env, uuid);
    } catch (const rtms::Exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
}

Napi::Value NodeClient::GetRtmsStreamId(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    try {
        std::string stream_id = client_->streamId();
        return Napi::String::New(env, stream_id);
    } catch (const rtms::Exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
}

Napi::Value NodeClient::SetAudioParameters(const Napi::CallbackInfo& info) {
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
        ReconfigureMediaTypes();
    }

    return Napi::Boolean::New(env, true);
}

Napi::Value NodeClient::SetVideoParameters(const Napi::CallbackInfo& info) {
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
        ReconfigureMediaTypes();
    }

    return Napi::Boolean::New(env, true);
}

Napi::Value NodeClient::SetJoinConfirm(const Napi::CallbackInfo& info) {
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

    client_->setJoinConfirm([this](int reason) {
        auto callback = [reason](Napi::Env env, Napi::Function jsCallback) {
            jsCallback.Call({Napi::Number::New(env, reason)});
        };
        tsfn_join_confirm_.BlockingCall(callback);
    });

    return Napi::Boolean::New(env, true);
}

Napi::Value NodeClient::SetSessionUpdate(const Napi::CallbackInfo& info) {
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

    client_->setSessionUpdate([this](int op, const rtms::Session& session) {
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

Napi::Value NodeClient::SetUserUpdate(const Napi::CallbackInfo& info) {
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

    client_->setUserUpdate([this](int op, const rtms::Participant& participant) {
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

Napi::Value NodeClient::SetAudioData(const Napi::CallbackInfo& info) {
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

    client_->setAudioData([this](const std::vector<uint8_t>& data, uint32_t timestamp, const rtms::Metadata& metadata) {
        auto callback = [data, timestamp, userName = metadata.userName(), userId = metadata.userId()]
                       (Napi::Env env, Napi::Function jsCallback) {
            Napi::Buffer<uint8_t> buffer = Napi::Buffer<uint8_t>::Copy(env, data.data(), data.size());
            
            Napi::Object metadataObj = Napi::Object::New(env);
            metadataObj.Set("userName", Napi::String::New(env, userName));
            metadataObj.Set("userId", Napi::Number::New(env, userId));

            jsCallback.Call({buffer, Napi::Number::New(env, timestamp), metadataObj});
        };
        tsfn_audio_data_.BlockingCall(callback);
    });

    // Auto-configure for audio
    configured_media_types_ |= SDK_AUDIO;
    if (is_configured_) {
        ReconfigureMediaTypes();
    }

    return Napi::Boolean::New(env, true);
}

Napi::Value NodeClient::SetVideoData(const Napi::CallbackInfo& info) {
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

    client_->setVideoData([this](const std::vector<uint8_t>& data, uint32_t timestamp, const std::string& session_id, const rtms::Metadata& metadata) {
        auto callback = [data, timestamp, session_id, userName = metadata.userName(), userId = metadata.userId()]
                       (Napi::Env env, Napi::Function jsCallback) {
            Napi::Buffer<uint8_t> buffer = Napi::Buffer<uint8_t>::Copy(env, data.data(), data.size());
            
            Napi::Object metadataObj = Napi::Object::New(env);
            metadataObj.Set("userName", Napi::String::New(env, userName));
            metadataObj.Set("userId", Napi::Number::New(env, userId));

            jsCallback.Call({buffer, Napi::Number::New(env, timestamp), Napi::String::New(env, session_id), metadataObj});
        };
        tsfn_video_data_.BlockingCall(callback);
    });

    // Auto-configure for video
    configured_media_types_ |= SDK_VIDEO;
    if (is_configured_) {
        ReconfigureMediaTypes();
    }

    return Napi::Boolean::New(env, true);
}

Napi::Value NodeClient::SetTranscriptData(const Napi::CallbackInfo& info) {
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

    client_->setTranscriptData([this](const std::vector<uint8_t>& data, uint32_t timestamp, const rtms::Metadata& metadata) {
        auto callback = [data, timestamp, userName = metadata.userName(), userId = metadata.userId()]
                       (Napi::Env env, Napi::Function jsCallback) {
            Napi::Buffer<uint8_t> buffer = Napi::Buffer<uint8_t>::Copy(env, data.data(), data.size());
            
            Napi::Object metadataObj = Napi::Object::New(env);
            metadataObj.Set("userName", Napi::String::New(env, userName));
            metadataObj.Set("userId", Napi::Number::New(env, userId));

            jsCallback.Call({buffer, Napi::Number::New(env, timestamp), metadataObj});
        };
        tsfn_transcript_data_.BlockingCall(callback);
    });

    // Auto-configure for transcript
    configured_media_types_ |= SDK_TRANSCRIPT;
    if (is_configured_) {
        ReconfigureMediaTypes();
    }

    return Napi::Boolean::New(env, true);
}

Napi::Value NodeClient::SetLeave(const Napi::CallbackInfo& info) {
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

    client_->setLeave([this](int reason) {
        auto callback = [reason](Napi::Env env, Napi::Function jsCallback) {
            jsCallback.Call({Napi::Number::New(env, reason)});
        };
        tsfn_leave_.BlockingCall(callback);
    });

    return Napi::Boolean::New(env, true);
}

Napi::Value NodeClient::StartPolling(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (polling_thread_) {
        return Napi::Boolean::New(env, true); // Already polling
    }

    stop_polling_ = false;
    
    try {
        while (!stop_polling_) {
            client_->poll();
            //std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Polling interval
        }
    } catch (const rtms::Exception& e) {
        std::cerr << "Polling thread error: " << e.what() << std::endl;
        stop_polling_ = true;
    }

/*     polling_thread_ = std::make_unique<std::thread>([this]() {
 
    }); */

    return Napi::Boolean::New(env, true);
}

Napi::Value NodeClient::StopPolling(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (!polling_thread_) {
        return Napi::Boolean::New(env, true); // Not polling
    }

    stop_polling_ = true;
    polling_thread_->join();
    polling_thread_.reset();

    return Napi::Boolean::New(env, true);
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    return NodeClient::Init(env, exports);
}

NODE_API_MODULE(rtms, Init);

Napi::Object NodeClient::Init(Napi::Env env, Napi::Object exports) {
    Napi::HandleScope scope(env);

    Napi::Function func = DefineClass(env, "Client", {
        StaticMethod("initialize", &NodeClient::Initialize),
        StaticMethod("uninitialize", &NodeClient::Uninitialize),
        InstanceMethod("join", &NodeClient::Join),
        InstanceMethod("configure", &NodeClient::Configure),
        InstanceMethod("poll", &NodeClient::Poll),
        InstanceMethod("release", &NodeClient::Release),
        InstanceMethod("getMeetingUuid", &NodeClient::GetMeetingUuid),
        InstanceMethod("getRtmsStreamId", &NodeClient::GetRtmsStreamId),
        InstanceMethod("setAudioParameters", &NodeClient::SetAudioParameters),
        InstanceMethod("setVideoParameters", &NodeClient::SetVideoParameters),
        InstanceMethod("onJoinConfirm", &NodeClient::SetJoinConfirm),
        InstanceMethod("setSessionUpdateCallback", &NodeClient::SetSessionUpdate),
        InstanceMethod("setUserUpdateCallback", &NodeClient::SetUserUpdate),
        InstanceMethod("onAudioData", &NodeClient::SetAudioData),
        InstanceMethod("setVideoDataCallback", &NodeClient::SetVideoData),
        InstanceMethod("setTranscriptDataCallback", &NodeClient::SetTranscriptData),
        InstanceMethod("setLeaveCallback", &NodeClient::SetLeave),
        InstanceMethod("startPolling", &NodeClient::StartPolling),
        InstanceMethod("stopPolling", &NodeClient::StopPolling),
    });

    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    env.SetInstanceData(constructor);

    exports.Set("Client", func);
    
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

NodeClient::NodeClient(const Napi::CallbackInfo& info) 
    : Napi::ObjectWrap<NodeClient>(info), 
      configured_media_types_(0),
      is_configured_(false),
      stop_polling_(false) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    try {
        client_ = std::make_unique<rtms::Client>();
    } catch (const rtms::Exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
    }
}

NodeClient::~NodeClient() {
    if (polling_thread_) {
        stop_polling_ = true;
        polling_thread_->join();
        polling_thread_.reset();
    }

    if (tsfn_join_confirm_) tsfn_join_confirm_.Release();
    if (tsfn_session_update_) tsfn_session_update_.Release();
    if (tsfn_user_update_) tsfn_user_update_.Release();
    if (tsfn_audio_data_) tsfn_audio_data_.Release();
    if (tsfn_video_data_) tsfn_video_data_.Release();
    if (tsfn_transcript_data_) tsfn_transcript_data_.Release();
    if (tsfn_leave_) tsfn_leave_.Release();
}

Napi::Value NodeClient::Initialize(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    try {
        std::string ca_path = "";
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

Napi::Value NodeClient::Uninitialize(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    rtms::Client::uninitialize();
    return Napi::Boolean::New(env, true);
}

void NodeClient::ReconfigureMediaTypes() {
    if (!client_ || !is_configured_) return;
    
    try {
        client_->configure(media_params_, configured_media_types_, false);
    } catch (const rtms::Exception& e) {
        // Log error but don't throw - this is called from callback setters
        std::cerr << "Failed to reconfigure media types: " << e.what() << std::endl;
    }
}

Napi::Value NodeClient::Configure(const Napi::CallbackInfo& info) {
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

Napi::Value NodeClient::Join(const Napi::CallbackInfo& info) {
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

    std::string meeting_uuid = info[0].As<Napi::String>();
    std::string rtms_stream_id = info[1].As<Napi::String>();
    std::string signature = info[2].As<Napi::String>();
    std::string server_url = info[3].As<Napi::String>();
    
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