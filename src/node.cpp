#include <napi.h>
#include <thread>
#include <iostream>
#include <functional>

#include "rtms.h"


using namespace Napi;

RTMS rtms;

class CallbackThread {
public:
    Function fn;
    ThreadSafeFunction tsfn;
    std::thread thread;
};

CallbackThread cb_onJoinConfirm;


void onJoinConfirm(struct rtms_csdk* sdk, int reason) {
    auto callback = []( Napi::Env env, Function jsCallback, int* value) {
        jsCallback.Call( {Number::New( env, *value )} );

        delete value;
    };

    // Create a native thread
    cb_onJoinConfirm.thread = std::thread([&] {
        int* v = new int(reason);

        cb_onJoinConfirm.tsfn.BlockingCall(v, callback);
        cb_onJoinConfirm.tsfn.Release();
    });


}


void onSessionUpdate(struct rtms_csdk* sdk,int,struct session_info* session_info) {
    return;
}



Boolean setOnJoinConfirm(const CallbackInfo& info) {
    auto env = info.Env();

    if (!info.Length() || !info[0].IsFunction())
        throw TypeError::New(env, "missing required callback");

     auto fn = info[0].As<Function>();

    cb_onJoinConfirm.tsfn = ThreadSafeFunction::New(
            env,
            fn,  // JavaScript function called asynchronously
            "Resource Name",         // Name
            0,                       // Unlimited queue
            1,                       // Only one thread will use this initially
            []( Napi::Env ) {        // Finalizer used to clean threads up
                cb_onJoinConfirm.thread.join();
            } );

    return Boolean::New(env, true);
}


Value init(const CallbackInfo& info) {
    auto env = info.Env();

    if (info.Length() < 1)
        throw TypeError::New(env, "init requires 1 argument (ca_path: string)");

    if (!info[0].IsString())
        throw TypeError::New(env, "First arg should be of type string");

    const string ca = info[0].As<String>();
    rtms.init(ca);

    rtms.setOnJoinConfirm(onJoinConfirm);
    rtms.setOnSessionUpdate(onSessionUpdate);

    return info.Env().Null();
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
    string signal_url = info[3].As<String>();

    const bool hasTimeout = info.Length() > 4;
    int timeout = hasTimeout ? info[4].As<Number>().Int32Value() : 0;

    return Number::New(env, rtms.join(uuid, session_id, signature, signal_url, timeout));
}

// Init
Object Init(Env env, Object exports) {
   exports.Set(String::New(env, "init"),
                Function::New(env, init));

    exports.Set(String::New(env, "setOnJoinConfirm"),
                Function::New(env, setOnJoinConfirm));

    exports.Set(String::New(env,"join"),
                Function::New(env, join));
    return exports;
}



NODE_API_MODULE("rtms-sdk", Init);