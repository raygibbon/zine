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

/* $Id: conf-parse.y,v 1.28 1998/12/01 03:37:50 rand Exp $ */

%{
static char *RCSid __attribute__ ((unused)) = "$Id: conf-parse.y,v 1.28 1998/12/01 03:37:50 rand Exp $";

#include "syslogd.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>

int yyerror(const char *msg); 
 
%}

%union {
    int integer;
    enum comparisons comparison;
    enum logWithPriorities logWithPriority;
    bool boolean;
    char *string;
    int name;
    int facilityBitmap;
    struct aclEntry *acl;
    struct aclList *acls;
    struct action *action;
    struct priority *priority;
    struct facility *facility;
    struct program *program;
    struct programNames *programNames;
}

%token DOMAIN HOST NETWORK

%token LBRACE RBRACE SEMI COMMA DOT SLASH

%token CREATE_FILTER
%token DAEMONIZE
%token DIRECTORY
%token HASH_TABLE_SIZE
%token HASH_TABLE_MAX_SIZE
%token INET_FORWARDING
%token INET_FORWARD_FROM
%token INET_RECEIVE
%token INET_RECEIVE_FROM
%token KLOG_DEVICE
%token LOCAL_HOSTNAME
%token LOG_DEVICE
%token LOG_DEVICE_MODE
%token LOG_FILE_MODE
%token LOG_UNWANTED_HOSTS
%token LOG_WITH_PRIORITY
%token MARK_INTERVAL
%token MAXIMUM_DUPLICATES
%token PID_FILE
%token REPEAT_INTERVAL
%token SIMPLE_HOSTNAME
%token SOCKET_BACKLOG
%token STRIP_DOMAINS
%token SYNC_EACH_MESSAGE
%token SYSLOG_PORT_NAME

%token WITH_HOST_NAME

%token FACILITY PROGRAM PRIORITY ACL OPTIONS

%token BREAK FILETOK FIFO FORWARD PORT

%token <name> FACILITY_NAME
%token <name> PRIORITY_NAME
%token <name> ALL NONE
%token <name> NUMERIC SYMBOLIC

%token <comparison> COMPARISON
%token LE GE EQ NE

%token <boolean> BOOLEAN

%token <string> STRING
%token <integer> INTEGER

%type <facilityBitmap> facilityNames facilityName

%type <priority> programStatements programStatement priority

%type <program> facilityStatements facilityStatement program

%type <action> actions action

%type <logWithPriority> logWithPriority

%type <programNames> programNames programName

%type <acl> acl
%type <acls> acls

%%

confFile: aclSection optionSection facilitySection ;

aclSection: /* empty */
	  | aclDeclaration
	  | aclSection aclDeclaration
	  ;

optionSection: /* empty */
	     | optionDeclaration
	     | optionSection optionDeclaration
	     ;

facilitySection: /* empty */
	       | facilityDeclaration
	       | facilitySection facilityDeclaration
	       ;

aclDeclaration: ACL STRING LBRACE acls RBRACE SEMI	{ MakeACL($2, $4); } ;

acls: /* empty */	{ $$ = NULL; }
    | acl		{ $$ = AppendACL(NULL, $1); }
    | acls acl		{ $$ = AppendACL($1, $2); }
    ;

acl: HOST STRING SEMI		{ $$ = CreateHostACL($2); }
   | DOMAIN STRING SEMI		{ $$ = CreateDomainACL($2); }
   | NETWORK INTEGER DOT INTEGER DOT INTEGER DOT INTEGER SLASH INTEGER SEMI
				{ $$ = CreateNetworkACL($2, $4, $6, $8, $10); }
   ;

optionDeclaration: OPTIONS LBRACE options RBRACE SEMI ;

options: /* empty */
       | option
       | options option
       ;

option: globalOptions
      ;

globalOptions: DAEMONIZE BOOLEAN SEMI			{ daemonize = $2; }
	     | DIRECTORY STRING SEMI			{ SetDirectory($2); }
	     | HASH_TABLE_SIZE INTEGER SEMI		{ hashTableSize = $2; }
	     | HASH_TABLE_MAX_SIZE INTEGER SEMI		{ hashTableMaxSize = $2; }
	     | INET_FORWARDING BOOLEAN SEMI		{ inetForwarding = $2; }
	     | INET_FORWARD_FROM NONE SEMI		{ SetInetForwardFrom(NULL); }
	     | INET_FORWARD_FROM STRING SEMI		{ SetInetForwardFrom($2); }
	     | INET_RECEIVE BOOLEAN SEMI		{ inetReceive = $2; }
	     | INET_RECEIVE_FROM NONE SEMI		{ SetInetReceiveFrom(NULL); }
	     | INET_RECEIVE_FROM STRING SEMI		{ SetInetReceiveFrom($2); }
	     | KLOG_DEVICE STRING SEMI			{ klogDevice = $2; }
	     | LOCAL_HOSTNAME STRING SEMI		{ hostName = strdup($2); }
	     | LOG_DEVICE STRING SEMI			{ logDevice = $2; }
	     | LOG_DEVICE_MODE INTEGER SEMI		{ logDeviceMode = $2; }
	     | LOG_FILE_MODE INTEGER SEMI		{ logFileMode = $2; }
	     | LOG_UNWANTED_HOSTS BOOLEAN SEMI		{ logUnwantedHosts = $2; }
	     | LOG_WITH_PRIORITY logWithPriority SEMI	{ logWithPriority = $2; }
	     | MARK_INTERVAL INTEGER SEMI		{ markInterval = $2; }
	     | MARK_INTERVAL STRING SEMI		{ SetMarkInterval($2); }
	     | MAXIMUM_DUPLICATES INTEGER SEMI		{ maxDuplicates = $2; }
	     | PID_FILE STRING SEMI			{ pidFile = $2; }
	     | REPEAT_INTERVAL INTEGER SEMI		{ repeatInterval = $2; }
	     | REPEAT_INTERVAL STRING SEMI		{ SetRepeatInterval($2); }
	     | SIMPLE_HOSTNAME NONE SEMI		{ SetSimpleHostnames(NULL); }
	     | SIMPLE_HOSTNAME STRING SEMI		{ SetSimpleHostnames($2); }
	     | SOCKET_BACKLOG INTEGER SEMI		{ socketBacklog = $2; }
	     | STRIP_DOMAINS NONE SEMI			{ SetStripDomains(NULL); }
	     | STRIP_DOMAINS STRING SEMI		{ SetStripDomains($2); }
	     | SYNC_EACH_MESSAGE BOOLEAN SEMI		{ syncEachMessage = $2; }
	     | SYSLOG_PORT_NAME STRING SEMI		{ SetSyslogPort($2); }
	     ;

