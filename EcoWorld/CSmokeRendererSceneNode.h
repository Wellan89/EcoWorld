#ifndef DEF_C_SMOKE_RENDERER_SCENE_NODE
#define DEF_C_SMOKE_RENDERER_SCENE_NODE

#include "global.h"

#ifdef USE_SPARK
#include "SparkManager.h"

using namespace scene;

// Scene node g�rant le rendu des fum�es des b�timents du jeu
class CSmokeRendererSceneNode : public ISceneNode
{
public:
	// Constructeur
	CSmokeRendererSceneNode(ISceneNode* parent, ISceneManager* smgr, int id = -1);

	// Ajoute les positions des fum�es pour ce rendu (doit �tre appel� � chaque frame par les b�timents qui en ont besoin)
	void registerSmokePosition(const core::list<core::vector3df>& smoke1Positions, const core::list<core::vector3df>& smoke2Positions);

	virtual void OnAnimate(u32 timeMs);
	virtual void OnRegisterSceneNode();
	virtual void render();

	// Une structure pour contenir la position d'une fum�e � afficher, ainsi que son type
	struct SSmokePosition
	{
		core::vector3df position;	// La position de la fum�e � afficher
		u8 id;						// Le type de la fum�e � afficher (1 : Petite fum�e ; 2 : Grande fum�e)

		// Constructeur
		SSmokePosition(const core::vector3df& smokePosition, u8 smokeId) : position(smokePosition), id(smokeId)	{ }
	};

protected:
	// Les positions de chaque fum�e � afficher � cette frame (r�initialis� � chaque fin de rendu)
	core::list<SSmokePosition> smokePositions;

	core::aabbox3df defaultBB;		// Une bounding box par d�faut, pour la fonction getBoundingBox()

public:
	// Accesseurs inline :

	// Obtient la bounding box de ce node, en r�alit� : une bounding box par d�faut centr�e sur l'origine
	virtual const core::aabbox3df& getBoundingBox() const
	{
		// Retourne une bounding box par d�faut, car ce node n'en a pas r�ellement
		return defaultBB;
	}
};

#endif
#endif
