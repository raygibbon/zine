/*
 *  Package: srpftp
 *  Author: Eugene Jhong
 */

/*
 * Copyright (c) 1997 Stanford University
 *
 * The use of this software for revenue-generating purposes may require a
 * license from the owners of the underlying intellectual property.
 *
 * Within that constraint, permission to use, copy, modify, and distribute
 * this software and its documentation for any purpose is hereby granted
 * without fee, provided that the above copyright notices and this permission
 * notice appear in all copies of the software and related documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * IN NO EVENT SHALL STANFORD BE LIABLE FOR ANY SPECIAL, INCIDENTAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER OR NOT ADVISED OF
 * THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF LIABILITY, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef USE_SRP

#include <t_pwd.h>
#include <t_client.h>
#include <krypto.h>
#include <arpa/ftp.h>

#include "ftp.h"

char  srp_user[FTP_BUFSIZ];
char *srp_pass;
char *srp_acct;

static BYTE srp_pref_cipher = 0;
static BYTE srp_pref_hash   = 0;

static struct t_client *tc       = NULL;
static BYTE            *skey     = NULL;
static krypto_context  *incrypt  = NULL;
static krypto_context  *outcrypt = NULL;

/*
 * srp_selcipher: select cipher
 */
int srp_selcipher (char *cname)
{
  cipher_desc *cd = cipher_getdescbyname (cname);

  if (!cd)
  {
    int   i;
    BYTE *list = cipher_getlist();

    fprintf (stderr, _LANG("ftp: supported ciphers:\n"));
    for (i = 0; i < strlen(list); i++)
        fprintf (stderr, "        %s\n", cipher_getdescbyid(list[i])->name);
    return (-1);
  }
  srp_pref_cipher = cd->id;
  return (0);
}

/*
 * srp_selhash: select hash
 */
int srp_selhash (char *hname)
{
  hash_desc *hd = hash_getdescbyname (hname);

  if (!hd)
  {
    int   i;
    BYTE *list = hash_getlist();

    fprintf (stderr, _LANG("ftp: supported hash functions:\n"));
    for (i = 0; i < strlen (list); i++)
        fprintf (stderr, "        %s\n", hash_getdescbyid(list[i])->name);
    return (-1);
  }       
  srp_pref_hash = hd->id;
  return (0);
}

/*
 * srp_userpass: get username and password
 */
void srp_userpass (char *host)
{
  struct URL url;

  memset (&url,0,sizeof(url));
  _strncpy (url.host, host, sizeof(url.host)-1);

  GetUserPass (&url);
  srp_pass = url.pass;
  srp_acct = url.acct;

  while (!url.user[0])
     UserPrompt (url.user, sizeof(url.user), "Name (%s): ", url.host);
  strcpy (srp_user, url.user);
}

/*
 * srp_reset: reset srp information
 */
void srp_reset (void)
{
  if (tc)
  {
    t_clientclose (tc);
    tc = NULL;
  }
  if (incrypt)
  {
    krypto_delete (incrypt);
    incrypt = NULL;
  }
  if (outcrypt)
  {
    krypto_delete (outcrypt);
    outcrypt = NULL;
  }
}

/*
 * srp_auth: perform srp authentication
 */
