/* src/admin_manage.c - commandes pr gerer les admins
 *
 * Copyright (C) 2002-2006 David Cortier  <Cesar@ircube.org>
 *                         Romain Bignon  <Progs@ir3.org>
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
 * $Id: admin_manage.c,v 1.25 2006/09/12 20:27:22 romexzf Exp $
 */

#include "main.h"
#include "add_info.h"
#include "outils.h"
#include "cs_cmds.h"
#include "hash.h"
#ifdef HAVE_TRACK
#include "track.h"
#endif

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

#ifdef HAVE_TRACK
	track_admin_quit(nick);
#endif

	while(i < adminmax && adminlist[i] != nick) ++i; /* searching in list.. */
	if(i < adminmax) adminlist[i] = NULL; /* free the slot */
	return 0;
}

/*
 * admin_level <username> <niveau>
 */

int admin_level(aNick *nick, aChan *chan, int parc, char **parv)
{
	anUser *user;
	int level;
#ifdef USE_MEMOSERV
	char memo[MEMOLEN + 1] = {0};
#endif

	if(!Strtoint(parv[2], &level, 1, MAXADMLVL))
		return csreply(nick, GetReply(nick, L_VALIDLEVEL));

	if(!(user = getuserinfo(parv[1])))
		return csreply(nick, GetReply(nick, L_NOSUCHUSER), parv[1]);

	if(user->level >= nick->user->level && nick->user->level != MAXADMLVL)
		return csreply(nick, GetReply(nick, L_GREATERLEVEL), user->nick);

	if(level >= nick->user->level && nick->user->level != MAXADMLVL)
		return csreply(nick, GetReply(nick, L_MAXLEVELISYOURS));

	if(level == user->level)
		return csreply(nick, "%s est déjà au niveau %d.", user->nick, level);

	if(level < ADMINLEVEL)
	{
		csreply(nick, "%s n'est plus Administrateur.", user->nick);
#ifdef USE_MEMOSERV
		if(!user->n) mysnprintf(memo, MEMOLEN, "Votre accès Administrateur a été supprimé par %s.",
						nick->user->nick);
#endif
		if(user->n) adm_active_del(user->n);
	}
	else if(user->level < ADMINLEVEL)
	{
		csreply(nick, "%s est maintenant Administrateur au niveau\2 %d\2.", user->nick, level);
#ifdef USE_MEMOSERV
		if(!user->n) mysnprintf(memo, MEMOLEN, "%s vous a nommé Administrateur.", nick->user->nick);
#endif
		if(user->n) adm_active_add(user->n);
	}
	else csreply(nick, "Vous avez modifié le niveau Administrateur de \2%s\2 en\2 %d\2.",
			user->nick, level);

#ifdef USE_MEMOSERV
	if(!user->n && *memo) add_memo(user, "Services", CurrentTS, memo, MEMO_AUTOEXPIRE);
#endif
	user->level = level;
	return 1;
}
