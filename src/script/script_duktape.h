#ifndef SCRIPT_DUKTAPE_H_INCLUDED
#define SCRIPT_DUKTAPE_H_INCLUDED

#include <string>
#include <duktape.h>

#include "nodeinfo.h"
#include "misc.h"

duk_context *duktape_init();
int duktape_push_nodeinfo(duk_context *ctx, const nodeInfo &node);
int duktape_push_nodeinfo_arr(duk_context *ctx, const nodeInfo &node, duk_idx_t index = -1);
int duktape_peval(duk_context *ctx, const std::string &script);
int duktape_call_function(duk_context *ctx, const std::string &name, size_t nargs, ...);
int duktape_get_res_int(duk_context *ctx);
std::string duktape_get_res_str(duk_context *ctx);
bool duktape_get_res_bool(duk_context *ctx);
std::string duktape_get_err_stack(duk_context *ctx);

#define SCRIPT_ENGINE_INIT(name) \
    duk_context* name = duktape_init(); \
    defer(duk_destroy_heap(name);)

#endif // SCRIPT_DUKTAPE_H_INCLUDED
