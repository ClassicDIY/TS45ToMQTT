#pragma once
#include <stdint.h>
#include <WString.h>

struct ChargeControllerInfo
{

//	int32_t unitID = 0;
//	String deviceName = "";
//	bool hasWhizbang = false;

//	String model;
//	String macAddress;
//	float lastVOC = 0;
//	String appVersion = "";
//	String netVersion = "";
//	String buildDate = "";
	uint16_t nominalBatteryVoltage = 0;
//	uint16_t mpptMode = 0;
//	float endingAmps = 0;

	float BatteryVoltage = 0;
	float BatterySenseVoltage = 0;
	float ArrayLoadVoltage = 0;
	float ChargeCurrent = 0;
	float LoadCurrent = 0;
	float BatteryVoltage_Slow = 0;
	float HeatsinkTemperature = 0;
	float BatteryTemperature = 0;
	float ChargeRegulatorReferenceVoltage = 0;
	float AmpHoursResetable = 0;
	float AmpHoursTotal = 0;
	uint32_t HourMeter = 0;
	uint32_t Alarm = 0;
	uint16_t Fault = 0;
	uint8_t DipSwitch = 0;
	uint8_t ControlMode = 0;
	uint8_t ControlState = 0;
	uint8_t PWM = 0;


//	float PCBTemperature = 0;
//	uint16_t FloatTimeTodaySeconds = 0;
//	uint16_t AbsorbTime = 0;
//	uint16_t EqualizeTime = 0;
//	bool Aux1 = false;
//	bool Aux2 = false;
	uint32_t PositiveAmpHours = 0;
	int32_t NegativeAmpHours = 0;
	uint32_t NetAmpHours = 0;
//	float ShuntTemperature = 0;
//	float WhizbangBatCurrent = 0;
//	uint16_t SOC = 0;
//	uint16_t RemainingAmpHours = 0;
	uint16_t TotalAmpHours = 0;
//	float VbattRegSetPTmpComp = 0;
//	uint16_t ReasonForResting = 0;
};

typedef struct 
{
	bool received;
	int address;
	int numberOfRegisters;
	std::function<void(uint8_t *data)> func;
} ModbusRegisterBank;

