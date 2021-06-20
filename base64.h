#ifndef __BASE64_H__
#define __BASE64_H__

#include <stdint.h>

void base64_encode(const uint8_t *data, int data_len, char **out);
void *base64_decode(const char *data, char **out);

#endif
