#include "pch.h"
// dllmain.cpp : Defines the entry point for the DLL application.

#pragma comment(lib, "detours.lib")

#include "detours.h"
#include "PEModule.h"
#include "common.h"
#include "dllmain.h"

#include <string>
#include <functional>
#include <iostream>
#include <unordered_map>
#include <set>
#include <fstream>
#include <utility>
#include <chrono>
#include <ctime>

#include "interceptionMacros.h"
#include "src/pickups/oreInterception.h"
#include "src/fixes/dashFixes.h"

//---------------------------------------------------Globals-----------------------------------------------------
char foundTree = -1;
char priorFoundTree = -1;
bool foundTreeFulfilled = false;

__int64 lastDesiredState = NULL;
__int64 priorDesiredState = NULL;
GameController_o* gameControllerInstancePointer = nullptr;

bool debug_enabled = false;
bool info_enabled = true;
bool error_enabled = true;
bool input_lock_callback = false;
const std::set<char> treeAbilities{0, 5, 8, 23, 51, 57, 62, 77, 97, 100, 101, 102, 104, 121};

bool isTree(char tree){
	return treeAbilities.find(tree) != treeAbilities.end();
}

InjectDLL::PEModule* CSharpLib = NULL;

std::string logFilePath = "C:\\moon\\inject_log.txt"; // change this if you need to
std::ofstream logfile;

#define DEBUG(message) \
    if (debug_enabled) { \
	    logfile.open(logFilePath, std::ios_base::app); \
	    logfile << "[" << pretty_time()  << "] (DEBUG): " << message << std::endl; \
	    logfile.close(); \
    }

#define LOG(message) \
    if (info_enabled) { \
	    logfile.open(logFilePath, std::ios_base::app); \
	    logfile << "[" << pretty_time() << "] (INFO): " << message << std::endl; \
	    logfile.close(); \
    }

#define ERR(message) \
    if (error_enabled) { \
	    logfile.open(logFilePath, std::ios_base::app); \
	    logfile << "[" << pretty_time() << "] (ERROR): " << message << std::endl; \
	    logfile.close(); \
    }

//---------------------------------------------------------Intercepts----------------------------------------------------------

BINDING(void, GameController__CreateCheckpoint, (GameController_o* this_ptr, bool doPerformSave, bool respectRestrictCheckpointZone))
void createCheckpoint (GameController_o* this_ptr)
{
    GameController__CreateCheckpoint(this_ptr, true, true);
}
//GameController$$createCheckpoint
;
BINDING(GameWorldArea_o*, GameWorld__GetArea, (GameWorld_o* this_ptr, int32_t areaID)) //GameWorld$$GetArea
BINDING(RuntimeGameWorldArea_o*, GameWorld__FindRuntimeArea, (GameWorld_o* this_ptr, GameWorldArea_o* area)) //GameWorld$$FindRuntimeArea
BINDING(void, RuntimeGameWorldArea__DiscoverAllAreas, (RuntimeGameWorldArea_o* this_ptr)) //RuntimeGameWorldArea$$DiscoverAllAreas

GameWorld_o* gameWorldInstance = 0;

bool foundGameWorld() {
    return gameWorldInstance != 0;
}

BINDING(void, Moon_UberStateController__ApplyAll, (int32_t context))

extern "C" __declspec(dllexport)
void magicFunction() { Moon_UberStateController__ApplyAll(1); }

INTERCEPT(void, GameWorld__Awake, (GameWorld_o* thisPtr), {
	if(gameWorldInstance != thisPtr) {
		debug("Found GameWorld instance!");
		gameWorldInstance = thisPtr;
	}
	GameWorld__Awake(thisPtr);
});

//TODO: :upside_down:
INTERCEPT(bool, sub180FC4D50, (__int64 mappingPtr, __int64 uberState), {
	//RVA: 13A7AA0. Called from PlayerStateMap.Mapping::Matches
	bool result = sub180FC4D50(mappingPtr, uberState);
	if(isTree(foundTree))
		result = CSharpLib->call<bool, BYTE>("DoInvertTree", foundTree) ^ result;
	return result;
		  });


// stop complaining about LOCK not having enough parameters
#pragma warning(disable: 4003)


INTERCEPT(void, GetAbilityOnCondition__FixedUpdate, (GetAbilityOnCondition_o* thisPtr), {
	//GetAbilityOnCondition$$FixedUpdate
    GetAbilityOnCondition__FixedUpdate(thisPtr);
// BAD PROBLEMS DESERVE BAD SOLUTIONS
if(lastDesiredState != *(__int64*) (thisPtr + 0x18)) {
		if(lastDesiredState != NULL && priorDesiredState != *(__int64*) (thisPtr + 0x18)) {
			priorDesiredState = *(__int64*) (thisPtr + 0x18);
			BYTE* desiredAbility = (BYTE*) (priorDesiredState + 0x18);
			priorFoundTree = *desiredAbility;
			DEBUG("GAOC.FU: got " << priorDesiredState << " for address 2 of condition (1 was " << lastDesiredState << "), wants " << (int) *desiredAbility);
		} else {
			lastDesiredState = *(__int64*) (thisPtr + 0x18);
			BYTE* desiredAbility = (BYTE*) (lastDesiredState + 0x18);
			foundTree = *desiredAbility;
			//				  auto condPtr = *(__int64*)(thisPtr + 0x20);
			//				  BYTE* condAbility = (BYTE*)(condPtr + 0x18);
			DEBUG("GAOC.FU: got " << lastDesiredState << " for address of condition, wants " << (int) *desiredAbility);
			//				  LOG("DesiredState ability " << (int)*desiredAbility);
		}
}

		  });

