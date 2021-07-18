 /* src/webserv.c - Moteur du Webserv
 * Copyright (C) 2004-2005 ircdreams.org
 *
 * contact: bugs@ircdreams.org
 * site web: http://www.ircdreams.org
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
 * $Id: webserv.c,v 1.51 2006/02/16 17:36:26 bugs Exp $
 */

/* NEW CS LIVE*/

#include "main.h"
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/socket.h>
#include <errno.h>
#include "debug.h"
#include "socket.h"
#include "webserv.h"

/* dear global vars */

int bind_sock = 0;

static WClient myWClient[MAXWEBCON+1];

w2c_hallow *w2c_hallowhead = NULL;


#ifdef W2CDEBUG

#ifdef W2CDEBUGALL
/* Webserv debug function, basicaly logs everything. */
static int w2cdebug(const char *fmt, ...)
{
	FILE *fd = fopen("logs/w2c.log", "a");
	va_list vl;

	if(!fd) return Debug(W_WARN, "Error while opening w2c log file : %s", strerror(errno));

	va_start(vl, fmt);
	fputs(get_time(NULL, CurrentTS), fd); /* TimeStamp */
	fputc(' ', fd);
	vfprintf(fd, fmt, vl);
	fputc('\n', fd);
	fclose(fd);

	va_end(vl);
	return 0;
}

#	define DEBUGF(x) w2cdebug x
#else
#	define DEBUGF(x)
#endif
#endif
/*** Socks ***/

/* send current client's buffer */
static inline int w2c_dequeue(WClient *cl)
{
	send(cl->fd, cl->QBuf, cl->buflen, 0);
	WDelFlush(cl);
	cl->buflen = 0;
	return 0;
}

/* format reply, queue the reply in send buffer and flush it if needed */
int w2c_sendrpl(WClient *cl, const char *pattern, ...)
{
	static char buf[512];
	va_list vl;
	int len;

	va_start(vl, pattern);
	len = myvsnprintf(buf, sizeof buf - 2, pattern, vl); /* format */

	DEBUGF(("sending [%s] to %s", buf, cl->ip));

	buf[len++] = '\n';
	buf[len] = 0;
	va_end(vl);

	bot.WEBtrafficUP += len;

	if(cl->buflen + len >= W2C_SENDSIZE) w2c_dequeue(cl); /* would overflow, flush it */
	memcpy(cl->QBuf + cl->buflen, buf, len + 1); /* append reply to buffer */
	cl->buflen += len;
	if(WIsFlush(cl)) w2c_dequeue(cl); /* Is flush forced ? */

	return 0;
}

/* disconnect a client, allow a reason to be given */
int w2c_exit_client(WClient *cl, const char *msg)
{
	if(msg && *msg) w2c_sendrpl(cl, msg);

	w2c_delclient(cl);
	return 0;
}

void w2c_parse_qstring(char *buf, size_t size, char **parv, int parc, int i, int *iptr) 
{ 
        int inquote = 0; 
        for(--size;size && i < parc;++i) 
        { 
                const char *p = parv[i]; 
                for(;size && *p;++p) 
                { 
                        if(*p == '"') 
                        { 
                                if((inquote = inquote ? 0 : 1)) continue; 
                                else break; 
                        } 
                        else if(*p == '\\') ++p; 
                        *buf++ = *p; 
                        --size; 
                } 
                if(!inquote) break; 
                if(size--) *buf++ = ' '; 
        } 
        *buf = 0; 
        *iptr = i - 1; 
} 

