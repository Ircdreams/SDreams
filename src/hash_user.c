/* src/hash_user.c - gestion des hash user
 * Copyright (C) 2004-2005 ircdreams.org
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
 * $Id: hash_user.c,v 1.26 2006/01/28 12:35:07 bugs Exp $
 */

#include "main.h"
#include "hash.h"
#include "debug.h"
#include "outils.h"
#include "fichiers.h"
#include "del_info.h"
#include "cs_cmds.h"
#include <ctype.h>
#include "timers.h"
#include "track.h"
#include "config.h"

static anUser *mail_tab[USERHASHSIZE];
static anAlias *alias_tab[ALIASHASHSIZE];
static anUser *vhost_tab[USERHASHSIZE];

static inline unsigned int do_hashu(const char *user)
{
	unsigned int checksum = 0;
	while(*user) checksum += (checksum << 3) + tolower((unsigned char) *user++);
	return checksum & (USERHASHSIZE-1);
}

static int hash_deluser(anUser *user)
{
	unsigned int hash = do_hashu(user->nick);
	anUser *tmp = user_tab[hash];

	if(tmp == user) user_tab[hash] = user->next;
	else
	{
		for(;tmp && tmp->next != user;tmp = tmp->next);
		if(tmp) tmp->next = user->next;
		else Debug(W_WARN|W_MAX, "H_del_user %s non trouvé à l'offset %u ?!", user->nick, hash);
	}
	return 0;
}

static int hash_adduser(anUser *user)
{
	unsigned int hash = do_hashu(user->nick); 
        user->next = user_tab[hash]; 
        user_tab[hash] = user; 
        return 0; 
}

int switch_user(anUser *user, const char *newuser) 
{ 
	hash_deluser(user);
	Strncpy(user->nick, newuser, NICKLEN);
	hash_adduser(user);
	if(user->n) cs_account(user->n, user);
	return 0;
}

static inline unsigned int do_hasha(const char *alias)
{
	unsigned int checksum = 0;
	while(*alias) checksum += (checksum << 3) + tolower((unsigned char) *alias++);
	return checksum & (ALIASHASHSIZE-1);
}

int hash_delalias(anAlias *alias)
{
	unsigned int hash = do_hashu(alias->name);
	anAlias *tmp = alias_tab[hash];

	if(tmp == alias) alias_tab[hash] = alias->hash_next;
	else
	{
		for(;tmp && tmp->hash_next != alias;tmp = tmp->hash_next);
		if(tmp) tmp->hash_next = alias->hash_next;
		else Debug(W_WARN|W_MAX, "H_del_alias %s non trouvé à l'offset %u ?!", alias->name, hash);
	}
	return 0;
}

int hash_addalias(anAlias *alias)
{
	unsigned int hash = do_hasha(alias->name); 
        alias->hash_next = alias_tab[hash]; 
        alias_tab[hash] = alias; 
        return 0; 
}

anUser *getuserinfo(const char *nick)
{
	unsigned int hashu = do_hashu(nick);
	unsigned int hasha = do_hasha(nick);
        register anUser *user = user_tab[hashu];
	register anAlias *alias = alias_tab[hasha];
        for(;user;user=user->next) if(!strcasecmp(nick, user->nick)) return user;
	for(;alias;alias=alias->user_nextalias) if(!strcasecmp(nick, alias->name)) return alias->user;
        return 0;
}

int checknickaliasbyuser(const char *nick, anUser *user) 
{ 
        anAlias *alias = NULL; 
        for(alias = user->aliashead;alias;alias = alias->user_nextalias)
        { 
                if(alias && !strcasecmp(nick, alias->name)) 
                        return 1; 
        } 
        return 0; 
}
    
int checkmatchaliasbyuser(const char *check, anUser *user) 
{ 
        anAlias *alias = NULL; 
        for(alias = user->aliashead;alias;alias = alias->user_nextalias) 
        {
                if(alias && !match(check, alias->name)) 
                        return 1;
        } 
        return 0; 
} 

