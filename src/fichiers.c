/* src/fichiers.c - Lecture/Écriture des données
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
 * $Id: fichiers.c,v 1.131 2008/01/05 18:34:13 romexzf Exp $
 */

#include "main.h"
#include "fichiers.h"
#include "outils.h"
#include "config.h"
#include "hash.h"
#include "debug.h"
#include "dnr.h"
#include "mylog.h"
#include "cs_cmds.h"
#include "add_info.h"
#include "data.h"
#ifdef HAVE_VOTE
#include "vote.h"
#endif
#ifdef USE_WELCOMESERV
#include "welcome.h"
#endif

static void db_parse_data(aData **dpp, void *data, int flag, char **ar)
{ /* <DATA> <from> <fin> <debut> :<raison> */
	long int fin = strtol(ar[2], NULL, 10);
	long int debut = strtol(ar[3], NULL, 10);

	data_load(dpp, ar[1], ar[4], flag, fin > 0 ? fin - debut : fin, debut, data);
}

int db_load_users(int quiet)
{
	FILE *fp;
	char buf[500], *ar[20];
	int adm = 0, count = 0, line = 0;
	anUser *u = NULL;
	anAccess *a;

	if(!(fp = fopen(DBDIR "/" DBUSERS, "r")))
	{
		ConfFlag |= CF_PREMIERE;
		return 0;
	}

	while(fgets(buf, sizeof buf, fp))
	{
		int items = split_buf(buf, ar, ASIZE(ar));
		++line;
		if(items < 2) continue;

		strip_newline(ar[items-1]); /* remove trailing \r\n */
		/* NICK <user> <pwd> <lvl> <TS> <flag> <RTS> <mail> <lastlogin> <lang> <id> */
		if(!strcmp(buf, "NICK"))
		{
			char *nick = ar[1], *last = ar[8], *lang = ar[9];
			int level = strtol(ar[3], NULL, 10), flag = strtol(ar[5], NULL, 10);
			time_t lastseen = strtol(ar[4], NULL, 10), regtime = strtol(ar[6], NULL, 10);

			u = NULL; /* reset */

			if(items < 11)
			{
				log_write(LOG_DB, LOG_DOTTY, "users::load(%s) item complet", ar[1]);
				continue;
			}

			if(!(u = add_regnick(nick, ar[2], lastseen, regtime, level, flag,
								ar[7], strtoul(ar[10], NULL, 10))))
			{
				log_write(LOG_DB, LOG_DOTTY, "users::load(%s) erreur interne", nick);
				continue;
			}
			++count;
			if(*last != '0') str_dup(&u->lastlogin, last);
			if(u->level >= ADMINLEVEL) ++adm; /* at least one admin in DB ? */
			/* if lang was not available anymore, set to default */
			if(!(u->lang = lang_isloaded(lang))) u->lang = DefaultLang;
		}
		else if(!strcmp(buf, "SUSPEND") && items > 4 && u)
			db_parse_data(&u->suspend, u, DATA_T_SUSPEND_USER, ar);

		else if(!strcmp(buf, "NOPURGE") && items > 4 && u)
			db_parse_data(&u->nopurge, u, DATA_T_NOPURGE, ar);

		else if(!strcmp(buf, "CANTREGCHAN") && items > 4 && u)
			db_parse_data(&u->cantregchan, u, DATA_T_CANTREGCHAN, ar);

#ifdef USE_MEMOSERV
		else if(!strcmp(buf, "MEMO") && items > 4 && u)
		{ /* MEMO <from> <DateTS> <flag> :message */
			add_memo(u, ar[1], strtol(ar[2], NULL, 10), ar[4], strtol(ar[3], NULL, 10));
		}
#endif
		else if(!strcmp(buf, "ACCESS") && items > 5 && u)
		{ /* ACCESS <#> <level> <flag> <lastseenTS> :info */
			int level = strtol(ar[2], NULL, 10), flag = strtol(ar[3], NULL, 10);
			time_t lastseen = strtol(ar[4], NULL, 10);

			a = add_access(u, ar[1], level, flag, lastseen);
			if(a && ar[5] && *ar[5]) str_dup(&a->info, ar[5]);
		}

		else if(!strcmp(buf, "COOKIE") && items > 1 && u)
		{
			if((u->cookie = malloc(PWDLEN + 1))) strcpy(u->cookie, ar[1]);
			else Debug(W_WARN|W_MAX, "users::load: malloc failed for %s's cookie", u->nick);
		}

		else if(!strcmp(buf, "V"))
		{	/* restore maxid in use */
			if(ar[2]) user_maxid = strtoul(ar[2], NULL, 10);
		}

		else if(u) log_write(LOG_DB, LOG_DOTTY, "users::load:%d: unknown/invalid data %s [%s]",
						line, buf, ar[1]);
	}
	fclose(fp);
	if(!adm) log_write(LOG_DB, LOG_DOTTY, "Aucun Admin localisé dans la Base de Données!");
	if(!quiet) printf("Base de donnée User chargée (%d)\n", count);
	return count;
}

