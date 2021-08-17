/* src/web2cs-engine.c - Moteur du Web2CS
 *
 * Copyright (C) 2002-2008 David Cortier  <Cesar@ircube.org>
 *                         Romain Bignon  <Progs@coderz.info>
 *                         Benjamin Beret <kouak@kouak.org>
 *
 * SDreams v2 (C) 2021 -- Ext by @bugsounet <bugsounet@bugsounet.fr>
 * site web: http://www.ircdreams.org
 *
 * Services pour serveur IRC. Supporté sur Ircdreams v3
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
 * $Id: web2cs-engine.c,v 1.95 2008/01/05 01:24:13 romexzf Exp $
 */

/* NEW CS LIVE */

#include "main.h"
#ifdef WEB2CS
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include "web2cs.h"
#include "web2cs-engine.h"
#include "web2cs-commands.h"
#include "socket.h"

/* dear global vars */

/*#define W2CDEBUG*/

#define W2C_LINELEN 512

int bind_sock = -1;

w2c_hallow *w2c_hallowhead = NULL;

static WClient myWClient[MAXWEBCON + 1];

/* Array indexing by sockets fds the index in myWClient array
 * Max fds in use is : 1 fd / logtype + irc + listen + w2c_clients + fd in use
 */

#define w2c_fd(x) 		(W2C_fd_to_idx[(x)])

static int W2C_fd_to_idx[LOG_TYPE_COUNT + 3 + MAXWEBCON + 1];

#ifdef W2CDEBUG

#	ifdef W2CDEBUGALL
/* Web2CS debug function, basicaly logs everything. */
static int w2cdebug(const char *fmt, ...)
{
	va_list vl;

	va_start(vl, fmt);
	log_writev(LOG_W2CRAW, 0, fmt, vl);
	va_end(vl);

	return 0;
}

#		define DEBUGF(x) w2cdebug x
#	else
#		define DEBUGF(x)
#	endif
#endif
/*** Socks ***/

/* send current client's buffer */
static inline int w2c_dequeue(WClient *cl)
{
	bot.WEBtrafficUP += cl->buflen; /* stats */
	send(cl->fd, cl->QBuf, cl->buflen, 0);
	WDelFlush(cl);
	cl->buflen = 0;
	return 0;
}

static void w2c_delclient(WClient *del)
{
	if(del->buflen) w2c_dequeue(del); /* flush data if any */

	socket_unregister(del->fd);

	w2c_fd(del->fd) = -1;

	DEBUGF(("Exiting client %s from %d", del->ip, del->fd));
	close(del->fd);
	memset(del, 0, sizeof *del); /* clean up */
	del->flag = W2C_FREE; /* now available for a new con */
}

