#pragma once

#include "Editor/UndoRedo/EditorFunctionCommand.h"

EditorFunctionCommand* CreateEditorTransformMatrixCommand(Transform* pTransform, Matrix4& rNewMatrix, Matrix4& rLastMatrix);