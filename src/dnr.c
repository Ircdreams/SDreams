/* src/dnr.c - Gestion des DNR (Do Not Register)
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
 * $Id: dnr.c,v 1.2 2007/12/02 17:01:09 romexzf Exp $
 */

#include "main.h"
#include "outils.h"
#include "mystring.h"
#include "mylog.h"
#include "cs_cmds.h"
#include "debug.h"
#include "dnr.h"

static aDNR *DNRList = NULL;

static aDNR *dnr_add(const char *mask, const char *from, const char *raison,
	time_t date, unsigned int flag)
{
	aDNR *dnr = calloc(1, sizeof *dnr);

	if(!dnr)
	{
		Debug(W_MAX|W_WARN, "add_dnr: OOM for %s[%s] (%s)", mask, from, raison);
		return NULL;
	}

	/* Copy data */
	str_dup(&dnr->mask, mask);
	str_dup(&dnr->raison, raison);
	Strncpy(dnr->from, from, NICKLEN);
	dnr->date = date;
	dnr->flag = flag;
	/* Add to dlinked list */
	dnr->next = DNRList;
	if(DNRList) DNRList->last = dnr;
	DNRList = dnr;

	return dnr;
}

static void dnr_del(aDNR *dnr)
{
	if(dnr->next) dnr->next->last = dnr->last;
	if(dnr->last) dnr->last->next = dnr->next;
	else DNRList = dnr->next;

	free(dnr->mask);
	free(dnr->raison);
	free(dnr);
}

aDNR *dnr_find(const char *pattern, int type)
{
	aDNR *dnr = DNRList;

	for(; dnr; dnr = dnr->next)
		if((type & dnr->flag) & DNR_TYPES /* check if provided type match item's one */
			/* then either compare or match item */
			&& ((type & DNR_MASK && !mmatch(dnr->mask, pattern))
				|| (!(type & DNR_MASK) && !strcasecmp(dnr->mask, pattern)))) break;
	return dnr;
}

static int dnr_manage(aNick *nick, int type, int parc, char **parv)
{
	if(!strcasecmp(parv[1], "list"))
	{
		int from = getoption("-from", parv, parc, 2, GOPT_STR);
		int mask = getoption("-match", parv, parc, 2, GOPT_STR), count = 0;
		aDNR *dnr = DNRList;

		for(; dnr; dnr = dnr->next)
			if(dnr->flag & type && (!from || !strcasecmp(parv[from], dnr->from))
				&& (!mask || !match(parv[mask], dnr->mask)))
			{
				csreply(nick, "DNRMask: \2%s\2 Auteur: \2%s\2 Posé le:\2 %s\2 Raison: %s",
					dnr->mask, dnr->from, get_time(nick, dnr->date), dnr->raison);
				++count;
			}
		csreply(nick, "%d matches correspondantes.", count);
		return 0;
	}

	else if(!strcasecmp(parv[1], "add") && parc >= 2)
	{
		aDNR *dnr = dnr_find(parv[2], type | DNR_MASK);
		int both = getoption("-both", parv, parc, 3, GOPT_FLAG);
		unsigned int flag = both ? DNR_TYPES : type;

		if(dnr)
			return csreply(nick, "%s est déjà un DNR mask (Ajouté par %s, le %s [%s])",
				dnr->mask, dnr->from, get_time(nick, dnr->date), dnr->raison);

		if(HasWildCard(parv[2]))
		{
			aDNR *dnr_next =  NULL;

			flag |= DNR_MASK;

			/* Remove overlapped DNRs */
			for(dnr = DNRList; dnr; dnr = dnr_next)
			{
				dnr_next = dnr->next;
				if((dnr->flag & DNR_TYPES) == (flag & DNR_TYPES) && !mmatch(parv[2], dnr->mask))
					dnr_del(dnr);
			}
		}

		dnr_add(parv[2], nick->user->nick,
			parc > (both ? 3 : 2) ? parv2msg(parc, parv, both ? 4 : 3, 250) : "Service",
			CurrentTS, flag);

		csreply(nick, "%s a été ajouté aux DNR masks.", parv[2]);
	}
	else if(!strcasecmp(parv[1], "del") && parc >= 2)
	{
		aDNR *dnr = dnr_find(parv[2], type);

		if(!dnr) return csreply(nick, "%s n'est pas un DNR mask.", parv[2]);

		dnr_del(dnr);
		csreply(nick, "DNR mask supprimé.");
	}
	else return csreply(nick, "Syntaxe: %s (ADD|DEL) <dnr-mask> [raison]", parv[0]);
	write_dnr();
	return 0;
}

int dnrchan_manage(aNick *nick, aChan *c, int parc, char **parv)
{
	return dnr_manage(nick, DNR_TYPECHAN, parc, parv);
}

int dnruser_manage(aNick *nick, aChan *c, int parc, char **parv)
{
	return dnr_manage(nick, DNR_TYPEUSER, parc, parv);
}

int load_dnr(int quiet)
{
	char buf[512], *ar[5];
	int count = 0, line = 0;
	FILE *fp = fopen(DNR_FILE, "r");

	if(!fp) return log_write(LOG_DB, LOG_DOTTY, "dnr::load: fopen() failed: %s", strerror(errno));

	while(fgets(buf, sizeof buf, fp))
	{
		strip_newline(buf);
		++line;

		if(split_buf(buf, ar, ASIZE(ar)) > 4
			&& dnr_add(*ar, ar[1], ar[4], strtol(ar[3], NULL, 10), strtoul(ar[2], NULL, 10)))
				++count;
		else log_write(LOG_DB, LOG_DOTTY, "dnr::load(%s): malformed DNR line %d", ar[0], line);
	}
	fclose(fp);
	if(!quiet) printf("Chargement des DNR mask... OK (%d)\n", count);
	return count;
}

void write_dnr(void)
{
	FILE *fp = fopen(DNR_FILE, "w");
	aDNR *tmp = DNRList;

	if(!fp)
	{
		log_write(LOG_DB, LOG_DOWALLOPS, "dnr::write: fopen() failed: %s", strerror(errno));
		return;
	}

	for(; tmp; tmp = tmp->next) fprintf(fp, "%s %s %u %ld :%s\n",tmp->mask, tmp->from,
									tmp->flag, tmp->date, tmp->raison);

	fclose(fp);
}

void dnr_clean(void)
{
	aDNR *dnr = DNRList, *dt = NULL;

	for(; dnr; dnr = dt)
	{
		dt = dnr->next;
		free(dnr->mask);
		free(dnr->raison);
		free(dnr);
	}
	DNRList = NULL;
}
