#include <stdint.h>

void crc16_ccitt_init (uint16_t *crc);
void crc16_ccitt_update (uint16_t *crc, uint8_t data);
void crc16_ccitt_finalize (uint16_t *crc);
