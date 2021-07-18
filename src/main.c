 /* src/main.c - Fichier principal
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
 * $Id: main.c,v 1.143 2006/03/16 07:08:43 bugs Exp $
 */

#include "main.h"
#include <unistd.h>
#include <signal.h>
#include <sys/resource.h>
#ifdef USEBSD
#include <netinet/in.h>
#endif
#include <arpa/inet.h>
#include <errno.h>
#include "config.h"
#include "debug.h"
#include "hash.h"
#include "outils.h"
#include "serveur.h"
#include "fichiers.h"
#include "chanserv.h"
#include "admin_user.h"
#include "nickserv.h"
#include "admin_cmds.h"
#include "admin_chan.h"
#include "admin_manage.h"
#include "divers.h"
#include "moduser.h"
#include "chanopt.h"
#include "opdeop.h"
#include "chaninfo.h"
#include "showcommands.h"
#include "aide.h"
#include "cs_register.h"
#include "cs_cmds.h"
#include "timers.h"
#include "socket.h"
#include "ban.h"
#include "flood.h"
#include "template.h"
#include "track.h"
#include "vote.h"
#include "memoserv.h"
#include "welcome.h"
#include "stats.h"
#include "version.h"
#include "webserv.h"

int ConfFlag = 0;
int running = 1;
int deconnexion = 0;
time_t CurrentTS = 0;
int complete = 0;
int nbuser = 0;
int nbmaxuser = 0;
int burst = 0;

struct ignore *ignorehead = NULL;
struct cntryinfo *cntryhead = NULL;
aKill *killhead = NULL;
aDNR *dnrhead = NULL;
Timer *Timers = NULL;
Lang *DefaultLang = NULL;

struct robot bot = {{0}};
char user_motd[400] = {0};
char admin_motd[400] = {0};
struct bots cs = {{0}};

aChan *chan_tab[CHANHASHSIZE] = {0}; 	/* hash chan*/
anUser *user_tab[USERHASHSIZE] = {0}; 	/* hash username*/
aNChan *nchan_tab[NCHANHASHSIZE] = {0};         /* hash nchan */
aNick **num_tab[MAXNUM] = {0}; 			/* table num -> struct nick*/
aServer *serv_tab[MAXNUM] = {0}; 		/* table numserv -> serv info*/
aNick *nick_tab[NICKHASHSIZE] = {0}; 	/* hash nick*/

/*            FONCTIONS COMMANDES
 *            =-=-=-=-=-=-=-=-=-=
 *
 *    Il y a deux types de fonctions commandes.
 *    Les normales :
 *       parv[0] = pseudo du robot appelé
 *       parv[1->parc] = arguments de la commandes
 *    Et les commandes salons (voir strict cs_commands) :
 *       parv[0] = nom de la commande appelée
 *       parv[1] = salon appelé
 *       parv[2->parv] = arguments de la commande (après le nom du salon)
 *    Toutes les commandes doivent respecter ce schéma.
 *    Lorsqu'une commande salon est tapée sur un salon, et que le premier
 *    argument n'est pas un nom de salon, le nom du salon actuel est placé
 *    automatiquement comme premier argument.
 *
 *    Chaque commande emploie une entête particulière :
 *    int commande(aNick *, aChan *, int, char **)
 *      aNick *nick = structure nickinfo de l'user qui a appelé la fonction
 *      aChan *chaninfo = structure chaninfo du salon si la cmd est chan=1 (sinon chaninfo=NULL)
 *      int parc = nombre d'arguments *SANS COMPTER PARV[0]*. (si le dernier arg
 *                 est parv[5], alors parc = 5.)
 *      char **parv = tableau contenant les arguments (une ligne = un mot).
 *
 *    Outils utiles :
 *      parv2msgn(int parc, char **parv, int position, char *buf, int size)
 *          permet de convertir une fin de tableau en un message normal.
 *          il faut spécifier le tableau et le nombre d'arguments contenu
 *          ainsi que la position à laquelle on va commencer à enregistrer
 *          le contenu dans la variable envoyée en paramètre (taille max size).
 *		parv2msg(int parc, char **parv, int position, int size)
 *			idem mais renvoit un pointeur vers la string formée plutot que l'écrire dans buf
 *      num2nickinfo(const char *num)
 *          renvoit la struct de la num spécifiée (accès direct désormais avec la table des nums)
 *      csreply(aNick *nick, register const char *format, ...)
 *          envoyer une notice ou private avec le CS
 *    Pour plus d'outils, voir outils.c.
 */