int db_write_users(void)
{
	anAccess *a;
	anUser *u;
#ifdef USE_MEMOSERV
	aMemo *memo;
#endif
 	int i = 0;
 	FILE *fp;

	if(rename(DBDIR "/" DBUSERS, DBDIR "/" DBUSERS ".back") < 0 && errno != ENOENT)
		return log_write(LOG_DB, LOG_DOWALLOPS, "users::write: rename() failed: %s",
					strerror(errno));

	if(!(fp = fopen(DBDIR "/" DBUSERS, "w")))
		return log_write(LOG_DB, LOG_DOWALLOPS, "users::write: fopen() failed: %s",
					strerror(errno));

	fprintf(fp, "V %d %lu\n", DBVERSION_U, user_maxid);

	for(; i < USERHASHSIZE; ++i) for(u = user_tab[i]; u; u = u->next)
	{
		fprintf(fp, "NICK %s %s %d %lu %d %ld %s %s %s %lu\n",
			u->nick, u->passwd, u->level, u->lastseen, u->flag & ~U_INTERNAL, u->reg_time,
			u->mail, u->lastlogin ? u->lastlogin : "0", u->lang->langue, u->userid);
		if(u->suspend) fprintf(fp, "SUSPEND %s %ld %ld :%s\n", u->suspend->from,
			u->suspend->expire, u->suspend->debut, u->suspend->raison);
		if(u->nopurge) fprintf(fp, "NOPURGE %s %ld %ld :%s\n", u->nopurge->from,
			u->nopurge->expire, u->nopurge->debut, u->nopurge->raison);
		if(u->cantregchan) fprintf(fp, "CANTREGCHAN %s %ld %ld :%s\n", u->cantregchan->from,
			u->cantregchan->expire, u->cantregchan->debut, u->cantregchan->raison);
		if(u->cookie) fprintf(fp, "COOKIE %s\n", u->cookie);

#ifdef USE_MEMOSERV
		for(memo = u->memohead; memo; memo = memo->next)
			fprintf(fp, "MEMO %s %ld %d :%s\n", memo->de, memo->date, memo->flag, memo->message);
#endif

    	for(a = u->accesshead; a; a = a->next)
    		fprintf(fp, "ACCESS %s %d %d %lu :%s\n", a->c->chan, a->level,
    			a->flag, AOnChan(a) ? CurrentTS : a->lastseen, NONE(a->info));
	}

    fclose(fp);
    return 1;
}

