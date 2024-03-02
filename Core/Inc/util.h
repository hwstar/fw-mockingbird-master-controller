
#pragma once

#include "top.h"

namespace Util {

class Util {
public:
	char *strncpy_term(char *dest, const char *source, size_t len);

};



} /* End namespace Util */

extern Util::Util Utility;
