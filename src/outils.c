/* src/outils.c - Divers outils
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
 * $Id: outils.c,v 1.98 2006/03/27 07:08:59 bugs Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h> 

#include "main.h"
#include "cs_cmds.h"
#include "admin_manage.h"
#include "hash.h"
#include "outils.h"
#include "config.h"
#include "version.h"
#include <netinet/in.h>
#include <arpa/inet.h>

static const struct Modes {
	char mode;
	int value;
} UMode[] = {
	{ 'i', N_INV },
	{ 'x', N_CRYPT },
	{ 'r', N_REG },
	{ 'o', N_OPER },
	{ 'k', N_SERVICE },
	{ 'a', N_ADM },
	{ 'Z', N_GOD },
	{ 'f', N_FEMME },
	{ 'h', N_HOMME },
	{ 'g', N_DEBUG },
	{ 'w', N_WALLOPS },
	{ 'D', N_DIE },
	{ 'd', N_DEAF },
	{ 'H', N_SPOOF },
	{ 'A', N_HELPER },
	{ 'W', N_WHOIS },
	{ 'C', N_CHANNEL },
	{ 'I', N_IDLE },
	{ 'P', N_PRIVATE },
	{ 'X', N_HIDE },
	{ 'R', N_REGPRIVATE },
	{ 'O', N_STRIPOPER },
},
	CMode[] = {
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
	{ 'R', C_MACCONLY },
	{ 'N', C_MNONOTICE },
	{ 'q', C_MNOQUITPARTS },
	{ 'D', C_MAUDITORIUM },
	{ 'T', C_MNOAMSG },
	{ 'M', C_MNOCAPS },
	{ 'W', C_MNOWEBPUB },
	{ 'P', C_MNOCHANPUB },
};

static const int CModesToFlag[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0 /* ! */, 0 /* " */, 0 /* # */, 0 /* $ */,
	0 /* % */, 0 /* & */, 0 /* ' */, 0 /* ( */, 0 /* ) */, 0 /* * */,
	0 /* + */, 0 /* , */, 0 /* - */, 0 /* . */, 0 /* / */, 0 /* 0 */,
	0 /* 1 */, 0 /* 2 */, 0 /* 3 */, 0 /* 4 */, 0 /* 5 */, 0 /* 6 */,
	0 /* 7 */, 0 /* 8 */, 0 /* 9 */, 0 /* : */, 0 /* ; */, 0 /* < */,
	0 /* = */, 0 /* > */, 0 /* ? */, 0 /* @ */, 0 /* A */, 0 /* B */,
	C_MNOCTCP /* C */, C_MAUDITORIUM /* D */, 0 /* E */, 0 /* F */, 0 /* G */,
	0 /* H */, 0 /* I */, 0 /* J */, 0 /* K */, 0 /* L */, C_MNOCAPS /* M */,
  	C_MNONOTICE /* N */, C_MOPERONLY /* O */, C_MNOCHANPUB /* P */, 0 /* Q */,
  	C_MACCONLY /* R */, 0 /* S */, C_MNOAMSG /* T */, 0 /* U */, 0 /* V */,
  	C_MNOWEBPUB /* W */, 0 /* X */, 0 /* Y */, 0 /* Z */, 0 /* [ */, 0 /* \ */,
  	0 /* ] */, 0 /* ^ */, 0 /* _ */, 0 /* ` */, 0 /* a */, 0 /* b */,
  	C_MNOCTRL /* c */, 0 /* d */, 0 /* e */, 0 /* f */, 0 /* g */,
  	0 /* h */, C_MINV /* i */, 0 /* j */, C_MKEY /* k */, C_MLIMIT /* l */,
  	C_MMODERATE /* m */, C_MMSG /* n */, 0 /* o */, C_MPRIVATE /* p */,
  	C_MNOQUITPARTS /* q */, C_MUSERONLY /* r */, C_MSECRET /* s */, C_MTOPIC /* t */,
  	0 /* u */, 0 /* v */, 0 /* w */, 0 /* x */, 0 /* y */, 0 /* z */,
  	0 /* { */, 0 /* | */, 0 /* } */, 0 /* ~ */, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

