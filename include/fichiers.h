/* include/fichiers.h - Lecture/Écriture des données
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
 * $Id: fichiers.h,v 1.19 2007/12/02 17:01:09 romexzf Exp $
 */

#ifndef HAVEINC_fichier
#define HAVEINC_fichier

#define HF_LOG	0x1

#define DBVERSION_U 3
#define DBVERSION_C 1

extern int db_load_users(int);
extern int db_load_chans(int);
extern int db_write_users(void);
extern int db_write_chans(void);

extern void write_cmds(void);
extern int load_cmds(int);

extern int write_files(aNick *, aChan *, int, char **);

#endif /*HAVEINC_fichier*/
