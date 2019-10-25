#include <algorithm>
#include <dlfcn.h>
#include <jni.h>
#include <jnivm.h>
#include <log.h>
#include <regex>
#include <string>

using namespace jnivm;

const char *ParseJNIType(const char *cur, const char *end, std::string &type) {
  switch (*cur) {
  case 'V':
    type = "void";
    break;
  case 'Z':
    type = "jboolean";
    break;
  case 'B':
    type = "jbyte";
    break;
  case 'S':
    type = "jshort";
    break;
  case 'I':
    type = "jint";
    break;
  case 'J':
    type = "jlong";
    break;
  case 'F':
    type = "jfloat";
    break;
  case 'D':
    type = "jdouble";
    break;
  case '[':
    cur = ParseJNIType(cur + 1, end, type);
    type = "Array<" + type + ">*";
    break;
  case 'L':
    auto cend = std::find(cur, end, ';');
    type = "Object<" +
           std::regex_replace(std::string(cur + 1, cend), std::regex("(/|\\$)"),
                              "::") +
           ">*";
    cur = cend;
    break;
  }
  return cur;
}

const char * ParseJNIValue(const char *cur, const char *end, va_list list, jvalue *value) {
    switch (*cur) {
    case 'V':
        // Void has size 0 ignore it
        break;
    case 'Z':
    case 'B':
    case 'S':
        // These are promoted to int (gcc warning)
        if(value) value->z = va_arg(list, int);
        break;
    case 'I':
        if(value) value->i = va_arg(list, jint);
        break;
    case 'J':
        if(value) value->j = va_arg(list, jlong);
        break;
    case 'F':
    case 'D':
        if(value) value->d = va_arg(list, jdouble);
        break;
    case '[':
        cur = ParseJNIValue(cur + 1, end, list, nullptr);
        if(value) value->l = va_arg(list, jobject);
        break;
    case 'L':
        cur = std::find(cur, end, ';');
        if(value) value->l = va_arg(list, jobject);
        break;
    case '(':
        return ParseJNIValue(cur + 1, end, list, value);
    }
    return cur + 1;
}

const char * GetParamCount(const char *cur, const char *end, size_t& count) {
    switch (*cur) {
    case '[':
        cur = GetParamCount(cur + 1, end, count);
        break;
    case 'L':
        cur = std::find(cur, end, ';');
        ++count;
        cur = GetParamCount(cur + 1, end, count);
        break;
    case 'V':
    case 'Z':
    case 'B':
    case 'S':
    case 'I':
    case 'J':
    case 'F':
    case 'D':
        ++count;
    case '(':
        return GetParamCount(cur + 1, end, count);
    }
    return cur;
}

ScopedVaList::~ScopedVaList() {
    va_end(list);
}

std::vector<jvalue> JValuesfromValist(va_list list, const char* signature) {
    std::vector<jvalue> values;
    signature++;
    const char* end = signature + strlen(signature);
    for(size_t i = 0; *signature != ')' && signature != end; ++i) {
        values.emplace_back();
        signature = ParseJNIValue(signature, end, list, &values.back());
    }
    return values;
}

void Declare(JNIEnv *env, const char *signature) {
  for (const char *cur = signature, *end = cur + strlen(cur); cur != end;
       cur++) {
    if (*cur == 'L') {
      auto cend = std::find(cur, end, ';');
      std::string classpath(cur + 1, cend);
      env->FindClass(classpath.data());
      cur = cend;
    }
  }
}

class Method {
public:
  std::string name;
  std::string signature;
  bool _static = false;
  void *nativehandle = 0;

  std::string GenerateHeader(const std::string &cname) {
    std::ostringstream ss;
    std::vector<std::string> parameters;
    std::string rettype;
    bool inarg = false;
    for (const char *cur = signature.data(), *end = cur + signature.length();
         cur != end; cur++) {
      std::string type;
      switch (*cur) {
      case '(':
        inarg = true;
        break;
      case ')':
        inarg = false;
        break;
      default:
        cur = ParseJNIType(cur, end, type);
      }
      if (!type.empty()) {
        if (inarg) {
          parameters.emplace_back(std::move(type));
        } else {
          rettype = std::move(type);
        }
      }
    }
    if (_static) {
      ss << "static ";
    }
    if (name == "<init>") {
      ss << cname;
    } else {
      ss << rettype << " " << name;
    }
    ss << "(";
    for (int i = 0; i < parameters.size(); i++) {
      if (i != 0) {
        ss << ", ";
      }
      ss << parameters[i];
    }
    ss << ")"
       << ";";
    return ss.str();
  }

  std::string GenerateStubs(std::string scope, const std::string &cname) {
    std::ostringstream ss;
    std::vector<std::string> parameters;
    std::string rettype;
    bool inarg = false;
    for (const char *cur = signature.data(), *end = cur + signature.length();
         cur != end; cur++) {
      std::string type;
      switch (*cur) {
      case '(':
        inarg = true;
        break;
      case ')':
        inarg = false;
        break;
      default:
        cur = ParseJNIType(cur, end, type);
      }
      if (!type.empty()) {
        if (inarg) {
          parameters.emplace_back(std::move(type));
        } else {
          rettype = std::move(type);
        }
      }
    }
    if (name == "<init>") {
      ss << scope << cname;
    } else {
      ss << rettype << " " << scope << name;
    }
    ss << "(";
    for (int i = 0; i < parameters.size(); i++) {
      if (i != 0) {
        ss << ", ";
      }
      ss << parameters[i] << " arg" << i;
    }
    ss << ") {\n    \n}\n\n";
    return ss.str();
  }

