#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "site.h"
#include "util.h"

#define MAX_ARCHIVES 16
#define BUILD_STATE_FILE "BUILD.DAT"
#define SITE_LAYOUT_VERSION 8UL

static const char *g_theme_switcher_script =
    "(function(){function init(){var b=document.getElementsByClassName('themebutton'),l=document.getElementById('theme-style'),a=document.getElementsByTagName('a'),body=document.body,d=body?(body.getAttribute('data-default-theme')||'vga'):'vga',i,t,q;function norm(x){return x==='win98'||x==='crt'?x:'vga';}function hrefFor(x){return x==='win98'?'WIN98.CSS':(x==='crt'?'CRT.CSS':'VGA.CSS');}function urlFor(x,h){var p,s;if(!h)h=window.location.href||'';s='';p=h.indexOf('#');if(p>=0){s=h.substring(p);h=h.substring(0,p);}p=h.indexOf('?');if(p>=0)h=h.substring(0,p);return h+'?theme='+x+s;}function setCurrentUrl(x){if(!window.history||!window.history.replaceState)return;try{window.history.replaceState(null,'',urlFor(x));}catch(e){}}function apply(x){var h;x=norm(x);if(l){h=hrefFor(x);if((l.getAttribute('href')||'')!==h)l.href=h;}setCurrentUrl(x);for(i=0;i<b.length;++i)b[i].className=(b[i].getAttribute('data-theme')===x)?'themebutton active':'themebutton';for(i=0;i<a.length;++i){h=a[i].getAttribute('href');if(!h||h.indexOf('http://')===0||h.indexOf('https://')===0||h.indexOf('mailto:')===0||h.indexOf('#')===0)continue;a[i].setAttribute('href',urlFor(x,h));}}q=(window.location.search||'').match(/[?&]theme=([^&]+)/);t=norm(q&&q[1]?q[1]:(function(){try{return localStorage.getItem('lz-theme')||d;}catch(e){return d;}})());apply(t);try{localStorage.setItem('lz-theme',t);}catch(e){}for(i=0;i<b.length;++i)b[i].onclick=function(){var n=norm(this.getAttribute('data-theme'));apply(n);try{localStorage.setItem('lz-theme',n);}catch(e){}};}if(document.readyState==='loading'&&document.addEventListener){document.addEventListener('DOMContentLoaded',init);}else{init();}})();";

typedef struct {
    char name[64];
    char output_name[16];
} ArchiveRef;

typedef struct {
    char source_name[16];
    char output_name[16];
    unsigned long meta_hash;
    unsigned long source_stamp;
} BuildEntry;

static char g_site_error[256];
static char g_site_message[256];
static char g_body_buffer[MAX_BODY];
static char g_site_path[MAX_LINE];
static char g_site_line[MAX_LINE];
static char g_site_tag[64];
static char g_site_current_tag[64];
static char g_site_normalized[MAX_LINE];
static char g_site_title[96];
static ArchiveRef g_categories[MAX_ARCHIVES];
static ArchiveRef g_tags[MAX_ARCHIVES];
static int g_category_count;
static int g_tag_count;
static BuildEntry g_prev_posts[MAX_POSTS];
static BuildEntry g_prev_pages[MAX_PAGES];
static char g_prev_archives[MAX_ARCHIVES * 2][16];
static int g_prev_post_count;
static int g_prev_page_count;
static int g_prev_archive_count;
static unsigned long g_prev_site_hash;
static int g_manifest_loaded;
static int g_site_write_count;

static FILE *open_output_file(const char *name);
static FILE *open_output_file_in(const char *folder, const char *name);
static int output_file_exists(const char *name);
static int output_file_exists_in(const char *folder, const char *name);
static void write_post_href(FILE *fp, const char *name);
static void write_page_href(FILE *fp, const char *name);
static void write_banner_byte(FILE *fp, unsigned char ch);
static int write_banner_utf8_sequence(FILE *fp, int first, FILE *banner_fp);
static void write_banner(FILE *fp);
static const char *theme_css_filename(const char *theme);

static void set_site_error(const char *message) {
    copy_string(g_site_error, sizeof(g_site_error), message);
}

static unsigned long hash_text(unsigned long hash, const char *text) {
    while (*text != '\0') {
        hash = ((hash << 5) + hash) + (unsigned char)*text;
        ++text;
    }

    return hash;
}

static unsigned long hash_file_contents(unsigned long hash, const char *path) {
    FILE *fp;
    int ch;

    fp = open_existing_file(path, "rb");
    if (fp == NULL) {
        return hash;
    }

    while ((ch = fgetc(fp)) != EOF) {
        hash = ((hash << 5) + hash) + (unsigned char)ch;
    }

    fclose(fp);
    return hash;
}

static const char *theme_css_filename(const char *theme) {
    if (strings_equal(theme, "win98")) {
        return "WIN98.CSS";
    }
    if (strings_equal(theme, "crt")) {
        return "CRT.CSS";
    }
    return "VGA.CSS";
}

static unsigned long hash_ulong(unsigned long hash, unsigned long value) {
    hash = ((hash << 5) + hash) + (value & 0xFFUL);
    hash = ((hash << 5) + hash) + ((value >> 8) & 0xFFUL);
    hash = ((hash << 5) + hash) + ((value >> 16) & 0xFFUL);
    hash = ((hash << 5) + hash) + ((value >> 24) & 0xFFUL);
    return hash;
}

static unsigned long compute_site_hash(const SiteConfig *config) {
    unsigned long hash;

    hash = 5381UL;
    hash = hash_ulong(hash, SITE_LAYOUT_VERSION);
    hash = hash_text(hash, config->title);
    hash = hash_text(hash, config->banner);
    hash = hash_text(hash, config->theme);
    hash = hash_text(hash, config->tagline);
    hash = hash_text(hash, config->description);
    hash = hash_text(hash, config->about);
    hash = hash_file_contents(hash, "BANNER.TXT");
    return hash;
}

static unsigned long compute_post_meta_hash(const Post *post) {
    unsigned long hash;

    hash = 5381UL;
    hash = hash_text(hash, post->title);
    hash = hash_text(hash, post->slug);
    hash = hash_text(hash, post->date);
    hash = hash_text(hash, post->category);
    hash = hash_text(hash, post->summary);
    hash = hash_text(hash, post->tags);
    hash = hash_ulong(hash, (unsigned long)post->draft);
    hash = hash_text(hash, post->output_name);
    hash = hash_text(hash, post->source_name);
    return hash;
}

static unsigned long compute_page_meta_hash(const Page *page) {
    unsigned long hash;

    hash = 5381UL;
    hash = hash_text(hash, page->title);
    hash = hash_text(hash, page->slug);
    hash = hash_text(hash, page->summary);
    hash = hash_text(hash, page->tags);
    hash = hash_ulong(hash, (unsigned long)page->draft);
    hash = hash_text(hash, page->output_name);
    hash = hash_text(hash, page->source_name);
    return hash;
}

static int find_previous_entry(const BuildEntry *entries, int count, const char *source_name) {
    int i;

    for (i = 0; i < count; ++i) {
        if (strings_equal_ignore_case(entries[i].source_name, source_name)) {
            return i;
        }
    }

    return -1;
}