INTERCEPT(void, GetAbilityOnCondition__AssignAbility, (GetAbilityOnCondition_o* thisPtr), {
	//GetAbilityOnCondition$$AssignAbility
	DEBUG("GAOC.ASS: intercepted and ignored ");
	if(isTree(foundTree))
		CSharpLib->call<void>("OnTree", foundTree);
		  });

//TODO: This had a second param once
INTERCEPT(bool, Moon_uberSerializationWisp_DesiredPlayerAbilityState__IsFulfilled, (Moon_uberSerializationWisp_DesiredPlayerAbilityState_o* this_ptr), {
	//Moon.uberSerializationWisp.DesiredPlayerAbilityState$$IsFulfilled
	if(lastDesiredState == (long long) this_ptr && isTree(foundTree))
		return CSharpLib->call<bool>("TreeFulfilled", foundTree);
	else if(priorDesiredState == (long long) this_ptr && isTree(priorFoundTree))
		return CSharpLib->call<bool>("TreeFulfilled", priorFoundTree);
	else
		return Moon_uberSerializationWisp_DesiredPlayerAbilityState__IsFulfilled(this_ptr);
		  });

INTERCEPT(void, GameController__FixedUpdate, (GameController_o* this_ptr), {
	//GameController$$FixedUpdate
    GameController__FixedUpdate(this_ptr);
	onFixedUpdate(this_ptr);
		  });
BINDING(int, SaveSlotsManager__get_CurrentSlotIndex, ())//SaveSlotsManager$$get_CurrentSlotIndex
INTERCEPT(void, NewGameAction__Perform, (NewGameAction_o* this_ptr, IContext_o* context), {
	//NewGameAction$$Perform
	CSharpLib->call<void, int>("NewGame", SaveSlotsManager__get_CurrentSlotIndex());
    NewGameAction__Perform(this_ptr, context);
		  });



INTERCEPT(void, SaveGameController__SaveToFile, (SaveGameController_o* this_ptr, int32_t slotIndex, int32_t backupIndex, System_Byte_array* bytes), {
	//SaveGameController$$SaveToFile
	CSharpLib->call<void, __int64>("OnSave", slotIndex);
    SaveGameController__SaveToFile(this_ptr, slotIndex, backupIndex, bytes);
		  });

INTERCEPT(void, SaveGameController__OnFinishedLoading, (SaveGameController_o* this_ptr), {
	//SaveGameController$$OnFinishedLoading
	CSharpLib->call<void, int>("OnLoad", SaveSlotsManager__get_CurrentSlotIndex());
    SaveGameController__OnFinishedLoading(this_ptr);
		  });

//this had a second param before! Double check!
INTERCEPT(void, SaveGameController__RestoreCheckpoint, (SaveGameController_o* this_ptr), {
	//SaveGameController$$RestoreCheckpoint
	CSharpLib->call<void, int>("OnLoad", SaveSlotsManager__get_CurrentSlotIndex());
    SaveGameController__RestoreCheckpoint(this_ptr);
});



// GameController::get_InputLocked
BINDING(bool, GameController__get_InputLocked, (GameController_o* this_ptr));
// GameController::get_LockInput
BINDING(bool, GameController__get_LockInput, (GameController_o* this_ptr));
// GameControler::get_IsSuspended
BINDING(bool, GameController__get_IsSuspended, (GameController_o* this_ptr));
// GameControler::get_SecondaryMapAndInventoryCanBeOpened
BINDING(bool, GameController__get_SecondaryMapAndInventoryCanBeOpened, (GameController_o* this_ptr));

STATIC(Game_Characters_c*, characters)

//---------------------------------------------------Actual Functions------------------------------------------------

Game_Characters_StaticFields* get_characters(){
	return characters->static_fields;
}

void onFixedUpdate(GameController_o* thisPointer){
	if(gameControllerInstancePointer != thisPointer) {
		DEBUG("got GameController.Instance pointer: " << thisPointer);
		gameControllerInstancePointer = thisPointer;
	}
	try {
		CSharpLib->call<int>("Update");
	} catch(int error)
	{
		LOG("got error code " << error);
	}
}

extern "C" __declspec(dllexport)
void setOre(int oreCount) {
    SeinLevel__set_Ore(get_characters()->m_sein->Level, oreCount);
}

