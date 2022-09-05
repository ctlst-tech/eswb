#include "crc16-ccitt.h"

#define           CRC16_CCITT_POLY     0x1021
void crc16_ccitt_init (uint16_t *crc) {
	*crc = 0xffff;
}

void crc16_ccitt_update (uint16_t *crc, uint8_t data) {
    uint16_t i, v;
    volatile uint16_t xor_flag;

    /*
    Align test bit with leftmost bit of the message byte.
    */
    v = 0x80;

    for (i=0; i<8; i++) {
        if (*crc & 0x8000) {
            xor_flag= 1;
        } else {
            xor_flag= 0;
        }
        *crc = *crc << 1;

        if (data & v) {
            /*
            Append next bit of message to end of CRC if it is not zero.
            The zero bit placed there by the shift above need not be
            changed if the next bit of the message is zero.
            */
            *crc= *crc + 1;
        }

        if (xor_flag) {
            *crc = *crc ^ CRC16_CCITT_POLY;
        }

        /*
        Align test bit with next bit of the message byte.
        */
        v = v >> 1;
    } 
}

void crc16_ccitt_finalize (uint16_t *crc) {
	uint16_t i, xor_flag, crc_l = *crc;
	for (i=0; i<16; i++) {
		if (crc_l & 0x8000) {
		    xor_flag= 1;
		} else {
		    xor_flag= 0;
		}
		crc_l = crc_l << 1;

		if (xor_flag) {
		    crc_l = crc_l ^ CRC16_CCITT_POLY;
		} 
	}
	*crc = crc_l;
}
