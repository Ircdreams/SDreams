/* src/mkpasswd.c - Cryptage des mots de pass
 *
 * Copyright (C) 2002-2006 David Cortier  <Cesar@ircube.org>
 *                         Romain Bignon  <Progs@ir3.org>
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
 * $Id: mkpasswd.c,v 1.17 2006/05/12 23:20:56 romexzf Exp $
 */

#include "main.h"
#include "crypt.h"
#include "checksum.h"
#include "debug.h"
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

	mysnprintf(p, sizeof hash, "%x%x", ntohl(dig[0]) + ntohl(dig[2]), ntohl(dig[1]) + ntohl(dig[3]));
	return p;
}

int checkpass(const char *pass, anUser *user)
{
	char *p = MD5pass(pass, NULL);

#ifdef MD5TRANSITION
	if(!UMD5(user) && !strcmp(cryptpass(pass), user->passwd)) /* normal check success */
		return password_update(user, p, PWD_HASHED);
#endif
	if(!strcmp(p, user->passwd)) return 1;

	if(UOubli(user) && !strcmp(create_password(user->passwd), pass)) /* give him another try... */
		return password_update(user, p, PWD_HASHED); /* save new pass as current */

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
	for(; p < &pwd[10]; ++p) *p = base[*p % (sizeof base -1)];
	pwd[10] = 0; /* cut the password. */
	return (char *) pwd;
}

char *create_cookie(anUser *user)
{
	if(!user->cookie && !(user->cookie = malloc(PWDLEN + 1)))
		Debug(W_WARN|W_MAX, "create_cookie: OOM for %s", user->nick);

	else mysnprintf(user->cookie, PWDLEN + 1, "%x%x", strtoul(user->passwd, NULL, 16) + rand(),
			strtoul(user->passwd + 8, NULL, 16) + rand());

	return user->cookie;
}

int password_update(anUser *user, const char *pass, int flag)
{
	if(flag & PWD_HASHED) strcpy(user->passwd, pass); /* Already hashed */
	else MD5pass(pass, user->passwd);

#ifdef MD5TRANSITION
	SetUMD5(user);
#endif
	/* Password just changed, update cookie */
	if(user->cookie) create_cookie(user);
	return 1;
}
