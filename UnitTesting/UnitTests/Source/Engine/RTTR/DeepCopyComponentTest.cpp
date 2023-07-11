/*
	!!!Deep copy test executed on all Components within the core engine!!!

	For every custom Component class derived from the base Component class
	a unit test should be generated to check if deep copy is valid/possible
	without a risk of getting/generating memory leaks.
	The deep copy test is done by getting the type within RTTR and
	get the size in bytes. The next step is to get all the properties
	of the class to test and get the size in bytes of those props.
	The test expects the outcome of all the sizes of props combined
	to be equal with the size of the to tested class.

	1.	Add the include of the specific class to test in the includes list
	2.	Add the Macro function ("DEEPCOPYTESTCOMPONENT") with as argument the custom class type
*/
/*
// Below the includes can be seen which are a must
#include <gtest/gtest.h>
#include <External/rttr/type>
#include "DeepCopyTest.h"
#include "Core/ECS/Component.h"

// Test on the Base Component itself should always be run
DEEPCOPYTESTCOMPONENT(Component)

// Includes for the various Components should be added below
// #include "Core/ECS//Components/AudioSource.h" // TODO: Fix errors when including Component
#include "Core/ECS/Components/BehaviorScript.h"
#include "Core/ECS/Components/BoxCollider.h"
#include "Core/ECS/Components/ChunkViewer.h"
#include "Core/ECS/Components/Collider.h"
#include "Core/ECS/Components/InputHandler.h"
#include "Core/ECS/Components/ParticleSystem.h"
#include "Core/ECS/Systems/Pathfinding/Grid/PathfindingObstacle.h"
#include "Core/ECS/Systems/Pathfinding/Navigation/Pathfinder.h"
#include "Core/ECS/Systems/Pathfinding/Navigation/PathfinderGoal.h"
#include "Core/ECS/Components/PhysicsBody.h"
#include "Core/ECS/Components/SpriteRenderer.h"
#include "Core/ECS/Components/TextRenderer.h"
#include "Core/ECS/Components/Transform.h"
#include "Core/ECS/Components/VoxAnimator.h"
// #include "Core/ECS/Components/VoxRenderer.h" // TODO: Fix errors when including Component

// Below the macro functions should be called to run the deep copy test 
// COMPONENTTYPE = Should be the class type of the given Component */
/*
// DEEPCOPYTESTCOMPONENT(AudioSource)  TODO: Fix errors when including Component
DEEPCOPYTESTCOMPONENT(BehaviorScript)
DEEPCOPYTESTCOMPONENT(BoxCollider)
DEEPCOPYTESTCOMPONENT(ChunkViewer)
DEEPCOPYTESTCOMPONENT(Collider)
DEEPCOPYTESTCOMPONENT(InputHandler)
DEEPCOPYTESTCOMPONENT(ParticleSystem)
//DEEPCOPYTESTCOMPONENT(pathfinding::PathfindingObstacle)
//DEEPCOPYTESTCOMPONENT(pathfinding::Pathfinder)
//DEEPCOPYTESTCOMPONENT(pathfinding::PathfinderGoal)
DEEPCOPYTESTCOMPONENT(PhysicsBody)
DEEPCOPYTESTCOMPONENT(SpriteRenderer)
DEEPCOPYTESTCOMPONENT(TextRenderer)
DEEPCOPYTESTCOMPONENT(Transform)
DEEPCOPYTESTCOMPONENT(VoxAnimator)
// DEEPCOPYTESTCOMPONENT(VoxRenderer) TODO: Fix errors when including Component*/