//#include "fdmc_global.h"
//#include "fdmc_thread.h"
#include "fdmc_des.h"

#include <string.h>

// This part was taken from open source software
// --------------------------------------
/* Copyright (C) 1992 Eric Young - see COPYING for more details */
/* Collected and modified by Werner Almesberger */

#define DES_AVAILABLE

/* from des.h */

typedef unsigned char des_cblock[8];
typedef struct des_ks_struct
{
	union	{
		des_cblock _;
		/* make sure things are correct size on machines with
		* 8 byte longs */
		unsigned long pad[2];
	} ks;
	/* #define _	ks._ */
} des_key_schedule[16];

#define DES_ENCRYPT	1
#define DES_DECRYPT	0

#define _	ks._

#define ITERATIONS 16

#define c2l(c,l)	(l =((unsigned long)(*((c)++)))    , \
	l|=((unsigned long)(*((c)++)))<< 8, \
	l|=((unsigned long)(*((c)++)))<<16, \
	l|=((unsigned long)(*((c)++)))<<24)


#define l2c(l,c)	(*((c)++)=(unsigned char)(((l)    )&0xff), \
	*((c)++)=(unsigned char)(((l)>> 8)&0xff), \
	*((c)++)=(unsigned char)(((l)>>16)&0xff), \
	*((c)++)=(unsigned char)(((l)>>24)&0xff))

/* from spr.h */