int srp_auth (char *host, char *user, char *pass)
{
  struct t_num *wp;
  struct t_num  N;
  struct t_num  g;
  struct t_num  s;
  struct t_num  yp;
  BYTE   buf[FTP_BUFSIZ];
  BYTE   tmp[FTP_BUFSIZ];
  BYTE  *bp, *cp;
  BYTE   cid  = 0;
  BYTE   hid  = 0;
  int    verb = cfg.verbose;
  int    e, clen, blen, i;

  srp_pass = srp_acct = NULL;
  cfg.verbose = 0;

  e = Command ("AUTH %s", "SRP");
  ftp_log ("SRP command: `AUTH SRP' -> %s", OkComplete(e));
  if (e == CONTINUE)
  {
    BYTE vers[4];
    memset (vers, 0, 4);
    vers[3] = SRP_PROT_VERSION;

    if (verb)
       xputs (CtrlText, _LANG("SRP accepted as authentication type.\r\n"));

    /* send protocol version
     */
    bp   = tmp;
    blen = 0;
    srp_put (vers, &bp, 4, &blen);
    e = radix_encode (tmp, buf, &blen, 0);
    if (e)
       goto encode_error;

    reply_parse = "ADAT=";
    e = Command ("ADAT %s", buf);
    ftp_log ("SRP command: `ADAT %s' -> %s", buf, OkComplete(e));
  }

  if (e == CONTINUE)
  {
    /* get protocol version
     */
    bp = buf;

    if (!reply_parse)
       goto data_error;
    e = radix_encode (reply_parse, bp, &blen, 1);
    if (e)
       goto decode_error;
    if (srp_get (&bp, &cp, &blen, &clen) != 4)
       goto data_error;

    /* get username and password if necessary
     */
    if (host)
       srp_userpass (host);
    else
    {
      strcpy (srp_user, user);
      srp_pass = pass;
    }

    /* send username
     */
    bp   = tmp;
    blen = 0;
    srp_put (srp_user, &bp, strlen (srp_user), &blen);
    e = radix_encode (tmp, buf, &blen, 0);
    if (e)
       goto encode_error;

    reply_parse = "ADAT=";
    e = Command ("ADAT %s", buf);
    ftp_log ("SRP command: `ADAT %s' -> %s", buf, OkComplete(e));
  }
  
  if (e == CONTINUE)
  {
    bp = buf;

    if (!reply_parse)
       goto data_error;
    e = radix_encode (reply_parse, bp, &blen, 1);
    if (e)
       goto decode_error;

    /* get N, g and s
     */
    if (srp_get (&bp, &N.data, &blen, &N.len) < 0)
       goto data_error;

    if (srp_get (&bp, &g.data, &blen, &g.len) < 0)
       goto data_error;

    if (srp_get (&bp, &s.data, &blen, &s.len) < 0)
       goto data_error;

    tc = t_clientopen (srp_user, &N, &g, &s);
    if (!tc)
    {
      WARN ("Unable to open SRP client structure.\r\n");
      goto bad;
    }

    wp = t_clientgenexp (tc);

    /* send wp
     */
    bp   = tmp;
    blen = 0;
    srp_put (wp->data, &bp, wp->len, &blen);
    e = radix_encode (tmp, buf, &blen, 0);
    if (e)
       goto encode_error;

    reply_parse = "ADAT=";
    e = Command ("ADAT %s", buf);
    ftp_log ("SRP command: `ADAT %s' -> %s", buf, OkComplete(e));
  }

  if (e == CONTINUE)
  {
    bp = buf;

    /* get yp
     */
    if (!reply_parse)
       goto data_error;

    e = radix_encode (reply_parse, bp, &blen, 1);
    if (e)
       goto decode_error;

    if (srp_get (&bp, &(yp.data), &blen, &yp.len) < 0)
       goto data_error;

    if (!srp_pass)
       srp_pass = GetPass ("SRP Password:");
    t_clientpasswd (tc, srp_pass);
    memset (srp_pass, 0, strlen(srp_pass));

    skey = t_clientgetkey (tc, &yp);

    /* send response
     */
    bp   = tmp;
    blen = 0;
    srp_put (t_clientresponse(tc), &bp, 20, &blen);
    e = radix_encode (tmp, buf, &blen, 0);
    if (e)
       goto encode_error;

    reply_parse = "ADAT=";
    e = Command ("ADAT %s", buf);
    ftp_log ("SRP command: `ADAT %s' -> %s", buf, OkComplete(e));
  }

  if (e == CONTINUE)
  {
    bp = buf;

    /* get response
     */
    if (!reply_parse)
       goto data_error;
    e = radix_encode (reply_parse, bp, &blen, 1);
    if (e)
       goto encode_error;
    if (srp_get (&bp, &cp, &blen, &clen) != 20)
       goto data_error;

    if (t_clientverify (tc, cp))
    {         
      WARN ("WARNING: bad response to client challenge.\r\n");
      goto bad;
    }

    /* send nothing
     */
    bp   = tmp;
    blen = 0;
    srp_put ("\0", &bp, 1, &blen);
    e = radix_encode (tmp, buf, &blen, 0);
    if (e)
       goto encode_error;

    reply_parse = "ADAT=";
    e = Command ("ADAT %s", buf);
    ftp_log ("SRP command: `ADAT %s' -> %s", buf, OkComplete(e));
  }

  if (e == CONTINUE)
  {
    BYTE  seqnum[4];
    BYTE *clist;
    BYTE *hlist;
    int   clist_len, hlist_len;

    bp = buf;

    /* get cipher list, hash list, seqnum
     */
    if (!reply_parse)
       goto data_error;

    e = radix_encode (reply_parse, bp, &blen, 1);
    if (e)
       goto encode_error;

    if (srp_get (&bp, &clist, &blen, &clist_len) < 0)
       goto data_error;

    if (srp_get (&bp, &hlist, &blen, &hlist_len) < 0)
       goto data_error;

    if (srp_get (&bp, &cp, &blen, &clen) != 4)
       goto data_error;

    memcpy (seqnum, cp, 4);

    /* choose cipher
     */
    if (cipher_supported (clist, srp_pref_cipher))
       cid = srp_pref_cipher;

    if (!cid && cipher_supported (clist, SRP_DEFAULT_CIPHER))
       cid = SRP_DEFAULT_CIPHER;

    if (!cid)
    {
      BYTE *loclist = cipher_getlist();
      for (i = 0; i < strlen(loclist); i++)
         if (cipher_supported (clist, loclist[i]))
         {
           cid = loclist[i];
           break;
         }
    }

    if (!cid)
    {
      WARN ("Unable to agree on cipher.\r\n");
      goto bad;
    }

    /* choose hash
     */
    if (srp_pref_hash && hash_supported (hlist, srp_pref_hash))
       hid = srp_pref_hash;

    if (!hid && hash_supported (hlist, SRP_DEFAULT_HASH))
       hid = SRP_DEFAULT_HASH;

    if (!hid)
    {
      BYTE *loclist = hash_getlist();

      for (i = 0; i < strlen (loclist); i++)
          if (hash_supported (hlist, loclist[i]))
          {
            hid = loclist[i];
            break;
          }
    }

    if (!hid)
    {
      WARN ("Unable to agree on hash.\r\n");
      goto bad;
    }

    /* set incrypt
     */
    incrypt = krypto_new (cid, hid, skey, 20, NULL, 0, seqnum, KRYPTO_DECODE);
    if (!incrypt)
       goto bad;

    /* generate random number for outkey and outseqnum
     */
    t_random (seqnum, 4);

    /* send cid, hid, outkey, outseqnum
     */
    bp   = tmp;
    blen = 0;
    srp_put (&cid, &bp, 1, &blen);
    srp_put (&hid, &bp, 1, &blen);
    srp_put (seqnum, &bp, 4, &blen);
    e = radix_encode (tmp, buf, &blen, 0);
    if (e)
       goto encode_error;

    reply_parse = "ADAT=";
    e = Command ("ADAT %s", buf);
    ftp_log ("SRP command: `ADAT %s' -> %s", buf, OkComplete(e));

    /* set outcrypt
     */
    outcrypt = krypto_new (cid, hid, skey+20, 20, NULL,
                           0, seqnum, KRYPTO_ENCODE);
    if (!outcrypt)
       goto bad;

    t_clientclose (tc);
    tc = NULL;
  }

  if (e != COMPLETE)
     goto bad;

  if (verb)
     xprintf (CtrlText, _LANG("SRP authentication succeeded.\r\n"
              "Using cipher %s and hash function %s.\r\n"),
              cipher_getdescbyid(cid)->name, hash_getdescbyid(hid)->name);

  cfg.verbose   = verb;
  reply_parse   = NULL;
  cfg.auth_type = "SRP";

  ftp_setpriv();
  return (1);

encode_error:
  WARN1 ("Base 64 encoding failed: %s.\r\n", radix_error(e));
  goto bad;

decode_error:
  WARN1 ("Base 64 decoding failed: %s.\r\n", radix_error(e));
  goto bad;

data_error:  
  WARN ("Unable to unmarshal authentication data.\r\n");
  goto bad;

bad:
  WARN ("SRP authentication failed, trying regular login.\r\n");
  cfg.verbose = verb;
  reply_parse = NULL;
  return (0);
}

