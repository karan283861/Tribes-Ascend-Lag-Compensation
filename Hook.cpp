#include <format>
#include <string>
#include "Hook.hpp"

#define PROCESSEVENT_ADDRESS	(0x00456F90)
#define PROCESSINTERNAL_ADDRESS	(0x00459040)
#define CALLFUNCTION_ADDRESS	(0x0045AD20)
#define FFRAME_STEP_ADDRESS		(0x0)

unsigned int indentLevel{ 0 };

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
	auto isNative{ CallingUFunction->iNative || CallingUFunction->FunctionFlags & FUNC_Native };
	PLOG_VERBOSE << GetIndentedTabString() << std::format("{0} {1} : {2}", isNative ? "[NATIVE]" : "",
														  CallingUFunction->GetFullName(), CallingUObject->GetFullName());
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
	auto isNative{ callingUFunction->iNative || callingUFunction->FunctionFlags & FUNC_Native };
	PLOG_VERBOSE << GetIndentedTabString() << std::format("{0} {1} : {2}", isNative ? "[NATIVE]" : "",
														  callingUFunction->GetFullName(), CallingUObject->GetFullName());
	ProcessInternalHooks.ExecuteHook(callingUFunction, CallingUObject, Unused, Stack, Result);
	indentLevel--;
}

CallFunctionPrototype OriginalCallFunctionFunction = reinterpret_cast<CallFunctionPrototype>(CALLFUNCTION_ADDRESS);
UFunctionHooks<CallFunctionPrototype> CallFunctionHooks(nullptr);
void __fastcall CallFunctionHook(UObject* CallingUObject,
								 void* Unused, FFrame& Stack,
								 void* Result, UFunction* CallingUFunction)
{
	if (!CallFunctionHooks.m_OriginalFunction)
	{
		return OriginalCallFunctionFunction(CallingUObject, Unused, Stack, Result, CallingUFunction);
	}

	indentLevel++;
	auto isNative{ CallingUFunction->iNative || CallingUFunction->FunctionFlags & FUNC_Native };
	PLOG_VERBOSE << GetIndentedTabString() << std::format("{0} {1} : {2}", isNative ? "[NATIVE]" : "",
														  CallingUFunction->GetFullName(), CallingUObject->GetFullName());
	CallFunctionHooks.ExecuteHook(CallingUFunction, CallingUObject, Unused, Stack, Result, CallingUFunction);
	indentLevel--;
}

//std::unordered_map<unsigned int, GNativeFunctionHookInformation> indexToGNativeHookInformation{};

//void __fastcall GNativeFunctionHook(UObject* CallingUObject,
//									void* Unused, FFrame& Stack,
//									void* Result)
//{
//	auto callingUFunction = reinterpret_cast<UFunction*>(Stack.m_Node);
//	auto isNative{ callingUFunction->iNative || callingUFunction->FunctionFlags & FUNC_Native };
//	PLOG_VERBOSE << GetIndentedTabString() << std::format("{0} {1} : {2}", isNative ? "[NATIVE]" : "",
//														  callingUFunction->GetFullName(), CallingUObject->GetFullName());
//	//return OriginalGNativeFunctionFromiNative.at(iNativeIndex)(Frame, Unused, Context, Result);
//}
