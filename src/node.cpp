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

static Napi::Object buildMetadataObj(Napi::Env env, const rtms::Metadata& metadata) {
    Napi::Object obj = Napi::Object::New(env);
    obj.Set("userName", Napi::String::New(env, metadata.userName()));
    obj.Set("userId", Napi::Number::New(env, metadata.userId()));
    obj.Set("startTs", Napi::Number::New(env, static_cast<double>(metadata.startTs())));
    obj.Set("endTs", Napi::Number::New(env, static_cast<double>(metadata.endTs())));

    const auto& aii = metadata.aiInterpreter();
    Napi::Object aiObj = Napi::Object::New(env);
    aiObj.Set("lid", Napi::Number::New(env, aii.lid()));
    aiObj.Set("timestamp", Napi::Number::New(env, static_cast<double>(aii.timestamp())));
    aiObj.Set("channelNum", Napi::Number::New(env, aii.channelNum()));
    aiObj.Set("sampleRate", Napi::Number::New(env, aii.sampleRate()));

    Napi::Array targets = Napi::Array::New(env, aii.targets().size());
    for (size_t i = 0; i < aii.targets().size(); ++i) {
        const auto& t = aii.targets()[i];
        Napi::Object tObj = Napi::Object::New(env);
        tObj.Set("lid", Napi::Number::New(env, t.lid()));
        tObj.Set("toneId", Napi::Number::New(env, t.toneId()));
        tObj.Set("voiceId", Napi::String::New(env, t.voiceId()));
        tObj.Set("engine", Napi::String::New(env, t.engine()));
        targets.Set(i, tObj);
    }
    aiObj.Set("targets", targets);
    obj.Set("aiInterpreter", aiObj);
    return obj;
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

    Napi::Value enableVideo(const Napi::CallbackInfo& info);
    Napi::Value enableAudio(const Napi::CallbackInfo& info);
    Napi::Value enableTranscript(const Napi::CallbackInfo& info);

    Napi::Value setDeskshareParams(const Napi::CallbackInfo& info);
    Napi::Value setAudioParams(const Napi::CallbackInfo& info);
    Napi::Value setVideoParams(const Napi::CallbackInfo& info);
    Napi::Value setTranscriptParams(const Napi::CallbackInfo& info);
    Napi::Value setProxy(const Napi::CallbackInfo& info);

    Napi::Value setOnJoinConfirm(const Napi::CallbackInfo& info);
    Napi::Value setOnSessionUpdate(const Napi::CallbackInfo& info);
    Napi::Value setOnUserUpdate(const Napi::CallbackInfo& info);
    Napi::Value setOnDeskshareData(const Napi::CallbackInfo& info);
    Napi::Value setOnAudioData(const Napi::CallbackInfo& info);
    Napi::Value setOnVideoData(const Napi::CallbackInfo& info);
    Napi::Value setOnTranscriptData(const Napi::CallbackInfo& info);
    Napi::Value setOnLeave(const Napi::CallbackInfo& info);
    Napi::Value setOnEventEx(const Napi::CallbackInfo& info);

    Napi::Value subscribeEvent(const Napi::CallbackInfo& info);
    Napi::Value unsubscribeEvent(const Napi::CallbackInfo& info);

    Napi::Value subscribeVideo(const Napi::CallbackInfo& info);
    Napi::Value setOnParticipantVideo(const Napi::CallbackInfo& info);
    Napi::Value setOnVideoSubscribed(const Napi::CallbackInfo& info);

    unique_ptr<rtms::Client> client_;
    Napi::ThreadSafeFunction tsfn_join_confirm_;
    Napi::ThreadSafeFunction tsfn_session_update_;
    Napi::ThreadSafeFunction tsfn_user_update_;
    Napi::ThreadSafeFunction tsfn_ds_data_;
    Napi::ThreadSafeFunction tsfn_audio_data_;
    Napi::ThreadSafeFunction tsfn_video_data_;
    Napi::ThreadSafeFunction tsfn_transcript_data_;
    Napi::ThreadSafeFunction tsfn_leave_;
    Napi::ThreadSafeFunction tsfn_event_ex_;
    Napi::ThreadSafeFunction tsfn_participant_video_;
    Napi::ThreadSafeFunction tsfn_video_subscribed_;
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

rtms::DeskshareParams readDsParams(const Napi::Object& params) {
    rtms::DeskshareParams ds_params;

    if (params.Has("contentType") && params.Get("contentType").IsNumber()) {
        ds_params.setContentType(params.Get("contentType").As<Napi::Number>().Int32Value());
    }
    
    if (params.Has("codec") && params.Get("codec").IsNumber()) {
        ds_params.setCodec(params.Get("codec").As<Napi::Number>().Int32Value());
    }
    
    if (params.Has("resolution") && params.Get("resolution").IsNumber()) {
        ds_params.setResolution(params.Get("resolution").As<Napi::Number>().Int32Value());
    }

    if (params.Has("fps") && params.Get("fps").IsNumber()) {
        ds_params.setFps(params.Get("fps").As<Napi::Number>().Int32Value());
    }
    
    return ds_params;
}

Napi::Value NodeClient::setDeskshareParams(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);
    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "Object argument expected").ThrowAsJavaScriptException();
        return env.Null();
    }
    Napi::Object params = info[0].As<Napi::Object>();
    auto ds_params = readDsParams(params);

    try {
        client_->setDeskshareParams(ds_params);
    } catch (const std::invalid_argument& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }

    return Napi::Boolean::New(env, true);
}

rtms::AudioParams readAudioParams(const Napi::Object& params) {
    // Start with sensible defaults (contentType=RAW_AUDIO, codec=OPUS, etc.)
    // Users can override any individual field without setting all fields
    rtms::AudioParams audio_params;

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

    return audio_params;
}

Napi::Value NodeClient::setAudioParams(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "Object argument expected").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Object params = info[0].As<Napi::Object>();
    auto audio_params = readAudioParams(params);

    try {
        client_->setAudioParams(audio_params);
    } catch (const std::invalid_argument& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }

    return Napi::Boolean::New(env, true);
}

