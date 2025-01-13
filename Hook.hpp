#pragma once

#include <vector>
#include <unordered_map>
#include <plog/Log.h>
#include "Tribes-Ascend-SDK/SdkHeaders.h"

//#define GNATIVES (0x01328C40);

//
// Function flags.
//
enum EFunctionFlags
{
	// Function flags.
	FUNC_Final = 0x00000001,	// Function is final (prebindable, non-overridable function).
	FUNC_Defined = 0x00000002,	// Function has been defined (not just declared).
	FUNC_Iterator = 0x00000004,	// Function is an iterator.
	FUNC_Latent = 0x00000008,	// Function is a latent state function.
	FUNC_PreOperator = 0x00000010,	// Unary operator is a prefix operator.
	FUNC_Singular = 0x00000020,   // Function cannot be reentered.
	FUNC_Net = 0x00000040,   // Function is network-replicated.
	FUNC_NetReliable = 0x00000080,   // Function should be sent reliably on the network.
	FUNC_Simulated = 0x00000100,	// Function executed on the client side.
	FUNC_Exec = 0x00000200,	// Executable from command line.
	FUNC_Native = 0x00000400,	// Native function.
	FUNC_Event = 0x00000800,   // Event function.
	FUNC_Operator = 0x00001000,   // Operator function.
	FUNC_Static = 0x00002000,   // Static function.
	FUNC_HasOptionalParms = 0x00004000,	// Function has optional parameters
	FUNC_Const = 0x00008000,   // Function doesn't modify this object.
	//						= 0x00010000,	// unused
	FUNC_Public = 0x00020000,	// Function is accessible in all classes (if overridden, parameters much remain unchanged).
	FUNC_Private = 0x00040000,	// Function is accessible only in the class it is defined in (cannot be overriden, but function name may be reused in subclasses.  IOW: if overridden, parameters don't need to match, and Super.Func() cannot be accessed since it's private.)
	FUNC_Protected = 0x00080000,	// Function is accessible only in the class it is defined in and subclasses (if overridden, parameters much remain unchanged).
	FUNC_Delegate = 0x00100000,	// Function is actually a delegate.
	FUNC_NetServer = 0x00200000,	// Function is executed on servers (set by replication code if passes check)
	FUNC_HasOutParms = 0x00400000,	// function has out (pass by reference) parameters
	FUNC_HasDefaults = 0x00800000,	// function has structs that contain defaults
	FUNC_NetClient = 0x01000000,	// function is executed on clients
	FUNC_DLLImport = 0x02000000,	// function is imported from a DLL

	// Combinations of flags.
	FUNC_FuncInherit = FUNC_Exec | FUNC_Event,
	FUNC_FuncOverrideMatch = FUNC_Exec | FUNC_Final | FUNC_Latent | FUNC_PreOperator | FUNC_Iterator | FUNC_Static | FUNC_Public | FUNC_Protected | FUNC_Const,
	FUNC_NetFuncFlags = FUNC_Net | FUNC_NetReliable | FUNC_NetServer | FUNC_NetClient,

	FUNC_AllFlags = 0xFFFFFFFF,
};

using ProcessEventPrototype = void(__fastcall*)(UObject* CallingUObject,
												void* Unused, UFunction* CallingUFunction,
												void* Parameters, void* Result);

void __fastcall ProcessEventHook(UObject* CallingUObject,
								 void* Unused, UFunction* CallingUFunction,
								 void* Parameters, void* Result);

extern ProcessEventPrototype OriginalProcessEventFunction;

class FFrame
{
public:
	void* m_VMT{};
	char m_UnknownData0[12]{};
	UStruct* m_Node{};
	UObject* m_Object{};
	char* m_Code{};
	char* m_Locals{};
	int	m_LineNum{};
	FFrame* m_PreviousFrame{};
};

using ProcessEventPrototype = void(__fastcall*)(UObject* CallingUObject,
												void* Unused, UFunction* CallingUFunction,
												void* Parameters, void* Result);

void __fastcall ProcessEventHook(UObject* CallingUObject,
								 void* Unused, UFunction* CallingUFunction,
								 void* Parameters, void* Result);

extern ProcessEventPrototype OriginalProcessEventFunction;

using ProcessInternalPrototype = void(__fastcall*)(UObject* CallingUObject,
												   void* Unused, FFrame& Stack,
												   void* Result);

void __fastcall ProcessInternalHook(UObject* CallingUObject,
									void* Unused, FFrame& Stack,
									void* Result);

extern ProcessInternalPrototype OriginalProcessInternalFunction;

using ProcessInternalPrototype = void(__fastcall*)(UObject* CallingUObject,
												   void* Unused, FFrame& Stack,
												   void* Result);

void __fastcall ProcessInternalHook(UObject* CallingUObject,
									void* Unused, FFrame& Stack,
									void* Result);

extern ProcessInternalPrototype OriginalProcessInternalFunction;

using CallFunctionPrototype = void(__fastcall*)(UObject* CallingUObject,
												void* Unused, FFrame& Stack,
												void* Result, UFunction* CallingUFunction);

void __fastcall CallFunctionHook(UObject* CallingUObject,
								 void* Unused, FFrame& Stack,
								 void* Result, UFunction* CallingUFunction);

extern CallFunctionPrototype OriginalCallFunctionFunction;

//using GNativeFunctionPrototype = void(__fastcall*)(UObject* CallingUObject,
//												   void* Unused, FFrame& Stack,
//												   void* Result);

using GNativeFunctionPrototype = void(__fastcall*)(FFrame& Stack, void* Unused,
												   void* Result);