static struct w2c_commands {
   char cmd[CMDLEN + 1];
   int (*func) (WClient *, int, char **);
   int args;
   int flag; 
#define W2C_CAUTH 0x1 
} w2c_cmds[] = {
	{"pass",		w2c_passwd,		1, 0},
	{"login",		w2c_login, 		3, 0},
	{"restart",		w2c_restart,		1, W2C_CAUTH},
	{"write",		w2c_write_files,	1, W2C_CAUTH},
	{"user",		w2c_user,		2, W2C_CAUTH},
	{"register",		w2c_register, 		4, 0},
	{"channel",		w2c_channel, 		2, W2C_CAUTH},
	{"memo", 		w2c_memo, 		1, W2C_CAUTH},
	{"regchan", 		w2c_regchan, 		2, W2C_CAUTH},
	{"oubli",               w2c_oubli,              2, 0},
	{"stats",		w2c_stats,		1, 0},
	{"whoison",		w2c_whoison,		1, 0},
	{"online",		w2c_online,		1, 0},
	{"userbyserv",		w2c_userbyserv,		1, 0},
	{"user_online",		w2c_user_online,	1, 0},
	{"liston",		w2c_liston,		1, 0},
	{"topic",		w2c_topic,		1, 0},
	{"whois",		w2c_whois,		1, 0},
	{"list",                w2c_list,               1, 0},
	{"motd",		w2c_motd,		1, 0},
	{"checkuser",		w2c_checkuser,		1, 0}
};

void w2c_initsocket(void)
{
	unsigned int reuse_addr = 1;
	struct sockaddr_in localhost = {0}; /* bind info structure */

	bind_sock = socket(AF_INET, SOCK_STREAM, 0); /* on demande un socket */
	if(bind_sock < 0)
	{
		Debug(W_MAX|W_TTY, "Impossible de créer le WebServ. (socket non créé)");
		exit(EXIT_FAILURE);
	}
	fcntl(bind_sock, F_SETFL, O_NONBLOCK);

	setsockopt(bind_sock, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof reuse_addr);

	localhost.sin_family = AF_INET;
	localhost.sin_addr.s_addr = INADDR_ANY;
	localhost.sin_port = htons(bot.w2c_port);

	if(bind(bind_sock, (struct sockaddr *) &localhost, sizeof localhost) < 0)
	{
		Debug(W_MAX|W_TTY, "Impossible d'écouter au port %d pour le WebServ.\n", bot.w2c_port);
		close(bind_sock);
		exit(EXIT_FAILURE);
	}

	listen(bind_sock, 5);
	SOCK_REGISTER(bind_sock);

	/* init myWClient tab */
	for(reuse_addr = 0;reuse_addr < ASIZE(myWClient);++reuse_addr)
		myWClient[reuse_addr].flag = W2C_FREE;
}

WClient *w2c_addclient(int fd, struct in_addr *ip_s)
{
	WClient *cl = NULL;
	w2c_hallow *tmp = w2c_hallowhead;
	char *ip = inet_ntoa(*ip_s);
	static time_t w2c_lastcheck = 0;

	DEBUGF(("Adding client [%s] at %d", ip, fd));

	if(CurrentTS - w2c_lastcheck > W2C_MAXIDLE) 
        {
		unsigned int i = 0;
        	for(;i < ASIZE(myWClient);++i) 
                           if(!(myWClient[i].flag & W2C_FREE) && CurrentTS - myWClient[i].lastread > W2C_MAXIDLE) 
                                   w2c_delclient(&myWClient[i]); 
                w2c_lastcheck = CurrentTS; 
        }

	++bot.CONtotal;
	for(;tmp && tmp->host != ip_s->s_addr;tmp = tmp->next); 
        /* non autorisé */ 
    
        if(!tmp) Debug(W_WARN, "Webserv: Tentative de connexion non autorisée [%s]", ip); 
	else if((unsigned int) w2c_fd(fd) >= ASIZE(myWClient))
		Debug(W_WARN, "Webserv: Trop de clients connectés! (Dernier %d sur %s, Max: %d)",
			fd, ip, ASIZE(myWClient));
	else if(!(myWClient[w2c_fd(fd)].flag & W2C_FREE))
		Debug(W_MAX|W_WARN, "Webserv: Connexion sur un slot déjà occupé!? (%s -> %d[%s])",
			ip, fd, myWClient[w2c_fd(fd)].ip);
	else
	{	/* register the socket in client list */
		cl = &myWClient[w2c_fd(fd)];
		cl->fd = fd;
		SOCK_REGISTER(fd);
		strcpy(cl->ip, ip);
		cl->user = NULL;
		cl->flag = 0;
	}

	return cl;
}