static int load_build_manifest(void) {
    FILE *fp;
    int post_index;
    int page_index;
    int archive_index;

    g_manifest_loaded = 0;
    g_prev_post_count = 0;
    g_prev_page_count = 0;
    g_prev_archive_count = 0;
    g_prev_site_hash = 0UL;

    join_path(g_site_path, sizeof(g_site_path), OUTPUT_DIR, BUILD_STATE_FILE);
    fp = open_existing_file(g_site_path, "r");
    if (fp == NULL) {
        return 0;
    }

    post_index = 0;
    page_index = 0;
    archive_index = 0;
    while (fgets(g_site_line, sizeof(g_site_line), fp) != NULL) {
        trim_line_endings(g_site_line);
        trim_whitespace(g_site_line);
        if (g_site_line[0] == '\0') {
            continue;
        }

        if (strncmp(g_site_line, "SITE ", 5) == 0) {
            g_prev_site_hash = strtoul(g_site_line + 5, NULL, 10);
            continue;
        }

        if (strncmp(g_site_line, "POSTS ", 6) == 0) {
            g_prev_post_count = atoi(g_site_line + 6);
            if (g_prev_post_count < 0 || g_prev_post_count > MAX_POSTS) {
                fclose(fp);
                return 0;
            }
            continue;
        }

        if (strncmp(g_site_line, "PAGES ", 6) == 0) {
            g_prev_page_count = atoi(g_site_line + 6);
            if (g_prev_page_count < 0 || g_prev_page_count > MAX_PAGES) {
                fclose(fp);
                return 0;
            }
            continue;
        }

        if (strncmp(g_site_line, "ARCHIVES ", 9) == 0) {
            g_prev_archive_count = atoi(g_site_line + 9);
            if (g_prev_archive_count < 0 || g_prev_archive_count > MAX_ARCHIVES * 2) {
                fclose(fp);
                return 0;
            }
            continue;
        }

        if (g_site_line[0] == 'P' && g_site_line[1] == ' ' && post_index < g_prev_post_count) {
            BuildEntry *entry;
            entry = &g_prev_posts[post_index];
            if (sscanf(g_site_line + 2, "%15s %15s %lu %lu",
                    entry->source_name,
                    entry->output_name,
                    &entry->meta_hash,
                    &entry->source_stamp) != 4) {
                fclose(fp);
                return 0;
            }
            ++post_index;
            continue;
        }

        if (g_site_line[0] == 'G' && g_site_line[1] == ' ' && page_index < g_prev_page_count) {
            BuildEntry *entry;
            entry = &g_prev_pages[page_index];
            if (sscanf(g_site_line + 2, "%15s %15s %lu %lu",
                    entry->source_name,
                    entry->output_name,
                    &entry->meta_hash,
                    &entry->source_stamp) != 4) {
                fclose(fp);
                return 0;
            }
            ++page_index;
            continue;
        }

        if (g_site_line[0] == 'A' && g_site_line[1] == ' ') {
            if (archive_index < g_prev_archive_count) {
                if (sscanf(g_site_line + 2, "%15s", g_prev_archives[archive_index]) != 1) {
                    fclose(fp);
                    return 0;
                }
                ++archive_index;
            }
            continue;
        }
    }

    fclose(fp);
    if (post_index != g_prev_post_count || page_index != g_prev_page_count || archive_index != g_prev_archive_count) {
        return 0;
    }

    g_manifest_loaded = 1;
    return 1;
}

static int save_build_manifest(const SiteConfig *config, const Post *posts, int post_count, const Page *pages, int page_count) {
    FILE *fp;
    int i;
    int ok;
    unsigned long source_stamp;

    fp = open_output_file(BUILD_STATE_FILE);
    if (fp == NULL) {
        return 0;
    }

    fprintf(fp, "SITE %lu\n", compute_site_hash(config));
    fprintf(fp, "POSTS %d\n", post_count);
    for (i = 0; i < post_count; ++i) {
        source_stamp = get_file_timestamp(posts[i].source_path, &ok);
        if (!ok) {
            fclose(fp);
            join_path(g_site_path, sizeof(g_site_path), OUTPUT_DIR, BUILD_STATE_FILE);
            remove(g_site_path);
            set_site_error("Failed to read post timestamp for manifest");
            return 0;
        }
        fprintf(fp, "P %s %s %lu %lu\n",
            posts[i].source_name,
            posts[i].output_name,
            compute_post_meta_hash(&posts[i]),
            source_stamp
        );
    }

    fprintf(fp, "PAGES %d\n", page_count);
    for (i = 0; i < page_count; ++i) {
        source_stamp = get_file_timestamp(pages[i].source_path, &ok);
        if (!ok) {
            fclose(fp);
            join_path(g_site_path, sizeof(g_site_path), OUTPUT_DIR, BUILD_STATE_FILE);
            remove(g_site_path);
            set_site_error("Failed to read page timestamp for manifest");
            return 0;
        }
        fprintf(fp, "G %s %s %lu %lu\n",
            pages[i].source_name,
            pages[i].output_name,
            compute_page_meta_hash(&pages[i]),
            source_stamp
        );
    }

    fprintf(fp, "ARCHIVES %d\n", g_category_count + g_tag_count);
    for (i = 0; i < g_category_count; ++i) {
        fprintf(fp, "A %s\n", g_categories[i].output_name);
    }
    for (i = 0; i < g_tag_count; ++i) {
        fprintf(fp, "A %s\n", g_tags[i].output_name);
    }

    fclose(fp);
    return 1;
}

static int metadata_changed_for_posts(const Post *posts, int post_count) {
    int i;

    if (!g_manifest_loaded || post_count != g_prev_post_count) {
        return 1;
    }

    for (i = 0; i < post_count; ++i) {
        int index;
        unsigned long hash;

        index = find_previous_entry(g_prev_posts, g_prev_post_count, posts[i].source_name);
        if (index < 0) {
            return 1;
        }

        hash = compute_post_meta_hash(&posts[i]);
        if (g_prev_posts[index].meta_hash != hash || !strings_equal_ignore_case(g_prev_posts[index].output_name, posts[i].output_name)) {
            return 1;
        }
    }

    return 0;
}

static int metadata_changed_for_pages(const Page *pages, int page_count) {
    int i;

    if (!g_manifest_loaded || page_count != g_prev_page_count) {
        return 1;
    }

    for (i = 0; i < page_count; ++i) {
        int index;
        unsigned long hash;

        index = find_previous_entry(g_prev_pages, g_prev_page_count, pages[i].source_name);
        if (index < 0) {
            return 1;
        }

        hash = compute_page_meta_hash(&pages[i]);
        if (g_prev_pages[index].meta_hash != hash || !strings_equal_ignore_case(g_prev_pages[index].output_name, pages[i].output_name)) {
            return 1;
        }
    }

    return 0;
}

static int source_changed_since_manifest(const BuildEntry *entries, int count, const char *source_name, const char *source_path) {
    int index;
    int ok;
    unsigned long current_stamp;

    if (!g_manifest_loaded) {
        return 1;
    }

    index = find_previous_entry(entries, count, source_name);
    if (index < 0) {
        return 1;
    }

    current_stamp = get_file_timestamp(source_path, &ok);
    if (!ok) {
        return 1;
    }

    return current_stamp != entries[index].source_stamp;
}

static int shared_outputs_missing(void) {
    int i;

    if (!output_file_exists("INDEX.HTM") ||
            !output_file_exists_in(PAGE_OUTPUT_DIR, "ARCHIVE.HTM") ||
            !output_file_exists_in(PAGE_OUTPUT_DIR, "ABOUT.HTM")) {
        return 1;
    }

    for (i = 0; i < g_category_count; ++i) {
        if (!output_file_exists_in(PAGE_OUTPUT_DIR, g_categories[i].output_name)) {
            return 1;
        }
    }

    for (i = 0; i < g_tag_count; ++i) {
        if (!output_file_exists_in(PAGE_OUTPUT_DIR, g_tags[i].output_name)) {
            return 1;
        }
    }

    return 0;
}

