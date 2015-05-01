#ifndef DEF_C_GUI_MINI_MAP_WINDOW
#define DEF_C_GUI_MINI_MAP_WINDOW

#include "global.h"
#include "CGUIReductableWindow.h"

using namespace gui;

class EcoWorldRenderer;

class CGUIMiniMapWindow : public CGUIReductableWindow
{
public:
	// Constructeur et destructeur
	CGUIMiniMapWindow(scene::ISceneManager* smgr, EcoWorldRenderer* m_renderer, IGUIEnvironment* environment, IGUIElement* parent, int id, core::recti rectangle);
	virtual ~CGUIMiniMapWindow();

	// Appel� lorsque la fen�tre a �t� redimensionn�e
	virtual void OnWindowResize();

	// Dessine cet �lement � l'�cran
	virtual void draw();

protected:
	scene::ISceneManager* sceneManager;
	video::IVideoDriver* driver;

	EcoWorldRenderer* renderer;

	// La nouvelle cam�ra avec laquelle les rendus dans la mini carte seront effectu�s
	scene::ICameraSceneNode* camera;
};

#endif
