/*
Copyright (c) 2011-2013 <comparator@gmx.de>

This file is part of the X13.Home project.
http://X13home.github.com

BSD New License
See LICENSE.txt file for license details.
*/

// TWI Driver Silicon Image Si7005, Temperature & Humidity

// Outs
// TW16384      Temperature counter (TC)
// TW16385      Relative Humidity Counter(RH)
// Temp�C  = (TC/32) - 50
//  A/32-50
// RH % = (RH/16) - 24
//  A/16-24
// RH_lin = RH-(RH^2*-0.00393 + RH*0.4008 - 4.7844)
//  A+(A*A*0.00393-A*0.4008+4.7844)
// RH_Compens = RH_lin + (Temp(�C) - 30)*(RH_lin*0.0237 + 0,1973)
//  A+(B-30)*(A*0.0237+0.1973)

#define SI7005_ADDR                 0x40

// Si7005 Registers
#define SI7005_REG_STATUS           0x00
#define SI7005_REG_DATA             0x01
#define SI7005_REG_CONFIG           0x03
#define SI7005_REG_ID               0x11

// Status Register
#define SI7005_STATUS_NOT_READY     0x01

// Config Register
#define SI7005_CONFIG_START         0x01
#define SI7005_CONFIG_HEAT          0x02
#define SI7005_CONFIG_HUMIDITY      0x00
#define SI7005_CONFIG_TEMPERATURE   0x10
#define SI7005_CONFIG_FAST          0x20

// ID Register
#define SI7005_ID_SI7005            0x50

#define SI7005_T_MIN_DELTA          5
#define SI7005_H_MIN_DELTA          8

// Process variables
static uint8_t  si7005_stat;
static uint16_t si7005_oldTemp;
static uint16_t si7005_oldHumi;
uint16_t        si7005_tmp;

void si7005_ReadStatus_cb(void)
{
  if(si7005_tmp & SI7005_STATUS_NOT_READY)
    si7005_stat--;
  else
  {
    si7005_tmp = SI7005_REG_DATA;
    twimExch_ISR(SI7005_ADDR, (TWIM_BUSY | TWIM_WRITE | TWIM_READ), 1, 2, 
                                (uint8_t *)&si7005_tmp, NULL);
    si7005_stat++;
  }
}

static uint8_t twi_SI7005_Read(subidx_t * pSubidx, uint8_t *pLen, uint8_t *pBuf)
{
    *pLen = 2;
    if(pSubidx->Base & 1)   // Read Humidity Counter
        *(uint16_t *)pBuf = si7005_oldHumi;
    else                    // Read Temperature counter TC
        *(uint16_t *)pBuf = si7005_oldTemp;

    return MQTTS_RET_ACCEPTED;
}

static uint8_t twi_SI7005_Write(subidx_t * pSubidx, uint8_t Len, uint8_t *pBuf)
{
    if(pSubidx->Base & 1)   // Renew Temperature
        si7005_oldHumi = *(uint16_t *)pBuf;
    else                    // Renew Humidity
        si7005_oldTemp = *(uint16_t *)pBuf;
    return MQTTS_RET_ACCEPTED;
}