int parse_umode(int flag, const char *p)
{ /* default is '+' */
	unsigned int w = 1, i;
	if(!p) return flag;
	for(; *p; ++p)
	{
		if(*p == '+') w = 1;
		else if(*p == '-') w = 0;
		if(*p == ' ') break;
		else
		{
			for(i = 0; i < ASIZE(UMode); ++i)
				if(*p == UMode[i].mode)
				{
					if(w) flag |= UMode[i].value;
					else flag &= ~UMode[i].value;
					break;
				}
		}
	}
	return flag;
}

char *GetModes(int flag)
{
	static char mode[ASIZE(UMode)];
	unsigned int i = 0, j = 0;
	for(; i < ASIZE(UMode); ++i) if(flag & UMode[i].value) mode[j++] = UMode[i].mode;
	mode[j] = '\0';
	return mode;
}

int cmodetoflag(int flag, const char *mode)
{
	int w = 1;
	register int i;

	if(!mode) return flag;
	for(; *mode; ++mode)
	{
		if(*mode == '+') w = 1;
		else if(*mode == '-') w = 0;
		else if((i = CModesToFlag[(unsigned char) *mode]))
		{
			if(w)
			{
				flag |= i;
				if(i == C_MSECRET && flag & C_MPRIVATE) flag &= ~C_MPRIVATE;
				else if(i == C_MPRIVATE && flag & C_MSECRET) flag &= ~C_MSECRET;
			}
			else flag &= ~i;
		}
	}
	return flag;
}

int string2scmode(struct cmode *mode, const char *modes, const char *key, const char *limit)
{
    int newmode = cmodetoflag(mode->modes, modes);

	if(newmode & C_MLIMIT)
	{
		int lim = 0;
		if(limit && (lim = strtol(limit, NULL, 10))) mode->limit = lim;
		else if(!(mode->modes & C_MLIMIT)) newmode &= ~C_MLIMIT;/* limit actually invalid */
	}
	else mode->limit = 0;

	if(newmode & C_MKEY)
	{
		if(key && *key) Strncpy(mode->key, key, KEYLEN);
		else if(!*mode->key) newmode &= ~C_MKEY;/* actually no key set */
	} /* -k autorisé seulement si key fournie */
	else if(mode->modes & C_MKEY && (!key || !*key)) newmode |= C_MKEY;
	else *mode->key = 0;
	return (mode->modes = newmode);
}

char *GetCModes(struct cmode modes)
{
	static char mode[ASIZE(CMode) + KEYLEN + 20];
	unsigned int i = 0, j = 0;

	for(; i < ASIZE(CMode); ++i) if(modes.modes & CMode[i].value) mode[j++] = CMode[i].mode;

	if(modes.modes & C_MLIMIT) j += mysnprintf(mode + j, sizeof mode - j, " %d", modes.limit);
	if(modes.modes & C_MKEY)
	{
		mode[j++] = ' ';
		strcpy(mode + j, modes.key);
	}
	else mode[j] = '\0';
	return mode;
}

void CModes2MBuf(struct cmode *modes, MBuf *mbuf, int flag)
{
	unsigned int i = 0, j = 0;

	*mbuf->param = 0; /* init */
	for(; i < ASIZE(CMode); ++i)
		if(modes->modes & CMode[i].value) mbuf->modes[j++] = CMode[i].mode;
	mbuf->modes[j] = 0;

	if(modes->modes & C_MLIMIT && !(flag & MBUF_NOLIMIT))
		j = mysnprintf(mbuf->param, sizeof mbuf->param, "%d ", modes->limit);
	else j = 0;
	if(modes->modes & C_MKEY && !(flag & MBUF_NOKEY))
		strcpy(mbuf->param + j, modes->key);
}

