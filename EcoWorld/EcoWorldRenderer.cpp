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

// La vitesse � laquelle le ciel tourne (en degr�s pour 10 ms : �/10ms) (commenter pour d�sactiver la rotation du ciel) :
#define VITESSE_TOURNER_CIEL	DAY_TIME_INV * 0.01f	// Un tour par par an : 360�/an <=> 360�/360 jours <=> 1�/jour <=> 1�/DAY_TIME s <=> DAY_TIME_INV�/s <=> (DAY_TIME_INV / 100) �/10ms

//#define USE_VARIOUS_TREES		// Si d�fini, permet d'utiliser des arbres diff�rents dans des for�ts (chaque arbre est unique, car g�n�r� al�atoirement) (ne comprend pas les arbres de pr�visualisation, qui sont identiques pour permettre une pr�visualisation r�pide)
								// Sinon, tous les arbres des for�ts seront identiques (utilisation des arbres de pr�visualisation) (optimisation du temps de rendu et du temps de cr�ation des arbres, utilisation m�moire inf�rieure)

#define USE_MATERIALS_MESHES	// Si d�fini, les b�timents non textur�s mais disponibles en version color�e (avec des mat�riaux) seront affich�s dans leur version avec mat�riaux
								// Sinon, ils seront affich�s sans mat�riaux sp�cifiques (soit compl�tement gris)

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

	// Initialise le tableau des arbres de pr�visualisation
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

	// Supprime les arbres de pr�visualisation
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
	// On suppose qu'ici, tous les scene nodes li�s au renderer (batiments...) ont d�j� �t� supprim�s du scene manager

#ifdef USE_COLLISIONS
	if (cameraTriangleSelector)
		cameraTriangleSelector->removeAllTriangleSelectors();
