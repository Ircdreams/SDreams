/* src/convertdb.c - conversion des DBs
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
 * $Id: convertdb.c,v 1.18 2008/01/05 01:24:13 romexzf Exp $
 */

#include "main.h"
#include "hash.h"
#include "fichiers.h"
#include <ctype.h>
#include <errno.h>
#include <unistd.h>

#define USERS_FILE DBDIR"/usr.db"
#define CHANS_FILE DBDIR"/chans.db"

aChan *chan_tab[CHANHASHSIZE] = {0}; 	/* hash chan */
anUser *user_tab[USERHASHSIZE] = {0}; 	/* hash username */
Lang *DefaultLang = NULL;

static int ver_u = 0, ver_c = 0; /* version des fichiers présents */

static unsigned int user_count = 0U;
static unsigned long maxid = 0UL ,lmaxid = 0UL;

/* String Tools */

int Debug(int flag, const char *fmt, ...)
{
	va_list vl;
	va_start(vl, fmt);
	vfprintf(stdout, fmt, vl);
	va_end(vl);
	return 0;
}

static void conv_char(const char *chan, char *string)
{

	for(; *chan; ++chan)
	{
		if(!isalnum((unsigned char) *chan))
		{
			*string++ = '%';
			string += sprintf(string, "%03o", *chan);
		}
		else *string++ = tolower((unsigned char) *chan);
	}
	*string = 0;
}

/* Internal DATA functions */

static const struct Modes {
	char mode;
	unsigned int value;
} CMode[] = {
	{ 'n', C_MMSG },
	{ 't', C_MTOPIC },
	{ 'i', C_MINV },
	{ 'l', C_MLIMIT },
	{ 'k', C_MKEY },
	{ 's', C_MSECRET },
	{ 'p', C_MPRIVATE },
	{ 'm', C_MMODERATE },
	{ 'c', C_MNOCTRL },
	{ 'C', C_MNOCTCP },
	{ 'O', C_MOPERONLY },
	{ 'r', C_MUSERONLY },
	{ 'D', C_MDELAYJOIN },
	{ 'N', C_MNONOTICE },
#ifdef HAVE_OPLEVELS
	{ 'A', C_MAPASS },
	{ 'U', C_MUPASS }
#endif
};

static unsigned int cmodetoflag(unsigned int flag, const char *mode)
{
	unsigned int w = 1, i;

	if(!mode) return flag;
	for(; *mode; ++mode)
	{
		if(*mode == '+') w = 1;
		else if(*mode == '-') w = 0;
		else for (i = 0U; i < ASIZE(CMode); ++i)
				if(*mode == CMode[i].mode)
				{
					if(w) flag |= CMode[i].value;
					else flag &= ~CMode[i].value;
					break;
				}
	}
	return flag;
}

static unsigned int string2scmode(struct cmode *mode, const char *modes,
						const char *key, const char *limit)
{
    unsigned int newmode = cmodetoflag(mode->modes, modes);

	if(newmode & C_MLIMIT)
	{
		int lim = 0;
		if(limit && (lim = strtol(limit, NULL, 10)) > 0) mode->limit = lim;
		else if(!(mode->modes & C_MLIMIT)) newmode &= ~C_MLIMIT;
	}
	else mode->limit = 0U;

	if(newmode & C_MKEY)
	{
		if(key && *key) Strncpy(mode->key, key, KEYLEN);
		else if (!*mode->key) newmode &= ~C_MKEY;/* +k mis pourtant aucun clé valide.. */
	} /* -k autorisé seulement si key fournie */
	else if(mode->modes & C_MKEY && (!key || !*key)) newmode |= C_MKEY;
	else *mode->key = 0;
	return (mode->modes = newmode);
}

static unsigned int do_hash(const char *str, int hashsize)
{
	unsigned int checksum = 0;
	while(*str) checksum += (checksum << 3) + tolower((unsigned char) *str++);
	return checksum & (hashsize-1);
}

