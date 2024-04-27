#include <vector>
#include "TS45.h"
#include "Log.h"
#include "RTUClient.h"

namespace TS45ToMQTT
{

RTUCallbackInterface* _cbi;

RTUClient::RTUClient()
{
    _rtu = new ModbusClientRTU(REDEPIN);
}

RTUClient::~RTUClient()
{
    delete _rtu;
}

// Define an onError handler function to receive error responses
// Arguments are the error code returned and a user-supplied token to identify the causing request
void handleError(Error error, uint32_t token) 
{
  // ModbusError wraps the error code and provides a readable error message for it
  ModbusError me(error);
  _cbi->handleError(me, token);
}

void handleData(ModbusMessage response, uint32_t token) 
{
    _cbi->handleData(response, token);
    return;
}

void RTUClient::begin(RTUCallbackInterface* pcb, unsigned long baud, uint32_t config, int8_t rxPin, int8_t txPin) { 
    _cbi = pcb; 
    // Set up Serial2 connected to Modbus RTU
  	RTUutils::prepareHardwareSerial(Serial2);
  	Serial2.begin(baud, config, rxPin, txPin);
    _rtu->onDataHandler(&handleData);
    // - provide onError handler function
    _rtu->onErrorHandler(&handleError);
    // Start ModbusRTU background task
    _rtu->begin(Serial2);
    while (!Serial2) {}
    logd("RTUClient ready");
}

void RTUClient::run(){
    Error err = _rtu->addRequest((uint32_t)READ_HOLD_TOKEN, 1, READ_HOLD_REGISTER, FIRST_REGISTER, NUM_VALUES);
    if (err!=SUCCESS) 
    {
        ModbusError e(err);
        loge("Error creating READ_HOLD_REGISTER request: %02X - %s\n", (int)e, (const char *)e);
    }
    err = _rtu->addRequest((uint32_t)READ_COIL_TOKEN, 1, READ_COIL, 0, 8);
    if (err!=SUCCESS) 
    {
        ModbusError e(err);
        loge("Error creating READ_COIL request: %02X - %s\n", (int)e, (const char *)e);
    }
}

void RTUClient::writeCoil(int coil, int val){
    Error err = _rtu->addRequest((uint32_t)WRITE_COIL_TOKEN, 1, WRITE_COIL, coil, val);
    if (err!=SUCCESS) 
    {
        ModbusError e(err);
        loge("Error creating WRITE_COIL request: %02X - %s\n", (int)e, (const char *)e);
    }
}

void RTUClient::deviceIdentification(){
    ModbusMessage m;
    Error rc = SUCCESS;
    uint8_t aob[3] = {0x0E, 0x01, 0x00};
    rc = m.setMessage(1, ENCAPSULATED_INTERFACE, (uint16_t)3, aob);
    Error err = _rtu->addRequest(m, (uint32_t)DEVICE_ID_TOKEN);
    if (err!=SUCCESS) 
    {
        ModbusError e(err);
        loge("Error creating request: %02X - %s\n", (int)e, (const char *)e);
    }
}

} // namespace TS45ToMQTT
