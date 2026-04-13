#include <stdio.h>
#include <string.h>
#include <direct.h>

#include "content.h"
#include "preview.h"
#include "scaffold.h"
#include "site.h"
#include "theme.h"
#include "util.h"

static SiteConfig g_config;
static Post g_posts[MAX_POSTS];
static Page g_pages[MAX_PAGES];
static Post g_sort_temp;

static void sort_posts_by_date(Post *posts, int post_count) {
    int i;
    int j;

    for (i = 0; i < post_count - 1; ++i) {
        for (j = i + 1; j < post_count; ++j) {
            if (strcmp(posts[j].date, posts[i].date) > 0) {
                g_sort_temp = posts[i];
                posts[i] = posts[j];
                posts[j] = g_sort_temp;
            }
        }
    }
}

static const char *ZINE_VERSION = "0.1.0";

static void print_usage(void) {
    printf("Usage:\n");
    printf("  zine.exe [-v]           Build site in current folder\n");
    printf("  zine.exe -V|--version   Print version information\n");
    printf("  zine.exe build DIR [-v] Build site in DIR\n");
    printf("  zine.exe init DIR  Create a starter site in DIR\n");
    printf("  zine.exe new SLUG  Create a new post in current folder\n");
    printf("  zine.exe newpage SLUG Create a new page in current folder\n");
    printf("  zine.exe delete SLUG Remove a post from current folder\n");
    printf("  zine.exe deletepage SLUG Remove a page from current folder\n");
    printf("  zine.exe clean     Remove generated site files\n");
    printf("  zine.exe serve [DIR] [PORT] Serve OUT preview, default port 8080\n");
}

static void print_version(void) {
    printf("Zine %s\n", ZINE_VERSION);
}

static const char *count_word(int count, const char *singular, const char *plural) {
    return count == 1 ? singular : plural;
}

static int is_verbose_arg(const char *arg) {
    return stricmp(arg, "-v") == 0 || stricmp(arg, "/v") == 0 || stricmp(arg, "verbose") == 0;
}

static int is_version_arg(const char *arg) {
    return stricmp(arg, "-V") == 0 || stricmp(arg, "--version") == 0 || stricmp(arg, "version") == 0;
}

static int parse_port_arg(const char *arg, int *port) {
    unsigned long value;
    const char *cursor;

    if (arg[0] == '\0') {
        return 0;
    }

    value = 0;
    cursor = arg;
    while (*cursor != '\0') {
        if (*cursor < '0' || *cursor > '9') {
            return 0;
        }
        value = value * 10 + (*cursor - '0');
        if (value > 65535) {
            return 0;
        }
        ++cursor;
    }

    if (value == 0) {
        return 0;
    }

    *port = (int)value;
    return 1;
}

static int is_output_dir_arg(const char *arg) {
    const char *end;
    const char *start;

    end = arg;
    while (*end != '\0') {
        ++end;
    }

    while (end > arg && (end[-1] == '\\' || end[-1] == '/')) {
        --end;
    }

    start = end;
    while (start > arg && start[-1] != '\\' && start[-1] != '/' && start[-1] != ':') {
        --start;
    }

    return (end - start) == 3
        && (start[0] == 'O' || start[0] == 'o')
        && (start[1] == 'U' || start[1] == 'u')
        && (start[2] == 'T' || start[2] == 't');
}

static int build_current_site(int verbose) {
    int post_count;
    int page_count;
    int changed_count;
    char font_path[MAX_LINE];
    char script_path[MAX_LINE];

    set_verbose_mode(verbose);
    if (!load_site_config("site.txt", &g_config)) {
        printf("%s\n", get_content_error());
        return 1;
    }

    post_count = load_posts("posts.lst", g_posts, MAX_POSTS);
    if (post_count < 0) {
        printf("%s\n", get_content_error());
        return 1;
    }

    page_count = load_pages("pages.lst", g_pages, MAX_PAGES);
    if (page_count < 0) {
        printf("%s\n", get_content_error());
        return 1;
    }

    if (!validate_post_page_output_names(g_posts, post_count, g_pages, page_count)) {
        printf("%s\n", get_content_error());
        return 1;
    }

    if (!ensure_dir(OUTPUT_DIR)) {
        printf("Failed to create %s directory\n", OUTPUT_DIR);
        return 1;
    }
    join_path(font_path, sizeof(font_path), OUTPUT_DIR, POST_OUTPUT_DIR);
    if (!ensure_dir(font_path)) {
        printf("Failed to create %s directory\n", font_path);
        return 1;
    }
    join_path(font_path, sizeof(font_path), OUTPUT_DIR, PAGE_OUTPUT_DIR);
    if (!ensure_dir(font_path)) {
        printf("Failed to create %s directory\n", font_path);
        return 1;
    }
    join_path(font_path, sizeof(font_path), OUTPUT_DIR, IMAGE_OUTPUT_DIR);
    if (!ensure_dir(font_path)) {
        printf("Failed to create %s directory\n", font_path);
        return 1;
    }
    join_path(font_path, sizeof(font_path), OUTPUT_DIR, FONT_OUTPUT_DIR);
    if (!ensure_dir(font_path)) {
        printf("Failed to create %s directory\n", font_path);
        return 1;
    }

    reset_theme_write_count();
    reset_scaffold_copy_count();
    reset_site_write_count();

    if (!write_theme_css(g_config.theme)) {
        printf("Failed to write theme CSS files\n");
        return 1;
    }

    join_path(font_path, sizeof(font_path), OUTPUT_DIR, FONT_OUTPUT_DIR);
    join_path(font_path, sizeof(font_path), font_path, "VGA.TTF");
    if (!write_theme_font(font_path)) {
        printf("Failed to write %s\n", font_path);
        return 1;
    }
    join_path(font_path, sizeof(font_path), OUTPUT_DIR, FONT_OUTPUT_DIR);
    join_path(font_path, sizeof(font_path), font_path, "CRT.TTF");
    if (!write_optional_theme_font("CRT.TTF", font_path)) {
        printf("Failed to write %s\n", font_path);
        return 1;
    }
    join_path(font_path, sizeof(font_path), OUTPUT_DIR, FONT_OUTPUT_DIR);
    join_path(font_path, sizeof(font_path), font_path, "W98.TTF");
    if (!write_optional_theme_font("W98.TTF", font_path)) {
        printf("Failed to write %s\n", font_path);
        return 1;
    }

    join_path(script_path, sizeof(script_path), OUTPUT_DIR, "THEME.JS");
    if (!write_theme_script(script_path)) {
        printf("Failed to write %s\n", script_path);
        return 1;
    }

    if (!copy_site_assets(g_posts, post_count, g_pages, page_count)) {
        printf("%s\n", get_scaffold_error());
        return 1;
    }

    sort_posts_by_date(g_posts, post_count);

    if (!generate_site(&g_config, g_posts, post_count, g_pages, page_count)) {
        printf("%s\n", get_site_error());
        return 1;
    }

    changed_count = get_theme_write_count() + get_scaffold_copy_count() + get_site_write_count();
    printf("Built %s: %d %s, %d %s, %d %s\n",
        OUTPUT_DIR,
        changed_count, count_word(changed_count, "change", "changes"),
        post_count, count_word(post_count, "post", "posts"),
        page_count, count_word(page_count, "page", "pages")
    );
    return 0;
}

