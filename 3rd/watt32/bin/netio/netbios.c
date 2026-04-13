/* netbios.c
 * NetBIOS access library
 *
 * Author:  Kai Uwe Rommel <rommel@ars.de>
 * Created: Tue Apr 02 1996
 */

static char *rcsid =
"$Id: netbios.c,v 1.3 1997/08/09 16:38:55 rommel Exp rommel $";
static char *rcsrev = "$Revision: 1.3 $";

/*
 * $Log: netbios.c,v $
 * Revision 1.3  1997/08/09 16:38:55  rommel
 * code simplification for different OS support
 *
 * Revision 1.2  1997/08/09 16:01:48  rommel
 * added Win32 support
 *
 * Revision 1.1  1997/08/09 15:07:58  rommel
 * Initial revision
 *
 */

#if !defined(UNIX) && !defined(WATT32)

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#include "netbios.h"

#ifdef WIN32

#define NetBIOS(PNCB) Netbios((struct _NCB *) PNCB)

#else /* !WIN32 */

#ifdef DOS /* only real-mode DOS with LAN-Manager */

#include <dos.h>
#define strncpy _fstrncpy

#else /* !DOS */

#define OS2
#define INCL_DOSSEMAPHORES
#define INCL_DOSMODULEMGR
#include <os2.h>

#ifdef __32BIT__
APIRET16 APIENTRY16 Dos16SemSet(PULONG);
APIRET16 APIENTRY16 Dos16SemClear(PULONG);
APIRET16 APIENTRY16 Dos16SemWait(PULONG, ULONG);
#define DosSemSet    Dos16SemSet
#define DosSemClear  Dos16SemClear
#define DosSemWait   Dos16SemWait
#define DosGetModHandle DosQueryModuleHandle
#define DosGetProcAddr(x,y,z) DosQueryProcAddr(x,0,y,z)
#define FUNCPTR * _Far16 _Pascal
#else /* !__32BIT__ */
#define FUNCPTR _far _pascal *
#define strncpy _fstrncpy
#endif /* __32BIT__ */

#endif /* DOS */
#endif /* WIN32 */

typedef struct
{
  USHORT Length;
  PBYTE Buffer;
}
*PBuf2;

typedef struct netbios_info_1
{
# define DEVLEN 8
# define NETBIOS_NAME_LEN 16
  char nb1_net_name[NETBIOS_NAME_LEN + 1];
  char nb1_driver_name[DEVLEN + 1];	/* OS/2 device driver name        */
  unsigned char nb1_lana_num;		/* LAN adapter number of this net */
  char nb1_pad_1;
  unsigned short nb1_driver_type;
  unsigned short nb1_net_status;
  unsigned long nb1_net_bandwidth;	/* Network bandwidth, bits/second */
  unsigned short nb1_max_sess;		/* Max number of sessions         */
  unsigned short nb1_max_ncbs;		/* Max number of outstanding NCBs */
  unsigned short nb1_max_names;		/* Max number of names            */
}
NETINFO1, *PNETINFO1;

BOOL NetBIOS_API = NETBIOS;

#ifdef WIN32

USHORT NetBIOS_Avail()
{
  return 0;
}

static void CALLBACK NCBPost(PNCB Ncb)
{
  if (Ncb -> basic_ncb.ncb_semaphore)
    SetEvent(Ncb -> basic_ncb.ncb_semaphore);
}

USHORT NCBWait(PNCB Ncb, ULONG timeout)
{
  if (timeout == -1)
    timeout = INFINITE;

  if (Ncb -> basic_ncb.ncb_semaphore)
    return WaitForSingleObject(Ncb -> basic_ncb.ncb_semaphore, timeout);
  else
    return -1;
}

static void NCBWaitInit(PNCB Ncb, BOOL wait)
{
  if (!wait)
  {
    if (Ncb -> basic_ncb.ncb_semaphore == 0)
      Ncb -> basic_ncb.ncb_semaphore = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (Ncb -> basic_ncb.ncb_semaphore)
      ResetEvent(Ncb -> basic_ncb.ncb_semaphore);
  }
}

#else /* !WIN32 */

#ifdef DOS

USHORT NetBIOS_Avail()
{
  return 0;
}

static USHORT NetBIOS(PNCB16 Ncb)
{
  _asm
  {
    mov bx,Ncb
    mov es,Ncb + 2
    int 0x5C /* call NetBIOS */
  }
}