rtms::VideoParams readVideoParams(const Napi::Object& params) {

    rtms::VideoParams video_params;

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

    return video_params;

}
Napi::Value NodeClient::setVideoParams(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "Object argument expected").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Object params = info[0].As<Napi::Object>();
    auto video_params = readVideoParams(params);

    try {
        client_->setVideoParams(video_params);
    } catch (const std::invalid_argument& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }

    return Napi::Boolean::New(env, true);
}

rtms::TranscriptParams readTranscriptParams(const Napi::Object& params) {
    rtms::TranscriptParams transcript_params;

    if (params.Has("srcLanguage") && params.Get("srcLanguage").IsNumber()) {
        transcript_params.setSrcLanguage(params.Get("srcLanguage").As<Napi::Number>().Int32Value());
    }

    if (params.Has("enableLid") && params.Get("enableLid").IsBoolean()) {
        transcript_params.setEnableLid(params.Get("enableLid").As<Napi::Boolean>().Value());
    }

    return transcript_params;
}

Napi::Value NodeClient::setTranscriptParams(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "Object argument expected").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Object params = info[0].As<Napi::Object>();
    auto transcript_params = readTranscriptParams(params);

    try {
        client_->setTranscriptParams(transcript_params);
    } catch (const std::invalid_argument& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }

    return Napi::Boolean::New(env, true);
}

