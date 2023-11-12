/*
 *  unzx0_c.c - ZX0 decompressor written in C.
 *  This is a very dirty translation of the 68000 version by Emmanuel Marty.
 *
 *  in:  in  = start of compressed data
 *       out = start of decompression buffer
 *
 *  Copyright (C) 2023 René W. Olsen
 *  ZX0 compression (c) 2021 Einar Saukas, https://github.com/einar-saukas/ZX0
 *
 *  This software is provided 'as-is', without any express or implied
 *  warranty.  In no event will the authors be held liable for any damages
 *  arising from the use of this software.
 *
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, subject to the following restrictions:
 *
 *  1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 *  2. Altered source versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software.
 *  3. This notice may not be removed or altered from any source distribution.
 */

// --

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

// --

struct myZX0
{
	uint8_t	*	Input;
	uint8_t	*	Output;

	int			XFlag;

	int32_t		RegD0;
	uint8_t		RegD1;
	int32_t		RegD2;
};

// --

static void Decrunch(		struct myZX0 *zx );
static void get_elias(		struct myZX0 *zx );
static int	get_offset(		struct myZX0 *zx );

// --

int zx0_decompress( uint8_t *in, uint8_t *out )
{
struct myZX0 zx;
int bytes;

	zx.Input	= in;
	zx.Output	= out;
	zx.RegD1	= 0x80;
	zx.RegD2	= -1;

	Decrunch( & zx );

	bytes = ( zx.Output - out );

//	printf( "%d Bytes decrunched\n", bytes );

	return( bytes );
}

// --

static void Decrunch( struct myZX0 *zx )
{
uint8_t *ptr;
int cnt;

	while( true )
	{
		zx->RegD0 = 1;

		get_elias( zx );

		for( cnt=0 ; cnt<zx->RegD0 ; cnt++ )
		{
			*zx->Output++ = *zx->Input++;
		}

		zx->XFlag = ( zx->RegD1 & 0x80 ) ? 1 : 0;
		zx->RegD1 = zx->RegD1 + zx->RegD1;

		if ( zx->XFlag )
		{
			if ( get_offset( zx ))
			{
				return;
			}
		}
		else
		{
			zx->RegD0 = 1;

			get_elias( zx );
		}

		while( true )
		{
			ptr = zx->Output;
			ptr += zx->RegD2;

			for( cnt=0 ; cnt<zx->RegD0 ; cnt++ )
			{
				*zx->Output++ = *ptr++;
			}

			zx->XFlag = ( zx->RegD1 & 0x80 ) ? 1 : 0;
			zx->RegD1 = zx->RegD1 + zx->RegD1;

			if ( ! zx->XFlag )
			{
				break;
			}

			if ( get_offset( zx ))
			{
				return;
			}
		}
	}
}

// --

static int get_offset( struct myZX0 *zx )
{
	zx->RegD0 = -2;

	get_elias( zx );

	zx->RegD0++;
	zx->RegD0 &= 0x000000ff;

	if ( ! zx->RegD0 )
	{
		// We are Done 
		return( 1 );
	}

	zx->RegD2 = ( 0xffff0000 ) + ( zx->RegD0 << 8 ) + ( *zx->Input++ );

	zx->RegD0 = 1;

	zx->XFlag = ( zx->RegD2 & 0x01 ) ? 1 : 0 ;
	zx->RegD2 = zx->RegD2 >> 1;

	if ( ! zx->XFlag )
	{
		zx->XFlag = ( zx->RegD1 & 0x80 ) ? 1 : 0;
		zx->RegD1 = zx->RegD1 + zx->RegD1;
		zx->RegD0 = zx->RegD0 + zx->RegD0 + zx->XFlag;

		get_elias( zx );
	}

	zx->RegD0++;

	return( 0 );
}

// --

static void get_elias( struct myZX0 *zx )
{
int x;

	while( true )
	{
		zx->XFlag = ( zx->RegD1 & 0x80 ) ? 1 : 0;
		zx->RegD1 = zx->RegD1 + zx->RegD1;

		if ( ! zx->RegD1 )
		{
			x = zx->XFlag;
			zx->RegD1 = *zx->Input++;
			zx->XFlag = ( zx->RegD1 & 0x80 ) ? 1 : 0;
			zx->RegD1 = zx->RegD1 + zx->RegD1 + x;
		}

		if ( zx->XFlag )
		{
			break;
		}

		zx->XFlag = ( zx->RegD1 & 0x80 ) ? 1 : 0;
		zx->RegD1 = zx->RegD1 + zx->RegD1;
		zx->RegD0 = zx->RegD0 + zx->RegD0 + zx->XFlag;
	}
}

// --