/* format reply, queue the reply in send buffer and flush it if needed */
int w2c_sendrpl(WClient *cl, const char *pattern, ...)
{
	va_list vl;
#ifdef W2CDEBUG
	size_t tmp = 0;
#endif

	if(cl->buflen + W2C_LINELEN >= W2C_SENDSIZE) w2c_dequeue(cl); /* may overflow, flush it */

	va_start(vl, pattern);
	/* format */
#ifdef W2CDEBUG
	tmp = cl->buflen;
#endif

	cl->buflen += myvsnprintf(cl->QBuf + cl->buflen, W2C_SENDSIZE - cl->buflen - 2,
					pattern, vl);
	va_end(vl);
	DEBUGF(("Sending [%s] to %s", cl->QBuf + tmp, cl->ip));

	/* end the line */
	cl->QBuf[cl->buflen++] = '\n';
	cl->QBuf[cl->buflen] = 0;

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

/* ********* */

static struct w2c_commands {
   char cmd[CMDLEN + 1];
   int (*func) (WClient *, int, char **);
   int args;
   int flag;
#define W2C_CAUTH 	0x1
#define W2C_CPWD1 	0x2
} w2c_cmds[] = {
	{"pass",		w2c_passwd,		1, 0},
	{"cookie", 		w2c_wcookie, 	3, W2C_CPWD1},
	{"auth", 		w2c_wauth, 		3, W2C_CPWD1},
	{"login",		w2c_login, 		3, W2C_CPWD1},
	{"user",		w2c_user,		2, W2C_CAUTH},
	{"register",	w2c_register, 	4, 0},
	{"isreg", 		w2c_isreg, 		1, 0},
	{"channel",		w2c_channel, 	2, W2C_CAUTH},
	{"memo", 		w2c_memo, 		1, W2C_CAUTH},
	{"regchan", 	w2c_regchan, 	2, W2C_CAUTH},
	{"oubli",		w2c_oubli, 		3, 0}
};

void w2c_initsocket(void)
{
	unsigned int reuse_addr = 1;
	struct sockaddr_in localhost = {0}; /* bind info structure */

	bind_sock = socket(AF_INET, SOCK_STREAM, 0); /* on demande un socket */
	if(bind_sock < 0)
	{
		log_write(LOG_SOCKET, LOG_DOTTY, "Impossible de créer le serveur W2C: %s",
			strerror(errno));
		exit(EXIT_FAILURE);
	}
	fcntl(bind_sock, F_SETFL, O_NONBLOCK);

	setsockopt(bind_sock, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof reuse_addr);

	localhost.sin_family = AF_INET;
	localhost.sin_addr.s_addr = INADDR_ANY;
	localhost.sin_port = htons(bot.w2c_port);

	if(bind(bind_sock, (struct sockaddr *) &localhost, sizeof localhost) < 0)
	{
		log_write(LOG_SOCKET, LOG_DOTTY, "Impossible d'écouter au port %d pour"
											" le serveur de DB.", bot.w2c_port);
		close(bind_sock);
		exit(EXIT_FAILURE);
	}

	listen(bind_sock, 5);
	socket_register(bind_sock);

	log_write(LOG_MAIN, 0, "Serveur de base de données du Channel Service OK (Port %d)",
		bot.w2c_port);

	/* init myWClient tab */
	for(reuse_addr = 0; reuse_addr < ASIZE(myWClient); ++reuse_addr)
		myWClient[reuse_addr].flag = W2C_FREE;

	for(reuse_addr = 0; reuse_addr < ASIZE(W2C_fd_to_idx); ++reuse_addr)
		W2C_fd_to_idx[reuse_addr] = -1;
}

static int w2c_getslot(void)
{
	unsigned int i = 0;

	for(; i < ASIZE(myWClient); ++i)
		if(myWClient[i].flag & W2C_FREE) return i;

	return -1;
}

static void w2c_purge_idling_clients(void)
{
	unsigned int i = 0;

	for(; i < ASIZE(myWClient); ++i)
	{
		WClient *tmp = &myWClient[i];

		if(!(tmp->flag & W2C_FREE) && CurrentTS - tmp->lastread > W2C_MAXIDLE)
		{
			log_write(LOG_W2C, LOG_DOWALLOPS, "Purging(Idle) client[%s]/fd#%d, %ld seconds",
				tmp->ip, tmp->fd, CurrentTS - tmp->lastread);
			w2c_delclient(tmp);
		}
	}
}

static w2c_hallow *w2c_ip_can_connect(struct in_addr *ip_s)
{
	w2c_hallow *tmp = w2c_hallowhead;

	for(; tmp && tmp->host != ip_s->s_addr; tmp = tmp->next);

	return tmp;
}

int w2c_addclient(int fd, struct in_addr *ip_s)
{
	WClient *cl = NULL;
	const char *ip = inet_ntoa(*ip_s);
	static time_t w2c_lastcheck = 0;

	DEBUGF(("Adding client [%s] at %d", ip, fd));

	if(CurrentTS - w2c_lastcheck > W2C_MAXIDLE)
	{
		w2c_purge_idling_clients();
		w2c_lastcheck = CurrentTS;
	}

	++bot.CONtotal;

	if(!w2c_ip_can_connect(ip_s))	/* non autorisé */
		log_write(LOG_W2C, LOG_DOWALLOPS, "Tentative de connexion non autorisée [%s]", ip);

	else if((unsigned int) fd >= ASIZE(W2C_fd_to_idx))
		log_write(LOG_W2C, LOG_DOWALLOPS, "Plus de sockets que possible ?! (Dernier %d"
			" sur %s, Max: %d)", fd, ip, ASIZE(W2C_fd_to_idx));

	else if(w2c_fd(fd) != -1)
		log_write(LOG_W2C, LOG_DOWALLOPS, "Connexion sur un slot déjà occupé!? (%s -> %d[%s])",
			ip, fd, myWClient[w2c_fd(fd)].ip);

	else if((w2c_fd(fd) = w2c_getslot()) == -1)
		log_write(LOG_W2C, LOG_DOWALLOPS, "Trop de clients connectés ! (Dernier %d"
			" sur %s, Max: %d)", fd, ip, ASIZE(W2C_fd_to_idx));

	else
	{	/* register the socket in client list */
		cl = &myWClient[w2c_fd(fd)];
		cl->fd = fd;
		socket_register(fd);
		strcpy(cl->ip, ip);
		cl->user = NULL;
		cl->flag = 0;
	}

	return !!cl;
}

static struct w2c_commands *w2c_findcmd(const char *cmd)
{
	unsigned int i = 0;
	for(; i < ASIZE(w2c_cmds); ++i)
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
		return w2c_exit_client(cl, "ERROR SYS_SYNTAX");

	*ptr++ = 0;

	if(!(cmdp = w2c_findcmd(parv[0])))
		return w2c_exit_client(cl, "ERROR SYS_UNKNOWNCMD");

	if(((cmdp->flag & W2C_CAUTH) && !cl->user)
	|| (!IsAuth(cl) && strcasecmp("pass", parv[0])))
		return w2c_exit_client(cl, "ERROR SYS_NOTAUTH");

	while(*ptr) /* split buf in a char * array */
	{
		while(*ptr == ' ') *ptr++ = 0; /* drop spaces */
		if(!*ptr || parc >= MAXPARAM) break;
		else parv[parc++] = ptr;
		while(*ptr && *ptr != ' ') ++ptr; /* go through token till end or space */
	}
	parv[parc] = NULL; /* ends array with NULL */

	if(parc <= cmdp->args) return w2c_exit_client(cl, "ERROR SYS_SYNTAX");

	if(log_need_action(LOG_W2CCMD) && IsAuth(cl))
	{
		static char pwd[] = "***";
		char *save = NULL;

		if(cmdp->flag & W2C_CPWD1 && parc > 3) /* replace pwd with '***' */
		{
			save = parv[2];
			parv[2] = pwd;
		}

		log_write(LOG_W2CCMD, 0, "%s %s", cmdp->cmd, parv2msg(parc-1, parv, 1, 512));

		if(save) parv[2] = save; /* put pwd back in place */
	}

	cmdp->func(cl, parc, parv);

	return 0;
}

int w2c_handle_read(int fd)
{
	WClient *cl;
	char buf[W2C_RECVSIZE + 1];
	int r;

	if((unsigned int) fd >= ASIZE(W2C_fd_to_idx) || w2c_fd(fd) == -1)
		return log_write(LOG_SOCKET, 0, "W2C: Reading data from sock #%d, not registered ?", fd);

	cl = &myWClient[w2c_fd(fd)];

	if((r = recv(fd, buf, sizeof buf -1, 0)) <= 0)
	{	/* if r == 0, we reached EOF */
		if(r) log_write(LOG_SOCKET, 0, "W2C: Erreur lors de recv(%s): [%s] (%d)",
					cl->ip, strerror(errno), errno);
		w2c_delclient(cl);
	}
	else
	{
		char *ptr = buf;

		buf[r] = 0;
		bot.WEBtrafficDL += r;

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
				cl->RecvBuf[cl->recvlen - (ptr[-1] == '\r' ? 1 : 0)] = 0; /* drop \r too */
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
			/* Note that if line was too long but newline was found, it drops the \n */
			++ptr;
		}
	}
	return 0;
}

#endif

