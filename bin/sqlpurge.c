/* sqlpurge.c
 * Copyright (C) 2002-2003 Inter System (Cesar@Inter-System.Net)
 *
 * contact: Progs@Inter-System.Net
 *          Cesar@Inter-System.Net
 *          kouak@kouak.org
 * site web: http://coderz.inter-system.net
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
 * $Id: sqlpurge.c,v 1.1 2004/01/02 18:57:57 romexzf Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <mysql.h>
#include <unistd.h>
#include <string.h>

#define CHANHASHSIZE 256

MYSQL *sql_id = NULL;

char *sql_ip = NULL;
char *sql_user = NULL;
char *sql_pass = NULL;
char *sql_db = NULL;
int sql_port = 3306;

typedef struct Channel {
	char chan[120];
	int entry;
	struct Channel *next;
} Channel;

Channel *Clist[CHANHASHSIZE] = {0}, *oldClist[CHANHASHSIZE] = {0};

int Clistlength = 0;
int oldClistlength = 0;
int oldentries = 0;

void strip_newline(char *string)
{
   register char *p = string;
   while(*p && *p != '\n' && *p != '\r') p++;
   *p = 0;
}

char *Strncpy(char *to, const char *from, size_t n)/* copie from dans to. ne copie que n char*/
{							/* MAIS AJOUTE LE CHAR \0 à la fin, from DOIT donc faire n+1 chars.*/
	const char *end = to + n;
	char *save = to;
	while(to < end && (*to++ = *from++));
	*to = 0;
	return save;
}

unsigned int hashc(const char *chan)
{
	unsigned int checksum = 0;
	while(*chan) checksum += (checksum << 3) + tolower(*chan++);
	return checksum & (CHANHASHSIZE-1);
}

Channel *getchan(const char *chan)
{
	unsigned int hash = hashc(chan);
	register Channel *tmp = Clist[hash];
	for(;tmp && strcasecmp(chan, tmp->chan);tmp = tmp->next);
	return tmp;
}

Channel *getoldchan(const char *chan)
{
	unsigned int hash = hashc(chan);
	register Channel *tmp = oldClist[hash];
	for(;tmp && strcasecmp(chan, tmp->chan);tmp = tmp->next);
	return tmp;
}

int sql_init(void)
{
	sql_id = mysql_init(NULL);

 	if(!mysql_real_connect(sql_id, sql_ip, sql_user, sql_pass, sql_db, sql_port, NULL, 0))
 		return 0;

	return 1;
}

void usage(void)
{
	fprintf(stderr, "Syntaxe: -u <user> -h <host> -d <database> [-f <path vers la database>] [-p <port>]\n");
	return;
}

int main(int argc, char **argv)
{
	char buf[512];
	char *path = "../database/chans.db";
	FILE *fp;
	MYSQL_RES *result = NULL;
	Channel *chan, *tmp;
	int i, del = 0;

	for(i = 1; i < argc;++i)
	{
		char *a = argv[i];
		if(!strcmp(argv[i], "-r"))
		{
			del = 1;
			continue;
		}
		if(*a != '-' || ++i >= argc) usage(), exit(EXIT_FAILURE);
		switch(*++a)
		{
			case 'u': sql_user = argv[i]; break;
			case 'p': sql_port = atoi(argv[i]); break;
			case 'd': sql_db = argv[i]; break;
			case 'h': sql_ip = argv[i]; break;
			case 'f': path = argv[i]; break;
			default: usage();
		}
	}
	if(!sql_user) fprintf(stderr, "Veuillez préciser un utilisateur MySQL\n"), usage(), exit(EXIT_FAILURE);
	if(!sql_ip) fprintf(stderr, "Veuillez préciser l'IP du serveur MySQL\n"), usage(), exit(EXIT_FAILURE);
	if(!sql_db) fprintf(stderr, "Veuillez préciser une DB MySQL\n"), usage(), exit(EXIT_FAILURE);

	if(!(sql_pass = getpass("MySQL password: "))) fprintf(stderr, "Veuillez préciser le pass de la DB MySQL\n"), usage(), exit(EXIT_FAILURE);

	/* load de la vraie DB*/
	if(!(fp = fopen(path, "r")))
	{
		printf("Erreur DB chan inaccessible au path '%s'.\n", path);
		exit(EXIT_FAILURE);
	}
	while(fgets(buf, sizeof buf, fp))
	{
		unsigned int hash;
		strip_newline(buf);
		chan = malloc(sizeof(struct Channel));
		strcpy(chan->chan, buf);
		hash = hashc(buf);

		chan->next = Clist[hash];
		Clist[hash] = chan;
		Clistlength++;
	}
	fclose(fp);

	printf("%d channels loaded from %s.\n", Clistlength, path);
	printf("Connexion à la DB [%s!%s@%s:%d]...\n", sql_user, sql_db, sql_ip, sql_port);
	if(!sql_init())
	{
		puts("Connexion échouée. EXIT");
		exit(EXIT_FAILURE);
	}
	puts("Envoi du query & examen...");

	if(mysql_query(sql_id, "SELECT chan,TS FROM chancmdslog;"))
	{
		puts("Erreur lors du query. EXIT");
		exit(EXIT_FAILURE);
	}

 	if((result = mysql_store_result(sql_id)))
 	{
 		MYSQL_ROW row;

 		while((row = mysql_fetch_row(result)))
 		{
 			mysql_field_seek(result, 0);

 			if(!getchan(row[0]))
 			{
				oldentries++;
#ifdef DEBUG
				fprintf(stderr, "Found: %s (%s)\n", row[0], row[1]);
#endif
				if(!(tmp = getoldchan(row[0])))
				{
					unsigned int hash = hashc(row[0]);
					printf("New ghosted entry: %s\n", row[0]);
					tmp = malloc(sizeof(struct Channel));
					strcpy(tmp->chan, row[0]);
					hash = hashc(row[0]);

					tmp->entry = 1;
					tmp->next = oldClist[hash];
					oldClist[hash] = tmp;
					oldClistlength++;
				}
				else tmp->entry++;
			}
 		}
 		mysql_free_result(result);
	}
	printf("%d entrées anciennes trouvées pour %d vieux salons\n", oldentries, oldClistlength);
	for(i = 0;i < CHANHASHSIZE;++i)
		for(chan = oldClist[i];chan;chan = tmp)
		{
			tmp = chan->next;
			if(del)
			{
				char buf[400];
				sprintf(buf, "DELETE FROM chancmdslog WHERE chan='%s';", chan->chan);
				mysql_query(sql_id, buf);
			}
			free(chan);
		}

	puts("Exiting...");

	for(i = 0;i < CHANHASHSIZE;++i)
		for(chan = Clist[i];chan;free(chan), chan = tmp) tmp = chan->next;

	return 0;
}