  std::string GenerateJNIBinding(std::string scope, const std::string &cname) {
    std::ostringstream ss;
    std::vector<std::string> parameters;
    std::string rettype;
    bool inarg = false;
    for (const char *cur = signature.data(), *end = cur + signature.length();
         cur != end; cur++) {
      std::string type;
      switch (*cur) {
      case '(':
        inarg = true;
        break;
      case ')':
        inarg = false;
        break;
      default:
        cur = ParseJNIType(cur, end, type);
      }
      if (!type.empty()) {
        if (inarg) {
          parameters.emplace_back(std::move(type));
        } else {
          rettype = std::move(type);
        }
      }
    }
    ss << "extern \"C\" ";
    auto cl = scope.substr(0, scope.length() - 2);
    if (name == "<init>") {
      scope += cname;
    } else {
      scope += name;
    }
    if (name == "<init>") {
      ss << "void ";
    } else {
      ss << rettype << " ";
    }
    ss << std::regex_replace(scope, std::regex("::"), "_") << "(";
    if (!_static) {
      ss << "Object<" << cl << ">* obj, ";
    }
    ss << "jvalue* values) {\n    ";
    if (!_static) {
      if (name != "<init>") {
        ss << "return (obj ? obj->value : nullptr)->" << name;
      } else {
        ss << "new (obj ? obj->value : nullptr) " << cl;
      }
    } else {
      ss << "return " << scope;
    }
    ss << "(";
    for (int i = 0; i < parameters.size(); i++) {
      if (i != 0) {
        ss << ", ";
      }
      ss << "(" << parameters[i] << "&)values[" << i << "]";
    }
    ss << ");\n}\n";
    return ss.str();
  }
};

class Field {
public:
  std::string name;
  std::string type;
  bool _static = false;
  void *getnativehandle;
  void *setnativehandle;

  std::string GenerateHeader() {
    std::ostringstream ss;
    std::string ctype;
    ParseJNIType(type.data(), type.data() + type.length(), ctype);
    if (_static) {
      ss << "static ";
    }
    ss << ctype << " " << name << ";";
    return ss.str();
  }

  std::string GenerateStubs(std::string scope, const std::string &cname) {
    if(!_static) return std::string();
    std::ostringstream ss;
    std::string rettype;
    ParseJNIType(type.data(), type.data() + type.length(), rettype);
    ss << rettype << " " << scope << name << " = " << rettype << "();\n\n";
    return ss.str();
  }

  std::string GenerateJNIBinding(std::string scope) {
    std::ostringstream ss;
    std::string rettype;
    ParseJNIType(type.data(), type.data() + type.length(), rettype);
    auto cl = scope.substr(0, scope.length() - 2);
    scope = std::regex_replace(scope, std::regex("::"), "_") + name;
    ss << "extern \"C\" " << rettype << " get_" << scope << "(";
    if (!_static) {
      ss << "Object<" << cl << ">* obj";
    }
    ss << ") {\n    return ";
    if (_static) {
      ss << cl << "::" << name;
    } else {
      ss << "obj->value->" << name;
    }
    ss << ";\n}\n\n";
    ss << "extern \"C\" void set_" << scope << "(";
    if (!_static) {
      ss << "Object<" << cl << ">* obj, ";
    }
    ss << rettype << " value) {\n    ";
    if (_static) {
      ss << cl << "::" << name;
    } else {
      ss << "obj->value->" << name;
    }
    ss << " = value;\n}\n\n";
    return ss.str();
  }
};

class Class {
public:
  std::string name;
  std::string nativeprefix;
  std::vector<std::shared_ptr<Class>> classes;
  std::vector<std::shared_ptr<Field>> fields;
  std::vector<std::shared_ptr<Method>> methods;

  std::string GenerateHeader(std::string scope) {
    std::ostringstream ss;
    scope += name;
    ss << "class " << scope << " {\npublic:\n";
    for (auto &cl : classes) {
      ss << std::regex_replace(cl->GeneratePreDeclaration(),
                               std::regex("(^|\n)([^\n]+)"), "$1    $2");
      ss << "\n";
    }
    for (auto &field : fields) {
      ss << std::regex_replace(field->GenerateHeader(),
                               std::regex("(^|\n)([^\n]+)"), "$1    $2");
      ss << "\n";
    }
    for (auto &method : methods) {
      ss << std::regex_replace(method->GenerateHeader(name),
                               std::regex("(^|\n)([^\n]+)"), "$1    $2");
      ss << "\n";
    }
    ss << "};";
    for (auto &cl : classes) {
      ss << "\n";
      ss << cl->GenerateHeader(scope + "::");
    }
    return ss.str();
  }

  std::string GeneratePreDeclaration() {
    std::ostringstream ss;
    ss << "class " << name << ";";
    return ss.str();
  }

  std::string GenerateStubs(std::string scope) {
    std::ostringstream ss;
    scope += name + "::";
    for (auto &cl : classes) {
      ss << cl->GenerateStubs(scope);
    }
    for (auto &field : fields) {
      ss << field->GenerateStubs(scope, name);
    }
    for (auto &method : methods) {
      ss << method->GenerateStubs(scope, name);
    }
    return ss.str();
  }

  std::string GenerateJNIBinding(std::string scope) {
    std::ostringstream ss;
    scope += name + "::";
    for (auto &cl : classes) {
      ss << cl->GenerateJNIBinding(scope);
    }
    for (auto &field : fields) {
      ss << field->GenerateJNIBinding(scope);
    }
    for (auto &method : methods) {
      ss << method->GenerateJNIBinding(scope, name);
    }
    return ss.str();
  }
};

class Namespace {
public:
  std::string name;
  std::vector<std::shared_ptr<Namespace>> namespaces;
  std::vector<std::shared_ptr<Class>> classes;

  std::string GenerateHeader(std::string scope) {
    std::ostringstream ss;
    if (name.length()) {
      scope += name + "::";
    }
    for (auto &cl : classes) {
      ss << cl->GenerateHeader(scope);
      ss << "\n";
    }
    for (auto &np : namespaces) {
      ss << np->GenerateHeader(scope);
      ss << "\n";
    }
    return ss.str();
  }

  std::string GeneratePreDeclaration() {
    std::ostringstream ss;
    bool indent = name.length();
    if (indent) {
      ss << "namespace " << name << " {\n";
    }
    for (auto &cl : classes) {
      ss << (indent
                 ? std::regex_replace(cl->GeneratePreDeclaration(),
                                      std::regex("(^|\n)([^\n]+)"), "$1    $2")
                 : cl->GeneratePreDeclaration());
      ss << "\n";
    }
    for (auto &np : namespaces) {
      ss << (indent
                 ? std::regex_replace(np->GeneratePreDeclaration(),
                                      std::regex("(^|\n)([^\n]+)"), "$1    $2")
                 : np->GeneratePreDeclaration());
      ss << "\n";
    }
    if (indent) {
      ss << "}";
    }
    return ss.str();
  }

