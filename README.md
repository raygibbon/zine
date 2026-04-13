# Zine

Local Zine is a 16-bit C static site generator for MS-DOS 6.22. It builds multi-page blog and page-based sites from plain text source files and runs as a real DOS executable under DOSBox-X or on vintage hardware.

## Overview

- builds a DOS `.EXE` with OpenWatcom
- reads posts from `posts.lst` and `posts/*.txt`
- reads standalone pages from `pages.lst` and `pages/*.txt`
- copies optional images from `IMG/` into `OUT/IMG/`
- writes generated HTML, CSS, and font files into `OUT/`
- keeps generated filenames DOS-safe with uppercase `.HTM`

The codebase stays intentionally small and dependency-free. Content is plain text. Output is generated directly from C.

The loader tolerates DOS-style case differences when opening existing files, so `SITE.TXT` and `site.txt` both work on a case-sensitive host. Slugs and list entries are normalized and validated more strictly than before to avoid DOS/host mismatches.

## Commands

```text
zine.exe
```

Build the current site into `OUT/`.

Builds are incremental. Unchanged outputs, theme/font files, and copied assets are skipped when possible.

```text
zine.exe -v
```

Build the current site in verbose mode and print each rewritten output.

```text
zine.exe build MYBLOG
```

Build the site in `MYBLOG` without changing into that folder first.

```text
zine.exe build MYBLOG -v
```

Build the site in `MYBLOG` with verbose output.

```text
zine.exe init MYBLOG
```

Create a starter site in `MYBLOG`.

```text
zine.exe new SLUG
```

Create a new post in the current site.

```text
zine.exe newpage SLUG
```

Create a new standalone page in the current site.

```text
zine.exe clean
```

Remove generated site files.

```text
zine.exe serve
zine.exe serve 8080
zine.exe serve MYBLOG
zine.exe serve MYBLOG 8080
zine.exe serve C:\MYBLOG\OUT 8080
```

Build the site, then start the local preview server for `OUT/`. When a site directory is passed, Local Zine enters that directory first, so `zine.exe serve MYBLOG` builds and serves `MYBLOG/OUT`. Passing a path ending in `OUT` serves that output directory directly. This launches the native `zhttp.exe` preview server.

## Preview Server Requirements

The preview server is for local testing. It uses `zhttp.exe`, which links Watt-32 for TCP/IP. Keep `zhttp.exe` next to `zine.exe` in `mnt/`, or one directory above the site folder when using `zine.exe serve MYBLOG`.

The `mnt/` directory is the intended DOS mount root. In DOSBox-X, mount it as `C:` and run Local Zine from there:

```text
MOUNT C /path/to/local-zine/mnt
C:
```

The `mnt/MTCP/` tools may stay in the mount root for DOS networking setup and debugging, but `zine.exe serve` launches `zhttp.exe`, not mTCP HttpServ.

For DOSBox-X, the usual setup is:

```text
SET WATTCP.CFG=C:\
NE2000 0x60 3 0x300
ZINE.EXE SERVE MYBLOG 8080
```

`WATTCP.CFG` must contain the packet interrupt and network settings for your DOSBox-X network. The checked-in VS Code DOSBox-X serve tasks run these setup lines before `zine.exe serve`:

```text
SET WATTCP.CFG=C:\
NE2000 0x60 3 0x300
```

If DOSBox-X is using the `slirp` backend, forward the preview port in your DOSBox-X config so the host browser can connect:

```ini
[ethernet, slirp]
tcp_port_forwards = 8080
```

Then open the preview from the host browser:

```text
http://localhost:8080
```

On real DOS, the preview server may work with a compatible packet driver, but DOSBox-X is the primary supported preview environment.

## ZHTTP Preview Server

`zhttp.exe` is the companion preview server used by `zine.exe serve`. It stays separate from `zine.exe` so Watt-32 networking is isolated from the generator:

```text
zine.exe   site generator and serve command launcher
zhttp.exe  static-file HTTP preview server
```

Watt-32 is only linked into `zhttp.exe`, not the generator.

Source layout:

