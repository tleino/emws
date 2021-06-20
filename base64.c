/*
 * Implements RFC4648
 */

#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char _alphabet[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void base64_encode(const uint8_t *data, int data_len, char **out)
{
	int data_i = 0;
	uint32_t value;
	*out = calloc(1, (data_len * 4 / 3) + 3 + 1);
	char *p = *out;

	while (data_i < data_len) {
		int padding = 0, i, j;

		/* Concatenate 3 input bytes to one 24 bit value */
		value = 0;
		for (i = 0; i < 3 && data_i < data_len; i++, data_i++) {
			value |= (uint32_t) (data[data_i] << (16 - (8 * i)));
		}

		/* If input data was not divisible by 3, calculate padding */
		padding = 3 - i;

		/* Divide the 24 bit value to 4 6-bit output bytes */
		for (j = 0; j < 4 - padding; j++) {
			uint8_t v = (value >> (18 - (6 * j)));
			*(p++) = _alphabet[v % 64];
		}
		/* Add padding */
		for (j = 0; j < padding; j++) {
			*(p++) = '=';
		}
	}
	*p = 0;
}

void *base64_decode(const char *data, char **out)
{
	int data_i = 0;
	int data_len = strlen(data);
	uint32_t value;
	*out = calloc(1, (data_len * 4 / 3) + 3 + 1);
	char *p = *out;

	while (data_i < data_len) {
		int i, j;

		/* Make a 24bit value from 4 input bytes */
		/* Extract 4 input bytes to one 24 bit value */
		value = 0;
		for (i = 0; i < 4 && data_i < data_len; i++, data_i++) {
			if (data[data_i] == '=') continue;
			int letterval = strchr(_alphabet, data[data_i]) - _alphabet;
			value |= (uint32_t) (letterval << (18 - (6 * i)));
		}

		/* Divide the 24 bit value to 3 8-bit output bytes */
		for (j = 0; j < 3; j++) {
			uint8_t v = (value >> (16 - (8 * j)));
			*(p++) = v;
		}
	}
	*p = 0;
	return out;
}
