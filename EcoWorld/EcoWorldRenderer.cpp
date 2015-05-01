#include "EcoWorldRenderer.h"
#include "EcoWorldSystem.h"
#include "WeatherManager.h"
#include "CLoadingScreen.h"
#include "CEcoWorldSkyDomeSceneNode.h"
#include "CSunSceneNode.h"
#include "CBatimentSceneNode.h"
#include "CShaderPreprocessor.h"
#include "CPostProcessManager.h"
#include "RTSCamera.h"

// La vitesse à laquelle le ciel tourne (en degrés pour 10 ms : °/10ms) (commenter pour désactiver la rotation du ciel) :
#define VITESSE_TOURNER_CIEL	DAY_TIME_INV * 0.01f	// Un tour par par an : 360°/an <=> 360°/360 jours <=> 1°/jour <=> 1°/DAY_TIME s <=> DAY_TIME_INV°/s <=> (DAY_TIME_INV / 100) °/10ms

//#define USE_VARIOUS_TREES		// Si défini, permet d'utiliser des arbres différents dans des forêts (chaque arbre est unique, car généré aléatoirement) (ne comprend pas les arbres de prévisualisation, qui sont identiques pour permettre une prévisualisation répide)
								// Sinon, tous les arbres des forêts seront identiques (utilisation des arbres de prévisualisation) (optimisation du temps de rendu et du temps de création des arbres, utilisation mémoire inférieure)

#define USE_MATERIALS_MESHES	// Si défini, les bâtiments non texturés mais disponibles en version colorée (avec des matériaux) seront affichés dans leur version avec matériaux
								// Sinon, ils seront affichés sans matériaux spécifiques (soit complètement gris)

EcoWorldRenderer::EcoWorldRenderer(const core::list<CBatimentSceneNode**>& gameSceneNodesPointers) : gameNodesPointers(gameSceneNodesPointers.size()),
#ifdef USE_COLLISIONS
 cameraTriangleSelector(0),
#endif
#ifdef USE_IRRKLANG
 isRainSoundPlaying(false),
#endif
 batimentsTriangleSelector(0), sunLight(0), shadowLight(0), skydome(0), currentWeatherID(WI_COUNT), newWeatherID(WI_COUNT), showNomsBatiments(indeterminate),
 skyDomeCallBack(0), skyDomeMaterial(-1), normalMappingCallBack(0), normalMappingMaterial(-1)
{
#ifdef USE_COLLISIONS
	cameraTriangleSelector = game->sceneManager->createMetaTriangleSelector();
#endif
	batimentsTriangleSelector = game->sceneManager->createMetaTriangleSelector();

	// Initialise le tableau des arbres de prévisualisation
	for (u32 i = 0; i < 4; ++i)
	{
		treesHighLOD[i] = NULL;
		treesMidLOD[i] = NULL;
	}

	// Copie le tableau des pointeurs de Game sur des scene nodes
	const core::list<CBatimentSceneNode**>::ConstIterator END = gameSceneNodesPointers.end();
	for (core::list<CBatimentSceneNode**>::ConstIterator it = gameSceneNodesPointers.begin(); it != END; ++it)
		gameNodesPointers.push_back(*it);

	reset();
}
EcoWorldRenderer::~EcoWorldRenderer()
{
	if (skyDomeCallBack)
		skyDomeCallBack->drop();
	if (normalMappingCallBack)
		normalMappingCallBack->drop();

	// Supprime les arbres de prévisualisation
	for (int i = 0; i < 4; ++i)
	{
		if (treesHighLOD[i])
			treesHighLOD[i]->drop();

		if (treesMidLOD[i])
			treesMidLOD[i]->drop();
	}

#ifdef USE_COLLISIONS
	if (cameraTriangleSelector)
		cameraTriangleSelector->drop();
#endif

	if (batimentsTriangleSelector)
		batimentsTriangleSelector->drop();
}
void EcoWorldRenderer::reset()
{
	// On suppose qu'ici, tous les scene nodes liés au renderer (batiments...) ont déjà été supprimés du scene manager

#ifdef USE_COLLISIONS
	if (cameraTriangleSelector)
		cameraTriangleSelector->removeAllTriangleSelectors();
#endif
	if (batimentsTriangleSelector)
		batimentsTriangleSelector->removeAllTriangleSelectors();

	/*
	// Supprime les arbres de prévisualisation
	for (int i = 0; i < 4; ++i)
	{
		if (treesHighLOD[i])
		{
			treesHighLOD[i]->Leaves.clear();
			treesHighLOD[i]->drop();
		}
		treesHighLOD[i] = NULL;

		if (treesMidLOD[i])
			treesMidLOD[i]->drop();
		treesMidLOD[i] = NULL;
	}
	*/

	// Indique au terrain et à SPARK de se réinitialiser
	terrainMgr.reset();
#ifdef USE_SPARK
	sparkMgr.reset();
#endif

	// Remet rous les pointeurs à 0
	sunLight = 0;
	shadowLight = 0;
	skydome = 0;
	sun = 0;
	currentWeatherID = WI_COUNT;
	newWeatherID = WI_COUNT;
	showNomsBatiments = indeterminate;
#ifdef USE_IRRKLANG
	isRainSoundPlaying = (ikMgr.sounds[IrrKlangManager::ESN_ambiant_rain].sounds.size() > 0);
#endif
}
void EcoWorldRenderer::initWorld(bool mainMenuLoading, CLoadingScreen* loadingScreen, float loadMin, float loadMax)
{
#ifdef USE_SPARK
	// Initialise SPARK
	sparkMgr.init();
#endif

	// Active un brouillard par défaut
	game->driver->setFog(video::SColor(255, 150, 150, 150), video::EFT_FOG_LINEAR, 1000.0f, 2500.0f, 0.0f, true, true);

	// Initialise les shaders du jeu s'ils sont activés
	if (gameConfig.shadersEnabled)
		initShaders(mainMenuLoading);



	// Vérifie si on doit utiliser les lumières pour XEffects
	const float LIGHT_RADIUS = TAILLE_CARTE * TAILLE_OBJETS;
	if (gameConfig.useXEffectsShadows && game->xEffects)
	{
		// Lumières de XEffects activées :
		game->sceneManager->setAmbientLight(video::SColorf(0.0f, 0.0f, 0.0f));

		// Lumière du soleil sous XEffects : lumière directionnelle
		if (!game->xEffects->getShadowLightCount())
		{
			game->xEffects->addShadowLight(SShadowLight(gameConfig.xEffectsShadowMapResolution, core::vector3df(0.0f, LIGHT_RADIUS, LIGHT_RADIUS), core::vector3df(0.0f),
				video::SColorf(0.0f, 0.0f, 0.0f), CAMERA_NEAR_VALUE, CAMERA_FAR_VALUE, 2000.0f, true));
		}
	}
	else
	{
		// Lumières de XEffects désactivées : utilisation des lumières d'Irrlicht :
		game->sceneManager->setAmbientLight(video::SColorf(0.5f, 0.5f, 0.5f));

		if (!sunLight)
		{
			// Lumière directionnelle du soleil ou de la lune
			sunLight = game->sceneManager->addLightSceneNode(0,
				core::vector3df(0.0f, 10.0f, 0.0f), video::SColorf(0.0f, 0.0f, 0.0f));
			sunLight->setLightType(video::ELT_DIRECTIONAL);
			sunLight->getLightData().AmbientColor.set(1.0f, 0.0f, 0.0f, 0.0f);
			sunLight->setRotation(core::vector3df(25.0f, 180.0f, 0.0f));
			sunLight->enableCastShadow(false);

#ifdef VITESSE_TOURNER_CIEL
			// Fait tourner la lumière en même temps que le ciel
			scene::ISceneNodeAnimator* const sunLightAnim = game->sceneManager->createRotationAnimator(
				core::vector3df(0.0f, VITESSE_TOURNER_CIEL, 0.0f));
			sunLight->addAnimator(sunLightAnim);
			sunLightAnim->drop();
#endif
		}

		if (!shadowLight && gameConfig.stencilShadowsEnabled)
		{
			// Lumière pour les ombres
			shadowLight = game->sceneManager->addLightSceneNode(0,
				core::vector3df(0.0f, LIGHT_RADIUS, LIGHT_RADIUS), video::SColorf(0.0f, 0.0f, 0.0f), 1.0f);
			shadowLight->getLightData().Attenuation.set(0.0f, 0.0f, 0.0f);
			shadowLight->getLightData().AmbientColor.set(1.0f, 0.0f, 0.0f, 0.0f);
			shadowLight->enableCastShadow(true);

#ifdef VITESSE_TOURNER_CIEL
			// Fait tourner la lumière en même temps que le ciel
			scene::ISceneNodeAnimator* const shadowLightAnim = game->sceneManager->createFlyCircleAnimator(
				core::vector3df(0.0f, LIGHT_RADIUS, 0.0f), LIGHT_RADIUS, VITESSE_TOURNER_CIEL * 0.1f * core::DEGTORAD,
				core::vector3df(0.0f, 1.0f, 0.0f), 0.75f);
			shadowLight->addAnimator(shadowLightAnim);
			shadowLightAnim->drop();
#endif
		}
		else
			shadowLight = NULL;
	}



	// Précharge toutes les textures de ciels (le menu principal peut aussi changer de temps)
	const bool lastTexCreationFlagMipMap = game->driver->getTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS);	// Désactive les niveaux de mips maps pour les textures du ciel
	game->driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, false);
	for (int i = 0; i < WI_COUNT; ++i)
	{
		WeatherInfos weatherInfos((WeatherID)i);
		io::path skydomePath = weatherInfos.skydomeTexturePath;
		skydomePath.append(getSkyTextureSuffixe(gameConfig.texturesQuality));
		game->driver->getTexture(skydomePath);
	}
	game->driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, lastTexCreationFlagMipMap);

	// Crée le ciel
	currentWeatherID = WI_COUNT;	// Assigne une valeur impossible à currentWeatherID pour forcer la re-création du temps du ciel
	newWeatherID = WI_COUNT;		// Assigne une valeur impossible à newWeatherID pour forcer la re-création du temps du ciel
	skydome = new CEcoWorldSkyDomeSceneNode(((gameConfig.skyDomeShadersEnabled && skyDomeMaterial != -1) ? (video::E_MATERIAL_TYPE)skyDomeMaterial : video::EMT_SOLID),
		game->sceneManager->getRootSceneNode(), game->sceneManager);
	skydome->drop();

#ifdef VITESSE_TOURNER_CIEL
	// Ajoute l'animator pour le ciel permettant de le faire tourner
	scene::ISceneNodeAnimator* const skyAnim = game->sceneManager->createRotationAnimator(core::vector3df(0.0f, VITESSE_TOURNER_CIEL, 0.0f));
	skydome->addAnimator(skyAnim);
	skyAnim->drop();
