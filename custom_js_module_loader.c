#include "quickjs-libc.h"
#include "quickjs.h"

JSModuleDef *custom_js_module_loader(JSContext *ctx, const char *module_name, void *opaque) {
    size_t buf_len;
    uint8_t *buf;
    JSValue val;

    buf = js_load_file(ctx, &buf_len, module_name);
    if (!buf)
        return NULL;

    val = JS_Eval(ctx, (const char *)buf, buf_len, module_name, JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
    js_free(ctx, buf);

    if (JS_IsException(val))
        return NULL;

    return (JSModuleDef *)JS_VALUE_GET_PTR(val);
}
