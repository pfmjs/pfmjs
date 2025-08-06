#include "quickjs.h"
#include "quickjs-libc.h"
#include "cutils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    JSRuntime *rt = JS_NewRuntime();
    JSContext *ctx = JS_NewContext(rt);

    // Initialize std + os + console
    js_std_add_helpers(ctx, argc, argv);
    js_init_module_std(ctx, "std");
    js_init_module_os(ctx, "os");

    // Evaluate the JS script
    JSValue val = JS_Eval(ctx, code, strlen(code), filename, JS_EVAL_TYPE_GLOBAL);

    if (JS_IsException(val)) {
        JSValue exc = JS_GetException(ctx);
        const char *err_str = JS_ToCString(ctx, exc);
        fprintf(stderr, "Uncaught exception: %s\n", err_str);
        JS_FreeCString(ctx, err_str);
        JS_FreeValue(ctx, exc);
    }

    JS_FreeValue(ctx, val);
    js_std_loop(ctx);           // handle promises and pending jobs
    js_std_free_handlers(rt);   // clean up std handlers

    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
    free(code);

    return 0;
}
