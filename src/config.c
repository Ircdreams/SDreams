/* src/fichiers.c - Lecture, parsage, distribution de la conf
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
 * $Id: config.c,v 1.44 2008/01/05 01:24:13 romexzf Exp $
 */

#include "main.h"
#include "config.h"
#include "outils.h"
#include "mylog.h"
#include <unistd.h>
#ifdef WEB2CS
#include <arpa/inet.h>
#include "web2cs.h"
#endif

int ConfFlag = 0;

int cf_kill_interval = 25;
int cf_ignoretime = 600;
int cf_maxlastseen = 1000000;
int cf_chanmaxidle = 7200;
int cf_write_delay = 7200;
int cf_limit_update = 60;
int cf_warn_purge_delay = 345600;
int cf_register_timeout = 86400;
int cf_unreg_reg_delay = 10800;
char *cf_quit_msg = NULL;
char *cf_pasdeperm = NULL;
char *cf_mailprog = NULL;
char cf_hidden_host[HOSTLEN + 1];
char cf_defraison[RAISONLEN + 1];

#define CF_REASONLEN 300

#define TabId(tab) (tab->id - CONF_IREAD)

#define EndOfTab(x) (x && *x == '}' && (*(x+1) == '\0' || *(x+1) == '\n' || *(x+1) == '\r'))

#define conf_error(fmt, arg) printf("conf:%d tab %s, " fmt "\n", line, curtab->item, (arg))
#define conf_error2(fmt, a, b) printf("conf:%d tab %s, " fmt "\n", line, curtab->item, (a), (b))

enum conf_id {CONF_UPLINK = 0, CONF_CSBOT, CONF_MYSERV, CONF_MISC, CONF_LANG, CONF_LOG,
#	ifdef WEB2CS
			CONF_W2C,
#	endif
			CONF_IREAD
};

struct conf_item {
	enum conf_id tabid;
	unsigned int flag;
#define CONF_READ 	0x001
#define CONF_TINT 	0x002
#define CONF_PORT 	0x004
#define CONF_TPTR 	0x008
#define CONF_TARRAY 0x010
#define CONF_IP 	0x020
#define CONF_TDUR 	0x040
#define CONF_TPRIV 	0x080
#define CONF_TFLAG 	0x100
	void *ptr;
#define CONF_MARRAY(x) x, sizeof x - 1
	size_t psize;
	const char *item;
	const char *description;
	int (*cf_valid)(struct conf_item *, char *);
};

static struct conf_tab {
	const char *item;
	const char *description;
	unsigned int id;
} conftab[] = {
	{"uplink", "configuration des infos nécessaire au link (ip, port)", CONF_UPLINK},
	{"csbot", "configuration des infos du Channel Service", CONF_CSBOT},
	{"myserver", "configuration des infos du serveur du CS", CONF_MYSERV},
	{"misc_conf", "configuration des diverses options (On The Fly!) des Services", CONF_MISC},
	{"lang", "configuration du multilangage", CONF_LANG},
	{"log", "configuration du système de log", CONF_LOG},
#ifdef WEB2CS
	{"web2cs", "configuration du web2cs", CONF_W2C},
#endif
};

static int cf_validnick(struct conf_item *i, char *buf)
{
	char *p = (char *) i->ptr;
	if(IsValidNick(p)) return 1;
	snprintf(buf, CF_REASONLEN, "'%s' n'est pas un nick valide.", p);
	return 0;
}

static int cf_validchan(struct conf_item *i, char *buf)
{
	char *p = (char *) i->ptr;
	if(*p == '#') return 1;
	snprintf(buf, CF_REASONLEN, "'%s' n'est pas un nom de salon valide.", p);
	return 0;
}

static int cf_validserv(struct conf_item *i, char *buf)
{
	if(strchr((char *) i->ptr, '.')) return 1;
	snprintf(buf, CF_REASONLEN, "item %s: le nom du serveur doit contenir un '.'.", i->item);
	return 0;
}

