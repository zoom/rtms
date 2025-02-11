#include <napi.h>
#include <thread>
#include <iostream>
#include <functional>

#include "rtms.h"

using namespace Napi;

struct CallbackThread {
    ThreadSafeFunction tsfn;
    std::thread thread;
};

RTMS rtms;

CallbackThread cb_onJoinConfirm;
CallbackThread cb_onSessionUpdate;
CallbackThread cb_onAudioData;
CallbackThread cb_onVideoData;
CallbackThread cb_onTranscriptData;
CallbackThread cb_onLeave;


Value join (const CallbackInfo& info) {
    auto env = info.Env();

    if (info.Length() < 2)
        throw TypeError::New(env, "Wrong number of arguments");


    for (int i=0; i < 3; i++)
        if (!info[i].IsString())
            throw TypeError::New(env, "Argument must be a string");

    string uuid = info[0].As<String>();
    string session_id = info[1].As<String>();
    string signature = info[2].As<String>();
    string signal_url = info[3].As<String>();

    const bool hasTimeout = info.Length() > 4;
    int timeout = hasTimeout ? info[4].As<Number>().Int32Value() : 0;

    return Number::New(env, rtms.join(uuid, session_id, signature, signal_url, timeout));
}


void onJoinConfirm(struct rtms_csdk* sdk, int reason) {
    auto callback = [&]( Napi::Env env, Function jsCallback) {
        jsCallback.Call({Number::New( env, reason )});
    };

    cb_onJoinConfirm.thread = std::thread([&] {
        cb_onJoinConfirm.tsfn.BlockingCall(callback);
        cb_onJoinConfirm.tsfn.Release();
    });
}


void onSessionUpdate(struct rtms_csdk* sdk, int op, struct session_info* session) {
    auto callback = [&]( Napi::Env env, Function jsCallback) {

        Object sessionInfo = Object::New(env);
        sessionInfo.Set("sessionId", string(session->session_id));
        sessionInfo.Set("operatorId", string(session->operator_id));
        sessionInfo.Set("statTime", session->stat_time);
        sessionInfo.Set("status", session->status);

        jsCallback.Call({
            Number::New(env, op),
            sessionInfo
        });
    };

    cb_onSessionUpdate.thread = std::thread([&] {
        cb_onSessionUpdate.tsfn.BlockingCall(callback);
        cb_onSessionUpdate.tsfn.Release();
    });

    return;
}


void onAudioData(struct rtms_csdk *sdk, unsigned char *buf, int size, unsigned int timestamp, struct rtms_metadata *md) {
    auto callback = [&]( Napi::Env env, Function jsCallback) {

        auto meta = Object::New(env);
        meta.Set("userName", string(md->user_name));
        meta.Set("userId", md->user_id);

        jsCallback.Call({
            Buffer<unsigned char>::New(env, buf, size),
            Number::New(env, size),
            Number::New(env, timestamp),
            meta
        });
    };

    cb_onAudioData.thread = std::thread([&] {
        cb_onAudioData.tsfn.BlockingCall(callback);
        cb_onAudioData.tsfn.Release();
    });

    return;

}
void onVideoData(struct rtms_csdk *sdk, unsigned char *buf, int size, unsigned int timestamp, const char *rtms_session_id, struct rtms_metadata *md){
    auto callback = [&]( Napi::Env env, Function jsCallback) {

        auto meta = Object::New(env);
        meta.Set("userName", string(md->user_name));
        meta.Set("userId", md->user_id);

        jsCallback.Call({
            Buffer<unsigned char>::New(env, buf, size),
            Number::New(env, size),
            Number::New(env, timestamp),
            String::New(env, string(rtms_session_id)),
            meta
        });
    };

    cb_onVideoData.thread = std::thread([&] {
        cb_onVideoData.tsfn.BlockingCall(callback);
        cb_onVideoData.tsfn.Release();
    });
}
void onTranscriptData(struct rtms_csdk *sdk, unsigned char *buf, int size, unsigned int timestamp, struct rtms_metadata *md){
    auto callback = [&]( Napi::Env env, Function jsCallback) {

        auto meta = Object::New(env);
        meta.Set("userName", string(md->user_name));
        meta.Set("userId", md->user_id);

        jsCallback.Call({
            Buffer<unsigned char>::New(env, buf, size),
            Number::New(env, size),
            Number::New(env, timestamp),
            meta
        });
    };

    cb_onTranscriptData.thread = std::thread([&] {
        cb_onTranscriptData.tsfn.BlockingCall(callback);
        cb_onTranscriptData.tsfn.Release();
    });
}
void onLeave(struct rtms_csdk *sdk, int reason){
    auto callback = [&]( Napi::Env env, Function jsCallback) {
        jsCallback.Call({Number::New(env, reason)});
    };

    cb_onJoinConfirm.thread = std::thread([&] {
        cb_onJoinConfirm.tsfn.BlockingCall(callback);
        cb_onJoinConfirm.tsfn.Release();
    });
}


