/*
 * Bitmaps
 */

/** @name Bitmap operations
 *
 * This module implements common bitmap operations.
 *
 * @{
 */

#include "lib/bitmap.h"
#include "kernel/panic.h"
#include "kernel/assert.h"

/**
 * Calculates the memory size in bytes needed to store a given number
 * of bits in a bitmap. The size of the bitmap will be a multiple of
 * 4.
 *
 * @param num_bits The number of bits the bitmap needs to hold.
 *
 * @return The size of the bitmap in bytes.
 */
int bitmap_sizeof(int num_bits)
{
  return ((num_bits + 31) / 32) * 4;
}

/**
 * Initialize a given bitmap. All entries in the bitmap are intialized
 * to 0.
 *
 * @param bitmap The bitmap to initialize
 *
 * @param size The number of bits in the bitmap.
 */
void bitmap_init(bitmap_t *bitmap, int size)
{
  int i;

  KERNEL_ASSERT(size >= 0);

  for(i = 0; i < (size + 31) / 32; i++)
    bitmap[i] = 0;
}

/**
 * Gets the value of a given bit in the bitmap.
 *
 * @param bitmap The bitmap
 *
 * @param pos The position of the bit, whose value will be returned.
 *
 * @return The value (0 or 1) of the given bit in the bitmap.
 */
int bitmap_get(bitmap_t *bitmap, int pos)
{
  int i;
  int j;

  KERNEL_ASSERT(pos >= 0);

  i = pos / 32;
  j = pos % 32;
    
  return ((bitmap[i] >> j) & 0x01);
}

/**
 * Sets the given bit in the bitmap.
 *
 * @param bitmap The bitmap
 *
 * @param pos The index of the bit to set
 *
 * @param value The new value of the given bit. Valid values are 0 and
 * 1.
 */
void bitmap_set(bitmap_t *bitmap, int pos, int value)
{
  int i;
  int j;

  KERNEL_ASSERT(pos >= 0);

  i = pos / 32;
  j = pos % 32;

  if (value == 0) {
    bitmap[i] = bitmap[i] & ~(1 << j);
  } else if (value == 1) {
    bitmap[i] = bitmap[i] | (1 << j);
  } else {
    KERNEL_PANIC("bit value other than 0 or 1");
  }
}


/**
 * Finds first zero and sets it to one.
 * 
 * @param bitmap The bitmap
 *
 * @param l Length of bitmap in bits
 * 
 * @return Number of bit set. Negative if failed.
 */

int bitmap_findnset(bitmap_t *bitmap, int l)
{
  int i,j;

  KERNEL_ASSERT(l >= 0);

  /* Loop through words until a one with at least one free bit is found. */
  for (i = 0; i < bitmap_sizeof(l)/4; i++) {
    if (bitmap[i] != 0xffffffff) {
      /* At least one bit in this word is zero */
      int basepos=i*32; /* Bit position of the first bit in this word. */

      /* Loop through the bits in the word until the free bit is found.*/
      for (j = 0; j < 32; j++) {
        if (bitmap_get(bitmap, basepos+j) == 0) {
          if (basepos+j >= l) {
            /* we are in the last word in the bitmap, but
               it is not fully in use and the first "free"
               bit is in unused portion -> return "not found". */
            return -1;
          }

          bitmap_set(bitmap, basepos+j, 1);
          return (basepos+j);
        }
      }         
    }
  }
        
  /* No free slots found */
  return -1;
}

/** @} */
