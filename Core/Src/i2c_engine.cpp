#include "i2c_engine.h"
#include "logging.h"

namespace I2C_Engine {

const char *TAG = "i2c_engine";

const osMessageQueueAttr_t queue_I2C_transactions_attributes = {
  .name = "Queue_I2C_Transactions"
};


bool I2C_Engine::_check_i2c_message(I2C_Queue_Message *m, uint8_t expected_message) {
	if(m->type == expected_message) {
		return true;
	}
	else {
		switch(m->type) {
			case MSG_I2C_ERR:
				LOG_DEBUG(TAG, "I2C Transaction error. Error code: %d",1);
				break;

			default:
				LOG_DEBUG(TAG, "Got unexpected transaction message: %d, expecting: %d", m->type, expected_message );
				break;

		}
	}
	return false;
}
/*
 * Called before RTOS initialization
 */

void I2C_Engine::setup(void) {
	/* Definitions for Queue_I2C_Transactions */

	/* Create queue_I2C_transactions */
	this->_queue_i2c_transactions = osMessageQueueNew (I2C_TRANSACTION_QUEUE_DEPTH, sizeof(I2C_Transaction), &queue_I2C_transactions_attributes);
}

/*
 * This function is used to place an i2c transaction in the outgoing queue
 */


bool I2C_Engine::queue_transaction(uint8_t type, uint8_t bus, uint8_t device_address,
		uint8_t register_address, uint8_t data_length, uint8_t *register_data, void (*callback)(I2C_Transaction *trans), uint32_t trans_id) {
	I2C_Transaction trans;

	/*  Sanity check parameters */
	if((type >= I2CT_MAX_I2C_TYPES) || (device_address > 0x7F) ||
			(bus >= NUM_I2C_BUSSES) || (data_length > MAX_I2C_REG_DATA) || (!register_data) || (!callback)) {
		return false;
	}
	trans.id = trans_id;
	trans.type = type;
	trans.bus_num = bus;
	trans.device_address = device_address;
	trans.device_address8 = device_address << 1;
	trans.register_address = register_address;
	trans.data_length = data_length;
	trans.caller_register_data = register_data;
	trans.callback = callback;
	/* Copy data if type is write */
	if(trans.type == I2CT_WRITE_REG8) {
		/* We write the register address and the data in one DMA transfer */
		trans.local_register_data[0] = trans.register_address;
		/* Append the data */
		memcpy(trans.local_register_data + 1, trans.caller_register_data, trans.data_length);
	}

	/* Choose the correct I2C bus handle */
	switch(trans.bus_num) {
		case 0:
			trans.bus = &hi2c1;
			break;

		case 1:
			trans.bus = &hi2c2;
			break;

	}
	/* Queue Transaction */
	osStatus_t status;
	status = osMessageQueuePut(this->_queue_i2c_transactions, &trans, 0U, 0U );
	if(status == osOK)
		return true;
	else {
		LOG_ERROR(TAG, "I2C transaction put failed, os status: %d", (uint8_t) status);
		return false;
	}
}
/*
 * Called repeatedly after RTOS initialization
 */

void I2C_Engine::loop(void) {
	osStatus_t status;
	I2C_Queue_Message msg;
	int res;


	/* Process an I2C interrupt callback if there is one */
	if(osMessageQueueGetCount(Queue_I2C_BussesHandle)) {
		status = osMessageQueueGet(Queue_I2C_BussesHandle, &msg, NULL, osWaitForever);
		if (status == osOK) {
			this->_i2c_msg_ready = true;
		}
		else {
			LOG_ERROR(TAG, "osMessageQueGet() failed");
		}
	}

	switch(this->_state) {
		case I2CS_IDLE:
			/* Look for work */
			if (osMessageQueueGetCount(this->_queue_i2c_transactions)) {
				status = osMessageQueueGet(this->_queue_i2c_transactions, &this->trans, NULL, osWaitForever);
				if (status == osOK) {
					/* Decode Transaction Type */
					switch (this->trans.type) {
						case I2CT_READ_REG8:
							this->_state = I2CS_READ_REG;
							break;
						case I2CT_WRITE_REG8:
							this->_state = I2CS_WRITE_REG;
							break;
					}
				}
				else {
					LOG_ERROR(TAG, "osMessageQueGet() failed");
				}
			}
			break;


		case I2CS_READ_REG:  /* I2C register read */
			this->_i2c_msg_ready = false;
			/* Send write register address transaction */
			res = HAL_I2C_Master_Transmit_DMA(this->trans.bus, this->trans.device_address8, &this->trans.register_address, 1);
			if (res != HAL_OK) {
				LOG_ERROR(TAG, "HAL_I2C_Master_Transmit_DMA failed");
				this->trans.hal_i2c_error_code = this->trans.bus->ErrorCode;
				this->trans.status = I2CEC_DMA_FAILED;
				this->_state = I2CS_FINISH;
			}
			else { /* Write register address DMA was started */
				this->_state = I2CS_READ_REG_WAIT_REG_XMIT;
			}
			break;


		case I2CS_READ_REG_WAIT_REG_XMIT: /* Transmit register address */
			if (this->_i2c_msg_ready) {
				this->_i2c_msg_ready = false;
				if (this->_check_i2c_message(&msg, MSG_I2C_TX)) {
					/* Expected response. Get the register data */
					res = HAL_I2C_Master_Receive_DMA(this->trans.bus, this->trans.device_address8, this->trans.local_register_data, this->trans.data_length);
					if (res != HAL_OK) {
						LOG_ERROR(TAG, "HAL_I2C_Master_Receive_DMA failed");
						this->trans.hal_i2c_error_code = this->trans.bus->ErrorCode;
						this->trans.status = I2CEC_DMA_FAILED;
						this->_state = I2CS_FINISH;
					}
					else {
						this->_state = I2CS_READ_REG_WAIT_RCV;
					}

				}
				else { /* Unexpected response */
					this->trans.hal_i2c_error_code = this->trans.bus->ErrorCode;
					if (this->trans.hal_i2c_error_code == HAL_I2C_ERROR_AF) {
						this->trans.status = I2CEC_NO_DEVICE;
					}
					else {
						this->trans.status = I2CEC_TRANS_FAILED;
					}
					this->_state = I2CS_FINISH;
				}
			}

			break;


		case I2CS_READ_REG_WAIT_RCV: /* Wait for data to be received */
			if (this->_i2c_msg_ready) {
				this->_i2c_msg_ready = false;
				if (this->_check_i2c_message(&msg, MSG_I2C_RX)) { /* Expected response */
					LOG_DEBUG(TAG,"I2C Read Register Complete");
					this->trans.status = I2CEC_OK;
					this->_state = I2CS_FINISH;
					}
				else { /* Unexpected response */
					this->trans.hal_i2c_error_code = this->trans.bus->ErrorCode;
					this->trans.status = I2CEC_TRANS_FAILED;
					this->_state = I2CS_FINISH;
				}
			}
			break;


		case I2CS_WRITE_REG: /* I2C register write */
			this->_i2c_msg_ready = false;
			/* Send write register transaction */
			/* We write the address and the data as one DMA transfer */
			res = HAL_I2C_Master_Transmit_DMA(this->trans.bus, this->trans.device_address8, this->trans.local_register_data, this->trans.data_length + 1);
			if (res != HAL_OK) {
				LOG_ERROR(TAG, "HAL_I2C_Master_Transmit_DMA failed");
				this->trans.hal_i2c_error_code = this->trans.bus->ErrorCode;
				this->trans.status = I2CEC_DMA_FAILED;
				this->_state = I2CS_FINISH;
			}
			else {
				this->_state = I2CS_WRITE_REG_WAIT_DATA_XMIT;
			}
			break;


		case I2CS_WRITE_REG_WAIT_DATA_XMIT:
			if (this->_i2c_msg_ready) {
				this->_i2c_msg_ready = false;
				if (this->_check_i2c_message(&msg, MSG_I2C_TX)) { /* Expected response */
					LOG_DEBUG(TAG,"I2C Write Register Complete");
					this->trans.status = I2CEC_OK;
					this->_state = I2CS_FINISH;
					}
				else { /* Unexpected response */
					this->trans.hal_i2c_error_code = this->trans.bus->ErrorCode;
					if (this->trans.hal_i2c_error_code == HAL_I2C_ERROR_AF) {
						this->trans.status = I2CEC_NO_DEVICE;
					}
					else {
						this->trans.status = I2CEC_TRANS_FAILED;
					}
					this->_state = I2CS_FINISH;
				}
			}
			break;

		case I2CS_FINISH: /* Final steps */
			/* If OK and the command was a read */
			if((this->trans.status == I2CEC_OK) && (this->trans.type == I2CT_READ_REG8)) {
				/* Copy the read data to the user's buffer pointer */
				memcpy(this->trans.caller_register_data, this->trans.local_register_data, this->trans.data_length);
			}

			/* Call the user-supplied callback function */
			(*this->trans.callback)(&this->trans);
			/* Go back to Idle and look for more work */
			this->_state = I2CS_IDLE;
			break;


		default:
			this->_state = I2CS_IDLE;
			break;
	}
	/* Give other lower priority threads a chance to execute */
	status = osThreadYield();
}


} /* End Namespace I2C_Engine */
