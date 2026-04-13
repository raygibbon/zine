#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "content.h"
#include "scaffold.h"
#include "util.h"

static char g_scaffold_error[256];
static char g_asset_output_path[MAX_LINE];
static char g_scaffold_line[MAX_LINE];
static char g_scaffold_src_line[MAX_LINE];
static char g_scaffold_key[64];
static char g_scaffold_value[64];
static char g_scaffold_path[MAX_LINE];
static char g_scaffold_src_path[MAX_LINE];
static char g_scaffold_dst_path[MAX_LINE];
static char g_scaffold_asset_name[MAX_LINE];
static int g_scaffold_copy_count;

static void set_scaffold_error(const char *message) {
    copy_string(g_scaffold_error, sizeof(g_scaffold_error), message);
}

static int write_text_file(const char *path, const char *text) {
    FILE *fp;

    fp = fopen(path, "w");
    if (fp == NULL) {
        static char message[256];
        sprintf(message, "Failed to write %s", path);
        set_scaffold_error(message);
        return 0;
    }

    fputs(text, fp);
    fclose(fp);
    return 1;
}

static int scaffold_file_exists(const char *path) {
    FILE *fp;

    fp = open_existing_file(path, "r");
    if (fp == NULL) {
        return 0;
    }

    fclose(fp);
    return 1;
}

