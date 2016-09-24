#pragma once
#include "GUIElement.hpp"

class Panel : public GUIElementBase
{
public:
	~Panel();
	virtual void PreRender(GUIRenderData rd, GUIElementBase*& inputElement) override;
	virtual void Render(GUIRenderData rd) override;

	// Calculates the image size if set
	Vector2 GetDesiredSize(GUIRenderData rd) override;

	class Slot : public GUISlotBase
	{
	public:
		virtual void PreRender(GUIRenderData rd, GUIElementBase*& inputElement);
		virtual void Render(GUIRenderData rd) override;
		virtual Vector2 GetDesiredSize(GUIRenderData rd) override;

		// Content alignment
		Vector2 alignment = Vector2(0.5f,0.5f);
	};

	// Sets panel content
	Slot* SetContent(GUIElement content);
	Slot* GetContentSlot();

	// Image fill mode
	FillMode imageFillMode = FillMode::Stretch;

	// Alignment of image when it's not stretcher or doesn't fite
	Vector2 imageAlignment = Vector2(0.5f, 0.5f);

	Color color = Color::White;
	Texture texture;

private:
	Slot* m_content = nullptr;
};