/* include/web2cs-engine.h - Déclarations du Web2CS pour web2cs-commands
 *
 * Copyright (C) 2002-2006 David Cortier  <Cesar@ircube.org>
 *                         Romain Bignon  <Progs@kouak.org>
 *                         Benjamin Beret <kouak@kouak.org>
 *
 * site web: http://sf.net/projects/scoderz/
 *
 * Services pour serveur IRC. Supporté sur IrcProgs et IRCoderz
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
 * $Id: web2cs-engine.h,v 1.1 2006/12/20 23:36:56 romexzf Exp $
 */

#ifndef HAVEINC_web2cs_engine
#define HAVEINC_web2cs_engine

#define MAXPARAM 128
#define W2CDEBUG
/*#define W2CDEBUGALL*/
#define MAXWEBCON 2
#define W2C_SENDSIZE 1024
#define W2C_RECVSIZE 256
#define W2C_MAXIDLE 10 /* in seconds */

/* structures */
typedef struct WClient {
	int fd; 			/* fd de la connexion */
	unsigned int flag;	/* divers infos */
#define W2C_AUTH 	0x01
#define W2C_FREE 	0x02
#define W2C_FLUSH 	0x04
	char ip[DOTTEDIPLEN + 1]; 	/* ip */
	size_t buflen;
	size_t recvlen;
	time_t lastread;
	char QBuf[W2C_SENDSIZE+1];
	char RecvBuf[W2C_RECVSIZE+1];
	anUser *user; 		/* ptr vers la struct user*/
} WClient;


#define IsAuth(x) 		((x)->flag & W2C_AUTH)
#define WSetFlush(x) 	(x)->flag |= W2C_FLUSH
#define WDelFlush(x) 	(x)->flag &= ~W2C_FLUSH
#define WIsFlush(x) 	((x)->flag & W2C_FLUSH)

int w2c_sendrpl(WClient *, const char *, ...);
int w2c_exit_client(WClient *, const char *);

#endif
