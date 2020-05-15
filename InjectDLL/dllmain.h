#pragma once
#include "PEModule.h"

void onFixedUpdate(GameController_o* thisPointer);
static std::string pretty_time();
bool isInShopScreen();
extern InjectDLL::PEModule* CSharpLib;
void log(std::string message);
void error(std::string message);
void debug(std::string message);