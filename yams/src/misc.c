/* YAMS -- Yet Another Machine Simulator
   Copyright (C) 2002 Juha Aatrokoski, Timo Lilja, Leena Salmela,
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

   $Id: misc.c,v 1.3 2002/11/20 12:44:32 ttakanen Exp $

   $Log: misc.c,v $
   Revision 1.3  2002/11/20 12:44:32  ttakanen
   YAMS = Yet Another Machine Simulator

   Revision 1.2  2002/11/11 15:28:32  tlilja
   Updated copyright notices

   Revision 1.1  2002/11/06 12:52:51  ttakanen
   Harmonized IO device headers


*/

#include <stdlib.h>
#include <stdio.h>

void *smalloc(int amount) {
	void *p;
  
    if((p = malloc(amount)) == NULL) {
	fprintf(stderr, 
		"Simulator run out of memory (short %d bytes)\n",
		amount);
	exit(1);
    }

    return p;
}
