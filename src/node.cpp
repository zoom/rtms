#include <napi.h>
#include <thread>
#include <iostream>
#include <functional>
#include "rtms.h"

using namespace Napi;
using namespace rtms;

Client client;

FunctionReference onJoinConfirmFn;
FunctionReference onSessionUpdateFn;
FunctionReference onUserUpdateFn;
FunctionReference onAudioDataFn;
FunctionReference onVideoDataFn;
FunctionReference onTranscriptDataFn;
FunctionReference onLeaveFn;

Value setCallback(const CallbackInfo& info, FunctionReference& fn) {
    auto env = info.Env();

    if (!info.Length() || !info[0].IsFunction())
        throw TypeError::New(env, "missing required callback argument");

    fn = Persistent(info[0].As<Function>());
    fn.SuppressDestruct();

    return env.Null();
}

Object getMetadataObj(const Env& env, struct rtms_metadata* md) {
    auto metadata = Object::New(env);
    metadata.Set("userName", string(md->user_name));
    metadata.Set("userId", md->user_id);
    return metadata;
}

Value setOnJoinConfirm(const CallbackInfo& info) {
    return setCallback(info, onJoinConfirmFn);
}

Value setOnSessionUpdate(const CallbackInfo& info) {
    return setCallback(info, onSessionUpdateFn);
}

Value setOnUserUpdate(const CallbackInfo& info) {
    return setCallback(info, onUserUpdateFn);
}

Value setOnAudioData(const CallbackInfo& info) {
    return setCallback(info, onAudioDataFn);
}

Value setOnVideoData(const CallbackInfo& info) {
    return setCallback(info, onVideoDataFn);
}

Value setOnTranscriptData(const CallbackInfo& info) {
    return setCallback(info, onTranscriptDataFn);
}

Value setOnLeave(const CallbackInfo& info) {
    return setCallback(info, onLeaveFn);
}

void onJoinConfirm(struct rtms_csdk* sdk, int reason) {
    if (onJoinConfirmFn.IsEmpty()) return;

    auto env = onJoinConfirmFn.Env();
    onJoinConfirmFn.Call({Number::New(env, reason)});
}

void onSessionUpdate(struct rtms_csdk* sdk, int op, struct session_info* session) {
    if (onSessionUpdateFn.IsEmpty()) return;

    auto env = onSessionUpdateFn.Env();

    Object sessionInfo = Object::New(env);
    sessionInfo.Set("sessionId", string(session->session_id));
    sessionInfo.Set("statTime", session->stat_time);
    sessionInfo.Set("status", session->status);

    onSessionUpdateFn.Call({
        Number::New(env, op),
        sessionInfo
    });
}

void onUserUpdate(struct rtms_csdk* sdk, int op, struct participant_info* pi) {
    if (onUserUpdateFn.IsEmpty()) return;

    auto env = onUserUpdateFn.Env();

    Object participantInfo = Object::New(env);
    participantInfo.Set("participantId", pi->participant_id);
    participantInfo.Set("participantName", string(pi->participant_name));

    onUserUpdateFn.Call({
        Number::New(env, op),
        participantInfo
    });
}


void onAudioData(struct rtms_csdk *sdk, unsigned char *buf, int size, unsigned int timestamp, struct rtms_metadata *md) {
    if (onAudioDataFn.IsEmpty()) return;

    auto env = onAudioDataFn.Env();

    onAudioDataFn.Call({
        Buffer<unsigned char>::New(env, buf, size),
        Number::New(env, size),
        Number::New(env, timestamp),
        getMetadataObj(env, md)
    });
}

void onVideoData(struct rtms_csdk *sdk, unsigned char *buf, int size, unsigned int timestamp, const char *rtms_session_id, struct rtms_metadata *md){
    if (onVideoDataFn.IsEmpty()) return;

    auto env = onVideoDataFn.Env();

    onVideoDataFn.Call({
        Buffer<unsigned char>::New(env, buf, size),
        Number::New(env, size),
        Number::New(env, timestamp),
        getMetadataObj(env, md)
    });
}

