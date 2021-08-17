/* src/main.c - Fichier principal
 *
 * Copyright (C) 2002-2007 David Cortier  <Cesar@ircube.org>
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
 */

#include "main.h"
#include <unistd.h>
#include <signal.h>
#include <sys/resource.h>
#ifdef USEBSD
#include <netinet/in.h>
#endif
#include <arpa/inet.h>
#include <sys/stat.h>
#include "config.h"
#include "mylog.h"
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
#include "dnr.h"
#include "flood.h"
#include "template.h"
#include "version.h"
#ifdef HAVE_TRACK
#include "track.h"
#endif
#ifdef HAVE_VOTE
#include "vote.h"
#endif
#ifdef USE_MEMOSERV
#include "memoserv.h"
#endif
#ifdef USE_WELCOMESERV
#include "welcome.h"
#endif
#ifdef SQLLOG
#include "sql_log.h"
#endif

int running = 1;
time_t CurrentTS = 0;

#ifdef USE_NICKSERV
aKill *killhead = NULL;
#endif
Lang *DefaultLang = NULL;

struct robot bot = {{0}};
#ifdef USE_WELCOMESERV
struct bots ws = {{0}};
char user_motd[400] = {0};
char admin_motd[400] = {0};
#endif
struct bots cs = {{0}};

aChan *chan_tab[CHANHASHSIZE] = {0}; 		/* hash chan */
anUser *user_tab[USERHASHSIZE] = {0}; 		/* hash username */
aNChan *nchan_tab[NCHANHASHSIZE] = {0}; 	/* hash nchan */
aNick **num_tab[MAXNUM] = {0}; 				/* table num -> struct nick */
aServer *serv_tab[MAXNUM] = {0}; 			/* table numserv -> serv info */
aNick *nick_tab[NICKHASHSIZE] = {0}; 		/* hash nick */

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
 *          renvoie la struct de la num spécifiée (accès direct désormais avec la table des nums)
 *      csntc(aNick *nick, register const char *format, ...)
 *          envoyer une notice avec le CS
 *    Pour plus d'outils, voir outils.c.
 */

static void sig_die(int c)
{
	log_write(LOG_MAIN, LOG_DOWALLOPS, "SIGTERM reçu! -- abandon..");
	running = 0;
}

static void sig_restart(int c)
{
	log_write(LOG_MAIN, LOG_DOWALLOPS, "SIGINT reçu! -- Restarting..");
	running = 0;
	ConfFlag |= CF_RESTART;

	putserv("%s "TOKEN_QUIT" :Restarting", cs.num);
	putserv("%s "TOKEN_SQUIT" %s 0 :SIGINT/RESTART", bot.servnum, bot.server);
}

static void sig_reload(int c)
{
	log_write(LOG_MAIN, LOG_DOWALLOPS, "Signal HUP reçu!");
	signal(SIGHUP, &sig_reload);
}

static void pid_write(void)
{
	FILE *fd = fopen(SDREAMS_PID, "w");

	if(fd)
	{
		fprintf(fd, "%d\n", (int) getpid());
		fclose(fd);
	}
	else log_write(LOG_MAIN, LOG_DOTTY, "Impossible d'écrire le fichier PID. [%s]",
			strerror(errno));
}

