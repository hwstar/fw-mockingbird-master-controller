#pragma once
#include "top.h"

namespace I2C_Engine {

enum {I2CS_IDLE=0, I2CS_READ_REG, I2CS_READ_REG_WAIT_REG_XMIT, I2CS_READ_REG_WAIT_RCV, I2CS_WRITE_REG,
	I2CS_WRITE_REG_WAIT_REG_XMIT, I2CS_WRITE_REG_WAIT_DATA_XMIT, I2CS_FINISH};

enum {I2CT_READ_REG8=0, I2CT_WRITE_REG8, I2CT_MAX_I2C_TYPES};
enum {I2CEC_OK=0, I2CEC_NO_DEVICE, I2CEC_TRANS_FAILED, I2CEC_DMA_FAILED};

const uint8_t NUM_I2C_BUSSES = 2;
const uint8_t MAX_I2C_REG_DATA = 8;
const uint8_t I2C_TRANSACTION_QUEUE_DEPTH = 16;


typedef struct I2C_Transaction {
	uint32_t hal_i2c_error_code;
	uint32_t id;
	uint8_t status;
	uint8_t type;
	uint8_t bus_num;
	uint8_t device_address;
	uint8_t device_address8;
	uint8_t register_address;
	uint8_t data_length;
	uint8_t *caller_register_data;
	void (*callback)(I2C_Transaction *trans);
	I2C_HandleTypeDef *bus;
	uint8_t local_register_data[MAX_I2C_REG_DATA+1]; // One more for write case to store register address
} I2C_Transaction;



class I2C_Engine {
public:
	void setup(void);
	void loop(void);
	bool queue_transaction(uint8_t type, uint8_t bus, uint8_t device_address,
			uint8_t register_address, uint8_t data_length, uint8_t *register_data, void (*callback)(I2C_Transaction *trans), uint32_t trans_id);
protected:
	bool _check_i2c_message(I2C_Queue_Message *m, uint8_t expected_message);
	bool _i2c_msg_ready;
	uint8_t _state;
	I2C_Transaction trans;
	osMessageQueueId_t _queue_i2c_transactions;
};

} /* End namespace I2C_Engine */
