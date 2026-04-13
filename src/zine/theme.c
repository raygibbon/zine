#include <stdio.h>
#include <string.h>

#include "theme.h"
#include "util.h"

static int g_theme_write_count;
static const char *g_default_vga_theme_source =
    "@font-face{font-family:\"VGA\";src:url(\"FNT/VGA.TTF\") format(\"truetype\");font-display:swap;}\n"
    "body{background:#111;color:#aaa;font:18px/1.15 \"VGA\",\"Lucida Console\",monospace;text-align:center;}\n"
    "#wrapper{width:min(980px,calc(100% - 32px));margin:0 auto;text-align:left;}\n"
    "a{color:#ffff55;text-decoration:none;}a:hover{background:#00aaaa;}\n"
    ".dosborder{padding:1rem;border:6px double #aaa;outline:5px solid #111;margin:5px 0 20px;}\n"
    ".dosblue{color:#55ffff;background:#0000aa;border-color:#55ffff;outline-color:#0000aa;}\n"
    ".dosblack{color:#aaa;background:#111;border-color:#aaa;outline-color:#111;}\n"
    ".doscyan{background:#14aeb2;color:#fff6a0;border-color:#000;outline-color:#14aeb2;}\n"
    ".boat pre{font-size:16px;line-height:20px;}\n"
    "@media(max-width:900px){.boat pre{font-size:12px;line-height:15px;}}\n"
    "@media(max-width:560px){.boat pre{font-size:10px;line-height:12px;}}\n";
static const char *g_default_win98_theme_source =
    "@font-face{font-family:\"W98\";src:url(\"FNT/W98.TTF\") format(\"truetype\");font-display:swap;}\n"
    "body{background:#008080;color:#000;font:13px/1.35 \"W98\",Tahoma,Arial,sans-serif;text-align:center;}\n"
    "#wrapper{width:min(980px,calc(100% - 32px));margin:18px auto;text-align:left;background:#c0c0c0;border:1px solid #fff;border-right-color:#404040;border-bottom-color:#404040;padding:8px;}\n"
    "a{color:#000080;text-decoration:underline;}a:hover{color:#800080;background:none;}\n"
    ".dosborder,.dosblue,.dosblack,.doscyan{background:#c0c0c0;color:#000;border:1px solid #fff;border-right-color:#404040;border-bottom-color:#404040;padding:8px;margin:5px 0 14px;}\n"
    "#masthead,.posttitlebox{background:#000080;color:#fff;padding:4px 6px;}\n"
    "#masthead{display:flex;justify-content:space-between;align-items:center;gap:1rem;}\n"
    "#mastleft{display:flex;align-items:center;gap:14px;flex-wrap:wrap;}\n"
    "#themepicker{display:flex;gap:8px;flex-wrap:wrap;}\n"
    ".themebutton{font:12px/1 \"W98\",Tahoma,Arial,sans-serif;background:#c0c0c0;color:#000;border:1px solid #fff;border-right-color:#404040;border-bottom-color:#404040;padding:3px 8px;}\n"
    ".themebutton.active{border-color:#404040;border-right-color:#fff;border-bottom-color:#fff;background:#d0d0d0;}\n"
    "@media(max-width:900px){#wrapper,.dosborder,.dosblue,.dosblack,.doscyan,.themebutton,.boat,#content pre,figure.postimage img{border-width:1px;}#wrapper{box-shadow:1px 1px 0 #000;}}\n";
static const char *g_default_crt_theme_source =
    "@font-face{font-family:\"CRT\";src:url(\"FNT/CRT.TTF\") format(\"truetype\");font-display:swap;}\n"
    "body{background:#020602;color:#66ff66;font:18px/1.25 \"CRT\",\"Lucida Console\",monospace;text-align:center;text-shadow:0 0 5px #33ff33;}\n"
    "#wrapper{width:min(980px,calc(100% - 32px));margin:0 auto;text-align:left;}\n"
    "a{color:#99ff99;text-decoration:none;}a:hover{background:#103810;}\n"
    ".dosborder,.dosblue,.dosblack,.doscyan{background:#020602;color:#66ff66;border:1px solid #33aa33;box-shadow:0 0 10px #063;padding:1rem;margin:5px 0 20px;}\n"
    "body:before{content:\"\";position:fixed;left:0;right:0;top:0;bottom:0;pointer-events:none;background:repeating-linear-gradient(0deg,rgba(0,0,0,0.08),rgba(0,0,0,0.08) 2px,transparent 2px,transparent 4px);}\n"
    ".boat pre{font-size:16px;line-height:20px;}\n"
    "@media(max-width:900px){.boat pre{font-size:12px;line-height:15px;}}\n"
    "@media(max-width:560px){.boat pre{font-size:10px;line-height:12px;}}\n";
