#ifndef C_BATIMENT_SCENE_NODE
#define C_BATIMENT_SCENE_NODE

#include "global.h"
#include "Interfaces.h"
#include "Batiment.h"
#include "CMultipleTriangleSelector.h"

using namespace scene;

class EcoWorldRenderer;
class EcoWorldSystem;

#ifdef USE_SPARK
namespace SPK
{
	class Group;
	class Renderer;
};
using namespace SPK;
#endif

#define ESNT_BATIMENT_NODE	MAKE_IRR_ID('b','a','t','n')	// Ce type indique que ce scene node est un CBatimentSceneNode



//#define ANIMATE_TREES_SIZE	// Si défini, une fois atteint leur taille maximale, les arbres seront animés par un agrandissement/réduction linéaire de ce node, ce qui semble faire bouger leurs feuilles
								// Animation peu recommandée du fait qu'elle modifie la taille apparente de l'arbre, et que vue de près, elle n'est que peu esthétique

#define ANIMATE_TREES_ROTATION	// Si défini, les arbres seront animés par une rotation par rapport au sol suivant la direction du vent, ce qui semble les faire bouger suivant différentes intensités du vent



// Scene Node permettant d'afficher des batiments du système dans le monde 3D
class CBatimentSceneNode : public IBatimentSceneNode
{
public:
	// Constructeur et destructeur
	CBatimentSceneNode(Batiment* bat, const core::vector3df& position, float denivele, ISceneNode* parent, int id = -1);
	virtual ~CBatimentSceneNode();

#ifdef USE_SPARK
	virtual void OnRegisterSceneNode();	// La surcharge de cette fonction est seulement nécessaire lorsque des fumées sont à afficher, si SPARK est désactivé, la fonction de base ISceneNode::OnRegisterSceneNode() suffit
#endif
	virtual void OnAnimate(u32 timeMs);
	virtual void render();

	// Enregistre les données du node dans un fichier
	//virtual void save(io::IAttributes* out) const;	// TODO : Si nécessaire

	// Charge les données du node depuis un fichier
	//virtual void load(io::IAttributes* in);			// TODO : Si nécessaire

protected:
	Batiment* batiment;				// Le bâtiment du système auquel ce node est attaché

	ISceneNode* constructingNode;	// Le node de construction du bâtiment
	ISceneNode* concreteNode;		// Le node de béton en-dessous de ce bâtiment, pour éviter de le voir "voler" lorsque le terrain est en pente
	ISceneNode* batimentNode;		// Le node normal du bâtiment
	ITextSceneNode* textNode;		// Le texte affichant le nom du bâtiment au-dessus de lui
#ifdef USE_SPARK
	core::list<core::vector3df> smoke1Positions;	// Les positions où sera rendu le système pour la fumée 1 de SPARK
	core::list<core::vector3df> smoke2Positions;	// Les positions où sera rendu le système pour la fumée 2 de SPARK
#endif

	bool textNodeVisible;			// True si le node de texte est visible (seulement utilisé lorsque EcoWorldRenderer::showNomsBatiments est à indeterminate)

	core::aabbox3df defaultBB;		// Une bounding box par défaut, pour la fonction getBoundingBox()

	CMultipleTriangleSelector* multipleTriangleSelector;	// Le triangle selector permettant de gérer les différents triangle selector possibles de ce node
	IMetaTriangleSelector* metaTriangleSelector;			// Le triangle selector global de ce node contenant le multipleTriangleSelector et le triangle selector du concreteNode

	void animateBatimentFromID(u32 timeMs, u32 elapsedTimeMs);	// Anime le node actuel suivant l'ID du bâtiment représenté



	// ---------- Variables et fonctions spécifique aux animations des bâtiments ---------- //
	u32 lastTimeMs;	// Permet de calculer le temps écoulé depuis le dernier appel à OnAnimate()

	// Arbres :
#ifdef ANIMATE_TREES_SIZE
	bool treeSizeReducing;
#endif
#ifdef ANIMATE_TREES_ROTATION
	// Optimisation :
	// Conserve les derniers paramêtres de la rotation calculée, car elle est identique pour tous les arbres à chaque frame.
	// Un seul calcul de rotation est donc nécessaire par frame pour tous les arbres.
	static u32 treeRotationLastComputeTimeMs;	// Le temps du jeu auquel la rotation en mémoire (treeLastRotation) a été calculée
	static core::vector2df treeLastRotation;	// La dernière rotation calculée
#endif

	// ---------- Fin des variables et fonctions spécifique aux animations des bâtiments ---------- //



public:
	// Accesseurs inline :

	// Retourne le bâtiment associé à ce scene node
	Batiment* getAssociatedBatiment()				{ return batiment; }

	// Retourne le bâtiment associé à ce scene node
	const Batiment* getAssociatedBatiment() const	{ return batiment; }

	// Retourne le scene node actuellement utilisé pour le rendu
	scene::ISceneNode* getCurrentVisibleNode()
	{
		if (batiment)
		{
			if (batiment->isConstructing())
				return constructingNode;
			else
				return batimentNode;
		}

		return NULL;
	}
	// Retourne le scene node actuellement utilisé pour le rendu
	const scene::ISceneNode* getCurrentVisibleNode() const
	{
		if (batiment)
		{
			if (batiment->isConstructing())
				return constructingNode;
			else
				return batimentNode;
		}

		return NULL;
	}

	bool getNomBatimentVisible() const				{ return textNodeVisible; }
	void setNomBatimentVisible(bool visible)		{ textNodeVisible = visible; }

	scene::ISceneNode* getTextNode()				{ return textNode; }

#ifdef USE_SPARK
	virtual void setPosition(const core::vector3df& newpos)
	{
		// Met à jour les positions de la fumée de SPARK avec la position actuelle de ce node
		const core::vector3df translation = newpos - RelativeTranslation;
		if (translation != core::vector3df(0.0f, 0.0f, 0.0f))
		{
			const core::list<core::vector3df>::Iterator END = smoke1Positions.end();
			core::list<core::vector3df>::Iterator it;

			for (it = smoke1Positions.begin(); it != END; ++it)
				(*it) += translation;
			for (it = smoke2Positions.begin(); it != END; ++it)
				(*it) += translation;
		}

		ISceneNode::setPosition(newpos);
	}
#endif

	virtual ITriangleSelector* getTriangleSelector()
	{
		return metaTriangleSelector;
	}
	virtual void setTriangleSelector(ITriangleSelector* selector)	{ return; }	// Fonction désactivée

	virtual const core::aabbox3df& getBoundingBox() const
	{
		const scene::ISceneNode* currentNode = getCurrentVisibleNode();
		if (currentNode)
			return currentNode->getBoundingBox();
		return defaultBB;
	}

	virtual ESCENE_NODE_TYPE getType() const
	{
		return (ESCENE_NODE_TYPE)(ESNT_BATIMENT_NODE);	// Ce type indique que ce scene node est un CBatimentSceneNode
	}
};

#endif
