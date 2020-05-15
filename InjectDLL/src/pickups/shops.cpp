#include "pch.h"
#include <functional>
#include <set>

#include "../../interceptionMacros.h"
#include "dllmain.h"


BINDING(bool, SeinCharacter__get_Active, (SeinCharacter_o* this_ptr)) //SeinCharacter$$get_Active - Also used by stuff like IsActive, or get_IsShopOpen. It's a magical binding
BINDING(bool, PurchaseThingScreen__get_IsShopOpen, (PurchaseThingScreen_o* this_ptr))
BINDING(WeaponmasterScreen_o*, WeaponmasterScreen__get_Instance, ()) //WeaponmasterScreen$$get_Instance
BINDING(bool, GameController__get_GameInTitleScreen, (GameController_o* this_ptr)) //GameController$$get_GameInTitleScreen

BINDING(bool, WeaponmasterItem__get_IsLocked, (WeaponmasterItem_o* this_ptr))       //WeaponmasterItem$$get_IsLocked
BINDING(bool, WeaponmasterItem__get_IsVisible, (WeaponmasterItem_o* this_ptr))      //WeaponmasterItem$$get_IsVisible
BINDING(bool, WeaponmasterItem__get_IsAffordable, (WeaponmasterItem_o* this_ptr))   //WeaponmasterItem$$get_IsAffordable

bool weaponmasterPurchaseInProgress = false;
const std::set<char> twillenShards{1, 2, 3, 5, 19, 22, 26, 40};
bool isTwillenShard(char shard){
	return twillenShards.find(shard) != twillenShards.end();
}

STATIC(GameController_c*, Class_GameController)
STATIC(SpiritShardsShopScreen_c*, Class_SpiritShardsShopScreen)

bool isInShopScreen(){	
	if(!Class_GameController)
		return false;
	const auto  gameController = Class_GameController->static_fields->Instance;
	if(!gameController || GameController__get_GameInTitleScreen(gameController))
		return false;

	const auto weaponmasterScreen = WeaponmasterScreen__get_Instance();
	if(weaponmasterScreen && PurchaseThingScreen__get_IsShopOpen((PurchaseThingScreen_o*) weaponmasterScreen))
		return true;

	if(Class_SpiritShardsShopScreen)
	{
		const auto spiritShardsShopScreen = Class_SpiritShardsShopScreen->static_fields->Instance;
		if(spiritShardsShopScreen && PurchaseThingScreen__get_IsShopOpen((PurchaseThingScreen_o*) spiritShardsShopScreen))
			return true;
	}
	return false;
};

BINDING(Moon_uberSerializationWisp_PlayerUberStateShards_Shard_o*, SpiritShardsShopScreen__get_SelectedSpiritShard, (SpiritShardsShopScreen_o* this_ptr))//SpiritShardsShopScreen$$get_SelectedSpiritShard
INTERCEPT(bool, SpiritShardsShopScreen__CanPurchase, (SpiritShardsShopScreen_o* this_ptr), {
	//SpiritShardsShopScreen$$CanPurchase
	auto result = SpiritShardsShopScreen__CanPurchase(this_ptr);
	return result;
});


BINDING(void, SpiritShardsShopScreen__UpdateContextCanvasShards, (SpiritShardsShopScreen_o* this_ptr))//SpiritShardsShopScreen$$UpdateContextCanvasShards
BINDING(void, Moon_uberSerializationWisp_PlayerUberStateShards_Shard__RunSetDirtyCallback, (Moon_uberSerializationWisp_PlayerUberStateShards_Shard_o* this_ptr))//Moon.uberSerializationWisp.PlayerUberStateShards.Shard$$RunSetDirtyCallback

INTERCEPT(void, SpiritShardsShopScreen__CompletePurchase, (SpiritShardsShopScreen_o* this_ptr), {
	//SpiritShardsShopScreen$$CompletePurchase
	//save shard new/purchased state
	auto shard = SpiritShardsShopScreen__get_SelectedSpiritShard(this_ptr);
	bool first = *(bool*) (shard + 24);
	bool second = *(bool*) (shard + 25);

    SpiritShardsShopScreen__CompletePurchase(this_ptr);

	// rollback vanilla purchase 
	*(bool*) (shard + 24) = first;
	*(bool*) (shard + 25) = second;

    // do the rando purchase /after/ rollback, xem ;3
    auto shardType = *(unsigned __int8*)(shard + 0x10);
    CSharpLib->call<void, char>("TwillenBuyShard", shardType);

    Moon_uberSerializationWisp_PlayerUberStateShards_Shard__RunSetDirtyCallback(shard);
    SpiritShardsShopScreen__UpdateContextCanvasShards(this_ptr);
});


