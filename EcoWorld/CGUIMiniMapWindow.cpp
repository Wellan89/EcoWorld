#include "CGUIMiniMapWindow.h"
#include "EcoWorldRenderer.h"

CGUIMiniMapWindow::CGUIMiniMapWindow(scene::ISceneManager* smgr, EcoWorldRenderer* m_renderer, IGUIEnvironment* environment, IGUIElement* parent, int id, core::recti rectangle)
 : CGUIReductableWindow(environment, parent, id, rectangle), sceneManager(smgr), driver(environment->getVideoDriver()), renderer(m_renderer), camera(0)
{
#ifdef _DEBUG
	setDebugName("CGUIMiniMapWindow");
#endif

	// Indique le titre de cette fenêtre
	setText(L"Mini-carte");

	// Indique la taille minimale de cette fenêtre
	setMinSize(core::dimension2du(100, 100));

	// Masque le bouton de fermeture de la fenêtre
	if (CloseButton)
		CloseButton->setVisible(false);

	if (smgr)
	{
		// Crée la caméra de la mini carte de sorte que tout le terrain constructible soit dans sa ligne de mire
		camera = smgr->addCameraSceneNode(0,
			core::vector3df(0.0f, 150.0f * TAILLE_OBJETS, -5.0f), core::vector3df(0.0f, 0.0f, 5.0f),	// Les -5.0f et 5.0f servent à orienter la caméra en Z
			-1, false);
		if (camera)
		{
			// Calcule l'aspect ratio de la caméra suivant la zone qu'elle doit dessiner
			camera->setAspectRatio((float)(rectangle.getWidth() - 20) / (float)(rectangle.getHeight() - 40));

			// Modifie le FOV (champ de vision) de la caméra pour que la perspective ne se fasse pas trop ressentir
			//camera->setFOV(72.0f * core::DEGTORAD);	// Par défaut : 72°

			// On doit retenir cette caméra pour éviter qu'elle ne soit détruite par une remise à zéro du scene manager
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
	// Recalcule l'aspect ratio de la caméra (avec un minimum de 0.5f et un maximum de 2.0f)
	camera->setAspectRatio(core::clamp((float)(RelativeRect.getWidth() - 20) / (float)(RelativeRect.getHeight() - 40), 0.5f, 2.0f));

	CGUIReductableWindow::OnWindowResize();
}
void CGUIMiniMapWindow::draw()
{
	// Dessine d'abord les élements de base de cette fenêtre
	CGUIReductableWindow::draw();

	// Vérifie que tous les pointeurs nécessaires sont valides, que la fenêtre n'est pas réduite et qu'on est bien visible
	if (!sceneManager || !driver || !camera || isReducted || !isVisible())
		return;

	// Calcule le rectangle dans lequel sera dessiné la mini carte, en le limitant aux bords de l'écran
	const core::dimension2du& screenSize = driver->getScreenSize();
	const core::recti miniMapRect(
		core::clamp(AbsoluteRect.UpperLeftCorner.X + 10, 0, (int)screenSize.Width),
		core::clamp(AbsoluteRect.UpperLeftCorner.Y + 30, 0, (int)screenSize.Height),	// AbsoluteRect.UpperLeftCorner.Y + 10 + titleBarSizeY = AbsoluteRect.UpperLeftCorner.Y + 30 avec titleBarSizeY = 20
		core::clamp(AbsoluteRect.LowerRightCorner.X - 15, 0, (int)screenSize.Width),
		core::clamp(AbsoluteRect.LowerRightCorner.Y - 15, 0, (int)screenSize.Height));

	// Vérifie que la zone où sera dessinée la mini carte n'est pas vide (ce qui peut arriver si la fenêtre est en dehors de l'écran par exemple)
	if (miniMapRect.getArea() <= 0)
		return;

#ifdef USE_SPARK
	// Masque le système de particules de SPARK
	const bool lastParticleSystemVisible = (renderer ? renderer->getSparkManager().isParticleSystemVisible() : false);
	if (renderer)
		renderer->getSparkManager().setParticleSystemVisible(false);
#endif

	// Se rend invisible (pour éviter d'être redessiné)
	setVisible(false);

	// Masque obligatoirement tous les noms au-dessus des bâtiments
	const tribool lastNomsBatimentsVisible = renderer ? renderer->getNomsBatimentsVisible() : false;
	if (renderer)
		renderer->setNomsBatimentsVisible(false);

	// Modifie le view port du driver pour dessiner dans cette fenêtre
	const core::recti lastViewPort = driver->getViewPort();
	driver->setViewPort(miniMapRect);

	// Change la caméra active du scene manager pour dessiner avec notre nouvelle caméra
	scene::ICameraSceneNode* const lastCamera = sceneManager->getActiveCamera();
	camera->setNearValue(lastCamera->getNearValue());
	camera->setFarValue(lastCamera->getFarValue());
	sceneManager->setActiveCamera(camera);

	// Rend la lumière ambiante plus forte (plus la lumière actuelle est faible, plus on l'augmente)
	const video::SColorf lastAmbiantLight = sceneManager->getAmbientLight();
	{
		// Formule du calcul de la couleur ambiante finale : pour chaque composante x :
		// X = -0.5 * (x - 1)² + 1 = -0.5x² + x + 0.5
		float valeurs[3] = {(lastAmbiantLight.r - 1.0f), (lastAmbiantLight.g - 1.0f), (lastAmbiantLight.b - 1.0f)};
		for (int i = 0; i < 3; ++i)
			valeurs[i] = (-0.5f * valeurs[i] * valeurs[i]) + 1.0f;
		sceneManager->setAmbientLight(video::SColorf(valeurs[0], valeurs[1], valeurs[2]));
	}

	// Désactive le brouillard (multiplie ses distances par 2)
	video::SColor lastFogColor;
	video::E_FOG_TYPE lastFogType;
	float lastFogStart, lastFogEnd, lastFogDensity;
	bool lastPixelFog, lastRangeFog;
	driver->getFog(lastFogColor, lastFogType, lastFogStart, lastFogEnd, lastFogDensity, lastPixelFog, lastRangeFog);
	driver->setFog(lastFogColor, lastFogType, max(lastFogStart * 2.0f, 500.0f), max(lastFogEnd * 2.0f, 2000.0f), lastFogDensity * 0.5f, lastPixelFog, lastRangeFog);

	// Dessine enfin la scène
	driver->clearZBuffer();				// Efface le Z-Buffer pendant le nouveau rendu de la scène (Attention : cela nécessite que la scène ne soit pas en train d'être rendue en même temps que cet élement de la GUI)
	game->render(true, false, false);	// Dessine uniquement la scène (pas la GUI), et sans réinitialiser le rendu actuel (Note : aucun effet post-rendu ne sera appliqué au rendu de la mini-carte)

	// Restaure le brouillard et la lumière ambiante
	driver->setFog(lastFogColor, lastFogType, lastFogStart, lastFogEnd, lastFogDensity, lastPixelFog, lastRangeFog);
	sceneManager->setAmbientLight(lastAmbiantLight);

	// Restaure enfin la caméra du scene manager ainsi que le view port du driver
	sceneManager->setActiveCamera(lastCamera);
	driver->setViewPort(lastViewPort);

	// Restaure l'état des noms au-dessus des bâtiments
	if (renderer)
		renderer->setNomsBatimentsVisible(lastNomsBatimentsVisible);

	// Et on se remet visible
	setVisible(true);

#ifdef USE_SPARK
	// Restaure le système de particules de SPARK
	if (renderer)
		renderer->getSparkManager().setParticleSystemVisible(lastParticleSystemVisible);
#endif
}
