#include <format>
#include "Hook.hpp"

ProcessEventPrototype OriginalProcessEventFunction = reinterpret_cast<ProcessEventPrototype>(0x0);
UFunctionHooks<ProcessEventPrototype> ProcessEventHooks(nullptr);
void __fastcall ProcessEventHook(UObject* CallingUObject,
								 void* Unused, UFunction* CallingUFunction,
								 void* Parameters, void* Result)
{
	PLOG_VERBOSE << std::format("{0} : {1}", CallingUFunction->GetFullName(), CallingUObject->GetFullName());
	ProcessEventHooks.ExecuteHook(CallingUFunction, CallingUObject, Unused, CallingUFunction, Parameters, Result);
}