//struct GNativeFunctionHookInformation
//{
//public:
//	unsigned int m_Index{};
//	GNativeFunctionPrototype m_OriginalFunction{};
//};
//
//extern std::unordered_map<unsigned int, GNativeFunctionHookInformation> indexToGNativeHookInformation;

//template <int index>
//void __fastcall GNativeFunctionHook(UObject* CallingUObject,
//									void* Unused, FFrame& Stack,
//									void* Result)
//{
//	indexToGNativeHookInformation.at(index).m_OriginalFunction(CallingUObject, Unused, Stack, Result);
//}


//template <GNativeFunctionPrototype func, typename... Args>
//void __fastcall GNativeFunctionHook(Args&&... args)
//{
//	func(std::forward<Args>(args)...);
//}

//void __fastcall GNativeFunctionHook(UObject* CallingUObject,
//									void* Unused, FFrame& Stack,
//									void* Result);

extern std::unordered_map<unsigned short, GNativeFunctionPrototype> OriginalGNativeFunctionFromiNative;

enum class FunctionHookType { kUnknown, kPre, kPost, kPreAndPost };
enum class FunctionHookAbsorb { kUnknown, kDoNotAbsorb, kAbsorb };

template <typename UFunctionHookPrototype>
class UFunctionHooks
{
	class UFunctionHookInformation
	{
	public:
		std::string m_UFunctionAsString{};
		UFunction* m_UFunction{};
		UFunctionHookPrototype m_HookFunction{};
		FunctionHookType m_HookType{ FunctionHookType::kUnknown };
		FunctionHookAbsorb m_HookAbsorb{ FunctionHookAbsorb::kUnknown };
	};

	class UFunctionHooksInformation
	{
		using Hooks = std::vector<UFunctionHookInformation>;

	public:
		Hooks m_PreHooks{};
		Hooks m_PostHooks{};
		FunctionHookAbsorb m_HooksAbsorb{ FunctionHookAbsorb::kUnknown };
	};

	std::unordered_map<const UFunction*, UFunctionHooksInformation> m_UFunctionToHooksInformation{};

public:
	UFunctionHookPrototype m_OriginalFunction{};
	UFunctionHooks(UFunctionHookPrototype OriginalFunction) : m_OriginalFunction(OriginalFunction) {};

public:
	void AddHook(std::string&& UFunctionAsString, UFunctionHookPrototype HookFunction,
				 FunctionHookType HookType = FunctionHookType::kPre,
				 FunctionHookAbsorb HookAbsorb = FunctionHookAbsorb::kDoNotAbsorb)
	{
		auto ufunctionObject = reinterpret_cast<UFunction*>(UObject::FindObject<UFunction>(UFunctionAsString.c_str()));
		if (ufunctionObject)
		{
			PLOG_INFO << "Found UFunction: " << UFunctionAsString;
			UFunctionHookInformation ufunctionHookInformation{ UFunctionAsString, ufunctionObject,
																HookFunction, HookType, HookAbsorb };
			switch (HookType)
			{
				case FunctionHookType::kPre:
				{
					m_UFunctionToHooksInformation[ufunctionObject].m_PreHooks.push_back(ufunctionHookInformation);
					break;
				}
				case FunctionHookType::kPost:
				{
					m_UFunctionToHooksInformation[ufunctionObject].m_PostHooks.push_back(ufunctionHookInformation);
					break;
				}
				case FunctionHookType::kPreAndPost:
				{
					m_UFunctionToHooksInformation[ufunctionObject].m_PreHooks.push_back(ufunctionHookInformation);
					m_UFunctionToHooksInformation[ufunctionObject].m_PostHooks.push_back(ufunctionHookInformation);
					break;
				}
			}

			if (HookAbsorb == FunctionHookAbsorb::kAbsorb)
			{
				PLOG_WARNING << "Added an abosrbing hook to UFunction: " << UFunctionAsString;
				m_UFunctionToHooksInformation[ufunctionObject].m_HooksAbsorb = FunctionHookAbsorb::kAbsorb;
			}

		}
		else
		{
			PLOG_ERROR << "Failed to find UFunction: " << UFunctionAsString;
		}
	}

	template<typename... Args>
	const void ExecuteHook(UFunction* ufunction, Args&&... args)
	{
		//if (!m_OriginalFunction)
		//	return;

		auto hooks = GetHooks(ufunction);
		bool absorbs = false;

		if (hooks)
		{
			auto& hooksRef = *hooks;
			absorbs = hooksRef.m_HooksAbsorb == FunctionHookAbsorb::kAbsorb;
			for (auto& ufunctionHookInformation : hooksRef.m_PreHooks)
			{
				ufunctionHookInformation.m_HookFunction(args...);
			}
		}

		if (m_OriginalFunction && !absorbs)
			m_OriginalFunction(std::forward<Args>(args)...);

		if (hooks)
		{
			auto& hooksRef = *hooks;
			for (auto& ufunctionHookInformation : hooksRef.m_PostHooks)
			{
				ufunctionHookInformation.m_HookFunction(args...);
			}
		}
	}

private:
	const UFunctionHooksInformation* GetHooks(const UFunction* UFunctionObject)
	{
		if ((UFunctionObject) && (m_UFunctionToHooksInformation.contains(UFunctionObject)))
		{
			return &m_UFunctionToHooksInformation[UFunctionObject];
		}
		else
		{
			return nullptr;
		}
	}
};

extern UFunctionHooks<ProcessEventPrototype> ProcessEventHooks;
extern UFunctionHooks<ProcessInternalPrototype> ProcessInternalHooks;
extern UFunctionHooks<CallFunctionPrototype> CallFunctionHooks;