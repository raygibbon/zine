/*
	FTP Server for WatTCP-32
	Written by L. De Cock, L. Van Deuren
	email: Luc.DeCock@planetinternet.be
	(C) 1998, 2000 L. De Cock
	This is free software. Use at your own risk

	This FTP server implements the base FTP command set.
	Passive mode is not supported yet.
*/

#ifndef _FTPSRV_H
#define _FTPSRV_H

#define VERSION "1.0.2-03"

#define msgPassRequired   "331 Password required for %s."
#define msgBanner         "220 ALTEST FTP Server ready."
#define msgLoginFailed    "530 Login incorrect."
#define msgLogged         "230 User %s logged in."
#define msgNotLogged      "530 Please login with USER and PASS."
#define msgCmdUnknown     "500 '%s': command not understood."
#define msgCWDSuccess     "250 CWD command successful \"%s\"."
#define msgTypeFailed     "500 'TYPE %s': command not understood."
#define msgTypeOk         "200 Type set to %s."
#define msgPortSuccess    "200 Port command successful."
#define msgPortFailed     "501 Invalid PORT command."
#define msgDirOpen        "150 Opening data connection for directory list."
#define msgDirError       "426 Connection closed; transfer aborted."
#define msgDirFailed      "451 Failed: %s."
#define msgDirOk          "226 Closing data connection."
#define msgPWDSuccess     "257 \"%s\" is current directory."
#define msgRetrDisabled   "500 Cannot RETR."
#define msgRetrSuccess    "150 Opening data connection for %s (%lu bytes)"
#define msgRetrFailed     "501 Cannot RETR. %s"
#define msgRetrAborted    "426 Connection closed; %s."
#define msgRetrOk         "226 File sent successfully."
#define msgStorDisabled   "500 Cannot STOR."
#define msgStorSuccess    "150 Opening data connection for %s."
#define msgStorFailed     "501 Cannot STOR. %s"
#define msgStorAborted    "426 Connection closed; %s."
#define msgStorOk         "226 File received successfully."
#define msgStorError      "426 Connection closed; transfer aborted."
#define msgQuit           "221 Goodbye."
#define msgDeleOk         "250 File '%s' deleted."
#define msgDeleFailed     "450 File '%s' can't be deleted."
#define msgDeleSyntax     "501 Syntax error in parameter."
#define msgDeleNotExists  "550 Error '%s': no such file or directory."
#define msgRmdOk          "250 '%s': directory removed."
#define msgRmdNotExists   "550 '%s': no such directory."
#define msgRmdFailed      "550 '%s': can't remove directory."
#define msgRmdSyntax      "501 Syntax error in parameter."
#define msgMkdOk          "257 '%s': directory created."
#define msgMkdAlready     "550 '%s': file or directory already exists."
#define msgMkdFailed      "550 '%s': can't create directory."
#define msgMkdSyntax      "501 Syntax error in parameter."
#define msgRnfrNotExists  "550 '%s': no such file or directory."
#define msgRnfrSyntax     "501 Syntax error is parameter."
#define msgRnfrOk         "350 File exists, ready for destination name."
#define msgRntoAlready    "553 '%s': file already exists."
#define msgRntoOk         "250 File '%s' renamed to '%s'."
#define msgRntoFailed     "450 File '%s' can't be renamed."
#define msgRntoSyntax     "501 Syntax error in parameter."

#endif
