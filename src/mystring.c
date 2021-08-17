/* src/mystrings.c - Fonction manipulant les strings
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
 * $Id: mystring.c,v 1.6 2008/01/20 13:50:39 romexzf Exp $
 */

#include <ctype.h>
#include "main.h"
#include "debug.h"

void strupr(char *buf)
{
	for(; *buf; ++buf) *buf = toupper((unsigned char) *buf);
}

void strlwr(char *buf)
{
	for(; *buf; ++buf) *buf = tolower((unsigned char) *buf);
}

int count_char(const char *str, int c)
{
	int i = 0;
	while(*str) if(*str++ == c) ++i;
	return i;
}

void strip_newline(char *string)
{
   register char *p = string;
   while(*p && *p != '\n' && *p != '\r') ++p;
   *p = '\0';
}

int is_num(const char *num)
{
   while(*num) if(!isdigit((unsigned char) *num++)) return 0;
   return 1;
}

int Strtoint(const char *str, int *to, int min, int max)
{
	char *ptr = NULL;
	int i = strtol(str, &ptr, 10);

	return (!ptr || !*ptr) && (*to = i) <= max && i >= min;
}

int HasWildCard(const char *str)
{
	while(*str && *str != '*' && *str != '?') ++str;

	return *str;
}

size_t fastfmtv(char *buf, const char *fmt, va_list vl)
{
	const char *s = buf;

	while(*fmt)
	{
		if(*fmt == '$')
		{
			register char *tmp = va_arg(vl, char *);
			if(tmp) while(*tmp) *buf++ = *tmp++;
		}
		else *buf++ = *fmt;
		++fmt;
	}
	*buf = 0;
	return buf - s;
}

size_t fastfmt(char *buf, const char *fmt, ...)
{
	va_list vl;
	size_t len;

	va_start(vl, fmt);
	len = fastfmtv(buf, fmt, vl);
	va_end(vl);
	return len;
}

char *str_dup(char **to, const char *from)
{
	if(from && *from)
	{
		*to = realloc(*to, strlen(from) + 1);
		if(!*to) Debug(W_MAX, "EXIT: strdup: no memory for 'to'(%p)=%s 'from'(%p)=%s\n",
			(void *) *to, *to, (const void *) from, from);
		else strcpy(*to, from);
	}
	return *to;
}

char *Strncpy(char *to, const char *from, size_t n) /* copie from dans to. ne copie que n char */
{							/* MAIS AJOUTE LE CHAR \0 à la fin, from DOIT donc faire n+1 chars. */
	const char *end = to + n;
	char *save = to;
	while(to < end && (*to++ = *from++));
	*to = 0;
	return save;
}

char *Strtok(char **save, char *str, int sep) /* fonction tirée d'ircu, simplifiée */
{
	register char *pos = *save; 	/* keep last position across calls */
	char *tmp;

	if(str) pos = str; 	/* new string scan */

	while(pos && *pos && sep == *pos) ++pos; /* skip leading separators */

	if(!pos || !*pos) return (pos = *save = NULL); /* string contains only sep's */

	tmp = pos; /* now, keep position of the token */

	if((pos = strchr(pos, sep))) *pos++ = 0; /* get sep -Cesar*/

	*save = pos;
	return tmp;
}

int split_buf(char *buf, char **parv, int size)
{
	int parc = 0, i;

	while(*buf && parc < size)
	{
		while(*buf == ' ') *buf++ = 0; /* on supprime les espaces pour le découpage */
		if(*buf == ':') /* last param */
		{
			parv[parc++] = buf + 1;
			break;
		}
		if(!*buf) break;
		parv[parc++] = buf;
		while(*buf && *buf != ' ') ++buf; /* on laisse passer l'arg.. */
	}
	for(i = parc; i < size; ++i) parv[i] = NULL; /* NULLify following items */
	return parc;
}

void parv2msgn(int parc, char **parv, int base, char *buf, size_t size)
{
	int i = base;
	size_t toksize, written = 0;

	for(; i <= parc; ++i)
	{
		toksize = strlen(parv[i]);

		if(i > base) buf[written++] = ' ';

		if(written + toksize >= size-1)
		{
			Strncpy(buf + written, parv[i], size - written - 1);
			written = size - 1;
			break;
		}
		else
		{
			strcpy(buf + written, parv[i]);
			written += toksize;
		}
	}
	buf[written] = 0;
	return;
}

char *parv2msg(int parc, char **parv, int base, size_t size)
{
	int i = base;
	size_t toksize, written = 0;
	static char buf[512];

	if(size > sizeof buf) size = sizeof buf;

	for(; i <= parc; ++i)
	{
		toksize = strlen(parv[i]);

		if(i > base) buf[written++] = ' ';

		if(written + toksize >= size-1)
		{
			Strncpy(buf + written, parv[i], size - written - 1);
			written = size - 1;
			break;
		}
		else
		{
			strcpy(buf + written, parv[i]);
			written += toksize;
		}
	}
	buf[written] = 0;
	return buf;
}

/* Simple and fast vsnprintf:
 * it handles only basic but most used (>99% of Z *snprintf calls) specifiers:
 * %s %c %d without any other flags (width, padding, precision)
 * Nevertheless, if such a specifier is matched, original vsnprintf is called. */