static int cf_validmail(struct conf_item *i, char *buf)
{
	char *p = (char *) i->ptr;

	ConfFlag |= CF_NOMAIL; /* default: no mail */
	if(strcasecmp(p, "nomail"))
	{
		char *sp = strchr(p, ' ');

		if(sp) *sp = 0;

		if(access(p, X_OK) < 0)
			printf("conf: Invalid sendmail program at '%s' (%s)\n", p, strerror(errno));
		else
		{
			if(sp) *sp = ' ';
			str_dup(&cf_mailprog, p);
			ConfFlag &= ~CF_NOMAIL; /* sendmail program looks ok */
		}
	}
	return 1;
}

static int cf_validport(struct conf_item *i, char *buf)
{
	int port = *(int *) i->ptr;
	if(port > 0 && port <= 65535) return 1;
	snprintf(buf, CF_REASONLEN, "item %s: %d n'est pas un port valide (1-65535).", i->item, port);
	return 0;
}

static int cf_validlang(struct conf_item *i, char *buf)
{
	char *p = (char *) i->ptr;
	if(lang_add(p) >= 0) return 1;
	snprintf(buf, CF_REASONLEN, "Impossible de charger le langage %s.", p);
	return 0;
}

static int cf_validcchar(struct conf_item *i, char *buf)
{
	bot.cara = *(char *) i->ptr;
	return 1;
}

static int cf_validnum(struct conf_item *i, char *buf)
{
	const char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789[]";
	int c = strtol((char *) i->ptr, NULL, 10);

	if(c > MAXNUM)
	{
		snprintf(buf, CF_REASONLEN, "item %s: Numeric non valide %d (0-%d)", i->item, c, MAXNUM);
		return 0;
	}

	bot.servnum[0] = base64[c / 64];
	bot.servnum[1] = base64[c & 63];
	bot.servnum[2] = '\0';

    snprintf(cs.num, sizeof cs.num, "%s"NNICK"A", bot.servnum);
	return 1;
}

static int cf_log_handle(struct conf_item *i, char *buf)
{
	char *ptr = (char *) i->ptr;

	log_conf_handler(i->item, ptr);
	return 1;
}

#ifdef WEB2CS
static int cf_addw2chost(struct conf_item *i, char *buf)
{
	w2c_hallow *hnew = malloc(sizeof *hnew);
	hnew->host = inet_addr((char *) i->ptr); /* ip */
	hnew->next = w2c_hallowhead; /* add trust to list */
	w2c_hallowhead = hnew;
	return 1;
}
#endif

struct conf_item conf_items[] = {
	/* uplink */
	{CONF_UPLINK, CONF_IP|CONF_TARRAY, CONF_MARRAY(bot.ip), "ip",
		"IP du Hub auquel le serveur doit se connecter", NULL},
	{CONF_UPLINK, CONF_IP|CONF_TARRAY, CONF_MARRAY(bot.bindip), "bindip",
		"IP à utiliser", NULL},
	{CONF_UPLINK, CONF_TARRAY, CONF_MARRAY(bot.pass), "pass", "Pass du link", NULL},
	{CONF_UPLINK, CONF_TINT|CONF_PORT, &bot.port, 0, "port",
		"Port du Hub auquel le serveur doit se connecter", cf_validport},

	/* csbot */
	{CONF_CSBOT, CONF_TARRAY, CONF_MARRAY(cs.nick), "nick", "Pseudo du CS", cf_validnick},
	{CONF_CSBOT, CONF_TARRAY, CONF_MARRAY(cs.ident), "ident", "Ident du CS", NULL},
	{CONF_CSBOT, CONF_TARRAY, CONF_MARRAY(cs.host), "host", "Host du CS", NULL},
	{CONF_CSBOT, CONF_TARRAY, CONF_MARRAY(cs.name), "realname", "Real Name du CS", NULL},
	{CONF_CSBOT, CONF_TARRAY, CONF_MARRAY(cs.mode), "modes",
		"UserModes que doit porter le CS à la connexion (+ok normalement)", NULL},
	{CONF_CSBOT, CONF_TARRAY, CONF_MARRAY(bot.pchan), "chan",
		"Salon de log où est envoyée l'activité du CS", cf_validchan},

