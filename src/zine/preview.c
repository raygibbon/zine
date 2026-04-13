#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <direct.h>

#include "content.h"
#include "preview.h"
#include "util.h"

static char g_docs_path[MAX_LINE];
static char g_abs_docs_path[MAX_LINE];
static char g_current_dir[MAX_LINE];
static char g_port_text[8];
static char g_wattcp_env[MAX_LINE + 16];

static int is_absolute_path(const char *path) {
    if (path == NULL || path[0] == '\0') {
        return 0;
    }

    if (((path[0] >= 'A' && path[0] <= 'Z') ||
         (path[0] >= 'a' && path[0] <= 'z')) &&
        path[1] == ':') {
        return 1;
    }

    if (path[0] == '\\' || path[0] == '/') {
        return 1;
    }

    return 0;
}

static int set_wattcp_dir(const char *dir_path) {
    if (dir_path == NULL || dir_path[0] == '\0') {
        return 0;
    }

    sprintf(g_wattcp_env, "WATTCP.CFG=%s", dir_path);
    return putenv(g_wattcp_env);
}

static void set_default_wattcp_config(void) {
    const char *existing;

    existing = getenv("WATTCP.CFG");
    if (existing != NULL && existing[0] != '\0') {
        return;
    }

    /*
        Watt-32 expects WATTCP.CFG to point to the directory
        containing WATTCP.CFG, not the file itself.
    */
    if (file_exists("WATTCP.CFG")) {
        set_wattcp_dir(".");
        return;
    }

    if (file_exists("..\\WATTCP.CFG")) {
        set_wattcp_dir("..");
    }
}

static int launch_zhttp(const char *exe_path, const char *docs_path, int port) {
    int rc;
    const char *wattcp_dir;

    if (exe_path == NULL || docs_path == NULL) {
        return -1;
    }

    if (!file_exists(exe_path)) {
        return -1;
    }

    sprintf(g_port_text, "%d", port);

    printf("Serving preview at http://localhost:%d\n", port);

    wattcp_dir = getenv("WATTCP.CFG");
    if (wattcp_dir != NULL && wattcp_dir[0] != '\0') {
        printf("Using Watt-32 config directory: %s\n", wattcp_dir);
    } else {
        printf("WATTCP.CFG not set\n");
    }

    rc = spawnl(P_WAIT, exe_path, "ZHTTP.EXE", docs_path, g_port_text, NULL);
    if (rc != 0) {
        printf("Networking not available (Watt-32 or packet driver not detected)\n");
        printf("Preview server requires DOSBox-X or a DOS TCP/IP stack\n");
    }

    return rc;
}

static int resolve_docs_path(const char *docs_dir) {
    if (docs_dir == NULL || docs_dir[0] == '\0' || strcmp(docs_dir, ".") == 0) {
        copy_string(g_docs_path, sizeof(g_docs_path), OUTPUT_DIR);
    } else {
        copy_string(g_docs_path, sizeof(g_docs_path), docs_dir);
    }

    if (!file_exists(g_docs_path)) {
        printf("Failed to find %s\n", g_docs_path);
        return 1;
    }

    if (getcwd(g_current_dir, sizeof(g_current_dir)) == NULL) {
        printf("Failed to read current directory\n");
        return 1;
    }

    if (is_absolute_path(g_docs_path)) {
        copy_string(g_abs_docs_path, sizeof(g_abs_docs_path), g_docs_path);
    } else {
        join_path(g_abs_docs_path, sizeof(g_abs_docs_path), g_current_dir, g_docs_path);
    }

    return 0;
}

int serve_preview(const char *docs_dir, int port) {
    int rc;

    rc = resolve_docs_path(docs_dir);
    if (rc != 0) {
        return rc;
    }

    set_default_wattcp_config();

    rc = launch_zhttp("ZHTTP.EXE", g_abs_docs_path, port);
    if (rc != -1) {
        return rc;
    }

    rc = launch_zhttp("..\\ZHTTP.EXE", g_abs_docs_path, port);
    if (rc != -1) {
        return rc;
    }

    printf("Failed to find ZHTTP.EXE\n");
    printf("Build zhttp.exe before using zine.exe serve\n");
    return 1;
}