/*
 *  Boa, an http server
 *  Copyright (C) 1995 Paul Phillips <psp@well.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 1, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "boa.h"

int   server_port;
char *server_admin;
char *server_root;
char *server_name;

char *document_root;
char *user_dir;
char *directory_index;
char *default_type;
char *cachedir;

int  ka_timeout;
int  ka_max;

struct user_ent {
       char            *name;   /* user's login name */
       char            *home;   /* user's home directory */
       struct user_ent *next;   /* next entry in linked list */
     };

static struct user_ent *user0 = NULL;

/*
 * Name: read_config_files
 * 
 * Description: Reads config files via yyparse, then makes sure that 
 * all required variables were set properly.
 */                           
void read_config_files (void)
{
  char path [_MAX_PATH];

  sprintf (path,"%s%c%s",server_root,SLASH,CONFIG_NAME);
  yyin = fopen (path,READ_MODE);
  if (!yyin)
  {
    fprintf (stderr, "Could not open /$(server-root)/%s for reading.\n",
             CONFIG_NAME);
    exit (1);
  }
  if (yyparse())
  {
    fprintf (stderr, "Error parsing config files, exiting\n");
    exit (1);
  }
  if (!server_name)
  {
    char buf[255];
    if (gethostname(buf,sizeof(buf)))
    {
      fprintf (stderr, "Cannot determine hostname.  ");
      fprintf (stderr, "Set ServerName in %s.\n",CONFIG_NAME);
      exit (1);
    }
    server_name = strdup (buf);
  }
}

/*
 * Name: get_home_dir
 * 
 * Description: Returns pointer to the supplied user's home directory.
 */
char *get_home_dir (char *name)
{
  struct user_ent *usr;

  for (usr = user0; usr && name; usr = usr->next)
      if (!stricmp(usr->name,name))
         return (usr->home);
  return (NULL);
}

void add_user_dir (char *user, char *home_dir)
{
  struct user_ent *usr = malloc (sizeof(*usr));

  if (!usr)
     die (OUT_OF_MEMORY);

  usr->name = strdup (user);
  usr->home = strdup (home_dir);
  usr->next = user0;
  user0     = usr;
  if (!usr->name || !usr->home)
     die (OUT_OF_MEMORY);
}