char *GetCModes(struct cmode modes)
{
	static char mode[ASIZE(CMode) + KEYLEN + 20];
	unsigned int i = 0U, j = 0U;

	for(; i < ASIZE(CMode); ++i)
		if(modes.modes & CMode[i].value) mode[j++] = CMode[i].mode;

	if(modes.modes & C_MLIMIT) j += sprintf(mode + j, " %u", modes.limit);
	if(modes.modes & C_MKEY)
	{
		mode[j++] = ' ';
		strcpy(mode + j, modes.key);
	}
	else mode[j] = '\0';
	return mode;
}

aChan *getchaninfo(const char *chan)
{
	unsigned int hash = do_hash(chan, CHANHASHSIZE);
	register aChan *tmp = chan_tab[hash];
	for(; tmp && strcasecmp(chan, tmp->chan); tmp = tmp->next);
	return tmp;
}

aChan *add_chan(const char *chan, const char *description)
{
	aChan *c = calloc(1, sizeof *c);
	unsigned int hash = do_hash(chan, CHANHASHSIZE);

	if(!c)
	{
		printf("error: add_chan, malloc a échoué pour aChan %s\n", chan);
		return NULL;
	}

	Strncpy(c->chan, chan, REGCHANLEN);
	Strncpy(c->description, description, DESCRIPTIONLEN);
	c->suspend = NULL;
	c->motd = NULL;
	c->banlevel = DEFAUT_BANLEVEL;
	c->cml = DEFAUT_CMODELEVEL;
	c->bantype = C_DEFAULT_BANTYPE;
	c->flag = C_DEFAULT;
	c->creation_time = time(0);
	c->defmodes.modes = C_DEFMODES;
	c->banhead = NULL;

	c->next = chan_tab[hash];		/* swap le vieux */
	return (chan_tab[hash] = c);	/* et le nouveau (mis en tête) */
}

#ifdef USE_MEMOSERV
static void add_memo(anUser *nick, const char *de, time_t date,
					const char *message, int flag)
{
	aMemo *m = calloc(1, sizeof *m), *tmp = nick->memotail;

	if(!m)
	{
		printf("error: add_memo, malloc a échoué pour aMemo %s (%s)\n", nick->nick, message);
		return;
	}

	Strncpy(m->de, de, NICKLEN);
	m->date = date;
	m->flag = flag;
	Strncpy(m->message, message, MEMOLEN);
	m->next = NULL;

	if(!(m->last = tmp)) nick->memohead = m;
	if(tmp) tmp->next = m;
	nick->memotail = m;
}
#endif

static anAccess *add_access(anUser *user, const char *chan, int level,
							int flag, time_t lastseen)
{
	anAccess *ptr = calloc(1, sizeof *ptr);

	if(!ptr)
	{
		printf("error: add_access, malloc a échoué pour l'accès %s (%s)\n", user->nick, chan);
		return NULL;
	}

	ptr->level = level;
	ptr->flag = flag;
	ptr->user = user;
	ptr->info = NULL;
	ptr->lastseen = lastseen ? lastseen : user->lastseen;

	if(!(ptr->c = getchaninfo(chan)))
	{
		printf("convertdb: add_access, accès de %s sur %s, non reg ?\n", user->nick, chan);
		free(ptr);
		return NULL;
	}

	ptr->next = user->accesshead;
	return (user->accesshead = ptr);
}

static void add_ban(aChan *chan, const char *mask, const char *raison,
					const char *de,	time_t debut, time_t fin, int level)
{
	aBan *ban = calloc(1, sizeof *ban);

	if(!ban)
	{
		printf("error: add_ban, malloc a échoué pour aBan %s sur %s de %s (%s)\n",
				mask, chan->chan, de, raison);
		return;
	}

	str_dup(&ban->mask, mask);
	str_dup(&ban->raison, raison);
	Strncpy(ban->de, de, NICKLEN);
	ban->debut = debut;
	ban->fin = fin;
	ban->level = level;

	if(chan->banhead) chan->banhead->last = ban;
	ban->next = chan->banhead;
	ban->last = NULL;
	chan->banhead = ban;
}