#endif



	// Initialise le soleil
	sun = new CSunSceneNode(core::dimension2df(200.0f, 200.0f), video::SColor(255, 255, 255, 255), game->sceneManager->getRootSceneNode(), game->sceneManager);
	sun->setMaterialTexture(0, game->driver->getTexture("sun.jpg"));	// Indique la texture du soleil
	sun->drop();



	// Met à jour le temps du monde d'après le Weather Manager, en initialisant le ciel et la position du soleil d'après le temps actuel du jeu
	update(true);

	// On ne doit que charger ce qui est nécéssaire au menu principal, on a donc terminé
	if (mainMenuLoading)
		return;



	// Pré-charge tous les batiments (les crée puis les supprime immédiatement)
	scene::ISceneNode* batiment = NULL;
	const float BI_COUNT_F_INV = 1.0f / ((float)BI_COUNT + 1.0f);	// Nécessaire pour la barre de chargement
	for (int i = 0; i <= BI_COUNT; ++i)
	{
		if (loadingScreen)
			if (loadingScreen->setPercentAndDraw((loadMax - loadMin) * (float)i * BI_COUNT_F_INV + loadMin))	return;

		// Si i ne représente actuellement aucun bâtiment, alors on charge les données pour la construction des bâtiments et pour leur dalle en béton
		// (l'ID du bâtiment n'a ici aucune importance, il doit juste être différent de BI_aucun)
		if (i == BI_aucun)
			batiment = loadConstructingBatiment(BI_maison_individuelle, NULL, true);
		else if (i == BI_COUNT)
			batiment = loadConcreteBatiment(0.0f, BI_maison_individuelle, NULL, true);
		else
			batiment = loadBatiment((BatimentID)i, NULL, true, true);

		if (batiment)
		{
			// Evite de laisser le batiment visible à la première frame du jeu
			batiment->setVisible(false);

			// Supprime le batiment à la prochaine frame
			game->sceneManager->addToDeletionQueue(batiment);
		}
	}

	if (loadingScreen)
		loadingScreen->setPercentAndDraw(loadMax);
}
void EcoWorldRenderer::initShaders(bool mainMenuLoading)
{
	if (!gameConfig.shadersEnabled)
		return;

	// SkyDome
	if (gameConfig.skyDomeShadersEnabled)
	{
		// Crée le callback s'il n'est pas déjà créé
		if (!skyDomeCallBack)
			skyDomeCallBack = new SkyDomeShaderCallBack();

		// Crée le matériau d'Irrlicht avec les shaders, s'il n'est pas déjà créé
		if (skyDomeMaterial == -1)
		{
			if (game->driver->getDriverType() != video::EDT_OPENGL)
			{
				skyDomeMaterial = game->shaderPreprocessor->addHighLevelShaderMaterialFromFiles(
					"SkyDome_vs.hlsl", "main", video::EVST_VS_1_1,
					"SkyDome_ps.hlsl", "main", video::EPST_PS_1_4,
					skyDomeCallBack, video::EMT_SOLID);

				if (skyDomeMaterial == -1)
					LOG(endl << "ERREUR : Impossible de compiler les shaders SkyDome_vs.hlsl (VS 1.1) et SkyDome_ps.hlsl (PS 1.4) !" << endl, ELL_WARNING);
			}
			else
			{
				skyDomeMaterial = game->shaderPreprocessor->addHighLevelShaderMaterialFromFiles(
					"SkyDome_vs.glsl", "main", video::EVST_VS_1_1,
					"SkyDome_ps.glsl", "main", video::EPST_PS_1_1,
					skyDomeCallBack, video::EMT_SOLID);

				if (skyDomeMaterial == -1)
					LOG(endl << "ERREUR : Impossible de compiler les shaders SkyDome_vs.glsl (VS 1.1) et SkyDome_ps.glsl (PS 1.1) !" << endl, ELL_WARNING);
			}
		}
	}

	// On ne doit charger que les shaders nécessaires au menu principal : on a donc terminé
	if (mainMenuLoading)
		return;

	// NormalMapping
	if (gameConfig.normalMappingEnabled)
	{
		// Crée le callback s'il n'est pas déjà créé
		if (!normalMappingCallBack)
			normalMappingCallBack = new NormalMappingShaderCallBack(game->sceneManager);

		// Crée le matériau d'Irrlicht avec les shaders, s'il n'est pas déjà créé
		if (normalMappingMaterial == -1)
		{
			if (game->driver->getDriverType() != video::EDT_OPENGL)
			{
				normalMappingMaterial = game->shaderPreprocessor->addHighLevelShaderMaterialFromFiles(
					"NormalMapping_vs.hlsl", "main", video::EVST_VS_1_1,
					"NormalMapping_ps.hlsl", "main", video::EPST_PS_2_0,
					normalMappingCallBack, video::EMT_SOLID);

				if (normalMappingMaterial == -1)
					LOG(endl << "ERREUR : Impossible de compiler les shaders NormalMapping_vs.hlsl (VS 1.1) et NormalMapping_ps.hlsl (PS 2.0) !" << endl, ELL_WARNING);
			}
			else
			{
				normalMappingMaterial = game->shaderPreprocessor->addHighLevelShaderMaterialFromFiles(
					"NormalMapping_vs.glsl", "main", video::EVST_VS_1_1,
					"NormalMapping_ps.glsl", "main", video::EPST_PS_1_1,
					normalMappingCallBack, video::EMT_SOLID);

				if (normalMappingMaterial == -1)
					LOG(endl << "ERREUR : Impossible de compiler les shaders NormalMapping_vs.glsl (VS 1.1) et NormalMapping_ps.glsl (PS 1.1) !" << endl, ELL_WARNING);
			}
		}
	}
}
void EcoWorldRenderer::loadNewTerrain(const io::path& adresse, bool createSounds, bool createTriangleSelectors, bool updateSystemTerrain, CLoadingScreen* loadingScreen, float loadMin, float loadMax)
{
	// Charge un nouveau terrain en faisant appel au terrain manager
	terrainMgr.loadNewTerrain(
#ifdef USE_SPARK
		sparkMgr,
#endif
		adresse, createSounds,
#ifdef USE_COLLISIONS
		createTriangleSelectors, cameraTriangleSelector,
#else
		false, NULL,
#endif
		updateSystemTerrain, loadingScreen, loadMin, loadMax);

#ifdef USE_SPARK
	// Indique la hauteur minimale du terrain à la pluie de SPARK
	float minHeight = terrainMgr.getTerrainBoundingBox().MinEdge.Y;
	if (terrainMgr.getTerrainInfos().water.visible)
		minHeight = max(minHeight, terrainMgr.getTerrainInfos().water.height);	// Prend la plus grande valeur entre la hauteur de l'eau et la hauteur minimale du terrain
	sparkMgr.setMinTerrainHeight(minHeight);

	// Indique la direction du vent à la pluie de SPARK
	core::vector2df windDirection = terrainMgr.getWindDirection();
	windDirection.X *= -1.0f;	// Inverse le sens du vent en X pour que le sens des particules dans SPARK corresponde au sens du vent dans le jeu
	sparkMgr.setWindDirection(windDirection);
#endif
}
scene::ISceneNode* EcoWorldRenderer::loadBatiment(BatimentID batimentID, scene::ISceneNode* parent, bool forLoadingOnly, bool isTmpBatiment)
{
	if (batimentID == BI_aucun)
		return NULL;

	if (!parent)
		parent = game->sceneManager->getRootSceneNode();

	// Le node que renverra cette fonction
	scene::ISceneNode* node = NULL;

#ifdef USE_MATERIALS_MESHES
	bool reinitMaterialsDiffuse = true;	// True si à la fin de cette fonction la couleur des matériaux de ce node devront être réinitialisés, false sinon : passer cette valeur à false lorsque le node chargé a aussi chargé ses matériaux
#endif

	// Agrandis un peu les batiments exportés avec Blender, car les unités d'Irrlicht sont plus grandes que celles de Blender
	// 1.0f sous Blender <-> 1 sous EcoWorldSystem <-> TAILLE_OBJETS / 2 sous Irrlicht
	const core::vector3df blenderObjectsScaling(TAILLE_OBJETS * 0.5f);

	switch (batimentID)
	{
#ifndef KID_VERSION	// En mode enfant : on désactive la maison de base et les routes qui sont inutiles
		// Tests
	case BI_maison:
		{
			// Charge seulement les mesh et les textures nécessaires à cette maison
			if (forLoadingOnly)
			{
				game->sceneManager->getMesh("Maison_BQ.obj");
				break;
			}

			node = game->sceneManager->addAnimatedMeshSceneNode(game->sceneManager->getMesh("Maison_BQ.obj"), parent, -1,
				core::vector3df(), core::vector3df(), blenderObjectsScaling);
			if (!node)
				break;

			// Crée une ombre pour ce bâtiment
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode();

			if (!isTmpBatiment)
			{
				// Crée le triangle selector de la maison :

#if 1	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelectorFromBoundingBox(node);
#endif
				if (selector)
				{
					node->setTriangleSelector(selector);
					selector->drop();
				}
			}
		}
		break;
	case BI_route:
		{
			// Charge seulement les mesh et les textures nécessaires à cette route
			if (forLoadingOnly)
			{
				game->driver->getTexture("Road.jpg");

				if (gameConfig.normalMappingEnabled && normalMappingMaterial != -1)
				{
					// Charge la texture pour le normal mapping
					game->driver->getTexture("Road_NormalMap.bmp");
				}
				break;
			}

#if 0	// Utilise un plan pour créer la route
			scene::IAnimatedMesh* routeMesh = game->sceneManager->addHillPlaneMesh("route", core::dimension2df(TAILLE_OBJETS, TAILLE_OBJETS),
				core::dimension2du(1, 1), 0, 0.0f, core::dimension2df(0.0f, 0.0f),
				core::dimension2df(1.0f, 1.0f));
#else	// Permet d'utiliser des cubes pour la route au lieu de plans : évite les problèmes d'affichage de la route, dûs à la faible épaisseur des plans, mais le rendu est plus lent
			// La taille en Y des cubes de la route peut être réglée suivant les besoins :
			// Plus ce nombre est grand, plus on pourra voir la route de haut sans problème d'affichage ou de clignotement, mais plus la route sera épaisse :
			// si l'épaisseur de la route est trop grande, on risque de remarquer que ce sont des cubes.
			scene::IMesh* routeMesh = game->sceneManager->getGeometryCreator()->createCubeMesh(
				core::vector3df(TAILLE_OBJETS, TAILLE_OBJETS * 0.2f, TAILLE_OBJETS));	// Taille en Y réglable
#endif
			if (!routeMesh)
				break;

			// Crée le node de ce bâtiment :
			if (!gameConfig.normalMappingEnabled || normalMappingMaterial == -1)
			{
				node = game->sceneManager->addMeshSceneNode(routeMesh, parent);
				routeMesh->drop();
				if (!node)
					break;
			}
			else
			{
				// Charge cette mesh en demandant à Irrlicht de créer ses tangentes pour permettre l'utilisation du normal mapping
				scene::IMesh* tangentMesh = game->sceneManager->getMeshManipulator()->createMeshWithTangents(routeMesh);
				node = game->sceneManager->addMeshSceneNode(tangentMesh, parent);
				if (tangentMesh)			tangentMesh->drop();
				if (!node)
					break;

				// Charge la texture pour le normal mapping et indique au node qu'on active le normal mapping
				node->setMaterialTexture(1, game->driver->getTexture("Road_NormalMap.bmp"));
				node->setMaterialType((video::E_MATERIAL_TYPE)normalMappingMaterial);
			}

			node->setMaterialTexture(0, game->driver->getTexture("Road.jpg"));

			if (!isTmpBatiment)
			{
				// Crée le triangle selector de la route :

#if 0	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelectorFromBoundingBox(node);
#endif
				if (selector)
				{
					node->setTriangleSelector(selector);
					selector->drop();
				}
			}
		}
		break;
#endif

		// Maisons
	case BI_maison_individuelle:
		{
			// Charge seulement les mesh et les textures nécessaires à cette maison
			if (forLoadingOnly)
			{
				game->sceneManager->getMesh("Maison Individuelle.obj");		// Mesh normal
				game->sceneManager->getMesh("Maison Individuelle Ombre.obj");	// Mesh de l'ombre et du triangle selector

				// Charge la texture de la maison
				{
					io::path textureName = "Maisons_Textures";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					game->driver->getTexture(textureName);
				}

				if (gameConfig.normalMappingEnabled && normalMappingMaterial != -1)
				{
					// Charge la texture pour le normal mapping
					io::path textureName = "Maisons_NormalMap";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					game->driver->getTexture(textureName);
				}

				break;
			}

			// Crée le node de ce bâtiment :
			if (!gameConfig.normalMappingEnabled || normalMappingMaterial == -1)
			{
				node = game->sceneManager->addAnimatedMeshSceneNode(
					game->sceneManager->getMesh("Maison Individuelle.obj"), parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling * 2.0f / 3.0f);
				if (!node)
					break;
			}
			else
			{
				// Charge cette mesh en demandant à Irrlicht de créer ses tangentes pour permettre l'utilisation du normal mapping
				scene::IMesh* tangentMesh = game->sceneManager->getMeshManipulator()->createMeshWithTangents(game->sceneManager->getMesh("Maison Individuelle.obj"));
				scene::IAnimatedMesh* tangentAnimatedMesh = game->sceneManager->getMeshManipulator()->createAnimatedMesh(tangentMesh);
				node = game->sceneManager->addAnimatedMeshSceneNode(tangentAnimatedMesh, parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling * 2.0f / 3.0f);
				if (tangentMesh)			tangentMesh->drop();
				if (tangentAnimatedMesh)	tangentAnimatedMesh->drop();
				if (!node)
					break;

				// Charge la texture pour le normal mapping et indique au node qu'on active le normal mapping
				{
					io::path textureName = "Maisons_NormalMap";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					node->setMaterialTexture(1, game->driver->getTexture(textureName));
				}
				node->setMaterialType((video::E_MATERIAL_TYPE)normalMappingMaterial);
			}

			// Charge la texture de la maison
			{
				io::path textureName = "Maisons_Textures";
				textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
				textureName.append(".bmp");
				node->setMaterialTexture(0, game->driver->getTexture(textureName));
			}

			// Crée une ombre pour ce bâtiment
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					game->sceneManager->getMesh("Maison Individuelle Ombre.obj"));

			if (!isTmpBatiment)
			{
				// Crée le triangle selector de la maison :
#if 1	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					//((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);		// Avec le mesh normal
					game->sceneManager->getMesh("Maison Individuelle Ombre.obj"), node);	// Avec le mesh de l'ombre
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelectorFromBoundingBox(node);
#endif
				if (selector)
				{
					node->setTriangleSelector(selector);
					selector->drop();
				}
			}
		}
		break;
	case BI_maison_basse_consommation:
		{
			// Charge seulement les mesh et les textures nécessaires à cette maison
			if (forLoadingOnly)
			{
				game->sceneManager->getMesh("Maison Basse Consommation.obj");	// Mesh normal
				// On utilise l'ombre de la maison individuelle car les deux bâtiments ont exactement la même ombre (économie de mémoire, accélération du chargement)
				//game->sceneManager->getMesh("Maison Basse Consommation Ombre.obj");	// Mesh de l'ombre et du triangle selector
				game->sceneManager->getMesh("Maison Individuelle Ombre.obj");	// Mesh de l'ombre et du triangle selector

				// Charge la texture de la maison
				{
					io::path textureName = "Maisons_Textures";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					game->driver->getTexture(textureName);
				}

				if (gameConfig.normalMappingEnabled && normalMappingMaterial != -1)
				{
					// Charge la texture pour le normal mapping
					io::path textureName = "Maisons_NormalMap";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					game->driver->getTexture(textureName);
				}
				break;
			}

			// Crée le node de ce bâtiment :
			if (!gameConfig.normalMappingEnabled || normalMappingMaterial == -1)
			{
				node = game->sceneManager->addAnimatedMeshSceneNode(
					game->sceneManager->getMesh("Maison Basse Consommation.obj"), parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling * 2.0f / 3.0f);
				if (!node)
					break;
			}
			else
			{
				// Charge cette mesh en demandant à Irrlicht de créer ses tangentes pour permettre l'utilisation du normal mapping
				scene::IMesh* tangentMesh = game->sceneManager->getMeshManipulator()->createMeshWithTangents(game->sceneManager->getMesh("Maison Basse Consommation.obj"));
				scene::IAnimatedMesh* tangentAnimatedMesh = game->sceneManager->getMeshManipulator()->createAnimatedMesh(tangentMesh);
				node = game->sceneManager->addAnimatedMeshSceneNode(tangentAnimatedMesh, parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling * 2.0f / 3.0f);
				if (tangentMesh)			tangentMesh->drop();
				if (tangentAnimatedMesh)	tangentAnimatedMesh->drop();
				if (!node)
					break;

				// Charge la texture pour le normal mapping et indique au node qu'on active le normal mapping
				{
					io::path textureName = "Maisons_NormalMap";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					node->setMaterialTexture(1, game->driver->getTexture(textureName));
				}
				node->setMaterialType((video::E_MATERIAL_TYPE)normalMappingMaterial);
			}

			// Charge la texture de la maison
			{
				io::path textureName = "Maisons_Textures";
				textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
				textureName.append(".bmp");
				node->setMaterialTexture(0, game->driver->getTexture(textureName));
			}

			// Crée une ombre pour ce bâtiment :
			// On utilise l'ombre de la maison individuelle car les deux bâtiments ont exactement la même ombre (économie de mémoire, accélération du chargement)
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					//game->sceneManager->getMesh("Maison Basse Consommation Ombre.obj"));
					game->sceneManager->getMesh("Maison Individuelle Ombre.obj"));

			if (!isTmpBatiment)
			{
				// Crée le triangle selector de la maison :
#if 1	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					//((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);			// Avec le mesh normal
					game->sceneManager->getMesh("Maison Individuelle Ombre.obj"), node);		// Avec le mesh de l'ombre	// "Maison Basse Consommation Ombre.obj"
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelectorFromBoundingBox(node);
#endif
				if (selector)
				{
					node->setTriangleSelector(selector);
					selector->drop();
				}
			}
		}
		break;
	case BI_maison_avec_panneaux_solaires:
		{
			// Charge seulement les mesh et les textures nécessaires à cette maison
			if (forLoadingOnly)
			{
				game->sceneManager->getMesh("Maison avec Panneaux Solaires.obj");			// Mesh normal
				game->sceneManager->getMesh("Maison avec Panneaux Solaires Ombre.obj");	// Mesh de l'ombre et du triangle selector

				// Charge la texture de la maison
				{
					io::path textureName = "Maisons_Textures";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					game->driver->getTexture(textureName);
				}

				if (gameConfig.normalMappingEnabled && normalMappingMaterial != -1)
				{
					// Charge la texture pour le normal mapping
					io::path textureName = "Maisons_NormalMap";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					game->driver->getTexture(textureName);
				}
				break;
			}

			// Crée le node de ce bâtiment :
			if (!gameConfig.normalMappingEnabled || normalMappingMaterial == -1)
			{
				node = game->sceneManager->addAnimatedMeshSceneNode(
					game->sceneManager->getMesh("Maison avec Panneaux Solaires.obj"), parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling * 2.0f / 3.0f);
				if (!node)
					break;
			}
			else
			{
				// Charge cette mesh en demandant à Irrlicht de créer ses tangentes pour permettre l'utilisation du normal mapping
				scene::IMesh* tangentMesh = game->sceneManager->getMeshManipulator()->createMeshWithTangents(game->sceneManager->getMesh("Maison avec Panneaux Solaires.obj"));
				scene::IAnimatedMesh* tangentAnimatedMesh = game->sceneManager->getMeshManipulator()->createAnimatedMesh(tangentMesh);
				node = game->sceneManager->addAnimatedMeshSceneNode(tangentAnimatedMesh, parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling * 2.0f / 3.0f);
				if (tangentMesh)			tangentMesh->drop();
				if (tangentAnimatedMesh)	tangentAnimatedMesh->drop();
				if (!node)
					break;

				// Charge la texture pour le normal mapping et indique au node qu'on active le normal mapping
				{
					io::path textureName = "Maisons_NormalMap";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					node->setMaterialTexture(1, game->driver->getTexture(textureName));
				}
				node->setMaterialType((video::E_MATERIAL_TYPE)normalMappingMaterial);
			}

			// Charge la texture de la maison
			{
				io::path textureName = "Maisons_Textures";
				textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
				textureName.append(".bmp");
				node->setMaterialTexture(0, game->driver->getTexture(textureName));
			}

			// Crée une ombre pour ce bâtiment
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					game->sceneManager->getMesh("Maison avec Panneaux Solaires Ombre.obj"));

			if (!isTmpBatiment)
			{
				// Crée le triangle selector de la maison :
#if 1	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					//((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);					// Avec le mesh normal
					game->sceneManager->getMesh("Maison avec Panneaux Solaires Ombre.obj"), node);	// Avec le mesh de l'ombre
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelectorFromBoundingBox(node);
#endif
				if (selector)
				{
					node->setTriangleSelector(selector);
					selector->drop();
				}
			}
		}
		break;
	case BI_grande_maison_individuelle:
		{
			// Charge seulement les mesh et les textures nécessaires à cette maison
			if (forLoadingOnly)
			{
				game->sceneManager->getMesh("Grande Maison Individuelle.obj");		// Mesh normal
				game->sceneManager->getMesh("Grande Maison Individuelle Ombre.obj");	// Mesh de l'ombre et du triangle selector

				// Charge la texture de la maison
				{
					io::path textureName = "Maisons_Textures";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					game->driver->getTexture(textureName);
				}

				if (gameConfig.normalMappingEnabled && normalMappingMaterial != -1)
				{
					// Charge la texture pour le normal mapping
					io::path textureName = "Maisons_NormalMap";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					game->driver->getTexture(textureName);
				}
				break;
			}

			// Crée le node de ce bâtiment :
			if (!gameConfig.normalMappingEnabled || normalMappingMaterial == -1)
			{
				node = game->sceneManager->addAnimatedMeshSceneNode(
					game->sceneManager->getMesh("Grande Maison Individuelle.obj"), parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling * 2.0f / 3.0f);
				if (!node)
					break;
			}
			else
			{
				// Charge cette mesh en demandant à Irrlicht de créer ses tangentes pour permettre l'utilisation du normal mapping
				scene::IMesh* tangentMesh = game->sceneManager->getMeshManipulator()->createMeshWithTangents(game->sceneManager->getMesh("Grande Maison Individuelle.obj"));
				scene::IAnimatedMesh* tangentAnimatedMesh = game->sceneManager->getMeshManipulator()->createAnimatedMesh(tangentMesh);
				node = game->sceneManager->addAnimatedMeshSceneNode(tangentAnimatedMesh, parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling * 2.0f / 3.0f);
				if (tangentMesh)			tangentMesh->drop();
				if (tangentAnimatedMesh)	tangentAnimatedMesh->drop();
				if (!node)
					break;

				// Charge la texture pour le normal mapping et indique au node qu'on active le normal mapping
				{
					io::path textureName = "Maisons_NormalMap";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					node->setMaterialTexture(1, game->driver->getTexture(textureName));
				}
				node->setMaterialType((video::E_MATERIAL_TYPE)normalMappingMaterial);
			}

			// Charge la texture de la maison
			{
				io::path textureName = "Maisons_Textures";
				textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
				textureName.append(".bmp");
				node->setMaterialTexture(0, game->driver->getTexture(textureName));
			}

			// Crée une ombre pour ce bâtiment
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					game->sceneManager->getMesh("Grande Maison Individuelle Ombre.obj"));

			if (!isTmpBatiment)
			{
				// Crée le triangle selector de la maison :
#if 1	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					//((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);				// Avec le mesh normal
					game->sceneManager->getMesh("Grande Maison Individuelle Ombre.obj"), node);	// Avec le mesh de l'ombre
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelectorFromBoundingBox(node);
#endif
				if (selector)
				{
					node->setTriangleSelector(selector);
					selector->drop();
				}
			}
		}
		break;
	case BI_chalet:
		{
			// Charge seulement les mesh et les textures nécessaires à ce chalet
			if (forLoadingOnly)
			{
				game->sceneManager->getMesh("Chalet.obj");		// Mesh normal
				game->sceneManager->getMesh("Chalet Ombre.obj");	// Mesh de l'ombre et du triangle selector

				// Charge la texture du chalet
				{
					io::path textureName = "Chalet_Textures";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					game->driver->getTexture(textureName);
				}

				if (gameConfig.normalMappingEnabled && normalMappingMaterial != -1)
				{
					// Charge la texture pour le normal mapping
					io::path textureName = "Chalet_NormalMap";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					game->driver->getTexture(textureName);
				}
				break;
			}

			// Crée le node de ce bâtiment :
			if (!gameConfig.normalMappingEnabled || normalMappingMaterial == -1)
			{
				node = game->sceneManager->addAnimatedMeshSceneNode(
					game->sceneManager->getMesh("Chalet.obj"), parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling * 2.0f / 3.0f);
				if (!node)
					break;
			}
			else
			{
				// Charge cette mesh en demandant à Irrlicht de créer ses tangentes pour permettre l'utilisation du normal mapping
				scene::IMesh* tangentMesh = game->sceneManager->getMeshManipulator()->createMeshWithTangents(game->sceneManager->getMesh("Chalet.obj"));
				scene::IAnimatedMesh* tangentAnimatedMesh = game->sceneManager->getMeshManipulator()->createAnimatedMesh(tangentMesh);
				node = game->sceneManager->addAnimatedMeshSceneNode(tangentAnimatedMesh, parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling * 2.0f / 3.0f);
				if (tangentMesh)			tangentMesh->drop();
				if (tangentAnimatedMesh)	tangentAnimatedMesh->drop();
				if (!node)
					break;

				// Charge la texture pour le normal mapping et indique au node qu'on active le normal mapping
				{
					io::path textureName = "Chalet_NormalMap";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					node->setMaterialTexture(1, game->driver->getTexture(textureName));
				}
				node->setMaterialType((video::E_MATERIAL_TYPE)normalMappingMaterial);
			}

			// Charge la texture du chalet
			{
				io::path textureName = "Chalet_Textures";
				textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
				textureName.append(".bmp");
				node->setMaterialTexture(0, game->driver->getTexture(textureName));
			}

			// Crée une ombre pour ce bâtiment
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					game->sceneManager->getMesh("Chalet Ombre.obj"));

			if (!isTmpBatiment)
			{
				// Crée le triangle selector du chalet :
#if 1	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					//((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);		// Avec le mesh normal
					game->sceneManager->getMesh("Chalet Ombre.obj"), node);				// Avec le mesh de l'ombre
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelectorFromBoundingBox(node);
#endif
				if (selector)
				{
					node->setTriangleSelector(selector);
					selector->drop();
				}
			}
		}
		break;

		// Immeubles
	case BI_immeuble_individuel:
		{
			// Charge seulement les mesh et les textures nécessaires à cet immeuble
			if (forLoadingOnly)
			{
				game->sceneManager->getMesh("Immeuble Individuel.obj");		// Mesh normal
				game->sceneManager->getMesh("Immeuble Individuel Ombre.obj");	// Mesh de l'ombre et du triangle selector

				// Charge la texture de l'immeuble
				{
					io::path textureName = "Immeubles_Textures";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					game->driver->getTexture(textureName);
				}

				if (gameConfig.normalMappingEnabled && normalMappingMaterial != -1)
				{
					// Charge la texture pour le normal mapping
					io::path textureName = "Immeubles_NormalMap";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					game->driver->getTexture(textureName);
				}
				break;
			}

			// Crée le node de ce bâtiment :
			if (!gameConfig.normalMappingEnabled || normalMappingMaterial == -1)
			{
				node = game->sceneManager->addAnimatedMeshSceneNode(
					game->sceneManager->getMesh("Immeuble Individuel.obj"), parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling * 2.0f / 3.0f);
				if (!node)
					break;
			}
			else
			{
				// Charge cette mesh en demandant à Irrlicht de créer ses tangentes pour permettre l'utilisation du normal mapping
				scene::IMesh* tangentMesh = game->sceneManager->getMeshManipulator()->createMeshWithTangents(game->sceneManager->getMesh("Immeuble Individuel.obj"));
				scene::IAnimatedMesh* tangentAnimatedMesh = game->sceneManager->getMeshManipulator()->createAnimatedMesh(tangentMesh);
				node = game->sceneManager->addAnimatedMeshSceneNode(tangentAnimatedMesh, parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling * 2.0f / 3.0f);
				if (tangentMesh)			tangentMesh->drop();
				if (tangentAnimatedMesh)	tangentAnimatedMesh->drop();
				if (!node)
					break;

				// Charge la texture pour le normal mapping et indique au node qu'on active le normal mapping
				{
					io::path textureName = "Immeubles_NormalMap";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					node->setMaterialTexture(1, game->driver->getTexture(textureName));
				}
				node->setMaterialType((video::E_MATERIAL_TYPE)normalMappingMaterial);
			}

			// Charge la texture de l'immeuble
			{
				io::path textureName = "Immeubles_Textures";
				textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
				textureName.append(".bmp");
				node->setMaterialTexture(0, game->driver->getTexture(textureName));
			}

			// Crée une ombre pour ce bâtiment
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					game->sceneManager->getMesh("Immeuble Individuel Ombre.obj"));

			if (!isTmpBatiment)
			{
				// Crée le triangle selector de l'immeuble :
#if 1	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					//((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);		// Avec le mesh normal
					game->sceneManager->getMesh("Immeuble Individuel Ombre.obj"), node);	// Avec le mesh de l'ombre
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelectorFromBoundingBox(node);
#endif
				if (selector)
				{
					node->setTriangleSelector(selector);
					selector->drop();
				}
			}
		}
		break;
	case BI_immeuble_basse_consommation:
		{
			// Charge seulement les mesh et les textures nécessaires à cet immeuble
			if (forLoadingOnly)
			{
				game->sceneManager->getMesh("Immeuble Basse Consommation.obj");		// Mesh normal
				// On utilise l'ombre de l'immeuble individuel car les deux bâtiments ont exactement la même ombre (économie de mémoire, accélération du chargement)
				//game->sceneManager->getMesh("Immeuble Basse Consommation Ombre.obj");	// Mesh de l'ombre et du triangle selector
				game->sceneManager->getMesh("Immeuble Individuel Ombre.obj");	// Mesh de l'ombre et du triangle selector

				// Charge la texture de l'immeuble
				{
					io::path textureName = "Immeubles_Textures";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					game->driver->getTexture(textureName);
				}

				if (gameConfig.normalMappingEnabled && normalMappingMaterial != -1)
				{
					// Charge la texture pour le normal mapping
					io::path textureName = "Immeubles_NormalMap";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					game->driver->getTexture(textureName);
				}
				break;
			}

			// Crée le node de ce bâtiment :
			if (!gameConfig.normalMappingEnabled || normalMappingMaterial == -1)
			{
				node = game->sceneManager->addAnimatedMeshSceneNode(
					game->sceneManager->getMesh("Immeuble Basse Consommation.obj"), parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling * 2.0f / 3.0f);
				if (!node)
					break;
			}
			else
			{
				// Charge cette mesh en demandant à Irrlicht de créer ses tangentes pour permettre l'utilisation du normal mapping
				scene::IMesh* tangentMesh = game->sceneManager->getMeshManipulator()->createMeshWithTangents(game->sceneManager->getMesh("Immeuble Basse Consommation.obj"));
				scene::IAnimatedMesh* tangentAnimatedMesh = game->sceneManager->getMeshManipulator()->createAnimatedMesh(tangentMesh);
				node = game->sceneManager->addAnimatedMeshSceneNode(tangentAnimatedMesh, parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling * 2.0f / 3.0f);
				if (tangentMesh)			tangentMesh->drop();
				if (tangentAnimatedMesh)	tangentAnimatedMesh->drop();
				if (!node)
					break;

				// Charge la texture pour le normal mapping et indique au node qu'on active le normal mapping
				{
					io::path textureName = "Immeubles_NormalMap";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					node->setMaterialTexture(1, game->driver->getTexture(textureName));
				}
				node->setMaterialType((video::E_MATERIAL_TYPE)normalMappingMaterial);
			}

			// Charge la texture de l'immeuble
			{
				io::path textureName = "Immeubles_Textures";
				textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
				textureName.append(".bmp");
				node->setMaterialTexture(0, game->driver->getTexture(textureName));
			}

			// Crée une ombre pour ce bâtiment :
			// On utilise l'ombre de l'immeuble individuel car les deux bâtiments ont exactement la même ombre (économie de mémoire, accélération du chargement)
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					//game->sceneManager->getMesh("Immeuble Basse Consommation Ombre.obj"));
					game->sceneManager->getMesh("Immeuble Individuel Ombre.obj"));

			if (!isTmpBatiment)
			{
				// Crée le triangle selector de l'immeuble :
#if 1	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					//((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);		// Avec le mesh normal
					game->sceneManager->getMesh("Immeuble Individuel Ombre.obj"), node);	// Avec le mesh de l'ombre	// "Immeuble Basse Consommation Ombre.obj"
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelectorFromBoundingBox(node);
#endif
				if (selector)
				{
					node->setTriangleSelector(selector);
					selector->drop();
				}
			}
		}
		break;
	case BI_immeuble_avec_panneaux_solaires:
		{
			// Charge seulement les mesh et les textures nécessaires à cet immeuble
			if (forLoadingOnly)
			{
				game->sceneManager->getMesh("Immeuble avec Panneaux Solaires.obj");		// Mesh normal
				game->sceneManager->getMesh("Immeuble avec Panneaux Solaires Ombre.obj");	// Mesh de l'ombre et du triangle selector

				// Charge la texture de l'immeuble
				{
					io::path textureName = "Immeubles_Textures";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					game->driver->getTexture(textureName);
				}

				if (gameConfig.normalMappingEnabled && normalMappingMaterial != -1)
				{
					// Charge la texture pour le normal mapping
					io::path textureName = "Immeubles_NormalMap";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					game->driver->getTexture(textureName);
				}
				break;
			}

			// Crée le node de ce bâtiment :
			if (!gameConfig.normalMappingEnabled || normalMappingMaterial == -1)
			{
				node = game->sceneManager->addAnimatedMeshSceneNode(
					game->sceneManager->getMesh("Immeuble avec Panneaux Solaires.obj"), parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling * 2.0f / 3.0f);
				if (!node)
					break;
			}
			else
			{
				// Charge cette mesh en demandant à Irrlicht de créer ses tangentes pour permettre l'utilisation du normal mapping
				scene::IMesh* tangentMesh = game->sceneManager->getMeshManipulator()->createMeshWithTangents(game->sceneManager->getMesh("Immeuble avec Panneaux Solaires.obj"));
				scene::IAnimatedMesh* tangentAnimatedMesh = game->sceneManager->getMeshManipulator()->createAnimatedMesh(tangentMesh);
				node = game->sceneManager->addAnimatedMeshSceneNode(tangentAnimatedMesh, parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling * 2.0f / 3.0f);
				if (tangentMesh)			tangentMesh->drop();
				if (tangentAnimatedMesh)	tangentAnimatedMesh->drop();
				if (!node)
					break;

				// Charge la texture pour le normal mapping et indique au node qu'on active le normal mapping
				{
					io::path textureName = "Immeubles_NormalMap";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					node->setMaterialTexture(1, game->driver->getTexture(textureName));
				}
				node->setMaterialType((video::E_MATERIAL_TYPE)normalMappingMaterial);
			}

			// Charge la texture de l'immeuble
			{
				io::path textureName = "Immeubles_Textures";
				textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
				textureName.append(".bmp");
				node->setMaterialTexture(0, game->driver->getTexture(textureName));
			}

			// Crée une ombre pour ce bâtiment
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					game->sceneManager->getMesh("Immeuble avec Panneaux Solaires Ombre.obj"));

			if (!isTmpBatiment)
			{
				// Crée le triangle selector de l'immeuble :
#if 1	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					//((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);					// Avec le mesh normal
					game->sceneManager->getMesh("Immeuble avec Panneaux Solaires Ombre.obj"), node);	// Avec le mesh de l'ombre
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelectorFromBoundingBox(node);
#endif
				if (selector)
				{
					node->setTriangleSelector(selector);
					selector->drop();
				}
			}
		}
		break;
	case BI_grand_immeuble_individuel:
		{
			// Charge seulement les mesh et les textures nécessaires à cet immeuble
			if (forLoadingOnly)
			{
				game->sceneManager->getMesh("Grand Immeuble Individuel.obj");			// Mesh normal
				game->sceneManager->getMesh("Grand Immeuble Individuel Ombre.obj");	// Mesh de l'ombre et du triangle selector

				// Charge la texture de l'immeuble
				{
					io::path textureName = "Immeubles_Textures";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					game->driver->getTexture(textureName);
				}

				if (gameConfig.normalMappingEnabled && normalMappingMaterial != -1)
				{
					// Charge la texture pour le normal mapping
					io::path textureName = "Immeubles_NormalMap";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					game->driver->getTexture(textureName);
				}
				break;
			}

			// Crée le node de ce bâtiment :
			if (!gameConfig.normalMappingEnabled || normalMappingMaterial == -1)
			{
				node = game->sceneManager->addAnimatedMeshSceneNode(
					game->sceneManager->getMesh("Grand Immeuble Individuel.obj"), parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling * 2.0f / 3.0f);
				if (!node)
					break;
			}
			else
			{
				// Charge cette mesh en demandant à Irrlicht de créer ses tangentes pour permettre l'utilisation du normal mapping
				scene::IMesh* tangentMesh = game->sceneManager->getMeshManipulator()->createMeshWithTangents(game->sceneManager->getMesh("Grand Immeuble Individuel.obj"));
				scene::IAnimatedMesh* tangentAnimatedMesh = game->sceneManager->getMeshManipulator()->createAnimatedMesh(tangentMesh);
				node = game->sceneManager->addAnimatedMeshSceneNode(tangentAnimatedMesh, parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling * 2.0f / 3.0f);
				if (tangentMesh)			tangentMesh->drop();
				if (tangentAnimatedMesh)	tangentAnimatedMesh->drop();
				if (!node)
					break;

				// Charge la texture pour le normal mapping et indique au node qu'on active le normal mapping
				{
					io::path textureName = "Immeubles_NormalMap";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					node->setMaterialTexture(1, game->driver->getTexture(textureName));
				}
				node->setMaterialType((video::E_MATERIAL_TYPE)normalMappingMaterial);
			}

			// Charge la texture de l'immeuble
			{
				io::path textureName = "Immeubles_Textures";
				textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
				textureName.append(".bmp");
				node->setMaterialTexture(0, game->driver->getTexture(textureName));
			}

			// Crée une ombre pour ce bâtiment
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					game->sceneManager->getMesh("Grand Immeuble Individuel Ombre.obj"));

			if (!isTmpBatiment)
			{
				// Crée le triangle selector de l'immeuble :
#if 1	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					//((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);				// Avec le mesh normal
					game->sceneManager->getMesh("Grand Immeuble Individuel Ombre.obj"), node);	// Avec le mesh de l'ombre
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelectorFromBoundingBox(node);
#endif
				if (selector)
				{
					node->setTriangleSelector(selector);
					selector->drop();
				}
			}
		}
		break;

		// Buildings
	case BI_building_individuel:
		{
			// Charge seulement les mesh et les textures nécessaires à ce building
			if (forLoadingOnly)
			{
				game->sceneManager->getMesh("Building Individuel.obj");		// Mesh normal
				game->sceneManager->getMesh("Building Individuel Ombre.obj");	// Mesh de l'ombre et du triangle selector

				// Charge la texture du building
				{
					io::path textureName = "Buildings_Textures";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					game->driver->getTexture(textureName);
				}

				if (gameConfig.normalMappingEnabled && normalMappingMaterial != -1)
				{
					// Charge la texture pour le normal mapping
					io::path textureName = "Buildings_NormalMap";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					game->driver->getTexture(textureName);
				}
				break;
			}

			// Crée le node de ce bâtiment :
			if (!gameConfig.normalMappingEnabled || normalMappingMaterial == -1)
			{
				node = game->sceneManager->addAnimatedMeshSceneNode(
					game->sceneManager->getMesh("Building Individuel.obj"), parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling);
				if (!node)
					break;
			}
			else
			{
				// Charge cette mesh en demandant à Irrlicht de créer ses tangentes pour permettre l'utilisation du normal mapping
				scene::IMesh* tangentMesh = game->sceneManager->getMeshManipulator()->createMeshWithTangents(game->sceneManager->getMesh("Building Individuel.obj"));
				scene::IAnimatedMesh* tangentAnimatedMesh = game->sceneManager->getMeshManipulator()->createAnimatedMesh(tangentMesh);
				node = game->sceneManager->addAnimatedMeshSceneNode(tangentAnimatedMesh, parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling);
				if (tangentMesh)			tangentMesh->drop();
				if (tangentAnimatedMesh)	tangentAnimatedMesh->drop();
				if (!node)
					break;

				// Charge la texture pour le normal mapping et indique au node qu'on active le normal mapping
				{
					io::path textureName = "Buildings_NormalMap";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					node->setMaterialTexture(1, game->driver->getTexture(textureName));
				}
				node->setMaterialType((video::E_MATERIAL_TYPE)normalMappingMaterial);
			}

			// Charge la texture du building
			{
				io::path textureName = "Buildings_Textures";
				textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
				textureName.append(".bmp");
				node->setMaterialTexture(0, game->driver->getTexture(textureName));
			}

			// Crée une ombre pour ce bâtiment
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					game->sceneManager->getMesh("Building Individuel Ombre.obj"));

			if (!isTmpBatiment)
			{
				// Crée le triangle selector du building :
#if 1	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					//((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);				// Avec le mesh normal
					game->sceneManager->getMesh("Building Individuel Ombre.obj"), node);	// Avec le mesh de l'ombre
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelectorFromBoundingBox(node);
#endif
				if (selector)
				{
					node->setTriangleSelector(selector);
					selector->drop();
				}
			}
		}
		break;
	case BI_building_basse_consommation:
		{
			// Charge seulement les mesh et les textures nécessaires à ce building
			if (forLoadingOnly)
			{
				game->sceneManager->getMesh("Building Basse Consommation.obj");		// Mesh normal
				// On utilise l'ombre du building individuel car les deux bâtiments ont exactement la même ombre (économie de mémoire, accélération du chargement)
				//game->sceneManager->getMesh("Building Basse Consommation Ombre.obj");	// Mesh de l'ombre et du triangle selector
				game->sceneManager->getMesh("Building Individuel Ombre.obj");	// Mesh de l'ombre et du triangle selector

				// Charge la texture du building
				{
					io::path textureName = "Buildings_Textures";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					game->driver->getTexture(textureName);
				}

				if (gameConfig.normalMappingEnabled && normalMappingMaterial != -1)
				{
					// Charge la texture pour le normal mapping
					io::path textureName = "Buildings_NormalMap";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					game->driver->getTexture(textureName);
				}
				break;
			}

			// Crée le node de ce bâtiment :
			if (!gameConfig.normalMappingEnabled || normalMappingMaterial == -1)
			{
				node = game->sceneManager->addAnimatedMeshSceneNode(
					game->sceneManager->getMesh("Building Basse Consommation.obj"), parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling);
				if (!node)
					break;
			}
			else
			{
				// Charge cette mesh en demandant à Irrlicht de créer ses tangentes pour permettre l'utilisation du normal mapping
				scene::IMesh* tangentMesh = game->sceneManager->getMeshManipulator()->createMeshWithTangents(game->sceneManager->getMesh("Building Basse Consommation.obj"));
				scene::IAnimatedMesh* tangentAnimatedMesh = game->sceneManager->getMeshManipulator()->createAnimatedMesh(tangentMesh);
				node = game->sceneManager->addAnimatedMeshSceneNode(tangentAnimatedMesh, parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling);
				if (tangentMesh)			tangentMesh->drop();
				if (tangentAnimatedMesh)	tangentAnimatedMesh->drop();
				if (!node)
					break;

				// Charge la texture pour le normal mapping et indique au node qu'on active le normal mapping
				{
					io::path textureName = "Buildings_NormalMap";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					node->setMaterialTexture(1, game->driver->getTexture(textureName));
				}
				node->setMaterialType((video::E_MATERIAL_TYPE)normalMappingMaterial);
			}

			// Charge la texture du building
			{
				io::path textureName = "Buildings_Textures";
				textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
				textureName.append(".bmp");
				node->setMaterialTexture(0, game->driver->getTexture(textureName));
			}

			// Crée une ombre pour ce bâtiment :
			// On utilise l'ombre du building individuel car les deux bâtiments ont exactement la même ombre (économie de mémoire, accélération du chargement)
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					//game->sceneManager->getMesh("Building Basse Consommation Ombre.obj")
					game->sceneManager->getMesh("Building Individuel Ombre.obj"));

			if (!isTmpBatiment)
			{
				// Crée le triangle selector du building :
#if 1	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					//((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);		// Avec le mesh normal
					game->sceneManager->getMesh("Building Individuel Ombre.obj"), node);	// Avec le mesh de l'ombre	// "Building Basse Consommation Ombre.obj"
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelectorFromBoundingBox(node);
#endif
				if (selector)
				{
					node->setTriangleSelector(selector);
					selector->drop();
				}
			}
		}
		break;
	case BI_building_avec_panneaux_solaires:
		{
			// Charge seulement les mesh et les textures nécessaires à ce building
			if (forLoadingOnly)
			{
				game->sceneManager->getMesh("Building avec Panneaux Solaires.obj");		// Mesh normal
				game->sceneManager->getMesh("Building avec Panneaux Solaires Ombre.obj");	// Mesh de l'ombre et du triangle selector

				// Charge la texture du building
				{
					io::path textureName = "Buildings_Textures";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					game->driver->getTexture(textureName);
				}

				if (gameConfig.normalMappingEnabled && normalMappingMaterial != -1)
				{
					// Charge la texture pour le normal mapping
					io::path textureName = "Buildings_NormalMap";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					game->driver->getTexture(textureName);
				}
				break;
			}

			// Crée le node de ce bâtiment :
			if (!gameConfig.normalMappingEnabled || normalMappingMaterial == -1)
			{
				node = game->sceneManager->addAnimatedMeshSceneNode(
					game->sceneManager->getMesh("Building avec Panneaux Solaires.obj"), parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling);
				if (!node)
					break;
			}
			else
			{
				// Charge cette mesh en demandant à Irrlicht de créer ses tangentes pour permettre l'utilisation du normal mapping
				scene::IMesh* tangentMesh = game->sceneManager->getMeshManipulator()->createMeshWithTangents(game->sceneManager->getMesh("Building avec Panneaux Solaires.obj"));
				scene::IAnimatedMesh* tangentAnimatedMesh = game->sceneManager->getMeshManipulator()->createAnimatedMesh(tangentMesh);
				node = game->sceneManager->addAnimatedMeshSceneNode(tangentAnimatedMesh, parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling);
				if (tangentMesh)			tangentMesh->drop();
				if (tangentAnimatedMesh)	tangentAnimatedMesh->drop();
				if (!node)
					break;

				// Charge la texture pour le normal mapping et indique au node qu'on active le normal mapping
				{
					io::path textureName = "Buildings_NormalMap";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					node->setMaterialTexture(1, game->driver->getTexture(textureName));
				}
				node->setMaterialType((video::E_MATERIAL_TYPE)normalMappingMaterial);
			}

			// Charge la texture du building
			{
				io::path textureName = "Buildings_Textures";
				textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
				textureName.append(".bmp");
				node->setMaterialTexture(0, game->driver->getTexture(textureName));
			}

			// Crée une ombre pour ce bâtiment
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					game->sceneManager->getMesh("Building avec Panneaux Solaires Ombre.obj"));

			if (!isTmpBatiment)
			{
				// Crée le triangle selector du building :
#if 1	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					//((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);				// Avec le mesh normal
					game->sceneManager->getMesh("Building avec Panneaux Solaires Ombre.obj"), node);	// Avec le mesh de l'ombre
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelectorFromBoundingBox(node);
#endif
				if (selector)
				{
					node->setTriangleSelector(selector);
					selector->drop();
				}
			}
		}
		break;
	case BI_grand_building_individuel:
		{
			// Charge seulement les mesh et les textures nécessaires à ce building
			if (forLoadingOnly)
			{
				game->sceneManager->getMesh("Grand Building Individuel.obj");			// Mesh normal
				game->sceneManager->getMesh("Grand Building Individuel Ombre.obj");	// Mesh de l'ombre et du triangle selector

				// Charge la texture du building
				{
					io::path textureName = "Buildings_Textures";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					game->driver->getTexture(textureName);
				}

				if (gameConfig.normalMappingEnabled && normalMappingMaterial != -1)
				{
					// Charge la texture pour le normal mapping
					io::path textureName = "Buildings_NormalMap";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					game->driver->getTexture(textureName);
				}
				break;
			}

			// Crée le node de ce bâtiment :
			if (!gameConfig.normalMappingEnabled || normalMappingMaterial == -1)
			{
				node = game->sceneManager->addAnimatedMeshSceneNode(
					game->sceneManager->getMesh("Grand Building Individuel.obj"), parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling);
				if (!node)
					break;
			}
			else
			{
				// Charge cette mesh en demandant à Irrlicht de créer ses tangentes pour permettre l'utilisation du normal mapping
				scene::IMesh* tangentMesh = game->sceneManager->getMeshManipulator()->createMeshWithTangents(game->sceneManager->getMesh("Grand Building Individuel.obj"));
				scene::IAnimatedMesh* tangentAnimatedMesh = game->sceneManager->getMeshManipulator()->createAnimatedMesh(tangentMesh);
				node = game->sceneManager->addAnimatedMeshSceneNode(tangentAnimatedMesh, parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling);
				if (tangentMesh)			tangentMesh->drop();
				if (tangentAnimatedMesh)	tangentAnimatedMesh->drop();
				if (!node)
					break;

				// Charge la texture pour le normal mapping et indique au node qu'on active le normal mapping
				{
					io::path textureName = "Buildings_NormalMap";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					node->setMaterialTexture(1, game->driver->getTexture(textureName));
				}
				node->setMaterialType((video::E_MATERIAL_TYPE)normalMappingMaterial);
			}

			// Charge la texture du building
			{
				io::path textureName = "Buildings_Textures";
				textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
				textureName.append(".bmp");
				node->setMaterialTexture(0, game->driver->getTexture(textureName));
			}

			// Crée une ombre pour ce bâtiment
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					game->sceneManager->getMesh("Grand Building Individuel Ombre.obj"));

			if (!isTmpBatiment)
			{
				// Crée le triangle selector du building :
#if 1	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					//((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);				// Avec le mesh normal
					game->sceneManager->getMesh("Grand Building Individuel Ombre.obj"), node);	// Avec le mesh de l'ombre
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelectorFromBoundingBox(node);
#endif
				if (selector)
				{
					node->setTriangleSelector(selector);
					selector->drop();
				}
			}
		}
		break;

		// Production d'énergie
	case BI_centrale_charbon:
		{
			// TODO

#ifdef USE_MATERIALS_MESHES
			// Indique que ce bâtiment dispose de sa propre couleur de matériaux
			reinitMaterialsDiffuse = false;
#endif

			// Charge seulement les mesh et les textures nécessaires à cette centrale
			if (forLoadingOnly)
			{
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Centrale nucleaire.obj");		// Mesh normal
#else
				game->sceneManager->getMesh("Centrale nucleaire - Avec materiaux.obj");		// Mesh normal avec matériaux
#endif
				//game->sceneManager->getMesh("Centrale nucleaire Ombre.obj");	// Mesh de l'ombre et du triangle selector (non utilisé finalement : à réactiver si ré-utilisé)

				// Charge la texture de la centrale
				//{
				//	io::path textureName = "";
				//	textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
				//	textureName.append(".bmp");
				//	game->driver->getTexture(textureName);
				//}
				break;
			}

			node = game->sceneManager->addAnimatedMeshSceneNode(
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Centrale nucleaire.obj"),
#else
				game->sceneManager->getMesh("Centrale nucleaire - Avec materiaux.obj"),
#endif
				parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling * 2.0f);
			if (!node)
				break;

			// Charge la texture de la centrale
			//{
			//	io::path textureName = "";
			//	textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
			//	textureName.append(".bmp");
			//	node->setMaterialTexture(0, game->driver->getTexture(textureName));
			//}

			// Crée une ombre pour ce bâtiment
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					//game->sceneManager->getMesh("Centrale nucleaire Ombre.obj")
					);

			if (!isTmpBatiment)
			{
				// Crée le triangle selector de la centrale :
#if 1	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);		// Avec le mesh normal
					//game->sceneManager->getMesh("Centrale nucleaire Ombre.obj"), node);	// Avec le mesh de l'ombre
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelectorFromBoundingBox(node);
#endif
				if (selector)
				{
					node->setTriangleSelector(selector);
					selector->drop();
				}
			}
		}
		break;
	case BI_panneau_solaire:
		{
			// Charge seulement les mesh et les textures nécessaires à ce panneau solaire
			if (forLoadingOnly)
			{
				game->sceneManager->getMesh("Panneau Solaire.obj");		// Mesh normal
				//game->sceneManager->getMesh("Panneau Solaire Ombre.obj");	// Mesh de l'ombre et du triangle selector (non utilisé finalement : à réactiver si ré-utilisé)

				// Charge la texture du panneau solaire
				{
					io::path textureName = "Panneau_Solaire_Textures";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					game->driver->getTexture(textureName);
				}

				if (gameConfig.normalMappingEnabled && normalMappingMaterial != -1)
				{
					// Charge la texture pour le normal mapping
					io::path textureName = "Panneau_Solaire_NormalMap";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					game->driver->getTexture(textureName);
				}

				break;
			}

			// Crée le node de ce bâtiment :
			if (!gameConfig.normalMappingEnabled || normalMappingMaterial == -1)
			{
				node = game->sceneManager->addAnimatedMeshSceneNode(
					game->sceneManager->getMesh("Panneau Solaire.obj"), parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling);
				if (!node)
					break;
			}
			else
			{
				// Charge cette mesh en demandant à Irrlicht de créer ses tangentes pour permettre l'utilisation du normal mapping
				scene::IMesh* tangentMesh = game->sceneManager->getMeshManipulator()->createMeshWithTangents(game->sceneManager->getMesh("Panneau Solaire.obj"));
				scene::IAnimatedMesh* tangentAnimatedMesh = game->sceneManager->getMeshManipulator()->createAnimatedMesh(tangentMesh);
				node = game->sceneManager->addAnimatedMeshSceneNode(tangentAnimatedMesh, parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling);
				if (tangentMesh)			tangentMesh->drop();
				if (tangentAnimatedMesh)	tangentAnimatedMesh->drop();
				if (!node)
					break;

				// Charge la texture pour le normal mapping et indique au node qu'on active le normal mapping
				{
					io::path textureName = "Panneau_Solaire_NormalMap";
					textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
					textureName.append(".bmp");
					node->setMaterialTexture(1, game->driver->getTexture(textureName));
				}
				node->setMaterialType((video::E_MATERIAL_TYPE)normalMappingMaterial);
			}

			// Charge la texture du panneau solaire
			{
				io::path textureName = "Panneau_Solaire_Textures";
				textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
				textureName.append(".bmp");
				node->setMaterialTexture(0, game->driver->getTexture(textureName));
			}

			// Crée une ombre pour ce bâtiment
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode();

			if (!isTmpBatiment)
			{
				// Crée le triangle selector du panneau solaire :
#if 1	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);		// Avec le mesh normal
					//game->sceneManager->getMesh("Panneau Solaire Ombre.obj"), node);	// Avec le mesh de l'ombre
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelectorFromBoundingBox(node);
#endif
				if (selector)
				{
					node->setTriangleSelector(selector);
					selector->drop();
				}
			}
		}
		break;
	case BI_eolienne:
		{
#ifdef USE_MATERIALS_MESHES
			// Indique que ce bâtiment dispose de sa propre couleur de matériaux
			reinitMaterialsDiffuse = false;
#endif

			// Charge seulement les mesh et les textures nécessaires à cette éolienne
			if (forLoadingOnly)
			{
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Eolienne.x");		// Mesh normal
#else
				game->sceneManager->getMesh("Eolienne - Avec materiaux.x");		// Mesh normal avec matériaux
#endif
				//game->sceneManager->getMesh("Eolienne Ombre.x");	// Mesh de l'ombre et du triangle selector (non utilisé finalement : à réactiver si ré-utilisé)

				// Charge la texture de l'éolienne
				//{
				//	io::path textureName = "";
				//	textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
				//	textureName.append(".bmp");
				//	game->driver->getTexture(textureName);
				//}
				break;
			}

			node = game->sceneManager->addAnimatedMeshSceneNode(
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Eolienne.x"),
#else
				game->sceneManager->getMesh("Eolienne - Avec materiaux.x"),
#endif
				parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling);
			if (!node)
				break;

			// Charge la texture de l'éolienne
			//{
			//	io::path textureName = "";
			//	textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
			//	textureName.append(".bmp");
			//	node->setMaterialTexture(0, game->driver->getTexture(textureName));
			//}

#ifdef _DEBUG
			// Affiche les données de débogage sur l'éolienne
			//node->setDebugDataVisible(scene::EDS_FULL & ~(scene::EDS_HALF_TRANSPARENCY));
#endif

			// Modifie la vitesse de l'animation (à 15 fps) et lui ajoute une valeur aléatoire entre -2.5f et 2.5f
			// -> Une éolienne a une vitesse de rotation d'environ : 1/3 tour/seconde
			((scene::IAnimatedMeshSceneNode*)node)->setAnimationSpeed(15.0f + ((rand() % 51) - 25) * 0.1f);

			// Désactive les ombres pour les éoliennes : buggées et trop coûteuses en temps de rendu
			//if (gameConfig.stencilShadowsEnabled)
			//	((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
			//		//game->sceneManager->getMesh("Eolienne Ombre.x")
			//		);

			if (!isTmpBatiment)
			{
				// Crée le triangle selector de l'éolienne :
#if 0	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)	// Désactivé : la caméra FPS peut être coincée dans ce triangle selector !
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);	// Avec le mesh normal
					//game->sceneManager->getMesh("Eolienne Ombre.x"), node);			// Avec le mesh de l'ombre
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelectorFromBoundingBox(node);
#endif
				if (selector)
				{
					node->setTriangleSelector(selector);
					selector->drop();
				}
			}
		}
		break;
	case BI_hydrolienne:
		{
#ifdef USE_MATERIALS_MESHES
			// Indique que ce bâtiment dispose de sa propre couleur de matériaux
			reinitMaterialsDiffuse = false;
#endif

			// Charge seulement les mesh et les textures nécessaires à cette hydrolienne
			if (forLoadingOnly)
			{
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Hydrolienne.x");		// Mesh normal
#else
				game->sceneManager->getMesh("Hydrolienne - Avec materiaux.x");		// Mesh normal avec matériaux
#endif
				//game->sceneManager->getMesh("Hydrolienne Ombre.x");	// Mesh de l'ombre et du triangle selector (non utilisé finalement : à réactiver si ré-utilisé)

				// Charge la texture de l'hydrolienne
				//{
				//	io::path textureName = "";
				//	textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
				//	textureName.append(".bmp");
				//	game->driver->getTexture(textureName);
				//}
				break;
			}

			node = game->sceneManager->addAnimatedMeshSceneNode(
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Hydrolienne.x"),
#else
				game->sceneManager->getMesh("Hydrolienne - Avec materiaux.x"),
#endif
				parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling * 2.0f);
			if (!node)
				break;

			// Charge la texture de l'hydrolienne
			//{
			//	io::path textureName = "";
			//	textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
			//	textureName.append(".bmp");
			//	node->setMaterialTexture(0, game->driver->getTexture(textureName));
			//}

#ifdef _DEBUG
			// Affiche les données de débogage sur l'hydrolienne
			//node->setDebugDataVisible(scene::EDS_FULL & ~(scene::EDS_HALF_TRANSPARENCY));
#endif

			// Modifie la vitesse de l'animation (à 15 fps) et lui ajoute une valeur aléatoire entre -2.5f et 2.5f
			// -> Les hydroliennes ont une fréquence de rotation d'environ 20 tours/min (15 frames par seconde)
			((scene::IAnimatedMeshSceneNode*)node)->setAnimationSpeed(15.0f + ((rand() % 51) - 25) * 0.1f);

			// Désactive les ombres pour les hydroliennes : buggées et trop coûteuses en temps de rendu
			//if (gameConfig.stencilShadowsEnabled)
			//	((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
			//		game->sceneManager->getMesh("Hydrolienne Ombre.x"));

			if (!isTmpBatiment)
			{
				// Crée le triangle selector de l'hydrolienne :
#if 0	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)	// Désactivé : la caméra FPS peut être coincée dans ce triangle selector !
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					//((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);	// Avec le mesh normal
					game->sceneManager->getMesh("Hydrolienne Ombre.x"), node);		// Avec le mesh de l'ombre
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelectorFromBoundingBox(node);
#endif
				if (selector)
				{
					node->setTriangleSelector(selector);
					selector->drop();
				}
			}
		}
		break;

		// Production d'eau
	case BI_pompe_extraction_eau:
		{
#ifdef USE_MATERIALS_MESHES
			// Indique que ce bâtiment dispose de sa propre couleur de matériaux
			reinitMaterialsDiffuse = false;
#endif

			// Charge seulement les mesh et les textures nécessaires à cette hydrolienne
			if (forLoadingOnly)
			{
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Pompe d'extraction d'eau.x");		// Mesh normal
#else
				game->sceneManager->getMesh("Pompe d'extraction d'eau - Avec materiaux.x");		// Mesh normal avec matériaux
#endif
				//game->sceneManager->getMesh("Pompe d'extraction d'eau Ombre.x");	// Mesh de l'ombre et du triangle selector (non utilisé finalement : à réactiver si ré-utilisé)

				// Charge la texture de la pompe d'extraction d'eau
				//{
				//	io::path textureName = "";
				//	textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
				//	textureName.append(".bmp");
				//	game->driver->getTexture(textureName);
				//}
				break;
			}

			node = game->sceneManager->addAnimatedMeshSceneNode(
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Pompe d'extraction d'eau.x"),
#else
				game->sceneManager->getMesh("Pompe d'extraction d'eau - Avec materiaux.x"),
#endif
				parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling);
			if (!node)
				break;

			// Charge la texture de la pompe d'extraction d'eau
			//{
			//	io::path textureName = "";
			//	textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
			//	textureName.append(".bmp");
			//	node->setMaterialTexture(0, game->driver->getTexture(textureName));
			//}

#ifdef _DEBUG
			// Affiche les données de débogage sur la pompe d'extraction d'eau
			//node->setDebugDataVisible(scene::EDS_FULL & ~(scene::EDS_HALF_TRANSPARENCY));
#endif

			// Modifie la vitesse de l'animation (à 15 fps) et lui ajoute une valeur aléatoire entre -2.5f et 2.5f
			// -> Les pompes d'extraction d'eau ont une fréquence de rotation d'environ 20 tours/min (15 frames par seconde)
			((scene::IAnimatedMeshSceneNode*)node)->setAnimationSpeed(15.0f + ((rand() % 51) - 25) * 0.1f);

			// Désactive les ombres pour les pompes d'extraction d'eau : buggées et trop coûteuses en temps de rendu
			//if (gameConfig.stencilShadowsEnabled)
			//	((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
			//		game->sceneManager->getMesh("Pompe d'extraction d'eau Ombre.x"));

			if (!isTmpBatiment)
			{
				// Crée le triangle selector de l'hydrolienne :
#if 1	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)	// Désactivé : la caméra FPS peut être coincée dans ce triangle selector !
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);					// Avec le mesh normal
					//game->sceneManager->getMesh("Pompe d'extraction d'eau Ombre.x"), node);	// Avec le mesh de l'ombre
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelectorFromBoundingBox(node);
#endif
				if (selector)
				{
					node->setTriangleSelector(selector);
					selector->drop();
				}
			}
		}
		break;

		// Usines
	case BI_usine_verre_petite:
		{
#ifdef USE_MATERIALS_MESHES
			// Indique que ce bâtiment dispose de sa propre couleur de matériaux
			reinitMaterialsDiffuse = false;
#endif

			// Charge seulement les mesh et les textures nécessaires à cette usine
			if (forLoadingOnly)
			{
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Usine de verre.obj");		// Mesh normal
#else
				game->sceneManager->getMesh("Usine de verre - Avec materiaux.obj");		// Mesh normal avec matériaux
#endif
				//game->sceneManager->getMesh("Usine de verre Ombre.obj");	// Mesh de l'ombre et du triangle selector

				// Charge la texture de l'usine
				//{
				//	io::path textureName = "Usines_Textures";
				//	textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
				//	textureName.append(".bmp");
				//	game->driver->getTexture(textureName);
				//}
				break;
			}

			// Petite usine :

			node = game->sceneManager->addAnimatedMeshSceneNode(
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Usine de verre.obj"),
#else
				game->sceneManager->getMesh("Usine de verre - Avec materiaux.obj"),
#endif
				parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling);
			if (!node)
				break;

			// Charge la texture de l'usine
			//{
			//	io::path textureName = "Usines_Textures";
			//	textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
			//	textureName.append(".bmp");
			//	node->setMaterialTexture(0, game->driver->getTexture(textureName));
			//}

			// Crée une ombre pour ce bâtiment
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					//game->sceneManager->getMesh("Usine de verre Ombre.obj")
					);

			if (!isTmpBatiment)
			{
				// Crée le triangle selector de l'usine :
#if 1	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);	// Avec le mesh normal
					//game->sceneManager->getMesh("Usine de verre Ombre.x"), node);	// Avec le mesh de l'ombre
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelectorFromBoundingBox(node);
#endif
				if (selector)
				{
					node->setTriangleSelector(selector);
					selector->drop();
				}
			}
		}
		break;
	case BI_usine_verre_grande:
		{
#ifdef USE_MATERIALS_MESHES
			// Indique que ce bâtiment dispose de sa propre couleur de matériaux
			reinitMaterialsDiffuse = false;
#endif

			// Charge seulement les mesh et les textures nécessaires à cette usine
			if (forLoadingOnly)
			{
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Usine de verre.obj");		// Mesh normal
#else
				game->sceneManager->getMesh("Usine de verre - Avec materiaux.obj");		// Mesh normal avec matériaux
#endif
				//game->sceneManager->getMesh("Usine de verre Ombre.obj");	// Mesh de l'ombre et du triangle selector

				// Charge la texture de l'usine
				//{
				//	io::path textureName = "Usines_Textures";
				//	textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
				//	textureName.append(".bmp");
				//	game->driver->getTexture(textureName);
				//}
				break;
			}

			// Grande usine :

			// Crée le node parent des quatre petites usines qui composeront cette grande usine
			node = game->sceneManager->addEmptySceneNode(parent);
			if (!node)
				break;

			// Indique l'agrandissement du node pour qu'il soit effectif pour ses enfants
			node->setScale(blenderObjectsScaling);

			// Charge la texture de l'usine
			//video::ITexture* usineTex = NULL;
			//{
			//	io::path textureName = "Usines_Textures";
			//	textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
			//	textureName.append(".bmp");
			//	usineTex = game->driver->getTexture(textureName);
			//}

			// Crée un meta triangle selector pour contenir les selectors des 4 usines et les assigner à leur node parent
			scene::IMetaTriangleSelector* metaTriSelector = NULL;
			if (!isTmpBatiment)
				metaTriSelector = game->sceneManager->createMetaTriangleSelector();

			// Crée les quatres usines côtes à côtes avec des rotations différentes
			scene::IAnimatedMeshSceneNode* nodes[4] = {0};
			for (int i = 0; i < 4; ++i)
			{
				nodes[i] = game->sceneManager->addAnimatedMeshSceneNode(
#ifndef USE_MATERIALS_MESHES
					game->sceneManager->getMesh("Usine de verre.obj"),
#else
					game->sceneManager->getMesh("Usine de verre - Avec materiaux.obj"),
#endif
					node, -1, core::vector3df(((i % 2) * 10.0f) - 5.0f, 0.0f, (i > 1 ? 10.0f : 0.0f) - 5.0f),
					core::vector3df(0.0f, i * 90.0f, 0.0f));
				if (!nodes[i])
					continue;

				//nodes[i]->setMaterialTexture(0, usineTex);

				// Crée une ombre pour ce bâtiment
				if (gameConfig.stencilShadowsEnabled)
					nodes[i]->addShadowVolumeSceneNode(
						//game->sceneManager->getMesh("Usine de verre Ombre.obj")
						);

				if (!isTmpBatiment)
				{
					// Crée le triangle selector de l'usine :
#if 1	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)
					scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
						nodes[i]->getMesh(), nodes[i]);									// Avec le mesh normal
						//game->sceneManager->getMesh("Usine de verre Ombre.x"), nodes[i]);	// Avec le mesh de l'ombre
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis)
					scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelectorFromBoundingBox(nodes[i]);
#endif
					if (selector)
					{
						if (metaTriSelector)
							metaTriSelector->addTriangleSelector(selector);
						selector->drop();
					}
				}
			}

			if (!isTmpBatiment)
			{
				// Indique le triangle selector du node
				if (metaTriSelector)
				{
					node->setTriangleSelector(metaTriSelector);
					metaTriSelector->drop();
				}
			}
		}
		break;
	case BI_usine_ciment_petite:
		{
#ifdef USE_MATERIALS_MESHES
			// Indique que ce bâtiment dispose de sa propre couleur de matériaux
			reinitMaterialsDiffuse = false;
#endif

			// Charge seulement les mesh et les textures nécessaires à cette usine
			if (forLoadingOnly)
			{
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Usine de ciment.obj");		// Mesh normal
#else
				game->sceneManager->getMesh("Usine de ciment - Avec materiaux.obj");		// Mesh normal avec matériaux
#endif
				//game->sceneManager->getMesh("Usine de ciment Ombre.obj");	// Mesh de l'ombre et du triangle selector

				// Charge la texture de l'usine
				//{
				//	io::path textureName = "Usines_Textures";
				//	textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
				//	textureName.append(".bmp");
				//	game->driver->getTexture(textureName);
				//}
				break;
			}

			// Petite usine :

			node = game->sceneManager->addAnimatedMeshSceneNode(
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Usine de ciment.obj"),
#else
				game->sceneManager->getMesh("Usine de ciment - Avec materiaux.obj"),
#endif
				parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling);
			if (!node)
				break;

			// Charge la texture de l'usine
			//{
			//	io::path textureName = "Usines_Textures";
			//	textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
			//	textureName.append(".bmp");
			//	node->setMaterialTexture(0, game->driver->getTexture(textureName));
			//}

			// Crée une ombre pour ce bâtiment
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					//game->sceneManager->getMesh("Usine de ciment Ombre.obj")
					);

			if (!isTmpBatiment)
			{
				// Crée le triangle selector de l'usine :
#if 1	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);	// Avec le mesh normal
					//game->sceneManager->getMesh("Usine de ciment Ombre.x"), node);	// Avec le mesh de l'ombre
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelectorFromBoundingBox(node);
#endif
				if (selector)
				{
					node->setTriangleSelector(selector);
					selector->drop();
				}
			}
		}
		break;
	case BI_usine_ciment_grande:
		{
#ifdef USE_MATERIALS_MESHES
			// Indique que ce bâtiment dispose de sa propre couleur de matériaux
			reinitMaterialsDiffuse = false;
#endif

			// Charge seulement les mesh et les textures nécessaires à cette usine
			if (forLoadingOnly)
			{
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Usine de ciment.obj");		// Mesh normal
#else
				game->sceneManager->getMesh("Usine de ciment - Avec materiaux.obj");		// Mesh normal avec matériaux
#endif
				//game->sceneManager->getMesh("Usine de ciment Ombre.obj");	// Mesh de l'ombre et du triangle selector

				// Charge la texture de l'usine
				//{
				//	io::path textureName = "Usines_Textures";
				//	textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
				//	textureName.append(".bmp");
				//	game->driver->getTexture(textureName);
				//}
				break;
			}

			// Grande usine :

			// Crée le node parent des quatre petites usines qui composeront cette grande usine
			node = game->sceneManager->addEmptySceneNode(parent);
			if (!node)
				break;

			// Indique l'agrandissement du node pour qu'il soit effectif pour ses enfants
			node->setScale(blenderObjectsScaling);

			// Charge la texture de l'usine
			//video::ITexture* usineTex = NULL;
			//{
			//	io::path textureName = "Usines_Textures";
			//	textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
			//	textureName.append(".bmp");
			//	usineTex = game->driver->getTexture(textureName);
			//}

			// Crée un meta triangle selector pour contenir les selectors des 4 usines et les assigner à leur node parent
			scene::IMetaTriangleSelector* metaTriSelector = NULL;
			if (!isTmpBatiment)
				metaTriSelector = game->sceneManager->createMetaTriangleSelector();

			// Crée les quatres usines côtes à côtes avec des rotations différentes
			scene::IAnimatedMeshSceneNode* nodes[4] = {0};
			for (int i = 0; i < 4; ++i)
			{
				nodes[i] = game->sceneManager->addAnimatedMeshSceneNode(
#ifndef USE_MATERIALS_MESHES
					game->sceneManager->getMesh("Usine de ciment.obj"),
#else
					game->sceneManager->getMesh("Usine de ciment - Avec materiaux.obj"),
#endif
					node, -1, core::vector3df(((i % 2) * 10.0f) - 5.0f, 0.0f, (i > 1 ? 10.0f : 0.0f) - 5.0f),
					core::vector3df(0.0f, i * 90.0f, 0.0f));
				if (!nodes[i])
					continue;

				//nodes[i]->setMaterialTexture(0, usineTex);

				// Crée une ombre pour ce bâtiment
				if (gameConfig.stencilShadowsEnabled)
					nodes[i]->addShadowVolumeSceneNode(
						//game->sceneManager->getMesh("Usine de ciment Ombre.obj")
						);

				if (!isTmpBatiment)
				{
					// Crée le triangle selector de l'usine :
#if 1	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)
					scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
						nodes[i]->getMesh(), nodes[i]);									// Avec le mesh normal
						//game->sceneManager->getMesh("Usine de ciment Ombre.x"), nodes[i]);	// Avec le mesh de l'ombre
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis)
					scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelectorFromBoundingBox(nodes[i]);
#endif
					if (selector)
					{
						if (metaTriSelector)
							metaTriSelector->addTriangleSelector(selector);
						selector->drop();
					}
				}
			}

			if (!isTmpBatiment)
			{
				// Indique le triangle selector du node
				if (metaTriSelector)
				{
					node->setTriangleSelector(metaTriSelector);
					metaTriSelector->drop();
				}
			}
		}
		break;
	case BI_usine_tuiles_petite:
		{
#ifdef USE_MATERIALS_MESHES
			// Indique que ce bâtiment dispose de sa propre couleur de matériaux
			reinitMaterialsDiffuse = false;
#endif

			// Charge seulement les mesh et les textures nécessaires à cette usine
			if (forLoadingOnly)
			{
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Usine de tuiles.obj");		// Mesh normal
#else
				game->sceneManager->getMesh("Usine de tuiles - Avec materiaux.obj");		// Mesh normal avec matériaux
#endif
				//game->sceneManager->getMesh("Usine de tuiles Ombre.obj");	// Mesh de l'ombre et du triangle selector

				// Charge la texture de l'usine
				//{
				//	io::path textureName = "Usines_Textures";
				//	textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
				//	textureName.append(".bmp");
				//	game->driver->getTexture(textureName);
				//}
				break;
			}

			// Petite usine :

			node = game->sceneManager->addAnimatedMeshSceneNode(
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Usine de tuiles.obj"),
#else
				game->sceneManager->getMesh("Usine de tuiles - Avec materiaux.obj"),
#endif
				parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling);
			if (!node)
				break;

			// Charge la texture de l'usine
			//{
			//	io::path textureName = "Usines_Textures";
			//	textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
			//	textureName.append(".bmp");
			//	node->setMaterialTexture(0, game->driver->getTexture(textureName));
			//}

			// Crée une ombre pour ce bâtiment
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					//game->sceneManager->getMesh("Usine de tuiles Ombre.obj")
					);

			if (!isTmpBatiment)
			{
				// Crée le triangle selector de l'usine :
#if 1	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);	// Avec le mesh normal
					//game->sceneManager->getMesh("Usine de tuiles Ombre.x"), node);	// Avec le mesh de l'ombre
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelectorFromBoundingBox(node);
#endif
				if (selector)
				{
					node->setTriangleSelector(selector);
					selector->drop();
				}
			}
		}
		break;
	case BI_usine_tuiles_grande:
		{
#ifdef USE_MATERIALS_MESHES
			// Indique que ce bâtiment dispose de sa propre couleur de matériaux
			reinitMaterialsDiffuse = false;
#endif

			// Charge seulement les mesh et les textures nécessaires à cette usine
			if (forLoadingOnly)
			{
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Usine de tuiles.obj");		// Mesh normal
#else
				game->sceneManager->getMesh("Usine de tuiles - Avec materiaux.obj");		// Mesh normal avec matériaux
#endif
				//game->sceneManager->getMesh("Usine de tuiles Ombre.obj");	// Mesh de l'ombre et du triangle selector

				// Charge la texture de l'usine
				//{
				//	io::path textureName = "Usines_Textures";
				//	textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
				//	textureName.append(".bmp");
				//	game->driver->getTexture(textureName);
				//}
				break;
			}

			// Grande usine :

			// Crée le node parent des quatre petites usines qui composeront cette grande usine
			node = game->sceneManager->addEmptySceneNode(parent);
			if (!node)
				break;

			// Indique l'agrandissement du node pour qu'il soit effectif pour ses enfants
			node->setScale(blenderObjectsScaling);

			// Charge la texture de l'usine
			//video::ITexture* usineTex = NULL;
			//{
			//	io::path textureName = "Usines_Textures";
			//	textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
			//	textureName.append(".bmp");
			//	usineTex = game->driver->getTexture(textureName);
			//}

			// Crée un meta triangle selector pour contenir les selectors des 4 usines et les assigner à leur node parent
			scene::IMetaTriangleSelector* metaTriSelector = NULL;
			if (!isTmpBatiment)
				metaTriSelector = game->sceneManager->createMetaTriangleSelector();

			// Crée les quatres usines côtes à côtes avec des rotations différentes
			scene::IAnimatedMeshSceneNode* nodes[4] = {0};
			for (int i = 0; i < 4; ++i)
			{
				nodes[i] = game->sceneManager->addAnimatedMeshSceneNode(
#ifndef USE_MATERIALS_MESHES
					game->sceneManager->getMesh("Usine de tuiles.obj"),
#else
					game->sceneManager->getMesh("Usine de tuiles - Avec materiaux.obj")	,
#endif
					node, -1, core::vector3df(((i % 2) * 10.0f) - 5.0f, 0.0f, (i > 1 ? 10.0f : 0.0f) - 5.0f),
					core::vector3df(0.0f, i * 90.0f, 0.0f));
				if (!nodes[i])
					continue;

				//nodes[i]->setMaterialTexture(0, usineTex);

				// Crée une ombre pour ce bâtiment
				if (gameConfig.stencilShadowsEnabled)
					nodes[i]->addShadowVolumeSceneNode(
						//game->sceneManager->getMesh("Usine de tuiles Ombre.obj")
						);

				if (!isTmpBatiment)
				{
					// Crée le triangle selector de l'usine :
#if 1	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)
					scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
						nodes[i]->getMesh(), nodes[i]);									// Avec le mesh normal
						//game->sceneManager->getMesh("Usine de tuiles Ombre.x"), nodes[i]);	// Avec le mesh de l'ombre
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis)
					scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelectorFromBoundingBox(nodes[i]);
#endif
					if (selector)
					{
						if (metaTriSelector)
							metaTriSelector->addTriangleSelector(selector);
						selector->drop();
					}
				}
			}

			if (!isTmpBatiment)
			{
				// Indique le triangle selector du node
				if (metaTriSelector)
				{
					node->setTriangleSelector(metaTriSelector);
					metaTriSelector->drop();
				}
			}
		}
		break;