static unsigned long des_SPtrans[8][64]={
	/* nibble 0 */
	{0x00820200L, 0x00020000L, 0x80800000L, 0x80820200L,
	0x00800000L, 0x80020200L, 0x80020000L, 0x80800000L,
	0x80020200L, 0x00820200L, 0x00820000L, 0x80000200L,
	0x80800200L, 0x00800000L, 0x00000000L, 0x80020000L,
	0x00020000L, 0x80000000L, 0x00800200L, 0x00020200L,
	0x80820200L, 0x00820000L, 0x80000200L, 0x00800200L,
	0x80000000L, 0x00000200L, 0x00020200L, 0x80820000L,
	0x00000200L, 0x80800200L, 0x80820000L, 0x00000000L,
	0x00000000L, 0x80820200L, 0x00800200L, 0x80020000L,
	0x00820200L, 0x00020000L, 0x80000200L, 0x00800200L,
	0x80820000L, 0x00000200L, 0x00020200L, 0x80800000L,
	0x80020200L, 0x80000000L, 0x80800000L, 0x00820000L,
	0x80820200L, 0x00020200L, 0x00820000L, 0x80800200L,
	0x00800000L, 0x80000200L, 0x80020000L, 0x00000000L,
	0x00020000L, 0x00800000L, 0x80800200L, 0x00820200L,
	0x80000000L, 0x80820000L, 0x00000200L, 0x80020200L},

	/* nibble 1 */
	{0x10042004L, 0x00000000L, 0x00042000L, 0x10040000L,
	0x10000004L, 0x00002004L, 0x10002000L, 0x00042000L,
	0x00002000L, 0x10040004L, 0x00000004L, 0x10002000L,
	0x00040004L, 0x10042000L, 0x10040000L, 0x00000004L,
	0x00040000L, 0x10002004L, 0x10040004L, 0x00002000L,
	0x00042004L, 0x10000000L, 0x00000000L, 0x00040004L,
	0x10002004L, 0x00042004L, 0x10042000L, 0x10000004L,
	0x10000000L, 0x00040000L, 0x00002004L, 0x10042004L,
	0x00040004L, 0x10042000L, 0x10002000L, 0x00042004L,
	0x10042004L, 0x00040004L, 0x10000004L, 0x00000000L,
	0x10000000L, 0x00002004L, 0x00040000L, 0x10040004L,
	0x00002000L, 0x10000000L, 0x00042004L, 0x10002004L,
	0x10042000L, 0x00002000L, 0x00000000L, 0x10000004L,
	0x00000004L, 0x10042004L, 0x00042000L, 0x10040000L,
	0x10040004L, 0x00040000L, 0x00002004L, 0x10002000L,
	0x10002004L, 0x00000004L, 0x10040000L, 0x00042000L},

	/* nibble 2 */
	{0x41000000L, 0x01010040L, 0x00000040L, 0x41000040L,
	0x40010000L, 0x01000000L, 0x41000040L, 0x00010040L,
	0x01000040L, 0x00010000L, 0x01010000L, 0x40000000L,
	0x41010040L, 0x40000040L, 0x40000000L, 0x41010000L,
	0x00000000L, 0x40010000L, 0x01010040L, 0x00000040L,
	0x40000040L, 0x41010040L, 0x00010000L, 0x41000000L,
	0x41010000L, 0x01000040L, 0x40010040L, 0x01010000L,
	0x00010040L, 0x00000000L, 0x01000000L, 0x40010040L,
	0x01010040L, 0x00000040L, 0x40000000L, 0x00010000L,
	0x40000040L, 0x40010000L, 0x01010000L, 0x41000040L,
	0x00000000L, 0x01010040L, 0x00010040L, 0x41010000L,
	0x40010000L, 0x01000000L, 0x41010040L, 0x40000000L,
	0x40010040L, 0x41000000L, 0x01000000L, 0x41010040L,
	0x00010000L, 0x01000040L, 0x41000040L, 0x00010040L,
	0x01000040L, 0x00000000L, 0x41010000L, 0x40000040L,
	0x41000000L, 0x40010040L, 0x00000040L, 0x01010000L},

	/* nibble 3 */
	{0x00100402L, 0x04000400L, 0x00000002L, 0x04100402L,
	0x00000000L, 0x04100000L, 0x04000402L, 0x00100002L,
	0x04100400L, 0x04000002L, 0x04000000L, 0x00000402L,
	0x04000002L, 0x00100402L, 0x00100000L, 0x04000000L,
	0x04100002L, 0x00100400L, 0x00000400L, 0x00000002L,
	0x00100400L, 0x04000402L, 0x04100000L, 0x00000400L,
	0x00000402L, 0x00000000L, 0x00100002L, 0x04100400L,
	0x04000400L, 0x04100002L, 0x04100402L, 0x00100000L,
	0x04100002L, 0x00000402L, 0x00100000L, 0x04000002L,
	0x00100400L, 0x04000400L, 0x00000002L, 0x04100000L,
	0x04000402L, 0x00000000L, 0x00000400L, 0x00100002L,
	0x00000000L, 0x04100002L, 0x04100400L, 0x00000400L,
	0x04000000L, 0x04100402L, 0x00100402L, 0x00100000L,
	0x04100402L, 0x00000002L, 0x04000400L, 0x00100402L,
	0x00100002L, 0x00100400L, 0x04100000L, 0x04000402L,
	0x00000402L, 0x04000000L, 0x04000002L, 0x04100400L},

	/* nibble 4 */
	{0x02000000L, 0x00004000L, 0x00000100L, 0x02004108L,
	0x02004008L, 0x02000100L, 0x00004108L, 0x02004000L,
	0x00004000L, 0x00000008L, 0x02000008L, 0x00004100L,
	0x02000108L, 0x02004008L, 0x02004100L, 0x00000000L,
	0x00004100L, 0x02000000L, 0x00004008L, 0x00000108L,
	0x02000100L, 0x00004108L, 0x00000000L, 0x02000008L,
	0x00000008L, 0x02000108L, 0x02004108L, 0x00004008L,
	0x02004000L, 0x00000100L, 0x00000108L, 0x02004100L,
	0x02004100L, 0x02000108L, 0x00004008L, 0x02004000L,
	0x00004000L, 0x00000008L, 0x02000008L, 0x02000100L,
	0x02000000L, 0x00004100L, 0x02004108L, 0x00000000L,
	0x00004108L, 0x02000000L, 0x00000100L, 0x00004008L,
	0x02000108L, 0x00000100L, 0x00000000L, 0x02004108L,
	0x02004008L, 0x02004100L, 0x00000108L, 0x00004000L,
	0x00004100L, 0x02004008L, 0x02000100L, 0x00000108L,
	0x00000008L, 0x00004108L, 0x02004000L, 0x02000008L},

	/* nibble 5 */
	{0x20000010L, 0x00080010L, 0x00000000L, 0x20080800L,
	0x00080010L, 0x00000800L, 0x20000810L, 0x00080000L,
	0x00000810L, 0x20080810L, 0x00080800L, 0x20000000L,
	0x20000800L, 0x20000010L, 0x20080000L, 0x00080810L,
	0x00080000L, 0x20000810L, 0x20080010L, 0x00000000L,
	0x00000800L, 0x00000010L, 0x20080800L, 0x20080010L,
	0x20080810L, 0x20080000L, 0x20000000L, 0x00000810L,
	0x00000010L, 0x00080800L, 0x00080810L, 0x20000800L,
	0x00000810L, 0x20000000L, 0x20000800L, 0x00080810L,
	0x20080800L, 0x00080010L, 0x00000000L, 0x20000800L,
	0x20000000L, 0x00000800L, 0x20080010L, 0x00080000L,
	0x00080010L, 0x20080810L, 0x00080800L, 0x00000010L,
	0x20080810L, 0x00080800L, 0x00080000L, 0x20000810L,
	0x20000010L, 0x20080000L, 0x00080810L, 0x00000000L,
	0x00000800L, 0x20000010L, 0x20000810L, 0x20080800L,
	0x20080000L, 0x00000810L, 0x00000010L, 0x20080010L},

	/* nibble 6 */
	{0x00001000L, 0x00000080L, 0x00400080L, 0x00400001L,
	0x00401081L, 0x00001001L, 0x00001080L, 0x00000000L,
	0x00400000L, 0x00400081L, 0x00000081L, 0x00401000L,
	0x00000001L, 0x00401080L, 0x00401000L, 0x00000081L,
	0x00400081L, 0x00001000L, 0x00001001L, 0x00401081L,
	0x00000000L, 0x00400080L, 0x00400001L, 0x00001080L,
	0x00401001L, 0x00001081L, 0x00401080L, 0x00000001L,
	0x00001081L, 0x00401001L, 0x00000080L, 0x00400000L,
	0x00001081L, 0x00401000L, 0x00401001L, 0x00000081L,
	0x00001000L, 0x00000080L, 0x00400000L, 0x00401001L,
	0x00400081L, 0x00001081L, 0x00001080L, 0x00000000L,
	0x00000080L, 0x00400001L, 0x00000001L, 0x00400080L,
	0x00000000L, 0x00400081L, 0x00400080L, 0x00001080L,
	0x00000081L, 0x00001000L, 0x00401081L, 0x00400000L,
	0x00401080L, 0x00000001L, 0x00001001L, 0x00401081L,
	0x00400001L, 0x00401080L, 0x00401000L, 0x00001001L},

	/* nibble 7 */
	{0x08200020L, 0x08208000L, 0x00008020L, 0x00000000L,
	0x08008000L, 0x00200020L, 0x08200000L, 0x08208020L,
	0x00000020L, 0x08000000L, 0x00208000L, 0x00008020L,
	0x00208020L, 0x08008020L, 0x08000020L, 0x08200000L,
	0x00008000L, 0x00208020L, 0x00200020L, 0x08008000L,
	0x08208020L, 0x08000020L, 0x00000000L, 0x00208000L,
	0x08000000L, 0x00200000L, 0x08008020L, 0x08200020L,
	0x00200000L, 0x00008000L, 0x08208000L, 0x00000020L,
	0x00200000L, 0x00008000L, 0x08000020L, 0x08208020L,
	0x00008020L, 0x08000000L, 0x00000000L, 0x00208000L,
	0x08200020L, 0x08008020L, 0x08008000L, 0x00200020L,
	0x08208000L, 0x00000020L, 0x00200020L, 0x08008000L,
	0x08208020L, 0x00200000L, 0x08200000L, 0x08000020L,
	0x00208000L, 0x00008020L, 0x08008020L, 0x08200000L,
	0x00000020L, 0x08208000L, 0x00208020L, 0x00000000L,
	0x08000000L, 0x08200020L, 0x00008000L, 0x00208020L}
};

