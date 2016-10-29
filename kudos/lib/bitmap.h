/*
 * Bitmaps
 */
#ifndef KUDOS_LIB_BITMAP_H
#define KUDOS_LIB_BITMAP_H

#include "lib/libc.h"

typedef uint32_t bitmap_t;

/* Bitmap functions */
int bitmap_sizeof(int num_bits);
void bitmap_init(bitmap_t *bitmap, int size);
int bitmap_get(bitmap_t *bitmap, int pos);
void bitmap_set(bitmap_t *bitmap, int pos, int value);
int bitmap_findnset(bitmap_t *bitmap, int l);

#endif // KUDOS_LIB_BITMAP_H
