#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "log.h"
#include "Enumerations.h"
#include "IOTCallbackInterface.h"
#include "Defines.h"
#include "RTUCallbackInterface.h"
#include "MQTTCommandInterface.h"
#include "RTUClient.h"

namespace TS45ToMQTT
{

class TS45 : public RTUCallbackInterface, public MQTTCommandInterface
{
    
public:
	TS45();
    ~TS45();
    void begin(IOTCallbackInterface* pcb);
    void run();
    
    //MQTTCommandInterface
    bool handleCommand(char *payload, size_t len);

    //RTUCallbackInterface
    void handleData(ModbusMessage msg, uint32_t token);
	void handleError(ModbusError error) {
        loge("Error response: %02X - %s\n", (int)error, (const char *)error);
    };

 protected:
    void PublishDiscovery();
    void PublishDiscoverySub(const char *component, const char *entityName, const char *jsonElement, const char *device_class, const char *unit_of_meas, const char *icon = "");

 	StaticJsonDocument<4096> _root;
    IOTCallbackInterface* _pcb;
    RTUClient* _rtuClient;

private:
    uint8_t lastRead[2048];
    bool _discoveryPublished = false;
    bool _mqttReadingsAvailable = false;
    bool _boilerPlateInfoPublished = false;
    bool _boilerPlateInfoRead = false;
    char _manufacturer[32];
    char _model[32];
    char _version[32];

};
} // namespace TS45ToMQTT