/* from sk.h */

static unsigned long des_skb[8][64]={
	/* for C bits (numbered as per FIPS 46) 1 2 3 4 5 6 */
	{0x00000000L, 0x00000010L, 0x20000000L, 0x20000010L,
	0x00010000L, 0x00010010L, 0x20010000L, 0x20010010L,
	0x00000800L, 0x00000810L, 0x20000800L, 0x20000810L,
	0x00010800L, 0x00010810L, 0x20010800L, 0x20010810L,
	0x00000020L, 0x00000030L, 0x20000020L, 0x20000030L,
	0x00010020L, 0x00010030L, 0x20010020L, 0x20010030L,
	0x00000820L, 0x00000830L, 0x20000820L, 0x20000830L,
	0x00010820L, 0x00010830L, 0x20010820L, 0x20010830L,
	0x00080000L, 0x00080010L, 0x20080000L, 0x20080010L,
	0x00090000L, 0x00090010L, 0x20090000L, 0x20090010L,
	0x00080800L, 0x00080810L, 0x20080800L, 0x20080810L,
	0x00090800L, 0x00090810L, 0x20090800L, 0x20090810L,
	0x00080020L, 0x00080030L, 0x20080020L, 0x20080030L,
	0x00090020L, 0x00090030L, 0x20090020L, 0x20090030L,
	0x00080820L, 0x00080830L, 0x20080820L, 0x20080830L,
	0x00090820L, 0x00090830L, 0x20090820L, 0x20090830L},
	/* for C bits (numbered as per FIPS 46) 7 8 10 11 12 13 */
	{0x00000000L, 0x02000000L, 0x00002000L, 0x02002000L,
	0x00200000L, 0x02200000L, 0x00202000L, 0x02202000L,
	0x00000004L, 0x02000004L, 0x00002004L, 0x02002004L,
	0x00200004L, 0x02200004L, 0x00202004L, 0x02202004L,
	0x00000400L, 0x02000400L, 0x00002400L, 0x02002400L,
	0x00200400L, 0x02200400L, 0x00202400L, 0x02202400L,
	0x00000404L, 0x02000404L, 0x00002404L, 0x02002404L,
	0x00200404L, 0x02200404L, 0x00202404L, 0x02202404L,
	0x10000000L, 0x12000000L, 0x10002000L, 0x12002000L,
	0x10200000L, 0x12200000L, 0x10202000L, 0x12202000L,
	0x10000004L, 0x12000004L, 0x10002004L, 0x12002004L,
	0x10200004L, 0x12200004L, 0x10202004L, 0x12202004L,
	0x10000400L, 0x12000400L, 0x10002400L, 0x12002400L,
	0x10200400L, 0x12200400L, 0x10202400L, 0x12202400L,
	0x10000404L, 0x12000404L, 0x10002404L, 0x12002404L,
	0x10200404L, 0x12200404L, 0x10202404L, 0x12202404L},
	/* for C bits (numbered as per FIPS 46) 14 15 16 17 19 20 */
	{0x00000000L, 0x00000001L, 0x00040000L, 0x00040001L,
	0x01000000L, 0x01000001L, 0x01040000L, 0x01040001L,
	0x00000002L, 0x00000003L, 0x00040002L, 0x00040003L,
	0x01000002L, 0x01000003L, 0x01040002L, 0x01040003L,
	0x00000200L, 0x00000201L, 0x00040200L, 0x00040201L,
	0x01000200L, 0x01000201L, 0x01040200L, 0x01040201L,
	0x00000202L, 0x00000203L, 0x00040202L, 0x00040203L,
	0x01000202L, 0x01000203L, 0x01040202L, 0x01040203L,
	0x08000000L, 0x08000001L, 0x08040000L, 0x08040001L,
	0x09000000L, 0x09000001L, 0x09040000L, 0x09040001L,
	0x08000002L, 0x08000003L, 0x08040002L, 0x08040003L,
	0x09000002L, 0x09000003L, 0x09040002L, 0x09040003L,
	0x08000200L, 0x08000201L, 0x08040200L, 0x08040201L,
	0x09000200L, 0x09000201L, 0x09040200L, 0x09040201L,
	0x08000202L, 0x08000203L, 0x08040202L, 0x08040203L,
	0x09000202L, 0x09000203L, 0x09040202L, 0x09040203L},
	/* for C bits (numbered as per FIPS 46) 21 23 24 26 27 28 */
	{0x00000000L, 0x00100000L, 0x00000100L, 0x00100100L,
	0x00000008L, 0x00100008L, 0x00000108L, 0x00100108L,
	0x00001000L, 0x00101000L, 0x00001100L, 0x00101100L,
	0x00001008L, 0x00101008L, 0x00001108L, 0x00101108L,
	0x04000000L, 0x04100000L, 0x04000100L, 0x04100100L,
	0x04000008L, 0x04100008L, 0x04000108L, 0x04100108L,
	0x04001000L, 0x04101000L, 0x04001100L, 0x04101100L,
	0x04001008L, 0x04101008L, 0x04001108L, 0x04101108L,
	0x00020000L, 0x00120000L, 0x00020100L, 0x00120100L,
	0x00020008L, 0x00120008L, 0x00020108L, 0x00120108L,
	0x00021000L, 0x00121000L, 0x00021100L, 0x00121100L,
	0x00021008L, 0x00121008L, 0x00021108L, 0x00121108L,
	0x04020000L, 0x04120000L, 0x04020100L, 0x04120100L,
	0x04020008L, 0x04120008L, 0x04020108L, 0x04120108L,
	0x04021000L, 0x04121000L, 0x04021100L, 0x04121100L,
	0x04021008L, 0x04121008L, 0x04021108L, 0x04121108L},
	/* for D bits (numbered as per FIPS 46) 1 2 3 4 5 6 */
	{0x00000000L, 0x10000000L, 0x00010000L, 0x10010000L,
	0x00000004L, 0x10000004L, 0x00010004L, 0x10010004L,
	0x20000000L, 0x30000000L, 0x20010000L, 0x30010000L,
	0x20000004L, 0x30000004L, 0x20010004L, 0x30010004L,
	0x00100000L, 0x10100000L, 0x00110000L, 0x10110000L,
	0x00100004L, 0x10100004L, 0x00110004L, 0x10110004L,
	0x20100000L, 0x30100000L, 0x20110000L, 0x30110000L,
	0x20100004L, 0x30100004L, 0x20110004L, 0x30110004L,
	0x00001000L, 0x10001000L, 0x00011000L, 0x10011000L,
	0x00001004L, 0x10001004L, 0x00011004L, 0x10011004L,
	0x20001000L, 0x30001000L, 0x20011000L, 0x30011000L,
	0x20001004L, 0x30001004L, 0x20011004L, 0x30011004L,
	0x00101000L, 0x10101000L, 0x00111000L, 0x10111000L,
	0x00101004L, 0x10101004L, 0x00111004L, 0x10111004L,
	0x20101000L, 0x30101000L, 0x20111000L, 0x30111000L,
	0x20101004L, 0x30101004L, 0x20111004L, 0x30111004L},
	/* for D bits (numbered as per FIPS 46) 8 9 11 12 13 14 */
	{0x00000000L, 0x08000000L, 0x00000008L, 0x08000008L,
	0x00000400L, 0x08000400L, 0x00000408L, 0x08000408L,
	0x00020000L, 0x08020000L, 0x00020008L, 0x08020008L,
	0x00020400L, 0x08020400L, 0x00020408L, 0x08020408L,
	0x00000001L, 0x08000001L, 0x00000009L, 0x08000009L,
	0x00000401L, 0x08000401L, 0x00000409L, 0x08000409L,
	0x00020001L, 0x08020001L, 0x00020009L, 0x08020009L,
	0x00020401L, 0x08020401L, 0x00020409L, 0x08020409L,
	0x02000000L, 0x0A000000L, 0x02000008L, 0x0A000008L,
	0x02000400L, 0x0A000400L, 0x02000408L, 0x0A000408L,
	0x02020000L, 0x0A020000L, 0x02020008L, 0x0A020008L,
	0x02020400L, 0x0A020400L, 0x02020408L, 0x0A020408L,
	0x02000001L, 0x0A000001L, 0x02000009L, 0x0A000009L,
	0x02000401L, 0x0A000401L, 0x02000409L, 0x0A000409L,
	0x02020001L, 0x0A020001L, 0x02020009L, 0x0A020009L,
	0x02020401L, 0x0A020401L, 0x02020409L, 0x0A020409L},
	/* for D bits (numbered as per FIPS 46) 16 17 18 19 20 21 */
	{0x00000000L, 0x00000100L, 0x00080000L, 0x00080100L,
	0x01000000L, 0x01000100L, 0x01080000L, 0x01080100L,
	0x00000010L, 0x00000110L, 0x00080010L, 0x00080110L,
	0x01000010L, 0x01000110L, 0x01080010L, 0x01080110L,
	0x00200000L, 0x00200100L, 0x00280000L, 0x00280100L,
	0x01200000L, 0x01200100L, 0x01280000L, 0x01280100L,
	0x00200010L, 0x00200110L, 0x00280010L, 0x00280110L,
	0x01200010L, 0x01200110L, 0x01280010L, 0x01280110L,
	0x00000200L, 0x00000300L, 0x00080200L, 0x00080300L,
	0x01000200L, 0x01000300L, 0x01080200L, 0x01080300L,
	0x00000210L, 0x00000310L, 0x00080210L, 0x00080310L,
	0x01000210L, 0x01000310L, 0x01080210L, 0x01080310L,
	0x00200200L, 0x00200300L, 0x00280200L, 0x00280300L,
	0x01200200L, 0x01200300L, 0x01280200L, 0x01280300L,
	0x00200210L, 0x00200310L, 0x00280210L, 0x00280310L,
	0x01200210L, 0x01200310L, 0x01280210L, 0x01280310L},
	/* for D bits (numbered as per FIPS 46) 22 23 24 25 27 28 */
	{0x00000000L, 0x04000000L, 0x00040000L, 0x04040000L,
	0x00000002L, 0x04000002L, 0x00040002L, 0x04040002L,
	0x00002000L, 0x04002000L, 0x00042000L, 0x04042000L,
	0x00002002L, 0x04002002L, 0x00042002L, 0x04042002L,
	0x00000020L, 0x04000020L, 0x00040020L, 0x04040020L,
	0x00000022L, 0x04000022L, 0x00040022L, 0x04040022L,
	0x00002020L, 0x04002020L, 0x00042020L, 0x04042020L,
	0x00002022L, 0x04002022L, 0x00042022L, 0x04042022L,
	0x00000800L, 0x04000800L, 0x00040800L, 0x04040800L,
	0x00000802L, 0x04000802L, 0x00040802L, 0x04040802L,
	0x00002800L, 0x04002800L, 0x00042800L, 0x04042800L,
	0x00002802L, 0x04002802L, 0x00042802L, 0x04042802L,
	0x00000820L, 0x04000820L, 0x00040820L, 0x04040820L,
	0x00000822L, 0x04000822L, 0x00040822L, 0x04040822L,
	0x00002820L, 0x04002820L, 0x00042820L, 0x04042820L,
	0x00002822L, 0x04002822L, 0x00042822L, 0x04042822L}
};

