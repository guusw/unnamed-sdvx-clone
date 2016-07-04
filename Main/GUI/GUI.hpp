#pragma once
#include "GUIRenderData.hpp"

/*
	Base class for GUI elements
*/
class GUIElementBase : public Unique, public RefCounted<GUIElementBase>
{
public:
	GUIElementBase() = default;
	virtual ~GUIElementBase() = default;
	// Called to draw the GUI element and it's children
	virtual void Render(GUIRenderData rd);
	// Calculates the desired size of this element, or false if it does not
	virtual bool GetDesiredSize(GUIRenderData rd, Vector2& sizeOut);

	// The slot that contains this element
	class GUISlotBase* slot = nullptr;

protected:
	// Template slot creation helper
	template<typename T> T* CreateSlot(Ref<GUIElementBase> element);
	// Called when added to slot
	virtual void m_AddedToSlot(GUISlotBase* slot);
	// Called when the ZOrder of a child slot changed
	virtual void m_OnZOrderChanged(GUISlotBase* slot);

	friend class GUISlotBase;
};

// Typedef pointer to GUI element objects
typedef Ref<GUIElementBase> GUIElement;

// Fill mode for gui elements
enum class FillMode
{
	None = 0,
	// Stretches the element, may result in incorrect image ratio
	Stretch = 0,
	// Fills the entire space with the content, may crop the element
	Fill,
	// Take the smallest size to fit the element, leaves black bars if it doesn't fit completely
	Fit,
};

/*
	Base class used for slots that can contain child elements inside an existing element.
	The slot class contains specific properties of how to layout the child element in its parent
*/
class GUISlotBase : public Unique
{
public:
	virtual ~GUISlotBase() = default;
	virtual void Render(GUIRenderData rd);
	virtual bool GetDesiredSize(GUIRenderData rd, Vector2& sizeOut);
	// Applies filling logic based on the selected fill mode
	Rect ApplyFill(const Vector2& inSize, const Rect& rect);
	// Applies filling logic based on the selected fill mode
	static Rect ApplyFill(FillMode fillMode, const Vector2& inSize, const Rect& rect);

	// Element that contains this slot
	GUIElementBase* parent = nullptr;
	// Padding that is applied to the element after it is fully placed
	Margin padding;
	// The element contained in this slot
	GUIElement element;
	// Fill mode of the element
	FillMode fillMode = FillMode::None;

	void SetZOrder(int32 zorder);
	int32 GetZOrder() const;

private:
	// Depth sorting for this slot, if applicable
	int32 m_zorder = 0;
};

class Panel : public GUIElementBase
{
public:
	void Render(GUIRenderData rd) override;

	// Calculates the image size if set
	bool GetDesiredSize(GUIRenderData rd, Vector2& sizeOut) override;

	class Slot : public GUISlotBase
	{
	};

	Color color = Color::White;
	Texture texture;

private:
	Slot* m_content = nullptr;
};

class Label : public GUIElementBase
{
public:
	void Render(GUIRenderData rd) override;
	bool GetDesiredSize(GUIRenderData rd, Vector2& sizeOut) override;

	// The text displayed
	WString GetText() const;
	void SetText(const WString& text);

	// The size(height) of the displayed text
	uint32 GetFontSize() const;
	void SetFontSize(uint32 size);

	// Color of the text
	Color color = Color::White;

private:
	void m_UpdateText(class GUIRenderer* renderer);

	bool m_dirty = true;
	// Text object that is displayed
	Text m_text;
	// Text string that is displayed
	WString m_textString;
	// Font override
	Font m_font;
	uint32 m_fontSize = 16;
};

class BoxSlotBase : public GUISlotBase
{
public:
	virtual void Render(GUIRenderData rd) override;
	virtual bool GetDesiredSize(GUIRenderData rd, Vector2& sizeOut) override;
	Rect ApplyAlignment(const Rect& rect, const Rect& parent);

	// Alignment of the element in the parent.
	// (0,0) Aligns to top-left
	// (1,0) Aligns to top-right
	// (0,1) Aligns to bottom-left
	Vector2 alignment = Vector2(0.5f, 0.5f);
};

/*
	Container that arranges elements in a vertical list
*/
class VerticalBox : public GUIElementBase
{
public:
	void Render(GUIRenderData rd) override;
	virtual bool GetDesiredSize(GUIRenderData rd, Vector2& sizeOut) override;
	void Add(GUIElement element);
	void Remove(GUIElement element);

	class Slot : public BoxSlotBase
	{
	};

	// If true will stretch all the elements to take up equal space
	//	otherwise they will get put after each other
	bool fill = false;

	const Vector<VerticalBox::Slot*>& GetChildren();

private:
	Vector<Slot*> m_children;
};

/*
	Canvas that sorts elements by depth and anchors them to the screen
*/
class Canvas : public GUIElementBase
{
public:
	~Canvas();
	void Render(GUIRenderData rd) override;
	void Add(GUIElement element);
	void Remove(GUIElement element);

	class Slot : public GUISlotBase
	{
	public:
		virtual void Render(GUIRenderData rd) override;

		Vector2 pos;
		Vector2 size;
		// if set to true, the size of the child element will be used instead
		bool autoSize = false;

		// Anchor in the canvas, this is added to the position but works using [0,1] ranged values
		// works in a similar way to alignment
		Vector2 anchor = Vector2(0, 0);

		// Alignment of the element in the parent.
		//	a value of (0,0) places the widget starting from the top-left corner extending to bottom-right
		//	a value of(1,0) places the widget starting from the top-right corner extending to bottom-left
		Vector2 alignment;
	};

	const Vector<Canvas::Slot*>& GetChildren();

protected:
	virtual void m_OnZOrderChanged(GUISlotBase* slot);

private:
	void m_SortChildren();

	Vector<Slot*> m_children;
};

// Slot creation helper
template<typename T> T* GUIElementBase::CreateSlot(Ref<GUIElementBase> element)
{
	static_assert(std::is_base_of<GUISlotBase, T>::value, "Class does not inherit from GUISlotBase");

	assert(element->slot == nullptr);
	T* newSlot = new T();
	newSlot->parent = this;
	newSlot->element = element;
	element->slot = newSlot;
	return newSlot;
}