int main(int argc, char **argv)
{
	int silent = 0, tmp = 0, background = 1;
	FILE *fd; /* uptime */
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
				puts("Services SDreams " SPVERSION " (Rev:" REVDATE")\n"
					" (Build " __DATE__ " "__TIME__ ")\n"
					"© 2021 @bugsounet");
				return 0;
			default:
				printf("Syntax: %s [-hvn]\n", argv[0]);
				exit(EXIT_FAILURE);
		}

	chdir(BINDIR); 		/* going in our main base directory */
	umask(077); 		/* to prevent our files from being accessible by other */
	srand(CurrentTS); 	/* randomize the seed */

	/* Hack to check if another instance is currently running (FIX? lock) */
	if((fd = fopen(SDREAMS_PID, "r")))
	{
		if(fscanf(fd, "%d", &tmp) == 1)
		{
			fprintf(stderr, "SDreams est déjà lancé sur le pid %d.\nSi ce n'est pas "
				"le cas, supprimez le fichier '"SDREAMS_PID"' et recommencez.\n", tmp);
		}
		fclose(fd);
		exit(EXIT_FAILURE);
	}

	if(load_config(FICHIER_CONF) == -1)
	{
		fputs("Erreur lors de la lecture de la configuration\n", stderr);
		exit(EXIT_FAILURE);
	}

	/* Is default language complete ? */
	if(!lang_check_default()) exit(EXIT_FAILURE);

	if((fd = fopen("uptime.tmp", "r")))
	{
		fscanf(fd, "%lu", (unsigned long *) &bot.uptime);
		fclose(fd);
		remove("uptime.tmp");
		silent = 1;
	}
	else bot.uptime = CurrentTS;

	RegisterCmd("die", 			4, CMD_ADMIN, 0, die);
	RegisterCmd("rehash",		4, CMD_ADMIN, 0, rehash_conf);
	RegisterCmd("restart", 		4, CMD_ADMIN, 0, restart_bot);
	RegisterCmd("adminlvl", 	4, CMD_ADMIN, 2, admin_level);
	RegisterCmd("chcomname", 	4, CMD_ADMIN, 2, chcomname);
	RegisterCmd("chlevel", 		4, CMD_ADMIN, 2, chlevel);
	RegisterCmd("disable", 		4, CMD_ADMIN, 2, disable_cmd);
	RegisterCmd("inviteme",  	2, CMD_ADMIN, 0, inviteme);
	RegisterCmd("write", 		2, CMD_ADMIN, 0, write_files);
	RegisterCmd("global", 		3, CMD_ADMIN, 2, globals_cmds);
	RegisterCmd("chan", 		2, CMD_ADMIN, 1, admin_chan);
#ifdef HAVE_TRACK
	RegisterCmd("track", 		2, CMD_ADMIN, 1, cmd_track);
#endif
	RegisterCmd("user", 		2, CMD_ADMIN | CMD_SECURE3, 2, admin_user);
	RegisterCmd("whois", 		2, CMD_NEEDNOAUTH|CMD_ADMIN, 1, cs_whois);
#ifdef HAVE_VOTE
	RegisterCmd("vote", 		3, CMD_ADMIN, 1, do_vote);
#endif
	RegisterCmd("showconfig", 	4, CMD_NEEDNOAUTH|CMD_ADMIN, 0, showconfig);
#ifdef USE_WELCOMESERV
	RegisterCmd("globwelcome", 	4, CMD_ADMIN, 1, global_welcome);
	RegisterCmd("adminmotd", 	4, CMD_ADMIN, 1, set_motds);
#endif
	RegisterCmd("dnrchan", 		3, CMD_ADMIN, 1, dnrchan_manage);
	RegisterCmd("dnruser", 		3, CMD_ADMIN, 1, dnruser_manage);
	RegisterCmd("whoison", 		2, CMD_ADMIN|CMD_CHAN|CMD_MBRSHIP, 0, whoison);
	RegisterCmd("say", 			2, CMD_ADMIN|CMD_CHAN|CMD_MBRSHIP, 2, admin_say);
	RegisterCmd("do", 			2, CMD_ADMIN|CMD_CHAN|CMD_MBRSHIP, 2, admin_do);

	RegisterCmd("register", 	0, CMD_NEEDNOAUTH | CMD_SECURE, 3, register_user);
	RegisterCmd("login", 		0, CMD_NEEDNOAUTH | CMD_SECURE, 2, ns_login);
#ifdef USE_NICKSERV
	RegisterCmd("recover", 		0, CMD_NEEDNOAUTH | CMD_SECURE, 1, recover);
#endif
	RegisterCmd("myaccess", 	1, 0, 0, myaccess);
	RegisterCmd("drop", 		1, CMD_SECURE, 1, drop_user);
	RegisterCmd("deauth", 		1, CMD_DISABLE, 0, deauth);
	RegisterCmd("oubli", 		0, CMD_NEEDNOAUTH, 2, oubli_pass);
	RegisterCmd("sendpass", 	0, CMD_NEEDNOAUTH, 2, oubli_pass);
	RegisterCmd("set", 			1, CMD_SECURE3, 1, user_set);
#ifdef USE_MEMOSERV
	RegisterCmd("memos", 		1, CMD_SECURE3, 1, memos);
#endif
#ifdef HAVE_VOTE
	RegisterCmd("voter", 		1, CMD_SECURE2, 0, voter);
	RegisterCmd("results", 		1, 0, 0, vote_results);
