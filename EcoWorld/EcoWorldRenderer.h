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



// Classe de Callback pour mettre � jour les variables du shader du SkyDome
class SkyDomeShaderCallBack : public video::IShaderConstantSetCallBack
{
public:
	SkyDomeShaderCallBack()	{ }

	virtual void OnSetConstants(video::IMaterialRendererServices* services, int userData);
};

// Classe de Callback pour mettre � jour les variables du shader de NormalMapping
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

	// Remet le renderer � z�ro
	void reset();

	// Initialise le scene manager et pr�charge les textures et les batiments
	// Si mainMenuLoading = true, seules les ressources n�c�ssaires au menu principal seront charg�es
	void initWorld(bool mainMenuLoading, CLoadingScreen* loadingScreen = NULL, float loadMin = 0.0f, float loadMax = 100.0f);

	// Charge un nouveau terrain et d�truit l'ancien en faisant appel au terrain manager (le syst�me devra avoir �t� remis � z�ro avant l'appel de cette fonction) :
	// Si updateSystemTerrain est � true, le terrain du syst�me de jeu sera mis � jour, sinon il sera conserv� tel quel
	void loadNewTerrain(const io::path& adresse, bool createSounds, bool createTriangleSelectors, bool updateSystemTerrain = true,
		CLoadingScreen* loadingScreen = NULL, float loadMin = 0.0f, float loadMax = 100.0f);

	// Charge un batiment (son model 3D, son scene node ainsi que son triangle selector)
	scene::ISceneNode* loadBatiment(BatimentID batimentID, scene::ISceneNode* parent = NULL, bool forLoadingOnly = false, bool isTmpBatiment = false);

	// Charge le fichier 3D (sous forme de scene node) montrant qu'un b�timent est en cours de construction
	scene::ISceneNode* loadConstructingBatiment(BatimentID batimentID, scene::ISceneNode* parent = NULL, bool forLoadingOnly = false);

	// Charge le fichier 3D (sous forme de scene node) de la dalle en b�ton en-dessous des b�timents pour �viter de les voir "voler" lorsque le terrain est en pente
	// denivele :	Le d�nivel� maximal du terrain sur la surface de ce b�timent, pour conna�tre la hauteur minimale de la dalle de b�ton
	scene::ISceneNode* loadConcreteBatiment(float denivele, BatimentID batimentID, scene::ISceneNode* parent = NULL, bool forLoadingOnly = false, bool isTmpBatiment = false);

	// Cr�e le texte indiquant le nom du b�timent
	scene::ITextSceneNode* loadTextBatiment(BatimentID batimentID, scene::ISceneNode* parent = NULL);

#ifdef USE_SPARK
	// Charge les positions de la fum�e pour un b�timent
	void loadBatimentSmokePositions(core::list<core::vector3df>& outSmoke1Positions, core::list<core::vector3df>& outSmoke2Positions,
		BatimentID batimentID, const core::vector3df& batScale, float batRotationY);
#endif

	// Ajoute un batiment dans le monde 3D, � la position indiqu�e si fournie, sinon elle sera recalcul�e suivant l'index du b�timent et la hauteur du terrain
	virtual CBatimentSceneNode* addBatiment(Batiment* batiment, const core::vector3df* position = NULL, const float* deniveleTerrain = NULL);

	// Supprime un batiment du monde 3D
	// wantedByUser est utilis� pour indiquer quel son jouer � l'IrrKlangManager : true si la destruction du b�timent a �t� volontairement demand�e par l'utilisateur, false sinon
	void deleteBatiment(CBatimentSceneNode* node, bool wantedByUser);

	// Supprime un batiment du monde 3D
	// wantedByUser est utilis� pour indiquer quel son jouer � l'IrrKlangManager : true si la destruction du b�timent a �t� volontairement demand�e par l'utilisateur, false sinon
	virtual void deleteBatiment(Batiment* batiment, bool wantedByUser) { deleteBatiment(Game::getBatimentNode(batiment->getSceneNode()), wantedByUser); }

	// Actualise le renderer : met � jour le temps actuel du monde
	// initSunPosition :	A appeler juste apr�s le chargement d'une partie sauvegard�e : Initialise la positin du soleil d'apr�s le temps du jeu �coul�
	//						(permet de conserver la position du soleil lors du chargement d'une partie enregistr�e : le soleil a la m�me position qu'il avait lorsque la partie a �t� sauvegard�e)
	void update(bool initSunPosition = false);

	// Enregistre les donn�es sur ce renderer dans un fichier
	//void save(io::IAttributes* out, io::IXMLWriter* writer) const;	// TODO : Si n�cessaire

	// Charge les donn�es sur ce renderer depuis un fichier
	//void load(io::IAttributes* in, io::IXMLReader* reader);			// TODO : Si n�cessaire