Napi::Value NodeClient::setProxy(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsString()) {
        Napi::TypeError::New(env, "Two string arguments expected: proxy_type, proxy_url").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string proxy_type = info[0].As<Napi::String>().Utf8Value();
    std::string proxy_url  = info[1].As<Napi::String>().Utf8Value();

    try {
        client_->setProxy(proxy_type, proxy_url);
    } catch (const rtms::Exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Null();
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
        auto callback = [op, sessionId = session.sessionId(), streamId = session.streamId(), meetingId = session.meetingId(), statTime = session.statTime(), status = session.status()]
                        (Napi::Env env, Napi::Function jsCallback) {
            Napi::Object sessionObj = Napi::Object::New(env);
            sessionObj.Set("sessionId", Napi::String::New(env, sessionId));
            sessionObj.Set("streamId", Napi::String::New(env, streamId));
            sessionObj.Set("meetingId", Napi::String::New(env, meetingId));
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

Napi::Value NodeClient::setOnDeskshareData(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Function argument expected").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Function callback = info[0].As<Napi::Function>();

    if (tsfn_ds_data_) {
        tsfn_ds_data_.Release();
    }

    tsfn_ds_data_ = Napi::ThreadSafeFunction::New(
        env, callback, "DeskshareDataCallback", 0, 1
    );

    client_->setOnDeskshareData([this](const vector<uint8_t>& data, uint64_t timestamp, const rtms::Metadata& metadata) {
        auto callback = [data, timestamp, metadata]
                       (Napi::Env env, Napi::Function jsCallback) {
            Napi::Buffer<uint8_t> buffer = Napi::Buffer<uint8_t>::Copy(env, data.data(), data.size());
            jsCallback.Call({buffer, Napi::Number::New(env, data.size()), Napi::Number::New(env, timestamp), buildMetadataObj(env, metadata)});
        };
        tsfn_ds_data_.BlockingCall(callback);
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

    client_->setOnAudioData([this](const vector<uint8_t>& data, uint64_t timestamp, const rtms::Metadata& metadata) {
        auto callback = [data, timestamp, metadata]
                       (Napi::Env env, Napi::Function jsCallback) {
            Napi::Buffer<uint8_t> buffer = Napi::Buffer<uint8_t>::Copy(env, data.data(), data.size());
            jsCallback.Call({buffer, Napi::Number::New(env, data.size()), Napi::Number::New(env, timestamp), buildMetadataObj(env, metadata)});
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

    client_->setOnVideoData([this](const vector<uint8_t>& data, uint64_t timestamp, const rtms::Metadata& metadata) {
        auto callback = [data, timestamp, metadata]
                       (Napi::Env env, Napi::Function jsCallback) {
            Napi::Buffer<uint8_t> buffer = Napi::Buffer<uint8_t>::Copy(env, data.data(), data.size());
            jsCallback.Call({buffer, Napi::Number::New(env, data.size()), Napi::Number::New(env, timestamp), buildMetadataObj(env, metadata)});
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

    client_->setOnTranscriptData([this](const vector<uint8_t>& data, uint64_t timestamp, const rtms::Metadata& metadata) {
        auto callback = [data, timestamp, metadata]
                       (Napi::Env env, Napi::Function jsCallback) {
            Napi::Buffer<uint8_t> buffer = Napi::Buffer<uint8_t>::Copy(env, data.data(), data.size());
            jsCallback.Call({buffer, Napi::Number::New(env, data.size()), Napi::Number::New(env, timestamp), buildMetadataObj(env, metadata)});
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

Napi::Value NodeClient::setOnEventEx(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Function argument expected").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Function callback = info[0].As<Napi::Function>();
    
    if (tsfn_event_ex_) {
        tsfn_event_ex_.Release();
    }

    tsfn_event_ex_ = Napi::ThreadSafeFunction::New(
        env, callback, "EventExCallback", 0, 1
    );

    client_->setOnEventEx([this](const string& eventData) {
        auto callback = [eventData](Napi::Env env, Napi::Function jsCallback) {
            jsCallback.Call({Napi::String::New(env, eventData)});
        };
        tsfn_event_ex_.BlockingCall(callback);
    });

    return Napi::Boolean::New(env, true);
}

Napi::Value NodeClient::subscribeEvent(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsArray()) {
        Napi::TypeError::New(env, "Array of event types expected").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Array arr = info[0].As<Napi::Array>();
    std::vector<int> events;
    for (uint32_t i = 0; i < arr.Length(); i++) {
        events.push_back(arr.Get(i).As<Napi::Number>().Int32Value());
    }

    try {
        client_->subscribeEvent(events);
        return Napi::Boolean::New(env, true);
    } catch (const rtms::Exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
}

Napi::Value NodeClient::unsubscribeEvent(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsArray()) {
        Napi::TypeError::New(env, "Array of event types expected").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Array arr = info[0].As<Napi::Array>();
    std::vector<int> events;
    for (uint32_t i = 0; i < arr.Length(); i++) {
        events.push_back(arr.Get(i).As<Napi::Number>().Int32Value());
    }

    try {
        client_->unsubscribeEvent(events);
        return Napi::Boolean::New(env, true);
    } catch (const rtms::Exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
}

Napi::Value NodeClient::subscribeVideo(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsBoolean()) {
        Napi::TypeError::New(env, "Two arguments expected: userId (number), subscribe (boolean)").ThrowAsJavaScriptException();
        return env.Null();
    }

    int user_id = info[0].As<Napi::Number>().Int32Value();
    bool subscribe = info[1].As<Napi::Boolean>().Value();

    try {
        client_->subscribeVideo(user_id, subscribe);
        return Napi::Boolean::New(env, true);
    } catch (const rtms::Exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
}

Napi::Value NodeClient::setOnParticipantVideo(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Function argument expected").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Function callback = info[0].As<Napi::Function>();

    if (tsfn_participant_video_) {
        tsfn_participant_video_.Release();
    }

    tsfn_participant_video_ = Napi::ThreadSafeFunction::New(
        env, callback, "ParticipantVideoCallback", 0, 1
    );

    client_->setOnParticipantVideo([this](const std::vector<int>& users, bool is_on) {
        auto callback = [users, is_on](Napi::Env env, Napi::Function jsCallback) {
            Napi::Array arr = Napi::Array::New(env, users.size());
            for (size_t i = 0; i < users.size(); i++) {
                arr.Set(static_cast<uint32_t>(i), Napi::Number::New(env, users[i]));
            }
            jsCallback.Call({arr, Napi::Boolean::New(env, is_on)});
        };
        tsfn_participant_video_.BlockingCall(callback);
    });

    return Napi::Boolean::New(env, true);
}

Napi::Value NodeClient::setOnVideoSubscribed(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Function argument expected").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Function callback = info[0].As<Napi::Function>();

    if (tsfn_video_subscribed_) {
        tsfn_video_subscribed_.Release();
    }

    tsfn_video_subscribed_ = Napi::ThreadSafeFunction::New(
        env, callback, "VideoSubscribedCallback", 0, 1
    );

    client_->setOnVideoSubscribed([this](int user_id, int status, const std::string& error) {
        auto callback = [user_id, status, error](Napi::Env env, Napi::Function jsCallback) {
            jsCallback.Call({
                Napi::Number::New(env, user_id),
                Napi::Number::New(env, status),
                Napi::String::New(env, error)
            });
        };
        tsfn_video_subscribed_.BlockingCall(callback);
    });

    return Napi::Boolean::New(env, true);
}

Napi::Value NodeClient::enableVideo(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsBoolean()) {
        Napi::TypeError::New(env, "Boolean argument expected").ThrowAsJavaScriptException();
        return env.Null();
    }

    bool enable = info[0].As<Napi::Boolean>().Value();
    client_->enableVideo(enable);
    
    return Napi::Boolean::New(env, true);
}

Napi::Value NodeClient::enableAudio(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsBoolean()) {
        Napi::TypeError::New(env, "Boolean argument expected").ThrowAsJavaScriptException();
        return env.Null();
    }

    bool enable = info[0].As<Napi::Boolean>().Value();
    client_->enableAudio(enable);
    
    return Napi::Boolean::New(env, true);
}

Napi::Value NodeClient::enableTranscript(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsBoolean()) {
        Napi::TypeError::New(env, "Boolean argument expected").ThrowAsJavaScriptException();
        return env.Null();
    }

    bool enable = info[0].As<Napi::Boolean>().Value();
    client_->enableTranscript(enable);
    
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
    if (tsfn_event_ex_) tsfn_event_ex_.Release();
}

Napi::Value NodeClient::initialize(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    try {
        string ca_path = "";
        int is_verify_cert = 1;
        string agent_str = "";
        const char* agent = nullptr;
        
        if (info.Length() > 0 && info[0].IsString()) {
            ca_path = info[0].As<Napi::String>();
        }
        
        if (info.Length() > 1 && info[1].IsNumber()) {
            is_verify_cert = info[1].As<Napi::Number>().Int32Value();
        }
        
        if (info.Length() > 2 && info[2].IsString()) {
            agent_str = info[2].As<Napi::String>();
            agent = agent_str.c_str();
        }
        
        rtms::Client::initialize(ca_path, is_verify_cert, agent);
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
        InstanceMethod("enableAudio", &NodeClient::enableAudio),
        InstanceMethod("enableVideo", &NodeClient::enableVideo),
        InstanceMethod("enableTranscript", &NodeClient::enableTranscript),
        InstanceMethod("setDeskshareParams", &NodeClient::setDeskshareParams),
        InstanceMethod("setAudioParams", &NodeClient::setAudioParams),
        InstanceMethod("setVideoParams", &NodeClient::setVideoParams),
        InstanceMethod("setTranscriptParams", &NodeClient::setTranscriptParams),
        InstanceMethod("setProxy", &NodeClient::setProxy),
        InstanceMethod("onJoinConfirm", &NodeClient::setOnJoinConfirm),
        InstanceMethod("onSessionUpdate", &NodeClient::setOnSessionUpdate),
        InstanceMethod("onUserUpdate", &NodeClient::setOnUserUpdate),
        InstanceMethod("onDeskshareData", &NodeClient::setOnDeskshareData),
        InstanceMethod("onAudioData", &NodeClient::setOnAudioData),
        InstanceMethod("onVideoData", &NodeClient::setOnVideoData),
        InstanceMethod("onTranscriptData", &NodeClient::setOnTranscriptData),
        InstanceMethod("onLeave", &NodeClient::setOnLeave),
        InstanceMethod("onEventEx", &NodeClient::setOnEventEx),
        InstanceMethod("subscribeEvent", &NodeClient::subscribeEvent),
        InstanceMethod("unsubscribeEvent", &NodeClient::unsubscribeEvent),
        InstanceMethod("subscribeVideo", &NodeClient::subscribeVideo),
        InstanceMethod("onParticipantVideo", &NodeClient::setOnParticipantVideo),
        InstanceMethod("onVideoSubscribed", &NodeClient::setOnVideoSubscribed),
    });

    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    env.SetInstanceData(constructor);

    exports.Set("Client", func);

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

    exports.Set(Napi::String::New(env, "USER_JOIN"), Napi::Number::New(env, USER_JOIN));
    exports.Set(Napi::String::New(env, "USER_LEAVE"), Napi::Number::New(env, USER_LEAVE));

    // Event types for subscribeEvent/unsubscribeEvent (used with onEventEx callback)
    // These match EVENT_TYPE from Zoom's C SDK
    exports.Set(Napi::String::New(env, "EVENT_UNDEFINED"),                    Napi::Number::New(env, (int)rtms::EVENT_TYPE::UNDEFINED));
    exports.Set(Napi::String::New(env, "EVENT_FIRST_PACKET_TIMESTAMP"),       Napi::Number::New(env, (int)rtms::EVENT_TYPE::FIRST_PACKET_TIMESTAMP));
    exports.Set(Napi::String::New(env, "EVENT_ACTIVE_SPEAKER_CHANGE"),        Napi::Number::New(env, (int)rtms::EVENT_TYPE::ACTIVE_SPEAKER_CHANGE));
    exports.Set(Napi::String::New(env, "EVENT_PARTICIPANT_JOIN"),             Napi::Number::New(env, (int)rtms::EVENT_TYPE::PARTICIPANT_JOIN));
    exports.Set(Napi::String::New(env, "EVENT_PARTICIPANT_LEAVE"),            Napi::Number::New(env, (int)rtms::EVENT_TYPE::PARTICIPANT_LEAVE));
    exports.Set(Napi::String::New(env, "EVENT_SHARING_START"),                Napi::Number::New(env, (int)rtms::EVENT_TYPE::SHARING_START));
    exports.Set(Napi::String::New(env, "EVENT_SHARING_STOP"),                 Napi::Number::New(env, (int)rtms::EVENT_TYPE::SHARING_STOP));
    exports.Set(Napi::String::New(env, "EVENT_MEDIA_CONNECTION_INTERRUPTED"), Napi::Number::New(env, (int)rtms::EVENT_TYPE::MEDIA_CONNECTION_INTERRUPTED));
    exports.Set(Napi::String::New(env, "EVENT_CONSUMER_ANSWERED"),            Napi::Number::New(env, (int)rtms::ZCC_VOICE_EVENT_TYPE::CONSUMER_ANSWERED));
    exports.Set(Napi::String::New(env, "EVENT_CONSUMER_END"),                 Napi::Number::New(env, (int)rtms::ZCC_VOICE_EVENT_TYPE::CONSUMER_END));
    exports.Set(Napi::String::New(env, "EVENT_USER_ANSWERED"),                Napi::Number::New(env, (int)rtms::ZCC_VOICE_EVENT_TYPE::USER_ANSWERED));
    exports.Set(Napi::String::New(env, "EVENT_USER_END"),                     Napi::Number::New(env, (int)rtms::ZCC_VOICE_EVENT_TYPE::USER_END));
    exports.Set(Napi::String::New(env, "EVENT_USER_HOLD"),                    Napi::Number::New(env, (int)rtms::ZCC_VOICE_EVENT_TYPE::USER_HOLD));
    exports.Set(Napi::String::New(env, "EVENT_USER_UNHOLD"),                  Napi::Number::New(env, (int)rtms::ZCC_VOICE_EVENT_TYPE::USER_UNHOLD));

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

    // Audio Content Type (MEDIA_CONTENT_TYPE)
    Napi::Object audioContentType = Napi::Object::New(env);
    audioContentType.Set("UNDEFINED",   Napi::Number::New(env, (int)rtms::MEDIA_CONTENT_TYPE::UNDEFINED));
    audioContentType.Set("RTP",         Napi::Number::New(env, (int)rtms::MEDIA_CONTENT_TYPE::RTP));
    audioContentType.Set("RAW_AUDIO",   Napi::Number::New(env, (int)rtms::MEDIA_CONTENT_TYPE::RAW_AUDIO));
    audioContentType.Set("FILE_STREAM", Napi::Number::New(env, (int)rtms::MEDIA_CONTENT_TYPE::FILE_STREAM));
    audioContentType.Set("TEXT",        Napi::Number::New(env, (int)rtms::MEDIA_CONTENT_TYPE::TEXT));
    exports.Set("AudioContentType", audioContentType);

    // Audio Codec (MEDIA_PAYLOAD_TYPE)
    Napi::Object audioCodec = Napi::Object::New(env);
    audioCodec.Set("UNDEFINED", Napi::Number::New(env, (int)rtms::MEDIA_PAYLOAD_TYPE::UNDEFINED));
    audioCodec.Set("L16",       Napi::Number::New(env, (int)rtms::MEDIA_PAYLOAD_TYPE::L16));
    audioCodec.Set("G711",      Napi::Number::New(env, (int)rtms::MEDIA_PAYLOAD_TYPE::G711));
    audioCodec.Set("G722",      Napi::Number::New(env, (int)rtms::MEDIA_PAYLOAD_TYPE::G722));
    audioCodec.Set("OPUS",      Napi::Number::New(env, (int)rtms::MEDIA_PAYLOAD_TYPE::OPUS));
    exports.Set("AudioCodec", audioCodec);

    // Audio Sample Rate (AUDIO_SAMPLE_RATE)
    Napi::Object audioSampleRate = Napi::Object::New(env);
    audioSampleRate.Set("SR_8K",  Napi::Number::New(env, (int)rtms::AUDIO_SAMPLE_RATE::SR_8K));
    audioSampleRate.Set("SR_16K", Napi::Number::New(env, (int)rtms::AUDIO_SAMPLE_RATE::SR_16K));
    audioSampleRate.Set("SR_32K", Napi::Number::New(env, (int)rtms::AUDIO_SAMPLE_RATE::SR_32K));
    audioSampleRate.Set("SR_48K", Napi::Number::New(env, (int)rtms::AUDIO_SAMPLE_RATE::SR_48K));
    exports.Set("AudioSampleRate", audioSampleRate);

    // Audio Channel (AUDIO_CHANNEL)
    Napi::Object audioChannel = Napi::Object::New(env);
    audioChannel.Set("MONO",   Napi::Number::New(env, (int)rtms::AUDIO_CHANNEL::MONO));
    audioChannel.Set("STEREO", Napi::Number::New(env, (int)rtms::AUDIO_CHANNEL::STEREO));
    exports.Set("AudioChannel", audioChannel);

    // Audio Data Option (MEDIA_DATA_OPTION)
    Napi::Object audioDataOption = Napi::Object::New(env);
    audioDataOption.Set("UNDEFINED",          Napi::Number::New(env, (int)rtms::MEDIA_DATA_OPTION::UNDEFINED));
    audioDataOption.Set("AUDIO_MIXED_STREAM",  Napi::Number::New(env, (int)rtms::MEDIA_DATA_OPTION::AUDIO_MIXED_STREAM));
    audioDataOption.Set("AUDIO_MULTI_STREAMS", Napi::Number::New(env, (int)rtms::MEDIA_DATA_OPTION::AUDIO_MULTI_STREAMS));
    exports.Set("AudioDataOption", audioDataOption);

    // ===== Video Parameters =====

    // Video Content Type (MEDIA_CONTENT_TYPE)
    Napi::Object videoContentType = Napi::Object::New(env);
    videoContentType.Set("UNDEFINED",   Napi::Number::New(env, (int)rtms::MEDIA_CONTENT_TYPE::UNDEFINED));
    videoContentType.Set("RTP",         Napi::Number::New(env, (int)rtms::MEDIA_CONTENT_TYPE::RTP));
    videoContentType.Set("RAW_VIDEO",   Napi::Number::New(env, (int)rtms::MEDIA_CONTENT_TYPE::RAW_VIDEO));
    videoContentType.Set("FILE_STREAM", Napi::Number::New(env, (int)rtms::MEDIA_CONTENT_TYPE::FILE_STREAM));
    videoContentType.Set("TEXT",        Napi::Number::New(env, (int)rtms::MEDIA_CONTENT_TYPE::TEXT));
    exports.Set("VideoContentType", videoContentType);

    // Video Codec (MEDIA_PAYLOAD_TYPE)
    Napi::Object videoCodec = Napi::Object::New(env);
    videoCodec.Set("UNDEFINED", Napi::Number::New(env, (int)rtms::MEDIA_PAYLOAD_TYPE::UNDEFINED));
    videoCodec.Set("JPG",       Napi::Number::New(env, (int)rtms::MEDIA_PAYLOAD_TYPE::JPG));
    videoCodec.Set("PNG",       Napi::Number::New(env, (int)rtms::MEDIA_PAYLOAD_TYPE::PNG));
    videoCodec.Set("H264",      Napi::Number::New(env, (int)rtms::MEDIA_PAYLOAD_TYPE::H264));
    exports.Set("VideoCodec", videoCodec);

    // Video Resolution (MEDIA_RESOLUTION)
    Napi::Object videoResolution = Napi::Object::New(env);
    videoResolution.Set("SD",  Napi::Number::New(env, (int)rtms::MEDIA_RESOLUTION::SD));
    videoResolution.Set("HD",  Napi::Number::New(env, (int)rtms::MEDIA_RESOLUTION::HD));
    videoResolution.Set("FHD", Napi::Number::New(env, (int)rtms::MEDIA_RESOLUTION::FHD));
    videoResolution.Set("QHD", Napi::Number::New(env, (int)rtms::MEDIA_RESOLUTION::QHD));
    exports.Set("VideoResolution", videoResolution);

    // Video Data Option (MEDIA_DATA_OPTION)
    Napi::Object videoDataOption = Napi::Object::New(env);
    videoDataOption.Set("UNDEFINED",                      Napi::Number::New(env, (int)rtms::MEDIA_DATA_OPTION::UNDEFINED));
    videoDataOption.Set("VIDEO_SINGLE_ACTIVE_STREAM",     Napi::Number::New(env, (int)rtms::MEDIA_DATA_OPTION::VIDEO_SINGLE_ACTIVE_STREAM));
    videoDataOption.Set("VIDEO_MIXED_SPEAKER_VIEW",       Napi::Number::New(env, (int)rtms::MEDIA_DATA_OPTION::VIDEO_SINGLE_INDIVIDUAL_STREAM));
    videoDataOption.Set("VIDEO_SINGLE_INDIVIDUAL_STREAM", Napi::Number::New(env, (int)rtms::MEDIA_DATA_OPTION::VIDEO_SINGLE_INDIVIDUAL_STREAM));
    videoDataOption.Set("VIDEO_MIXED_GALLERY_VIEW",       Napi::Number::New(env, (int)rtms::MEDIA_DATA_OPTION::VIDEO_MIXED_GALLERY_VIEW));
    exports.Set("VideoDataOption", videoDataOption);

    // ===== Media Types =====

    // Media Data Type (MEDIA_DATA_TYPE)
    Napi::Object mediaDataType = Napi::Object::New(env);
    mediaDataType.Set("UNDEFINED",  Napi::Number::New(env, (int)rtms::MEDIA_DATA_TYPE::UNDEFINED));
    mediaDataType.Set("AUDIO",      Napi::Number::New(env, (int)rtms::MEDIA_DATA_TYPE::AUDIO));
    mediaDataType.Set("VIDEO",      Napi::Number::New(env, (int)rtms::MEDIA_DATA_TYPE::VIDEO));
    mediaDataType.Set("DESKSHARE",  Napi::Number::New(env, (int)rtms::MEDIA_DATA_TYPE::DESKSHARE));
    mediaDataType.Set("TRANSCRIPT", Napi::Number::New(env, (int)rtms::MEDIA_DATA_TYPE::TRANSCRIPT));
    mediaDataType.Set("CHAT",       Napi::Number::New(env, (int)rtms::MEDIA_DATA_TYPE::CHAT));
    mediaDataType.Set("ALL",        Napi::Number::New(env, (int)rtms::MEDIA_DATA_TYPE::ALL));
    exports.Set("MediaDataType", mediaDataType);

    // ===== Session States =====

    // Session State (SESSION_STATE)
    Napi::Object sessionState = Napi::Object::New(env);
    sessionState.Set("INACTIVE",   Napi::Number::New(env, (int)rtms::SESSION_STATE::INACTIVE));
    sessionState.Set("INITIALIZE", Napi::Number::New(env, (int)rtms::SESSION_STATE::INITIALIZE));
    sessionState.Set("STARTED",    Napi::Number::New(env, (int)rtms::SESSION_STATE::STARTED));
    sessionState.Set("PAUSED",     Napi::Number::New(env, (int)rtms::SESSION_STATE::PAUSED));
    sessionState.Set("RESUMED",    Napi::Number::New(env, (int)rtms::SESSION_STATE::RESUMED));
    sessionState.Set("STOPPED",    Napi::Number::New(env, (int)rtms::SESSION_STATE::STOPPED));
    exports.Set("SessionState", sessionState);

    // Stream State (STREAM_STATE)
    Napi::Object streamState = Napi::Object::New(env);
    streamState.Set("INACTIVE",    Napi::Number::New(env, (int)rtms::STREAM_STATE::INACTIVE));
    streamState.Set("ACTIVE",      Napi::Number::New(env, (int)rtms::STREAM_STATE::ACTIVE));
    streamState.Set("INTERRUPTED", Napi::Number::New(env, (int)rtms::STREAM_STATE::INTERRUPTED));
    streamState.Set("TERMINATING", Napi::Number::New(env, (int)rtms::STREAM_STATE::TERMINATING));
    streamState.Set("TERMINATED",  Napi::Number::New(env, (int)rtms::STREAM_STATE::TERMINATED));
    streamState.Set("PAUSED",      Napi::Number::New(env, (int)rtms::STREAM_STATE::PAUSED));
    streamState.Set("RESUMED",     Napi::Number::New(env, (int)rtms::STREAM_STATE::RESUMED));
    exports.Set("StreamState", streamState);

    // Event Type (EVENT_TYPE + ZCC_VOICE_EVENT_TYPE for backward compat)
    Napi::Object eventType = Napi::Object::New(env);
    eventType.Set("UNDEFINED",                    Napi::Number::New(env, (int)rtms::EVENT_TYPE::UNDEFINED));
    eventType.Set("FIRST_PACKET_TIMESTAMP",       Napi::Number::New(env, (int)rtms::EVENT_TYPE::FIRST_PACKET_TIMESTAMP));
    eventType.Set("ACTIVE_SPEAKER_CHANGE",        Napi::Number::New(env, (int)rtms::EVENT_TYPE::ACTIVE_SPEAKER_CHANGE));
    eventType.Set("PARTICIPANT_JOIN",             Napi::Number::New(env, (int)rtms::EVENT_TYPE::PARTICIPANT_JOIN));
    eventType.Set("PARTICIPANT_LEAVE",            Napi::Number::New(env, (int)rtms::EVENT_TYPE::PARTICIPANT_LEAVE));
    eventType.Set("SHARING_START",                Napi::Number::New(env, (int)rtms::EVENT_TYPE::SHARING_START));
    eventType.Set("SHARING_STOP",                 Napi::Number::New(env, (int)rtms::EVENT_TYPE::SHARING_STOP));
    eventType.Set("MEDIA_CONNECTION_INTERRUPTED", Napi::Number::New(env, (int)rtms::EVENT_TYPE::MEDIA_CONNECTION_INTERRUPTED));
    eventType.Set("PARTICIPANT_VIDEO_ON",         Napi::Number::New(env, (int)rtms::EVENT_TYPE::PARTICIPANT_VIDEO_ON));
    eventType.Set("PARTICIPANT_VIDEO_OFF",        Napi::Number::New(env, (int)rtms::EVENT_TYPE::PARTICIPANT_VIDEO_OFF));
    eventType.Set("CONSUMER_ANSWERED",            Napi::Number::New(env, (int)rtms::ZCC_VOICE_EVENT_TYPE::CONSUMER_ANSWERED));
    eventType.Set("CONSUMER_END",                 Napi::Number::New(env, (int)rtms::ZCC_VOICE_EVENT_TYPE::CONSUMER_END));
    eventType.Set("USER_ANSWERED",                Napi::Number::New(env, (int)rtms::ZCC_VOICE_EVENT_TYPE::USER_ANSWERED));
    eventType.Set("USER_END",                     Napi::Number::New(env, (int)rtms::ZCC_VOICE_EVENT_TYPE::USER_END));
    eventType.Set("USER_HOLD",                    Napi::Number::New(env, (int)rtms::ZCC_VOICE_EVENT_TYPE::USER_HOLD));
    eventType.Set("USER_UNHOLD",                  Napi::Number::New(env, (int)rtms::ZCC_VOICE_EVENT_TYPE::USER_UNHOLD));
    exports.Set("EventType", eventType);

    // Message Type (MESSAGE_TYPE) — complete set of 30 values
    Napi::Object messageType = Napi::Object::New(env);
    messageType.Set("UNDEFINED",                Napi::Number::New(env, (int)rtms::MESSAGE_TYPE::UNDEFINED));
    messageType.Set("SIGNALING_HAND_SHAKE_REQ", Napi::Number::New(env, (int)rtms::MESSAGE_TYPE::SIGNALING_HAND_SHAKE_REQ));
    messageType.Set("SIGNALING_HAND_SHAKE_RESP",Napi::Number::New(env, (int)rtms::MESSAGE_TYPE::SIGNALING_HAND_SHAKE_RESP));
    messageType.Set("DATA_HAND_SHAKE_REQ",      Napi::Number::New(env, (int)rtms::MESSAGE_TYPE::DATA_HAND_SHAKE_REQ));
    messageType.Set("DATA_HAND_SHAKE_RESP",     Napi::Number::New(env, (int)rtms::MESSAGE_TYPE::DATA_HAND_SHAKE_RESP));
    messageType.Set("EVENT_SUBSCRIPTION",       Napi::Number::New(env, (int)rtms::MESSAGE_TYPE::EVENT_SUBSCRIPTION));
    messageType.Set("EVENT_UPDATE",             Napi::Number::New(env, (int)rtms::MESSAGE_TYPE::EVENT_UPDATE));
    messageType.Set("CLIENT_READY_ACK",         Napi::Number::New(env, (int)rtms::MESSAGE_TYPE::CLIENT_READY_ACK));
    messageType.Set("STREAM_STATE_UPDATE",      Napi::Number::New(env, (int)rtms::MESSAGE_TYPE::STREAM_STATE_UPDATE));
    messageType.Set("SESSION_STATE_UPDATE",     Napi::Number::New(env, (int)rtms::MESSAGE_TYPE::SESSION_STATE_UPDATE));
    messageType.Set("SESSION_STATE_REQ",        Napi::Number::New(env, (int)rtms::MESSAGE_TYPE::SESSION_STATE_REQ));
    messageType.Set("SESSION_STATE_RESP",       Napi::Number::New(env, (int)rtms::MESSAGE_TYPE::SESSION_STATE_RESP));
    messageType.Set("KEEP_ALIVE_REQ",           Napi::Number::New(env, (int)rtms::MESSAGE_TYPE::KEEP_ALIVE_REQ));
    messageType.Set("KEEP_ALIVE_RESP",          Napi::Number::New(env, (int)rtms::MESSAGE_TYPE::KEEP_ALIVE_RESP));
    messageType.Set("MEDIA_DATA_AUDIO",         Napi::Number::New(env, (int)rtms::MESSAGE_TYPE::MEDIA_DATA_AUDIO));
    messageType.Set("MEDIA_DATA_VIDEO",         Napi::Number::New(env, (int)rtms::MESSAGE_TYPE::MEDIA_DATA_VIDEO));
    messageType.Set("MEDIA_DATA_SHARE",         Napi::Number::New(env, (int)rtms::MESSAGE_TYPE::MEDIA_DATA_SHARE));
    messageType.Set("MEDIA_DATA_TRANSCRIPT",    Napi::Number::New(env, (int)rtms::MESSAGE_TYPE::MEDIA_DATA_TRANSCRIPT));
    messageType.Set("MEDIA_DATA_CHAT",          Napi::Number::New(env, (int)rtms::MESSAGE_TYPE::MEDIA_DATA_CHAT));
    messageType.Set("STREAM_STATE_REQ",         Napi::Number::New(env, (int)rtms::MESSAGE_TYPE::STREAM_STATE_REQ));
    messageType.Set("STREAM_STATE_RESP",        Napi::Number::New(env, (int)rtms::MESSAGE_TYPE::STREAM_STATE_RESP));
    messageType.Set("STREAM_CLOSE_REQ",         Napi::Number::New(env, (int)rtms::MESSAGE_TYPE::STREAM_CLOSE_REQ));
    messageType.Set("STREAM_CLOSE_RESP",        Napi::Number::New(env, (int)rtms::MESSAGE_TYPE::STREAM_CLOSE_RESP));
    messageType.Set("META_DATA_AUDIO",          Napi::Number::New(env, (int)rtms::MESSAGE_TYPE::META_DATA_AUDIO));
    messageType.Set("META_DATA_VIDEO",          Napi::Number::New(env, (int)rtms::MESSAGE_TYPE::META_DATA_VIDEO));
    messageType.Set("META_DATA_SHARE",          Napi::Number::New(env, (int)rtms::MESSAGE_TYPE::META_DATA_SHARE));
    messageType.Set("META_DATA_TRANSCRIPT",     Napi::Number::New(env, (int)rtms::MESSAGE_TYPE::META_DATA_TRANSCRIPT));
    messageType.Set("META_DATA_CHAT",           Napi::Number::New(env, (int)rtms::MESSAGE_TYPE::META_DATA_CHAT));
    messageType.Set("VIDEO_SUBSCRIPTION_REQ",   Napi::Number::New(env, (int)rtms::MESSAGE_TYPE::VIDEO_SUBSCRIPTION_REQ));
    messageType.Set("VIDEO_SUBSCRIPTION_RESP",  Napi::Number::New(env, (int)rtms::MESSAGE_TYPE::VIDEO_SUBSCRIPTION_RESP));
    exports.Set("MessageType", messageType);

    // Stop Reason (STOP_REASON) — complete set of 27 values
    Napi::Object stopReason = Napi::Object::New(env);
    stopReason.Set("UNDEFINED",                               Napi::Number::New(env, (int)rtms::STOP_REASON::UNDEFINED));
    stopReason.Set("HOST_TRIGGERED",                          Napi::Number::New(env, (int)rtms::STOP_REASON::HOST_TRIGGERED));
    stopReason.Set("USER_TRIGGERED",                          Napi::Number::New(env, (int)rtms::STOP_REASON::USER_TRIGGERED));
    stopReason.Set("USER_LEFT",                               Napi::Number::New(env, (int)rtms::STOP_REASON::USER_LEFT));
    stopReason.Set("USER_EJECTED",                            Napi::Number::New(env, (int)rtms::STOP_REASON::USER_EJECTED));
    stopReason.Set("HOST_DISABLED_APP",                       Napi::Number::New(env, (int)rtms::STOP_REASON::HOST_DISABLED_APP));
    stopReason.Set("MEETING_ENDED",                           Napi::Number::New(env, (int)rtms::STOP_REASON::MEETING_ENDED));
    stopReason.Set("STREAM_CANCELED",                         Napi::Number::New(env, (int)rtms::STOP_REASON::STREAM_CANCELED));
    stopReason.Set("STREAM_REVOKED",                          Napi::Number::New(env, (int)rtms::STOP_REASON::STREAM_REVOKED));
    stopReason.Set("ALL_APPS_DISABLED",                       Napi::Number::New(env, (int)rtms::STOP_REASON::ALL_APPS_DISABLED));
    stopReason.Set("INTERNAL_EXCEPTION",                      Napi::Number::New(env, (int)rtms::STOP_REASON::INTERNAL_EXCEPTION));
    stopReason.Set("CONNECTION_TIMEOUT",                      Napi::Number::New(env, (int)rtms::STOP_REASON::CONNECTION_TIMEOUT));
    stopReason.Set("INSTANCE_CONNECTION_INTERRUPTED",         Napi::Number::New(env, (int)rtms::STOP_REASON::INSTANCE_CONNECTION_INTERRUPTED));
    stopReason.Set("SIGNAL_CONNECTION_INTERRUPTED",           Napi::Number::New(env, (int)rtms::STOP_REASON::SIGNAL_CONNECTION_INTERRUPTED));
    stopReason.Set("DATA_CONNECTION_INTERRUPTED",             Napi::Number::New(env, (int)rtms::STOP_REASON::DATA_CONNECTION_INTERRUPTED));
    stopReason.Set("SIGNAL_CONNECTION_CLOSED_ABNORMALLY",     Napi::Number::New(env, (int)rtms::STOP_REASON::SIGNAL_CONNECTION_CLOSED_ABNORMALLY));
    stopReason.Set("DATA_CONNECTION_CLOSED_ABNORMALLY",       Napi::Number::New(env, (int)rtms::STOP_REASON::DATA_CONNECTION_CLOSED_ABNORMALLY));
    stopReason.Set("EXIT_SIGNAL",                             Napi::Number::New(env, (int)rtms::STOP_REASON::EXIT_SIGNAL));
    stopReason.Set("AUTHENTICATION_FAILURE",                  Napi::Number::New(env, (int)rtms::STOP_REASON::AUTHENTICATION_FAILURE));
    stopReason.Set("AWAIT_RECONNECTION_TIMEOUT",              Napi::Number::New(env, (int)rtms::STOP_REASON::AWAIT_RECONNECTION_TIMEOUT));
    stopReason.Set("RECEIVER_REQUEST_CLOSE",                  Napi::Number::New(env, (int)rtms::STOP_REASON::RECEIVER_REQUEST_CLOSE));
    stopReason.Set("CUSTOMER_DISCONNECTED",                   Napi::Number::New(env, (int)rtms::STOP_REASON::CUSTOMER_DISCONNECTED));
    stopReason.Set("AGENT_DISCONNECTED",                      Napi::Number::New(env, (int)rtms::STOP_REASON::AGENT_DISCONNECTED));
    stopReason.Set("ADMIN_DISABLED_APP",                      Napi::Number::New(env, (int)rtms::STOP_REASON::ADMIN_DISABLED_APP));
    stopReason.Set("KEEP_ALIVE_TIMEOUT",                      Napi::Number::New(env, (int)rtms::STOP_REASON::KEEP_ALIVE_TIMEOUT));
    stopReason.Set("MANUAL_API_TRIGGERED",                    Napi::Number::New(env, (int)rtms::STOP_REASON::MANUAL_API_TRIGGERED));
    stopReason.Set("STREAMING_NOT_SUPPORTED",                 Napi::Number::New(env, (int)rtms::STOP_REASON::STREAMING_NOT_SUPPORTED));
    exports.Set("StopReason", stopReason);

    // Transcript language constants (mirrors AudioCodec/VideoCodec pattern)
    Napi::Object transcriptLanguage = Napi::Object::New(env);
    transcriptLanguage.Set("NONE",                Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::NONE));
    transcriptLanguage.Set("ARABIC",              Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::ARABIC));
    transcriptLanguage.Set("BENGALI",             Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::BENGALI));
    transcriptLanguage.Set("CANTONESE",           Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::CANTONESE));
    transcriptLanguage.Set("CATALAN",             Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::CATALAN));
    transcriptLanguage.Set("CHINESE_SIMPLIFIED",  Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::CHINESE_SIMPLIFIED));
    transcriptLanguage.Set("CHINESE_TRADITIONAL", Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::CHINESE_TRADITIONAL));
    transcriptLanguage.Set("CZECH",               Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::CZECH));
    transcriptLanguage.Set("DANISH",              Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::DANISH));
    transcriptLanguage.Set("DUTCH",               Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::DUTCH));
    transcriptLanguage.Set("ENGLISH",             Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::ENGLISH));
    transcriptLanguage.Set("ESTONIAN",            Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::ESTONIAN));
    transcriptLanguage.Set("FINNISH",             Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::FINNISH));
    transcriptLanguage.Set("FRENCH_CANADA",       Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::FRENCH_CANADA));
    transcriptLanguage.Set("FRENCH_FRANCE",       Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::FRENCH_FRANCE));
    transcriptLanguage.Set("GERMAN",              Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::GERMAN));
    transcriptLanguage.Set("HEBREW",              Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::HEBREW));
    transcriptLanguage.Set("HINDI",               Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::HINDI));
    transcriptLanguage.Set("HUNGARIAN",           Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::HUNGARIAN));
    transcriptLanguage.Set("INDONESIAN",          Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::INDONESIAN));
    transcriptLanguage.Set("ITALIAN",             Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::ITALIAN));
    transcriptLanguage.Set("JAPANESE",            Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::JAPANESE));
    transcriptLanguage.Set("KOREAN",              Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::KOREAN));
    transcriptLanguage.Set("MALAY",               Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::MALAY));
    transcriptLanguage.Set("PERSIAN",             Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::PERSIAN));
    transcriptLanguage.Set("POLISH",              Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::POLISH));
    transcriptLanguage.Set("PORTUGUESE",          Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::PORTUGUESE));
    transcriptLanguage.Set("ROMANIAN",            Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::ROMANIAN));
    transcriptLanguage.Set("RUSSIAN",             Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::RUSSIAN));
    transcriptLanguage.Set("SPANISH",             Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::SPANISH));
    transcriptLanguage.Set("SWEDISH",             Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::SWEDISH));
    transcriptLanguage.Set("TAGALOG",             Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::TAGALOG));
    transcriptLanguage.Set("TAMIL",               Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::TAMIL));
    transcriptLanguage.Set("TELUGU",              Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::TELUGU));
    transcriptLanguage.Set("THAI",                Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::THAI));
    transcriptLanguage.Set("TURKISH",             Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::TURKISH));
    transcriptLanguage.Set("UKRAINIAN",           Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::UKRAINIAN));
    transcriptLanguage.Set("VIETNAMESE",          Napi::Number::New(env, (int)rtms::TRANSCRIPT_LANGUAGE::VIETNAMESE));
    exports.Set("TranscriptLanguage", transcriptLanguage);

    return exports;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    return NodeClient::init(env, exports);
}

NODE_API_MODULE(rtms, Init)