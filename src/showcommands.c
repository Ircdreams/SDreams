/* src/showcommands.c - Liste les commandes
 *
 * Copyright (C) 2002-2008 David Cortier  <Cesar@ircube.org>
 *                         Romain Bignon  <Progs@coderz.info>
 *                         Benjamin Beret <kouak@kouak.org>
 *
 * SDreams v2 (C) 2021 -- Ext by @bugsounet <bugsounet@bugsounet.fr>
 * site web: http://www.ircdreams.org
 *
 * Services pour serveur IRC. Supporté sur Ircdreams v3
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
 * $Id: showcommands.c,v 1.33 2008/01/05 01:24:13 romexzf Exp $
 */

#include "main.h"
#include "outils.h"
#include "cs_cmds.h"
#include "showcommands.h"
#include "hash.h"

static struct scmd_cmds {
	int level;
	int nb;
	char (*buf)[100];
} scmd_user[MAXADMLVL + 1], *scmd_chan = NULL;

static int scmd_chan_c = 0;

#define SHWCMD_MAXLEN 85

/* 	BuildCommandsTable(): construit deux tableaux contenant pour
 *		chaque level (struct.level) la liste des commandes correspondantes
 *		struct.nb contient le nombre d'élement du tableau struct.buf
 *		while(i < struct.nb) printf("level=%d %s\n", struct.level, struct.buf[i++]);
 * => appelée une fois au démarrage, permet des appels 8-10 fois plus rapide
 * => de showcommands puisque le triage n'est plus à faire. -Cesar
 */

int BuildCommandsTable(int rebuild)
{
	int i = 0, level = 0, chan_c = 10;
	aHashCmd *cmd;
	struct scmd_cmds *tmp = NULL;

	if(rebuild) /* reconstruction de la table au runtime, nettoyage de l'ancienne */
	{
		unsigned int j = 0U;
		for(; j < ASIZE(scmd_user); ++j)
		{
			free(scmd_user[j].buf);
			scmd_user[j].nb = 0;
			scmd_user[j].buf = NULL;
		}
		for(i = 0; i <= scmd_chan_c; ++i) free(scmd_chan[i].buf);
		free(scmd_chan);
		scmd_chan = NULL;
	}

	for(i = 0; i < CMDHASHSIZE; ++i) for(cmd = cmd_hash[i]; cmd; cmd = cmd->next)
	{
		level = cmd->level;

		if((ChanCmd(cmd) && !AdmCmd(cmd)) || IsCTCP(cmd) || DisableCmd(cmd)) continue;

		tmp = &scmd_user[level];
		if(!tmp->buf) /* premiere commande de ce level */
		{ 	/* preparons les buffers */
			tmp->nb = 1;
			tmp->buf = malloc(sizeof *tmp->buf);
			tmp->level = snprintf(*tmp->buf, sizeof *tmp->buf, "\2\0033Niveau %3d:\2\3", level);
		}
		else if(tmp->level > SHWCMD_MAXLEN) /* ligne pleine, ajoutons une nouvelle */
		{
			tmp->buf = realloc(tmp->buf, sizeof *tmp->buf * ++tmp->nb);
			strcpy(tmp->buf[tmp->nb-1], "           ");
			tmp->level = 11;
		}
		/* ajout de la commande dans les buffers */
		tmp->level += fastfmt(tmp->buf[tmp->nb-1] + tmp->level, " $", cmd->name);
	} /* for cmds */

	scmd_chan_c = -1;
	scmd_chan = malloc(sizeof *scmd_chan * chan_c);
	scmd_chan[0].level = -1;
	tmp = &scmd_chan[0];
	for(level = 0; level <= OWNERLEVEL;)
	{
		int next_level = OWNERLEVEL;
		size_t size = 0; /* taille de la ligne du buffer courant */

		for(i = 0; i < CMDHASHSIZE; ++i) for(cmd = cmd_hash[i]; cmd; cmd = cmd->next)
		{
			if(ChanCmd(cmd) && cmd->level > level && cmd->level < next_level)
				next_level = cmd->level; /* find out which is the next level with commands */

			if(cmd->level != level || !ChanCmd(cmd) || AdmCmd(cmd)
				|| IsCTCP(cmd) || DisableCmd(cmd)) continue;

			if(tmp->level != level) /* premiere commande de ce level */
			{ 	/* preparons les buffers */
				if(++scmd_chan_c >= chan_c)/* commandes trop réparties */
				{
					chan_c *= 2; /* augmentons le buffer! */
					scmd_chan = realloc(scmd_chan, chan_c * sizeof *scmd_chan);
				}

				tmp = &scmd_chan[scmd_chan_c]; /* realloc hop?! */
				tmp->level = level;
				tmp->nb = 1;
				tmp->buf = malloc(sizeof *tmp->buf);
				size = snprintf(*tmp->buf, sizeof *tmp->buf, "\2\0033Niveau %3d:\2\3", level);
			}
			else if(size > SHWCMD_MAXLEN) /* ligne pleine, ajoutons une nouvelle */
			{
				tmp->buf = realloc(tmp->buf, sizeof *tmp->buf * ++tmp->nb);
				strcpy(tmp->buf[tmp->nb-1], "           ");
				size = 11;
			}
			/* ajout de la commande dans les buffers */
			size += fastfmt(tmp->buf[tmp->nb-1] + size, " $", cmd->name);
		} /* for cmds */
		level = next_level > level ? next_level : level + 1;
	} /* for level */
	return 0;
}

int showcommands(aNick *nick, aChan *chan, int parc, char **parv)
{
	int i, j, maxlevel = 0;

	if(nick->user) /* Cherche le level maximum sur tous ses salons */
	{			/* si un salon est précisé, utilisation du level de celui ci */
		anAccess *acces;
		if(IsAdmin(nick->user) || (parc && !strcasecmp("all", parv[1])))
			maxlevel = OWNERLEVEL;
		else if(parc && *parv[1] == '#')
				maxlevel = ChanLevelbyUserI(nick->user, getchaninfo(parv[1]));
		else
			for(acces = nick->user->accesshead; acces; acces = acces->next)
				if(maxlevel < acces->level) maxlevel = acces->level;
	}

	csreply(nick, GetReply(nick, L_SHOWCMDUSER));
	for(i = 0; i <= (nick->user ? nick->user->level : 0); ++i)
	{
		if(i == HELPLEVEL) csreply(nick, "Helper Commands:");
		if(i == ADMINLEVEL) csreply(nick, GetReply(nick, L_SHOWCMDADMIN));
		for(j = 0; j < scmd_user[i].nb; ++j)
    		csreply(nick, "%s", scmd_user[i].buf[j]);
	}

	csreply(nick, GetReply(nick, L_SHOWCMDCHAN));
	for(i = 0; i <= scmd_chan_c && scmd_chan[i].level <= maxlevel; ++i)
    	for(j = 0; j < scmd_chan[i].nb; ++j)
    		csreply(nick, "%s", scmd_chan[i].buf[j]);

	csreply(nick, GetReply(nick, L_SHOWCMDHELP1), cs.nick, RealCmd("aide"));
	csreply(nick, GetReply(nick, L_SHOWCMDHELP2), bot.chan);
	return 1;
}
