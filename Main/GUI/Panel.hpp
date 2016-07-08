#pragma once
#include "GUIElement.hpp"

class Panel : public GUIElementBase
{
public:
	~Panel();
	void Render(GUIRenderData rd) override;

	// Calculates the image size if set
	bool GetDesiredSize(GUIRenderData rd, Vector2& sizeOut) override;

	class Slot : public GUISlotBase
	{
	public:
		virtual void Render(GUIRenderData rd);
		// Content alignment
		Vector2 alignment = Vector2(0.5f,0.5f);
	};

	// Sets panel content
	Slot* SetContent(GUIElement content);
	Slot* GetContentSlot();

	// Image fill mode
	FillMode imageFillMode = FillMode::Stretch;

	Color color = Color::White;
	Texture texture;

private:
	Slot* m_content = nullptr;
};