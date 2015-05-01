#include "CGUIMiniMapWindow.h"
#include "EcoWorldRenderer.h"

CGUIMiniMapWindow::CGUIMiniMapWindow(scene::ISceneManager* smgr, EcoWorldRenderer* m_renderer, IGUIEnvironment* environment, IGUIElement* parent, int id, core::recti rectangle)
 : CGUIReductableWindow(environment, parent, id, rectangle), sceneManager(smgr), driver(environment->getVideoDriver()), renderer(m_renderer), camera(0)
{
#ifdef _DEBUG
	setDebugName("CGUIMiniMapWindow");
#endif

	// Indique le titre de cette fen�tre
	setText(L"Mini-carte");

	// Indique la taille minimale de cette fen�tre
	setMinSize(core::dimension2du(100, 100));

	// Masque le bouton de fermeture de la fen�tre
	if (CloseButton)
		CloseButton->setVisible(false);

	if (smgr)
	{
		// Cr�e la cam�ra de la mini carte de sorte que tout le terrain constructible soit dans sa ligne de mire
		camera = smgr->addCameraSceneNode(0,
			core::vector3df(0.0f, 150.0f * TAILLE_OBJETS, -5.0f), core::vector3df(0.0f, 0.0f, 5.0f),	// Les -5.0f et 5.0f servent � orienter la cam�ra en Z
			-1, false);
		if (camera)
		{
			// Calcule l'aspect ratio de la cam�ra suivant la zone qu'elle doit dessiner
			camera->setAspectRatio((float)(rectangle.getWidth() - 20) / (float)(rectangle.getHeight() - 40));

			// Modifie le FOV (champ de vision) de la cam�ra pour que la perspective ne se fasse pas trop ressentir
			//camera->setFOV(72.0f * core::DEGTORAD);	// Par d�faut : 72�

			// On doit retenir cette cam�ra pour �viter qu'elle ne soit d�truite par une remise � z�ro du scene manager
			camera->grab();
		}
	}
}
CGUIMiniMapWindow::~CGUIMiniMapWindow()
{
	if (camera)
		camera->drop();
}
void CGUIMiniMapWindow::OnWindowResize()
{
	// Recalcule l'aspect ratio de la cam�ra (avec un minimum de 0.5f et un maximum de 2.0f)
	camera->setAspectRatio(core::clamp((float)(RelativeRect.getWidth() - 20) / (float)(RelativeRect.getHeight() - 40), 0.5f, 2.0f));

	CGUIReductableWindow::OnWindowResize();
}
void CGUIMiniMapWindow::draw()
{
	// Dessine d'abord les �lements de base de cette fen�tre
	CGUIReductableWindow::draw();

	// V�rifie que tous les pointeurs n�cessaires sont valides, que la fen�tre n'est pas r�duite et qu'on est bien visible
	if (!sceneManager || !driver || !camera || isReducted || !isVisible())
		return;

	// Calcule le rectangle dans lequel sera dessin� la mini carte, en le limitant aux bords de l'�cran
	const core::dimension2du& screenSize = driver->getScreenSize();
	const core::recti miniMapRect(
		core::clamp(AbsoluteRect.UpperLeftCorner.X + 10, 0, (int)screenSize.Width),
		core::clamp(AbsoluteRect.UpperLeftCorner.Y + 30, 0, (int)screenSize.Height),	// AbsoluteRect.UpperLeftCorner.Y + 10 + titleBarSizeY = AbsoluteRect.UpperLeftCorner.Y + 30 avec titleBarSizeY = 20
		core::clamp(AbsoluteRect.LowerRightCorner.X - 15, 0, (int)screenSize.Width),
		core::clamp(AbsoluteRect.LowerRightCorner.Y - 15, 0, (int)screenSize.Height));

	// V�rifie que la zone o� sera dessin�e la mini carte n'est pas vide (ce qui peut arriver si la fen�tre est en dehors de l'�cran par exemple)
	if (miniMapRect.getArea() <= 0)
		return;

#ifdef USE_SPARK
	// Masque le syst�me de particules de SPARK
	const bool lastParticleSystemVisible = (renderer ? renderer->getSparkManager().isParticleSystemVisible() : false);
	if (renderer)
		renderer->getSparkManager().setParticleSystemVisible(false);
#endif

	// Se rend invisible (pour �viter d'�tre redessin�)
	setVisible(false);

	// Masque obligatoirement tous les noms au-dessus des b�timents
	const tribool lastNomsBatimentsVisible = renderer ? renderer->getNomsBatimentsVisible() : false;
	if (renderer)
		renderer->setNomsBatimentsVisible(false);

	// Modifie le view port du driver pour dessiner dans cette fen�tre
	const core::recti lastViewPort = driver->getViewPort();
	driver->setViewPort(miniMapRect);

	// Change la cam�ra active du scene manager pour dessiner avec notre nouvelle cam�ra
	scene::ICameraSceneNode* const lastCamera = sceneManager->getActiveCamera();
	camera->setNearValue(lastCamera->getNearValue());
	camera->setFarValue(lastCamera->getFarValue());
	sceneManager->setActiveCamera(camera);

	// Rend la lumi�re ambiante plus forte (plus la lumi�re actuelle est faible, plus on l'augmente)
	const video::SColorf lastAmbiantLight = sceneManager->getAmbientLight();
	{
		// Formule du calcul de la couleur ambiante finale : pour chaque composante x :
		// X = -0.5 * (x - 1)� + 1 = -0.5x� + x + 0.5
		float valeurs[3] = {(lastAmbiantLight.r - 1.0f), (lastAmbiantLight.g - 1.0f), (lastAmbiantLight.b - 1.0f)};
		for (int i = 0; i < 3; ++i)
			valeurs[i] = (-0.5f * valeurs[i] * valeurs[i]) + 1.0f;
		sceneManager->setAmbientLight(video::SColorf(valeurs[0], valeurs[1], valeurs[2]));
	}

	// D�sactive le brouillard (multiplie ses distances par 2)
	video::SColor lastFogColor;
	video::E_FOG_TYPE lastFogType;
	float lastFogStart, lastFogEnd, lastFogDensity;
	bool lastPixelFog, lastRangeFog;
	driver->getFog(lastFogColor, lastFogType, lastFogStart, lastFogEnd, lastFogDensity, lastPixelFog, lastRangeFog);
	driver->setFog(lastFogColor, lastFogType, max(lastFogStart * 2.0f, 500.0f), max(lastFogEnd * 2.0f, 2000.0f), lastFogDensity * 0.5f, lastPixelFog, lastRangeFog);

	// Dessine enfin la sc�ne
	driver->clearZBuffer();				// Efface le Z-Buffer pendant le nouveau rendu de la sc�ne (Attention : cela n�cessite que la sc�ne ne soit pas en train d'�tre rendue en m�me temps que cet �lement de la GUI)
	game->render(true, false, false);	// Dessine uniquement la sc�ne (pas la GUI), et sans r�initialiser le rendu actuel (Note : aucun effet post-rendu ne sera appliqu� au rendu de la mini-carte)

	// Restaure le brouillard et la lumi�re ambiante
	driver->setFog(lastFogColor, lastFogType, lastFogStart, lastFogEnd, lastFogDensity, lastPixelFog, lastRangeFog);
	sceneManager->setAmbientLight(lastAmbiantLight);

	// Restaure enfin la cam�ra du scene manager ainsi que le view port du driver
	sceneManager->setActiveCamera(lastCamera);
	driver->setViewPort(lastViewPort);

	// Restaure l'�tat des noms au-dessus des b�timents
	if (renderer)
		renderer->setNomsBatimentsVisible(lastNomsBatimentsVisible);

	// Et on se remet visible
	setVisible(true);

#ifdef USE_SPARK
	// Restaure le syst�me de particules de SPARK
	if (renderer)
		renderer->getSparkManager().setParticleSystemVisible(lastParticleSystemVisible);
#endif
}
