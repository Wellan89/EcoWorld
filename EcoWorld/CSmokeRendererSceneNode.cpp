#include "global.h"

#ifdef USE_SPARK
#include "CSmokeRendererSceneNode.h"
#include "Game.h"
#include "EcoWorldRenderer.h"

#define MAX_SMOKE_DRAW_DISTANCE_SQ	250000.0f	// La distance maximale au carr� entre la position de la fum�e et la cam�ra � partir de laquelle la fum�e ne sera pas rendue pour ce b�timent (commenter pour d�sactiver)

// Structure utilis�e pour trier les positions des fum�es par rapport � leur distance � la cam�ra
struct SmokeCameraDistance
{
	// La distance de cette fum�e par rapport � la cam�ra
	float distanceFromCamera;

	// La position de la fum�e � afficher li�e � cette structure
	const CSmokeRendererSceneNode::SSmokePosition* position;

	// Constructeur
	SmokeCameraDistance(float distanceFromCam, const CSmokeRendererSceneNode::SSmokePosition* smokePosition)
		: distanceFromCamera(distanceFromCam), position(smokePosition)
	{ }

	// Op�rateur de comparaison, permettant de trier diff�rentes instances de cette structure
	bool operator<(const SmokeCameraDistance& other) const
	{
		return (distanceFromCamera < other.distanceFromCamera);
	}

	// Op�rateur d'affectation
	SmokeCameraDistance& operator=(const SmokeCameraDistance& other)
	{
		distanceFromCamera = other.distanceFromCamera;
		position = other.position;
		return (*this);
	}
};