  std::string GenerateStubs(std::string scope) {
    std::ostringstream ss;
    if (name.length()) {
      scope += name + "::";
    }
    for (auto &cl : classes) {
      ss << cl->GenerateStubs(scope);
    }
    for (auto &np : namespaces) {
      ss << np->GenerateStubs(scope);
    }
    return ss.str();
  }

  std::string GenerateJNIBinding(std::string scope) {
    std::ostringstream ss;
    if (name.length()) {
      scope += name + "::";
    }
    for (auto &cl : classes) {
      ss << cl->GenerateJNIBinding(scope);
    }
    for (auto &np : namespaces) {
      ss << np->GenerateJNIBinding(scope);
    }
    return ss.str();
  }
};

jint GetVersion(JNIEnv *) { Log::trace("jnienv", "GetVersion"); };
jclass DefineClass(JNIEnv *, const char *, jobject, const jbyte *, jsize) {
  Log::trace("jnienv", "DefineClass");
};
jclass FindClass(JNIEnv *env, const char *name) {
  Log::trace("jnienv", "FindClass %s", name);
  auto prefix = std::regex_replace(name, std::regex("(/|\\$)"), "_") + '_';
  auto end = name + strlen(name);
  auto pos = name;
  Namespace *cur = (Namespace *)env->functions->reserved0;
  while ((pos = std::find(name, end, '/')) != end) {
    std::string sname = std::string(name, pos);
    auto namsp = std::find_if(cur->namespaces.begin(), cur->namespaces.end(),
                              [&sname](std::shared_ptr<Namespace> &namesp) {
                                return namesp->name == sname;
                              });
    Namespace *next;
    if (namsp != cur->namespaces.end()) {
      next = namsp->get();
    } else {
      cur->namespaces.emplace_back(new Namespace());
      next = cur->namespaces.back().get();
      next->name = std::move(sname);
    }
    cur = next;
    name = pos + 1;
  }
  Class *curc = nullptr;
  do {
    pos = std::find(name, end, '$');
    std::string sname = std::string(name, pos);
    Class *next;
    if (curc) {
      auto cl = std::find_if(curc->classes.begin(), curc->classes.end(),
                             [&sname](std::shared_ptr<Class> &namesp) {
                               return namesp->name == sname;
                             });
      if (cl != curc->classes.end()) {
        next = cl->get();
      } else {
        curc->classes.emplace_back(new Class());
        next = curc->classes.back().get();
        next->name = std::move(sname);
      }
    } else {
      auto cl = std::find_if(cur->classes.begin(), cur->classes.end(),
                             [&sname](std::shared_ptr<Class> &namesp) {
                               return namesp->name == sname;
                             });
      if (cl != cur->classes.end()) {
        next = cl->get();
      } else {
        cur->classes.emplace_back(new Class());
        next = cur->classes.back().get();
        next->name = std::move(sname);
      }
    }
    curc = next;
    name = pos + 1;
  } while (pos != end);
  curc->nativeprefix = std::move(prefix);
  return (jclass)curc;
};
jmethodID FromReflectedMethod(JNIEnv *, jobject) {
  Log::trace("jnienv", "FromReflectedMethod");
};
jfieldID FromReflectedField(JNIEnv *, jobject) {
  Log::trace("jnienv", "FromReflectedField");
};
/* spec doesn't show jboolean parameter */
jobject ToReflectedMethod(JNIEnv *, jclass, jmethodID, jboolean) {
  Log::trace("jnienv", "ToReflectedMethod");
};
jclass GetSuperclass(JNIEnv *, jclass) {
  Log::trace("jnienv", "GetSuperclass");
};
jboolean IsAssignableFrom(JNIEnv *, jclass, jclass) {
  Log::trace("jnienv", "IsAssignableFrom");
};
/* spec doesn't show jboolean parameter */
jobject ToReflectedField(JNIEnv *, jclass, jfieldID, jboolean) {
  Log::trace("jnienv", "ToReflectedField");
};
jint Throw(JNIEnv *, jthrowable) { Log::trace("jnienv", "Throw"); };
jint ThrowNew(JNIEnv *, jclass, const char *) {
  Log::trace("jnienv", "ThrowNew");
};
jthrowable ExceptionOccurred(JNIEnv *) {
  Log::trace("jnienv", "ExceptionOccurred");
  return (jthrowable)1;
};
void ExceptionDescribe(JNIEnv *) { Log::trace("jnienv", "ExceptionDescribe"); };
void ExceptionClear(JNIEnv *) { Log::trace("jnienv", "ExceptionClear"); };
void FatalError(JNIEnv *, const char *) { Log::trace("jnienv", "FatalError"); };
jint PushLocalFrame(JNIEnv *, jint) { Log::trace("jnienv", "PushLocalFrame"); };
jobject PopLocalFrame(JNIEnv *, jobject ob) {
  Log::trace("jnienv", "PopLocalFrame");
};
jobject NewGlobalRef(JNIEnv *, jobject obj) {
  Log::trace("jnienv", "NewGlobalRef %d", (int)obj);
  return obj;
};
void DeleteGlobalRef(JNIEnv *, jobject) {
  Log::trace("jnienv", "DeleteGlobalRef");
};
void DeleteLocalRef(JNIEnv *, jobject) {
  Log::trace("jnienv", "DeleteLocalRef");
};
jboolean IsSameObject(JNIEnv *, jobject, jobject) {
  Log::trace("jnienv", "IsSameObject");
};
jobject NewLocalRef(JNIEnv *, jobject) { Log::trace("jnienv", "NewLocalRef"); };
jint EnsureLocalCapacity(JNIEnv *, jint) {
  Log::trace("jnienv", "EnsureLocalCapacity");
};
jobject AllocObject(JNIEnv *env, jclass cl) {
  Log::trace("jnienv", "AllocObject");
  // return (jobject) new Object<> { cl = cl };
};
jobject NewObject(JNIEnv *, jclass, jmethodID, ...) {
  Log::trace("jnienv", "NewObject");
};
jobject NewObjectV(JNIEnv *env, jclass cl, jmethodID mid, va_list list) {
  Log::trace("jnienv", "NewObjectV");
  auto obj = new Object<void>{.cl = cl, .value = 0};
  env->CallVoidMethodV((jobject)obj, mid, list);
  return (jobject)obj;
};
jobject NewObjectA(JNIEnv *, jclass, jmethodID, jvalue *) {
  Log::trace("jnienv", "NewObjectA");
};
jclass GetObjectClass(JNIEnv *env, jobject jo) {
  Log::trace("jnienv", "GetObjectClass %d", jo);
  return ((Object<void> *)jo)->cl;
};
jboolean IsInstanceOf(JNIEnv *, jobject, jclass) {
  Log::trace("jnienv", "IsInstanceOf");
};
jmethodID GetMethodID(JNIEnv *env, jclass cl, const char *str0,
                      const char *str1) {
  std::string &classname = ((Class *)cl)->name;
  Log::trace("jnienv", "GetMethodID(%s, '%s','%s')", classname.data(), str0,
             str1);
  auto cur = (Class *)cl;
  auto sname = str0;
  auto ssig = str1;
  auto ccl =
      std::find_if(cur->methods.begin(), cur->methods.end(),
                   [&sname, &ssig](std::shared_ptr<Method> &namesp) {
                     return namesp->name == sname && namesp->signature == ssig;
                   });
  Method *next;
  if (ccl != cur->methods.end()) {
    next = ccl->get();
  } else {
    cur->methods.emplace_back(new Method());
    next = cur->methods.back().get();
    next->name = std::move(sname);
    next->signature = std::move(ssig);
    Declare(env, next->signature.data());
    auto This = dlopen(nullptr, RTLD_LAZY);
    std::string symbol = ((Class *)cl)->nativeprefix +
                         (!strcmp(str0, "<init>") ? classname : str0);
    if (!(next->nativehandle = dlsym(This, symbol.data()))) {
      Log::trace("JNIBinding", "Unresolved symbol %s", symbol.data());
      Log::debug("Missing wrapper", "%s", next->GenerateJNIBinding(((Class *)cl)->nativeprefix, "").data());
    }
    dlclose(This);
  }
  return (jmethodID)next;
};

