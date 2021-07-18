/* src/memoserv.c - Diverses commandes sur le module memoserv
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
 * $Id: memoserv.c,v 1.28 2006/03/29 15:45:40 bugs Exp $
 */

#include <errno.h>
#include "debug.h"
#include "main.h"
#include "memoserv.h"
#include "fichiers.h"
#include "aide.h"
#include "hash.h"
#include "outils.h"
#include "cs_cmds.h"
#include "add_info.h"
#include "del_info.h"
#include "config.h"
#include "divers.h"
#include "template.h"

void show_notes(aNick *nick)
{
	aMemo *memo = nick->user->memohead;
	int i = 0, count = 0;

	for(;memo;memo = memo->next)
	{
		if(!MemoRead(memo)) ++i;
		++count;
	}

	if(i) csreply(nick, GetReply(nick, L_HAVENEWMEMO), i, PLUR(i), cs.nick, RealCmd("memo"), PLUR(i));
	if(count >= WARNMEMO) csreply(nick, "ATTENTION: Votre Boite a MEMOS est pleine! (%d/%d mémos). Veuillez en supprimer pour en recevoir des nouveaux.", count , WARNMEMO); 
}

int memo_del(anUser *user, int *todel, int i) 
{ 
        aMemo *memo = user->memohead, *mnext, *mlast = NULL; 
        int p = 0, found = 0, tmp = 0; 
    
        for(;memo;memo = mnext) 
        { 
                mnext = memo->next; 
                ++p; 
                if(!i) free(memo), ++found; 
                else if((tmp = item_isinlist(todel, i, p)) != -1) /* need to be deleted */ 
                { 
                        if(mlast) mlast->next = mnext; 
                        else user->memohead = mnext; 
                        todel[tmp] = -todel[tmp]; /* mark as deleted */ 
                        free(memo); 
                        if(i == ++found) break; /* found all memos in list, drop! */ 
                } 
                else mlast = memo; 
        } 
        if(!i && found) user->memohead = NULL; /* all */ 
        return found; 
} 

