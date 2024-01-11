#include "PCH.h"
#include "Scene.h"
#include "Components/Settings/SimulationSettings.h"
#include "Components/Rendering/Camera.h"

namespace Scene
{
	////////////////////////////////////////////////////////////////////////////////
	ComponentTypes& componentTypes() { static ComponentTypes s_componentTypes; return s_componentTypes; };
	ObjectTypes& objectTypes() { static ObjectTypes s_objectTypes; return s_objectTypes; };
	ComponentConstructors& componentConstructors() { static ComponentConstructors s_componentConstructors; return s_componentConstructors; };
	ComponentNames& componentNames() { static ComponentNames s_componentNames; return s_componentNames; };
	ObjectNames& objectNames() { static ObjectNames s_objectNames; return s_objectNames; };
	ObjectInitializers& objectInitializers() { static ObjectInitializers s_objectInitializers; return s_objectInitializers; };
	ObjectReleasers& objectReleasers() { static ObjectReleasers s_objectReleasers; return s_objectReleasers; };
	ObjectDemoSceneSetupFunctions& objectDemoSceneSetupFunctions() { static ObjectDemoSceneSetupFunctions s_objectDemoSceneSetupFunctions; return s_objectDemoSceneSetupFunctions; }
	ObjectInputFunctions& objectInputFunctions() { static ObjectInputFunctions s_objectInputFunctions; return s_objectInputFunctions; };
	ObjectUpdateFunctions& objectUpdateFunctions() { static ObjectUpdateFunctions s_objectUpdateFunctions; return s_objectUpdateFunctions; };
	ObjectUpdateFunctionRegistrators& objectUpdateFunctionRegistrators() { static ObjectUpdateFunctionRegistrators s_objectUpdateFunctionRegistrators; return s_objectUpdateFunctionRegistrators; };
	ObjectRenderFunctions& objectRenderFunctionsOpenGL() { static ObjectRenderFunctions s_objectRenderFunctionsOpenGL; return s_objectRenderFunctionsOpenGL; };
	ObjectRenderFunctionRegistrators& objectRenderFunctionRegistratorsOpenGL() { static ObjectRenderFunctionRegistrators s_objectRenderFunctionRegistratorsOpenGL; return s_objectRenderFunctionRegistratorsOpenGL; };
	ObjectGuiCategories& objectGuiCategories() { static ObjectGuiCategories s_objectGuiCategories; return s_objectGuiCategories; };
	ObjectGuiGenerators& objectGuiGenerators() { static ObjectGuiGenerators s_objectGuiGenerators; return s_objectGuiGenerators; };
}