static void _interrupt NCBPost(void)
{
  PNCB16 Ncb;

  _asm
  {
    mov Ncb,bx
    mov Ncb + 2,es
  }

  Ncb -> basic_ncb.ncb_semaphore = 0;
}

USHORT NCBWait(PNCB Ncb, ULONG timeout)
{
  while (Ncb -> basic_ncb.ncb_semaphore)
  {
    _asm
    {
      mov ax,0x1680
      int 0x2F /* give up time slice under DPMI */
    }
  }

  return 0;
}

static void NCBWaitInit(PNCB Ncb, BOOL wait)
{
  if (!wait)
    Ncb -> basic_ncb.ncb_semaphore = 1;
}

#else /* !DOS */

static USHORT (FUNCPTR NetBIOS)(PNCB16) = NULL;

static USHORT (FUNCPTR NetBIOS_Submit)(USHORT, USHORT, PNCB16) = NULL;
static USHORT (FUNCPTR NetBIOS_Close)(USHORT, USHORT) = NULL;
static USHORT (FUNCPTR NetBIOS_Open)(PSZ, PSZ, USHORT, PUSHORT);
static USHORT (FUNCPTR NetBIOS_Enum)(PSZ, USHORT, PBYTE, USHORT, PUSHORT, PUSHORT) = NULL;

static USHORT NetBEUI_Handle[4] = {0, 0, 0, 0};
static PNETINFO1 pNetinfo = NULL;
static USHORT Netentries = 0;

static USHORT Load_API(PSZ module, PSZ proc, PFN FAR *addr)
{
  int rc, rc1 = 0;
  HMODULE mh;

  if ((rc = DosGetModHandle(module, &mh)) == 0)
    rc1 = DosGetProcAddr(mh, proc, addr);

  if (rc || rc1)
  {
    rc = DosLoadModule(NULL, 0, module, &mh);
    rc1 = 1;
  }

  if (rc == 0 && rc1)
    rc = DosGetProcAddr(mh, proc, addr);

  return rc;
}

USHORT NetBIOS_Avail()
{
  int rc = 0;

  if (NetBIOS_API == NETBEUI)
  {
    if (!NetBIOS_Submit)
    {
      rc |= Load_API("NETAPI", "NETBIOSSUBMIT", (PFN *) &NetBIOS_Submit);
      rc |= Load_API("NETAPI", "NETBIOSCLOSE", (PFN *) &NetBIOS_Close);
      rc |= Load_API("NETAPI", "NETBIOSOPEN", (PFN *) &NetBIOS_Open);
      rc |= Load_API("NETAPI", "NETBIOSENUM", (PFN *) &NetBIOS_Enum);
    }
  }
  else
  {
    if (!NetBIOS)
      rc = Load_API("ACSNETB", "NETBIOS", (PFN *) &NetBIOS);
  }

  return rc;
}

#ifdef __32BIT__
#pragma stack16(256)
static void _Far16 _Cdecl
#else /* !__32BIT__ */
static void _far _loadds
#endif /* __32BIT__ */
NCBPost(USHORT Junk, PNCB16 Ncb)
{
  DosSemClear(&Ncb -> basic_ncb.ncb_semaphore);
}

USHORT NCBWait(PNCB Ncb, ULONG timeout)
{
  if (timeout == -1)
    timeout = SEM_INDEFINITE_WAIT;
  return DosSemWait(&Ncb -> basic_ncb.ncb_semaphore, timeout);
}

static void NCBWaitInit(PNCB Ncb, BOOL wait)
{
  if (!wait)
    DosSemSet((HSEM) &Ncb -> basic_ncb.ncb_semaphore);
}

static USHORT NetBEUI_Config(USHORT lana,
			     PUSHORT sessions, PUSHORT commands, PUSHORT names)
{
  USHORT rc = NB_INADEQUATE_RESOURCES, blen, MaxEntries, i;
  PNETINFO1 temp = NULL;

  if (NetBIOS_Enum)
  {
    if (!pNetinfo)
    {
      NetBIOS_Enum(NULL, 1, (PBYTE) pNetinfo, 0, &Netentries, &MaxEntries);

      if ((pNetinfo = (PNETINFO1)
	   malloc(blen = sizeof(NETINFO1) * MaxEntries)) != NULL)
	if ((rc = NetBIOS_Enum(NULL, 1, (PBYTE) pNetinfo, blen,
			       &Netentries, &MaxEntries)) != 0)
	{
	  free(pNetinfo);
	  pNetinfo = NULL;
	}
    }

    if (pNetinfo)
      if (lana <= Netentries)
      {
	*sessions = pNetinfo[lana].nb1_max_sess;
	*commands = pNetinfo[lana].nb1_max_ncbs;
	*names = pNetinfo[lana].nb1_max_names;
	rc = NB_COMMAND_SUCCESSFUL;
      }
      else
	rc = NB_INVALID_ADAPTER;
  }

  return rc;
}

