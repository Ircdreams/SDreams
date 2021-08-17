/* src/vote.c - Effectuer un vote
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
 * $Id: vote.c,v 1.36 2007/11/06 13:42:08 romexzf Exp $
 */

#include "main.h"
#ifdef HAVE_VOTE
#include "vote.h"
#include "cs_cmds.h"
#include "mylog.h"
#include "hash.h"
#include "outils.h"
#include "timers.h"

struct votes vote[MAXVOTEPROP + 1]; /* sujet en #0 + MAXVOTEPROP propositions */
struct Vote Vote = {0, 0, 0, 0, 0, NULL};

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

	if(!Vote.votes)
	{
		csreply(nick, "Aucun votant jusqu'à présent.");
		return;
	}

	for(; i <= Vote.nbprop; ++i)
		csreply(nick, "%d. %.2f%% [%d] \2%s", i, (show[i].voies * 100.0) / Vote.votes,
			show[i].voies, show[i].prop);
}

static void vote_close(void)
{
	if(Vote.timer_end)
	{
		timer_remove(Vote.timer_end);
		Vote.timer_end = NULL;
	}

	Vote.actif = 0;
	qsort(vote + 1, Vote.nbprop, sizeof *vote, vote_sort);
	write_votes();
	log_write(LOG_VOTE, 0, "Vote clos: Sujet(%s) %d votant", vote[0].prop, Vote.votes);
}

static int callback_vote_end(Timer *timer)
{
	Vote.timer_end = NULL; /* to prevent vote_close() to delete it */
	vote_close();
	cswall("Le vote \2%s\2 a été automatiquement clos.", vote[0].prop);
	return 1; 				/* as timer_run() will do it */
}

void show_vote(aNick *nick)
{
	int i = 1;
	csreply(nick, "\2Sujet:\2 %s", vote[0].prop);
	csreply(nick, "\2Propositions:\2");
	for(; i <= Vote.nbprop; ++i) csreply(nick, "%d. %s", i, vote[i].prop);
}

/*
 * vote_results
 */

int vote_results(aNick *nick, aChan *c, int parc, char **parv)
{
	if(Vote.actif)
		return csreply(nick, "Vous ne pouvez pas voir les résultats,"
								" le vote est encore en cours.");

	if(!Vote.nbprop) return csreply(nick, "Il n'y a aucun vote existant.");

	if(nick->user->level < Vote.level)
		return csreply(nick, GetReply(nick, L_NEEDTOBEADMIN));

	show_results(nick, vote);

	return 1;
}

/*
 * voter parv[1] = n° de la proposition
 */

int voter(aNick *nick, aChan *chan, int parc, char **parv)
{
	int choix = 0;

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
			csreply(nick, "Pour voter utilisez: \2/%s %s <n° de la proposition>\2",
				cs.nick, parv[0]);
		return 1;
	}

	if(nick->user->reg_time > Vote.start_time)
		return csreply(nick, "Vous avez enregistré votre username après l'ouverture du vote."
			"Vous ne pourrez voter qu'au prochain vote");

	if(UVote(nick->user)) return csreply(nick, "Vous avez déjà voté.");

	if(!is_num(parv[1])) return csreply(nick, "Le n° de la proposition doit être un nombre.");

	if((choix = atoi(parv[1])) < 1 || choix > Vote.nbprop)
		return csreply(nick, "Les propositions vont de 1 à %d", Vote.nbprop);

	SetUVote(nick->user);
	++vote[choix].voies;
	++Vote.votes;
	csreply(nick, "Votre vote pour \2%s\2 a bien été pris en compte.", vote[choix].prop);
	write_votes(); /* to avoid multiple votes in case of crash */
	return 1;
}

/*
 * vote NEW <level> <sujet>
 * 		PROPOSITION <proposition>
 * 		CLORE
 * 		RESULTS
 *      OPEN [[%XjXmX] [-level <level>]]
 *      CLEAR
 */