#endif
	if (batimentsTriangleSelector)
		batimentsTriangleSelector->removeAllTriangleSelectors();

	/*
	// Supprime les arbres de pr�visualisation
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

	// Indique au terrain et � SPARK de se r�initialiser
	terrainMgr.reset();
#ifdef USE_SPARK
	sparkMgr.reset();
#endif

	// Remet rous les pointeurs � 0
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

	// Active un brouillard par d�faut
	game->driver->setFog(video::SColor(255, 150, 150, 150), video::EFT_FOG_LINEAR, 1000.0f, 2500.0f, 0.0f, true, true);

	// Initialise les shaders du jeu s'ils sont activ�s
	if (gameConfig.shadersEnabled)
		initShaders(mainMenuLoading);



	// V�rifie si on doit utiliser les lumi�res pour XEffects
	const float LIGHT_RADIUS = TAILLE_CARTE * TAILLE_OBJETS;
	if (gameConfig.useXEffectsShadows && game->xEffects)
	{
		// Lumi�res de XEffects activ�es :
		game->sceneManager->setAmbientLight(video::SColorf(0.0f, 0.0f, 0.0f));

		// Lumi�re du soleil sous XEffects : lumi�re directionnelle
		if (!game->xEffects->getShadowLightCount())
		{
			game->xEffects->addShadowLight(SShadowLight(gameConfig.xEffectsShadowMapResolution, core::vector3df(0.0f, LIGHT_RADIUS, LIGHT_RADIUS), core::vector3df(0.0f),
				video::SColorf(0.0f, 0.0f, 0.0f), CAMERA_NEAR_VALUE, CAMERA_FAR_VALUE, 2000.0f, true));
		}
	}
	else
	{
		// Lumi�res de XEffects d�sactiv�es : utilisation des lumi�res d'Irrlicht :
		game->sceneManager->setAmbientLight(video::SColorf(0.5f, 0.5f, 0.5f));

		if (!sunLight)
		{
			// Lumi�re directionnelle du soleil ou de la lune
			sunLight = game->sceneManager->addLightSceneNode(0,
				core::vector3df(0.0f, 10.0f, 0.0f), video::SColorf(0.0f, 0.0f, 0.0f));
			sunLight->setLightType(video::ELT_DIRECTIONAL);
			sunLight->getLightData().AmbientColor.set(1.0f, 0.0f, 0.0f, 0.0f);
			sunLight->setRotation(core::vector3df(25.0f, 180.0f, 0.0f));
			sunLight->enableCastShadow(false);

#ifdef VITESSE_TOURNER_CIEL
			// Fait tourner la lumi�re en m�me temps que le ciel
			scene::ISceneNodeAnimator* const sunLightAnim = game->sceneManager->createRotationAnimator(
				core::vector3df(0.0f, VITESSE_TOURNER_CIEL, 0.0f));
			sunLight->addAnimator(sunLightAnim);
			sunLightAnim->drop();
#endif
		}

		if (!shadowLight && gameConfig.stencilShadowsEnabled)
		{
			// Lumi�re pour les ombres
			shadowLight = game->sceneManager->addLightSceneNode(0,
				core::vector3df(0.0f, LIGHT_RADIUS, LIGHT_RADIUS), video::SColorf(0.0f, 0.0f, 0.0f), 1.0f);
			shadowLight->getLightData().Attenuation.set(0.0f, 0.0f, 0.0f);
			shadowLight->getLightData().AmbientColor.set(1.0f, 0.0f, 0.0f, 0.0f);
			shadowLight->enableCastShadow(true);

#ifdef VITESSE_TOURNER_CIEL
			// Fait tourner la lumi�re en m�me temps que le ciel
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



	// Pr�charge toutes les textures de ciels (le menu principal peut aussi changer de temps)
	const bool lastTexCreationFlagMipMap = game->driver->getTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS);	// D�sactive les niveaux de mips maps pour les textures du ciel
	game->driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, false);
	for (int i = 0; i < WI_COUNT; ++i)
	{
		WeatherInfos weatherInfos((WeatherID)i);
		io::path skydomePath = weatherInfos.skydomeTexturePath;
		skydomePath.append(getSkyTextureSuffixe(gameConfig.texturesQuality));
		game->driver->getTexture(skydomePath);
	}
	game->driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, lastTexCreationFlagMipMap);

	// Cr�e le ciel
	currentWeatherID = WI_COUNT;	// Assigne une valeur impossible � currentWeatherID pour forcer la re-cr�ation du temps du ciel
	newWeatherID = WI_COUNT;		// Assigne une valeur impossible � newWeatherID pour forcer la re-cr�ation du temps du ciel
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



	// Met � jour le temps du monde d'apr�s le Weather Manager, en initialisant le ciel et la position du soleil d'apr�s le temps actuel du jeu
	update(true);

	// On ne doit que charger ce qui est n�c�ssaire au menu principal, on a donc termin�
	if (mainMenuLoading)
		return;



	// Pr�-charge tous les batiments (les cr�e puis les supprime imm�diatement)
	scene::ISceneNode* batiment = NULL;
	const float BI_COUNT_F_INV = 1.0f / ((float)BI_COUNT + 1.0f);	// N�cessaire pour la barre de chargement
	for (int i = 0; i <= BI_COUNT; ++i)
	{
		if (loadingScreen)
			if (loadingScreen->setPercentAndDraw((loadMax - loadMin) * (float)i * BI_COUNT_F_INV + loadMin))	return;

		// Si i ne repr�sente actuellement aucun b�timent, alors on charge les donn�es pour la construction des b�timents et pour leur dalle en b�ton
		// (l'ID du b�timent n'a ici aucune importance, il doit juste �tre diff�rent de BI_aucun)
		if (i == BI_aucun)
			batiment = loadConstructingBatiment(BI_maison_individuelle, NULL, true);
		else if (i == BI_COUNT)
			batiment = loadConcreteBatiment(0.0f, BI_maison_individuelle, NULL, true);
		else
			batiment = loadBatiment((BatimentID)i, NULL, true, true);

		if (batiment)
		{
			// Evite de laisser le batiment visible � la premi�re frame du jeu
			batiment->setVisible(false);

			// Supprime le batiment � la prochaine frame
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
		// Cr�e le callback s'il n'est pas d�j� cr��
		if (!skyDomeCallBack)
			skyDomeCallBack = new SkyDomeShaderCallBack();

		// Cr�e le mat�riau d'Irrlicht avec les shaders, s'il n'est pas d�j� cr��
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

	// On ne doit charger que les shaders n�cessaires au menu principal : on a donc termin�
	if (mainMenuLoading)
		return;

	// NormalMapping
	if (gameConfig.normalMappingEnabled)
	{
		// Cr�e le callback s'il n'est pas d�j� cr��
		if (!normalMappingCallBack)
			normalMappingCallBack = new NormalMappingShaderCallBack(game->sceneManager);

		// Cr�e le mat�riau d'Irrlicht avec les shaders, s'il n'est pas d�j� cr��
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
	// Indique la hauteur minimale du terrain � la pluie de SPARK
	float minHeight = terrainMgr.getTerrainBoundingBox().MinEdge.Y;
	if (terrainMgr.getTerrainInfos().water.visible)
		minHeight = max(minHeight, terrainMgr.getTerrainInfos().water.height);	// Prend la plus grande valeur entre la hauteur de l'eau et la hauteur minimale du terrain
	sparkMgr.setMinTerrainHeight(minHeight);

	// Indique la direction du vent � la pluie de SPARK
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
	bool reinitMaterialsDiffuse = true;	// True si � la fin de cette fonction la couleur des mat�riaux de ce node devront �tre r�initialis�s, false sinon : passer cette valeur � false lorsque le node charg� a aussi charg� ses mat�riaux
#endif

	// Agrandis un peu les batiments export�s avec Blender, car les unit�s d'Irrlicht sont plus grandes que celles de Blender
	// 1.0f sous Blender <-> 1 sous EcoWorldSystem <-> TAILLE_OBJETS / 2 sous Irrlicht
	const core::vector3df blenderObjectsScaling(TAILLE_OBJETS * 0.5f);

	switch (batimentID)
	{
#ifndef KID_VERSION	// En mode enfant : on d�sactive la maison de base et les routes qui sont inutiles
		// Tests
	case BI_maison:
		{
			// Charge seulement les mesh et les textures n�cessaires � cette maison
			if (forLoadingOnly)
			{
				game->sceneManager->getMesh("Maison_BQ.obj");
				break;
			}

			node = game->sceneManager->addAnimatedMeshSceneNode(game->sceneManager->getMesh("Maison_BQ.obj"), parent, -1,
				core::vector3df(), core::vector3df(), blenderObjectsScaling);
			if (!node)
				break;

			// Cr�e une ombre pour ce b�timent
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode();

			if (!isTmpBatiment)
			{
				// Cr�e le triangle selector de la maison :

#if 1	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis)
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
			// Charge seulement les mesh et les textures n�cessaires � cette route
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

#if 0	// Utilise un plan pour cr�er la route
			scene::IAnimatedMesh* routeMesh = game->sceneManager->addHillPlaneMesh("route", core::dimension2df(TAILLE_OBJETS, TAILLE_OBJETS),
				core::dimension2du(1, 1), 0, 0.0f, core::dimension2df(0.0f, 0.0f),
				core::dimension2df(1.0f, 1.0f));
#else	// Permet d'utiliser des cubes pour la route au lieu de plans : �vite les probl�mes d'affichage de la route, d�s � la faible �paisseur des plans, mais le rendu est plus lent
			// La taille en Y des cubes de la route peut �tre r�gl�e suivant les besoins :
			// Plus ce nombre est grand, plus on pourra voir la route de haut sans probl�me d'affichage ou de clignotement, mais plus la route sera �paisse :
			// si l'�paisseur de la route est trop grande, on risque de remarquer que ce sont des cubes.
			scene::IMesh* routeMesh = game->sceneManager->getGeometryCreator()->createCubeMesh(
				core::vector3df(TAILLE_OBJETS, TAILLE_OBJETS * 0.2f, TAILLE_OBJETS));	// Taille en Y r�glable
#endif
			if (!routeMesh)
				break;

			// Cr�e le node de ce b�timent :
			if (!gameConfig.normalMappingEnabled || normalMappingMaterial == -1)
			{
				node = game->sceneManager->addMeshSceneNode(routeMesh, parent);
				routeMesh->drop();
				if (!node)
					break;
			}
			else
			{
				// Charge cette mesh en demandant � Irrlicht de cr�er ses tangentes pour permettre l'utilisation du normal mapping
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
				// Cr�e le triangle selector de la route :

#if 0	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis)
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
			// Charge seulement les mesh et les textures n�cessaires � cette maison
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

			// Cr�e le node de ce b�timent :
			if (!gameConfig.normalMappingEnabled || normalMappingMaterial == -1)
			{
				node = game->sceneManager->addAnimatedMeshSceneNode(
					game->sceneManager->getMesh("Maison Individuelle.obj"), parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling * 2.0f / 3.0f);
				if (!node)
					break;
			}
			else
			{
				// Charge cette mesh en demandant � Irrlicht de cr�er ses tangentes pour permettre l'utilisation du normal mapping
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

			// Cr�e une ombre pour ce b�timent
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					game->sceneManager->getMesh("Maison Individuelle Ombre.obj"));

			if (!isTmpBatiment)
			{
				// Cr�e le triangle selector de la maison :
#if 1	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					//((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);		// Avec le mesh normal
					game->sceneManager->getMesh("Maison Individuelle Ombre.obj"), node);	// Avec le mesh de l'ombre
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis)
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
			// Charge seulement les mesh et les textures n�cessaires � cette maison
			if (forLoadingOnly)
			{
				game->sceneManager->getMesh("Maison Basse Consommation.obj");	// Mesh normal
				// On utilise l'ombre de la maison individuelle car les deux b�timents ont exactement la m�me ombre (�conomie de m�moire, acc�l�ration du chargement)
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

			// Cr�e le node de ce b�timent :
			if (!gameConfig.normalMappingEnabled || normalMappingMaterial == -1)
			{
				node = game->sceneManager->addAnimatedMeshSceneNode(
					game->sceneManager->getMesh("Maison Basse Consommation.obj"), parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling * 2.0f / 3.0f);
				if (!node)
					break;
			}
			else
			{
				// Charge cette mesh en demandant � Irrlicht de cr�er ses tangentes pour permettre l'utilisation du normal mapping
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

			// Cr�e une ombre pour ce b�timent :
			// On utilise l'ombre de la maison individuelle car les deux b�timents ont exactement la m�me ombre (�conomie de m�moire, acc�l�ration du chargement)
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					//game->sceneManager->getMesh("Maison Basse Consommation Ombre.obj"));
					game->sceneManager->getMesh("Maison Individuelle Ombre.obj"));

			if (!isTmpBatiment)
			{
				// Cr�e le triangle selector de la maison :
#if 1	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					//((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);			// Avec le mesh normal
					game->sceneManager->getMesh("Maison Individuelle Ombre.obj"), node);		// Avec le mesh de l'ombre	// "Maison Basse Consommation Ombre.obj"
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis)
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
			// Charge seulement les mesh et les textures n�cessaires � cette maison
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

			// Cr�e le node de ce b�timent :
			if (!gameConfig.normalMappingEnabled || normalMappingMaterial == -1)
			{
				node = game->sceneManager->addAnimatedMeshSceneNode(
					game->sceneManager->getMesh("Maison avec Panneaux Solaires.obj"), parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling * 2.0f / 3.0f);
				if (!node)
					break;
			}
			else
			{
				// Charge cette mesh en demandant � Irrlicht de cr�er ses tangentes pour permettre l'utilisation du normal mapping
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

			// Cr�e une ombre pour ce b�timent
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					game->sceneManager->getMesh("Maison avec Panneaux Solaires Ombre.obj"));

			if (!isTmpBatiment)
			{
				// Cr�e le triangle selector de la maison :
#if 1	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					//((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);					// Avec le mesh normal
					game->sceneManager->getMesh("Maison avec Panneaux Solaires Ombre.obj"), node);	// Avec le mesh de l'ombre
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis)
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
			// Charge seulement les mesh et les textures n�cessaires � cette maison
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

			// Cr�e le node de ce b�timent :
			if (!gameConfig.normalMappingEnabled || normalMappingMaterial == -1)
			{
				node = game->sceneManager->addAnimatedMeshSceneNode(
					game->sceneManager->getMesh("Grande Maison Individuelle.obj"), parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling * 2.0f / 3.0f);
				if (!node)
					break;
			}
			else
			{
				// Charge cette mesh en demandant � Irrlicht de cr�er ses tangentes pour permettre l'utilisation du normal mapping
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

			// Cr�e une ombre pour ce b�timent
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					game->sceneManager->getMesh("Grande Maison Individuelle Ombre.obj"));

			if (!isTmpBatiment)
			{
				// Cr�e le triangle selector de la maison :
#if 1	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					//((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);				// Avec le mesh normal
					game->sceneManager->getMesh("Grande Maison Individuelle Ombre.obj"), node);	// Avec le mesh de l'ombre
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis)
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
			// Charge seulement les mesh et les textures n�cessaires � ce chalet
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

			// Cr�e le node de ce b�timent :
			if (!gameConfig.normalMappingEnabled || normalMappingMaterial == -1)
			{
				node = game->sceneManager->addAnimatedMeshSceneNode(
					game->sceneManager->getMesh("Chalet.obj"), parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling * 2.0f / 3.0f);
				if (!node)
					break;
			}
			else
			{
				// Charge cette mesh en demandant � Irrlicht de cr�er ses tangentes pour permettre l'utilisation du normal mapping
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

			// Cr�e une ombre pour ce b�timent
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					game->sceneManager->getMesh("Chalet Ombre.obj"));

			if (!isTmpBatiment)
			{
				// Cr�e le triangle selector du chalet :
#if 1	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					//((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);		// Avec le mesh normal
					game->sceneManager->getMesh("Chalet Ombre.obj"), node);				// Avec le mesh de l'ombre
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis)
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
			// Charge seulement les mesh et les textures n�cessaires � cet immeuble
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

			// Cr�e le node de ce b�timent :
			if (!gameConfig.normalMappingEnabled || normalMappingMaterial == -1)
			{
				node = game->sceneManager->addAnimatedMeshSceneNode(
					game->sceneManager->getMesh("Immeuble Individuel.obj"), parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling * 2.0f / 3.0f);
				if (!node)
					break;
			}
			else
			{
				// Charge cette mesh en demandant � Irrlicht de cr�er ses tangentes pour permettre l'utilisation du normal mapping
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

			// Cr�e une ombre pour ce b�timent
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					game->sceneManager->getMesh("Immeuble Individuel Ombre.obj"));

			if (!isTmpBatiment)
			{
				// Cr�e le triangle selector de l'immeuble :
#if 1	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					//((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);		// Avec le mesh normal
					game->sceneManager->getMesh("Immeuble Individuel Ombre.obj"), node);	// Avec le mesh de l'ombre
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis)
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
			// Charge seulement les mesh et les textures n�cessaires � cet immeuble
			if (forLoadingOnly)
			{
				game->sceneManager->getMesh("Immeuble Basse Consommation.obj");		// Mesh normal
				// On utilise l'ombre de l'immeuble individuel car les deux b�timents ont exactement la m�me ombre (�conomie de m�moire, acc�l�ration du chargement)
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

			// Cr�e le node de ce b�timent :
			if (!gameConfig.normalMappingEnabled || normalMappingMaterial == -1)
			{
				node = game->sceneManager->addAnimatedMeshSceneNode(
					game->sceneManager->getMesh("Immeuble Basse Consommation.obj"), parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling * 2.0f / 3.0f);
				if (!node)
					break;
			}
			else
			{
				// Charge cette mesh en demandant � Irrlicht de cr�er ses tangentes pour permettre l'utilisation du normal mapping
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

			// Cr�e une ombre pour ce b�timent :
			// On utilise l'ombre de l'immeuble individuel car les deux b�timents ont exactement la m�me ombre (�conomie de m�moire, acc�l�ration du chargement)
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					//game->sceneManager->getMesh("Immeuble Basse Consommation Ombre.obj"));
					game->sceneManager->getMesh("Immeuble Individuel Ombre.obj"));

			if (!isTmpBatiment)
			{
				// Cr�e le triangle selector de l'immeuble :
#if 1	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					//((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);		// Avec le mesh normal
					game->sceneManager->getMesh("Immeuble Individuel Ombre.obj"), node);	// Avec le mesh de l'ombre	// "Immeuble Basse Consommation Ombre.obj"
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis)
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
			// Charge seulement les mesh et les textures n�cessaires � cet immeuble
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

			// Cr�e le node de ce b�timent :
			if (!gameConfig.normalMappingEnabled || normalMappingMaterial == -1)
			{
				node = game->sceneManager->addAnimatedMeshSceneNode(
					game->sceneManager->getMesh("Immeuble avec Panneaux Solaires.obj"), parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling * 2.0f / 3.0f);
				if (!node)
					break;
			}
			else
			{
				// Charge cette mesh en demandant � Irrlicht de cr�er ses tangentes pour permettre l'utilisation du normal mapping
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

			// Cr�e une ombre pour ce b�timent
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					game->sceneManager->getMesh("Immeuble avec Panneaux Solaires Ombre.obj"));

			if (!isTmpBatiment)
			{
				// Cr�e le triangle selector de l'immeuble :
#if 1	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					//((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);					// Avec le mesh normal
					game->sceneManager->getMesh("Immeuble avec Panneaux Solaires Ombre.obj"), node);	// Avec le mesh de l'ombre
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis)
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
			// Charge seulement les mesh et les textures n�cessaires � cet immeuble
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

			// Cr�e le node de ce b�timent :
			if (!gameConfig.normalMappingEnabled || normalMappingMaterial == -1)
			{
				node = game->sceneManager->addAnimatedMeshSceneNode(
					game->sceneManager->getMesh("Grand Immeuble Individuel.obj"), parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling * 2.0f / 3.0f);
				if (!node)
					break;
			}
			else
			{
				// Charge cette mesh en demandant � Irrlicht de cr�er ses tangentes pour permettre l'utilisation du normal mapping
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

			// Cr�e une ombre pour ce b�timent
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					game->sceneManager->getMesh("Grand Immeuble Individuel Ombre.obj"));

			if (!isTmpBatiment)
			{
				// Cr�e le triangle selector de l'immeuble :
#if 1	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					//((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);				// Avec le mesh normal
					game->sceneManager->getMesh("Grand Immeuble Individuel Ombre.obj"), node);	// Avec le mesh de l'ombre
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis)
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
			// Charge seulement les mesh et les textures n�cessaires � ce building
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

			// Cr�e le node de ce b�timent :
			if (!gameConfig.normalMappingEnabled || normalMappingMaterial == -1)
			{
				node = game->sceneManager->addAnimatedMeshSceneNode(
					game->sceneManager->getMesh("Building Individuel.obj"), parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling);
				if (!node)
					break;
			}
			else
			{
				// Charge cette mesh en demandant � Irrlicht de cr�er ses tangentes pour permettre l'utilisation du normal mapping
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

			// Cr�e une ombre pour ce b�timent
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					game->sceneManager->getMesh("Building Individuel Ombre.obj"));

			if (!isTmpBatiment)
			{
				// Cr�e le triangle selector du building :
#if 1	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					//((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);				// Avec le mesh normal
					game->sceneManager->getMesh("Building Individuel Ombre.obj"), node);	// Avec le mesh de l'ombre
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis)
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
			// Charge seulement les mesh et les textures n�cessaires � ce building
			if (forLoadingOnly)
			{
				game->sceneManager->getMesh("Building Basse Consommation.obj");		// Mesh normal
				// On utilise l'ombre du building individuel car les deux b�timents ont exactement la m�me ombre (�conomie de m�moire, acc�l�ration du chargement)
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

			// Cr�e le node de ce b�timent :
			if (!gameConfig.normalMappingEnabled || normalMappingMaterial == -1)
			{
				node = game->sceneManager->addAnimatedMeshSceneNode(
					game->sceneManager->getMesh("Building Basse Consommation.obj"), parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling);
				if (!node)
					break;
			}
			else
			{
				// Charge cette mesh en demandant � Irrlicht de cr�er ses tangentes pour permettre l'utilisation du normal mapping
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

			// Cr�e une ombre pour ce b�timent :
			// On utilise l'ombre du building individuel car les deux b�timents ont exactement la m�me ombre (�conomie de m�moire, acc�l�ration du chargement)
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					//game->sceneManager->getMesh("Building Basse Consommation Ombre.obj")
					game->sceneManager->getMesh("Building Individuel Ombre.obj"));

			if (!isTmpBatiment)
			{
				// Cr�e le triangle selector du building :
#if 1	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					//((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);		// Avec le mesh normal
					game->sceneManager->getMesh("Building Individuel Ombre.obj"), node);	// Avec le mesh de l'ombre	// "Building Basse Consommation Ombre.obj"
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis)
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
			// Charge seulement les mesh et les textures n�cessaires � ce building
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

			// Cr�e le node de ce b�timent :
			if (!gameConfig.normalMappingEnabled || normalMappingMaterial == -1)
			{
				node = game->sceneManager->addAnimatedMeshSceneNode(
					game->sceneManager->getMesh("Building avec Panneaux Solaires.obj"), parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling);
				if (!node)
					break;
			}
			else
			{
				// Charge cette mesh en demandant � Irrlicht de cr�er ses tangentes pour permettre l'utilisation du normal mapping
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

			// Cr�e une ombre pour ce b�timent
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					game->sceneManager->getMesh("Building avec Panneaux Solaires Ombre.obj"));

			if (!isTmpBatiment)
			{
				// Cr�e le triangle selector du building :
#if 1	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					//((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);				// Avec le mesh normal
					game->sceneManager->getMesh("Building avec Panneaux Solaires Ombre.obj"), node);	// Avec le mesh de l'ombre
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis)
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
			// Charge seulement les mesh et les textures n�cessaires � ce building
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

			// Cr�e le node de ce b�timent :
			if (!gameConfig.normalMappingEnabled || normalMappingMaterial == -1)
			{
				node = game->sceneManager->addAnimatedMeshSceneNode(
					game->sceneManager->getMesh("Grand Building Individuel.obj"), parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling);
				if (!node)
					break;
			}
			else
			{
				// Charge cette mesh en demandant � Irrlicht de cr�er ses tangentes pour permettre l'utilisation du normal mapping
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

			// Cr�e une ombre pour ce b�timent
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					game->sceneManager->getMesh("Grand Building Individuel Ombre.obj"));

			if (!isTmpBatiment)
			{
				// Cr�e le triangle selector du building :
#if 1	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					//((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);				// Avec le mesh normal
					game->sceneManager->getMesh("Grand Building Individuel Ombre.obj"), node);	// Avec le mesh de l'ombre
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis)
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

		// Production d'�nergie
	case BI_centrale_charbon:
		{
			// TODO

#ifdef USE_MATERIALS_MESHES
			// Indique que ce b�timent dispose de sa propre couleur de mat�riaux
			reinitMaterialsDiffuse = false;
#endif

			// Charge seulement les mesh et les textures n�cessaires � cette centrale
			if (forLoadingOnly)
			{
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Centrale nucleaire.obj");		// Mesh normal
#else
				game->sceneManager->getMesh("Centrale nucleaire - Avec materiaux.obj");		// Mesh normal avec mat�riaux
#endif
				//game->sceneManager->getMesh("Centrale nucleaire Ombre.obj");	// Mesh de l'ombre et du triangle selector (non utilis� finalement : � r�activer si r�-utilis�)

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

			// Cr�e une ombre pour ce b�timent
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					//game->sceneManager->getMesh("Centrale nucleaire Ombre.obj")
					);

			if (!isTmpBatiment)
			{
				// Cr�e le triangle selector de la centrale :
#if 1	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);		// Avec le mesh normal
					//game->sceneManager->getMesh("Centrale nucleaire Ombre.obj"), node);	// Avec le mesh de l'ombre
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis)
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
			// Charge seulement les mesh et les textures n�cessaires � ce panneau solaire
			if (forLoadingOnly)
			{
				game->sceneManager->getMesh("Panneau Solaire.obj");		// Mesh normal
				//game->sceneManager->getMesh("Panneau Solaire Ombre.obj");	// Mesh de l'ombre et du triangle selector (non utilis� finalement : � r�activer si r�-utilis�)

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

			// Cr�e le node de ce b�timent :
			if (!gameConfig.normalMappingEnabled || normalMappingMaterial == -1)
			{
				node = game->sceneManager->addAnimatedMeshSceneNode(
					game->sceneManager->getMesh("Panneau Solaire.obj"), parent, -1, core::vector3df(), core::vector3df(), blenderObjectsScaling);
				if (!node)
					break;
			}
			else
			{
				// Charge cette mesh en demandant � Irrlicht de cr�er ses tangentes pour permettre l'utilisation du normal mapping
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

			// Cr�e une ombre pour ce b�timent
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode();

			if (!isTmpBatiment)
			{
				// Cr�e le triangle selector du panneau solaire :
#if 1	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);		// Avec le mesh normal
					//game->sceneManager->getMesh("Panneau Solaire Ombre.obj"), node);	// Avec le mesh de l'ombre
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis)
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
			// Indique que ce b�timent dispose de sa propre couleur de mat�riaux
			reinitMaterialsDiffuse = false;
#endif

			// Charge seulement les mesh et les textures n�cessaires � cette �olienne
			if (forLoadingOnly)
			{
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Eolienne.x");		// Mesh normal
#else
				game->sceneManager->getMesh("Eolienne - Avec materiaux.x");		// Mesh normal avec mat�riaux
#endif
				//game->sceneManager->getMesh("Eolienne Ombre.x");	// Mesh de l'ombre et du triangle selector (non utilis� finalement : � r�activer si r�-utilis�)

				// Charge la texture de l'�olienne
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

			// Charge la texture de l'�olienne
			//{
			//	io::path textureName = "";
			//	textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
			//	textureName.append(".bmp");
			//	node->setMaterialTexture(0, game->driver->getTexture(textureName));
			//}

#ifdef _DEBUG
			// Affiche les donn�es de d�bogage sur l'�olienne
			//node->setDebugDataVisible(scene::EDS_FULL & ~(scene::EDS_HALF_TRANSPARENCY));
#endif

			// Modifie la vitesse de l'animation (� 15 fps) et lui ajoute une valeur al�atoire entre -2.5f et 2.5f
			// -> Une �olienne a une vitesse de rotation d'environ : 1/3 tour/seconde
			((scene::IAnimatedMeshSceneNode*)node)->setAnimationSpeed(15.0f + ((rand() % 51) - 25) * 0.1f);

			// D�sactive les ombres pour les �oliennes : bugg�es et trop co�teuses en temps de rendu
			//if (gameConfig.stencilShadowsEnabled)
			//	((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
			//		//game->sceneManager->getMesh("Eolienne Ombre.x")
			//		);

			if (!isTmpBatiment)
			{
				// Cr�e le triangle selector de l'�olienne :
#if 0	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)	// D�sactiv� : la cam�ra FPS peut �tre coinc�e dans ce triangle selector !
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);	// Avec le mesh normal
					//game->sceneManager->getMesh("Eolienne Ombre.x"), node);			// Avec le mesh de l'ombre
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis)
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
			// Indique que ce b�timent dispose de sa propre couleur de mat�riaux
			reinitMaterialsDiffuse = false;
#endif

			// Charge seulement les mesh et les textures n�cessaires � cette hydrolienne
			if (forLoadingOnly)
			{
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Hydrolienne.x");		// Mesh normal
#else
				game->sceneManager->getMesh("Hydrolienne - Avec materiaux.x");		// Mesh normal avec mat�riaux
#endif
				//game->sceneManager->getMesh("Hydrolienne Ombre.x");	// Mesh de l'ombre et du triangle selector (non utilis� finalement : � r�activer si r�-utilis�)

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
			// Affiche les donn�es de d�bogage sur l'hydrolienne
			//node->setDebugDataVisible(scene::EDS_FULL & ~(scene::EDS_HALF_TRANSPARENCY));
#endif

			// Modifie la vitesse de l'animation (� 15 fps) et lui ajoute une valeur al�atoire entre -2.5f et 2.5f
			// -> Les hydroliennes ont une fr�quence de rotation d'environ 20 tours/min (15 frames par seconde)
			((scene::IAnimatedMeshSceneNode*)node)->setAnimationSpeed(15.0f + ((rand() % 51) - 25) * 0.1f);

			// D�sactive les ombres pour les hydroliennes : bugg�es et trop co�teuses en temps de rendu
			//if (gameConfig.stencilShadowsEnabled)
			//	((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
			//		game->sceneManager->getMesh("Hydrolienne Ombre.x"));

			if (!isTmpBatiment)
			{
				// Cr�e le triangle selector de l'hydrolienne :
#if 0	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)	// D�sactiv� : la cam�ra FPS peut �tre coinc�e dans ce triangle selector !
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					//((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);	// Avec le mesh normal
					game->sceneManager->getMesh("Hydrolienne Ombre.x"), node);		// Avec le mesh de l'ombre
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis)
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
			// Indique que ce b�timent dispose de sa propre couleur de mat�riaux
			reinitMaterialsDiffuse = false;
#endif

			// Charge seulement les mesh et les textures n�cessaires � cette hydrolienne
			if (forLoadingOnly)
			{
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Pompe d'extraction d'eau.x");		// Mesh normal
#else
				game->sceneManager->getMesh("Pompe d'extraction d'eau - Avec materiaux.x");		// Mesh normal avec mat�riaux
#endif
				//game->sceneManager->getMesh("Pompe d'extraction d'eau Ombre.x");	// Mesh de l'ombre et du triangle selector (non utilis� finalement : � r�activer si r�-utilis�)

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
			// Affiche les donn�es de d�bogage sur la pompe d'extraction d'eau
			//node->setDebugDataVisible(scene::EDS_FULL & ~(scene::EDS_HALF_TRANSPARENCY));
#endif

			// Modifie la vitesse de l'animation (� 15 fps) et lui ajoute une valeur al�atoire entre -2.5f et 2.5f
			// -> Les pompes d'extraction d'eau ont une fr�quence de rotation d'environ 20 tours/min (15 frames par seconde)
			((scene::IAnimatedMeshSceneNode*)node)->setAnimationSpeed(15.0f + ((rand() % 51) - 25) * 0.1f);

			// D�sactive les ombres pour les pompes d'extraction d'eau : bugg�es et trop co�teuses en temps de rendu
			//if (gameConfig.stencilShadowsEnabled)
			//	((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
			//		game->sceneManager->getMesh("Pompe d'extraction d'eau Ombre.x"));

			if (!isTmpBatiment)
			{
				// Cr�e le triangle selector de l'hydrolienne :
#if 1	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)	// D�sactiv� : la cam�ra FPS peut �tre coinc�e dans ce triangle selector !
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);					// Avec le mesh normal
					//game->sceneManager->getMesh("Pompe d'extraction d'eau Ombre.x"), node);	// Avec le mesh de l'ombre
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis)
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
			// Indique que ce b�timent dispose de sa propre couleur de mat�riaux
			reinitMaterialsDiffuse = false;
#endif

			// Charge seulement les mesh et les textures n�cessaires � cette usine
			if (forLoadingOnly)
			{
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Usine de verre.obj");		// Mesh normal
#else
				game->sceneManager->getMesh("Usine de verre - Avec materiaux.obj");		// Mesh normal avec mat�riaux
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

			// Cr�e une ombre pour ce b�timent
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					//game->sceneManager->getMesh("Usine de verre Ombre.obj")
					);

			if (!isTmpBatiment)
			{
				// Cr�e le triangle selector de l'usine :
#if 1	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);	// Avec le mesh normal
					//game->sceneManager->getMesh("Usine de verre Ombre.x"), node);	// Avec le mesh de l'ombre
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis)
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
			// Indique que ce b�timent dispose de sa propre couleur de mat�riaux
			reinitMaterialsDiffuse = false;
#endif

			// Charge seulement les mesh et les textures n�cessaires � cette usine
			if (forLoadingOnly)
			{
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Usine de verre.obj");		// Mesh normal
#else
				game->sceneManager->getMesh("Usine de verre - Avec materiaux.obj");		// Mesh normal avec mat�riaux
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

			// Cr�e le node parent des quatre petites usines qui composeront cette grande usine
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

			// Cr�e un meta triangle selector pour contenir les selectors des 4 usines et les assigner � leur node parent
			scene::IMetaTriangleSelector* metaTriSelector = NULL;
			if (!isTmpBatiment)
				metaTriSelector = game->sceneManager->createMetaTriangleSelector();

			// Cr�e les quatres usines c�tes � c�tes avec des rotations diff�rentes
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

				// Cr�e une ombre pour ce b�timent
				if (gameConfig.stencilShadowsEnabled)
					nodes[i]->addShadowVolumeSceneNode(
						//game->sceneManager->getMesh("Usine de verre Ombre.obj")
						);

				if (!isTmpBatiment)
				{
					// Cr�e le triangle selector de l'usine :
#if 1	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)
					scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
						nodes[i]->getMesh(), nodes[i]);									// Avec le mesh normal
						//game->sceneManager->getMesh("Usine de verre Ombre.x"), nodes[i]);	// Avec le mesh de l'ombre
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis)
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
			// Indique que ce b�timent dispose de sa propre couleur de mat�riaux
			reinitMaterialsDiffuse = false;
#endif

			// Charge seulement les mesh et les textures n�cessaires � cette usine
			if (forLoadingOnly)
			{
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Usine de ciment.obj");		// Mesh normal
#else
				game->sceneManager->getMesh("Usine de ciment - Avec materiaux.obj");		// Mesh normal avec mat�riaux
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

			// Cr�e une ombre pour ce b�timent
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					//game->sceneManager->getMesh("Usine de ciment Ombre.obj")
					);

			if (!isTmpBatiment)
			{
				// Cr�e le triangle selector de l'usine :
#if 1	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);	// Avec le mesh normal
					//game->sceneManager->getMesh("Usine de ciment Ombre.x"), node);	// Avec le mesh de l'ombre
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis)
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
			// Indique que ce b�timent dispose de sa propre couleur de mat�riaux
			reinitMaterialsDiffuse = false;
#endif

			// Charge seulement les mesh et les textures n�cessaires � cette usine
			if (forLoadingOnly)
			{
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Usine de ciment.obj");		// Mesh normal
#else
				game->sceneManager->getMesh("Usine de ciment - Avec materiaux.obj");		// Mesh normal avec mat�riaux
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

			// Cr�e le node parent des quatre petites usines qui composeront cette grande usine
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

			// Cr�e un meta triangle selector pour contenir les selectors des 4 usines et les assigner � leur node parent
			scene::IMetaTriangleSelector* metaTriSelector = NULL;
			if (!isTmpBatiment)
				metaTriSelector = game->sceneManager->createMetaTriangleSelector();

			// Cr�e les quatres usines c�tes � c�tes avec des rotations diff�rentes
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

				// Cr�e une ombre pour ce b�timent
				if (gameConfig.stencilShadowsEnabled)
					nodes[i]->addShadowVolumeSceneNode(
						//game->sceneManager->getMesh("Usine de ciment Ombre.obj")
						);

				if (!isTmpBatiment)
				{
					// Cr�e le triangle selector de l'usine :
#if 1	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)
					scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
						nodes[i]->getMesh(), nodes[i]);									// Avec le mesh normal
						//game->sceneManager->getMesh("Usine de ciment Ombre.x"), nodes[i]);	// Avec le mesh de l'ombre
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis)
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
			// Indique que ce b�timent dispose de sa propre couleur de mat�riaux
			reinitMaterialsDiffuse = false;
#endif

			// Charge seulement les mesh et les textures n�cessaires � cette usine
			if (forLoadingOnly)
			{
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Usine de tuiles.obj");		// Mesh normal
#else
				game->sceneManager->getMesh("Usine de tuiles - Avec materiaux.obj");		// Mesh normal avec mat�riaux
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

			// Cr�e une ombre pour ce b�timent
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					//game->sceneManager->getMesh("Usine de tuiles Ombre.obj")
					);

			if (!isTmpBatiment)
			{
				// Cr�e le triangle selector de l'usine :
#if 1	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);	// Avec le mesh normal
					//game->sceneManager->getMesh("Usine de tuiles Ombre.x"), node);	// Avec le mesh de l'ombre
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis)
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
			// Indique que ce b�timent dispose de sa propre couleur de mat�riaux
			reinitMaterialsDiffuse = false;
#endif

			// Charge seulement les mesh et les textures n�cessaires � cette usine
			if (forLoadingOnly)
			{
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Usine de tuiles.obj");		// Mesh normal
#else
				game->sceneManager->getMesh("Usine de tuiles - Avec materiaux.obj");		// Mesh normal avec mat�riaux
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

			// Cr�e le node parent des quatre petites usines qui composeront cette grande usine
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

			// Cr�e un meta triangle selector pour contenir les selectors des 4 usines et les assigner � leur node parent
			scene::IMetaTriangleSelector* metaTriSelector = NULL;
			if (!isTmpBatiment)
				metaTriSelector = game->sceneManager->createMetaTriangleSelector();

			// Cr�e les quatres usines c�tes � c�tes avec des rotations diff�rentes
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

				// Cr�e une ombre pour ce b�timent
				if (gameConfig.stencilShadowsEnabled)
					nodes[i]->addShadowVolumeSceneNode(
						//game->sceneManager->getMesh("Usine de tuiles Ombre.obj")
						);

				if (!isTmpBatiment)
				{
					// Cr�e le triangle selector de l'usine :
#if 1	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)
					scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
						nodes[i]->getMesh(), nodes[i]);									// Avec le mesh normal
						//game->sceneManager->getMesh("Usine de tuiles Ombre.x"), nodes[i]);	// Avec le mesh de l'ombre
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis)
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
#ifdef KID_VERSION	// En mode enfant : on r�activer le papier comme ressource consomm�e par les habitants pour leur satisfaction
	case BI_usine_papier_petite:
		{
#ifdef USE_MATERIALS_MESHES
			// Indique que ce b�timent dispose de sa propre couleur de mat�riaux
			reinitMaterialsDiffuse = false;
#endif

			// Charge seulement les mesh et les textures n�cessaires � cette usine
			if (forLoadingOnly)
			{
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Usine de papier.obj");		// Mesh normal
#else
				game->sceneManager->getMesh("Usine de papier - Avec materiaux.obj");		// Mesh normal avec mat�riaux
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

			// Cr�e une ombre pour ce b�timent
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					//game->sceneManager->getMesh("Usine de papier Ombre.obj")
					);

			if (!isTmpBatiment)
			{
				// Cr�e le triangle selector de l'usine :
#if 1	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);	// Avec le mesh normal
					//game->sceneManager->getMesh("Usine de papier Ombre.x"), node);	// Avec le mesh de l'ombre
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis)
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
#else				// On d�sactive l'usine � tout faire en mode enfant qui est devenue inutile
	case BI_usine_tout:
#endif
		{
#ifdef USE_MATERIALS_MESHES
			// Indique que ce b�timent dispose de sa propre couleur de mat�riaux
			reinitMaterialsDiffuse = false;
#endif

			// Charge seulement les mesh et les textures n�cessaires � cette usine
			if (forLoadingOnly)
			{
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Usine de papier.obj");		// Mesh normal
#else
				game->sceneManager->getMesh("Usine de papier - Avec materiaux.obj");		// Mesh normal avec mat�riaux
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

			// Cr�e le node parent des quatre petites usines qui composeront cette grande usine
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

			// Cr�e un meta triangle selector pour contenir les selectors des 4 usines et les assigner � leur node parent
			scene::IMetaTriangleSelector* metaTriSelector = NULL;
			if (!isTmpBatiment)
				metaTriSelector = game->sceneManager->createMetaTriangleSelector();

			// Cr�e les quatres usines c�tes � c�tes avec des rotations diff�rentes
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

				// Cr�e une ombre pour ce b�timent
				if (gameConfig.stencilShadowsEnabled)
					nodes[i]->addShadowVolumeSceneNode(
						//game->sceneManager->getMesh("Usine de papier Ombre.obj")
						);

				if (!isTmpBatiment)
				{
					// Cr�e le triangle selector de l'usine :
#if 1	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)
					scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
						nodes[i]->getMesh(), nodes[i]);									// Avec le mesh normal
						//game->sceneManager->getMesh("Usine de papier Ombre.x"), nodes[i]);	// Avec le mesh de l'ombre
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis)
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

		// Gestion de l'effet de serre et des d�chets
	case BI_decharge:
		{
			// TODO

#ifdef USE_MATERIALS_MESHES
			// Indique que ce b�timent dispose de sa propre couleur de mat�riaux
			reinitMaterialsDiffuse = false;
#endif

			// Charge seulement les mesh et les textures n�cessaires � cette d�charge
			if (forLoadingOnly)
			{
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Usine de papier.obj");		// Mesh normal
#else
				game->sceneManager->getMesh("Usine de papier - Avec materiaux.obj");		// Mesh normal avec mat�riaux
#endif
				//game->sceneManager->getMesh("Usine de papier Ombre.obj");	// Mesh de l'ombre et du triangle selector

				// Charge la texture de la d�charge
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

			// Charge la texture de la d�charge
			//{
			//	io::path textureName = "";
			//	textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
			//	textureName.append(".bmp");
			//	node->setMaterialTexture(0, game->driver->getTexture(textureName));
			//}

			// Cr�e une ombre pour ce b�timent
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					//game->sceneManager->getMesh("Usine de papier Ombre.obj")
					);

			if (!isTmpBatiment)
			{
				// Cr�e le triangle selector de la d�charge :
#if 1	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);	// Avec le mesh normal
					//game->sceneManager->getMesh("Usine de papier Ombre.x"), node);	// Avec le mesh de l'ombre
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis)
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
			// Indique que ce b�timent dispose de sa propre couleur de mat�riaux
			reinitMaterialsDiffuse = false;
#endif

			// Charge seulement les mesh et les textures n�cessaires � cette d�charge
			if (forLoadingOnly)
			{
#ifndef USE_MATERIALS_MESHES
				game->sceneManager->getMesh("Centrale nucleaire.obj");		// Mesh normal
#else
				game->sceneManager->getMesh("Centrale nucleaire - Avec materiaux.obj");		// Mesh normal avec mat�riaux
#endif
				//game->sceneManager->getMesh("Centrale nucleaire Ombre.obj");	// Mesh de l'ombre et du triangle selector

				// Charge la texture de l'usine d'incin�ration
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

			// Charge la texture de l'usine d'incin�ration
			//{
			//	io::path textureName = "";
			//	textureName.append(getBatimentTextureSuffixe(gameConfig.usePowerOf2Textures, gameConfig.texturesQuality));
			//	textureName.append(".bmp");
			//	node->setMaterialTexture(0, game->driver->getTexture(textureName));
			//}

			// Cr�e une ombre pour ce b�timent
			if (gameConfig.stencilShadowsEnabled)
				((scene::IAnimatedMeshSceneNode*)node)->addShadowVolumeSceneNode(
					//game->sceneManager->getMesh("Centrale nucleaire Ombre.obj")
					);

			if (!isTmpBatiment)
			{
				// Cr�e le triangle selector de l'usine d'incin�ration :
#if 1	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(
					((scene::IAnimatedMeshSceneNode*)node)->getMesh(), node);		// Avec le mesh normal
					//game->sceneManager->getMesh("Centrale nucleaire Ombre.x"), node);	// Avec le mesh de l'ombre
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis)
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

			// Obtient les fichiers li�s au type d'arbre "arbreType"
			sprintf_SS("%s.xml", arbreType.c_str());

			scene::CTreeGenerator* generator = NULL;
			if (!isTmpBatiment || forLoadingOnly)	// On initialise le g�n�rateur si ce n'est pas un b�timent temporaire ou si on est en mode de chargement
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



			// Calcule l'index de cet arbre dans la liste des arbres de pr�visualisation
			const int index = (int)batimentID - (int)BI_arbre_aspen;

			// G�n�re un nombre al�atoire pour la cr�ation de l'arbre
			const int seed = rand();

			// Les pointeurs sur les mesh buffers des deux premiers niveaux de d�tail de l'arbre
			scene::STreeMesh* highLOD = NULL;
			scene::STreeMesh* midLOD = NULL;
			if (forLoadingOnly)
			{
				// Si doit juste charger les arbres, on ne cr�e que leur buffer de pr�visualisation
				if (!treesHighLOD[index])
				{
					treesHighLOD[index] = generator->generateTree(3, seed, true, 0);	// On DOIT laisser le cutoffLevel du niveau de LOD le plus haut � 0 pour que sa bounding box soit correctement calcul�e
					highLOD = treesHighLOD[index];
				}
				if (!treesMidLOD[index])
				{
					treesMidLOD[index] = generator->generateTree(3, seed, false, 2);
					midLOD = treesMidLOD[index];
				}
			}
#ifdef USE_VARIOUS_TREES	// G�n�re un nouvel arbre unique
			else if (!isTmpBatiment)
			{
				// G�n�re un nouvel arbre s'il n'est pas temporaire
				highLOD = generator->generateTree(3, seed, true, 0);	// Par d�faut : (8, seed, true, 0)
				midLOD = generator->generateTree(3, seed, false, 2);	// Par d�faut : (4, seed, false, 1)
			}
#endif						// Sinon, on utilise les arbres de pr�visualisation pour le rendu
			else
			{
				// Finalement, si l'arbre est temporaire, on prend celui qui est d�j� cr��
				highLOD = treesHighLOD[index];
				midLOD = treesMidLOD[index];
			}

			// Calcule les proportions des agrandissements pour que l'arbre ait une taille en X et en Z de TAILLE_OBJETS, et une taille de 100.0f pour TAILLE_OBJETS = 40.0f en Y (on utilisera TAILLE_OBJETS * 0.9f pour que les feuilles de l'arbre ne d�passnt pas trop de la place disponible)
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

			if (forLoadingOnly)	// Si on ne doit que charger le mesh buffer de l'arbre, on quitte ici avant de cr�er son node
			{
				if (generator)
					generator->drop();

				break;
			}

			// Cr�e l'arbre
			scene::CTreeSceneNode* const tree = new scene::CTreeSceneNode(parent, game->sceneManager);

			// On n'utilise que le niveau de d�tail moyen si l'arbre est temporaire
			tree->setup(highLOD, midLOD, billTexture);

#ifdef USE_VARIOUS_TREES
			if (!forLoadingOnly && !isTmpBatiment)
			{
				// Lib�re les mesh des arbres s'ils ont �t� g�n�r�s
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

			// TODO : Cr�er des ombres pour les arbres
			//if (gameConfig.stencilShadowsEnabled) // Aucune ombre ne peut �tre cr�e

			// Indique le node � renvoyer
			node = tree;

			if (!isTmpBatiment)
			{
				// Cr�e le triangle selector de l'arbre :
#if 0
				// D�sactiv� : les triangle selector obtenus �taient tellements complexes qu'ils bloquaient la cam�ra FPS (elle n'arrivait plus � ressortir des arbres)
				// Triangle selector pr�cis d'apr�s les triangles du buffer de niveau de d�tail moyen
				scene::SMesh selectorMesh;
				selectorMesh.addMeshBuffer(midLOD->MeshBuffer);
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelector(&selectorMesh, tree);
				tree->setTriangleSelector(selector);
				selector->drop();
#else
				// Triangle selector d'apr�s la bounding box de l'arbre (qui est prise d'apr�s la bounding box de son plus haut niveau de d�tail)
				scene::ITriangleSelector* selector = game->sceneManager->createTriangleSelectorFromBoundingBox(tree);
				tree->setTriangleSelector(selector);
				selector->drop();
#endif
			}

			// Lib�re la m�moire
			tree->drop();
			if (generator)
				generator->drop();
		}
		break;

	default:
		LOG_DEBUG("EcoWorldRenderer::loadBatiment(" << batimentID << ") : Type de batiment inconnu : " << batimentID, ELL_WARNING);
		break;
	}

	// Modifie quelques propri�t�s des mat�riaux de ce node et de ses enfants
	if (node)
	{
		// D�termine si on doit activer ou d�sactiver les lumi�res dynamiques, suivant si les lumi�res sont g�r�es par Irrlicht ou game->xEffects
		const bool enableLighting = !(gameConfig.useXEffectsShadows && game->xEffects);

		// Propri�t�s des mat�riaux du node :
		const u32 nodeMatCount = node->getMaterialCount();
		const bool nodeNormalizeNormals = !(node->getScale().equals(core::vector3df(1.0f), 0.01f));	// D�termine si ce node est agrandi/r�tr�ci pour savoir si on doit activer la normalisation des normales
		for (u32 i = 0; i < nodeMatCount; ++i)
		{
			// Obtient le mat�riau actuel
			video::SMaterial& mat = node->getMaterial(i);

			mat.Lighting = enableLighting;				// Active/D�sactive les lumi�res dynamiques suivant si les lumi�res sont g�r�es par Irrlicht ou xEffects
#ifdef USE_MATERIALS_MESHES
			if (reinitMaterialsDiffuse)
			{
#endif
				mat.DiffuseColor.set(255, 255, 255, 255);	// Active la lumi�re diffuse
				mat.AmbientColor.set(255, 255, 255, 255);	// Active la lumi�re ambiante
#ifdef USE_MATERIALS_MESHES
			}
			else
				mat.AmbientColor = mat.DiffuseColor;	// Indique comme lumi�re ambiante la m�me valeur que la lumi�re diffuse, pour des raisons de coh�rence des lumi�res (aussi li� avec la fonction Game::resetBatimentColor)
#endif
			mat.Shininess = 0.0f;						// Enl�ve les reflets de lumi�re

			mat.NormalizeNormals = nodeNormalizeNormals;// Active la normalisation des normales si ce node est agrandi/r�tr�ci
			mat.FogEnable = true;						// Active le brouillard
		}

		// V�rifie que ce node est temporaire (destin� � la pr�visualisation), sinon on ne lui ajoute ni effet de flou de profondeur, ni d'ombre : ces effets seront ajout�s plus loin dans EcoWorldRenderer::addBatiment
		if (isTmpBatiment)
		{
			// Ajoute l'effet de flou de profondeur � ce node gr�ce � PostProcess (ses enfants seront aussi affect�s)
			if (gameConfig.usePostProcessEffects && gameConfig.postProcessUseDepthRendering && game->postProcessManager)
				game->postProcessManager->addNodeToDepthPass(node);

			// Ajoute une ombre � ce node gr�ce � XEffects, et lui permet aussi de les recevoir (ses enfants seront aussi affect�s)
			if (gameConfig.useXEffectsShadows && game->xEffects)
				game->xEffects->addShadowToNode(node, ESM_BOTH);
		}



		// Propri�t� des mat�riaux des enfants :
		const core::list<scene::ISceneNode*>& nodeList = node->getChildren();
		const core::list<scene::ISceneNode*>::ConstIterator END = nodeList.end();
		for (core::list<scene::ISceneNode*>::ConstIterator it = nodeList.begin(); it != END; ++it)
		{
			scene::ISceneNode* const child = (*it);

			const u32 childMatCount = child->getMaterialCount();
			const bool childNormalizeNormals = !(child->getScale().equals(core::vector3df(1.0f), 0.01f));	// D�termine si ce node est agrandi/r�tr�ci pour savoir si on doit activer la normalisation des normales
			for (u32 i = 0; i < childMatCount; ++i)
			{
				// Obtient le mat�riau actuel
				video::SMaterial& mat = child->getMaterial(i);

				mat.Lighting = enableLighting;				// Active/D�sactive les lumi�res dynamiques suivant si les lumi�res sont g�r�es par Irrlicht ou xEffects
#ifdef USE_MATERIALS_MESHES
				if (reinitMaterialsDiffuse)
				{
#endif
					mat.DiffuseColor.set(255, 255, 255, 255);	// Active la lumi�re diffuse
					mat.AmbientColor.set(255, 255, 255, 255);	// Active la lumi�re ambiante
#ifdef USE_MATERIALS_MESHES
				}
				else
					mat.AmbientColor = mat.DiffuseColor;	// Indique comme lumi�re ambiante la m�me valeur que la lumi�re diffuse, pour des raisons de coh�rence des lumi�res (aussi li� avec la fonction Game::resetBatimentColor)
#endif
				mat.Shininess = 0.0f;						// Enl�ve les reflets de lumi�re

				mat.NormalizeNormals = childNormalizeNormals;// Active la normalisation des normales si ce node est agrandi/r�tr�ci
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

	// Permet de cr�er un centre de construction entre les bords de la construction
	// Peut �tre d�sactiv� car le b�timent comporte toujours une dalle en b�ton en-dessous de lui, qui peut donc ainsi faire office de centre de construction
//#define USE_CONSTRUCTION_CENTER

#ifdef USE_CONSTRUCTION_CENTER
	// Permet d'utiliser un grand cube pour le centre de construction, au lieu de charger le fichier "Construction Center.obj" qui ne repr�sente actuellement qu'un simple plan textur� et de le r�peter de nombreuses fois (un seul cube sera cr��)
	// (Avec le plan du fichier 3D, les m�mes probl�mes de clignotement que pour la route plane se font sentir lorsque l'objet est � grande distance de la cam�ra)
#define USE_CUBE_CONSTRUCTION_CENTER
#endif

	// Permet d'utiliser de grands plans pour les bords de la construction, au lieu de charger le fichier "Construction Border.obj" qui ne repr�sente actuellement qu'un simple plan textur� et de le r�peter de nombreuses fois
#define USE_PLANE_CONSTRUCTION_BORDER

	// Charge seulement les mesh et les textures n�cessaires � la construction
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
			textureName.append(getBatimentTextureSuffixe(false, gameConfig.texturesQuality));	// Cette texture n'est pas disponible en puissances de deux : elle y est par d�faut
			textureName.append(".bmp");
			game->driver->getTexture(textureName);

#ifdef USE_CONSTRUCTION_CENTER
			textureName = "Construction_Center_Texture";
			textureName.append(getBatimentTextureSuffixe(false, gameConfig.texturesQuality));	// Cette texture n'est pas disponible en puissances de deux : elle y est par d�faut
			textureName.append(".bmp");
			game->driver->getTexture(textureName);
#endif
		}

		return NULL;
	}

	// Obtient la taille du b�timent d�sir�
	const core::dimension2du& size = StaticBatimentInfos::getInfos(batimentID).taille;
	const core::dimension2df& halfSize = StaticBatimentInfos::getInfos(batimentID).halfTaille;

	// Agrandis un peu les batiments export�s avec Blender, car les unit�s d'Irrlicht sont plus grandes que celles de Blender
	// 2.0f sous Blender <=> 1.0f sous EcoWorld avec : 20.0f / 40.0f * TAILLE_OBJETS = TAILLE_OBJETS * 0.5f
	const core::vector3df blenderObjectsScaling(TAILLE_OBJETS * 0.5f);

	// Le node parent de tous les �lements de la construction
	scene::ISceneNode* const node = game->sceneManager->addEmptySceneNode(parent);

	// Cr�e un meta triangle selector pour contenir les selectors des bords et les assigner � leur node parent
	scene::IMetaTriangleSelector* metaTriSelector = game->sceneManager->createMetaTriangleSelector();

	// Variables d'it�ration :
	u32 i
#if !defined(USE_PLANE_CONSTRUCTION_BORDER) || (defined(USE_CONSTRUCTION_CENTER) && !defined(USE_CUBE_CONSTRUCTION_CENTER))
		, j , k
#endif
		;
	scene::IAnimatedMeshSceneNode* tmpNode;		// Node temporaire pour la cr�ation des �lements du node de la construction
	scene::ITriangleSelector* selector;			// Triangle Selector temporaire pour la cr�ation des triangles selectors des �lements du node de la construction

#ifndef USE_PLANE_CONSTRUCTION_BORDER
	// Cr�e les bords du haut et du bas
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
				textureName.append(getBatimentTextureSuffixe(false, gameConfig.texturesQuality));	// Cette texture n'est pas disponible en puissances de deux : elle y est par d�faut
				textureName.append(".bmp");
				tmpNode->setMaterialTexture(0, game->driver->getTexture(textureName));
			}

			// Indique que ce node a une texture transparente
#if 0	// Texture pr�cise compl�tement d�sactiv�e : beaucoup trop lente (-> la texture a �t� recr��e avec ces deux niveaux de transparence seulement)
		// + Ne fonctionne pas avec game->xEffects
			if (gameConfig.texturesQuality == GameConfiguration::ETQ_HIGH || gameConfig.texturesQuality == GameConfiguration::ETQ_MEDIUM)
				tmpNode->setMaterialType(video::EMT_TRANSPARENT_ALPHA_CHANNEL);		// Texture transparente pr�cise mais lente (utilise tous les niveaux de transparence de l'image)
			else
#endif
				tmpNode->setMaterialType(video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF);	// Texture transparente peu pr�cise mais rapide (utilise seulement 2 niveaux de transparence)

			// Modifie quelques propri�t�s des mat�riaux
			for (k = 0; k < tmpNode->getMaterialCount(); ++k)
			{
				video::SMaterial& mat = tmpNode->getMaterial(k);			// R�cup�re le mat�riau actif
				mat.Shininess = 0.0f;										// Enl�ve les reflets de lumi�re
				mat.AmbientColor = video::SColor(255, 255, 255, 255);		// Active la lumi�re ambiante
			}
			tmpNode->setMaterialFlag(video::EMF_NORMALIZE_NORMALS, true);
			tmpNode->setMaterialFlag(video::EMF_FOG_ENABLE, true);			// Active le brouillard
			tmpNode->setMaterialFlag(video::EMF_BACK_FACE_CULLING, false);	// D�sactive le culling

			if (gameConfig.anisotropicFilterEnabled)
				tmpNode->setMaterialFlag(video::EMF_ANISOTROPIC_FILTER, true);

			if (gameConfig.stencilShadowsEnabled)
				tmpNode->addShadowVolumeSceneNode(
					//game->sceneManager->getMesh("Construction Border Ombre.obj")
					);
	}

	// Cr�e les bords de gauche et de droite
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
				textureName.append(getBatimentTextureSuffixe(false, gameConfig.texturesQuality));	// Cette texture n'est pas disponible en puissances de deux : elle y est par d�faut
				textureName.append(".bmp");
				tmpNode->setMaterialTexture(0, game->driver->getTexture(textureName));
			}

			// Indique que ce node a une texture transparente
#if 0	// Texture pr�cise compl�tement d�sactiv�e : beaucoup trop lente
		// + Ne fonctionne pas avec game->xEffects
			if (gameConfig.texturesQuality == GameConfiguration::ETQ_HIGH || gameConfig.texturesQuality == GameConfiguration::ETQ_MEDIUM)
				tmpNode->setMaterialType(video::EMT_TRANSPARENT_ALPHA_CHANNEL);		// Texture transparente pr�cise mais lente (utilise tous les niveaux de transparence de l'image)
			else
#endif
				tmpNode->setMaterialType(video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF);	// Texture transparente peu pr�cise mais rapide (utilise seulement 2 niveaux de transparence)

			// Modifie quelques propri�t�s des mat�riaux
			for (k = 0; k < tmpNode->getMaterialCount(); ++k)
			{
				video::SMaterial& mat = tmpNode->getMaterial(k);			// R�cup�re le mat�riau actif
				mat.Shininess = 0.0f;										// Enl�ve les reflets de lumi�re
				mat.AmbientColor = video::SColor(255, 255, 255, 255);		// Active la lumi�re ambiante
			}
			tmpNode->setMaterialFlag(video::EMF_NORMALIZE_NORMALS, true);
			tmpNode->setMaterialFlag(video::EMF_FOG_ENABLE, true);			// Active le brouillard
			tmpNode->setMaterialFlag(video::EMF_BACK_FACE_CULLING, false);	// D�sactive le culling

			if (gameConfig.anisotropicFilterEnabled)
				tmpNode->setMaterialFlag(video::EMF_ANISOTROPIC_FILTER, true);

			if (gameConfig.stencilShadowsEnabled)
				tmpNode->addShadowVolumeSceneNode(
					//game->sceneManager->getMesh("Construction Border Ombre.obj")
					);

			// Cr�e le triangle selector de ce bord :
#if 0	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)
			selector = game->sceneManager->createTriangleSelector(tmpNode->getMesh(), tmpNode);
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis) (-> surtout moins bugg� dans ce cas !)
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
						textureName.append(getBatimentTextureSuffixe(false, gameConfig.texturesQuality));	// Cette texture n'est pas disponible en puissances de deux : elle y est par d�faut
						textureName.append(".bmp");
						tmpNode->setMaterialTexture(0, game->driver->getTexture(textureName));
					}

					// Indique que ce node a une texture transparente
#if 0	// Texture pr�cise compl�tement d�sactiv�e : beaucoup trop lente (-> la texture a �t� recr��e avec ces deux niveaux de transparence seulement)
		// + Ne fonctionne pas avec XEffects
					if (gameConfig.texturesQuality == GameConfiguration::ETQ_HIGH || gameConfig.texturesQuality == GameConfiguration::ETQ_MEDIUM)
						tmpNode->setMaterialType(video::EMT_TRANSPARENT_ALPHA_CHANNEL);		// Texture transparente pr�cise mais lente (utilise tous les niveaux de transparence de l'image)
					else
#endif
						tmpNode->setMaterialType(video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF);	// Texture transparente peu pr�cise mais rapide (utilise seulement 2 niveaux de transparence)

					tmpNode->setMaterialFlag(video::EMF_FOG_ENABLE, true);			// Active le brouillard
					tmpNode->setMaterialFlag(video::EMF_BACK_FACE_CULLING, false);	// D�sactive le culling

					if (gameConfig.stencilShadowsEnabled)
						tmpNode->addShadowVolumeSceneNode();

					// Cr�e le triangle selector de ce bord :
#if 0	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)
					selector = game->sceneManager->createTriangleSelector(tmpNode->getMesh(), tmpNode);
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis) (-> surtout moins bugg� dans ce cas !)
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
	// Cr�e les centres
#ifndef USE_CUBE_CONSTRUCTION_CENTER
	for (i = 0; i < size.Width; ++i)
	{
		for (j = 0; j < size.Height; ++j)
		{
			tmpNode = game->sceneManager->addAnimatedMeshSceneNode(
				game->sceneManager->getMesh("Construction Center.obj"), node, -1,
				core::vector3df(((float)i - halfSize.Width) * 2.0f + 1.0f, 0.0f, ((float)j - halfSize.Height) * 2.0f + 1.0f) * blenderObjectsScaling,
				core::vector3df(0.0f, (rand() % 4) * 90.0f, 0.0f),		// Rotation en Y al�atoire
				blenderObjectsScaling);
			if (!tmpNode)
				continue;

			// Charge la texture de la construction
			{
				io::path textureName = "Construction_Center_Texture";
				textureName.append(getBatimentTextureSuffixe(false, gameConfig.texturesQuality));	// Cette texture n'est pas disponible en puissances de deux : elle y est par d�faut
				textureName.append(".bmp");
				tmpNode->setMaterialTexture(0, game->driver->getTexture(textureName));
			}

			// Modifie quelques propri�t�s des mat�riaux
			for (k = 0; k < tmpNode->getMaterialCount(); ++k)
			{
				video::SMaterial& mat = tmpNode->getMaterial(k);			// R�cup�re le mat�riau actif
				mat.Shininess = 0.0f;										// Enl�ve les reflets de lumi�re
				mat.AmbientColor = video::SColor(255, 255, 255, 255);		// Active la lumi�re ambiante
			}
			tmpNode->setMaterialFlag(video::EMF_NORMALIZE_NORMALS, true);
			tmpNode->setMaterialFlag(video::EMF_FOG_ENABLE, true);			// Active le brouillard
			tmpNode->setMaterialFlag(video::EMF_BACK_FACE_CULLING, false);	// D�sactive le culling

			if (gameConfig.anisotropicFilterEnabled)
				tmpNode->setMaterialFlag(video::EMF_ANISOTROPIC_FILTER, true);

			if (gameConfig.stencilShadowsEnabled)
				tmpNode->addShadowVolumeSceneNode(
					//game->sceneManager->getMesh("Construction Center Ombre.obj")
					);

			// Cr�e le triangle selector de ce centre :
#if 1	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis)
			selector = game->sceneManager->createTriangleSelector(tmpNode->getMesh(), tmpNode);
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis) (-> Pas dans le cas d'un plan comme utilis� ici => Ce cas-l� n'est pas bugg� (le "bug" provient des normales des faces : la collision ne fonctionne que dans un sens))
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

		// Modifie le nombre de r�petitions de la texture du centre sur les face du cube
		if (centerMesh)
		{
			scene::IMeshBuffer* const mb = centerMesh->getMeshBuffer(0);
			if (mb)
			{
				// V�rifie que le type de vertices utilis� est bien le type standard et que notre cube comporte bien 12 vertices
				if (mb->getVertexType() == video::EVT_STANDARD && mb->getVertexCount() == 12)
				{
					// Obtient les vertices de ce mesh
					video::S3DVertex* vertices = (video::S3DVertex*)mb->getVertices();

					// Mutliplie chaque coordonn�e de texture de chaque vertice par l'agrandissement de la texture
					// TODO : Les textures sont beaucoup trop �tir�es en hauteur sur les c�t�s du cube, r�soudre ce probl�me
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
					textureName.append(getBatimentTextureSuffixe(false, gameConfig.texturesQuality));	// Cette texture n'est pas disponible en puissances de deux : elle y est par d�faut
					textureName.append(".bmp");
					tmpCubeNode->setMaterialTexture(0, game->driver->getTexture(textureName));
				}

				tmpCubeNode->setMaterialFlag(video::EMF_FOG_ENABLE, true);			// Active le brouillard
				//tmpCubeNode->setMaterialFlag(video::EMF_GOURAUD_SHADING, false);	// D�sactive l'�clairage Gouraud pour �viter des effets de lumi�re peu coh�rents

				if (gameConfig.anisotropicFilterEnabled)
					tmpCubeNode->setMaterialFlag(video::EMF_ANISOTROPIC_FILTER, true);

				// Cr�e le triangle selector de ce centre :
#if 0	// Si activ� : gr�ce � sa g�om�trie 3D (plus lent, mais plus pr�cis) (-> Pas dans le cas d'un cube comme utilis� ici : ici, les deux cas sont �quivalents)
				selector = game->sceneManager->createTriangleSelector(tmpCubeNode->getMesh(), tmpCubeNode);
#else	// Sinon : gr�ce � sa bounding box (plus rapide, mais moins pr�cis)
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

	// Retourne enfin le node cr��
	return node;
}
scene::ISceneNode* EcoWorldRenderer::loadConcreteBatiment(float denivele, BatimentID batimentID, scene::ISceneNode* parent, bool forLoadingOnly, bool isTmpBatiment)
{
	if (batimentID == BI_aucun)
		return NULL;

	// V�rifie que ce b�timent n�cessite un bloc de b�ton sous lui
	if (!StaticBatimentInfos::needConcreteUnderBatiment(batimentID))
		return NULL;

	if (!parent)
		parent = game->sceneManager->getRootSceneNode();

	// Charge la texture n�cessaire � la dalle en b�ton
	if (forLoadingOnly)
	{
		game->driver->getTexture("Concrete.jpg");

		// Charge la texture pour le normal mapping
		if (gameConfig.normalMappingEnabled && normalMappingMaterial != -1)
			game->driver->getTexture("Concrete_NormalMap.bmp");

		return NULL;
	}

	// Obtient la taille du b�timent d�sir�
	const core::dimension2du& size = StaticBatimentInfos::getInfos(batimentID).taille;
	const core::dimension2df sizeF((float)size.Width, (float)size.Height);
	const core::vector3df nodeSize(
		sizeF.Width * TAILLE_OBJETS,
		(core::ceil32(denivele / TAILLE_OBJETS) + 2.0f) * TAILLE_OBJETS,	// La valeur ajout�e au d�nivel� (ici : 2.0f * TAILLE_OBJETS) est r�glable suivant la pr�cision moyenne des d�nivel�s fournis ; Le d�nivel� est ici arrondi � la valeur sup�rieure de TAILLE_OBJETS pour minimiser les probl�mes de textures sur le cube
		sizeF.Height * TAILLE_OBJETS);

	// Cr�e le mesh en cube de la dalle de b�ton
	// Bas� sur CGeometryCreator.cpp d'Irrlicht 1.7.2 : CGeometryCreator::createCubeMesh
	scene::SMesh* mesh = NULL;
	{
		// Sch�ma pris de CCubeSceneNode.cpp d'Irrlicht 1.7.2 puis modifi� :
		/*
		Sur ce sch�ma, la face inf�rieure du cube est incluse :

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
		
		// Le mutliplicateur d'agrandissement de la texture de b�ton sur les faces du cube
		// (si les c�t�s du cube � cr�er mesurent 1.0f, alors il y aura exactement TEXTURE_SCALE r�p�titions de la texture par face)
#define TEXTURE_SCALE	0.05f
		const float X = nodeSize.X * TEXTURE_SCALE;
		const float Y = nodeSize.Y * TEXTURE_SCALE;
		const float Z = nodeSize.Z * TEXTURE_SCALE;
		const video::SColor clr(255, 255, 255, 255);

#if 1
		// Cr�e le cube sans cr�er sa face inf�rieure, qui ne sera normalement jamais visible et qui peut poser des probl�mes de texturage suivant le d�nivel� du terrain indiqu�
		// (pour �viter ces probl�mes en conservant cette face, il faudrait que TEXTURE_SCALE puisse s'�crire sous la forme : TEXTURE_SCALE = TAILLE_OBJETS * 10^x o� x est un entier relatif ; ou augmenter l'arrondissement du d�nivel� ci-dessus)

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
		// Cr�e le cube en entier, en incluant sa face inf�rieure

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

	// Cr�e le scene node d'apr�s ce mesh :
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
		// Charge cette mesh en demandant � Irrlicht de cr�er ses tangentes pour permettre l'utilisation du normal mapping
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
	//node->setMaterialFlag(video::EMF_GOURAUD_SHADING, false);	// D�sactive l'�clairage Gouraud pour �viter des effets de lumi�re peu coh�rents

	if (gameConfig.stencilShadowsEnabled)
		node->addShadowVolumeSceneNode();

	// Cr�e le triangle selector de ce node si ce n'est pas un b�timent temporaire
	if (!isTmpBatiment)
	{
		scene::ITriangleSelector* const selector = game->sceneManager->createTriangleSelectorFromBoundingBox(node);
		node->setTriangleSelector(selector);
		selector->drop();
	}

	// Retourne enfin le node cr��
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
#ifndef KID_VERSION	// On d�sactive les usines en mode enfant : Les ressources qu'elles n�cessitent ne peuvent pas �tre produites !
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

		// Production d'�nergie
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
#ifdef KID_VERSION	// En mode enfant : on r�activer le papier comme ressource consomm�e par les habitants pour leur satisfaction
	case BI_usine_papier_petite:
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 6.0f, 0.0f), parent);	break;
	case BI_usine_papier_grande:
#else				// On d�sactive l'usine � tout faire en mode enfant qui est devenue inutile
	case BI_usine_tout:
#endif
		textNode = addNomBatiment(StaticBatimentInfos::getInfos(batimentID).name, core::vector3df(0.0f, 9.0f, 0.0f), parent);	break;

		// Gestion de l'effet de serre et des d�chets
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
#ifndef KID_VERSION	// En mode enfant : on d�sactive la maison de base et les routes qui sont inutiles
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

		// Production d'�nergie
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
				// Valeurs obtenues d'apr�s EcoWorldRenderer::loadBatiment : case BI_usine_verre_grande :
				const core::vector3df addedPos(((i % 2) * 10.0f) - 5.0f, 0.0f, (i > 1 ? 10.0f : 0.0f) - 5.0f);

				// Le (i + 1) semble n�cessaire pour concorder avec la rotation du b�timent
				// TODO : Normalement, cet ajustement ne devrait pas �tre n�cessaire !
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
				// Valeurs obtenues d'apr�s EcoWorldRenderer::loadBatiment : case BI_usine_ciment_grande :
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
				// Valeurs obtenues d'apr�s EcoWorldRenderer::loadBatiment : case BI_usine_tuiles_grande :
				const core::vector3df addedPos(((i % 2) * 10.0f) - 5.0f, 0.0f, (i > 1 ? 10.0f : 0.0f) - 5.0f);

				// Le (i + 1) semble n�cessaire pour concorder avec la rotation du b�timent
				// TODO : Normalement, cet ajustement ne devrait pas �tre n�cessaire !
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
#ifdef KID_VERSION	// En mode enfant : on r�activer le papier comme ressource consomm�e par les habitants pour leur satisfaction
	case BI_usine_papier_petite:
		outSmoke1Positions.push_back(core::vector3df(-3.125f, 4.0f, 2.5f));	// Radius : 0.3f
		outSmoke1Positions.push_back(core::vector3df(-1.875f, 4.0f, 2.5f));	// Radius : 0.3f
		outSmoke1Positions.push_back(core::vector3df(3.0f, 4.0f, 0.5f));	// Radius : 0.5f
		break;
	case BI_usine_papier_grande:
#else				// On d�sactive l'usine � tout faire en mode enfant qui est devenue inutile
	case BI_usine_tout:
#endif
		{
			core::vector3df pos;
			for (int i = 0; i < 4; ++i)
			{
				// Valeurs obtenues d'apr�s EcoWorldRenderer::loadBatiment : case BI_usine_papier_grande :
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

		// Gestion de l'effet de serre et des d�chets
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

	// Ajuste la rotation des positions pour �tre conforme au b�timent
	// TODO : Normalement, ces ajustements ne devraient pas �tre n�cessaires !
	batRotationY += 180.0f;
	batRotationY *= -1.0f;

	// Transforme les positions avec les param�tres d'agrandissement et de rotation sp�cifi�s
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

	// Calcule la position de ce b�timent
	core::vector3df batPos;
	float denivele;
	if (position && deniveleTerrain)
	{
		batPos.set(*position);
		denivele = (*deniveleTerrain);
	}
	else
	{
		// Obtient la position en X et en Z du b�timent gr�ce � son index
		batPos = EcoWorldSystem::getPositionFromIndex(batiment->getIndex());

		// Obtient la position en Y du b�timent d'apr�s le terrain actuel
		denivele = getNewBatimentPos(batPos, batiment->getID(), batiment->getRotation());
	}

	// Cr�e un nouveau b�timent en utilisant la classe CBatimentSceneNode
	CBatimentSceneNode* node = new CBatimentSceneNode(batiment, batPos, denivele, game->sceneManager->getRootSceneNode());

	// Ajoute l'effet de flou de profondeur � ce node gr�ce � PostProcess (ses enfants seront aussi affect�s)
	if (gameConfig.usePostProcessEffects && gameConfig.postProcessUseDepthRendering && game->postProcessManager)
		game->postProcessManager->addNodeToDepthPass(node);

	// Ajoute une ombre � ce b�timent gr�ce � XEffects, et lui permet aussi de les recevoir (ses enfants seront aussi affect�s)
	if (gameConfig.useXEffectsShadows && game->xEffects)
		game->xEffects->addShadowToNode(node, ESM_BOTH);

	if (node->getTriangleSelector())
	{
#ifdef USE_COLLISIONS
		// Ajoute son triangle selector � celui de la cam�ra si ce n'est pas une route
		if (cameraTriangleSelector
#ifndef KID_VERSION	// En mode enfant : on d�sactive les routes qui sont inutiles
			&& batiment->getID() != BI_route
#endif
			)
			cameraTriangleSelector->addTriangleSelector(node->getTriangleSelector());
#endif

		// Ajoute son triangle selector � celui des batiments
		if (batimentsTriangleSelector)
			batimentsTriangleSelector->addTriangleSelector(node->getTriangleSelector());
	}

	// Lib�re le node cr��
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

	// V�rifie qu'aucun pointeur de Game ne pointe actuellement sur ce node, sinon ces pointeurs deviendront invalides
	checkGameNodePointersForNode(node);

	// Si PostProcess est activ�, on lui indique qu'il ne doit plus prendre en compte ce b�timent dans le flou de profondeur
	if (game->postProcessManager)
		game->postProcessManager->removeNodeFromDepthPass(node);

	// Si XEffects est activ�, on lui indique qu'il ne doit plus afficher l'ombre de ce b�timent
	if (game->xEffects)
		game->xEffects->removeShadowFromNode(node);

#ifdef USE_IRRKLANG
#if 0	// Mode 3D d�sactiv� car le son n'�tait pas assez fort
	// Joue le son d'un b�timent d�truit en mode 3D � la position du b�timent
	ikMgr.playGameSound3D(wantedByUser ? IrrKlangManager::EGSE_BATIMENT_DESTROYED : IrrKlangManager::EGSE_BATIMENT_DESTROYED_UNWANTED,
		node->getAbsolutePosition());
#else
	// Joue le son d'un b�timent d�truit en mode 2D
	ikMgr.playGameSound(wantedByUser ? IrrKlangManager::EGSE_BATIMENT_DESTROYED : IrrKlangManager::EGSE_BATIMENT_DESTROYED_UNWANTED);
#endif
#endif

	// Supprime le batiment
	node->setVisible(false);
	game->sceneManager->addToDeletionQueue(node);
}
void EcoWorldRenderer::update(bool initSunPosition)
{
	// Met � jour le scene manager, IrrKlang et SPARK suivant le temps actuel dans le Weather Manager :

	const WeatherInfos& weatherInfos = game->system.getWeatherManager().getCurrentWeatherInfos();

	// Brouillard
	game->driver->setFog(weatherInfos.fogColor, video::EFT_FOG_LINEAR, weatherInfos.fogStart, weatherInfos.fogEnd, 0.0f, true, true);

	// SkyDome
	if (skydome)
	{
#ifdef VITESSE_TOURNER_CIEL
		// Calcule la rotation de d�part du ciel d'apr�s le temps actuel du syst�me de jeu (la position du soleil est r�gl�e d'apr�s la rotation du ciel)
		if (initSunPosition)
			skydome->setRotation(core::vector3df(0.0f,
				game->system.getTime().getTotalTime() * VITESSE_TOURNER_CIEL * 100.0f,	// * 100.0f : (* 100.0f = * 1000.0f / 10.0f) car VITESSE_TOURNER_CIEL est en degr�s pour 10 ms, getTotalTime est en secondes et skydomeRotationY est en degr�s
				0.0f));
#endif

		// V�rifie si le shader du skydome est activ�
		if (gameConfig.skyDomeShadersEnabled && skyDomeMaterial != -1)
		{
			// Shader de skydome activ� :

			// On change la texture du dernier temps du ciel, s'il a chang�
			const WeatherID realLastWeatherID = game->system.getWeatherManager().getLastWeatherID();
			if (currentWeatherID != realLastWeatherID)
			{
				const WeatherInfos& lastWeatherInfos = game->system.getWeatherManager().getLastWeatherInfos();

				io::path skydomeTexturePath = lastWeatherInfos.skydomeTexturePath;
				skydomeTexturePath.append(getSkyTextureSuffixe(gameConfig.texturesQuality));
				skydome->setLastWeather(game->driver->getTexture(skydomeTexturePath), lastWeatherInfos.skydomeTexturePercentage, lastWeatherInfos.skydomeTextureOffset);

				currentWeatherID = realLastWeatherID;
			}

			// On change la texture du nouveau temps du ciel, s'il a chang�
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
			// Shader de skydome d�sactiv� :

			// On change la texture du ciel lorsque le temps du jeu a chang�
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
	// TODO : Trouver une solution plus rapide que d'utiliser cosf et sinf (fonctions trigonom�triques lentes) ?
#define SUN_RADIUS	1000.0f		// Le rayon du soleil (sa distance constante par rapport � la cam�ra, sans prendre en compte la coordonn�e Y)
#define SUN_HEIGHT	400.0f		// La hauteur du soleil en Y par rapport � la cam�ra
	const core::vector3df sunPosition(
		cosf(sunRotation) * SUN_RADIUS,
		SUN_HEIGHT,
		sinf(sunRotation) * SUN_RADIUS);
	if (sun)
	{
		// Indique sa nouvelle couleur
		sun->setColor(weatherInfos.sunColor);

		// Indique la position du soleil, d'apr�s sa hauteur et son rayon
		sun->setPosition(sunPosition);
	}

	// Ancienne m�thode de calcul de la position du soleil :
	/*{
		// Le soleil se d�place ici sur un cercle qui a pour centre la position actuelle de la cam�ra et pour rayon une valeur constante : radius.
		// Pour modifier le centre du cercle de la trajectoire du soleil (dont sa hauteur), il faut ajouter un nombre de type float � cameraPosition.? dans le calcul du nouveau vecteur de position du soleil.
		// Pour modifier le rayon du cercle de la trajectoire du soleil, il faut modifier la valeur de "radius".
		// Pour modifier le point de d�part du soleil sur sa trajectoire, il faut ajouter un nombre de type float � "rotation".
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

	// Lumi�res
	if (gameConfig.useXEffectsShadows && game->xEffects)
	{
		// Lumi�res de game->xEffects utilis�es :

		// Si initSunPosition est � true, on doit red�finier la position r�elle de la lumi�re :
		// On demande � la cam�ra RTS de recalculer sa position (m�me si cela est automatiquement effectu� � chaque rendu normalement)
		if (game->cameraRTS)
			game->cameraRTS->updateXEffects();

		// Indique la lumi�re ambiante � game->xEffects
		game->xEffects->setAmbientColor(weatherInfos.ambientLight.toSColor());

		// Met � jour la couleur de la lumi�re du soleil (sa position et sa direction seront mis � jour dans RTSCamera::updateXEffects(), lors de l'enregistrement de la cam�ra)
		if (game->xEffects->getShadowLightCount())
			game->xEffects->getShadowLight(0).setLightColor(weatherInfos.sunLightColor);
	}
	else
	{
		// Lumi�res d'Irrlicht utilis�es :

#ifdef VITESSE_TOURNER_CIEL
		// Calcule la rotation de d�part de la lumi�re du soleil et de celle des ombres d'apr�s le temps actuel du syst�me de jeu
		if (initSunPosition)
		{
			const float systemTotalTime = game->system.getTime().getTotalTime();
			if (sunLight)
			{
				// La lumi�re directionnelle du soleil est anim�e par un animator modifiant sa rotation :
				// Cet animator supporte la modification directe de la rotation du node
				core::vector3df sunLightRotation = sunLight->getRotation();
				sunLightRotation.Y = 180.0f + systemTotalTime * VITESSE_TOURNER_CIEL * 100.0f;	// *100.0f car VITESSE_TOURNER_CIEL est en degr�s pour 10 ms (�/10ms), et game->system.getTime().getTotalTime() est en secondes
				sunLight->setRotation(sunLightRotation);
			}
			if (shadowLight && gameConfig.stencilShadowsEnabled)
			{
				// La lumi�re ponctuelle du soleil pour les ombres est anim�e par un animator modifiant sa position suivant un cercle :
				// Cet animator ne supporte pas la modification directe de la position du node,
				// on doit donc le supprimer (normalement cette lumi�re ne contient que cet animator) puis le recr�er avec les bons param�tres
				shadowLight->removeAnimators();

				// Cr�e le nouvel animator avec les bons param�tres de d�part pour avoir une animation coh�rente avec la position du soleil
				// Calcul de la position de d�part : C'est le nombre de tours parcourus pendant le temps du jeu d�j� �coul� :
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

		// Met � jour la couleur de la lumi�re ambiante
		game->sceneManager->setAmbientLight(weatherInfos.ambientLight);

		// Met � jour la couleur de la lumi�re diffuse
		if (sunLight)
			sunLight->getLightData().DiffuseColor = weatherInfos.sunLightColor;

		// Ajuste la couleur des ombres d'Irrlicht
		if (gameConfig.stencilShadowsEnabled)
			game->sceneManager->setShadowColor(weatherInfos.shadowColor);
	}

#ifdef USE_IRRKLANG
	// Joue le son de la pluie si n�cessaire (d�termin� par le weather manager)
	const bool needRainSound = game->system.getWeatherManager().isRainSoundNeeded();
	if (needRainSound != isRainSoundPlaying)
	{
		ikMgr.playRainSound(needRainSound);
		isRainSoundPlaying = needRainSound;
	}
#endif
#ifdef USE_SPARK
	// Indique la densit� de la pluie
	sparkMgr.setRainFlow(weatherInfos.rainFlow);
#endif
}
void NormalMappingShaderCallBack::OnSetConstants(video::IMaterialRendererServices* services, int userData)
{
	// lightDirection et diffuseColor (d�termin�s d'apr�s la premi�re lumi�re dynamique disponible)
	if (game->driver->getDynamicLightCount() > 0)
	{
		const video::SLight& light = game->driver->getDynamicLight(0);
		services->setVertexShaderConstant("lightDirection", reinterpret_cast<const float*>(&light.Direction), 3);

		if (material)
		{
			// Inclus la couleur diffuse de la lumi�re et la couleur diffuse du mat�riau dans la couleur diffuse � envoyer au pixel shader
			video::SColorf diffuseColor(material->DiffuseColor);
			diffuseColor.r *= light.DiffuseColor.r;
			diffuseColor.g *= light.DiffuseColor.g;
			diffuseColor.b *= light.DiffuseColor.b;
			services->setPixelShaderConstant("diffuseColor", reinterpret_cast<const float*>(&diffuseColor), 3);
		}
		else
			services->setPixelShaderConstant("diffuseColor", reinterpret_cast<const float*>(&light.DiffuseColor), 3);
	}
	else	// Aucune lumi�re dynamique n'est disponible : on envoie des valeurs par d�faut aux shaders
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

	// Lit les informations � partir du fichier
	reader->resetPosition();
	if (in->read(reader, false, L"Renderer"))
	{
		// TODO

		in->clear();
	}
}*/