#endif /* DOS */

#endif /* WIN32 */

USHORT NCBConfig(PNCB Ncb, USHORT lana,
		 PUSHORT sessions, PUSHORT commands, PUSHORT names)
{
  SHORT rc;

#ifdef OS2
  if (NetBIOS_API == NETBEUI)
  {
    memset(Ncb, 0, NCBSIZE);

    if (!(rc = NetBEUI_Config(lana, sessions, commands, names)))
    {
      Ncb -> basic_ncb.bncb.ncb_name[8] = *sessions;
      Ncb -> basic_ncb.bncb.ncb_name[9] = *commands;
      Ncb -> basic_ncb.bncb.ncb_name[10] = *names;
    }
  }
  else
#endif /* OS2 */
  {
    rc = NCBReset(Ncb, lana, 255, 255, 255);
    *sessions = Ncb -> basic_ncb.bncb.ncb_name[0];
    *commands = Ncb -> basic_ncb.bncb.ncb_name[1];
    *names = Ncb -> basic_ncb.bncb.ncb_name[2];
    NCBClose(Ncb, lana);
  }

  return rc;
}

USHORT NCBClose(PNCB Ncb, USHORT lana)
{
  USHORT rc;

#ifdef OS2
  if (NetBIOS_API == NETBEUI)
  {
    if (NetBEUI_Handle[lana])
      rc = NetBIOS_Close(NetBEUI_Handle[lana], 0);
    else
      rc = NB_ENVIRONMENT_NOT_DEFINED;
  }
  else
#endif /* OS2 */
  {
    memset (Ncb, 0, NCBSIZE);
    Ncb -> reset.ncb_command = NB_RESET_WAIT;
    Ncb -> reset.ncb_lsn = 255;
    Ncb -> reset.ncb_lana_num = lana;
    rc = NetBIOS(Ncb);
  }

  return rc;
}

USHORT NCBReset(PNCB Ncb, USHORT lana,
		USHORT sessions, USHORT commands, USHORT names)
{
  int i, rc = NB_INADEQUATE_RESOURCES;

#ifdef OS2
  if (NetBIOS_API == NETBEUI)
  {
    if (!pNetinfo)
      rc = NetBEUI_Config(lana, &sessions, &commands, &names);

    if (pNetinfo)
    {
      if (lana <= Netentries)
      {
	if (pNetinfo[lana].nb1_max_sess >= sessions &&
	    pNetinfo[lana].nb1_max_ncbs >= commands &&
	    pNetinfo[lana].nb1_max_names >= names)
	  rc = NetBIOS_Open(pNetinfo[lana].nb1_net_name,
			       NULL, 1, &NetBEUI_Handle[lana]);
      }
      else
	rc = NB_INVALID_ADAPTER;
    }
  }
  else
#endif /* OS2 */
  {
    memset(Ncb, 0, NCBSIZE);

    Ncb -> reset.ncb_command = NB_RESET_WAIT;
    Ncb -> reset.ncb_lana_num = lana;
    Ncb -> reset.req_sessions = sessions;
    Ncb -> reset.req_commands = commands;
    Ncb -> reset.req_names = names;

    NetBIOS(Ncb);

    rc = Ncb -> reset.ncb_retcode;
  }

  return rc;
}

int NCBSubmit(USHORT lana, PNCB Ncb)
{
#if defined(DOS) || defined(WIN32)
  return NetBIOS(Ncb);
#else /* !DOS */
  return (NetBIOS_API == NETBEUI)
         ? NetBIOS_Submit(NetBEUI_Handle[lana], 0, Ncb)
         : NetBIOS(Ncb);
#endif /* DOS */
}

/* from now on, only system independent code */

int NCBSubmitWait(USHORT lana, PNCB Ncb, BOOL wait)
{
  int rc;

  Ncb -> basic_ncb.bncb.off44.ncb_post_address = (address) (wait ? NULL : NCBPost);

  NCBWaitInit(Ncb, wait);
  rc = NCBSubmit(lana, Ncb);

  return wait ? Ncb -> basic_ncb.bncb.ncb_retcode : rc;
}