/* from ecb_encrypt.c */

#define PERM_OP(a,b,t,n,m) ((t)=((((a)>>(n))^(b))&(m)),\
	(b)^=(t),\
	(a)=((a)^((t)<<(n))))

#define HPERM_OP(a,t,n,m) ((t)=((((a)<<(16-(n)))^(a))&(m)),\
	(a)=(a)^(t)^(t>>(16-(n))))

static char shifts2[16]={0,0,1,1,1,1,1,1,0,1,1,1,1,1,1,0};



#define D_ENCRYPT(L,R,S)	\
	u=(R^s[S  ]); \
	t=R^s[S+1]; \
	t=((t>>4)+(t<<28)); \
	L^=	des_SPtrans[1][(t    )&0x3f]| \
	des_SPtrans[3][(t>> 8)&0x3f]| \
	des_SPtrans[5][(t>>16)&0x3f]| \
	des_SPtrans[7][(t>>24)&0x3f]| \
	des_SPtrans[0][(u    )&0x3f]| \
	des_SPtrans[2][(u>> 8)&0x3f]| \
	des_SPtrans[4][(u>>16)&0x3f]| \
	des_SPtrans[6][(u>>24)&0x3f];



static void des_encrypt(unsigned long *input,unsigned long *output,
						des_key_schedule ks,int encrypt)
{
	register unsigned long l,r,t,u;

	register int i;
	register unsigned long *s;

	l=input[0];
	r=input[1];

	/* do IP */
	PERM_OP(r,l,t, 4,0x0f0f0f0fL);
	PERM_OP(l,r,t,16,0x0000ffffL);
	PERM_OP(r,l,t, 2,0x33333333L);
	PERM_OP(l,r,t, 8,0x00ff00ffL);
	PERM_OP(r,l,t, 1,0x55555555L);
	/* r and l are reversed - remember that :-) - fix
	* it in the next step */

	/* Things have been modified so that the initial rotate is
	* done outside the loop.  This required the 
	* des_SPtrans values in sp.h to be rotated 1 bit to the right.
	* One perl script later and things have a 5% speed up on a sparc2.
	* Thanks to Richard Outerbridge <71755.204@CompuServe.COM>
	* for pointing this out. */
	t=(r<<1)|(r>>31);
	r=(l<<1)|(l>>31); 
	l=t;

	/* clear the top bits on machines with 8byte longs */
	l&=0xffffffffL;
	r&=0xffffffffL;

	s=(unsigned long *)ks;
	/* I don't know if it is worth the effort of loop unrolling the
	* inner loop */
	if (encrypt)
	{
		for (i=0; i<32; i+=4)
		{
			D_ENCRYPT(l,r,i+0); /*  1 */
			D_ENCRYPT(r,l,i+2); /*  2 */
		}
	}
	else
	{
		for (i=30; i>0; i-=4)
		{
			D_ENCRYPT(l,r,i-0); /* 16 */
			D_ENCRYPT(r,l,i-2); /* 15 */
		}
	}
	l=(l>>1)|(l<<31);
	r=(r>>1)|(r<<31);
	/* clear the top bits on machines with 8byte longs */
	l&=0xffffffffL;
	r&=0xffffffffL;

	/* swap l and r
	* we will not do the swap so just remember they are
	* reversed for the rest of the subroutine
	* luckily FP fixes this problem :-) */

	PERM_OP(r,l,t, 1,0x55555555L);
	PERM_OP(l,r,t, 8,0x00ff00ffL);
	PERM_OP(r,l,t, 2,0x33333333L);
	PERM_OP(l,r,t,16,0x0000ffffL);
	PERM_OP(r,l,t, 4,0x0f0f0f0fL);

	output[0]=l;
	output[1]=r;
}

