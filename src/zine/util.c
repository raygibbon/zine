#include <direct.h>
#include <io.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "util.h"

#define PATH_BUFFER_SIZE 260

static char g_path_candidate[PATH_BUFFER_SIZE];
static char g_path_segment[PATH_BUFFER_SIZE];
static char g_resolved_path[PATH_BUFFER_SIZE];
static char g_copy_buffer[256];
static struct stat g_file_stat;
static int g_verbose_mode;
static char g_archive_stem[4];
static char g_archive_hash[5];

void copy_string(char *dest, int dest_size, const char *src) {
    int i;

    if (dest_size <= 0) {
        return;
    }

    for (i = 0; i < dest_size - 1 && src[i] != '\0'; ++i) {
        dest[i] = src[i];
    }

    dest[i] = '\0';
}

void copy_range_string(char *dest, int dest_size, const char *start, const char *end) {
    int i;

    if (dest_size <= 0) {
        return;
    }

    i = 0;
    while (start < end && i < dest_size - 1) {
        dest[i++] = *start++;
    }
    dest[i] = '\0';
}

void append_string(char *dest, int dest_size, const char *src) {
    append_string_checked(dest, dest_size, src);
}

int append_string_checked(char *dest, int dest_size, const char *src) {
    int len;
    int src_len;

    len = (int)strlen(dest);
    if (len >= dest_size - 1) {
        return 0;
    }

    src_len = (int)strlen(src);
    if (len + src_len >= dest_size) {
        return 0;
    }

    copy_string(dest + len, dest_size - len, src);
    return 1;
}

void join_path(char *dest, int dest_size, const char *left, const char *right) {
    copy_string(dest, dest_size, left);

    if (dest[0] != '\0' && dest[(int)strlen(dest) - 1] != '\\') {
        append_string(dest, dest_size, "\\");
    }

    append_string(dest, dest_size, right);
}

int ensure_dir(const char *path) {
    if (mkdir(path) == 0) {
        return 1;
    }

    return access(path, 0) == 0;
}

void set_verbose_mode(int enabled) {
    g_verbose_mode = enabled != 0;
}

int get_verbose_mode(void) {
    return g_verbose_mode;
}

int file_exists(const char *path) {
    return resolve_existing_path(g_resolved_path, sizeof(g_resolved_path), path);
}

static int resolve_existing_path_variant(char *dest, int dest_size, const char *path, int make_upper) {
    const char *cursor;
    const char *segment_end;
    int first_segment;

    cursor = path;
    g_path_candidate[0] = '\0';
    first_segment = 1;

    while (*cursor != '\0') {
        segment_end = cursor;
        while (*segment_end != '\0' && *segment_end != '/' && *segment_end != '\\') {
            ++segment_end;
        }

        copy_range_string(g_path_segment, sizeof(g_path_segment), cursor, segment_end);
        if (g_path_segment[0] != '\0') {
            if (make_upper) {
                uppercase_string(g_path_segment);
            } else {
                lowercase_string(g_path_segment);
            }
        }

        if (!first_segment && !append_string_checked(g_path_candidate, sizeof(g_path_candidate), "\\")) {
            return 0;
        }
        if (!append_string_checked(g_path_candidate, sizeof(g_path_candidate), g_path_segment)) {
            return 0;
        }

        first_segment = 0;
        cursor = segment_end;
        if (*cursor == '/' || *cursor == '\\') {
            ++cursor;
        }
    }

    if (g_path_candidate[0] == '\0') {
        return 0;
    }

    if (access(g_path_candidate, 0) != 0) {
        return 0;
    }

    copy_string(dest, dest_size, g_path_candidate);
    return 1;
}

int resolve_existing_path(char *dest, int dest_size, const char *path) {
    copy_string(dest, dest_size, path);
    if (access(dest, 0) == 0) {
        return 1;
    }

    if (resolve_existing_path_variant(dest, dest_size, path, 1)) {
        return 1;
    }

    if (resolve_existing_path_variant(dest, dest_size, path, 0)) {
        return 1;
    }

    return 0;
}

FILE *open_existing_file(const char *path, const char *mode) {
    FILE *fp;

    fp = fopen(path, mode);
    if (fp != NULL) {
        return fp;
    }

    if (!resolve_existing_path(g_resolved_path, sizeof(g_resolved_path), path)) {
        return NULL;
    }

    return fopen(g_resolved_path, mode);
}

int copy_file(const char *src_path, const char *dst_path) {
    FILE *src;
    FILE *dst;
    int count;

    src = open_existing_file(src_path, "rb");
    if (src == NULL) {
        return 0;
    }

    dst = fopen(dst_path, "wb");
    if (dst == NULL) {
        fclose(src);
        return 0;
    }

    while ((count = (int)fread(g_copy_buffer, 1, sizeof(g_copy_buffer), src)) > 0) {
        fwrite(g_copy_buffer, 1, count, dst);
    }

    fclose(src);
    fclose(dst);
    return 1;
}

