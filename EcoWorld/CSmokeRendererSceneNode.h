#ifndef DEF_C_SMOKE_RENDERER_SCENE_NODE
#define DEF_C_SMOKE_RENDERER_SCENE_NODE

#include "global.h"

#ifdef USE_SPARK
#include "SparkManager.h"

using namespace scene;

// Scene node gérant le rendu des fumées des bâtiments du jeu
class CSmokeRendererSceneNode : public ISceneNode
{
public:
	// Constructeur
	CSmokeRendererSceneNode(ISceneNode* parent, ISceneManager* smgr, int id = -1);

	// Ajoute les positions des fumées pour ce rendu (doit être appelé à chaque frame par les bâtiments qui en ont besoin)
	void registerSmokePosition(const core::list<core::vector3df>& smoke1Positions, const core::list<core::vector3df>& smoke2Positions);

	virtual void OnAnimate(u32 timeMs);
	virtual void OnRegisterSceneNode();
	virtual void render();

	// Une structure pour contenir la position d'une fumée à afficher, ainsi que son type
	struct SSmokePosition
	{
		core::vector3df position;	// La position de la fumée à afficher
		u8 id;						// Le type de la fumée à afficher (1 : Petite fumée ; 2 : Grande fumée)

		// Constructeur
		SSmokePosition(const core::vector3df& smokePosition, u8 smokeId) : position(smokePosition), id(smokeId)	{ }
	};

protected:
	// Les positions de chaque fumée à afficher à cette frame (réinitialisé à chaque fin de rendu)
	core::list<SSmokePosition> smokePositions;

	core::aabbox3df defaultBB;		// Une bounding box par défaut, pour la fonction getBoundingBox()

public:
	// Accesseurs inline :

	// Obtient la bounding box de ce node, en réalité : une bounding box par défaut centrée sur l'origine
	virtual const core::aabbox3df& getBoundingBox() const
	{
		// Retourne une bounding box par défaut, car ce node n'en a pas réellement
		return defaultBB;
	}
};

#endif
#endif
