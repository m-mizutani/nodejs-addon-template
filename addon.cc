#define BUILDING_NODE_EXTENSION
#include <node.h>

namespace addon {
  v8::Handle<v8::Value> Add(const v8::Arguments& args) {
    v8::HandleScope scope;

    if (args.Length() < 2) {
      v8::ThrowException(v8::Exception::TypeError(v8::String::New("Wrong number of arguments")));
      return scope.Close(v8::Undefined());
    }

    if (!args[0]->IsNumber() || !args[1]->IsNumber()) {
      v8::ThrowException(v8::Exception::TypeError(v8::String::New("Wrong arguments")));
      return scope.Close(v8::Undefined());
    }

    v8::Local<v8::Number> num = v8::Number::New(args[0]->NumberValue() +
                                                args[1]->NumberValue());
    return scope.Close(num);
  }

  v8::Handle<v8::Value> RunCallback(const v8::Arguments& args) {
    v8::HandleScope scope;

    v8::Local<v8::Function> cb = v8::Local<v8::Function>::Cast(args[0]);
    const unsigned argc = 1;
    v8::Local<v8::Value> argv[argc] = { 
      v8::Local<v8::Value>::New(v8::String::New("hello world")) 
    };
    cb->Call(v8::Context::GetCurrent()->Global(), argc, argv);

    return scope.Close(v8::Undefined());
  }

  void Init(v8::Handle<v8::Object> target) {
    target->Set(v8::String::NewSymbol("add"),
                v8::FunctionTemplate::New(Add)->GetFunction());
    target->Set(v8::String::NewSymbol("runCallback"),
                v8::FunctionTemplate::New(RunCallback)->GetFunction());
  }

  NODE_MODULE(addon, Init);
}


