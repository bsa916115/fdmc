#ifndef _FDMC_BITOPS_INCLUDE
#define _FDMC_BITOPS_INCLUDE

#include "fdmc_exception.h"

typedef unsigned char FIELD_BIT;

/* Function set for bitmaps processing */
extern int fdmc_bit_is_set(byte *bitmap, FIELD_BIT field_number);

extern int fdmc_set_bit(byte *bitmap, FIELD_BIT field_number);

extern int fdmc_set_bits(byte *bitmap, FIELD_BIT *fieldarray);

extern int fdmc_clear_bit(byte *bitmap, FIELD_BIT field_number);

extern int fdmc_clear_bits(byte *bitmap, FIELD_BIT *fieldarray);

extern int fdmc_check_bits(byte *bitmap, FIELD_BIT *bits, FDMC_EXCEPTION *err);

#endif