static int index_output_stale(void) {
    join_path(g_site_path, sizeof(g_site_path), OUTPUT_DIR, "INDEX.HTM");
    if (!file_exists(g_site_path)) {
        return 1;
    }

    if (should_copy_file("BANNER.TXT", g_site_path)) {
        return 1;
    }

    return 0;
}

static void remove_previous_outputs(void) {
    int i;

    if (!g_manifest_loaded) {
        return;
    }

    for (i = 0; i < g_prev_post_count; ++i) {
        remove(g_prev_posts[i].output_name);
        join_path(g_site_path, sizeof(g_site_path), OUTPUT_DIR, POST_OUTPUT_DIR);
        join_path(g_site_path, sizeof(g_site_path), g_site_path, g_prev_posts[i].output_name);
        remove(g_site_path);
    }

    for (i = 0; i < g_prev_page_count; ++i) {
        remove(g_prev_pages[i].output_name);
        join_path(g_site_path, sizeof(g_site_path), OUTPUT_DIR, PAGE_OUTPUT_DIR);
        join_path(g_site_path, sizeof(g_site_path), g_site_path, g_prev_pages[i].output_name);
        remove(g_site_path);
    }

    for (i = 0; i < g_prev_archive_count; ++i) {
        remove(g_prev_archives[i]);
        join_path(g_site_path, sizeof(g_site_path), OUTPUT_DIR, PAGE_OUTPUT_DIR);
        join_path(g_site_path, sizeof(g_site_path), g_site_path, g_prev_archives[i]);
        remove(g_site_path);
    }

    for (i = 0; i < MAX_ARCHIVES; ++i) {
        sprintf(g_site_title, "C%02d.HTM", i + 1);
        remove(g_site_title);
        join_path(g_site_path, sizeof(g_site_path), OUTPUT_DIR, PAGE_OUTPUT_DIR);
        join_path(g_site_path, sizeof(g_site_path), g_site_path, g_site_title);
        remove(g_site_path);
        sprintf(g_site_title, "T%02d.HTM", i + 1);
        remove(g_site_title);
        join_path(g_site_path, sizeof(g_site_path), OUTPUT_DIR, PAGE_OUTPUT_DIR);
        join_path(g_site_path, sizeof(g_site_path), g_site_path, g_site_title);
        remove(g_site_path);
    }

    remove("INDEX.HTM");
    remove("ARCHIVE.HTM");
    remove("ABOUT.HTM");
    join_path(g_site_path, sizeof(g_site_path), OUTPUT_DIR, "INDEX.HTM");
    remove(g_site_path);
    join_path(g_site_path, sizeof(g_site_path), OUTPUT_DIR, PAGE_OUTPUT_DIR);
    join_path(g_site_path, sizeof(g_site_path), g_site_path, "ARCHIVE.HTM");
    remove(g_site_path);
    join_path(g_site_path, sizeof(g_site_path), OUTPUT_DIR, PAGE_OUTPUT_DIR);
    join_path(g_site_path, sizeof(g_site_path), g_site_path, "ABOUT.HTM");
    remove(g_site_path);
}

static int output_file_exists(const char *name) {
    join_path(g_site_path, sizeof(g_site_path), OUTPUT_DIR, name);
    return file_exists(g_site_path);
}

static FILE *open_output_file(const char *name) {
    return open_output_file_in("", name);
}

static int output_file_exists_in(const char *folder, const char *name) {
    join_path(g_site_path, sizeof(g_site_path), OUTPUT_DIR, folder);
    join_path(g_site_path, sizeof(g_site_path), g_site_path, name);
    return file_exists(g_site_path);
}

static FILE *open_output_file_in(const char *folder, const char *name) {
    FILE *fp;
    int count_write;

    if (folder[0] == '\0') {
        join_path(g_site_path, sizeof(g_site_path), OUTPUT_DIR, name);
    } else {
        join_path(g_site_path, sizeof(g_site_path), OUTPUT_DIR, folder);
        join_path(g_site_path, sizeof(g_site_path), g_site_path, name);
    }
    fp = fopen(g_site_path, "w");
    if (fp == NULL) {
        sprintf(g_site_message, "Failed to open %s", g_site_path);
        set_site_error(g_site_message);
        return NULL;
    }

    count_write = !strings_equal_ignore_case(name, BUILD_STATE_FILE);
    if (count_write) {
        ++g_site_write_count;
        if (get_verbose_mode()) {
            printf("Wrote %s\n", name);
        }
    }
    return fp;
}

static void write_post_href(FILE *fp, const char *name) {
    fputs(POST_OUTPUT_DIR, fp);
    fputc('/', fp);
    fputs(name, fp);
}

static void write_page_href(FILE *fp, const char *name) {
    fputs(PAGE_OUTPUT_DIR, fp);
    fputc('/', fp);
    fputs(name, fp);
}

static void write_banner_byte(FILE *fp, unsigned char ch) {
    switch (ch) {
        case '&': fputs("&amp;", fp); return;
        case '<': fputs("&lt;", fp); return;
        case '>': fputs("&gt;", fp); return;
        case '"': fputs("&quot;", fp); return;
        case 9: fputs("    ", fp); return;
        case 0xB0: fputs("&#9617;", fp); return;
        case 0xB1: fputs("&#9618;", fp); return;
        case 0xB2: fputs("&#9619;", fp); return;
        case 0xB3: fputs("&#9474;", fp); return;
        case 0xB4: fputs("&#9508;", fp); return;
        case 0xB5: fputs("&#9575;", fp); return;
        case 0xB6: fputs("&#9576;", fp); return;
        case 0xB7: fputs("&#9572;", fp); return;
        case 0xB8: fputs("&#9558;", fp); return;
        case 0xB9: fputs("&#9571;", fp); return;
        case 0xBA: fputs("&#9553;", fp); return;
        case 0xBB: fputs("&#9559;", fp); return;
        case 0xBC: fputs("&#9565;", fp); return;
        case 0xBD: fputs("&#9564;", fp); return;
        case 0xBE: fputs("&#9563;", fp); return;
        case 0xBF: fputs("&#9488;", fp); return;
        case 0xC0: fputs("&#9492;", fp); return;
        case 0xC1: fputs("&#9524;", fp); return;
        case 0xC2: fputs("&#9516;", fp); return;
        case 0xC3: fputs("&#9500;", fp); return;
        case 0xC4: fputs("&#9472;", fp); return;
        case 0xC5: fputs("&#9532;", fp); return;
        case 0xC6: fputs("&#9566;", fp); return;
        case 0xC7: fputs("&#9567;", fp); return;
        case 0xC8: fputs("&#9562;", fp); return;
        case 0xC9: fputs("&#9556;", fp); return;
        case 0xCA: fputs("&#9577;", fp); return;
        case 0xCB: fputs("&#9574;", fp); return;
        case 0xCC: fputs("&#9568;", fp); return;
        case 0xCD: fputs("&#9552;", fp); return;
        case 0xCE: fputs("&#9580;", fp); return;
        case 0xCF: fputs("&#9578;", fp); return;
        case 0xD0: fputs("&#9579;", fp); return;
        case 0xD1: fputs("&#9557;", fp); return;
        case 0xD2: fputs("&#9554;", fp); return;
        case 0xD3: fputs("&#9569;", fp); return;
        case 0xD4: fputs("&#9570;", fp); return;
        case 0xD5: fputs("&#9550;", fp); return;
        case 0xD6: fputs("&#9551;", fp); return;
        case 0xD7: fputs("&#9573;", fp); return;
        case 0xD8: fputs("&#9576;", fp); return;
        case 0xD9: fputs("&#9496;", fp); return;
        case 0xDA: fputs("&#9484;", fp); return;
        case 0xDB: fputs("&#9608;", fp); return;
        case 0xDC: fputs("&#9604;", fp); return;
        case 0xDD: fputs("&#9612;", fp); return;
        case 0xDE: fputs("&#9616;", fp); return;
        case 0xDF: fputs("&#9600;", fp); return;
    }

    if (ch >= 32 && ch <= 126) {
        fputc((int)ch, fp);
    } else {
        fputc('?', fp);
    }
}