unsigned long get_file_timestamp(const char *path, int *ok) {
    if (!resolve_existing_path(g_resolved_path, sizeof(g_resolved_path), path)) {
        *ok = 0;
        return 0UL;
    }

    if (stat(g_resolved_path, &g_file_stat) != 0) {
        *ok = 0;
        return 0UL;
    }

    *ok = 1;
    return (unsigned long)g_file_stat.st_mtime;
}

int is_file_newer_than(const char *left_path, const char *right_path) {
    int left_ok;
    int right_ok;
    unsigned long left_stamp;
    unsigned long right_stamp;

    left_stamp = get_file_timestamp(left_path, &left_ok);
    right_stamp = get_file_timestamp(right_path, &right_ok);

    if (!left_ok) {
        return 0;
    }

    if (!right_ok) {
        return 1;
    }

    return left_stamp > right_stamp;
}

int copy_file_if_needed(const char *src_path, const char *dst_path) {
    if (!should_copy_file(src_path, dst_path)) {
        return 1;
    }

    return copy_file(src_path, dst_path);
}

static int files_have_same_contents(const char *left_path, const char *right_path) {
    FILE *left_fp;
    FILE *right_fp;
    int left_ch;
    int right_ch;

    left_fp = open_existing_file(left_path, "rb");
    if (left_fp == NULL) {
        return 0;
    }

    right_fp = open_existing_file(right_path, "rb");
    if (right_fp == NULL) {
        fclose(left_fp);
        return 0;
    }

    for (;;) {
        left_ch = fgetc(left_fp);
        right_ch = fgetc(right_fp);

        if (left_ch != right_ch) {
            fclose(left_fp);
            fclose(right_fp);
            return 0;
        }

        if (left_ch == EOF) {
            break;
        }
    }

    fclose(left_fp);
    fclose(right_fp);
    return 1;
}

int should_copy_file(const char *src_path, const char *dst_path) {
    int src_ok;
    int dst_ok;
    unsigned long src_stamp;
    unsigned long dst_stamp;

    src_stamp = get_file_timestamp(src_path, &src_ok);
    if (!src_ok) {
        return 0;
    }

    dst_stamp = get_file_timestamp(dst_path, &dst_ok);
    if (dst_ok && dst_stamp >= src_stamp && files_have_same_contents(src_path, dst_path)) {
        return 0;
    }

    return 1;
}

void uppercase_string(char *text) {
    while (*text != '\0') {
        *text = (char)toupper((unsigned char)*text);
        ++text;
    }
}

void lowercase_string(char *text) {
    while (*text != '\0') {
        *text = (char)tolower((unsigned char)*text);
        ++text;
    }
}

void trim_line_endings(char *text) {
    int len;

    len = (int)strlen(text);
    while (len > 0 && (text[len - 1] == '\n' || text[len - 1] == '\r')) {
        text[len - 1] = '\0';
        --len;
    }
}

void trim_whitespace(char *text) {
    int start;
    int end;
    int i;

    start = 0;
    while (text[start] != '\0' && isspace((unsigned char)text[start])) {
        ++start;
    }

    end = (int)strlen(text);
    while (end > start && isspace((unsigned char)text[end - 1])) {
        --end;
    }

    if (start > 0) {
        for (i = 0; start + i < end; ++i) {
            text[i] = text[start + i];
        }
        text[i] = '\0';
        end = i;
    } else {
        text[end] = '\0';
    }

    text[end] = '\0';
}

int strings_equal(const char *left, const char *right) {
    return strcmp(left, right) == 0;
}

int strings_equal_ignore_case(const char *left, const char *right) {
    while (*left != '\0' && *right != '\0') {
        if (toupper((unsigned char)*left) != toupper((unsigned char)*right)) {
            return 0;
        }
        ++left;
        ++right;
    }

    return *left == '\0' && *right == '\0';
}

int starts_with_ignore_case(const char *text, const char *prefix) {
    while (*prefix != '\0') {
        if (*text == '\0') {
            return 0;
        }

        if (toupper((unsigned char)*text) != toupper((unsigned char)*prefix)) {
            return 0;
        }

        ++text;
        ++prefix;
    }

    return 1;
}

int is_valid_dos_slug(const char *slug) {
    int len;
    int i;

    len = (int)strlen(slug);
    if (len == 0 || len > 8) {
        return 0;
    }

    for (i = 0; i < len; ++i) {
        if (!isalnum((unsigned char)slug[i]) && slug[i] != '_' && slug[i] != '-') {
            return 0;
        }
    }

    return 1;
}

