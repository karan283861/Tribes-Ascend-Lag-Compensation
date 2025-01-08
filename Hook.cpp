#include <format>
#include <string>
#include "Hook.hpp"

#define PROCESSEVENT_ADDRESS	(0x00456F90)
#define PROCESSINTERNAL_ADDRESS	(0x00459040)

unsigned int indentLevel{0};

std::string GetIndentedTabString(void)
{
	auto s = std::string();
	for (int i = 0; i < indentLevel; i++)
	{
		s.append("\t");
	}
	return s;
}

ProcessEventPrototype OriginalProcessEventFunction = reinterpret_cast<ProcessEventPrototype>(PROCESSEVENT_ADDRESS);
UFunctionHooks<ProcessEventPrototype> ProcessEventHooks(nullptr);
void __fastcall ProcessEventHook(UObject* CallingUObject,
								 void* Unused, UFunction* CallingUFunction,
								 void* Parameters, void* Result)
{
	indentLevel++;
	PLOG_VERBOSE << GetIndentedTabString() << std::format("{0} : {1}", CallingUFunction->GetFullName(), CallingUObject->GetFullName());
	ProcessEventHooks.ExecuteHook(CallingUFunction, CallingUObject, Unused, CallingUFunction, Parameters, Result);
	indentLevel--;
}

ProcessInternalPrototype OriginalProcessInternalFunction = reinterpret_cast<ProcessInternalPrototype>(PROCESSINTERNAL_ADDRESS);
UFunctionHooks<ProcessInternalPrototype> ProcessInternalHooks(nullptr);
void __fastcall ProcessInternalHook(UObject* CallingUObject,
									void* Unused, FFrame& Stack,
									void* Result)
{
	indentLevel++;
	auto callingUFunction = reinterpret_cast<UFunction*>(Stack.m_Node);
	PLOG_VERBOSE << GetIndentedTabString() << std::format("{0} : {1}", callingUFunction->GetFullName(), CallingUObject->GetFullName());
	ProcessInternalHooks.ExecuteHook(callingUFunction, CallingUObject, Unused, Stack, Result);
	indentLevel--;
}