facilityDeclaration: FACILITY facilityNames LBRACE facilityStatements RBRACE SEMI
			{ MakeFacility($2, $4); }
		   ;

facilityStatements: /* empty */					{ $$ = NULL; }
		  | facilityStatement				{ $$ = $1; }
		  | facilityStatements facilityStatement	{ $$ = AppendProgram($1, $2); }
		  ;

facilityStatement: actions		{ $$ = MakeProgram(MakeProgramName(NULL),
							   MakePriority(ge, LOG_DEBUG, MakeActions($1))); }
		 | priority		{ $$ = MakeProgram(MakeProgramName(NULL), $1); }
		 | program		{ $$ = $1; }
		 ;

program: PROGRAM programNames LBRACE programStatements RBRACE SEMI	{ $$ = MakeProgram($2, $4); }
       ;

programStatements: /* empty */				{ $$ = NULL; }
		 | programStatement			{ $$ = $1; }
		 | programStatements programStatement	{ $$ = AppendPriority($1, $2); }
		 ;

programStatement: actions		{ $$ = MakePriority(ge, LOG_DEBUG, MakeActions($1)); }
		| priority		{ $$ = $1; }
		;

programNames: programName			{ $$ = $1; }
	    | programName COMMA programNames	{ $$ = AppendProgramName($1, $3); }

programName: STRING			{ $$ = MakeProgramName($1); }
	   ;

priority: PRIORITY COMPARISON PRIORITY_NAME LBRACE actions RBRACE SEMI
			{ $$ = MakePriority($2, $3, MakeActions($5)); }
	;

actions: /* empty */		{ $$ = NULL; }
       | action			{ $$ = $1; }
       | actions action		{ $$ = AppendAction($1, $2); }
       ;

/* Notice that we re-init the options first. */
action: BREAK SEMI				{ InitActionOptions(); $$ = MakeAction(actionBreak, ""); }
      | FILETOK STRING SEMI			{ InitActionOptions(); $$ = MakeAction(actionFile, $2); }
      | FIFO STRING SEMI			{ InitActionOptions(); $$ = MakeAction(actionFIFO, $2); }
      | FORWARD STRING SEMI			{ InitActionOptions(); $$ = MakeAction(actionForward, $2); }
      | FILETOK STRING { InitActionOptions(); } LBRACE fileOptions RBRACE SEMI
			{ $$ = MakeAction(actionFile, $2); }
      | FIFO STRING { InitActionOptions(); } LBRACE fileOptions RBRACE SEMI
			{ $$ = MakeAction(actionFIFO, $2); }
      | FORWARD STRING { InitActionOptions(); } LBRACE forwardOptions RBRACE SEMI
			{ $$ = MakeAction(actionForward, $2); }
      ;

actionOption: LOG_WITH_PRIORITY logWithPriority SEMI	{ actionOptions.logWithPriority = $2; }

fileOptions: /* empty */
	   | fileOption
	   | fileOptions fileOption
	   ;

fileOption: SYNC_EACH_MESSAGE BOOLEAN SEMI	{ actionOptions.syncEachMessage = $2; }
	  ;

forwardOptions: /* empty */
	      | forwardOption
	      | forwardOptions forwardOption
	      ;

forwardOption: actionOption
	     | PORT STRING SEMI			{ actionOptions.port = GetPortByName($2); }
	     | PORT INTEGER SEMI		{ actionOptions.port = htons($2); }
	     | WITH_HOST_NAME BOOLEAN SEMI	{ actionOptions.forwardWithHostname = $2; }
	     ;

facilityNames: facilityName				{ $$ = $1; }
	     | facilityNames COMMA facilityName		{ $$ = $1 | $3; }

facilityName: FACILITY_NAME	{ $$ = facilityNames[$1].bitValue; }
	    | ALL		{ $$ = 0xffffffff; }
	    ;

logWithPriority: NUMERIC	{ $$ = numeric; }
	       | SYMBOLIC	{ $$ = symbolic; }
	       | NONE		{ $$ = none; }
	       ;

%%

int yyerror(const char *msg)
{
    fprintf(stderr, "%s: %d: %s\n", configFile, lineNumber, msg);
    return(1);
}
