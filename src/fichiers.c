/* src/fichiers.c - Lecture/Écriture des données
 * Copyright (C) 2004-2005 IrcDreams.org
 *
 * contact: bugs@ircdreams.org
 * site web: http://www.ircdreams.org
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
 * $Id: fichiers.c,v 1.42 2006/03/15 06:43:23 bugs Exp $
 */

#include <errno.h>
#include "main.h"
#include "fichiers.h"
#include "outils.h"
#include "config.h"
#include "hash.h"
#include "debug.h"
#include "cs_cmds.h"
#include "add_info.h"
#include "vote.h"
#include "welcome.h"

static void db_parse_suspend(struct suspendinfo **suspend, char **ar)
{ /* SUSPEND UserName Fin Debut :raison */
	long int fin = strtol(ar[2], NULL, 10);
	long int debut = strtol(ar[3], NULL, 10);

	do_suspend(suspend, ar[4], ar[1], fin > 0 ? fin - debut : fin, debut);
}

int db_load_users(void)
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

		if(!strcmp(buf, "NICK"))
		{ /* NICK <user> <pass> <lvl> <TS> <flag> <RTS> <mail> <lastlogin> <cantregchan> <vhost> <lang> */
			char *nick = ar[1], *last = ar[8], *vhost = ar[10], *lang = ar[11];
			int level = strtol(ar[3], NULL, 10), flag = strtol(ar[5], NULL, 10);
			time_t lastseen = strtol(ar[4], NULL, 10), regtime = strtol(ar[6], NULL, 10);

			u = NULL; /* reset */

			if(items < 11)
			{
				Debug(W_TTY, "Erreur lors de la lecture de la DB pour %s, item incomplet", ar[1]);
				continue;
			}

			if(!(u = add_regnick(nick, ar[2], lastseen,
				regtime < 1000 ? 0 : regtime, level, flag, ar[7], vhost)))
			{
				Debug(W_TTY, "Erreur lors de la lecture de la DB pour %s, ierreur interne", nick);
				continue;
			}
			++count;
			if(*last != '0') str_dup(&u->lastlogin, last);
			if(u->level >= ADMINLEVEL) ++adm; /* at least one admin in DB ? */
			/* if no lang was saved, or not available anymore, set to default */
                        if(!lang || !(u->lang = lang_isloaded(lang))) u->lang = DefaultLang;

			u->cantregchan = strtol(ar[9], NULL, 10);
			if(u->cantregchan > 0 && u->cantregchan < CurrentTS) u->cantregchan = -1;
		}
		else if(!strcmp(buf, "SWHOIS") && items > 1 && u)
			str_dup(&u->swhois, ar[1]);
		else if(!strcmp(buf, "SUSPEND") && items > 4 && u)
			db_parse_suspend(&u->suspend, ar);
		else if(!strcmp(buf, "ALIAS") && items > 1 && u)
			add_alias(u, ar[1]);
		else if(GetConf(CF_MEMOSERV) && !strcmp(buf, "MEMO") && items > 4 && u)
		{ /* MEMO <from> <DateTS> <flag> :message */
			add_memo(u, ar[1], strtol(ar[2], NULL, 10), ar[4], strtol(ar[3], NULL, 10));
		}
		else if(!strcmp(buf, "ACCESS") && items > 5 && u)
		{ /* ACCESS <#> <level> <flag> <lastseenTS> :info */
			int level = strtol(ar[2], NULL, 10), flag = strtol(ar[3], NULL, 10);
			time_t lastseen = strtol(ar[4], NULL, 10);

			a = add_access(u, ar[1], level, flag, lastseen);
			if(a && ar[5] && *ar[5]) str_dup(&a->info, ar[5]);
		}
		else if(strcmp(buf, "V") && u)
				Debug(W_TTY, "DB::load: unknown/invalid data %s [%s] line %d", buf, ar[1], line);
	}
	fclose(fp);
	if(!adm) Debug(W_TTY, "Aucun Admin localisé dans la Base de Données!");
	return count;
}

