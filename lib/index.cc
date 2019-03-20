#include <node.h>
#include <nan.h>
#include <iostream>
#include <fstream>

namespace uh_no_eval {

using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::String;
using v8::Value;

namespace dynamic_eval {

    const char* kEvalCallbackKey = "eval_callback_key";

    bool dynamic_callback(Local<v8::Context> ctx, Local<String> str) {
        auto global = ctx->Global();
        auto key = String::NewFromUtf8(ctx->GetIsolate(), kEvalCallbackKey);
        Local<v8::Value> callback = Nan::GetPrivate(global, key).ToLocalChecked();

        bool should_eval;
        if (callback->IsFunction()) {
            auto func = callback.As<v8::Function>();
            auto args = str.As<v8::Value>();
            auto ret = Nan::Call(func, Nan::Undefined().As<v8::Object>(), 1, &args).ToLocalChecked();
            should_eval = ret.As<v8::Boolean>()->Value();
        } else if (callback->IsBoolean()) {
            should_eval = callback.As<v8::Boolean>()->Value();
        } else {
            Nan::ThrowTypeError("Unable to find eval callback");
            return false;
        }
        return should_eval;
    }

    void set_eval_allowed(const FunctionCallbackInfo<Value>& args) {
        auto ctx = Nan::GetCurrentContext();
        auto isolate = ctx->GetIsolate();
        auto global = ctx->Global();
        auto private_key = String::NewFromUtf8(isolate, kEvalCallbackKey);

        auto bool_or_callback = args[0];

        if (bool_or_callback->IsBoolean()) {
            bool allowed = bool_or_callback.As<v8::Boolean>()->Value();
            Nan::DeletePrivate(global, private_key);
            isolate->SetAllowCodeGenerationFromStringsCallback(nullptr);
            ctx->AllowCodeGenerationFromStrings(allowed);
        } else if (bool_or_callback->IsFunction()) {
            Nan::SetPrivate(global, private_key, bool_or_callback);
            ctx->AllowCodeGenerationFromStrings(false);
            isolate->SetAllowCodeGenerationFromStringsCallback(dynamic_callback);
        } else {
            Nan::ThrowTypeError("Argument 0 must be a function or boolean");
        }
    }
}

void Initialize(Local<Object> exports) {
    NODE_SET_METHOD(exports, "setEvalAllowed", dynamic_eval::set_eval_allowed);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)

}