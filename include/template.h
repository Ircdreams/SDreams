/* include/template.h - Template
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
 * $Id: template.h,v 1.4 2006/03/15 06:43:23 bugs Exp $
 */

#ifndef HAVEINC_template
#define HAVEINC_template

#define TMPL_PATH "template" /* path à partir de BINDIR dans lequel se trouvent les fichiers .tmpl */

extern struct Template tmpl_mail_register, tmpl_mail_oubli, tmpl_mail_memo;

extern int tmpl_clean(void);
extern int tmpl_load(void);
extern int tmpl_mailsend(struct Template *, const char *, const char *, const char *, const char *, const char *, const char *);
#endif /*HAVEINC_template*/
