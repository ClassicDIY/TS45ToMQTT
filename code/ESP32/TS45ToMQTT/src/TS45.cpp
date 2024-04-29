#include <vector>
#include "TS45.h"
#include "Log.h"
#include "ModbusMessage.h"

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

void TS45::handleError(ModbusError error, uint32_t token) 
{
	loge("RTU Error for token %d: %02X - %s", token, (int)error, (const char *)error);
	if (token == DEVICE_ID_TOKEN) {
		_rtuDeviceIDErrorCount++;
		if (_rtuDeviceIDErrorCount == RTU_ERROR_RETRY_LIMIT) { // give uo trying to get device ID, move on...
			_deviceInfoInfoRead = true;
		}
	}
}

std::vector<std::string> alarmLookup = {
        "RTS open",
		"RTS shorted",
		"RTS disconnected",
		"Ths disconnected",
		"Ths shorted",
		"TriStar hot",
		"Current limit",
		"Current offset",
		"Battery Sense",
		"Batt Sense disc",
		"Uncalibrated",
		"RTS miswire",
		"HVD",
		"high d",
		"miswire",
		"FET open",
		"P12",
		"Load Disc",
		"Alarm 19",
		"Alarm 20",
		"Alarm 21",
		"Alarm 22",
		"Alarm 23",
		"Alarm 24"
  };

std::vector<std::string> faultLookup = {
        "External Short",
		"Overcurrent",
		"FET short",
		"Software",
		"HVD",
		"TriStar hot",
		"DIP sw changed",
		"Setting edit",
		"reset?",
		"Miswire",
		"RTS shorted",
		"RTS disconnected",
		"Fault 12",
		"Fault 13",
		"Fault 14",
		"Fault 15"
  };

std::vector<std::string> modeLookup = {
	"Charge",
	"Load",
	"Diversion",
	"Lighting"
};

std::vector<std::string> stateLookup_chargeDiversion = {
	"Start",
	"Night Check",
	"Disconnect",
	"Night",
	"Fault",
	"Bulk",
	"PWM",
	"Float",
	"Equalize"
};

std::vector<std::string> stateLookup_loadLighting = {
	"Start",
	"Normal",
	"LVD Warn",
	"LVD",
	"Fault",
	"Disconnect",
	"Normal Off",
	"Override LVD"
};