#ifdef KID_VERSION	// En mode enfant : on réactiver le papier comme ressource consommée par les habitants pour leur satisfaction
	case BI_usine_papier_petite:
		{
#ifdef USE_MATERIALS_MESHES
			// Indique que ce bâtiment dispose de sa propre couleur de matériaux
			reinitMaterialsDiffuse = false;
#endif

			// Charge seulement les mesh et les textures nécessaires à cette usine
			if (forLoadingOnly)
			{
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Usine de papier.obj");		// Mesh normal
#else
				game->sceneManager->getMesh("Usine de papier - Avec materiaux.obj");		// Mesh normal avec matériaux
#endif
				//game->sceneManager->getMesh("Usine de papier Ombre.obj");	// Mesh de l'ombre et du triangle selector

				// Charge la texture de l'usine
				//{
				//	io::path textureName = "Usines_Textures";
				//	textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
				//	textureName.append(".bmp");
				//	game->driver->getTexture(textureName);
				//}
				break;
			}

			node = game->sceneManager->addAnimatedMeshSceneNode(
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Usine de papier.obj"),
#else
				game->sceneManager->getMesh("Usine de papier - Avec materiaux.obj"),
#endif
				parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling);
			if (!node)
				break;

			// Charge la texture de l'usine
			//{
			//	io::path textureName = "Usines_Textures";
			//	textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
			//	textureName.append(".bmp");
			//	node->setMaterialTexture(0, game->driver->getTexture(textureName));
			//}

			// Crée une ombre pour ce bâtiment
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					//game->sceneManager->getMesh("Usine de papier Ombre.obj")
					);

			if (!isTmpBatiment)
			{
				// Crée le triangle selector de l'usine :
#if 1	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);	// Avec le mesh normal
					//game->sceneManager->getMesh("Usine de papier Ombre.x"), node);	// Avec le mesh de l'ombre
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelectorFromBoundingBox(node);
#endif
				if (selector)
				{
					node->setTriangleSelector(selector);
					selector->drop();
				}
			}
		}
		break;
	case BI_usine_papier_grande:
