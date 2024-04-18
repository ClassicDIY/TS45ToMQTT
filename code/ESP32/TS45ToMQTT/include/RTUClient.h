#pragma once
#include "Arduino.h"
#include "Enumerations.h"
#include "ModbusClientRTU.h"
#include "RTUCallbackInterface.h"

namespace TS45ToMQTT
{

class RTUClient
{
 public:
	RTUClient();
	~RTUClient();
	void begin(RTUCallbackInterface* cbi, unsigned long baud, uint32_t config, int8_t rxPin, int8_t txPin);
	void run();
	void writeCoil(int coil, int val);
	void deviceIdentification();

 protected:

    ModbusClientRTU* _rtu;
};

} // namespace TS45ToMQTT
