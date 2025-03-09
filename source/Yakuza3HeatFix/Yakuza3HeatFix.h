#pragma once


namespace HeatFix {
	inline void **PlayerActor = nullptr;
	inline constexpr uint16_t MAX_DrainTimer = 0x73;
	inline constexpr float floatMulti = 1.0f / 2.0f; // for float values that have to be halved when running at 60 fps
	inline constexpr uint8_t integerDiv = 2; // for integer values that have to be halved when running at 60 fps

	typedef bool (*GetActorBoolType)(void **);
	typedef float (*GetActorFloatType)(void **);
	typedef uint8_t(*AddSubtractHeatType)(void **, int32_t);
	typedef char *(*GetDisplayStringType)(void *, uint32_t, uint32_t);

	extern GetActorBoolType verifyIsPlayerDrunk;
	extern GetActorFloatType verifyGetCurHeat, verifyGetMaxHeat;
	extern AddSubtractHeatType verifyAddHeat;
	extern GetDisplayStringType origGetDisplayString;

	bool PatchedIsPlayerDrunk(void **playerActor, const float newHeatVal, const uint16_t newDrainTimer);
	uint8_t AddSubtractHeatWrapper(void **playerActor, int32_t amount);
	uint16_t GetNewHeatDrainTimer(void **playerActor, const uint16_t curDrainTimer);
	uint8_t AddHeatComboFinisher(void **playerActor, int32_t amount);
	float GetNewHeatValue(void **playerActor, const float newHeatVal, const float baseDrainRate, const uint16_t newDrainTimer);
	float GetMaxHeatValue(void **playerActor);
	char *PatchedGetDisplayString(void *param1, uint32_t param2, uint32_t param3);
}

namespace LegacyHeatFix {
	void InitPatch();
}
