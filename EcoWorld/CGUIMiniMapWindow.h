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

	// Appelé lorsque la fenêtre a été redimensionnée
	virtual void OnWindowResize();

	// Dessine cet élement à l'écran
	virtual void draw();

protected:
	scene::ISceneManager* sceneManager;
	video::IVideoDriver* driver;

	EcoWorldRenderer* renderer;

	// La nouvelle caméra avec laquelle les rendus dans la mini carte seront effectués
	scene::ICameraSceneNode* camera;
};

#endif