void sig_die(int c)
{
	Debug(W_WARN|W_MAX, "SIGTERM reçu! -- abandon..");
	putserv("%s " TOKEN_QUIT " :%s", cs.num, cf_quit_msg);
        putserv("%s " TOKEN_SQUIT " %s 0 :%s", bot.servnum, bot.server, cf_quit_msg);
#ifdef USEBSD
	usleep(500000);
#endif
	running = 0;
}

void sig_restart(int c) 
{ 
        Debug(W_WARN|W_MAX, "SIGINT reçu! -- Restarting.."); 
 
        putserv("%s "TOKEN_QUIT" :Restarting", cs.num); 
        putserv("%s "TOKEN_SQUIT" %s 0 :SIGINT/RESTART", bot.servnum, bot.server);

#ifdef USEBSD
	usleep(500000);
#endif
        ConfFlag |= CF_RESTART;
	running = 0;
} 

void sig_reload (int c)
{
	Debug(W_WARN, "Signal HUP reçu!");
	signal(SIGHUP, &sig_reload);
}

int main(int argc, char **argv)
{
	int silent = 0, tmp = 0, background = 1;
	FILE *fd; /* pid & uptime */
    struct rlimit rlim; /* used for core size */

	CurrentTS = time(NULL);

	while((tmp = getopt(argc, argv, "hvn")) != EOF)
		switch(tmp)
		{
			case 'n':
				background = 0;
				break;
			case 'h':
				silent = 1;
				break;
			case 'v': 
                                printf("Services SDreams " SPVERSION); 
                                printf(" (Build " __DATE__ " "__TIME__ ") "); 
                                printf("© IrcDreams.org\n"); 
                                exit(0); 
                                break; 
			default:
				printf("Syntaxe: %s [-hvn]\n", argv[0]);
				exit(EXIT_FAILURE);
		}

	chdir(BINDIR); /* on se place dans le répertoire principal de SDreams */

	if((fd = fopen(SDREAMS_PID, "r")))
	{
		if(fscanf(fd, "%d", &tmp) == 1)
		{
			fprintf(stderr, "SDreams est déjà lancé sur le pid %d.\n"
			"Si ce n'est pas le cas, supprimez le fichier '"SDREAMS_PID"' et recommencez.\n", tmp);
		}
		fclose(fd);
		exit(EXIT_FAILURE);
	}

	if(load_config(FICHIER_CONF) == -1)
	{
		fputs("Erreur lors de la lecture de la configuration\n", stderr);
		exit(EXIT_FAILURE);
	}

	for(tmp = 0;tmp < LANGMSGNB;++tmp)
                if(!DefaultLang->msg[tmp])
                {
                        Debug(W_TTY, "lang: langage par défaut incomplet (msgid %d manquante)", tmp);
                        exit(EXIT_FAILURE);
                }

	if((fd = fopen("uptime.tmp", "r")))
	{
		fscanf(fd, "%lu", (unsigned long *) &bot.uptime);
		fclose(fd);
		remove("uptime.tmp");
		silent = 1;
	}
	else bot.uptime = CurrentTS;
	
	RegisterCmd("DIE", 		7, CMD_ADMIN, 0, die);
	RegisterCmd("REHASH",		7, CMD_ADMIN, 0, rehash_conf);
	RegisterCmd("RESTART", 		7, CMD_ADMIN, 0, restart_bot);
	RegisterCmd("LEVEL",	 	6, CMD_ADMIN, 2, admin_level);
	RegisterCmd("CHCOMNAME", 	6, CMD_ADMIN, 2, chcomname);
	RegisterCmd("CHLEVEL", 		6, CMD_ADMIN, 2, chlevel);
	RegisterCmd("DISABLE", 		6, CMD_ADMIN, 1, disable_cmd);
	RegisterCmd("INVITEME",  	3, CMD_ADMIN, 0, inviteme);
	RegisterCmd("WRITE", 		6, CMD_ADMIN, 0, write_files);
	RegisterCmd("CHAN", 		2, CMD_HELPER, 1, admin_chan);
	RegisterCmd("USER", 		2, CMD_HELPER|CMD_SECURE3, 2, admin_user);
	RegisterCmd("WHOIS", 		3, CMD_NEEDNOAUTH|CMD_ADMIN, 1, cs_whois);
	RegisterCmd("SHOWCONFIG", 	7, CMD_NEEDNOAUTH|CMD_ADMIN, 0, showconfig);
	RegisterCmd("DNRCHAN",          4, CMD_ADMIN, 1, dnrchan_manage);
	RegisterCmd("DNRUSER",          4, CMD_ADMIN, 1, dnruser_manage);
	RegisterCmd("WHOISON",          2, CMD_CHAN|CMD_HELPER|CMD_MBRSHIP, 0, whoison);
	RegisterCmd("SAY",              4, CMD_CHAN|CMD_ADMIN|CMD_MBRSHIP, 2, admin_say);
	RegisterCmd("DO", 		4, CMD_CHAN|CMD_ADMIN|CMD_MBRSHIP, 2, admin_do);
	RegisterCmd("REGISTER", 	0, CMD_NEEDNOAUTH|CMD_SECURE, 3, register_user);
	RegisterCmd("LOGIN", 		0, CMD_NEEDNOAUTH|CMD_SECURE, 2, ns_login);
	RegisterCmd("MYACCESS", 	1, 0, 0, myaccess);
	RegisterCmd("MYINFO",         1, 0, 0, myinfo);
	RegisterCmd("DROP", 		1, CMD_SECURE, 1, drop_user);
	RegisterCmd("DEAUTH", 		1, CMD_DISABLE, 0, deauth);
	RegisterCmd("OUBLI", 		0, CMD_NEEDNOAUTH, 2, oubli_pass);
	RegisterCmd("SET", 		1, CMD_SECURE3, 1, user_set);
	RegisterCmd("AIDE",             0, CMD_NEEDNOAUTH, 0, aide);
	RegisterCmd("SHOWCOMMANDS",	0, CMD_NEEDNOAUTH, 0, showcommands);
	RegisterCmd("ADMIN", 		0, CMD_NEEDNOAUTH, 0, show_admins);
	RegisterCmd("UPTIME", 		0, CMD_NEEDNOAUTH, 0, uptime);
	RegisterCmd("SEEN", 		1, CMD_NEEDNOAUTH, 1, lastseen);
	RegisterCmd("VERIFY", 		2, CMD_HELPER, 1, verify);
	RegisterCmd("IGNORELIST", 	4, CMD_ADMIN, 0, show_ignores);
	RegisterCmd("\1PING\1",		0, CMD_NEEDNOAUTH, 0, ctcp_ping);
	RegisterCmd("\1VERSION\1", 	0, CMD_NEEDNOAUTH, 0, ctcp_version);
	RegisterCmd("VERSION",      	0, CMD_NEEDNOAUTH, 0, version);
	RegisterCmd("REGCHAN", 		1, 0, 2, register_chan);
	RegisterCmd("UNREG", 		OWNERLEVEL, CMD_CHAN, 1, unreg_chan);
	RegisterCmd("RENCHAN", 		OWNERLEVEL, CMD_CHAN, 2, ren_chan);
	RegisterCmd("ACCESS", 		0, CMD_NEEDNOAUTH|CMD_CHAN, 2, show_access);
	RegisterCmd("BANLIST", 		0, CMD_NEEDNOAUTH|CMD_CHAN, 1, banlist);
	RegisterCmd("CHANINFO", 	0, CMD_NEEDNOAUTH|CMD_CHAN, 1, chaninfo);
	RegisterCmd("ALIST", 		100, CMD_NEEDNOAUTH|CMD_CHAN, 1, see_alist);
	RegisterCmd("INFO", 		100, CMD_CHAN, 0, info);
	RegisterCmd("INVITE", 		100, CMD_CHAN|CMD_MBRSHIP, 0, invite);
	RegisterCmd("ADDUSER", 		450, CMD_CHAN, 3, add_user);
	RegisterCmd("DELUSER", 		450, CMD_CHAN, 2, del_user);
	RegisterCmd("BAN", 		100, CMD_CHAN|CMD_MBRSHIP, 2, ban_cmd);
	RegisterCmd("UNBAN", 		300, CMD_CHAN|CMD_MBRSHIP, 2, unban);
	RegisterCmd("KICKBAN", 		300, CMD_CHAN|CMD_MBRSHIP, 2, kickban);
	RegisterCmd("CLEARBANS", 	300, CMD_CHAN, 0, clear_bans);
	RegisterCmd("UNBANME", 		1, CMD_CHAN|CMD_MBRSHIP, 1, unbanme);
	RegisterCmd("KICK", 		100, CMD_CHAN|CMD_MBRSHIP, 2, kick);
	RegisterCmd("MODE", 		100, CMD_CHAN|CMD_MBRSHIP, 2, mode);
	RegisterCmd("TOPIC", 		100, CMD_CHAN|CMD_MBRSHIP, 2, topic);
	RegisterCmd("OPALL", 		400, CMD_CHAN|CMD_MBRSHIP, 1, opall);
	RegisterCmd("DEOPALL", 		400, CMD_CHAN|CMD_MBRSHIP, 1, deopall);
	RegisterCmd("CLEARMODES", 	400, CMD_CHAN|CMD_MBRSHIP, 0, clearmodes);
	RegisterCmd("VOICEALL", 	300, CMD_CHAN|CMD_MBRSHIP, 1, voiceall);
	RegisterCmd("DEVOICEALL", 	300, CMD_CHAN|CMD_MBRSHIP, 1, devoiceall);
	RegisterCmd("OP",		100, CMD_CHAN|CMD_MBRSHIP, 1, op);
	RegisterCmd("DEOP", 		100, CMD_CHAN|CMD_MBRSHIP, 1, deop);
	RegisterCmd("DEVOICE", 		50, CMD_CHAN|CMD_MBRSHIP, 1, devoice);
	RegisterCmd("VOICE", 		50, CMD_CHAN|CMD_MBRSHIP, 1, voice);
	RegisterCmd("RDEFMODES", 	300, CMD_CHAN|CMD_MBRSHIP, 1, rdefmodes);
	RegisterCmd("RDEFTOPIC", 	300, CMD_CHAN|CMD_MBRSHIP, 1, rdeftopic);
	RegisterCmd("MODUSER", 		400, CMD_CHAN, 3, generic_moduser);
	RegisterCmd("AUTOOP", 		400, CMD_CHAN, 1, moduser_autoop);
	RegisterCmd("AUTOVOICE", 	400, CMD_CHAN, 1, moduser_autovoice);
	RegisterCmd("PROTECT", 		400, CMD_CHAN, 1, moduser_protect);
	RegisterCmd("LOCKTOPIC", 	400, CMD_CHAN, 0, locktopic);
	RegisterCmd("STRICTOP", 	450, CMD_CHAN, 1, strictop);
	RegisterCmd("NOBANS", 		450, CMD_CHAN, 0, nobans);
	RegisterCmd("NOOPS", 		450, CMD_CHAN, 0, noops);
	RegisterCmd("DEFTOPIC", 	400, CMD_CHAN, 1, deftopic);
	RegisterCmd("DEFMODES", 	400, CMD_CHAN, 1, defmodes);
	RegisterCmd("THEME",	 	450, CMD_CHAN, 1, theme);
	RegisterCmd("WELCOME", 		450, CMD_CHAN, 1, csetwelcome);
	RegisterCmd("SETWELCOME", 	450, CMD_CHAN, 1, activwelcome);
	RegisterCmd("BANLEVEL", 	400, CMD_CHAN, 1, banlevel);
	RegisterCmd("BANTYPE", 		400, CMD_CHAN, 1, bantype);
	RegisterCmd("CHANURL", 		450, CMD_CHAN, 1, chanurl);
	RegisterCmd("MOTD", 		450, CMD_CHAN, 1, define_motd);
	RegisterCmd("CHANOPT", 		450, CMD_CHAN, 2, generic_chanopt);
	RegisterCmd("SETHOST",          2, CMD_HELPER, 2, sethost);
	RegisterCmd("SWHOIS",		2, CMD_HELPER, 2, swhois);
	RegisterCmd("STATS",        	3, CMD_ADMIN, 0, stats);
	RegisterCmd("HELPEUR",          0, CMD_NEEDNOAUTH, 0, show_helper);
	RegisterCmd("COUNTRY",		1, CMD_NEEDNOAUTH, 1, show_country);

	RegisterCmd("AUTOHOP",         400, CMD_CHAN, 1, moduser_autohalfop);
	RegisterCmd("HOP",              75, CMD_CHAN|CMD_MBRSHIP, 1, halfop);
        RegisterCmd("DEHOP",            75, CMD_CHAN|CMD_MBRSHIP, 1, dehalfop);
	RegisterCmd("HOPALL",           350, CMD_CHAN|CMD_MBRSHIP, 1, halfopall);
        RegisterCmd("DEHOPALL",         350, CMD_CHAN|CMD_MBRSHIP, 1, dehalfopall);
	RegisterCmd("GLOBAL",           5, CMD_ADMIN, 2, globals_cmds);

	if(GetConf(CF_NICKSERV)) RegisterCmd("RECOVER",          0, CMD_NEEDNOAUTH|CMD_SECURE, 2, recover);
	if(GetConf(CF_MEMOSERV)) {
		RegisterCmd("MEMO",            1, CMD_SECURE3, 1, memos);
		RegisterCmd("CHANMEMO",        450, CMD_CHAN|CMD_SECURE3, 1, chanmemo); 
	}
	if(GetConf(CF_WELCOMESERV)) {
		RegisterCmd("GLOBWELCOME",      5, CMD_ADMIN, 1, global_welcome);
		RegisterCmd("ADMINMOTD",        5, CMD_ADMIN, 1, set_motds);
	}
	if(GetConf(CF_VOTESERV)) {
		RegisterCmd("VOTE",             5, CMD_ADMIN, 1, do_vote);
	        RegisterCmd("VOTER",            1, CMD_SECURE2, 0, voter);
	        RegisterCmd("RESULTS",          1, 0, 0, vote_results);
	}
	if(GetConf(CF_TRACKSERV)) RegisterCmd("TRACK",            3, CMD_ADMIN, 1, cmd_track);

	if(!silent) puts("Service SDreams " SPVERSION " © 2004-2006 IrcDreams.org");

	tmp = load_country();
	if (!tmp) exit(EXIT_FAILURE);
        if(!silent) printf("Chargement des codes ISO des Pays... OK (%d)\n", tmp);
        tmp = tmpl_load();
        if(!tmp) exit(EXIT_FAILURE);
        if(!silent) printf("Chargement des Templates mail... OK\n");
	tmp = db_load_chans();
	help_load(NULL);

	if(!silent) printf("Base de donnée Salon chargée (%d)\n", tmp);
	tmp = db_load_users();
	if(!silent) printf("Base de donnée User chargée (%d)\n", tmp);
	if(tmp)
        { 
                tmp = load_cmds(); 
                if(!silent) printf("Chargement des commandes IRC... OK (%d)\n", tmp); 
		tmp = load_dnr();
		if(!silent) printf("Chargement des DNR mask... OK (%d)\n", tmp);
		if(GetConf(CF_WELCOMESERV)) load_welcome();
		load_votes();
		load_maxuser();
	}
	if(!silent && GetConf(CF_WEBSERV)) printf("WebServ activé sur le port %d\n", bot.w2c_port);

	BuildCommandsTable(0);

	if(GetConf(CF_PREMIERE))
        {
                printf("Premier lancement de SDreams. Merci de votre choix.\n");
                printf("\n");
                printf("Lorsque les services seront sur votre réseau IRC,\n tapez : "
                        "/%s %s <username> <email> <email> <pass>\n", cs.nick, RealCmd("register"));
        }

	signal(SIGPIPE, SIG_IGN);
	signal(SIGHUP, &sig_reload);
	signal(SIGINT, &sig_restart);
	signal(SIGTERM, &sig_die);

	if(background && (tmp = fork()) == -1)
        {
                Debug(W_TTY, "Impossible de se lancer en tache de fond.\n");
                exit(EXIT_FAILURE);
        }
        else if(background && tmp > 1) /* fork ok */
        {
                if(!silent) printf("Lancement en tache de fond...\n");
                exit(0);
        }

	if(isatty(0)) close(0); /* closing stdin and stderr to free up some fds */ 
        if(isatty(2)) close(2); /* therefore avoiding to waste mem in web2cs */ 

	if(!getrlimit(RLIMIT_CORE, &rlim) && rlim.rlim_cur != RLIM_INFINITY)
	{
		rlim.rlim_cur = RLIM_INFINITY;
		rlim.rlim_max = RLIM_INFINITY;
		setrlimit(RLIMIT_CORE, &rlim);
	}

	if((fd = fopen(SDREAMS_PID, "w")))
	{
		fprintf(fd, "%d\n", (int) getpid());
		fclose(fd);
	}
	else Debug(W_TTY, "Impossible d'écrire le fichier PID. [%s]", strerror(errno));

	timer_add(60, TIMER_PERIODIC, callback_check_accounts, NULL, NULL); 
        timer_add(60, TIMER_PERIODIC, callback_check_chans, NULL, NULL);
        timer_add(60, TIMER_PERIODIC, callback_write_dbs, NULL, NULL); 

	FD_ZERO(&global_fd_set);

	if(GetConf(CF_WEBSERV)) w2c_initsocket();

	init_bot(); /* 1st main socket initialization */
	run_bot(); /* main loop */

	if(!GetConf(CF_PREMIERE)) 
        { 
                db_write_users(); 
                db_write_chans(); 
		write_cmds();
	}
	putlog(LOG_PARSES, "Fermeture normale du programme");
	remove(SDREAMS_PID);
	CleanUp();
	if(GetConf(CF_RESTART)) execlp(argv[0], argv[0], "-h", NULL); /* restarting.. */
	return 0;
}
