#pragma once
#include <Arduino.h>
#include "ModbusClientRTU.h"

class RTUCallbackInterface
{
public:
    virtual void handleData(ModbusMessage msg, uint32_t token) = 0;
	virtual void handleError(ModbusError error, uint32_t token) = 0;
};