```text
src/
  zine/
    main.c
    content.c
    preview.c
    scaffold.c
    site.c
    theme.c
    util.c
  shared/
    zpath.c
    zfile.c
    zlog.c
  zhttp/
    main.c
    http_parse.c
    http_reply.c
    serve_file.c
    mime.c
    net_watt32.c
```

`zhttp.exe` is intentionally small: static files only, HTTP/1.0-style responses, GET and HEAD, one connection at a time, no TLS, no CGI, no chunked encoding, no keep-alive, no threads, and no range requests.

Command shape:

```text
ZHTTP.EXE
ZHTTP.EXE OUT
ZHTTP.EXE C:\MYBLOG\OUT 8080
```

The `net_watt32.c` module is the only module that includes Watt-32 headers or calls Watt-32 functions. The local build expects Watt-32 under `$HOME/opt/watt32` by default and links `$HOME/opt/watt32/src/lib/wattcpwl.lib`. Override `WATT_ROOT` if your Watt-32 tree lives somewhere else. See `ZHTTP_BUILD.TXT` for the separate build notes.

## Typical Workflow

```text
ZINE.EXE INIT TEST
ZINE.EXE BUILD TEST
CD TEST
..\ZINE.EXE NEW POST
..\ZINE.EXE NEWPAGE LINKS
..\ZINE.EXE
```

Then open:

```text
OUT\INDEX.HTM
```

In VS Code, `Build Zine` compiles `mnt/zine.exe`. To rebuild the example site under `MYBLOG/`, use the `Build MYBLOG` task or run `zine.exe build MYBLOG`.

## Toolchain

- C
- OpenWatcom 2.x
- DOSBox-X
- VS Code tasks for build and run

Current build command:

```sh
scripts/build-zine.sh
```

Host builds place intermediate object files under `build/`, copy the DOS
executables into `mnt/`, and refresh the default theme/banner/font seed files
from `src/zine/templates/`.

## Site Layout

After `zine.exe init MYBLOG`, a site looks like this:

```text
MYBLOG/
├── site.txt
├── posts.lst
├── pages.lst
├── assets.lst
├── posts/
│   ├── welcome.txt
│   └── firstlog.txt
├── pages/
│   └── links.txt
├── IMG/
└── OUT/
    ├── INDEX.HTM
    ├── PST/
    ├── PAG/
    ├── IMG/
    └── FNT/
```

Source files stay in the site root. Generated files go into `OUT/`.

## Theme And Banner

- `VGA.SRC` is the editable classic IBM PC / VGA text-mode theme source.
- `WIN98.SRC` is the editable Windows 95/98 style UI theme source.
- `CRT.SRC` is the editable monochrome green-screen CRT theme source.
- `BANNER.TXT` is user-owned banner art. Edit it directly to control the banner shown on the site.
- `BANNER.TXT` is the main editable DOS banner source and should be saved as CP437 if you use box-drawing or block characters.
- `src/zine/templates/` contains the checked-in seed copies used by the host build.
- `scripts/build-zine.sh` copies those seeds into `mnt/` so `zine.exe init MYBLOG` can copy them into the new site.
- `src/zine/scaffold.c` only contains fallback copies used if those DOS-root files are missing during `init`.

Local Zine reads `BANNER.TXT` and renders it into the site as-is. The build does not regenerate, normalize, or reformat banner art. It does not generate banner text from `site.txt`.

Set `theme: vga`, `theme: win98`, or `theme: crt` in `site.txt` to choose the default theme for first load. The generated pages also include a client-side theme switcher that remembers the last chosen theme in browser local storage.

Themes must reference fonts through `FNT/`: VGA uses `FNT/VGA.TTF`, WIN98 uses `FNT/W98.TTF`, and CRT uses `FNT/CRT.TTF`.

If you want to restyle the site, edit `VGA.SRC`, `WIN98.SRC`, `CRT.SRC`, and `BANNER.TXT`, not the embedded fallback strings in `src/zine/scaffold.c`.

During build, `BANNER.TXT` is read as DOS text and banner drawing characters are converted safely for the generated UTF-8 HTML output.

## Content Format

### Posts

