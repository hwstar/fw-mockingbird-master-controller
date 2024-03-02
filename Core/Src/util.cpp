
#include "top.h"
#include "util.h"
#include <string.h>

namespace Util {
/*
 * Strncpy with guaranteed termination
 */

char *Util::strncpy_term(char *dest, const char *source, size_t len) {
	if((!dest) || (!source)) {
		return NULL;
	}
	char *res = strncpy(dest, source, len);
	dest[len-1] = 0;
	return res;
}


} // End namespace Util
