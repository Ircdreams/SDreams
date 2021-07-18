/* src/mkpasswd.c - Cryptage des mots de pass
 * Copyright (C) 2004 ircdreams.org
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
 * $Id: mkpasswd.c,v 1.13 2005/10/22 23:07:56 bugs Exp $
 */

#include "main.h"
#include "crypt.h"
#include "checksum.h"
#include <netinet/in.h>

char *MD5pass(const char *pass, char *to)
{
	static char hash[PWDLEN+1];
	UINT4 dig[4];
	MD5_CTX context;
	char key[2] = { *pass, pass[1] }, *p = to ? to : hash;

	MD5Init(&context);
	MD5Update(&context, (unsigned char *) key, sizeof key);
	MD5Update(&context, (unsigned char *) pass, strlen(pass));
	MD5Final(dig, &context);

	snprintf(p, sizeof hash, "%x%x", ntohl(dig[0]) + ntohl(dig[2]), ntohl(dig[1]) + ntohl(dig[3]));
	return p;
}

int checkpass(const char *pass, anUser *user)
{
	char *p = MD5pass(pass, NULL);

	char *ptr = UMD5(user) ? p : cryptpass(pass);
	if(!strcmp(ptr, user->passwd))
	{
		if(!UMD5(user)) /* password is ok, just store the new md5 hash */
		{
			strcpy(user->passwd, p);
			SetUMD5(user); /* mark it as md5 */
		}
		return 1;
	}

	if(UOubli(user) && !strcmp(create_password(user->passwd), pass)) /* give him another try... */
	{
		strcpy(user->passwd, p); /* save new pass as current */
		SetUMD5(user); /* mark it as md5 */
		return 1;
	}
	return 0; /* default: failed! */
}

char *create_password(const char *seed) 
{ 
        static unsigned char pwd[33]; 
        static const char base[] = 
                "Az0By1Cx2Dw3Ev4FuGt6HsIr8Jq9KpLoMnNm/OlPkQjRi+ShTg[Uf5VeWd7XcYbZa]"; 
        MD5_CTX context; 
        unsigned char *p = pwd; 
    
        MD5Init(&context); 
        MD5Update(&context, (unsigned char *) seed, strlen(seed)); 
        MD5Final((UINT4 *) pwd, &context); 
        /* finally translate the obfuscated MD5 data into our base */
	for(;p < &pwd[10];++p) *p = base[*p % (sizeof base -1)];
        pwd[10] = 0; /* cut the password. */ 
        return (char *) pwd; 
} 

