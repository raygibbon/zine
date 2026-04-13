/*
** syslogd -- system logging daemon
** Copyright (c) 1998 Douglas K. Rand   <rand@aero.und.edu>
** All Rights Reserved.
*/

/*
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; see the file COPYING.  If not, write to
** the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
** Boston, MA 02111-1307, USA.
*/

/* $Id: conf-lex.l,v 1.23 1998/12/03 17:13:12 rand Exp $ */

%{
static char *RCSid __attribute__ ((unused)) = "$Id: conf-lex.l,v 1.23 1998/12/03 17:13:12 rand Exp $";

#include "syslogd.h"
#include "conf-par.h"
#include <stdio.h>
#include <ctype.h>

void yyerror(char *msg);
%}

%option noyywrap

/*
** The definitions of string and comment are fairly stupid.
** Specifically, you can't escape quotes or have a string span
** newlines. Also no context sensing on comments are done (such
** as in strings.
*/
whiteSpace	[ \t]+
newLine		\n
integer		([0-9]+)|(0x[0-9]+)
string		\"[^\"\n]*[\"\n]
comment		\#.*$

%%

{whiteSpace}		;
{newLine}		{ lineNumber++; }

{comment}		;

{string}		{ /* Try to catch unterminated strings */
				char *p = yytext;
				    if(p != NULL && *p != '\0') {
					p++;
					yylval.string = p;
					p += strlen(p) - 1;
					if(*p == '"') *p = '\0';
					else yyerror("unterminated string");
				    }
				    else yyerror("string is NULL or empty");
				return(STRING);
			}
{integer}		{ /* strtol() with a base of 0 does octal, hex, and decimal */
				yylval.integer = strtol(yytext, NULL, 0);
				return(INTEGER);
			}

"acl"			{ return(ACL); }
"options"		{ return(OPTIONS); }

"domain"		{ return(DOMAIN); }
"host"			{ return(HOST); }
"network"		{ return(NETWORK); }

"create-filter"		{ return(CREATE_FILTER); }
"daemonize"		{ return(DAEMONIZE); }
"directory"		{ return(DIRECTORY); }
"hash-table-size"	{ return(HASH_TABLE_SIZE); }
"hash-table-max-size"	{ return(HASH_TABLE_MAX_SIZE); }
"inet-forward-from"	{ return(INET_FORWARD_FROM); }
"inet-forwarding"	{ return(INET_FORWARDING); }
"inet-receive"		{ return(INET_RECEIVE); }
"inet-receive-from"	{ return(INET_RECEIVE_FROM); }
"klog-device"		{ return(KLOG_DEVICE); }
"local-hostname"	{ return(LOCAL_HOSTNAME); }
"log-device"		{ return(LOG_DEVICE); }
"log-device-mode"	{ return(LOG_DEVICE_MODE); }
"log-file-mode"		{ return(LOG_FILE_MODE); }
"log-unwanted-hosts"	{ return(LOG_UNWANTED_HOSTS); }
"log-with-priority"	{ return(LOG_WITH_PRIORITY); }
"mark-interval"		{ return(MARK_INTERVAL); }
"maximum-duplicates"	{ return(MAXIMUM_DUPLICATES); }
"pid-file"		{ return(PID_FILE); }
"repeat-interval"	{ return(REPEAT_INTERVAL); }
"simple-hostname"	{ return(SIMPLE_HOSTNAME); }
"socket-backlog"	{ return(SOCKET_BACKLOG); }
"strip-domains"		{ return(STRIP_DOMAINS); }
"sync-each-message"	{ return(SYNC_EACH_MESSAGE); }
"syslog-port-name"	{ return(SYSLOG_PORT_NAME); }

"facility"		{ return(FACILITY); }
"program"		{ return(PROGRAM); }
"priority"		{ return(PRIORITY); }

"break"			{ return(BREAK); }
"file"			{ return(FILETOK); }
"fifo"			{ return(FIFO); }
"forward"		{ return(FORWARD); }

"with-host-name"	{ return(WITH_HOST_NAME); }
"port"			{ return(PORT); }

"auth"|"authpriv"|"bsdcron"|"sysvcron"|"daemon"|"kern"|"local"[0-7]|"lpr"|"mail"|"news"|"ntp"|"user"|"uucp"|"syslog" {
		yylval.name = LookupFacility(yytext);
		return(FACILITY_NAME);
	}

"emerg"|"alert"|"crit"|"err"|"warning"|"notice"|"info"|"debug" {
		yylval.name = LookupPriority(yytext);
		return(PRIORITY_NAME);
	}

"all"			{ yylval.name = ALL; return(ALL); }
"none"			{ yylval.name = NONE; return(NONE); }

"numeric"		{ yylval.name = NUMERIC; return(NUMERIC); }
"symbolic"		{ yylval.name = SYMBOLIC; return(SYMBOLIC); }

"{"			{ return(LBRACE); }
"}"			{ return(RBRACE); }
";"			{ return(SEMI); }
","			{ return(COMMA); }
"."			{ return(DOT); }
"/"			{ return(SLASH); }

"<="			{ yylval.comparison = le; return(COMPARISON); }
">="			{ yylval.comparison = ge; return(COMPARISON); }
"=="			{ yylval.comparison = eq; return(COMPARISON); }
"!="			{ yylval.comparison = ne; return(COMPARISON); }

"true"|"on"		{ yylval.boolean = true; return(BOOLEAN); }
"false"|"off"		{ yylval.boolean = false; return(BOOLEAN); }