USHORT NCBAddName(PNCB Ncb, USHORT lana, PBYTE name)
{
  memset(Ncb, 0, NCBSIZE);

  Ncb -> basic_ncb.bncb.ncb_command = NB_ADD_NAME_WAIT;
  Ncb -> basic_ncb.bncb.ncb_lana_num = lana;
  strncpy (Ncb -> basic_ncb.bncb.ncb_name, name, 16);
  Ncb -> basic_ncb.bncb.ncb_name[15] = 0xff;

  NCBSubmit(lana, Ncb);

  return (Ncb -> basic_ncb.bncb.ncb_retcode);
}

USHORT NCBDeleteName(PNCB Ncb, USHORT lana, PBYTE name)
{
  memset(Ncb, 0, NCBSIZE);

  Ncb -> basic_ncb.bncb.ncb_command = NB_DELETE_NAME_WAIT;
  Ncb -> basic_ncb.bncb.ncb_lana_num = lana;
  strncpy (Ncb -> basic_ncb.bncb.ncb_name, name, 16);
  Ncb -> basic_ncb.bncb.ncb_name[15] = 0xff;

  NCBSubmit(lana, Ncb);

  return (Ncb -> basic_ncb.bncb.ncb_retcode);
}

USHORT NCBAddGroupName(PNCB Ncb, USHORT lana, PBYTE name)
{
  memset(Ncb, 0, NCBSIZE);

  Ncb -> basic_ncb.bncb.ncb_command = NB_ADD_GROUP_NAME_WAIT;
  Ncb -> basic_ncb.bncb.ncb_lana_num = lana;
  strncpy (Ncb -> basic_ncb.bncb.ncb_name, name, 16);
  Ncb -> basic_ncb.bncb.ncb_name[15] = 0xff;

  NCBSubmit(lana, Ncb);

  return (Ncb -> basic_ncb.bncb.ncb_retcode);
}

USHORT NCBCall(PNCB Ncb, USHORT lana, PBYTE lclname, PBYTE rmtname,
	       USHORT recv_timeout, USHORT send_timeout, BOOL wait)
{
  memset(Ncb, 0, NCBSIZE);

  Ncb -> basic_ncb.bncb.ncb_command = wait ? NB_CALL_WAIT : NB_CALL;
  Ncb -> basic_ncb.bncb.ncb_lana_num = lana;
  Ncb -> basic_ncb.bncb.ncb_rto = recv_timeout << 1;	/* times 2 since in   */
  Ncb -> basic_ncb.bncb.ncb_sto = send_timeout << 1;	/* steps of 500 msecs */
  strncpy (Ncb -> basic_ncb.bncb.ncb_name, lclname, 16);
  Ncb -> basic_ncb.bncb.ncb_name[15] = 0xff;
  strncpy (Ncb -> basic_ncb.bncb.ncb_callname, rmtname, 16);
  Ncb -> basic_ncb.bncb.ncb_callname[15] = 0xff;

  return NCBSubmitWait(lana, Ncb, wait);
}

USHORT NCBListen(PNCB Ncb, USHORT lana, PBYTE lclname, PBYTE rmtname,
		 USHORT recv_timeout, USHORT send_timeout, BOOL wait)
{
  memset(Ncb, 0, NCBSIZE);

  Ncb -> basic_ncb.bncb.ncb_command = wait ? NB_LISTEN_WAIT : NB_LISTEN;
  Ncb -> basic_ncb.bncb.ncb_lana_num = lana;
  Ncb -> basic_ncb.bncb.ncb_rto = recv_timeout << 1;	/* times 2 since in   */
  Ncb -> basic_ncb.bncb.ncb_sto = send_timeout << 1;	/* steps of 500 msecs */
  strncpy (Ncb -> basic_ncb.bncb.ncb_name, lclname, 16);
  Ncb -> basic_ncb.bncb.ncb_name[15] = 0xff;
  strncpy (Ncb -> basic_ncb.bncb.ncb_callname, rmtname, 16);
  Ncb -> basic_ncb.bncb.ncb_callname[15] = 0xff;

  return NCBSubmitWait(lana, Ncb, wait);
}