int memos(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	aMemo *memo = nick->user->memohead, *mnext, *mlast = NULL;
	int i = 0, p = 0, found = 0, nb = 0;
	const char *cmd = parv[1];

	if(!strcasecmp(cmd, "LIRE") || !strcasecmp(cmd, "READ"))
	{	/* default (no arg) is NEW. */
		if(parc < 2 || !strcasecmp(parv[2], "NEW")) i = -2;
		else if(!strcasecmp(parv[2], "ALL")) i = -1;
		else if(!(i = strtol(parv[2], NULL, 10)))
			return csreply(nick, "Syntaxe : %s LIRE <ALL|NEW|N° DU MEMO>", parv[0]);

		for(;memo;memo = mnext)
		{
			mnext = memo->next;
			if(++p == i || i == -1 || (i == -2 && !MemoRead(memo)))
			{
				csreply(nick, "%c#%d De\2 %s\2 [%s] : %s", MemoRead(memo) ? ' ' : '*',
					p, memo->de, get_time(nick, memo->date), memo->message); 
                                found = 1; 
                                if(memo->flag & MEMO_AUTOEXPIRE) 
				{
					if(mlast) mlast->next = mnext;
					else nick->user->memohead = mnext;
					free(memo);
					continue; /* do not update mlast */
				}
				if(!MemoRead(memo)) memo->flag |= MEMO_READ;
                        } 
                        mlast = memo; 
		}
		if(found)
                {
                        if(i == -1) csreply(nick, GetReply(nick, L_MEMOTOTAL), p, PLUR(p), cs.nick, parv[0]);
                        else if(i == -2) csreply(nick, GetReply(nick, L_MEMONEWTOTAL), p, PLURX(p), PLUR(p));
                        else csreply(nick, GetReply(nick, L_MEMOEND), i, cs.nick, parv[0], i);
                }
                else if(i == -1) csreply(nick, GetReply(nick, L_NOMEMOFOUND));
                else if(i == -2) csreply(nick, GetReply(nick, L_MEMONEWNOTFOUND));
                else csreply(nick, GetReply(nick, L_MEMONOTFOUND), i);
	}
	else if(!strcasecmp(cmd, "SUPPR") || !strcasecmp(cmd, "ERASE")
		|| !strcasecmp(cmd, "SUPPRIMER") || !strcasecmp(cmd, "DEL"))
	{
		int todel[10];

		if(parc < 2) return csreply(nick, "Syntaxe : %s SUPPR <ALL|N° DU MEMO>", parv[0]);

		if(strcasecmp(parv[2], "all") && !(i = item_parselist(parv[2], todel, ASIZE(todel))))
			return csreply(nick, "Syntaxe : %s SUPPR <ALL|n°,list>", parv[0]);
		found = memo_del(nick->user, todel, i);
		
		if(!i) csreply(nick, GetReply(nick, found ? L_MEMOSDEL : L_NOMEMOFOUND), found);

		for(p = 0; p < i; ++p) /* report results */
                        if(todel[p] < 0) csreply(nick, GetReply(nick, L_MEMODEL), -todel[p]);
                        else csreply(nick, GetReply(nick, L_MEMONOTFOUND), todel[p]);
	}
	else if(!strcasecmp(cmd, "SEND") || !strcasecmp(cmd, "ENVOYER"))
	{
		anUser *user;
		aNick *who;
		char *tmp;

		if(parc < 3)
			return csreply(nick, "Syntaxe : %s SEND <Pseudo|%%UserName> <message>", parv[0]);

		if(!(user = ParseNickOrUser(nick, parv[2]))) return 0;

		if(user == nick->user)
			return csreply(nick, GetReply(nick, L_CANTSENDYOURSELF));

		if(UNoMemo(user) && !IsAdmin(nick->user))
			return csreply(nick, GetReply(nick, L_XNOMEMO), user->nick);

		tmp = parv2msg(parc, parv, 3, MEMOLEN + 3);

		if(strlen(tmp) > MEMOLEN)
			return csreply(nick, GetReply(nick, L_MEMOLEN), MEMOLEN);
	
		for(memo = user->memohead;memo;memo = memo->next)
		{
			if(!strcasecmp(memo->de, nick->user->nick)) ++i;
			++nb;
		}

		if(i >= MAXMEMOS && !IsAdmin(nick->user))
			return csreply(nick, "Vous avez déjà envoyé suffisament de mémos à %s. (MAX: %d)", user->nick, MAXMEMOS);
				
		if (nb >= WARNMEMO && !IsAdmin(nick->user))
			return csreply(nick, "La boite a MEMO de %s est pleine.", user->nick);

		if((who = user->n) && !IsAway(who))
                        csreply(who, GetReply(who, L_MEMOFROM), nick->nick, nick->user->nick, tmp);

		add_memo(user, nick->user->nick, CurrentTS, tmp, (who && !IsAway(who)) ? MEMO_READ : 0);
		csreply(nick, GetReply(nick, L_MEMOSENT), user->nick);
		csreply(nick, "\2Texte:\2 %s", tmp);
		if(!UNoMail(user)) tmpl_mailsend(&tmpl_mail_memo, user->mail, user->nick, NULL, NULL, nick->user->nick, tmp);
	}
	else return csreply(nick, GetReply(nick, L_UNKNOWNOPTION), cmd);
	return 1;
}

int chanmemo(aNick *nick, aChan *chan, int parc, char **parv)
{
	int i = 0;
	int min_level = getoption("-min", parv, parc, 2, 1);
	int max_level = getoption("-max", parv, parc, 2, 1);
	char memo[MEMOLEN + 1];
	aLink *l;

	for(i = 2; i < parc && *parv[i] == '-'; i += 2);

	if(parc < i)
		return csreply(nick, "Syntaxe: %s #channel [-min level] [-max level] <msg>", parv[0]);

	if(!(chan = getchaninfo(parv[1])))
		return csreply(nick, "\2%s\2 n'est pas un salon enregistré", parv[1]);

	parv2msgn(parc, parv, i, memo, MEMOLEN);

	for(i = 0, l = chan->access; l; l = l->next)
	{
		anAccess *a = l->value.a;
		if(a->user != nick->user && (!min_level || a->level >= min_level)
		   && (!max_level || a->level <= max_level))
		{
			if(!UNoMail(a->user)) tmpl_mailsend(&tmpl_mail_memo, a->user->mail, a->user->nick, NULL, NULL, nick->user->nick, memo);
			if(a->user->n && !IsAway(a->user->n))
				csreply(a->user->n, "\2MEMO Global (%s) de %s:\2 %s", chan->chan, nick->user->nick, memo);
			else add_memo(a->user, nick->user->nick, CurrentTS, memo, MEMO_AUTOEXPIRE);
			++i;
		}
	}
	csreply(nick, "Un memo suivant a été envoyé à %d users: %s", i, memo);
	return 1;
}