void TS45::handleData(ModbusMessage response, uint32_t token) 
{
	logd("handleData:Token %d", token);
	_lastModbusResponseTimeStamp =  millis();
	// logd("Response: serverID=%d, FC=%d, Token=%08X, length=%d: \n", response.getServerID(), response.getFunctionCode(), token, response.size());
	JsonDocument doc;
	switch (token) {
		case DEVICE_ID_TOKEN: //Read Device Identification
		{
			auto it = response.begin();
			if (response.size() > 9) {
				it+=9;
				uint8_t s1 = *it++;
				int i = 0;
				for (i = 0; i < s1; i++) {
					_manufacturer[i] = *it++;
				}
				_manufacturer[i] = '\0';
				it++;
				uint8_t s2 = *it++;
				for (i = 0; i < s2; i++) {
					_model[i] = *it++;
				}
				_model[i] = '\0';
				it++;
				uint8_t s3 = *it++;
				for (i = 0; i < s3; i++) {
					_version[i] = *it++;
				}
				_version[i] = '\0';
				_deviceInfoInfoRead = true;
			}
			break;
		}
		case READ_COIL_TOKEN:
			// logd("Response: serverID=%d, FC=%d, Token=%08X, length=%d: \n", response.getServerID(), response.getFunctionCode(), token, response.size());
			// printHexString((char*)response.data(), response.size());
			if (response.size() >= 4) {
				if (response.data()[3] != _lastCoilRead) { 
					doc["equalize"] = response.data()[3] & 0x01 ? true : false;
					doc["disconnect"] = response.data()[3] & 0x02 ? true : false;
					String info;
					serializeJson(doc, info);
					_pcb->Publish("ctrl", info.c_str(), false);
					_lastCoilRead = response.data()[3];
				}
			}
			break;
  		case READ_HOLD_TOKEN:
			#ifdef MODBUS_LOG // modbus logging enabled?
				printHexString((char*)response.data(), response.size());
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
			#endif
			uint16_t val = 0;
			if (_deviceInfoInfoRead == true && _boilerPlateInfoRead == false) {
				doc["manufacturer"] = _manufacturer;
				doc["model"] = _model;
				doc["version"] = _version;
				response.get(35, val);
				doc["ChargeRegulatorReferenceVoltage"] = ScalingFunction96(val);
				response.get(53, val);
				uint8_t original = val & 0x00FF;
				String dips;
				for (unsigned int i = 0; i < 8; ++i)
				{
					unsigned int test_bit = 1LL << i;
					dips += (original & test_bit) ? "On" : "Off";
					if (i < 7) { dips += "|"; }
				}
				doc["DipSwitch"] = dips;
				response.get(55, val);
				_controlMode = val;
				doc["ControlModeCode"] = val;
				if (val < modeLookup.size()) {
					doc["ControlMode"] = modeLookup[val];
				}
				if (!_discoveryPublished) {
					PublishDiscovery();
				} else {
					String info;
					serializeJson(doc, info);
					_pcb->Publish("info", info.c_str(), false);
					_boilerPlateInfoRead = true;
				}
			}
						
			doc.clear();
			response.get(19, val);
			doc["BatteryVoltage"] = ScalingFunction96(val);
			response.get(21, val);
			doc["BatterySenseVoltage"] = ScalingFunction96(val);
			response.get(23, val);
			doc["ArrayLoadVoltage"] = ScalingFunction139(val);
			response.get(25, val);
			doc["ChargeCurrent"] = ScalingFunction66(val);
			response.get(27, val);
			doc["LoadCurrent"] = ScalingFunction316(val);
			response.get(29, val);
			doc["BatteryVoltage_Slow"] = ScalingFunction96(val);
			response.get(31, val);
			doc["HeatsinkTemperature"] = val;
			response.get(33, val);
			doc["BatteryTemperature"] = val;
			uint16_t valLo = 0;
			response.get(37, val);
			response.get(39, valLo);
			doc["AmpHoursResetable"] = (val << 16 | valLo) * 0.1;
			response.get(41, val);
			response.get(43, valLo);
			doc["AmpHoursTotal"] = (val << 16 | valLo) * 0.1;
			response.get(45, val);
			response.get(47, valLo);
			doc["HourMeter"] = (val << 16 | valLo);
			response.get(61, val);
			response.get(49, valLo);
			uint32_t alarmCode = val << 16 | valLo;
			doc["AlarmCode"] = alarmCode;
			std::string alarms;
			int ones = 0;
			int count = alarmLookup.size();
			for (unsigned int i = 0; i < count; ++i)
			{
				uint32_t test_bit = 1LL << i;
				if (alarmCode & test_bit) {
					if ((ones > 0) && (i < count)) { alarms += "|"; } 
					ones++;
					alarms += alarmLookup[i];
				}
			}
			doc["Alarms"] = alarms.size() == 0 ? "None" : alarms;
			response.get(51, val);
			doc["FaultCode"] = val;
			std::string faults = "";
			ones = 0;
			count = faultLookup.size();
			for (unsigned int i = 0; i < count; ++i)
			{
				unsigned int test_bit = 1LL << i;
				if (val & test_bit) {
					if ((ones++ > 0) && (i < count)) { faults += "|"; } 
					faults += faultLookup[i];
				}
			}
			doc["Faults"] = faults.size() == 0 ? "None" : faults;
			response.get(57, val);
			doc["ControlStateCode"] = val;
			if (_controlMode == 0x00 || _controlMode == 0x02) {
				if (val < stateLookup_chargeDiversion.size()) {
					doc["ControlState"] = stateLookup_chargeDiversion[val];
				}
			} else {
				if (val < stateLookup_loadLighting.size()) {
					doc["ControlState"] = stateLookup_loadLighting[val];
				}				
			}
			response.get(59, val);
			if (val > 203) {
				val = 230;
			}
			doc["PWM"] = val * 100 / 230;
			String readings;
			serializeJson(doc, readings);
			_pcb->Publish("readings", readings.c_str(), false);
			break;
	}
}

const std::map<std::string, int> commandLookup = {
        {{"equalize"}, 0},
        {{"disconnect"}, 1},
        {{"clearAhTotal"}, 16},
		{{"clearAhResetable"}, 17},
		{{"clearkWh"}, 18},
		{{"resetServiceReminder"}, 19},
		{{"clearFaults"}, 20},
		{{"clearAlarms"}, 21},
		{{"reboot"}, 255},
		{{"end"}, -1}
  };

  int noteValue (const std::string& s) {
    auto found = commandLookup . find (s);
    return found == commandLookup . end () ? -1 : found -> second;
  }

bool TS45::handleCommand(char *payload, size_t len) 
{
	bool rVal = false;
	int coil = -1;
	int val = 0xFF00;

	JsonDocument doc;
	DeserializationError err = deserializeJson(doc, payload);
	if (err) { // not json? look for simple string
		int i = 0;
		char buf[64];
		for (; i < len; i++) {
			buf[i] = payload[i];
		}
		buf[i] = '\0';
		coil = noteValue(buf);
	}
	else {
		JsonObject documentRoot = doc.as<JsonObject>();
		for (JsonPair keyValue : documentRoot) {
			coil = noteValue(keyValue.key().c_str());
			val = keyValue.value().as<int>() == 0x00 ? 0x00 : 0xFF00 ;
			break; // use first element
		}
	}
	if (coil != -1) {
		logd("Processing MQTT command coil: %d val: %02X", coil, val);
		_rtuClient->writeCoil(coil, val);
		rVal = true;
	}
	return rVal;
}

