/* src/socket.c - Gestion des sockets & parse
 * Copyright (C) 2004-2005 ircdreams.org
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
 * $Id: socket.c,v 1.40 2006/01/09 15:00:47 bugs Exp $
 */

#include <unistd.h>
#include "main.h"
#include "outils.h"
#include "serveur.h"
#include "socket.h"
#include "hash.h"
#include "debug.h"
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
#include <errno.h>
#include "webserv.h"
#include "config.h"
fd_set global_fd_set; 
int highsock = 0; 

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
  {TOKEN_WHOIS, m_whois},
  {TOKEN_SERVER, m_server},
  {TOKEN_SQUIT, m_squit},
  {TOKEN_EOB, m_eob},
  {TOKEN_PASS, m_pass},
  {TOKEN_SVSHOST, m_svshost},
  {RECUP_MOTD, r_motd},
  {"ERROR", m_error},
};

static void parse_this(char *msg)
{
	int parc = 0;
	unsigned int i=0;
	char *cmd = NULL, *parv[MAXPARA + 2];
#ifdef DEBUG
	putlog(LOG_PARSES, "R - %s", msg);
#endif

	if(!mainhub) parv[parc++] = bot.server;

	while(*msg)
	{
		while(*msg == ' ') *msg++ = 0;/* on supprime les espaces pour le découpage*/
		if(*msg == ':') /* last param*/
		{
			parv[parc++] = msg + 1;
			break;
		}
		if(!*msg) break;

		if(parc == 1 && !cmd) cmd = msg;/* premier passage à parc 1 > cmd*/
		else parv[parc++] = msg;
		while(*msg && *msg != ' ') msg++;/*on laisse passer l'arg..*/
	}
	parv[parc] = NULL;

	for(;i < ASIZE(msgtab);++i)
		if (!strcasecmp(msgtab[i].cmd, cmd))
		{
			msgtab[i].func(parc, parv);
			return;/*on sait jamais */
		}
}

void sockets_close(void) 
{ 
        int i = 0; 
    
        for(;i <= highsock;++i) if(FD_ISSET(i, &global_fd_set)) close(i); 
    
        FD_ZERO(&global_fd_set); 
} 

static int callback_sock_reconnect(Timer *timer) 
{ 
        init_bot(); 
        return 1; /* remove timer, socket_reconnect recreate it */ 
} 
 
static void socket_reconnect(void) 
{ 
	if(bot.sock >= 0)
        { 
                if(FD_ISSET(bot.sock, &global_fd_set)) FD_CLR(bot.sock, &global_fd_set); 
                close(bot.sock); 
                bot.sock = -1; 
	        putlog(LOG_ERREURS, "Déconnecté du serveur."); 
 	}
	purge_network(); /* clean nicks, servers and channels data */
 
        timer_add(WAIT_CONNECT, TIMER_RELATIF, callback_sock_reconnect, NULL, NULL); 
} 

static int init_socket(void)
{
	struct sockaddr_in fsocket = {0};
	int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP), opts;
	unsigned long bindip = inet_addr(bot.bindip);

	if((bot.sock = sock) < 0)
		return Debug(W_MAX, "Impossible de créer le socket: erreur #%d [%s]", errno, strerror(errno));

	fsocket.sin_family = AF_INET;
	fsocket.sin_addr.s_addr = inet_addr(bot.ip);
	fsocket.sin_port = htons(bot.port);

	if(bindip) /* trying to bind some local IP */
	{
		struct sockaddr_in bindsa = {0};

		bindsa.sin_family = AF_INET;
		bindsa.sin_addr.s_addr = bindip;

		if(bind(sock, (struct sockaddr *) &bindsa, sizeof bindsa) < 0)
			return Debug(W_MAX, "Bind failed pour %s: erreur #%d [%s]",
				bot.bindip, errno, strerror(errno));
	}

	if(connect(sock, (struct sockaddr *) &fsocket, sizeof fsocket) < 0)
		return Debug(W_MAX, "Erreur #%d lors de la connexion: %s", errno, strerror(errno));

	SOCK_REGISTER(sock);

	if((opts = fcntl(sock, F_GETFL)) < 0) Debug(W_MAX, "Erreur lors de fcntl(F_GETFL)");
	else if(fcntl(sock, F_SETFL, opts | O_NONBLOCK) < 0)
			Debug(W_MAX, "Erreur lors de fcntl(F_SETFL)");

	return 1; /* success */
}