template <class T>
T CallMethod(JNIEnv * env, jobject obj, jmethodID id, jvalue * param) {
  auto mid = ((Method *)id);
  Log::trace("jnienv", "CallMethod %s", mid->name.data());
  if (mid->nativehandle) {
    return ((T(*)(jobject, jvalue *))mid->nativehandle)(obj, param);
  }
};

template <class T>
T CallMethod(JNIEnv * env, jobject obj, jmethodID id, va_list param) {
    return CallMethod<T>(env, obj, id, JValuesfromValist(param, ((Method *)id)->signature.data()).data());
};

template <class T>
T CallMethod(JNIEnv * env, jobject obj, jmethodID id, ...) {
    ScopedVaList param;
    size_t count;
    GetParamCount(((Method *)id)->signature.data(), ((Method *)id)->signature.data() + ((Method *)id)->signature.length(), count);
    va_start(param.list, count);
    return CallMethod<T>(env, obj, id, param.list);
};

jobject CallNonvirtualObjectMethod(JNIEnv *, jobject, jclass, jmethodID, ...) {
  Log::trace("jnienv", "CallNonvirtualObjectMethod");
};
jobject CallNonvirtualObjectMethodV(JNIEnv *, jobject, jclass, jmethodID,
                                    va_list) {
  Log::trace("jnienv", "CallNonvirtualObjectMethodV");
};
jobject CallNonvirtualObjectMethodA(JNIEnv *, jobject, jclass, jmethodID,
                                    jvalue *) {
  Log::trace("jnienv", "CallNonvirtualObjectMethodA");
};
jboolean CallNonvirtualBooleanMethod(JNIEnv *, jobject, jclass, jmethodID,
                                     ...) {
  Log::trace("jnienv", "CallNonvirtualBooleanMethod");
};
jboolean CallNonvirtualBooleanMethodV(JNIEnv *, jobject, jclass, jmethodID,
                                      va_list) {
  Log::trace("jnienv", "CallNonvirtualBooleanMethodV");
};
jboolean CallNonvirtualBooleanMethodA(JNIEnv *, jobject, jclass, jmethodID,
                                      jvalue *) {
  Log::trace("jnienv", "CallNonvirtualBooleanMethodA");
};
jbyte CallNonvirtualByteMethod(JNIEnv *, jobject, jclass, jmethodID, ...) {
  Log::trace("jnienv", "CallNonvirtualByteMethod");
};
jbyte CallNonvirtualByteMethodV(JNIEnv *, jobject, jclass, jmethodID, va_list) {
  Log::trace("jnienv", "CallNonvirtualByteMethodV");
};
jbyte CallNonvirtualByteMethodA(JNIEnv *, jobject, jclass, jmethodID,
                                jvalue *) {
  Log::trace("jnienv", "CallNonvirtualByteMethodA");
};
jchar CallNonvirtualCharMethod(JNIEnv *, jobject, jclass, jmethodID, ...) {
  Log::trace("jnienv", "CallNonvirtualCharMethod");
};
jchar CallNonvirtualCharMethodV(JNIEnv *, jobject, jclass, jmethodID, va_list) {
  Log::trace("jnienv", "CallNonvirtualCharMethodV");
};
jchar CallNonvirtualCharMethodA(JNIEnv *, jobject, jclass, jmethodID,
                                jvalue *) {
  Log::trace("jnienv", "CallNonvirtualCharMethodA");
};
jshort CallNonvirtualShortMethod(JNIEnv *, jobject, jclass, jmethodID, ...) {
  Log::trace("jnienv", "CallNonvirtualShortMethod");
};
jshort CallNonvirtualShortMethodV(JNIEnv *, jobject, jclass, jmethodID,
                                  va_list) {
  Log::trace("jnienv", "CallNonvirtualShortMethodV");
};
jshort CallNonvirtualShortMethodA(JNIEnv *, jobject, jclass, jmethodID,
                                  jvalue *) {
  Log::trace("jnienv", "CallNonvirtualShortMethodA");
};
jint CallNonvirtualIntMethod(JNIEnv *, jobject, jclass, jmethodID, ...) {
  Log::trace("jnienv", "CallNonvirtualIntMethod");
};
jint CallNonvirtualIntMethodV(JNIEnv *, jobject, jclass, jmethodID, va_list) {
  Log::trace("jnienv", "CallNonvirtualIntMethodV");
};
jint CallNonvirtualIntMethodA(JNIEnv *, jobject, jclass, jmethodID, jvalue *) {
  Log::trace("jnienv", "CallNonvirtualIntMethodA");
};
jlong CallNonvirtualLongMethod(JNIEnv *, jobject, jclass, jmethodID, ...) {
  Log::trace("jnienv", "CallNonvirtualLongMethod");
};
jlong CallNonvirtualLongMethodV(JNIEnv *, jobject, jclass, jmethodID, va_list) {
  Log::trace("jnienv", "CallNonvirtualLongMethodV");
};
jlong CallNonvirtualLongMethodA(JNIEnv *, jobject, jclass, jmethodID,
                                jvalue *) {
  Log::trace("jnienv", "CallNonvirtualLongMethodA");
};
jfloat CallNonvirtualFloatMethod(JNIEnv *, jobject, jclass, jmethodID, ...) {
  Log::trace("jnienv", "CallNonvirtualFloatMethod");
};
jfloat CallNonvirtualFloatMethodV(JNIEnv *, jobject, jclass, jmethodID,
                                  va_list) {
  Log::trace("jnienv", "CallNonvirtualFloatMethodV");
};
jfloat CallNonvirtualFloatMethodA(JNIEnv *, jobject, jclass, jmethodID,
                                  jvalue *) {
  Log::trace("jnienv", "CallNonvirtualFloatMethodA");
};
jdouble CallNonvirtualDoubleMethod(JNIEnv *, jobject, jclass, jmethodID, ...) {
  Log::trace("jnienv", "CallNonvirtualDoubleMethod");
};
jdouble CallNonvirtualDoubleMethodV(JNIEnv *, jobject, jclass, jmethodID,
                                    va_list) {
  Log::trace("jnienv", "CallNonvirtualDoubleMethodV");
};
jdouble CallNonvirtualDoubleMethodA(JNIEnv *, jobject, jclass, jmethodID,
                                    jvalue *) {
  Log::trace("jnienv", "CallNonvirtualDoubleMethodA");
};
void CallNonvirtualVoidMethod(JNIEnv *, jobject, jclass, jmethodID, ...) {
  Log::trace("jnienv", "CallNonvirtualVoidMethod");
};
void CallNonvirtualVoidMethodV(JNIEnv *, jobject, jclass, jmethodID, va_list) {
  Log::trace("jnienv", "CallNonvirtualVoidMethodV");
};
void CallNonvirtualVoidMethodA(JNIEnv *, jobject, jclass, jmethodID, jvalue *) {
  Log::trace("jnienv", "CallNonvirtualVoidMethodA");
};
jfieldID GetFieldID(JNIEnv *env, jclass cl, const char *name,
                    const char *type) {
  std::string &classname = ((Class *)cl)->name;
  Log::trace("jnienv", "GetFieldID(%s, '%s','%s')", classname.data(), name,
             type);
  auto cur = (Class *)cl;
  auto sname = name;
  auto ssig = type;
  auto ccl =
      std::find_if(cur->fields.begin(), cur->fields.end(),
                   [&sname, &ssig](std::shared_ptr<Field> &namesp) {
                     return namesp->name == sname && namesp->type == ssig;
                   });
  Field *next;
  if (ccl != cur->fields.end()) {
    next = ccl->get();
  } else {
    cur->fields.emplace_back(new Field());
    next = cur->fields.back().get();
    next->name = std::move(sname);
    next->type = std::move(ssig);
    Declare(env, next->type.data());
    auto This = dlopen(nullptr, RTLD_LAZY);
    std::string symbol = ((Class *)cl)->nativeprefix + name;
    std::string gsymbol = "get_" + symbol;
    std::string ssymbol = "set_" + symbol;
    if (!(next->setnativehandle = dlsym(This, ssymbol.data()))) {
      Log::trace("JNIBinding", "Unresolved symbol %s", symbol.data());
    }
    if (!(next->getnativehandle = dlsym(This, gsymbol.data()))) {
      Log::trace("JNIBinding", "Unresolved symbol %s", symbol.data());
    }
    dlclose(This);
  }
  return (jfieldID)next;
};