#else				// On désactive l'usine à tout faire en mode enfant qui est devenue inutile
	case BI_usine_tout:
#endif
		{
#ifdef USE_MATERIALS_MESHES
			// Indique que ce bâtiment dispose de sa propre couleur de matériaux
			reinitMaterialsDiffuse = false;
#endif

			// Charge seulement les mesh et les textures nécessaires à cette usine
			if (forLoadingOnly)
			{
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Usine de papier.obj");		// Mesh normal
#else
				game->sceneManager->getMesh("Usine de papier - Avec materiaux.obj");		// Mesh normal avec matériaux
#endif
				//game->sceneManager->getMesh("Usine de papier Ombre.obj");	// Mesh de l'ombre et du triangle selector

				// Charge la texture de l'usine
				//{
				//	io::path textureName = "Usines_Textures";
				//	textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
				//	textureName.append(".bmp");
				//	game->driver->getTexture(textureName);
				//}
				break;
			}

			// Grande usine :

			// Crée le node parent des quatre petites usines qui composeront cette grande usine
			node = game->sceneManager->addEmptySceneNode(parent);
			if (!node)
				break;

			// Indique l'agrandissement du node pour qu'il soit effectif pour ses enfants
			node->setScale(blenderObjectsScaling);

			// Charge la texture de l'usine
			//video::ITexture* usineTex = NULL;
			//{
			//	io::path textureName = "Usines_Textures";
			//	textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
			//	textureName.append(".bmp");
			//	usineTex = game->driver->getTexture(textureName);
			//}

			// Crée un meta triangle selector pour contenir les selectors des 4 usines et les assigner à leur node parent
			scene::IMetaTriangleSelector* metaTriSelector = NULL;
			if (!isTmpBatiment)
				metaTriSelector = game->sceneManager->createMetaTriangleSelector();

			// Crée les quatres usines côtes à côtes avec des rotations différentes
			scene::IAnimatedMeshSceneNode* nodes[4] = {0};
			for (int i = 0; i < 4; ++i)
			{
				nodes[i] = game->sceneManager->addAnimatedMeshSceneNode(
#ifndef USE_MATERIALS_MESHES
					game->sceneManager->getMesh("Usine de papier.obj"),
#else
					game->sceneManager->getMesh("Usine de papier - Avec materiaux.obj"),
#endif
					node, -1, core::vector3df(((i % 2) * 10.0f) - 5.0f, 0.0f, (i > 1 ? 10.0f : 0.0f) - 5.0f),
					core::vector3df(0.0f, i * 90.0f, 0.0f));
				if (!nodes[i])
					continue;

				//nodes[i]->setMaterialTexture(0, usineTex);

				// Crée une ombre pour ce bâtiment
				if (gameConfig.stencilShadowsEnabled)
					nodes[i]->addShadowVolumeSceneNode(
						//game->sceneManager->getMesh("Usine de papier Ombre.obj")
						);

				if (!isTmpBatiment)
				{
					// Crée le triangle selector de l'usine :
#if 1	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)
					scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
						nodes[i]->getMesh(), nodes[i]);									// Avec le mesh normal
						//game->sceneManager->getMesh("Usine de papier Ombre.x"), nodes[i]);	// Avec le mesh de l'ombre
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis)
					scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelectorFromBoundingBox(nodes[i]);
#endif
					if (selector)
					{
						if (metaTriSelector)
							metaTriSelector->addTriangleSelector(selector);
						selector->drop();
					}
				}
			}

			if (!isTmpBatiment)
			{
				// Indique le triangle selector du node
				if (metaTriSelector)
				{
					node->setTriangleSelector(metaTriSelector);
					metaTriSelector->drop();
				}
			}
		}
		break;

		// Gestion de l'effet de serre et des déchets
	case BI_decharge:
		{
			// TODO

#ifdef USE_MATERIALS_MESHES
			// Indique que ce bâtiment dispose de sa propre couleur de matériaux
			reinitMaterialsDiffuse = false;
#endif

			// Charge seulement les mesh et les textures nécessaires à cette décharge
			if (forLoadingOnly)
			{
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Usine de papier.obj");		// Mesh normal
#else
				game->sceneManager->getMesh("Usine de papier - Avec materiaux.obj");		// Mesh normal avec matériaux
#endif
				//game->sceneManager->getMesh("Usine de papier Ombre.obj");	// Mesh de l'ombre et du triangle selector

				// Charge la texture de la décharge
				//{
				//	io::path textureName = "";
				//	textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
				//	textureName.append(".bmp");
				//	game->driver->getTexture(textureName);
				//}
				break;
			}

			node = game->sceneManager->addAnimatedMeshSceneNode(
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Usine de papier.obj"),
#else
				game->sceneManager->getMesh("Usine de papier - Avec materiaux.obj"),
#endif
				parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling);
			if (!node)
				break;

			// Charge la texture de la décharge
			//{
			//	io::path textureName = "";
			//	textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
			//	textureName.append(".bmp");
			//	node->setMaterialTexture(0, game->driver->getTexture(textureName));
			//}

			// Crée une ombre pour ce bâtiment
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					//game->sceneManager->getMesh("Usine de papier Ombre.obj")
					);

			if (!isTmpBatiment)
			{
				// Crée le triangle selector de la décharge :
#if 1	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);	// Avec le mesh normal
					//game->sceneManager->getMesh("Usine de papier Ombre.x"), node);	// Avec le mesh de l'ombre
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelectorFromBoundingBox(node);
#endif
				if (selector)
				{
					node->setTriangleSelector(selector);
					selector->drop();
				}
			}
		}
		break;
	case BI_usine_incineration_dechets:
		{
			// TODO

#ifdef USE_MATERIALS_MESHES
			// Indique que ce bâtiment dispose de sa propre couleur de matériaux
			reinitMaterialsDiffuse = false;
#endif

			// Charge seulement les mesh et les textures nécessaires à cette décharge
			if (forLoadingOnly)
			{
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Centrale nucleaire.obj");		// Mesh normal
#else
				game->sceneManager->getMesh("Centrale nucleaire - Avec materiaux.obj");		// Mesh normal avec matériaux
#endif
				//game->sceneManager->getMesh("Centrale nucleaire Ombre.obj");	// Mesh de l'ombre et du triangle selector

				// Charge la texture de l'usine d'incinération
				//{
				//	io::path textureName = "";
				//	textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
				//	textureName.append(".bmp");
				//	game->driver->getTexture(textureName);
				//}
				break;
			}

			node = game->sceneManager->addAnimatedMeshSceneNode(
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Centrale nucleaire.obj"),
#else
				game->sceneManager->getMesh("Centrale nucleaire - Avec materiaux.obj"),
#endif
				parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling * 2.0f);
			if (!node)
				break;

			// Charge la texture de l'usine d'incinération
			//{
			//	io::path textureName = "";
			//	textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
			//	textureName.append(".bmp");
			//	node->setMaterialTexture(0, game->driver->getTexture(textureName));
			//}

			// Crée une ombre pour ce bâtiment
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					//game->sceneManager->getMesh("Centrale nucleaire Ombre.obj")
					);

			if (!isTmpBatiment)
			{
				// Crée le triangle selector de l'usine d'incinération :
#if 1	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);		// Avec le mesh normal
					//game->sceneManager->getMesh("Centrale nucleaire Ombre.x"), node);	// Avec le mesh de l'ombre
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelectorFromBoundingBox(node);
#endif
				if (selector)
				{
					node->setTriangleSelector(selector);
					selector->drop();
				}
			}
		}
		break;

		// Arbres
	case BI_arbre_aspen:
	case BI_arbre_oak:
	case BI_arbre_pine:
	case BI_arbre_willow:
		{
			core::stringc arbreType = "Aspen";
			switch (batimentID)
			{
				case BI_arbre_aspen:	/* arbreType = "Aspen"; */	break;
				case BI_arbre_oak:		arbreType = "Oak";			break;
				case BI_arbre_pine:		arbreType = "Pine";			break;
				case BI_arbre_willow:	arbreType = "Willow";		break;
			}

			// Obtient les fichiers liés au type d'arbre "arbreType"
			sprintf_SS("%s.xml", arbreType.c_str());

			scene::CTreeGenerator* generator = NULL;
			if (!isTmpBatiment || forLoadingOnly)	// On initialise le générateur si ce n'est pas un bâtiment temporaire ou si on est en mode de chargement
			{
				generator = new scene::CTreeGenerator(game->sceneManager);
				io::IXMLReader* xml = game->fileSystem->createXMLReader(text_SS);
				if (xml)
				{
					generator->loadFromXML(xml);
					xml->drop();
				}
			}

			sprintf_SS("%sBark.png", arbreType.c_str());
			video::ITexture* const treeTexture = game->driver->getTexture(text_SS);

			sprintf_SS("%sLeaf.png", arbreType.c_str());
			video::ITexture* const leafTexture = game->driver->getTexture(text_SS);

			sprintf_SS("%sBillboard.png", arbreType.c_str());
			video::ITexture* const billTexture = game->driver->getTexture(text_SS);



			// Calcule l'index de cet arbre dans la liste des arbres de prévisualisation
			const int index = (int)batimentID - (int)BI_arbre_aspen;

			// Génère un nombre aléatoire pour la création de l'arbre
			const int seed = rand();

			// Les pointeurs sur les mesh buffers des deux premiers niveaux de détail de l'arbre
			scene::STreeMesh* highLOD = NULL;
			scene::STreeMesh* midLOD = NULL;
			if (forLoadingOnly)
			{
				// Si doit juste charger les arbres, on ne crée que leur buffer de prévisualisation
				if (!treesHighLOD[index])
				{
					treesHighLOD[index] = generator->generateTree(3, seed, true, 0);	// On DOIT laisser le cutoffLevel du niveau de LOD le plus haut à 0 pour que sa bounding box soit correctement calculée
					highLOD = treesHighLOD[index];
				}
				if (!treesMidLOD[index])
				{
					treesMidLOD[index] = generator->generateTree(3, seed, false, 2);
					midLOD = treesMidLOD[index];
				}
			}
#ifdef USE_VARIOUS_TREES	// Génère un nouvel arbre unique
			else if (!isTmpBatiment)
			{
				// Génère un nouvel arbre s'il n'est pas temporaire
				highLOD = generator->generateTree(3, seed, true, 0);	// Par défaut : (8, seed, true, 0)
				midLOD = generator->generateTree(3, seed, false, 2);	// Par défaut : (4, seed, false, 1)
			}
#endif						// Sinon, on utilise les arbres de prévisualisation pour le rendu
			else
			{
				// Finalement, si l'arbre est temporaire, on prend celui qui est déjà créé
				highLOD = treesHighLOD[index];
				midLOD = treesMidLOD[index];
			}

			// Calcule les proportions des agrandissements pour que l'arbre ait une taille en X et en Z de TAILLE_OBJETS, et une taille de 100.0f pour TAILLE_OBJETS = 40.0f en Y (on utilisera TAILLE_OBJETS * 0.9f pour que les feuilles de l'arbre ne dépassnt pas trop de la place disponible)
			core::vector3df treeScale = core::vector3df(0.2f);
			if (highLOD)
			{
				const core::vector3df currentTreeSize = highLOD->MeshBuffer->BoundingBox.getExtent();
				const core::vector3df desiredTreeSize(TAILLE_OBJETS * 0.9f, 2.25f * TAILLE_OBJETS, TAILLE_OBJETS * 0.9f);	// en Y : 100.0f * 0.9f * TAILLE_OBJETS / 40.0f = 2.25f * TAILLE_OBJETS
#ifdef _DEBUG
				if (core::equals(currentTreeSize.X, 0.0f) || core::equals(currentTreeSize.Y, 0.0f) || core::equals(currentTreeSize.Z, 0.0f))
					LOG_DEBUG("EcoWorldRenderer::loadBatiment : Creation de l'arbre (" << arbreType.c_str() << ") : Calcul de l'agrandissement :" << endl <<
						"    currentTreeSize a un indice nul : X=" << currentTreeSize.X << " Y=" << currentTreeSize.Y << "Z=" << currentTreeSize.Z, ELL_WARNING);
#endif

				// Et par produit en croix, on a :
				treeScale = desiredTreeSize / currentTreeSize;
			}

			if (forLoadingOnly)	// Si on ne doit que charger le mesh buffer de l'arbre, on quitte ici avant de créer son node
			{
				if (generator)
					generator->drop();

				break;
			}

			// Crée l'arbre
			scene::CTreeSceneNode* const tree = new scene::CTreeSceneNode(parent, game->sceneManager);

			// On n'utilise que le niveau de détail moyen si l'arbre est temporaire
			tree->setup(highLOD, midLOD, billTexture);

#ifdef USE_VARIOUS_TREES
			if (!forLoadingOnly && !isTmpBatiment)
			{
				// Libère les mesh des arbres s'ils ont été générés
				highLOD->drop();
				midLOD->drop();
			}
#endif

			tree->setScale(treeScale);

			scene::CBillboardGroupSceneNode* const leafNode = tree->getLeafNode();
			if (leafNode)
			{
				leafNode->setMaterialTexture(0, leafTexture);
				leafNode->setMaterialType(video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF);
			}
			tree->getMaterial(0).setTexture(0, treeTexture);

			// Modifie les distances de LOD de l'arbre
			tree->setDistances(5.0f * TAILLE_OBJETS, 20.0f * TAILLE_OBJETS);	// (100.0f, 400.0f) pour TAILLE_OBJETS = 20.0f

			// TODO : Créer des ombres pour les arbres
			//if (gameConfig.stencilShadowsEnabled) // Aucune ombre ne peut être crée

			// Indique le node à renvoyer
			node = tree;

			if (!isTmpBatiment)
			{
				// Crée le triangle selector de l'arbre :
#if 0
				// Désactivé : les triangle selector obtenus étaient tellements complexes qu'ils bloquaient la caméra FPS (elle n'arrivait plus à ressortir des arbres)
				// Triangle selector précis d'après les triangles du buffer de niveau de détail moyen
				scene::SMesh selectorMesh;
				selectorMesh.addMeshBuffer(midLOD->MeshBuffer);
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(&selectorMesh, tree);
				tree->setTriangleSelector(selector);
				selector->drop();
#else
				// Triangle selector d'après la bounding box de l'arbre (qui est prise d'après la bounding box de son plus haut niveau de détail)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelectorFromBoundingBox(tree);
				tree->setTriangleSelector(selector);
				selector->drop();
#endif
			}

			// Libère la mémoire
			tree->drop();
			if (generator)
				generator->drop();
		}
		break;

	default:
		LOG_DEBUG("EcoWorldRenderer::loadBatiment(" << batimentID << ") : Type de batiment inconnu : " << batimentID, ELL_WARNING);
		break;
	}

	// Modifie quelques propriétés des matériaux de ce node et de ses enfants
	if (node)
	{
		// Détermine si on doit activer ou désactiver les lumières dynamiques, suivant si les lumières sont gérées par Irrlicht ou game->xEffects
		const bool enableLighting = !(gameConfig.useXEffectsShadows && game->xEffects);

		// Propriétés des matériaux du node :
		const u32 nodeMatCount = node->getMaterialCount();
		const bool nodeNormalizeNormals = !(node->getScale().equals(core::vector3df(1.0f), 0.01f));	// Détermine si ce node est agrandi/rétréci pour savoir si on doit activer la normalisation des normales
		for (u32 i = 0; i < nodeMatCount; ++i)
		{
			// Obtient le matériau actuel
			video::SMaterial& mat = node->getMaterial(i);

			mat.Lighting = enableLighting;				// Active/Désactive les lumières dynamiques suivant si les lumières sont gérées par Irrlicht ou xEffects
#ifdef USE_MATERIALS_MESHES
			if (reinitMaterialsDiffuse)
			{
#endif
				mat.DiffuseColor.set(255, 255, 255, 255);	// Active la lumière diffuse
				mat.AmbientColor.set(255, 255, 255, 255);	// Active la lumière ambiante
#ifdef USE_MATERIALS_MESHES
			}
			else
				mat.AmbientColor = mat.DiffuseColor;	// Indique comme lumière ambiante la même valeur que la lumière diffuse, pour des raisons de cohérence des lumières (aussi lié avec la fonction Game::resetBatimentColor)
#endif
			mat.Shininess = 0.0f;						// Enlève les reflets de lumière

			mat.NormalizeNormals = nodeNormalizeNormals;// Active la normalisation des normales si ce node est agrandi/rétréci
			mat.FogEnable = true;						// Active le brouillard
		}

		// Vérifie que ce node est temporaire (destiné à la prévisualisation), sinon on ne lui ajoute ni effet de flou de profondeur, ni d'ombre : ces effets seront ajoutés plus loin dans EcoWorldRenderer::addBatiment
		if (isTmpBatiment)
		{
			// Ajoute l'effet de flou de profondeur à ce node grâce à PostProcess (ses enfants seront aussi affectés)
			if (gameConfig.usePostProcessEffects && gameConfig.postProcessUseDepthRendering && game->postProcessManager)
				game->postProcessManager->addNodeToDepthPass(node);

			// Ajoute une ombre à ce node grâce à XEffects, et lui permet aussi de les recevoir (ses enfants seront aussi affectés)
			if (gameConfig.useXEffectsShadows && game->xEffects)
				game->xEffects->addShadowToNode(node, ESM_BOTH);
		}



		// Propriété des matériaux des enfants :
		const core::list<scene::ISceneNode*>& nodeList = node->getChildren();
		const core::list<scene::ISceneNode*>::ConstIterator END = nodeList.end();
		for (core::list<scene::ISceneNode*>::ConstIterator it = nodeList.begin(); it != END; ++it)
		{
			scene::ISceneNode* const child = (*it);

			const u32 childMatCount = child->getMaterialCount();
			const bool childNormalizeNormals = !(child->getScale().equals(core::vector3df(1.0f), 0.01f));	// Détermine si ce node est agrandi/rétréci pour savoir si on doit activer la normalisation des normales
			for (u32 i = 0; i < childMatCount; ++i)
			{
				// Obtient le matériau actuel
				video::SMaterial& mat = child->getMaterial(i);

				mat.Lighting = enableLighting;				// Active/Désactive les lumières dynamiques suivant si les lumières sont gérées par Irrlicht ou xEffects
#ifdef USE_MATERIALS_MESHES
				if (reinitMaterialsDiffuse)
				{
#endif
					mat.DiffuseColor.set(255, 255, 255, 255);	// Active la lumière diffuse
					mat.AmbientColor.set(255, 255, 255, 255);	// Active la lumière ambiante
#ifdef USE_MATERIALS_MESHES
				}
				else
					mat.AmbientColor = mat.DiffuseColor;	// Indique comme lumière ambiante la même valeur que la lumière diffuse, pour des raisons de cohérence des lumières (aussi lié avec la fonction Game::resetBatimentColor)
#endif
				mat.Shininess = 0.0f;						// Enlève les reflets de lumière

				mat.NormalizeNormals = childNormalizeNormals;// Active la normalisation des normales si ce node est agrandi/rétréci
				mat.FogEnable = true;						// Active le brouillard
			}
		}
	}

	return node;
}
scene::ISceneNode* EcoWorldRenderer::loadConstructingBatiment(BatimentID batimentID, scene::ISceneNode* parent, bool forLoadingOnly)
{
	if (batimentID == BI_aucun)
		return NULL;
	if (!parent)
		parent = game->sceneManager->getRootSceneNode();

	// Permet de créer un centre de construction entre les bords de la construction
	// Peut être désactivé car le bâtiment comporte toujours une dalle en béton en-dessous de lui, qui peut donc ainsi faire office de centre de construction
//#define USE_CONSTRUCTION_CENTER

#ifdef USE_CONSTRUCTION_CENTER
	// Permet d'utiliser un grand cube pour le centre de construction, au lieu de charger le fichier "Construction Center.obj" qui ne représente actuellement qu'un simple plan texturé et de le répeter de nombreuses fois (un seul cube sera créé)
	// (Avec le plan du fichier 3D, les mêmes problèmes de clignotement que pour la route plane se font sentir lorsque l'objet est à grande distance de la caméra)
#define USE_CUBE_CONSTRUCTION_CENTER
#endif

	// Permet d'utiliser de grands plans pour les bords de la construction, au lieu de charger le fichier "Construction Border.obj" qui ne représente actuellement qu'un simple plan texturé et de le répeter de nombreuses fois
#define USE_PLANE_CONSTRUCTION_BORDER

	// Charge seulement les mesh et les textures nécessaires à la construction
	if (forLoadingOnly)
	{
#ifndef USE_PLANE_CONSTRUCTION_BORDER
		game->sceneManager->getMesh("Construction Border.obj");		// Bord : Mesh normal
		//game->sceneManager->getMesh("Construction Border Ombre.obj");	// Bord : Mesh de l'ombre et du triangle selector
#endif
#ifdef USE_CONSTRUCTION_CENTER
#ifndef USE_CUBE_CONSTRUCTION_CENTER
		game->sceneManager->getMesh("Construction Center.obj");		// Centre : Mesh normal
		//game->sceneManager->getMesh("Construction Center Ombre.obj");	// Centre : Mesh de l'ombre et du triangle selector
#endif
#endif

		// Charge les texture de la construction
		{
			io::path textureName = "Construction_Border_Texture";
			textureName.append(getBatimentTextureSuffixe(false, gameConfig.texturesQuality));	// Cette texture n'est pas disponible en puissances de deux : elle y est par défaut
			textureName.append(".bmp");
			game->driver->getTexture(textureName);

#ifdef USE_CONSTRUCTION_CENTER
			textureName = "Construction_Center_Texture";
			textureName.append(getBatimentTextureSuffixe(false, gameConfig.texturesQuality));	// Cette texture n'est pas disponible en puissances de deux : elle y est par défaut
			textureName.append(".bmp");
			game->driver->getTexture(textureName);
#endif
		}

		return NULL;
	}

	// Obtient la taille du bâtiment désiré
	const core::dimension2du& size = StaticBatimentInfos::getInfos(batimentID).taille;
	const core::dimension2df& halfSize = StaticBatimentInfos::getInfos(batimentID).halfTaille;

	// Agrandis un peu les batiments exportés avec Blender, car les unités d'Irrlicht sont plus grandes que celles de Blender
	// 2.0f sous Blender <=> 1.0f sous EcoWorld avec : 20.0f / 40.0f * TAILLE_OBJETS = TAILLE_OBJETS * 0.5f
	const core::vector3df blenderObjectsScaling(TAILLE_OBJETS * 0.5f);

	// Le node parent de tous les élements de la construction
	scene::ISceneNode* const node = game->sceneManager->addEmptySceneNode(parent);

	// Crée un meta triangle selector pour contenir les selectors des bords et les assigner à leur node parent
	scene::IMetaTriangleSelector* metaTriSelector = game->sceneManager->createMetaTriangleSelector();

	// Variables d'itération :
	u32 i
#if !defined(USE_PLANE_CONSTRUCTION_BORDER) || (defined(USE_CONSTRUCTION_CENTER) && !defined(USE_CUBE_CONSTRUCTION_CENTER))
		, j , k
#endif
		;
	scene::IAnimatedMeshSceneNode* tmpNode;		// Node temporaire pour la création des élements du node de la construction
	scene::ITriangleSelector* selector;			// Triangle Selector temporaire pour la création des triangles selectors des élements du node de la construction

#ifndef USE_PLANE_CONSTRUCTION_BORDER
	// Crée les bords du haut et du bas
	for (i = 0; i < size.Width; ++i)
	{
		for (j = 0; j < 2; ++j)	// Permet de faire Haut puis Bas en changeant seulement la position et la rotation du node
		{
			tmpNode = game->sceneManager->addAnimatedMeshSceneNode(
				game->sceneManager->getMesh("Construction Border.obj"), node, -1,
				core::vector3df(((float)i - halfSize.Width) * 2.0f + 1.0f, 0.0f, halfSize.Height * (j == 0 ? -2.0f : 2.0f)) * blenderObjectsScaling,
				core::vector3df(0.0f, j * 180.0f, 0.0f),
				blenderObjectsScaling);
			if (!tmpNode)
				continue;

			// Charge la texture de la construction
			{
				io::path textureName = "Construction_Border_Texture";
				textureName.append(getBatimentTextureSuffixe(false, gameConfig.texturesQuality));	// Cette texture n'est pas disponible en puissances de deux : elle y est par défaut
				textureName.append(".bmp");
				tmpNode->setMaterialTexture(0, game->driver->getTexture(textureName));
			}

			// Indique que ce node a une texture transparente
#if 0	// Texture précise complètement désactivée : beaucoup trop lente (-> la texture a été recréée avec ces deux niveaux de transparence seulement)
		// + Ne fonctionne pas avec game->xEffects
			if (gameConfig.texturesQuality == GameConfiguration::ETQ_HIGH || gameConfig.texturesQuality == GameConfiguration::ETQ_MEDIUM)
				tmpNode->setMaterialType(video::EMT_TRANSPARENT_ALPHA_CHANNEL);		// Texture transparente précise mais lente (utilise tous les niveaux de transparence de l'image)
			else
#endif
				tmpNode->setMaterialType(video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF);	// Texture transparente peu précise mais rapide (utilise seulement 2 niveaux de transparence)

			// Modifie quelques propriétés des matériaux
			for (k = 0; k < tmpNode->getMaterialCount(); ++k)
			{
				video::SMaterial& mat = tmpNode->getMaterial(k);			// Récupère le matériau actif
				mat.Shininess = 0.0f;										// Enlève les reflets de lumière
				mat.AmbientColor = video::SColor(255, 255, 255, 255);		// Active la lumière ambiante
			}
			tmpNode->setMaterialFlag(video::EMF_NORMALIZE_NORMALS, true);
			tmpNode->setMaterialFlag(video::EMF_FOG_ENABLE, true);			// Active le brouillard
			tmpNode->setMaterialFlag(video::EMF_BACK_FACE_CULLING, false);	// Désactive le culling

			if (gameConfig.anisotropicFilterEnabled)
				tmpNode->setMaterialFlag(video::EMF_ANISOTROPIC_FILTER, true);

			if (gameConfig.stencilShadowsEnabled)
				tmpNode->addShadowVolumeSceneNode(
					//game->sceneManager->getMesh("Construction Border Ombre.obj")
					);
	}

	// Crée les bords de gauche et de droite
	for (i = 0; i < size.Height; ++i)
	{
		for (j = 0; j < 2; ++j)	// Permet de faire Gauche puis Droite en changeant seulement la position et la rotation du node
		{
			tmpNode = game->sceneManager->addAnimatedMeshSceneNode(
				game->sceneManager->getMesh("Construction Border.obj"), node, -1,
				core::vector3df(halfSize.Width * (j == 0 ? -2.0f : 2.0f), 0.0f, ((float)i - halfSize.Height) * 2.0f + 1.0f) * blenderObjectsScaling,
				core::vector3df(0.0f, 90.0f + j * 180.0f, 0.0f),
				blenderObjectsScaling);
			if (!tmpNode)
				continue;

			// Charge la texture de la construction
			{
				io::path textureName = "Construction_Border_Texture";
				textureName.append(getBatimentTextureSuffixe(false, gameConfig.texturesQuality));	// Cette texture n'est pas disponible en puissances de deux : elle y est par défaut
				textureName.append(".bmp");
				tmpNode->setMaterialTexture(0, game->driver->getTexture(textureName));
			}

			// Indique que ce node a une texture transparente
#if 0	// Texture précise complètement désactivée : beaucoup trop lente
		// + Ne fonctionne pas avec game->xEffects
			if (gameConfig.texturesQuality == GameConfiguration::ETQ_HIGH || gameConfig.texturesQuality == GameConfiguration::ETQ_MEDIUM)
				tmpNode->setMaterialType(video::EMT_TRANSPARENT_ALPHA_CHANNEL);		// Texture transparente précise mais lente (utilise tous les niveaux de transparence de l'image)
			else
#endif
				tmpNode->setMaterialType(video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF);	// Texture transparente peu précise mais rapide (utilise seulement 2 niveaux de transparence)

			// Modifie quelques propriétés des matériaux
			for (k = 0; k < tmpNode->getMaterialCount(); ++k)
			{
				video::SMaterial& mat = tmpNode->getMaterial(k);			// Récupère le matériau actif
				mat.Shininess = 0.0f;										// Enlève les reflets de lumière
				mat.AmbientColor = video::SColor(255, 255, 255, 255);		// Active la lumière ambiante
			}
			tmpNode->setMaterialFlag(video::EMF_NORMALIZE_NORMALS, true);
			tmpNode->setMaterialFlag(video::EMF_FOG_ENABLE, true);			// Active le brouillard
			tmpNode->setMaterialFlag(video::EMF_BACK_FACE_CULLING, false);	// Désactive le culling

			if (gameConfig.anisotropicFilterEnabled)
				tmpNode->setMaterialFlag(video::EMF_ANISOTROPIC_FILTER, true);

			if (gameConfig.stencilShadowsEnabled)
				tmpNode->addShadowVolumeSceneNode(
					//game->sceneManager->getMesh("Construction Border Ombre.obj")
					);

			// Crée le triangle selector de ce bord :
#if 0	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)
			selector = game->sceneManager->createTriangleSelector(tmpNode->getMesh(), tmpNode);
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis) (-> surtout moins buggé dans ce cas !)
			selector = game->sceneManager->createTriangleSelectorFromBoundingBox(tmpNode);
#endif
			if (selector)
			{
				if (metaTriSelector)
					metaTriSelector->addTriangleSelector(selector);
				selector->drop();
			}
		}
	}
#else
	scene::IMesh* borderMesh = NULL;
	scene::SAnimatedMesh* borderAnimatedMesh = NULL;
	for (i = 0; i < 4; ++i)	// Permet de faire Haut puis Bas, puis Gauche puis Droite en changeant seulement la position et la rotation du node
	{
		const bool parallelToXAxis = ((i % 2) == 0);
		const bool firstSide = (i < 2);

		borderMesh = game->sceneManager->getGeometryCreator()->createPlaneMesh(
			core::dimension2df((parallelToXAxis ? size.Width : size.Height) * TAILLE_OBJETS, TAILLE_OBJETS), core::dimension2du(1, 1), NULL,
			core::dimension2df((float)(parallelToXAxis ? size.Width : size.Height), 1.0f));
		borderAnimatedMesh = NULL;
		tmpNode = NULL;

		if (borderMesh)
		{
			borderAnimatedMesh = new SAnimatedMesh(borderMesh);
			borderMesh->drop();

			if (borderAnimatedMesh)
			{
				tmpNode = game->sceneManager->addAnimatedMeshSceneNode(borderAnimatedMesh, node, -1, core::vector3df(
						(parallelToXAxis ? 0.0f : ((firstSide ? -halfSize.Width : halfSize.Width) * TAILLE_OBJETS)),
						TAILLE_OBJETS * 0.5f,
						(parallelToXAxis ? ((firstSide ? -halfSize.Height : halfSize.Height) * TAILLE_OBJETS) : 0.0f)),
					core::vector3df(-90.0f, i * 90.0f, 0.0f));
				borderAnimatedMesh->drop();

				if (tmpNode)
				{
					// Charge la texture de la construction
					{
						io::path textureName = "Construction_Border_Texture";
						textureName.append(getBatimentTextureSuffixe(false, gameConfig.texturesQuality));	// Cette texture n'est pas disponible en puissances de deux : elle y est par défaut
						textureName.append(".bmp");
						tmpNode->setMaterialTexture(0, game->driver->getTexture(textureName));
					}

					// Indique que ce node a une texture transparente
#if 0	// Texture précise complètement désactivée : beaucoup trop lente (-> la texture a été recréée avec ces deux niveaux de transparence seulement)
		// + Ne fonctionne pas avec XEffects
					if (gameConfig.texturesQuality == GameConfiguration::ETQ_HIGH || gameConfig.texturesQuality == GameConfiguration::ETQ_MEDIUM)
						tmpNode->setMaterialType(video::EMT_TRANSPARENT_ALPHA_CHANNEL);		// Texture transparente précise mais lente (utilise tous les niveaux de transparence de l'image)
					else
#endif
						tmpNode->setMaterialType(video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF);	// Texture transparente peu précise mais rapide (utilise seulement 2 niveaux de transparence)

					tmpNode->setMaterialFlag(video::EMF_FOG_ENABLE, true);			// Active le brouillard
					tmpNode->setMaterialFlag(video::EMF_BACK_FACE_CULLING, false);	// Désactive le culling

					if (gameConfig.stencilShadowsEnabled)
						tmpNode->addShadowVolumeSceneNode();

					// Crée le triangle selector de ce bord :
#if 0	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)
					selector = game->sceneManager->createTriangleSelector(tmpNode->getMesh(), tmpNode);
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis) (-> surtout moins buggé dans ce cas !)
					selector = game->sceneManager->createTriangleSelectorFromBoundingBox(tmpNode);
#endif
					if (selector)
					{
						if (metaTriSelector)
							metaTriSelector->addTriangleSelector(selector);
						selector->drop();
					}
				}
			}
		}
	}
