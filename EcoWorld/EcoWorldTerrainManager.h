#ifndef DEF_ECO_WORLD_TERRAIN_MANAGER
#define DEF_ECO_WORLD_TERRAIN_MANAGER

#include "global.h"
#include "Game.h"
#include "Batiments.h"
#include "GameConfiguration.h"

#include "CTerrain_Copland.h"		// Copland Terrain
#include "CTerrainNode_tmyke.h"		// TMyke Terrain
#include "TerrainNode_n3xtd.h"		// N3xtD Terrain

#ifdef USE_SPARK
class SparkManager;
#endif

using namespace scene;

class EcoWorldRenderer;
class EcoWorldSystem;
class Batiment;
class RealisticWaterSceneNode;
class CLoadingScreen;

class EcoWorldTerrainManager
{
public:
	// Enum indiquant les diff�rents moteurs de terrain utilisables
	enum E_TERRAIN_ENGINE
	{
		ETE_IRRLICHT_TERRAIN,
		ETE_COPLAND_TERRAIN,
		ETE_TMYKE_TERRAIN,
		ETE_N3XTD_TERRAIN
	};

	// Structure contenant les informations sur un terrain
	struct TerrainInfos
	{
		// Global
		core::stringc terrainName;

		// Terrain
		struct Terrain
		{
			core::stringc heightMap;
			E_TERRAIN_ENGINE engine;
			core::vector3df position;
			core::vector3df size;

			// Irrlicht Terrain
			struct IrrlichtTerrain
			{
				core::stringc colorTexture;
				core::stringc detailTexture;
				float colorTextureScale;
				float detailTextureScale;
				video::SColor vertexColor;
				E_TERRAIN_PATCH_SIZE patchSize;
				int maxLOD;
				int smoothFactor;
			} irrlicht;

			// Copland Terrain
			struct CoplandTerrain
			{
				int heightMapResolution;
				CTerrain_Copland::TerrainQuality quality;
				float textureScale;				// Le facteur de r�p�tition de toutes les textures (m�me celles utilis�es pour les shaders), sauf pour la color map du texturage normal qui est toujours plac�e une seule fois
				float maxRenderDistance;

				// Texturage avec shaders
				struct TextureSplatting
				{
					bool enabled;
					float height;
					core::stringc texture0;		// Grass
					core::stringc texture1;		// Rock
					core::stringc texture2;		// Snow
					core::stringc texture3;		// Detail
				} textSplat;

				// Texturage normal
				struct NormalTexturing
				{
					core::stringc colorTexture;
					core::stringc detailTexture;
				} textNorm;
			} copland;

			// TMyke Terrain
			struct TMykeTerrain
			{
				int heightMapResolution;
				int numQuads;
				int numFaces;
				core::stringc colorTexture;		// Texture 0
				core::stringc detailTexture;	// Texture 1
			} tmyke;

			// N3xtD Terrain
			struct N3xtDTerrain
			{
				int heightMapResolution;
				CTerrainSceneNode_N3xtD::TerrainQuality quality;
				float textureScale;				// Le facteur de r�p�tition de toutes les textures (m�me celles utilis�es pour les shaders), sauf pour la color map du texturage normal qui est toujours plac�e une seule fois
				bool smooth;

				// Texturage avec shaders
				struct TextureSplatting
				{
					bool enabled;
					core::stringc texture0;		// Masque (indique les proportions de chaque texture sur le terrain)
					core::stringc texture1;		// Texture correspondant � la couleur Noir
					core::stringc texture2;		// Texture correspondant � la couleur Blanche
					core::stringc texture3;		// Detail
				} textSplat;

				// Texturage normal
				struct NormalTexturing
				{
					core::stringc colorTexture;
					core::stringc detailTexture;
				} textNorm;
			} n3xtd;
		} terrain;

		// Eau
		struct Water
		{
			bool visible;
			float height;