static int write_banner_utf8_sequence(FILE *fp, int first, FILE *banner_fp) {
    int b1;
    int b2;

    if (first != 0xE2) {
        return 0;
    }

    b1 = fgetc(banner_fp);
    if (b1 == EOF) {
        write_banner_byte(fp, (unsigned char)first);
        return 1;
    }

    b2 = fgetc(banner_fp);
    if (b2 == EOF) {
        write_banner_byte(fp, (unsigned char)first);
        write_banner_byte(fp, (unsigned char)b1);
        return 1;
    }

    if (b1 == 0x96 && b2 == 0x88) {
        write_banner_byte(fp, 0xDB);
        return 1;
    }
    if (b1 == 0x95 && b2 == 0x91) {
        write_banner_byte(fp, 0xBA);
        return 1;
    }
    if (b1 == 0x95 && b2 == 0x90) {
        write_banner_byte(fp, 0xCD);
        return 1;
    }
    if (b1 == 0x95 && b2 == 0x94) {
        write_banner_byte(fp, 0xC9);
        return 1;
    }
    if (b1 == 0x95 && b2 == 0x97) {
        write_banner_byte(fp, 0xBB);
        return 1;
    }
    if (b1 == 0x95 && b2 == 0x9A) {
        write_banner_byte(fp, 0xC8);
        return 1;
    }
    if (b1 == 0x95 && b2 == 0x9D) {
        write_banner_byte(fp, 0xBC);
        return 1;
    }

    write_banner_byte(fp, (unsigned char)first);
    write_banner_byte(fp, (unsigned char)b1);
    write_banner_byte(fp, (unsigned char)b2);
    return 1;
}

static void write_banner(FILE *fp) {
    FILE *banner_fp;
    int ch;

    banner_fp = open_existing_file("BANNER.TXT", "rb");
    if (banner_fp == NULL) {
        fputs("LOCAL ZINE\n", fp);
        return;
    }

    while ((ch = fgetc(banner_fp)) != EOF) {
        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            fputc('\n', fp);
            continue;
        }
        if ((unsigned char)ch == 0xE2) {
            write_banner_utf8_sequence(fp, ch, banner_fp);
            continue;
        }
        write_banner_byte(fp, (unsigned char)ch);
    }

    fclose(banner_fp);
}

static int next_tag_value(const char **cursor, char *dest, int dest_size) {
    const char *start;
    const char *end;

    while (**cursor == ' ' || **cursor == '\t' || **cursor == ',') {
        ++(*cursor);
    }

    if (**cursor == '\0') {
        return 0;
    }

    start = *cursor;
    end = start;
    while (*end != '\0' && *end != ',') {
        ++end;
    }

    copy_range_string(dest, dest_size, start, end);
    trim_whitespace(dest);
    *cursor = end;

    return dest[0] != '\0';
}

static int archive_matches(const ArchiveRef *ref, const char *name) {
    return strings_equal_ignore_case(ref->name, name);
}

static const char *lookup_category_output(const char *category) {
    int i;

    for (i = 0; i < g_category_count; ++i) {
        if (archive_matches(&g_categories[i], category)) {
            return g_categories[i].output_name;
        }
    }

    return "";
}

static const char *lookup_tag_output(const char *tag) {
    int i;

    for (i = 0; i < g_tag_count; ++i) {
        if (archive_matches(&g_tags[i], tag)) {
            return g_tags[i].output_name;
        }
    }

    return "";
}

static int post_has_tag(const Post *post, const char *tag) {
    const char *cursor;

    cursor = post->tags;
    while (next_tag_value(&cursor, g_site_current_tag, sizeof(g_site_current_tag))) {
        if (strings_equal_ignore_case(g_site_current_tag, tag)) {
            return 1;
        }
    }

    return 0;
}

static int count_posts_in_category(const Post *posts, int post_count, const char *category) {
    int i;
    int matches;

    matches = 0;
    for (i = 0; i < post_count; ++i) {
        if (strings_equal_ignore_case(posts[i].category, category)) {
            ++matches;
        }
    }

    return matches;
}

static int count_posts_with_tag(const Post *posts, int post_count, const char *tag) {
    int i;
    int matches;

    matches = 0;
    for (i = 0; i < post_count; ++i) {
        if (post_has_tag(&posts[i], tag)) {
            ++matches;
        }
    }

    return matches;
}

static int is_reserved_output_name(const char *name) {
    return strings_equal_ignore_case(name, "INDEX.HTM") ||
        strings_equal_ignore_case(name, "ARCHIVE.HTM") ||
        strings_equal_ignore_case(name, "ABOUT.HTM");
}

static int archive_output_conflicts(const char *output_name, const Post *posts, int post_count, const Page *pages, int page_count) {
    int i;

    if (is_reserved_output_name(output_name)) {
        return 1;
    }

    (void)posts;
    (void)post_count;

    for (i = 0; i < page_count; ++i) {
        if (strings_equal_ignore_case(output_name, pages[i].output_name)) {
            return 1;
        }
    }

    for (i = 0; i < g_category_count; ++i) {
        if (strings_equal_ignore_case(output_name, g_categories[i].output_name)) {
            return 1;
        }
    }

    for (i = 0; i < g_tag_count; ++i) {
        if (strings_equal_ignore_case(output_name, g_tags[i].output_name)) {
            return 1;
        }
    }

    return 0;
}

static int append_archive_ref(ArchiveRef *refs, int *count, const char *name, char prefix, const Post *posts, int post_count, const Page *pages, int page_count) {
    int attempt;

    if (*count >= MAX_ARCHIVES) {
        sprintf(g_site_message, "Too many %s; maximum is %d", prefix == 'C' ? "categories" : "tags", MAX_ARCHIVES);
        set_site_error(g_site_message);
        return 0;
    }

    copy_string(refs[*count].name, sizeof(refs[*count].name), name);
    for (attempt = 0; attempt < 64; ++attempt) {
        make_archive_output_name(refs[*count].output_name, sizeof(refs[*count].output_name), prefix, name, attempt);
        if (!archive_output_conflicts(refs[*count].output_name, posts, post_count, pages, page_count)) {
            ++(*count);
            return 1;
        }
    }

    sprintf(g_site_message, "Failed to allocate archive filename for %s", name);
    set_site_error(g_site_message);
    refs[*count].output_name[0] = '\0';
    refs[*count].name[0] = '\0';
    return 0;
}