void modes_reverse(aNChan *chan, const char *modes, const char *key, const char *limit)
{
        char modesp_b[20] = "+", modesl_b[20] = "-", marg[KEYLEN + 20] = {0};
        int modesp_i = 1, modesl_i = 1, marg_i = 0, sign = 1, needed = 0;;
  	 
        for(; *modes; ++modes)
        {
                switch(*modes)
                {
                        case 'o': case 'b': case 'h': case 'v':
                                break;
  	 
                        case '+': sign = 1; break;
                        case '-': sign = 0; break;
  	 
                        case 'k': /* same key as ours */
                                if(sign && key && !strcmp(chan->modes.key, key)) break;
                                needed |= C_MKEY;
                                break;
  	 
                        case 'l': /* same limit as ours */
                                if(sign && limit && strtol(limit, NULL, 10) == chan->modes.limit) break;
                                needed |= C_MLIMIT;
                                break;
  	 
                        default:
                                if(sign && HasMode(chan, CModesToFlag[(unsigned char) *modes])) break;

                                if(!sign) modesp_b[modesp_i++] = *modes;
                                else modesl_b[modesl_i++] = *modes;
                                break;
                } /* switch */
        } /* for */
        /* Key and limit are handled at the end so as to be sure of their respective
         * order in a modebuf, therefore marg is in the right order too */
        if(needed & C_MLIMIT)
        {
                if(HasMode(chan, C_MLIMIT)) /* I had a limit, different than yours! */
                {       /* set back to mine */
                        modesp_b[modesp_i++] = 'l';
                        marg_i = mysnprintf(marg, sizeof marg, "%d ", chan->modes.limit);
                }
                else modesl_b[modesl_i++] = 'l';
        }
        if(needed & C_MKEY)
        {
                if(HasMode(chan, C_MKEY)) modesp_b[modesp_i++] = 'k';
                else modesl_b[modesl_i++] = 'k';
                strcpy(marg + marg_i, HasMode(chan, C_MKEY) ? chan->modes.key : key);
        }

        if(modesp_i > 1 || modesl_i > 1)
        {
                modesp_b[modesp_i > 1 ? modesp_i : 0] = 0;
                modesl_b[modesl_i > 1 ? modesl_i : 0] = 0;
                /* then, flush modes */
                putserv("%s "TOKEN_MODE" %s %s%s %s", cs.num, chan->chan, modesp_b, modesl_b, marg);
        }
}

#define b64to10(x) convert2n[(unsigned char) (x)]

static const unsigned char convert2n[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,52,53,54,55,
   56,57,58,59,60,61, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,
   13,14,15,16,17,18,19,20,21,22,23,24,25,62, 0,63, 0, 0, 0,26,27,28,29,30,31,32,
   33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51, 0, 0, 0, 0, 0
};

unsigned int base64toint(const char *s)
{
     unsigned int i = 0;
     while(*s) i = (i << 6) + b64to10(*s++);
     return i;
}

int is_ip(const char *ip)
{
	char *ptr = NULL;
	int i = 0, d = 0;

	for(;i < 4;++i) /* 4 dots expected (IPv4) */
	{       /* Note about strtol: stores in endptr either NULL or '\0' if conversion is complete */
		if(!isdigit((unsigned char) *ip) /* most current case (not ip, letter host) */
			|| (d = strtol(ip, &ptr, 10)) < 0 || d > 255 /* ok, valid number? */
			|| (ptr && *ptr != 0 && *ptr != '.' && ptr != ip)) return 0;
		if(ptr) ip = ptr + 1, ptr = NULL; /* jump the dot */
	}
	return 1;
}

char *GetIP(const char *base64)
{
	struct in_addr addr2;

	addr2.s_addr = htonl(base64toint(base64));
	return inet_ntoa(addr2);
}

