/* yams -- Yet Another Machine Simulator
   Copyright (C) 2002-2005 Juha Aatrokoski, Timo Lilja, Leena Salmela,
   Teemu Takanen, Aleksi Virtanen

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
   USA.

   $Id: async_input.h,v 1.3 2005/06/05 14:18:51 jaatroko Exp $
*/
#ifndef YAMS_ASYNC_INPUT_H
#define YAMS_ASYNC_INPUT_H

int async_input_start();
int async_input_register_fd(int fd);
int async_input_check_fd(int fd);
int async_input_verify_fd(int fd);
void async_input_lock();
void async_input_unlock();

#endif /* YAMS_ASYNC_INPUT_H */