USHORT NCBSend(PNCB Ncb, USHORT lana, USHORT lsn,
	       PBYTE message, word length, BOOL wait)
{
  memset(Ncb, 0, NCBSIZE);

  Ncb -> basic_ncb.bncb.ncb_command = wait ? NB_SEND_WAIT : NB_SEND;
  Ncb -> basic_ncb.bncb.ncb_lana_num = lana;
  Ncb -> basic_ncb.bncb.ncb_lsn = lsn;
  Ncb -> basic_ncb.bncb.ncb_buffer_address = message;
  Ncb -> basic_ncb.bncb.ncb_length = length;

  return NCBSubmitWait(lana, Ncb, wait);
}

USHORT NCBSendDatagram(PNCB Ncb, USHORT lana, USHORT lsn,
		       PSZ rmtname, PBYTE message, word length, BOOL wait)
{
  memset(Ncb, 0, NCBSIZE);

  Ncb -> basic_ncb.bncb.ncb_command = wait ? NB_SEND_DATAGRAM_WAIT : NB_SEND_DATAGRAM;
  Ncb -> basic_ncb.bncb.ncb_lana_num = lana;
  Ncb -> basic_ncb.bncb.ncb_num = lsn;
  Ncb -> basic_ncb.bncb.ncb_buffer_address = message;
  Ncb -> basic_ncb.bncb.ncb_length = length;
  strncpy (Ncb -> basic_ncb.bncb.ncb_callname, rmtname, 16);

  return NCBSubmitWait(lana, Ncb, wait);
}

USHORT NCBSendBroadcast(PNCB Ncb, USHORT lana, USHORT lsn,
			PBYTE message, word length, BOOL wait)
{
  memset(Ncb, 0, NCBSIZE);

  Ncb -> basic_ncb.bncb.ncb_command = wait ? NB_SEND_BROADCAST_DATAGRAM_WAIT : NB_SEND_BROADCAST_DATAGRAM;
  Ncb -> basic_ncb.bncb.ncb_lana_num = lana;
  Ncb -> basic_ncb.bncb.ncb_num = lsn;
  Ncb -> basic_ncb.bncb.ncb_buffer_address = message;
  Ncb -> basic_ncb.bncb.ncb_length = length;

  return NCBSubmitWait(lana, Ncb, wait);
}

USHORT NCBSendNoAck(PNCB Ncb, USHORT lana, USHORT lsn,
		    PBYTE message, word length, BOOL wait)
{
  memset(Ncb, 0, NCBSIZE);

  Ncb -> basic_ncb.bncb.ncb_command = wait ? NB_SEND_NO_ACK_WAIT : NB_SEND_NO_ACK;
  Ncb -> basic_ncb.bncb.ncb_lana_num = lana;
  Ncb -> basic_ncb.bncb.ncb_lsn = lsn;
  Ncb -> basic_ncb.bncb.ncb_buffer_address = message;
  Ncb -> basic_ncb.bncb.ncb_length = length;

  return NCBSubmitWait(lana, Ncb, wait);
}

USHORT NCBChainSend(PNCB Ncb, USHORT lana, USHORT lsn,
		    PBYTE message, word length,
		    PBYTE Buffer2, word Length2, BOOL wait)
{
  PBuf2 b2;

  memset(Ncb, 0, NCBSIZE);

  Ncb -> basic_ncb.bncb.ncb_command = wait ? NB_CHAIN_SEND_WAIT : NB_CHAIN_SEND;
  Ncb -> basic_ncb.bncb.ncb_lana_num = lana;
  Ncb -> basic_ncb.bncb.ncb_lsn = lsn;
  Ncb -> basic_ncb.bncb.ncb_buffer_address = message;
  Ncb -> basic_ncb.bncb.ncb_length = length;

  b2 = (PBuf2) &Ncb -> basic_ncb.bncb.ncb_callname;
  b2 -> Length = Length2;
  b2 -> Buffer = Buffer2;

  return NCBSubmitWait(lana, Ncb, wait);
}

USHORT NCBChainSendNoAck(PNCB Ncb, USHORT lana, USHORT lsn,
			 PBYTE message, word length,
			 PBYTE Buffer2, word Length2, BOOL wait)
{
  PBuf2 b2;

  memset(Ncb, 0, NCBSIZE);

  Ncb -> basic_ncb.bncb.ncb_command = wait ? NB_CHAIN_SEND_NO_ACK_WAIT : NB_CHAIN_SEND_NO_ACK;
  Ncb -> basic_ncb.bncb.ncb_lana_num = lana;
  Ncb -> basic_ncb.bncb.ncb_lsn = lsn;
  Ncb -> basic_ncb.bncb.ncb_buffer_address = message;
  Ncb -> basic_ncb.bncb.ncb_length = length;

  b2 = (PBuf2) &Ncb -> basic_ncb.bncb.ncb_callname;
  b2 -> Length = Length2;
  b2 -> Buffer = Buffer2;

  return NCBSubmitWait(lana, Ncb, wait);
}

