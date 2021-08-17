/* src/debug.c
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
 * $Id: debug.c,v 1.13 2006/03/09 19:56:46 romexzf Exp $
 */

#include "main.h"
#include "debug.h"
#include "cs_cmds.h"
#include "outils.h"
#include <unistd.h>

int Debug(int flag, const char *format, ...)
{
	FILE *fp = fopen(LOG_ERREURS, "a");
	char buf[2048];
	int i = 0;
	va_list va;

	if(flag & W_PROTO) buf[i++] = 'P';
	if(flag & W_DESYNCH) buf[i++] = 'D';
	if(flag & W_MAX) buf[i++] = 'M';
	buf[i++] = ' ';

	va_start(va, format);
	myvsnprintf(buf + i, sizeof buf - i, format, va);
	va_end(va);

	if(flag & W_WARN && bot.sock >= 0) cswallops("%s", buf); /* send to opers if connect */
	if(flag & W_TTY && isatty(1)) puts(buf); /* display on console (if available) */
	if(fp)
	{
		fputc('[', fp);
		fputs(get_time(NULL, CurrentTS), fp);
		fputs("] ", fp);
		fputs(buf, fp);
		fputc('\n', fp);
		fclose(fp);
	}
	return 0;
}
