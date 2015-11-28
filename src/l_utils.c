#include <string.h>
#include <stdlib.h>

#include "l_utils.h"

// http://www.cse.yorku.ca/~oz/hash.html
hash_t hash(const char *string)
{
	hash_t val = 538l;

	int c;
	while((c = *string++)){
		val = ((val << 5) + val) + c;
	}

	return val;
}
