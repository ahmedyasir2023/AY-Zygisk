/*
 * AY Runtime Lab -- original experimental plugin protocol, NOT Zygisk API.
 * Copyright (c) 2026 Eng. Ahmed Yasir. SPDX-License-Identifier: MIT.
 */
#ifndef AY_RUNTIME_API_H
#define AY_RUNTIME_API_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define AY_RUNTIME_ABI_V1 1u
#define AY_RUNTIME_PREPARE 1u
#define AY_RUNTIME_COMPLETE 2u
typedef struct ay_runtime_plugin_v1 {
    uint32_t abi_version;
    const char *id;
    /* Output must fit out_bytes, NUL-terminated. 0 means success. */
    int (*on_event)(uint32_t event, const char *context, char *out, size_t out_bytes);
} ay_runtime_plugin_v1;
typedef const ay_runtime_plugin_v1 *(*ay_runtime_get_plugin_v1)(void);
/* Exported by AY Runtime Lab TEST plugins: NOT the Zygisk module entrypoint. */
const ay_runtime_plugin_v1 *ay_runtime_get_v1(void);
#ifdef __cplusplus
}
#endif
#endif