int do_vote(aNick *nick, aChan *chan, int parc, char **parv)
{
	const char *cmd = parv[1];

	if(!strcasecmp(cmd, "NEW"))
	{
		anUser *u;
		char *msg = NULL;
		int i = 0;

		if(parc < 3) return csreply(nick, "Syntaxe: %s NEW <level> <sujet>", parv[0]);

		if(Vote.actif) return csreply(nick, "Le vote est déjà actif");

		if(!Strtoint(parv[2], &Vote.level, 1, MAXADMLVL))
			return csreply(nick, GetReply(nick, L_VALIDLEVEL));

		msg = parv2msg(parc, parv, 3, MEMOLEN + 3);
		if(strlen(msg) > MEMOLEN)
			return csreply(nick, "Le sujet du vote est limité à %d caractères", MEMOLEN);

		/* init struct */
		memset(vote, 0, sizeof vote);
		Strncpy(vote[0].prop, msg, MEMOLEN);
		/* clear users' vote status */
		for(; i < USERHASHSIZE; ++i) for(u = user_tab[i]; u; u = u->next) DelUVote(u);

		csreply(nick, "Le vote est en cours de création au level %d.", Vote.level);
		csreply(nick, "\2Sujet:\2 %s", vote[0].prop);
		csreply(nick, "Vous devez maintenant définir les propositions avec "
				"\2/%s %s PROPOSITION <nom>\2", cs.nick, parv[0]);
		Vote.votes = 0;
		Vote.nbprop = 0;
		Vote.start_time = CurrentTS;
	}

	else if(!strcasecmp(cmd, "PROPOSITION"))
	{
		char *msg = NULL;

		if(parc < 2) return csreply(nick, "Syntaxe: %s PROPOSITION <proposition>", parv[0]);

		if(Vote.actif) return csreply(nick, "Le vote est déjà actif.");

		if(!*vote[0].prop) return csreply(nick, "Aucun vote n'est en cours d'enregistrement.");

		if(Vote.nbprop > MAXVOTEPROP) /* and not (== MAXVOTEPROP) because prop #0 is subject. */
			return csreply(nick, "Le nombre de proposition est limité à %d.", MAXVOTEPROP);

		msg = parv2msg(parc, parv, 2, MEMOLEN + 3);
		if(strlen(msg) > MEMOLEN)
			return csreply(nick, "Une proposition est limitée à %d caractères.", MEMOLEN);

        ++Vote.nbprop;
		Strncpy(vote[Vote.nbprop].prop, msg, MEMOLEN);
		vote[Vote.nbprop].voies = 0;

		csreply(nick, "Vous avez ajouté la %dème proposition.", Vote.nbprop);
	}

	else if(!strcasecmp(cmd, "OPEN")) /* Syntaxe: vote open [[%XjXmX] [-level <level>]] */
	{
		time_t timeout = 0;
		int level = getoption("-level", parv, parc, 2, GOPT_INT);

		if(Vote.nbprop < 2)
			return csreply(nick, "Vous devez ajouter au moins deux propositions.");

		if(Vote.votes) return csreply(nick, "Vous ne pouvez réouvrir un vote clos.");

		if(parc > 1 && *parv[2] == '%' && (timeout = convert_duration(++parv[2])) <= 0)
			return csreply(nick, GetReply(nick, L_INCORRECTDURATION));

		/* allow to set a dead line for the vote */
		if(timeout)
		{
			if(Vote.timer_end) timer_remove(Vote.timer_end); /* changing time */
			Vote.timer_end = timer_add(timeout, TIMER_RELATIF, callback_vote_end, NULL, NULL);
			csreply(nick, "Le vote s'achèvera le %s", get_time(nick, CurrentTS + timeout));
		}

		/* also allow to change the level */
		if(level && !Strtoint(parv[level], &Vote.level, 1, MAXADMLVL))
			return csreply(nick, GetReply(nick, L_VALIDLEVEL));

		/* we can change level & timeout even on a running vote */
		if(!level && !timeout && Vote.actif)
			return csreply(nick, "Le vote est déjà actif.");

        Vote.actif = 1;
        log_write(LOG_VOTE, 0, "Vote(%s) actif, level(%d), expire(%s), propositions(%d)",
        	vote[0].prop, Vote.level, timeout ? duration(timeout) : "-", Vote.nbprop);
        csreply(nick, "Le vote est maintenant \2ACTIF\2, accessible au niveau %d", Vote.level);
        show_vote(nick);
        write_votes();
	}

	else if(!strcasecmp(cmd, "CLORE"))
	{
		if(!Vote.actif) return csreply(nick, "Il n'y a pas de vote actif.");

		vote_close();
		csreply(nick, "Le vote est clos, les users peuvent maintenant voir les résultats.");
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

		for(; i < USERHASHSIZE; ++i) for(u = user_tab[i]; u; u = u->next)
			if(u->level >= Vote.level && u->reg_time <= Vote.start_time && !UNoVote(u)) ++usr;

		csreply(nick, "%d des %d users ont voté (%.2f%%)",
			Vote.votes, usr, (Vote.votes * 100.0) / usr);
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

	if(!fp)
	{
		log_write(LOG_DB, 0, "votes::write: fopen() failed: %s", strerror(errno));
		return;
	}

	/* VOTE <actif?> <start_TS> <level> <end_TS> :<subject ...> */
	fprintf(fp, "VOTE %d %ld %d %ld :%s\n", Vote.actif, Vote.start_time,
		Vote.level, Vote.timer_end ? Vote.timer_end->expire : 0, vote[0].prop);

	for(; i <= Vote.nbprop; ++i)
		fprintf(fp, "PROP %d :%s\n", vote[i].voies, vote[i].prop);
	fclose(fp);
}

void load_votes(void)
{
    FILE *fp = fopen(VOTE_FILE, "r");
    char buf[300], *ar[6];

    if(!fp)
	{
		log_write(LOG_DB, LOG_DOTTY, "vote::load: fopen() failed: %s", strerror(errno));
		return;
	}

    while(fgets(buf, sizeof buf, fp))
    {
        int items = split_buf(buf, ar, ASIZE(ar));
        strip_newline(ar[items - 1]);
        if(!strcmp(buf, "VOTE") && items > 4)
        {
			time_t expire = strtol(ar[4], NULL, 10);
            Vote.actif = strtol(ar[1], NULL, 10);
            Vote.start_time = strtol(ar[2], NULL, 10);
            Vote.level = strtol(ar[3], NULL, 10);
            Strncpy(vote[0].prop, ar[items-1], MEMOLEN);

            if(items > 5 && Vote.actif && expire > 0)
            	Vote.timer_end = timer_add(expire, TIMER_RELATIF, callback_vote_end, NULL, NULL);
        }
        else if(!strcmp(buf, "PROP") && items > 2)
        {
            if(++Vote.nbprop >= MAXVOTEPROP)
            {
				log_write(LOG_VOTE, LOG_DOTTY, "Trying to add more than %d propositions",
                    MAXVOTEPROP);
                break;
            }
            Strncpy(vote[Vote.nbprop].prop, ar[2], MEMOLEN);
            vote[Vote.nbprop].voies = strtol(ar[1], NULL, 10);
            Vote.votes += vote[Vote.nbprop].voies;
        }
    }
    fclose(fp);
}

#endif