anUser *add_regnick(const char *user, const char *pass, time_t lastseen,
			time_t regtime, int level, int flag, const char *mail, unsigned long userid)
{
	anUser *u = calloc(1, sizeof *u);
	unsigned int hash = do_hash(user, USERHASHSIZE);

	if(!u)
	{
		printf("error: add_regnick, malloc a échoué pour anUser %s\n", user);
		return NULL;
	}

	Strncpy(u->nick, user, NICKLEN);
	Strncpy(u->mail, mail, MAILLEN);
	strcpy(u->passwd, pass);
	u->lastseen = lastseen;
	u->reg_time = regtime < 1000 ? 0 : regtime;
	u->level = level;
	u->flag = flag;
	u->userid = userid;
	u->lastlogin = NULL;
	u->suspend = u->cantregchan = u->nopurge = NULL;
	++user_count;
	if(userid > lmaxid) lmaxid = userid;

	u->next = user_tab[hash];
	return (user_tab[hash] = u);
}

static int data_load_(aData **d_pp, const char *from, const char *raison, int flag,
	time_t expire, time_t debut, void *data)
{
	aData *d;

	if(!(*d_pp = malloc(sizeof **d_pp)))
		return printf("data::load: OOM from=%s, raison=%s\n", from, raison);

	d = *d_pp;
	Strncpy(d->from, from, NICKLEN);
	*d->raison = 0;
	if(raison && *raison) Strncpy(d->raison, raison, RAISONLEN);
	d->debut = debut;
	d->flag = flag;
	d->data = data;
	d->expire = expire < 0 ? time(0) - 5 : expire;

	switch(d->flag & DATA_TYPES)
	{
		anUser *u = NULL;
		aChan *c = NULL;

		case DATA_T_SUSPEND_CHAN:
			c = d->data;
			if(!expire || expire > time(0)) SetCSuspend(c);
			break;

		case DATA_T_SUSPEND_USER:
			u = d->data;
			if(!expire || expire > time(0)) SetUSuspend(u);
			break;

		case DATA_T_NOPURGE:
			u = d->data;
			SetUNopurge(u);
			break;

		case DATA_T_CANTREGCHAN:
			u = d->data;
			SetUCantRegChan(u);
			break;

		default: /* Who fucked the database ? */
			printf("data::load: aData=%p, data=%p, type=%d not found!\n",
				(void *) d, d->data, d->flag);
	}

	return 0;
}

static void db_parse_data(aData **dpp, void *data, int flag, char **ar)
{ /* <DATA> <from> <fin> <debut> :<raison> */
	data_load_(dpp, ar[1], ar[4], flag, strtol(ar[2], NULL, 10),
		strtol(ar[3], NULL, 10), data);
}

/* We accept ALL language : that's Z' job to check it */
Lang *lang_isloaded(const char *name)
{
	Lang *lang = DefaultLang;
	for(; lang && strcasecmp(lang->langue, name); lang = lang->next);

	if(!lang)
	{
		Lang *temp = DefaultLang;

		if(!(lang = calloc(1, sizeof *lang)))
		{
			printf("error: lang_add, malloc a échoué pour Lang %s\n", name);
			return NULL;
		}
		lang->next = NULL;
		Strncpy(lang->langue, name, sizeof lang->langue -1);

		if(!DefaultLang) DefaultLang = lang;
		else
		{
			for(; temp->next; temp = temp->next); /* ajout à la fin */
			if(temp) temp->next = lang;
		}
	}
	return lang;
}

/* FILE */