static const char *g_default_theme_script =
    "(function(){function init(){var b=document.getElementsByClassName('themebutton'),l=document.getElementById('theme-style'),a=document.getElementsByTagName('a'),body=document.body,d=body?(body.getAttribute('data-default-theme')||'vga'):'vga',i,t,q;function norm(x){return x==='win98'||x==='crt'?x:'vga';}function hrefFor(x){return x==='win98'?'WIN98.CSS':(x==='crt'?'CRT.CSS':'VGA.CSS');}function urlFor(x,h){var p,s;if(!h)h=window.location.href||'';s='';p=h.indexOf('#');if(p>=0){s=h.substring(p);h=h.substring(0,p);}p=h.indexOf('?');if(p>=0)h=h.substring(0,p);return h+'?theme='+x+s;}function setCurrentUrl(x){if(!window.history||!window.history.replaceState)return;try{window.history.replaceState(null,'',urlFor(x));}catch(e){}}function apply(x){var h;x=norm(x);if(l){h=hrefFor(x);if((l.getAttribute('href')||'')!==h)l.href=h;}setCurrentUrl(x);for(i=0;i<b.length;++i)b[i].className=(b[i].getAttribute('data-theme')===x)?'themebutton active':'themebutton';for(i=0;i<a.length;++i){h=a[i].getAttribute('href');if(!h||h.indexOf('http://')===0||h.indexOf('https://')===0||h.indexOf('mailto:')===0||h.indexOf('#')===0)continue;a[i].setAttribute('href',urlFor(x,h));}}q=(window.location.search||'').match(/[?&]theme=([^&]+)/);t=norm(q&&q[1]?q[1]:(function(){try{return localStorage.getItem('lz-theme')||d;}catch(e){return d;}})());apply(t);try{localStorage.setItem('lz-theme',t);}catch(e){}for(i=0;i<b.length;++i)b[i].onclick=function(){var n=norm(this.getAttribute('data-theme'));apply(n);try{localStorage.setItem('lz-theme',n);}catch(e){}};}if(document.readyState==='loading'&&document.addEventListener){document.addEventListener('DOMContentLoaded',init);}else{init();}})();\n";
static char g_theme_source_path[64];

static const char *find_theme_source(const char *name, const char *fallback) {
    if (file_exists(name)) {
        return name;
    }

    join_path(g_theme_source_path, sizeof(g_theme_source_path), "..", name);
    if (file_exists(g_theme_source_path)) {
        return g_theme_source_path;
    }

    join_path(g_theme_source_path, sizeof(g_theme_source_path), "..\\OUT\\FNT", name);
    if (file_exists(g_theme_source_path)) {
        return g_theme_source_path;
    }

    if (fallback != NULL) {
        if (file_exists(fallback)) {
            return fallback;
        }

        join_path(g_theme_source_path, sizeof(g_theme_source_path), "..", fallback);
        if (file_exists(g_theme_source_path)) {
            return g_theme_source_path;
        }
    }

    return NULL;
}

