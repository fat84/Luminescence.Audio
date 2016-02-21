#pragma once

#include "stdafx.h"

#include <stdlib.h> // calloc free malloc
#include <intrin.h>  // __popcnt

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#pragma unmanaged

static bool check_popcount()
{
   int cpuInfo[4];
   __cpuid(cpuInfo, 1);
   return cpuInfo[2] & (1 << 23) || false;
}

static const bool has_popcount = check_popcount();

// http://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
static inline unsigned int popcount(unsigned int i)
{
   i = i - ((i >> 1) & 0x55555555);
   i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
   return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

// https://bitbucket.org/acoustid/pg_acoustid  -->  acoustid_compare.c
float match_fingerprints3(int *a, int asize, int *b, int bsize, int maxoffset)
{
   int jbegin = 0;
   int jend = bsize;
   int numcounts = asize + bsize + 1;
   int *counts = (int*)calloc(numcounts, sizeof(int));

   if (has_popcount)
   {
      for (int i = 0; i < asize; i++)
      {
         if (maxoffset > -1)
         {
            jbegin = MAX(0, i - maxoffset);
            jend = MIN(bsize, i + maxoffset);
         }

         for (int j = jbegin; j < jend; j++)
         {
            int biterror = __popcnt(a[i] ^ b[j]);
            if (biterror < 16)
               counts[i - j + bsize] += 16 - biterror;
         }
      }
   }
   else
   {
      for (int i = 0; i < asize; i++)
      {
         if (maxoffset > -1)
         {
            jbegin = MAX(0, i - maxoffset);
            jend = MIN(bsize, i + maxoffset);
         }

         for (int j = jbegin; j < jend; j++)
         {
            int biterror = popcount(a[i] ^ b[j]);
            if (biterror < 16)
               counts[i - j + bsize] += 16 - biterror;
         }
      }
   }

   int topcount = 0;
   for (int i = 0; i < numcounts; i++)
   {
      if (counts[i] > topcount)
         topcount = counts[i];
   }

   free(counts);

   return (float)topcount / (16.0f * MIN(asize, bsize));
}

#pragma managed