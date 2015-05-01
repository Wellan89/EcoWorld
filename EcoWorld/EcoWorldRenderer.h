#ifndef DEF_ECO_WORLD_RENDERER
#define DEF_ECO_WORLD_RENDERER

#include <boost/logic/tribool.hpp>	// Pour l'utilisation du type tribool
using namespace boost::logic;

#include "global.h"
#include "Game.h"
#include "Interfaces.h"
#include "GameConfiguration.h"
#include "Batiments.h"
#include "Weathers.h"
#include "EcoWorldTerrainManager.h"
#include "CTreeSceneNode.h"
#include "Batiment.h"
#include "XEffects/EffectHandler.h"
#include "CTextSceneNodeCustom.h"
#ifdef USE_IRRKLANG
#include "IrrKlangManager.h"
#endif
#ifdef USE_SPARK
#include "SparkManager.h"
#endif

class CLoadingScreen;
class CEcoWorldSkyDomeSceneNode;
class CSunSceneNode;
class EcoWorldSystem;



// Classe de Callback pour mettre à jour les variables du shader du SkyDome
class SkyDomeShaderCallBack : public video::IShaderConstantSetCallBack
{
public:
	SkyDomeShaderCallBack()	{ }

	virtual void OnSetConstants(video::IMaterialRendererServices* services, int userData);
};

// Classe de Callback pour mettre à jour les variables du shader de NormalMapping
class NormalMappingShaderCallBack : public video::IShaderConstantSetCallBack
{
protected:
	scene::ISceneManager* smgr;
	const video::SMaterial* material;

public:
	NormalMappingShaderCallBack(scene::ISceneManager* sceneManager) : smgr(sceneManager), material(NULL)	{ }

	virtual void OnSetMaterial(const video::SMaterial& Material)
	{
		material = &Material;
	}

	virtual void OnSetConstants(video::IMaterialRendererServices* services, int userData);
};



class EcoWorldRenderer : public ISystemRenderer
{
public:
	// Constructeur et destructeur
	EcoWorldRenderer(const core::list<CBatimentSceneNode**>& gameSceneNodesPointers);
	~EcoWorldRenderer();

	// Remet le renderer à zéro
	void reset();

	// Initialise le scene manager et précharge les textures et les batiments
	// Si mainMenuLoading = true, seules les ressources nécéssaires au menu principal seront chargées
	void initWorld(bool mainMenuLoading, CLoadingScreen* loadingScreen = NULL, float loadMin = 0.0f, float loadMax = 100.0f);

	// Charge un nouveau terrain et détruit l'ancien en faisant appel au terrain manager (le système devra avoir été remis à zéro avant l'appel de cette fonction) :
	// Si updateSystemTerrain est à true, le terrain du système de jeu sera mis à jour, sinon il sera conservé tel quel
	void loadNewTerrain(const io::path& adresse, bool createSounds, bool createTriangleSelectors, bool updateSystemTerrain = true,
		CLoadingScreen* loadingScreen = NULL, float loadMin = 0.0f, float loadMax = 100.0f);

	// Charge un batiment (son model 3D, son scene node ainsi que son triangle selector)
	scene::ISceneNode* loadBatiment(BatimentID batimentID, scene::ISceneNode* parent = NULL, bool forLoadingOnly = false, bool isTmpBatiment = false);

	// Charge le fichier 3D (sous forme de scene node) montrant qu'un bâtiment est en cours de construction
	scene::ISceneNode* loadConstructingBatiment(BatimentID batimentID, scene::ISceneNode* parent = NULL, bool forLoadingOnly = false);

	// Charge le fichier 3D (sous forme de scene node) de la dalle en béton en-dessous des bâtiments pour éviter de les voir "voler" lorsque le terrain est en pente
	// denivele :	Le dénivelé maximal du terrain sur la surface de ce bâtiment, pour connaître la hauteur minimale de la dalle de béton
	scene::ISceneNode* loadConcreteBatiment(float denivele, BatimentID batimentID, scene::ISceneNode* parent = NULL, bool forLoadingOnly = false, bool isTmpBatiment = false);

	// Crée le texte indiquant le nom du bâtiment
	scene::ITextSceneNode* loadTextBatiment(BatimentID batimentID, scene::ISceneNode* parent = NULL);

#ifdef USE_SPARK
	// Charge les positions de la fumée pour un bâtiment
	void loadBatimentSmokePositions(core::list<core::vector3df>& outSmoke1Positions, core::list<core::vector3df>& outSmoke2Positions,
		BatimentID batimentID, const core::vector3df& batScale, float batRotationY);
#endif

