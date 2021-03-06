/* include/timers.h
 * Copyright (C) 2004 ircdreams.org
 *
 * contact: bugs@ircdreams.org
 * site web: http://ircdreams.org
 *
 * Services pour serveur IRC. Support? sur IrcDreams V.2
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
 * $Id: timers.h,v 1.9 2005/08/26 23:55:25 bugs Exp $
 */

#ifndef HAVEINC_timers
#define HAVEINC_timers

Timer *timer_add(time_t, enum TimerType, int (*) (Timer *),     void *, void *);

Timer *timer_enqueue(Timer *);

void timer_free(Timer *);
void timer_dequeue(Timer *);
void timer_remove(Timer *);
void timers_run(void);

int callback_ban(Timer *);
int callback_kill(Timer *);
int callback_check_accounts(Timer *);
int callback_check_chans(Timer *);
int callback_write_dbs(Timer *);
int callback_unsuspend(Timer *);
int callback_fl_update(Timer *);
    
void check_chans(void); 
void check_accounts(void); 

#endif /*HAVEINC_timers*/
