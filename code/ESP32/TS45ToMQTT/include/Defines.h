
#pragma once

#define TAG "TS45ToMQTT"
#define CONFIG_VERSION "V1.1.0" // major.minor.build (major or minor will invalidate the configuration)
#define HOME_ASSISTANT_PREFIX "homeassistant" // MQTT prefix used in autodiscovery
#define STR_LEN 255                            // general string buffer size
#define CONFIG_LEN 32                         // configuration string buffer size
#define NUMBER_CONFIG_LEN 6
#define MQTT_TOPIC_LEN 128
#define AP_TIMEOUT 30000
#define MAX_PUBLISH_RATE 30000
#define MIN_PUBLISH_RATE 1000
#define CheckBit(var,pos) ((var) & (1<<(pos))) ? true : false
#define toShort(i, v) (v[i++]<<8) | v[i++]

#define WAKE_PUBLISH_RATE 2000
#define SNOOZE_PUBLISH_RATE 300000
#define WAKE_COUNT 60
#define WATCHDOG_TIMER 600000 //time in ms to trigger the watchdog

#define RXPIN GPIO_NUM_16
#define TXPIN GPIO_NUM_17
#define REDEPIN GPIO_NUM_4
#define BAUDRATE 9600
#define FIRST_REGISTER 0x0000
#define NUM_VALUES 30
#define RTU_ERROR_RETRY_LIMIT 5
#define GoOfflineAfterNoRTUResponse 120000

#define DEVICE_ID_TOKEN 4443 // modbus read device Identification (0x2B)
#define WRITE_COIL_TOKEN 4442 // modbus write COIL Identification (0x05)
#define READ_COIL_TOKEN 4441 // modbus read COIL Identification (0x01)
#define READ_HOLD_TOKEN 4440 // modbus read REGISTER Identification (0x03)

// #define MQTT_MIN_FREE_MEMORY 4096