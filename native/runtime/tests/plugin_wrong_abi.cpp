#include "ay_runtime_api.h"
namespace {
int callback(uint32_t,const char*,char*,size_t) {return 0;}
const ay_runtime_plugin_v1 plugin{99,"wrong-abi",callback};
}
extern "C" const ay_runtime_plugin_v1* ay_runtime_get_v1() {return &plugin;}
