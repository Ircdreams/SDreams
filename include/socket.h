/* include/socket.h
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
 * $Id: socket.h,v 1.12 2007/12/01 02:22:31 romexzf Exp $
 */

#ifndef HAVEINC_socket
#define HAVEINC_socket

#define TCPWINDOWSIZE 513 /* à augmenter si le réseau augmente de taille */
#define MAXPARA 16

extern void init_bot(void);
extern int run_bot(void);
extern void socket_close(void);
extern void socket_init(void);
extern void socket_register(int);
extern void socket_unregister(int);

extern int fd_in_use(int);

#endif /*HAVEINC_socket*/