static int load_an_user(const char *buf, char **ar, int items, anUser **upp)
{
	int count = (ver_u > 1 ? 12 : 10);
	anUser *u = *upp;

	if(!strcmp(buf, "NICK"))
	{
/* NICK <usr> <pwd> <lvl> <TS> <flag> <RTS> <mail> <host> [v<3:cantregchan] <lang> [v>1:id] */
		char *nick = ar[1], *last = ar[8], *lang = ar[10];
		time_t cantregchan = 0;
		*upp = NULL; /* reset */

		if(items < count)
		{
			printf("Erreur lors de la lecture de la DB pour %s, item incomplet\n", ar[1]);
			return 0;
		}

		if(!(*upp = add_regnick(nick,
								ar[2], 						/* password */
								strtol(ar[4], NULL, 10), 	/* lastseen */
								strtol(ar[6], NULL, 10), 	/* regtime */
								strtol(ar[3], NULL, 10), 	/* level */
								strtol(ar[5], NULL, 10), 	/* flag */
								ar[7], 						/* mail */
								ver_u > 1 ?  strtoul(ar[11], NULL, 10) : 0UL))) /* userid */
		{
			printf("Erreur lors de la lecture de la DB pour %s, erreur interne\n", nick);
			return 0;
		}
		u = *upp;

		if(*last != '0') str_dup(&u->lastlogin, last);
		/* if lang was not available anymore, set to default */
		if(!(u->lang = lang_isloaded(lang))) u->lang = DefaultLang;

		cantregchan = strtol(ar[9], NULL, 10);
		if(!cantregchan || cantregchan > time(0))
		{
			SetUNopurge(u);
			if(cantregchan) data_load_(&u->cantregchan, "Services", NULL,
								DATA_T_CANTREGCHAN, cantregchan, time(0), u);
		}
	}
	else if(!strcmp(buf, "SUSPEND") && items > 4 && u)
			db_parse_data(&u->suspend, u, DATA_T_SUSPEND_USER, ar);

#ifdef USE_MEMOSERV /* MEMO <from> <DateTS> <flag> :message */
	else if(!strcmp(buf, "MEMO") && items > 4 && u)
		add_memo(u, ar[1], strtol(ar[2], NULL, 10), ar[4], strtol(ar[3], NULL, 10));
#endif

	else if(!strcmp(buf, "ACCESS") && items > 5 && u)
	{ /* ACCESS <#> <level> <flag> <lastseenTS> :info */
		anAccess *a = add_access(u, ar[1], strtol(ar[2], NULL, 10),
						strtol(ar[3], NULL, 10), strtol(ar[4], NULL, 10));

		if(a && ar[5] && *ar[5]) str_dup(&a->info, ar[5]);
	}

	else if(!strcmp(buf, "COOKIE") && items > 1 && u)
	{
		if((u->cookie = malloc(PWDLEN + 1))) strcpy(u->cookie, ar[1]);
	}

	else if(strcmp(buf, "V") && u) return 1;
	return 0;
}

static int old_load_users(void)
{
	FILE *fp, *fp1;
	char buff[100];
	anUser *u = NULL;

	if(!(fp = fopen(USERS_FILE, "r"))) return 0;

	while(fgets(buff, sizeof buff, fp))
	{
		char buff1[500] = DBDIR "/usr/", *ar[20];
		strip_newline(buff);
		conv_char(buff, buff1 + sizeof(DBDIR "/usr/") - 1);
		if(!(fp1 = fopen(buff1, "r")))
		{
			printf("convertdb: Impossible d'ouvrir le fichier '%s' pour '%s': %s\n",
				buff1, buff, strerror(errno));
			continue;
		}

		while(fgets(buff1, sizeof buff1, fp1))
		{
			int items = split_buf(buff1, ar, ASIZE(ar));
			if(items < 2) continue;
			strip_newline(ar[items-1]);

			if(load_an_user(buff1, ar, items, &u))
				printf("DB::load: unknown/invalid data %s [%s]\n", buff1, ar[1]);
		}
		fclose(fp1);
	}
	fclose(fp);
	return 0;
}

int db_load_usersv1(void)
{
	FILE *fp;
	char buf[500], *ar[20];
	int line = 0;
	anUser *u = NULL;

	if(!(fp = fopen(DBDIR "/" DBUSERS, "r")))
	{
		printf("convertdb: Impossible d'ouvrir le fichier "DBDIR "/" DBUSERS": %s\n",
			strerror(errno));
		return 0;
	}
	while(fgets(buf, sizeof buf, fp))
	{
		int items = split_buf(buf, ar, ASIZE(ar));
		++line;
		if(items < 2) continue;

		strip_newline(ar[items-1]); /* remove trailing \r\n */

		if(load_an_user(buf, ar, items, &u))
			printf("DB::load: unknown/invalid data %s [%s] line %d\n", buf, ar[1], line);
	}
	fclose(fp);
	return 0;
}