			// Eau avec shaders (si anim�e : utilise aussi les param�tres de AnimatedWater sauf pour les textures)
			struct ShaderWater
			{
				core::stringc bumpTexture;
				float windForce;
				core::vector2df windDirection;
				video::SColorf waterColor;
				float colorBlendFactor;
			} shader;

			// Eau anim�e
			struct AnimatedWater
			{
				core::stringc reflectiveTexture;					// Aussi utilis� pour l'eau non anim�e et sans shaders (Texture 0)
				core::stringc nonReflectiveTexture;					// Aussi utilis� pour l'eau non anim�e et sans shaders (Texture 1)
				core::dimension2df texturesRepeatCount;				// Aussi utilis� pour l'eau non anim�e et sans shaders
				core::dimension2du subdivisions;
				float waveHeight;
				float waveSpeed;
				float waveLenght;
			} animated;
		} water;

		// Syst�me de jeu
		struct GameSystem
		{
			core::stringc constructionMap;
		} gameSystem;

		void reset()
		{
			// Global
			{
				//terrainName = "";
			}

			// Terrain
			{
				terrain.heightMap = "Height Map.bmp";
				terrain.engine = ETE_IRRLICHT_TERRAIN;
				terrain.position.set(-1.0f, -1.0f, -1.0f);
				terrain.size.set(-1.0f, -1.0f, -1.0f);

				// Irrlicht Terrain
				{
					terrain.irrlicht.colorTexture = "Color Map 1.bmp";
					terrain.irrlicht.detailTexture = "Details 1.bmp";
					terrain.irrlicht.colorTextureScale = 1.0f;
					terrain.irrlicht.detailTextureScale = 10.0f;
					terrain.irrlicht.vertexColor.set(255, 255, 255, 255);
					terrain.irrlicht.patchSize = ETPS_9;
					terrain.irrlicht.maxLOD = 3;
					terrain.irrlicht.smoothFactor = 1;
				}

				// Copland Terrain
				{
					terrain.copland.heightMapResolution = 513;
					terrain.copland.quality = CTerrain_Copland::Low;
					terrain.copland.textureScale = 10.0f;
					terrain.copland.maxRenderDistance = 3000.0f;

					// Texturage avec shaders
					terrain.copland.textSplat.enabled = false;
					terrain.copland.textSplat.height = 220.0f;
					terrain.copland.textSplat.texture0 = "Grass 1.bmp";			// Grass
					terrain.copland.textSplat.texture1 = "Rock 1.bmp";			// Rock
					terrain.copland.textSplat.texture2 = "Dirt 1.bmp";			// Snow
					terrain.copland.textSplat.texture3 = "Details 1.bmp";		// Details

					// Texturage normal
					terrain.copland.textNorm.colorTexture = "Color Map 1.bmp";
					terrain.copland.textNorm.detailTexture = "Details 1.bmp";
				}

				// TMyke Terrain
				{
					terrain.tmyke.heightMapResolution = 1025;
					terrain.tmyke.numQuads = 8;
					terrain.tmyke.numFaces = 16;
					terrain.tmyke.colorTexture = "Color Map 1.bmp";
					terrain.tmyke.detailTexture = "Details 1.bmp";
				}

				// N3xtD Terrain
				{
					terrain.n3xtd.heightMapResolution = 513;
					terrain.n3xtd.quality = CTerrainSceneNode_N3xtD::Low;
					terrain.n3xtd.textureScale = 10.0f;
					terrain.n3xtd.smooth = true;

					// Texturage avec shaders
					{
						terrain.n3xtd.textSplat.enabled = false;
						terrain.n3xtd.textSplat.texture0 = "Mask 1.png";	// Masque (indique les proportions de chaque texture sur le terrain)
						terrain.n3xtd.textSplat.texture1 = "Grass 1.bmp";	// Texture correspondant � la couleur Noir
						terrain.n3xtd.textSplat.texture2 = "Dirt 1.bmp";	// Texture correspondant � la couleur Blanche
						terrain.n3xtd.textSplat.texture3 = "Details 1.bmp";	// Detail
					}

					// Texturage normal
					{
						terrain.n3xtd.textNorm.colorTexture = "Color Map 1.bmp";
						terrain.n3xtd.textNorm.detailTexture = "Details 1.bmp";
					}
				}
			}

			// Eau
			{
				water.visible = false;		// Par d�faut : false, m�me si not� true dans les nouveaux terrains (ainsi la valeur par d�faut est vraiment une valeur de compatibilit� par d�faut)
				water.height = -50.0f;

				// Eau avec shaders
				{
					water.shader.bumpTexture = "Water Bump 1.png";
					water.shader.windForce = 20.0f;
					water.shader.windDirection.set(-0.5f, 1.0f);
					water.shader.waterColor.set(0.1f, 0.1f, 0.6f, 1.0f);
					water.shader.colorBlendFactor = 0.2f;
				}

				// Eau sans shaders
				{
					water.animated.reflectiveTexture = "Stones 1.jpg";
					water.animated.nonReflectiveTexture = "Water 1.jpg";
					water.animated.texturesRepeatCount.set(20.0f, 20.0f);
					water.animated.subdivisions.set(50, 50);
					water.animated.waveHeight = 2.0f;
					water.animated.waveSpeed = 300.0f;
					water.animated.waveLenght = 10.0f;
				}
			}

			// Syst�me de jeu
			{
				gameSystem.constructionMap = "Construction Map.bmp";
			}
		}

