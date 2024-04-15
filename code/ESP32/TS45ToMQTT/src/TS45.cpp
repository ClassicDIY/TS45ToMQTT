#include <vector>
#include "TS45.h"
#include "Log.h"

namespace TS45ToMQTT
{

TS45::TS45()
{
    _rtuClient = new RTUClient();
}

TS45::~TS45()
{
    delete _rtuClient;
}

#define ScalingFunction96(val) (int)(((val * 96.667)/32768)*100.0+0.5)/100.0
#define ScalingFunction139(val) (int)(((val * 139.15)/32768)*100.0+0.5)/100.0
#define ScalingFunction66(val) (int)(((val * 66.667)/32768)*100.0+0.5)/100.0
#define ScalingFunction316(val) (int)(((val * 316.67)/32768)*100.0+0.5)/100.0

void printHex(int num) {
    char tmp[16];
    sprintf(tmp, "%02X ", num);
    Serial.print(tmp);
}

byte Reverse_Bits(byte input)
{    
    char output = 0;
    for (byte mask = 1; mask > 0; mask <<= 1)
    {
        output <<= 1;
        if (input & mask)
            output |= 1;
    }
    return output;
}

void TS45::handleData(ModbusMessage response, uint32_t token) 
{
  if (token > 1111) {
    logd("Response: serverID=%d, FC=%d, Token=%08X, length=%d: \n", response.getServerID(), response.getFunctionCode(), token, response.size());
    //printHexString((char*)response.data(), response.size());
	char *ptr = (char*)response.data();
	for (int i = 0; i < response.size(); i++) {
		if (ptr[i] != lastRead[i]) {
			Serial.print("\033[1;31m");
			printHex(ptr[i]);
			Serial.print("\033[0m");
		} else {
			printHex(ptr[i]);
		}
		if (((i + 1) % 16) == 0){
			Serial.print("\n");
		}
	}
	Serial.print("\n");
	memcpy(lastRead, response.data(), response.size());

    _root.clear();
    uint16_t val = 0;
	response.get(19, val);
	_root["BatteryVoltage"] = ScalingFunction96(val);
    response.get(21, val);
	_root["BatterySenseVoltage"] = ScalingFunction96(val);
    response.get(23, val);
	_root["ArrayLoadVoltage"] = ScalingFunction139(val);
    response.get(25, val);
	_root["ChargeCurrent"] = ScalingFunction66(val);
    response.get(27, val);
	_root["LoadCurrent"] = ScalingFunction316(val);
    response.get(29, val);
	_root["BatteryVoltage_Slow"] = ScalingFunction96(val);
    response.get(31, val);
	_root["HeatsinkTemperature"] = val;
    response.get(33, val);
	_root["BatteryTemperature"] = val;
    response.get(35, val);
	_root["ChargeRegulatorReferenceVoltage"] = ScalingFunction96(val);
    uint16_t valLo = 0;
	response.get(37, val);
	response.get(39, valLo);
	_root["AmpHoursResetable"] = (val << 16 | valLo) * 0.1;
    response.get(41, val);
	response.get(43, valLo);
	_root["AmpHoursTotal"] = (val << 16 | valLo) * 0.1;
    response.get(45, val);
	response.get(47, valLo);
	_root["HourMeter"] = (val << 16 | valLo);
    response.get(61, val);
	response.get(49, valLo);
	_root["Alarm"] = (val << 16 | valLo);
    response.get(51, val);
	_root["Fault"] = val;
	response.get(53, val);
	uint8_t original = val & 0x00FF;
	_root["DipSwitch"] = Reverse_Bits(original);
    response.get(55, val);
	_root["ControlMode"] = val;
    response.get(57, val);
	_root["ControlState"] = val;
	response.get(59, val);
	if (val > 203) {
		val = 230;
	}
	_root["PWM"] = val * 100 / 230;

	String s;
	serializeJson(_root, s);
	_pcb->Publish("readings", s.c_str(), false);

	if (_boilerPlateInfoRead == false) {
        _root["deviceType"] = "TS-45";
		response.get(35, val);
        _root["nominalBatteryVoltage"] = ScalingFunction96(val);
		_boilerPlateInfoRead = true;
	}
	PublishDiscovery();
  }
}

void TS45::begin(IOTCallbackInterface* pcb) { 
    _pcb = pcb; 
    _rtuClient->begin(this, BAUDRATE, SERIAL_8N1, RXPIN, TXPIN);
    logd("TS45 ready");
}

void TS45::run() {
    _rtuClient->run();
}

void TS45::PublishDiscovery()
{
    if (!_discoveryPublished) {
        logd("Publishing discovery for TS45");
        PublishDiscoverySub("sensor", "BatteryVoltage", "BatteryVoltage", "voltage", "V", "mdi:lightning-bolt");
        PublishDiscoverySub("sensor", "ChargeCurrent", "ChargeCurrent", "current", "A", "mdi:current-dc");
		PublishDiscoverySub("sensor", "HeatsinkTemperature", "HeatsinkTemperature", "temperature", "°C", "mdi:thermometer");
		PublishDiscoverySub("sensor", "BatteryTemperature", "BatteryTemperature", "temperature", "°C", "mdi:thermometer");
        // PublishDiscoverySub("sensor", "SOC", "SOC", "battery", "%");
        // PublishDiscoverySub("sensor", "RemainingCapacity", "RemainingCapacity", "current", "Ah", "mdi:ev-station");
        // PublishTempsDiscovery();
        // PublishCellsDiscovery();
        _discoveryPublished = true;
    }
}

void TS45::PublishDiscoverySub(const char *component, const char *entity, const char *jsonElement, const char *device_class, const char *unit_of_meas, const char *icon)
{
	char buffer[STR_LEN];
	StaticJsonDocument<1024> doc; // MQTT discovery
	doc["device_class"] = device_class;
	doc["unit_of_measurement"] = unit_of_meas;
	doc["state_class"] = "measurement";

	doc["name"] = entity;
	if (strlen(icon) > 0) {
		doc["icon"] = icon;
	}

	sprintf(buffer, "%s/stat/readings", _pcb->getRootTopicPrefix().c_str());
	doc["state_topic"] = buffer;

	sprintf(buffer, "ESP%X_%s", _pcb->getUniqueId(), entity);
	doc["unique_id"] = buffer;
	String object_id = buffer;

	sprintf(buffer, "{{ value_json.%s }}", jsonElement);
	doc["value_template"] = buffer;

	sprintf(buffer, "%s/tele/LWT", _pcb->getRootTopicPrefix().c_str());
	doc["availability_topic"] = buffer;
    doc["pl_avail"] = "Online";
    doc["pl_not_avail"] = "Offline";
	JsonObject device = doc.createNestedObject("device");
	device["name"] = _pcb->getDeviceName();
	device["via_device"] = _pcb->getThingName();
	device["sw_version"] = CONFIG_VERSION;
	device["manufacturer"] = "ClassicDIY";
	device["model"] = "TS-45";
	sprintf(buffer, "%X_%s", _pcb->getUniqueId(), _pcb->getDeviceName().c_str());
	device["identifiers"] = buffer;
	sprintf(buffer, "%s/%s/%s/config", HOME_ASSISTANT_PREFIX, component, object_id.c_str());
	_pcb->PublishMessage(buffer, doc);
}

} // namespace TS45ToMQTT