#ifndef __SRP_H
#define __SRP_H

#define SRP_PROT_VERSION     1
#define SRP_DEFAULT_HASH     HASH_ID_SHA
#define SRP_DEFAULT_CIPHER   CIPHER_ID_CAST5_CBC

extern unsigned maxbuf;      /* maximum output buffer size */
extern unsigned actualbuf;   /* malloced output buffer size */
extern BYTE    *ucbuf;       /* cleartext buffer */

extern char  srp_user [FTP_BUFSIZ];
extern char *srp_pass;
extern char *srp_acct;
extern BYTE  srp_pref_cipher, srp_pref_hash;

extern int  srp_selcipher(char *cname);
extern int  srp_selhash  (char *hname);
extern void srp_userpass (char *host);
extern void srp_reset    (void);
extern void srp_put      (BYTE  *in, BYTE **out, int  inlen, int *outlen);
extern int  srp_get      (BYTE **in, BYTE **out, int *inlen, int *outlen);
extern int  srp_encode   (int private, BYTE *in, BYTE *out, unsigned len);
extern int  srp_decode   (int private, BYTE *in, BYTE *out, unsigned len);
extern int  srp_auth     (char *host, char *user, char *pass);

#endif