static uint8_t twi_SI7005_Pool1(subidx_t * pSubidx)
{
  if(si7005_stat == 0)
  {
    if(twim_access & (TWIM_BUSY | TWIM_ERROR | TWIM_RELEASE | TWIM_READ | TWIM_WRITE))
      return 0;
    si7005_stat = 1;
  }
  else
  {
    if(twim_access & (TWIM_ERROR | TWIM_RELEASE))   // Bus Error, or request to release bus
    {
      si7005_stat = 0x40;
      if(twim_access & TWIM_RELEASE)
        twim_access = TWIM_RELEASE;
      return 0;
    }

    if(twim_access & (TWIM_READ | TWIM_WRITE))      // Bus Busy
      return 0;
  }

  switch(si7005_stat)
  {
    case 1:             // Start Dummy Conversion
      si7005_tmp = (SI7005_REG_CONFIG | ((SI7005_CONFIG_START | SI7005_CONFIG_TEMPERATURE | SI7005_CONFIG_FAST)<<8));
      twimExch_ISR(SI7005_ADDR, (TWIM_BUSY | TWIM_WRITE), 2, 0, (uint8_t *)&si7005_tmp, NULL);
      break;
    case 3:             // Start Conversion, Humidity
      si7005_tmp = (SI7005_REG_CONFIG | ((SI7005_CONFIG_START | SI7005_CONFIG_HUMIDITY)<<8));
      twimExch_ISR(SI7005_ADDR, (TWIM_BUSY | TWIM_WRITE), 2, 0, (uint8_t *)&si7005_tmp, NULL);
      break;
    // !! Conversion Time 35mS - Normal / 18 mS - Fast
    case 6:             // Read Busy Flag
    case 12:
      si7005_tmp = SI7005_REG_STATUS;
      twimExch_ISR(SI7005_ADDR, (TWIM_BUSY | TWIM_WRITE | TWIM_READ), 1, 1, 
          (uint8_t *)&si7005_tmp, &si7005_ReadStatus_cb);
      break;
    case 7:
    case 13:
    case 14:
      return 0;
    case 8:             // Read Humidity
      si7005_stat++;
      si7005_tmp = ((si7005_tmp & 0xFF)<<4) | (si7005_tmp>>12);
      if((si7005_tmp > si7005_oldHumi ? si7005_tmp - si7005_oldHumi : si7005_oldHumi - si7005_tmp)
                                                                             > SI7005_H_MIN_DELTA)
      {
        si7005_oldHumi = si7005_tmp;
        return 1;
      }
      return 0;
    case 9:             // Start Conversion Temprature
      si7005_tmp = (SI7005_REG_CONFIG | ((SI7005_CONFIG_START | SI7005_CONFIG_TEMPERATURE)<<8));
      twimExch_ISR(SI7005_ADDR, (TWIM_BUSY | TWIM_WRITE), 2, 0, (uint8_t *)&si7005_tmp, NULL);
      break;
  }
  si7005_stat++;

  return 0;
}

static uint8_t twi_SI7005_Pool2(subidx_t * pSubidx)
{
  if(si7005_stat == 14)
  {
    si7005_stat++;
    twim_access = 0;        // Bus Free
    
    si7005_tmp = ((si7005_tmp & 0xFF)<<6) | (si7005_tmp>>10);
      if((si7005_tmp > si7005_oldTemp ? si7005_tmp - si7005_oldTemp : si7005_oldTemp - si7005_tmp)
                                                                             > SI7005_T_MIN_DELTA)
    {
      si7005_oldTemp = si7005_tmp;
      return 1;
    }
  }
  return 0;
}

static uint8_t twi_SI7005_Config(void)
{
    uint8_t reg = SI7005_REG_ID;
    if((twimExch(SI7005_ADDR, (TWIM_READ | TWIM_WRITE), 1, 1, &reg) != 
                                         TW_SUCCESS) ||     // Communication error
       (reg != SI7005_ID_SI7005))                           // Bad device ID
        return 0;

    si7005_stat = 0;
    si7005_oldTemp = 0;
    si7005_oldHumi = 0;

    // Register variable 1, Temperature 0,1�C
    indextable_t * pIndex1;
    pIndex1 = getFreeIdxOD();
    if(pIndex1 == NULL)
        return 0;

    pIndex1->cbRead  =  &twi_SI7005_Read;
    pIndex1->cbWrite =  &twi_SI7005_Write;
    pIndex1->cbPool  =  &twi_SI7005_Pool1;
    pIndex1->sidx.Place = objTWI;                   // Object TWI
    pIndex1->sidx.Type =  objUInt16;                // Variables Type -  UInt16
    pIndex1->sidx.Base = (SI7005_ADDR<<8) + 1;      // Device addr

    // Register variable 2, Humidity - uncompensated
    indextable_t * pIndex2;
    pIndex2 = getFreeIdxOD();
    if(pIndex2 == NULL)
    {
        pIndex1->Index = 0xFFFF;                    // Free Index
        return 0;
    }

    pIndex2->cbRead  =  &twi_SI7005_Read;
    pIndex2->cbWrite =  &twi_SI7005_Write;
    pIndex2->cbPool  =  &twi_SI7005_Pool2;
    pIndex2->sidx.Place = objTWI;                   // Object TWI
    pIndex2->sidx.Type =  objUInt16;
    pIndex2->sidx.Base = (SI7005_ADDR<<8);          // Device addr

    return 2;
}