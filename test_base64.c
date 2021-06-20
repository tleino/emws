#include "base64.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char **argv)
{
	const char teststrs[4][20] = {
		"Man",
		"Mann",
		"Mannn",
		"Hello world"
	};
	char *out = 0, *decoded = 0;
	int i;
	for (i = 0; i < 4; i++) {
		base64_encode(teststrs[i], strlen(teststrs[i]), &out);
		base64_decode(out, &decoded);
		printf("%-16s\t%-16s\t%-16s\n", teststrs[i], out, decoded);
		assert(!strcmp(teststrs[i], decoded));
		free(out);
		free(decoded);
	}
}

