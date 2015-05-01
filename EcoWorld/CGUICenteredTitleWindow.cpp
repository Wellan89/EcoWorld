#include "CGUICenteredTitleWindow.h"

CGUICenteredTitleWindow::CGUICenteredTitleWindow(IGUIEnvironment* environment, IGUIElement* parent, int id, const core::recti& rectangle)
: CGUIResizableWindow(environment, parent, id, rectangle)
{
#ifdef _DEBUG
	setDebugName("CGUICenteredTitleWindow");
#endif
}
void CGUICenteredTitleWindow::draw()
{
	// Basé sur la fonction draw() de la classe CGUIWindow d'Irrlicht SVN 1.8.0-alpha :

	if (IsVisible)
	{
		IGUISkin* const skin = Environment->getSkin();

		// update each time because the skin is allowed to change this always.
		updateClientRect();

		if ( CurrentIconColor != skin->getColor(isEnabled() ? EGDC_WINDOW_SYMBOL : EGDC_GRAY_WINDOW_SYMBOL) )
			refreshSprites();

		// draw body fast
		if (DrawBackground)
		{
			core::recti rect = skin->draw3DWindowBackground(this, DrawTitlebar,
					skin->getColor(IsActive ? EGDC_ACTIVE_BORDER : EGDC_INACTIVE_BORDER),
					AbsoluteRect, &AbsoluteClippingRect);

			if (DrawTitlebar && Text.size())
			{
				rect.UpperLeftCorner.X += skin->getSize(EGDS_TITLEBARTEXT_DISTANCE_X);
				rect.UpperLeftCorner.Y += skin->getSize(EGDS_TITLEBARTEXT_DISTANCE_Y);
				rect.LowerRightCorner.X -= skin->getSize(EGDS_WINDOW_BUTTON_WIDTH) + 5;

				IGUIFont* const font = skin->getFont(EGDF_WINDOW);
				if (font)
				{
					font->draw(Text.c_str(), rect,
							skin->getColor(IsActive ? EGDC_ACTIVE_CAPTION:EGDC_INACTIVE_CAPTION),
							true, true, &AbsoluteClippingRect);
				}
			}
		}
	}

	IGUIElement::draw();
}