USHORT NCBReceive(PNCB Ncb, USHORT lana, USHORT lsn,
		  PBYTE buffer, word length, BOOL wait)
{
  memset(Ncb, 0, NCBSIZE);

  Ncb -> basic_ncb.bncb.ncb_command = wait ? NB_RECEIVE_WAIT : NB_RECEIVE;
  Ncb -> basic_ncb.bncb.ncb_lana_num = lana;
  Ncb -> basic_ncb.bncb.ncb_lsn = lsn;
  Ncb -> basic_ncb.bncb.ncb_buffer_address = buffer;
  Ncb -> basic_ncb.bncb.ncb_length = length;

  return NCBSubmitWait(lana, Ncb, wait);
}

USHORT NCBReceiveAny(PNCB Ncb, USHORT lana, USHORT lsn,
		     PBYTE buffer, word length, BOOL wait)
{
  memset(Ncb, 0, NCBSIZE);

  Ncb -> basic_ncb.bncb.ncb_command = wait ? NB_RECEIVE_ANY_WAIT : NB_RECEIVE_ANY;
  Ncb -> basic_ncb.bncb.ncb_lana_num = lana;
  Ncb -> basic_ncb.bncb.ncb_num = lsn;
  Ncb -> basic_ncb.bncb.ncb_buffer_address = buffer;
  Ncb -> basic_ncb.bncb.ncb_length = length;

  return NCBSubmitWait(lana, Ncb, wait);
}

USHORT NCBReceiveDatagram(PNCB Ncb, USHORT lana, USHORT lsn,
			  PBYTE buffer, word length, BOOL wait)
{
  memset(Ncb, 0, NCBSIZE);

  Ncb -> basic_ncb.bncb.ncb_command = wait ? NB_RECEIVE_DATAGRAM_WAIT : NB_RECEIVE_DATAGRAM;
  Ncb -> basic_ncb.bncb.ncb_lana_num = lana;
  Ncb -> basic_ncb.bncb.ncb_num = lsn;
  Ncb -> basic_ncb.bncb.ncb_buffer_address = buffer;
  Ncb -> basic_ncb.bncb.ncb_length = length;

  return NCBSubmitWait(lana, Ncb, wait);
}

USHORT NCBReceiveBroadcast(PNCB Ncb, USHORT lana, USHORT lsn,
			   PBYTE buffer, word length, BOOL wait)
{
  memset(Ncb, 0, NCBSIZE);

  Ncb -> basic_ncb.bncb.ncb_command = wait ? NB_RECEIVE_BROADCAST_DATAGRAM_W : NB_RECEIVE_BROADCAST_DATAGRAM;
  Ncb -> basic_ncb.bncb.ncb_lana_num = lana;
  Ncb -> basic_ncb.bncb.ncb_num = lsn;
  Ncb -> basic_ncb.bncb.ncb_buffer_address = buffer;
  Ncb -> basic_ncb.bncb.ncb_length = length;

  return NCBSubmitWait(lana, Ncb, wait);
}

USHORT NCBHangup(PNCB Ncb, USHORT lana, USHORT lsn)
{
  memset(Ncb, 0, NCBSIZE);

  Ncb -> basic_ncb.bncb.ncb_command = NB_HANG_UP_WAIT;
  Ncb -> basic_ncb.bncb.ncb_lana_num = lana;
  Ncb -> basic_ncb.bncb.ncb_lsn = lsn;

  NCBSubmit(lana, Ncb);

  return (Ncb -> basic_ncb.bncb.ncb_retcode);
}

USHORT NCBCancel(PNCB Ncb, USHORT lana, PNCB NcbToCancel)
{
  memset(Ncb, 0, NCBSIZE);

  Ncb -> basic_ncb.bncb.ncb_command = NB_CANCEL_WAIT;
  Ncb -> basic_ncb.bncb.ncb_lana_num = lana;
  Ncb -> basic_ncb.bncb.ncb_buffer_address = (address) NcbToCancel;

  NCBSubmit(lana, Ncb);

  return (Ncb -> basic_ncb.bncb.ncb_retcode);
}

#endif /* !UNIX && !WATT32 */

/* end of netbios.c */
