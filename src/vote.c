/* src/vote.c - Effectuer un vote
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
 * $Id: vote.c,v 1.17 2006/03/16 07:01:03 bugs Exp $
 */

#include "main.h"
#include "vote.h"
#include "cs_cmds.h"
#include "fichiers.h"
#include "hash.h"
#include "outils.h"

struct votes vote[MAXVOTEPROP + 1];
struct Vote Vote = {0, 0, 0};

static int vote_sort(const void *a, const void *b)
{
	const struct votes *va = a;
	const struct votes *vb = b;
	return vb->voies - va->voies;
}

static void show_results(aNick *nick, struct votes *show)
{
	int i = 1;

	csreply(nick, "Resultats du vote\2 %s\2", show[0].prop);

	for(;i <= Vote.nbprop;++i)
		csreply(nick, "%d. %.2f%% [%d] \2%s", i, (show[i].voies * 100.0) / Vote.votes,
			show[i].voies, show[i].prop);
}

void show_vote(aNick *nick)
{
	int i = 1;
	csreply(nick, "\2Sujet:\2 %s", vote[0].prop);
	csreply(nick, "\2Propositions:\2");
	for(;i <= Vote.nbprop;++i) csreply(nick, "%d. %s", i, vote[i].prop);
}

/*
 * vote_results
 */
int vote_results(aNick *nick, aChan *c, int parc, char **parv)
{
	if(Vote.actif)
		return csreply(nick, "Vous ne pouvez pas voir les résultats, le vote est encore en cours.");

	if(!Vote.nbprop) return csreply(nick, "Il n'y a aucun vote existant.");

	if(nick->user->level < Vote.level)
		return csreply(nick, GetReply(nick, L_NEEDTOBEADMIN));

        show_results(nick, vote);

	return 1;
}

/*
 * voter parv[1] = n° de la proposition
 */
int voter(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	int nb_prop = 0;

	if(nick->user->level < Vote.level)
		return csreply(nick, GetReply(nick, L_NEEDTOBEADMIN));

	if(!Vote.actif)
	{
		if(!Vote.nbprop) return csreply(nick, "Le vote n'est pas actif");
		else return csreply(nick, "Le vote est clos. Pour voir les résultats, tapez /%s %s",
						cs.nick, RealCmd("results"));
	}

	if(UNoVote(nick->user)) 
                return csreply(nick, "Vous avez choisi de ne pas participer aux votes."); 

	if(!parc)
	{
		csreply(nick, "Un vote est actuellement en cours.");
		show_vote(nick);
		if(nick->user && !UVote(nick->user))
			csreply(nick, "Pour voter utilisez: \2/%s %s <n° de la proposition>\2", cs.nick, parv[0]);
		return 1;
	}

	if(nick->user->reg_time > Vote.start_time)
		return csreply(nick, "Vous avez enregistré votre username après l'ouverture du vote."
			"Vous ne pourrez voter qu'au prochain vote");

	if(UVote(nick->user)) return csreply(nick, "Vous avez déjà voté.");

	if(!is_num(parv[1])) return csreply(nick, "Le n° de la proposition doit être un nombre.");

	if((nb_prop = atoi(parv[1])) < 1 || nb_prop > Vote.nbprop)
		return csreply(nick, "Les propositions vont de 1 à %d", Vote.nbprop);

	SetUVote(nick->user);
	++vote[nb_prop].voies;
	++Vote.votes;
	csreply(nick, "Votre vote pour \2%s\2 a bien été pris en compte.", vote[nb_prop].prop);
	write_votes();
	return 1;
}

/*
 * vote NEW <nombre de prop> <level> <sujet>
 * 		PROPOSITION <proposition>
 * 		CLORE
 * 		RESULTS
 *		CLEAR
 */

