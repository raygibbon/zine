%{

/*
 *  lang_scanner.l - Foreign language lexical scanner for dos ftp
 *
 *  Gisle Vanem 1997
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>

#define YY_NEVER_INTERACTIVE 1

#include "ftp.h"
#include "crc.h"

#ifdef TEST
  #define INCLINE()   fprintf (stderr, "<line %d>\n", lang.line++)
  #define DEBUG(s, a) fprintf (stderr, s, a)
#else
  #define INCLINE()   lang.line++
  #define DEBUG(s, a) ((void)0)
#endif

#define BUF_SIZE    500

static void AddChar    (int ch);
static void AddDecimal (char *buf);
static void AddHexcode (char *buf);
static void AddOctal   (char *buf);
static void AddEntry   (void);
static void Abort      (char *fmt, ...)
#ifdef __GNUC__
__attribute__((format(printf,1,2)))
#endif
;

static char  buf [BUF_SIZE];
static char *ptr       = NULL;
static char *end       = buf+BUF_SIZE;
static int   line_cont = 0;
static DWORD crc_ref   = 0;
static DWORD do_ref    = 1;

struct Node {
       DWORD        crc;
       char        *string;
       struct Node *next;
     };

struct Elem {
       DWORD  crc;
       char  *string;
     };

static struct Language {
       struct Node *list;
       struct Elem *array;
       char        *file;          /* "watlang.txt"    */
       char         prefix[4];     /* "en:"            */
       DWORD        entries;
       DWORD        line;
     } lang = {
       NULL, NULL, "watlang.txt",
       "uk", 0L, 0L
     };

/*---------------------------------------------------------------------*/

%}

%option prefix = "ftp_lang"

%x STRING EO_LINE PREFIX

NL                   \r?\n
WS                   [\t ]+
HEX                  (x[0-9a-fA-F]{1,2})
OCTAL                (0[0-7]{1,2})
DECIMAL              ([0-9]{1,3})
LANG                 "de"|"no"|"sv"

/*
 *  Possible LANG codes are according to ISO 3166
 *
 *  Ref. ftp://ftp.informatik.uni-erlangen.de/pub/doc/ISO/
 *
 *  aa Afar          ba Bashkir           ca Catalan
 *  ab Abkhazian     be Byelorussian      co Corsican
 *  af Afrikaans     bg Bulgarian         cs Czech
 *  am Amharic       bh Bihari            cy Welsh
 *  ar Arabic        bi Bislama
 *  as Assamese      bn Bengali; Bangla   da Danish
 *  ay Aymara        bo Tibetan           de German
 *  az Azerbaijani   br Breton            dz Bhutani
 *
 *  el Greek         fa Persian           ga Irish
 *  en English       fi Finnish           gd Scots Gaelic
 *  eo Esperanto     fj Fiji              gl Galician
 *  es Spanish       fo Faroese           gn Guarani
 *  et Estonian      fr French            gu Gujarati
 *  eu Basque        fy Frisian
 *
 *  ha Hausa         ia Interlingua       ja Japanese
 *  he Hebrew        id Indonesian        jw Javanese
 *  hi Hindi         ie Interlingue
 *  hr Croatian      ik Inupiak
 *  hu Hungarian     is Icelandic
 *  hy Armenian      it Italian
 *                   iu Inuktitut
 *  ka Georgian                           mg Malagasy
 *  kk Kazakh        la Latin             mi Maori
 *  kl Greenlandic   ln Lingala           mk Macedonian
 *  km Cambodian     lo Laothian          ml Malayalam
 *  kn Kannada       lt Lithuanian        mn Mongolian
 *  ko Korean        lv Latvian, Lettish  mo Moldavian
 *  ks Kashmiri                           mr Marathi
 *  ku Kurdish                            ms Malay
 *  ky Kirghiz                            mt Maltese
 *                                        my Burmese
 *  na Nauru         oc Occitan
 *  ne Nepali        om (Afan) Oromo      pa Punjabi
 *  nl Dutch         or Oriya             pl Polish
 *  no Norwegian                          ps Pashto, Pushto
 *                                        pt Portuguese
 *  qu Quechua
 *
 *  rm Rhaeto-Romance  sa Sanskrit        ta Tamil
 *  rn Kirundi         sd Sindhi          te Telugu
 *  ro Romanian        sg Sangro          tg Tajik
 *  ru Russian         sh Serbo-Croatian  th Thai
 *  rw Kinyarwanda     si Sinhalese       ti Tigrinya
 *                     sk Slovak          tk Turkmen
 *  ug Uighur          sl Slovenian       tl Tagalog
 *  uk Ukrainian       sm Samoan          tn Setswana
 *  ur Urdu            sn Shona           to Tonga
 *  uz Uzbek           so Somali          tr Turkish
 *                     sq Albanian        ts Tsonga
 *  vi Vietnamese      sr Serbian         tt Tatar
 *  vo Volapuk         ss Siswati         tw Twi
 *                     st Sesotho
 *  wo Wolof           su Sundanese
 *                     sv Swedish
 *  xh Xhosa           sw Swahili
 *
 *  yi Yiddish (formerly ji)
 *  yo Yoruba
 *
 *  za Zhuang
 *  zh Chinese
 *  zu Zulu
 */