#endif

#ifdef USE_CONSTRUCTION_CENTER
	// Crée les centres
#ifndef USE_CUBE_CONSTRUCTION_CENTER
	for (i = 0; i < size.Width; ++i)
	{
		for (j = 0; j < size.Height; ++j)
		{
			tmpNode = game->sceneManager->addAnimatedMeshSceneNode(
				game->sceneManager->getMesh("Construction Center.obj"), node, -1,
				core::vector3df(((float)i - halfSize.Width) * 2.0f + 1.0f, 0.0f, ((float)j - halfSize.Height) * 2.0f + 1.0f) * blenderObjectsScaling,
				core::vector3df(0.0f, (rand() % 4) * 90.0f, 0.0f),		// Rotation en Y aléatoire
				blenderObjectsScaling);
			if (!tmpNode)
				continue;

			// Charge la texture de la construction
			{
				io::path textureName = "Construction_Center_Texture";
				textureName.append(getBatimentTextureSuffixe(false, gameConfig.texturesQuality));	// Cette texture n'est pas disponible en puissances de deux : elle y est par défaut
				textureName.append(".bmp");
				tmpNode->setMaterialTexture(0, game->driver->getTexture(textureName));
			}

			// Modifie quelques propriétés des matériaux
			for (k = 0; k < tmpNode->getMaterialCount(); ++k)
			{
				video::SMaterial& mat = tmpNode->getMaterial(k);			// Récupère le matériau actif
				mat.Shininess = 0.0f;										// Enlève les reflets de lumière
				mat.AmbientColor = video::SColor(255, 255, 255, 255);		// Active la lumière ambiante
			}
			tmpNode->setMaterialFlag(video::EMF_NORMALIZE_NORMALS, true);
			tmpNode->setMaterialFlag(video::EMF_FOG_ENABLE, true);			// Active le brouillard
			tmpNode->setMaterialFlag(video::EMF_BACK_FACE_CULLING, false);	// Désactive le culling

			if (gameConfig.anisotropicFilterEnabled)
				tmpNode->setMaterialFlag(video::EMF_ANISOTROPIC_FILTER, true);

			if (gameConfig.stencilShadowsEnabled)
				tmpNode->addShadowVolumeSceneNode(
					//game->sceneManager->getMesh("Construction Center Ombre.obj")
					);

			// Crée le triangle selector de ce centre :
#if 1	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis)
			selector = game->sceneManager->createTriangleSelector(tmpNode->getMesh(), tmpNode);
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis) (-> Pas dans le cas d'un plan comme utilisé ici => Ce cas-là n'est pas buggé (le "bug" provient des normales des faces : la collision ne fonctionne que dans un sens))
			selector = game->sceneManager->createTriangleSelectorFromBoundingBox(tmpNode);
#endif

			if (selector)
			{
				if (metaTriSelector)
					metaTriSelector->addTriangleSelector(selector);
				selector->drop();
			}
		}
	}
