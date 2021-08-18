/* src/socket.c - Gestion des sockets & parse
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
 * $Id: socket.c,v 1.37 2008/01/05 01:24:13 romexzf Exp $
 */

#include <unistd.h>
#include "main.h"
#include "outils.h"
#include "serveur.h"
#include "socket.h"
#include "hash.h"
#include "mylog.h"
#include "cs_cmds.h"
#include "add_info.h"
#include "timers.h"
#ifdef USEBSD
#include <netinet/in.h>
#endif
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <fcntl.h>
#ifdef WEB2CS
#include "web2cs.h"
#endif

static fd_set global_fd_set;
static int highsock = 0;

static struct aMsg {
  const char *cmd;
  int (*func) (int parc, char **parv);
} msgtab[] = {
  {TOKEN_PRIVMSG, m_privmsg},
  {TOKEN_JOIN, m_join},
  {TOKEN_NICK, m_nick},
  {TOKEN_QUIT, m_quit},
  {TOKEN_MODE, m_mode},
  {TOKEN_PART, m_part},
  {TOKEN_KICK, m_kick},
  {TOKEN_CREATE, m_create},
  {TOKEN_BURST, m_burst},
  {TOKEN_AWAY, m_away},
  {TOKEN_PING, m_ping},
  {"SERVER", m_server},
  {"OM", m_mode},
  {"CM", m_clearmode},
  {TOKEN_TOPIC, m_topic},
  {TOKEN_KILL, m_kill},
#ifdef HAVE_OPLEVELS
  {TOKEN_DESTRUCT, m_destruct},
#endif
  {TOKEN_WHOIS, m_whois},
  {TOKEN_SERVER, m_server},
  {TOKEN_SQUIT, m_squit},
  {TOKEN_EOB, m_eob},
  {TOKEN_PASS, m_pass},
  {"ERROR", m_error},
};

static void parse_this(char *msg)
{
	int parc = 0;
	unsigned int i = 0;
	char *cmd = NULL, *parv[MAXPARA + 2];

#ifdef DEBUG
	log_write(LOG_RAW, 0, "R - %s", msg);
#endif

	if(!mainhub) parv[parc++] = bot.server;

	while(*msg)
	{
		while(*msg == ' ') *msg++ = 0; /* on supprime les espaces pour le découpage */
		if(*msg == ':') /* last param */
		{
			parv[parc++] = msg + 1;
			break;
		}
		if(!*msg) break;

		if(parc == 1 && !cmd) cmd = msg; /* premier passage à parc 1 > cmd */
		else parv[parc++] = msg;
		while(*msg && *msg != ' ') ++msg; /* on laisse passer l'arg.. */
	}
	parv[parc] = NULL;

	for(; i < ASIZE(msgtab); ++i)
		if(!strcasecmp(msgtab[i].cmd, cmd))
		{
			msgtab[i].func(parc, parv);
			return; /* on sait jamais */
		}
}

int fd_in_use(int fd)
{
	return (FD_ISSET(fd, &global_fd_set) || log_fd_in_use(fd));
}

void socket_close(void)
{
	int i = 0;

	for(; i <= highsock; ++i) if(FD_ISSET(i, &global_fd_set)) close(i);

	FD_ZERO(&global_fd_set);
}

static int callback_sock_reconnect_irc(Timer *timer)
{
	init_bot();
	return 1; /* remove timer, socket_reconnect recreate it */
}

static void socket_reconnect_irc(void)
{
	if(bot.sock >= 0)
	{
		if(FD_ISSET(bot.sock, &global_fd_set)) FD_CLR(bot.sock, &global_fd_set);
		close(bot.sock);
		bot.sock = -1;
		log_write(LOG_MAIN, 0, "Déconnecté du serveur IRC %s", bot.ip);
	}

	purge_network(); /* clean nicks, servers and channels data */

	timer_add(WAIT_CONNECT, TIMER_RELATIF, callback_sock_reconnect_irc, NULL, NULL);
}

static int socket_create_irc(void)
{
	struct sockaddr_in fsocket = {0};
	int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP), opts;
	unsigned long bindip = inet_addr(bot.bindip);

	if((bot.sock = sock) < 0)
		return log_write(LOG_SOCKET, 0, "Impossible de créer le socket: erreur #%d [%s]",
							errno, strerror(errno));

	fsocket.sin_family = AF_INET;
	fsocket.sin_addr.s_addr = inet_addr(bot.ip);
	fsocket.sin_port = htons(bot.port);

	if(bindip) /* trying to bind some local IP */
	{
		struct sockaddr_in bindsa = {0};

		bindsa.sin_family = AF_INET;
		bindsa.sin_addr.s_addr = bindip;

		if(bind(sock, (struct sockaddr *) &bindsa, sizeof bindsa) < 0)
			return log_write(LOG_SOCKET, 0, "bind failed pour %s: erreur #%d [%s]",
						bot.bindip, errno, strerror(errno));
	}

	if(connect(sock, (struct sockaddr *) &fsocket, sizeof fsocket) < 0)
		return log_write(LOG_SOCKET, 0, "Erreur #%d lors de la connexion: %s",
					errno, strerror(errno));

	socket_register(sock);

	if((opts = fcntl(sock, F_GETFL)) < 0)
		log_write(LOG_SOCKET, 0, "Erreur lors de fcntl(F_GETFL)");

	else if(fcntl(sock, F_SETFL, opts | O_NONBLOCK) < 0)
			log_write(LOG_SOCKET, 0, "Erreur lors de fcntl(F_SETFL)");

	return 1; /* success */
}