/*
 * getoption:
 * recherche dans un tableau char ** (de 'parc' entrées)
 * l'expression "opt" à partir de l'élément 'deb'
 * -si exp_int est > 0:
 *  cela veut dire qu'on attend une valeur numerique apres le char 'opt'
 *  si c'est le cas, la fct renvoit la valeur, sinon 0
 * -si exp_int est < 0:
 *  cela veut dire que l'on recherche uniquement si 'opt' est dans parv,
 *  si c'est le cas, retourne 1
 * -si exp_int est = 0:
 *  cela veut dire que l'on attend une string apres le 'opt',
 *  si c'est le cas retourne l'index de cette string dans parv, sinon 0.
 *
 */

int getoption(const char *opt, char **parv, int parc, int deb, int exp_int)
{
	for(;deb <= parc;++deb)
		if(!strcasecmp(opt, parv[deb]))
		{
			if(exp_int < 0) return 1;
			if(++deb <= parc)
			{
				if(!exp_int) return deb;
				else if(is_num(parv[deb])) return strtol(parv[deb],NULL,10);
			}
			break;
		}
	return 0;
}

char *duration(int s)
{
	static char dur[30 /* 5 + 4 + 2 + 4 + 2 + 5 + 3 + 1 */];
	int i = 1;
	dur[0] = '\002'; 
    
        if(s >= 86400) 
		i += mysnprintf(dur + i, sizeof dur - i, "%d", s/86400), 
                                s %= 86400, strcpy(dur + i, "\2j \002"), i += 4; 

        if(s >= 3600) 
		i += mysnprintf(dur + i, sizeof dur - i, "%d", s/3600), 
                                s %= 3600, strcpy(dur + i, "\2h \002"), i += 4; 
        if(s >= 60) 
		i += mysnprintf(dur + i, sizeof dur - i, "%d", s/60),
				s %= 60, strcpy(dur + i, "\2mn \002"), i += 5;
	if(s) i += mysnprintf(dur + i, sizeof dur - i, "%d\2s",s);
        else dur[i-2]= 0; 

	return dur;
}

time_t convert_duration(const char *p)/* extrait une durée en secondes*/
{										/* d'un format XdXhXm.. - Cesar*/
	const char *p1 = p;
	unsigned long int compute = 0;

	while(*p1)
	{
		if(!isdigit(*p1) && *p1 != 'h' && *p1 != 'H' && *p1 != 'j'
			&& *p1 != 'J' && *p1 != 'd' && *p1 != 'D' && *p1 != 'm')
				return 0; /* invalid format or integer */
		switch(*p1)
		{
			case 'H': case 'h':
						compute += 3600 * strtoul(p, NULL, 10);
						p = p1 + 1;
						break;
			case 'm':
						compute += 60 * strtoul(p, NULL, 10);
						p = p1 + 1;
						break;
			case 'd': case 'j':
			case 'J': case 'D':
						compute += 86400 * strtoul(p, NULL, 10);
						p = p1 + 1;
						break;
		}
		++p1;
	}
	if(compute >= 0) compute += strtoul(p, NULL, 10); /* add trailing seconds */
	if(compute + CurrentTS > LONG_MAX) return 0; /* Would overflow a time_t */
	return (time_t) compute; /* finally convert it to time_t */
}

char *get_time(aNick *nick, time_t mytime)
{
        static char buftime[TIMELEN + 1];
        register struct tm *lt = localtime(&mytime);
        const char *ptr = nick ? GetReply(nick, lt->tm_wday + L_DAYS) :
                                                         DefaultLang->msg[lt->tm_wday + L_DAYS];
        int i = 0;

        while((buftime[i++] = *ptr++));

        buftime[i-1] = ',';
        buftime[i] = ' ';

        snprintf(buftime + i + 1, sizeof buftime, "%d-%02d-%02d %02d:%02d:%02d",
                1900 + lt->tm_year,     lt->tm_mon + 1, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);

        return buftime;
}

