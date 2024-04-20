/*
 * Copyright © 2010-2014 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "mbdata.h"
#include "mbreg.h"
#if MB_ASSERT_ENABLE
#include <stdio.h>
#endif
static inline uint16_t bswap_16(uint16_t x) {
	return (x >> 8) | (x << 8);
}

static inline uint32_t bswap_32(uint32_t x) {
	return (bswap_16(x & 0xffff) << 16) | (bswap_16(x >> 16));
}

static int32_t ntohl(int32_t d) {
	return d;
}
static int32_t htonl(int32_t d) {
	return d;
}

/* Sets many bits from a single byte value (all 8 bits of the byte value are
 set) */
void modbus_set_bits_from_byte(uint8_t *dest, int idx, const uint8_t value) {
	int i;

	for (i = 0; i < 8; i++) {
		dest[idx + i] = (value & (1 << i)) ? 1 : 0;
	}
}

/* Sets many bits from a table of bytes (only the bits between idx and
 idx + nb_bits are set) */
void modbus_set_bits_from_bytes(uint8_t *dest, int idx, unsigned int nb_bits,
		const uint8_t *tab_byte) {
	unsigned int i;
	int shift = 0;

	for (i = idx; i < idx + nb_bits; i++) {
		dest[i] = tab_byte[(i - idx) / 8] & (1 << shift) ? 1 : 0;
		/* gcc doesn't like: shift = (++shift) % 8; */
		shift++;
		shift %= 8;
	}
}

/* Gets the byte value from many bits.
 To obtain a full byte, set nb_bits to 8. */
uint8_t modbus_get_byte_from_bits(const uint8_t *src, int idx,
		unsigned int nb_bits) {
	unsigned int i;
	uint8_t value = 0;

	if (nb_bits > 8) {
		/* Assert is ignored if NDEBUG is set */
		assert(nb_bits < 8);
		nb_bits = 8;
	}

	for (i = 0; i < nb_bits; i++) {
		value |= (src[idx + i] << i);
	}

	return value;
}

/* Get a float from 4 bytes (Modbus) without any conversion (ABCD) */
float modbus_get_float_abcd(const uint16_t *src) {
	float f;
	uint32_t i;

	i = ntohl(((uint32_t) src[0] << 16) + src[1]);
	memcpy(&f, &i, sizeof(float));

	return f;
}

/* Get a float from 4 bytes (Modbus) in inversed format (DCBA) */
float modbus_get_float_dcba(const uint16_t *src) {
	float f;
	uint32_t i;

	i = ntohl(bswap_32((((uint32_t) src[0]) << 16) + src[1]));
	memcpy(&f, &i, sizeof(float));

	return f;
}

/* Get a float from 4 bytes (Modbus) with swapped bytes (BADC) */
float modbus_get_float_badc(const uint16_t *src) {
	float f;
	uint32_t i;

	i = ntohl((uint32_t) (bswap_16(src[0]) << 16) + bswap_16(src[1]));
	memcpy(&f, &i, sizeof(float));

	return f;
}

/* Get a float from 4 bytes (Modbus) with swapped words (CDAB) */
float modbus_get_float_cdab(const uint16_t *src) {
	float f;
	uint32_t i;

	i = ntohl((((uint32_t) src[1]) << 16) + src[0]);
	memcpy(&f, &i, sizeof(float));

	return f;
}

/* DEPRECATED - Get a float from 4 bytes in sort of Modbus format */
float modbus_get_float(const uint16_t *src) {
	float f;
	uint32_t i;

	i = (((uint32_t) src[1]) << 16) + src[0];
	memcpy(&f, &i, sizeof(float));

	return f;
}

/* Set a float to 4 bytes for Modbus w/o any conversion (ABCD) */
void modbus_set_float_abcd(float f, uint16_t *dest) {
	uint32_t i;

	memcpy(&i, &f, sizeof(uint32_t));
	i = htonl(i);
	dest[0] = (uint16_t) (i >> 16);
	dest[1] = (uint16_t) i;
}

/* Set a float to 4 bytes for Modbus with byte and word swap conversion (DCBA) */
void modbus_set_float_dcba(float f, uint16_t *dest) {
	uint32_t i;

	memcpy(&i, &f, sizeof(uint32_t));
	i = bswap_32(htonl(i));
	dest[0] = (uint16_t) (i >> 16);
	dest[1] = (uint16_t) i;
}

/* Set a float to 4 bytes for Modbus with byte swap conversion (BADC) */
void modbus_set_float_badc(float f, uint16_t *dest) {
	uint32_t i;

	memcpy(&i, &f, sizeof(uint32_t));
	i = htonl(i);
	dest[0] = (uint16_t) bswap_16(i >> 16);
	dest[1] = (uint16_t) bswap_16(i & 0xFFFF);
}

/* Set a float to 4 bytes for Modbus with word swap conversion (CDAB) */
void modbus_set_float_cdab(float f, uint16_t *dest) {
	uint32_t i;

	memcpy(&i, &f, sizeof(uint32_t));
	i = htonl(i);
	dest[0] = (uint16_t) i;
	dest[1] = (uint16_t) (i >> 16);
}

