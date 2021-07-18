/* include/del_info.h - Suppression d'informations en m�moire
 * Copyright (C) 2004 ircdreams.org
 *
 * contact: bugs@ircdreams.org
 * site web: http://ircdreams.org
 *
 * Services pour serveur IRC. Support� sur IrcDreams V.2
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
 * $Id: del_info.h,v 1.13 2006/01/06 23:12:59 bugs Exp $
 */

#ifndef HAVEINC_del_info
#define HAVEINC_del_info

extern int kill_remove(aNick *); 
extern void kill_free(aKill *); 

extern void del_dnr(aDNR *);
extern void del_alias(anUser *, const char *);
extern void del_access(anUser *, aChan *);
extern void del_join(aNick *, aNChan *);
extern void del_link(aNChan *, aLink *);
extern void del_ban(aChan *, aBan *);
extern void del_alljoin(aNick *);

#endif /*HAVEINC_del_info*/
