#pragma once

#include "Editor/UndoRedo/BaseCommand.h"

#include "External/rttr/type"

class World;
class Component;

class EditorPropertyCommand : public BaseCommand
{
public:
	EditorPropertyCommand(rttr::instance targetInstance, rttr::property targetProperty, rttr::variant redoValue);
	virtual ~EditorPropertyCommand();

	virtual void Redo();
	virtual void Undo();

private:
	void SetValue(rttr::variant& valueToSet);

private:
	World* m_pWorld = nullptr;
	uint64_t m_TargetEntityID = UINT_MAX;

	rttr::instance m_TargetInstance;

	rttr::type m_TargetType;
	rttr::property m_Property;
	rttr::variant m_RedoValue;
	rttr::variant m_UndoValue;
};