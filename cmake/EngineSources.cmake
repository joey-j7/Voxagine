# Engine translation units.
#
# Generated from the tree and then maintained by hand: the Linux port adds and
# removes files often enough that a GLOB would silently pick up half-ported
# code. Keep this list sorted.

set(VOXAGINE_ENGINE_SOURCES
    ${VOXAGINE_SOURCE_DIR}/Core/Application.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Input/SDL/SDLGamePad.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Input/SDL/SDLKeyboard.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Input/SDL/SDLMouse.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Component.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Components/AudioPlaylist.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Components/AudioSource.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Components/BehaviorScript.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Components/BoxCollider.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Components/ChunkViewer.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Components/Collider.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Components/InputHandler.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Components/Particles/Emitters/BoxEmitter.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Components/Particles/Emitters/SphereEmitter.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Components/Particles/Emitters/VoxFrameEmitter.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Components/Particles/Modules/AttractorModule.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Components/Particles/Modules/BasicTimerModule.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Components/Particles/Modules/CollisionModule.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Components/Particles/ParticleEmitter.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Components/Particles/ParticlePool.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Components/Particles/ParticleSystem.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Components/PhysicsBody.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Components/SpriteRenderer.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Components/TextRenderer.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Components/Transform.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Components/UI/UIButton.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Components/UI/UIComponent.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Components/UI/UISlider.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Components/VoxAnimator.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Components/VoxRenderer.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/ComponentSystem.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Entities/Camera.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Entities/UI/Canvas.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Entities/ViewPoint.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Entity.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Systems/AudioSystem.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Systems/Chunk/Chunk.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Systems/Chunk/ChunkSystem.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Systems/Chunk/ChunkUpdateGroup.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Systems/Pathfinding/Grid/PathfindingChunk.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Systems/Pathfinding/Grid/PathfindingChunkGrid.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Systems/Pathfinding/Grid/PathfindingNode.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Systems/Pathfinding/Grid/PathfindingObstacle.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Systems/Pathfinding/Jobs/ChunkBuilderJob.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Systems/Pathfinding/Navigation/ContinuumCrowdsGroup.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Systems/Pathfinding/Navigation/Pathfinder.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Systems/Pathfinding/Navigation/PathfinderGoal.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Systems/Pathfinding/Navigation/PathfinderGroup.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Systems/Physics/Box.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Systems/Physics/IntegrityJob.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Systems/Physics/ParticleLinkedList.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Systems/Physics/PhysicsSystem.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Systems/Physics/Sphere.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Systems/Physics/VoxelGrid.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Systems/Rendering/DebugRenderer.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Systems/Rendering/RenderSystem.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Systems/Rendering/VoxelBaker.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/Systems/ScriptSystem.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/World.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/ECS/WorldManager.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/FileBrowser.cpp

    # Both of these shipped as a header plus a Windows .lib, so there was
    # nothing to link against. TeenyPath is reimplemented on std::filesystem
    # and is portable; the file dialogs are per-platform and selected below.
    ${VOXAGINE_SOURCE_DIR}/External/teenypath/teenypath.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/JsonSerializer.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/LoggingSystem/LoggingSystem.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Memory/Allocators/BaseAlloc.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Memory/Allocators/FreeListAlloc.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Memory/Allocators/LinearAlloc.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Memory/Allocators/PoolAlloc.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Memory/Allocators/StackAlloc.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Objects/TSubclass.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Objects/VClass.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Audio/AudioContext.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Audio/NullAudioContext.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Resources/Formats/NullSoundReference.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Input/InputContext.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Input/Temp/GamePadController.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Input/Temp/InputBindingAction.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Input/Temp/InputBindingAxis.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Input/Temp/InputBindingBase.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Input/Temp/InputBindingHandlerInterface.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Input/Temp/InputBindingMap.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Input/Temp/InputBindingMapInterface.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Input/Temp/InputContextNew.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Input/Temp/InputController.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Input/Temp/KeyboardController.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Input/Temp/KeyboardControllerInterface.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Input/Temp/MouseController.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Input/Temp/MouseControllerInterface.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Input/Temp/PlayerController.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Platform.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Rendering/CommandEngine.cpp
    # FrameProfiler.cpp lives in the voxagine_vulkan target instead (see
    # CMakeLists.txt) - voxagine links that publicly, so it's still callable
    # from here, without compiling it into both libraries.
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Rendering/Managers/IDManager.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Rendering/Managers/ModelManager.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Rendering/Managers/TextureManager.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Rendering/Objects/Buffer.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Rendering/Passes/DebugPass.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Rendering/Passes/ParticlePass.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Rendering/Passes/PostProcessingPass.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Rendering/Passes/UIPass.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Rendering/Passes/VoxelBakePass.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Rendering/Passes/VoxelPass.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Rendering/RenderContext.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Rendering/Vulkan/Managers/VKModelManager.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Rendering/Vulkan/Managers/VKTextureManager.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Rendering/Vulkan/VKRenderContext.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Rendering/Vulkan/VKComputePass.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Rendering/Vulkan/VKRenderPass.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Rendering/Vulkan/Objects/VKBuffer.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Rendering/Vulkan/Objects/VKMapper.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Rendering/Vulkan/Objects/VKSampler.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Rendering/Vulkan/Objects/VKShader.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Rendering/Vulkan/Objects/VKView.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Time/Chrono/ChronoGameTimer.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Window/SDL/SDLWindowContext.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Window/WindowContext.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/PlayerPrefs/PlayerPrefs.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Resources/Formats/ShaderReference.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Resources/Formats/TextureReference.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Resources/Formats/VoxModel.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Resources/ResourceManager.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Settings.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/System/Posix/PosixFileSystem.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Threading/Job.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Threading/JobManager.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Threading/JobQueue.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Threading/JobThread.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Utils/DataHook/DataHook.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Utils/Utils.cpp
    ${VOXAGINE_SOURCE_DIR}/Editor/Configuration/BaseSettings.cpp
    ${VOXAGINE_SOURCE_DIR}/Editor/Configuration/ConfigurationWindow.cpp
    ${VOXAGINE_SOURCE_DIR}/Editor/Configuration/Editor/EditorConfigurationWindow.cpp
    ${VOXAGINE_SOURCE_DIR}/Editor/Configuration/Editor/UserSettings.cpp
    ${VOXAGINE_SOURCE_DIR}/Editor/Configuration/Project/ProjectConfigurationWindow.cpp
    ${VOXAGINE_SOURCE_DIR}/Editor/Configuration/Project/ProjectSettings.cpp
    ${VOXAGINE_SOURCE_DIR}/Editor/Configuration/WorldCreateConfig.cpp
    ${VOXAGINE_SOURCE_DIR}/Editor/ConsoleLog/ConsoleLog.cpp
    ${VOXAGINE_SOURCE_DIR}/Editor/EditorButton.cpp
    ${VOXAGINE_SOURCE_DIR}/Editor/EditorCamera.cpp
    ${VOXAGINE_SOURCE_DIR}/Editor/Editor.cpp
    ${VOXAGINE_SOURCE_DIR}/Editor/EditorReferenceManager.cpp
    ${VOXAGINE_SOURCE_DIR}/Editor/EditorRenderMapper.cpp
    ${VOXAGINE_SOURCE_DIR}/Editor/EditorWorld.cpp
    ${VOXAGINE_SOURCE_DIR}/Editor/EntityHierarchy/EntityHierarchy.cpp
    ${VOXAGINE_SOURCE_DIR}/Editor/EntityInspector/EntityInspector.cpp
    ${VOXAGINE_SOURCE_DIR}/Editor/EntityWizard/EntityWizard.cpp
    ${VOXAGINE_SOURCE_DIR}/Editor/imgui/ImguiSystem.cpp
    ${VOXAGINE_SOURCE_DIR}/Editor/imgui/Contexts/VKImContext.cpp
    ${VOXAGINE_SOURCE_DIR}/Editor/imgui/Platforms/SDLImPlatform.cpp
    ${VOXAGINE_SOURCE_DIR}/Editor/PropertyRenderer/PropertyRenderer.cpp
    ${VOXAGINE_SOURCE_DIR}/Editor/PropertyRenderer/TMap.cpp
    ${VOXAGINE_SOURCE_DIR}/Editor/SnappingTool/SnappingTool.cpp
    ${VOXAGINE_SOURCE_DIR}/Editor/UndoRedo/CommandManager.cpp
    ${VOXAGINE_SOURCE_DIR}/Editor/UndoRedo/EditorComponentCommand.cpp
    ${VOXAGINE_SOURCE_DIR}/Editor/UndoRedo/EditorEntityChildCommand.cpp
    ${VOXAGINE_SOURCE_DIR}/Editor/UndoRedo/EditorEntityCommand.cpp
    ${VOXAGINE_SOURCE_DIR}/Editor/UndoRedo/EditorFunctionCommand.cpp
    ${VOXAGINE_SOURCE_DIR}/Editor/UndoRedo/EditorPropertyCommand.cpp
    ${VOXAGINE_SOURCE_DIR}/Editor/UndoRedo/EditorSelectedEntityCommand.cpp
    ${VOXAGINE_SOURCE_DIR}/Editor/UndoRedo/EditorTransformMatrixCommand.cpp
    ${VOXAGINE_SOURCE_DIR}/Editor/Window.cpp
    ${VOXAGINE_SOURCE_DIR}/pch.cpp

    # Vendored sources compiled into the engine.
    #
    # glm is header-only. External/glm/detail/glm.cpp is its optional static
    # library TU and includes <glm/...> unqualified, which resolves to the
    # system glm 1.x and collides with the vendored 0.9.x. Never add it back.
    ${VOXAGINE_SOURCE_DIR}/External/imgui/imgui.cpp
    ${VOXAGINE_SOURCE_DIR}/External/imgui/imgui_demo.cpp
    ${VOXAGINE_SOURCE_DIR}/External/imgui/imgui_draw.cpp
    ${VOXAGINE_SOURCE_DIR}/External/imgui/imgui_dropdown.cpp
    ${VOXAGINE_SOURCE_DIR}/External/imgui/imgui_stdlib.cpp
    ${VOXAGINE_SOURCE_DIR}/External/imgui/imgui_stl.cpp
    ${VOXAGINE_SOURCE_DIR}/External/imgui/imgui_widgets.cpp
    ${VOXAGINE_SOURCE_DIR}/External/imguizmo/ImGuizmo.cpp
    ${VOXAGINE_SOURCE_DIR}/External/imguizmo/ImSequencer.cpp
    ${VOXAGINE_SOURCE_DIR}/External/STB/image_DXT.c
    ${VOXAGINE_SOURCE_DIR}/External/STB/image_helper.c
    ${VOXAGINE_SOURCE_DIR}/External/STB/stb_image_aug.c
)

# FMOD is a proprietary SDK downloaded by hand, so its translation units are
# only compiled when VOXAGINE_ENABLE_FMOD is on. NullAudioContext stands in
# otherwise and the engine runs silent.
set(VOXAGINE_FMOD_SOURCES
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Audio/FMODContext.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Resources/Formats/FMODSoundReference.cpp
)

# File dialogs are the one piece of the editor with no portable implementation:
# Win32 has GetOpenFileName, and on Linux we shell out to whatever portal-aware
# dialog the desktop provides rather than taking a GTK dependency.
if(WIN32)
    list(APPEND VOXAGINE_ENGINE_SOURCES
        ${VOXAGINE_SOURCE_DIR}/External/nativefiledialog/nfd_win32.cpp)
else()
    list(APPEND VOXAGINE_ENGINE_SOURCES
        ${VOXAGINE_SOURCE_DIR}/External/nativefiledialog/nfd_portal.cpp)
endif()
