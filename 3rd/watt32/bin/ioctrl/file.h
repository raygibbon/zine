/*
 * 
 * servidor_file.h - Suporte para leitura de ficheiros
 * 
 * 
 * (c) 1998 Nuno Sucena Almeida <slug@student.dee.uc.pt>
 * 
 */

struct command
{
   char *command_name;
   char *command_value;
   int port, bit , bit_value;
   int init_value;
   struct command *next;
};
typedef struct command commands;

struct porto
{
   int port, value;
   struct porto *next;
};
typedef struct porto portos;

extern int read_command_file ( char *name , commands **list );
extern int parse_command_line ( char *read_line , char *separator , commands **line );
extern void show_command ( commands *list );
extern int check_password_file ( char *nome , char *login , char *chave );
extern portos *find_port_in_list ( portos *list , int port_number );
extern void create_port_list ( commands *listc , portos **listp );
extern void show_port_list ( portos *list , int *descriptor , int tipo);
extern void free_command_list ( commands **list );
extern int save_port_list ( char *name , portos *list );
extern int read_port_list ( char *name , portos *list );
extern int redirecciona_log ( char *nome_stdout , char *nome_stderr );
extern signed int verify_address_on_file ( char *nome , char *endereco );
extern int verify_connect_address ( char *nome , void *current );


/* Nome do ficheiro dos comandos: */
extern char *V_command_file;
/* Nome do ficheiro com os utilizadores: */
extern char *V_password_file;
/* Nome do ficheiro com os valores dos portos: */
extern char *V_ports_file;
/* Nome do ficheiro com os enderecos: */
extern char *V_address_file;
/* Nome do ficheiro de log do STDOUT: */
extern char *V_stdout_file;
/* Nome do ficheiro de log do STDERR: */
extern char *V_stderr_file;
/* Nome da directoria principal: */
extern char *V_root_directory;
