#pragma once

#include <vector>
#include <unordered_map>
#include <plog/Log.h>
#include "Tribes-Ascend-SDK/SdkHeaders.h"

using ProcessEventPrototype = void(__fastcall*)(UObject* CallingUObject,
												void* Unused, UFunction* CallingUFunction,
												void* Parameters, void* Result);
void __fastcall ProcessEventHook(UObject* CallingUObject,
								 void* Unused, UFunction* CallingUFunction,
								 void* Parameters, void* Result);

extern ProcessEventPrototype OriginalProcessEventFunction;

enum class FunctionHookType { kUnknown, kPre, kPost, kPreAndPost };
enum class FunctionHookAbsorb { kUnknown, kDoNotAbsorb, kAbsorb };

template <typename UFunctionHookPrototype>
class UFunctionHooks
{
	class UFunctionHookInformation
	{
	public:
		std::string	m_UFunctionAsString{};
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
	const void ExecuteHook(UFunction* ufunction, Args... args)
	{
		if (!m_OriginalFunction)
			return;

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
			m_OriginalFunction(args...);

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