	// Ajoute un batiment dans le monde 3D, à la position indiquée si fournie, sinon elle sera recalculée suivant l'index du bâtiment et la hauteur du terrain
	virtual CBatimentSceneNode* addBatiment(Batiment* batiment, const core::vector3df* position = NULL, const float* deniveleTerrain = NULL);

	// Supprime un batiment du monde 3D
	// wantedByUser est utilisé pour indiquer quel son jouer à l'IrrKlangManager : true si la destruction du bâtiment a été volontairement demandée par l'utilisateur, false sinon
	void deleteBatiment(CBatimentSceneNode* node, bool wantedByUser);

	// Supprime un batiment du monde 3D
	// wantedByUser est utilisé pour indiquer quel son jouer à l'IrrKlangManager : true si la destruction du bâtiment a été volontairement demandée par l'utilisateur, false sinon
	virtual void deleteBatiment(Batiment* batiment, bool wantedByUser) { deleteBatiment(Game::getBatimentNode(batiment->getSceneNode()), wantedByUser); }

	// Actualise le renderer : met à jour le temps actuel du monde
	// initSunPosition :	A appeler juste après le chargement d'une partie sauvegardée : Initialise la positin du soleil d'après le temps du jeu écoulé
	//						(permet de conserver la position du soleil lors du chargement d'une partie enregistrée : le soleil a la même position qu'il avait lorsque la partie a été sauvegardée)
	void update(bool initSunPosition = false);

	// Enregistre les données sur ce renderer dans un fichier
	//void save(io::IAttributes* out, io::IXMLWriter* writer) const;	// TODO : Si nécessaire

	// Charge les données sur ce renderer depuis un fichier
	//void load(io::IAttributes* in, io::IXMLReader* reader);			// TODO : Si nécessaire

protected:
	// Crée et initialise les shaders du jeu : SkyDome, NormalMapping
	void initShaders(bool mainMenuLoading);

	SkyDomeShaderCallBack* skyDomeCallBack;
	int skyDomeMaterial;
	NormalMappingShaderCallBack* normalMappingCallBack;
	int normalMappingMaterial;

	// Variables à n'utiliser que pour le skydome !
	// Permettent de gérer le changement de textures du skydome d'après le Weather Manager
	WeatherID currentWeatherID;
	WeatherID newWeatherID;

	// Ne pas utilser (seulement utilisé lors de la destruction d'un bâtiment) :
	// Pointeurs de Game susceptibles de pointer sur des scene nodes invalides, s'ils sont détruits
	core::array<CBatimentSceneNode**> gameNodesPointers;

#ifdef USE_IRRKLANG
	bool isRainSoundPlaying;	// A n'utiliser que pour le son de la pluie !
#endif

#ifdef USE_SPARK
	SparkManager sparkMgr;
#endif

	// Arbres "Modèles" utilisés pour leur prévisualisation (accélère ainsi leur chargement lorsqu'ils sont prévisualisés en grand nombre)
	scene::STreeMesh* treesHighLOD[4];
	scene::STreeMesh* treesMidLOD[4];

	// Moteur de terrain
	EcoWorldTerrainManager terrainMgr;

	// Lumières
	scene::ILightSceneNode* sunLight;		// La lumière directionnelle du soleil ou de la lune
	scene::ILightSceneNode* shadowLight;	// La lumière utilisée pour les ombres

	// Ciel
	CEcoWorldSkyDomeSceneNode* skydome;

	// Soleil (ou Lune)
	CSunSceneNode* sun;

#ifdef USE_COLLISIONS
	// Le triangle selector pour la caméra
	scene::IMetaTriangleSelector* cameraTriangleSelector;
#endif

	// Le triangle selector pour les batiments
	scene::IMetaTriangleSelector* batimentsTriangleSelector;

	// Un tribool servant à indiquer si les nodes affichant les noms des bâtiments sont visibles :
	// Si textNodeVisible = true, alors TOUS les nodes affichant les noms des bâtiments sont visibles
	// Si textNodeVisible = indeterminate, alors les nodes affichant les noms des bâtiments seront visibles indépendamments entre les bâtiments, suivant la valeur CBatimentSceneNode::textNodeVisible
	// Si textNodeVisible = false, alors AUCUN node affichant les noms des bâtiments n'est visible
	tribool showNomsBatiments;