CSmokeRendererSceneNode::CSmokeRendererSceneNode(ISceneNode* parent, ISceneManager* smgr, int id)
 : ISceneNode(parent, smgr, id)
{
	// D�sactive le culling automatique de ce node
	AutomaticCullingState = EAC_OFF;
}
void CSmokeRendererSceneNode::registerSmokePosition(const core::list<core::vector3df>& smoke1Positions, const core::list<core::vector3df>& smoke2Positions)
{
	// Ajoute les positions des fum�es pour leur rendu :

	// Positions pour la fum�e 1
	const core::list<core::vector3df>::ConstIterator END = smoke1Positions.end();
	core::list<core::vector3df>::ConstIterator it = smoke1Positions.begin();
	for (; it != END; ++it)
		smokePositions.push_back(SSmokePosition((*it), 1));

	// Positions pour la fum�e 2
	for (it = smoke2Positions.begin(); it != END; ++it)
		smokePositions.push_back(SSmokePosition((*it), 2));
}
void CSmokeRendererSceneNode::OnAnimate(u32 timeMs)
{
	// Avant le rendu, on r�initialise les positions des fum�es � afficher, elles doivent �tre enregistr�es � chaque rendu par les b�timents
	smokePositions.clear();

	// Permet � nos enfants de s'animer
	// D�sactiv� : Normalement, aucun node enfant ni aucun animator ne nous est assign�
	//ISceneNode::OnAnimate(timeMs);
}
void CSmokeRendererSceneNode::OnRegisterSceneNode()
{
	// On s'enregistre pour le rendu, m�me si on n'a aucune fum�e � afficher pour l'instant
	// (elles seront ajout�es plus tard par les b�timents, et la v�rification sera faite dans la m�thode CSmokeRendererSceneNode::render())
	if (IsVisible)
		SceneManager->registerNodeForRendering(this, scene::ESNRP_TRANSPARENT);

	// Permet aux nodes enfants visibles de s'enregistrer
	// D�sactiv� : Normalement, aucun node enfant ni aucun animator ne nous est assign�
	//ISceneNode::OnRegisterSceneNode();
}
void CSmokeRendererSceneNode::render()
{
	// Affiche les particules de fum�e de SPARK :
	const u32 smokePositionSize = smokePositions.size();
	if (game->renderer->getSparkManager().isParticleSystemVisible() && smokePositionSize)	// V�rifie que le syst�me de particules de SPARK est bien visible pour ce rendu et que la liste des positions de la fum�e n'est pas vide
	{
		// Trie les fum�es du plus �loign� (par rapport � la cam�ra) au plus proche, pour conserver la coh�rence dans le rendu :
		// Ceci est n�cessaire car les fum�es sont transparentes (les particules les plus proches masquent les plus �loign�es)

		// Obtient la position de la cam�ra
		scene::ISceneNode* const cameraNode = SceneManager->getActiveCamera();
		const core::vector3df cameraPos = cameraNode ? cameraNode->getAbsolutePosition() : core::vector3df(0.0f);

		// Ajoute les diff�rentes distances des positions par rapport � la cam�ra dans un tableau
		core::array<SmokeCameraDistance> sortedSmoke(smokePositionSize);
		const core::list<SSmokePosition>::ConstIterator END = smokePositions.end();
		for (core::list<SSmokePosition>::ConstIterator it = smokePositions.begin(); it != END; ++it)
		{
			const SSmokePosition& smokePos = (*it);
			const float smokeDistanceSQ = cameraPos.getDistanceFromSQ(smokePos.position);

#ifdef MAX_SMOKE_DRAW_DISTANCE_SQ
			// V�rifie aussi que cette fum�e n'est pas trop loin pour �tre affich�e
			if (smokeDistanceSQ < MAX_SMOKE_DRAW_DISTANCE_SQ)
#endif
				sortedSmoke.push_back(SmokeCameraDistance(smokeDistanceSQ, &smokePos));
		}

		// Trie le tableau des diff�rentes positions des fum�es par rapport � leur distance � la cam�ra
		sortedSmoke.set_sorted(false);
		sortedSmoke.sort();



		// Affiche les fum�es :

		// Obtient les syst�mes de particules pour la fum�e de SPARK, et v�rifie qu'ils sont bien valides
		IRRSystem* const smoke1System = game->renderer->getSparkManager().getSmoke1ParticleSystem();
		IRRSystem* const smoke2System = game->renderer->getSparkManager().getSmoke2ParticleSystem();
		if (!smoke1System || !smoke2System)
			return;

		// Affiche les syst�mes de particules pour la fum�e
		smoke1System->setVisible(true);
		smoke2System->setVisible(true);

		// Indique la rotation du syst�me de particules
		//smoke1System->setRotation(AbsoluteTransformation.getRotationDegrees());	// TODO : N�cessaire ???

#ifdef MAX_SMOKE_DRAW_DISTANCE_SQ
		// Remet � jour la taille du tableau des fum�es tri�,
		// car il peut �tre diff�rent de la taille de la liste des fum�es si certaines ont �t� exclues du rendu car elles �taient trop loin de la cam�ra
		const u32 sortedSmokePositionSize = sortedSmoke.size();
		for (u32 i = 0; i < sortedSmokePositionSize; ++i)
#else
		for (u32 i = 0; i < smokePositionSize; ++i)
#endif
		{
			// Choisis le syst�me de particules � afficher, le place � la position de la fum�e, puis l'affiche
			const SSmokePosition* smokePos = sortedSmoke[i].position;
#ifdef _DEBUG
			if (!smokePos)
			{
				LOG_DEBUG("CSmokeRendererSceneNode::render() : La position d'une fumee apres son tri ne peut etre obtenue : smokePos = " << smokePos, ELL_ERROR);
				continue;
			}
#endif
			IRRSystem* const smokeSystem = (smokePos->id == 2 ? smoke2System : smoke1System);
			smokeSystem->setPosition(smokePos->position);
			smokeSystem->updateAbsolutePosition();
			((const IRRSystem* const)(smokeSystem))->render();
		}

		// Masque � nouveau les syst�mes de particules pour la fum�e car ils ne doivent �tre affich�s que par cette fonction, et non pas par Irrlicht
		smoke1System->setVisible(false);
		smoke2System->setVisible(false);
	}
}

#endif