void init_bot(void)
{
	time_t LTS = time(NULL);
	char num[7];

	if(!init_socket()) /* failed */
	{
		socket_reconnect(); /* schedule reconnection... */
		return;
	}
	/* socket is opened (, binded) and connected, begin Burst ! */

	mainhub = NULL;
	snprintf(num, sizeof num, "%s]]]", bot.servnum); /* num + max user */

	putserv("PASS %s", bot.pass);
	putserv("SERVER %s 1 %lu %lu J10 %s +s :%s", bot.server, bot.uptime, LTS, num, bot.name);
	add_server(bot.server, num, "1", "J10", bot.server);

	putserv("%s "TOKEN_NICK" %s 1 %lu %s %s %s B]AAAB %s :%s", bot.servnum,
		cs.nick, LTS, cs.ident, cs.host, cs.mode, cs.num, cs.name);
	add_nickinfo(cs.nick, cs.ident, cs.host, "B]AAAB", cs.num,
		num2servinfo(bot.servnum), cs.name, LTS, cs.mode);
	buildmymotd();
}

int run_bot(void)
{
	char tbuf[513] = {0}; /* IRC buf */
	int t = 0, read, i, events, err = 0;
	char bbuf[TCPWINDOWSIZE];/* fonction commune de parsage de l'ircsock*/
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
                                Debug(W_MAX|W_WARN, "Erreur lors de select() (%d: %s)", errno, strerror(errno));
				if(++err > 20) return Debug(W_MAX|W_WARN, "Too many select() errors.");
                        }
                }
                else
                {
			for(i = 0;i <= highsock;++i)
                        {
                                if(!FD_ISSET(i, &tmp_fdset)) continue;
                                if(i == bot.sock)
                                {
					/* lecture du buffer de recv(), puis découpage selon les \r\n et,
					enfin on garde en mémoire dans tbuf le début d'un paquet IRC s'il
					est incomplet. au select suivant les bytes lui appartenant lui seront
					ajoutée grace à t, index permanent.       packet1\r\npacket2\r\n
					la lecture de bloc de 512 bytes plutot que byte à byte est
					au moins 6 fois plus rapide!*/
					read = recv(i, bbuf, sizeof bbuf -1, 0);
					ptr = bbuf;

					if(read <= 0 && errno != EINTR)
					{
						Debug(W_MAX, "Erreur lors de recv(): [%s]", strerror(errno));
						socket_reconnect();
						continue;
					}

					bbuf[read] = 0;
					bot.dataQ += read; /* compteur du traffic incoming*/
		
					while(*ptr)
					{/* si ircd compatible, \r avant \n..*/
						if(*ptr == '\n') 		/* fin de la ligne courante*/
						{
							tbuf[t-1] = 0;		/* efface le \r*/
							parse_this(tbuf);
							t = 0;				/* index du buffer permanent*/
						}
						else tbuf[t++] = *ptr;	/* copie dans le buffer permanent */
						++ptr;					/* qui garde la ligne courante*/
					}
                                }
                                else if(GetConf(CF_WEBSERV) && i == bind_sock)
                                {
                                        struct sockaddr_in newcon;
                                        unsigned int addrlen = sizeof newcon;
                                        int newfd = accept(bind_sock, (struct sockaddr *) &newcon, &addrlen);
					if(newfd < 0) Debug(W_WARN, "accept() failed: %s", strerror(errno));

					else if(!w2c_addclient(newfd, &newcon.sin_addr)) close(newfd);
                                }
                                else {
					if(GetConf(CF_WEBSERV))
						w2c_handle_read(i);
				}
                        } /* for(highsock) */
                } /* select() */

	} /* end main loop */

	if(!running) sockets_close();

	return 0;
}
