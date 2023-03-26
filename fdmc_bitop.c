#include "fdmc_global.h"
#include "fdmc_bitop.h"
#include "fdmc_logfile.h"
#include "fdmc_exception.h"

#include <stdio.h>

MODULE_NAME("fdmc_bitops.c");

//---------------------------------------------------------
//  name: fdmc_bit_is_set
//---------------------------------------------------------
//  purpose: check if bit with specified number was set in bitmap
//  designer: Serge Borisov (BSA)
//  started: 24.03.10
//	parameters:
//		bitmap - pointer to bitmap
//		field_number - number of bit
//	return:
//		Success - true
//		Failure - false
//---------------------------------------------------------
int fdmc_bit_is_set(byte *bitmap, FIELD_BIT field_number)
{
//	FUNC_NAME("fdmc_bit_is_set");
	FIELD_BIT element_number;
	byte bit_number;
	int result;

	if(!field_number)
	{
		return 1;
	}
	if(!bitmap)
	{
		return 0;
	}
	element_number = (field_number-1) >> 3;
	bit_number =(byte)( 7 - ((byte)(field_number-1) % 8));

	result = bitmap[element_number] & (byte)(1 << bit_number) ? 1 : 0;
	return result;
}

//---------------------------------------------------------
//  name: fdmc_set_bit
//---------------------------------------------------------
//  purpose: set bit in bitmap
//  designer: Serge Borisov (BSA)
//  started: 24.03.10
//	parameters:
//		bitmap - pointer to bitmap
//		field_number - number of bit
//	return:
//		True - ok
//		False - failure
//---------------------------------------------------------
int fdmc_set_bit(byte *bitmap, FIELD_BIT field_number)
{
//	FUNC_NAME("fdmc_set_bit");
	FIELD_BIT element_number;
	byte bit_number;

	if(!bitmap || !field_number)
		return 0;

	element_number = (field_number-1) >> 3; 
	bit_number =(byte)( 7 - ((field_number-1) % 8));

	bitmap[element_number] |= (byte)(1 << bit_number);

	return 1;
}

//---------------------------------------------------------
//  name: fdmc_set_bits
//---------------------------------------------------------
//  purpose: set bit array in bitmap
//  designer: Serge Borisov (BSA)
//  started: 24.03.10
//	parameters:
//		bitmap - pointer to bitmap
//		fieldarray - zero terminated array of bit numbers
//	return:
//		True - ok
//		False - failure
//---------------------------------------------------------
int fdmc_set_bits(byte *bitmap, FIELD_BIT *fieldarray)
{
//	FUNC_NAME("fdmc_set_bits");

	if(!bitmap || !fieldarray)
		return 0;
	for(; *fieldarray; fieldarray ++) 
	{
		fdmc_set_bit(bitmap, *fieldarray);
	}
	return 1;
}

//---------------------------------------------------------
//  name: fdmc_clear_bit
//---------------------------------------------------------
//  purpose: clear bit in bitmap
//  designer: Serge Borisov (BSA)
//  started: 24.03.10
//	parameters:
//		bitmap - pointer to bitmap
//		field_number - number of bit
//	return:
//		True - ok
//		False - failure
//---------------------------------------------------------
int fdmc_clear_bit(byte *bitmap, FIELD_BIT field_number)
{
	FIELD_BIT element_number ;
	byte bit_number;

	if(!bitmap || !field_number)
		return 0;
	element_number = (field_number-1) >> 3; /* div 8 */
	bit_number =(byte)( 7 - ((field_number-1) % 8));

	bitmap[element_number] &=(byte)( ~(1 << bit_number));

	return 1;
}

//---------------------------------------------------------
//  name: fdmc_clear_bits
//---------------------------------------------------------
//  purpose: clear set of bits in bitmap
//  designer: Serge Borisov (BSA)
//  started: 24.03.10
//	parameters:
//		bitmap - pointer to bitmap
//		fieldarray - zero terminated array of bit numbers
//	return:
//		True - ok
//		False - failure
//---------------------------------------------------------
int fdmc_clear_bits(byte *bitmap, FIELD_BIT *fieldarray)
{
	if(!bitmap || !fieldarray)
		return 0;
	for(; *fieldarray; fieldarray ++) 
	{
		fdmc_clear_bit(bitmap, *fieldarray);
	}
	return 1;
}

//---------------------------------------------------------
//  name: fdmc_check_bits
//---------------------------------------------------------
//  purpose: check set of bits in bitmap
//  designer: Serge Borisov (BSA)
//  started: 24.03.10
//	parameters:
//		bitmap - pointer to bitmap
//		bits - zero terminated array of bit numbers
//		err - error handler
//	return:
//		True - ok
//		False - failure
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
int fdmc_check_bits(byte *bitmap, FIELD_BIT *bits, FDMC_EXCEPTION *err)
{
	FUNC_NAME("check_bits");
	FDMC_EXCEPTION x;
	int success = 1;

	TRYF(x)
	{
		CHECK_NULL(bitmap, "bitmap", x);
		CHECK_NULL(bits, "bits", x);
		for(; *bits; bits++)
		{
			if(!fdmc_bit_is_set(bitmap, *bits))
			{
				fdmc_raisef(FDMC_DATA_ERROR, &x, 
					"Mandatory bit %d does not present in bitmap", *bits);
			}
		}
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		success = 0;
	}
	return success;
}