#endif
	RegisterCmd("help", 		0, CMD_NEEDNOAUTH, 0, aide);
	RegisterCmd("aide", 		0, CMD_NEEDNOAUTH, 0, aide);
	RegisterCmd("showcommands", 0, CMD_NEEDNOAUTH, 0, showcommands);
	RegisterCmd("admin", 		0, CMD_NEEDNOAUTH, 0, show_admins);
	RegisterCmd("uptime", 		0, CMD_NEEDNOAUTH, 0, uptime);
	RegisterCmd("seen", 		1, CMD_NEEDNOAUTH, 1, lastseen);

	RegisterCmd("verify", 		0, CMD_NEEDNOAUTH, 1, verify);
	RegisterCmd("ignorelist", 	1, CMD_NEEDNOAUTH, 0, show_ignores);
	RegisterCmd("\1ping", 		0, CMD_NEEDNOAUTH, 0, ctcp_ping);
	RegisterCmd("\1version\1", 	0, CMD_NEEDNOAUTH, 0, ctcp_version);

	RegisterCmd("regchan", 		1, 0, 2, register_chan);
	RegisterCmd("unreg", 		OWNERLEVEL, CMD_CHAN, 1, unreg_chan);
	RegisterCmd("renchan", 		OWNERLEVEL, CMD_CHAN, 2, ren_chan);
	RegisterCmd("access", 		0, CMD_NEEDNOAUTH|CMD_CHAN, 2, show_access);
	RegisterCmd("banlist", 		0, CMD_NEEDNOAUTH|CMD_CHAN, 0, banlist);
	RegisterCmd("chaninfo", 	0, CMD_NEEDNOAUTH|CMD_CHAN, 1, chaninfo);
	RegisterCmd("alist", 		100, CMD_NEEDNOAUTH|CMD_CHAN, 1, see_alist);
	RegisterCmd("info", 		100, CMD_CHAN, 0, info);
	RegisterCmd("invite", 		100, CMD_CHAN|CMD_MBRSHIP, 0, invite);
	RegisterCmd("adduser", 		450, CMD_CHAN, 3, add_user);
	RegisterCmd("deluser", 		450, CMD_CHAN, 2, del_user);
	RegisterCmd("ban", 			100, CMD_CHAN|CMD_MBRSHIP, 2, ban_cmd);
	RegisterCmd("unban", 		300, CMD_CHAN|CMD_MBRSHIP, 2, unban);
	RegisterCmd("kickban", 		300, CMD_CHAN|CMD_MBRSHIP, 2, kickban);
	RegisterCmd("clearbans", 	300, CMD_CHAN, 0, clear_bans);
	RegisterCmd("unbanme", 		1, CMD_CHAN|CMD_MBRSHIP, 1, unbanme);
	RegisterCmd("kick", 		100, CMD_CHAN|CMD_MBRSHIP, 2, kick);
	RegisterCmd("mode", 		100, CMD_CHAN|CMD_MBRSHIP, 2, mode);
	RegisterCmd("topic", 		100, CMD_CHAN|CMD_MBRSHIP, 2, topic);
	RegisterCmd("opall", 		400, CMD_CHAN|CMD_MBRSHIP, 1, opall);
	RegisterCmd("deopall", 		400, CMD_CHAN|CMD_MBRSHIP, 1, deopall);
	RegisterCmd("clearmodes", 	400, CMD_CHAN|CMD_MBRSHIP, 0, clearmodes);
	RegisterCmd("voiceall", 	300, CMD_CHAN|CMD_MBRSHIP, 1, voiceall);
	RegisterCmd("devoiceall", 	300, CMD_CHAN|CMD_MBRSHIP, 1, devoiceall);
	RegisterCmd("op", 			100, CMD_CHAN|CMD_MBRSHIP, 1, op);
	RegisterCmd("deop", 		100, CMD_CHAN|CMD_MBRSHIP, 1, deop);
	RegisterCmd("devoice", 		50, CMD_CHAN|CMD_MBRSHIP, 1, devoice);
	RegisterCmd("voice", 		50, CMD_CHAN|CMD_MBRSHIP, 1, voice);
	RegisterCmd("rdefmodes", 	300, CMD_CHAN|CMD_MBRSHIP, 1, rdefmodes);
	RegisterCmd("rdeftopic", 	300, CMD_CHAN|CMD_MBRSHIP, 1, rdeftopic);
	RegisterCmd("moduser", 		400, CMD_CHAN, 3, generic_moduser);
	RegisterCmd("autoop", 		400, CMD_CHAN, 1, moduser_autoop);
	RegisterCmd("autovoice", 	400, CMD_CHAN, 1, moduser_autovoice);
	RegisterCmd("protect", 		400, CMD_CHAN, 1, moduser_protect);
	RegisterCmd("locktopic", 	400, CMD_CHAN, 0, locktopic);
	RegisterCmd("strictop", 	450, CMD_CHAN, 1, strictop);
	RegisterCmd("nobans", 		450, CMD_CHAN, 0, nobans);
	RegisterCmd("noops", 		450, CMD_CHAN, 0, noops);
	RegisterCmd("deftopic", 	400, CMD_CHAN, 1, deftopic);
	RegisterCmd("defmodes", 	400, CMD_CHAN, 1, defmodes);
	RegisterCmd("description",	450, CMD_CHAN, 1, description);
	RegisterCmd("welcome", 		450, CMD_CHAN, 1, csetwelcome);
	RegisterCmd("setwelcome", 	450, CMD_CHAN, 1, activwelcome);
	RegisterCmd("banlevel", 	400, CMD_CHAN, 1, banlevel);
	RegisterCmd("bantype", 		400, CMD_CHAN, 1, bantype);
	RegisterCmd("chanurl", 		450, CMD_CHAN, 1, chanurl);
	RegisterCmd("motd", 		450, CMD_CHAN, 1, define_motd);
	RegisterCmd("chanopt", 		450, CMD_CHAN, 2, generic_chanopt);

	if(!silent) puts("Services SDreams " SPVERSION " v2 © 2021");

	db_load_chans(silent); 	/* load channels first */
	db_load_users(silent); 	/* so load_users() will manage to add accesses */

	if(!GetConf(CF_PREMIERE))
	{
		load_cmds(silent);
		load_dnr(silent);
#ifdef USE_WELCOMESERV
		load_welcome();
#endif
#ifdef HAVE_VOTE
		load_votes();
#endif
	} /* first time */

	BuildCommandsTable(0);
	tmpl_load(); 		/* load templates for mails */
	help_load(NULL); 	/* load all languages */

	signal(SIGPIPE, SIG_IGN);
	signal(SIGHUP,	&sig_reload);
	signal(SIGINT, 	&sig_restart);
	signal(SIGTERM, &sig_die);

	if(background && (tmp = fork()) == -1)
	{
		log_write(LOG_MAIN, LOG_DOTTY, "Impossible de se lancer en background. (#%d: %s)",
			errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
	else if(background && tmp > 1) /* fork ok */
	{
		if(!silent) puts("Lancement en background...");
		exit(0);
	}

	if(!getrlimit(RLIMIT_CORE, &rlim) && rlim.rlim_cur != RLIM_INFINITY)
	{
		log_write(LOG_MAIN, LOG_DOTTY, "Core size limitée à %Uk, changement en illimité.",
			rlim.rlim_cur);
		rlim.rlim_cur = RLIM_INFINITY;
		rlim.rlim_max = RLIM_INFINITY;
		setrlimit(RLIMIT_CORE, &rlim);
	}

	if(GetConf(CF_PREMIERE)) printf("Lorsque les services seront sur votre réseau IRC,\n"
		"tapez : /%s %s <username> <email> <email> <pass>\n", cs.nick, RealCmd("register"));

	if(!fd_in_use(0)) close(0); /* closing std(in|out|err) to free up some fds */
	if(!fd_in_use(2)) close(2); /* therefore avoiding to waste mem in web2cs */
	if(background && !fd_in_use(1)) close(1);

#ifdef SQLLOG
	if(!sql_init(1)) exit(EXIT_FAILURE);
#endif

	pid_write();

	timer_add(PURGEDELAY, TIMER_PERIODIC, callback_check_accounts, NULL, NULL);
	timer_add(PURGEDELAY, TIMER_PERIODIC, callback_check_chans, NULL, NULL);
	timer_add(cf_write_delay, TIMER_PERIODIC, callback_write_dbs, NULL, NULL);

	socket_init();
	init_bot(); /* 1st main socket initialization */
	run_bot(); /* main loop */

	if(!GetConf(CF_PREMIERE))
	{
		db_write_users();
		db_write_chans();
		write_cmds();
	}

	log_write(LOG_MAIN, 0, "Fermeture du programme%s",
		GetConf(CF_RESTART) ? " (Restarting)" : "");

	remove(SDREAMS_PID);
	CleanUp();

	if(GetConf(CF_RESTART)) execlp(argv[0], argv[0], "-h", NULL); /* restarting.. */
	return 0;
}
