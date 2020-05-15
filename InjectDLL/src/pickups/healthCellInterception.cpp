#include "pch.h"
#include "../../interceptionMacros.h"

bool is_collecting_health_cell = false;
INTERCEPT(void, SeinPickupProcessor__OnCollectMaxHealthHalfContainerPickup, (SeinPickupProcessor_o* this_ptr, MaxHealthHalfContainerPickup_o* maxHealthContainerPickup), {
	is_collecting_health_cell = true;
	SeinPickupProcessor__OnCollectMaxHealthHalfContainerPickup(this_ptr, maxHealthContainerPickup);
	is_collecting_health_cell = false;
});

INTERCEPT(void, SeinHealthController__set_BaseMaxHealth, (SeinHealthController_o* this_ptr, int32_t value), {	
	if(is_collecting_health_cell)
		return;

	SeinHealthController__set_BaseMaxHealth(this_ptr, value);
});

INTERCEPT(void, SeinHealthController__set_Amount, (SeinHealthController_o* this_ptr, float value), {
	if(is_collecting_health_cell)
		return;
	SeinHealthController__set_Amount(this_ptr, value);
});

INTERCEPT(void, SeinLevel__set_PartialHealthContainers, (SeinLevel_o* this_ptr, int32_t value), {
	if(is_collecting_health_cell)
		return;

	SeinLevel__set_PartialHealthContainers(this_ptr, value);
});

INTERCEPT(int, SeinLevel__get_PartialHealthContainers, (SeinLevel_o* this_ptr), {
	if(is_collecting_health_cell)
		return 1;

	return SeinLevel__get_PartialHealthContainers(this_ptr);
});