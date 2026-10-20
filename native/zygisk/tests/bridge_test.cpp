#include "zygisk.hpp"
#include <cassert>
#include <cstdio>
extern "C" zygisk::ModuleBase* ay_test_create();
struct MockApi : zygisk::Api {
    int unload_calls = 0;
    void setOption(zygisk::Option opt) override {
        assert(opt == zygisk::Option::DLCLOSE_MODULE_LIBRARY);
        ++unload_calls;
    }
};
int main() {
    auto* m = ay_test_create();
    assert(m != nullptr);
    MockApi api;
    zygisk::AppSpecializeArgs app;
    zygisk::ServerSpecializeArgs server;
    m->preAppSpecialize(&app); // null api => must be safe
    m->onLoad(&api, nullptr);
    m->preAppSpecialize(&app);
    m->preServerSpecialize(&server);
    assert(api.unload_calls == 2);
    delete m;
    std::puts("AY API bridge mock callbacks: PASS (2 unload requests)");
}
