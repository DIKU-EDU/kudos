/* yams -- Yet Another Machine Simulator
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

   $Id: device.c,v 1.2 2002/11/26 11:29:40 ttakanen Exp $
*/

#include "device.h"
#include "simulator.h"
#include <assert.h>
#include <string.h>

void device_set_irq(device_t *dev, uint32_t irq) {
    assert(irq <= 5);

    dev->irq = irq;
}

void device_set_vendor(device_t *dev, char *vendor) {
    if(vendor == NULL) {
	memcpy(dev->vendor_string, "xxxxxxxx", 8);
    } else {
	int l;
	l = strlen(vendor);
	if(l > 8) {
	    l=8;
	    printf("Warning: truncating vendor '%s' to 8 chars\n",
		   vendor);
	}

	memset(dev->vendor_string, 0, 8);
	memcpy(dev->vendor_string, vendor, l);
    }
}
