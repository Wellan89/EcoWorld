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



//#define ANIMATE_TREES_SIZE	// Si d�fini, une fois atteint leur taille maximale, les arbres seront anim�s par un agrandissement/r�duction lin�aire de ce node, ce qui semble faire bouger leurs feuilles
								// Animation peu recommand�e du fait qu'elle modifie la taille apparente de l'arbre, et que vue de pr�s, elle n'est que peu esth�tique

#define ANIMATE_TREES_ROTATION	// Si d�fini, les arbres seront anim�s par une rotation par rapport au sol suivant la direction du vent, ce qui semble les faire bouger suivant diff�rentes intensit�s du vent



// Scene Node permettant d'afficher des batiments du syst�me dans le monde 3D
class CBatimentSceneNode : public IBatimentSceneNode
{
public:
	// Constructeur et destructeur
	CBatimentSceneNode(Batiment* bat, const core::vector3df& position, float denivele, ISceneNode* parent, int id = -1);
	virtual ~CBatimentSceneNode();

#ifdef USE_SPARK
	virtual void OnRegisterSceneNode();	// La surcharge de cette fonction est seulement n�cessaire lorsque des fum�es sont � afficher, si SPARK est d�sactiv�, la fonction de base ISceneNode::OnRegisterSceneNode() suffit
#endif
	virtual void OnAnimate(u32 timeMs);
	virtual void render();

	// Enregistre les donn�es du node dans un fichier
	//virtual void save(io::IAttributes* out) const;	// TODO : Si n�cessaire

	// Charge les donn�es du node depuis un fichier
	//virtual void load(io::IAttributes* in);			// TODO : Si n�cessaire

protected:
	Batiment* batiment;				// Le b�timent du syst�me auquel ce node est attach�

	ISceneNode* constructingNode;	// Le node de construction du b�timent
	ISceneNode* concreteNode;		// Le node de b�ton en-dessous de ce b�timent, pour �viter de le voir "voler" lorsque le terrain est en pente
	ISceneNode* batimentNode;		// Le node normal du b�timent
	ITextSceneNode* textNode;		// Le texte affichant le nom du b�timent au-dessus de lui
#ifdef USE_SPARK
	core::list<core::vector3df> smoke1Positions;	// Les positions o� sera rendu le syst�me pour la fum�e 1 de SPARK
	core::list<core::vector3df> smoke2Positions;	// Les positions o� sera rendu le syst�me pour la fum�e 2 de SPARK
#endif

	bool textNodeVisible;			// True si le node de texte est visible (seulement utilis� lorsque EcoWorldRenderer::showNomsBatiments est � indeterminate)

	core::aabbox3df defaultBB;		// Une bounding box par d�faut, pour la fonction getBoundingBox()

	CMultipleTriangleSelector* multipleTriangleSelector;	// Le triangle selector permettant de g�rer les diff�rents triangle selector possibles de ce node
	IMetaTriangleSelector* metaTriangleSelector;			// Le triangle selector global de ce node contenant le multipleTriangleSelector et le triangle selector du concreteNode

	void animateBatimentFromID(u32 timeMs, u32 elapsedTimeMs);	// Anime le node actuel suivant l'ID du b�timent repr�sent�



	// ---------- Variables et fonctions sp�cifique aux animations des b�timents ---------- //
	u32 lastTimeMs;	// Permet de calculer le temps �coul� depuis le dernier appel � OnAnimate()

	// Arbres :
#ifdef ANIMATE_TREES_SIZE
	bool treeSizeReducing;
#endif
#ifdef ANIMATE_TREES_ROTATION
	// Optimisation :
	// Conserve les derniers param�tres de la rotation calcul�e, car elle est identique pour tous les arbres � chaque frame.
	// Un seul calcul de rotation est donc n�cessaire par frame pour tous les arbres.
	static u32 treeRotationLastComputeTimeMs;	// Le temps du jeu auquel la rotation en m�moire (treeLastRotation) a �t� calcul�e
	static core::vector2df treeLastRotation;	// La derni�re rotation calcul�e
#endif

	// ---------- Fin des variables et fonctions sp�cifique aux animations des b�timents ---------- //



public:
	// Accesseurs inline :

	// Retourne le b�timent associ� � ce scene node
	Batiment* getAssociatedBatiment()				{ return batiment; }

	// Retourne le b�timent associ� � ce scene node
	const Batiment* getAssociatedBatiment() const	{ return batiment; }

	// Retourne le scene node actuellement utilis� pour le rendu
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
	// Retourne le scene node actuellement utilis� pour le rendu
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
		// Met � jour les positions de la fum�e de SPARK avec la position actuelle de ce node
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
	virtual void setTriangleSelector(ITriangleSelector* selector)	{ return; }	// Fonction d�sactiv�e

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