static int collect_archives(const Post *posts, int post_count, const Page *pages, int page_count) {
    int i;

    g_category_count = 0;
    g_tag_count = 0;

    for (i = 0; i < post_count; ++i) {
        int j;
        int found;
        const char *cursor;

        found = 0;
        for (j = 0; j < g_category_count; ++j) {
            if (archive_matches(&g_categories[j], posts[i].category)) {
                found = 1;
                break;
            }
        }
        if (!found && !append_archive_ref(g_categories, &g_category_count, posts[i].category, 'C', posts, post_count, pages, page_count)) {
            return 0;
        }

        cursor = posts[i].tags;
        while (next_tag_value(&cursor, g_site_tag, sizeof(g_site_tag))) {
            found = 0;
            for (j = 0; j < g_tag_count; ++j) {
                if (archive_matches(&g_tags[j], g_site_tag)) {
                    found = 1;
                    break;
                }
            }
            if (!found && !append_archive_ref(g_tags, &g_tag_count, g_site_tag, 'T', posts, post_count, pages, page_count)) {
                return 0;
            }
        }
    }

    return 1;
}

static void write_common_head(FILE *fp, const char *page_title, const SiteConfig *config, int post_count, int nested_page) {
    static const char *nav_labels[] = { "Home", "Archive", "About" };
    static const char *nav_links[] = { "INDEX.HTM", "PAG/ARCHIVE.HTM", "PAG/ABOUT.HTM" };
    int nav_index;

    fprintf(fp,
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "  <meta charset=\"utf-8\">\n"
        "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
        "  <title>");
    write_html_escaped(fp, page_title);
    fprintf(fp, " - ");
    write_html_escaped(fp, config->title);
    fprintf(fp,
        "</title>\n"
    );
    if (nested_page) {
        fputs("  <base href=\"../\">\n", fp);
    }
    fprintf(fp,
        "  <script>(function(){var d='");
    write_html_escaped(fp, config->theme);
    fprintf(fp,
        "',q=(window.location.search||'').match(/[?&]theme=([^&]+)/),t;function n(x){return x==='win98'||x==='crt'?x:'vga';}function h(x){return x==='win98'?'WIN98.CSS':(x==='crt'?'CRT.CSS':'VGA.CSS');}try{t=q&&q[1]?q[1]:localStorage.getItem('lz-theme')||d;}catch(e){t=d;}document.write('<link id=\"theme-style\" rel=\"stylesheet\" href=\"'+h(n(t))+'\">');})();</script>\n"
        "  <noscript><link id=\"theme-style\" rel=\"stylesheet\" href=\"");
    fputs(theme_css_filename(config->theme), fp);
    fprintf(fp,
        "\"></noscript>\n"
        "</head>\n"
        "<body data-default-theme=\"");
    write_html_escaped(fp, config->theme);
    fprintf(fp,
        "\">\n"
        "  <div id=\"wrapper\">\n"
        "    <div id=\"masthead\">\n"
        "      <div id=\"mastleft\">\n"
        "      <h1 id=\"logotop\"><a href=\"INDEX.HTM\">");
    write_html_escaped(fp, config->title);
    fprintf(fp,
        "</a></h1>\n"
        "      <div id=\"themepicker\" aria-label=\"Theme switcher\">\n"
        "        <button type=\"button\" class=\"themebutton\" data-theme=\"vga\">VGA</button>\n"
        "        <button type=\"button\" class=\"themebutton\" data-theme=\"win98\">WIN98</button>\n"
        "        <button type=\"button\" class=\"themebutton\" data-theme=\"crt\">CRT</button>\n"
        "      </div>\n"
        "      </div>\n"
        "      <p id=\"powerline\">Static HTML from a DOS executable</p>\n"
        "    </div>\n"
        "    <div id=\"header\" class=\"dosborder doscyan\">\n"
        "      <div id=\"headerbar\">\n"
        "        <div id=\"topnav\">\n"
        "          <ul>\n");
    for (nav_index = 0; nav_index < 3; ++nav_index) {
        fprintf(fp, "            <li><a href=\"%s\"><span class=\"navlead\">%c</span>", nav_links[nav_index], nav_labels[nav_index][0]);
        write_html_escaped(fp, nav_labels[nav_index] + 1);
        fputs("</a></li>\n", fp);
    }
    fprintf(fp,
        "          </ul>\n"
        "        </div>\n"
        "        <p id=\"postcount\">No. of posts: <span class=\"countlead\">%d</span></p>\n"
        "      </div>\n"
        "    </div>\n"
        "    <div class=\"clearfix\">\n",
        post_count
    );
}

static void write_common_footer(FILE *fp) {
    fputs(
        "    </div>\n"
        "    <div id=\"footer\" class=\"dosborder doscyan\">\n"
        "      <p>Built with Local Zine. Static HTML from a DOS executable.</p>\n"
        "    </div>\n"
        "  </div>\n"
        "  <script>",
        fp
    );
    fputs(g_theme_switcher_script, fp);
    fputs(
        "</script>\n"
        "</body>\n"
        "</html>\n",
        fp
    );
}

static void write_banner_block(FILE *fp) {
    fputs(
        "        <div class=\"boat\">\n"
        "          <pre>",
        fp
    );
    write_banner(fp);
    fputs(
        "          </pre>\n"
        "        </div>\n",
        fp
    );
}

static void write_panel_page_start(FILE *fp, const char *title) {
    fputs(
        "      <div id=\"maincol\">\n"
        "      <div id=\"content\" class=\"articlecontent\">\n"
        "        <div class=\"post\">\n"
        "          <div class=\"posttitlebox dosborder doscyan\">\n"
        "            <h2>",
        fp
    );
    write_html_escaped(fp, title);
    fputs(
        "</h2>\n"
        "          </div>\n"
        "          <div class=\"articlepanel dosborder dosblack\">\n",
        fp
    );
}

static void write_panel_page_end(FILE *fp) {
    fputs(
        "          </div>\n"
        "        </div>\n"
        "      </div>\n"
        "      </div>\n",
        fp
    );
}

static void write_post_list_item(FILE *fp, const Post *post) {
    const char *category_output;

    fputs(
        "          <li class=\"postlist-item\">\n"
        "            <div class=\"postrow-main\">\n"
        "              <span class=\"postrow-mark\">&gt;</span>\n"
        "              <a class=\"postrow-title\" href=\"",
        fp
    );
    write_post_href(fp, post->output_name);
    fputs("\">", fp);
    write_html_escaped(fp, post->title);
    fputs("</a>\n              <span class=\"postmeta postrow-meta\">", fp);
    write_html_escaped(fp, post->date);
    fputs(" :: ", fp);

    category_output = lookup_category_output(post->category);
    if (category_output[0] != '\0') {
        fputs("<a href=\"", fp);
        write_page_href(fp, category_output);
        fputs("\">", fp);
        write_html_escaped(fp, post->category);
        fputs("</a>", fp);
    } else {
        write_html_escaped(fp, post->category);
    }

    fputs("</span>\n            </div>\n", fp);
    if (post->summary[0] != '\0') {
        fputs("            <p class=\"postrow-summary\">", fp);
        write_html_escaped(fp, post->summary);
        fputs("</p>\n", fp);
    }
    fputs("          </li>\n", fp);
}

static void write_tag_links(FILE *fp, const char *tags) {
    const char *cursor;
    int first;

    cursor = tags;
    first = 1;
    while (next_tag_value(&cursor, g_site_tag, sizeof(g_site_tag))) {
        const char *output_name;

        output_name = lookup_tag_output(g_site_tag);
        if (!first) {
            fputs(", ", fp);
        }

        if (output_name[0] != '\0') {
            fputs("<a href=\"", fp);
            write_page_href(fp, output_name);
            fputs("\">", fp);
            write_html_escaped(fp, g_site_tag);
            fputs("</a>", fp);
        } else {
            write_html_escaped(fp, g_site_tag);
        }
        first = 0;
    }
}

