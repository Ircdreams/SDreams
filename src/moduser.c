/* src/moduser.c - Modification des paramètres d'un accès
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
 * $Id: moduser.c,v 1.25 2006/03/16 07:01:03 bugs Exp $
 */

#include "main.h"
#include "outils.h"
#include "cs_cmds.h"
#include "fichiers.h"
#include "hash.h"
#include "config.h"

/*
 * generic_moduser #salon option nick argument
 */
int generic_moduser(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	anUser *user;
	anAccess *access;
	const char *c = parv[1];
	int nfrom = ChanLevelbyUserI(nick->user, chaninfo); /* level du requérant*/

	if(!(user = ParseNickOrUser(nick, parv[3]))) return 0;

	if(!(access = GetAccessIbyUserI(user, chaninfo)))
		return csreply(nick, GetReply(nick, L_XNOACCESSON), user->nick, c);

	if(user != nick->user && !IsAdmin(nick->user) && nfrom <= access->level)
		return csreply(nick, GetReply(nick, L_GREATERLEVEL), user->nick);

	if(!strcasecmp(parv[2], "protect"))
		switch_option(nick, parc < 4 ? NULL : parv[4], parv[2], user->nick, &access->flag, A_PROTECT);
	
	else if(!strcasecmp(parv[2], "autoop"))
		switch_option(nick, parc < 4 ? NULL : parv[4], parv[2], user->nick, &access->flag, A_OP);
	else if(!strcasecmp(parv[2], "autohop"))
		switch_option(nick, parc < 4 ? NULL : parv[4], parv[2], user->nick, &access->flag, A_HALFOP);
	else if(!strcasecmp(parv[2], "autovoice"))
		switch_option(nick, parc < 4 ? NULL : parv[4], parv[2], user->nick, &access->flag, A_VOICE);

	else if(!strcasecmp(parv[2], "suspend"))
	{
		if(nick->user == user) return csreply(nick, "Vous ne pouvez pas vous suspendre.");
		switch_option(nick, parc < 4 ? NULL : parv[4], parv[2], user->nick, &access->flag, A_SUSPEND);
	}

	else if(!strcasecmp(parv[2], "level"))
	{
		int l;
		if(parc < 4 || !Strtoint(parv[4], &l, 1, OWNERLEVEL-1))
			return csreply(nick, GetReply(nick, L_VALIDLEVEL));

		if(AOwner(access))
			return csreply(nick, GetReply(nick, L_XISOWNER), user->nick);

		if(l >= nfrom && !IsAdmin(nick->user))
			return csreply(nick, GetReply(nick, L_MAXLEVELISYOURS));

		access->level = l;
		csreply(nick, GetReply(nick, L_LEVELCHANGED), user->nick, l, access->c->chan);
	}

	else if(!strcasecmp(parv[2], "info"))
	{
		if(parc < 4) return csreply(nick, GetReply(nick, L_INFOLINEORNONE));

		if(!strcasecmp(parv[4], "none"))
		{
			if(access->info)
			{
				free(access->info);
				access->info = NULL;
			}
			csreply(nick, GetReply(nick, L_OKDELETED), user->nick);
		}
		else
		{
			str_dup(&access->info, parv2msg(parc, parv, 4, 199));
			csreply(nick, GetReply(nick, L_INFOCHANGED), user->nick, access->info);
		}
	}

	else if(*parv[2] == '+' || *parv[2] == '-')
	{
		int w = 1;
		/* change flag +flag-flag*/
#define TOGGLE_FLAG(cflag) do { if(w) access->flag |= cflag; else access->flag &= ~cflag; } while(0)
		while(*parv[2])
		{
			switch(*parv[2])
			{
				case '+': w = 1; break;
				case '-': w = 0; break;
				case 'o': TOGGLE_FLAG(A_OP); break;
				case 'h': TOGGLE_FLAG(A_HALFOP); break;
				case 'p': TOGGLE_FLAG(A_PROTECT); break;
				case 'v': TOGGLE_FLAG(A_VOICE); break;
				case 's':
						if(user == nick->user) csreply(nick, "Vous ne pouvez pas vous suspendre.");
						else TOGGLE_FLAG(A_SUSPEND); /* break in default */
				default: break;
			}
			++parv[2];
		}
		csreply(nick, "Les Options de %s sur %s sont désormais: %s", user->nick,
			c, GetAccessOptions(access));
	}
	else return csreply(nick, GetReply(nick, L_UNKNOWNOPTION), parv[2]);
	return 1;
}


int moduser_protect(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	char *pparv[] = {parv[0], parv[1], "protect", nick->user->nick, parv[2], NULL};

	return generic_moduser(nick, chaninfo, parc + 2, pparv);
}

int moduser_autoop(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	char *pparv[] = {parv[0], parv[1], "autoop", nick->user->nick, parv[2], NULL};

	return generic_moduser(nick, chaninfo, parc + 2, pparv);
}

int moduser_autohalfop(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
        char *pparv[] = {parv[0], parv[1], "autohop", nick->user->nick, parv[2], NULL};
        
        return generic_moduser(nick, chaninfo, parc + 2, pparv);
}
               
int moduser_autovoice(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	char *pparv[] = {parv[0], parv[1], "autovoice", nick->user->nick, parv[2], NULL};

	return generic_moduser(nick, chaninfo, parc + 2, pparv);
}
