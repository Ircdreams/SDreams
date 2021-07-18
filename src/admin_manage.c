/* src/admin_manage.c - commandes pr gerer les admins
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
 * $Id: admin_manage.c,v 1.25 2006/03/15 17:36:47 bugs Exp $
 */

#include "main.h"
#include "add_info.h"
#include "outils.h"
#include "cs_cmds.h"
#include "fichiers.h"
#include "hash.h"
#include "track.h"
#include "config.h"
#include "divers.h"
#include "template.h"

aNick **adminlist = NULL; 
int adminmax = 0; 
    
/* Try to insert a new admin in list, otherwise append it. */ 
int adm_active_add(aNick *nick) 
{ 
        int i = 0; 
 
        while(i < adminmax && adminlist[i]) ++i; /* look for a free slot */ 
        /* can't find one, make the list grow up */ 
        if(i >= adminmax) adminlist = realloc(adminlist, sizeof *adminlist * ++adminmax); 
        adminlist[i] = nick; /* insert it into the free slot */ 
        return adminmax; 
} 
    
/* Try to find admin in list and empties the slot */ 
int adm_active_del(aNick *nick) 
{ 
        int i = 0; 

	struct track *track = trackhead, *tt = NULL;
	if(GetConf(CF_TRACKSERV)) {
		for(;track;track = tt)
		{
			tt = track->next;
			if(nick == track->tracker) del_track(track);
		}
    	}
        while(i < adminmax && adminlist[i] != nick) ++i; /* searching in list.. */ 
        if(i < adminmax) adminlist[i] = NULL; /* free the slot */ 
        return 0; 
} 

/*
 * admin_level <username> <niveau>
 */

int admin_level(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	anUser *user;
	int level;
	char memo[MEMOLEN + 1];

	if(!Strtoint(parv[2], &level, 1, MAXADMLVL))
		return csreply(nick, GetReply(nick, L_VALIDLEVEL));

	if(!(user = getuserinfo(parv[1])))
		return csreply(nick, GetReply(nick, L_NOSUCHUSER), parv[1]);

	if(level >= nick->user->level && nick->user->level != MAXADMLVL)
		return csreply(nick, GetReply(nick, L_GREATERLEVEL));

	if(level == user->level)
		return csreply(nick, "%s est déjà au niveau %d.", user->nick, level);
	
	/* beug reporté par FuNMasteR:
           correction : en cas de changement de level de la commande LEVEL
           la commande n'est plus libre de droit !
        */
        if(level <= nick->user->level && nick->user->level <= user->level && nick->user->level != MAXADMLVL)
                	return csreply(nick, GetReply(nick, L_MAXLEVELISYOURS), user->nick);

	if(level < HELPLEVEL && user->level == 2)
	{
		csreply(nick, "%s n'est plus Helpeur.", user->nick);
		if(user->n) {
			putserv("%s " TOKEN_SVSMODE" %s :-A", cs.num, user->n->numeric);
			csreply(user->n, "Votre accès Helpeur a été supprimé par %s.", nick->user->nick);
		}
		snprintf(memo, MEMOLEN, "Votre accès Helpeur a été supprimé par %s.", nick->user->nick);
	}

	else if(level < ADMINLEVEL && user->level > 2 && level != 2)
	{
		csreply(nick, "%s n'est plus Administrateur.", user->nick);
		if(user->n) {
			adm_active_del(user->n);
			csreply(user->n, "Votre accès Administrateur a été supprimé par %s.", nick->user->nick);
		}
		snprintf(memo, MEMOLEN, "Votre accès Administrateur a été supprimé par %s.", nick->user->nick);
	}
	else if (level == HELPLEVEL)
        {
                csreply(nick, "%s est maintenant helpeur de niveau \2%d\2.", user->nick, level);
		if(user->n) {
			putserv("%s " TOKEN_SVSMODE" %s :+A", cs.num, user->n->numeric);
			csreply(user->n, "%s vous a nommé Helpeur.", nick->user->nick);
		}
		snprintf(memo, MEMOLEN, "%s vous a nommé Helpeur.", nick->user->nick);
        }

	else if(user->level < ADMINLEVEL)
	{
		csreply(nick, "%s est maintenant Administrateur au niveau\2 %d\2.", user->nick, level);
		if(user->n) {
			if(user->level == HELPLEVEL) putserv("%s " TOKEN_SVSMODE" %s :-A", cs.num, user->n->numeric); /* si le gars était un helper */
			adm_active_add(user->n);
			csreply(user->n, "%s vous a nommé Administrateur.", nick->user->nick);
		}
		snprintf(memo, MEMOLEN, "%s vous a nommé Administrateur.", nick->user->nick);
	}
	else {
		csreply(nick, "Vous avez modifié le niveau Administrateur de \2%s\2 en\2 %d\2.",
			user->nick, level);
		if(user->n) csreply(user->n, "%s a modifié votre niveau Administrateur de \2%d\2 en \2%d\2.",
                        nick->user->nick, user->level, level);
		snprintf(memo, MEMOLEN, "%s a modifié votre niveau Administrateur de %d en %d.",
			nick->user->nick, user->level, level);
	}
	if(GetConf(CF_MEMOSERV)){
		if(!UNoMail(user)) tmpl_mailsend(&tmpl_mail_memo, user->mail, user->nick, NULL, NULL, cs.nick, memo);
		if(!user->n) add_memo(user, cs.nick, CurrentTS, memo, MEMO_AUTOEXPIRE);
	}
	user->level = level;
	return 1;
}
