/* include/admin_user.h - commandes admins pour gerer les users
 * Copyright (C) 2004 ircdreams.org
 *
 * contact: bugs@ircdreams.org
 * site web: http://ircdreams.org
 *
 * Services pour serveur IRC. Supporté sur IrcDreams V.2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef HAVEINC_adminuser
#define HAVEINC_adminuser

#define MAXMATCHES 40
#define WARNACCESS 30
#define WARNBAN 30

extern int show_userinfo(aNick *, anUser *, int, int);
extern int cs_whois(aNick *, aChan *, int, char **);
extern int admin_user(aNick *, aChan *, int, char **);
extern void show_ususpend(aNick *, anUser *);

#endif /*HAVEINC_adminusr*/
