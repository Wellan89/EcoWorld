// Modal screen dessin� gr�ce � une texture de fond, bas� sur la classe CGUIModalScreen d'Irrlicht 1.7.2

#ifndef DEF_C_GUI_TEXTURED_MODAL_SCREEN
#define DEF_C_GUI_TEXTURED_MODAL_SCREEN

#include "global.h"
#include "CGUIModalScreen.h"

using namespace gui;

class CGUITexturedModalScreen : public CGUIModalScreen
{
public:
	// Constructeur
	CGUITexturedModalScreen(IGUIElement* parent, int id, IGUIEnvironment* environment, IGUIElement* child = NULL,
		video::ITexture* backgroundTexture = NULL, bool useAlphaChannel = true, bool scale = true);

	virtual void draw();

protected:
	// L'�lement de la GUI qui nous permet de dessiner l'image du fond
	IGUIImage* backgroundImage;
};

#endif