static int file_matches_text(const char *path, const char *text) {
    FILE *fp;
    char buffer[1024];
    int expected;
    int offset;
    int count;

    fp = open_existing_file(path, "rb");
    if (fp == NULL) {
        return 0;
    }

    expected = (int)strlen(text);
    offset = 0;
    while ((count = (int)fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        if (offset + count > expected || memcmp(buffer, text + offset, count) != 0) {
            fclose(fp);
            return 0;
        }
        offset += count;
    }

    fclose(fp);
    return offset == expected;
}

static int write_text_if_needed(const char *path, const char *text, const char *label) {
    FILE *fp;
    int length;

    if (file_matches_text(path, text)) {
        return 1;
    }

    fp = fopen(path, "wb");
    if (fp == NULL) {
        return 0;
    }

    length = (int)strlen(text);
    if ((int)fwrite(text, 1, length, fp) != length) {
        fclose(fp);
        return 0;
    }

    fclose(fp);
    ++g_theme_write_count;
    if (get_verbose_mode()) {
        printf("Wrote %s\n", label);
    }
    return 1;
}

static int copy_named_theme_if_needed(const char *src_path, const char *dst_path, const char *label) {
    if (should_copy_file(src_path, dst_path)) {
        ++g_theme_write_count;
        if (get_verbose_mode()) {
            printf("Wrote %s\n", label);
        }
    }
    return copy_file_if_needed(src_path, dst_path);
}

int write_theme_css(const char *selected_theme) {
    const char *active_name;
    char vga_output[64];
    char win98_output[64];
    char crt_output[64];
    char active_output[64];

    join_path(vga_output, sizeof(vga_output), OUTPUT_DIR, "VGA.CSS");
    join_path(win98_output, sizeof(win98_output), OUTPUT_DIR, "WIN98.CSS");
    join_path(crt_output, sizeof(crt_output), OUTPUT_DIR, "CRT.CSS");
    join_path(active_output, sizeof(active_output), OUTPUT_DIR, "THEME.CSS");

    if (file_exists("VGA.SRC")) {
        if (!copy_named_theme_if_needed("VGA.SRC", vga_output, "VGA.CSS")) {
            return 0;
        }
    } else if (!write_text_if_needed(vga_output, g_default_vga_theme_source, "VGA.CSS")) {
        return 0;
    }

    if (file_exists("WIN98.SRC")) {
        if (!copy_named_theme_if_needed("WIN98.SRC", win98_output, "WIN98.CSS")) {
            return 0;
        }
    } else if (!write_text_if_needed(win98_output, g_default_win98_theme_source, "WIN98.CSS")) {
        return 0;
    }

    if (file_exists("CRT.SRC")) {
        if (!copy_named_theme_if_needed("CRT.SRC", crt_output, "CRT.CSS")) {
            return 0;
        }
    } else if (!write_text_if_needed(crt_output, g_default_crt_theme_source, "CRT.CSS")) {
        return 0;
    }

    if (strings_equal(selected_theme, "win98")) {
        active_name = win98_output;
    } else if (strings_equal(selected_theme, "crt")) {
        active_name = crt_output;
    } else {
        active_name = vga_output;
    }
    if (should_copy_file(active_name, active_output)) {
        ++g_theme_write_count;
        if (get_verbose_mode()) {
            printf("Wrote THEME.CSS\n");
        }
    }
    return copy_file_if_needed(active_name, active_output);
}

int write_theme_font(const char *path) {
    const char *src_path;

    src_path = find_theme_source("VGA.TTF", "font.ttf");
    if (src_path == NULL) {
        return 1;
    }

    if (should_copy_file(src_path, path)) {
        ++g_theme_write_count;
        if (get_verbose_mode()) {
            printf("Wrote VGA.TTF\n");
        }
    }
    return copy_file_if_needed(src_path, path);
}

int write_optional_theme_font(const char *name, const char *path) {
    const char *src_path;

    src_path = find_theme_source(name, NULL);
    if (src_path == NULL) {
        return 1;
    }

    if (should_copy_file(src_path, path)) {
        ++g_theme_write_count;
        if (get_verbose_mode()) {
            printf("Wrote %s\n", name);
        }
    }

    return copy_file_if_needed(src_path, path);
}

int write_theme_script(const char *path) {
    if (file_exists("THEME.JS")) {
        return copy_named_theme_if_needed("THEME.JS", path, "THEME.JS");
    }
    return write_text_if_needed(path, g_default_theme_script, "THEME.JS");
}

void reset_theme_write_count(void) {
    g_theme_write_count = 0;
}

int get_theme_write_count(void) {
    return g_theme_write_count;
}
