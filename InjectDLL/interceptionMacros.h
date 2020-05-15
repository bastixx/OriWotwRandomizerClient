#pragma once
#include "interception.h"

#define MANUAL_INTERCEPT(offset, returnType, name, params, ...) \
	returnType (*name) params; \
	returnType name##Intercept params __VA_ARGS__ \
	intercept binding_##name (offset, &(PVOID&) name, name##Intercept, #returnType" "#name" "#params";");

#define INTERCEPT(returnType, name, params, ...) \
	returnType (*name) params; \
	returnType name##Intercept params __VA_ARGS__ \
	intercept binding_##name (&(PVOID&) name, name##Intercept, #returnType" "#name" "#params";");

#define MANUAL_BINDING(offset, returnType, name, params) \
	returnType (*name) params; \
	intercept binding_##name (offset, &(PVOID&) name, nullptr, #returnType" "#name" "#params";");


#define BINDING(returnType, name, params) \
	returnType (*name) params; \
	intercept binding_##name (&(PVOID&) name, nullptr, #returnType" "#name" "#params";");

#define STATIC(type, name) \
    type name; \
    intercept binding_##name (&(PVOID&) name, nullptr, #type);