static int load_a_chan(char *buff, char *field, aChan **cpp)
{
	char *ar[10];
	aChan *c = *cpp;

	if(!strcmp(buff, "CHANNEL"))
	{
		char *desc = strchr(field, ' '); /* find desc */
		if(desc) *desc++ = 0;
		else printf("Erreur lors de la lecture de la DB pour %s, desc manquante\n", field);

		if(!(*cpp = add_chan(field, desc)))
		{
			printf("Erreur interne lors du chargement de %s\n", field);
			return 0;
		}
	}
	else if(!strcmp(buff, "DEFMODES") && c)
	{
		char *lim = strchr(field, ' '), *key = NULL;
		if(lim)
		{
			*lim++ = 0;
			if((key = strchr(lim, ' '))) *key++ = 0;
		}
		c->defmodes.modes = 0U; /* otherwise, global defaults would be appended */
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
				printf("DB::load: Options incomplètes pour %s [%s]\n", c->chan, field);

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
		c->creation_time = (creation_t && *creation_t) ? strtoul(creation_t, 0, 10) : 0;
	}
	else if(!strcmp(buff, "SUSPEND") && c && split_buf(field, ar + 1, ASIZE(ar) -1) == 4)
			db_parse_data(&c->suspend, c, DATA_T_SUSPEND_CHAN, ar);

	else if(!strcmp(buff, "B") && c && split_buf(field, ar, ASIZE(ar)) == 6)
	{	/* mask de debut fin level :raison */
		time_t fin = strtol(ar[3], 0, 10);

		if(!fin || time(0) < fin)
			add_ban(c, ar[0], ar[5], ar[1], strtol(ar[2], 0, 10), fin, strtol(ar[4], 0, 10));
	}
	else if(strcmp(buff, "V") && c) return 1;

	return 0;
}

static int old_load_chans(void)
{
	FILE *fp, *fp1;
	char buff[250], *ar[6];

	if(!(fp = fopen(CHANS_FILE, "r"))) return 0;

	while(fgets(buff, sizeof buff, fp))
	{
		char *chan = buff, nchan[3 * REGCHANLEN] = {0}, buff1[500] = DBDIR "/chans/";
		aChan *c;

		strip_newline(buff);
		conv_char(chan, nchan);
		strcpy(buff1 + sizeof(DBDIR "/chans/") - 1, nchan);

		if(!(fp1 = fopen(buff1, "r")))
		{
			printf("convertdb: Impossible d'ouvrir le fichier '%s' pour '%s': %s\n",
					buff1, buff, strerror(errno));
			continue;
		}

		if(!(c = add_chan(chan, "")))
		{
			printf("convertdb: Erreur lors du chargement de %s\n", chan);
			break;
		}

		while(fgets(buff1, sizeof buff1, fp1))
		{
			char *field = strchr(buff1, ' ');
			strip_newline(buff1);
			if(field) *field++ = 0;
			if(!field || !*field) continue;

			load_a_chan(buff1, field, &c);
		}

		fclose(fp1);
		/* load bans */
		snprintf(buff1, sizeof buff1, DBDIR "/bans/%s", nchan);
		if(!(fp1 = fopen(buff1, "r"))) continue;
		while(fgets(buff1, sizeof buff1, fp1))
		{
			time_t fin;
			int i = split_buf(buff1, ar, ASIZE(ar));
			if(i < 6) continue;

			strip_newline(ar[i-1]);

			fin = strtol(ar[3], 0, 10);

			if(!fin || time(0) < fin)
				add_ban(c, ar[0], ar[5], ar[1], strtol(ar[2], 0, 10), fin, strtol(ar[4], 0, 10));
		}
		fclose(fp1);
	}
	fclose(fp);
	return 0;
}