		TerrainInfos() : terrainName("") { reset(); }
	};

	// Constructeur et destructeur
	EcoWorldTerrainManager();
	~EcoWorldTerrainManager();

	// Remet le terrain manager � z�ro
	void reset();

	// Les informations sur un terrain affich�es lors de sa pr�visualisation
	struct TerrainPreviewInfos
	{
		core::stringw descriptionText;	// Le texte de description du terrain
		video::ITexture* previewImage;	// L'image d'aper�u du terrain

		TerrainPreviewInfos() : descriptionText(L"Aucune description disponible"), previewImage(NULL)	{ }
	};

	// Charge les inforamtions de pr�visualisation d'un terrain d'apr�s son adresse
	TerrainPreviewInfos readPreviewInfos(const io::path& adresse);

	// Charge un nouveau terrain et d�truit l'ancien (le syst�me devra avoir �t� remis � z�ro avant l'appel de cette fonction) :
	// Si updateSystemTerrain est � true, le terrain du syst�me de jeu sera mis � jour, sinon il sera conserv� tel quel
	void loadNewTerrain(
#ifdef USE_SPARK
		SparkManager& sparkManager,
#endif
		const io::path& adresse, bool createSounds, bool createTriangleSelectors, scene::IMetaTriangleSelector* cameraTriangleSelector, bool updateSystemTerrain = true,
		CLoadingScreen* loadingScreen = NULL, float loadMin = 0.0f, float loadMax = 100.0f);

	// TODO : Cr�er les fonctions save et load si n�cessaire

protected:
	// Lit les informations sur un terrain
	// (son archive devra avoir d�j� �t� ajout�e au syst�me de fichiers d'Irrlicht ; ne remplit pas le nom du terrain ; m�me en cas d'�chec, ne r�initialise pas les informations actuelles sur le terrain)
	void readTerrainInfos();

	// Calcule la position et la taille du terrain lorsque ces param�tres sont r�gl�s sur "Automatique" (-1.0f), en utilisant les donn�es du terrain d�j� pr�sentes en m�moire
	void calculateTerrainPosAndSize(float heightMapMaxAltitude);