int db_write_chans(void)
{
	FILE *fp;
	int i = 0;
	aChan *chan;
	aBan *ban;

	if(rename(DBDIR "/" DBCHANS, DBDIR "/" DBCHANS ".back") < 0 && errno != ENOENT)
		return log_write(LOG_DB, LOG_DOWALLOPS, "channels::write: rename() failed: %s",
					strerror(errno));

	if(!(fp = fopen(DBDIR "/" DBCHANS, "w")))
		return log_write(LOG_DB, LOG_DOWALLOPS, "channels::write: fopen() failed: %s",
					strerror(errno));

	fprintf(fp, "V %d\n", DBVERSION_C);

	for(; i < CHANHASHSIZE; ++i) for(chan = chan_tab[i]; chan; chan = chan->next)
	{
		fprintf(fp, "CHANNEL %s %s\nOPTIONS %d %d %d %d %lu %u %u\nURL %s %lu\n",
			chan->chan, chan->description, chan->flag, chan->banlevel, chan->bantype, chan->cml,
			chan->bantime, chan->limit_min, chan->limit_inc, chan->url, chan->creation_time);

		if(chan->defmodes.modes) fprintf(fp, "DEFMODES %s\n", GetCModes(chan->defmodes));
		if(*chan->deftopic) fprintf(fp, "DEFTOPIC %s\n", chan->deftopic);
		if(*chan->welcome) fprintf(fp, "WELCOME %s\n", chan->welcome);
		if(chan->motd) fprintf(fp, "MOTD %s\n", chan->motd);
		if(chan->suspend) fprintf(fp, "SUSPEND %s %ld %ld :%s\n", chan->suspend->from,
			chan->suspend->expire, chan->suspend->debut, chan->suspend->raison);

		for(ban = chan->banhead; ban; ban = ban->next)
			fprintf(fp, "B %s %s %lu %lu %d :%s\n", ban->mask, ban->de,
				ban->debut, ban->fin, ban->level, ban->raison);
	}

	fclose(fp);
	return 1;
}

int db_load_chans(int quiet)
{
	FILE *fp;
	aChan *c = NULL;
	char buff[512], *ar[10];
	int count = 0, line = 0;

	if(!(fp = fopen(DBDIR "/" DBCHANS, "r")))
	{
		ConfFlag |= CF_PREMIERE;
		return 0;
	}

	while(fgets(buff, sizeof buff, fp))
	{
		char *field = strchr(buff, ' ');
		++line;
		if(field) *field++ = 0;
		else continue;

		strip_newline(field);

		if(!strcmp(buff, "CHANNEL"))
		{
			char *desc = strchr(field, ' '); /* find desc */
			if(desc) *desc++ = 0;
			else log_write(LOG_DB, LOG_DOTTY, "channels::load(%s): desc manquante", field);

			if(!(c = add_chan(field, desc)))
			{
				log_write(LOG_DB, LOG_DOTTY, "channels::load(%s): erreur interne", field);
				continue;
			}
			++count;
		}
		else if(!strcmp(buff, "DEFMODES") && c)
		{
			char *lim = strchr(field, ' '), *key = NULL;
			if(lim)
			{
				*lim++ = 0;
				if((key = strchr(lim, ' '))) *key++ = 0;
			}
			c->defmodes.modes = 0; /* otherwise, global defaults would be appended */
			string2scmode(&c->defmodes, field, !key && strchr(field, 'k') ? lim : key, lim);
		}
		else if(!strcmp(buff, "DEFTOPIC") && c) Strncpy(c->deftopic, field, TOPICLEN);
		else if(!strcmp(buff, "WELCOME") && c) Strncpy(c->welcome, field, TOPICLEN);
		else if(!strcmp(buff, "OPTIONS") && c)
		{
			int flag = 0, bantype = 0;
			unsigned long bandefault = 0UL;

			if(sscanf(field, "%d %d %d %d %lu %u %u", &flag, &c->banlevel, &bantype,
				&c->cml, &bandefault, &c->limit_min, &c->limit_inc) != 7)
					log_write(LOG_DB, LOG_DOTTY, "channels::load(%s): options incomplètes [%s]",
						c->chan, field);

			c->flag = (flag & ~C_JOINED);
			c->bantype = (bantype > 4 || bantype < 1) ? 1 : bantype;
			c->bantime = (time_t) bandefault;
		}
		else if(!strcmp(buff, "MOTD") && c) str_dup(&c->motd, field);
		else if(!strcmp(buff, "URL") && c) /* url + channel ts */
		{
			char *creation_t = strchr(field, ' ');
			if(creation_t) *creation_t++ = 0;
			Strncpy(c->url, field, sizeof c->url -1);
			c->creation_time = (creation_t && *creation_t) ? strtoul(creation_t, NULL, 10) : 0;
		}
		else if(!strcmp(buff, "SUSPEND") && c)
		{
			if(split_buf(field, ar + 1, ASIZE(ar) -1) == 4)
				db_parse_data(&c->suspend, c, DATA_T_SUSPEND_CHAN, ar);
		}
		else if(!strcmp(buff, "B") && c)
		{	/* mask de debut fin level :raison */
			time_t debut, fin;

			split_buf(field, ar, ASIZE(ar));
			debut = strtol(ar[2], NULL, 10);
			fin = strtol(ar[3], NULL, 10);
			if(!fin || CurrentTS < fin)
				ban_load(c, ar[0], ar[5], ar[1], debut, fin ? fin - debut : 0, atoi(ar[4]));
		}
		else if(strcmp(buff, "V") && c)
				log_write(LOG_DB, LOG_DOTTY, "channels::load:%d: unknown data %s [%s]",
					line, buff, field);
	}
	fclose(fp);

	if(!quiet) printf("Base de donnée Salon chargée (%d)\n", count);
	return count;
}