	// Ajoute un texte au-dessus du bâtiment pour indiquer son nom
	scene::ITextSceneNode* addNomBatiment(const wchar_t* text, const core::vector3df& position, scene::ISceneNode* parent)
	{
		// Ancien : Utilisation de ITextSceneNode normal :
		//scene::ITextSceneNode* textNode = game->sceneManager->addTextSceneNode(game->gui->getSkin()->getFont(),
		//	text, video::SColor(255, 180, 255, 100), parent, position);

		// Nouveau : Utilisation de CTextSceneNodeCustom :
		// Permet aux noms des bâtiments d'être toujours au-dessus des bâtiments rendus, même s'ils sont transparents (s'ils ont de la fumée par exemple)
		CTextSceneNodeCustom* textNode = new CTextSceneNodeCustom(parent, game->sceneManager, -1, game->gui->getSkin()->getFont(),
			position, text, video::SColor(255, 180, 255, 100));
		textNode->drop();
		textNode->setVisible(showNomsBatiments == true);
		return textNode;
	}

public:
	// Vérifie les pointeurs de Game susceptibles de pointer sur un scene node invalide (sur le point de se détruire)
	// Utilité : Un CBatimentSceneNode doit se détruire, mais s'il le fait directement, les pointeurs de Game pointant sur lui ne seront plus valides,
	//	cette fonction vérifie donc chacun des pointeurs de Game et réinitialise ceux qui pointent sur le node spécifié
	// Les pointeurs de Game sont généralement ceux utilisés pour la sélection et la destruction de nodes
	void checkGameNodePointersForNode(CBatimentSceneNode* node)
	{
		const u32 gameNodesPointersSize = gameNodesPointers.size();
		for (u32 i = 0; i < gameNodesPointersSize; ++i)
			if ((*gameNodesPointers[i]) == node)
				(*gameNodesPointers[i]) = NULL;
	}

	// Obtient le suffixe ajouté au nom de la texture lors de son chargement suivant sa qualité désirée et si elle doit avoir une taille en puissance de deux
	static io::path getBatimentTextureSuffixe(bool usePowerOf2Textures, GameConfiguration::E_TEXTURE_QUALITY texturesQuality)
	{
		io::path str;

		if (usePowerOf2Textures)
			str.append("_P2");	// P2 : Power Of 2
		else
			str.append("_OS");	// OS : Original Size

		switch (texturesQuality)
		{
		case GameConfiguration::ETQ_VERY_LOW:
			str.append("_TBQ");	// TBQ : Très Basse Qualité
			break;
		case GameConfiguration::ETQ_LOW:
			str.append("_BQ");	// BQ : Basse Qualité
			break;
		case GameConfiguration::ETQ_MEDIUM:
			str.append("_MQ");	// MQ : Moyenne Qualité
			break;
		case GameConfiguration::ETQ_HIGH:
			str.append("_HQ");	// HQ : Haute Qualité
			break;

		default:
			LOG_DEBUG("EcoWorldRenderer::getBatimentTextureSuffixe(" << texturesQuality << ") : La qualite des textures demandee n'est pas valide : texturesQuality = " << texturesQuality, ELL_WARNING);

			str.append("_BQ");
			break;
		}

		return str;
	}
	// Obtient le suffixe ajouté au nom de la texture lors de son chargement suivant sa qualité désirée et si elle doit avoir une taille en puissance de deux
	static io::path getSkyTextureSuffixe(GameConfiguration::E_TEXTURE_QUALITY texturesQuality)
	{
		io::path str;

		switch (texturesQuality)
		{
		case GameConfiguration::ETQ_VERY_LOW:
			str.append("_VBQ.jpg");
			break;
		case GameConfiguration::ETQ_LOW:
			str.append("_BQ.jpg");
			break;
		case GameConfiguration::ETQ_MEDIUM:
			str.append("_MQ.jpg");
			break;
		case GameConfiguration::ETQ_HIGH:
			str.append("_HQ.jpg");
			break;

		default:
			LOG_DEBUG("EcoWorldRenderer::getSkyTextureSuffixe(" << texturesQuality << ") : La qualite des textures demandee n'est pas valide : texturesQuality = " << texturesQuality, ELL_WARNING);

			str.append("_BQ.jpg");
			break;
		}

		return str;
	}

