    /*
 ** SidPi Server
 */

#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#include "pico/multicore.h"
#include "sid_server.h"
#include "hardware/pio.h"
#include "sidpio.pio.h"

unsigned int dataWritePos = 0;
unsigned int dataReadPos = 0;
long inputClock = 0;
int sidfd;
uint8_t *dataRead, *dataWrite;



size_t return_ok(uint8_t *dataWrite) {
	dataWrite[0] = OK;
	return 1;
}
size_t return_err(uint8_t *dataWrite) {
	dataWrite[0] = ERR;
	return 1;
}
size_t processReadBuffer(sid_server_t *state) {
    size_t dataReadPos = state->recv_len, dataWritePos = 0;
	dataRead = state->buffer_recv;
	dataWrite = state->buffer_sent;
	
    uint8_t command, sidNumber;
    uint16_t dataLength;

	command = dataRead[dataReadPos++];
	sidNumber = dataRead[dataReadPos++];

	dataLength = (dataRead[dataReadPos++] << 8) | dataRead[dataReadPos++];
	
	switch (command) {
	case TRY_DELAY: {
		dataReadPos +=2;
		dataWritePos += return_ok(dataWrite);
		break;
	}
	case TRY_WRITE: {
		if (dataLength < 4 && (dataLength % 4) != 0) {
			printf("We have a problem\n");
			break;
		}
		if(multicore_fifo_wready()) {
		//printf("Sid write : %02x %02x cycles %02x\n", dataRead[dataReadPos], dataRead[dataReadPos+1], (dataRead[dataReadPos+2] << 8) | dataRead[dataReadPos+3]);
			for(int i = 0; i < dataLength; i+=4) {
				multicore_fifo_push_blocking((dataRead[dataReadPos++] << 24) | (dataRead[dataReadPos++] << 16) | (dataRead[dataReadPos++] << 8) | dataRead[dataReadPos++]);
			}	
			dataWritePos += return_ok(dataWrite);
		} else{
			dataWrite[dataWritePos++] = BUSY;
		}
		break;
	}
	case GET_VERSION: {
		dataWrite[dataWritePos++] = VERSION;
		dataWrite[dataWritePos++] = SID_NETWORK_PROTOCOL_VERSION;
		break;
	}
	case GET_CONFIG_INFO: {
		dataWrite[dataWritePos++] = INFO;
		dataWrite[dataWritePos++] = 0;
		strcpy(dataWrite + dataWritePos,SID_PI);
		dataWritePos += sizeof SID_PI;
		//dataWrite[dataWritePos++] = 0;
		break;
	}
	case FLUSH: {
		multicore_fifo_drain();
		dataWritePos += return_ok(dataWrite);
		break;
	}
	case TRY_SET_SID_COUNT: {
		dataWritePos += return_ok(dataWrite);
		break;
	}
	case MUTE: {
		dataReadPos += 2;
		dataWritePos += return_ok(dataWrite);
		break;
	}
	case TRY_RESET: {
		dataReadPos ++;
		dataWritePos += return_ok(dataWrite);
		break;
	}
	case TRY_READ: {
		dataReadPos ++;
		dataWritePos += return_ok(dataWrite);
		break;
	}
	case TRY_SET_SAMPLING: {
		dataReadPos ++;
		dataWritePos += return_ok(dataWrite);
		break;
	}
	case TRY_SET_CLOCKING: {
		dataReadPos ++;
		dataWritePos += return_ok(dataWrite);
		break;
	}
	case GET_CONFIG_COUNT: {
		dataWrite[dataWritePos++] = COUNT;
		dataWrite[dataWritePos++] = 1;
		break;
	}
	case SET_SID_POSITION: {
		dataReadPos ++;
		dataWritePos += return_ok(dataWrite);
		break;
	}
	case SET_SID_LEVEL: {
		dataReadPos ++;
		dataWritePos += return_ok(dataWrite);
		break;
	}
	case TRY_SET_SID_MODEL: {
		dataReadPos ++;
		dataWritePos += return_ok(dataWrite);
		break;
	}
	default:
		dataWritePos += return_err(dataWrite);
	}
	
	state->sent_len = dataWritePos;
	state->recv_len = 0;

	return dataReadPos;	
}