	/* myserv */
	{CONF_MYSERV, CONF_TARRAY, CONF_MARRAY(bot.server), "server",
		"Nom littéral du serveur", cf_validserv},
	{CONF_MYSERV, CONF_TARRAY, CONF_MARRAY(bot.name), "infos",
		"Infos du serveur, visible dans le /links", NULL},
	{CONF_MYSERV, CONF_TPRIV|CONF_TINT, NULL, 0, "numeric",
		"Numeric P10 du serveur (en chiffre)", cf_validnum},
	{CONF_MYSERV, CONF_TPTR, &cf_quit_msg, 0, "quit_msg",
		"Message envoyé lors de la maintenance des services", NULL},

	/* misc */
	{CONF_MISC, CONF_TPRIV, NULL, 0, "commandchar",
		"caractère prefixant les commandes salons", cf_validcchar},
	{CONF_MISC, CONF_TPRIV, NULL, 0, "mailprog", "path vers Sendmail", cf_validmail},
	{CONF_MISC, CONF_TARRAY, CONF_MARRAY(cf_hidden_host), "hidden_host",
		"Suffixe de l'host des users logués aux services", NULL},
	{CONF_MISC, CONF_TPTR, &cf_pasdeperm, 0, "pas_de_perm",
		"message envoyé à un user utilisant une commande nécessitant d'être logué.", NULL},
	{CONF_MISC, CONF_TARRAY, CONF_MARRAY(bot.chan), "help_chan", "salon d'aide", cf_validchan},
	{CONF_MISC, CONF_TDUR, &cf_kill_interval, 0, "kill_time",
		"Intervalle qu'attend le robot avant d'enforcer les protections de pseudo", NULL},
	{CONF_MISC, CONF_TDUR, &cf_maxlastseen, 0, "maxlastseen", "lastseen maximal avant"
		" la suppression de l'username", NULL},
	{CONF_MISC, CONF_TINT|CONF_TFLAG, &ConfFlag, CF_ADMINEXEMPT, "adminexempt",
		"exemption des admins pour la protection anti flood", NULL},
	{CONF_MISC, CONF_TINT|CONF_TFLAG, &ConfFlag, CF_KILLFORFLOOD, "kill_for_flood",
		"si actif, les users qui flood seront kill", NULL},
	{CONF_MISC, CONF_TINT|CONF_TFLAG, &ConfFlag, CF_NOKILL, "nokill_on_nick",
		"si actif, les pseudos ne seront plus protégés par le kill (sauf recover)", NULL},
	{CONF_MISC, CONF_TDUR, &cf_ignoretime, 0, "ignoretime",
		"nombre de secondes qu'ignore le robot", NULL},
	{CONF_MISC, CONF_TINT|CONF_TFLAG, &ConfFlag, CF_HOSTHIDING, "host_hidding",
		"permet le changement de l'host des users en <login>.<HIDDEN_HOST>", NULL},
	{CONF_MISC, CONF_TINT|CONF_TFLAG, &ConfFlag, CF_XMODE, "xmode", "Si actif, le masquage"
		"de l'host ne se fera pas lors de l'identification mais lors de la mise du +x", NULL},
	{CONF_MISC, CONF_TDUR, &cf_chanmaxidle, 0, "chanmaxidle",
		"Temps avant que le CS ne quitte un salon vide", NULL},
	{CONF_MISC, CONF_TDUR, &cf_write_delay, 0, "save_delay",
		"Intervalle entre les sauvegardes de la DB", NULL},
	{CONF_MISC, CONF_TDUR, &cf_limit_update, 0, "limit_delay",
		"Intervalle entre les mises à jour de la limite automatique", NULL},
	{CONF_MISC, CONF_TDUR, &cf_warn_purge_delay, 0, "warn_purge_delay",
		"Délai avant alerte de purge", NULL},
	{CONF_MISC, CONF_TDUR, &cf_register_timeout, 0, "register_timeout",
		"Temps max entre le register et le premier login", NULL},
	{CONF_MISC, CONF_TDUR, &cf_unreg_reg_delay, 0, "unreg_reg_delay",
		"Délai entre un unreg et un regchan", NULL},
	{CONF_MISC, CONF_TARRAY, CONF_MARRAY(cf_defraison), "def_raison",
		"Raison par default pour les kicks", NULL},