	// Obtient la position 3D (en Y) d'un nouveau bâtiment d'après le terrain actuel
	// batPos :		La position actuelle de ce bâtiment : doit être valide en X et en Z, la valeur en Y sera recalculée ici. Cet argument est passé par référence : ce sera la position de sortie de cette fonction.
	// batimentID :	L'ID du bâtiment à placer, nécessaire pour connaître sa taille réelle
	// rotation :	La rotation en Y du bâtiment à placer
	// Retourne une valeur utile pour le CBatimentSceneNode : le dénivelé maximal trouvé sur la surface du bâtiment
	float getNewBatimentPos(core::vector3df& batPos, BatimentID batimentID, float rotation) const
	{
		// Obtient la position du terrain en Y en son milieu
		batPos.Y = terrainMgr.getTerrainHeight(batPos.X, batPos.Z);

		// Obtient la position maximale en hauteur du terrain en Y sur la surface du bâtiment (en prenant en compte aussi la hauteur du bâtiment à ses coins)
		// (permet d'éviter dans de nombreux cas qu'une partie du bâtiment se retrouve sous le terrain)
		float minY = batPos.Y;
		const StaticBatimentInfos& staticBatInfos = StaticBatimentInfos::getInfos(batimentID);
		const bool needConcrete = StaticBatimentInfos::needConcreteUnderBatiment(batimentID);
		if (needConcrete)	// Vérifie que le bâtiment à placer nécessite bien un bloc de béton sous celui-ci (ce calcul n'est utilisé que dans ce cas-là)
		{
			// Calcule la bounding box de ce bâtiment en prenant en compte sa taille et sa rotation
			const core::dimension2df& halfSize = staticBatInfos.halfTaille;
			const obbox2df box(core::rectf(-halfSize.Width, -halfSize.Height, halfSize.Width, halfSize.Height), rotation);

			for (int i = 0; i < 4; ++i)
			{
				// Choisis le point à tester
				const core::vector2df& point = (i < 2 ?
					(i == 0 ? box.getA() : box.getB()) :
					(i == 2 ? box.getC() : box.getD()));

				const float currentTerrainHeight = terrainMgr.getTerrainHeight(batPos.X + point.X, batPos.Z + point.Y);
				batPos.Y = max(batPos.Y, currentTerrainHeight);
				minY = min(minY, currentTerrainHeight);
			}
		}

		// Interdit à la position de tomber en-dessous du niveau de l'eau si elle est visible
		if (terrainMgr.getTerrainInfos().water.visible)
			batPos.Y = max(terrainMgr.getTerrainInfos().water.height, batPos.Y);

		// Calcule le dénivelé maximal sur la surface du bâtiment
		if (needConcrete)
			minY = max(batPos.Y - minY, 0.0f);
		else
			minY = 0.0f;	// Aucun dénivelé renvoyé si le bâtiment à placer ne nécessite pas de bloc de béton

		// Ajoute un peu de hauteur au batiment pour éviter les effets désagréables avec le sol
		batPos.Y += HAUTEUR_AJOUTEE_BATIMENTS;

		// Retourne le dénivelé maximal trouvé sur la surface du bâtiment
		return minY;
	}

	// Obtient la visibilité des textes au-dessus des bâtiments
	tribool getNomsBatimentsVisible() const							{ return showNomsBatiments; }

	// Modifie la visibilité des textes au-dessus des bâtiments
	void setNomsBatimentsVisible(tribool visible)					{ showNomsBatiments = visible; }

	// Retourne le terrain manager
	const EcoWorldTerrainManager& getTerrainManager() const			{ return terrainMgr; }
	EcoWorldTerrainManager& getTerrainManager()						{ return terrainMgr; }

	// Retourne la hauteur d'un certain point du terrain (en passant par le terrain manager : le terrain doit avoir été créé auparavant)
	virtual float getTerrainHeight(float x, float y)				{ return terrainMgr.getTerrainHeight(x, y); }

#ifdef USE_COLLISIONS
	// Retourne le triangle selector de la caméra
	scene::IMetaTriangleSelector* getCameraTriangleSelector()		{ return cameraTriangleSelector; }
#endif

	// Retourne le triangle selector des batiments
	scene::IMetaTriangleSelector* getBatimentsTriangleSelector()	{ return batimentsTriangleSelector; }

	// Retourne le triangle selector pour le terrain
	scene::IMetaTriangleSelector* getTerrainTriangleSelector()		{ return terrainMgr.getTerrainTriangleSelector(); }

	// Obtient le scene node du soleil (ou de la lune)
	CSunSceneNode* getSunSceneNode()								{ return sun; }

#ifdef USE_SPARK
	// Retourne le SPARK Manager
	SparkManager& getSparkManager()									{ return sparkMgr; }
#endif
};

#endif
