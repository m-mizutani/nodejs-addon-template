#include <node.h>
#include <unistd.h>
#include <v8.h>

// 趣味。特にnamespaceを切る必要はない
namespace addon {
  
  // ----------------------------------------------------------------------------
  // 引数を受け取って、普通に結果を返すだけの関数
  v8::Handle<v8::Value> Add(const v8::Arguments& args) {
    v8::HandleScope scope;

    // 引数はv8::Argumentsにまとめられている
    // 引数の個数を見る時はargs.Length ()
    if (args.Length() < 2) {
      // ThrowExceptionだがここで例外を投げて抜けるわけではない。
      // 例外のキューに入るイメージみたい
      v8::ThrowException(v8::Exception::TypeError(v8::String::New("Wrong number of arguments")));
      
      // 値を返さない時はUndefinedを返すのがお作法らしい
      return scope.Close(v8::Undefined());
    }

    // 数値や文字列などはC++標準ではなく、それぞれv8独自の型を使う
    // 型はわからないので、IsNumber()などを使って型を確認する
    if (!args[0]->IsNumber() || !args[1]->IsNumber()) {
      // 期待する型じゃなかったらException投げる
      v8::ThrowException(v8::Exception::TypeError(v8::String::New("Wrong arguments")));
      return scope.Close(v8::Undefined());
    }

    // 足し算するだけ
    // 返す値もv8の型にあわせる
    v8::Local<v8::Number> num = v8::Number::New(args[0]->NumberValue() +
                                                args[1]->NumberValue());
    return scope.Close(num);
  }



  // ----------------------------------------------------------------------------
  // 新しいオブジェクト（連想配列）を生成して返す
  v8::Handle<v8::Value> CreateObject(const v8::Arguments& args) {
    v8::HandleScope scope;

    // v8::Object::New ()で作成する。
    v8::Local<v8::Object> obj = v8::Object::New();
    
    // Setを使う。連想配列のキーはNewSymbolで生成する
    obj->Set(v8::String::NewSymbol("msg"), args[0]->ToString());

    return scope.Close(obj);
  }



  // ----------------------------------------------------------------------------
  // 処理が終わったらコールバックを呼び出す関数
  v8::Handle<v8::Value> RunCallback(const v8::Arguments& args) {
    v8::HandleScope scope;

    // 本当はちゃんとargs[0]が関数かどうかチェックする必要あり
    // v8::Functionにキャストする
    v8::Local<v8::Function> cb = v8::Local<v8::Function>::Cast(args[0]);
    
    // コールバックに渡す引数を作成する
    const unsigned argc = 1;
    v8::Local<v8::Value> argv[argc] = { 
      v8::Local<v8::Value>::New(v8::String::New("hello world")) 
    };
    // コールバック呼び出し
    cb->Call(v8::Context::GetCurrent()->Global(), argc, argv);

    return scope.Close(v8::Undefined());
  }



  // ----------------------------------------------------------------------------
  // 独自クラスを使う
  // node::ObjectWrapを継承する
  class Blue : public node::ObjectWrap {
  public:
    // クラス名や関数名を登録するため、初期化関数を静的に用意しておく
    static void init (v8::Handle<v8::Object> target);
    
  private:
    Blue () : count_(0) {}
    ~Blue () {}

    // v8から呼び出される関数はすべて静的関数にする
    // 引数const v8::Argumentsに固定
    static v8::Handle<v8::Value> New (const v8::Arguments &args);
    static v8::Handle<v8::Value> Five (const v8::Arguments &args);
    static v8::Handle<v8::Value> Count (const v8::Arguments &args);
    int count_;
  };

  // 初期化関数。v8のプロセスで一度しか呼び出されない（はず）
  void Blue::init (v8::Handle<v8::Object> target) {
    // FunctionTemplateを作成し、自前のオブジェクト生成関数New (newではない)を設定
    v8::Local<v8::FunctionTemplate> tmpl = v8::FunctionTemplate::New(New);
    
    // クラス名決める
    tmpl->SetClassName (v8::String::NewSymbol ("blue"));
    tmpl->InstanceTemplate ()->SetInternalFieldCount (1);

    // メンバ関数も名前とFunctionで登録
    tmpl->PrototypeTemplate ()->Set (v8::String::NewSymbol ("Five"), 
                                     v8::FunctionTemplate::New (Five)->GetFunction ());
    tmpl->PrototypeTemplate ()->Set (v8::String::NewSymbol ("Count"), 
                                     v8::FunctionTemplate::New (Count)->GetFunction ());

    // 最後にクラス生成の関数を登録
    // 多分、ここのシンボル名とSetClassNameの名前は変えられると思うが、
    // 混乱すると思うのであまりオススメできない＆未検証
    target->Set (v8::String::NewSymbol ("blue"), 
                 v8::Persistent<v8::Function>::New(tmpl->GetFunction ()));
  }
  