static void write_sidebar(FILE *fp, const SiteConfig *config, const Post *posts, int post_count, const Page *pages, int page_count) {
    int i;

    fputs(
        "      <div id=\"sidebar\">\n"
        "        <div class=\"widget dosborder dosblue\">\n"
        "          <h3>Site</h3>\n"
        "          <p>",
        fp
    );
    write_html_escaped(fp, config->tagline);
    fputs(
        "</p>\n"
        "        </div>\n"
        "        <div class=\"widget dosborder dosblue sidepanel\">\n"
        "          <h3>Browse</h3>\n"
        "          <ul>\n"
        "            <li><a href=\"INDEX.HTM\">Home</a></li>\n"
        "            <li><a href=\"PAG/ARCHIVE.HTM\">Archive</a></li>\n"
        "            <li><a href=\"PAG/ABOUT.HTM\">About</a></li>\n",
        fp
    );

    for (i = 0; i < page_count; ++i) {
        fputs("            <li><a href=\"", fp);
        write_page_href(fp, pages[i].output_name);
        fputs("\">", fp);
        write_html_escaped(fp, pages[i].title);
        fputs("</a></li>\n", fp);
    }

    fputs(
        "          </ul>\n"
        "        </div>\n"
        "        <div class=\"widget dosborder dosblue sidepanel\">\n"
        "          <h3>Latest entries</h3>\n"
        "          <ul>\n",
        fp
    );

    for (i = 0; i < post_count; ++i) {
        fputs("            <li><a href=\"", fp);
        write_post_href(fp, posts[i].output_name);
        fputs("\">", fp);
        write_html_escaped(fp, posts[i].title);
        fputs("</a></li>\n", fp);
    }

    fputs(
        "          </ul>\n"
        "        </div>\n"
        "      </div>\n",
        fp
    );
}

static void write_inline_markup(FILE *fp, const char *text) {
    const char *cursor;
    int in_emphasis;

    cursor = text;
    in_emphasis = 0;

    while (*cursor != '\0') {
        if (*cursor == '*') {
            fputs(in_emphasis ? "</em>" : "<em>", fp);
            in_emphasis = !in_emphasis;
            ++cursor;
            continue;
        }

        if (*cursor == '[') {
            const char *start;
            const char *mid;
            const char *end;

            start = cursor + 1;
            mid = strchr(start, ']');
            if (mid != NULL && mid[1] == '(') {
                end = strchr(mid + 2, ')');
                if (end != NULL) {
                    fputs("<a href=\"", fp);
                    write_html_escaped_range(fp, mid + 2, end);
                    fputs("\">", fp);
                    write_html_escaped_range(fp, start, mid);
                    fputs("</a>", fp);
                    cursor = end + 1;
                    continue;
                }
            }
        }

        write_html_escaped_range(fp, cursor, cursor + 1);
        ++cursor;
    }

    if (in_emphasis) {
        fputs("</em>", fp);
    }
}

static int parse_image_line(const char *line, char *alt_text, int alt_size, char *image_url, int url_size) {
    const char *alt_start;
    const char *alt_end;
    const char *url_start;
    const char *url_end;

    if (line[0] != '!' || line[1] != '[') {
        return 0;
    }

    alt_start = line + 2;
    alt_end = strchr(alt_start, ']');
    if (alt_end == NULL || alt_end[1] != '(') {
        return 0;
    }

    url_start = alt_end + 2;
    url_end = strchr(url_start, ')');
    if (url_end == NULL || url_end[1] != '\0') {
        return 0;
    }

    copy_range_string(alt_text, alt_size, alt_start, alt_end);
    copy_range_string(image_url, url_size, url_start, url_end);
    trim_whitespace(alt_text);
    trim_whitespace(image_url);
    return image_url[0] != '\0';
}

static void write_image_src(FILE *fp, const char *image_url) {
    if (starts_with_ignore_case(image_url, "IMG/") || starts_with_ignore_case(image_url, "IMG\\")) {
        copy_string(g_site_normalized, sizeof(g_site_normalized), image_url);
        uppercase_string(g_site_normalized);
        if (g_site_normalized[3] == '\\') {
            g_site_normalized[3] = '/';
        }
        write_html_escaped(fp, g_site_normalized);
        return;
    }

    write_html_escaped(fp, image_url);
}

static void close_open_markup(FILE *fp, int *in_list, int *in_quote) {
    if (*in_list) {
        fputs("          </ul>\n", fp);
        *in_list = 0;
    }
    if (*in_quote) {
        fputs("          </blockquote>\n", fp);
        *in_quote = 0;
    }
}

static void write_post_body(FILE *fp, const char *body) {
    static char line[MAX_LINE];
    static char link_url[MAX_LINE];
    static char link_text[MAX_LINE];
    static char image_url[MAX_LINE];
    static char image_alt[MAX_LINE];
    const char *cursor;
    int in_list;
    int in_code;
    int in_quote;

    cursor = body;
    in_list = 0;
    in_code = 0;
    in_quote = 0;

    while (*cursor != '\0') {
        const char *line_start;
        int len;
        int i;

        line_start = cursor;
        while (*cursor != '\0' && *cursor != '\n') {
            ++cursor;
        }

        copy_range_string(line, sizeof(line), line_start, cursor);
        trim_whitespace(line);
        len = (int)strlen(line);

        if (strings_equal(line, "```")) {
            close_open_markup(fp, &in_list, &in_quote);
            if (in_code) {
                fputs("          </code></pre>\n", fp);
                in_code = 0;
            } else {
                fputs("          <pre><code>\n", fp);
                in_code = 1;
            }
        } else if (in_code) {
            write_html_escaped(fp, line);
            fputc('\n', fp);
        } else if (len == 0) {
            close_open_markup(fp, &in_list, &in_quote);
        } else if (line[0] == '#' && line[1] == ' ') {
            close_open_markup(fp, &in_list, &in_quote);
            fputs("          <h3>", fp);
            write_inline_markup(fp, line + 2);
            fputs("</h3>\n", fp);
        } else if (line[0] == '#' && line[1] == '#' && line[2] == ' ') {
            close_open_markup(fp, &in_list, &in_quote);
            fputs("          <h4>", fp);
            write_inline_markup(fp, line + 3);
            fputs("</h4>\n", fp);
        } else if (line[0] == '>' && line[1] == ' ') {
            if (in_list) {
                fputs("          </ul>\n", fp);
                in_list = 0;
            }
            if (!in_quote) {
                fputs("          <blockquote>\n", fp);
                in_quote = 1;
            }
            fputs("            <p>", fp);
            write_inline_markup(fp, line + 2);
            fputs("</p>\n", fp);
        } else if (line[0] == '-' && line[1] == ' ') {
            if (in_quote) {
                fputs("          </blockquote>\n", fp);
                in_quote = 0;
            }
            if (!in_list) {
                fputs("          <ul>\n", fp);
                in_list = 1;
            }
            fputs("            <li>", fp);
            write_inline_markup(fp, line + 2);
            fputs("</li>\n", fp);
        } else if (line[0] == '=' && line[1] == '>') {
            close_open_markup(fp, &in_list, &in_quote);
            link_url[0] = '\0';
            link_text[0] = '\0';
            i = 2;
            while (line[i] == ' ') {
                ++i;
            }
            while (line[i] != '\0' && line[i] != ' ') {
                ++i;
            }
            copy_range_string(link_url, sizeof(link_url), line + 2, line + i);
            trim_whitespace(link_url);
            while (line[i] == ' ') {
                ++i;
            }
            if (line[i] != '\0') {
                copy_string(link_text, sizeof(link_text), line + i);
                trim_whitespace(link_text);
            }
            if (link_url[0] != '\0') {
                if (link_text[0] == '\0') {
                    copy_string(link_text, sizeof(link_text), link_url);
                }
                fputs("          <p><a href=\"", fp);
                write_html_escaped(fp, link_url);
                fputs("\">", fp);
                write_inline_markup(fp, link_text);
                fputs("</a></p>\n", fp);
            }
        } else if (parse_image_line(line, image_alt, sizeof(image_alt), image_url, sizeof(image_url))) {
            close_open_markup(fp, &in_list, &in_quote);
            fputs("          <figure class=\"postimage\">", fp);
            fputs("<img src=\"", fp);
            write_image_src(fp, image_url);
            fputs("\" alt=\"", fp);
            write_html_escaped(fp, image_alt);
            fputs("\">", fp);
            if (image_alt[0] != '\0') {
                fputs("<figcaption>", fp);
                write_inline_markup(fp, image_alt);
                fputs("</figcaption>", fp);
            }
            fputs("</figure>\n", fp);
        } else {
            close_open_markup(fp, &in_list, &in_quote);
            fputs("          <p>", fp);
            write_inline_markup(fp, line);
            fputs("</p>\n", fp);
        }

        if (*cursor == '\n') {
            ++cursor;
        }
    }

    close_open_markup(fp, &in_list, &in_quote);
    if (in_code) {
        fputs("          </code></pre>\n", fp);
    }
}

