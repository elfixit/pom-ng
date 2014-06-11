/*
 *  This file is part of pom-ng.
 *  Copyright (C) 2010-2014 Guy Martin <gmsoft@tuxicoman.be>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#ifndef __POM_NG_TIMER_H__
#define __POM_NG_TIMER_H__

#include <pom-ng/base.h>

#include <time.h>
#include <sys/time.h>

struct timer *timer_alloc(void* priv, int (*handler) (void*, ptime));
int timer_cleanup(struct timer *t);
int timer_queue(struct timer *t, unsigned int expiry);
int timer_queue_now(struct timer *t, unsigned int expiry, ptime now);
int timer_dequeue(struct timer *t);

struct timer_sys* timer_sys_alloc(void *priv, int (*handler) (void*));
int timer_sys_queue(struct timer_sys *t, time_t timeout);
int timer_sys_dequeue(struct timer_sys *t);
int timer_sys_cleanup(struct timer_sys *t);

#endif
