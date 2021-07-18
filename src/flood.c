/* src/flood.c - protection contre le flood
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
 * $Id: flood.c,v 1.11 2006/03/15 19:04:42 bugs Exp $
 */

#include "main.h"
#include "cs_cmds.h"
#include "config.h"
#include "timers.h"
#include "hash.h"
#include "outils.h"
#include "flood.h"
#include "debug.h"

static void add_ignore(const char *mask) 
{ 
        struct ignore *i = malloc(sizeof *i);

        if(!i)
        { 
                Debug(W_MAX|W_WARN, "add_ignore, malloc a échoué pour l'ignore de %s", mask); 
                return; 
        } 
    
        strcpy(i->host, mask); 
        i->expire = CurrentTS + ignoretime; 
        timer_add(i->expire, TIMER_ABSOLU, callback_delignore, i, NULL); 
    
        i->last = NULL; 
        if(ignorehead) ignorehead->last = i; 
        i->next = ignorehead; 
        ignorehead = i; 
} 

/*Renvoie 1 si un flood est detecté*/
int checkflood(aNick *nick)
{

	if(GetConf(CF_ADMINEXEMPT) && IsAnAdmin(nick->user)) return 0;

	if(nick->floodtime + FLOODTIME < CurrentTS) /*le privmsg precedent date de trop longtemps*/
	{
		nick->floodtime = CurrentTS; /*on met a jour la variable du dernier privmsg*/
		nick->floodcount = 1; /*on remet a 1 le nombre de lignes*/
	}
	else /*le privmsg est assez recent pour etre controlé*/
	{
		if(nick->floodcount >= FLOODLINES) /*flood detecté !*/
		{
			add_ignore(nick->base64);
			cswall("Flood de %s%s (%s@%s)", GetPrefix(nick), nick->nick, nick->ident, nick->host);
			if(GetConf(CF_KILLFORFLOOD)) 
                        { 
				putserv("%s "TOKEN_KILL" %s :%s (%s)", cs.num, nick->numeric,
                                        cs.nick, GetReply(nick, L_FLOODMSG));
                                del_nickinfo(nick->numeric, "Flood kill");
                        }
			else csreply(nick, GetReply(nick, L_FLOODMSG));
			return 1;
		}
		else ++nick->floodcount;
	}
	return 0;
}

int isignore(aNick *nick)
{
	struct ignore *i = ignorehead;

	for(;i && strcmp(i->host, nick->base64);i = i->next);
	return i ? 1 : 0;
}

int callback_delignore(Timer *timer)
{ 
        struct ignore *i = timer->data1; 
    
        if(i->next) i->next->last = i->last; 
        if(i->last) i->last->next = i->next; 
        else ignorehead = i->next; 
        free(i); 
        return 1; 
}
