/* src/cs_cmds.c commandes IRC du CS
 *
 * Copyright (C) 2002-2008 David Cortier  <Cesar@ircube.org>
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
 * $Id: cs_cmds.c,v 1.45 2008/01/20 13:50:39 romexzf Exp $
 */

#include "main.h"
#include "cs_cmds.h"
#include "admin_manage.h"
#include "outils.h"
#include "mylog.h"
#include "hash.h"
#include "timers.h"
#include <sys/socket.h>

void putserv(const char *fmt, ...)
{
	char buf[512];
	size_t len;
	va_list vl;

	va_start(vl, fmt);
	len = myvsnprintf(buf, sizeof buf - 2, fmt, vl);
	va_end(vl);

#ifdef DEBUG
	log_write(LOG_RAW, 0, "S - %s", buf);
#endif

	buf[len++] = '\r';
	buf[len++] = '\n';
	send(bot.sock, buf, len, 0);
	bot.dataS += len;
}

int csreply(aNick *nick, const char *format, ...)
{	/* APACS P YYXXX :f */
	static char buf[512] = " PACS P YYXXX :";
	register char *p = buf;
	register const char *fmt = format;
	const char *end = p + 509;
	char t;
	size_t len = 0U;
	va_list vl;

	va_start(vl, format);

	if(*buf == ' ') buf[0] = cs.num[0], buf[1] = cs.num[1], buf[2] = cs.num[2],
		buf[3] = cs.num[3], buf[4] = cs.num[4];

	p += 15;

	buf[6] = (nick->user && UPMReply(nick->user)) ? TOKEN_PRIVMSG[0] : TOKEN_NOTICE[0];
	buf[8] = nick->numeric[0];
	buf[9] = nick->numeric[1];
	buf[10] = nick->numeric[2];
	buf[11] = nick->numeric[3];
	buf[12] = nick->numeric[4];

	while((t = *fmt++) && p < end) /* %sa (t = %, *pattern = s) */
	{
		if(t == '%')
		{
			t = *fmt++; /* on drop le formateur (t = s, *pattern = a) */
			if(t == 's')
			{	/* copie de la string */
				register char *tmps = va_arg(vl, char *);
				while(*tmps) *p++ = *tmps++;
				continue;
			}
			if(t == 'd')
			{
				int tmpi = va_arg(vl, int);
				char bufi[32];
				unsigned int pos = sizeof bufi - 1;

				if(tmpi <= 0)
				{
					if(!tmpi)
					{
						*p++ = '0';
						continue;
					}
					*p++ = '-';
					tmpi = -tmpi;
				}
				while(tmpi) /* on converti une int en base 10 en string */
				{		/* écriture dans l'ordre inverse 51 > '   1' > '  51'*/
					bufi[pos--] = '0' + (tmpi % 10);
					tmpi /= 10;
				}
				while(pos < sizeof bufi -1 && p < end) *p++ = bufi[++pos];
				continue;
			}
			if(t == 'u')
			{
				unsigned int tmpi = va_arg(vl, unsigned int);
				char bufi[32];
				unsigned int pos = sizeof bufi - 1;

				if(!tmpi)
				{
					*p++ = '0';
					continue;
				}
				while(tmpi) /* on converti une int en base 10 en string */
				{		/* écriture dans l'ordre inverse 51 > '   1' > '  51'*/
					bufi[pos--] = '0' + (tmpi % 10);
					tmpi /= 10;
				}
				while(pos < sizeof bufi -1 && p < end) *p++ = bufi[++pos];
				continue;
			}
			if(t == 'c')
			{
				*p++ = (char) va_arg(vl, int);
				continue;
			}
			if(t != '%')
			{	/* on sous traite le reste à vsnprintf (-2 because of the %*) */
				size_t i = vsnprintf(p, end - p, fmt - 2, vl);
				p += i < end - p ? i : end - p;
				break;
			}
		}
		*p++ = t;
	}
	*p++ = '\r';
	*p++ = '\n';
	*p = '\0';

	len = p - end + 509;
	send(bot.sock, buf, len, 0);
	bot.dataS += len;
	va_end(vl);
	return 0;
}

void putchan(const char *buf)
{
	char fmt[] = "%s " TOKEN_PRIVMSG " %s :[  :  :  ] %s";
	time_t tmt = CurrentTS;
	struct tm *ntime = localtime(&tmt);

	fmt[10] = (ntime->tm_hour / 10) + '0';
	fmt[11] = (ntime->tm_hour % 10) + '0';
	fmt[13] = (ntime->tm_min / 10) + '0';
	fmt[14] = (ntime->tm_min % 10) + '0';
	fmt[16] = (ntime->tm_sec / 10) + '0';
	fmt[17] = (ntime->tm_sec % 10) + '0';

	putserv(fmt, cs.num, bot.pchan, buf);
}