size_t myvsnprintf(char *buf, size_t size, const char *format, va_list vl)
{
	register char *p = buf;
	register const char *fmt = format;
	const char *end = p + size - 1; /* -1 pour le \0 */
	char t;

	while((t = *fmt++) && p < end) /* %sa (t = %, *pattern = s) */
	{
		if(t == '%')
		{
			t = *fmt++; /* on drop le formateur (t = s, *pattern = a) */
			if(t == 's')
			{	/* copie de la string */
				register const char *tmps = va_arg(vl, char *);
				while(*tmps && p < end) *p++ = *tmps++;
				continue;
			}
			if(t == 'd')
			{
				int tmpi = va_arg(vl, int);
				char bufi[20]; /* 2^64 is 3*6+2 char long */
				unsigned int pos = sizeof bufi -1; /* index of last index from bufi */

				if(tmpi <= 0)
				{
					if(!tmpi)
					{
						*p++ = '0';
						continue;
					}
					*p++ = '-';
					tmpi = -tmpi;
				}
				while(tmpi) /* on converti une int en base 10 en string */
				{		/* écriture dans l'ordre inverse 51 > '   1' > '  51'*/
					bufi[pos--] = '0' + (tmpi % 10);
					tmpi /= 10;
				}
				while(pos < sizeof bufi -1 && p < end) *p++ = bufi[++pos];
				continue;
			}
			if(t == 'U')
			{
				unsigned long tmpi = va_arg(vl, unsigned long);
				char bufi[20]; /* 2^64 is 3*6+2 char long */
				unsigned int pos = sizeof bufi - 1; /* index of last index from bufi */

				if(!tmpi)
				{
					*p++ = '0';
					continue;
				}
				while(tmpi) /* on converti une int en base 10 en string */
				{		/* écriture dans l'ordre inverse 51 > '   1' > '  51'*/
					bufi[pos--] = '0' + (tmpi % 10);
					tmpi /= 10;
				}
				while(pos < sizeof bufi -1 && p < end) *p++ = bufi[++pos];
				continue;
			}
			if(t == 'T')
			{
				time_t time_tmp = va_arg(vl, time_t);
				char bufi[20]; /* 2^64 is 3*6+2 char long */
				unsigned int pos = sizeof bufi -1; /* index of last index from bufi */

				if(time_tmp <= 0)
				{
					if(!time_tmp)
					{
						*p++ = '0';
						continue;
					}
					*p++ = '-';
					time_tmp = -time_tmp;
				}
				while(time_tmp) /* on converti une int en base 10 en string */
				{		/* écriture dans l'ordre inverse 51 > '   1' > '  51'*/
					bufi[pos--] = '0' + (time_tmp % 10);
					time_tmp /= 10;
				}
				while(pos < sizeof bufi -1 && p < end) *p++ = bufi[++pos];
				continue;
			}
			if(t == 'x' || t == 'X')
			{
				static char dec2hex[2][17] = { "0123456789abcdef", "0123456789ABCDEF" };
				unsigned long tmpi = va_arg(vl, unsigned long);
				char bufi[10], *convert = dec2hex[(t == 'x') ? 0 : 1];
				unsigned int pos = sizeof bufi - 1; /* index of last index from bufi */

				if(!tmpi)
				{
					*p++ = '0';
					continue;
				}
				while(tmpi)
				{
					bufi[pos--] = convert[tmpi & 0xF];
					tmpi >>= 4;
				}
				while(pos < sizeof bufi -1 && p < end) *p++ = bufi[++pos];
				continue;
			}
			if(t == 'u')
			{
				unsigned int tmpi = va_arg(vl, unsigned int);
				char bufi[20]; /* 2^64 is 3*6+2 char long */
				unsigned int pos = sizeof bufi -1; /* index of last index from bufi */

				if(!tmpi)
				{
					*p++ = '0';
					continue;
				}
				while(tmpi) /* on converti une int en base 10 en string */
				{		/* écriture dans l'ordre inverse 51 > '   1' > '  51'*/
					bufi[pos--] = '0' + (tmpi % 10);
					tmpi /= 10;
				}
				while(pos < sizeof bufi -1 && p < end) *p++ = bufi[++pos];
				continue;
			}
			if(t == 'c')
			{
				*p++ = (char) va_arg(vl, int);
				continue;
			}
			if(t != '%')
			{	/* on sous traite le reste à vsnprintf (-2 because of the %*)
					on laisse vsnprintf écrire le \0 d'où le + 1 */
				size_t i = vsnprintf(p, end - p + 1, fmt - 2, vl);
				p += i < end - p ? i : end - p; /* si i >= size : overflow bloqué */
				break;
			}
		}
		*p++ = t;
	}
	*p = '\0';

	return p - end + size - 1; /* on ne retourne que la taille effectivement utilisée */
}

size_t mysnprintf(char *buf, size_t size, const char *format, ...)
{
	va_list vl;
	size_t len;

	va_start(vl, format);
	len = myvsnprintf(buf, size, format, vl);
	va_end(vl);
	return len;
}
