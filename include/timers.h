/* include/timers.h
 *
 * Copyright (C) 2002-2007 David Cortier  <Cesar@ircube.org>
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
 * $Id: timers.h,v 1.5 2007/01/02 19:44:30 romexzf Exp $
 */

#ifndef HAVEINC_timers
#define HAVEINC_timers

extern Timer *Timers;

extern Timer *timer_add(time_t, enum TimerType, int (*) (Timer *),	void *, void *);
extern Timer *timer_enqueue(Timer *);

extern void timer_free(Timer *);
extern void timer_dequeue(Timer *);
extern void timer_remove(Timer *);
extern void timer_clean(void);
extern void timers_run(void);

extern int callback_ban(Timer *);
extern int callback_kill(Timer *);
extern int callback_check_accounts(Timer *);
extern int callback_check_chans(Timer *);
extern int callback_write_dbs(Timer *);
extern int callback_fl_update(Timer *);

extern void check_chans(void);
extern void check_accounts(void);

#endif /*HAVEINC_timers*/