	// Charge le terrain correspondant
	void loadIrrlichtTerrain(video::IImage* heightMap, bool createTriangleSelectors, scene::IMetaTriangleSelector* cameraTriangleSelector, float heightMapMaxAltitude,
		CLoadingScreen* loadingScreen = NULL, float loadMin = 0.0f, float loadMax = 100.0f);
	void loadCoplandTerrain(video::IImage* heightMap, bool createTriangleSelectors, scene::IMetaTriangleSelector* cameraTriangleSelector, float heightMapMaxAltitude,
		CLoadingScreen* loadingScreen = NULL, float loadMin = 0.0f, float loadMax = 100.0f);
	void loadTMykeTerrain(video::IImage* heightMap, bool createTriangleSelectors, scene::IMetaTriangleSelector* cameraTriangleSelector, float heightMapMaxAltitude,
		CLoadingScreen* loadingScreen = NULL, float loadMin = 0.0f, float loadMax = 100.0f);
	void loadN3xtDTerrain(video::IImage* heightMap, bool createTriangleSelectors, scene::IMetaTriangleSelector* cameraTriangleSelector, float heightMapMaxAltitude,
		CLoadingScreen* loadingScreen = NULL, float loadMin = 0.0f, float loadMax = 100.0f);

	// Cr�e l'eau et le sol invisible, ainsi que les plans de collision sur les c�t�s du terrain (ou les d�truits si ils ne sont plus n�cessaires)
	void createTerrainWater(
#ifdef USE_SPARK
		SparkManager& sparkManager,
#endif
		bool createTriangleSelectors, scene::IMetaTriangleSelector* cameraTriangleSelector);

#ifdef USE_IRRKLANG
	// Cr�e les sons pour le terrain
	void createTerrainSounds();
#endif

	// Charge le terrain syst�me suivant les donn�es actuellement en m�moire dans terrainInfos
	void loadSystemTerrain(E_TERRAIN_ENGINE currentEngine);

	// Les informations sur le terrain actuel
	TerrainInfos terrainInfos;

	// Irrlicht Terrain
	scene::ITerrainSceneNode* irrlichtTerrainSceneNode;

	// Copland Terrain
	CTerrain_Copland* coplandTerrainSceneNode;

	// TMyke Terrain
	CTerrainNode* tmykeTerrainSceneNode;

	// N3xtD Terrain
	CTerrainSceneNode_N3xtD* n3xtdTerrainSceneNode;

	// L'eau avec shaders
	RealisticWaterSceneNode* shaderWaterSceneNode;

	// L'eau anim�e sans shaders
	scene::ISceneNode* animatedWaterSceneNode;

	// Le triangle selector pour le terrain : le triangle selector du terrain + celui du sol invisible
	scene::IMetaTriangleSelector* terrainTriangleSelector;

	// La liste de toutes les textures utilis�es par le terrain actuel, pour pouvoir les lib�rer � la destruction du terrain
	// -> Evite un bug d� aux caches des textures : si le terrain A charge une texture de son archive nomm�e "tex",
	//	puis que ce terrain A est d�truit et qu'un autre terrain B charge une texture du m�me nom ("tex") de son archive,
	//	alors la nouvelle texture sera identique � celle du terrain A, car elle aura �t� gard�e en cache par Irrlicht
	core::list<video::ITexture*> texturesList;

	// Charge une texture pour le terrain :
	video::ITexture* loadTerrainTexture(const io::path& filename)
	{
		video::ITexture* tex = NULL;
		if (filename.size() && filename.find("disabled") == -1)	// V�rifie que la texture n'est pas d�sactiv�e (si elle ne contient pas "disabled")
			tex = game->driver->getTexture(filename);
		if (tex)
			texturesList.push_back(tex);		// Ajoute cette texture � la liste des textures des terrains
		return tex;
	}

	// Manipulation des images :