/*
 * srp_put: put item to send buffer
 */
void srp_put (BYTE *in, BYTE **out, int inlen, int *outlen)
{
  DWORD net_len = htonl (inlen);

  memcpy (*out, &net_len, 4);
  *out += 4; *outlen += 4;   
  memcpy (*out, in, inlen);  
  *out    += inlen;
  *outlen += inlen;
}

/*
 * srp_get: get item from receive buffer
 */
int srp_get (BYTE **in, BYTE **out, int *inlen, int *outlen)
{
  DWORD net_len;

  if (*inlen < 4)
     return (-1);

  memcpy (&net_len, *in, sizeof(net_len));
  *inlen -= 4;
  *in    += 4;
  *outlen = ntohl (net_len);

  if (*inlen < *outlen)
     return (-1);

  *out    = *in;
  *inlen -= *outlen;
  *in    += *outlen;
  return (*outlen);
}

/*
 * srp_encode: encode control message
 */
int srp_encode (int private, BYTE *in, BYTE *out, unsigned len)
{
  if (private)
     return krypto_msg_priv (outcrypt, in, out, len);
  return krypto_msg_safe (outcrypt, in, out, len);
}

/*
 * srp_decode: decode control message
 */
int srp_decode (int private, BYTE *in, BYTE *out, unsigned len)
{
  if (private)
     return krypto_msg_priv (incrypt, in, out, len);
  return krypto_msg_safe (incrypt, in, out, len);
}

#endif /* USE_SRP */