int db_load_chansv1(void)
{
	FILE *fp;
	aChan *c = NULL;
	char buff[512];
	int line = 0;

	if(!(fp = fopen(DBDIR "/" DBCHANS, "r")))
	{
		printf("convertdb: Impossible d'ouvrir le fichier "DBDIR "/" DBCHANS": %s\n",
			strerror(errno));
		return 0;
	}

	while(fgets(buff, sizeof buff, fp))
	{
		char *field = strchr(buff, ' ');
		++line;
		if(field) *field++ = 0;
		else continue;

		strip_newline(field);

		if(load_a_chan(buff, field, &c))
			printf("DB::load: unknown data %s [%s] line %d\n", buff, field, line);
	}
	fclose(fp);
	return 0;
}

static int ts_sort(const void *a, const void *b)
{
	return (*(const anUser **) a)->reg_time - (*(const anUser **) b)->reg_time;
}

static void gen_userid(void)
{
	anUser **tab = malloc(user_count * sizeof *tab), *u;
	unsigned int i = 0U, j = 0U;

	for(; i < ASIZE(user_tab); ++i)
		for(u = user_tab[i]; u; u = u->next)
			tab[j++] = u;

	qsort(tab, user_count, sizeof *tab, ts_sort);

	for(i = 0U; i < user_count; ++i) tab[i]->userid = ++maxid;

	free(tab);
}

int db_write_users(void)
{
	anAccess *a;
	anUser *u;
#ifdef USE_MEMOSERV
	aMemo *m;
#endif
 	int i = 0;
 	FILE *fp;

	rename(DBDIR "/" DBUSERS, DBDIR "/" DBUSERS ".back");

	if(!(fp = fopen(DBDIR "/" DBUSERS, "w")))
		return printf("DB::Write: Impossible d'écrire la DB user: %s\n",
						strerror(errno));

	fprintf(fp, "V %d %lu\n", DBVERSION_U, maxid);

	for(; i < USERHASHSIZE; ++i) for(u = user_tab[i]; u; u = u->next)
	{
		fprintf(fp, "NICK %s %s %d %lu %d %ld %s %s %s %lu\n",
			u->nick, u->passwd,	u->level, u->lastseen, u->flag, u->reg_time, u->mail,
			u->lastlogin ? u->lastlogin : "0", u->lang->langue, u->userid);
		if(u->suspend) fprintf(fp, "SUSPEND %s %ld %ld :%s\n", u->suspend->from,
			u->suspend->expire, u->suspend->debut, u->suspend->raison);
		if(u->nopurge) fprintf(fp, "NOPURGE %s %ld %ld :%s\n", u->nopurge->from,
			u->nopurge->expire, u->nopurge->debut, u->nopurge->raison);
		if(u->cantregchan) fprintf(fp, "CANTREGCHAN %s %ld %ld :%s\n", u->cantregchan->from,
			u->cantregchan->expire, u->cantregchan->debut, u->cantregchan->raison);
		if(u->cookie) fprintf(fp, "COOKIE %s\n", u->cookie);
#ifdef USE_MEMOSERV
		for(m = u->memohead; m; m = m->next)
			fprintf(fp, "MEMO %s %ld %d :%s\n", m->de, m->date, m->flag, m->message);
#endif

    	for(a = u->accesshead; a; a = a->next)
    		fprintf(fp, "ACCESS %s %d %d %lu :%s\n", a->c->chan, a->level,
    			a->flag, AOnChan(a) ? time(0) : a->lastseen, NONE(a->info));
	}

    fclose(fp);
    return 1;
}

