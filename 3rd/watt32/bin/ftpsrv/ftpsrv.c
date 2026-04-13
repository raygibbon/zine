/*
	FTP Server for WatTCP-32
	Written by L. De Cock, L. Van Deuren
	email: Luc.DeCock@planetinternet.be
	(C) 1998, 2000 L. De Cock
	This is free software. Use at your own risk

	This FTP server implements the base FTP command set.
	Passive mode is not supported yet.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <stdarg.h>
#include <ctype.h>
#include <dos.h>
#include <io.h>
#include <sys/stat.h>
#include <tcp.h>
#include <dir.h>

#if defined(__DJGPP__)
  #include <unistd.h>
  #define mkdir(path) mkdir(path,0700)
#endif

#include "ftpsrv.h"

#define SOCK_FLUSH(s) ((void)0) /* sock_flush() makes things slower */

tcp_Socket communicationSock, *srv = &communicationSock;
tcp_Socket dataSock, *cli = &dataSock;
void *cli_buf;
const char *strMon[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
int usrCmd;
int loggedIn;
int dataConn;
union {
	unsigned char ip[4];
	unsigned long ina;
} dataIP;

unsigned dataPort;
char user[31];
char password[9];
char renamefrom[128];
char sbuf[2048];
char smsg[256];
char cbuf[8*1024];

int  joke_syst = 1;

const char *joke_syst_str[] = {   /* :-) */
           "Atari ST",
           "Commodore C/64",
           "AIM 65",
           "PET-20",
           "Altair",
           "Sinclair ZX80",
           "Texas TI-95 (yeah really!)",
           "\"too old to remember\""
         };

const char *MsgSystem (int ready)
{
  static int idx = 0;
  static char buf[100];
  const char *serv = "ALTEST FTP";

  if (joke_syst)
  {
    serv = joke_syst_str[idx];
    if (++idx >= sizeof(joke_syst_str)/sizeof(joke_syst_str[0]))
       idx = 0;
  }
  sprintf (buf, "200 %s server", serv);
  if (ready)
     strcat (buf," ready");
  return (buf);
}

void Turn(char clean)
{
	static char st_ch[] = { '/','-','\\','|' };
	static int st = 0;

	if (clean) {
		st = 0;
                printf("\b%c",clean);
                fflush(stdout);
		return;
	}
        printf("\b%c",st_ch[st]);
        fflush(stdout);
	if (++st > 3) {
		st = 0;
	}
}

void FTPMessage(const char *msg, ...)
{
	va_list arg;

	va_start(arg,msg);
	vsprintf(smsg, msg, arg);
	va_end(arg);
	strcat(smsg,"\r\n");
	tcp_tick(NULL);
	sock_fastwrite(srv,smsg,strlen(smsg));
	tcp_tick(NULL);
}

int MakeDataConnection(void)
{
        int res = 0;
	int retry = 3;

        cli_buf = NULL;
	dataConn = 0;
	while (retry--) {
		tcp_tick(NULL);
		res = tcp_open(cli,20,dataIP.ina,dataPort,NULL);
		if (res)  break;
		if (kbhit()) getch();
	}
	if (!res) {
		return 0;
	}
        cli_buf = calloc (16*1024,1);
        sock_setbuf (cli, cli_buf, 16*1024);

	while (!sock_established(cli)) {
		tcp_tick (NULL);
		if (!tcp_tick(cli)) {
			return 0;
		}
		if (kbhit()) getch();
	}
	dataConn = 1;
        sock_mode (cli, /* TCP_MODE_NONAGLE | */ SOCK_MODE_BINARY);

	return 1;
}

void FreeClientBuf (void)
{
  if (cli_buf)
     free (cli_buf);
  cli_buf = NULL;
}

void StripCmd(char *cmd)
{
	int len;

	len = strlen(cmd)-1;
	while ((len >= 0) && ((cmd[len] == '\n') || (cmd[len] == '\r'))) {
		cmd[len--] = 0;
	}
}

void SwitchSlashesToBackSlashes(char *newdir)
{
	int idx, len = strlen(newdir);

	for (idx = 0; idx < len; idx++) {
		if (newdir[idx] == '/') {
			newdir[idx] = '\\';
		}
	}
}

void SwitchBackSlashesToSlashes(char *newdir)
{
	int idx, len = strlen(newdir);

	for (idx = 0; idx < len; idx++) {
		if (newdir[idx] == '\\') {
			newdir[idx] = '/';
		}
	}
}

int CheckCommandAndReturnArg(const char *which, const char *cmd, char **arg)
{
	if (!*cmd) {
		/* no command given */
		*arg = NULL;
		return 0;
	}
	while (*cmd && isalpha(*cmd)) {

		if (*which != toupper(*cmd)) {
			/* it's not this command */
			return 0;
		}
		cmd++;
		which++;
	}
	while (*cmd && ((*cmd == ' ') || (*cmd == '\t'))) {
		cmd++;
	}

        *arg = (char*) cmd;
	return 1;
}

int CheckForAbort(void)
{
	int len;

	if (!tcp_tick(srv)) {
		/* connection closed by other side */
		return 1;
	}
	if (sock_dataready(srv)) {
		len = sock_fastread (srv, sbuf, 512);
		sbuf[len] = 0;
		if (len > 0) {
			if (!sbuf[0] || sbuf[0] == 0) {
				return 1;
			}
			if (!strncmp(sbuf,"QUIT",4) || !strncmp(sbuf,"ABOR",4)) {
				return 1;
			}
		}
	}
	return 0;
}

int CheckUser(const char *u)
{
	*user = 0;
	*password = 0;
	strncpy(user,u,30);
	return 1;
}

int CheckPassword(const char *pass)
{
	*password = 0;
	/*
	   All users and any password is accepted.
	   Up to you to implements your needs.
	   return a value 1 if password accepted or 0 if not
	*/
	return 1;
}

void ChangeDir(char *newdir)
{
	int len = strlen(newdir);
        char *dd = newdir;

	/* Does it include a drive letter? */
	if ((len > 2) && isalpha(*newdir) && (newdir[1] == ':')) {
		setdisk(toupper(*newdir)-'A');
		newdir += 2;
		len -= 2;
	}
	SwitchSlashesToBackSlashes(newdir);
	if (chdir(newdir)) {
		FTPMessage(msgDeleNotExists, dd);
	} else {
		SwitchBackSlashesToSlashes(dd);
		FTPMessage(msgCWDSuccess, dd);
	}
}

void ConfigDataSocket(const char *data)
{
	int nr;
	unsigned pHi, pLo;
	unsigned ip[4];

	nr = sscanf(data,"%u,%u,%u,%u,%u,%u", &(ip[0]), &(ip[1]),
			&(ip[2]), &(ip[3]), &pHi, &pLo);
	if (nr != 6) {
		FTPMessage(msgPortFailed);
		return;
	}
	dataIP.ip[0] = (unsigned char) ip[3];
	dataIP.ip[1] = (unsigned char) ip[2];
	dataIP.ip[2] = (unsigned char) ip[1];
	dataIP.ip[3] = (unsigned char) ip[0];
	dataPort = ((unsigned) pHi << 8) | pLo;
	FTPMessage(msgPortSuccess);
}

void SendDirectory(char *path, int longFormat)
{
	struct ffblk entry;
	struct date da;
	unsigned short year;

	if (!path || !*path) {
		path = "*.*";
	}
	SwitchSlashesToBackSlashes(path);
	if (longFormat) {
		getdate(&da);
	}
	FTPMessage(msgDirOpen);
	if (!MakeDataConnection()) {
		FTPMessage(msgDirError);
		return;
	}
        if (findfirst(path,&entry, _A_NORMAL|_A_ARCH|_A_SUBDIR|_A_RDONLY)) {
                SOCK_FLUSH(cli);
		sock_close(cli);
		tcp_tick(NULL);
		FTPMessage(msgDirFailed, path);
		return;
	}
	do {
		tcp_tick (NULL);
                if (kbhit())
                   getch();
		if (entry.ff_attrib & (FA_LABEL | FA_HIDDEN)) {
			/* skip volume label and hidden files */
			continue;
		}
		if (longFormat) {
			year = (entry.ff_fdate >> 9)+1980;
			if (year == da.da_year) {
				sprintf(cbuf,"-rw-rw-rw-   1 user     ftp  %11ld %s %2.2d %2.2d:%2.2d %s\r\n",
					entry.ff_fsize, strMon[((entry.ff_fdate >> 5) & 0xF)-1],
					entry.ff_fdate & 0x1F, entry.ff_ftime >> 11,
					(entry.ff_ftime >> 5) & 0x3F, entry.ff_name);
			} else {
				sprintf(cbuf,"-rw-rw-rw-   1 user     ftp  %11ld %s %2.2d %5d %s\r\n",
					entry.ff_fsize, strMon[((entry.ff_fdate >> 5) & 0xF)-1],
					entry.ff_fdate & 0x1F, year, entry.ff_name);
			}
			if (entry.ff_attrib & FA_DIREC) {
				cbuf[0] = 'd';
			}
			if (entry.ff_attrib & FA_RDONLY) {
				cbuf[2] = '-';
				cbuf[5] = '-';
				cbuf[8] = '-';
			}
		} else {
			sprintf(cbuf,"%s\r\n", entry.ff_name);
		}
                sock_write(cli,cbuf,strlen(cbuf));  // !! was sock_fastwrite
		if (!tcp_tick(cli)) {
			FTPMessage(msgDirError);
			return;
		}
		if (CheckForAbort()) {
			/* transfer should abort */
                        SOCK_FLUSH(cli);
			sock_close(cli);
			tcp_tick(NULL);
			FTPMessage(msgDirError);
			return;
		}
	} while (!findnext(&entry));

        FTPMessage(msgDirOk);
        SOCK_FLUSH(cli);
	sock_close(cli);
	tcp_tick(cli);
}

void SendFile(char *filename)
{
	FILE *f;
	long flen, processed;
	int len;
	int sent, toSend;
	int res;

	SwitchSlashesToBackSlashes(filename);
        if ((f = fopen(filename,"rb")) == NULL) {
		FTPMessage(msgRetrFailed, filename);
		return;
	}

        FTPMessage(msgRetrSuccess, filename, filelength(fileno(f)));
	if (!MakeDataConnection()) {
		fclose(f);
		FTPMessage(msgRetrAborted, filename);
		return;
	}
	flen = filelength(fileno(f));
	processed = 0;
        printf(" ");
        fflush (stdout);

        
        while (processed < flen) {
		Turn(0);
		tcp_tick (NULL);
                if (kbhit())
                   getch();
                len = fread(cbuf,1,sizeof(cbuf), f);
		if (len < 0) {
			fclose(f);
                        SOCK_FLUSH(cli);
			sock_close(cli);
			tcp_tick(NULL);
			FTPMessage(msgRetrFailed, filename);
			Turn('e');
			return;
		}
		toSend = len;
		sent = 0;
		while (sent < len) {
                        if (kbhit())
                           getch();
                        res = sock_write(cli,&cbuf[sent],toSend);
			sent += res;
			toSend -= res;
#if 1
                        if (!tcp_tick(cli)) {
				fclose(f);
				FTPMessage(msgRetrFailed, filename);
				Turn('f');
				return;
			}
#endif
		}
		processed += len;
		if (CheckForAbort()) {
			/* transfer should abort */
			fclose(f);
                        SOCK_FLUSH(cli);
			sock_close(cli);
			tcp_tick(NULL);
			FTPMessage(msgRetrAborted, filename);
			Turn('a');
			return;
		}
	}
	Turn('+');
	fclose(f);
	FTPMessage(msgRetrOk);
        SOCK_FLUSH(cli);
	sock_close(cli);
	tcp_tick(cli);
}

void ReceiveFile(char *filename)
{
	FILE *f;
	int len;

	SwitchSlashesToBackSlashes(filename);
        if ((f = fopen(filename,"wb")) == NULL) {
		FTPMessage(msgStorFailed, filename);
		return;
	}
	FTPMessage(msgStorSuccess, filename);
	if (!MakeDataConnection()) {
		fclose(f);
		unlink(filename);
		FTPMessage(msgStorAborted, filename);
		return;
	}

        printf(" ");
        fflush(stdout);
	do {
		Turn(0);
		tcp_tick (NULL);
		if (kbhit()) getch();
		if (sock_dataready(cli)) {
                        len = sock_fastread(cli,cbuf,sizeof(cbuf));
			fwrite(cbuf,1,len,f);
		}
		if (CheckForAbort()) {
			/* transfer should abort */
                        SOCK_FLUSH(cli);
			sock_close(cli);
			tcp_tick(NULL);
			FTPMessage(msgRetrAborted, filename);
			Turn('a');
			fclose(f);
			return;
		}
		if (!tcp_tick(cli)) {
			fclose(f);
			FTPMessage(msgStorOk);
			Turn('+');
			return;
		}
	} while(1);
}

void DeleteFile(char *filename)
{
	if (!filename || !*filename) {
		/* no param */
		FTPMessage(msgDeleSyntax);
		return;
	}
	SwitchSlashesToBackSlashes(filename);
	if (access(filename,0)) {
		FTPMessage(msgDeleNotExists,filename);
	} else {
		if (unlink(filename)) {
			FTPMessage(msgDeleFailed, filename);
		} else {
			FTPMessage(msgDeleOk, filename);
		}
	}
}

void RemoveDirectory(char *thedir)
{
	if (!thedir || !*thedir) {
		/* no param */
		FTPMessage(msgRmdSyntax);
		return;
	}
	SwitchSlashesToBackSlashes(thedir);
	if (access(thedir,0)) {
		FTPMessage(msgRmdNotExists,thedir);
	} else {
		if (rmdir(thedir)) {
			FTPMessage(msgRmdFailed, thedir);
		} else {
			FTPMessage(msgRmdOk, thedir);
		}
	}
}

void MakeDirectory(char *thedir)
{
	if (!thedir || !*thedir) {
		/* no param */
		FTPMessage(msgMkdSyntax);
		return;
	}
	SwitchSlashesToBackSlashes(thedir);
	if (!access(thedir,0)) {
		FTPMessage(msgMkdAlready,thedir);
	} else {
                if (mkdir(thedir)) {
			FTPMessage(msgMkdFailed, thedir);
		} else {
			FTPMessage(msgMkdOk, thedir);
		}
	}
}

void RenameFrom(char *thename)
{
	if (!thename || !*thename) {
		/* no param */
		FTPMessage(msgRnfrSyntax);
		return;
	}
	SwitchSlashesToBackSlashes(thename);
	if (access(thename,0)) {
		FTPMessage(msgRnfrNotExists,thename);
	} else {
		strcpy(renamefrom,thename);
		FTPMessage(msgRnfrOk);
	}
}

void RenameTo(char *thename)
{
	if (!thename || !*thename) {
		/* no param */
		FTPMessage(msgRntoSyntax);
		return;
	}
	SwitchSlashesToBackSlashes(thename);
	if (!access(thename,0)) {
		FTPMessage(msgRntoAlready,thename);
	} else {
		if (rename(renamefrom,thename)) {
			FTPMessage(msgRntoFailed,renamefrom);
		}
		else {
			FTPMessage(msgRntoOk,renamefrom,thename);
		}
	}
}

void ServeClient(void)
{
	int len;
	int execCmd;
	char *arg;

        sock_mode(srv, /* TCP_MODE_NONAGLE | */ SOCK_MODE_ASCII);

	len = 0;	/* length of received command */
	execCmd = 0;	/* true if command received */
	usrCmd = 0;	/* true if USER command received (needed for PASS) */
	loggedIn = 0;	/* true if a known user is logged in */
	dataPort = 20;	/* default data connection on port 20 */
	dataIP.ina = 0;
	dataConn = 0;	/* no data connection open */
	*renamefrom = 0;
        sock_setbuf (srv, malloc(16*1024), 16*1024);

        FTPMessage (MsgSystem(0));
	while (tcp_tick(srv)) {
		tcp_tick (NULL);
		if (kbhit()) getch();	/* allow CTR-C */
		execCmd = 0;
		if (sock_dataready(srv)) {
			len = sock_gets (srv, sbuf, 512);
			sbuf[len] = 0;
			if (len > 0) {
				if (!sbuf[0]) {
					/* Close forced */
					sock_close(srv);
					sock_close(cli);
					tcp_tick(NULL);
					continue;
				}
			}
			if (len > 2) {
				/* a command is a least 3 chars long */
				execCmd = 1;
			}
			StripCmd(sbuf);
		}
		if (!execCmd) {
			continue;
		}
		/* check out which command the client sent */

		printf("."); fflush(stdout);

		if (CheckCommandAndReturnArg("USER",sbuf, &arg)) {
			if (CheckUser(arg)) {
				/* user is known */
				FTPMessage(msgPassRequired,user);
				usrCmd = 1;
			} else {
				usrCmd = 0;
			}
			continue;
		}
		if (CheckCommandAndReturnArg("PASS",sbuf,&arg) && usrCmd) {
			if (CheckPassword(arg)) {
				loggedIn = 1;
				FTPMessage(msgLogged,user);
			} else {
				FTPMessage(msgLoginFailed);
				loggedIn = 0;
				usrCmd = 0;
			}
			continue;
		}
		if (CheckCommandAndReturnArg("QUIT",sbuf,&arg)) {
			FTPMessage(msgQuit);
			sock_close(srv);
			tcp_tick(NULL);
			break;
		}
                if (CheckCommandAndReturnArg("SYST",sbuf,&arg)) {
                        FTPMessage(MsgSystem(1));
			continue;
		}

		if (CheckCommandAndReturnArg("PORT",sbuf, &arg)) {
			ConfigDataSocket(arg);
			continue;
		}
		if (!loggedIn) {
			FTPMessage(msgNotLogged);
			continue;
		}
		if (CheckCommandAndReturnArg("CWD",sbuf, &arg)) {
			ChangeDir(arg);
			continue;
		}
		if (CheckCommandAndReturnArg("TYPE",sbuf, &arg)) {
			FTPMessage(msgTypeOk, arg);
			continue;
		}
		if (CheckCommandAndReturnArg("NLST",sbuf, &arg)) {
			SendDirectory(arg, 0);
			continue;
		}
		if (CheckCommandAndReturnArg("LIST",sbuf, &arg)) {
			SendDirectory(arg, 1);
			continue;
		}
		if (CheckCommandAndReturnArg("PWD",sbuf, &arg) ||
		    CheckCommandAndReturnArg("XPWD",sbuf, &arg)) {
			getcwd(sbuf,128);
			SwitchBackSlashesToSlashes(sbuf);
			FTPMessage(msgPWDSuccess, sbuf);
			continue;
		}
		if (CheckCommandAndReturnArg("RETR",sbuf, &arg)) {
			SendFile(arg);
			continue;
		}
		if (CheckCommandAndReturnArg("STOR",sbuf, &arg)) {
			ReceiveFile(arg);
			continue;
		}
		if (CheckCommandAndReturnArg("DELE",sbuf, &arg)) {
			DeleteFile(arg);
			continue;
		}
		if (CheckCommandAndReturnArg("RMD",sbuf, &arg) ||
		    CheckCommandAndReturnArg("XRMD",sbuf, &arg)) {
				RemoveDirectory(arg);
				continue;
		}
		if (CheckCommandAndReturnArg("MKD",sbuf, &arg) ||
		    CheckCommandAndReturnArg("XMKD",sbuf, &arg)) {
			MakeDirectory(arg);
			continue;
		}
		if (CheckCommandAndReturnArg("RNFR",sbuf, &arg)) {
			RenameFrom(arg);
			continue;
		}
		if (CheckCommandAndReturnArg("RNTO",sbuf, &arg)) {
			RenameTo(arg);
			continue;
		}
		FTPMessage(msgCmdUnknown, sbuf);
	}
	return;
}

int main(int argc,char **argv)
{
	int stop;
	int listening;

	printf("FTP Server for WatTCP32 * Version "VERSION"  "__DATE__" "__TIME__"\n");
        if (argc > 1 && !strcmp(argv[1],"-d"))
           dbug_init();
	sock_init();

	stop = 0;
	listening = 0;
	while (!stop) {
		tcp_tick (NULL);
		if (!listening) {
			tcp_listen(srv, 21, 0, 0, NULL, 0);
			printf("Listening... "); fflush(stdout);
			listening = 1;
		}
		if (kbhit()) {		/* allow CTRL-C */
			if (getch() == 27) {
                                printf("Interrupted.");
                                fflush(stdout);
				stop = 0;
				break;
			}
		}
		if (sock_established(srv)) {
                        printf("\nConnected [");
                        fflush(stdout);
			ServeClient();
			listening = 0;
			printf("] Closed.\n"); fflush(stdout);
		}
	}
	return 0;
}
