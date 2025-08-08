#include "quickjs-linux.h"
#include "quickjs-libc.h"
#include "cutils.h"
#include "custom_js_module_loader.h"

#include <stdio.h>
#ifdef __linux__
#include <arpa/inet.h>
#endif
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h> // for inet_pton and such
#include <windows.h>
#endif
#include <unistd.h>
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

void serve(int port,const char *filePath, const char *message) {
if (message != NULL && !filePath) {
    printf("message field should be Null if you want to set a file path\n");
}

if (!message && !filePath, !port) {
    printf("Usage: pfmServer(port, /* filepath: a specific file or if not avaible use messsage and let this be null */, /*message: if don't want to display a file add a message*/)\n");
}

if (port <= 0) {
    printf("port field can`t be Null\n");
}
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed to initialize Winsock: %d\n", WSAGetLastError());
        return;
    }
#endif

    int server_fd;
#ifdef _WIN32
    SOCKET new_socket;
#else
    int new_socket;
#endif

    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[4096] = {0};

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
#ifdef _WIN32
    if (server_fd == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return;
    }
#else
    if (server_fd == -1) {
        perror("socket failed");
        return;
    }
#endif

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

#ifdef _WIN32
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR)
#else
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0)
#endif
    {
        perror("bind failed");
#ifdef _WIN32
        closesocket(server_fd);
        WSACleanup();
#else
        close(server_fd);
#endif
        return;
    }

    if (listen(server_fd, 5)
#ifdef _WIN32
        == SOCKET_ERROR
#else
        < 0
#endif
    ) {
        perror("listen failed");
#ifdef _WIN32
        closesocket(server_fd);
        WSACleanup();
#else
        close(server_fd);
#endif
        return;
    }

    printf("Server running on http://localhost:%d\n", port);

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
#ifdef _WIN32
        if (new_socket == INVALID_SOCKET) {
            printf("Accept failed: %d\n", WSAGetLastError());
            break;
        }
#else
        if (new_socket < 0) {
            perror("accept failed");
            break;
        }
#endif

#ifdef _WIN32
        recv(new_socket, buffer, sizeof(buffer) - 1, 0);
#else
        read(new_socket, buffer, sizeof(buffer) - 1);
#endif
        printf("Request:\n%s\n", buffer);

        const char *response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n"
            "\r\n";

#ifdef _WIN32
        send(new_socket, response, strlen(response), 0);
        send(new_socket, message, strlen(message), 0);
        closesocket(new_socket);
#else
        send(new_socket, response, strlen(response), 0);
        send(new_socket, message, strlen(message), 0);
        close(new_socket);
#endif
    }

#ifdef _WIN32
    closesocket(server_fd);
    WSACleanup();
#else
    close(server_fd);
#endif
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

static JSValue __pfm_js_serve(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    int32_t port;
    const char *file_path = JS_ToCString(ctx, argv[1]);
    const char *message   = JS_ToCString(ctx, argv[2]);
    if (JS_ToInt32(ctx, &port, argv[0]))
        return JS_EXCEPTION;
    serve(port, file_path, message);
    JS_FreeCString(ctx, file_path);
    JS_FreeCString(ctx, message);
    return JS_UNDEFINED;
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
    JS_SetPropertyStr(ctx, global_obj, "pfmServe", JS_NewCFunction(ctx, __pfm_js_serve, "pfmServe", 3));
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