int is_valid_dos83_name(const char *name) {
    const char *dot;
    int stem_len;
    int ext_len;
    int i;

    if (name[0] == '\0' || strchr(name, ' ') != NULL || strchr(name, '/') != NULL ||
            strchr(name, '\\') != NULL || strchr(name, ':') != NULL || strstr(name, "..") != NULL) {
        return 0;
    }

    dot = strrchr(name, '.');
    if (dot == NULL || dot == name || dot[1] == '\0') {
        return 0;
    }

    stem_len = (int)(dot - name);
    ext_len = (int)strlen(dot + 1);
    if (stem_len < 1 || stem_len > 8 || ext_len < 1 || ext_len > 3) {
        return 0;
    }

    for (i = 0; name[i] != '\0'; ++i) {
        if (name[i] == '.') {
            continue;
        }
        if (!isalnum((unsigned char)name[i]) && name[i] != '_') {
            return 0;
        }
        if (toupper((unsigned char)name[i]) != (unsigned char)name[i]) {
            return 0;
        }
    }

    return 1;
}

int is_valid_dos_image_name(const char *name) {
    int i;

    if (!is_valid_dos83_name(name)) {
        return 0;
    }

    for (i = 0; i < 6; ++i) {
        if (!isdigit((unsigned char)name[i])) {
            return 0;
        }
    }

    return isupper((unsigned char)name[6]) &&
        isdigit((unsigned char)name[7]) &&
        name[8] == '.';
}

void normalize_slug(char *dest, int dest_size, const char *slug) {
    copy_string(dest, dest_size, slug);
    lowercase_string(dest);
}

void make_output_name(char *dest, int dest_size, const char *slug) {
    normalize_slug(dest, dest_size, slug);
    uppercase_string(dest);
    append_string(dest, dest_size, ".HTM");
}

void make_source_filename(char *dest, int dest_size, const char *slug) {
    normalize_slug(dest, dest_size, slug);
    append_string(dest, dest_size, ".txt");
}

static unsigned long hash_archive_name(const char *name, int attempt) {
    unsigned long hash;

    hash = 5381UL + (unsigned long)attempt;
    while (*name != '\0') {
        hash = ((hash << 5) + hash) + (unsigned char)toupper((unsigned char)*name);
        ++name;
    }

    return hash;
}

static void to_base36_4(char *dest, unsigned long value) {
    static const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int i;

    for (i = 3; i >= 0; --i) {
        dest[i] = digits[value % 36UL];
        value /= 36UL;
    }
    dest[4] = '\0';
}

void make_archive_output_name(char *dest, int dest_size, char prefix, const char *name, int attempt) {
    const char *cursor;
    int stem_len;
    unsigned long hash;

    stem_len = 0;
    cursor = name;
    while (*cursor != '\0' && stem_len < 3) {
        if (isalnum((unsigned char)*cursor)) {
            g_archive_stem[stem_len++] = (char)toupper((unsigned char)*cursor);
        }
        ++cursor;
    }

    while (stem_len < 3) {
        g_archive_stem[stem_len++] = 'X';
    }
    g_archive_stem[3] = '\0';

    hash = hash_archive_name(name, attempt);
    to_base36_4(g_archive_hash, hash % 1679616UL);

    dest[0] = prefix;
    copy_string(dest + 1, dest_size - 1, g_archive_stem);
    copy_string(dest + 4, dest_size - 4, g_archive_hash);
    dest[8] = '\0';
    append_string(dest, dest_size, ".HTM");
}

int is_valid_list_entry_name(const char *name) {
    const char *dot;
    char slug[64];

    if (name[0] == '\0' || strchr(name, '/') != NULL || strchr(name, '\\') != NULL || strchr(name, ':') != NULL || strstr(name, "..") != NULL) {
        return 0;
    }

    dot = strrchr(name, '.');
    if (dot == NULL || !strings_equal_ignore_case(dot, ".txt")) {
        return 0;
    }

    copy_range_string(slug, sizeof(slug), name, dot);
    return is_valid_dos_slug(slug);
}

static void write_html_char(FILE *fp, char ch) {
    if (ch == '&') {
        fputs("&amp;", fp);
    } else if (ch == '<') {
        fputs("&lt;", fp);
    } else if (ch == '>') {
        fputs("&gt;", fp);
    } else if (ch == '"') {
        fputs("&quot;", fp);
    } else if (ch == '\n') {
        fputc(' ', fp);
    } else {
        fputc(ch, fp);
    }
}

void write_html_escaped(FILE *fp, const char *text) {
    while (*text != '\0') {
        write_html_char(fp, *text);
        ++text;
    }
}

void write_html_escaped_range(FILE *fp, const char *start, const char *end) {
    while (start < end) {
        write_html_char(fp, *start);
        ++start;
    }
}
