/*
** Declaration of functions.
**
**  @(#)defs.h              e07@nikhef.nl (Eric Wassenaar) 970511
*/

/*
** Internal modules of the host utility
** ------------------------------------
*/
  /* main.c */

int main                (int, char **);
void set_defaults       (char *, int, char **);
int process_argv        (int, char **);
int process_file        (FILE *);
int process_name        (char *);
int execute_name        (char *);
bool execute            (char *, ipaddr_t);
bool host_query         (char *, ipaddr_t);
char *myhostname        (void);
void set_server         (char *);
void set_logfile        (char *);
void fatal              (char *, ...);
void errmsg             (char *, ...);

  /* info.c */

bool get_hostinfo       (char *, bool);
bool get_domaininfo     (char *, char *);
int get_info            (querybuf *, char *, int, int);
bool print_info         (querybuf *, int, char *, int, int, bool);
void print_data         (char *, ...);
u_char *print_rrec      (char *, int, int, u_char *, u_char *, u_char *, bool);
u_char *skip_qrec       (char *, int, int, u_char *, u_char *, u_char *);
bool get_recursive      (char **);

  /* list.c */

bool list_zone          (char *);
bool find_servers       (char *);
bool get_servers        (char *);
bool get_nsinfo         (querybuf *, int, char *);
void sort_servers       (void);
bool skip_transfer      (char *);
void do_check           (char *);
void do_soa             (char *, struct in_addr, char *);
bool do_transfer        (char *);
bool transfer_zone      (char *, struct in_addr, char *);
bool get_zone           (char *, struct in_addr, char *);
void update_zone        (char *);
bool get_mxrec          (char *);
char *get_primary       (char *);
bool check_zone         (char *);
bool get_soainfo        (querybuf *, int, char *);
void check_soa          (querybuf *, char *);
bool check_dupl         (ipaddr_t);
bool check_ttl          (char *, int, int, int);
void clear_ttltab       (void);
int host_index          (char *, bool);
void clear_hosttab      (void);
int zone_index          (char *, bool);
void clear_zonetab      (void);
int check_canon         (char *);

  /* addr.c */

bool check_addr         (char *);
bool check_addr_name    (struct in_addr, char *);
bool check_name         (ipaddr_t);
bool check_name_addr    (char *, ipaddr_t);

  /* geth.c */

struct hostent *geth_byname (char *);
struct hostent *geth_byaddr (char *, int, int);

  /* util.c */

int parse_type          (char *);
int parse_class         (char *);
char *in_addr_arpa      (char *);
char *nsap_int          (char *);
void print_host         (char *, struct hostent *);
void show_res           (void);
void print_statistics   (char *, int, int);
void clear_statistics   (void);
void show_types         (char *, int, int);
void ns_error           (char *, int, int, char *);
char *decode_error      (int);
void print_status       (querybuf *, int);
void pr_error           (char *, ...);
void pr_warning         (char *, ...);
bool want_type          (int, int);
bool want_class         (int, int);
bool indomain           (char *, char *, bool);
bool samedomain         (char *, char *, bool);
bool gluerecord         (char *, char *, char **, int);
int matchlabels         (char *, char *);
char *pr_domain         (char *, bool);
char *pr_dotname        (char *);
char *pr_nsap           (char *);
char *pr_type           (int);
char *pr_class          (int);
int expand_name         (char *, int, u_char *, u_char *, u_char *, char *);
int check_size          (char *, int, u_char *, u_char *, u_char *, int);
bool valid_name         (char *, bool, bool, bool);
int canonical           (char *);
char *mapreverse        (char *, struct in_addr);
int compare_name        (const void *, const void *);

  /* misc.c */

void *xalloc            (void*, int);
char *xtoa              (int);
char *stoa              (u_char *, int, bool);
char *base_ntoa         (u_char *, int);
char *nsap_ntoa         (u_char *, int);
char *ipng_ntoa         (u_char *);
char *pr_date           (int);
char *pr_time           (int, bool);
char *pr_spherical      (int, char *, char *);
char *pr_vertical       (int, char *, char *);
char *pr_precision      (int);

  /* send.c */

#ifdef HOST_RES_SEND
int res_send            (qbuf_t *, int, qbuf_t *, int);
#endif

int _res_connect        (int, struct sockaddr_in *, int);
int _res_write          (int, struct sockaddr_in *, char *, char *, int);
int _res_read           (int, struct sockaddr_in *, char *, char *, int);
void _res_perror        (struct sockaddr_in *, char *, char *);

