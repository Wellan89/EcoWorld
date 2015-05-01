#ifndef DEF_C_GUI_CENTERED_TITLE_WINDOW
#define DEF_C_GUI_CENTERED_TITLE_WINDOW

#include "global.h"
#include "CGUIResizableWindow.h"

using namespace gui;

// Fenêtre redimensionnable dont le texte de la barre de titre est centré, basée sur la classe CGUIWindow d'Irrlicht SVN 1.8.0-alpha
class CGUICenteredTitleWindow : public CGUIResizableWindow
{
public:
	// Constructeur
	CGUICenteredTitleWindow(IGUIEnvironment* environment, IGUIElement* parent, int id, const core::recti& rectangle);

	// Dessine cette fenêtre
	virtual void draw();
};

#endif