template <class T> T GetField(JNIEnv *, jobject obj, jfieldID id) {
  auto fid = ((Field *)id);
  Log::trace("jnienv", "GetField %s", fid->name.data());
  if (fid->getnativehandle) {
    return ((T(*)(jobject))fid->getnativehandle)(obj);
  }
}

template <class T> void SetField(JNIEnv *, jobject obj, jfieldID id, T value) {
  auto fid = ((Field *)id);
  Log::trace("jnienv", "SetField %s", fid->name.data());
  if (fid->getnativehandle) {
    ((void (*)(jobject, T))fid->setnativehandle)(obj, value);
  }
}

jmethodID GetStaticMethodID(JNIEnv *env, jclass cl, const char *str0,
                            const char *str1) {
  std::string &classname = ((Class *)cl)->name;
  Log::trace("jnienv", "GetStaticMethodID(%s, '%s','%s')", classname.data(),
             str0, str1);
  auto cur = (Class *)cl;
  auto sname = str0;
  auto ssig = str1;
  auto ccl =
      std::find_if(cur->methods.begin(), cur->methods.end(),
                   [&sname, &ssig](std::shared_ptr<Method> &namesp) {
                     return namesp->name == sname && namesp->signature == ssig;
                   });
  Method *next;
  if (ccl != cur->methods.end()) {
    next = ccl->get();
  } else {
    cur->methods.emplace_back(new Method());
    next = cur->methods.back().get();
    next->name = std::move(sname);
    next->signature = std::move(ssig);
    next->_static = true;
    Declare(env, next->signature.data());
    auto This = dlopen(nullptr, RTLD_LAZY);
    std::string symbol = ((Class *)cl)->nativeprefix + str0;
    if (!(next->nativehandle = dlsym(This, symbol.data()))) {
      Log::trace("JNIBinding", "Unresolved symbol %s", symbol.data());
    }
    dlclose(This);
  }
  return (jmethodID)next;
};

template <class T>
T CallStaticMethod(JNIEnv * env, jclass cl, jmethodID id, jvalue * param) {
  auto mid = ((Method *)id);
  Log::trace("jnienv", "CallStaticMethod %s", mid->name.data());
  if (mid->nativehandle) {
    return ((T(*)(jvalue *))mid->nativehandle)(param);
  }
};

template <class T>
T CallStaticMethod(JNIEnv * env, jclass cl, jmethodID id, va_list param) {
    return CallStaticMethod<T>(env, cl, id, JValuesfromValist(param, ((Method *)id)->signature.data()).data());
};

