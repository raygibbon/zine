/*
 * 
 * servidor_util.h - Algumas funcoes úteis para o servidor
 * 
 * (c) 1998 Nuno Sucena Almeida <slug@student.dee.uc.pt>
 * 
 */

struct stats
{
   char *login, *password, *hostname , *hostip;
   int login_ok, password_ok, ok;
   int soquete;
   struct stats *next, *previous;
};

typedef struct stats status;

/*
 * Watt-32 foreign language translation facility.
 * Based on idea from PGP, but totally rewritten (using flex)
 *
 * Strings with _LANG() around them are found by the `mklang' tool and
 * put into a text file to be translated into foreign language at run-time.
 */   
#ifdef LANG_XLAT
  extern void lang_init (char *value);
  extern const char *lang_xlate (const char *str);
  #define _LANG(str) lang_xlate(str)
#else
  #define _LANG(str)  str
#endif

extern void mostra_versao (void);
extern void show_memory_usage (void);
extern void mostra_ajuda(void);
extern int parse_server_options ( int argc , char *argv[] );
extern int parse_user_commands ( int soquete , char *command_line , status *list_status , void *command_list , void *port_list);
extern void create_new_connection ( status **list , status **new_pointer );
extern status *find_new_connection ( status *list , int soquete );
extern void free_new_connection ( status **list , int soquete );
extern char *craig_strtok ( char **handle, char *string, char *delim );
extern char *strtok_r (char *s, const char *delim, char **save_ptr);

