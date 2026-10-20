// Copyright (c) 2026 Eng. Ahmed Yasir — AY Zygisk
// Original AY Zygisk API-v5 smoke-test bridge.
// No hooking, hiding, process-name collection, or system modification.
// Built only against the upstream official zygisk.hpp header.
#include "zygisk.hpp"

class AYBridge final : public zygisk::ModuleBase {
public:
    void onLoad(zygisk::Api* api, JNIEnv* env) override {
        api_ = api;
        (void)env;
    }

    void preAppSpecialize(zygisk::AppSpecializeArgs* args) override {
        (void)args;
        unload_after_specialize();
    }

    void preServerSpecialize(zygisk::ServerSpecializeArgs* args) override {
        (void)args;
        unload_after_specialize();
    }

private:
    void unload_after_specialize() {
        // No hooks installed: exit cleanly after callback dispatch.
        if (api_) api_->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
    }
    zygisk::Api* api_ = nullptr;
};

REGISTER_ZYGISK_MODULE(AYBridge)