template <class T>
T CallStaticMethod(JNIEnv * env, jclass cl, jmethodID id, ...) {
    ScopedVaList param;
    size_t count;
    GetParamCount(((Method *)id)->signature.data(), ((Method *)id)->signature.data() + ((Method *)id)->signature.length(), count);
    va_start(param.list, count);
    return CallStaticMethod<T>(env, cl, id, param.list);
};

jfieldID GetStaticFieldID(JNIEnv *env, jclass cl, const char *name,
                          const char *type) {
  std::string &classname = ((Class *)cl)->name;
  Log::trace("jnienv", "GetStaticFieldID(%s, '%s','%s')", classname.data(), name,
             type);
  auto cur = (Class *)cl;
  auto sname = name;
  auto ssig = type;
  auto ccl =
      std::find_if(cur->fields.begin(), cur->fields.end(),
                   [&sname, &ssig](std::shared_ptr<Field> &namesp) {
                     return namesp->name == sname && namesp->type == ssig;
                   });
  Field *next;
  if (ccl != cur->fields.end()) {
    next = ccl->get();
  } else {
    cur->fields.emplace_back(new Field());
    next = cur->fields.back().get();
    next->name = std::move(sname);
    next->type = std::move(ssig);
    next->_static = true;
    Declare(env, next->type.data());
    auto This = dlopen(nullptr, RTLD_LAZY);
    std::string symbol = ((Class *)cl)->nativeprefix + name;
    std::string gsymbol = "get_" + symbol;
    std::string ssymbol = "set_" + symbol;
    if (!(next->setnativehandle = dlsym(This, ssymbol.data()))) {
      Log::trace("JNIBinding", "Unresolved symbol %s", symbol.data());
    }
    if (!(next->getnativehandle = dlsym(This, gsymbol.data()))) {
      Log::trace("JNIBinding", "Unresolved symbol %s", symbol.data());
    }
    dlclose(This);
  }
  return (jfieldID)next;
};

template <class T> T GetStaticField(JNIEnv *, jclass cl, jfieldID id) {
  auto fid = ((Field *)id);
  Log::trace("jnienv", "GetStaticField %s", fid->name.data());
  if (fid->getnativehandle) {
    return ((T(*)())fid->getnativehandle)();
  }
}

template <class T>
void SetStaticField(JNIEnv *, jclass cl, jfieldID id, T value) {
  auto fid = ((Field *)id);
  Log::trace("jnienv", "SetStaticField %s", fid->name.data());
  if (fid->getnativehandle) {
    ((void (*)(T))fid->setnativehandle)(value);
  }
}

jstring NewString(JNIEnv *, const jchar *, jsize) {
  Log::trace("jnienv", "NewString");
};
jsize GetStringLength(JNIEnv *, jstring) {
  Log::trace("jnienv", "GetStringLength");
};
const jchar *GetStringChars(JNIEnv *, jstring, jboolean *) {
  Log::trace("jnienv", "GetStringChars");
};
void ReleaseStringChars(JNIEnv *, jstring, const jchar *) {
  Log::trace("jnienv", "ReleaseStringChars");
};
jstring NewStringUTF(JNIEnv *, const char *str) {
  Log::trace("jnienv", "NewStringUTF %s", str);
  return (jstring)(
      new Object<std::string>{.cl = 0, .value = new std::string(str)});
};
jsize GetStringUTFLength(JNIEnv *, jstring str) {
  Log::trace("jnienv", "GetStringUTFLength");
  return str && ((Object<std::string> *)str)->value
             ? ((Object<std::string> *)str)->value->length() : 36;
};
/* JNI spec says this returns const jbyte*, but that's inconsistent */
const char *GetStringUTFChars(JNIEnv *, jstring str, jboolean *copy) {
  if (copy) {
    *copy = false;
  }
  Log::trace("jnienv", "GetStringUTFChars");
  return str && ((Object<std::string> *)str)->value
             ? ((Object<std::string> *)str)->value->data()
             : "daa78df1-373a-444d-9b1d-4c71a14bb559";
};
void ReleaseStringUTFChars(JNIEnv *, jstring, const char *) {
  Log::trace("jnienv", "ReleaseStringUTFChars");
};
jsize GetArrayLength(JNIEnv *, jarray a) {
  Log::trace("jnienv", "GetArrayLength");
  return a ? ((Array<void> *)a)->length : 0;
};
jobjectArray NewObjectArray(JNIEnv *, jsize size, jclass c, jobject init) {
  Log::trace("jnienv", "NewObjectArray");
  return (jobjectArray)new Array<jobject> { .cl = 0, .value = new jobject[size] {init} , .length = size };
};
jobject GetObjectArrayElement(JNIEnv *, jobjectArray a, jsize i ) {
  Log::trace("jnienv", "GetObjectArrayElement");
  return ((Array<jobject>*)a)->value[i];
};
void SetObjectArrayElement(JNIEnv *, jobjectArray a, jsize i, jobject v) {
  Log::trace("jnienv", "SetObjectArrayElement");
  ((Array<jobject>*)a)->value[i] = v;
};

template <class T> typename JNITypes<T>::Array NewArray(JNIEnv * env, jsize size) {
  Log::trace("jnienv", "NewArray");
  return (typename JNITypes<T>::Array)new Array<T> { .cl = 0, .value = new T[size], .length = size };
};

template <class T>
T *GetArrayElements(JNIEnv *, typename JNITypes<T>::Array a, jboolean *iscopy) {
  Log::trace("jnienv", "GetArrayElements");
  if (iscopy) {
    *iscopy = false;
  }
  return a ? ((Array<T> *)a)->value : 0;
};

template <class T>
void ReleaseArrayElements(JNIEnv *, typename JNITypes<T>::Array a, T *carr, jint) {
  Log::trace("jnienv", "ReleaseArrayElements");
};

template <class T>
void GetArrayRegion(JNIEnv *, typename JNITypes<T>::Array a, jsize start, jsize len, T * buf) {
  Log::trace("jnienv", "GetArrayRegion");
  auto arr = (Array<T> *)a;
  std::copy(arr->value + start, arr->value + start + len, buf);
};