static void write_archive_summary(FILE *fp, const Post *posts, int post_count) {
    int i;

    if (g_category_count > 0) {
        fputs("            <div class=\"postmeta postmeta-panel\">Categories</div>\n", fp);
        fputs("            <p class=\"tagline\">", fp);
        for (i = 0; i < g_category_count; ++i) {
            if (i > 0) {
                fputs(" | ", fp);
            }
            fputs("<a href=\"", fp);
            write_page_href(fp, g_categories[i].output_name);
            fputs("\">", fp);
            write_html_escaped(fp, g_categories[i].name);
            fprintf(fp, "</a> (%d)", count_posts_in_category(posts, post_count, g_categories[i].name));
        }
        fputs("</p>\n", fp);
    }

    if (g_tag_count > 0) {
        fputs("            <div class=\"postmeta postmeta-panel\">Tags</div>\n", fp);
        fputs("            <p class=\"tagline\">", fp);
        for (i = 0; i < g_tag_count; ++i) {
            if (i > 0) {
                fputs(" | ", fp);
            }
            fputs("<a href=\"", fp);
            write_page_href(fp, g_tags[i].output_name);
            fputs("\">", fp);
            write_html_escaped(fp, g_tags[i].name);
            fprintf(fp, "</a> (%d)", count_posts_with_tag(posts, post_count, g_tags[i].name));
        }
        fputs("</p>\n", fp);
    }
}

static void write_post_meta(FILE *fp, const Post *post) {
    const char *category_output;

    fputs("            <div class=\"postmeta postmeta-panel\">", fp);
    write_html_escaped(fp, post->date);
    fputs(" :: ", fp);
    category_output = lookup_category_output(post->category);
    if (category_output[0] != '\0') {
        fputs("<a href=\"", fp);
        write_page_href(fp, category_output);
        fputs("\">", fp);
        write_html_escaped(fp, post->category);
        fputs("</a>", fp);
    } else {
        write_html_escaped(fp, post->category);
    }
    fputs("</div>\n", fp);

    if (post->summary[0] != '\0') {
        fputs("            <p class=\"summaryline\"><strong>Summary:</strong> ", fp);
        write_html_escaped(fp, post->summary);
        fputs("</p>\n", fp);
    }
    if (post->tags[0] != '\0') {
        fputs("            <p class=\"tagline\"><strong>Tags:</strong> ", fp);
        write_tag_links(fp, post->tags);
        fputs("</p>\n", fp);
    }
}

static void write_post_nav(FILE *fp, const Post *posts, int post_count, int index) {
    fputs("            <hr>\n", fp);
    fputs("            <p class=\"navline\">", fp);
    if (index > 0) {
        fputs("<strong>Newer:</strong> <a href=\"", fp);
        write_post_href(fp, posts[index - 1].output_name);
        fputs("\">", fp);
        write_html_escaped(fp, posts[index - 1].title);
        fputs("</a>", fp);
    }
    if (index > 0 && index + 1 < post_count) {
        fputs(" | ", fp);
    }
    if (index + 1 < post_count) {
        fputs("<strong>Older:</strong> <a href=\"", fp);
        write_post_href(fp, posts[index + 1].output_name);
        fputs("\">", fp);
        write_html_escaped(fp, posts[index + 1].title);
        fputs("</a>", fp);
    }
    fputs(" | <strong>Archive:</strong> <a href=\"PAG/ARCHIVE.HTM\">Browse all posts</a></p>\n", fp);
}

static int render_standard_page(FILE *fp, const SiteConfig *config, const char *page_title, const Post *posts, int post_count, const Page *pages, int page_count, int nested_page) {
    write_common_head(fp, page_title, config, post_count, nested_page);
    write_sidebar(fp, config, posts, post_count, pages, page_count);
    return 1;
}

static int write_index_page(const SiteConfig *config, const Post *posts, int post_count, const Page *pages, int page_count) {
    FILE *fp;
    int i;

    fp = open_output_file("INDEX.HTM");
    if (fp == NULL) {
        return 0;
    }

    render_standard_page(fp, config, "Home", posts, post_count, pages, page_count, 0);
    fputs("      <div id=\"maincol\">\n", fp);
    write_banner_block(fp);
    fputs(
        "      <div id=\"content\" class=\"dosborder dosblack\">\n"
        "        <p class=\"leadcopy\">",
        fp
    );
    write_html_escaped(fp, config->description);
    fputs(
        "</p>\n"
        "        <hr>\n"
        "        <h2>Last posts added</h2>\n"
        "        <ul class=\"postlist\">\n",
        fp
    );
    for (i = 0; i < post_count; ++i) {
        write_post_list_item(fp, &posts[i]);
    }
    fputs(
        "        </ul>\n"
        "      </div>\n"
        "      </div>\n",
        fp
    );
    write_common_footer(fp);
    fclose(fp);
    return 1;
}

static int write_archive_page(const SiteConfig *config, const Post *posts, int post_count, const Page *pages, int page_count) {
    FILE *fp;
    int i;

    fp = open_output_file_in(PAGE_OUTPUT_DIR, "ARCHIVE.HTM");
    if (fp == NULL) {
        return 0;
    }

    render_standard_page(fp, config, "Archive", posts, post_count, pages, page_count, 1);
    write_panel_page_start(fp, "Archive");
    write_archive_summary(fp, posts, post_count);
    fputs("            <ul class=\"postlist\">\n", fp);
    for (i = 0; i < post_count; ++i) {
        write_post_list_item(fp, &posts[i]);
    }
    fputs("            </ul>\n", fp);
    write_panel_page_end(fp);
    write_common_footer(fp);
    fclose(fp);
    return 1;
}

static int write_about_page(const SiteConfig *config, const Post *posts, int post_count, const Page *pages, int page_count) {
    FILE *fp;

    fp = open_output_file_in(PAGE_OUTPUT_DIR, "ABOUT.HTM");
    if (fp == NULL) {
        return 0;
    }

    render_standard_page(fp, config, "About", posts, post_count, pages, page_count, 1);
    write_panel_page_start(fp, "About");
    fputs("            <div class=\"postmeta postmeta-panel\">Site notes</div>\n", fp);
    write_post_body(fp, config->about);
    write_panel_page_end(fp);
    write_common_footer(fp);
    fclose(fp);
    return 1;
}