INTERCEPT(bool, Moon_uberSerializationWisp_PlayerUberStateShards_Shard__get_PurchasableInShop, (Moon_uberSerializationWisp_PlayerUberStateShards_Shard_o* this_ptr), {
	//Moon.uberSerializationWisp.PlayerUberStateShards.Shard$$get_PurchasableInShop
	auto shardType = *(unsigned __int8*) (this_ptr + 0x10);
	return true;
		  });

#pragma warning(disable: 4244)
BINDING(Il2CppObject*, System_Collections_Generic_List_T___get_Item, (System_Collections_Generic_List_T__o* this_ptr, int32_t index)) //System.Collections.Generic.List<T>$$get_Item
BINDING(int32_t, System_Collections_Generic_List_T___get_Count, (System_Collections_Generic_List_T__o* this_ptr)) //System.Collections.Generic.List<T>$$get_Count
void forEachIndexed(System_Collections_Generic_List_T__o* list, std::function<void(Il2CppObject*, int)> fun){
	if(!list)
		return;

	int size = System_Collections_Generic_List_T___get_Count(list);
	for(int i = 0; i < size; i++)
	{
		fun(System_Collections_Generic_List_T___get_Item(list, i), i);
	}
}
#pragma warning(default: 4244)

void initShardDescription(unsigned __int8 shard, SpiritShardDescription_o* spiritShardDescription){
	//Set purchase cost (normal):
	if(!isTwillenShard(shard))
		return;
	*(int*) (spiritShardDescription + 0x38) = CSharpLib->call<int, char>("TwillenShardCost", shard);

	auto upgradableAbilityList = *(System_Collections_Generic_List_T__o**) (spiritShardDescription + 0x40);
	forEachIndexed(upgradableAbilityList, [shard](Il2CppObject* upgradableAbilityLevel, int index)-> void{
		if(upgradableAbilityLevel)
		{
			//Set upgrade cost (normal):
			// *(int*)(upgradableAbilityLevel + 0x18) = 10;// 1337 * 2;
			//TODO: @Eiko: Get this from c# too
		}
    });
}

STATIC(SpiritShardDescription_c*, Class_SpiritShardDescription)

//TODO: This had *another* extra param... halp
INTERCEPT(Il2CppObject*, EnumDictionary_ENUMTYPE__VALUETYPE___GetValue, (EnumDictionary_ENUMTYPE__VALUETYPE__o* this_ptr, Il2CppObject* key), {
	//EnumDictionary<ENUMTYPE, VALUETYPE>$$GetValue
	auto value = EnumDictionary_ENUMTYPE__VALUETYPE___GetValue(this_ptr, key);
    
    if(value && value->klass == (Il2CppClass*) Class_SpiritShardDescription){
        
	    initShardDescription((unsigned __int8) key, (SpiritShardDescription_o*) value);
    }
    return value;
});

INTERCEPT(int32_t, Moon_uberSerializationWisp_PlayerUberStateShards_Shard__GetCostForLevel, (Moon_uberSerializationWisp_PlayerUberStateShards_Shard_o* this_ptr, int32_t level), {
	//Moon.uberSerializationWisp.PlayerUberStateShards.Shard$$GetCostForLevel - For whenever we want random upgrade costs
	return Moon_uberSerializationWisp_PlayerUberStateShards_Shard__GetCostForLevel(this_ptr, level);
});

INTERCEPT(bool, PlayerSpiritShards__HasShard, (PlayerSpiritShards_o* this_ptr, uint8_t shardType), {
	//PlayerSpiritShards$$HasShard
	if(isInShopScreen() && isTwillenShard(shardType))
	{
		return CSharpLib->call<bool, char>("TwillenBoughtShard", shardType);
	}
	return PlayerSpiritShards__HasShard(this_ptr, shardType);
});


char getWeaponMasterAbilityItemGranted(WeaponmasterItem_o* weaponmasterItem){
	return  *(char*) ((*(__int64*) (weaponmasterItem + 0x10)) + 0x39);
}
char getWeaponMasterAbilityItemRequired(WeaponmasterItem_o* weaponmasterItem){
	return  *(char*) ((*(__int64*) (weaponmasterItem + 0x10)) + 0x38);
}