int db_write_users(void)
{
	anAccess *a;
	anUser *u;
	aMemo *memo;
	anAlias *alias;
 	int i = 0;
 	FILE *fp;

	if(rename(DBDIR "/" DBUSERS, DBDIR "/" DBUSERS ".back") < 0 && errno != ENOENT)
			return Debug(W_MAX|W_WARN, "DB::Write: rename() failed: %s", strerror(errno));

	if(!(fp = fopen(DBDIR "/" DBUSERS, "w")))
		return Debug(W_MAX|W_WARN, "DB::Write: Impossible d'écrire la DB user: %s",
						strerror(errno));

	fprintf(fp, "V %d\n", DBVERSION_U);

	for(;i < USERHASHSIZE;++i) for(u = user_tab[i];u;u = u->next)
	{
		fprintf(fp, "NICK %s %s %d %lu %d %ld %s %s %ld %s %s\n",
			u->nick, u->passwd,     u->level, u->lastseen, u->flag, u->reg_time, u->mail,
			u->lastlogin ?  u->lastlogin : "0", u->cantregchan, u->vhost, u->lang->langue);
		if(u->swhois) fprintf(fp, "SWHOIS :%s\n", u->swhois);
		if(u->suspend) fprintf(fp, "SUSPEND %s %ld %ld :%s\n", u->suspend->from,
			u->suspend->expire, u->suspend->debut, u->suspend->raison);
		for(alias = u->aliashead;alias;alias = alias->user_nextalias)
			fprintf(fp, "ALIAS %s\n", alias->name);

		if(GetConf(CF_MEMOSERV)) {
			for(memo = u->memohead;memo;memo = memo->next)
				fprintf(fp, "MEMO %s %ld %d :%s\n", memo->de, memo->date, memo->flag, memo->message);
		}

    		for(a = u->accesshead;a;a = a->next)
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
			return Debug(W_MAX|W_WARN, "DB::Write: rename() failed: %s", strerror(errno));

	if(!(fp = fopen(DBDIR "/" DBCHANS, "w")))
		return Debug(W_MAX|W_WARN, "DB::Wwrite: Impossible d'écrire la DB chans: %s",
					strerror(errno));

	fprintf(fp, "V %d\n", DBVERSION_C);

	for(;i < CHANHASHSIZE;++i) for(chan = chan_tab[i];chan; chan = chan->next)
	{
		fprintf(fp, "CHANNEL %s %s\nOPTIONS %d %d %d %d %lu %d %d\nURL %s %lu\n",
			chan->chan, chan->description, chan->flag, chan->banlevel, chan->bantype, chan->cml,
			chan->bantime, chan->limit_min, chan->limit_inc, chan->url, chan->creation_time);

		if(chan->defmodes.modes) fprintf(fp, "DEFMODES %s\n", GetCModes(chan->defmodes));
		if(*chan->deftopic) fprintf(fp, "DEFTOPIC %s\n", chan->deftopic);
		if(*chan->welcome) fprintf(fp, "WELCOME %s\n", chan->welcome);
		if(chan->motd) fprintf(fp, "MOTD %s\n", chan->motd);
		if(chan->suspend) fprintf(fp, "SUSPEND %s %ld %ld :%s\n", chan->suspend->from,
			chan->suspend->expire, chan->suspend->debut, chan->suspend->raison);

		for(ban = chan->banhead;ban;ban = ban->next)
			fprintf(fp, "B %s %s %lu %lu %d :%s\n", ban->mask, ban->de,
				ban->debut, ban->fin, ban->level, ban->raison);
	}

	fclose(fp);
	return 1;
}

int db_load_chans(void)
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
			else Debug(W_TTY, "Erreur lors de la lecture de la DB pour %s, desc manquante", field);

			if(!(c = add_chan(field, desc)))
			{
				Debug(W_TTY, "Erreur interne lors du chargement de %s", field);
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
			unsigned long bandefault = 0;

			if(sscanf(field, "%d %d %d %d %lu %d %d", &flag, &c->banlevel, &bantype,
				&c->cml, &bandefault, &c->limit_min, &c->limit_inc) != 7)
					Debug(W_MAX, "DB::load: Options incomplètes pour %s [%s]", c->chan, field);

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
				db_parse_suspend(&c->suspend, ar), c->suspend->data = c;
		}
		else if(!strcmp(buff, "B") && c)
		{	/* mask de debut fin level :raison */
			time_t debut, fin;

			split_buf(field, ar, ASIZE(ar));
			debut = strtol(ar[2], NULL, 10);
			fin = strtol(ar[3], NULL, 10);
			if(!fin || CurrentTS < fin)
				add_ban(c, ar[0], ar[5], ar[1], debut, fin ? fin - debut : 0, atoi(ar[4]));
		}
		else if(strcmp(buff, "V") && c)
				Debug(W_TTY, "DB::load: unknown data %s [%s] line %d", buff, field, line);
	}
	fclose(fp);
	return count;
}

int load_dnr(void)
{
	char buf[512], *ar[5];
	int count = 0, line = 0;
	FILE *fp = fopen(DNR_FILE, "r");

	if(!fp) return Debug(W_TTY, "DB::load: Erreur lors de la lecture des dnr: %s",
						strerror(errno));

	while(fgets(buf, sizeof buf, fp))
	{
		strip_newline(buf);
		++line;

		if(split_buf(buf, ar, ASIZE(ar)) > 4 && add_dnr(*ar, ar[1],	ar[4],
			strtol(ar[3], NULL, 10), strtoul(ar[2], NULL, 10))) ++count;
		else Debug(W_MAX|W_TTY, "load_dnr: malformed DNR line %d (%s)", line, *ar);
	}
	fclose(fp);

	return count;
}

