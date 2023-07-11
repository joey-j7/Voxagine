/*
	!!!Deep copy test executed on all Entities within the core engine!!!

	For every custom Entity class derived from the base Entity class
	a unit test should be generated to check if deep copy is valid/possible
	without a risk of getting/generating memory leaks.
	The deep copy test is done by getting the type within RTTR and
	get the size in bytes. The next step is to get all the properties
	of the class to test and get the size in bytes of those props.
	The test expects the outcome of all the sizes of props combined
	to be equal with the size of the to tested class.

	1.	Add the include of the specific class to test in the includes list
	2.	Add the Macro function ("DEEPCOPYTESTENTITY") with as argument the custom class type
*/

// Below the includes can be seen which are a must
#include <gtest/gtest.h>
#include <External/rttr/type>
#include "DeepCopyTest.h"
#include "Core/ECS/Entity.h"

// Test on the Base Entity itself should always be run
DEEPCOPYTESTENTITY(Entity)

// Includes for the various Entities should be added below
#include "Core/ECS/Entities/Camera.h"

// Below the macro functions should be called to run the deep copy test 
// ENTITYTYPE = Should be the class type of the given Entity */

DEEPCOPYTESTENTITY(Camera);