void onTranscriptData(struct rtms_csdk *sdk, unsigned char *buf, int size, unsigned int timestamp, struct rtms_metadata *md){
    if (onTranscriptDataFn.IsEmpty()) return;

    auto env = onTranscriptDataFn.Env();

    onTranscriptDataFn.Call({
        Buffer<unsigned char>::New(env, buf, size),
        Number::New(env, size),
        Number::New(env, timestamp),
        getMetadataObj(env, md)
    });
}

void onLeave(struct rtms_csdk *sdk, int reason){
    if (onLeaveFn.IsEmpty()) return;

    auto env = onLeaveFn.Env();

    onLeaveFn.Call({ Number::New(env, reason)});
}

Value useAudio(const CallbackInfo& info) {
    auto env = info.Env();

    bool hasAudio = info[0].IsBoolean() ? info[0].As<Boolean>() : true;

    client.useAudio(hasAudio);

    return env.Null();
}

Value useVideo(const CallbackInfo& info) {
    auto env = info.Env();

    bool hasVideo = info[0].IsBoolean() ? info[0].As<Boolean>() : true;

    client.useVideo(hasVideo);

    return env.Null();
}

Value useTranscript(const CallbackInfo& info) {
    auto env = info.Env();

    bool hasTranscript = info[0].IsBoolean() ? info[0].As<Boolean>() : true;

    client.useTranscript(hasTranscript);

    return env.Null();
}

Value setMediaTypes(const CallbackInfo& info) {
    auto env = info.Env();


    if (info.Length() < 1)
        throw TypeError::New(env, "setMediaTypes requires at least 1 argument");

    bool hasAudio = info[0].As<Boolean>();
    bool hasVideo = info[1].IsBoolean() ? info[0].As<Boolean>() : false;
    bool hasTranscript = info[2].IsBoolean() ? info[0].As<Boolean>() : false;

    client.setMediaTypes(hasAudio, hasVideo, hasTranscript);

    return env.Null();
}


Value init(const CallbackInfo& info) {
    auto env = info.Env();

    if (info.Length() < 1)
        throw TypeError::New(env, "init requires 1 argument (ca_path: string)");

    if (!info[0].IsString())
        throw TypeError::New(env, "First arg should be of type string");

    const string ca = info[0].As<String>();

    client.setOnJoinConfirm(onJoinConfirm);
    client.setOnUserUpdate(onUserUpdate);
    client.setOnSessionUpdate(onSessionUpdate);
    client.setOnAudioData(onAudioData);
    client.setOnVideoData(onVideoData);
    client.setOnTranscriptData(onTranscriptData);
    client.setOnLeave(onLeave); 
 
    client.init(ca);

    return info.Env().Null();
}

Value isInit(const CallbackInfo& info) {
    return Boolean::New(info.Env(), client.isInit());
}

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
    string server_urls = info[3].As<String>();

    const bool hasTimeout = info.Length() > 4;
    int timeout = hasTimeout ? info[4].As<Number>().Int32Value() : 0;

    return Number::New(env, client.join(uuid, session_id, signature, server_urls, timeout));
}


Object Init(Env env, Object exports) {
    exports.Set(String::New(env, "_init"),
                Function::New(env, init));

    exports.Set(String::New(env,"_join"),
                Function::New(env, join));

    exports.Set(String::New(env, "_isInit"),
                Function::New(env, isInit));
    
    exports.Set(String::New(env, "useAudio"),
                Function::New(env, useAudio));
                
    exports.Set(String::New(env, "useVideo"),
                Function::New(env, useVideo));

    exports.Set(String::New(env, "useTranscript"),
                Function::New(env, useTranscript));

    exports.Set(String::New(env, "setMediaTypes"),
                Function::New(env, setMediaTypes));

    exports.Set(String::New(env, "onJoinConfirm"),
                Function::New(env, setOnJoinConfirm));

    exports.Set(String::New(env, "onSessionUpdate"),
                Function::New(env, setOnSessionUpdate));

    exports.Set(String::New(env, "onUserUpdate"),
                Function::New(env, setOnUserUpdate));

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

NODE_API_MODULE(rtms, Init) 