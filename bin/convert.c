/* bin/convert.c : convert mIRC color codes to html tag
 * Copyright (C) 2002-2004 Inter System (Cesar@Inter-System.Net)
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
 * $Id: convert.c,v 1.1.1.1 2004/04/17 01:07:45 bugs Exp $
 */

#include <stdio.h>
#include <stdlib.h>

#define HAVEDEBUG

#ifdef HAVEDEBUG
#   define DEBUGF(x) pDebug x
#else
#   define DEBUGF(x)
#endif

static inline int pDebug(const char *fmt, ...)
{
    va_list vl;

    va_start(vl, fmt);
    vfprintf(stderr, fmt, vl);
    va_end(vl);
    return 0;
}

/*
 * TODO: Find better html color, more accurate ones
 */

static char *GetHtml(const short ctrl)
{
	static const char *html[16] = { "white", "black", "navy", "green", "red",
		"maroon", "purple", "olive", "yellow", "lime", "aqua", "fuschia",
		"blue", "teal", "gray", "silver" };

	DEBUGF(("GetHtml: color #%d -> [%s]\n", ctrl, html[ctrl&15]));
	return html[ctrl&15];
}

static inline int AddToBuf(char **buf, const char *add, const int size)
{
	DEBUGF(("Adding [%s]!\n", add));
	strcpy(*buf, add);
	*buf += size;
	return 0;
}

/*
 * TODO: protect from buffer overflow
 * TODO: support or at least drop background color
 */

char *ConvertToHtml(const char *input)
{
	static char buf[2048];
	register const char *p = input;
	char *out = buf;

	struct Convert {
		unsigned int flag;
		#define CV_BOLD 	0x01
		#define CV_UNDER 	0x02
		#define CV_COLOR 	0x04
		#define CV_BG 		0x08

		#define SetBold() (convert.flag |= CV_BOLD)
		#define SetUnder() (convert.flag |= CV_UNDER)
		#define SetBG() (convert.flag |= CV_BG)
		#define SetColor() (convert.flag |= CV_COLOR)

		#define ClrBold() (convert.flag &= ~CV_BOLD)
		#define ClrUnder() (convert.flag &= ~CV_UNDER)
		#define ClrBG() (convert.flag &= ~CV_BG)
		#define ClrColor() (convert.flag &= ~CV_COLOR)

		#define IsBold() (convert.flag & CV_BOLD)
		#define IsUnder() (convert.flag & CV_UNDER)
		#define IsBG() (convert.flag & CV_BG)
		#define IsColor() (convert.flag & CV_COLOR)

		int color;
		int bg;
	} convert = {0,0,0};

	while(*p)
	{
		DEBUGF(("Trying %c(%d) [%d,%d,%d]\n", *p, *p, convert.flag, convert.color, convert.bg));

		if(*p == '\002') /* got bold */
		{
			if(!IsBold())
			{
				AddToBuf(&out, "<b>", 3);
				SetBold();
			}
			else
			{
				AddToBuf(&out, "</b>", 4);
				ClrBold();
			}
		}
		else if(*p == '\037') /* got underline */
		{
			if(!IsUnder())
			{
				AddToBuf(&out, "<u>", 3);
				SetUnder();
			}
			else
			{
				AddToBuf(&out, "</u>", 4);
				ClrUnder();
			}
		}
		else if(*p == '\003') /* got a color code */
		{
			/* if end of color area or change of color in a color area,
			 * we need to cancel old area! */
			if(IsColor()) AddToBuf(&out, "</font>", 7);

			if(!isdigit(p[1])) ClrColor(); /* next char isn't a digit, end of color !*/
			else
			{
				/* Ok, try to grap next two char to extract color number..*/
				char *endptr = NULL;
				char tmp[3] = { p[1], p[2], 0};
				short color = (short) strtol(tmp, &endptr, 10);

				if(color >= 0 && endptr != &tmp[0]) /* valid color.. */
				{
					/* convert it to html tag */
					DEBUGF(("color detected %i | %i\n", color, color & 15));
					out += sprintf(out, "<font color=%s>", GetHtml(color));
					/* jump .. (don't forget normal loop will drop one char too)*/
					p += endptr - &tmp[0];
				}
				SetColor();
			}

		}
		else if(*p == 15) /* ctrl O, cancel everything! */
		{
			if(IsBold()) AddToBuf(&out, "</b>", 4);
			if(IsUnder()) AddToBuf(&out, "</u>", 4);
			if(IsColor()) AddToBuf(&out, "</font>", 7);
			convert.flag = 0;
		}
		else *out++ = *p; /* copy standard char */

		p++;
	}

	/* finish all pending tag.. */
	if(IsBold()) AddToBuf(&out, "</b>", 4);
	if(IsUnder()) AddToBuf(&out, "</u>", 4);
	if(IsColor()) AddToBuf(&out, "</font>", 7);

	return buf;
}