	/* lang */
	{CONF_LANG, CONF_TPRIV, NULL, 0, "lang", "Nom du fichier langage (sans le .lang)",
		cf_validlang},

	/* log */
	{CONF_LOG, CONF_TPRIV, NULL, 0, "main", "", cf_log_handle},
	{CONF_LOG, CONF_TPRIV|CONF_READ, NULL, 0, "socket", "", cf_log_handle},
	{CONF_LOG, CONF_TPRIV|CONF_READ, NULL, 0, "db", "", cf_log_handle},
	{CONF_LOG, CONF_TPRIV|CONF_READ, NULL, 0, "raw", "", cf_log_handle},
#	ifdef HAVE_VOTE
	{CONF_LOG, CONF_TPRIV|CONF_READ, NULL, 0, "vote", "", cf_log_handle},
#	endif
#	ifdef WEB2CS
	{CONF_LOG, CONF_TPRIV|CONF_READ, NULL, 0, "W2C", "", cf_log_handle},
	{CONF_LOG, CONF_TPRIV|CONF_READ, NULL, 0, "W2C_cmd", "", cf_log_handle},
	{CONF_LOG, CONF_TPRIV|CONF_READ, NULL, 0, "W2C_raw", "", cf_log_handle},
#	endif

#ifdef WEB2CS
	/* Web2CS */
	{CONF_W2C, CONF_TINT|CONF_PORT, &bot.w2c_port, 0, "port", "Port du serveur web2cs", NULL},
	{CONF_W2C, CONF_TPRIV|CONF_IP, NULL, 0, "allow_host",
		"IP autorisée à se connecter au serveur web2cs", cf_addw2chost},
	{CONF_W2C, CONF_TARRAY, CONF_MARRAY(bot.w2c_pass), "pass",
		"Pass que délivrera le php pour se connecter (non crypté)", NULL},
#endif

};

static struct conf_tab *get_tab(const char *item)
{
	unsigned int i = 0;
	for(; i < ASIZE(conftab); ++i)
		if(!strcasecmp(conftab[i].item, item))
		{
			if(conftab[i].id < CONF_IREAD) conftab[i].id += CONF_IREAD;
			return &conftab[i];
		}
	return NULL;
}

static int get_item(struct conf_tab *tab, const char *item)
{
	unsigned int i = 0;

	for(; i < ASIZE(conf_items) && conf_items[i].tabid <= TabId(tab); ++i)
		if(conf_items[i].tabid == TabId(tab) && !strcasecmp(conf_items[i].item, item))
			return i;
	return -1;
}

static int check_missing_item(struct conf_tab *tab)
{
	unsigned int i = 0;

	for(; i < ASIZE(conf_items) && conf_items[i].tabid <= TabId(tab); ++i)
	{
		struct conf_item *item = &conf_items[i];
		if(item->tabid == TabId(tab) && !(item->flag & CONF_READ))
		{
			printf("conf: tab %s, item '%s' manquant (%s)\n",
				tab->item, item->item, item->description);
			return 1;
		}
	}
	return 0;
}

static char *conf_parse(char *ptr, struct conf_tab *tab, struct conf_item *item)
{
	ptr = strtok(NULL, "=\r\n");
	while(ptr && *ptr && (*ptr == ' ' || *ptr == '\t')) ++ptr;

	if(!ptr || !*ptr)
	{
		printf("conf: tab '%s', item '%s' sans valeur.\n", tab->item, item->item);
		return NULL;
	}
	item->flag |= CONF_READ;
	return ptr;
}