static int post_list_contains(const char *slug) {
    FILE *fp;
    char line[MAX_LINE];
    char expected[16];

    fp = open_existing_file("posts.lst", "r");
    if (fp == NULL) {
        return 0;
    }

    make_source_filename(expected, sizeof(expected), slug);

    while (fgets(line, sizeof(line), fp) != NULL) {
        trim_line_endings(line);
        trim_whitespace(line);
        lowercase_string(line);

        if (strings_equal(line, expected)) {
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return 0;
}

static int page_list_contains(const char *slug) {
    FILE *fp;
    char line[MAX_LINE];
    char expected[16];

    fp = open_existing_file("pages.lst", "r");
    if (fp == NULL) {
        return 0;
    }

    make_source_filename(expected, sizeof(expected), slug);

    while (fgets(line, sizeof(line), fp) != NULL) {
        trim_line_endings(line);
        trim_whitespace(line);
        lowercase_string(line);

        if (strings_equal(line, expected)) {
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return 0;
}

static int parse_key_value(const char *line, char *key, int key_size, char *value, int value_size) {
    const char *colon = strchr(line, ':');

    if (colon == NULL) {
        return 0;
    }

    copy_range_string(key, key_size, line, colon);
    trim_whitespace(key);

    ++colon;
    while (*colon == ' ' || *colon == '\t') {
        ++colon;
    }

    copy_string(value, value_size, colon);
    trim_whitespace(value);
    return 1;
}

static int append_post_list_entry(const char *slug) {
    FILE *fp;
    char source_name[16];

    fp = open_existing_file("posts.lst", "a");
    if (fp == NULL) {
        fp = fopen("posts.lst", "a");
        if (fp == NULL) {
            return 0;
        }
    }

    make_source_filename(source_name, sizeof(source_name), slug);
    fprintf(fp, "%s\n", source_name);
    fclose(fp);
    return 1;
}

static int append_page_list_entry(const char *slug) {
    FILE *fp;
    char source_name[16];

    fp = open_existing_file("pages.lst", "a");
    if (fp == NULL) {
        fp = fopen("pages.lst", "a");
        if (fp == NULL) {
            return 0;
        }
    }

    make_source_filename(source_name, sizeof(source_name), slug);
    fprintf(fp, "%s\n", source_name);
    fclose(fp);
    return 1;
}

static int remove_list_entry(const char *list_path, const char *slug) {
    FILE *src_fp;
    FILE *tmp_fp;
    int removed;
    char expected[16];

    src_fp = open_existing_file(list_path, "r");
    if (src_fp == NULL) {
        return 0;
    }

    tmp_fp = fopen("ZINE$LST.TMP", "w");
    if (tmp_fp == NULL) {
        fclose(src_fp);
        return 0;
    }

    make_source_filename(expected, sizeof(expected), slug);
    removed = 0;

    while (fgets(g_scaffold_line, sizeof(g_scaffold_line), src_fp) != NULL) {
        copy_string(g_scaffold_src_line, sizeof(g_scaffold_src_line), g_scaffold_line);
        trim_line_endings(g_scaffold_src_line);
        trim_whitespace(g_scaffold_src_line);
        lowercase_string(g_scaffold_src_line);

        if (!removed && strings_equal(g_scaffold_src_line, expected)) {
            removed = 1;
            continue;
        }

        fputs(g_scaffold_line, tmp_fp);
    }

    fclose(src_fp);
    fclose(tmp_fp);

    if (!removed) {
        remove("ZINE$LST.TMP");
        return 0;
    }

    remove(list_path);
    if (rename("ZINE$LST.TMP", list_path) != 0) {
        remove("ZINE$LST.TMP");
        return 0;
    }

    return 1;
}

static int read_output_name_from_source(const char *path, char *output_name, int output_size) {
    FILE *fp;

    output_name[0] = '\0';
    fp = open_existing_file(path, "r");
    if (fp == NULL) {
        return 0;
    }

    while (fgets(g_scaffold_line, sizeof(g_scaffold_line), fp) != NULL) {
        trim_line_endings(g_scaffold_line);
        if (g_scaffold_line[0] == '\0') {
            break;
        }

        if (!parse_key_value(g_scaffold_line, g_scaffold_key, sizeof(g_scaffold_key), g_scaffold_value, sizeof(g_scaffold_value))) {
            continue;
        }

        if (strings_equal(g_scaffold_key, "slug")) {
            make_output_name(output_name, output_size, g_scaffold_value);
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return 0;
}

static int read_output_name_from_manifest(char marker, const char *source_name, char *output_name, int output_size) {
    FILE *fp;
    char entry_source[16];
    char entry_output[16];

    output_name[0] = '\0';
    join_path(g_scaffold_path, sizeof(g_scaffold_path), OUTPUT_DIR, "BUILD.DAT");
    fp = open_existing_file(g_scaffold_path, "r");
    if (fp == NULL) {
        return 0;
    }

    while (fgets(g_scaffold_line, sizeof(g_scaffold_line), fp) != NULL) {
        trim_line_endings(g_scaffold_line);
        if (g_scaffold_line[0] != marker || g_scaffold_line[1] != ' ') {
            continue;
        }

        if (sscanf(g_scaffold_line + 2, "%15s %15s", entry_source, entry_output) == 2 &&
                strings_equal_ignore_case(entry_source, source_name)) {
            copy_string(output_name, output_size, entry_output);
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return 0;
}

static int is_valid_asset_entry(const char *entry) {
    return is_valid_dos_image_name(entry);
}

static void get_current_date(char *dest, int dest_size) {
    time_t now;
    struct tm *today;

    now = time(NULL);
    today = localtime(&now);

    if (today == NULL) {
        copy_string(dest, dest_size, "1996-12-03");
        return;
    }

    sprintf(dest, "%04d-%02d-%02d",
        today->tm_year + 1900,
        today->tm_mon + 1,
        today->tm_mday);
}

static void remove_if_exists(const char *path) {
    remove(path);
}

static void remove_generated_file(const char *name) {
    remove_if_exists(name);
    join_path(g_scaffold_path, sizeof(g_scaffold_path), OUTPUT_DIR, name);
    remove_if_exists(g_scaffold_path);
}

static void remove_generated_file_in(const char *folder, const char *name) {
    remove_if_exists(name);
    join_path(g_scaffold_path, sizeof(g_scaffold_path), OUTPUT_DIR, folder);
    join_path(g_scaffold_path, sizeof(g_scaffold_path), g_scaffold_path, name);
    remove_if_exists(g_scaffold_path);
}

static void remove_outputs_from_list(const char *list_path, const char *folder, const char *output_folder) {
    FILE *list_fp;
    FILE *src_fp;
    char output_name[80];

    list_fp = open_existing_file(list_path, "r");
    if (list_fp == NULL) {
        return;
    }

    while (fgets(g_scaffold_line, sizeof(g_scaffold_line), list_fp) != NULL) {
        trim_line_endings(g_scaffold_line);
        trim_whitespace(g_scaffold_line);

        if (g_scaffold_line[0] == '\0' || g_scaffold_line[0] == '#') {
            continue;
        }

        if (!is_valid_list_entry_name(g_scaffold_line)) {
            continue;
        }

        join_path(g_scaffold_src_path, sizeof(g_scaffold_src_path), folder, g_scaffold_line);
        src_fp = open_existing_file(g_scaffold_src_path, "r");
        if (src_fp == NULL) {
            continue;
        }

        while (fgets(g_scaffold_src_line, sizeof(g_scaffold_src_line), src_fp) != NULL) {
            trim_line_endings(g_scaffold_src_line);
            if (g_scaffold_src_line[0] == '\0') {
                break;
            }

            if (!parse_key_value(g_scaffold_src_line, g_scaffold_key, sizeof(g_scaffold_key), g_scaffold_value, sizeof(g_scaffold_value))) {
                continue;
            }

            if (strings_equal(g_scaffold_key, "slug")) {
                make_output_name(output_name, sizeof(output_name), g_scaffold_value);
                remove_generated_file_in(output_folder, output_name);
                break;
            }
        }

        fclose(src_fp);
    }

    fclose(list_fp);
}

static int copy_assets_from_list(const char *list_path, const char *src_folder, const char *dst_folder) {
    FILE *list_fp;

    list_fp = open_existing_file(list_path, "r");
    if (list_fp == NULL) {
        return 1;
    }

    if (!ensure_dir(dst_folder)) {
        fclose(list_fp);
            sprintf(g_scaffold_error, "Failed to create directory %s", dst_folder);
            return 0;
        }

    while (fgets(g_scaffold_line, sizeof(g_scaffold_line), list_fp) != NULL) {
        trim_line_endings(g_scaffold_line);
        trim_whitespace(g_scaffold_line);

        if (g_scaffold_line[0] == '\0' || g_scaffold_line[0] == '#') {
            continue;
        }

        if (!is_valid_asset_entry(g_scaffold_line)) {
            fclose(list_fp);
            sprintf(g_scaffold_error, "Invalid asset entry %s", g_scaffold_line);
            return 0;
        }

        join_path(g_scaffold_src_path, sizeof(g_scaffold_src_path), src_folder, g_scaffold_line);
        join_path(g_scaffold_dst_path, sizeof(g_scaffold_dst_path), dst_folder, g_scaffold_line);

        if (should_copy_file(g_scaffold_src_path, g_scaffold_dst_path)) {
            ++g_scaffold_copy_count;
            if (get_verbose_mode()) {
                printf("Copied asset %s\n", g_scaffold_line);
            }
        }

        if (!copy_file_if_needed(g_scaffold_src_path, g_scaffold_dst_path)) {
            fclose(list_fp);
            sprintf(g_scaffold_error, "Failed to copy asset %s", g_scaffold_line);
            return 0;
        }
    }

    fclose(list_fp);
    return 1;
}

static int parse_image_reference(const char *line, char *asset_name, int asset_size) {
    const char *alt_end;
    const char *url_start;
    const char *url_end;

    if (line[0] != '!' || line[1] != '[') {
        return 0;
    }

    alt_end = strchr(line + 2, ']');
    if (alt_end == NULL || alt_end[1] != '(') {
        return 0;
    }

    url_start = alt_end + 2;
    url_end = strchr(url_start, ')');
    if (url_end == NULL || url_end[1] != '\0') {
        return 0;
    }

    if (!starts_with_ignore_case(url_start, "IMG/") && !starts_with_ignore_case(url_start, "IMG\\")) {
        return 0;
    }

    url_start += 4;
    copy_range_string(asset_name, asset_size, url_start, url_end);
    trim_whitespace(asset_name);

    if (!is_valid_asset_entry(asset_name)) {
        sprintf(g_scaffold_error, "Invalid image filename %s", asset_name);
        return -1;
    }

    return 1;
}

static int copy_asset_if_needed(const char *asset_name, const char *dst_folder) {
    join_path(g_scaffold_src_path, sizeof(g_scaffold_src_path), IMAGE_OUTPUT_DIR, asset_name);
    join_path(g_scaffold_dst_path, sizeof(g_scaffold_dst_path), dst_folder, asset_name);

    if (should_copy_file(g_scaffold_src_path, g_scaffold_dst_path)) {
        ++g_scaffold_copy_count;
        if (get_verbose_mode()) {
            printf("Copied asset %s\n", asset_name);
        }
    }

    if (!copy_file_if_needed(g_scaffold_src_path, g_scaffold_dst_path)) {
        sprintf(g_scaffold_error, "Failed to copy asset %s", asset_name);
        return 0;
    }

    return 1;
}

static int copy_assets_from_source_file(const char *path, const char *dst_folder) {
    FILE *fp;
    int in_body;

    fp = open_existing_file(path, "r");
    if (fp == NULL) {
        return 1;
    }

    in_body = 0;

    while (fgets(g_scaffold_line, sizeof(g_scaffold_line), fp) != NULL) {
        trim_line_endings(g_scaffold_line);

        if (!in_body) {
            if (g_scaffold_line[0] == '\0') {
                in_body = 1;
            }
            continue;
        }

        trim_whitespace(g_scaffold_line);

        {
            int image_ref;
            image_ref = parse_image_reference(g_scaffold_line, g_scaffold_asset_name, sizeof(g_scaffold_asset_name));
            if (image_ref < 0) {
                fclose(fp);
                return 0;
            }
            if (image_ref) {
            if (!copy_asset_if_needed(g_scaffold_asset_name, dst_folder)) {
                fclose(fp);
                return 0;
            }
            }
        }
    }

    fclose(fp);
    return 1;
}

static void remove_asset_outputs_from_list(const char *list_path, const char *folder) {
    FILE *list_fp;

    list_fp = open_existing_file(list_path, "r");
    if (list_fp == NULL) {
        return;
    }

    while (fgets(g_scaffold_line, sizeof(g_scaffold_line), list_fp) != NULL) {
        trim_line_endings(g_scaffold_line);
        trim_whitespace(g_scaffold_line);

        if (g_scaffold_line[0] == '\0' || g_scaffold_line[0] == '#') {
            continue;
        }

        if (!is_valid_asset_entry(g_scaffold_line)) {
            continue;
        }

        join_path(g_scaffold_path, sizeof(g_scaffold_path), folder, g_scaffold_line);
        remove_if_exists(g_scaffold_path);
    }

    fclose(list_fp);
}

static void remove_archive_outputs_from_manifest(void) {
    FILE *fp;

    join_path(g_scaffold_path, sizeof(g_scaffold_path), OUTPUT_DIR, "BUILD.DAT");
    fp = open_existing_file(g_scaffold_path, "r");
    if (fp == NULL) {
        return;
    }

    while (fgets(g_scaffold_line, sizeof(g_scaffold_line), fp) != NULL) {
        trim_line_endings(g_scaffold_line);
        trim_whitespace(g_scaffold_line);

        if (g_scaffold_line[0] == 'A' && g_scaffold_line[1] == ' ') {
            copy_string(g_scaffold_value, sizeof(g_scaffold_value), g_scaffold_line + 2);
            trim_whitespace(g_scaffold_value);
            if (g_scaffold_value[0] != '\0') {
                remove_generated_file_in(PAGE_OUTPUT_DIR, g_scaffold_value);
            }
        }
    }

    fclose(fp);
}

/* Fallback only: prefer copying VGA.SRC, WIN98.SRC, and CRT.SRC from the DOS root. */
static int write_default_vga_theme_source(const char *path) {
    return write_text_file(
        path,
        "@font-face { font-family: \"VGA\"; src: url(\"FNT/VGA.TTF\") format(\"truetype\"); font-display: swap; }\n"
        "body { background: #111; color: #aaa; font: 18px/1.2 \"VGA\", \"Lucida Console\", Monaco, monospace; }\n"
        "#wrapper { width: 960px; margin: 0 auto; }\n"
        "a { color: #ffff55; text-decoration: none; }\n"
        ".dosborder { padding: 1rem; border: 4px double #aaa; margin: 0 0 20px; }\n"
        ".dosblue { background: #0000aa; color: #55ffff; border-color: #55ffff; }\n"
        ".dosblack { background: #111; color: #aaa; border-color: #aaa; }\n"
        ".doscyan { background: #14aeb2; color: #fff6a0; border-color: #000; }\n"
        ".clearfix { overflow: hidden; }\n"
        "#sidebar { float: left; width: 26%; }\n"
        "#content, #maincol { float: right; width: 71%; }\n"
        ".boat pre { color: #d8d24b; white-space: pre; overflow-x: auto; }\n"
        "@media(max-width:900px){.boat pre{font-size:12px;line-height:15px;}}\n"
        "@media(max-width:560px){.boat pre{font-size:10px;line-height:12px;}}\n"
        "#footer { clear: both; }\n"
    );
}

static int write_default_win98_theme_source(const char *path) {
    return write_text_file(
        path,
        "@font-face{font-family:\"W98\";src:url(\"FNT/W98.TTF\") format(\"truetype\");font-display:swap;}\n"
        "body{background:#008080;color:#000;font:13px/1.35 \"W98\",Tahoma,Arial,sans-serif;}\n"
        "#wrapper{width:min(980px,calc(100% - 32px));margin:18px auto;background:#c0c0c0;border:1px solid #fff;border-right-color:#404040;border-bottom-color:#404040;padding:8px;}\n"
        "a{color:#000080;text-decoration:underline;}\n"
        ".dosborder,.dosblue,.dosblack,.doscyan{background:#c0c0c0;color:#000;border:1px solid #fff;border-right-color:#404040;border-bottom-color:#404040;padding:8px;margin:5px 0 14px;}\n"
        "#masthead{display:flex;justify-content:space-between;align-items:center;gap:1rem;}\n"
        "#mastleft{display:flex;align-items:center;gap:14px;flex-wrap:wrap;}\n"
        "#themepicker{display:flex;gap:8px;flex-wrap:wrap;}\n"
        "@media(max-width:900px){#wrapper,.dosborder,.dosblue,.dosblack,.doscyan,.themebutton,.boat,#content pre,figure.postimage img{border-width:1px;}#wrapper{box-shadow:1px 1px 0 #000;}}\n"
    );
}

static int write_default_crt_theme_source(const char *path) {
    return write_text_file(
        path,
        "@font-face{font-family:\"CRT\";src:url(\"FNT/CRT.TTF\") format(\"truetype\");font-display:swap;}\n"
        "body{background:#020602;color:#66ff66;font:18px/1.25 \"CRT\",\"Lucida Console\",monospace;text-shadow:0 0 5px #33ff33;}\n"
        "#wrapper{width:min(980px,calc(100% - 32px));margin:0 auto;}\n"
        "a{color:#99ff99;text-decoration:none;}\n"
        ".dosborder,.dosblue,.dosblack,.doscyan{background:#020602;color:#66ff66;border:1px solid #33aa33;box-shadow:0 0 10px #063;padding:1rem;margin:5px 0 20px;}\n"
        "@media(max-width:900px){.boat pre{font-size:12px;line-height:15px;}}\n"
        "@media(max-width:560px){.boat pre{font-size:10px;line-height:12px;}}\n"
    );
}

/* Fallback only: prefer copying BANNER.TXT from the DOS root. */
static int write_default_banner(const char *path) {
    return write_text_file(
        path,
        "  ____________________\n"
        " / LOCAL ZINE        /|\n"
        "/ STATIC HTML ON DOS/ |\n"
        "|___________________|/\n"
    );
}

int init_site(const char *folder) {
    char path[MAX_LINE];
    char message[256];

    g_scaffold_error[0] = '\0';

    if (!ensure_dir(folder)) {
        sprintf(message, "Failed to create directory %s", folder);
        set_scaffold_error(message);
        return 0;
    }

    join_path(path, sizeof(path), folder, "posts");
    if (!ensure_dir(path)) {
        sprintf(message, "Failed to create directory %s", path);
        set_scaffold_error(message);
        return 0;
    }

    join_path(path, sizeof(path), folder, "pages");
    if (!ensure_dir(path)) {
        sprintf(message, "Failed to create directory %s", path);
        set_scaffold_error(message);
        return 0;
    }

    join_path(path, sizeof(path), folder, IMAGE_OUTPUT_DIR);
    if (!ensure_dir(path)) {
        sprintf(message, "Failed to create directory %s", path);
        set_scaffold_error(message);
        return 0;
    }

    join_path(path, sizeof(path), folder, "site.txt");
    if (!write_text_file(
            path,
            "title: My Local Zine\n"
            "theme: vga\n"
            "tagline: DOS-native static blogging.\n"
            "description: A handmade blog generated by Local Zine.\n"
            "about: This site was created with Local Zine.\n"
            "Edit site.txt, add posts, and rebuild.\n")) {
        return 0;
    }

    join_path(path, sizeof(path), folder, "posts.lst");
    if (!write_text_file(
            path,
            "welcome.txt\n"
            "firstlog.txt\n")) {
        return 0;
    }

    join_path(path, sizeof(path), folder, "pages.lst");
    if (!write_text_file(
            path,
            "links.txt\n")) {
        return 0;
    }

    join_path(path, sizeof(path), folder, "assets.lst");
    if (!write_text_file(
            path,
            "# List image filenames from the IMG directory.\n"
            "# Filenames must be uppercase DOS 8.3 names.\n"
            "# Example:\n"
            "# 260411A1.PNG\n")) {
        return 0;
    }

    join_path(path, sizeof(path), folder, "posts\\welcome.txt");
    if (!write_text_file(
            path,
            "title: Welcome\n"
            "slug: welcome\n"
            "date: 1996-12-02\n"
            "category: notes\n"
            "summary: Welcome to your new Local Zine site.\n"
            "\n"
            "Welcome to your Local Zine site.\n"
            "\n"
            "Edit this post, add more files under POSTS, and rebuild the site.\n")) {
        return 0;
    }

    join_path(path, sizeof(path), folder, "posts\\firstlog.txt");
    if (!write_text_file(
            path,
            "title: First Log Entry\n"
            "slug: firstlog\n"
            "date: 1996-12-01\n"
            "category: devlog\n"
            "summary: A second starter post for your site.\n"
            "tags: devlog, starter\n"
            "\n"
            "This is a second starter post.\n"
            "\n"
            "Keep filenames and slugs DOS-safe and 8.3 friendly.\n")) {
        return 0;
    }

    join_path(path, sizeof(path), folder, "pages\\links.txt");
    if (!write_text_file(
            path,
            "title: Links\n"
            "slug: links\n"
            "summary: Extra page links and notes.\n"
            "tags: pages, links\n"
            "\n"
            "Use pages for standalone content that should not appear in the blog archive.\n"
            "\n"
            "This makes Local Zine feel more like a static site generator and less like a single-purpose blog tool.\n")) {
        return 0;
    }

    join_path(path, sizeof(path), folder, "VGA.SRC");
    if (!copy_file("VGA.SRC", path) && !write_default_vga_theme_source(path)) {
        set_scaffold_error("Failed to create VGA.SRC");
        return 0;
    }

    join_path(path, sizeof(path), folder, "WIN98.SRC");
    if (!copy_file("WIN98.SRC", path) && !write_default_win98_theme_source(path)) {
        set_scaffold_error("Failed to create WIN98.SRC");
        return 0;
    }

    join_path(path, sizeof(path), folder, "CRT.SRC");
    if (!copy_file("CRT.SRC", path) && !write_default_crt_theme_source(path)) {
        set_scaffold_error("Failed to create CRT.SRC");
        return 0;
    }

    join_path(path, sizeof(path), folder, "BANNER.TXT");
    if (!copy_file("BANNER.TXT", path) && !write_default_banner(path)) {
        set_scaffold_error("Failed to create BANNER.TXT");
        return 0;
    }

    join_path(path, sizeof(path), folder, "font.ttf");
    copy_file("font.ttf", path);

    return 1;
}

int copy_site_assets(const Post *posts, int post_count, const Page *pages, int page_count) {
    int i;

    g_scaffold_copy_count = 0;

    join_path(g_asset_output_path, sizeof(g_asset_output_path), OUTPUT_DIR, IMAGE_OUTPUT_DIR);

    if (!copy_assets_from_list("assets.lst", IMAGE_OUTPUT_DIR, g_asset_output_path)) {
        return 0;
    }

    for (i = 0; i < post_count; ++i) {
        if (!copy_assets_from_source_file(posts[i].source_path, g_asset_output_path)) {
            return 0;
        }
    }

    for (i = 0; i < page_count; ++i) {
        if (!copy_assets_from_source_file(pages[i].source_path, g_asset_output_path)) {
            return 0;
        }
    }

    return 1;
}

int get_scaffold_copy_count(void) {
    return g_scaffold_copy_count;
}

void reset_scaffold_copy_count(void) {
    g_scaffold_copy_count = 0;
}

int create_post(const char *slug) {
    FILE *fp;
    char post_path[MAX_LINE];
    char normalized_slug[32];
    char date_text[32];

    g_scaffold_error[0] = '\0';

    if (!is_valid_dos_slug(slug)) {
        set_scaffold_error("Post slug must be DOS-safe and 8 chars or fewer");
        return 0;
    }

    normalize_slug(normalized_slug, sizeof(normalized_slug), slug);

    if (post_list_contains(normalized_slug)) {
        set_scaffold_error("Post already exists in posts.lst");
        return 0;
    }

    if (page_list_contains(normalized_slug)) {
        set_scaffold_error("Post slug conflicts with an existing page");
        return 0;
    }

    join_path(post_path, sizeof(post_path), "posts", normalized_slug);
    append_string(post_path, sizeof(post_path), ".txt");

    if (scaffold_file_exists(post_path)) {
        set_scaffold_error("Post file already exists");
        return 0;
    }

    fp = fopen(post_path, "w");
    if (fp == NULL) {
        set_scaffold_error("Failed to create post file");
        return 0;
    }

    get_current_date(date_text, sizeof(date_text));

    fprintf(fp,
        "title: %s\n"
        "slug: %s\n"
        "date: %s\n"
        "category: notes\n"
        "summary: Short summary for %s.\n"
        "tags: notes\n"
        "\n"
        "Write your new post here.\n"
        "\n"
        "Add more paragraphs as needed.\n",
        normalized_slug,
        normalized_slug
        ,
        date_text,
        normalized_slug
    );
    fclose(fp);

    if (!append_post_list_entry(normalized_slug)) {
        set_scaffold_error("Failed to update posts.lst");
        return 0;
    }

    return 1;
}

int create_page(const char *slug) {
    FILE *fp;
    char page_path[MAX_LINE];
    char normalized_slug[32];

    g_scaffold_error[0] = '\0';

    if (!is_valid_dos_slug(slug)) {
        set_scaffold_error("Page slug must be DOS-safe and 8 chars or fewer");
        return 0;
    }

    normalize_slug(normalized_slug, sizeof(normalized_slug), slug);

    if (page_list_contains(normalized_slug)) {
        set_scaffold_error("Page already exists in pages.lst");
        return 0;
    }

    if (post_list_contains(normalized_slug)) {
        set_scaffold_error("Page slug conflicts with an existing post");
        return 0;
    }

    join_path(page_path, sizeof(page_path), "pages", normalized_slug);
    append_string(page_path, sizeof(page_path), ".txt");

    if (scaffold_file_exists(page_path)) {
        set_scaffold_error("Page file already exists");
        return 0;
    }

    fp = fopen(page_path, "w");
    if (fp == NULL) {
        set_scaffold_error("Failed to create page file");
        return 0;
    }

    fprintf(fp,
        "title: %s\n"
        "slug: %s\n"
        "summary: Short summary for %s.\n"
        "tags: pages\n"
        "\n"
        "# %s\n"
        "\n"
        "Write your standalone page here.\n"
        "\n"
        "- First item\n"
        "- Second item\n"
        "\n"
        "=> https://example.com Example link\n",
        normalized_slug,
        normalized_slug,
        normalized_slug,
        normalized_slug
    );
    fclose(fp);

    if (!append_page_list_entry(normalized_slug)) {
        set_scaffold_error("Failed to update pages.lst");
        return 0;
    }

    return 1;
}

int delete_post(const char *slug) {
    char normalized_slug[32];
    char post_path[MAX_LINE];
    char output_name[16];
    char source_name[16];

    g_scaffold_error[0] = '\0';

    if (!is_valid_dos_slug(slug)) {
        set_scaffold_error("Post slug must be DOS-safe and 8 chars or fewer");
        return 0;
    }

    normalize_slug(normalized_slug, sizeof(normalized_slug), slug);
    if (!post_list_contains(normalized_slug)) {
        set_scaffold_error("Post not found in posts.lst");
        return 0;
    }

    join_path(post_path, sizeof(post_path), "posts", normalized_slug);
    append_string(post_path, sizeof(post_path), ".txt");

    output_name[0] = '\0';
    make_source_filename(source_name, sizeof(source_name), normalized_slug);
    if (!read_output_name_from_manifest('P', source_name, output_name, sizeof(output_name))) {
        read_output_name_from_source(post_path, output_name, sizeof(output_name));
    }

    if (!remove_list_entry("posts.lst", normalized_slug)) {
        set_scaffold_error("Failed to update posts.lst");
        return 0;
    }

    remove(post_path);
    if (output_name[0] != '\0') {
        remove_generated_file_in(POST_OUTPUT_DIR, output_name);
    }
    remove_generated_file("BUILD.DAT");
    return 1;
}

int delete_page(const char *slug) {
    char normalized_slug[32];
    char page_path[MAX_LINE];
    char output_name[16];
    char source_name[16];

    g_scaffold_error[0] = '\0';

    if (!is_valid_dos_slug(slug)) {
        set_scaffold_error("Page slug must be DOS-safe and 8 chars or fewer");
        return 0;
    }

    normalize_slug(normalized_slug, sizeof(normalized_slug), slug);
    if (!page_list_contains(normalized_slug)) {
        set_scaffold_error("Page not found in pages.lst");
        return 0;
    }

    join_path(page_path, sizeof(page_path), "pages", normalized_slug);
    append_string(page_path, sizeof(page_path), ".txt");

    output_name[0] = '\0';
    make_source_filename(source_name, sizeof(source_name), normalized_slug);
    if (!read_output_name_from_manifest('G', source_name, output_name, sizeof(output_name))) {
        read_output_name_from_source(page_path, output_name, sizeof(output_name));
    }

    if (!remove_list_entry("pages.lst", normalized_slug)) {
        set_scaffold_error("Failed to update pages.lst");
        return 0;
    }

    remove(page_path);
    if (output_name[0] != '\0') {
        remove_generated_file_in(PAGE_OUTPUT_DIR, output_name);
    }
    remove_generated_file("BUILD.DAT");
    return 1;
}

int clean_site(void) {
    int i;
    static char name[16];
    static char image_output_path[MAX_LINE];

    remove_generated_file("INDEX.HTM");
    remove_generated_file_in(PAGE_OUTPUT_DIR, "ARCHIVE.HTM");
    remove_generated_file_in(PAGE_OUTPUT_DIR, "ABOUT.HTM");
    remove_generated_file("THEME.CSS");
    remove_generated_file("VGA.CSS");
    remove_generated_file("WIN98.CSS");
    remove_generated_file("CRT.CSS");
    remove_generated_file_in(FONT_OUTPUT_DIR, "VGA.TTF");
    remove_generated_file_in(FONT_OUTPUT_DIR, "CRT.TTF");
    remove_generated_file_in(FONT_OUTPUT_DIR, "W98.TTF");

    remove_outputs_from_list("posts.lst", "posts", POST_OUTPUT_DIR);
    remove_outputs_from_list("pages.lst", "pages", PAGE_OUTPUT_DIR);
    join_path(image_output_path, sizeof(image_output_path), OUTPUT_DIR, IMAGE_OUTPUT_DIR);
    remove_asset_outputs_from_list("assets.lst", image_output_path);
    remove_archive_outputs_from_manifest();
    remove_generated_file("BUILD.DAT");

    for (i = 0; i < 32; ++i) {
        sprintf(name, "C%02d.HTM", i + 1);
        remove_generated_file_in(PAGE_OUTPUT_DIR, name);
        sprintf(name, "T%02d.HTM", i + 1);
        remove_generated_file_in(PAGE_OUTPUT_DIR, name);
    }

    return 1;
}

const char *get_scaffold_error(void) {
    return g_scaffold_error;
}
