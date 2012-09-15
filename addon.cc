#define BUILDING_NODE_EXTENSION
#include <node.h>
#include <unistd.h>

namespace addon {
  
  // == a function with return value ==========================================
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



  // == a function with return value (object) =============================
  v8::Handle<v8::Value> CreateObject(const v8::Arguments& args) {
    v8::HandleScope scope;

    v8::Local<v8::Object> obj = v8::Object::New();
    obj->Set(v8::String::NewSymbol("msg"), args[0]->ToString());

    return scope.Close(obj);
  }



  // == a function with callback ==========================================
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



  // == using C++ class object =============================================
  class Blue : public node::ObjectWrap {
  public:
    static void init (v8::Handle<v8::Object> target);
  private:
    Blue () : count_(0) {}
    ~Blue () {}

    static v8::Handle<v8::Value> New (const v8::Arguments &args);
    static v8::Handle<v8::Value> Five (const v8::Arguments &args);
    static v8::Handle<v8::Value> Count (const v8::Arguments &args);
    int count_;
  };

  void Blue::init (v8::Handle<v8::Object> target) {
    v8::Local<v8::FunctionTemplate> tmpl = v8::FunctionTemplate::New(New);
    tmpl->SetClassName (v8::String::NewSymbol ("blue"));
    tmpl->InstanceTemplate ()->SetInternalFieldCount (1);

    tmpl->PrototypeTemplate ()->Set (v8::String::NewSymbol ("Five"), 
                                     v8::FunctionTemplate::New (Five)->GetFunction ());
    tmpl->PrototypeTemplate ()->Set (v8::String::NewSymbol ("Count"), 
                                     v8::FunctionTemplate::New (Count)->GetFunction ());

    target->Set (v8::String::NewSymbol ("blue"), 
                 v8::Persistent<v8::Function>::New(tmpl->GetFunction ()));
  }
  
  v8::Handle<v8::Value> Blue::New (const v8::Arguments &args) {
    v8::HandleScope scope;
    Blue * obj = new Blue ();
    obj->Wrap (args.This ());
    return args.This ();
  }

  v8::Handle<v8::Value> Blue::Five (const v8::Arguments &args) {
    v8::HandleScope scope;
    return scope.Close (v8::String::New ("not sane"));
  }

  v8::Handle<v8::Value> Blue::Count (const v8::Arguments &args){
    v8::HandleScope scope;
    Blue * ao = node::ObjectWrap::Unwrap <Blue> (args.This ());
    ao->count_ += 1;
    return scope.Close (v8::Number::New (ao->count_));
  }



  // == using C++ class object with callback object ========================
  class Orange : public node::ObjectWrap {
  public:
    static void init (v8::Handle<v8::Object> target);
  private:
    static v8::Handle<v8::Value> New (const v8::Arguments &args);
    static v8::Handle<v8::Value> SetCallback (const v8::Arguments & args);
    static v8::Handle<v8::Value> RunCallback (const v8::Arguments & args);
    v8::Persistent<v8::Function> callback_;
  };

  void Orange::init (v8::Handle<v8::Object> target) {
    v8::Local<v8::FunctionTemplate> tmpl = v8::FunctionTemplate::New(New);
    tmpl->SetClassName (v8::String::NewSymbol ("orange"));
    tmpl->InstanceTemplate ()->SetInternalFieldCount (1);

    tmpl->PrototypeTemplate ()->Set (v8::String::NewSymbol ("set_cb"), 
                                     v8::FunctionTemplate::New (SetCallback)->GetFunction ());
    tmpl->PrototypeTemplate ()->Set (v8::String::NewSymbol ("run_cb"), 
                                     v8::FunctionTemplate::New (RunCallback)->GetFunction ());

    target->Set (v8::String::NewSymbol ("orange"), 
                 v8::Persistent<v8::Function>::New(tmpl->GetFunction ()));
  }

  v8::Handle<v8::Value> Orange::New (const v8::Arguments &args) {
    v8::HandleScope scope;
    Orange * obj = new Orange ();
    obj->Wrap (args.This ());
    return args.This ();
  }

  v8::Handle<v8::Value> Orange::SetCallback (const v8::Arguments & args) {
    v8::HandleScope scope;
    Orange * orange = node::ObjectWrap::Unwrap <Orange> (args.This ());

    if (args.Length () != 1 || !args[0]->IsFunction ()) {
      v8::ThrowException (v8::Exception::TypeError (v8::String::New ("Invalid argument")));
      return scope.Close (v8::Undefined ());
    }

    orange->callback_ = 
      v8::Persistent<v8::Function>::New (v8::Local<v8::Function>::Cast(args[0]));

    return scope.Close(v8::Undefined());
  }

  v8::Handle<v8::Value> Orange::RunCallback (const v8::Arguments & args) {
    v8::HandleScope scope;
    Orange * orange = node::ObjectWrap::Unwrap <Orange> (args.This ());

    v8::Local<v8::Value> argv[1];    
    argv[0] = v8::String::New ("scar");
    v8::Local<v8::Function> cb = v8::Local<v8::Function>::New(orange->callback_);
    cb->Call (v8::Context::GetCurrent ()->Global (), 1, argv);

    return scope.Close(v8::Undefined());
  }





  // == Asynchronous processing =============================================
  class Alice {
  public:
    v8::Persistent<v8::Function> callback_;
  };

  void DoTask (uv_work_t *req) {
    /* Something computationally expensive here */
    sleep (5);
  }

  /* the "after work" callback; called on the main thread */
  void FinishTask (uv_work_t *req) {
    v8::HandleScope scope;

    Alice * a = static_cast<Alice *> (req->data);

    v8::Handle<v8::Value> argv[1];
    argv[0] = v8::String::New ("ploy");

    v8::TryCatch try_catch;
    a->callback_->Call(v8::Context::GetCurrent()->Global(), 1, argv);

    // cleanup
    a->callback_.Dispose();
    delete a;
    delete req;

    if (try_catch.HasCaught())
      node::FatalException(try_catch);
  }

  /* the JS entry point */
  v8::Handle<v8::Value> RunTask (const v8::Arguments& args) {
    v8::HandleScope scope;

    uv_work_t *req = new uv_work_t;
    Alice * a = new Alice ();
    a->callback_ = v8::Persistent<v8::Function>::New(v8::Local<v8::Function>::Cast(args[0]));
    req->data = a;

    uv_queue_work(uv_default_loop(), req, DoTask, FinishTask);

    return scope.Close (v8::Undefined());
  }

  // == Initializer ========================================================
  void Init(v8::Handle<v8::Object> target) {
    target->Set (v8::String::NewSymbol("add"),
                 v8::FunctionTemplate::New(Add)->GetFunction());
    target->Set (v8::String::NewSymbol("createObj"),
                 v8::FunctionTemplate::New(CreateObject)->GetFunction ());
    target->Set (v8::String::NewSymbol("runCallback"),
                 v8::FunctionTemplate::New(RunCallback)->GetFunction());
    target->Set (v8::String::NewSymbol("async_task"),
                 v8::FunctionTemplate::New(RunTask)->GetFunction());

    Blue::init (target);
    Orange::init (target);
  }



  NODE_MODULE(addon, Init);
}