void w2c_delclient(WClient *del)
{
	if(del->fd >= highsock) /* Was the highest..*/
		while(highsock >= bot.sock) /* loop only till bot.sock */
			 if(!(myWClient[w2c_fd(highsock--)].flag & W2C_FREE)) break;

	if(del->buflen) w2c_dequeue(del); /* flush data if any */
	FD_CLR(del->fd, &global_fd_set);

	DEBUGF(("Exiting client %s from %d", del->ip, del->fd));
	close(del->fd);
	memset(del, 0, sizeof *del); /* clean up */
	del->flag = W2C_FREE; /* now available for a new con */
	return;
}

static struct w2c_commands *w2c_findcmd(const char *cmd)
{
	unsigned int i = 0;
	for(;i < ASIZE(w2c_cmds);++i)
		if(!strcasecmp(w2c_cmds[i].cmd, cmd)) return &w2c_cmds[i];
	return NULL;
}

static int w2c_parsemsg(WClient *cl)
{
	char *parv[MAXPARAM + 1];
	register char *ptr;
	int parc = 1;
	struct w2c_commands *cmdp;

	DEBUGF(("Parsing [%s] from client %s@%d", cl->RecvBuf, cl->ip, cl->fd));

	parv[0] = cl->RecvBuf; /* cmd */
	if(!(ptr = strchr(cl->RecvBuf, ' ')))
		return w2c_exit_client(cl, "erreur Syntaxe incorrecte");

	*ptr++ = 0;

	if(!(cmdp = w2c_findcmd(parv[0])))
		return w2c_exit_client(cl, "erreur Commande inconnue");

	if(((cmdp->flag & W2C_CAUTH) && !cl->user) || (!IsAuth(cl) && strcasecmp("pass", parv[0]))) 
                return w2c_exit_client(cl, "erreur Non autorisé"); 

	while(*ptr) /* split buf in a char * array */
	{
		while(*ptr == ' ') *ptr++ = 0; /* drop spaces */
		if(!*ptr || parc >= MAXPARAM) break;
		else parv[parc++] = ptr;
		while(*ptr && *ptr != ' ') ++ptr; /* go through token till end or space */
	}
	parv[parc] = NULL; /* ends array with NULL */

	if(parc <= cmdp->args) return w2c_exit_client(cl, "erreur Syntaxe incorrecte");
	cmdp->func(cl, parc, parv);

	return 0;
}

int w2c_handle_read(int fd)
{
	WClient *cl;
	char buf[400];
	int read;

	if((unsigned int) w2c_fd(fd) >= ASIZE(myWClient))
		return Debug(W_MAX|W_WARN, "Web: reading data from sock #%d, not registered ?", fd);

	cl = &myWClient[w2c_fd(fd)];

	if((read = recv(fd, buf, sizeof buf -1, 0)) < 0)
	{
		Debug(W_MAX|W_WARN, "Web: Erreur lors de recv(%s): [%s] (%d)",
			cl->ip, strerror(errno), errno);
		w2c_delclient(cl);
	}
	else
	{
		char *ptr = buf;

		buf[read] = 0;
		bot.WEBtrafficDL += read;

		/* split read buffer in lines,
		   if its length is more than acceptable, truncate it.
		   if a newline is found is the trailing buffer, go on parsing,
		   otherwise abort..
		   if line is unfinished, store it in fd own recv's buffer (cl->RecvBuf)
		  */
		while(*ptr)
		{
			if(*ptr == '\n' || cl->recvlen >= W2C_RECVSIZE)
			{
				cl->RecvBuf[cl->recvlen-1] = 0;
				cl->lastread = CurrentTS;
				w2c_parsemsg(cl);
				if(cl->flag & W2C_FREE) break; /* in case of failed pass */

				if(cl->recvlen >= W2C_RECVSIZE && !(ptr = strchr(ptr + 1, '\n')))
				{ 	/* line exceeds size and no newline found */
					cl->recvlen = 0;
					break; /* abort parsing */
				}
				cl->recvlen = 0; /* go on on newline */
			}
			else cl->RecvBuf[cl->recvlen++] = *ptr; /* copy */
			/*next char, Note that if line was to long but newline was found, it drops the \n */
			++ptr;
		}
	}
	return 0;
}