extern "C" __declspec(dllexport)
bool playerCanMove() {
    if (gameControllerInstancePointer == NULL)
        return false; // can't move if the game controller doesn't exist
    // TODO: figure out which of these are superflous
    auto gcip = gameControllerInstancePointer;
    DEBUG("gIL: " << GameController__get_InputLocked(gcip) << ", gLI: " << GameController__get_LockInput(gcip) << ", gIS: " << GameController__get_IsSuspended(gcip) << ", gSMA: " << GameController__get_SecondaryMapAndInventoryCanBeOpened(gcip));
    return !(GameController__get_InputLocked(gcip) || GameController__get_LockInput(gcip) || GameController__get_IsSuspended(gcip)) && GameController__get_SecondaryMapAndInventoryCanBeOpened(gcip);
}

extern "C" __declspec(dllexport)
void foundDash() {
    hasRealDash = true;
}

extern "C" __declspec(dllexport)
void save() {
    if (gameControllerInstancePointer == NULL) {
        LOG("no pointer to game controller: can't save!");
        return;        
    }
    DEBUG("Checkpoint requested by c# code");
    createCheckpoint(gameControllerInstancePointer);
}

extern "C" __declspec(dllexport)
bool discoverEverything(){
	if(gameWorldInstance)
	{
        for(unsigned __int8 i = 0; i <= 15; i++)
		{
			auto area = GameWorld__GetArea(gameWorldInstance, i);
			if(!area)
			{
				//Areas: None, WeepingRidge, GorlekMines, Riverlands would crash the game
				continue;
			}
			auto runtimeArea = GameWorld__FindRuntimeArea(gameWorldInstance, area);
			if(!runtimeArea)
			{
				continue;
			}
			RuntimeGameWorldArea__DiscoverAllAreas(runtimeArea);
		}
        DEBUG("Map revealed");
        return true;
    } else {
		log("Tried to discover all, but haven't found the GameWorld Instance yet :(");
        return false;
	}
}

//--------------------------------------------------------------Old-----------------------------------------------------------

void log(std::string message){
	LOG(message);
}
void error(std::string message){
	ERR(message);
}
void debug(std::string message){
	DEBUG(message);
}

bool attached = false;
bool shutdown = false;

void MainThread(){
	log("loading c# dll...");
	CSharpLib = new InjectDLL::PEModule(_T("C:\\moon\\RandoMainDLL.dll"));
	if(CSharpLib->call<bool>("Initialize"))
	{
		debug_enabled = CSharpLib->call<bool>("InjectDebugEnabled");
		info_enabled = CSharpLib->call<bool>("InjectLogEnabled");
		LOG("debug: " << debug_enabled << " log: " << info_enabled);
		log("c# init complete");
		interception_init();
		return;
	} else
	{
		log("Failed to initialize, shutting down");
		shutdown = true;
		FreeLibraryAndExitThread(GetModuleHandleA("InjectDLL.dll"), 0);
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved){
    Beep(523, 500);
    MessageBoxA(NULL, "test", "test", NULL);
    std::cerr << "aaaaaa";
    std::cout << 45;
    log("hi");
	if(DetourIsHelperProcess())
	{
		return TRUE;
	}
	switch(fdwReason)
	{
		case DLL_PROCESS_ATTACH:
			if(!attached) {
				debug("init start");
				CreateThread(0, 0, (LPTHREAD_START_ROUTINE) MainThread, 0, 0, 0);
				attached = true;
			}
			break;
		case DLL_PROCESS_DETACH:
			shutdown = true;
			delete CSharpLib;
			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());
			interception_detach();

			log("detatch commit: " + std::to_string(DetourTransactionCommit()));
			break;
		default:
			break;
	}
	return TRUE;
}


// strftime format
#define LOGGER_PRETTY_TIME_FORMAT "%Y-%m-%d %H:%M:%S"

// printf format
#define LOGGER_PRETTY_MS_FORMAT ".%03d"


// convert current time to milliseconds since unix epoch
template <typename T>
static int to_ms(const std::chrono::time_point<T>& tp){
	using namespace std::chrono;

	auto dur = tp.time_since_epoch();
	return static_cast<int>(duration_cast<milliseconds>(dur).count());
}

// format it in two parts: main part with date and time and part with milliseconds
#pragma warning(disable:4267)
static std::string pretty_time(){
	auto tp = std::chrono::system_clock::now();
	std::time_t current_time = std::chrono::system_clock::to_time_t(tp);

	std::tm time_info;
	localtime_s(&time_info, &current_time);

	char buffer[128];

	int string_size = strftime(
		buffer, sizeof(buffer),
		LOGGER_PRETTY_TIME_FORMAT,
		&time_info
		);

	int ms = to_ms(tp) % 1000;

	string_size += std::snprintf(
		buffer + string_size, sizeof(buffer) - string_size,
		LOGGER_PRETTY_MS_FORMAT, ms
		);

	return std::string(buffer, buffer + string_size);
}
#pragma warning(default:4267)

extern "C" __declspec(dllexport)VOID NullExport(VOID){}
