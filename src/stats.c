/* src/stats.c - commande stats
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
 * $Id: stats.c,v 1.8 2006/02/18 07:08:44 bugs Exp $
 */

#include <ctype.h>
#include "main.h"
#include "debug.h"
#include "cs_cmds.h"
#include "hash.h"
#include "outils.h"
#include "stats.h"
#include "add_info.h"
#include "del_info.h"
#include "fichiers.h"
#include "aide.h"
#include "divers.h"
#include "admin_user.h"
#include "config.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int stats(aNick *nick, aChan *chan, int parc, char **parv)
{
	anUser *u = NULL;
	aNick *n;

	int memn = getoption("mem", parv, parc, 1, -1);
	int cmds = getoption("cmds", parv, parc, 1, 0);
	int traffic = getoption("traffic", parv, parc, 1, -1);
	int all = getoption("all", parv, parc, 1, -1);

	if(memn || all)
	{ 
		aBan *ban;
		aNChan *nchan;
		aLink *lp;
		aMemo *m;
		int nick_count = 0, user_count = 0, suspend_count = 0, chan_count = 0,
			nchan_count = 0, member_count = 0, auth_count = 0, memo_count = 0,
			longest, i, used, list_size, ma, mb, maxbans, maxaccess, mem = 0;
		if(!(maxbans = getoption("-maxbans", parv, parc, 2, 1))) maxbans = WARNACCESS;
		if(!(maxaccess = getoption("-maxaccess", parv, parc, 2, 1))) maxaccess = WARNACCESS;
		uptime(nick, chan, parc, parv);

#define show_hash_stat(type, size, count) 	csreply(nick, "Hash "type": Offsets (Utilisés " \
	"/ Dispos) %d / %d (Max. %d Av. %.3f)", used, size, longest, (double) count / used)
		for(i = 0, longest = 0, used = 0;i < NICKHASHSIZE;++i)
		{
			for(list_size = 0, n = nick_tab[i];n;n = n->next, ++list_size);
			if(list_size > longest) longest = list_size;
			if(list_size) ++used;
			nick_count += list_size;
		}
		show_hash_stat("Nick", NICKHASHSIZE, nick_count);
		for(i = 0, longest = 0, used = 0;i < NCHANHASHSIZE;++i)
		{
			for(list_size = 0, nchan = nchan_tab[i];nchan;nchan = nchan->next, ++list_size)
				member_count += nchan->users;
			nchan_count += list_size;
			if(list_size > longest) longest = list_size;
			if(list_size) ++used;
		}
		show_hash_stat("NetChan", NCHANHASHSIZE, nchan_count);

		for(i = 0, longest = 0, used = 0;i < USERHASHSIZE;++i)
		{
			for(list_size = 0, u = user_tab[i];u;u = u->next, ++list_size)
			{
				for(m = u->memohead; m; m = m->next) ++memo_count;
				if(u->suspend) ++suspend_count;
				if(u->lastlogin) mem += strlen(u->lastlogin);
				if(u->n) ++auth_count;
			}
			if(list_size > longest) longest = list_size;
			if(list_size) ++used;
			user_count += list_size;
		}
		show_hash_stat("User", USERHASHSIZE, user_count);

		for(i = 0, longest = 0, used = 0;i < CHANHASHSIZE;++i)
		{
			for(list_size = 0, chan = chan_tab[i];chan;chan = chan->next, ++list_size)
			{
				if(chan->suspend) ++suspend_count;
				if(chan->motd) mem += strlen(chan->motd);

				for(mb = 0, ban = chan->banhead;ban;ban = ban->next, ++mb)
					if(ban->raison) mem += strlen(ban->raison);
				if(mb >= maxbans)
					csreply(nick, "Warning Bans Limite: \2\00304%d\2\3 Bans sur %s", mb, chan->chan);

				for(ma = 0, lp = chan->access;lp;lp = lp->next, ++ma)
					if(lp->value.a->info) mem += strlen(lp->value.a->info);
				if(ma >= maxaccess)
					csreply(nick, "Warning Acces Limite: \2\00304%d\2\3 Accès sur %s", ma, chan->chan);
				mem += ma * (sizeof(anAccess) + sizeof(aLink)) + mb * sizeof(aBan);
			}
			if(list_size > longest) longest = list_size;
			if(list_size) ++used;
			chan_count += list_size;
		}
		show_hash_stat("Chan", CHANHASHSIZE, chan_count);

		mem += nick_count * sizeof(aNick) + user_count * sizeof(anUser) + chan_count
			* sizeof(aChan) + nchan_count * sizeof(aNChan) + member_count *
			(sizeof(aLink) + sizeof(aJoin))	+ memo_count * sizeof(aMemo)
			+ suspend_count * sizeof(struct suspendinfo) + (NCHANHASHSIZE + MAXNUM
				+ USERHASHSIZE + CHANHASHSIZE + NICKHASHSIZE + CMDHASHSIZE) * sizeof(void *);

		csreply(nick, "Memoire Utilisée pour les Users/Chans: %.3f KB", mem / 1024.0);
		csreply(nick, "Il y a %d Usernames et %d Chans enregistrés.", user_count, chan_count);
		csreply(nick, "%d salons formés, %d Chatteurs connectés, dont %.1f %% logués.",
			nchan_count, nick_count, (double) auth_count / nick_count * 100);
	}
	if(cmds || all)
	{
		int x = 0, adm = 0, usr = 0, help = 0;
		aHashCmd *cmd;

		for(; x < CMDHASHSIZE;++x) for(cmd = cmd_hash[x];cmd;cmd = cmd->next)
		{
			if(AdmCmd(cmd)) adm += cmd->used;
			else if(HelpCmd(cmd)) help += cmd->used;
			else usr += cmd->used;
			if(cmds && *cmd->name != '\1' && !match(parv[cmds], cmd->name))
				csreply(nick, "- %s (\002%d\2)", cmd->name, cmd->used);
		}
		csreply(nick, "Un total de %d commandes utilisées, dont %d Admins (Ratio: %.2f%), %d Helpeurs (Ratio: %.2f%)",
			(adm + usr + help), adm, (adm / (double) (usr + adm + help)) * 100, help, (help / (double) (usr + adm + help)) * 100);
	}
	if(traffic || all)
	{
		double up = bot.dataS/1024.0, down = bot.dataQ/1024.0;
		int mbu = 0, mbd = 0;

		if(up >= 1024) up /= 1024.0, mbu = 1;
		if(down >= 1024) down /= 1024.0, mbd = 1;

		csreply(nick, "Traffic: Up:\2 %.3f %s\2 Down:\2 %.3f %s\2", up, mbu ? "MB":"KB", down, mbd ? "MB":"KB");
		csreply(nick, "Traffic Actuel: %.3f Ko/s", (bot.dataQ - bot.lastbytes) / ((CurrentTS - bot.lasttime) * 1024.0));
		if(GetConf(CF_WEBSERV))
			csreply(nick, "WebServ: Traffic Up:\2 %.3f Ko\2 Down:\2 %.3f Ko\2 dans %d connexions",
				bot.WEBtrafficUP/1024.0, bot.WEBtrafficDL/1024.0, bot.CONtotal);		
	}
	if(!all && !memn && !traffic && !cmds) {
			csreply(nick, "Option inconnue.");
			csreply(nick, "Option disponible: ALL / MEM / CMDS <commande> / TRAFFIC / SERV");
	}
	return 1;
}