#else
	{
		scene::IMesh* const centerMesh = game->sceneManager->getGeometryCreator()->createCubeMesh(
			core::vector3df(TAILLE_OBJETS * size.Width, CUBE_ROAD_HEIGHT, TAILLE_OBJETS * size.Height));

		// Modifie le nombre de répetitions de la texture du centre sur les face du cube
		if (centerMesh)
		{
			scene::IMeshBuffer* const mb = centerMesh->getMeshBuffer(0);
			if (mb)
			{
				// Vérifie que le type de vertices utilisé est bien le type standard et que notre cube comporte bien 12 vertices
				if (mb->getVertexType() == video::EVT_STANDARD && mb->getVertexCount() == 12)
				{
					// Obtient les vertices de ce mesh
					video::S3DVertex* vertices = (video::S3DVertex*)mb->getVertices();

					// Mutliplie chaque coordonnée de texture de chaque vertice par l'agrandissement de la texture
					// TODO : Les textures sont beaucoup trop étirées en hauteur sur les côtés du cube, résoudre ce problème
					for (i = 0; i < 12; ++i)
					{
						vertices[i].TCoords.X *= (float)size.Height;
						vertices[i].TCoords.Y *= (float)size.Width;
					}
				}
			}

			scene::IMeshSceneNode* const tmpCubeNode = game->sceneManager->addMeshSceneNode(centerMesh, node);
			centerMesh->drop();
			if (tmpCubeNode)
			{
				// Charge la texture de la construction
				{
					io::path textureName = "Construction_Center_Texture";
					textureName.append(getBatimentTextureSuffixe(false, gameConfig.texturesQuality));	// Cette texture n'est pas disponible en puissances de deux : elle y est par défaut
					textureName.append(".bmp");
					tmpCubeNode->setMaterialTexture(0, game->driver->getTexture(textureName));
				}

				tmpCubeNode->setMaterialFlag(video::EMF_FOG_ENABLE, true);			// Active le brouillard
				//tmpCubeNode->setMaterialFlag(video::EMF_GOURAUD_SHADING, false);	// Désactive l'éclairage Gouraud pour éviter des effets de lumière peu cohérents

				if (gameConfig.anisotropicFilterEnabled)
					tmpCubeNode->setMaterialFlag(video::EMF_ANISOTROPIC_FILTER, true);

				// Crée le triangle selector de ce centre :
#if 0	// Si activé : grâce à sa géométrie 3D (plus lent, mais plus précis) (-> Pas dans le cas d'un cube comme utilisé ici : ici, les deux cas sont équivalents)
				selector = game->sceneManager->createTriangleSelector(tmpCubeNode->getMesh(), tmpCubeNode);
#else	// Sinon : grâce à sa bounding box (plus rapide, mais moins précis)
				selector = game->sceneManager->createTriangleSelectorFromBoundingBox(tmpCubeNode);
#endif
				if (selector)
				{
					if (metaTriSelector)
						metaTriSelector->addTriangleSelector(selector);
					selector->drop();
				}
			}
		}
#endif
	}