	// Donne la valeur enti�re valide (validValues) la plus proche de la valeur actuelle (currentValue)
	// Si currentValue est exactement �gal � une des valeurs de la liste validValues, outFindExact sera pass� � true comme valeur de sortie
	int getNearestValue(int currentValue, const core::array<int>& validValues, bool* outFindExact = NULL)
	{
		if (outFindExact)
			(*outFindExact) = false;
		if (!validValues.size())
			return currentValue;

		// Conserve en m�moire l'�cart minimal trouv� ainsi que son index (par d�faut, c'est la premi�re valeur dans la liste des tailles valides)
		int minEcart = abs(validValues[0] - currentValue), minI = 0;
		for (u32 i = 1; i < validValues.size(); ++i)
		{
			// Optimisation pour les longues listes : si la valeur actuelle est �gale � une des valeurs dans la liste, on la retourne directement
			if (currentValue == validValues[i])
			{
				if (outFindExact)
					(*outFindExact) = true;
				return currentValue;
			}
			else
			{
				// Calcul l'�cart actuel et le conserve en m�moire si il est plus petit que le pr�c�dent
				const int currentEcart = abs(validValues[i] - currentValue);
				if (currentEcart < minEcart)
				{
					minEcart = currentEcart;
					minI = i;
				}
			}
		}

		// Retourne enfin l'�cart le plus petit trouv�
		return validValues[minI];
	}
#if 0	// Non utilis�
	// Donne la valeur enti�re valide (validValues) la plus proche de la valeur actuelle (currentValue), en conservant toujours la valeur de retour inf�rieure ou �gale � currentValue
	// Si currentValue est exactement �gal � une des valeurs de la liste validValues, outFindExact sera pass� � true comme valeur de sortie
	int getNearestLowerValue(int currentValue, const core::array<int>& validValues, bool* outFindExact = NULL)
	{
		if (outFindExact)
			(*outFindExact) = false;
		if (!validValues.size())
			return currentValue;

		// Conserve en m�moire l'�cart minimal trouv� ainsi que son index (par d�faut, c'est la premi�re valeur dans la liste des tailles valides)
		int minEcart = currentValue - validValues[0], minI = 0;
		for (u32 i = 1; i < validValues.size(); ++i)
		{
			// Optimisation pour les longues listes : si la valeur actuelle est �gale � une des valeurs dans la liste, on la retourne directement
			if (currentValue == validValues[i])
			{
				if (outFindExact)
					(*outFindExact) = true;
				return currentValue;
			}
			else
			{
				// Calcul l'�cart actuel et le conserve en m�moire si il est plus petit que le pr�c�dent, en restant toujours positif
				const int currentEcart = currentValue - validValues[i];
				if (currentEcart < minEcart && currentEcart > 0)
				{
					minEcart = currentEcart;
					minI = i;
				}
			}
		}

		// Retourne enfin l'�cart le plus petit trouv� s'il est positif
		if (minEcart > 0)
			return validValues[minI];
		else
			return currentValue;
	}
#endif
#if 0	// Non utilis�
	// Donne la valeur enti�re valide (validValues) la plus proche de la valeur actuelle (currentValue), en conservant toujours la valeur de retour sup�rieure ou �gale � currentValue
	// Si currentValue est exactement �gal � une des valeurs de la liste validValues, outFindExact sera pass� � true comme valeur de sortie
	int getNearestHigherValue(int currentValue, const core::array<int>& validValues, bool* outFindExact = NULL)
	{
		if (outFindExact)
			(*outFindExact) = false;
		if (!validValues.size())
			return currentValue;

		// Conserve en m�moire l'�cart minimal trouv� ainsi que son index (par d�faut, c'est la premi�re valeur dans la liste des tailles valides)
		int minEcart = validValues[0] - currentValue, minI = 0;
		for (u32 i = 1; i < validValues.size(); ++i)
		{
			// Optimisation pour les longues listes : si la valeur actuelle est �gale � une des valeurs dans la liste, on la retourne directement
			if (currentValue == validValues[i])
			{
				if (outFindExact)
					(*outFindExact) = true;
				return currentValue;
			}
			else
			{
				// Calcul l'�cart actuel et le conserve en m�moire si il est plus petit que le pr�c�dent, en restant toujours positif
				const int currentEcart = validValues[i] - currentValue;
				if (currentEcart < minEcart && currentEcart > 0)
				{
					minEcart = currentEcart;
					minI = i;
				}
			}
		}

		// Retourne enfin l'�cart le plus petit trouv� s'il est positif
		if (minEcart > 0)
			return validValues[minI];
		else
			return currentValue;
	}
#endif

#if 0	// Non utilis�
	// Tronque une image � la taille sp�cifi�e
	// Attention : le pointeur retourn� devra �tre lib�r� par un appel � image->drop()
	video::IImage* clipImage(video::IImage* sourceImg, const core::dimension2du& newImageSize)
	{
		if (!sourceImg)
			return NULL;
		if (sourceImg->getDimension().Width < newImageSize.Width || sourceImg->getDimension().Height < newImageSize.Height)
			return NULL;

		// Cr�e la nouvelle image aux dimensions sp�cifi�es
		u8* imgData = new u8[newImageSize.getArea() * sourceImg->getBytesPerPixel()];
		video::IImage* destImg = driver->createImageFromData(sourceImg->getColorFormat(), newImageSize, imgData);

		// Copie l'image source dans la nouvelle en tronquant les parties qui ne peuvent �tre copi�es
		sourceImg->copyTo(destImg);

		delete[] imgData;
		return destImg;
	}
#endif
	// Agrandis/R�duit une image pour qu'elle ait les dimensions d�sir�es (en lui appliquant un filtre "Box")
	// Attention : le pointeur retourn� devra �tre lib�r� par un appel � image->drop()
	video::IImage* resizeImage(video::IImage* sourceImg, const core::dimension2du& newImageSize)
	{
		// Cr�e la nouvelle image aux dimensions sp�cifi�es
		u8* imgData = new u8[newImageSize.getArea() * sourceImg->getBytesPerPixel()];
		video::IImage* destImg = game->driver->createImageFromData(sourceImg->getColorFormat(), newImageSize, imgData);

		// Redimensionne l'image source vers la nouvelle image, en lui appliquant un filtre "Box"
		sourceImg->copyToScalingBoxFilter(destImg);

		delete[] imgData;
		return destImg;
	}

#if 0	// Non utilis�
	// Redimensionne une image qui a une largeur/hauteur quelconque en une nouvelle image qui a une largeur/hauteur �gale � 2^n (ou � 2^n + 1 si sp�cifi�)
	// Si addOne est � true, toutes les puissances de deux se verront ajouter +1 � leur valeur (cela n'affectera pas la v�rification minSize <= 2^n <= maxSize qui sera toujours faite avec les puissances de 2 exactes)
	// Si square est � true, la largeur et la hauteur de la nouvelle image seront �gales
	// minSize et maxSize sp�cifient la taille minimale et maximale que peut prendre la nouvelle image (attention toutefois, la taille de l'image n'exc�dera jamais 4096*4096 pixels, de m�me qu'elle ne sera jamais inf�rieure � 1*1 pixel)
	// Attention : le pointeur retourn� devra �tre lib�r� par un appel � image->drop()
	video::IImage* resizeImageToPowerOf2(video::IImage* sourceImg, bool addOne = false, bool forceSquare = false, int minSize = 1, int maxSize = 4096)
	{
		if (!sourceImg)
			return NULL;

		// Conserve les tailles minimales et maximales comprises dans des valeurs coh�rentes
		minSize = core::clamp(minSize, 1, 4096);
		maxSize = core::clamp(maxSize, 1, 4096);
		if (minSize > maxSize)
			core::swap(minSize, maxSize);

		// Cr�e le tableau contenant toutes les tailles valides possibles
		const int nbToAdd = addOne ? 1 : 0;
		core::array<int> validSizes(13);
		if (minSize <= 1)
			validSizes.push_back(1 + nbToAdd);			// 2^ 0
		if (minSize <= 2 && 2 <= maxSize)
			validSizes.push_back(2 + nbToAdd);			// 2^ 1
		if (minSize <= 4 && 4 <= maxSize)
			validSizes.push_back(4 + nbToAdd);			// 2^ 2
		if (minSize <= 8 && 8 <= maxSize)
			validSizes.push_back(8 + nbToAdd);			// 2^ 3
		if (minSize <= 16 && 16 <= maxSize)
			validSizes.push_back(16 + nbToAdd);			// 2^ 4
		if (minSize <= 32 && 32 <= maxSize)
			validSizes.push_back(32 + nbToAdd);			// 2^ 5
		if (minSize <= 64 && 64 <= maxSize)
			validSizes.push_back(64 + nbToAdd);			// 2^ 6
		if (minSize <= 128 && 128 <= maxSize)
			validSizes.push_back(128 + nbToAdd);		// 2^ 7
		if (minSize <= 256 && 256 <= maxSize)
			validSizes.push_back(256 + nbToAdd);		// 2^ 8
		if (minSize <= 512 && 512 <= maxSize)
			validSizes.push_back(512 + nbToAdd);		// 2^ 9
		if (minSize <= 1024 && 1024 <= maxSize)
			validSizes.push_back(1024 + nbToAdd);		// 2^10
		if (minSize <= 2048 && 2048 <= maxSize)
			validSizes.push_back(2048 + nbToAdd);		// 2^11
		if (4096 <= maxSize)
			validSizes.push_back(4096 + nbToAdd);		// 2^12

		// Calcule les nouvelles dimensions de l'image
		core::dimension2du newSize(256, 256);
		if (forceSquare)
		{
			const int size = getNearestValue(min(sourceImg->getDimension().Width, sourceImg->getDimension().Height), validSizes);
			newSize.set(size, size);
		}
		else
			newSize.set(
				getNearestValue(sourceImg->getDimension().Width, validSizes),		// Largeur
				getNearestValue(sourceImg->getDimension().Height, validSizes));		// Hauteur

		// Cr�e la nouvelle image redimensionn�e avec ces nouvelles dimensions
		return resizeImage(sourceImg, newSize);
	}
#endif

#if 0	// Non utilis�
	// Calcule le d�nivel� d'une height map (la diff�rence entre la couleur la plus claire et la couleur la plus fonc�e)
	float calculateHeightMapAltitude(video::IImage* heightMap)
	{
		if (!heightMap)	// Si la height map n'est pas valide, on renvoie une valeur par d�faut
			return 255.0f;

		// Permet de retenir les altitudes minimales/maximales rencontr�es
		float currentAltitude = heightMap->getPixel(0, 0).getLuminance();
		float minAltitude = currentAltitude, maxAltitude = currentAltitude;

		// Parcours ensuite toute l'image pour mettre � jour les altitudes minimales/maximales
		const core::dimension2du& heightMapSize = heightMap->getDimension();
		for (u32 x = 0; x < heightMapSize.Width; ++x)
		{
			for (u32 y = 0; y < heightMapSize.Height; ++y)
			{
				currentAltitude = heightMap->getPixel(x, y).getLuminance();
				minAltitude = min(currentAltitude, minAltitude);
				maxAltitude = max(currentAltitude, maxAltitude);
			}
		}

		// Renvoie enfin le d�nivel� du terrain
		return (maxAltitude - minAltitude);
	}
#endif
	// Calcule la hauteur maximale d'une height map (la valeur de la couleur la plus fonc�e)
	float calculateHeightMapMaxAltitude(video::IImage* heightMap)
	{
		if (!heightMap)	// Si la height map n'est pas valide, on renvoie une valeur par d�faut
			return 255.0f;

		// Permet de retenir l'altitude maximale rencontr�e
		float maxAltitude = heightMap->getPixel(0, 0).getLuminance();

		// Parcours ensuite toute l'image pour mettre � jour l'altitude maximale
		const core::dimension2du& heightMapSize = heightMap->getDimension();
		for (u32 x = 0; x < heightMapSize.Width; ++x)
			for (u32 y = 0; y < heightMapSize.Height; ++y)
				maxAltitude = max(heightMap->getPixel(x, y).getLuminance(), maxAltitude);

		// Renvoie enfin l'altitude maximale du terrain
		return maxAltitude;
	}

public:
	// Retourne la bo�te englobante du terrain, calcul�e d'apr�s les informations actuelles du terrain (d'apr�s sa position et sa taille) (et donc ind�pendante du moteur de terrain utilis�)
	core::aabbox3df getTerrainBoundingBox() const
	{
		return core::aabbox3df(
			terrainInfos.terrain.position.X - terrainInfos.terrain.size.X * 0.5f,
			terrainInfos.terrain.position.Y - terrainInfos.terrain.size.Y * 0.5f,
			terrainInfos.terrain.position.Z - terrainInfos.terrain.size.Z * 0.5f,
			terrainInfos.terrain.position.X + terrainInfos.terrain.size.X * 0.5f,
			terrainInfos.terrain.position.Y + terrainInfos.terrain.size.Y * 0.5f,
			terrainInfos.terrain.position.Z + terrainInfos.terrain.size.Z * 0.5f);
	}

