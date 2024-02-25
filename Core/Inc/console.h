/*
 * console.h
 *
 *  Created on: Feb 21, 2024
 *      Author: srodgers
 */

#pragma once
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "top.h"
#include "logging.h"

namespace Console {

class Console {
public:
	void setup(void);
	void loop(void);
};

} /* End namespace console */




