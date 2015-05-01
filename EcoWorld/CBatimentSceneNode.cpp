#include "CBatimentSceneNode.h"
#include "Game.h"
#include "EcoWorldRenderer.h"
#include "EcoWorldSystem.h"
#ifdef USE_SPARK
#include "SparkManager.h"
#endif

#ifdef ANIMATE_TREES_ROTATION
// Initialise les informations statiques sur l'animation en rotation des arbres
u32 CBatimentSceneNode::treeRotationLastComputeTimeMs = 0;
core::vector2df CBatimentSceneNode::treeLastRotation(0.0f, 0.0f);
#endif

CBatimentSceneNode::CBatimentSceneNode(Batiment* bat, const core::vector3df& position, float denivele, ISceneNode* parent, int id)
 : IBatimentSceneNode(parent, game->sceneManager, id), batiment(bat), constructingNode(NULL), concreteNode(NULL), batimentNode(NULL), textNode(NULL),
#ifdef ANIMATE_TREES
 treeSizeReducing(false),
#endif
 multipleTriangleSelector(NULL), textNodeVisible(false), lastTimeMs(0)
{
#ifdef _DEBUG
	setDebugName("CBatimentSceneNode");
#endif

	if (bat)
	{
		// Charge les nodes pour ce bâtiment
		const BatimentID batID = bat->getID();
		constructingNode = game->renderer->loadConstructingBatiment(batID, this);
		if (StaticBatimentInfos::needConcreteUnderBatiment(batID))	// Vérifie que ce bâtiment nécessite bien un bloc de béton en-dessous de lui
			concreteNode = game->renderer->loadConcreteBatiment(denivele, batID, this);
		batimentNode = game->renderer->loadBatiment(batID, this);
		textNode = game->renderer->loadTextBatiment(batID, this);

		// Modifie la position du node de texte avec l'agrandissement du bâtiment qu'elle représente, puisqu'elle a été calculée suivant ce dernier
		if (textNode && batimentNode)
			textNode->setPosition(textNode->getPosition() * batimentNode->getScale());

#ifdef USE_SPARK
		// Obtient les positions de la fumée pour ce bâtiment
		game->renderer->loadBatimentSmokePositions(smoke1Positions, smoke2Positions, batID,
			(batimentNode ? batimentNode->getScale() : core::vector3df(1.0f)),
			bat->getRotation());
#endif

		// Place ce batiment
		setPosition(position);
		setRotation(core::vector3df(0.0f, bat->getRotation(), 0.0f));
	}

	// Crée le Multiple Triangle Selector et lui indique les triangle selector de ce node
	multipleTriangleSelector = new CMultipleTriangleSelector(2);
	multipleTriangleSelector->addTriangleSelector(constructingNode ? constructingNode->getTriangleSelector() : NULL);
	multipleTriangleSelector->addTriangleSelector(batimentNode ? batimentNode->getTriangleSelector() : NULL);

	// Crée le meta triangle selector de ce node
	metaTriangleSelector = game->sceneManager->createMetaTriangleSelector();
	metaTriangleSelector->addTriangleSelector(multipleTriangleSelector);
	metaTriangleSelector->addTriangleSelector(concreteNode ? concreteNode->getTriangleSelector() : NULL);
}
CBatimentSceneNode::~CBatimentSceneNode()
{
	if (metaTriangleSelector)
		metaTriangleSelector->drop();

	if (multipleTriangleSelector)
		multipleTriangleSelector->drop();
}
#ifdef USE_SPARK
void CBatimentSceneNode::OnRegisterSceneNode()
{
	if (IsVisible)
	{
		// Enregistre les positions des fumées de ce bâtiment pour le rendu, s'il n'est pas en train d'être construit ou détruit, et que son facteur de production actuel n'est pas nul, et que les systèmes de particules sont visibles pour ce rendu
		if (batiment && game->renderer->getSparkManager().isParticleSystemVisible())
			if (!batiment->isConstructingOrDestroying() && batiment->getInfos().pourcentageProductionActuel >= 0.01f)
				game->renderer->getSparkManager().registerSmokePosition(smoke1Positions, smoke2Positions);

		// Permet aux nodes enfants de s'enregistrer pour le rendu
		ISceneNode::OnRegisterSceneNode();
	}
}
#endif
void CBatimentSceneNode::OnAnimate(u32 timeMs)
{
	if (!IsVisible)
		return;

	// Rend les nodes enfants visibles ou invisibles suivant l'état actuel du bâtiment : s'ils sont visibles, ils seront animés juste après ce node, à l'appel de ISceneNode::OnAnimate
	if (batiment)
	{
		const bool isConstructing = (batiment->isConstructingOrDestroying());
		if (constructingNode)
			constructingNode->setVisible(isConstructing);
		if (concreteNode)
			concreteNode->setVisible(true);
		if (batimentNode)
			batimentNode->setVisible(!isConstructing);
		if (textNode)
		{
			if (game->renderer->getNomsBatimentsVisible() == true)
				textNode->setVisible(true);
			else if (game->renderer->getNomsBatimentsVisible() == false)
				textNode->setVisible(false);
			else
				textNode->setVisible(textNodeVisible);
		}

		multipleTriangleSelector->setCurrentTriangleSelector(isConstructing ? 0 : 1);
	}
	else	// Ne devrait jamais se produire !
	{
		LOG_DEBUG("CBatimentSceneNode::OnAnimate(" << timeMs << ") : Aucun batiment n'est associe a ce node : batiment = " << batiment, ELL_WARNING);

		if (constructingNode)
			constructingNode->setVisible(false);
		if (concreteNode)
			concreteNode->setVisible(false);
		if (batimentNode)
			batimentNode->setVisible(false);
		if (textNode)
			textNode->setVisible(false);

		multipleTriangleSelector->setCurrentTriangleSelector(-1);
	}

	// Anime ce node si le bâtiment lié est valide
	if (batiment)
	{
		// Calcule le temps écoulé depuis la dernière frame
		u32 elapsedTimeMs = 0;
		if (lastTimeMs < timeMs)	// Vérifie que le nombre qu'on obtiendra après la soustraction sera bien positif
			elapsedTimeMs = timeMs - lastTimeMs;
		lastTimeMs = timeMs;

		if (elapsedTimeMs != 0)
			animateBatimentFromID(timeMs, elapsedTimeMs);
	}

	// Anime les enfants de ce node
	ISceneNode::OnAnimate(timeMs);
}
void CBatimentSceneNode::animateBatimentFromID(u32 timeMs, u32 elapsedTimeMs)
{
	switch (batiment->getID())
	{
		// Arbres
	case BI_arbre_aspen:
	case BI_arbre_oak:
	case BI_arbre_pine:
	case BI_arbre_willow:
		if (!batiment->isConstructingOrDestroying())
		{
#ifdef ANIMATE_TREES_ROTATION
			// Animation en rotation des arbres par rapport au sol, suivant la direction du vent :
			// L'arbre oscille suivant la direction du vent, en une fonction sinusoïdale du temps

#define TREE_ROTATION_SPEED				0.0002f	// Vitesse de rotation de l'arbre (en oscillations/ms) <=> (Vitesse de rotation en oscillation/s) / 1000
#define TREE_ROTATION_HALF_AMPLITUDE	1.5f	// Angle maximal (demi-amplitude) que peut atteindre rotation de l'arbre (en degrés) (l'amplitude totale de cette rotation est donc de 2 * TREE_ROTATION_AMPLITUDE)

			// Vérifie si le temps a changé depuis le dernier calcul de rotation des arbres (permet de ne le calculer qu'une seule fois par frame, indépendament du nombre d'arbres à afficher)
			if (timeMs != treeRotationLastComputeTimeMs)
			{
				// Le temps a changé depuis le dernier calcul de rotation des arbres :
				// On recalcule la rotation des arbres suivant le nouveau temps actuel

				// Obtient la direction normalisée du vent en coordonnées 3D
				const core::vector2df& windDirection2D = game->renderer->getTerrainManager().getWindDirection();
				core::vector3df windDirection(windDirection2D.X, 0.0f, windDirection2D.Y);
				windDirection.normalize();

				// Détermine l'axe de la rotation des arbres, par un produit vectoriel entre le vecteur vertical (d'axe Y) et la direction du vent
				const core::vector3df upVector(0.0f, 1.0f, 0.0f);
				const core::vector3df rotationAxis = upVector.crossProduct(windDirection);

				// Détermine l'angle de rotation des arbres, d'après une fonction sinusoïdale du temps
				// A un temps d'origine de 0 ms, l'arbre a une position verticale, et au cours du temps, sa rotation parcourt un domaine de [-TREE_ROTATION_HALF_AMPLITUDE ; TREE_ROTATION_HALF_AMPLITUDE]
				// Formule : rotation = f(temps) = sin(temps * TREE_ROTATION_SPEED * PI) * TREE_ROTATION_HALF_AMPLITUDE
				const float rotationAngleRad = sinf((float)timeMs * TREE_ROTATION_SPEED * core::PI) * TREE_ROTATION_HALF_AMPLITUDE * core::DEGTORAD;



				// Calcule la nouvelle rotation des arbres, en conservant sa rotation suivant l'axe Y :

				// Retiens la rotation de l'arbre en Y, car elle est indépendante de cette animation et doit être maintenue constante
				const float rotationY = RelativeRotation.Y;

				// Utilise les quaternions pour permettre un calcul simple de la nouvelle rotation, d'après l'angle et le vecteur de la rotation
				core::quaternion quat;
				quat.fromAngleAxis(rotationAngleRad, rotationAxis);	// Crée le quaternion représentant cette rotation d'après l'angle et le vecteur de l'axe de la rotation : l'angle doit être en radians, et le vecteur de l'axe de la rotation doit être normalisé
				quat.toEuler(RelativeRotation);						// Convertit ce quaternion en angles d'Euler et les assigne à la rotation de cet arbre
				RelativeRotation.X *= core::RADTODEG;				// Convertit la rotation en X de cet arbre de radians en degrés
				RelativeRotation.Z *= core::RADTODEG;				// Convertit la rotation en Z de cet arbre de radians en degrés
				RelativeRotation.Y = rotationY;						// Rétablit la rotation en Y de cet arbre

				// Retiens les dernières rotations calculées, pour qu'elles puissent être directement utilisées pour les prochains arbres à calculer
				treeRotationLastComputeTimeMs = timeMs;
				treeLastRotation.set(RelativeRotation.X, RelativeRotation.Z);
			}
			else
			{
				// Le temps du jeu n'a pas changé depuis le dernier calcul de rotation des arbres :
				// On applique la dernière rotation calculée aux arbres
				RelativeRotation.X = treeLastRotation.X;
				RelativeRotation.Z = treeLastRotation.Y;
			}
#endif



			// Pour les arbres : les arbres grossissent quadratiquement entre 0 et 1 an, et conservent leur taille passé cet âge :
			const u32 joursEcoules = batiment->getDaysPassed();

			if (joursEcoules >= 360)	// Plus d'1 an : >= 360 jours
			{
#ifdef ANIMATE_TREES_SIZE
				// Si l'arbre a déjà atteint sa taille adulte, on lui donne tout de même un peu de vie
				// en l'agrandissant et réduisant légèrement (semble ainsi faire bouger ses feuilles) :

#define TREE_SCALING_SPEED		0.002f	// Vitesse de grandissement de l'arbre (par seconde)
#define TREE_SCALING_DELTA_MAX	0.002f	// Grandissement et réduction maximale de l'arbre (par rapport à 1.0f)

				float scale = RelativeScale.X;	// On suppose que l'agrandissement est le même en X, Y et Z

				// Modifie l'agrandissement de l'arbre avec le temps écoulé
				scale += TREE_SCALING_SPEED * elapsedTimeMs * 0.001f * (treeSizeReducing ? -1.0f : 1.0f);

				// Remet l'agrandissement dans les valeurs limites et commande l'inversion de sens si nécessaire
				if (scale < (1.0f - TREE_SCALING_DELTA_MAX))
				{
					treeSizeReducing = false;
					scale = 1.0f - TREE_SCALING_DELTA_MAX;
				}
				else if (scale > (1.0f + TREE_SCALING_DELTA_MAX))
				{
					treeSizeReducing = true;
					scale = 1.0f + TREE_SCALING_DELTA_MAX;
				}

				// Indique la nouvelle taille de cet arbre
				RelativeScale = scale;
				break;
#else
				// Force l'arbre à avoir une taille normale
				RelativeScale.set(1.0f, 1.0f, 1.0f);
				break;
#endif
			}

			// Moins d'1 an : < 360 jours
			
			// Ajoute le temps en trop du timer système au temps de vie de l'arbre, pour éviter le grandissement par accoups de l'arbre
			float time = (float)joursEcoules + batiment->getSystem().getTime().getExtraTime() * DAY_TIME_INV;

			// Formule d'agrandissement : X = f(t) = -0.8 * ((t / 360) - 1)² + 1
			float scale = time / 360.0f - 1.0f;
			scale *= scale * -0.8f;
			scale += 1.0f;
			RelativeScale.set(scale, scale, scale);
		}
		else
			RelativeScale.set(1.0f, 1.0f, 1.0f);
		break;
	}
}
void CBatimentSceneNode::render()
{
	// Ne fait rien : Les enfants de ce node s'afficheront d'eux-même
}
/*void CBatimentSceneNode::save(io::IAttributes* out) const
{
	if (!out)
		return;

	// TODO

	// Les infos du bâtiment seront enregistrées par le système
}
void CBatimentSceneNode::load(io::IAttributes* in)
{
	if (!in)
		return;

	// Les infos du bâtiment ont déjà été lues par le système, il ne reste plus qu'à les récupérer

	// TODO
}
*/
