#ifndef SITE_H
#define SITE_H

#include "content.h"

int generate_site(const SiteConfig *config, const Post *posts, int post_count, const Page *pages, int page_count);
int get_site_write_count(void);
void reset_site_write_count(void);
const char *get_site_error(void);

#endif
