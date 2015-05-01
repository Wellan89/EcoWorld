#include "EcoWorldTerrainManager.h"
#include "Game.h"
#include "EcoWorldRenderer.h"
#include "EcoWorldSystem.h"
#include "Batiment.h"
#include "RealisticWater.h"
#include "CLoadingScreen.h"
#include "CPostProcessManager.h"
#ifdef USE_IRRKLANG
#include "IrrKlangManager.h"
#include "irrKlangSceneNode.h"
#endif

EcoWorldTerrainManager::EcoWorldTerrainManager()
 : terrainTriangleSelector(0), irrlichtTerrainSceneNode(0), coplandTerrainSceneNode(0), tmykeTerrainSceneNode(0), n3xtdTerrainSceneNode(0),
 shaderWaterSceneNode(0), animatedWaterSceneNode(0)
{
	// Crée le meta triangle selector du terrain
	terrainTriangleSelector = game->sceneManager->createMetaTriangleSelector();

	// Initialise cette classe
	reset();
}
EcoWorldTerrainManager::~EcoWorldTerrainManager()
{
	// Détruit le triangle selector du terrain
	if (terrainTriangleSelector)
		terrainTriangleSelector->drop();
}
void EcoWorldTerrainManager::reset()
{
	// On suppose qu'ici, tous les scene nodes liés au terrain (terrain, eau...) ont déjà été supprimés du scene manager

	// Efface le triangle selector du terrain
	if (terrainTriangleSelector)
		terrainTriangleSelector->removeAllTriangleSelectors();

	// Remet tous les pointeurs à zéro et réinitialise les informations du terrain
	irrlichtTerrainSceneNode = 0;
	coplandTerrainSceneNode = 0;
	tmykeTerrainSceneNode = 0;
	n3xtdTerrainSceneNode = 0;
	animatedWaterSceneNode = 0;
	shaderWaterSceneNode = 0;
	terrainInfos.reset();

	// Supprime toutes les textures utilisées par le précédent terrain
	const core::list<video::ITexture*>::Iterator END = texturesList.end();
	for (core::list<video::ITexture*>::Iterator it = texturesList.begin(); it != END; ++it)
		game->driver->removeTexture(*it);
	texturesList.clear();
}
EcoWorldTerrainManager::TerrainPreviewInfos EcoWorldTerrainManager::readPreviewInfos(const io::path& adresse)
{
	TerrainPreviewInfos previewInfos;

	io::path imageAddress("Preview.bmp");

	// Ajoute l'archive de ce terrain au début du système de fichier d'Irrlicht :
	// Ainsi par exemple, si l'archive du terrain contient une texture nommée "texture.bmp" et que le dossier du jeu contient lui-aussi une texture du même nom,
	// alors en ajoutant cette nouvelle archive au début du système de fichier, ce sera la texture de l'archive du terrain qui sera chargée par l'appel à driver->getTexture("texture.bmp"), et non pas la texture de l'archive.
	// Les fichiers de l'archive du terrain (spécifique au terrain) sont ainsi prioritaires sur les autres fichiers du système de fichiers (globaux pour tous les terrains), ce qui permet de remplacer simplement une texture par défaut.
	io::IFileArchive* terrainFileArchive = NULL;	// Le pointeur vers la nouvelle archive du terrain, permettant ainsi de la supprimer du système de fichiers d'Irrlicht dès qu'elle ne sera plus nécessaire
	game->fileSystem->addFirstFileArchive(adresse, true, true, io::EFAT_ZIP, "", &terrainFileArchive);

	// Lit l'adresse de l'image de prévisualisation contenue dans le fichier "Terrain.xml"
	io::IXMLReader* const reader = game->fileSystem->createXMLReader("Terrain.xml");
	if (reader)
	{
		io::IAttributes* const in = game->fileSystem->createEmptyAttributes(game->driver);
		if (in)
		{
			//reader->resetPosition();
			if (in->read(reader, false, L"Preview"))
			{
				if (in->existsAttribute("Description Text"))	previewInfos.descriptionText = in->getAttributeAsStringW("Description Text");
				if (in->existsAttribute("Preview Image"))		imageAddress = in->getAttributeAsString("Preview Image");
				in->clear();
			}
			in->drop();
		}
		reader->drop();
	}

	// On indique au driver qu'il ne doit pas créer de niveaux de mip maps pour cette texture
	const bool lastMipMapsState = game->driver->getTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS);
	game->driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, false);

	// Demande au driver de charger l'image de prévisualisation
	previewInfos.previewImage = game->driver->getTexture(imageAddress);

	// On restaure la création de mip maps pour le driver
	game->driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, lastMipMapsState);

	// On supprime l'archive du terrain ajoutée précédemment, car elle n'est maintenant plus nécessaire (au contraire, ses fichiers pourraient continuer de remplacer les fichiers par défaut du jeu, créant des incohérences dans les données)
	if (terrainFileArchive)
		game->fileSystem->removeFileArchive(terrainFileArchive);

	return previewInfos;
}
void EcoWorldTerrainManager::loadNewTerrain(
#ifdef USE_SPARK
	SparkManager& sparkManager,
#endif
	const io::path& adresse, bool createSounds, bool createTriangleSelectors, scene::IMetaTriangleSelector* cameraTriangleSelector, bool updateSystemTerrain, CLoadingScreen* loadingScreen, float loadMin, float loadMax)
{
#ifdef USE_COLLISIONS
	// Si on ne crée pas de triangle selector, on désactive le triangle selector de la caméra
	if (!createTriangleSelectors)
		cameraTriangleSelector = NULL;
#else
	// Si les collisions sont désactivées, on force la désactivation de ces dernières
	createTriangleSelectors = false;
	cameraTriangleSelector = NULL;
#endif

	// Ajoute l'archive de ce terrain au début du système de fichier d'Irrlicht :
	// Ainsi par exemple, si l'archive du terrain contient une texture nommée "texture.bmp" et que le dossier du jeu contient lui-aussi une texture du même nom,
	// alors en ajoutant cette nouvelle archive au début du système de fichier, ce sera la texture de l'archive du terrain qui sera chargée par l'appel à driver->getTexture("texture.bmp"), et non pas la texture de l'archive.
	// Les fichiers de l'archive du terrain (spécifique au terrain) sont ainsi prioritaires sur les autres fichiers du système de fichiers (globaux pour tous les terrains), ce qui permet de remplacer simplement une texture par défaut.
	io::IFileArchive* terrainFileArchive = NULL;	// Le pointeur vers la nouvelle archive du terrain, permettant ainsi de la supprimer du système de fichiers d'Irrlicht dès qu'elle ne sera plus nécessaire
	game->fileSystem->addFirstFileArchive(adresse, true, true, io::EFAT_ZIP, "", &terrainFileArchive);
	if (!terrainFileArchive)
	{
		LOG_DEBUG(endl << "EcoWorldTerrainManager::loadNewTerrain(" << adresse.c_str() << ", ...) : Impossible d'ajouter l'archive du terrain au systeme de jeu !", ELL_WARNING);
		LOG_RELEASE(endl << "AVERTISSEMENT : Terrain non trouvé : \"" << adresse.c_str() << "\" => Chargement du terrain par défaut.", ELL_WARNING);
	}

	if (loadingScreen)
		if (loadingScreen->setPercentAndDraw(loadMin))	return;

	// On suppose ici qu'il ne subsiste pas de trace des précédents terrains (que la fonction reset() a déjà été appelée)

	// Réinitialise les informations du terrain
	terrainInfos.reset();
	terrainInfos.terrainName = adresse;

	// Lit les informations sur le terrain en chargement
	readTerrainInfos();

	// Vérifie que la height map est bien valide
	video::IImage* heightMap = game->driver->createImageFromFile(terrainInfos.terrain.heightMap);
	if (!heightMap)
	{
		LOG_DEBUG(endl << "EcoWorldTerrainManager::loadNewTerrain(" << adresse.c_str() << ", ...) : Height Map du terrain (\"" << terrainInfos.terrain.heightMap.c_str() << "\") invalide : Utilisation de la height map par defaut." << endl, ELL_WARNING);
		LOG_RELEASE(endl << "AVERTISSEMENT : Height Map du terrain (\"" << terrainInfos.terrain.heightMap.c_str() << "\") invalide => Utilisation de la height map par défaut !" << endl, ELL_WARNING);

		// Si la height map n'est pas valide, on crée une height map par défaut mesurant 1 seul pixel de valeur de hauteur 60
		heightMap = game->driver->createImage(video::ECF_R8G8B8, core::dimension2du(1, 1));
		heightMap->setPixel(0, 0, video::SColor(0, 60, 60, 60));
	}

	// Ici, on est certain que le pointeur sur la height map est bien valide

	const float heightMapMaxAltitude = max(calculateHeightMapMaxAltitude(heightMap), 1.0f);	// Vérifie que l'altitude maximale du terrain n'est pas en-dessous de 1.0f pour éviter les divisions par zéro dans les terrains qui ont un dénivelé nul

	// Calcule la position et la taille du terrain
	calculateTerrainPosAndSize(heightMapMaxAltitude);

	if (loadingScreen)
		if (loadingScreen->setPercentAndDraw((loadMax - loadMin) * 0.1f + loadMin))
		{
			// Supprime la height map
			heightMap->drop(); heightMap = NULL;
			return;
		}

	LOG_DEBUG("EcoWorldTerrainManager : Moteur de terrain actuel : " << terrainInfos.terrain.engine, ELL_INFORMATION);

	// Charge le terrain suivant le moteur choisi
	scene::ISceneNode* terrainSceneNode = NULL;
	switch (terrainInfos.terrain.engine)
	{
	case ETE_IRRLICHT_TERRAIN:
		loadIrrlichtTerrain(heightMap, createTriangleSelectors, cameraTriangleSelector, heightMapMaxAltitude, loadingScreen, (loadMax - loadMin) * 0.1f + loadMin, (loadMax - loadMin) * 0.8f + loadMin);
		terrainSceneNode = irrlichtTerrainSceneNode;
		break;
	case ETE_COPLAND_TERRAIN:
		loadCoplandTerrain(heightMap, createTriangleSelectors, cameraTriangleSelector, heightMapMaxAltitude, loadingScreen, (loadMax - loadMin) * 0.1f + loadMin, (loadMax - loadMin) * 0.8f + loadMin);
		terrainSceneNode = coplandTerrainSceneNode;
		break;
	case ETE_TMYKE_TERRAIN:
		loadTMykeTerrain(heightMap, createTriangleSelectors, cameraTriangleSelector, heightMapMaxAltitude, loadingScreen, (loadMax - loadMin) * 0.1f + loadMin, (loadMax - loadMin) * 0.8f + loadMin);
		terrainSceneNode = tmykeTerrainSceneNode;
		break;
	case ETE_N3XTD_TERRAIN:
		loadN3xtDTerrain(heightMap, createTriangleSelectors, cameraTriangleSelector, heightMapMaxAltitude, loadingScreen, (loadMax - loadMin) * 0.1f + loadMin, (loadMax - loadMin) * 0.8f + loadMin);
		terrainSceneNode = n3xtdTerrainSceneNode;
		break;

	default:
		LOG_DEBUG("EcoWorldTerrainManager::loadNewTerrain(" << adresse.c_str() << ", ...) : Moteur de terrain inconnu : " << terrainInfos.terrain.engine, ELL_WARNING);
		LOG_RELEASE("AVERTISSEMENT : Moteur de terrain inconnu (" << terrainInfos.terrain.engine << ") : Le moteur de terrain par défaut (" << ETE_IRRLICHT_TERRAIN << " ; Irrlicht Terrain) sera utilisé.", ELL_WARNING);

		// Restaure le moteur de terrain avec le moteur par défaut : celui d'Irrlicht
		terrainInfos.terrain.engine = ETE_IRRLICHT_TERRAIN;
		loadIrrlichtTerrain(heightMap, createTriangleSelectors, cameraTriangleSelector, heightMapMaxAltitude, loadingScreen, (loadMax - loadMin) * 0.1f + loadMin, (loadMax - loadMin) * 0.8f + loadMin);
		terrainSceneNode = irrlichtTerrainSceneNode;
		break;
	}

	// Ajoute l'effet de flou de profondeur à ce terrain grâce à PostProcess
	CPostProcessManager* const postProcessManager = game->postProcessManager;
	const bool postProcessEnabled = (gameConfig.usePostProcessEffects && gameConfig.postProcessUseDepthRendering && postProcessManager);
	if (postProcessEnabled)
		postProcessManager->addNodeToDepthPass(terrainSceneNode);

	// Indique à XEffects que ce terrain reçoit les ombres des autres objets et peut aussi leur en envoyer (par son dénivelé)
	EffectHandler* const xEffects = game->xEffects;
	const bool xEffectsEnabled = (gameConfig.useXEffectsShadows && xEffects);
	if (xEffectsEnabled && terrainSceneNode)
		xEffects->addShadowToNode(terrainSceneNode, ESM_BOTH);

	// La height map n'est plus valide après le chargement du terrain, on libère son image et on supprime son pointeur pour plus de sécurité
	heightMap->drop(); heightMap = NULL;

	if (loadingScreen)
		if (loadingScreen->setPercentAndDraw((loadMax - loadMin) * 0.8f + loadMin))	return;

	// Crée l'eau, le sol invisible et les plans de collisions au bord du terrain
	createTerrainWater(
#ifdef USE_SPARK
		sparkManager,
#endif
		createTriangleSelectors, cameraTriangleSelector);

	scene::ISceneNode* const waterSceneNode = (gameConfig.waterShaderEnabled ? shaderWaterSceneNode : animatedWaterSceneNode);
	if (waterSceneNode)
	{
		// Ajoute l'effet de flou de profondeur à l'eau grâce à PostProcess
		CPostProcessManager* const postProcessManager = game->postProcessManager;
		if (postProcessEnabled)
			postProcessManager->addNodeToDepthPass(waterSceneNode);

		// Indique à XEffects que l'eau reçoit les ombres des autres objets
		if (xEffectsEnabled)
			xEffects->addShadowToNode(waterSceneNode, ESM_RECEIVE);
	}

	if (loadingScreen)
		if (loadingScreen->setPercentAndDraw((loadMax - loadMin) * 0.9f + loadMin))	return;

#ifdef USE_IRRKLANG
	// Crée les sons du terrain si nécessaire
	// TODO : Ajouter d'autres sons que ceux de l'eau + Gérer la loading bar à l'intérieur de cette fonction
	if (createSounds)
		createTerrainSounds();
#endif

	// Met à jour le terrain système si nécessaire
	if (updateSystemTerrain)
		loadSystemTerrain(terrainInfos.terrain.engine);

	// On supprime l'archive du terrain ajoutée précédemment, car elle n'est maintenant plus nécessaire (au contraire, ses fichiers pourraient continuer de remplacer les fichiers par défaut du jeu, créant des incohérences dans les données)
	if (terrainFileArchive)
		game->fileSystem->removeFileArchive(terrainFileArchive);

	if (loadingScreen)
		loadingScreen->setPercentAndDraw(loadMax);

	LOG_DEBUG("Terrain charge : " << adresse.c_str(), ELL_INFORMATION);
}
void EcoWorldTerrainManager::readTerrainInfos()
{
	// Lit les données du fichier "Terrain.xml"
	io::IXMLReader* const reader = game->fileSystem->createXMLReader("Terrain.xml");
	if (!reader)
	{
		LOG_DEBUG(endl << "EcoWorldTerrainManager::readTerrainInfos() : Fichier d'informations du terrain \"Terrain.xml\" non trouve (Terrain en chargement : \"" << terrainInfos.terrainName.c_str() << "\") !" << endl, ELL_WARNING);
		LOG_RELEASE(endl << "AVERTISSEMENT : Fichier d'informations du terrain \"Terrain.xml\" non trouvé (Terrain en chargement : \"" << terrainInfos.terrainName.c_str() << "\") => Utilisation des valeurs par défaut." << endl, ELL_WARNING);
		return;
	}

	io::IAttributes* const in = game->fileSystem->createEmptyAttributes(game->driver);
	if (!in)
	{
		reader->drop();
		return;
	}

	reader->resetPosition();
	if (in->read(reader, false, L"Terrain"))
	{
		if (in->existsAttribute("Height Map"))					terrainInfos.terrain.heightMap = in->getAttributeAsString("Height Map");
		if (in->existsAttribute("Engine"))						terrainInfos.terrain.engine = (E_TERRAIN_ENGINE)in->getAttributeAsInt("Engine");
		if (in->existsAttribute("Position"))					terrainInfos.terrain.position = in->getAttributeAsVector3d("Position");
		if (in->existsAttribute("Size"))						terrainInfos.terrain.size = in->getAttributeAsVector3d("Size");
		in->clear();
	}
	reader->resetPosition();
	if (in->read(reader, false, L"Irrlicht_Terrain"))
	{
		if (in->existsAttribute("Color Texture"))				terrainInfos.terrain.irrlicht.colorTexture = in->getAttributeAsString("Color Texture");
		if (in->existsAttribute("Detail Texture"))				terrainInfos.terrain.irrlicht.detailTexture = in->getAttributeAsString("Detail Texture");
		if (in->existsAttribute("Color Texture Scale"))			terrainInfos.terrain.irrlicht.colorTextureScale = in->getAttributeAsFloat("Color Texture Scale");
		if (in->existsAttribute("Detail Texture Scale"))		terrainInfos.terrain.irrlicht.detailTextureScale = in->getAttributeAsFloat("Detail Texture Scale");
		if (in->existsAttribute("Vertex Color"))				terrainInfos.terrain.irrlicht.vertexColor = in->getAttributeAsColor("Vertex Color");

		if (in->existsAttribute("Patch Size"))
		{
			// Vérifie que la valeur de patchSize est bien valide
			const int patchSize = in->getAttributeAsInt("Patch Size");
			if (patchSize == ETPS_9 || patchSize == ETPS_17 || patchSize == ETPS_33 || patchSize == ETPS_65 || patchSize == ETPS_129)
				terrainInfos.terrain.irrlicht.patchSize = (E_TERRAIN_PATCH_SIZE)patchSize;
		}

		if (in->existsAttribute("Max LOD"))						terrainInfos.terrain.irrlicht.maxLOD = in->getAttributeAsInt("Max LOD");
		if (in->existsAttribute("Smooth Factor"))				terrainInfos.terrain.irrlicht.smoothFactor = in->getAttributeAsInt("Smooth Factor");
		in->clear();
	}
	reader->resetPosition();
	if (in->read(reader, false, L"Copland_Terrain"))
	{
		if (in->existsAttribute("Height Map Resolution"))		terrainInfos.terrain.copland.heightMapResolution = in->getAttributeAsInt("Height Map Resolution");
		if (in->existsAttribute("Quality"))						terrainInfos.terrain.copland.quality = (CTerrain_Copland::TerrainQuality)in->getAttributeAsInt("Quality");
		if (in->existsAttribute("Texture Scale"))				terrainInfos.terrain.copland.textureScale = in->getAttributeAsFloat("Texture Scale");
		if (in->existsAttribute("Max Render Distance"))			terrainInfos.terrain.copland.maxRenderDistance = in->getAttributeAsFloat("Max Render Distance");
		in->clear();
	}
	reader->resetPosition();
	if (in->read(reader, false, L"Copland_Terrain_Texture_Splatting"))
	{
		if (in->existsAttribute("Enabled"))						terrainInfos.terrain.copland.textSplat.enabled = in->getAttributeAsBool("Enabled");
		if (in->existsAttribute("Height"))						terrainInfos.terrain.copland.textSplat.height = in->getAttributeAsFloat("Height");
		if (in->existsAttribute("Texture 0"))					terrainInfos.terrain.copland.textSplat.texture0 = in->getAttributeAsString("Texture 0");
		if (in->existsAttribute("Texture 1"))					terrainInfos.terrain.copland.textSplat.texture1 = in->getAttributeAsString("Texture 1");
		if (in->existsAttribute("Texture 2"))					terrainInfos.terrain.copland.textSplat.texture2 = in->getAttributeAsString("Texture 2");
		if (in->existsAttribute("Detail Texture"))				terrainInfos.terrain.copland.textSplat.texture3 = in->getAttributeAsString("Detail Texture");
		in->clear();
	}
	reader->resetPosition();
	if (in->read(reader, false, L"Copland_Terrain_Normal_Texturing"))
	{
		if (in->existsAttribute("Color Texture"))				terrainInfos.terrain.copland.textNorm.colorTexture = in->getAttributeAsString("Color Texture");
		if (in->existsAttribute("Detail Texture"))				terrainInfos.terrain.copland.textNorm.detailTexture = in->getAttributeAsString("Detail Texture");
		in->clear();
	}
	reader->resetPosition();
	if (in->read(reader, false, L"TMyke_Terrain"))
	{
		if (in->existsAttribute("Height Map Resolution"))		terrainInfos.terrain.tmyke.heightMapResolution = in->getAttributeAsInt("Height Map Resolution");
		if (in->existsAttribute("Number of Quads in a row"))	terrainInfos.terrain.tmyke.numQuads = in->getAttributeAsInt("Number of Quads in a row");
		if (in->existsAttribute("Number of Faces per Quad"))	terrainInfos.terrain.tmyke.numFaces = in->getAttributeAsInt("Number of Faces per Quad");
		if (in->existsAttribute("Color Texture"))				terrainInfos.terrain.tmyke.colorTexture = in->getAttributeAsString("Color Texture");
		if (in->existsAttribute("Detail Texture"))				terrainInfos.terrain.tmyke.detailTexture = in->getAttributeAsString("Detail Texture");
		in->clear();
	}
	reader->resetPosition();
	if (in->read(reader, false, L"N3xtD_Terrain"))
	{
		if (in->existsAttribute("Height Map Resolution"))		terrainInfos.terrain.n3xtd.heightMapResolution = in->getAttributeAsInt("Height Map Resolution");
		if (in->existsAttribute("Quality"))						terrainInfos.terrain.n3xtd.quality = (CTerrainSceneNode_N3xtD::TerrainQuality)in->getAttributeAsInt("Quality");
		if (in->existsAttribute("Texture Scale"))				terrainInfos.terrain.n3xtd.textureScale = in->getAttributeAsFloat("Texture Scale");
		if (in->existsAttribute("Smooth Terrain"))				terrainInfos.terrain.n3xtd.smooth = in->getAttributeAsBool("Smooth Terrain");
		in->clear();
	}
	reader->resetPosition();
	if (in->read(reader, false, L"N3xtD_Terrain_Texture_Splatting"))
	{
		if (in->existsAttribute("Enabled"))						terrainInfos.terrain.n3xtd.textSplat.enabled = in->getAttributeAsBool("Enabled");
		if (in->existsAttribute("Mask Texture"))				terrainInfos.terrain.n3xtd.textSplat.texture0 = in->getAttributeAsString("Mask Texture");
		if (in->existsAttribute("Black Texture"))				terrainInfos.terrain.n3xtd.textSplat.texture1 = in->getAttributeAsString("Black Texture");
		if (in->existsAttribute("White Texture"))				terrainInfos.terrain.n3xtd.textSplat.texture2 = in->getAttributeAsString("White Texture");
		if (in->existsAttribute("Detail Texture"))				terrainInfos.terrain.n3xtd.textSplat.texture3 = in->getAttributeAsString("Detail Texture");
		in->clear();
	}
	reader->resetPosition();
	if (in->read(reader, false, L"N3xtD_Terrain_Normal_Texturing"))
	{
		if (in->existsAttribute("Color Texture"))				terrainInfos.terrain.n3xtd.textNorm.colorTexture = in->getAttributeAsString("Color Texture");
		if (in->existsAttribute("Detail Texture"))				terrainInfos.terrain.n3xtd.textNorm.detailTexture = in->getAttributeAsString("Detail Texture");
		in->clear();
	}
	reader->resetPosition();
	if (in->read(reader, false, L"Water"))
	{
		if (in->existsAttribute("Visible"))						terrainInfos.water.visible = in->getAttributeAsBool("Visible");
		if (in->existsAttribute("Height"))						terrainInfos.water.height = in->getAttributeAsFloat("Height");
		in->clear();
	}
	reader->resetPosition();
	if (in->read(reader, false, L"Shader_Water"))
	{
		if (in->existsAttribute("Bump Texture"))				terrainInfos.water.shader.bumpTexture = in->getAttributeAsString("Bump Texture");
		if (in->existsAttribute("Wind Force"))					terrainInfos.water.shader.windForce = in->getAttributeAsFloat("Wind Force");

		if (in->existsAttribute("Wind Direction"))
		{
			// Convertit le vector3df de in->getAttributeAsVector3d en vector2df
			const core::vector3df windDirection = in->getAttributeAsVector3d("Wind Direction");
			terrainInfos.water.shader.windDirection.set(windDirection.X, windDirection.Z);
		}

		if (in->existsAttribute("Water Color"))					terrainInfos.water.shader.waterColor = in->getAttributeAsColorf("Water Color");
		if (in->existsAttribute("Color Blend Factor"))			terrainInfos.water.shader.colorBlendFactor = in->getAttributeAsFloat("Color Blend Factor");

		in->clear();
	}
	reader->resetPosition();
	if (in->read(reader, false, L"Animated_Water"))
	{
		if (in->existsAttribute("Reflective Texture"))			terrainInfos.water.animated.reflectiveTexture = in->getAttributeAsString("Reflective Texture");
		if (in->existsAttribute("Non Reflective Texture"))		terrainInfos.water.animated.nonReflectiveTexture = in->getAttributeAsString("Non Reflective Texture");

		// Convertit deux floats de in->getAttributeAsFloat en dimension2df
		if (in->existsAttribute("Reflective Texture Repeat Count") && in->existsAttribute("Non Reflective Texture Repeat Count"))
		{
			const float reflectiveTextureRepeatCount = in->getAttributeAsFloat("Reflective Texture Repeat Count");
			const float nonReflectiveTextureRepeatCount = in->getAttributeAsFloat("Non Reflective Texture Repeat Count");
			terrainInfos.water.animated.texturesRepeatCount.set(reflectiveTextureRepeatCount, nonReflectiveTextureRepeatCount);
		}

		if (in->existsAttribute("Subdivisions"))				terrainInfos.water.animated.subdivisions = in->getAttributeAsDimension2d("Subdivisions");
		if (in->existsAttribute("Wave Height"))					terrainInfos.water.animated.waveHeight = in->getAttributeAsFloat("Wave Height");
		if (in->existsAttribute("Wave Speed"))					terrainInfos.water.animated.waveSpeed = in->getAttributeAsFloat("Wave Speed");
		if (in->existsAttribute("Wave Lenght"))					terrainInfos.water.animated.waveLenght = in->getAttributeAsFloat("Wave Lenght");
		in->clear();
	}
	reader->resetPosition();
	if (in->read(reader, false, L"Game_Map"))
	{
		if (in->existsAttribute("Construction Map"))			terrainInfos.gameSystem.constructionMap = in->getAttributeAsString("Construction Map");
		in->clear();
	}

	in->drop();
	reader->drop();

	/* Ecriture des données du fichier :

	// Terrain
	out->addString("Height Map", terrainInfos.terrain.heightMap.c_str());
	out->addInt("Engine", (int)terrainInfos.terrain.engine);
	out->addVector3d("Position", terrainInfos.terrain.position);
	out->addVector3d("Size", terrainInfos.terrain.size);
	out->write(writer, false, L"Terrain");
	out->clear();

	// Irrlicht Terrain
	out->addString("Color Texture", terrainInfos.terrain.irrlicht.colorTexture.c_str());
	out->addString("Detail Texture", terrainInfos.terrain.irrlicht.detailTexture.c_str());
	out->addFloat("Color Texture Scale", terrainInfos.terrain.irrlicht.colorTextureScale);
	out->addFloat("Detail Texture Scale", terrainInfos.terrain.irrlicht.detailTextureScale);
	out->addColor("Vertex Color", terrainInfos.terrain.irrlicht.vertexColor);
	out->addInt("Patch Size", (int)terrainInfos.terrain.irrlicht.patchSize);
	out->addInt("Max LOD", terrainInfos.terrain.irrlicht.maxLOD);
	out->addInt("Smooth Factor", terrainInfos.terrain.irrlicht.smoothFactor);
	out->write(writer, false, L"Irrlicht_Terrain");
	out->clear();

	// Copland Terrain
	out->addInt("Height Map Resolution", terrainInfos.terrain.copland.heightMapResolution);
	out->addInt("Quality", (int)terrainInfos.terrain.copland.quality);
	out->addFloat("Texture Scale", terrainInfos.terrain.copland.textureScale);
	out->addFloat("Max Render Distance", terrainInfos.terrain.copland.maxRenderDistance);
	out->write(writer, false, L"Copland_Terrain");
	out->clear();

	// Copland Terrain Texture Splatting
	out->addBool("Enabled", terrainInfos.terrain.copland.textSplat.enabled);
	out->addFloat("Height", terrainInfos.terrain.copland.textSplat.height);
	out->addString("Texture 0", terrainInfos.terrain.copland.textSplat.texture0.c_str());
	out->addString("Texture 1", terrainInfos.terrain.copland.textSplat.texture1.c_str());
	out->addString("Texture 2", terrainInfos.terrain.copland.textSplat.texture2.c_str());
	out->addString("Detail Texture", terrainInfos.terrain.copland.textSplat.texture3.c_str());
	out->write(writer, false, L"Copland_Terrain_Texture_Splatting");
	out->clear();

	// Copland Terrain Normal Texturing
	out->addString("Color Texture", terrainInfos.terrain.copland.textNorm.colorTexture.c_str());
	out->addString("Detail Texture", terrainInfos.terrain.copland.textNorm.detailTexture.c_str());
	out->write(writer, false, L"Copland_Terrain_Normal_Texturing");
	out->clear();

	// TMyke Terrain
	out->addInt("Height Map Resolution", terrainInfos.terrain.tmyke.heightMapResolution);
	out->addInt("Number of Quads in a row", terrainInfos.terrain.tmyke.numQuads);
	out->addInt("Number of Faces per Quad", terrainInfos.terrain.tmyke.numFaces);
	out->addString("Color Texture", terrainInfos.terrain.tmyke.colorTexture.c_str());
	out->addString("Detail Texture", terrainInfos.terrain.tmyke.detailTexture.c_str());
	out->write(writer, false, L"TMyke_Terrain");
	out->clear();

	// N3xtD Terrain
	out->addInt("Height Map Resolution", terrainInfos.terrain.n3xtd.heightMapResolution);
	out->addInt("Quality", (int)terrainInfos.terrain.n3xtd.quality);
	out->addFloat("Texture Scale", terrainInfos.terrain.n3xtd.textureScale);
	out->addBool("Smooth Terrain", terrainInfos.terrain.n3xtd.smooth);
	out->write(writer, false, L"N3xtD_Terrain");
	out->clear();

	// N3xtD Terrain Texture Splatting
	out->addBool("Enabled", terrainInfos.terrain.n3xtd.textSplat.enabled);
	out->addString("Mask Texture", terrainInfos.terrain.n3xtd.textSplat.texture0.c_str());
	out->addString("Black Texture", terrainInfos.terrain.n3xtd.textSplat.texture1.c_str());
	out->addString("White Texture", terrainInfos.terrain.n3xtd.textSplat.texture2.c_str());
	out->addString("Detail Texture", terrainInfos.terrain.n3xtd.textSplat.texture3.c_str());
	out->write(writer, false, L"N3xtD_Terrain_Texture_Splatting");
	out->clear();

	// N3xtD Terrain Normal Texturing
	out->addString("Color Texture", terrainInfos.terrain.n3xtd.textNorm.colorTexture.c_str());
	out->addString("Detail Texture", terrainInfos.terrain.n3xtd.textNorm.detailTexture.c_str());
	out->write(writer, false, L"N3xtD_Terrain_Normal_Texturing");
	out->clear();

	// Water
	out->addBool("Visible", terrainInfos.water.visible);
	out->addFloat("Height", terrainInfos.water.height);
	out->write(writer, false, L"Water");
	out->clear();

	// Shader Water
	out->addString("Bump Texture", terrainInfos.water.shader.bumpTexture.c_str());
	out->addFloat("Wind Force", terrainInfos.water.shader.windForce);
	out->addVector3d("Wind Direction", core::vector3df(terrainInfos.water.shader.windDirection.X, 0.0f, terrainInfos.water.shader.windDirection.Y));
	out->addColorf("Water Color", terrainInfos.water.shader.waterColor);
	out->addFloat("Color Blend Factor", terrainInfos.water.shader.colorBlendFactor);
	out->write(writer, false, L"Shader_Water");
	out->clear();

	// Animated Water
	out->addString("Reflective Texture", terrainInfos.water.animated.reflectiveTexture.c_str());
	out->addString("Non Reflective Texture", terrainInfos.water.animated.nonReflectiveTexture.c_str());
	out->addFloat("Reflective Texture Repeat Count", terrainInfos.water.animated.texturesRepeatCount.Width);
	out->addFloat("Non Reflective Texture Repeat Count", terrainInfos.water.animated.texturesRepeatCount.Height);
	out->addDimension2d("Subdivisions", terrainInfos.water.animated.subdivisions);
	out->addFloat("Wave Height", terrainInfos.water.animated.waveHeight);
	out->addFloat("Wave Speed", terrainInfos.water.animated.waveSpeed);
	out->addFloat("Wave Lenght", terrainInfos.water.animated.waveLenght);
	out->write(writer, false, L"Animated_Water");
	out->clear();

	// Game Map
	out->addString("Construction Map", terrainInfos.gameSystem.constructionMap.c_str());
	out->write(writer, false, L"Game_Map");
	out->clear();

	out->drop();

	*/
}
void EcoWorldTerrainManager::calculateTerrainPosAndSize(float heightMapMaxAltitude)
{
	// Ajuste la position du terrain pour qu'il soit au centre de la carte si elle est réglée sur "Automatique" (-1.0f) :
	// On lui met toutes ses valeurs à 0.0f, le moteur de terrain utilisé centrera le terrain sur cette position
	if (terrainInfos.terrain.position.X == -1.0f)
		terrainInfos.terrain.position.X = 0.0f;
	if (terrainInfos.terrain.position.Y == -1.0f)
		terrainInfos.terrain.position.Y = 0.0f;
	if (terrainInfos.terrain.position.Z == -1.0f)
		terrainInfos.terrain.position.Z = 0.0f;

	// Evite les valeurs nulles pour la taille du terrain (ce cas n'est pas géré ci-dessous) : on les remplace par -1.0f (valeur automatique)
	if (terrainInfos.terrain.size.X == 0.0f)
		terrainInfos.terrain.size.X = -1.0f;
	if (terrainInfos.terrain.size.Y == 0.0f)
		terrainInfos.terrain.size.Y = -1.0f;
	if (terrainInfos.terrain.size.Z == 0.0f)
		terrainInfos.terrain.size.Z = -1.0f;

	// Définit la taille du terrain en X et en Z : Fonctionnement :
	// - Si terrainInfos.terrain.size > 0.0f (mode "Manuel") :
	//     La taille du terrain sera celle spécifiée dans le fichier (terrainInfos.terrain.size)
	// - Si terrainInfos.terrain.size == -1.0f ou terrainInfos.terrain.size.Y == 0.0f (mode "Automatique") :
	//     La taille du terrain sera exactement celle de la carte du système (TAILLE_CARTE * TAILLE_OBJETS)
	// - Si terrainInfos.terrain.size < 0.0f (mode "Automatique ajusté") :
	//     La taille du terrain sera un mutliple de celle de la carte du système (TAILLE_CARTE * TAILLE_OBJETS * -terrainInfos.terrain.size)
	// (Note : On admettra que ce système fonctionne parfaitement, que TAILLE_CARTE soit pair ou non)
	if (terrainInfos.terrain.size.X < 0.0f)
		terrainInfos.terrain.size.X = TAILLE_CARTE * TAILLE_OBJETS * -terrainInfos.terrain.size.X;
	if (terrainInfos.terrain.size.Z < 0.0f)
		terrainInfos.terrain.size.Z = TAILLE_CARTE * TAILLE_OBJETS * -terrainInfos.terrain.size.Z;

	// Définit la taille du terrain en Y : Fonctionnement :
	// - Si terrainInfos.terrain.size.Y > 0.0f (mode "Manuel") :
	//     La taille du terrain sera celle spécifiée dans le fichier (terrainInfos.terrain.size.Y)
	// - Si terrainInfos.terrain.size.Y == -1.0f ou terrainInfos.terrain.size.Y == 0.0f (mode "Automatique") :
	//     La taille du terrain sera la hauteur maximale du terrain d'après le dénivelé de sa height map
	// - Si terrainInfos.terrain.size.Y < 0.0f (mode "Automatique ajusté") :
	//     La taille du terrain sera un mutliple de la hauteur maximale du terrain
	//
	// => La hauteur maximale du terrain est maintenant rapportée à TAILLE_OBJETS : heightMapMaxAltitude = 255.0f lorsque TAILLE_OBJETS = 5.0f
	// TODO : Données ci-dessus à réactiver et à revoir
	if (terrainInfos.terrain.size.Y < 0.0f)
		terrainInfos.terrain.size.Y = heightMapMaxAltitude * -terrainInfos.terrain.size.Y;	// * TAILLE_OBJETS * 0.2f

	// Si le niveau de l'eau est négatif, on rend sa hauteur relative à l'agrandissement en hauteur du terrain
	if (terrainInfos.water.height < 0.0f)
		terrainInfos.water.height = -terrainInfos.water.height * terrainInfos.terrain.size.Y / heightMapMaxAltitude;

	// Dans l'eau en shader, oppose la direction du vent en X car l'eau ne va pas dans le sens attendu
	terrainInfos.water.shader.windDirection.X *= -1.0f;
}
void EcoWorldTerrainManager::loadIrrlichtTerrain(video::IImage* heightMap, bool createTriangleSelectors, scene::IMetaTriangleSelector* cameraTriangleSelector, float heightMapMaxAltitude, CLoadingScreen* loadingScreen, float loadMin, float loadMax)
{
	if (loadingScreen)
		if (loadingScreen->setPercentAndDraw(loadMin))	return;

	// Modifie la heightmap du terrain pour que ses dimensions soient compatibles avec le moteur de terrain utilisé
	io::IReadFile* heightMapFileReader = NULL;	// Le file reader de la height map qui sera fourni au terrain d'Irrlicht
	u8* heightMapData = NULL;
	if (heightMap)
	{
		// Obtient les dimensions actuelles de la heightmap, ainsi que ses dimensions nécessaires
		const core::dimension2du heightMapNeededDim(257, 257);
		const core::dimension2du heightMapDim = heightMap->getDimension();

		// Redimensionne la nouvelle heightmap vers sa nouvelle taille si nécessaire
		if (heightMapDim != heightMapNeededDim)
			heightMap = resizeImage(heightMap, heightMapNeededDim);	// Crée la nouvelle image redimensionnée
		else	// Si on ne recrée pas de nouvelle height map, on grab celle-ci car sinon elle sera détruite juste après le chargement du terrain avec heightMap->drop()
			heightMap->grab();

		// Crée le nouvel emplacement mémoire pour la height map
		const u32 heightDataSize = heightMapDim.getArea() * 3 + 512;		// 24 bits par pixel (dû à l'enregistrement de CImageWriterBMP) ; 512 octets supplémentaires pour le header BMP (une petite de marge supplémentaire prise ici)
		heightMapData = new u8[heightDataSize];
		if (!heightMapData)
			return;

		// Enregistre la height map actuelle dans la mémoire sous forme de fichier BMP en utilisant un write file mémoire
		io::IFileSystem* const fileSystem = game->fileSystem;
		io::IWriteFile* const writeFile = fileSystem->createMemoryWriteFile(	// Voir CImageWriterBMP::writeImage (source d'Irrlicht) : les fichiers sont toujours enregistrés en mode 24 bits/pixel
			heightMapData, heightDataSize, "tmp_IrrlichtTerrain_HM.bmp");
		game->driver->writeImageToFile(heightMap, writeFile);
		writeFile->drop();

		// Crée le file reader de la height map pour le terrain d'après les informations de la height map en mémoire
		heightMapFileReader = game->fileSystem->createMemoryReadFile(
			heightMapData, heightDataSize, "tmp_IrrlichtTerrain_HM.bmp");

		// Libère la height map
		heightMap->drop();
	}
	else	// On n'a pas de pointeur sur la height map, on abandonne
		return;
	heightMap = NULL;	// La height map n'a plus d'utilité à partir d'ici : on efface son pointeur pour plus de sécurité

	// Calcule l'agrandissement et la position du terrain (fait avant sa création pour optimiser son temps de chargement)
	core::vector3df scale(1.0f), position(0.0f);
	{
		// Calcule la bounding box qu'aura le terrain juste après sa création si aucune transformation ne lui est appliquée
		const core::aabbox3df currentTerrainBoundingBox(0.0f, 0.0f, 0.0f, 256.0f, heightMapMaxAltitude, 256.0f);

		// Agrandis le terrain pour qu'il ait la taille désirée
		// Scale	Size
		// 1.0f <->	currentSize
		//   x  <->	terrainSize
		const float sizeY = currentTerrainBoundingBox.MaxEdge.Y - currentTerrainBoundingBox.MinEdge.Y;
		const core::vector3df currentSize(
			// Taille actuelle en X (non protégée)
			currentTerrainBoundingBox.MaxEdge.X - currentTerrainBoundingBox.MinEdge.X,
			// Taille actuelle en Y (protégée : non nulle)
			sizeY != 0.0f ? sizeY : 1.0f, // Evite les divisions par 0 si le dénivelé actuel du terrain est nul
			// Taille actuelle en Z (non protégée)
			currentTerrainBoundingBox.MaxEdge.Z - currentTerrainBoundingBox.MinEdge.Z);
		scale = terrainInfos.terrain.size / currentSize;

		// Place le terrain à la position désirée et lui ajoute la position du centre de ce terrain en X et en Z
		position = currentTerrainBoundingBox.getCenter();
		position.set(terrainInfos.terrain.position.X + position.X * -scale.X,
			terrainInfos.terrain.position.Y,
			terrainInfos.terrain.position.Z + position.Z * -scale.Z);
	}

	if (loadingScreen)
		if (loadingScreen->setPercentAndDraw((loadMax - loadMin) * 0.2f + loadMin))
		{
			// Libère la mémoire des données de la nouvelle height map
			if (heightMapData)	delete[] heightMapData;

			// Libère le fichier de lecture de la height map
			heightMapFileReader->drop();
			return;
		}

	// Crée le terrain (ce moteur nécessite une taille de terrain de 257 !)
	// On tourne aussi le terrain de 180° en Y pour que le terrain résultant soit dans le même sens que la height map de départ
	irrlichtTerrainSceneNode = game->sceneManager->addTerrainSceneNode(
		heightMapFileReader, 0, -1,
		position, core::vector3df(0.0f, 180.0f, 0.0f), scale,
		terrainInfos.terrain.irrlicht.vertexColor, terrainInfos.terrain.irrlicht.maxLOD,
		terrainInfos.terrain.irrlicht.patchSize, terrainInfos.terrain.irrlicht.smoothFactor);

	// Libère la mémoire des données de la nouvelle height map
	if (heightMapData)	{ delete[] heightMapData;	heightMapData = NULL; }

	// Libère le fichier de lecture de la height map
	heightMapFileReader->drop();	heightMapFileReader = NULL;

	if (loadingScreen)
		if (loadingScreen->setPercentAndDraw((loadMax - loadMin) * 0.6f + loadMin))	return;

	if (irrlichtTerrainSceneNode)
	{
		// Charge la première texture du terrain
		irrlichtTerrainSceneNode->setMaterialTexture(0, loadTerrainTexture(terrainInfos.terrain.irrlicht.colorTexture));

		// Crée les détails si le fichier de détails existe
		if (game->fileSystem->existFile(terrainInfos.terrain.irrlicht.detailTexture))
		{
			irrlichtTerrainSceneNode->setMaterialTexture(1, loadTerrainTexture(terrainInfos.terrain.irrlicht.detailTexture));
			irrlichtTerrainSceneNode->setMaterialType(video::EMT_DETAIL_MAP);
		}
		irrlichtTerrainSceneNode->scaleTexture(terrainInfos.terrain.irrlicht.colorTextureScale, terrainInfos.terrain.irrlicht.detailTextureScale);

		// Active le brouillard pour le terrain
		irrlichtTerrainSceneNode->setMaterialFlag(video::EMF_FOG_ENABLE, true);

		if (loadingScreen)
			if (loadingScreen->setPercentAndDraw((loadMax - loadMin) * 0.8f + loadMin))	return;

		if (createTriangleSelectors)
		{
			// Crée le sélécteur de triangles pour le terrain et l'ajoute au triangle selector de la caméra
			scene::ITriangleSelector* terrainSelector = game->sceneManager->createTerrainTriangleSelector(irrlichtTerrainSceneNode, 0);
			irrlichtTerrainSceneNode->setTriangleSelector(terrainSelector);

#ifdef USE_COLLISIONS
			if (cameraTriangleSelector)
				cameraTriangleSelector->addTriangleSelector(terrainSelector);
#endif

			if (terrainTriangleSelector)
				terrainTriangleSelector->addTriangleSelector(terrainSelector);

			terrainSelector->drop();
		}
	}

	if (loadingScreen)
		loadingScreen->setPercentAndDraw(loadMax);
}
void EcoWorldTerrainManager::loadCoplandTerrain(video::IImage* heightMap, bool createTriangleSelectors, scene::IMetaTriangleSelector* cameraTriangleSelector, float heightMapMaxAltitude, CLoadingScreen* loadingScreen, float loadMin, float loadMax)
{
	if (loadingScreen)
		if (loadingScreen->setPercentAndDraw(loadMin))	return;

	// Modifie la heightmap du terrain pour que ses dimensions soient compatibles avec le moteur de terrain utilisé
	if (heightMap)
	{
		// Obtient les dimensions actuelles de la heightmap, ainsi que ses dimensions nécessaires
		core::array<int> validSizes(6);
		validSizes.push_back(65);
		validSizes.push_back(129);
		validSizes.push_back(257);
		validSizes.push_back(513);
		validSizes.push_back(769);		// La taille 769x769 est aussi supportée
		validSizes.push_back(1025);
		validSizes.push_back(2049);
		terrainInfos.terrain.copland.heightMapResolution = getNearestValue(terrainInfos.terrain.copland.heightMapResolution, validSizes);
		const core::dimension2du heightMapNeededDim(terrainInfos.terrain.copland.heightMapResolution, terrainInfos.terrain.copland.heightMapResolution);
		const core::dimension2du heightMapDim = heightMap->getDimension();

		// Redimensionne la nouvelle heightmap vers sa nouvelle taille si nécessaire
		if (heightMapDim != heightMapNeededDim)
			heightMap = resizeImage(heightMap, heightMapNeededDim);	// Crée la nouvelle image redimensionnée
		else	// Si on ne recrée pas de nouvelle height map, on grab celle-ci car sinon elle sera détruite juste après le chargement du terrain avec heightMap->drop()
			heightMap->grab();
	}
	else	// On n'a pas de pointeur sur la height map, on abandonne
		return;

	// Calcule l'agrandissement et la position du terrain (fait avant sa création pour optimiser son temps de chargement)
	core::vector3df scale(1.0f), position(0.0f);
	{
		// Calcule la bounding box qu'aura le terrain juste après sa création si aucune transformation ne lui est appliquée
		const core::dimension2du& heightMapDim = heightMap->getDimension();
		const core::aabbox3df currentTerrainBoundingBox(0.0f, 0.0f, 0.0f, (float)heightMapDim.Width, heightMapMaxAltitude, (float)heightMapDim.Height);

		// Agrandis le terrain pour qu'il ait la taille désirée
		// Scale	Size
		// 1.0f <->	currentSize
		//   x  <->	terrainSize
		const float sizeY = currentTerrainBoundingBox.MaxEdge.Y - currentTerrainBoundingBox.MinEdge.Y;
		const core::vector3df currentSize(
			// Taille actuelle en X (non protégée)
			currentTerrainBoundingBox.MaxEdge.X - currentTerrainBoundingBox.MinEdge.X,
			// Taille actuelle en Y (protégée : non nulle)
			sizeY != 0.0f ? sizeY : 1.0f, // Evite les divisions par 0 si le dénivelé actuel du terrain est nul
			// Taille actuelle en Z (non protégée)
			currentTerrainBoundingBox.MaxEdge.Z - currentTerrainBoundingBox.MinEdge.Z);
		scale = terrainInfos.terrain.size / currentSize;

		// Place le terrain à la position désirée et lui ajoute la position du centre de ce terrain en X et en Z
		position = currentTerrainBoundingBox.getCenter();
		position.set(terrainInfos.terrain.position.X + position.X * -scale.X,
			terrainInfos.terrain.position.Y,
			terrainInfos.terrain.position.Z + position.Z * -scale.Z);
	}

	if (loadingScreen)
		if (loadingScreen->setPercentAndDraw((loadMax - loadMin) * 0.1f + loadMin))
		{
			// Supprime la height map
			heightMap->drop();	heightMap = NULL;
			return;
		}

	// Crée le terrain Copland (ce moteur nécessite dorénavant une taille de terrain de 2^n+1)
	coplandTerrainSceneNode = new CTerrain_Copland(heightMap, terrainInfos.terrain.copland.quality,
		terrainInfos.terrain.copland.textureScale, position, scale, game->sceneManager->getRootSceneNode(), game->sceneManager, -1);

	// Libère le terrain
	if (coplandTerrainSceneNode)
		coplandTerrainSceneNode->drop();

	// Supprime enfin la height map
	heightMap->drop();	heightMap = NULL;

	if (loadingScreen)
		if (loadingScreen->setPercentAndDraw((loadMax - loadMin) * 0.6f + loadMin))	return;

	if (coplandTerrainSceneNode)
	{
		// Modifie la distance maximale de rendu du terrain
		coplandTerrainSceneNode->setRenderDistance(terrainInfos.terrain.copland.maxRenderDistance);

		// Détermine quel mode de texturage utiliser
		if (terrainInfos.terrain.copland.textSplat.enabled && gameConfig.terrainsShadersEnabled)
		{
			// Terrain avec shaders de splatting
			coplandTerrainSceneNode->ActivateSplattingTextures(terrainInfos.terrain.copland.textSplat.height);
			coplandTerrainSceneNode->SetTextureSplat(0, loadTerrainTexture(terrainInfos.terrain.copland.textSplat.texture0));
			coplandTerrainSceneNode->SetTextureSplat(1, loadTerrainTexture(terrainInfos.terrain.copland.textSplat.texture1));
			coplandTerrainSceneNode->SetTextureSplat(2, loadTerrainTexture(terrainInfos.terrain.copland.textSplat.texture2));
			coplandTerrainSceneNode->SetTextureSplat(3, loadTerrainTexture(terrainInfos.terrain.copland.textSplat.texture3));
		}
		else
		{
			// Terrain avec deux textures sans shader
			coplandTerrainSceneNode->setColorTexture(loadTerrainTexture(terrainInfos.terrain.copland.textNorm.colorTexture));
			coplandTerrainSceneNode->setDetailTexture(loadTerrainTexture(terrainInfos.terrain.copland.textNorm.detailTexture));
		}

		// Active le brouillard pour le terrain (nécessaire avec ou sans le shader)
		coplandTerrainSceneNode->setMaterialFlag(video::EMF_FOG_ENABLE, true);

		if (loadingScreen)
			if (loadingScreen->setPercentAndDraw((loadMax - loadMin) * 0.8f + loadMin))	return;

		// Crée le triangle selector du terrain
		if (createTriangleSelectors)
		{
			// Crée le sélécteur de triangles pour le terrain et l'ajoute au triangle selector de la caméra
			coplandTerrainSceneNode->createTriangleSelectors();
			scene::ITriangleSelector* terrainSelector = coplandTerrainSceneNode->getTriangleSelector();

#ifdef USE_COLLISIONS
			if (cameraTriangleSelector)
				cameraTriangleSelector->addTriangleSelector(terrainSelector);
#endif

			if (terrainTriangleSelector)
				terrainTriangleSelector->addTriangleSelector(terrainSelector);
		}
	}

	if (loadingScreen)
		loadingScreen->setPercentAndDraw(loadMax);
}
void EcoWorldTerrainManager::loadTMykeTerrain(video::IImage* heightMap, bool createTriangleSelectors, scene::IMetaTriangleSelector* cameraTriangleSelector, float heightMapMaxAltitude, CLoadingScreen* loadingScreen, float loadMin, float loadMax)
{
	if (loadingScreen)
		if (loadingScreen->setPercentAndDraw(loadMin))	return;

	// Modifie la heightmap du terrain pour que ses dimensions soient compatibles avec le moteur de terrain utilisé
	if (heightMap)
	{
		// Obtient les dimensions actuelles de la heightmap, ainsi que ses dimensions nécessaires
		core::array<int> validSizes(9);
		validSizes.push_back(65);		// Ce moteur peut supporter des tailles encore plus petites ! (mais ces dernières ne seront pas autorisées ici)
		validSizes.push_back(129);
		validSizes.push_back(257);
		validSizes.push_back(513);
		validSizes.push_back(1025);
		validSizes.push_back(2049);
		validSizes.push_back(4097);
		validSizes.push_back(8193);		// Ce moteur peut supporter des tailles encore plus grandes ! (mais ces dernières ne seront pas autorisées ici)
		terrainInfos.terrain.tmyke.heightMapResolution = getNearestValue(terrainInfos.terrain.tmyke.heightMapResolution, validSizes);
		const core::dimension2du heightMapNeededDim(terrainInfos.terrain.tmyke.heightMapResolution, terrainInfos.terrain.tmyke.heightMapResolution);
		const core::dimension2du heightMapDim = heightMap->getDimension();

		// Redimensionne la nouvelle heightmap vers sa nouvelle taille si nécessaire
		if (heightMapDim != heightMapNeededDim)
			heightMap = resizeImage(heightMap, heightMapNeededDim);	// Crée la nouvelle image redimensionnée
		else	// Si on ne recrée pas de nouvelle height map, on grab celle-ci car sinon elle sera détruite juste après le chargement du terrain avec heightMap->drop()
			heightMap->grab();
	}
	else	// On n'a pas de pointeur sur la height map, on abandonne
		return;

	// Calcule l'agrandissement et la position du terrain (ces données seront appliquées après la création du terrain, il n'y a donc pas d'optimisation ici, mais une cohérence par rapport à l'ordre de création des autres terrains)
	core::vector3df scale(1.0f), position(0.0f);
	{
		// Agrandis le terrain pour qu'il ait la taille désirée en Z (dans les paramêtres du terrain, la taille en X et en Z se règle uniformément)
		scale.set(1.0f, 1.0f, terrainInfos.terrain.size.Z / terrainInfos.terrain.size.X);

		// Place le terrain à la position désirée et lui ajoute la position du centre de ce terrain en X et en Z
		position.set(terrainInfos.terrain.position.X + terrainInfos.terrain.size.X * -0.5f,
			terrainInfos.terrain.position.Y,
			terrainInfos.terrain.position.Z + terrainInfos.terrain.size.Z * -0.5f);
	}

	if (loadingScreen)
		if (loadingScreen->setPercentAndDraw((loadMax - loadMin) * 0.1f + loadMin))
		{
			// Supprime la height map
			heightMap->drop();	heightMap = NULL;
			return;
		}

	// Initialise le terrain TMyke (ce moteur nécessite dorénavant une taille de terrain de 2^n+1)
	tmykeTerrainSceneNode = new CTerrainNode(game->sceneManager->getRootSceneNode(), game->sceneManager, -1);

	if (tmykeTerrainSceneNode)
	{
		// Libère le terrain
		tmykeTerrainSceneNode->drop();

		{
			// Force terrainInfos.terrain.tmyke.numQuads et terrainInfos.terrain.tmyke.numFaces à être des puissances de 2 (entre 2 et 64)
			core::array<int> validValues(7);
			validValues.push_back(2);
			validValues.push_back(4);
			validValues.push_back(8);
			validValues.push_back(16);
			validValues.push_back(32);
			validValues.push_back(64);
			terrainInfos.terrain.tmyke.numQuads = getNearestValue(terrainInfos.terrain.tmyke.numQuads, validValues);
			terrainInfos.terrain.tmyke.numFaces = getNearestValue(terrainInfos.terrain.tmyke.numFaces, validValues);
		}

		// Indique les nombres de quads formant le terrain
		/*
			terrain->SetValues:
				8 = nombre de quads par coté format l'ensemble du terrain (soit 64 quad au total)
				16 = nombre par coté de faces par quad, soit 256 face au total par quad
				2048 = taille globale du terrain
		*/
		tmykeTerrainSceneNode->SetValues(terrainInfos.terrain.tmyke.numQuads, terrainInfos.terrain.tmyke.numFaces, terrainInfos.terrain.size.X);

		// Crée le terrain
		/*
			terrain->Construct("media/Height113.bmp", 1.15f);
				construction du terrain à proprement parlé, aavec donc 
				le fichier 'Height113.bmp' comme réfrence pour le relief, la valuer '1.15f' représentant
				l'echelle de hauteur par rapport à ce fichier
		*/
		tmykeTerrainSceneNode->Construct(heightMap, terrainInfos.terrain.size.Y / (heightMapMaxAltitude == 0.0f ? 1.0f : heightMapMaxAltitude));

		// Supprime enfin la height map
		heightMap->drop();	heightMap = NULL;

		if (loadingScreen)
			if (loadingScreen->setPercentAndDraw((loadMax - loadMin) * 0.6f + loadMin))	return;

		// Indique l'agrandissement et la position du terrain tel qu'ils ont été calculés juste avant
		tmykeTerrainSceneNode->setScale(scale);
		tmykeTerrainSceneNode->setPosition(position);

		// Terrain avec deux textures
		/*
			terrain->setMaterialTexture( 0 , smgr->getVideoDriver()->getTexture("media/sol113.jpg") );
			terrain->setMaterialTexture( 1 , smgr->getVideoDriver()->getTexture("media/detail3.bmp") );
									les deux textures de base servant à habiller notre terrain, la première texture est la
									texture globale, la seconde la texture de detail, uniquement au niveau de chaque quad.
		*/
		tmykeTerrainSceneNode->setMaterialType(video::EMT_DETAIL_MAP);
		tmykeTerrainSceneNode->setMaterialTexture(0, loadTerrainTexture(terrainInfos.terrain.tmyke.colorTexture));
		tmykeTerrainSceneNode->setMaterialTexture(1, loadTerrainTexture(terrainInfos.terrain.tmyke.detailTexture));

		// Active le brouillard pour le terrain
		tmykeTerrainSceneNode->setMaterialFlag(video::EMF_FOG_ENABLE, true);

		if (loadingScreen)
			if (loadingScreen->setPercentAndDraw((loadMax - loadMin) * 0.8f + loadMin))	return;

		// Crée le triangle selector du terrain
		if (createTriangleSelectors)
		{
			// Crée le sélécteur de triangles pour le terrain et l'ajoute au triangle selector de la caméra
			tmykeTerrainSceneNode->createTriangleSelectors();
			scene::ITriangleSelector* terrainSelector = tmykeTerrainSceneNode->getTriangleSelector();

#ifdef USE_COLLISIONS
			if (cameraTriangleSelector)
				cameraTriangleSelector->addTriangleSelector(terrainSelector);
#endif

			if (terrainTriangleSelector)
				terrainTriangleSelector->addTriangleSelector(terrainSelector);
		}
	}

	if (loadingScreen)
		loadingScreen->setPercentAndDraw(loadMax);
}
void EcoWorldTerrainManager::loadN3xtDTerrain(video::IImage* heightMap, bool createTriangleSelectors, scene::IMetaTriangleSelector* cameraTriangleSelector, float heightMapMaxAltitude, CLoadingScreen* loadingScreen, float loadMin, float loadMax)
{
	if (loadingScreen)
		if (loadingScreen->setPercentAndDraw(loadMin))	return;

	// Modifie la heightmap du terrain pour que ses dimensions soient compatibles avec le moteur de terrain utilisé
	if (heightMap)
	{
		// Obtient les dimensions actuelles de la heightmap, ainsi que ses dimensions nécessaires
		core::array<int> validSizes(6);
		validSizes.push_back(65);
		validSizes.push_back(129);
		validSizes.push_back(257);
		validSizes.push_back(513);
		validSizes.push_back(769);		// La taille 769x769 est aussi supportée
		validSizes.push_back(1025);
		validSizes.push_back(2049);
		terrainInfos.terrain.n3xtd.heightMapResolution = getNearestValue(terrainInfos.terrain.n3xtd.heightMapResolution, validSizes);
		const core::dimension2du heightMapNeededDim(terrainInfos.terrain.n3xtd.heightMapResolution, terrainInfos.terrain.n3xtd.heightMapResolution);
		const core::dimension2du heightMapDim = heightMap->getDimension();

		// Redimensionne la nouvelle heightmap vers sa nouvelle taille si nécessaire
		if (heightMapDim != heightMapNeededDim)
			heightMap = resizeImage(heightMap, heightMapNeededDim);	// Crée la nouvelle image redimensionnée
		else	// Si on ne recrée pas de nouvelle height map, on grab celle-ci car sinon elle sera détruite juste après le chargement du terrain avec heightMap->drop()
			heightMap->grab();
	}
	else	// On n'a pas de pointeur sur la height map, on abandonne
		return;

	// Calcule l'agrandissement et la position du terrain (fait avant sa création pour optimiser son temps de chargement)
	core::vector3df scale(1.0f), position(0.0f);
	{
		// Calcule la bounding box qu'aura le terrain juste après sa création si aucune transformation ne lui est appliquée
		const core::dimension2du& heightMapDim = heightMap->getDimension();
		const core::aabbox3df currentTerrainBoundingBox(0.0f, 0.0f, 0.0f, (float)heightMapDim.Width, heightMapMaxAltitude, (float)heightMapDim.Height);

		// Agrandis le terrain pour qu'il ait la taille désirée
		// Scale	Size
		// 1.0f <->	currentSize
		//   x  <->	terrainSize
		const float sizeY = currentTerrainBoundingBox.MaxEdge.Y - currentTerrainBoundingBox.MinEdge.Y;
		const core::vector3df currentSize(
			// Taille actuelle en X (non protégée)
			currentTerrainBoundingBox.MaxEdge.X - currentTerrainBoundingBox.MinEdge.X,
			// Taille actuelle en Y (protégée : non nulle)
			sizeY != 0.0f ? sizeY : 1.0f, // Evite les divisions par 0 si le dénivelé actuel du terrain est nul
			// Taille actuelle en Z (non protégée)
			currentTerrainBoundingBox.MaxEdge.Z - currentTerrainBoundingBox.MinEdge.Z);
		scale = terrainInfos.terrain.size / currentSize;

		// Place le terrain à la position désirée et lui ajoute la position du centre de ce terrain en X et en Z
		position = currentTerrainBoundingBox.getCenter();
		position.set(terrainInfos.terrain.position.X + position.X * -scale.X,
			terrainInfos.terrain.position.Y,
			terrainInfos.terrain.position.Z + position.Z * -scale.Z);
	}

	if (loadingScreen)
		if (loadingScreen->setPercentAndDraw((loadMax - loadMin) * 0.1f + loadMin))
		{
			// Supprime la height map
			heightMap->drop();	heightMap = NULL;
			return;
		}

	// Initialise le terrain N3xtD (ce moteur nécessite dorénavant une taille de terrain de 2^n ou de 2^n+1)
	n3xtdTerrainSceneNode = new CTerrainSceneNode_N3xtD(heightMap,
		terrainInfos.terrain.n3xtd.quality, position, scale, terrainInfos.terrain.n3xtd.textureScale, terrainInfos.terrain.n3xtd.smooth,
		game->sceneManager->getRootSceneNode(), game->sceneManager, -1);

	// Libère le terrain
	if (n3xtdTerrainSceneNode)
		n3xtdTerrainSceneNode->drop();

	// Supprime enfin la height map
	heightMap->drop();	heightMap = NULL;

	if (loadingScreen)
		if (loadingScreen->setPercentAndDraw((loadMax - loadMin) * 0.6f + loadMin))	return;

	if (n3xtdTerrainSceneNode)
	{
		// Détermine quel mode de texturage utiliser
		if (terrainInfos.terrain.n3xtd.textSplat.enabled && gameConfig.terrainsShadersEnabled)
		{
			// Terrain avec shaders de splatting + une light map si désiré
			n3xtdTerrainSceneNode->ActivateSplattingTextures();
			n3xtdTerrainSceneNode->SetTextureTerrain(0, loadTerrainTexture(terrainInfos.terrain.n3xtd.textSplat.texture0));
			n3xtdTerrainSceneNode->SetTextureTerrain(1, loadTerrainTexture(terrainInfos.terrain.n3xtd.textSplat.texture1));
			n3xtdTerrainSceneNode->SetTextureTerrain(2, loadTerrainTexture(terrainInfos.terrain.n3xtd.textSplat.texture2));
			n3xtdTerrainSceneNode->SetTextureTerrain(3, loadTerrainTexture(terrainInfos.terrain.n3xtd.textSplat.texture3));
		}
		else
		{
			// Terrain avec deux textures sans shaders
			n3xtdTerrainSceneNode->setColorTexture(loadTerrainTexture(terrainInfos.terrain.n3xtd.textNorm.colorTexture));
			n3xtdTerrainSceneNode->setDetailTexture(loadTerrainTexture(terrainInfos.terrain.n3xtd.textNorm.detailTexture));
		}

		// Active le brouillard pour le terrain
		n3xtdTerrainSceneNode->setMaterialFlag(video::EMF_FOG_ENABLE, true);

		if (loadingScreen)
			if (loadingScreen->setPercentAndDraw((loadMax - loadMin) * 0.8f + loadMin))	return;

		// Crée le triangle selector du terrain
		if (createTriangleSelectors)
		{
			// Crée le sélécteur de triangles pour le terrain et l'ajoute au triangle selector de la caméra
			n3xtdTerrainSceneNode->createTriangleSelectors();
			scene::ITriangleSelector* terrainSelector = n3xtdTerrainSceneNode->getTriangleSelector();

#ifdef USE_COLLISIONS
			if (cameraTriangleSelector)
				cameraTriangleSelector->addTriangleSelector(terrainSelector);
#endif

			if (terrainTriangleSelector)
				terrainTriangleSelector->addTriangleSelector(terrainSelector);
		}
	}

	if (loadingScreen)
		loadingScreen->setPercentAndDraw(loadMax);
}
void EcoWorldTerrainManager::createTerrainWater(
#ifdef USE_SPARK
	SparkManager& sparkManager,
#endif
	bool createTriangleSelectors, scene::IMetaTriangleSelector* cameraTriangleSelector)
{
	// Détruit les deux nodes de l'eau s'ils existent
	if (animatedWaterSceneNode)
	{
		animatedWaterSceneNode->remove();
		animatedWaterSceneNode = NULL;
	}
	if (shaderWaterSceneNode)
	{
		shaderWaterSceneNode->remove();
		shaderWaterSceneNode = NULL;
	}

	// Vérifie que ce terrain nécessite de l'eau, sinon on ne créée que les triangle selector autour des limites du terrain
	if (terrainInfos.water.visible)
	{
		// Crée l'eau :

		// Permet de donner à l'eau la même surface que le terrain
		if (gameConfig.waterShaderEnabled)
		{
			// Eau personnalisée avec shaders (animée ou plane)
			shaderWaterSceneNode = new RealisticWaterSceneNode(
#ifdef USE_SPARK
				sparkManager,
#endif
				terrainInfos.terrain.size.X, terrainInfos.terrain.size.Z,
				loadTerrainTexture(terrainInfos.water.shader.bumpTexture),
				gameConfig.animatedWater, terrainInfos.water.animated.subdivisions,
				terrainInfos.water.animated.waveHeight, terrainInfos.water.animated.waveSpeed, terrainInfos.water.animated.waveLenght,
				gameConfig.waterShaderRenderTargetSize, game->sceneManager->getRootSceneNode());
			shaderWaterSceneNode->drop();

			// Modifie les paramêtres de l'eau
			shaderWaterSceneNode->setWindForce(terrainInfos.water.shader.windForce);
			shaderWaterSceneNode->setWindDirection(terrainInfos.water.shader.windDirection);
			shaderWaterSceneNode->setWaterColor(terrainInfos.water.shader.waterColor);
			shaderWaterSceneNode->setColorBlendFactor(terrainInfos.water.shader.colorBlendFactor);
		}
		else
		{
			if (!gameConfig.animatedWater)
			{
				// Eau plane non animée, sans shaders
				scene::IAnimatedMesh* waterMesh = game->sceneManager->addHillPlaneMesh("waterMeshPlane",
					core::dimension2df(terrainInfos.terrain.size.X, terrainInfos.terrain.size.Z),
					core::dimension2du(1, 1), 0, 0.0f, core::dimension2df(0.0f, 0.0f),
					terrainInfos.water.animated.texturesRepeatCount);

				animatedWaterSceneNode = game->sceneManager->addMeshSceneNode(waterMesh);
				animatedWaterSceneNode->setMaterialType(video::EMT_REFLECTION_2_LAYER);
				animatedWaterSceneNode->setMaterialTexture(0, loadTerrainTexture(terrainInfos.water.animated.reflectiveTexture));
				animatedWaterSceneNode->setMaterialTexture(1, loadTerrainTexture(terrainInfos.water.animated.nonReflectiveTexture));

				// Supprime la mesh du cache du scene manager pour que la prochaine mesh d'eau (avec ce nom) qu'on ait besoin soit recréée, au cas où sa taille serait différente
				game->sceneManager->getMeshCache()->removeMesh(waterMesh);
			}
			else
			{
				// Eau d'Irrlicht animée et non plane, sans shaders
				scene::IAnimatedMesh* waterMesh = game->sceneManager->addHillPlaneMesh("waterMeshAnimated",
					core::dimension2df(terrainInfos.terrain.size.X / (float)terrainInfos.water.animated.subdivisions.Width,
						terrainInfos.terrain.size.Z / (float)terrainInfos.water.animated.subdivisions.Height),
					terrainInfos.water.animated.subdivisions, 0, 0.0f, core::dimension2df(0.0f, 0.0f), terrainInfos.water.animated.texturesRepeatCount);

				animatedWaterSceneNode = game->sceneManager->addWaterSurfaceSceneNode(waterMesh,
					terrainInfos.water.animated.waveHeight, terrainInfos.water.animated.waveSpeed, terrainInfos.water.animated.waveLenght);
				animatedWaterSceneNode->setMaterialType(video::EMT_REFLECTION_2_LAYER);
				animatedWaterSceneNode->setMaterialTexture(0, loadTerrainTexture(terrainInfos.water.animated.reflectiveTexture));
				animatedWaterSceneNode->setMaterialTexture(1, loadTerrainTexture(terrainInfos.water.animated.nonReflectiveTexture));

				// Supprime la mesh du cache du scene manager pour que la prochaine mesh d'eau (avec ce nom) qu'on ait besoin soit recréée, au cas où sa taille serait différente
				game->sceneManager->getMeshCache()->removeMesh(waterMesh);
			}
		}

		scene::ISceneNode* const waterSceneNode = (gameConfig.waterShaderEnabled ? shaderWaterSceneNode : animatedWaterSceneNode);
		if (waterSceneNode)
		{
			// Règle la position de l'eau
			waterSceneNode->setPosition(core::vector3df(0.0f, terrainInfos.water.height, 0.0f));

			// Active le brouillard
			waterSceneNode->setMaterialFlag(video::EMF_FOG_ENABLE, true);
		}

#ifdef USE_COLLISIONS
		// Ce node ne sert qu'à simuler un triangle selector plat au niveau de l'eau : on ne le crée pas si on n'a pas besoin des triangle selector ou si l'eau n'est pas visible
		if (createTriangleSelectors && terrainInfos.water.visible)
		{
			// Crée un node plat invisible avec la taille du monde
			scene::IAnimatedMeshSceneNode* const waterSelectorNode = game->sceneManager->addAnimatedMeshSceneNode(game->sceneManager->addHillPlaneMesh("LimitWater",
				core::dimension2df(TAILLE_CARTE * TAILLE_OBJETS, TAILLE_CARTE * TAILLE_OBJETS),
				dimension2du(1, 1)), 0, -1, core::vector3df(0.0f, terrainInfos.water.height, 0.0f));
			waterSelectorNode->setVisible(false);

			// Ajoute son triangle selector au triangle selector de la caméra et du terrain
			scene::ITriangleSelector* const waterTriangleSelector = game->sceneManager->createTriangleSelector(waterSelectorNode);

			if (cameraTriangleSelector)
				cameraTriangleSelector->addTriangleSelector(waterTriangleSelector);
			if (terrainTriangleSelector)
				terrainTriangleSelector->addTriangleSelector(waterTriangleSelector);

			waterTriangleSelector->drop();
		}
	}

	// Crée des triangle selector plats verticaux autour du terrain pour éviter que la caméra FPS n'en sorte
	if (cameraTriangleSelector)
	{
#define MIN_Y			0.0f								// Le point le plus bas des terrains
#define TAILLE_PLANS	TAILLE_CARTE * TAILLE_OBJETS * 1.5f	// Au minimum : TAILLE_CARTE * TAILLE_OBJETS pour couvrir toute la largeur du terrain

		// 1er côté
		scene::IAnimatedMeshSceneNode* tmpNode = game->sceneManager->addAnimatedMeshSceneNode(game->sceneManager->addHillPlaneMesh("LimitX1",
				core::dimension2df(TAILLE_PLANS, TAILLE_PLANS),
				dimension2du(1, 1)), 0, -1,
				core::vector3df(0.0f, TAILLE_PLANS * 0.5f + MIN_Y, -TERRAIN_LIMITS),
				core::vector3df(90.0f, 0.0f, 0.0f));
		tmpNode->setVisible(false);
		scene::ITriangleSelector* tmpSelector = game->sceneManager->createTriangleSelector(tmpNode);
		cameraTriangleSelector->addTriangleSelector(tmpSelector);
		tmpSelector->drop();

		// 2e côté
		tmpNode = game->sceneManager->addAnimatedMeshSceneNode(game->sceneManager->addHillPlaneMesh("LimitZ1",
				core::dimension2df(TAILLE_PLANS, TAILLE_PLANS),
				dimension2du(1, 1)), 0, -1,
				core::vector3df(-TERRAIN_LIMITS, TAILLE_PLANS * 0.5f + MIN_Y, 0.0f),
				core::vector3df(0.0f, 0.0f, -90.0f));
		tmpNode->setVisible(false);
		tmpSelector = game->sceneManager->createTriangleSelector(tmpNode);
		cameraTriangleSelector->addTriangleSelector(tmpSelector);
		tmpSelector->drop();

		// 3e côté
		tmpNode = game->sceneManager->addAnimatedMeshSceneNode(game->sceneManager->addHillPlaneMesh("LimitX2",
				core::dimension2df(TAILLE_PLANS, TAILLE_PLANS),
				dimension2du(1, 1)), 0, -1,
				core::vector3df(0.0f, TAILLE_PLANS * 0.5f + MIN_Y, TERRAIN_LIMITS),
				core::vector3df(-90.0f, 0.0f, 0.0f));
		tmpNode->setVisible(false);
		tmpSelector = game->sceneManager->createTriangleSelector(tmpNode);
		cameraTriangleSelector->addTriangleSelector(tmpSelector);
		tmpSelector->drop();

		// 4e côté
		tmpNode = game->sceneManager->addAnimatedMeshSceneNode(game->sceneManager->addHillPlaneMesh("LimitZ2",
				core::dimension2df(TAILLE_PLANS, TAILLE_PLANS),
				dimension2du(1, 1)), 0, -1,
				core::vector3df(TERRAIN_LIMITS, TAILLE_PLANS * 0.5f + MIN_Y, 0.0f),
				core::vector3df(0.0f, 0.0f, 90.0f));
		tmpNode->setVisible(false);
		tmpSelector = game->sceneManager->createTriangleSelector(tmpNode);
		cameraTriangleSelector->addTriangleSelector(tmpSelector);
		tmpSelector->drop();
#endif
	}
}
void EcoWorldTerrainManager::loadSystemTerrain(E_TERRAIN_ENGINE currentEngine)
{
	// Charge la constructibilité du terrain
	video::IImage* constructionMap = game->driver->createImageFromFile(terrainInfos.gameSystem.constructionMap);
	if (constructionMap)
	{
		// Obtient les dimensions nécessaires de la construction map, ainsi que ses dimensions actuelles
		const core::dimension2du constructionMapNeededDim(TAILLE_CARTE, TAILLE_CARTE);
		const core::dimension2du& constructionMapDim(constructionMap->getDimension());

		// Vérifie que la construction map a bien les dimensions nécessaires
		if (constructionMapDim == constructionMapNeededDim)
		{
			// Si le pixel est blanc, le terrain est constructible ; si il est noir, le terrain n'est pas constructible
			for (u32 x = 0; x < TAILLE_CARTE; ++x)
				for (u32 y = 0; y < TAILLE_CARTE; ++y)
					game->system.carte[x][TAILLE_CARTE - 1 - y].constructible = (constructionMap->getPixel(x, y).getLuminance() >= 128.0f);
		}
		else
		{
			// La construction map n'a pas les dimensions nécessaires : on la redimensionne
			video::IImage* resizedConstructionMap = resizeImage(constructionMap, constructionMapNeededDim);
			if (resizedConstructionMap)
			{
				// Si le pixel est blanc, le terrain est constructible ; si il est noir, le terrain n'est pas constructible
				for (u32 x = 0; x < TAILLE_CARTE; ++x)
					for (u32 y = 0; y < TAILLE_CARTE; ++y)
						game->system.carte[x][TAILLE_CARTE - 1 - y].constructible = (resizedConstructionMap->getPixel(x, y).getLuminance() >= 128.0f);

				resizedConstructionMap->drop();
				resizedConstructionMap = NULL;
			}
		}

		constructionMap->drop();
		constructionMap = NULL;
	}
	else
	{
		// Rend tout le terrain constructible
		for (u32 x = 0; x < TAILLE_CARTE; ++x)
			for (u32 y = 0; y < TAILLE_CARTE; ++y)
				game->system.carte[x][y].constructible = true;
	}

	// Recherche les zones d'eau profondes (indépendant du moteur de terrain utilisé car la fonction "getTerrainHeight" prend en compte le moteur de terrain actuel)
	if ((currentEngine == ETE_IRRLICHT_TERRAIN && irrlichtTerrainSceneNode)
		|| (currentEngine == ETE_COPLAND_TERRAIN && coplandTerrainSceneNode)
		|| (currentEngine == ETE_TMYKE_TERRAIN && tmykeTerrainSceneNode)
		|| (currentEngine == ETE_N3XTD_TERRAIN && n3xtdTerrainSceneNode))
	{
		// L'eau est considérée comme "profonde" lorsque le sol est en-dessous de l'eau d'au moins 10% de la hauteur maximale du terrain ET d'au moins la hauteur de l'hydrolienne, rendue relative de TAILLE_OBJETS
		const float hauteurMax = terrainInfos.water.height - max(0.10f * terrainInfos.terrain.size.Y, 1.5f * TAILLE_OBJETS);
		for (int x = 0; x < TAILLE_CARTE; ++x)
		{
			for (int y = 0; y < TAILLE_CARTE; ++y)
			{
				const core::vector3df pos = EcoWorldSystem::getPositionFromIndex(core::vector2di(x, y));
				const float terrainHeight = getTerrainHeight(pos.X, pos.Z);

				// Vérifie que la hauteur du terrain est bien valide
				if (terrainHeight != -999999.9f)
				{
					game->system.carte[x][y].deepWater = (terrainHeight <= hauteurMax);
				}
				else
				{
					// Si elle est invalide, on suppose que c'est parce qu'on est en dehors des limites du terrain : on rend alors ce terrain non constructible (évite ainsi de pouvoir construire en dehors du terrain 3D)
					game->system.carte[x][y].constructible = false;	// TODO : Système à revérifier : il vaudrait mieux indiquer l'erreur "Hors des limites du terrain" plutôt que "Terrain non constructible"
					game->system.carte[x][y].deepWater = false;
				}
			}
		}
	}
	else
	{
		// Rend tout le terrain sans eau profonde
		for (u32 x = 0; x < TAILLE_CARTE; ++x)
			for (u32 y = 0; y < TAILLE_CARTE; ++y)
				game->system.carte[x][y].deepWater = false;
	}
}
#ifdef USE_IRRKLANG
void EcoWorldTerrainManager::createTerrainSounds()
{
	// On ne peut actuellement créer que des sons pour l'eau
	if (!terrainInfos.water.visible || !ikMgr.irrKlang)
		return;

	// Ajoute des sons à l'eau :

	// Règle le pas et la tolérance pour l'échantillonage du terrain au niveau des côtes et de l'eau
	// (actuellement, ces valeurs semblent bonnes, mais restent ajustables si nécessaire)
#define PAS						20.0f
#define COTES_TOLERANCE			2.0f
#define COTES_NEAREST_SOUND_SQ	100000.0f
#define COTES_SOUND_MIN_DIST	120.0f
#define AMBIANT_NEAREST_POINT	120.0f
#define AMBIANT_FAREST_GROUP_SQ	50000.0f
#define AMBIANT_SOUND_MIN_DIST	250.0f

#if defined(_DEBUG) && 0
#define SHOW_SPHERE_SOUNDS	// Permet d'afficher des sphères aux positions des sons, et avec leur portée minimale pour rayon
#endif

	// Le scène node de l'eau qui sera parent de ces sons
	scene::ISceneNode* const waterSceneNode = (gameConfig.waterShaderEnabled ? shaderWaterSceneNode : animatedWaterSceneNode);

	// Conserve la position des sons déjà créés ou à créer
	core::list<core::vector2df> positionsSonsCotes;
	core::list<core::vector2df> positionsProbablesSonsAmbianceEau;

	// Parcours le terrain
	core::list<core::vector2df>::Iterator it;
	const core::list<core::vector2df>::Iterator END = positionsSonsCotes.end();
	for (float x = -TERRAIN_LIMITS; x <= TERRAIN_LIMITS; x += PAS)
	{
		for (float y = -TERRAIN_LIMITS; y <= TERRAIN_LIMITS; y += PAS)
		{
			// Obtient la hauteur du terrain à cet endroit
			const float terrainHeight = getTerrainHeight(x, y);

			// Cherche les endroits du terrain qui sont en contact avec l'eau (qui ont une hauteur proche de celle de l'eau) et y ajoute un son de côtes
			if (core::equals(terrainHeight, terrainInfos.water.height + 0.5f, COTES_TOLERANCE))
			{
				// Détermine aléatoirement le son qui sera joué
				const int nb = rand() % 6;
				irrklang::ISoundSource* const soundSource = ikMgr.sounds[IrrKlangManager::ESN_water_1 + nb].soundSource;

				// Recherche dans les sons déjà créés si un son n'est pas trop près de la position de ce nouveau son
				bool canCreateSound = true;
				for (it = positionsSonsCotes.begin(); it != END; ++it)
				{
					if (core::vector2df(it->X - x, it->Y - y).getLengthSQ() < COTES_NEAREST_SOUND_SQ)
					{
						canCreateSound = false;
						break;
					}
				}

				if (canCreateSound)
				{
					// Ajoute le son de l'eau contre la plage à cet endroit
					CIrrKlangSceneNode* irrKlangSceneNode = new CIrrKlangSceneNode(ikMgr.irrKlang, waterSceneNode, game->sceneManager);
					irrKlangSceneNode->setPosition(core::vector3df(x, 0.0f, y));
					irrKlangSceneNode->setSoundSource(soundSource);
					irrKlangSceneNode->setRandomMode(1000, 5000);
					irrKlangSceneNode->setMinMaxSoundDistance(COTES_SOUND_MIN_DIST);
					irrKlangSceneNode->drop();

#ifdef SHOW_SPHERE_SOUNDS
					// Ajoute une sphere de débogage pour indiquer la position et la portée minimale du son
					// Sa couleur sera bleue claire (voire violet suivant son nombre aléatoire) et représentera les son des côtes
					scene::IMeshSceneNode* const sphere = game->sceneManager->addSphereSceneNode(COTES_SOUND_MIN_DIST, 8, waterSceneNode, -1, core::vector3df(x, 0.0f, y));
					sphere->getMaterial(0).AmbientColor.set(0, nb * 25, 150, 255);
#endif

					// Ajoute la position actuelle du son à la liste
					positionsSonsCotes.push_back(core::vector2df(x, y));
				}
			}
			// Cherche les endroits du terrain où on ne voit que l'eau (qui ont une hauteur en-dessous de celle de l'eau) et y ajoute un son d'ambiance d'eau
			else if (terrainHeight < terrainInfos.water.height - 8.0f && terrainHeight > -90000.0f) // Les hauteurs inférieures à -90000.0f sont invalides
			{
				bool canAddPos = true;

				// Recherche dans les positions déjà ajoutées si une n'est pas trop près de cette nouvelle position
				for (it = positionsProbablesSonsAmbianceEau.begin(); it != END; ++it)
				{
					if (core::vector2df(it->X - x, it->Y - y).getLengthSQ() < AMBIANT_NEAREST_POINT)
					{
						canAddPos = false;
						break;
					}
				}

				if (canAddPos)
					positionsProbablesSonsAmbianceEau.push_back(core::vector2df(x, y));
			}
		}
	}

	// Parcours les positions probables pour les sons d'ambiance de l'eau et les regroupe
	core::list<core::vector2df*> groupePositions;	// Le groupe actuel des positions
	core::list<core::vector2df*>::Iterator it2;
	const core::list<core::vector2df*>::Iterator END2 = groupePositions.end();
	while (positionsProbablesSonsAmbianceEau.size())
	{
		// Ajoute la première position probable de la liste
		groupePositions.push_back((&(*positionsProbablesSonsAmbianceEau.begin())));

		// Parcours toutes les positions probables et ajoute celles qui sont assez proche des positions déjà incluses dans la liste
		for (it = positionsProbablesSonsAmbianceEau.begin(); it != END; ++it)
		{
			for (it2 = groupePositions.begin(); it2 != END2; ++it2)
			{
				if ((&(*it)) != (*it2)
					&& core::vector2df(it->X - (*it2)->X, it->Y - (*it2)->Y).getLengthSQ() < AMBIANT_FAREST_GROUP_SQ)
				{
					groupePositions.push_back((&(*it)));
					break;
				}
			}
		}

		// Calcule l'isobarycentre de toutes les positions du groupe
		// et supprime de la liste globale toutes les positions englobées par ce nouveau son
		double sommePositionsX = 0.0f; double sommePositionsY = 0.0f;
		const double diviseur = 1 / (double)groupePositions.size();
		for (it2 = groupePositions.begin(); it2 != END2; ++it2)
		{
			sommePositionsX += (*it2)->X;
			sommePositionsY += (*it2)->Y;

			for (it = positionsProbablesSonsAmbianceEau.begin(); it != END; ++it)
			{
				if ((&(*it)) == (*it2))
				{
					positionsProbablesSonsAmbianceEau.erase(it);
					break;
				}
			}
		}
		const core::vector3df position((float)(sommePositionsX * diviseur), 20.0f, (float)(sommePositionsY * diviseur));

		// Crée un nouveau son d'ambiance de l'eau à cette position
		{
			// Ajoute le son d'ambiance de l'eau à cet endroit
			CIrrKlangSceneNode* irrKlangSceneNode = new CIrrKlangSceneNode(
				ikMgr.irrKlang, waterSceneNode, game->sceneManager);
			irrKlangSceneNode->setPosition(position);
			irrKlangSceneNode->setSoundSource(ikMgr.sounds[IrrKlangManager::ESN_ambiant_water].soundSource);
			irrKlangSceneNode->setLoopingStreamMode();
			irrKlangSceneNode->setMinMaxSoundDistance(AMBIANT_SOUND_MIN_DIST);
			irrKlangSceneNode->drop();

#ifdef SHOW_SPHERE_SOUNDS
			// Ajoute une sphere de débogage pour indiquer la position et la portée minimale du son
			// Sa couleur sera bleue foncée et représentera les son d'ambiance de l'eau
			scene::IMeshSceneNode* sphere = game->sceneManager->addSphereSceneNode(AMBIANT_SOUND_MIN_DIST, 8, waterSceneNode, -1, position);
			sphere->getMaterial(0).AmbientColor.set(0, 20, 70, 150);
#endif
		}

		// Vide la liste actuelle des positions
		groupePositions.clear();
	}
}
#endif
