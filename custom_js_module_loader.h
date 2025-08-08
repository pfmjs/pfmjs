#ifndef JS_MODULE_LOADER_H
#define JS_MODULE_LOADER_H

#include "quickjs.h"

JSModuleDef *js_module_loader(JSContext *ctx, const char *module_name, void *opaque);

#endif
#ifndef CUSTOM_JS_MODULE_LOADER_H

#include "quickjs.h"

JSModuleDef *custom_js_module_loader(JSContext *ctx, const char *module_name, void *opaque);

#endif