void socket_register(int fd)
{
	if(fd > highsock) highsock = fd;

	FD_SET(fd, &global_fd_set);
}

void socket_unregister(int fd)
{
	FD_CLR(fd, &global_fd_set);

	if(fd >= highsock) 										/* Was the highest.. */
		for(; highsock >= 0; --highsock) 					/* decrease highsock until  */
			if(FD_ISSET(highsock, &global_fd_set)) break; 	/* we reach an in-use socket */
}

void socket_init(void)
{
	FD_ZERO(&global_fd_set);
#ifdef WEB2CS
	w2c_initsocket();
#endif
}

void init_bot(void)
{
	time_t LTS = time(NULL);
	char num[7];

	if(!socket_create_irc()) /* failed */
	{
		socket_reconnect_irc(); /* schedule reconnection... */
		return;
	}
	/* socket is opened (, binded) and connected, begin Burst ! */

	mainhub = NULL;
	snprintf(num, sizeof num, "%s"NNICK"D", bot.servnum); /* num + max user */

	putserv("PASS %s", bot.pass);
	putserv("SERVER %s 1 %T %T J10 %s +s6 :%s", bot.server, bot.uptime, LTS, num, bot.name);
	add_server(bot.server, num, "1", "J10", bot.server);

	putserv("%s "TOKEN_NICK" %s 1 %T %s %s %s B]AAAB %s :%s", bot.servnum,
		cs.nick, LTS, cs.ident, cs.host, cs.mode, cs.num, cs.name);
	add_nickinfo(cs.nick, cs.ident, cs.host, "B]AAAB", cs.num,
		num2servinfo(bot.servnum), cs.name, LTS, cs.mode);
}

int run_bot(void)
{
	char tbuf[513] = {0}; /* IRC buf */
	int idx_perm = 0, r = 0, i = 0, events = 0, err = 0;
	char bbuf[TCPWINDOWSIZE]; /* fonction commune de parsage de l'ircsock */
	register char *ptr;
	fd_set tmp_fdset;
	struct timeval timeout = {0};

	while(running)
	{
		if(Timers->expire <= CurrentTS) timers_run();
		/* update timeout to remaining time */
		if((timeout.tv_sec = Timers->expire - CurrentTS) < 0) timeout.tv_sec = 0;
		timeout.tv_usec = 0;
		/* save fd set before select */
		tmp_fdset = global_fd_set;

		events = select(highsock + 1, &tmp_fdset, NULL, NULL, &timeout);

		CurrentTS = time(NULL); /* update TS AFTER select() to avoid time shift. */

		if(events < 0)
		{
			if(errno != EINTR)
			{
				log_write(LOG_SOCKET, 0, "Erreur lors de select() (%d: %s)",
					errno, strerror(errno));

				if(++err > 20)
					return log_write(LOG_SOCKET, LOG_DOWALLOPS, "Too many select() errors.");
			}
		}
		else
		{
			/* Handle the most probable case : only an IRC event */

			if(bot.sock >= 0 && FD_ISSET(bot.sock, &tmp_fdset))
			{
				/* lecture du buffer de recv(), puis découpage selon les \r\n et,
				enfin on garde en mémoire dans tbuf le début d'un paquet IRC s'il
				est incomplet. au select suivant les bytes lui appartenant lui
				seront ajoutée grace à t, index permanent.
				la lecture de bloc de 512 bytes plutot que byte à byte est
				au moins 6 fois plus rapide! */

				r = recv(bot.sock, bbuf, sizeof bbuf -1, 0);
				ptr = bbuf;

				if(r <= 0 && errno != EINTR)
				{
					log_write(LOG_SOCKET, 0, "Erreur lors de recv(): [%s]", strerror(errno));
					socket_reconnect_irc();
					continue;
				}

				bbuf[r] = 0;
				bot.dataQ += r; /* compteur du traffic incoming */

				while(*ptr)
				{	/* si ircd compatible, \r avant \n.. */
					if(*ptr == '\n') 		/* fin de la ligne courante */
					{
						tbuf[idx_perm-1] = 0;		/* efface le \r */
						parse_this(tbuf);
						idx_perm = 0;				/* index du buffer permanent */
					}
					else tbuf[idx_perm++] = *ptr;	/* copie dans le buffer permanent */
					++ptr;					/* qui garde la ligne courante */
				}

				if(events == 1) continue;
			} /* IRC event */

#ifdef WEB2CS
			for(i = 0; i <= highsock; ++i)
			{
				if(!FD_ISSET(i, &tmp_fdset)) continue;

				else if(i == bind_sock)
				{
					struct sockaddr_in newcon;
					unsigned int addrlen = sizeof newcon;
					int newfd = accept(bind_sock, (struct sockaddr *) &newcon, &addrlen);

					if(newfd < 0) log_write(LOG_SOCKET, 0, "accept(%s) failed: %s",
									inet_ntoa(newcon.sin_addr), strerror(errno));

					else if(!w2c_addclient(newfd, &newcon.sin_addr)) close(newfd);
				}

				else w2c_handle_read(i);

			} /* for(highsock) */
#endif /* WEB2CS */
		} /* select() */
	} /* end main loop */

	if(!running) socket_close();
	return 0;
}

