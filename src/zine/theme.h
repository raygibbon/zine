#ifndef THEME_H
#define THEME_H

int write_theme_css(const char *selected_theme);
int write_theme_font(const char *path);
int write_optional_theme_font(const char *name, const char *path);
int write_theme_script(const char *path);
int normalize_banner_file(const char *path);
int get_theme_write_count(void);
void reset_theme_write_count(void);

#endif
