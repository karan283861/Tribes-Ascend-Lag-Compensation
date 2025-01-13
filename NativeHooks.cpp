#include <format>
#include <unordered_map>
#include <plog/Log.h>
#include "NativeHooks.hpp"
#include "LagCompensation.hpp"
#include "Helper.hpp"

static auto projectileMoved{ false };
static AWorldInfo* worldInfo{ nullptr };
static LagCompensationTick latestLagCompensationTick{};

AActorMovePrototype OriginalAActorMove = reinterpret_cast<AActorMovePrototype>(MOVEACTOR_ADDRESS);
bool __fastcall AActor_Move_Hook(AActor* Actor, void* Unused, FVector Delta)
{
	PLOG_DEBUG << Actor->GetFullName();
	return OriginalAActorMove(Actor, Unused, Delta);
}

AActorSetLocationPrototype OriginalAActorSetLocation = reinterpret_cast<AActorSetLocationPrototype>(SETLOCATION_ADDRESS);
bool __fastcall AActor_SetLocation_Hook(AActor* Actor, void* Unused, FVector NewLocation)
{
	PLOG_DEBUG << Actor->GetFullName();
	return OriginalAActorSetLocation(Actor, Unused, NewLocation);
}

extern UWorldMoveActorPrototype OriginalUWorldMoveActor = reinterpret_cast<UWorldMoveActorPrototype>(UWORLD_MOVEACTOR_ADDRESS);
bool __fastcall UWorld_MoveActor_Hook(UWorld* World, void* Unused,
									   AActor* Actor,
									   const FVector& Delta,
									   const FRotator& NewRotation,
									   unsigned int			MoveFlags,
									   int& Hit)
{
	if (isClient)
	{
		return OriginalUWorldMoveActor(World, Unused, Actor, Delta, NewRotation, MoveFlags, Hit);
	}

	PLOG_DEBUG << Actor->GetFullName();

	if (Actor->IsA(ATrPlayerPawn::StaticClass()))
	{
		auto returnValue{ OriginalUWorldMoveActor(World, Unused, Actor, Delta, NewRotation, MoveFlags, Hit) };
		auto pawn = reinterpret_cast<ATrPlayerPawn*>(Actor);
		latestLagCompensationTick.m_PawnToLocation.insert_or_assign(pawn, pawn->Location);
		pawnToLocation.insert_or_assign(pawn, pawn->Location);
		//PLOG_DEBUG << std::format("Location\t{0}", FVectorToString(pawn->Location));
		//PLOG_DEBUG << std::format("Delta\t{0}", FVectorToString(Delta));
		return returnValue;
	}
	else if (Actor->IsA(ATrProjectile::StaticClass()))
	{
		/*auto valid{ true };*/
		if (!projectileMoved)
		{
			projectileMoved = true;
			lagCompensationBuffer.push_back(latestLagCompensationTick);
		}

		auto gameProjectile{ reinterpret_cast<ATrProjectile*>(Actor) };
		auto controller{ GetProjectileOwner(gameProjectile) };

		if (!controller)
		{
			PLOG_ERROR << "Projectile is not owned by a player";
			return OriginalUWorldMoveActor(World, Unused, Actor, Delta, NewRotation, MoveFlags, Hit);
		}

		auto pingInMS{ GetProjectilePingInMS(gameProjectile)};
		if (pingInMS < 0)
		{
			PLOG_ERROR << "Projectile controller ping is invalid";
			return OriginalUWorldMoveActor(World, Unused, Actor, Delta, NewRotation, MoveFlags, Hit);
		}

#ifdef _DEBUG
		if (DEBUG_PING != 0)
		{
			pingInMS = DEBUG_PING;
		}
#endif

		auto lagCompensationTick = GetLagCompensationTick(pingInMS);
		auto previousLagCompensationTick = GetPreviousLagCompensationTick(pingInMS);
		if (lagCompensationTick && previousLagCompensationTick)
		{
			MovePawns(pingInMS);
			auto returnValue{ OriginalUWorldMoveActor(World, Unused, Actor, Delta, NewRotation, MoveFlags, Hit) };
			RestorePawns(pingInMS);
			return returnValue;
		}
	}
	
	return OriginalUWorldMoveActor(World, Unused, Actor, Delta, NewRotation, MoveFlags, Hit);
}

extern UGameEngineTickPrototype OriginalUGameEngineTick = reinterpret_cast<UGameEngineTickPrototype>(UGAMEENGINE_TICK_ADDRESS);
void __fastcall UGameEngine_Tick_Hook(UGameEngine* GameEngine, void* Unused, float DeltaSeconds)
{
	PLOG_DEBUG << std::format("GameEngine = {0}", static_cast<void*>(GameEngine));
	projectileMoved = false;
	pawnToLocation.clear();
	latestLagCompensationTick = LagCompensationTick();
	worldInfo = GameEngine->GetCurrentWorldInfo();
	OriginalUGameEngineTick(GameEngine, Unused, DeltaSeconds);
	if (!projectileMoved && !isClient)
	{
		lagCompensationBuffer.push_back(latestLagCompensationTick);
	}
}

extern UWorldTickPrototype OriginalUWorldTick = reinterpret_cast<UWorldTickPrototype>(UWORLD_TICK_ADDRESS);
void __fastcall UWorld_Tick_Hook(UWorld* World, void* Unused, unsigned int TickType, float DeltaSeconds)
{
	PLOG_DEBUG << std::format("World = {0}", static_cast<void*>(World));
	return OriginalUWorldTick(World, Unused, TickType, DeltaSeconds);
}