int do_vote(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	const char *cmd = parv[1];

	if(!strcasecmp(cmd, "NEW"))
	{
		anUser *u;
		char *msg = NULL;
		int i = 0;

		if(parc < 4)
			return csreply(nick, "Syntaxe: %s NEW <nombre de propositions> <level> :<sujet>", parv[0]);

		if(Vote.actif) return csreply(nick, "Le vote est déjà actif");

		if(!is_num(parv[2]))
			return csreply(nick, "Le nombre de propositions doit être une valeur numérique");

		if((Vote.nbprop = atoi(parv[2])) < 2 || Vote.nbprop > MAXVOTEPROP)
			return csreply(nick, "Il peut y avoir de 2 à %d propositions possibles.", MAXVOTEPROP);

		if(!Strtoint(parv[3], &Vote.level, 1, MAXADMLVL))
			return csreply(nick, GetReply(nick, L_VALIDLEVEL));

		msg = parv2msg(parc, parv, 4, MEMOLEN + 3);
		if(strlen(msg) > MEMOLEN)
			return csreply(nick, "La longueur du sujet du vote est limitée à %d caractères", MEMOLEN);

		memset(vote, 0, sizeof vote);
		Strncpy(vote[0].prop, msg, MEMOLEN);
		for(;i < USERHASHSIZE;++i) for(u = user_tab[i];u;u = u->next) DelUVote(u);

		csreply(nick, "Le vote est en cours de création au level %d.", Vote.level);
		csreply(nick, "\2Sujet:\2 %s", vote[0].prop);
		csreply(nick, "Vous devez maintenant définir les propositions avec: "
				"\2/%s %s PROPOSITION :<nom>\2 jusqu'à que vous ayez donné les %d propositions",
				cs.nick, parv[0], Vote.nbprop);
		Vote.votes = 0;
		Vote.start_time = CurrentTS;
	}
	else if(!strcasecmp(cmd, "PROPOSITION"))
	{
		int i = 1;
		char *msg = NULL;

		if(parc < 2) return csreply(nick, "Syntaxe: %s PROPOSITION :<proposition>", parv[0]);

		if(Vote.actif) return csreply(nick, "Le vote est déjà actif");

		if(!Vote.nbprop) return csreply(nick, "Aucun vote n'est en cours d'enregistrement");

		for(;*vote[i].prop && i <= Vote.nbprop;++i);
		if(i > Vote.nbprop)
			return csreply(nick, "Vous avez bien donné toutes les propositions pour le vote");

		msg = parv2msg(parc, parv, 2, MEMOLEN + 3);
		if(strlen(msg) > MEMOLEN)
			return csreply(nick, "La longueur d'une proposition est limitée à %d caractères", MEMOLEN);

		Strncpy(vote[i].prop, msg, MEMOLEN);
		vote[i].voies = 0;

		if(i == Vote.nbprop)
		{
			Vote.actif = 1;
			csreply(nick, "Le vote est maintenant \2ACTIF\2.");
			show_vote(nick);
			write_votes();
		}
		else csreply(nick, "Vous avez donné %d des %d propositions.", i, Vote.nbprop);
	}
	else if(!strcasecmp(cmd, "CLORE"))
	{
		if(!Vote.actif) return csreply(nick, "Il n'y a pas de vote actif.");

		Vote.actif = 0;
		qsort(vote + 1, Vote.nbprop, sizeof *vote, vote_sort);
		csreply(nick, "Le vote est clos, les users peuvent maintenant voir les résultats.");
		write_votes();
	}
	else if(!strcasecmp(cmd, "RESULTS"))
	{
		struct votes lvote[MAXVOTEPROP + 1];
		int i = 0, usr = 0;
		anUser *u;

		if(!Vote.nbprop) return csreply(nick, "Il n'y a aucun vote existant.");

		if(Vote.actif)
		{
			memcpy(lvote, vote, sizeof vote);
			qsort(lvote + 1, Vote.nbprop, sizeof *lvote, vote_sort);
		}

		show_results(nick, Vote.actif ? lvote : vote);
	
		for(;i < USERHASHSIZE;++i) for(u = user_tab[i];u;u = u->next)
			if(u->level >= Vote.level && u->reg_time <= Vote.start_time && !UNoVote(u)) ++usr;

		csreply(nick, "%d des %d users ont voté (%.2f%%)", Vote.votes, usr, (Vote.votes*100.0)/ usr);
	}
	else if(!strcasecmp(cmd, "CLEAR")) 
        { 
                if(Vote.actif) return csreply(nick, "Un vote est actif, veuillez d'abord le clore."); 
 
                memset(vote, 0, sizeof vote); 
 
                Vote.votes = 0; 
                Vote.start_time = 0; 
                Vote.nbprop = 0;

		csreply(nick, "Il n'y a plus aucun vote sauvegardé.");
		write_votes();
        } 
	else csreply(nick, "La sous commande %s n'existe pas", cmd);
	return 1;
}

void write_votes(void)
{
   FILE *fp = fopen(VOTE_FILE, "w");
   int i = 1;

   if(!fp) return;

   fprintf(fp, "VOTE %d %ld %d :%s\n", Vote.actif, Vote.start_time, Vote.level, vote[0].prop);
   for(;i <= Vote.nbprop;++i) fprintf(fp, "PROP %d :%s\n", vote[i].voies, vote[i].prop);
   fclose(fp);
   return;
}

void load_votes(void)
{
   FILE *fp = fopen(VOTE_FILE, "r");
   char buf[300], *ar[5];

   if(!fp) return;
   while(fgets(buf, sizeof buf, fp))
   {
	int items = split_buf(buf, ar, ASIZE(ar)); 
        strip_newline(ar[items - 1]); 
        if(!strcmp(buf, "VOTE") && items > 4) 
	{
		Vote.actif = strtol(ar[1], NULL, 10);
		Vote.start_time = strtol(ar[2], NULL, 10);
		Vote.level = strtol(ar[3], NULL, 10);
		Strncpy(vote[0].prop, ar[4], MEMOLEN);
	}
	else if(!strcmp(buf, "PROP") && items > 2)
	{
		Strncpy(vote[++Vote.nbprop].prop, ar[2], MEMOLEN);
		vote[Vote.nbprop].voies = strtol(ar[1], NULL, 10);
		Vote.votes += vote[Vote.nbprop].voies;
	}
   }
   fclose(fp);
}
