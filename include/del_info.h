/* include/del_info.h - Suppression d'informations en mémoire
 *
 * Copyright (C) 2002-2007 David Cortier  <Cesar@ircube.org>
 *                         Romain Bignon  <Progs@coderz.info>
 *                         Benjamin Beret <kouak@kouak.org>
 *
 * site web: http://sf.net/projects/scoderz/
 *
 * Services pour serveur IRC. Supporté sur IRCoderz
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
 * $Id: del_info.h,v 1.9 2007/12/16 20:48:15 romexzf Exp $
 */

#ifndef HAVEINC_del_info
#define HAVEINC_del_info

#ifdef USE_NICKSERV
extern int kill_remove(aNick *);
extern void kill_free(aKill *);
#endif

extern void del_access(anUser *, aChan *);
extern void del_join(aNick *, aNChan *);
extern void del_link(aNChan *, aLink *);
extern void del_alljoin(aNick *);

extern void ban_del(aChan *, aBan *);
extern void ban_remove(aChan *, aBan *);
extern void ban_free(aBan *);

#endif /*HAVEINC_del_info*/