%%

<INITIAL>{                               /* get reference string */
        {WS}         /* discard */
        [#;].*{NL}   INCLINE();
        {NL}         INCLINE();
        \"           {
                       if (!line_cont)
                          ptr = buf;
                       BEGIN (STRING);
                     }
        .            {
                       unput (yytext[0]);
                       BEGIN (PREFIX);
                     }
}

<STRING>{
        \"           {
                       line_cont = 0;
                       BEGIN (EO_LINE);
                     }
        \\a          AddChar ('\7');
        \\b          AddChar ('\b');
        \\t          AddChar ('\t');
        \\f          AddChar ('\f');
        \\e          AddChar (27);
        \\r          AddChar ('\r');
        \\n          AddChar ('\n');
        \\\\         AddChar ('\\');
        \\\"         AddChar ('\"');
        \\{HEX}      AddHexcode (yytext+2);
        \\{OCTAL}    AddOctal   (yytext+1);
        \\{DECIMAL}  AddDecimal (yytext+1);
        \\.          Abort ("Unknown ESC code `%s'",yytext);
        {NL}         Abort ("unterminated line\n");
        .            AddChar (yytext[0]);
        <<EOF>>      Abort ("Unexpected EOF");
}

<EO_LINE>{
        \\\\         {
                       line_cont = 1;
                       BEGIN (INITIAL);
                     }
        {NL}         {            /* real end-of-string reached */
                       *ptr = 0;
                       ptr  = NULL;
                       if (do_ref)
                            crc_ref = string_crc (buf);
                       else AddEntry();
                       INCLINE();
                       BEGIN (INITIAL);
                     }
        .            /* discard */
        <<EOF>>      Abort ("Unexpected end-of-string");
}

