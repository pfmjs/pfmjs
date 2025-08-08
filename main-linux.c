#include "quickjs.h"
#include "quickjs-libc.h"
#include "cutils.h"
#include "custom_js_module_loader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern JSModuleDef *custom_js_module_loader(JSContext *ctx, const char *module_name, void *opaque);

// Helper to write file contents
int __pfm_fs_write_helper(const char* path, const char* content) {
    FILE* file = fopen(path, "w");
    if (!file) return 0;
    fwrite(content, 1, strlen(content), file);
    fclose(file);
    return 1;
}

// JS-exposed fs.write(path, content)
static JSValue __pfm_js_fs_write(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    const char* path = JS_ToCString(ctx, argv[0]);
    const char* content = JS_ToCString(ctx, argv[1]);
    if (!path || !content) {
        if (path) JS_FreeCString(ctx, path);
        if (content) JS_FreeCString(ctx, content);
        return JS_EXCEPTION;
    }

    int success = __pfm_fs_write_helper(path, content);

    JS_FreeCString(ctx, path);
    JS_FreeCString(ctx, content);

    return JS_NewBool(ctx, success);
}

// JS-exposed fs.read(path)
static JSValue __pfm_js_fs_read(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    const char *filename = JS_ToCString(ctx, argv[0]);
    if (!filename)
        return JS_EXCEPTION;

    FILE *f = fopen(filename, "rb");
    if (!f) {
        JS_FreeCString(ctx, filename);
        return JS_ThrowReferenceError(ctx, "Could not open file: %s", filename);
    }

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);

    if (len < 0) {
        fclose(f);
        JS_FreeCString(ctx, filename);
        return JS_ThrowInternalError(ctx, "Failed to determine file size");
    }

    char *buffer = malloc(len + 1);
    if (!buffer) {
        fclose(f);
        JS_FreeCString(ctx, filename);
        return JS_ThrowInternalError(ctx, "Memory allocation failed");
    }

    size_t bytesRead = fread(buffer, 1, len, f);
    buffer[bytesRead] = '\0';

    JSValue result = JS_NewString(ctx, buffer);
    free(buffer);
    fclose(f);
    JS_FreeCString(ctx, filename);
    return result;
}

// Helper to read .js file
char *read_file(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *buffer = malloc(size + 1);
    if (!buffer) return NULL;

    fread(buffer, 1, size, f);
    buffer[size] = '\0';
    fclose(f);
    return buffer;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <script.js>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    char *code = read_file(filename);
    if (!code) {
        perror("Failed to read file");
        return 1;
    }

    // Runtime and context
    JSRuntime *rt = JS_NewRuntime();
    js_std_init_handlers(rt); // Handles std output, ctrl+c, etc.

    JSContext *ctx = JS_NewContext(rt);
    js_std_add_helpers(ctx, argc, argv);  // ✅ Adds 'console', std, os, etc.

    // Init modules
    js_init_module_std(ctx, "std");
    js_init_module_os(ctx, "os");

    // Custom module loader (your system)
    JS_SetModuleLoaderFunc(rt, NULL, custom_js_module_loader, NULL);

    // Bind native C file helpers
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global_obj, "__pfm_fs_read", JS_NewCFunction(ctx, __pfm_js_fs_read, "__pfm_fs_read", 1));
    JS_SetPropertyStr(ctx, global_obj, "__pfm_fs_write", JS_NewCFunction(ctx, __pfm_js_fs_write, "__pfm_fs_write", 2));
    JS_FreeValue(ctx, global_obj);

    // Compile as module
    JSValue mod = JS_Eval(ctx, code, strlen(code), filename, JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
    free(code);

    if (JS_IsException(mod)) {
        js_std_dump_error(ctx);
        JS_FreeContext(ctx);
        JS_FreeRuntime(rt);
        return 1;
    }

    // Evaluate compiled module
    JSValue eval_res = JS_EvalFunction(ctx, mod);
    if (JS_IsException(eval_res)) {
        js_std_dump_error(ctx);
    }
    JS_FreeValue(ctx, eval_res);

    // ✅ Ensures all stdout/stderr output is printed
    js_std_loop(ctx);
    fflush(stdout);

    js_std_free_handlers(rt);
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
    return 0;
}