  // クラス生成関数
  v8::Handle<v8::Value> Blue::New (const v8::Arguments &args) {
    v8::HandleScope scope;
    // ここだけは普通にnew
    Blue * obj = new Blue ();
    // これを呼び出す必要がある
    obj->Wrap (args.This ());
    return args.This ();
  }

  v8::Handle<v8::Value> Blue::Five (const v8::Arguments &args) {
    v8::HandleScope scope;
    return scope.Close (v8::Number::New (5));
  }

  v8::Handle<v8::Value> Blue::Count (const v8::Arguments &args){
    v8::HandleScope scope;

    // node::ObjectWrap::Unwrap <クラス名> (args.This ()) することで、
    // インスタンスを取り出すことができる
    Blue * ao = node::ObjectWrap::Unwrap <Blue> (args.This ());
    
    // 後は好きにごにょごにょいじる
    ao->count_ += 1;
    return scope.Close (v8::Number::New (ao->count_));
  }


  // ----------------------------------------------------------------------------
  // 任意のタイミング（関数の終了時などではない）でコールバックを呼び出す
  class Orange : public node::ObjectWrap {
  public:
    static void init (v8::Handle<v8::Object> target);
  private:
    static v8::Handle<v8::Value> New (const v8::Arguments &args);
    static v8::Handle<v8::Value> SetCallback (const v8::Arguments & args);
    static v8::Handle<v8::Value> RunCallback (const v8::Arguments & args);
    
    // v8::Persistentでメンバ変数を用意するのが重要
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

    // v8::Persistent<v8::Function>::New でコールバック関数を保持するインスタンスを
    // 生成しなければならない
    orange->callback_ = 
      v8::Persistent<v8::Function>::New (v8::Local<v8::Function>::Cast(args[0]));

    return scope.Close(v8::Undefined());
  }

  v8::Handle<v8::Value> Orange::RunCallback (const v8::Arguments & args) {
    v8::HandleScope scope;
    Orange * orange = node::ObjectWrap::Unwrap <Orange> (args.This ());

    v8::Local<v8::Value> argv[1];    
    argv[0] = v8::String::New ("scar");
    
    // コールバック関数を呼び出す時は、もう一度v8::Localに
    v8::Local<v8::Function> cb = v8::Local<v8::Function>::New(orange->callback_);
    
    // コールバック！
    cb->Call (v8::Context::GetCurrent ()->Global (), 1, argv);

    return scope.Close(v8::Undefined());
  }



 // ----------------------------------------------------------------------------
 // libuvの機能を使った非同期処理

  // データをやりとりするためのクラス
  class Alice {
  public:
    v8::Persistent<v8::Function> callback_;
    long long res_;
  };

  // 処理をする関数
  void DoTask (uv_work_t *req) {
    // 適当になんかやらせる
    long long a = 1, p, q;
    p = q = a;
    for (long long n = 0; n < 10000000000; n++) {
      a = p + q;
      q = p;
      p = a;
    }

    // req->dataをキャストすると、そのまま受け渡したインスタンスが見える
    Alice * k = static_cast<Alice *> (req->data);
    k->res_ = a;
  }

  // 処理終了後の後始末関数
  void FinishTask (uv_work_t *req) {
    v8::HandleScope scope;

    Alice * a = static_cast<Alice *> (req->data);

    v8::Handle<v8::Value> argv[1];
    // 処理結果を受け取る
    argv[0] = v8::Integer::New (a->res_);

    v8::TryCatch try_catch;
    a->callback_->Call(v8::Context::GetCurrent()->Global(), 1, argv);

    // お片づけ
    a->callback_.Dispose();
    delete a;
    delete req;

    if (try_catch.HasCaught())
      node::FatalException(try_catch);
  }

  // 処理開始（この関数が呼び出される）
  v8::Handle<v8::Value> RunTask (const v8::Arguments& args) {
    v8::HandleScope scope;

    // uv_work_tを生成
    uv_work_t *req = new uv_work_t;
    
    // 結果をやりとりするためのクラスを生成
    Alice * a = new Alice ();
    
    // コールバック登録
    a->callback_ = v8::Persistent<v8::Function>::New(v8::Local<v8::Function>::Cast(args[0]));
    req->data = a;

    // uv_queue_work()でキューに入れる。
    uv_queue_work(uv_default_loop(), req, DoTask,
                  (uv_after_work_cb)FinishTask);

    return scope.Close (v8::Undefined());
  }

  // == Initializer ========================================================
  void Init(v8::Handle<v8::Object> target) {
    // targetに関数のシンボル（JavaScript側から呼び出すときの名前）と関数テンプレート化された
    // 関数を入れ込む
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


  // 初期化。"addon"という名前のアドオンになる
  // 二番目の引数は初期化
  NODE_MODULE(addon, Init);
}