Posts live in `posts/*.txt` and are listed in `posts.lst`.

Example:

```text
title: Welcome
slug: welcome
date: 1996-12-02
category: notes
summary: Welcome to your new Local Zine site.
tags: dos, starter
draft: no

# Welcome

This is the first paragraph.

- First item
- Second item

Here is an [inline link](https://example.com).

=> https://example.com Example link

![Screenshot](IMG/260411A1.PNG)

```
code sample
```
```

### Pages

Pages live in `pages/*.txt` and are listed in `pages.lst`.

Example:

```text
title: Links
slug: links
summary: Extra links and notes.
tags: pages, links
draft: no

# Links

This is a standalone page.
```

### Images

Images are optional and are copied during build from `IMG/` into `OUT/IMG/`.
Image references like `![Alt](IMG/260411A1.PNG)` are copied automatically.
`assets.lst` is optional and only needed for extra images that are not referenced directly from content.

Image filenames must be uppercase DOS 8.3 names using the `YYMMDD` + letter + digit stem, for example `260411A1.PNG`. Local Zine does not rename, hash, resize, or process images.

Example:

```text
# assets.lst
260411A1.PNG
260411A2.JPG
```

## Supported Fields

### Posts

- `title`
- `slug`
- `date`
- `category`
- `summary`
- `tags`
- `draft`

Required for posts:

- `title`
- `slug`
- `date`
- body text

### Pages

- `title`
- `slug`
- `summary`
- `tags`
- `draft`

Required for pages:

- `title`
- `slug`
- body text

## Lightweight Markup

Local Zine does not implement full Markdown. It uses a small line-based markup format:

- `# Heading`
- `## Subheading`
- `- list item`
- `=> URL text`
- `> quote`
- `*emphasis*`
- `[label](https://example.com)` inline links
- `![Alt text](IMG/260411A1.PNG)` block images
- `` ``` `` fenced code blocks
- blank lines separate paragraphs

This keeps the parser small enough for the DOS target.

## DOS-Safe Rules

- slugs must be DOS-safe and 8 characters or fewer
- source filenames in `posts.lst` and `pages.lst` must be DOS-safe `.txt` names
- a post or page slug must match its source filename
- published post slugs must be unique
- published page slugs must be unique
- generated output uses short directories: `OUT`, `PST`, `PAG`, `IMG`, and `FNT`
- post pages are written as `OUT/PST/P0001.HTM`
- page and archive pages are written under `OUT/PAG/`
- images are copied flat into `OUT/IMG/`
- fonts are copied flat into `OUT/FNT/`
- generated files use uppercase 8.3 names
- category and tag archive filenames are stable DOS-safe names derived from their labels

Draft entries are still parsed and validated, but they are filtered out of generated output, counts, archives, and navigation.

## Loader And Memory Notes

Local Zine now loads post/page metadata first and only reads a body when rendering that specific page. That keeps the persistent memory footprint lower for the 16-bit DOS build while preserving the CLI and the generated HTML style.

## Incremental Builds

Local Zine now keeps a small manifest in `OUT/BUILD.DAT` and uses file timestamps plus cached metadata hashes to avoid rewriting every output on every build.

What gets skipped:

- unchanged post pages when only other post bodies changed
- unchanged standalone pages when only other page bodies changed
- theme/font copies when the destination is already current
- asset copies when the destination is already current

What still forces a full site refresh:

- changes to `site.txt`
- changes to post/page metadata such as title, slug, date, category, summary, or tags
- changes to `posts.lst` or `pages.lst`
- missing shared outputs like `INDEX.HTM` or archive pages under `PAG/`

This keeps correctness for sidebars, archives, tags, categories, and navigation while making large body-only edits much cheaper on DOS.

The build status line now reports actual rewritten outputs. A no-op rebuild should report `0 changes`.

Category and tag archive pages now use stable derived filenames instead of positional names like `C01.HTM` and `T01.HTM`. That reduces output churn and makes archive URLs more stable as the site grows.

## Current Features

- starter site scaffolding
- post creation
- page creation
- `build DIR` command
- cleanup command
- posts and standalone pages
- draft support
- category and tag archive pages