static int des_ecb_encrypt(des_cblock *input,des_cblock *output,des_key_schedule ks,
						   int encrypt)
{
	register unsigned long l0,l1;
	register unsigned char *in,*out;
	unsigned long ll[2];

	in=(unsigned char *)input;
	out=(unsigned char *)output;
	c2l(in,l0);
	c2l(in,l1);
	ll[0]=l0;
	ll[1]=l1;
	des_encrypt(ll,ll,ks,encrypt);
	l0=ll[0];
	l1=ll[1];
	l2c(l0,out);
	l2c(l1,out);
	return(0);
}


static void des_set_key(des_cblock *key,des_key_schedule schedule)
{
	register unsigned long c,d,t,s;
	register unsigned char *in;
	register unsigned long *k;
	register int i;

	k=(unsigned long *)schedule;
	in=(unsigned char *)key;

	c2l(in,c);
	c2l(in,d);

	/* I now do it in 47 simple operations :-)
	* Thanks to John Fletcher (john_fletcher@lccmail.ocf.llnl.gov)
	* for the inspiration. :-) */
	PERM_OP (d,c,t,4,0x0f0f0f0fL);
	HPERM_OP(c,t,-2,0xcccc0000L);
	HPERM_OP(d,t,-2,0xcccc0000L);
	PERM_OP (d,c,t,1,0x55555555L);
	PERM_OP (c,d,t,8,0x00ff00ffL);
	PERM_OP (d,c,t,1,0x55555555L);
	d=	(((d&0x000000ffL)<<16)| (d&0x0000ff00L)     |
		((d&0x00ff0000L)>>16)|((c&0xf0000000L)>>4));
	c&=0x0fffffffL;

	for (i=0; i<ITERATIONS; i++)
	{
		if (shifts2[i])
		{ c=((c>>2)|(c<<26)); d=((d>>2)|(d<<26)); }
		else
		{ c=((c>>1)|(c<<27)); d=((d>>1)|(d<<27)); }
		c&=0x0fffffffL;
		d&=0x0fffffffL;
		/* could be a few less shifts but I am to lazy at this
		* point in time to investigate */
		s=	des_skb[0][ (c    )&0x3f                ]|
			des_skb[1][((c>> 6)&0x03)|((c>> 7)&0x3c)]|
			des_skb[2][((c>>13)&0x0f)|((c>>14)&0x30)]|
			des_skb[3][((c>>20)&0x01)|((c>>21)&0x06) |
			((c>>22)&0x38)];
		t=	des_skb[4][ (d    )&0x3f                ]|
			des_skb[5][((d>> 7)&0x03)|((d>> 8)&0x3c)]|
			des_skb[6][ (d>>15)&0x3f                ]|
			des_skb[7][((d>>21)&0x0f)|((d>>22)&0x30)];

		/* table contained 0213 4657 */
		*(k++)=((t<<16)|(s&0x0000ffffL))&0xffffffffL;
		s=     ((s>>16)|(t&0xffff0000L));

		s=(s<<4)|(s>>28);
		*(k++)=s&0xffffffffL;
	}
}