void csmode(aChan *chan, int type, const char *fmt, ...)
{
	char buf[512], mode[50] = {0}, *key = NULL, *limit = NULL;
	char *tmp = buf, *last = NULL, *tmp2 = NULL;
	unsigned int w = 1, cur = 0;
	va_list vl;

	if(!CJoined(chan)) return; /* Not on chan... */

	va_start(vl, fmt);
	if(type & MODE_ALLFMT) myvsnprintf(buf, sizeof buf, fmt, vl);
	else fastfmtv(buf, fmt, vl);
	va_end(vl);

	putserv("%s " TOKEN_MODE " %s %s", cs.num, chan->chan, buf);

	if(type & MODE_OBV) return; /* modes given are only obv, nothing more to do */

	for(Strtok(&tmp2, tmp, ' '); *tmp && cur < sizeof mode; ++tmp)
	{ 	/* parse modes, filter out +obv and get key/limit index */
		switch(*tmp)
		{
			case 'o': case 'v': case 'b': /* ignore +ovb */
				Strtok(&tmp2, NULL, ' ');
				break;
			case 'k':
			    last = Strtok(&tmp2, NULL, ' ');
			    /* keep only first key if several given */
				if(!key && last) key = last;
				mode[cur++] = *tmp;
				break;
			case 'l':
				if(w)
				{
					last = Strtok(&tmp2, NULL, ' ');
					if(!limit && last) limit = last;
				}
				mode[cur++] = *tmp;
				break;
			case '-': w = 0; mode[cur++] = *tmp; break;
			case '+': w = 1; mode[cur++] = *tmp; break;
			default:
				mode[cur++] = *tmp;	break;
		}
	}

	string2scmode(&chan->netchan->modes, mode, key, limit);
}

void cstopic(aChan *chan, const char *topic)
{
	if(!chan->netchan) return;
	putserv("%s " TOKEN_TOPIC " %s :%s", cs.num, chan->chan, topic);
	strcpy(chan->netchan->topic, topic);
}

void cskick(const char *chan, const char *vict, const char *fmt, ...)
{
	char buf[512];
	va_list vl;

	va_start(vl, fmt);
	fastfmtv(buf, fmt, vl);
	va_end(vl);
	putserv("%s " TOKEN_KICK " %s %s :%s", cs.num, chan, vict, buf);
}

void cswallops(const char *pattern, ...)
{
	char buf[512];
	va_list vl;

	va_start(vl, pattern);
	myvsnprintf(buf, sizeof buf, pattern, vl);
	va_end(vl);
	putserv("%s " TOKEN_WALLOPS " :%s", cs.num, buf);
}

void csjoin(aChan *c, int flag)
{
	aNChan *netchan = c->netchan ? c->netchan : GetNChan(c->chan);

	if(!netchan)
	{
		if(!(flag & JOIN_FORCE)) return;  /* chan is empty and nojoin is specified. */
		netchan = new_chan(c->chan, CurrentTS);
		flag |= JOIN_CREATE;
	}
	netchan->regchan = c;
	if(!c->netchan) c->netchan = netchan;

 	if(flag & JOIN_CREATE) /* channel does not exist on net */
 	{
		putserv("%s " TOKEN_CREATE " %s %T", cs.num, c->chan, CurrentTS);
		if(c->defmodes.modes)
			putserv("%s "TOKEN_MODE" %s +%s", cs.num, c->chan, GetCModes(c->defmodes));
	}
	else /* auto op Create or Join/+o */
	{
		MBuf def_buf, net_buf;

		putserv("%s " TOKEN_JOIN " %s", cs.num, c->chan);
		CModes2MBuf(&c->defmodes, &def_buf, 0);

		if((netchan->modes.modes &= ~c->defmodes.modes)) /* net modes != defmodes */
		{
			CModes2MBuf(&netchan->modes, &net_buf, MBUF_NOLIMIT);
			putserv("%s "TOKEN_MODE" %s +o%s-%s %s %s %s", OPPREFIX, c->chan,
				def_buf.modes, net_buf.modes, cs.num, def_buf.param, net_buf.param);
		}
		else putserv("%s "TOKEN_MODE" %s +o%s %s %s", OPPREFIX, c->chan,
				def_buf.modes, cs.num, def_buf.param);
	}
	do_cs_join(c, netchan, JOIN_TOPIC);
	netchan->modes = c->defmodes;
}

void cspart(aChan *chan, const char *raison)
{
	if(chan->fltimer)
	{
		timer_remove(chan->fltimer);
		chan->fltimer = NULL;
	}
	if(!chan->netchan->users) del_nchan(chan->netchan); /* I was the last member */
	putserv("%s "TOKEN_PART" %s :%s", cs.num, chan->chan, NONE(raison));
	DelCJoined(chan);
}

void cs_account(aNick *nick, anUser *user)
{
	if(!user && nick->user) /* update forward ptr (user->n) if desaccounting.. */
	{
		anAccess *a = nick->user->accesshead;
		for(; a; a = a->next)
			if(AOnChan(a)) a->lastseen = CurrentTS;

		if(nick->user->n == nick) nick->user->n = NULL;
		if(IsAdmin(nick->user)) adm_active_del(nick); /* got deauthed */
		nick->user->lastseen = CurrentTS; /* last auth */
		nick->user = NULL;
	}
	if(user && nick->user != user) /* only update if he wasn't loggued or on different user */
	{
		nick->user = user;
		user->n = nick;
	}

	putserv("%s " TOKEN_ACCOUNT " %s %s", bot.servnum, nick->numeric, user ? user->nick : "");
}
