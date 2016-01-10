#ifndef __HAL_H
#define __HAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <avr/io.h>
#include <avr/eeprom.h>

#define ENTER_CRITICAL_SECTION()    asm volatile ( "in      __tmp_reg__, __SREG__" :: );    \
                                    asm volatile ( "cli" :: );                              \
                                    asm volatile ( "push    __tmp_reg__" :: )

#define LEAVE_CRITICAL_SECTION()    asm volatile ( "pop     __tmp_reg__" :: );              \
                                    asm volatile ( "out     __SREG__, __tmp_reg__" :: )

#define eeprom_init_hw()
#define eeprom_read(pBuf, Addr, Len)  eeprom_read_block((void *)pBuf, (const void *)Addr, (size_t)Len)
#define eeprom_write(pBuf, Addr, Len) eeprom_write_block((const void *)pBuf, (void *)Addr, (size_t)Len)

// AVR Architecture specifics.
#define portBYTE_ALIGNMENT          1
#define portPOINTER_SIZE_TYPE       uint16_t
#define configTOTAL_HEAP_SIZE       1024

//////////////////////////////////////////////////////////////
// DIO Section
#define DIO_PORT_POS                3
#define DIO_PORT_MASK               0x07
#define DIO_PORT_TYPE               uint8_t

// DIO Types
typedef enum
{
    DIO_MODE_IN_FLOAT   = 0,
    DIO_MODE_IN_PD      = 0,
    DIO_MODE_IN_PU      = 0x01,

    DIO_MODE_OUT_PP     = 0x08,
    
    DIO_MODE_AIN        = 0x18
}DIOmode_e;

uint8_t     hal_dio_base2pin(uint16_t base);
void        hal_dio_configure(uint8_t PortNr, uint8_t Mask, uint8_t Mode);
uint8_t     hal_dio_read(uint8_t PortNr);
void        hal_dio_set(uint8_t PortNr, uint8_t Mask);
void        hal_dio_reset(uint8_t PortNr, uint8_t Mask);

// DIO Section
//////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////
// AIN Section
uint8_t     hal_ain_apin2dio(uint8_t apin);
void        hal_ain_configure(uint8_t apin, uint8_t unused);
void        hal_ain_select(uint8_t apin, uint8_t unused);
int16_t     hal_ain_get(void);
// AIN Section
//////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif  //  __HAL_H