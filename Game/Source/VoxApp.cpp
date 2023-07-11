#include "VoxApp.h"
#include <iostream>
#include "Core/ECS/World.h"
#include "Core/Platform/Input/Temp/InputContextNew.h"

#include "Humanoids/Enemies/HordeMonster.h"

#ifndef EDITOR
	#include "Editor/Configuration/Project/ProjectSettings.h"
	#include "TestWorld.h"
#endif

void VoxApp::OnCreate()
{
	// Randomness
	srand(time(nullptr));

	// Initialize PlayerPrefs
	m_PlayerPrefs.Initialize(&m_Serializer, "PlayerPrefs.vgprefs");

#if !defined(EDITOR)
	auto projectSettings = ProjectSettings();
	projectSettings.Initialize(&m_Serializer, "ProjectSettings.vgps");
	std::string outPath = projectSettings.GetDefaultMap();

	if (outPath.empty())
	{
		outPath = std::string("Content/Worlds/Menus/Main_Menu.wld");
	}

	World* pStartWorld = new World(this);
	pStartWorld->PreLoad(outPath);

	m_WorldManager.LoadWorld(pStartWorld);
#endif

#ifndef _ORBIS
	m_Platform.GetInputContext()->RegisterAxis("MoveRight", IK_A, -1.f);
	m_Platform.GetInputContext()->RegisterAxis("MoveRight", IK_D, 1.f);
	m_Platform.GetInputContext()->RegisterAxis("MoveRight", IK_GAMEPADLEFTSTICKAXISX, 1.f);

	m_Platform.GetInputContext()->RegisterAxis("MoveForward", IK_S, -1.f);
	m_Platform.GetInputContext()->RegisterAxis("MoveForward", IK_W, 1.f);
	m_Platform.GetInputContext()->RegisterAxis("MoveForward", IK_GAMEPADLEFTSTICKAXISY, 1.f);

	m_Platform.GetInputContext()->RegisterAxis("RotateRight", IK_RIGHT, 1.f);
	m_Platform.GetInputContext()->RegisterAxis("RotateRight", IK_LEFT, -1.f);
	m_Platform.GetInputContext()->RegisterAxis("RotateRight", IK_GAMEPADRIGHTSTICKAXISX, 1.f);

	m_Platform.GetInputContext()->RegisterAxis("RotateUp", IK_UP, 1.f);
	m_Platform.GetInputContext()->RegisterAxis("RotateUp", IK_DOWN, -1.f);
	m_Platform.GetInputContext()->RegisterAxis("RotateUp", IK_GAMEPADRIGHTSTICKAXISY, 1.f);

	m_Platform.GetInputContext()->RegisterAction("Jump", IKS_HELD, IK_SPACE);
	m_Platform.GetInputContext()->RegisterAction("Jump", IKS_HELD, IK_GAMEPADRIGHTPADRIGHT);
	m_Platform.GetInputContext()->RegisterAction("Jump", IKS_RELEASED, IK_SPACE);
	m_Platform.GetInputContext()->RegisterAction("Jump", IKS_RELEASED, IK_GAMEPADRIGHTPADRIGHT);

	m_Platform.GetInputContext()->RegisterAction("Dash", IKS_PRESSED, IK_O);
	m_Platform.GetInputContext()->RegisterAction("Dash", IKS_PRESSED, IK_GAMEPADRIGHTPADLEFT);
	m_Platform.GetInputContext()->RegisterAction("Dash", IKS_PRESSED, IK_GAMEPADLEFTSHOULDER2);

	m_Platform.GetInputContext()->RegisterAction("Fire", IKS_PRESSED, IK_P);
	m_Platform.GetInputContext()->RegisterAction("Fire", IKS_PRESSED, IK_GAMEPADRIGHTPADDOWN);
	m_Platform.GetInputContext()->RegisterAction("Fire", IKS_PRESSED, IK_GAMEPADRIGHTSHOULDER2);

	m_Platform.GetInputContext()->RegisterAction("Special", IKS_PRESSED, IK_I);
	m_Platform.GetInputContext()->RegisterAction("Special", IKS_PRESSED, IK_GAMEPADRIGHTPADUP);
	m_Platform.GetInputContext()->RegisterAction("Special", IKS_PRESSED, IK_GAMEPADLEFTSHOULDER);

	m_Platform.GetInputContext()->RegisterAction("Reset", IKS_PRESSED, IK_U);
	m_Platform.GetInputContext()->RegisterAction("Reset", IKS_PRESSED, IK_GAMEPADRIGHTPADDOWN);
	m_Platform.GetInputContext()->RegisterAction("Reset", IKS_PRESSED, IK_GAMEPADRIGHTSHOULDER);

	m_Platform.GetInputContext()->RegisterAction("Escape", IKS_PRESSED, IK_ESCAPE);
	m_Platform.GetInputContext()->RegisterAction("Escape", IKS_PRESSED, IK_GAMEPADOPTION);

	m_Platform.GetInputContext()->RegisterAction("Pause_Game", IKS_PRESSED, IK_PAUSE);
	m_Platform.GetInputContext()->RegisterAction("Pause_Game", IKS_PRESSED, IK_ESCAPE);
	m_Platform.GetInputContext()->RegisterAction("Pause_Game", IKS_PRESSED, IK_GAMEPADOPTION);
	m_Platform.GetInputContext()->RegisterAction("Pause_Game", IKS_PRESSED, IK_GAMEPADSELECT);

	m_Platform.GetInputContext()->RegisterAction("Sprint", IKS_PRESSED, IK_M);
	m_Platform.GetInputContext()->RegisterAction("Sprint", IKS_RELEASED, IK_M);
#endif
}

void VoxApp::OnUpdate()
{

}

void VoxApp::OnDraw()
{

}

void VoxApp::OnExit()
{

}