/* DEPRECATED - Set a float to 4 bytes in a sort of Modbus format! */
void modbus_set_float(float f, uint16_t *dest) {
	uint32_t i;

	memcpy(&i, &f, sizeof(uint32_t));
	dest[0] = (uint16_t) i;
	dest[1] = (uint16_t) (i >> 16);
}

//下面都是modbus 从机寄存器快速读写函数。
//hold reg fun
void mb_hd_fset(float f, uint16_t add) {
	mb_hd_fset_abcd(f, add);
}
void mb_hd_fset_abcd(float f, uint16_t add) {
	uint16_t *dest;
	MB_ASSERT(IS_MB_HOLDING_REG(add));
	dest = &usSHoldBuf[add - REG_HOLDING_START];
	modbus_set_float_abcd(f, dest);
}
void mb_hd_fset_dcba(float f, uint16_t add) {
	uint16_t *dest;
	MB_ASSERT(IS_MB_HOLDING_REG(add));
	dest = &usSHoldBuf[add - REG_HOLDING_START];
	modbus_set_float_dcba(f, dest);
}
void mb_hd_fset_badc(float f, uint16_t add) {
	uint16_t *dest;
	MB_ASSERT(IS_MB_HOLDING_REG(add));
	dest = &usSHoldBuf[add - REG_HOLDING_START];
	modbus_set_float_badc(f, dest);
}
void mb_hd_fset_cdab(float f, uint16_t add) {
	uint16_t *dest;
	MB_ASSERT(IS_MB_HOLDING_REG(add));
	dest = &usSHoldBuf[add - REG_HOLDING_START];
	modbus_set_float_cdab(f, dest);
}
void mb_hd_s64set(int64_t d, uint16_t add) {
	uint32_t index;
	MB_ASSERT(IS_MB_HOLDING_REG(add));
	index = add - REG_HOLDING_START;
	MODBUS_SET_INT64_TO_INT16(usSHoldBuf, index, d);
}
void mb_hd_s32set(int32_t d, uint16_t add) {
	uint32_t index;
	MB_ASSERT(IS_MB_HOLDING_REG(add));
	index = add - REG_HOLDING_START;
	MODBUS_SET_INT32_TO_INT16(usSHoldBuf, index, d);
}
void mb_hd_s16set(int16_t d, uint16_t add) {
	uint32_t index;
	MB_ASSERT(IS_MB_HOLDING_REG(add));
	index = add - REG_HOLDING_START;
	usSHoldBuf[index] = d;
}
float mb_hd_fget(uint16_t add) {
	return mb_hd_fget_abcd(add);
}
float mb_hd_fget_abcd(uint16_t add) {
	uint16_t *dest;
	MB_ASSERT(IS_MB_HOLDING_REG(add));
	dest = &usSHoldBuf[add - REG_HOLDING_START];
	return modbus_get_float_abcd(dest);
}
float mb_hd_fget_dcba(uint16_t add) {
	uint16_t *dest;
	MB_ASSERT(IS_MB_HOLDING_REG(add));
	dest = &usSHoldBuf[add - REG_HOLDING_START];
	return modbus_get_float_dcba(dest);
}
float mb_hd_fget_badc(uint16_t add) {
	uint16_t *dest;
	MB_ASSERT(IS_MB_HOLDING_REG(add));
	dest = &usSHoldBuf[add - REG_HOLDING_START];
	return modbus_get_float_badc(dest);
}
float mb_hd_fget_cdab(uint16_t add) {
	uint16_t *dest;
	MB_ASSERT(IS_MB_HOLDING_REG(add));
	dest = &usSHoldBuf[add - REG_HOLDING_START];
	return modbus_get_float_cdab(dest);
}
int64_t mb_hd_s64get(uint16_t add) {
	uint32_t index;
	MB_ASSERT(IS_MB_HOLDING_REG(add));
	index = add - REG_HOLDING_START;
	return MODBUS_GET_INT64_FROM_INT16(usSHoldBuf, index);
}
int32_t mb_hd_s32get(uint16_t add) {
	uint32_t index;
	MB_ASSERT(IS_MB_HOLDING_REG(add));
	index = add - REG_HOLDING_START;
	return MODBUS_GET_INT32_FROM_INT16(usSHoldBuf, index);
}
int16_t mb_hd_s16get(uint16_t add) {
	uint32_t index;
	MB_ASSERT(IS_MB_HOLDING_REG(add));
	index = add - REG_HOLDING_START;
	return usSHoldBuf[index];
}
// input reg
void mb_in_fset(float f, uint16_t add) {
	mb_in_fset_abcd(f, add);
}
void mb_in_fset_abcd(float f, uint16_t add) {
	uint16_t *dest;
	MB_ASSERT(IS_MB_INPUT_REG(add));
	dest = &usSInBuf[add - REG_INPUT_START];
	modbus_set_float_abcd(f, dest);
}
void mb_in_fset_dcba(float f, uint16_t add) {
	uint16_t *dest;
	MB_ASSERT(IS_MB_INPUT_REG(add));
	dest = &usSInBuf[add - REG_INPUT_START];
	modbus_set_float_dcba(f, dest);
}
void mb_in_fset_badc(float f, uint16_t add) {
	uint16_t *dest;
	MB_ASSERT(IS_MB_INPUT_REG(add));
	dest = &usSInBuf[add - REG_INPUT_START];
	modbus_set_float_badc(f, dest);
}

