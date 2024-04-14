#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "log.h"
#include "Enumerations.h"
#include "IOTCallbackInterface.h"
#include "Defines.h"
#include "RTUCallbackInterface.h"
#include "RTUClient.h"
#include "ChargeControllerInfo.h"

namespace TS45ToMQTT
{

class TS45 : public RTUCallbackInterface
{
    
public:
	TS45();
    ~TS45();
    void begin(IOTCallbackInterface* pcb);
    void run();
    
    //RTUCallbackInterface
    void handleData(ModbusMessage msg, uint32_t token);
	void handleError(ModbusError error) {
        loge("Error response: %02X - %s\n", (int)error, (const char *)error);
    };

 protected:
    void PublishDiscovery();
 	StaticJsonDocument<4096> _root;
    IOTCallbackInterface* _pcb;
    RTUClient* _rtuClient;

private:
    uint8_t lastRead[2048];
    bool _discoveryPublished = false;
    bool _mqttReadingsAvailable = false;
    bool _boilerPlateInfoPublished = false;
    bool _boilerPlateInfoRead = false;
    ChargeControllerInfo _chargeControllerInfo;
};
} // namespace TS45ToMQTT