protected:
	// Cr�e et initialise les shaders du jeu : SkyDome, NormalMapping
	void initShaders(bool mainMenuLoading);

	SkyDomeShaderCallBack* skyDomeCallBack;
	int skyDomeMaterial;
	NormalMappingShaderCallBack* normalMappingCallBack;
	int normalMappingMaterial;

	// Variables � n'utiliser que pour le skydome !
	// Permettent de g�rer le changement de textures du skydome d'apr�s le Weather Manager
	WeatherID currentWeatherID;
	WeatherID newWeatherID;

	// Ne pas utilser (seulement utilis� lors de la destruction d'un b�timent) :
	// Pointeurs de Game susceptibles de pointer sur des scene nodes invalides, s'ils sont d�truits
	core::array<CBatimentSceneNode**> gameNodesPointers;

#ifdef USE_IRRKLANG
	bool isRainSoundPlaying;	// A n'utiliser que pour le son de la pluie !
#endif

#ifdef USE_SPARK
	SparkManager sparkMgr;
#endif

	// Arbres "Mod�les" utilis�s pour leur pr�visualisation (acc�l�re ainsi leur chargement lorsqu'ils sont pr�visualis�s en grand nombre)
	scene::STreeMesh* treesHighLOD[4];
	scene::STreeMesh* treesMidLOD[4];

	// Moteur de terrain
	EcoWorldTerrainManager terrainMgr;

	// Lumi�res
	scene::ILightSceneNode* sunLight;		// La lumi�re directionnelle du soleil ou de la lune
	scene::ILightSceneNode* shadowLight;	// La lumi�re utilis�e pour les ombres

	// Ciel
	CEcoWorldSkyDomeSceneNode* skydome;

	// Soleil (ou Lune)
	CSunSceneNode* sun;

#ifdef USE_COLLISIONS
	// Le triangle selector pour la cam�ra
	scene::IMetaTriangleSelector* cameraTriangleSelector;
#endif

	// Le triangle selector pour les batiments
	scene::IMetaTriangleSelector* batimentsTriangleSelector;

	// Un tribool servant � indiquer si les nodes affichant les noms des b�timents sont visibles :
	// Si textNodeVisible = true, alors TOUS les nodes affichant les noms des b�timents sont visibles
	// Si textNodeVisible = indeterminate, alors les nodes affichant les noms des b�timents seront visibles ind�pendamments entre les b�timents, suivant la valeur CBatimentSceneNode::textNodeVisible
	// Si textNodeVisible = false, alors AUCUN node affichant les noms des b�timents n'est visible
	tribool showNomsBatiments;

	// Ajoute un texte au-dessus du b�timent pour indiquer son nom
	scene::ITextSceneNode* addNomBatiment(const wchar_t* text, const core::vector3df& position, scene::ISceneNode* parent)
	{
		// Ancien : Utilisation de ITextSceneNode normal :
		//scene::ITextSceneNode* textNode = game->sceneManager->addTextSceneNode(game->gui->getSkin()->getFont(),
		//	text, video::SColor(255, 180, 255, 100), parent, position);

		// Nouveau : Utilisation de CTextSceneNodeCustom :
		// Permet aux noms des b�timents d'�tre toujours au-dessus des b�timents rendus, m�me s'ils sont transparents (s'ils ont de la fum�e par exemple)
		CTextSceneNodeCustom* textNode = new CTextSceneNodeCustom(parent, game->sceneManager, -1, game->gui->getSkin()->getFont(),
			position, text, video::SColor(255, 180, 255, 100));
		textNode->drop();
		textNode->setVisible(showNomsBatiments == true);
		return textNode;
	}

