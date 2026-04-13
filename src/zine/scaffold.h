#ifndef SCAFFOLD_H
#define SCAFFOLD_H

int init_site(const char *folder);
int create_post(const char *slug);
int create_page(const char *slug);
int delete_post(const char *slug);
int delete_page(const char *slug);
int copy_site_assets(const Post *posts, int post_count, const Page *pages, int page_count);
int get_scaffold_copy_count(void);
void reset_scaffold_copy_count(void);
int clean_site(void);
const char *get_scaffold_error(void);

#endif