void TS45::begin(IOTCallbackInterface* pcb) { 
    _pcb = pcb; 
    _rtuClient->begin(this, BAUDRATE, SERIAL_8N2, RXPIN, TXPIN);
	_rtuDeviceIDErrorCount = 0;
    logd("TS45 ready");
}

void TS45::run() {
	if (_deviceInfoInfoRead == false) {
		_rtuClient->deviceIdentification();
	} else {
    	_rtuClient->run();
	}
	_pcb->PublishTelemetery((_lastModbusResponseTimeStamp + GoOfflineAfterNoRTUResponse) > millis());
}

void TS45::PublishDiscovery()
{
	logd("Publishing discovery for TS45");
	PublishEntity("readings", "voltage", "measurement", "BatteryVoltage", "V", "mdi:lightning-bolt");
	PublishEntity("readings", "voltage", "measurement", "ArrayLoadVoltage", "V", "mdi:lightning-bolt");
	PublishEntity("readings", "current", "measurement", "ChargeCurrent", "A", "mdi:current-dc");
	PublishEntity("readings", "current", "measurement", "LoadCurrent", "A", "mdi:current-dc");
	PublishEntity("readings", "temperature", "measurement", "HeatsinkTemperature", "°C", "mdi:thermometer");
	PublishEntity("readings", "temperature", "measurement", "BatteryTemperature", "°C", "mdi:thermometer");
	PublishEntity("readings", "duration", "", "HourMeter", "H", "mdi:timer");
	PublishEntity("readings", "power_factor", "measurement", "PWM", "%", "mdi:file-percent-outline");
	PublishEntity("info", "enum", "", "ControlMode");
	PublishEntity("info", "enum", "", "DipSwitch");
	PublishEntity("info", "voltage", "measurement", "ChargeRegulatorReferenceVoltage", "V", "mdi:lightning-bolt");
	PublishEntity("info", "enum", "", "version");
	PublishEntity("readings", "enum", "", "ControlState");
	PublishEntity("readings", "ENERGY", "total_increasing", "AmpHoursResetable", "aH", "mdi:meter-electric-outline"); 
	PublishEntity("readings", "ENERGY", "total_increasing", "AmpHoursTotal", "aH", "mdi:meter-electric-outline");
	PublishEntity("readings", "enum", "", "Alarms");
	PublishEntity("readings", "enum", "", "Faults");

	_discoveryPublished = true;
}

void TS45::PublishEntity(const char *subtopic, const char *device_class, const char *state_class, const char *entity, const char *unit_of_meas, const char *icon)
{
	char buffer[STR_LEN];
	JsonDocument doc; // MQTT discovery
	doc["device_class"] = device_class;
	if (strlen(state_class) > 0) {
		doc["state_class"] = state_class;
	}
	doc["name"] = entity;
	if (strlen(icon) > 0) {
		doc["icon"] = icon;
	}
	if (strlen(unit_of_meas) > 0)
	{ 
		doc["unit_of_measurement"] = unit_of_meas;
	}
	sprintf(buffer, "~/stat/%s", subtopic);
	doc["state_topic"] = buffer;
	sprintf(buffer, "{{ value_json.%s }}", entity);
	doc["value_template"] = buffer;
	doc["~"] = _pcb->getRootTopicPrefix();
	doc["availability_topic"] = "~/tele/LWT";
    doc["pl_avail"] = "Online";
    doc["pl_not_avail"] = "Offline";
	JsonObject device = doc["device"].to<JsonObject>();
	device["name"] = _pcb->getDeviceName();
	device["via_device"] = _pcb->getThingName();
	device["sw_version"] = CONFIG_VERSION;
	device["manufacturer"] = _manufacturer;
	device["model"] = _model;
	device["hw_version"] = _version;
	sprintf(buffer, "%X_%s", _pcb->getUniqueId(), _pcb->getDeviceName().c_str());
	device["identifiers"] = buffer;
	sprintf(buffer, "ESP%X_%s", _pcb->getUniqueId(), entity);
	doc["unique_id"] = buffer;
	String object_id = buffer;
	sprintf(buffer, "%s/sensor/%s/config", HOME_ASSISTANT_PREFIX, object_id.c_str());
	_pcb->PublishMessage(buffer, doc);
}

} // namespace TS45ToMQTT