//------------------------------------------------------------------------------
// This is fdmc source
static des_key_schedule dk;
//static FDMC_THREAD_LOCK *des_lock = NULL;

unsigned char traffic_key[8];
unsigned char traffic_dbl_key[16];

int fdmc_des_block_encrypt(char *buf, int len)
{
	int clen;

	clen = fdmc_des_buffer_encrypt(buf, len, traffic_key);
	return clen;
}

int fdmc_des_block_decrypt(char *buf, int len)
{
	int clen;

	clen = fdmc_des_buffer_decrypt(buf, len, traffic_key);
	return clen;
}

int fdmc_des_block_trpl_encrypt(char *buf, int len)
{
	int clen;

	clen = fdmc_des_buffer_trpl_encrypt(buf, len, traffic_dbl_key);
	return clen;
}

int fdmc_des_block_trpl_decrypt(char *buf, int len)
{
	int clen;

	clen = fdmc_des_buffer_trpl_decrypt(buf, len, traffic_key);
	return clen;
}

int fdmc_des_buffer_encrypt(char *buf, int len, unsigned char *key)
{
	int clen, i;

	//if(des_lock == NULL)
	//{
	//	if((des_lock = fdmc_thread_lock_create(NULL)) == NULL)
	//	{
	//		return 0;
	//	}
	//}
	//fdmc_thread_lock(des_lock, NULL);
	if(len%8)
		clen = len + (8-(len%8));
	else
		clen = len;
	for(i = len; i < clen; i++)
		buf[i] = '\0';
	des_set_key((des_cblock*)key, dk);
	for(i=0; i < clen; i+=8)
	{
		des_ecb_encrypt((des_cblock*)buf+i, (des_cblock*)buf+i, dk, 1);
	}
	//fdmc_thread_unlock(des_lock, NULL);
	return clen;
}

