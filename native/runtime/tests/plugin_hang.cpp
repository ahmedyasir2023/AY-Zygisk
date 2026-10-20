#include "ay_runtime_api.h"
#include <unistd.h>
namespace {
int callback(uint32_t,const char*,char*,size_t) {sleep(20);return 0;}
const ay_runtime_plugin_v1 plugin{AY_RUNTIME_ABI_V1,"hanging",callback};
}
extern "C" const ay_runtime_plugin_v1* ay_runtime_get_v1() {return &plugin;}
