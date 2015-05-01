#ifndef DEF_C_GUI_CENTERED_TITLE_WINDOW
#define DEF_C_GUI_CENTERED_TITLE_WINDOW

#include "global.h"
#include "CGUIResizableWindow.h"

using namespace gui;

// Fen�tre redimensionnable dont le texte de la barre de titre est centr�, bas�e sur la classe CGUIWindow d'Irrlicht SVN 1.8.0-alpha
class CGUICenteredTitleWindow : public CGUIResizableWindow
{
public:
	// Constructeur
	CGUICenteredTitleWindow(IGUIEnvironment* environment, IGUIElement* parent, int id, const core::recti& rectangle);

	// Dessine cette fen�tre
	virtual void draw();
};

#endif
