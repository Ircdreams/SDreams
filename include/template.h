/* include/template.h - Template
 *
 * Copyright (C) 2002-2006 David Cortier  <Cesar@ircube.org>
 *                         Romain Bignon  <Progs@kouak.org>
 *                         Benjamin Beret <kouak@kouak.org>
 *
 * site web: http://sf.net/projects/scoderz/
 *
 * Services pour serveur IRC. Supporté sur IrcProgs et IRCoderz
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
 * $Id: template.h,v 1.2 2006/02/16 18:12:26 romexzf Exp $
 */

#ifndef HAVEINC_template
#define HAVEINC_template

extern struct Template tmpl_mail_register, tmpl_mail_oubli;

extern int tmpl_clean(void);
extern int tmpl_load(void);
extern int tmpl_mailsend(struct Template *, const char *, const char *,
						const char *, const char *);

#endif /*HAVEINC_template*/
