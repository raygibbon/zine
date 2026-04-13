#ifndef __POP_SMTP_H
#define __POP_SMTP_H

#define POP_PORT        110
#define SMTP_PORT       25
#define TEMP_BUF_LEN    1000
#define NO_FUNC         ((void)0)

typedef enum {
        US_ACSII,
        ISO_8859
      } Encoding;

struct Header {
       char to   [100];
       char from [100];
       char subj [100];
       char reply[100];
       char date [100];
       Encoding charset;
     };

extern int    POP_debugOn, SMTP_debugOn;
extern int    SMTP_errCond;
extern char  *SMTP_errMsg;
extern struct Header msg_header;


extern tcp_Socket *pop_init      (const char *host);
extern int         pop_login     (tcp_Socket *sock, int apop, const char *userid, const char *password);
extern int         pop_status    (tcp_Socket *sock, long *count, long *totallength);
extern long        pop_length    (tcp_Socket *sock, long msg_num, long *size);
extern int         pop_list      (tcp_Socket *sock, long *msg_nums, long *sizes);
extern int         pop_header    (tcp_Socket *sock, long msg_num, FILE *fil);
extern int         pop_delete    (tcp_Socket *sock, long msg_num);
extern int         pop_getf      (tcp_Socket *sock, FILE *f, long msg_num);
extern int         pop_get_nextf (tcp_Socket *sock, FILE *f);
extern int         pop_get_file  (tcp_Socket *sock, char *fname, long msg_num);
extern int         pop_shutdown  (tcp_Socket *sock, int fast);

extern tcp_Socket *smtp_init               (char *host, char * dom);
extern char       *smtp_parse_from_line    (FILE *f);
extern char      **smtp_parse_to_line      (FILE *f);
extern char       *smtp_send_MAIL_FROM_line(tcp_Socket *sock, FILE * f);
extern int         smtp_send_RCPT_TO_line  (tcp_Socket *sock, FILE * f);
extern int         smtp_sendf              (tcp_Socket *sock, FILE *f);
extern int         smtp_send_file          (tcp_Socket *sock, char *fname);
extern int         smtp_shutdown           (tcp_Socket *sock);


/*
 * SMTP reply codes (from RFC821)
 */

 /*  4.2.1.  REPLY CODES BY FUNCTION GROUPS */

#define SMTP_SYNTAX   500  /* Syntax error, command unrecognized               */
                           /* [such as command line too long]                  */
#define SMTP_PARAM    501  /* Syntax error in parameters or arguments          */
#define SMTP_COM_NI   502  /* Command not implemented                          */
#define SMTP_BAD_SEQ  503  /* Bad sequence of commands                         */
#define SMTP_BAD_PARM 504  /* Command parameter not implemented                */

#define SMTP_STATUS   211  /* System status, or system help reply              */
#define SMTP_HELP     214  /* Help message                                     */
                           /* [Information on how to use the receiver or the   */
                           /* meaning of a particular non-standard command;    */
                           /* this reply is useful only to the human user]     */

#define SMTP_READY    220  /* <domain> Service ready                           */
#define SMTP_BYE      221  /* <domain> Service closing transmission channel    */
#define SMTP_OOPS     421  /* <domain> Service not available, closing          */
                           /* transmission channel                             */
                           /* [This may be a reply to any command if the       */
                           /* service knows it must shut down]                 */

#define SMTP_OK       250  /* Requested mail action okay, completed            */
#define SMTP_WILL_FWD 251  /* User not local; will forward to <forward-path>   */
#define SMTP_BUSY     450  /* Requested action not taken: mailbox unavailable  */
                           /* [E.g., mailbox busy]                             */
#define SMTP_ACCESS   550  /* Requested action not taken: mailbox unavailable  */
                           /* [E.g., mailbox not found, no access]             */
#define SMTP_ERROR    451  /* Requested action aborted: error in processing    */
#define SMTP_YOU_FWD  551  /* User not local; please try <forward-path>        */
#define SMTP_SQUEEZED 452  /* Requested action not taken: insufficient system  */
                           /* storage                                          */
#define SMTP_FULL     552  /* Requested mail action aborted: exceeded storage  */
                           /* allocation                                       */
#define SMTP_BAD_NAM  553  /* Requested action not taken: E.g., mailbox syntax */
                           /* incorrect]                                       */
#define SMTP_GIMME    354  /* Start mail input; end with <CRLF>.<CRLF>         */
#define SMTP_FAILED   554  /* Transaction failed                               */
#define SMTP_BAD_FILE 401  /* Unable to open file                              */
#define SMTP_ERR      520  /* Host timed out                                   */


#define SMTP_S_CLOSED 511  /* Host closed unexpectedly                         */
#define SMTP_PROB     410  /* Host timed out                                   */

#endif
