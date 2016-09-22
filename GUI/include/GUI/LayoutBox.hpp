#pragma once
#include "GUIElement.hpp"

/*
Container that arranges elements in a vertical/horizontal list
*/
class LayoutBox : public GUIElementBase
{
public:
	~LayoutBox();
	virtual void PreRender(GUIRenderData rd, GUIElementBase*& inputElement) override;
	virtual void Render(GUIRenderData rd) override;
	virtual Vector2 GetDesiredSize(GUIRenderData rd) override;

	// Calculates child element sizes based on the current settings
	Vector<float> CalculateSizes(const GUIRenderData& rd) const;

	enum LayoutDirection
	{
		Horizontal,
		Vertical
	};

	// The layout direction of the box
	LayoutDirection layoutDirection = LayoutDirection::Horizontal;

	class Slot : public GUISlotBase
	{
	public:
		// If true will stretch all the elements to take up equal space
		//	otherwise they will get put after each other
		bool fillX = false;
		bool fillY = false;
		// The amount of space to fill
		//	0.5 would take up half of what it would normally
		//	this only works if this slot is set to fill in the container's direction
		float fillAmount = 1.0f;

		Vector2 alignment = Vector2(0.0f, 0.0f);

		virtual void PreRender(GUIRenderData rd, GUIElementBase*& inputElement);
		virtual void Render(GUIRenderData rd) override;
	};

	Slot* Add(GUIElement element);
	void Remove(GUIElement element);
	void Clear();

	const Vector<LayoutBox::Slot*>& GetChildren();
private:
	Vector<Slot*> m_children;
};