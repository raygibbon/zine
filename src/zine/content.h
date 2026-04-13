#ifndef CONTENT_H
#define CONTENT_H

#define MAX_POSTS 128
#define MAX_PAGES 32
#define MAX_TEXT 512
#define MAX_BODY 2048
#define MAX_LINE 256

typedef struct {
    char title[64];
    char banner[64];
    char theme[16];
    char tagline[128];
    char description[256];
    char about[MAX_TEXT];
} SiteConfig;

typedef struct {
    char title[64];
    char slug[16];
    char date[16];
    char category[32];
    char summary[96];
    char tags[64];
    int draft;
    int has_body;
    long body_offset;
    char output_name[16];
    char source_name[16];
    char source_path[64];
} Post;

typedef struct {
    char title[64];
    char slug[16];
    char summary[96];
    char tags[64];
    int draft;
    int has_body;
    long body_offset;
    char output_name[16];
    char source_name[16];
    char source_path[64];
} Page;

int load_site_config(const char *path, SiteConfig *config);
int load_posts(const char *list_path, Post *posts, int max_posts);
int load_pages(const char *list_path, Page *pages, int max_pages);
int validate_post_page_output_names(const Post *posts, int post_count, const Page *pages, int page_count);
int load_post_body(const Post *post, char *body, int body_size);
int load_page_body(const Page *page, char *body, int body_size);
const char *get_content_error(void);

#endif
