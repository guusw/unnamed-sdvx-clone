#pragma once
#include "GUIElement.hpp"

/*
Container that arranges elements in a vertical/horizontal list
*/
class LayoutBox : public GUIElementBase
{
public:
	~LayoutBox();
	void Render(GUIRenderData rd) override;
	virtual bool GetDesiredSize(GUIRenderData rd, Vector2& sizeOut) override;

	// Calculates child element sizes based on the current settings
	Vector<float> CalculateSizes(const GUIRenderData& rd) const;

	enum LayoutDirection
	{
		Horizontal,
		Vertical
	};

	// The layout direction of the box
	LayoutDirection layoutDirection = LayoutDirection::Horizontal;

	// If true will stretch all the elements to take up equal space
	//	otherwise they will get put after each other
	bool fill = false;

	class Slot : public GUISlotBase
	{
	public:
		// Should fill all possible space? or just take up the wanted amount of space
		bool fill = false;
	};

	Slot* Add(GUIElement element);
	void Remove(GUIElement element);
	void Clear();

	const Vector<LayoutBox::Slot*>& GetChildren();
private:
	Vector<Slot*> m_children;
};