int fdmc_des_buffer_decrypt(char *buf, int len, unsigned char *key)
{
	int clen, i;

	//if(des_lock == NULL)
	//{
	//	if((des_lock = fdmc_thread_lock_create(NULL)) == NULL)
	//	{
	//		return 0;
	//	}
	//}
	//fdmc_thread_lock(des_lock, NULL);
	clen = len;	
	des_set_key((des_cblock*)key, dk);
	for(i=0; i < clen; i+=8)
	{
		des_ecb_encrypt((des_cblock*)buf+i, (des_cblock*)buf+i, dk, 0);
	}
	//fdmc_thread_unlock(des_lock, NULL);
	return clen;
}

int fdmc_des_buffer_trpl_encrypt(char *buf, int len, unsigned char *key)
{
	int clen, i;

	/* Justify the length of the message */
	if(len % 8)
		clen = len + (8-(len%8));
	else
		clen = len;
	for(i = len; i < clen; i++)
		buf[i] = '\0';
	/* Encrypt message by the first key */
	fdmc_des_buffer_encrypt(buf, clen, key);
	/* Decrypt message by the second key */
	fdmc_des_buffer_decrypt(buf, clen, key + 8);
	/* Encrypt message by the first key again */
	fdmc_des_buffer_encrypt(buf, clen, key);
	return clen;
}

int fdmc_des_buffer_trpl_decrypt(char *buf, int len, unsigned char *key)
{
	int clen, i;

	/* Justify the length of the message */
	if(len % 8)
		clen = len + (8-(len%8));
	else
		clen = len;

	for(i = len; i < clen; i++)
		buf[i] = '\0';

	/* Decrypt message by first key */
	fdmc_des_buffer_decrypt(buf, clen, key);
	/* Encrypt message by the second key */
	fdmc_des_buffer_encrypt(buf, clen, key + 8);
	/* Decrypt message by first key again */
	fdmc_des_buffer_decrypt(buf, clen, key);
	return clen;
}

