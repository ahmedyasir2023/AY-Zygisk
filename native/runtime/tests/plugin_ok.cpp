#include "ay_runtime_api.h"
#include <cstdio>
namespace {
int callback(uint32_t event,const char* ctx,char* out,size_t size) {
  if (!ctx || !out || size<12) return -1;
  snprintf(out,size,"event-%u-ok",event);
  return (event==AY_RUNTIME_PREPARE || event==AY_RUNTIME_COMPLETE)?0:-1;
}
const ay_runtime_plugin_v1 plugin{AY_RUNTIME_ABI_V1,"ay-fixture",callback};
}
extern "C" const ay_runtime_plugin_v1* ay_runtime_get_v1() {return &plugin;}