void write_dnr(void)
{
	FILE *fp = fopen(DNR_FILE, "w");
	aDNR *tmp = dnrhead;
	if(!fp) return;

	for(;tmp;tmp = tmp->next) fprintf(fp, "%s %s %u %ld :%s\n",tmp->mask, tmp->from,
									tmp->flag, tmp->date, tmp->raison);
	fclose(fp);
}

void write_cmds(void)
{
	FILE *fp = fopen(CMDS_FILE, "w");
	int i = 0;
	aHashCmd *cmd;
	if(!fp) return;

	for(;i < CMDHASHSIZE;++i) for(cmd = cmd_hash[i];cmd;cmd = cmd->next)
	{
		if(*cmd->name != '\001') fprintf(fp, "%s %s %d %d %d\n", cmd->name,
			cmd->corename, cmd->level, cmd->flag, cmd->used);
	}
	fclose(fp);
}

int write_maxuser(void)
{
        FILE *fp;

        if(!(fp = fopen(DBDIR "/" MAXUSER, "w")))
                return Debug(W_MAX|W_WARN, "DB::Write: Impossible d'écrire la DB maxuser: %s",
                                                strerror(errno));
	fprintf(fp, "%d", nbmaxuser);
	fclose(fp);
	return 1;
}

int load_maxuser(void)
{
	FILE *fd;

	if((fd = fopen(DBDIR "/" MAXUSER, "r"))) /* remise en place du maxuser */
        {
                fscanf(fd, "%d", &nbmaxuser);
                fclose(fd);
        }
	return 1;
}

int load_cmds(void)
{
	char buf[80], *ar[5];
	int i = 0;
	aHashCmd *cmd;
	FILE *fp = fopen(CMDS_FILE, "r");

	if(!fp) return Debug(W_TTY, "DB::load: Erreur lors de la lecture des commandes: %s",
						strerror(errno));

	while(fgets(buf, sizeof buf, fp) && split_buf(buf, ar, ASIZE(ar)) == 5)
	{
		int level = strtol(ar[2], NULL, 10), flag = strtol(ar[3], NULL, 10);

		if(!(cmd = FindCommand(ar[1])))/* recherche sur corename, mais c'est au load */
		{							   	/* donc corename == name */
			Debug(W_TTY, "load_cmds: commande %s non trouvée dans la hash (%s)", ar[1], ar[0]);
			continue;
		}
		/* cmd core différente de celle du network? */
		if(strcasecmp(cmd->name, ar[0])) HashCmd_switch(cmd, ar[0]);

		if(level || NeedNoAuthCmd(cmd)) cmd->level = level;
		/* ok.. set flag to the saved ones plus internal ones .. */
		cmd->flag = (flag & ~CMD_INTERNAL) | (cmd->flag & CMD_INTERNAL);
		cmd->used = strtol(ar[4], NULL, 10);
		++i;
	}
	fclose(fp);
	return i;
}

int write_files(aNick *nick, aChan *chan, int parc, char **parv)
{
	int write = 0;

	if(getoption("-user", parv, parc, 1, -1)) write |= 0x1;
	if(getoption("-chan", parv, parc, 1, -1)) write |= 0x2;

	switch(write)
	{
		case 0:
			if(GetConf(CF_WELCOMESERV)) write_welcome();
			write_dnr();
			write_cmds();
			if(GetConf(CF_VOTESERV)) write_votes();
			write_maxuser();
			db_write_chans();
                        db_write_users();
			cswallops("Ecriture des bases de données");
			break;
		case 0x1|0x2:
			db_write_chans();
			db_write_users();
			cswallops("Ecriture des bases de données users/chans");
			break;
		case 0x2:
			db_write_chans();
			cswallops("Ecriture de la base donnée chans");
			break;
		case 0x1:
			db_write_users();
			cswallops("Ecriture de la base donnée users");
			break;
	}
	return csreply(nick, "Les fichiers ont bien été écrits.");
}

int load_country(void)
{
   FILE *fp;
   int count = 0;
   struct cntryinfo *new, *current;
   char buff[10000], *cntry = NULL;

   buff[0]='\0';
   strcpy(buff, "database/country.db");
   if((fp = fopen(buff, "r")) == NULL) return Debug(W_TTY, "Country::load: Erreur lors de la lecture des pays: %s", strerror(errno));
   
   while(fgets(buff, 1500, fp) != NULL)
   {
      strip_newline(buff);
      if((cntry = strchr(buff, ' '))) *cntry++ = '\0';
      new = (struct cntryinfo *) calloc(1, sizeof(struct cntryinfo));
      if(new != NULL)
      {
         new->next = NULL;

         strcpy(new->iso, buff);
         strcpy(new->cntry, cntry);
         if(cntryhead == NULL) cntryhead = new;
         else
         {
            current = cntryhead;
            while(current->next != NULL) current = current->next;
            current->next = new;
         }
	++count;
      }
   }
   return count;
}
