#include "global.h"

#ifdef USE_SPARK
#include "CSmokeRendererSceneNode.h"
#include "Game.h"
#include "EcoWorldRenderer.h"

#define MAX_SMOKE_DRAW_DISTANCE_SQ	250000.0f	// La distance maximale au carré entre la position de la fumée et la caméra à partir de laquelle la fumée ne sera pas rendue pour ce bâtiment (commenter pour désactiver)

// Structure utilisée pour trier les positions des fumées par rapport à leur distance à la caméra
struct SmokeCameraDistance
{
	// La distance de cette fumée par rapport à la caméra
	float distanceFromCamera;

	// La position de la fumée à afficher liée à cette structure
	const CSmokeRendererSceneNode::SSmokePosition* position;

	// Constructeur
	SmokeCameraDistance(float distanceFromCam, const CSmokeRendererSceneNode::SSmokePosition* smokePosition)
		: distanceFromCamera(distanceFromCam), position(smokePosition)
	{ }

	// Opérateur de comparaison, permettant de trier différentes instances de cette structure
	bool operator<(const SmokeCameraDistance& other) const
	{
		return (distanceFromCamera < other.distanceFromCamera);
	}

	// Opérateur d'affectation
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
	// Désactive le culling automatique de ce node
	AutomaticCullingState = EAC_OFF;
}
void CSmokeRendererSceneNode::registerSmokePosition(const core::list<core::vector3df>& smoke1Positions, const core::list<core::vector3df>& smoke2Positions)
{
	// Ajoute les positions des fumées pour leur rendu :

	// Positions pour la fumée 1
	const core::list<core::vector3df>::ConstIterator END = smoke1Positions.end();
	core::list<core::vector3df>::ConstIterator it = smoke1Positions.begin();
	for (; it != END; ++it)
		smokePositions.push_back(SSmokePosition((*it), 1));

	// Positions pour la fumée 2
	for (it = smoke2Positions.begin(); it != END; ++it)
		smokePositions.push_back(SSmokePosition((*it), 2));
}
void CSmokeRendererSceneNode::OnAnimate(u32 timeMs)
{
	// Avant le rendu, on réinitialise les positions des fumées à afficher, elles doivent être enregistrées à chaque rendu par les bâtiments
	smokePositions.clear();

	// Permet à nos enfants de s'animer
	// Désactivé : Normalement, aucun node enfant ni aucun animator ne nous est assigné
	//ISceneNode::OnAnimate(timeMs);
}
void CSmokeRendererSceneNode::OnRegisterSceneNode()
{
	// On s'enregistre pour le rendu, même si on n'a aucune fumée à afficher pour l'instant
	// (elles seront ajoutées plus tard par les bâtiments, et la vérification sera faite dans la méthode CSmokeRendererSceneNode::render())
	if (IsVisible)
		SceneManager->registerNodeForRendering(this, scene::ESNRP_TRANSPARENT);

	// Permet aux nodes enfants visibles de s'enregistrer
	// Désactivé : Normalement, aucun node enfant ni aucun animator ne nous est assigné
	//ISceneNode::OnRegisterSceneNode();
}
void CSmokeRendererSceneNode::render()
{
	// Affiche les particules de fumée de SPARK :
	const u32 smokePositionSize = smokePositions.size();
	if (game->renderer->getSparkManager().isParticleSystemVisible() && smokePositionSize)	// Vérifie que le système de particules de SPARK est bien visible pour ce rendu et que la liste des positions de la fumée n'est pas vide
	{
		// Trie les fumées du plus éloigné (par rapport à la caméra) au plus proche, pour conserver la cohérence dans le rendu :
		// Ceci est nécessaire car les fumées sont transparentes (les particules les plus proches masquent les plus éloignées)

		// Obtient la position de la caméra
		scene::ISceneNode* const cameraNode = SceneManager->getActiveCamera();
		const core::vector3df cameraPos = cameraNode ? cameraNode->getAbsolutePosition() : core::vector3df(0.0f);

		// Ajoute les différentes distances des positions par rapport à la caméra dans un tableau
		core::array<SmokeCameraDistance> sortedSmoke(smokePositionSize);
		const core::list<SSmokePosition>::ConstIterator END = smokePositions.end();
		for (core::list<SSmokePosition>::ConstIterator it = smokePositions.begin(); it != END; ++it)
		{
			const SSmokePosition& smokePos = (*it);
			const float smokeDistanceSQ = cameraPos.getDistanceFromSQ(smokePos.position);

#ifdef MAX_SMOKE_DRAW_DISTANCE_SQ
			// Vérifie aussi que cette fumée n'est pas trop loin pour être affichée
			if (smokeDistanceSQ < MAX_SMOKE_DRAW_DISTANCE_SQ)
#endif
				sortedSmoke.push_back(SmokeCameraDistance(smokeDistanceSQ, &smokePos));
		}

		// Trie le tableau des différentes positions des fumées par rapport à leur distance à la caméra
		sortedSmoke.set_sorted(false);
		sortedSmoke.sort();



		// Affiche les fumées :

		// Obtient les systèmes de particules pour la fumée de SPARK, et vérifie qu'ils sont bien valides
		IRRSystem* const smoke1System = game->renderer->getSparkManager().getSmoke1ParticleSystem();
		IRRSystem* const smoke2System = game->renderer->getSparkManager().getSmoke2ParticleSystem();
		if (!smoke1System || !smoke2System)
			return;

		// Affiche les systèmes de particules pour la fumée
		smoke1System->setVisible(true);
		smoke2System->setVisible(true);

		// Indique la rotation du système de particules
		//smoke1System->setRotation(AbsoluteTransformation.getRotationDegrees());	// TODO : Nécessaire ???

#ifdef MAX_SMOKE_DRAW_DISTANCE_SQ
		// Remet à jour la taille du tableau des fumées trié,
		// car il peut être différent de la taille de la liste des fumées si certaines ont été exclues du rendu car elles étaient trop loin de la caméra
		const u32 sortedSmokePositionSize = sortedSmoke.size();
		for (u32 i = 0; i < sortedSmokePositionSize; ++i)
#else
		for (u32 i = 0; i < smokePositionSize; ++i)
#endif
		{
			// Choisis le système de particules à afficher, le place à la position de la fumée, puis l'affiche
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

		// Masque à nouveau les systèmes de particules pour la fumée car ils ne doivent être affichés que par cette fonction, et non pas par Irrlicht
		smoke1System->setVisible(false);
		smoke2System->setVisible(false);
	}
}

#endif