public:
	// V�rifie les pointeurs de Game susceptibles de pointer sur un scene node invalide (sur le point de se d�truire)
	// Utilit� : Un CBatimentSceneNode doit se d�truire, mais s'il le fait directement, les pointeurs de Game pointant sur lui ne seront plus valides,
	//	cette fonction v�rifie donc chacun des pointeurs de Game et r�initialise ceux qui pointent sur le node sp�cifi�
	// Les pointeurs de Game sont g�n�ralement ceux utilis�s pour la s�lection et la destruction de nodes
	void checkGameNodePointersForNode(CBatimentSceneNode* node)
	{
		const u32 gameNodesPointersSize = gameNodesPointers.size();
		for (u32 i = 0; i < gameNodesPointersSize; ++i)
			if ((*gameNodesPointers[i]) == node)
				(*gameNodesPointers[i]) = NULL;
	}

	// Obtient le suffixe ajout� au nom de la texture lors de son chargement suivant sa qualit� d�sir�e et si elle doit avoir une taille en puissance de deux
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
			str.append("_TBQ");	// TBQ : Tr�s Basse Qualit�
			break;
		case GameConfiguration::ETQ_LOW:
			str.append("_BQ");	// BQ : Basse Qualit�
			break;
		case GameConfiguration::ETQ_MEDIUM:
			str.append("_MQ");	// MQ : Moyenne Qualit�
			break;
		case GameConfiguration::ETQ_HIGH:
			str.append("_HQ");	// HQ : Haute Qualit�
			break;

		default:
			LOG_DEBUG("EcoWorldRenderer::getBatimentTextureSuffixe(" << texturesQuality << ") : La qualite des textures demandee n'est pas valide : texturesQuality = " << texturesQuality, ELL_WARNING);

			str.append("_BQ");
			break;
		}

		return str;
	}
	// Obtient le suffixe ajout� au nom de la texture lors de son chargement suivant sa qualit� d�sir�e et si elle doit avoir une taille en puissance de deux
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

	// Obtient la position 3D (en Y) d'un nouveau b�timent d'apr�s le terrain actuel
	// batPos :		La position actuelle de ce b�timent : doit �tre valide en X et en Z, la valeur en Y sera recalcul�e ici. Cet argument est pass� par r�f�rence : ce sera la position de sortie de cette fonction.
	// batimentID :	L'ID du b�timent � placer, n�cessaire pour conna�tre sa taille r�elle
	// rotation :	La rotation en Y du b�timent � placer
	// Retourne une valeur utile pour le CBatimentSceneNode : le d�nivel� maximal trouv� sur la surface du b�timent
	float getNewBatimentPos(core::vector3df& batPos, BatimentID batimentID, float rotation) const
	{
		// Obtient la position du terrain en Y en son milieu
		batPos.Y = terrainMgr.getTerrainHeight(batPos.X, batPos.Z);

		// Obtient la position maximale en hauteur du terrain en Y sur la surface du b�timent (en prenant en compte aussi la hauteur du b�timent � ses coins)
		// (permet d'�viter dans de nombreux cas qu'une partie du b�timent se retrouve sous le terrain)
		float minY = batPos.Y;
		const StaticBatimentInfos& staticBatInfos = StaticBatimentInfos::getInfos(batimentID);
		const bool needConcrete = StaticBatimentInfos::needConcreteUnderBatiment(batimentID);
		if (needConcrete)	// V�rifie que le b�timent � placer n�cessite bien un bloc de b�ton sous celui-ci (ce calcul n'est utilis� que dans ce cas-l�)
		{
			// Calcule la bounding box de ce b�timent en prenant en compte sa taille et sa rotation
			const core::dimension2df& halfSize = staticBatInfos.halfTaille;
			const obbox2df box(core::rectf(-halfSize.Width, -halfSize.Height, halfSize.Width, halfSize.Height), rotation);

			for (int i = 0; i < 4; ++i)
			{
				// Choisis le point � tester
				const core::vector2df& point = (i < 2 ?
					(i == 0 ? box.getA() : box.getB()) :
					(i == 2 ? box.getC() : box.getD()));

				const float currentTerrainHeight = terrainMgr.getTerrainHeight(batPos.X + point.X, batPos.Z + point.Y);
				batPos.Y = max(batPos.Y, currentTerrainHeight);
				minY = min(minY, currentTerrainHeight);
			}
		}

		// Interdit � la position de tomber en-dessous du niveau de l'eau si elle est visible
		if (terrainMgr.getTerrainInfos().water.visible)
			batPos.Y = max(terrainMgr.getTerrainInfos().water.height, batPos.Y);

		// Calcule le d�nivel� maximal sur la surface du b�timent
		if (needConcrete)
			minY = max(batPos.Y - minY, 0.0f);
		else
			minY = 0.0f;	// Aucun d�nivel� renvoy� si le b�timent � placer ne n�cessite pas de bloc de b�ton

		// Ajoute un peu de hauteur au batiment pour �viter les effets d�sagr�ables avec le sol
		batPos.Y += HAUTEUR_AJOUTEE_BATIMENTS;

		// Retourne le d�nivel� maximal trouv� sur la surface du b�timent
		return minY;
	}

	// Obtient la visibilit� des textes au-dessus des b�timents
	tribool getNomsBatimentsVisible() const							{ return showNomsBatiments; }

	// Modifie la visibilit� des textes au-dessus des b�timents
	void setNomsBatimentsVisible(tribool visible)					{ showNomsBatiments = visible; }

	// Retourne le terrain manager
	const EcoWorldTerrainManager& getTerrainManager() const			{ return terrainMgr; }
	EcoWorldTerrainManager& getTerrainManager()						{ return terrainMgr; }

	// Retourne la hauteur d'un certain point du terrain (en passant par le terrain manager : le terrain doit avoir �t� cr�� auparavant)
	virtual float getTerrainHeight(float x, float y)				{ return terrainMgr.getTerrainHeight(x, y); }

#ifdef USE_COLLISIONS
	// Retourne le triangle selector de la cam�ra
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
