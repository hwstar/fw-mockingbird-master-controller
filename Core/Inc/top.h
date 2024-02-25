/*
 * top.h
 *
 *  Created on: Feb 20, 2024
 *      Author: srodgers
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "main.h"
#include "cmsis_os.h"

/* Functions which need C-compatiable linker names */
#ifdef __cplusplus
extern "C" {
#endif

extern void Top_init(void);
extern void Top_process_MF_frame(uint8_t buffer_number);
extern void Top_switch_task(void);
extern void Top_console_task(void);
extern void Top_send_I2S_Audio_Frame(uint8_t buffer_number);

#ifdef __cplusplus
}
#endif