void mb_in_fset_cdab(float f, uint16_t add) {
	uint16_t *dest;
	MB_ASSERT(IS_MB_INPUT_REG(add));
	dest = &usSInBuf[add - REG_INPUT_START];
	modbus_set_float_cdab(f, dest);
}
void mb_in_s64set(int64_t d, uint16_t add) {
	uint32_t index;
	MB_ASSERT(IS_MB_INPUT_REG(add));
	index = add - REG_INPUT_START;
	MODBUS_SET_INT64_TO_INT16(usSInBuf, index, d);
}
void mb_in_s32set(int32_t d, uint16_t add) {
	uint32_t index;
	MB_ASSERT(IS_MB_INPUT_REG(add));
	index = add - REG_INPUT_START;
	MODBUS_SET_INT32_TO_INT16(usSInBuf, index, d);
}
void mb_in_s16set(int16_t d, uint16_t add) {
	uint32_t index;
	MB_ASSERT(IS_MB_INPUT_REG(add));
	index = add - REG_INPUT_START;
	usSInBuf[index] = d;
}
float mb_in_fget(uint16_t add){
	return mb_in_fget_abcd(add);
}
float mb_in_fget_abcd(uint16_t add) {
	uint16_t *dest;
	MB_ASSERT(IS_MB_INPUT_REG(add));
	dest = &usSInBuf[add - REG_INPUT_START];
	return modbus_get_float_abcd(dest);
}
float mb_in_fget_dcba(uint16_t add){
	uint16_t *dest;
	MB_ASSERT(IS_MB_INPUT_REG(add));
	dest = &usSInBuf[add - REG_INPUT_START];
	return modbus_get_float_dcba(dest);
}
float mb_in_fget_badc(uint16_t add){
	uint16_t *dest;
	MB_ASSERT(IS_MB_INPUT_REG(add));
	dest = &usSInBuf[add - REG_INPUT_START];
	return modbus_get_float_badc(dest);
}
float mb_in_fget_cdab(uint16_t add){
	uint16_t *dest;
	MB_ASSERT(IS_MB_INPUT_REG(add));
	dest = &usSInBuf[add - REG_INPUT_START];
	return modbus_get_float_cdab(dest);
}
int64_t mb_in_s64get(uint16_t add) {
	uint32_t index;
	MB_ASSERT(IS_MB_INPUT_REG(add));
	index = add - REG_INPUT_START;
	return MODBUS_GET_INT64_FROM_INT16(usSInBuf, index);
}
int32_t mb_in_s32get(uint16_t add){
	uint32_t index;
	MB_ASSERT(IS_MB_INPUT_REG(add));
	index = add - REG_INPUT_START;
	return MODBUS_GET_INT32_FROM_INT16(usSInBuf, index);
}
int16_t mb_in_s16get(uint16_t add){
	uint32_t index;
	MB_ASSERT(IS_MB_INPUT_REG(add));
	index = add - REG_INPUT_START;
	return usSInBuf[index];
}

#define _SET_BIT(REG, BIT)     ((REG) |= (BIT))
#define _CLEAR_BIT(REG, BIT)   ((REG) &= ~(BIT))
#define _READ_BIT(REG, BIT)    ((REG) & (BIT))
// coil reg
void mb_co_set(uint8_t d, uint16_t add){
	uint16_t _index;
	uint8_t _bit;
	MB_ASSERT(IS_MB_COIL_REG(add));
	_index = add - REG_COILS_START;
	_bit = 0x01 << (_index % 8);
	if(d){
		_SET_BIT(usSCoilBuf[_index/8],_bit);
	}else {
		_CLEAR_BIT(usSCoilBuf[_index/8],_bit);
	}
}
uint8_t mb_co_get(uint16_t add){
	uint16_t _index;
	MB_ASSERT(IS_MB_COIL_REG(add));

	_index = add - REG_COILS_START;
	return usSCoilBuf[_index] ? TRUE : FALSE;
}

// discrete reg
void mb_di_set(uint8_t d,uint16_t add){
	uint16_t _index;

	MB_ASSERT(IS_MB_DISCRETE_REG(add));
	_index = add - REG_DISCRETE_START;
	usSDiscInBuf[_index] = d ? TRUE : FALSE;
}
uint8_t mb_di_get(uint16_t add){
	uint16_t _index;

	MB_ASSERT(IS_MB_DISCRETE_REG(add));

	_index = add - REG_DISCRETE_START;
	return usSDiscInBuf[_index] ? TRUE : FALSE;
}

/****************************************************************************/