<PREFIX>{
        {LANG}":"    {
                       DEBUG ("lang `%s',\n",yytext);
                       if (!strncmp(yytext,lang.prefix,3))
                       {
                         ptr    = buf;
                         do_ref = 0;
                       }
                       else
                       {
                         ptr    = NULL;
                         do_ref = 1;
                       }
                       BEGIN (INITIAL);
                     }
        [#;].*{NL}   lang.line++;
        {NL}         INCLINE();
        .            Abort ("unknown language prefix `%s'", yytext);
}

%%

/*---------------------------------------------------------------------*/


/*
 *  terminate yylex() when EOF reached
 */
int yywrap (void)
{
  return (1);
}

/*
 *  Add a character to output buffer
 */
static void AddChar (int ch)
{
  if (ptr)
  {
    DEBUG ("%c", ch);
    *ptr++ = ch;
    if (ptr >= end)
       fprintf (stderr, "string too large at line %lu\n", lang.line);
  }
}

/*
 *  Add a decimal code to output buffer
 */
static void AddDecimal (char *buf)
{
  if (ptr)
  {
    unsigned dec;
    sscanf (buf, "%d", &dec);
    DEBUG ("dec <%d>", dec);
    *ptr++ = dec;
    if (dec > 255)
       Abort ("Illegal decimal");
    if (ptr >= end)
       fprintf (stderr, "string too large at line %lu\n", lang.line);
  }
}

/*
 *  Add a hex code to output buffer
 */
static void AddHexcode (char *buf)
{
  if (ptr)
  {
    WORD hex;
    sscanf (buf, "%hx", &hex);
    DEBUG ("hex <%X>", hex);
    *ptr++ = hex;
    if (ptr >= end)
       fprintf (stderr, "string too large at line %lu\n", lang.line);
  }
}

/*
 *  Add an octal code to output buffer
 */
static void AddOctal (char *buf)
{
  if (ptr)
  {
    unsigned oct;
    sscanf (buf, "%o", &oct);
    DEBUG ("oct <%o>", oct);
    *ptr++ = oct;
    if (ptr >= end)
       fprintf (stderr, "string too large at line %lu\n", lang.line);
  }
}

/*
 *  Add an language entry for translation
 */
static void AddEntry (void)
{
  char        *str;
  struct Node *list = NULL;

  if ((str = strdup(buf)) == NULL ||
     (list = malloc(sizeof(*list))) == NULL)
    fprintf (stderr, "Language file too big!\n");

  list->next   = lang.list;
  list->crc    = crc_ref;
  list->string = str;
  lang.list    = list;
  lang.entries++;
  do_ref = 1;
}

/*
 * Comparision routine needed by Quick-sort routine.
 * Don't use unsigned arithmetic.
 */
typedef int (*CompareFunc) (const void*, const void*);

static int Compare (struct Elem *a, struct Elem *b)
{
  if (a->crc > b->crc) return (1);
  if (a->crc < b->crc) return (-1);
  return (0);
}

/*
 * Sort the linked list in accending order of CRC value
 */
static int SortLanguageEntries (DWORD num)
{
  struct Node  *lng, *next;
  struct Elem *array = malloc (num * sizeof(*array));

  lang.array = array;
  if (!array)
     return (0);

  for (lng = lang.list; lng; lng = next)
  {
    array->crc    = lng->crc;
    array->string = lng->string;
    array++;
    next = lng->next;
    free (lng);
  }
  lang.list = NULL;
  qsort (lang.array, (size_t)num, sizeof(*array), (CompareFunc)Compare);
  return (1);
}

/*
 *  Initialize the linked list for language.
 *
 *  The wattcp.cfg entry is on format LANGUAGE = lang,language-file
 *  e.g. "LANGUAGE = de,$(ETC)\language.txt"
 *
 *  Country-code MUST be two characters.
 */
void ftp_lang_init (const char *value)
{
  FILE *langf;
  char *s;

  if (!value)
     return;

  s = strchr (value, ',');
  if (!s || (s - value) != 2)
     Abort ("Illegal LANGUAGE syntax");

  lang.prefix[0] = value[0];
  lang.prefix[1] = value[1];
  lang.prefix[2] = ':';
  lang.prefix[3] = 0;
  strlwr (lang.prefix);

  if (!strncmp(lang.prefix,"en",2))  /* english is default language */
     return;

  if (!crc_init())
     Abort ("crc_init() failed");

  lang.file = ++s;
  if (lang.file[0] == '~' &&    /* "~/" -> home-directory */
      lang.file[1] == '/')
  {
    char fname [_MAX_PATH];
    sprintf (fname, "%s%c%s", progPath, SLASH, lang.file+2);
    lang.file = fname;
  }

  langf = fopen (lang.file, "rb");
  if (!langf)
     Abort ("Cannot open language file `%s'", lang.file);

  lang.entries = 0;
  yyin = langf;
  yylex();
  yy_delete_buffer (YY_CURRENT_BUFFER);
  fclose (langf);

#ifdef TEST
  printf ("lang.file    = `%s'\n",   lang.file);
  printf ("lang.prefix  = `%s'\n",   lang.prefix);
  printf ("lang.entries = %lu\n",    lang.entries);
  printf ("lang.lines   = %lu\n",    lang.line);
  printf ("lang.array   = %08lX\n\n",lang.array);
#endif

  if (!SortLanguageEntries(lang.entries))
     Abort ("Could not sort LANGUAGE entries");

#ifndef TEST_MAIN
  TranslateCmdTab();
#endif
}

/*
 * return foreign translation of string 'str'
 */
const char *lang_xlat (const char *str)
{
  struct Elem *elem;
  DWORD  crc;

  if (!lang.entries || !str)
     return (str);

  crc  = string_crc (str);
  elem = (struct Elem*) bsearch (&crc, lang.array, lang.entries,
                                 sizeof(*elem), (CompareFunc)Compare);
  if (!elem || !elem->string || !elem->string[0])
     return (str);

  return (const char*)elem->string;
}

/*
 * Print message and abort program
 */
static void Abort (char *fmt, ...)
{
  va_list args;

  if (lang.line)
       fprintf (stderr, "\nline %lu: ", lang.line);
  else fputc ('\n', stderr);

  va_start (args, fmt);
  vfprintf (stderr, fmt, args);
  fputc ('\n', stderr);
  va_end (args);
  exit (1);
}

/*----------------------------------------------*/

#ifdef TEST_MAIN
char progPath [_MAX_PATH] = ".\\";

int main (void)
{
  struct Elem *elem;
  int    i;

  ftp_lang_init ("sv,~/ftp.lng");  /* test Swedish language */

  for (i = 0, elem = lang.array; i < lang.entries; i++, elem++)
      printf ("CRC = %08lX, <%s>\n", elem->crc, elem->string);
  return (0);
}
#endif