char *GetNUHbyNick(aNick *nick, int type)
{
	static char nuhbuf[NUHLEN];
	register char *ptr = nick->nick, *ptr2 = nuhbuf;

	while((*ptr2++ = *ptr++));
	for(*(ptr2-1) = '!', ptr = nick->ident;(*ptr2++ = *ptr++););
	for(*(ptr2-1) = '@', GetConf(CF_HAVE_CRYPTHOST) ? (ptr = (type == 1) ? nick->crypt : nick->host) : (ptr = nick->host);
		(*ptr2++ = *ptr++););

	return nuhbuf;
}

char *GetPrefix(aNick *nick)
{
	static char pref[12];
	int i = 0;
	pref[0] = '\0';

	if(IsOper(nick)) strcpy(pref, "\2\0037*\2\003"), i = 6;
	if(nick->user) strcpy(pref + i, IsAdmin(nick->user) ? "\00312^\003" : "\0035!\003");
	return pref;
}

char *GetChanPrefix(aNick *nick, aJoin *join)
{
	static char pref[23];
	int i = 0;

	if(IsOper(nick)) strcpy(pref, "\0037*\003"), i = 4;
	if(IsHelper(nick)) strcpy(pref + i, "\0033$\003"), i += 4;
	if(nick->user)
		strcpy(pref + i, IsAnAdmin(nick->user) ? "\00312^\003" : "\00305!\003"), i += 5;
	if(IsOp(join)) pref[i++] = '@';
	if(IsVoice(join)) pref[i++] = '+';
	if(IsHalfop(join)) pref[i++] = '%';
	pref[i] = 0;

	return pref;
}

void putlog(const char *fichier, const char *fmt, ...)
{
	va_list vl;
	FILE *fp = fopen(fichier, "a");

	if(!fp) return;

	fputc('[', fp);
	fputs(get_time(NULL, CurrentTS), fp);
	fputs("] ", fp);

	va_start(vl, fmt);
	vfprintf(fp, fmt, vl);
	va_end(vl);

	fputc('\n', fp);
	fclose(fp);
}

int check_protect(aNick *from, aNick *cible, aChan *chan)
{
	anAccess *a1, *a2;

	if(from == cible) return 1;
	if(cible->flag & (N_GOD|N_SERVICE))
		return csreply(from, "L'user \2%s\2 est protégé", cible->nick);

	if(!cible->user) return 1;

	if(IsAnHelper(cible->user) && !IsAdmin(from->user))
                return csreply(from, "L'user \2%s\2 est un Agent d'aide du réseau.", cible->nick);

        if(from->user->level > cible->user->level) return 1;

	/* from admin de niveau supérieur */
	if(IsAdmin(from->user)) return csreply(from, "L'user \2%s\2 est un Admin "
		"de niveau supérieur ou égal au votre.", cible->nick);
	if(IsAdmin(cible->user)) /* cible est admin */
		return csreply(from, "L'user \2%s\2 est un Administrateur des Services.", cible->nick);

	/* optimisation: on check dabord l'acces de l'executé! */
	if(!(a2 = GetAccessIbyUserI(cible->user, chan)) || ASuspend(a2) || !AProtect(a2)) return 1;
	
	/* on boucle les access de from pour voir si on en trouve 1 sur le chan */
	if(!(a1 = GetAccessIbyUserI(from->user, chan)) || ASuspend(a1)) return 0;
	/* Should not happen: from is neither Admin nor has access */

	if(a1->level > a2->level) return 1;
	/* on verifie si from a un level superieur a cible, ou si cible n'est pas protect */
	csreply(from, "L'user \2%s\2 est protégé ou a un access supérieur au votre sur \2%s\2.", cible->nick, chan->chan);
	return 0; /*arrive pour tous les autres cas*/
}

int IsValidNick(char *p)
{
	if(*p == '-' || isdigit(*p)) return 0;
	while(*p)
	{
		if(!isalnum(*p) && !strchr(SPECIAL_CHAR, *p)) return 0;
		++p;
	}
	return 1;
}