/* spec shows these without const; some jni.h do, some don't */
template <class T>
void SetArrayRegion(JNIEnv *, typename JNITypes<T>::Array a, jsize start, jsize len, const T * buf) {
  Log::trace("jnienv", "SetArrayRegion");
  auto arr = (Array<T> *)a;
  std::copy(buf, buf + len, arr->value + start);
};

jint RegisterNatives(JNIEnv *env, jclass c, const JNINativeMethod *method,
                     jint i) {
  Log::trace("jnienv", "RegisterNatives");
};
jint UnregisterNatives(JNIEnv *, jclass) {
  Log::trace("jnienv", "UnregisterNatives");
};
jint MonitorEnter(JNIEnv *, jobject) { Log::trace("jnienv", "MonitorEnter"); };
jint MonitorExit(JNIEnv *, jobject) { Log::trace("jnienv", "MonitorExit"); };
jint GetJavaVM(JNIEnv *, JavaVM **) { Log::trace("jnienv", "GetJavaVM"); };
void GetStringRegion(JNIEnv *, jstring, jsize, jsize, jchar *) {
  Log::trace("jnienv", "GetStringRegion");
};
void GetStringUTFRegion(JNIEnv *, jstring, jsize, jsize, char *) {
  Log::trace("jnienv", "GetStringUTFRegion");
};
void *GetPrimitiveArrayCritical(JNIEnv *, jarray, jboolean *) {
  Log::trace("jnienv", "GetPrimitiveArrayCritical");
};
void ReleasePrimitiveArrayCritical(JNIEnv *, jarray, void *, jint) {
  Log::trace("jnienv", "ReleasePrimitiveArrayCritical");
};
const jchar *GetStringCritical(JNIEnv *, jstring, jboolean *) {
  Log::trace("jnienv", "GetStringCritical");
};
void ReleaseStringCritical(JNIEnv *, jstring, const jchar *) {
  Log::trace("jnienv", "ReleaseStringCritical");
};
jweak NewWeakGlobalRef(JNIEnv *, jobject) {
  Log::trace("jnienv", "NewWeakGlobalRef");
};
void DeleteWeakGlobalRef(JNIEnv *, jweak) {
  Log::trace("jnienv", "DeleteWeakGlobalRef");
};
jboolean ExceptionCheck(JNIEnv *) { Log::trace("jnienv", "ExceptionCheck"); };
jobject NewDirectByteBuffer(JNIEnv *, void *, jlong) {
  Log::trace("jnienv", "NewDirectByteBuffer");
};
void *GetDirectBufferAddress(JNIEnv *, jobject) {
  Log::trace("jnienv", "GetDirectBufferAddress");
};
jlong GetDirectBufferCapacity(JNIEnv *, jobject) {
  Log::trace("jnienv", "GetDirectBufferCapacity");
};
/* added in JNI 1.6 */
jobjectRefType GetObjectRefType(JNIEnv *, jobject) {
  Log::trace("jnienv", "GetObjectRefType");
};

