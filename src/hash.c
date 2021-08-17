/* src/hash.c dohash()
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
 * $Id: hash.c,v 1.25 2007/01/02 19:44:30 romexzf Exp $
 */

#include "main.h"
#include "hash.h"

int ChanLevelbyUserI(anUser *user, aChan *chan)
{
	anAccess *acces = GetAccessIbyUserI(user, chan);
	return acces ? acces->level : 0;
}

aJoin *getjoininfo(aNick *nick, const char *chan)
{
	register aJoin *join = nick->joinhead;
	for(; join && strcasecmp(chan, join->chan->chan); join = join->next);
	return join;
}

aJoin *GetJoinIbyNC(aNick *nick, aNChan *chan)
{
	register aJoin *join = nick->joinhead;
	for(; join && chan != join->chan; join = join->next);
	return join;
}

char *IsAnOwner(anUser *user)
{
	anAccess *acces = user->accesshead;
	for(; acces && !AOwner(acces); acces = acces->next);
	return acces ? acces->c->chan : NULL;
}

anAccess *GetAccessIbyUserI(anUser *user, aChan *chan)
{
	register anAccess *a = user->accesshead;
	for(; a && a->c != chan; a = a->next);
	return (a && !AWait(a)) ? a : NULL;
}

aNick *GetMemberIbyNick(aChan *chan, const char *nick)
{
	aLink *l = chan->netchan->members;
	for(; l && strcasecmp(l->value.j->nick->nick, nick); l = l->next);
	return l ? l->value.j->nick : NULL;
}

aServer *GetLinkIbyServ(const char *serv)
{
	int i = 0;
	for(; i < MAXNUM; ++i)
		if(serv_tab[i] && !strcasecmp(serv_tab[i]->serv, serv)) return serv_tab[i];
	return NULL;
}

char *GetAccessOptions(anAccess *acces)
{
	static char options[7+8+10+9+1];
	int i = 0;
	options[0] = '\0';

	if(AOp(acces)) strcpy(options, " autoop"), i += 7;
	if(AProtect(acces)) strcpy(options + i, " protégé"), i += 8;
	if(AVoice(acces)) strcpy(options + i, " autovoice"), i += 10;
	if(ASuspend(acces)) strcpy(options + i, " suspendu");
	if(options[0] == '\0') strcpy(options, " Aucune");
	return options;
}