int IsValidMail(char *p)
{
	int fa = 0, fd = 0, ispoint = 0;
	if(strchr("-_.@+", *p)) return 0;
	while(*p)
	{
		if(*p == '@')
		{
			if(!fa) fa = 1;
			else return 0;
		}
		if(fa && *p != '.') ispoint = 0;
		if(fa && *p == '.')
		{
			if(ispoint) return 0;/* last char was a dot too.. */
			fd = 1;
			ispoint = 1;
		}
		if(!isalnum(*p) && *p != '.' && *p != '-' && *p != '_' && *p != '@' && (fa || *p != '+')) return 0; 
		++p;
	}
	if(fa && fd) return 1;
	return 0;
}

anUser *ParseNickOrUser(aNick *nick, char *arg)
{
	anUser *user = NULL;

	if(*arg == '%') /* recherche sur le login */
	{
		if(*++arg == 0)
                        csreply(nick, "Veuillez spécifier un UserName après '%%'");
 		else if(!(user = getuserinfo(arg))) /* recherche sur le username */
			csreply(nick, GetReply(nick, L_NOSUCHUSER), arg);
	} else { /* recherche sur le pseudo courant */
		aNick *n;
		if(!(n = getnickbynick(arg))) csreply(nick, GetReply(nick, L_NOSUCHNICK), arg);
		else if(!n->user) csreply(nick, GetReply(nick, L_NOTLOGUED), arg);
		else return n->user;
	}
	return user;
}

int switch_option(aNick *nick, const char *choix, const char *opt, 
	const char *targ, int *flagto, int flag) 
{
	if(!choix) *flagto ^= flag;
	else if(!strcasecmp(choix, "on")) *flagto |= flag;
	else if(!strcasecmp(choix, "off")) *flagto &= ~flag;
	else return csreply(nick, GetReply(nick, L_SHOULDBEONOFF));

	if(*flagto & flag) csreply(nick, GetReply(nick, L_OPTIONNOWON), opt, targ);
	else csreply(nick, GetReply(nick, L_OPTIONNOWOFF), opt, targ);
	return 0;
}

int cswall(const char *format, ...)
{
	int i = 0;
	char buf[400];
	va_list vl;

	va_start(vl, format);
	myvsnprintf(buf, sizeof buf, format, vl);
	va_end(vl);

	for(;i < adminmax;++i) if(adminlist[i]) csreply(adminlist[i], "%s", buf);

	return 1;
}

/* importation de IrcDreams */

int IsValidHost(char *host)
{
  char *s;
  char nhost[(HOSTLEN + 1)];

  /* Sanity check */
  if (!host)
    return -1;

  /* Copy host to a temp. array */
  Strncpy(nhost,host,HOSTLEN);

  /* If current char. in hostmask is not alphanumeric AND isn't "-" or ".", it's
   * invalid according to DNS RFC - so disallow
   */

  for (s = nhost; *s; s++) {
    if (!isalnum(*s) && !isdigit(*s) && !(*s == '.') && !(*s == '-') && !(*s == '_') && !(*s == '~') &&!(*s == 'é') &&!(*s == 'è') &&!(*s == 'ç')
	&&!(*s == 'à') &&!(*s == 'ê') &&!(*s == 'ù') &&!(*s == 'û') &&!(*s == 'î') &&!(*s == 'ï') &&!(*s == 'â')) {
       return 0;
    }
  }

  return 1;
}

int item_isinlist(const int *list, const int max, const int item)
{ 
        int i = 0; 

	for(;i < max;++i) if(list[i] == item) return i;
        return -1; 
} 
    
int item_parselist(char *list, int *notes, const int size) 
{ 
    char *ptr = NULL, *p; 
    int i = 0;

    for(p = Strtok(&ptr, list, LISTSEP);p && i < size; p = Strtok(&ptr, NULL, LISTSEP))
	if((notes[i] = atoi(p)) > 0 && item_isinlist(notes, i, notes[i]) == -1) ++i;

    return i;
}