int purchases = 0;
bool hasBeenPurchasedBefore(WeaponmasterItem_o* weaponMasterItem){
	char grantedType = getWeaponMasterAbilityItemGranted(weaponMasterItem);
	char requiredType = getWeaponMasterAbilityItemRequired(weaponMasterItem);
	if((int) grantedType != -1)
		return CSharpLib->call<bool, char>("OpherBoughtWeapon", grantedType);
	if((int) requiredType == -1) // fast travel; 255, 255 -> 105, 0
		return CSharpLib->call<bool, char>("OpherBoughtWeapon", 105);
	return CSharpLib->call<bool, char>("OpherBoughtUpgrade", requiredType);
}

bool purchasable(WeaponmasterItem_o* weaponmasterItem){
	return !hasBeenPurchasedBefore(weaponmasterItem) && !WeaponmasterItem__get_IsLocked(weaponmasterItem) &&
		WeaponmasterItem__get_IsVisible(weaponmasterItem) && WeaponmasterItem__get_IsAffordable(weaponmasterItem);

}

INTERCEPT(bool, WeaponmasterItem__get_IsOwned, (WeaponmasterItem_o* this_ptr), {
	//WeaponmasterItem$$get_IsOwned
	if(isInShopScreen())
	{
		return hasBeenPurchasedBefore(this_ptr);
	}
	return WeaponmasterItem__get_IsOwned(this_ptr);
});

INTERCEPT(int, WeaponmasterItem__GetCostForLevel, (WeaponmasterItem_o* this_ptr, int32_t level), {
	//WeaponmasterItem$$GetCostForLevel
	if(isInShopScreen())
	{
		char abilityType = getWeaponMasterAbilityItemGranted(this_ptr);
		//TODO: @Eiko - you know what to do
		if((int) abilityType == -1) {
			if((int) getWeaponMasterAbilityItemRequired(this_ptr) == -1) // fast travel; 255, 255 -> 105, 0
				return CSharpLib->call<int, char>("OpherWeaponCost", 105);
			return WeaponmasterItem__GetCostForLevel(this_ptr, level);
		}
		return CSharpLib->call<int, char>("OpherWeaponCost", abilityType);
	}
	return WeaponmasterItem__GetCostForLevel(this_ptr, level);
});


INTERCEPT(bool, WeaponmasterItem__TryPurchase, (WeaponmasterItem_o* this_ptr, System_Action_MessageProvider__o* ShowHint, UISoundSettingsAsset_o* Sounds, ShopKeeperHints_o* Hints), {
	//WeaponmasterItem$$TryPurchase
	if(purchasable(this_ptr)) {
        return true;
	}		

    WeaponmasterItem__TryPurchase(this_ptr, ShowHint, Sounds, Hints);
	return false;
});


INTERCEPT(Moon_uberSerializationWisp_PlayerUberStateInventory_InventoryItem_o*, SpellInventory__AddNewSpellToInventory, (SpellInventory_o* this_ptr, int32_t type, bool adding), {
	//SpellInventory$$AddNewSpellToInventory
	if(weaponmasterPurchaseInProgress)
		return 0;

	auto result = SpellInventory__AddNewSpellToInventory(this_ptr, type, adding);
	return result;
});

INTERCEPT(void, Moon_SerializedByteUberState__set_Value, (Moon_SerializedByteUberState_o* this_ptr, uint8_t value), {
	//Moon.SerializedByteUberState$$set_Value
    if(weaponmasterPurchaseInProgress)
		return;
  
    Moon_SerializedByteUberState__set_Value(this_ptr, value);
});

INTERCEPT(void, WeaponmasterItem__DoPurchase, (WeaponmasterItem_o* item, PurchaseContext_o context), {
	//Weaponmasteritem$$DoPurchase
	weaponmasterPurchaseInProgress = true;
	auto abilityType = getWeaponMasterAbilityItemGranted(item);
	if((int) abilityType != -1)
		CSharpLib->call<void, char>("OpherBuyWeapon", abilityType);
	else {
		char requiredType = getWeaponMasterAbilityItemRequired(item);
		if((int) requiredType == -1) // fast travel; 255, 255 -> 105, 0
			CSharpLib->call<void, char>("OpherBuyWeapon", 105);
        else {
            CSharpLib->call<void, char>("OpherBuyUpgrade", requiredType);
            weaponmasterPurchaseInProgress = false; // so upgrade buying isn't no-opped
        }
	}
    WeaponmasterItem__DoPurchase(item, context);
    weaponmasterPurchaseInProgress = false;
})