int db_write_chans(void)
{
	FILE *fp;
	int i = 0;
	aChan *c;
	aBan *ban;

	rename(DBDIR "/" DBCHANS, DBDIR "/" DBCHANS ".back");

	if(!(fp = fopen(DBDIR "/" DBCHANS, "w")))
		return printf("DB::Write: Impossible d'écrire la DB chans: %s\n",
					strerror(errno));

	fprintf(fp, "V %d\n", DBVERSION_C);

	for(; i < CHANHASHSIZE; ++i) for(c = chan_tab[i]; c; c = c->next)
	{
		fprintf(fp, "CHANNEL %s %s\nOPTIONS %d %d %d %d %lu %u %u\nURL %s %lu\n",
			c->chan, c->description, c->flag, c->banlevel, c->bantype, c->cml,
			c->bantime, c->limit_min, c->limit_inc, c->url, c->creation_time);

		if(c->defmodes.modes) fprintf(fp, "DEFMODES %s\n", GetCModes(c->defmodes));
		if(*c->deftopic) fprintf(fp, "DEFTOPIC %s\n", c->deftopic);
		if(*c->welcome) fprintf(fp, "WELCOME %s\n", c->welcome);
		if(c->motd) fprintf(fp, "MOTD %s\n", c->motd);
		if(c->suspend) fprintf(fp, "SUSPEND %s %ld %ld :%s\n", c->suspend->from,
			c->suspend->expire, c->suspend->debut, c->suspend->raison);

		for(ban = c->banhead; ban; ban = ban->next)
			fprintf(fp, "B %s %s %lu %lu %d :%s\n", ban->mask, ban->de,
				ban->debut, ban->fin, ban->level, ban->raison);
	}

	fclose(fp);
	return 1;
}


static int check_versions(int print)
{
	FILE *fp;
	char buf[500] = {0};

	if((fp = fopen(DBDIR "/" DBUSERS, "r"))) /* look for userfile's version */
	{
		char *max = NULL;
		fgets(buf, sizeof buf, fp);
		ver_u = !strncmp("V ", buf, 2) ? strtol(buf + 2, &max, 10) : 0;
		/* extract max id if available */
		if(max && *max == ' ') maxid = strtol(max + 1, 0, 10);
	}
	else if(!(fp = fopen(USERS_FILE, "r"))) return -1; /* no db at all => first run */
	else ver_u = 0; /* before migration */

	if(fp) fclose(fp);

	if(print && ver_u < DBVERSION_U)
	{
		puts("La DB User n'est pas à jour, lancez 'bin/convertdb -c' pour la convertir.");
		return 0;
	}

	if((fp = fopen(DBDIR "/" DBCHANS, "r")))
	{
		fgets(buf, sizeof buf, fp); /* extract DB version */
		ver_c = !strncmp("V ", buf, 2) ? strtol(buf + 2, 0, 10) : 0;
		fclose(fp);
	}
	else ver_c = 0;

	if(print && ver_c < DBVERSION_C)
	{
		puts("La DB Salon n'est pas à jour, lancez 'bin/convertdb -c' pour la convertir.");
		return 0;
	}

	if(print)
	{
		puts("Les DBs sont à jour.");
		exit(EXIT_SUCCESS);
	}

	return 0;
}

static void load_db(void)
{
	switch(ver_c)
	{
		case 0:
			old_load_chans();
			break;
		case 1:
			db_load_chansv1();
			break;
		default:
			printf("convertdb: No handler for channel DB version %d\n", ver_c);
			exit(EXIT_FAILURE);
	}

	switch(ver_u)
	{
		case 0:
			old_load_users();
			gen_userid();
			break;
		case 1:
			db_load_usersv1();
			if(!lmaxid || lmaxid > maxid) gen_userid();
			break;
		case 2:
			db_load_usersv1();
			break;
		default:
			printf("converdb: No handler for user DB version %d\n", ver_u);
			exit(EXIT_FAILURE);
	}
}

int main(int argc, char **argv)
{
	if(chdir(BINDIR) < 0)
	{
		printf("BINDIR("BINDIR") n'est pas correctement configuré : %s\n", strerror(errno));
		return 0;
	}

	if(argc <= 1) check_versions(1);
	else if(!strcmp("-c", argv[1]))
	{
		if(check_versions(0) < 0)
		{
			puts("C'est le premier lancement des services, il n'y a pas de DB.");
			return 0;
		}

		load_db();
		if(ver_u < DBVERSION_U) db_write_users();
		if(ver_c < DBVERSION_C) db_write_chans();
		puts("Les DB ont été converties");
	}
	return 0;
}
