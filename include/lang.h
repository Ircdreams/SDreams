/* src/lang.h - Gestion du multilangage
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
 * $Id: lang.h,v 1.24 2007/12/01 02:22:31 romexzf Exp $
 */

#define LANGMSGNB 188 /* nombre total de messages (dans le langage par défaut) */

#define LANGMSGMAX 150 /* longueur max d'un message si supérieure,
 						  le message est tronqué (pb possible avec les formateurs */

#define LANG_PATH "lang" /* path à partir de BINDIR dans lequel se trouvent
 							les fichiers .lang */

extern struct Lang *lang_isloaded(const char *);
extern int lang_add(char *);
extern int lang_clean(void);
extern int lang_check_default(void);

extern int LangCount;

#define L_NOSUCHUSER 			0
#define L_NOSUCHCHAN 			1
#define L_NOSUCHNICK 			2
#define L_INFO_ABOUT 			3
#define L_LOGUEDIN 	 			4
#define L_CMDALREADYUSED		5
#define L_USERLENLIMIT 			6 /* username limité à %d*/
#define L_MAILLENLIMIT 			7
#define L_CHANNELNAME			8 /* channel begins with# */
#define L_USER_INVALID 			9
#define L_UCANTREG_INUSE	 	10
#define L_MAIL_INVALID 			11
#define L_MAIL_INUSE 			12
#define L_TOOKBADNICK 			13
#define L_ADM_USER_REGUED 		14
#define L_PASS_SENT 			15
#define L_BADPASS 				16
#define L_LOGINSUCCESS 			17
#define L_LOGINADMIN 			18
#define L_PASS_LIMIT 			19
#define L_LOGINMOTD 			20
#define L_ACCESSON 				21
#define L_PROPACCESS 			22
#define L_NOACCESS 				23
#define L_NOPROPACCESS 			24
#define L_MAILIS 				25
#define L_USERISADMIN 			26 /**/
#define L_OPTIONS 				27 /* Options: %s*/
#define L_ALREADYREG 			28 /* %s déjà reg*/
#define L_CMDDISABLE 			29
#define L_NEEDCONFIRM 			30
#define L_ELSEALREADYLOG	 	31
#define L_ELSETRYTOLOG 			32
#define L_OKCHANGED 			33
#define L_UNKNOWNOPTION 		34
#define L_TOOKREGNICK 			35
#define L_NICKAVAIABLE 			36
#define L_LASTLOGINTIME 		37
#define L_LANGLIST 				38
#define L_NEWLANG 				39
#define L_NOSUCHLANG 			40
#define L_REGISTERTIMEOUT 		41
#define L_NEEDTOBEOP 			42
#define L_CHANNOWREG 			43
#define L_CHANUNREG 			44
#define L_ALREADYOWNER 			45
#define L_MINDESCRIPTIONLEN 	46
#define L_NEEDTOBEOWNER 		47
#define L_VALIDLEVEL 			48
#define L_NOTONCHAN 			49
#define L_MAXLEVELISYOURS 		50
#define L_NOTMATCHINGMAIL 		51
#define L_IDENTICMAIL 			52
#define L_XNOACCESSON 			53
#define L_YOUNOACCESSON 		54
#define L_GREATERLEVEL 			55
#define L_SHOULDBEONOFF 		56
#define L_OPTIONNOWON 			57
#define L_OPTIONNOWOFF 			58
#define L_NOMATCHINGBAN 		59
#define L_BANDELETED 			60
#define L_CANTREMOVEBAN 		61
#define L_NOSUCHCMD 			62
#define L_LOCKTOPIC 			63
#define L_INFOLINEORNONE 		64
#define L_DEFMODESARE 			65
#define L_WELCOMEIS				66
#define L_MOTDIS 				67
#define L_DEFTOPICIS 			68
#define L_DESCRIPTIONIS 		69
#define L_CHANURLIS 			70
#define L_BANLEVELIS 			71
#define L_CHMODESLEVELIS	 	72
#define L_BANTIMEIS 			73
#define L_BANTYPEIS 			74
#define L_AUTOLIMITIS 			75
#define L_OKDELETED 			76
#define L_NOTLOGUED 			77
#define L_LEVELCHANGED 			78
#define L_INFOCHANGED 			79
#define L_NOINFOAVAILABLE 		80
#define L_INCORRECTDURATION 	81
#define L_BANLVLON 				82
#define L_NOBANSON 				83
#define L_TOTALBANDEL 			84
#define L_TOTALBAN 				85
#define L_ALREADYMATCHED 		86
#define L_CIOWNER 				87
#define L_CIDESCRIPTION 		88
#define L_CIDEFMODES 			89
#define L_CIDEFTOPIC 			90
#define L_CIMOTD 				91
#define L_CICHANURL 			92
#define L_CIBANLVL 				93
#define L_CICHMODESLVL 			94
#define L_CIWELCOME 			95
#define L_CIACTUALTOPIC 		96
#define L_CIACTUALMODES 		97
#define L_CIOPTIONS 			98
#define L_BLMASK 				99
#define L_BLFROM 				100
#define L_BLEXPIRE 				101
#define L_XISOWNER 				102
#define L_TOTALFOUND 			103
#define L_CMLIS 				104
#define L_CANTCHANGEMODE 		105
#define L_ALREADYAWAIT 			106
#define L_ALREADYACCESS 		107
#define L_XREFUSEACCESS 		108
#define L_XHASACCESS 			109
#define L_XHASAWAIT 			110
#define L_YOUHAVEACCESS 		111
#define L_YOUHAVEAWAIT1 		112
#define L_YOUHAVEAWAIT2 		113
#define L_ALREADYONCHAN 		114
#define L_XISBANCANTINV 		115
#define L_CANTINVELSE 			116
#define L_CHANMAXLEN 			117
#define L_CANREGDNR 			118
#define L_ALREADYYOURNICK 		119
#define L_DEAUTHBYRECOVER 		120
#define L_YOUNOPROPACCESS 		121
#define L_NOWACCESS 			122
#define L_PROPREFUSED 			123
#define L_OWNERMUSTUNREG 		124
#define L_NICKKILLED 			125
#define L_NICKCHANGED 			126
#define L_ACCESSSUSPENDED 		127
#define L_ACCESSTOOLOW 			128
#define L_PREFUSE				129
#define L_PACCEPT 				130
#define L_PASK 					131
#define L_SHOWCMDUSER 			132
#define L_SHOWCMDADMIN 			133
#define L_SHOWCMDCHAN 			134
#define L_SHOWCMDHELP1 			135
#define L_SHOWCMDHELP2 			136
#define L_ACCESSLIST 			137
#define L_ACUSER 				138
#define L_ACLASTSEEN 			139
#define L_ACONCHAN 				140
#define L_MEMOTOTAL 			141
#define L_MEMONEWTOTAL 			142
#define L_MEMOEND 				143
#define L_NOMEMOFOUND 			144
#define L_MEMONEWNOTFOUND 		145
#define L_MEMONOTFOUND 			146
#define L_MEMOSDEL 				147
#define L_MEMODEL 				148
#define L_CANTSENDYOURSELF 		149
#define L_XNOMEMO 				150
#define L_MEMOLEN 				151
#define L_MAXMEMO 				152
#define L_MEMOSENT 				153
#define L_MEMOFROM 				154
#define L_WELCOMEJOIN 			155
#define L_NEEDTOBEADMIN 		156
#define L_HAVENEWMEMO 			157
#define L_NOADMINAVAILABLE 		158
#define L_ADMINAVAILABLE 		159
#define L_MOREHELP 				160
#define L_FULLLASTSEEN 			161
#define L_UPTIME 				162
#define L_HELP1 				163
#define L_HELP2 				164
#define L_FLOODMSG 				165
#define L_EXCESSMATCHES 		166
#define L_DAYS 					167
#define L_DAYSM					168
#define L_DAYSMM				169
#define L_DAYSJ					170
#define L_DAYSV					171
#define L_DAYSS					172
#define L_DAYSD					173
#define L_SPNICK 				174
#define L_SPKILL 				175
#define L_SPNONE 				176
#define L_SECURECMD				177
#define L_NEED_PASS_TWICE 		178
#define L_PASS_CHANGE_DIFF 		179
#define L_UREGTIME 				180
#define L_NEEDMEMBERSHIP 		181
#define L_MAXMEMOWARN 			182
#define L_CHANNELPURGEWARN 		183
#define L_UWASSUSPEND 			184
#define L_CANTREGCHAN 			185
#define L_JUSTLOGOUT 			186
#define L_SERVICESWELCOME 		187
