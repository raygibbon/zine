#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>

#define OUTPUT_DIR "OUT"
#define POST_OUTPUT_DIR "PST"
#define PAGE_OUTPUT_DIR "PAG"
#define IMAGE_OUTPUT_DIR "IMG"
#define FONT_OUTPUT_DIR "FNT"

void copy_string(char *dest, int dest_size, const char *src);
void copy_range_string(char *dest, int dest_size, const char *start, const char *end);
void append_string(char *dest, int dest_size, const char *src);
int append_string_checked(char *dest, int dest_size, const char *src);
void join_path(char *dest, int dest_size, const char *left, const char *right);
int ensure_dir(const char *path);
int copy_file(const char *src_path, const char *dst_path);
int copy_file_if_needed(const char *src_path, const char *dst_path);
int should_copy_file(const char *src_path, const char *dst_path);
FILE *open_existing_file(const char *path, const char *mode);
int resolve_existing_path(char *dest, int dest_size, const char *path);
int file_exists(const char *path);
unsigned long get_file_timestamp(const char *path, int *ok);
int is_file_newer_than(const char *left_path, const char *right_path);
void uppercase_string(char *text);
void lowercase_string(char *text);
void trim_line_endings(char *text);
void trim_whitespace(char *text);
int strings_equal(const char *left, const char *right);
int strings_equal_ignore_case(const char *left, const char *right);
int starts_with_ignore_case(const char *text, const char *prefix);
void set_verbose_mode(int enabled);
int get_verbose_mode(void);
int is_valid_dos_slug(const char *slug);
int is_valid_dos83_name(const char *name);
int is_valid_dos_image_name(const char *name);
void normalize_slug(char *dest, int dest_size, const char *slug);
void make_output_name(char *dest, int dest_size, const char *slug);
void make_source_filename(char *dest, int dest_size, const char *slug);
void make_archive_output_name(char *dest, int dest_size, char prefix, const char *name, int attempt);
int is_valid_list_entry_name(const char *name);
void write_html_escaped(FILE *fp, const char *text);
void write_html_escaped_range(FILE *fp, const char *start, const char *end);

#endif