/* Repris en majorité d'ircu (Carlo Wood (Run)) */
char *pretty_mask(char *mask)
{
	static char star[2] = "*", retmask[NUHLEN];
	char *last_dot = NULL, *ptr = mask;
	/* Case 1: default */
	char *nick = mask, *user = star, *host = star;

	/* Do a _single_ pass through the characters of the mask: */
	for(; *ptr; ++ptr)
	{
		if(*ptr == '!')
		{	/* Case 3 or 5: Found first '!' (without finding a '@' yet) */
			user = ++ptr;
			host = star;
		}
		else if(*ptr == '@')
		{	/* Case 4: Found last '@' (without finding a '!' yet) */
			nick = star;
			user = mask;
			host = ++ptr;
		}
		else if(*ptr == '.')
		{	/* Case 2: Found last '.' (without finding a '!' or '@' yet) */
			last_dot = ptr;
			continue;
		}
		else continue;

		for(; *ptr; ++ptr) if(*ptr == '@') host = ptr + 1;
		break;
	}
	if(user == star && last_dot)
	{	/* Case 2: */
		nick = star;
		host = mask;
	}
	/* Check lengths */
	if(nick != star)
	{
		char *nick_end = (user != star) ? user - 1 : ptr;
		if(nick_end - nick > NICKLEN) nick[NICKLEN] = 0;
		*nick_end = 0;
	}
	if(user != star)
	{
		char *user_end = (host != star) ? host - 1 : ptr;
		if(user_end - user > USERLEN)
		{
			user = user_end - USERLEN;
			*user = '*';
		}
		*user_end = 0;
	}
	if(host != star && ptr - host > HOSTLEN)
	{
		host = ptr - HOSTLEN;
		*host = '*';
	}

        if(!*host) host = star; /* prevent '*!@*' or 'a!b@' */
        if(!*user) user = star;
        if(!*nick) nick = star; /* ... and !b@c */

	for(ptr = retmask;*nick;++nick) if(*nick != '*' || ptr == retmask || nick[-1] != '*') *ptr++ = *nick;
	for(*ptr++ = '!';*user;++user) if(*user != '*' || ptr[-1] != '*') *ptr++ = *user;
	for(*ptr++ = '@';*host;++host) if(*host != '*' || ptr[-1] != '*') *ptr++ = *host;
	*ptr = 0;
	return retmask;
}

void buildmymotd()
{
        FILE *fp;

        if(!(fp = fopen(bot.server, "a"))) return;
        fprintf(fp, "- SDreams Version %s\r\n", SPVERSION);
	fprintf(fp, "- Modules actifs:\r\n");
	fprintf(fp, "- Nickerv: %s\r\n", GetConf(CF_NICKSERV) ? "Oui" : "Non");
        fprintf(fp, "- MemoServ: %s\r\n", GetConf(CF_MEMOSERV) ? "Oui" : "Non");
        fprintf(fp, "- WelcomeServ: %s\r\n", GetConf(CF_WELCOMESERV) ? "Oui" : "Non");
        fprintf(fp, "- VoteServ: %s\r\n", GetConf(CF_VOTESERV) ? "Oui" : "Non");
        fprintf(fp, "- TrackServ: %s\r\n", GetConf(CF_TRACKSERV) ? "Oui" : "Non");
        fprintf(fp, "- WebServ: %s\r\n", GetConf(CF_WEBSERV) ? "Oui" : "Non");
	fprintf(fp, "- \r\n");
	fprintf(fp, "- Créé par BuGs - Dupont Cédric <bugs@ircdreams.org>\r\n");
	fprintf(fp, "- © IrcDreams.org\r\n");
	fprintf(fp, "- Compilé le " __DATE__ " "__TIME__ "\r\n");
	
        fclose(fp);
}