JavaVM *jnivm::createJNIVM() {
  return new JavaVM{new JNIInvokeInterface{
      new JNIEnv{new JNINativeInterface{
          new Namespace(),
          NULL,
          NULL,
          NULL,
          GetVersion,
          DefineClass,
          FindClass,
          FromReflectedMethod,
          FromReflectedField,
          /* spec doesn't show jboolean parameter */
          ToReflectedMethod,
          GetSuperclass,
          IsAssignableFrom,
          /* spec doesn't show jboolean parameter */
          ToReflectedField,
          Throw,
          ThrowNew,
          ExceptionOccurred,
          ExceptionDescribe,
          ExceptionClear,
          FatalError,
          PushLocalFrame,
          PopLocalFrame,
          NewGlobalRef,
          DeleteGlobalRef,
          DeleteLocalRef,
          IsSameObject,
          NewLocalRef,
          EnsureLocalCapacity,
          AllocObject,
          NewObject,
          NewObjectV,
          NewObjectA,
          GetObjectClass,
          IsInstanceOf,
          GetMethodID,
          CallMethod<jobject>,
          CallMethod<jobject>,
          CallMethod<jobject>,
          CallMethod<jboolean>,
          CallMethod<jboolean>,
          CallMethod<jboolean>,
          CallMethod<jbyte>,
          CallMethod<jbyte>,
          CallMethod<jbyte>,
          CallMethod<jchar>,
          CallMethod<jchar>,
          CallMethod<jchar>,
          CallMethod<jshort>,
          CallMethod<jshort>,
          CallMethod<jshort>,
          CallMethod<jint>,
          CallMethod<jint>,
          CallMethod<jint>,
          CallMethod<jlong>,
          CallMethod<jlong>,
          CallMethod<jlong>,
          CallMethod<jfloat>,
          CallMethod<jfloat>,
          CallMethod<jfloat>,
          CallMethod<jdouble>,
          CallMethod<jdouble>,
          CallMethod<jdouble>,
          CallMethod<void>,
          CallMethod<void>,
          CallMethod<void>,
          CallNonvirtualObjectMethod,
          CallNonvirtualObjectMethodV,
          CallNonvirtualObjectMethodA,
          CallNonvirtualBooleanMethod,
          CallNonvirtualBooleanMethodV,
          CallNonvirtualBooleanMethodA,
          CallNonvirtualByteMethod,
          CallNonvirtualByteMethodV,
          CallNonvirtualByteMethodA,
          CallNonvirtualCharMethod,
          CallNonvirtualCharMethodV,
          CallNonvirtualCharMethodA,
          CallNonvirtualShortMethod,
          CallNonvirtualShortMethodV,
          CallNonvirtualShortMethodA,
          CallNonvirtualIntMethod,
          CallNonvirtualIntMethodV,
          CallNonvirtualIntMethodA,
          CallNonvirtualLongMethod,
          CallNonvirtualLongMethodV,
          CallNonvirtualLongMethodA,
          CallNonvirtualFloatMethod,
          CallNonvirtualFloatMethodV,
          CallNonvirtualFloatMethodA,
          CallNonvirtualDoubleMethod,
          CallNonvirtualDoubleMethodV,
          CallNonvirtualDoubleMethodA,
          CallNonvirtualVoidMethod,
          CallNonvirtualVoidMethodV,
          CallNonvirtualVoidMethodA,
          GetFieldID,
          GetField<jobject>,
          GetField<jboolean>,
          GetField<jbyte>,
          GetField<jchar>,
          GetField<jshort>,
          GetField<jint>,
          GetField<jlong>,
          GetField<jfloat>,
          GetField<jdouble>,
          SetField<jobject>,
          SetField<jboolean>,
          SetField<jbyte>,
          SetField<jchar>,
          SetField<jshort>,
          SetField<jint>,
          SetField<jlong>,
          SetField<jfloat>,
          SetField<jdouble>,
          GetStaticMethodID,
          CallStaticMethod<jobject>,
          CallStaticMethod<jobject>,
          CallStaticMethod<jobject>,
          CallStaticMethod<jboolean>,
          CallStaticMethod<jboolean>,
          CallStaticMethod<jboolean>,
          CallStaticMethod<jbyte>,
          CallStaticMethod<jbyte>,
          CallStaticMethod<jbyte>,
          CallStaticMethod<jchar>,
          CallStaticMethod<jchar>,
          CallStaticMethod<jchar>,
          CallStaticMethod<jshort>,
          CallStaticMethod<jshort>,
          CallStaticMethod<jshort>,
          CallStaticMethod<jint>,
          CallStaticMethod<jint>,
          CallStaticMethod<jint>,
          CallStaticMethod<jlong>,
          CallStaticMethod<jlong>,
          CallStaticMethod<jlong>,
          CallStaticMethod<jfloat>,
          CallStaticMethod<jfloat>,
          CallStaticMethod<jfloat>,
          CallStaticMethod<jdouble>,
          CallStaticMethod<jdouble>,
          CallStaticMethod<jdouble>,
          CallStaticMethod<void>,
          CallStaticMethod<void>,
          CallStaticMethod<void>,
          GetStaticFieldID,
          GetStaticField<jobject>,
          GetStaticField<jboolean>,
          GetStaticField<jbyte>,
          GetStaticField<jchar>,
          GetStaticField<jshort>,
          GetStaticField<jint>,
          GetStaticField<jlong>,
          GetStaticField<jfloat>,
          GetStaticField<jdouble>,
          SetStaticField<jobject>,
          SetStaticField<jboolean>,
          SetStaticField<jbyte>,
          SetStaticField<jchar>,
          SetStaticField<jshort>,
          SetStaticField<jint>,
          SetStaticField<jlong>,
          SetStaticField<jfloat>,
          SetStaticField<jdouble>,
          NewString,
          GetStringLength,
          GetStringChars,
          ReleaseStringChars,
          NewStringUTF,
          GetStringUTFLength,
          /* JNI spec says this returns const jbyte*, but that's inconsistent */
          GetStringUTFChars,
          ReleaseStringUTFChars,
          GetArrayLength,
          NewObjectArray,
          GetObjectArrayElement,
          SetObjectArrayElement,
          NewArray<jboolean>,
          NewArray<jbyte>,
          NewArray<jchar>,
          NewArray<jshort>,
          NewArray<jint>,
          NewArray<jlong>,
          NewArray<jfloat>,
          NewArray<jdouble>,
          GetArrayElements<jboolean>,
          GetArrayElements<jbyte>,
          GetArrayElements<jchar>,
          GetArrayElements<jshort>,
          GetArrayElements<jint>,
          GetArrayElements<jlong>,
          GetArrayElements<jfloat>,
          GetArrayElements<jdouble>,
          ReleaseArrayElements<jboolean>,
          ReleaseArrayElements<jbyte>,
          ReleaseArrayElements<jchar>,
          ReleaseArrayElements<jshort>,
          ReleaseArrayElements<jint>,
          ReleaseArrayElements<jlong>,
          ReleaseArrayElements<jfloat>,
          ReleaseArrayElements<jdouble>,
          GetArrayRegion<jboolean>,
          GetArrayRegion<jbyte>,
          GetArrayRegion<jchar>,
          GetArrayRegion<jshort>,
          GetArrayRegion<jint>,
          GetArrayRegion<jlong>,
          GetArrayRegion<jfloat>,
          GetArrayRegion<jdouble>,
          /* spec shows these without const; some jni.h do, some don't */
          SetArrayRegion<jboolean>,
          SetArrayRegion<jbyte>,
          SetArrayRegion<jchar>,
          SetArrayRegion<jshort>,
          SetArrayRegion<jint>,
          SetArrayRegion<jlong>,
          SetArrayRegion<jfloat>,
          SetArrayRegion<jdouble>,
          RegisterNatives,
          UnregisterNatives,
          MonitorEnter,
          MonitorExit,
          GetJavaVM,
          GetStringRegion,
          GetStringUTFRegion,
          GetPrimitiveArrayCritical,
          ReleasePrimitiveArrayCritical,
          GetStringCritical,
          ReleaseStringCritical,
          NewWeakGlobalRef,
          DeleteWeakGlobalRef,
          ExceptionCheck,
          NewDirectByteBuffer,
          GetDirectBufferAddress,
          GetDirectBufferCapacity,
          /* added in JNI 1.6 */
          GetObjectRefType,
      }},
      NULL,
      NULL,
      [](JavaVM *) -> jint { Log::trace("jnivm", "DestroyJavaVM"); },
      [](JavaVM *, JNIEnv **, void *) -> jint {
        Log::trace("jnivm", "AttachCurrentThread");
      },
      [](JavaVM *) -> jint { Log::trace("jnivm", "DetachCurrentThread"); },
      [](JavaVM *vm, void **penv, jint) -> jint {
        Log::trace("jnivm", "GetEnv");
        *penv = vm->functions->reserved0;
        return JNI_OK;
      },
      [](JavaVM *, JNIEnv **, void *) -> jint {
        Log::trace("jnivm", "AttachCurrentThreadAsDaemon");
      },
  }};
}

std::string jnivm::GeneratePreDeclaration(JNIEnv * env) {
  return ((Namespace*&)env->functions->reserved0)->GeneratePreDeclaration();
}

std::string jnivm::GenerateHeader(JNIEnv * env) {
  return ((Namespace*&)env->functions->reserved0)->GenerateHeader("");
}

std::string jnivm::GenerateStubs(JNIEnv * env) {
  return ((Namespace*&)env->functions->reserved0)->GenerateStubs("");
}

std::string jnivm::GenerateJNIBinding(JNIEnv * env) {
  return ((Namespace*&)env->functions->reserved0)->GenerateJNIBinding("");
}