void write_cmds(void)
{
	FILE *fp = fopen(CMDS_FILE, "w");
	int i = 0;
	aHashCmd *cmd;

	if(!fp)
	{
		log_write(LOG_DB, LOG_DOWALLOPS, "cmds::write: fopen() failed: %s", strerror(errno));
		return;
	}

	for(; i < CMDHASHSIZE; ++i) for(cmd = cmd_hash[i]; cmd; cmd = cmd->next)
	{
		if(!IsCTCP(cmd)) fprintf(fp, "%s %s %d %d %d\n", cmd->name,
			cmd->corename, cmd->level, cmd->flag, cmd->used);
	}
	fclose(fp);
}

int load_cmds(int quiet)
{
	char buf[80], *ar[5];
	int count = 0;
	aHashCmd *cmd;
	FILE *fp = fopen(CMDS_FILE, "r");

	if(!fp) return log_write(LOG_DB, LOG_DOTTY, "cmds::load: fopen() failed: %s", strerror(errno));

	while(fgets(buf, sizeof buf, fp) && split_buf(buf, ar, ASIZE(ar)) == 5)
	{
		int level = strtol(ar[2], NULL, 10), flag = strtol(ar[3], NULL, 10);

		if(!(cmd = FindCommand(ar[1])))/* recherche sur corename, mais c'est au load */
		{							   	/* donc corename == name */
			log_write(LOG_DB, LOG_DOTTY, "cmds::load: commande %s non trouvé dans la hash (%s)",
				ar[1], ar[0]);
			continue;
		}
		/* cmd core différente de celle du network? */
		if(strcasecmp(cmd->name, ar[0])) HashCmd_switch(cmd, ar[0]);

		if(level || NeedNoAuthCmd(cmd)) cmd->level = level;
		/* ok.. set flag to the saved ones plus internal ones .. */
		cmd->flag = (flag & ~CMD_INTERNAL) | (cmd->flag & CMD_INTERNAL);
		cmd->used = strtol(ar[4], NULL, 10);
		++count;
	}
	fclose(fp);
	if(!quiet) printf("Chargement des commandes IRC... OK (%d)\n", count);
	return count;
}

int write_files(aNick *nick, aChan *chan, int parc, char **parv)
{
	int write = 0;

	if(getoption("-user", parv, parc, 1, GOPT_FLAG)) write |= 0x1;
	if(getoption("-chan", parv, parc, 1, GOPT_FLAG)) write |= 0x2;

	switch(write)
	{
		case 0:
#ifdef USE_WELCOMESERV
			write_welcome();
#endif
			write_dnr();
			write_cmds();
#ifdef HAVE_VOTE
			write_votes();
#endif
		case 0x1|0x2:
			db_write_chans();
			db_write_users();
			break;
		case 0x2:
			db_write_chans();
			break;
		case 0x1:
			db_write_users();
			break;
	}
	return csntc(nick, "Les fichiers ont bien été écrits.");
}