static int serve_site(const char *site_dir, int port) {
    int build_rc;

    if (site_dir != NULL && site_dir[0] != '\0') {
        if (chdir(site_dir) != 0) {
            printf("Failed to enter %s\n", site_dir);
            return 1;
        }
    }

    build_rc = build_current_site(0);
    if (build_rc != 0) {
        return build_rc;
    }

    return serve_preview(NULL, port);
}

int main(int argc, char **argv) {
    int port;

    if (argc == 2 && is_version_arg(argv[1])) {
        print_version();
        return 0;
    }

    if (argc == 2 && is_verbose_arg(argv[1])) {
        return build_current_site(1);
    }

    if (argc == 2 && stricmp(argv[1], "serve") == 0) {
        return serve_site(NULL, 8080);
    }

    if (argc == 3 && stricmp(argv[1], "serve") == 0) {
        if (parse_port_arg(argv[2], &port)) {
            return serve_site(NULL, port);
        }

        if (is_output_dir_arg(argv[2])) {
            return serve_preview(argv[2], 8080);
        }

        return serve_site(argv[2], 8080);
    }

    if (argc == 4 && stricmp(argv[1], "serve") == 0) {
        if (!parse_port_arg(argv[3], &port)) {
            printf("Invalid port %s\n", argv[3]);
            return 1;
        }

        if (is_output_dir_arg(argv[2])) {
            return serve_preview(argv[2], port);
        }

        return serve_site(argv[2], port);
    }

    if (argc == 3 && stricmp(argv[1], "init") == 0) {
        if (!init_site(argv[2])) {
            printf("%s\n", get_scaffold_error());
            return 1;
        }

        printf("Created starter site in %s\n", argv[2]);
        return 0;
    }

    if (argc == 3 && stricmp(argv[1], "new") == 0) {
        if (!create_post(argv[2])) {
            printf("%s\n", get_scaffold_error());
            return 1;
        }

        printf("Created post %s\n", argv[2]);
        return 0;
    }

    if (argc == 3 && stricmp(argv[1], "newpage") == 0) {
        if (!create_page(argv[2])) {
            printf("%s\n", get_scaffold_error());
            return 1;
        }

        printf("Created page %s\n", argv[2]);
        return 0;
    }

    if (argc == 3 && stricmp(argv[1], "delete") == 0) {
        if (!delete_post(argv[2])) {
            printf("%s\n", get_scaffold_error());
            return 1;
        }

        printf("Deleted post %s\n", argv[2]);
        return 0;
    }

    if (argc == 3 && stricmp(argv[1], "deletepage") == 0) {
        if (!delete_page(argv[2])) {
            printf("%s\n", get_scaffold_error());
            return 1;
        }

        printf("Deleted page %s\n", argv[2]);
        return 0;
    }

    if (argc == 2 && stricmp(argv[1], "clean") == 0) {
        if (!clean_site()) {
            printf("Failed to clean site output\n");
            return 1;
        }

        printf("Cleaned generated site files\n");
        return 0;
    }

    if (argc == 3 && stricmp(argv[1], "build") == 0) {
        if (chdir(argv[2]) != 0) {
            printf("Failed to enter %s\n", argv[2]);
            return 1;
        }

        return build_current_site(0);
    }

    if (argc == 4 && stricmp(argv[1], "build") == 0 && is_verbose_arg(argv[3])) {
        if (chdir(argv[2]) != 0) {
            printf("Failed to enter %s\n", argv[2]);
            return 1;
        }

        return build_current_site(1);
    }

    if (argc != 1) {
        print_usage();
        return 1;
    }

    return build_current_site(0);
}