	// Retourne la hauteur d'un point du terrain
	// ATTENTION : Sur les terrains Copland et Paged, les triangle selector doivent �tre cr��s pour que cette fonction puisse fonctionner
	float getTerrainHeight(float x, float y) const
	{
		// V�rifie que x et y sont dans les limites du terrain, sinon on renvoie une valeur par d�faut
		if (x < terrainInfos.terrain.size.X * -0.5f + terrainInfos.terrain.position.X
			|| x > terrainInfos.terrain.size.X * 0.5f + terrainInfos.terrain.position.X
			|| y < terrainInfos.terrain.size.Z * -0.5f + terrainInfos.terrain.position.Z
			|| y > terrainInfos.terrain.size.Z * 0.5f + terrainInfos.terrain.position.Z)
			return -999999.9f;

		switch (terrainInfos.terrain.engine)
		{
		case ETE_IRRLICHT_TERRAIN:	if (irrlichtTerrainSceneNode)	{ return irrlichtTerrainSceneNode->getHeight(x, y); }		break;	// Fonctionne parfaitement
		case ETE_COPLAND_TERRAIN:	if (coplandTerrainSceneNode)	{ return coplandTerrainSceneNode->getHeight(x, y); }		break;	// Fonctionne parfaitement
		case ETE_TMYKE_TERRAIN:		if (tmykeTerrainSceneNode)		{ return tmykeTerrainSceneNode->GetTerrainHeight(x, y); }	break;	// Semble fonctionner
		case ETE_N3XTD_TERRAIN:		if (n3xtdTerrainSceneNode)		{ return n3xtdTerrainSceneNode->getHeight(x, y); }			break;	// Fonctionne parfaitement
		}
		return -999999.9f;
	}

	// Retourne les informations sur le terrain
	const TerrainInfos& getTerrainInfos() const						{ return terrainInfos; }

	// Retourne le triangle selector pour le terrain
	scene::IMetaTriangleSelector* getTerrainTriangleSelector()		{ return terrainTriangleSelector; }

	// Retourne la direction du vent
	const core::vector2df& getWindDirection() const					{ return terrainInfos.water.shader.windDirection; }

	// Retourne le node de l'eau avec shaders
	RealisticWaterSceneNode* getShaderWaterSceneNode()				{ return shaderWaterSceneNode; }
};

#endif
