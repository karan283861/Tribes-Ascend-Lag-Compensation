
# Tribes Ascend Lag Compensation Native
A server-side DLL mod for Tribes: Ascend which implements lag compensation for projectiles.

https://github.com/user-attachments/assets/b92043c7-0f10-4157-95c6-77f6ad96aef8

*A demonstration of playing with 400 ping. Note that projectiles are still drawn delayed on the client depending on ping.*

## What is Tribes: Ascend
Tribes: Ascend is a video game from the Tribes series released by Hi-Rez Studios in 2012. It uses Unreal Engine 3 and DirectX 9.

## Why Tribes Ascend Lag Compensation Native?
Tribes: Ascend is a first person shooter where gameplay heavily revolves around projectile bullets. These projectiles are *relatively* slow moving and thus require leading to hit a target.
There is no lag compensation implemented in Tribes: Ascend and thus projectiles are spawned depending on the ping of the shooter. As ping increases, the required leading for a projectile to hit a player becomes exponentially harder to predict, essentially making the game unplayable. This project aims to remedy this issue by effectively giving projectiles a ping of zero.

## What is Tribes Ascend Lag Compensation Native
A simple and straight forward implementation of lag compensation implemented by hooking native functions of Unreal Engine 3 through reverse engineering. The goal of this project to reduce the apparent ping of projectiles to zero.

### Tribes Ascend Lag Compensation Overlay
The original scope of this project was to write a complete lag compensation overlay which required

 - Hooking projectile construction and destruction UFunctions to keep a list of valid projectiles
 - Hooking a UFunction called once per game engine tick to
	 - keep a map of players and their respective states
	 - encapsulate player and player state map in a ring buffer
	 - nested loop through all projectiles and players to check for collisions
 
 The list above is an extreme simplification of the process. Without delving into the nitty gritty details, the lag compensation overlay idea was inefficient, intensive, mathematically complex and required handling numerous edge cases - at which point I might as well have worked on writing my own physics engine.

### Tribes Ascend Lag Compensation Native
Almost all the functionality I require has already been implemented within the game engine. By reverse engineering the game engine and locating specific native functions I can harness their power to implement a form of lag compensation that is much faster, efficient and shorter than the overlay project.

#### Features

 - [x] Lag compensation for all projectile direct hits
	 - [x] Linear projectiles
	 - [x] Arcing (gravity affected) projectiles
- [x] Lag compensation of radial damage (splash damage)

## Requirements
 -  Microsoft Visual C++ Redistributable 2022 (x86)

## Tools used
 - Visual Studio 2022
 - Ghidra
 - x64dbg
