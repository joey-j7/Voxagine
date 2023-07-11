#include "VoxApp.h"
#include <iostream>
#include "Core/ECS/World.h"
#include "Core/Platform/Input/Temp/InputContextNew.h"

#ifndef EDITOR
#include "Editor/Configuration/Project/ProjectSettings.h"
#endif

void VoxApp::OnCreate()
{
#ifndef EDITOR
	auto projectSettings = ProjectSettings();
	projectSettings.Initialize(&m_Serializer, "ProjectSettings.vgps");
	const std::string outPath = projectSettings.GetDefaultMap();

	//auto NewPlayWorld = new World(this);
	//
	//if (!outPath.empty())
	//{
	//	Document EditorWorldDocument;
	//
	//	if (!NewPlayWorld->PreLoad(outPath))
	//	{
	//		m_LoggingSystem.Log(LOGLEVEL_ERROR, "Editor", "Can't load world file since its version is not supported, file path: " + outPath);
	//	}
	//}

	World* pTestWorld = new World(this);
	pTestWorld->PreLoad("Content/Worlds/Menus/Main_Menu.wld");

	// World* pTestWorld = new TestWorld(this);
	m_WorldManager.LoadWorld(pTestWorld);
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
	m_Platform.GetInputContext()->RegisterAxis("RotateRight", IK_GAMEPADRIGHTSTICKAXISY, 1.f);

	m_Platform.GetInputContext()->RegisterAction("Jump", IKS_HELD, IK_SPACE);
	m_Platform.GetInputContext()->RegisterAction("Jump", IKS_HELD, IK_GAMEPADRIGHTPADRIGHT);
	m_Platform.GetInputContext()->RegisterAction("Jump", IKS_RELEASED, IK_SPACE);
	m_Platform.GetInputContext()->RegisterAction("Jump", IKS_RELEASED, IK_GAMEPADRIGHTPADRIGHT);

	m_Platform.GetInputContext()->RegisterAction("Fire", IKS_HELD, IK_O);
	m_Platform.GetInputContext()->RegisterAction("Fire", IKS_HELD, IK_GAMEPADRIGHTSHOULDER);

	m_Platform.GetInputContext()->RegisterAction("Attack", IKS_PRESSED, IK_P);
	m_Platform.GetInputContext()->RegisterAction("Attack", IKS_PRESSED, IK_GAMEPADRIGHTPADDOWN);

	m_Platform.GetInputContext()->RegisterAction("LFire", IKS_RELEASED, IK_U);
	m_Platform.GetInputContext()->RegisterAction("LFire", IKS_RELEASED, IK_GAMEPADLEFTSHOULDER);

	m_Platform.GetInputContext()->RegisterAction("Escape", IKS_PRESSED, IK_ESCAPE);
	m_Platform.GetInputContext()->RegisterAction("Escape", IKS_PRESSED, IK_GAMEPADOPTION);

	m_Platform.GetInputContext()->RegisterAction("Sprint", IKS_PRESSED, IK_M);
	m_Platform.GetInputContext()->RegisterAction("Sprint", IKS_RELEASED, IK_M);

	m_Platform.GetInputContext()->RegisterAction("Dash", IKS_PRESSED, IK_N);

	m_Platform.GetInputContext()->RegisterAxis("AimRight", IK_F, -1.f);
	m_Platform.GetInputContext()->RegisterAxis("AimRight", IK_H, 1.f);
	m_Platform.GetInputContext()->RegisterAxis("AimRight", IK_GAMEPADRIGHTSTICKAXISX, 1.f);

	m_Platform.GetInputContext()->RegisterAxis("AimForward", IK_G, -1.f);
	m_Platform.GetInputContext()->RegisterAxis("AimForward", IK_T, 1.f);
	m_Platform.GetInputContext()->RegisterAxis("AimForward", IK_GAMEPADRIGHTSTICKAXISY, 1.f);

	m_Platform.GetInputContext()->RegisterAction("Fire2aim", IKS_PRESSED, IK_Y);
	m_Platform.GetInputContext()->RegisterAction("Fire2aim", IKS_PRESSED, IK_GAMEPADRIGHTSHOULDER2);
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