#endif

	// Indique le triangle selector du node
	if (metaTriSelector)
	{
		node->setTriangleSelector(metaTriSelector);
		metaTriSelector->drop();
	}

	// Retourne enfin le node créé
	return node;
}
scene::ISceneNode* EcoWorldRenderer::loadConcreteBatiment(float denivele, BatimentID batimentID, scene::ISceneNode* parent, bool forLoadingOnly, bool isTmpBatiment)
{
	if (batimentID == BI_aucun)
		return NULL;

	// Vérifie que ce bâtiment nécessite un bloc de béton sous lui
	if (!StaticBatimentInfos::needConcreteUnderBatiment(batimentID))
		return NULL;

	if (!parent)
		parent = game->sceneManager->getRootSceneNode();

	// Charge la texture nécessaire à la dalle en béton
	if (forLoadingOnly)
	{
		game->driver->getTexture("Concrete.jpg");

		// Charge la texture pour le normal mapping
		if (gameConfig.normalMappingEnabled && normalMappingMaterial != -1)
			game->driver->getTexture("Concrete_NormalMap.bmp");

		return NULL;
	}

	// Obtient la taille du bâtiment désiré
	const core::dimension2du& size = StaticBatimentInfos::getInfos(batimentID).taille;
	const core::dimension2df sizeF((float)size.Width, (float)size.Height);
	const core::vector3df nodeSize(
		sizeF.Width * TAILLE_OBJETS,
		(core::ceil32(denivele / TAILLE_OBJETS) + 2.0f) * TAILLE_OBJETS,	// La valeur ajoutée au dénivelé (ici : 2.0f * TAILLE_OBJETS) est réglable suivant la précision moyenne des dénivelés fournis ; Le dénivelé est ici arrondi à la valeur supérieure de TAILLE_OBJETS pour minimiser les problèmes de textures sur le cube
		sizeF.Height * TAILLE_OBJETS);

	// Crée le mesh en cube de la dalle de béton
	// Basé sur CGeometryCreator.cpp d'Irrlicht 1.7.2 : CGeometryCreator::createCubeMesh
	scene::SMesh* mesh = NULL;
	{
		// Schéma pris de CCubeSceneNode.cpp d'Irrlicht 1.7.2 puis modifié :
		/*
		Sur ce schéma, la face inférieure du cube est incluse :

			  /6,8------/5,9      y
			 /  |      / |        ^  z
			/   |     /  |        | /
			13,3------2  |        |/
			|  7,10 - - 4,11      *---->x
			|  /      |  /
			|/        | /
			12,0------1/
		*/
		scene::SMeshBuffer* buffer = new SMeshBuffer();
		
		// Le mutliplicateur d'agrandissement de la texture de béton sur les faces du cube
		// (si les côtés du cube à créer mesurent 1.0f, alors il y aura exactement TEXTURE_SCALE répétitions de la texture par face)
#define TEXTURE_SCALE	0.05f
		const float X = nodeSize.X * TEXTURE_SCALE;
		const float Y = nodeSize.Y * TEXTURE_SCALE;
		const float Z = nodeSize.Z * TEXTURE_SCALE;
		const video::SColor clr(255, 255, 255, 255);

#if 1
		// Crée le cube sans créer sa face inférieure, qui ne sera normalement jamais visible et qui peut poser des problèmes de texturage suivant le dénivelé du terrain indiqué
		// (pour éviter ces problèmes en conservant cette face, il faudrait que TEXTURE_SCALE puisse s'écrire sous la forme : TEXTURE_SCALE = TAILLE_OBJETS * 10^x où x est un entier relatif ; ou augmenter l'arrondissement du dénivelé ci-dessus)

		// Create indices
		const u16 u[30] = {0,2,1, 0,3,2, 1,5,4, 1,2,5, 4,6,7, 4,5,6, 7,11,10, 7,6,11, 3,9,2, 3,8,9};
		buffer->Indices.set_used(30);
		for (u32 i = 0; i < 30; ++i)
			buffer->Indices[i] = u[i];

		// Create vertices
		buffer->Vertices.reallocate(12);
		buffer->Vertices.push_back(video::S3DVertex(-0.5f,-1.0f,-0.5f, -1.0f,-1.0f,-1.0f, clr, 0.0f, Y));				//  0
		buffer->Vertices.push_back(video::S3DVertex( 0.5f,-1.0f,-0.5f,  1.0f,-1.0f,-1.0f, clr, X, Y));					//  1
		buffer->Vertices.push_back(video::S3DVertex( 0.5f, 0.0f,-0.5f,  1.0f, 1.0f,-1.0f, clr, X, 0.0f));				//  2
		buffer->Vertices.push_back(video::S3DVertex(-0.5f, 0.0f,-0.5f, -1.0f, 1.0f,-1.0f, clr, 0.0f, 0.0f));			//  3
		buffer->Vertices.push_back(video::S3DVertex( 0.5f,-1.0f, 0.5f,  1.0f,-1.0f, 1.0f, clr, X + Z, Y));				//  4
		buffer->Vertices.push_back(video::S3DVertex( 0.5f, 0.0f, 0.5f,  1.0f, 1.0f, 1.0f, clr, X + Z, 0.0f));			//  5
		buffer->Vertices.push_back(video::S3DVertex(-0.5f, 0.0f, 0.5f, -1.0f, 1.0f, 1.0f, clr, X * 2.0f + Z, 0.0f));	//  6
		buffer->Vertices.push_back(video::S3DVertex(-0.5f,-1.0f, 0.5f, -1.0f,-1.0f, 1.0f, clr, X * 2.0f + Z, Y));		//  7
		buffer->Vertices.push_back(video::S3DVertex(-0.5f, 0.0f, 0.5f, -1.0f, 1.0f, 1.0f, clr, 0.0f, Z));				//  8
		buffer->Vertices.push_back(video::S3DVertex( 0.5f, 0.0f, 0.5f,  1.0f, 1.0f, 1.0f, clr, X, Z));					//  9
		buffer->Vertices.push_back(video::S3DVertex(-0.5f,-1.0f,-0.5f, -1.0f,-1.0f,-1.0f, clr, (X + Z) * 2.0f, Y));		// 10
		buffer->Vertices.push_back(video::S3DVertex(-0.5f, 0.0f,-0.5f, -1.0f, 1.0f,-1.0f, clr, (X + Z) * 2.0f, 0.0f));	// 11

		for (u32 i = 0; i < 12; ++i)
			buffer->Vertices[i].Pos *= nodeSize;
#else
		// Crée le cube en entier, en incluant sa face inférieure

		// Create indices
		const u16 u[36] = {0,2,1, 0,3,2, 1,5,4, 1,2,5, 4,6,7, 4,5,6, 7,13,12, 7,6,13, 3,9,2, 3,8,9, 0,1,11, 0,11,10};
		buffer->Indices.set_used(36);
		for (u32 i = 0; i < 36; ++i)
			buffer->Indices[i] = u[i];

		// Create vertices
		buffer->Vertices.reallocate(14);
		buffer->Vertices.push_back(video::S3DVertex(-0.5f,-1.0f,-0.5f, -1.0f,-1.0f,-1.0f, clr, 0.0f, Y));				//  0
		buffer->Vertices.push_back(video::S3DVertex( 0.5f,-1.0f,-0.5f,  1.0f,-1.0f,-1.0f, clr, X, Y));					//  1
		buffer->Vertices.push_back(video::S3DVertex( 0.5f, 0.0f,-0.5f,  1.0f, 1.0f,-1.0f, clr, X, 0.0f));				//  2
		buffer->Vertices.push_back(video::S3DVertex(-0.5f, 0.0f,-0.5f, -1.0f, 1.0f,-1.0f, clr, 0.0f, 0.0f));			//  3
		buffer->Vertices.push_back(video::S3DVertex( 0.5f,-1.0f, 0.5f,  1.0f,-1.0f, 1.0f, clr, X + Z, Y));				//  4
		buffer->Vertices.push_back(video::S3DVertex( 0.5f, 0.0f, 0.5f,  1.0f, 1.0f, 1.0f, clr, X + Z, 0.0f));			//  5
		buffer->Vertices.push_back(video::S3DVertex(-0.5f, 0.0f, 0.5f, -1.0f, 1.0f, 1.0f, clr, X * 2.0f + Z, 0.0f));	//  6
		buffer->Vertices.push_back(video::S3DVertex(-0.5f,-1.0f, 0.5f, -1.0f,-1.0f, 1.0f, clr, X * 2.0f + Z, Y));		//  7
		buffer->Vertices.push_back(video::S3DVertex(-0.5f, 0.0f, 0.5f, -1.0f, 1.0f, 1.0f, clr, 0.0f, Z));				//  8
		buffer->Vertices.push_back(video::S3DVertex( 0.5f, 0.0f, 0.5f,  1.0f, 1.0f, 1.0f, clr, X, Z));					//  9
		buffer->Vertices.push_back(video::S3DVertex(-0.5f,-1.0f, 0.5f, -1.0f,-1.0f, 1.0f, clr, 0.0f, Y + Z));			// 10
		buffer->Vertices.push_back(video::S3DVertex( 0.5f,-1.0f, 0.5f,  1.0f,-1.0f, 1.0f, clr, X, Y + Z));				// 11
		buffer->Vertices.push_back(video::S3DVertex(-0.5f,-1.0f,-0.5f, -1.0f,-1.0f,-1.0f, clr, (X + Z) * 2.0f, Y));		// 12
		buffer->Vertices.push_back(video::S3DVertex(-0.5f, 0.0f,-0.5f, -1.0f, 1.0f,-1.0f, clr, (X + Z) * 2.0f, 0.0f));	// 13

		for (u32 i = 0; i < 14; ++i)
			buffer->Vertices[i].Pos *= nodeSize;
#endif

		// Recalculate bounding box
		buffer->recalculateBoundingBox();

		mesh = new scene::SMesh();
		mesh->addMeshBuffer(buffer);
		buffer->drop();
	}

	// Crée le scene node d'après ce mesh :
	scene::IAnimatedMeshSceneNode* node = NULL;
	if (!gameConfig.normalMappingEnabled || normalMappingMaterial == -1)
	{
		scene::SAnimatedMesh* animatedMesh = new scene::SAnimatedMesh(mesh);
		if (mesh)					mesh->drop();

		node = game->sceneManager->addAnimatedMeshSceneNode(animatedMesh, parent);
		if (animatedMesh)			animatedMesh->drop();
		if (!node)					return NULL;
	}
	else
	{
		// Charge cette mesh en demandant à Irrlicht de créer ses tangentes pour permettre l'utilisation du normal mapping
		scene::IMesh* tangentMesh = game->sceneManager->getMeshManipulator()->createMeshWithTangents(mesh);
		scene::IAnimatedMesh* tangentAnimatedMesh = new scene::SAnimatedMesh(tangentMesh);
		node = game->sceneManager->addAnimatedMeshSceneNode(tangentAnimatedMesh, parent);
		if (mesh)					mesh->drop();
		if (tangentMesh)			tangentMesh->drop();
		if (tangentAnimatedMesh)	tangentAnimatedMesh->drop();
		if (!node)					return NULL;

		// Charge la texture pour le normal mapping et indique au node qu'on active le normal mapping
		node->setMaterialTexture(1, game->driver->getTexture("Concrete_NormalMap.bmp"));
		node->setMaterialType((video::E_MATERIAL_TYPE)normalMappingMaterial);
	}

	// Charge la texture principale (de couleur) du node
	node->setMaterialTexture(0, game->driver->getTexture("concrete.jpg"));

	node->setMaterialFlag(video::EMF_FOG_ENABLE, true);			// Active le brouillard
	//node->setMaterialFlag(video::EMF_GOURAUD_SHADING, false);	// Désactive l'éclairage Gouraud pour éviter des effets de lumière peu cohérents

	if (gameConfig.stencilShadowsEnabled)
		node->addShadowVolumeSceneNode();

	// Crée le triangle selector de ce node si ce n'est pas un bâtiment temporaire
	if (!isTmpBatiment)
	{
		scene::ITriangleSelector* const selector = game->sceneManager->createTriangleSelectorFromBoundingBox(node);
		node->setTriangleSelector(selector);
		selector->drop();
	}

	// Retourne enfin le node créé
	return node;
}
scene::ITextSceneNode* EcoWorldRenderer::loadTextBatiment(BatimentID batimentID, scene::ISceneNode* parent)
{
	if (batimentID == BI_aucun)
		return NULL;

	if (!parent)
		parent = game->sceneManager->getRootSceneNode();

	// Le texte que renverra cette fonction
	scene::ITextSceneNode* textNode = NULL;

	switch (batimentID)
	{
#ifndef KID_VERSION	// On désactive les usines en mode enfant : Les ressources qu'elles nécessitent ne peuvent pas être produites !
		// Tests
	case BI_maison:
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 3.8f, 0.0f), parent);	break;
	case BI_route:
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 10.0f, 0.0f), parent);	break;
#endif

		// Maisons
	case BI_maison_individuelle:
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 6.0f, 0.0f), parent);	break;
	case BI_maison_basse_consommation:
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 6.0f, 0.0f), parent);	break;
	case BI_maison_avec_panneaux_solaires:
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 5.0f, 0.0f), parent);	break;
	case BI_grande_maison_individuelle:
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 6.0f, 0.0f), parent);	break;
	case BI_chalet:
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 6.0f, 0.0f), parent);	break;

		// Immeubles
	case BI_immeuble_individuel:
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 10.5f, 0.0f), parent);	break;
	case BI_immeuble_basse_consommation:
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 10.5f, 0.0f), parent);	break;
	case BI_immeuble_avec_panneaux_solaires:
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 11.0f, 0.0f), parent);	break;
	case BI_grand_immeuble_individuel:
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 13.5f, 0.0f), parent);	break;

		// Buildings
	case BI_building_individuel:
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 16.5f, 0.0f), parent);	break;
	case BI_building_basse_consommation:
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 16.5f, 0.0f), parent);	break;
	case BI_building_avec_panneaux_solaires:
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 17.0f, 0.0f), parent);	break;
	case BI_grand_building_individuel:
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 19.5f, 0.0f), parent);	break;

		// Production d'énergie
	case BI_centrale_charbon:
		// TODO
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 8.0f, 0.0f), parent);	break;
	case BI_panneau_solaire:
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 2.5f, 0.0f), parent);	break;
	case BI_eolienne:
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 9.0f, 0.0f), parent);	break;
	case BI_hydrolienne:
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 3.5f, 0.0f), parent);	break;

		// Production d'eau
	case BI_pompe_extraction_eau:
		// TODO
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 7.0f, 0.0f), parent);	break;

		// Usines
	case BI_usine_verre_petite:
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 6.0f, 0.0f), parent);	break;
	case BI_usine_verre_grande:
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 9.0f, 0.0f), parent);	break;
	case BI_usine_ciment_petite:
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 7.0f, 0.0f), parent);	break;
	case BI_usine_ciment_grande:
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 10.0f, 0.0f), parent);	break;
	case BI_usine_tuiles_petite:
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 7.0f, 0.0f), parent);	break;
	case BI_usine_tuiles_grande:
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 10.0f, 0.0f), parent);	break;
#ifdef KID_VERSION	// En mode enfant : on réactiver le papier comme ressource consommée par les habitants pour leur satisfaction
	case BI_usine_papier_petite:
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 6.0f, 0.0f), parent);	break;
	case BI_usine_papier_grande:
#else				// On désactive l'usine à tout faire en mode enfant qui est devenue inutile
	case BI_usine_tout:
