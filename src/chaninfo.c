/* src/chaninfo.c - Affiche les informations à propos d'un salon
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
 * $Id: chaninfo.c,v 1.51 2008/01/04 13:21:34 romexzf Exp $
 */

#include "main.h"
#include "outils.h"
#include "ban.h"
#include "admin_chan.h"
#include "cs_cmds.h"
#include "hash.h"
#include "data.h"
#include "chaninfo.h"

static char *GetChanOptions(aChan *chan)
{
	static char coptions[10+6+9+7+9+8+11+7+7+8+14+20+1];
	int i = 0;
	coptions[0] = 0;

	if(CLockTopic(chan)) strcpy(coptions, " locktopic"), i += 10;
	if(CNoOps(chan)) strcpy(coptions + i, " noops"), i += 6;
	if(CNoVoices(chan)) strcpy(coptions + i, " novoices"), i += 9;
	if(CNoBans(chan)) strcpy(coptions + i, " nobans"), i += 7;
	if(CStrictOp(chan)) strcpy(coptions + i, " strictop"), i += 9;
	if(CSetWelcome(chan)) strcpy(coptions + i, " welcome"), i += 8;
	if(CAutoInvite(chan)) strcpy(coptions + i, " autoinvite"), i += 11;
	if(CWarned(chan)) strcpy(coptions + i, " warned"), i += 7;
	if(CNoInfo(chan)) strcpy(coptions + i, " noinfo"), i += 7;
	if(chan->flag & C_ALREADYRENAME) strcpy(coptions + i, " renommé"), i += 8;
	if(CFLimit(chan)) mysnprintf(coptions + i, sizeof coptions - i, " auto-limit(%u,%u)",
						chan->limit_inc, chan->limit_min);
	if(coptions[0] == 0) strcpy(coptions, " Aucune");

	return coptions;
}

/*
 * chaninfo #salon
 */
int chaninfo(aNick *nick, aChan *chan, int parc, char **parv)
{
	aNChan *netchan = chan->netchan;
	int can_see = (nick->user && (IsAdmin(nick->user) || GetAccessIbyUserI(nick->user, chan)));

	if(netchan && HasMode(netchan, C_MSECRET | C_MPRIVATE) /* salon +s ou +p */
		&& !can_see && !GetJoinIbyNC(nick, netchan)) /* ET ni admin/accès, ni présent */
			return csreply(nick, GetReply(nick, L_YOUNOACCESSON), chan->chan);

	csreply(nick, GetReply(nick, L_INFO_ABOUT), parv[1]);
	csreply(nick, GetReply(nick, L_CIOWNER), chan->owner ? chan->owner->user->nick : "Aucun",
		UNDEF(chan->creation_time));
	csreply(nick, GetReply(nick, L_CIDESCRIPTION), chan->description);

	if(can_see || !HasDMode(chan, C_MKEY))
		csreply(nick, GetReply(nick, L_CIDEFMODES), GetCModes(chan->defmodes));

	csreply(nick, GetReply(nick, L_CIDEFTOPIC), chan->deftopic);
	if(can_see && chan->motd) csreply(nick, GetReply(nick, L_CIMOTD), chan->motd);
	csreply(nick, GetReply(nick, L_CICHANURL), chan->url);

	csreply(nick, GetReply(nick, L_CIOPTIONS), GetChanOptions(chan));
	csreply(nick, GetReply(nick, L_CIBANLVL), chan->banlevel, GetBanType(chan));

	csreply(nick, GetReply(nick, L_CICHMODESLVL), chan->cml,
			chan->bantime ? duration(chan->bantime) : "Aucun");

	if(CSetWelcome(chan) && *chan->welcome)
		csreply(nick, GetReply(nick, L_CIWELCOME), chan->welcome);

	if(IsAnAdmin(nick->user)) show_csuspend(nick, chan);

	if(netchan)
	{
		csreply(nick, GetReply(nick, L_CIACTUALTOPIC), netchan->topic);
		if(can_see || !HasMode(netchan, C_MKEY) || GetJoinIbyNC(nick, netchan))
			csreply(nick, GetReply(nick, L_CIACTUALMODES),
				netchan->modes.modes ? GetCModes(netchan->modes) : "Aucun");
		if(IsAnAdmin(nick->user)) whoison(nick, chan, parc, parv);
	}

	return 1;
}