Value init(const CallbackInfo& info) {
    auto env = info.Env();

    if (info.Length() < 1)
        throw TypeError::New(env, "init requires 1 argument (ca_path: string)");

    if (!info[0].IsString())
        throw TypeError::New(env, "First arg should be of type string");

    const string ca = info[0].As<String>();

    rtms.setOnJoinConfirm(onJoinConfirm);
    rtms.setOnSessionUpdate(onSessionUpdate);
    rtms.setOnAudioData(onAudioData);
    rtms.setOnVideoData(onVideoData);
    rtms.setOnTranscriptData(onTranscriptData);
    rtms.setOnLeave(onLeave);

    rtms.init(ca);

    return info.Env().Null();
}

Value setCallback(const string& name, CallbackThread& cbt, const CallbackInfo& info) {
    auto env = info.Env();

    if (!info.Length() || !info[0].IsFunction())
        throw TypeError::New(env, "missing required callback argument");

    auto fn = info[0].As<Function>();

    cbt.tsfn = ThreadSafeFunction::New(
            env,
            fn,                       // JavaScript function called asynchronously
            name,         // Name
            0,                       // Unlimited queue
            1,                       // Only one thread will use this initially
            [&]( Napi::Env ) {        // Finalizer used to clean threads up
                cbt.thread.join();
            } );

    return info.Env().Null();
}


Value setOnJoinConfirm(const CallbackInfo& info) {
    return setCallback("onJoinConfirm", cb_onJoinConfirm, info);
}

Value setOnSessionUpdate(const CallbackInfo& info) {
    return setCallback("onSessionUpdate", cb_onSessionUpdate, info);
}

Value setOnAudioData(const CallbackInfo& info) {
    return setCallback("onAudioData", cb_onAudioData, info);
}

Value setOnVideoData(const CallbackInfo& info) {
    return setCallback("onVideoData", cb_onVideoData, info);
}

Value setOnTranscriptData(const CallbackInfo& info) {
    return setCallback("onTranscriptData", cb_onTranscriptData, info);
}

Value setOnLeave(const CallbackInfo& info) {
    return setCallback("onLeave", cb_onLeave, info);
}


Object Init(Env env, Object exports) {
    exports.Set(String::New(env, "init"),
                Function::New(env, init));

    exports.Set(String::New(env,"_join"),
                Function::New(env, join));

    exports.Set(String::New(env, "onJoinConfirm"),
                Function::New(env, setOnJoinConfirm));

    exports.Set(String::New(env, "onSessionUpdate"),
                Function::New(env, setOnSessionUpdate));

    exports.Set(String::New(env, "onAudioData"),
                Function::New(env, setOnAudioData));

    exports.Set(String::New(env, "onVideoData"),
                Function::New(env, setOnVideoData));

    exports.Set(String::New(env, "onTranscriptData"),
                Function::New(env, setOnTranscriptData));

    exports.Set(String::New(env, "onLeave"),
                Function::New(env, setOnLeave));

    return exports;
}

NODE_API_MODULE("rtms-sdk", Init);