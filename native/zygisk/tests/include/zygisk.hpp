// TEST-ONLY stub. Never used for Android builds. Does NOT provide a Zygisk implementation.
#pragma once
#include <cstdint>
using JNIEnv = struct JNINativeInterfaceMock;
namespace zygisk {
struct AppSpecializeArgs {};
struct ServerSpecializeArgs {};
enum class Option { DLCLOSE_MODULE_LIBRARY };
class Api {
public:
  virtual ~Api() = default;
  virtual void setOption(Option) = 0;
};
class ModuleBase {
public:
  virtual ~ModuleBase() = default;
  virtual void onLoad(Api*, JNIEnv*) {}
  virtual void preAppSpecialize(AppSpecializeArgs*) {}
  virtual void preServerSpecialize(ServerSpecializeArgs*) {}
};
}
#define REGISTER_ZYGISK_MODULE(ModuleClass) \
    extern "C" zygisk::ModuleBase* ay_test_create() { return new ModuleClass(); }