#endif
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 9.0f, 0.0f), parent);	break;

		// Gestion de l'effet de serre et des déchets
	case BI_decharge:
		// TODO
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 6.0f, 0.0f), parent);	break;
	case BI_usine_incineration_dechets:
		// TODO
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 8.0f, 0.0f), parent);	break;

		// Arbres
	case BI_arbre_aspen:
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 390.0f, 0.0f), parent);	break;
	case BI_arbre_oak:
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 300.0f, 0.0f), parent);	break;
	case BI_arbre_pine:
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 420.0f, 0.0f), parent);	break;
	case BI_arbre_willow:
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 360.0f, 0.0f), parent);	break;

	default:
		LOG_DEBUG("EcoWorldRenderer::loadTextBatiment(" << batimentID << ") : Type de batiment inconnu : " << batimentID, ELL_WARNING);
		break;
	}

	return textNode;
}
#ifdef USE_SPARK
void EcoWorldRenderer::loadBatimentSmokePositions(core::list<core::vector3df>& outSmoke1Positions, core::list<core::vector3df>& outSmoke2Positions, BatimentID batimentID, const core::vector3df& batScale, float batRotationY)
{
	if (batimentID == BI_aucun)
		return;

	switch (batimentID)
	{
#ifndef KID_VERSION	// En mode enfant : on désactive la maison de base et les routes qui sont inutiles
		// Tests
	case BI_maison:
		break;
	case BI_route:
		break;
#endif

		// Maisons
	case BI_maison_individuelle:
		break;
	case BI_maison_basse_consommation:
		break;
	case BI_maison_avec_panneaux_solaires:
		break;
	case BI_grande_maison_individuelle:
		break;
	case BI_chalet:
		break;

		// Immeubles
	case BI_immeuble_individuel:
		break;
	case BI_immeuble_basse_consommation:
		break;
	case BI_immeuble_avec_panneaux_solaires:
		break;
	case BI_grand_immeuble_individuel:
		break;

		// Buildings
	case BI_building_individuel:
		break;
	case BI_building_basse_consommation:
		break;
	case BI_building_avec_panneaux_solaires:
		break;
	case BI_grand_building_individuel:
		break;

		// Production d'énergie
	case BI_centrale_charbon:
		outSmoke2Positions.push_back(core::vector3df(-2.0f, 3.0f, 2.0f));	// Radius : 1.0f	// (-2.0f, 2.5f, 2.0f)
		outSmoke2Positions.push_back(core::vector3df(2.0f, 3.0f, -2.0f));	// Radius : 1.0f	// (2.0f, 2.5f, -2.0f)
		break;
	case BI_panneau_solaire:
		break;
	case BI_eolienne:
		break;
	case BI_hydrolienne:
		break;

		// Production d'eau
	case BI_pompe_extraction_eau:
		break;

		// Usines
	case BI_usine_verre_petite:
		outSmoke1Positions.push_back(core::vector3df(1.0f, 4.0f, 3.0f));	// Radius : 0.5f
		outSmoke1Positions.push_back(core::vector3df(2.25f, 4.5f, 3.0f));	// Radius : 0.2f
		outSmoke1Positions.push_back(core::vector3df(3.5f, 4.0f, 3.0f));	// Radius : 0.5f
		break;
	case BI_usine_verre_grande:
		{
			core::vector3df pos;
			for (int i = 0; i < 4; ++i)
			{
				// Valeurs obtenues d'après EcoWorldRenderer::loadBatiment : case BI_usine_verre_grande :
				const core::vector3df addedPos(((i % 2) * 10.0f) - 5.0f, 0.0f, (i > 1 ? 10.0f : 0.0f) - 5.0f);

				// Le (i + 1) semble nécessaire pour concorder avec la rotation du bâtiment
				// TODO : Normalement, cet ajustement ne devrait pas être nécessaire !
				const float rotY = (i + 1) * 90.0f;

				pos = core::vector3df(1.0f, 4.0f, 3.0f);			// Radius : 0.5f
				pos.rotateXZBy(rotY);
				pos += addedPos;
				outSmoke1Positions.push_back(pos);

				pos = core::vector3df(2.25f, 4.5f, 3.0f);			// Radius : 0.2f
				pos.rotateXZBy(rotY);
				pos += addedPos;
				outSmoke1Positions.push_back(pos);

				pos = core::vector3df(3.5f, 4.0f, 3.0f);			// Radius : 0.5f
				pos.rotateXZBy(rotY);
				pos += addedPos;
				outSmoke1Positions.push_back(pos);
			}
		}
		break;
	case BI_usine_ciment_petite:
		outSmoke1Positions.push_back(core::vector3df(-3.0f, 5.0f, -3.0f));	// Radius : 0.5f
		outSmoke1Positions.push_back(core::vector3df(-1.0f, 5.0f, -3.0f));	// Radius : 0.5f
		outSmoke1Positions.push_back(core::vector3df(1.0f, 5.0f, -3.0f));	// Radius : 0.5f
		outSmoke1Positions.push_back(core::vector3df(3.0f, 5.0f, -3.0f));	// Radius : 0.5f
		break;
	case BI_usine_ciment_grande:
		{
			core::vector3df pos;
			for (int i = 0; i < 4; ++i)
			{
				// Valeurs obtenues d'après EcoWorldRenderer::loadBatiment : case BI_usine_ciment_grande :
				const core::vector3df addedPos(((i % 2) * 10.0f) - 5.0f, 0.0f, (i > 1 ? 10.0f : 0.0f) - 5.0f);

				const float rotY = i * 90.0f;

				pos = core::vector3df(3.0f, 5.0f, -3.0f);			// Radius : 0.5f
				pos.rotateXZBy(rotY);
				pos += addedPos;
				outSmoke1Positions.push_back(pos);

				pos = core::vector3df(3.0f, 5.0f, -1.0f);			// Radius : 0.5f
				pos.rotateXZBy(rotY);
				pos += addedPos;
				outSmoke1Positions.push_back(pos);

				pos = core::vector3df(3.0f, 5.0f, 1.0f);			// Radius : 0.5f
				pos.rotateXZBy(rotY);
				pos += addedPos;
				outSmoke1Positions.push_back(pos);

				pos = core::vector3df(3.0f, 5.0f, 3.0f);			// Radius : 0.5f
				pos.rotateXZBy(rotY);
				pos += addedPos;
				outSmoke1Positions.push_back(pos);
			}
		}
		break;
	case BI_usine_tuiles_petite:
		outSmoke1Positions.push_back(core::vector3df(0.5f, 5.5f, 3.0f));	// Radius : 0.4f
		outSmoke1Positions.push_back(core::vector3df(1.75f, 5.5f, 3.0f));	// Radius : 0.4f
		outSmoke1Positions.push_back(core::vector3df(3.0f, 5.5f, 3.0f));	// Radius : 0.4f
		break;
	case BI_usine_tuiles_grande:
		{
			core::vector3df pos;
			for (int i = 0; i < 4; ++i)
			{
				// Valeurs obtenues d'après EcoWorldRenderer::loadBatiment : case BI_usine_tuiles_grande :
				const core::vector3df addedPos(((i % 2) * 10.0f) - 5.0f, 0.0f, (i > 1 ? 10.0f : 0.0f) - 5.0f);

				// Le (i + 1) semble nécessaire pour concorder avec la rotation du bâtiment
				// TODO : Normalement, cet ajustement ne devrait pas être nécessaire !
				const float rotY = (i + 1) * 90.0f;

				pos = core::vector3df(0.5f, 5.5f, 3.0f);			// Radius : 0.4f
				pos.rotateXZBy(rotY);
				pos += addedPos;
				outSmoke1Positions.push_back(pos);

				pos = core::vector3df(1.75f, 5.5f, 3.0f);			// Radius : 0.4f
				pos.rotateXZBy(rotY);
				pos += addedPos;
				outSmoke1Positions.push_back(pos);

				pos = core::vector3df(3.0f, 5.5f, 3.0f);			// Radius : 0.4f
				pos.rotateXZBy(rotY);
				pos += addedPos;
				outSmoke1Positions.push_back(pos);
			}
		}
		break;
#ifdef KID_VERSION	// En mode enfant : on réactiver le papier comme ressource consommée par les habitants pour leur satisfaction
	case BI_usine_papier_petite:
		outSmoke1Positions.push_back(core::vector3df(-3.125f, 4.0f, 2.5f));	// Radius : 0.3f
		outSmoke1Positions.push_back(core::vector3df(-1.875f, 4.0f, 2.5f));	// Radius : 0.3f
		outSmoke1Positions.push_back(core::vector3df(3.0f, 4.0f, 0.5f));	// Radius : 0.5f
		break;
	case BI_usine_papier_grande:
#else				// On désactive l'usine à tout faire en mode enfant qui est devenue inutile
	case BI_usine_tout:
#endif
		{
			core::vector3df pos;
			for (int i = 0; i < 4; ++i)
			{
				// Valeurs obtenues d'après EcoWorldRenderer::loadBatiment : case BI_usine_papier_grande :
				const core::vector3df addedPos(((i % 2) * 10.0f) - 5.0f, 0.0f, (i > 1 ? 10.0f : 0.0f) - 5.0f);

				const float rotY = i * 90.0f;

				pos = core::vector3df(-3.125f, 4.0f, 2.5f);			// Radius : 0.3f
				pos.rotateXZBy(rotY);
				pos += addedPos;
				outSmoke1Positions.push_back(pos);

				pos = core::vector3df(-1.875f, 4.0f, 2.5f);			// Radius : 0.3f
				pos.rotateXZBy(rotY);
				pos += addedPos;
				outSmoke1Positions.push_back(pos);

				pos = core::vector3df(3.0f, 4.0f, 0.5f);			// Radius : 0.3f
				pos.rotateXZBy(rotY);
				pos += addedPos;
				outSmoke1Positions.push_back(pos);
			}
		}
		break;

		// Gestion de l'effet de serre et des déchets
	case BI_decharge:
		outSmoke1Positions.push_back(core::vector3df(2.5f, 4.0f, 3.125f));	// Radius : 0.3f
		outSmoke1Positions.push_back(core::vector3df(2.5f, 4.0f, 1.875f));	// Radius : 0.3f
		outSmoke1Positions.push_back(core::vector3df(0.5f, 4.0f, -3.0f));	// Radius : 0.3f
		break;
	case BI_usine_incineration_dechets:
		outSmoke2Positions.push_back(core::vector3df(-2.0f, 3.0f, 2.0f));	// Radius : 1.0f	// (-2.0f, 2.5f, 2.0f)
		outSmoke2Positions.push_back(core::vector3df(2.0f, 3.0f, -2.0f));	// Radius : 1.0f	// (2.0f, 2.5f, -2.0f)
		break;

		// Arbres
	case BI_arbre_aspen:
		break;
	case BI_arbre_oak:
		break;
	case BI_arbre_pine:
		break;
	case BI_arbre_willow:
		break;

	default:
		LOG_DEBUG("EcoWorldRenderer::loadBatimentSmokePositions(..., " << batimentID << ", ...) : Type de batiment inconnu : " << batimentID, ELL_WARNING);
		break;
	}

	// Ajuste la rotation des positions pour être conforme au bâtiment
	// TODO : Normalement, ces ajustements ne devraient pas être nécessaires !
	batRotationY += 180.0f;
	batRotationY *= -1.0f;

	// Transforme les positions avec les paramêtres d'agrandissement et de rotation spécifiés
	const core::list<core::vector3df>::Iterator END = outSmoke1Positions.end();
	core::list<core::vector3df>::Iterator it = outSmoke1Positions.begin();
	for (; it != END; ++it)
	{
		core::vector3df& pos = (*it);
		pos *= batScale;
		pos.rotateXZBy(batRotationY);
	}
	for (it = outSmoke2Positions.begin(); it != END; ++it)
	{
		core::vector3df& pos = (*it);
		pos *= batScale;
		pos.rotateXZBy(batRotationY);
	}
}
#endif
CBatimentSceneNode* EcoWorldRenderer::addBatiment(Batiment* batiment, const core::vector3df* position, const float* deniveleTerrain)
{
	if (!batiment)
	{
		LOG_DEBUG("EcoWorldRenderer::addBatiment(" << batiment << ") : Le batiment specifie n'est pas valide : batiment = " << batiment, ELL_WARNING);
		return NULL;
	}

	// Calcule la position de ce bâtiment
	core::vector3df batPos;
	float denivele;
	if (position && deniveleTerrain)
	{
		batPos.set(*position);
		denivele = (*deniveleTerrain);
	}
	else
	{
		// Obtient la position en X et en Z du bâtiment grâce à son index
		batPos = EcoWorldSystem::getPositionFromIndex(batiment->getIndex());

		// Obtient la position en Y du bâtiment d'après le terrain actuel
		denivele = getNewBatimentPos(batPos, batiment->getID(), batiment->getRotation());
	}

	// Crée un nouveau bâtiment en utilisant la classe CBatimentSceneNode
	CBatimentSceneNode* node = new CBatimentSceneNode(batiment, batPos, denivele, game->sceneManager->getRootSceneNode());

	// Ajoute l'effet de flou de profondeur à ce node grâce à PostProcess (ses enfants seront aussi affectés)
	if (gameConfig.usePostProcessEffects && gameConfig.postProcessUseDepthRendering && game->postProcessManager)
		game->postProcessManager->addNodeToDepthPass(node);

	// Ajoute une ombre à ce bâtiment grâce à XEffects, et lui permet aussi de les recevoir (ses enfants seront aussi affectés)
	if (gameConfig.useXEffectsShadows && game->xEffects)
		game->xEffects->addShadowToNode(node, ESM_BOTH);

	if (node->getTriangleSelector())
	{
#ifdef USE_COLLISIONS
		// Ajoute son triangle selector à celui de la caméra si ce n'est pas une route
		if (cameraTriangleSelector
#ifndef KID_VERSION	// En mode enfant : on désactive les routes qui sont inutiles
			&& batiment->getID() != BI_route
#endif
			)
			cameraTriangleSelector->addTriangleSelector(node->getTriangleSelector());
#endif

		// Ajoute son triangle selector à celui des batiments
		if (batimentsTriangleSelector)
			batimentsTriangleSelector->addTriangleSelector(node->getTriangleSelector());
	}

	// Libère le node créé
	node->drop();

	return node;
}
void EcoWorldRenderer::deleteBatiment(CBatimentSceneNode* node, bool wantedByUser)
{
	if (!node)
	{
		LOG_DEBUG("EcoWorldRenderer::deleteBatiment(" << node << ") : Le node specifie n'est pas valide : node = " << node, ELL_WARNING);
		return;
	}

	// Supprime le triangle selector du batiment des listes des triangles selector
	if (node->getTriangleSelector())
	{
#ifdef USE_COLLISIONS
		if (cameraTriangleSelector)
			cameraTriangleSelector->removeTriangleSelector(node->getTriangleSelector());
#endif

		if (batimentsTriangleSelector)
			batimentsTriangleSelector->removeTriangleSelector(node->getTriangleSelector());
	}

	// Vérifie qu'aucun pointeur de Game ne pointe actuellement sur ce node, sinon ces pointeurs deviendront invalides
	checkGameNodePointersForNode(node);

	// Si PostProcess est activé, on lui indique qu'il ne doit plus prendre en compte ce bâtiment dans le flou de profondeur
	if (game->postProcessManager)
		game->postProcessManager->removeNodeFromDepthPass(node);

	// Si XEffects est activé, on lui indique qu'il ne doit plus afficher l'ombre de ce bâtiment
	if (game->xEffects)
		game->xEffects->removeShadowFromNode(node);

#ifdef USE_IRRKLANG
#if 0	// Mode 3D désactivé car le son n'était pas assez fort
	// Joue le son d'un bâtiment détruit en mode 3D à la position du bâtiment
	ikMgr.playGameSound3D(wantedByUser ? IrrKlangManager::EGSE_BATIMENT_DESTROYED : IrrKlangManager::EGSE_BATIMENT_DESTROYED_UNWANTED,
		node->getAbsolutePosition());
#else
	// Joue le son d'un bâtiment détruit en mode 2D
	ikMgr.playGameSound(wantedByUser ? IrrKlangManager::EGSE_BATIMENT_DESTROYED : IrrKlangManager::EGSE_BATIMENT_DESTROYED_UNWANTED);
#endif
#endif

	// Supprime le batiment
	node->setVisible(false);
	game->sceneManager->addToDeletionQueue(node);
}
void EcoWorldRenderer::update(bool initSunPosition)
{
	// Met à jour le scene manager, IrrKlang et SPARK suivant le temps actuel dans le Weather Manager :

	const WeatherInfos& weatherInfos = game->system.getWeatherManager().getCurrentWeatherInfos();

	// Brouillard
	game->driver->setFog(weatherInfos.fogColor, video::EFT_FOG_LINEAR, weatherInfos.fogStart, weatherInfos.fogEnd, 0.0f, true, true);

	// SkyDome
	if (skydome)
	{
#ifdef VITESSE_TOURNER_CIEL
		// Calcule la rotation de départ du ciel d'après le temps actuel du système de jeu (la position du soleil est réglée d'après la rotation du ciel)
		if (initSunPosition)
			skydome->setRotation(core::vector3df(0.0f,
				game->system.getTime().getTotalTime() * VITESSE_TOURNER_CIEL * 100.0f,	// * 100.0f : (* 100.0f = * 1000.0f / 10.0f) car VITESSE_TOURNER_CIEL est en degrés pour 10 ms, getTotalTime est en secondes et skydomeRotationY est en degrés
				0.0f));
#endif

		// Vérifie si le shader du skydome est activé
		if (gameConfig.skyDomeShadersEnabled && skyDomeMaterial != -1)
		{
			// Shader de skydome activé :

			// On change la texture du dernier temps du ciel, s'il a changé
			const WeatherID realLastWeatherID = game->system.getWeatherManager().getLastWeatherID();
			if (currentWeatherID != realLastWeatherID)
			{
				const WeatherInfos& lastWeatherInfos = game->system.getWeatherManager().getLastWeatherInfos();

				io::path skydomeTexturePath = lastWeatherInfos.skydomeTexturePath;
				skydomeTexturePath.append(getSkyTextureSuffixe(gameConfig.texturesQuality));
				skydome->setLastWeather(game->driver->getTexture(skydomeTexturePath), lastWeatherInfos.skydomeTexturePercentage, lastWeatherInfos.skydomeTextureOffset);

				currentWeatherID = realLastWeatherID;
			}

			// On change la texture du nouveau temps du ciel, s'il a changé
			const WeatherID realNewWeatherID = game->system.getWeatherManager().getNewWeatherID();
			if (newWeatherID != realNewWeatherID)
			{
				const WeatherInfos& newWeatherInfos = game->system.getWeatherManager().getNewWeatherInfos();

				io::path skydomeTexturePath = newWeatherInfos.skydomeTexturePath;
				skydomeTexturePath.append(getSkyTextureSuffixe(gameConfig.texturesQuality));
				skydome->setNewWeather(game->driver->getTexture(skydomeTexturePath), newWeatherInfos.skydomeTexturePercentage, newWeatherInfos.skydomeTextureOffset);

				newWeatherID = realNewWeatherID;
			}
		}
		else
		{
			// Shader de skydome désactivé :

			// On change la texture du ciel lorsque le temps du jeu a changé
			const WeatherID realCurrentWeatherID = game->system.getWeatherManager().getCurrentWeatherID();
			if (currentWeatherID != realCurrentWeatherID)
			{
				io::path skydomeTexturePath = weatherInfos.skydomeTexturePath;
				skydomeTexturePath.append(getSkyTextureSuffixe(gameConfig.texturesQuality));
				skydome->setLastWeather(game->driver->getTexture(skydomeTexturePath), weatherInfos.skydomeTexturePercentage, weatherInfos.skydomeTextureOffset);

				currentWeatherID = realCurrentWeatherID;
			}
		}
	}

	// Soleil (ou Lune)
	const float sunRotation = (skydome ? ((skydome->getRotation().Y - 90.0f) * -core::DEGTORAD) : 0.0f);

	// Calcule la position du soleil par rapport avec l'origine avec cette rotation
	// TODO : Trouver une solution plus rapide que d'utiliser cosf et sinf (fonctions trigonométriques lentes) ?
#define SUN_RADIUS	1000.0f		// Le rayon du soleil (sa distance constante par rapport à la caméra, sans prendre en compte la coordonnée Y)
#define SUN_HEIGHT	400.0f		// La hauteur du soleil en Y par rapport à la caméra
	const core::vector3df sunPosition(
		cosf(sunRotation) * SUN_RADIUS,
		SUN_HEIGHT,
		sinf(sunRotation) * SUN_RADIUS);
	if (sun)
	{
		// Indique sa nouvelle couleur
		sun->setColor(weatherInfos.sunColor);

		// Indique la position du soleil, d'après sa hauteur et son rayon
		sun->setPosition(sunPosition);
	}

	// Ancienne méthode de calcul de la position du soleil :
	/*{
		// Le soleil se déplace ici sur un cercle qui a pour centre la position actuelle de la caméra et pour rayon une valeur constante : radius.
		// Pour modifier le centre du cercle de la trajectoire du soleil (dont sa hauteur), il faut ajouter un nombre de type float à cameraPosition.? dans le calcul du nouveau vecteur de position du soleil.
		// Pour modifier le rayon du cercle de la trajectoire du soleil, il faut modifier la valeur de "radius".
		// Pour modifier le point de départ du soleil sur sa trajectoire, il faut ajouter un nombre de type float à "rotation".
		const core::vector3df cameraPosition = game->sceneManager->getActiveCamera() ? game->sceneManager->getActiveCamera()->getAbsolutePosition() : core::vector3df(0.0f, 0.0f, 0.0f);
		const float radius = 1000.0f;
		const float rotation = skydome ? ((skydome->getRotation().Y - 90.0f - weatherInfos.skydomeRotationY) * -DEGTORAD) : 0.0f;

		// Tourne le soleil suivant la rotation actuelle du ciel en prenant sa position actuelle comme centre
		// (Pris de la source d'Irrlicht 1.7.2 : CSceneNodeAnimatorFlyCircle.cpp)
		sun->setPosition(core::vector3df(
			cameraPosition.X + radius * cosf(rotation),
			cameraPosition.Y + 400.0f,
			cameraPosition.Z + radius * sinf(rotation)));
	}*/

	// Lumières
	if (gameConfig.useXEffectsShadows && game->xEffects)
	{
		// Lumières de game->xEffects utilisées :

		// Si initSunPosition est à true, on doit redéfinier la position réelle de la lumière :
		// On demande à la caméra RTS de recalculer sa position (même si cela est automatiquement effectué à chaque rendu normalement)
		if (game->cameraRTS)
			game->cameraRTS->updateXEffects();

		// Indique la lumière ambiante à game->xEffects
		game->xEffects->setAmbientColor(weatherInfos.ambientLight.toSColor());

		// Met à jour la couleur de la lumière du soleil (sa position et sa direction seront mis à jour dans RTSCamera::updateXEffects(), lors de l'enregistrement de la caméra)
		if (game->xEffects->getShadowLightCount())
			game->xEffects->getShadowLight(0).setLightColor(weatherInfos.sunLightColor);
	}
	else
	{
		// Lumières d'Irrlicht utilisées :

#ifdef VITESSE_TOURNER_CIEL
		// Calcule la rotation de départ de la lumière du soleil et de celle des ombres d'après le temps actuel du système de jeu
		if (initSunPosition)
		{
			const float systemTotalTime = game->system.getTime().getTotalTime();
			if (sunLight)
			{
				// La lumière directionnelle du soleil est animée par un animator modifiant sa rotation :
				// Cet animator supporte la modification directe de la rotation du node
				core::vector3df sunLightRotation = sunLight->getRotation();
				sunLightRotation.Y = 180.0f + systemTotalTime * VITESSE_TOURNER_CIEL * 100.0f;	// *100.0f car VITESSE_TOURNER_CIEL est en degrés pour 10 ms (°/10ms), et game->system.getTime().getTotalTime() est en secondes
				sunLight->setRotation(sunLightRotation);
			}
			if (shadowLight && gameConfig.stencilShadowsEnabled)
			{
				// La lumière ponctuelle du soleil pour les ombres est animée par un animator modifiant sa position suivant un cercle :
				// Cet animator ne supporte pas la modification directe de la position du node,
				// on doit donc le supprimer (normalement cette lumière ne contient que cet animator) puis le recréer avec les bons paramêtres
				shadowLight->removeAnimators();

				// Crée le nouvel animator avec les bons paramêtres de départ pour avoir une animation cohérente avec la position du soleil
				// Calcul de la position de départ : C'est le nombre de tours parcourus pendant le temps du jeu déjà écoulé :
				// - Pour systemTotalTime = 0.0f, startPosition = 0.75f
				// - Nombre de tours par seconde : N = (VITESSE_TOURNER_CIEL * 100.0f) / 360.0f = VITESSE_TOURNER_CIEL / 3.6f
				//		donc : startPosition = N * systemTotalTime + 0.75f = systemTotalTime * VITESSE_TOURNER_CIEL / 3.6f + 0.75f
				const float LIGHT_RADIUS = TAILLE_CARTE * TAILLE_OBJETS;
				const float startPosition = systemTotalTime * VITESSE_TOURNER_CIEL / 3.6f + 0.75f;
				scene::ISceneNodeAnimator* const shadowLightAnim = game->sceneManager->createFlyCircleAnimator(
					core::vector3df(0.0f, LIGHT_RADIUS, 0.0f), LIGHT_RADIUS, VITESSE_TOURNER_CIEL * 0.1f * core::DEGTORAD,
					core::vector3df(0.0f, 1.0f, 0.0f), startPosition);
				shadowLight->addAnimator(shadowLightAnim);
				shadowLightAnim->drop();
			}
		}
#endif

		// Met à jour la couleur de la lumière ambiante
		game->sceneManager->setAmbientLight(weatherInfos.ambientLight);

		// Met à jour la couleur de la lumière diffuse
		if (sunLight)
			sunLight->getLightData().DiffuseColor = weatherInfos.sunLightColor;

		// Ajuste la couleur des ombres d'Irrlicht
		if (gameConfig.stencilShadowsEnabled)
			game->sceneManager->setShadowColor(weatherInfos.shadowColor);
	}

#ifdef USE_IRRKLANG
	// Joue le son de la pluie si nécessaire (déterminé par le weather manager)
	const bool needRainSound = game->system.getWeatherManager().isRainSoundNeeded();
	if (needRainSound != isRainSoundPlaying)
	{
		ikMgr.playRainSound(needRainSound);
		isRainSoundPlaying = needRainSound;
	}
#endif
#ifdef USE_SPARK
	// Indique la densité de la pluie
	sparkMgr.setRainFlow(weatherInfos.rainFlow);
#endif
}
void NormalMappingShaderCallBack::OnSetConstants(video::IMaterialRendererServices* services, int userData)
{
	// lightDirection et diffuseColor (déterminés d'après la première lumière dynamique disponible)
	if (game->driver->getDynamicLightCount() > 0)
	{
		const video::SLight& light = game->driver->getDynamicLight(0);
		services->setVertexShaderConstant("lightDirection", reinterpret_cast<const float*>(&light.Direction), 3);

		if (material)
		{
			// Inclus la couleur diffuse de la lumière et la couleur diffuse du matériau dans la couleur diffuse à envoyer au pixel shader
			video::SColorf diffuseColor(material->DiffuseColor);
			diffuseColor.r *= light.DiffuseColor.r;
			diffuseColor.g *= light.DiffuseColor.g;
			diffuseColor.b *= light.DiffuseColor.b;
			services->setPixelShaderConstant("diffuseColor", reinterpret_cast<const float*>(&diffuseColor), 3);
		}
		else
			services->setPixelShaderConstant("diffuseColor", reinterpret_cast<const float*>(&light.DiffuseColor), 3);
	}
	else	// Aucune lumière dynamique n'est disponible : on envoie des valeurs par défaut aux shaders
	{
		const core::vector3df lightDirection(1.0f, 0.0f, 0.0f);
		services->setVertexShaderConstant("lightDirection", reinterpret_cast<const float*>(&lightDirection), 3);

		const video::SColorf diffuseColor(0.0f, 0.0f, 0.0f, 1.0f);
		services->setPixelShaderConstant("diffuseColor", reinterpret_cast<const float*>(&diffuseColor), 3);
	}

	// ambientColor
	video::SColorf ambientColor = smgr->getAmbientLight();
	if (material)
	{
		const float inv = 1.0f / 255.0f;
		ambientColor.r *= material->AmbientColor.getRed() * inv;
		ambientColor.g *= material->AmbientColor.getGreen() * inv;
		ambientColor.b *= material->AmbientColor.getBlue() * inv;
	}
	services->setPixelShaderConstant("ambientColor", reinterpret_cast<const float*>(&ambientColor), 3);

	// worldTransposed
	const core::matrix4& world = game->driver->getTransform(video::ETS_WORLD);
	{
		// Conversion et transposition d'une matrice 4x4 en matrice 3x3
		const float* M = world.pointer();
		float worldTransposed[9] =
			{	M[0], M[4], M[8],
				M[1], M[5], M[9],
				M[2], M[6], M[10] };
		services->setVertexShaderConstant("worldTransposed", worldTransposed, 9);
	}

	if (game->driver->getDriverType() != video::EDT_OPENGL)
	{
		// Direct3D 9

		// worldViewProj
		core::matrix4 worldViewProj(game->driver->getTransform(video::ETS_PROJECTION));
		worldViewProj *= game->driver->getTransform(video::ETS_VIEW);
		worldViewProj *= world;
		services->setVertexShaderConstant("worldViewProj", worldViewProj.pointer(), 16);
	}
	else
	{
		// OpenGL

		// fogEnabled
		float fogEnabled = 0.0f;
		if (material && material->FogEnable)
			fogEnabled = 1.0f;
		services->setVertexShaderConstant("fogEnabled", &fogEnabled, 1);

		// baseMap et bumpMap
		const int map[] = { 0, 1 };
		services->setPixelShaderConstant("baseMap", (float*)&map[0], 1);
		services->setPixelShaderConstant("bumpMap", (float*)&map[1], 1);
	}
}
void SkyDomeShaderCallBack::OnSetConstants(video::IMaterialRendererServices* services, int userData)
{
	// tex1Percentage
	const float tex1Percentage = game->system.getWeatherManager().getCurrentInterpolation();
	services->setPixelShaderConstant("tex1Percentage", &tex1Percentage, 1);

	if (game->driver->getDriverType() != video::EDT_OPENGL)
	{
		// Direct3D 9

		// worldViewProj
		core::matrix4 worldViewProj(game->driver->getTransform(video::ETS_PROJECTION));
		worldViewProj *= game->driver->getTransform(video::ETS_VIEW);
		worldViewProj *= game->driver->getTransform(video::ETS_WORLD);
		services->setVertexShaderConstant("worldViewProj", worldViewProj.pointer(), 16);
	}
	else
	{
		// OpenGL

		// tex0 et tex1
		const int map[] = { 0, 1 };
		services->setPixelShaderConstant("tex0", (float*)&map[0], 1);
		services->setPixelShaderConstant("tex1", (float*)&map[1], 1);
	}
}
/*void EcoWorldRenderer::save(io::IAttributes* out, io::IXMLWriter* writer) const
{
	if (!out || !writer)
		return;

	// TODO

	// Ecrit les informations dans le fichier
	out->write(writer, false, L"Renderer");
	out->clear();
}
void EcoWorldRenderer::load(io::IAttributes* in, io::IXMLReader* reader)
{
	if (!in || !reader)
		return;

	// Lit les informations à partir du fichier
	reader->resetPosition();
	if (in->read(reader, false, L"Renderer"))
	{
		// TODO

		in->clear();
	}
}*/
