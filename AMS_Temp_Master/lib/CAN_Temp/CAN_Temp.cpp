#include "CAN_Temp.hpp"

#include <SPI.h>
#define CAN_2515
#if defined(SEEED_WIO_TERMINAL) && defined(CAN_2518FD)
const int SPI_CS_PIN = BCM8;
const int CAN_INT_PIN = BCM25;
#else
const int SPI_CS_PIN = 9;
const int CAN_INT_PIN = 2;
#endif
#ifdef CAN_2518FD
#include "mcp2518fd_can.h"
mcp2518fd CAN(SPI_CS_PIN); // Set CS pin
#endif
#ifdef CAN_2515
#include "mcp2515_can.h"
mcp2515_can CAN(SPI_CS_PIN); // Set CS pin

CAN_Temp_MSG::CAN_Temp_MSG()
    : msg{0, 0, 0, 0, 0, 0, 0, 0}
{
}

CAN_Temp::CAN_Temp(const unsigned long id)
    : id(id)
{
    msg = new CAN_Temp_MSG();
}

unsigned char CAN_Temp::sendTempMsg(unsigned char printFlag)
{
    return 1;
}

#endif