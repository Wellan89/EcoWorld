#ifndef ___CGUITEXTBOX_H
#define ___CGUITEXTBOX_H

// Cette version a été légèrement modifiée

#include "global.h"
//#include <IRR/irrlicht.h>
//#include <wchar.h>

namespace irr
{
namespace gui
{
class CGUITextBox : public IGUIElement
{
public:
	// font needed to determine text size
	CGUITextBox(irr::gui::IGUIFont * font, const wchar_t * text,
		IGUIEnvironment* environment, core::rect<s32> rectangle,
		IGUIElement* parent, s32 id);

	//! Called if an event happened.
	virtual bool OnEvent(const SEvent& event);

	// Sets how the scrollbar is used.
	// true: One Step of the scrollbar is equal one line of the text
	void setScrollModeLines(bool bLines);

	bool isScrollModeLines(void);

	// Sets on which side the Scrollbar should be.
	// true: Scrolbar on the right of the Text.
	void setScrollbarRight(bool bRight);

	// true: Scrollbar is on the right of the Text.
	// false: on the left
	bool isScrollbarRight(void);	

	virtual void setText(const wchar_t* text);

	virtual void setRelativePosition(const core::rect<s32> r);
	void setVisibleText(s32 startLine);	// Déplacé : anciennement dans private
private:
	s32 calculateScrollbarSize(void);

	void reset(void);

   	IGUIScrollBar * m_pScrollbar;
	IGUIStaticText * m_pStaticText;
	IGUIStaticText * m_pDummyText; // just for the border

	IGUIFont * m_pFont;

	bool m_bScrollModeLines;
	bool m_bScrollbarRight;

	// Ajouté :
public:
	IGUIScrollBar* getScrollBar() { return m_pScrollbar; }
};

}	// end namespace gui
}	// end namespace irr

#endif
