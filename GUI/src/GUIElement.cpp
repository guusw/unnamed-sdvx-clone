#include "stdafx.h"
#include "GUI.hpp"
#include "GUIRenderer.hpp"
#include <Shared/Color.hpp>
#include <Shared/Color.hpp>

GUIElementBase::GUIElementBase()
{
	renderTransform.Notify = [&]()
	{
		InvalidationEvent evt;
		evt.Propagate(*this);
	};
}
GUIElementBase::~GUIElementBase()
{
}
void GUIElementBase::InvalidateArea()
{
	m_transformValid.Invalidate();
}
void GUIElementBase::OnInvalidate(InvalidationEvent& event)
{
	if(event.invalidateArea)
	{
		InvalidateArea();
	}
}
void GUIElementBase::OnAssignGUI(AssignGUIEvent& event)
{
	m_gui = event.gui;
}
void GUIElementBase::UpdateAnimations(float deltaTime)
{
	bool update = false;
	for(auto it = m_animations.begin(); it != m_animations.end();)
	{
		auto anim = *it;
		bool done = !anim->Update(deltaTime);
		if(done)
		{
			anim.Destroy();
			it = m_animations.erase(it);
			continue;
		}
		it++;
		update = true;
	}
	if(update)
		m_PostAnimationUpdate();
}
void GUIElementBase::Update(GUIUpdateData data)
{
	m_UpdateTransform(data.area, data.renderTransform);
}
Vector2 GUIElementBase::m_GetDesiredBaseSize(GUIUpdateData data)
{
	return Vector2();
}
Vector2 GUIElementBase::GetDesiredSize(GUIUpdateData data)
{
	if(visibility == Visibility::Collapsed)
		return Vector2();
	return m_GetDesiredBaseSize(data);
}
Rect GUIElementBase::GetCachedArea() const
{
	return m_area;
}
Ref<GUIAnimation> GUIElementBase::AddAnimation(Action<void, float> anim, float duration, Interpolation::TimeFunction timeFunction)
{
	if(m_animationsByUpdater.Contains(anim))
	{
		// Remove old animation
		RemoveAnimation(anim);
	}

	Ref<GUIAnimation> newAnimation = *new GUIAnimation(anim, duration, timeFunction);
	m_animations.Add(newAnimation);
	m_animationsByUpdater.Add(anim, newAnimation);
	return newAnimation;
}
void GUIElementBase::RemoveAnimation(Action<void, float> anim)
{
	auto foundAnim = m_animationsByUpdater.Find(anim);
	if(foundAnim)
	{
		m_animations.erase(foundAnim[0]);
		m_animationsByUpdater.erase(anim);
	}
}
void GUIElementBase::RemoveAnimation(Ref<GUIAnimation> anim)
{
	if(m_animations.Contains(anim))
	{
		m_animationsByUpdater.erase(anim->updater);
		m_animations.erase(anim);
	}
}
Ref<GUIAnimation> GUIElementBase::FindAnimation(Action<void, float> updater)
{
	auto found = m_animationsByUpdater.Find(updater);
	if(found)
		return *found;
	return Ref<GUIAnimation>();
}
GUISlotBase* GUIElementBase::GetSlot()
{
	return m_slot;
}
ContainerBase* GUIElementBase::GetParent()
{
	return m_slot ?  m_slot->parent : nullptr;
}
void GUIElementBase::m_AttachChild(GUIElementBase& element, GUISlotBase* slot)
{
	element.m_slot = slot;
	AssignGUIEvent evt;
	evt.gui = m_gui;
	evt.Propagate(element);
}
void GUIElementBase::m_UpdateTransform(Rect area, Transform2D parentTransform)
{
	if(!m_transformValid.IsValid())
	{
		m_area = area;
		m_renderTransform = renderTransform.Get() * parentTransform;

		m_objectTransform = Transform2D::Translation(area.pos);
		m_objectTransform *= m_renderTransform;
		m_objectTransform *= Transform2D::Scale(area.size);
		m_transformValid.Update();
	}
}
void GUIElementBase::m_UpdateTransform(GUIUpdateData& data)
{
	m_UpdateTransform(data.area, data.renderTransform);
	data.renderTransform = m_renderTransform;
	data.area = m_area;
}
void GUIElementBase::m_AddedToSlot(GUISlotBase* slot)
{
}
void GUIElementBase::m_OnRemovedFromParent()
{
	m_slot = nullptr;
	AssignGUIEvent evt;
	evt.gui = nullptr;
	evt.Propagate(*this);
}