static int readconf(FILE *fp)
{
	int line = 0, current_item = -1, i = -1;
	struct conf_item *citem = NULL;
	char buf[512], vreason[CF_REASONLEN + 1];
	struct conf_tab *curtab = NULL;

#ifdef WEB2CS
	w2c_hallow *hnew = w2c_hallowhead, *hnext;
	/* Avoid doublons in case of rehash */
	for(; hnew; free(hnew), hnew = hnext) hnext = hnew->next;
	w2c_hallowhead = NULL;
#endif

	log_clean(1); /* Close log files & reset flags */

	while(fgets(buf, sizeof buf, fp))
	{
		char *ptr = buf;
		++line;

		while(*ptr == '\t' || *ptr == ' ') ++ptr; /* skip tabs & spaces */
		/* skip empty lines */
		if(*ptr == '#' || *ptr == '\r' || *ptr == '\n' || !*ptr) continue;
		ptr = strtok(ptr, " ");

		if(i != -2 && !curtab) /* no tab selected! */
		{
			if(!(curtab = get_tab(ptr)))
				printf("conf line: %d, tab inconnue: %s\n", line, ptr), i = -2;
			continue;
		}

		if(EndOfTab(ptr)) /* maybe I'm just at the end '}' */
		{
			if(curtab && check_missing_item(curtab)) return -1; /* end of /tab */
			citem = NULL;
			curtab = NULL;
			current_item = i = -1;
			continue;
		}
		if(i == -2) continue;
		/* enforce some type checks */

		if((current_item = get_item(curtab, ptr)) == -1)
		{
			conf_error("item '%s' inconnu.", ptr);
			continue;
		}
		else if(!(ptr = conf_parse(ptr, curtab, (citem = &conf_items[current_item]))))
				return -1;

		if(citem->flag & CONF_TINT && !is_num(ptr))
		{
			conf_error("l'item '%s' doit être un nombre.", citem->item);
			return -1;
		}
		if(citem->flag & CONF_IP && !is_ip(ptr))
		{
			conf_error2("item %s: %s ne semble pas être une IP valide.", citem->item, ptr);
			return -1;
		}
		/* now copy and transtype the data */
		if(citem->flag & CONF_TPRIV) /* ok let's its function handle the whole stuff */
		{
			citem->ptr = ptr; /* pass the value via the void * */
			if(citem->cf_valid && !citem->cf_valid(citem, vreason))
			{
				conf_error("%s", vreason);
				return -1;
			}
			continue;
		}
		else if(citem->flag & CONF_TARRAY)
			Strncpy((char *) citem->ptr, ptr, citem->psize);
		else if(citem->flag & CONF_TPTR) str_dup((char **) citem->ptr, ptr);
		else if(citem->flag & CONF_TFLAG)
		{
			if(strtol(ptr, NULL, 10)) *(int *)citem->ptr |= citem->psize;
			else *(int *)citem->ptr &= ~citem->psize;
		}
		else if(citem->flag & CONF_TINT)
			*(int *)citem->ptr = strtol(ptr, NULL, 10);
		else if(citem->flag & CONF_TDUR)
		{
			if((*(int *)citem->ptr = convert_duration(ptr)) <= 0)
			{
				conf_error2("item %s: '%s' n'est pas un format de durée valide.",
					citem->item, ptr);
				return -1;
			}
		}
		else conf_error("l'item '%s' n'a pas de type défini!!", citem->item);
		/* more check */

		if(citem->cf_valid && !citem->cf_valid(citem, vreason))
		{
			conf_error("%s", vreason);
			return -1;
		}
	}

	if(curtab)
	{
		printf("conf: erreur, parsage incomplet, missing '}', last tab : %s\n",
			curtab->item);
		return -1;
	}
	else
	{
		unsigned int j = 0;
		for(; j < ASIZE(conftab); ++j)
			if(conftab[j].id < CONF_IREAD)
			{
				printf("conf: tab '%s' manquante (%s).\n", conftab[j].item,
					conftab[j].description);
				return -1;
			}
	}
	return 1;
}

int load_config(const char *file)
{
	FILE *fp = fopen(file, "r");
	int error = -1;
	if(fp)
	{
		error = readconf(fp);
		fclose(fp);
	}
	else printf("conf: Impossible d'ouvrir le fichier de conf (%s)\n", strerror(errno));
	return error;
}
