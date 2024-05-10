#pragma once
#include "Arduino.h"
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
	void readCoil(int coil);
	void writeCoil(int coil, int val);
	void deviceIdentification();
	void reset();

 protected:

    ModbusClientRTU* _rtu;
	int _toggle = 0;
};

} // namespace TS45ToMQTT