static int write_post_page(const SiteConfig *config, const Post *post, const Post *posts, int post_count, const Page *pages, int page_count) {
    FILE *fp;
    int index;

    fp = open_output_file_in(POST_OUTPUT_DIR, post->output_name);
    if (fp == NULL) {
        return 0;
    }

    if (!load_post_body(post, g_body_buffer, sizeof(g_body_buffer))) {
        set_site_error(get_content_error());
        fclose(fp);
        return 0;
    }

    render_standard_page(fp, config, post->title, posts, post_count, pages, page_count, 1);
    write_panel_page_start(fp, post->title);
    write_post_meta(fp, post);
    write_post_body(fp, g_body_buffer);
    index = (int)(post - posts);
    write_post_nav(fp, posts, post_count, index);
    write_panel_page_end(fp);
    write_common_footer(fp);
    fclose(fp);
    return 1;
}

static int write_page_file(const SiteConfig *config, const Page *page, const Post *posts, int post_count, const Page *pages, int page_count) {
    FILE *fp;

    fp = open_output_file_in(PAGE_OUTPUT_DIR, page->output_name);
    if (fp == NULL) {
        return 0;
    }

    if (!load_page_body(page, g_body_buffer, sizeof(g_body_buffer))) {
        set_site_error(get_content_error());
        fclose(fp);
        return 0;
    }

    render_standard_page(fp, config, page->title, posts, post_count, pages, page_count, 1);
    write_panel_page_start(fp, page->title);
    if (page->summary[0] != '\0') {
        fputs("            <p class=\"summaryline\"><strong>Summary:</strong> ", fp);
        write_html_escaped(fp, page->summary);
        fputs("</p>\n", fp);
    }
    if (page->tags[0] != '\0') {
        fputs("            <p class=\"tagline\"><strong>Tags:</strong> ", fp);
        write_tag_links(fp, page->tags);
        fputs("</p>\n", fp);
    }
    write_post_body(fp, g_body_buffer);
    fputs("            <hr>\n", fp);
    fputs("            <p class=\"navline\"><strong>Archive:</strong> <a href=\"PAG/ARCHIVE.HTM\">Browse all posts</a></p>\n", fp);
    write_panel_page_end(fp);
    write_common_footer(fp);
    fclose(fp);
    return 1;
}

static int write_category_pages(const SiteConfig *config, const Post *posts, int post_count, const Page *pages, int page_count) {
    int i;

    for (i = 0; i < g_category_count; ++i) {
        FILE *fp;
        int j;

        fp = open_output_file_in(PAGE_OUTPUT_DIR, g_categories[i].output_name);
        if (fp == NULL) {
            return 0;
        }

        copy_string(g_site_title, sizeof(g_site_title), "Category: ");
        if (!append_string_checked(g_site_title, sizeof(g_site_title), g_categories[i].name)) {
            set_site_error("Category title too long");
            fclose(fp);
            return 0;
        }

        render_standard_page(fp, config, g_site_title, posts, post_count, pages, page_count, 1);
        write_panel_page_start(fp, g_site_title);
        fputs("            <p class=\"navline\"><strong>Back:</strong> <a href=\"PAG/ARCHIVE.HTM\">Archive</a></p>\n", fp);
        fputs("            <ul class=\"postlist\">\n", fp);
        for (j = 0; j < post_count; ++j) {
            if (strings_equal_ignore_case(posts[j].category, g_categories[i].name)) {
                write_post_list_item(fp, &posts[j]);
            }
        }
        fputs("            </ul>\n", fp);
        write_panel_page_end(fp);
        write_common_footer(fp);
        fclose(fp);
    }

    return 1;
}

static int write_tag_pages(const SiteConfig *config, const Post *posts, int post_count, const Page *pages, int page_count) {
    int i;

    for (i = 0; i < g_tag_count; ++i) {
        FILE *fp;
        int j;

        fp = open_output_file_in(PAGE_OUTPUT_DIR, g_tags[i].output_name);
        if (fp == NULL) {
            return 0;
        }

        copy_string(g_site_title, sizeof(g_site_title), "Tag: ");
        if (!append_string_checked(g_site_title, sizeof(g_site_title), g_tags[i].name)) {
            set_site_error("Tag title too long");
            fclose(fp);
            return 0;
        }

        render_standard_page(fp, config, g_site_title, posts, post_count, pages, page_count, 1);
        write_panel_page_start(fp, g_site_title);
        fputs("            <p class=\"navline\"><strong>Back:</strong> <a href=\"PAG/ARCHIVE.HTM\">Archive</a></p>\n", fp);
        fputs("            <ul class=\"postlist\">\n", fp);
        for (j = 0; j < post_count; ++j) {
            if (post_has_tag(&posts[j], g_tags[i].name)) {
                write_post_list_item(fp, &posts[j]);
            }
        }
        fputs("            </ul>\n", fp);
        write_panel_page_end(fp);
        write_common_footer(fp);
        fclose(fp);
    }

    return 1;
}

int generate_site(const SiteConfig *config, const Post *posts, int post_count, const Page *pages, int page_count) {
    int i;
    int rebuild_all;
    int build_shared;
    int build_index;

    g_site_error[0] = '\0';
    g_body_buffer[0] = '\0';
    g_site_write_count = 0;
    load_build_manifest();

    if (!collect_archives(posts, post_count, pages, page_count)) {
        return 0;
    }

    rebuild_all = !g_manifest_loaded ||
        g_prev_site_hash != compute_site_hash(config) ||
        metadata_changed_for_posts(posts, post_count) ||
        metadata_changed_for_pages(pages, page_count);

    build_shared = rebuild_all || shared_outputs_missing();
    build_index = build_shared || index_output_stale();

    if (rebuild_all) {
        remove_previous_outputs();
    }

    if (build_index) {
        if (!write_index_page(config, posts, post_count, pages, page_count)) {
            return 0;
        }
    }
    if (build_shared) {
        if (!write_archive_page(config, posts, post_count, pages, page_count)) {
            return 0;
        }
        if (!write_about_page(config, posts, post_count, pages, page_count)) {
            return 0;
        }
    }

    for (i = 0; i < post_count; ++i) {
        if (rebuild_all || !output_file_exists_in(POST_OUTPUT_DIR, posts[i].output_name) ||
                source_changed_since_manifest(g_prev_posts, g_prev_post_count, posts[i].source_name, posts[i].source_path)) {
            if (!write_post_page(config, &posts[i], posts, post_count, pages, page_count)) {
                return 0;
            }
        }
    }

    for (i = 0; i < page_count; ++i) {
        if (rebuild_all || !output_file_exists_in(PAGE_OUTPUT_DIR, pages[i].output_name) ||
                source_changed_since_manifest(g_prev_pages, g_prev_page_count, pages[i].source_name, pages[i].source_path)) {
            if (!write_page_file(config, &pages[i], posts, post_count, pages, page_count)) {
                return 0;
            }
        }
    }

    if (build_shared) {
        if (!write_category_pages(config, posts, post_count, pages, page_count)) {
            return 0;
        }
        if (!write_tag_pages(config, posts, post_count, pages, page_count)) {
            return 0;
        }
    }

    if (!save_build_manifest(config, posts, post_count, pages, page_count)) {
        return 0;
    }

    return 1;
}

int get_site_write_count(void) {
    return g_site_write_count;
}

void reset_site_write_count(void) {
    g_site_write_count = 0;
}

const char *get_site_error(void) {
    if (g_site_error[0] == '\0') {
        return "Failed to generate site pages";
    }

    return g_site_error;
}
