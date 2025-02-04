#include "napi.h"
#include <thread>
#include "rtms.h"

using namespace Napi;

auto rtms = RTMS("/tmp/rtms-js-sdk/ca.pem");
ThreadSafeFunction ts_onJoinConfirm;
static FunctionReference emit;


/**
 * Callback for on_join_confirm event
 * @param sdk
 * @param reason
 */
void onJoinConfirm(struct rtms_csdk* sdk, int reason) {
    auto callback = [reason, sdk](Env env, Function jsCallback) {
        jsCallback.Call({
            String::New(env, sdk->meeting_uuid),
            Number::New(env, reason)
        });
    };
    ts_onJoinConfirm.NonBlockingCall(callback);
    ts_onJoinConfirm.Release();
}

void onSessionUpdate(struct rtms_csdk* sdk,int,struct session_info* session_info) {
    return;
}



Boolean setOnJoinConfirm(const CallbackInfo& info) {
    auto env = info.Env();

    if (!info.Length() || !info[0].IsFunction())
        throw Error::New(env, "missing required callback");

    auto fn = info[0].As<Function>();

    ts_onJoinConfirm = ThreadSafeFunction::New(env, fn, "Callback", 0, 1);

    return Boolean::New(env, true);
}


Value connect(const CallbackInfo& info) {
    rtms.setOnJoinConfirm(onJoinConfirm);
    rtms.setOnSessionUpdate(onSessionUpdate);

    return info.Env().Null();
}

// Init
Object Init(Env env, Object exports) {
   exports.Set(String::New(env, "connect"),
                Function::New(env, connect));

    exports.Set(String::New(env, "setOnJoinConfirm"),
                Function::New(env, setOnJoinConfirm));
    return exports;
}



NODE_API_MODULE("rtms_js_sdk", Init);