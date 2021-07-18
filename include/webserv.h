/* include/web2cs.h - Déclarations du Web2CS
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

#ifndef HAVEINC_web2cs
#define HAVEINC_web2cs

#define MAXPARAM 128
#define W2CDEBUG 
/*#define W2CDEBUGALL*/ 

#define MAXWEBCON 128
#define W2C_SENDSIZE 1024
#define W2C_RECVSIZE 256
#define W2C_MAXIDLE 10 /* in seconds */

#define w2c_fd(x)               ((x) - 2)
/* structures */
typedef struct WClient {
	int fd; 			/* fd de la connexion */
	unsigned int flag;	/* divers infos */
#define W2C_AUTH 	0x01
#define W2C_FREE 	0x02
#define W2C_FLUSH 	0x04
	char ip[16]; 	/* ip */
	size_t buflen;
	size_t recvlen;
	time_t lastread;
	char QBuf[W2C_SENDSIZE+1];
	char RecvBuf[W2C_RECVSIZE+1];
	anUser *user; 		/* ptr vers la struct user*/
} WClient;

typedef struct w2c_hallow {
	unsigned long int host;
	struct w2c_hallow *next;
} w2c_hallow;

#define IsAuth(x) 		((x)->flag & W2C_AUTH)
#define WSetFlush(x) 	(x)->flag |= W2C_FLUSH
#define WDelFlush(x) 	(x)->flag &= ~W2C_FLUSH
#define WIsFlush(x) 	((x)->flag & W2C_FLUSH)

/* dear global vars */

extern int bind_sock;

extern w2c_hallow *w2c_hallowhead;

extern int w2c_handle_read(int);
extern void w2c_initsocket(void);
extern WClient *w2c_addclient(int, struct in_addr *);
extern void w2c_delclient(WClient *);
extern int w2c_sendrpl(WClient *, const char *, ...);
extern int w2c_exit_client(WClient *, const char *);
extern void w2c_parse_qstring(char *, size_t , char **, int , int , int *);

extern int w2c_userbyserv(WClient *, int , char **);
extern int w2c_stats(WClient *, int , char **);
extern int w2c_whoison(WClient *, int , char **);
extern int w2c_online(WClient *, int , char **);
extern int w2c_user_online(WClient *, int , char **);
extern int w2c_liston(WClient *, int , char **);
extern int w2c_topic(WClient *, int , char **);
extern int w2c_whois(WClient *, int , char **);
extern int w2c_list(WClient *, int , char **);
extern int w2c_motd(WClient *, int , char **);
extern int w2c_checkuser(WClient *, int , char **);
extern int w2c_passwd(WClient *, int , char **);
extern int w2c_login(WClient *, int , char **);
extern int w2c_restart(WClient *, int , char **);
extern int w2c_write_files(WClient *, int , char **);
extern int w2c_user(WClient *, int , char **);
extern int w2c_register(WClient *, int , char **);
extern int w2c_channel(WClient *, int , char **);
extern int w2c_memo(WClient *, int , char **);
extern int w2c_regchan(WClient *, int , char **);
extern int w2c_oubli(WClient *, int , char **);

#endif
