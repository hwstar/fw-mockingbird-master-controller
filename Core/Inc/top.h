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


const uint8_t MF_KP = 0x0a;
const uint8_t MF_ST = 0x0b;
const uint8_t MF_STP = 0x0c;
const uint8_t MF_ST2P = 0x0d;
const uint8_t MF_ST3P = 0x0e;

/* Functions which need C-compatiable linker names */
#ifdef __cplusplus
extern "C" {
#endif

extern void Top_init(void);
extern void Top_process_MF_frame(uint8_t buffer_number);
extern void Top_switch_task(void);
extern void Top_console_task(void);
extern void Top_i2c_task(void);
extern void Top_send_I2S_Audio_Frame(uint8_t buffer_number);
extern void Top_Int_Handler_Uart6(void);

#ifdef __cplusplus
}
#endif





