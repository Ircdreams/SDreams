/* include/hash.h
 * Copyright (C) 2004-2006 ircdreams.org
 *
 * contact: bugs@ircdreams.org
 * site web: http://www.ircdreams.org
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
 * $Id: hash.h,v 1.27 2006/02/28 06:36:45 bugs Exp $
 */

#ifndef HAVEINC_hash
#define HAVEINC_hash

/* 	doivent être des multiples de 2 (1024, 512, 256)
	pour pouvoir (&)and-er au lieu de (%)moduler*/
#define CHANHASHSIZE 512  /* taille de la hash des chan avec une hash de 200 et 200 chan regs
						131 offsets utilisée, temps réduit par 10 pour getchaninfo()*/
#define USERHASHSIZE 2048
#define NICKHASHSIZE 1024
#define NCHANHASHSIZE 1024
#define CMDHASHSIZE 140
#define ALIASHASHSIZE 2048

#define SUSPEND_REMOVE(x) do {                                  \
        if(x) {                                                                         \
                if((x)->timer) timer_remove((x)->timer);\
                free(x);                                                                \
        }                                                                                       \
} while(0)

/* chan */

extern void switch_chan(aChan *, const char *);
extern aChan *getchaninfo(const char *);
extern aChan *add_chan(const char *, const char *);
extern void del_chan(aChan *, int, const char *);

extern void floating_limit_update_timer(aChan *); 
extern void modes_reset_default(aChan *); 
extern void enforce_access_opts(aChan *, aNick *, anAccess *, aJoin *);
    
/* nchan */ 
extern aNChan *GetNChan(const char *); 
extern aNChan *new_chan(const char *, time_t); 
extern void del_nchan(aNChan *); 
extern void do_cs_join(aChan *, aNChan *, int); 

/* nick */
extern unsigned int base64toint(const char *);
extern aNick *num2nickinfo(const char *);
extern aNick *getnickbynick(const char *);
extern aNick *add_nickinfo(const char *, const char *, const char *, const char *,
			const char *, aServer *, const char *, time_t, const char *);
extern void del_nickinfo(const char *, const char *);
extern int switch_nick(aNick *, const char*);
extern void purge_network(void);

/* user */
extern int switch_user(anUser *, const char *);
extern int switch_mail(anUser *, const char *);
extern anUser *getuserinfo(const char *);
extern anUser *getuseralias(const char *);
extern anUser *add_regnick(const char *, const char *, time_t, time_t, int, int, const char *, const char *);
extern void del_regnick(anUser *, int, const char *);
extern char *GetUserOptions(anUser *);
extern int checknickaliasbyuser(const char *, anUser *);
extern int checkmatchaliasbyuser(const char *, anUser *);
extern int hash_addalias(anAlias *);
extern int hash_delalias(anAlias *);

/* cmd */
extern int RegisterCmd(const char *, int, int, int, int (*) (aNick *, aChan *, int, char **));
extern void HashCmd_switch(aHashCmd *, const char *);
extern aHashCmd *FindCommand(const char *);
extern aHashCmd *FindCoreCommand(const char *);
extern char *RealCmd(const char *);

/* misc */
extern anUser *GetUserIbyMail(const char *);
extern int ChanLevelbyUserI(anUser *, aChan *); 
extern aJoin *getjoininfo(aNick *, const char *);
extern aJoin *GetJoinIbyNC(aNick *, aNChan *);
aDNR *find_dnr(const char *, int); 
extern char *IsAnOwner(anUser *); 
extern anAccess *GetAccessIbyUserI(anUser *, aChan *); 
extern aNick *GetMemberIbyNum(aChan *, const char *); 
extern aNick *GetMemberIbyNick(aChan *, const char *); 
extern aServer *num2servinfo(const char *); 
extern aServer *GetLinkIbyServ(const char *);
extern anUser *GetUserIbyVhost(const char *); 
extern int hash_addvhost(anUser *);
extern int hash_delvhost(anUser *);
extern int switch_vhost(anUser *, const char *);
extern char *GetAccessOptions(anAccess *);

extern int handle_suspend(struct suspendinfo **, const char *, const char *, time_t); 
extern void do_suspend(struct suspendinfo **, const char *, const char *, time_t, time_t); 

#endif /*HAVEINC_hash*/
