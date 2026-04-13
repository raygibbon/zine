/* netbios.h
 * NetBIOS access library
 *
 * Author:  Kai Uwe Rommel <rommel@ars.de>
 * Created: Tue Apr 02 1996
 */
 
/* $Id: netbios.h,v 1.3 1997/08/09 16:38:55 rommel Exp rommel $ */

/*
 * $Log: netbios.h,v $
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

#ifndef _NETBIOS_H
#define _NETBIOS_H

typedef unsigned char byte;
typedef unsigned short int word;
typedef unsigned long int dword;

#ifdef WIN32

#define NCB __NCB
#define PNCB __PNCB
#include <windows.h>
#undef NCB
#undef PNCB

union ncb_types;
typedef HANDLE semaphore;
typedef unsigned char *address;
typedef union ncb_types NCB, *PNCB;

#else /* !WIN32 */

#ifndef OS2DEF_INCLUDED
#include <os2def.h>
#endif

union ncb_types;
typedef unsigned long semaphore;

#ifdef __32BIT__
typedef unsigned char * _Seg16 address;
typedef union ncb_types NCB, *PNCB, * _Seg16 PNCB16;
#else
typedef unsigned char far * address;
typedef union ncb_types NCB, *PNCB, _far * PNCB16;
#endif

#endif /* WIN32 */

#include "netb_1_c.h"		       /* NCB defines      */
#include "netb_2_c.h"		       /* NCB structures   */

union ncb_types
{
  struct fncb
  {
    struct network_control_block bncb;
    semaphore ncb_semaphore;
  }
  basic_ncb;
  struct ncb_chain_send send;
  struct ncb_reset reset;
};
#define NCBSIZE sizeof(union ncb_types)

#define NETBIOS 0
#define NETBEUI 1
extern BOOL NetBIOS_API;

USHORT NetBIOS_Avail(VOID);
       
USHORT NCBConfig(PNCB Ncb, USHORT lana, 
		 PUSHORT sessions, PUSHORT commands, PUSHORT names);
USHORT NCBReset(PNCB Ncb, USHORT lana, 
		USHORT sessions, USHORT commands, USHORT names);
USHORT NCBClose(PNCB Ncb, USHORT lana);
       
USHORT NCBAddGroupName(PNCB Ncb, USHORT lana, PBYTE name);
USHORT NCBAddName(PNCB Ncb, USHORT lana, PBYTE name);
USHORT NCBDeleteName(PNCB Ncb, USHORT lana, PBYTE lclname);
       
USHORT NCBCall(PNCB Ncb, USHORT lana, PBYTE lclname, PBYTE rmtname, 
	       USHORT recv_timeout, USHORT send_timeout, BOOL wait);
USHORT NCBListen(PNCB Ncb, USHORT lana, PBYTE lclname, PBYTE rmtname, 
		 USHORT recv_timeout, USHORT send_timeout, BOOL wait);

USHORT NCBWait(PNCB Ncb, ULONG timeout);

USHORT NCBHangup(PNCB Ncb, USHORT lana, USHORT lsn);
USHORT NCBCancel(PNCB Ncb, USHORT lana, PNCB NcbToCancel);
       
USHORT NCBSend(PNCB Ncb, USHORT lana, USHORT lsn, 
	       PBYTE message, USHORT length, BOOL wait);
USHORT NCBSendBroadcast(PNCB Ncb, USHORT lana, USHORT lsn, 
			PBYTE message, USHORT length, BOOL wait);
USHORT NCBSendDatagram(PNCB Ncb, USHORT lana, USHORT lsn, 
		       PBYTE rmtname, PBYTE message, USHORT length, BOOL wait);
USHORT NCBSendNoAck(PNCB Ncb, USHORT lana, USHORT lsn, 
		    PBYTE message, USHORT length, BOOL wait);
       
USHORT NCBChainSend(PNCB Ncb, USHORT lana, USHORT lsn, 
		    PBYTE message, USHORT length, 
		    PBYTE Buffer2, USHORT Length2, BOOL wait);
USHORT NCBChainSendNoAck(PNCB Ncb, USHORT lana, USHORT lsn, 
			 PBYTE message, USHORT length, 
			 PBYTE Buffer2, USHORT Length2, BOOL wait);

USHORT NCBReceive(PNCB Ncb, USHORT lana, USHORT lsn, 
		  PBYTE buffer, USHORT length, BOOL wait);
USHORT NCBReceiveAny(PNCB Ncb, USHORT lana, USHORT lsn, 
		     PBYTE buffer, USHORT length, BOOL wait);
USHORT NCBReceiveBroadcast(PNCB Ncb, USHORT lana, USHORT lsn, 
			   PBYTE buffer, USHORT length, BOOL wait);
USHORT NCBReceiveDatagram(PNCB Ncb, USHORT lana, USHORT lsn, 
			  PBYTE buffer, USHORT length, BOOL wait);

#endif /* _NETBIOS_H */

/* end of netbios.h */