int hash_delvhost(anUser *user)
{
        unsigned int hash = do_hashu(user->vhost);
        anUser *tmp = vhost_tab[hash];
	
	if(!strcasecmp(user->vhost, "none")) return 0;

        if(tmp == user) vhost_tab[hash] = user->vhostnext;
        else
        {
                for(;tmp && tmp->vhostnext != user;tmp = tmp->vhostnext);
                if(tmp) tmp->vhostnext = user->vhostnext;
                else Debug(W_WARN|W_MAX, "H_del_vhost %s non trouvé à l'offset %u ?!", user->vhost, hash);
        }
        return 0;
}

int hash_addvhost(anUser *user)
{
        unsigned int hash = do_hashu(user->vhost);
        user->vhostnext = vhost_tab[hash];
        vhost_tab[hash] = user;
        return 0;
}

int switch_vhost(anUser *user, const char *newvhost)
{
        hash_delvhost(user);
        Strncpy(user->vhost, newvhost, HOSTLEN);
        hash_addvhost(user);
        return 0;
}

anUser *GetUserIbyVhost(const char *vhost)
{
        unsigned int hash = do_hashu(vhost);
        register anUser *tmp = vhost_tab[hash];
        for(;tmp && strcasecmp(vhost, tmp->vhost);tmp = tmp->vhostnext);
        return tmp;
}

static int hash_delmail(anUser *user) 
{ 
        unsigned int hash = do_hashu(user->mail); 
        anUser *tmp = mail_tab[hash]; 
 
        if(tmp == user) mail_tab[hash] = user->mailnext; 
        else 
        { 
                for(;tmp && tmp->mailnext != user;tmp = tmp->mailnext); 
                if(tmp) tmp->mailnext = user->mailnext; 
                else Debug(W_WARN|W_MAX, "H_del_mail %s non trouvé à l'offset %u ?!", user->mail, hash); 
        } 
        return 0; 
} 
    
static int hash_addmail(anUser *user) 
{ 
        unsigned int hash = do_hashu(user->mail); 
        user->mailnext = mail_tab[hash]; 
        mail_tab[hash] = user; 
        return 0; 
} 
 
int switch_mail(anUser *user, const char *newmail) 
{ 
        hash_delmail(user); 
        Strncpy(user->mail, newmail, MAILLEN); 
        hash_addmail(user); 
        return 0; 
} 

anUser *GetUserIbyMail(const char *mail) 
{ 
        unsigned int hash = do_hashu(mail); 
        register anUser *tmp = mail_tab[hash]; 
        for(;tmp && strcasecmp(mail, tmp->mail);tmp = tmp->mailnext); 
        return tmp; 
} 

anUser *add_regnick(const char *user, const char *pass, time_t lastseen,
		time_t regtime, int level, int flag, const char *mail, const char *vhost)
{
	anUser *u = calloc(1, sizeof *u);

	if(!u)
	{
		Debug(W_MAX|W_WARN, "add_regnick, malloc a échoué pour anUser %s", user);
		return NULL;
	}

	Strncpy(u->nick, user, NICKLEN);
	strcpy(u->passwd, pass);
	u->lastseen = lastseen;
	u->reg_time = regtime;
	u->level = level;
	Strncpy(u->mail, mail, MAILLEN);
	u->flag = flag;
	u->suspend = NULL;
	u->lastlogin = NULL;
	u->cantregchan = -1;
	Strncpy(u->vhost, vhost, HOSTLEN);

	hash_adduser(u);
	hash_addmail(u);
	if(strcasecmp(vhost, "none")) hash_addvhost(u);
	return u;
}

void del_regnick(anUser *user, int flag, const char *raison)
{
	anAccess *acces, *acces_t;
	aMemo *memo = user->memohead, *mem;
        struct track *track = istrack(user); 
    
        if(GetConf(CF_TRACKSERV) && track) 
        { 
                csreply(track->tracker, "[\2Track\2] L'UserName %s que vous trackez vient d'être supprimé.", 
                        user->nick); 
                del_track(track); 
        } 

	for(acces = user->accesshead;acces;acces = acces_t)
	{
		acces_t = acces->next;
		if(AOwner(acces)) del_chan(acces->c, flag, raison);
		else del_access(user, acces->c);
	}
	for(;memo;memo = mem)
	{
		mem = memo->next;
		free(memo);
	}

	hash_deluser(user);
	hash_delmail(user);
	SUSPEND_REMOVE(user->suspend);
	if(user->lastlogin) free(user->lastlogin);
	free(user);
}