int fdmc_des_buffer_trplcbc_encrypt(unsigned char *buff, unsigned len, unsigned char *key)
{
	unsigned i, j, just_len;
	unsigned char xor_block[8];

	just_len = len;

	while(just_len % 8)
		buff[just_len++] = '\0';

	memset(xor_block, 0, 8);
	for(i = 0; i < just_len; i += 8)
	{
		for(j = 0; j < 8; j++)
		{
			buff[i + j] ^= xor_block[j];
		}
		fdmc_des_buffer_trpl_encrypt((char*)buff + i, 8, key);
		memcpy(xor_block, buff + i, 8);
	}
	return just_len;
}

int fdmc_des_buffer_trplcbc_decrypt(unsigned char *buff, unsigned len, unsigned char *key)
{
	unsigned i, j, just_len;
	unsigned char xor_block[8], crypt_block[8];

	just_len = len;

	while(just_len % 8)
		buff[just_len++] = '\0';

	memset(xor_block, 0, 8);
	for(i = 0; i < just_len; i += 8)
	{
		memcpy(crypt_block, buff + i, 8);
		fdmc_des_buffer_trpl_decrypt((char*)crypt_block, 8, key);
		for(j = 0; j < 8; j++)
		{
			crypt_block[j] ^= xor_block[j];
		}
		memcpy(xor_block, buff + i, 8);
		memcpy(buff + i, crypt_block, 8);
	}
	return just_len;
}

static char hexch(unsigned char x)
{
	return x <= 9 ? '0' + x : 'A' + x - 10;
}

static unsigned char *bin_to_hex(unsigned char bin, char *hex)
{
	hex[0] = hexch(bin >> 4);
	hex[1] = hexch(bin & 0xf);
	return (unsigned char*)hex;
}

static unsigned char binch(char ch)
{
	unsigned char ret = (unsigned char)ch <= '9' ? ch - '0' : ch - 'A' + 10;
	return ret;
}

static unsigned char hex_to_bin(char *hex)
{
	unsigned char ret = binch(hex[0]) * 0x10 + binch(hex[1]);
	return ret;
}

char* fdmc_des_key_to_hex(unsigned char *key, char *hex, int key_type)
{
	int i;

	for(i = 0; i < 8 * key_type; i++)
	{
		bin_to_hex(key[i], &hex[i * 2]);
	}
	return hex;
}

unsigned char* fdmc_des_hex_to_key(char *hex, unsigned char *key, int key_type)
{
	int i;

	for(i = 0; i < 16 * key_type; i += 2)
	{
		key[i / 2] = hex_to_bin(hex + i);
	}
	return key;
}

static int byte_parity_check(unsigned char b)
{
	int pflag;
	int psum;
	int i;

	// Get parity flag
	pflag = b & 1;
	b = b >> 1;
	// Calculate parity
	psum = 0;
	for(i = 0; i < 7; i++, b = b >> 1)
	{
		psum += (b & 1);
	}
	psum %= 2;
	return psum != pflag;
}

unsigned char* fdmc_des_parity_set(unsigned char *key, int len)
{
	int i, j;
	unsigned char uc, sum;

	for(j = 0; j < len; j++)
	{
		uc = *key >> 1;
		sum = 0;
		for(i = 0; i < 7; i++, uc = uc >> 1)
		{
			sum += (uc & 1);
		}
		sum %= 2;
		if(!sum)
		{
			*key |= 1;
		}
		else
		{
			*key &= (~1);
		}
	}
	return key;
}

int fdmc_des_parity_check(unsigned char *key, int key_type)
{
	int i, j;

	j = 8 * key_type;

	for(i = 0; i < j; i++)
	{
		if(!byte_parity_check(key[i]))
		{
			return 0;
		}
	}
	return 1;
}

char fdmc_luhn_calc(char *card_no)
{
	char res = 0;
	size_t len = strlen(card_no);
	size_t i;
	char k = 0;
	char c;

	if (len == 0)
	{
		return 0;
	}
	/* Loop from the end of the string */
	for(i = len; i > 0; i--)
	{
		c = card_no[i - 1];
		/* Only digits are available in this version */
		if (c > '9' || c < '0')
		{
			return 0;
		}
		c -= '0';
		if(k++ % 2 == 0)
		{
			/* Multiple symbol by 2, if even, starting from zero */
			c *= 2;
			if(c > 9) 
			{
				/* if number is greater than 9 (two digits), sum 2 digits of the number, for example 14 : 1+4 = 5 */
				c = c % 10 + 1;
			}
		}
		/* Sum result */
		res += c;
	}
	/* Satisfy condition - luhn % 10 == 0 */
	res = 10 - res % 10;
	return res + '0';
}

