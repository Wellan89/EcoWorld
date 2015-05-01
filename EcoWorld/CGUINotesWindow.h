#ifndef DEF_NOTES_WINDOW
#define DEF_NOTES_WINDOW

#include "global.h"
#include "CGUIReductableWindow.h"

using namespace gui;

class CGUINotesWindow : public CGUIReductableWindow
{
public:
	CGUINotesWindow(IGUIEnvironment* environment, IGUIElement* parent, int id, core::recti rectangle);

	virtual void OnWindowResize();

protected:
	IGUIEditBox* notesEditBox;

	core::recti getAbsoluteRectangle(double x, double y, double x2, double y2)
	{
		return getAbsoluteRectangle(x, y, x2, y2, ClientRect);
	}
	core::recti getAbsoluteRectangle(double x, double y, double x2, double y2, core::recti parentRect)
	{
		parentRect.repair();

		return getAbsoluteRectangle(x, y, x2, y2, core::dimension2du(
				parentRect.LowerRightCorner.X - parentRect.UpperLeftCorner.X,
				parentRect.LowerRightCorner.Y - parentRect.UpperLeftCorner.Y));
	}
	core::recti getAbsoluteRectangle(double x, double y, double x2, double y2, core::dimension2du parentSize)
	{
		// Remet les valeurs entre 0.0 et 1.0
		//if (x < 0.0)	x = 0.0;
		//if (x > 1.0)	x = 1.0;
		//if (y < 0.0)	y = 0.0;
		//if (y > 1.0)	y = 1.0;
		//if (x2 < 0.0)	x2 = 0.0;
		//if (x2 > 1.0)	x2 = 1.0;
		//if (y2 < 0.0)	y2 = 0.0;
		//if (y2 > 1.0)	y2 = 1.0;

		return core::recti((int)(x * parentSize.Width), (int)(y * parentSize.Height),
			(int)(x2 * parentSize.Width), (int)(y2 * parentSize.Height));
	}

public:
	// True si le texte de notes a le focus
	bool isTextFocused() const { return (Environment->getFocus() == notesEditBox); }

	// Obtient le texte de notes
	const wchar_t* getNotesTexte() const { return notesEditBox->getText(); }
	void setNotesTexte(const wchar_t* text) { notesEditBox->setText(text); }
};

#endif
