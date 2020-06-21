#include <fstream>
#include <iostream>
#include <nan.h>
#include <node.h>

namespace uh_no_eval {

using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::String;
using v8::Value;

namespace dynamic_eval {

    const char *kEvalCallbackKey = "eval_callback_key";
    const char *kEvalPatternKey = "eval_pattern_key";
    const char *kEvalPattern = "(\\(\\s*function\\s+anonymous\\s*\\(\\s*\\)\\s*\\{)?"
                               "\\s*return\\s+this\\s*;?\\s*"
                               "(\\}\\s*\\))?\\s*";

    Local<v8::String> new_string(Local<v8::Context> ctx, const char *string_data) {
        return String::NewFromUtf8(ctx->GetIsolate(), string_data, v8::NewStringType::kInternalized).ToLocalChecked();
    }

    Local<v8::RegExp> getPattern(Local<v8::Context> ctx) {
        auto global = ctx->Global();
        auto key = new_string(ctx, kEvalPatternKey);
        Local<v8::Value> callback = Nan::GetPrivate(global, key).ToLocalChecked();
        return callback.As<v8::RegExp>();
    }

    bool codeIsGlobalThis(Local<v8::Context> ctx, Local<String> str) {
        auto regexp = getPattern(ctx);
        auto test_method = regexp->Get(new_string(ctx, "test")).As<v8::Function>();
        auto args = str.As<v8::Value>();
        auto ret = Nan::Call(test_method, regexp.As<v8::Object>(), 1, &args).ToLocalChecked().As<v8::Boolean>();
        return ret->IsTrue();
    }

    bool dynamic_callback(Local<v8::Context> ctx, Local<String> str) {
        auto global = ctx->Global();
        auto key = new_string(ctx, kEvalCallbackKey);
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

        if (!should_eval) {
            should_eval = codeIsGlobalThis(ctx, str);
        }
        return should_eval;
    }

    void set_eval_allowed(const FunctionCallbackInfo<Value> &args) {
        auto ctx = Nan::GetCurrentContext();
        auto isolate = ctx->GetIsolate();
        auto global = ctx->Global();
        auto private_key = String::NewFromUtf8(isolate, kEvalCallbackKey);

        auto bool_or_callback = args[0];

        if (bool_or_callback->IsBoolean()) {
            bool allowed = bool_or_callback.As<v8::Boolean>()->Value();
            if (allowed) {
                Nan::DeletePrivate(global, private_key);
                ctx->AllowCodeGenerationFromStrings(true);
            } else {
                Nan::SetPrivate(global, private_key, bool_or_callback);
                isolate->SetAllowCodeGenerationFromStringsCallback(dynamic_callback);
                ctx->AllowCodeGenerationFromStrings(false);
            }
        } else if (bool_or_callback->IsFunction()) {
            Nan::SetPrivate(global, private_key, bool_or_callback);
            ctx->AllowCodeGenerationFromStrings(false);
            isolate->SetAllowCodeGenerationFromStringsCallback(dynamic_callback);
        } else {
            Nan::ThrowTypeError("Argument 0 must be a function or boolean");
        }
    }
} // namespace dynamic_eval

void Initialize(Local<Object> exports) {
    auto ctx = Nan::GetCurrentContext();
    auto isolate = ctx->GetIsolate();
    auto global = ctx->Global();

    auto private_key = String::NewFromUtf8(isolate, dynamic_eval::kEvalPatternKey, v8::NewStringType::kNormal).ToLocalChecked();
    auto pattern_string = String::NewFromUtf8(isolate, dynamic_eval::kEvalPattern, v8::NewStringType::kNormal).ToLocalChecked();
    auto pattern = Nan::New<v8::RegExp>(pattern_string, v8::RegExp::kIgnoreCase).ToLocalChecked();

    Nan::SetPrivate(global, private_key, pattern);

    NODE_SET_METHOD(exports, "setEvalAllowed", dynamic_eval::set_eval_allowed);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)

} // namespace uh_no_eval
