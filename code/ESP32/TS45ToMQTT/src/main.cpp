#include <Arduino.h>
#include <SPI.h>

#include "Log.h"
#include "Enumerations.h"
#include "IOT.h"
#include "ModbusClientRTU.h"
#include "TS45.h"

#ifdef MODBUS_LOG
// #define LOCAL_LOG_LEVEL LOG_LEVEL_DEBUG
#undef LOCAL_LOG_LEVEL
#include "Logging.h" // modbus logging
#endif

using namespace TS45ToMQTT;

TS45ToMQTT::TS45 _ts45 = TS45ToMQTT::TS45();
TS45ToMQTT::IOT _iot = TS45ToMQTT::IOT();
hw_timer_t *_watchdogTimer = NULL;

unsigned long _lastPublishTimeStamp = 0;
unsigned long _lastModbusPollTimeStamp = 0;
unsigned long _currentPublishRate = WAKE_PUBLISH_RATE; // rate currently being used
unsigned long _wakePublishRate = WAKE_PUBLISH_RATE; // wake publish rate set by config or mqtt command
boolean _stayAwake = false;
int _publishCount = 0;

void IRAM_ATTR resetModule()
{
	// ets_printf("watchdog timer expired - rebooting\n");
	esp_restart();
}

void init_watchdog()
{
	if (_watchdogTimer == NULL)
	{
		_watchdogTimer = timerBegin(0, 80, true);					   //timer 0, div 80
		timerAttachInterrupt(_watchdogTimer, &resetModule, true);	  //attach callback
		timerAlarmWrite(_watchdogTimer, WATCHDOG_TIMER * 1000, false); //set time in us
		timerAlarmEnable(_watchdogTimer);							   //enable interrupt
	}
}

void feed_watchdog()
{
	if (_watchdogTimer != NULL)
	{
		timerWrite(_watchdogTimer, 0); // feed the watchdog
	}
}

void Wake()
{
	_currentPublishRate = _wakePublishRate;
	_lastPublishTimeStamp = 0;
	_lastModbusPollTimeStamp = 0;
	_publishCount = 0;
}

void setup()
{
	Serial.begin(115200);
	while (!Serial) {}
	logd("Booting");
	_iot.Init(&_ts45);
	init_watchdog();
	_lastPublishTimeStamp = millis() + WAKE_PUBLISH_RATE;
	_ts45.begin(&_iot);
	logd("Done setup");
}

void loop()
{
	if (_iot.Run()) {
		
		if (_lastPublishTimeStamp < millis())
		{
			feed_watchdog();
			_ts45.run(); 
			_lastPublishTimeStamp = millis() + _iot.PublishRate();
		}
	}
	else {
		feed_watchdog(); // don't reset when not configured
	}
}
