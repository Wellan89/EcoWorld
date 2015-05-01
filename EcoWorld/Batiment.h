#ifndef DEF_BATIMENT
#define DEF_BATIMENT

#include "global.h"
#include "Batiments.h"
#include "Utils.h"
#include "EcoWorldInfos.h"
#include "Interfaces.h"

class EcoWorldSystem;
class EcoWorldModifiers;

class Batiment
{
public:
	// Constructeur
	Batiment(BatimentID batimentID, EcoWorldSystem& m_system, EcoWorldInfos& m_worldInfos, EcoWorldModifiers& m_modifiers,
		const core::vector2di m_index, float m_rotation);

	// Met à jour le système de jeu d'après les informations de construction de ce bâtiment (à appeler à sa construction)
	void onConstruct();

	// Met à jour le système de jeu d'après les informations de destruction de ce bâtiment (appelé à sa destruction)
	void onDestroy();

	// Met à jour le temps de vie de ce bâtiment, ainsi que ses temps de construction et de destruction.
	// Si ce bâtiment doit être détruit, il s'ajoutera automatiquement aux listes de destruction du système de jeu.
	// A chaque appel de cette fonction, on considèrera qu'un jour supplémentaire s'est écoulé.
	// outNormalBatimentDestroy :	Si fourni, valeur de sortie indiquant si la destruction d'un bâtiment vient d'être terminée (destruction volontaire du joueur) (attention : valeur non initialisée à l'appel de la fonction)
	// outOldBatimentDestroy :		Si fourni, valeur de sortie indiquant si un bâtiment vient d'être détruit par vieillesse (destruction involontaire du joueur : durée de vie terminée)) (attention : valeur non initialisée à l'appel de la fonction)
	void updateDaysPassed(bool* outNormalBatimentDestroy = NULL, bool* outOldBatimentDestroy = NULL);

	// Met à jour les informations spécifiques à ce bâtiment pour ce jour-ci :
	// effectué juste après l'appel à updateDaysPassed() lorsque le bâtiment n'est ni en construction ni en destruction, et effectué avant la mise à jour des informations du monde avec update().
	// Aucune mise à jour des informations du monde n'est effectuée, simplement une mise à jour des informations de ce bâtiment indépendamment des autres informations du monde.
	// Cette fonction peut être appelée plusieurs fois par jour, à condition que le booléen firstCall soit passé à false lors des appels supplémentaires.
	// firstCall :							True si c'est le premier appel de cette fonction ce jour (dans ce cas, on considère qu'un jour supplémentaire s'est écoulé), false sinon.
	// pourcentageRessourcesDisponibles :	Représente le pourcentage disponible de chaque ressources pour cette mise à jour (permet de limiter la consommation relative de chaque usine)
	void updateBatimentInfos(bool firstCall, const float pourcentageRessourcesDisponibles[Ressources::RI_COUNT]);

	// Met à jour les informations du monde et de ce bâtiment avec le nombre de jours, mois et années écoulés
	// A chaque appel de cette fonction, on considèrera qu'un jour supplémentaire s'est écoulé
	void update(bool moisEcoule, bool anneeEcoulee);

	// Enregistre les données du batiment dans un fichier
	void save(io::IAttributes* out) const;

	// Charge les données du batiment depuis un fichier
	void load(io::IAttributes* in);

protected:
	// Donne un comportement du bâtiment sur le monde différent suivant son ID (spécialisation de la fonction updateBatimentInfos() pour chaque bâtiment).
	// Cette fonction peut être appelée plusieurs fois par jour.
	// pourcentageRessourcesDisponibles :	Représente le pourcentage disponible de chaque ressources pour cette mise à jour (permet de limiter la consommation relative de chaque usine)
	void updateBatimentInfosFromBatimentID(const float pourcentageRessourcesDisponibles[Ressources::RI_COUNT]);

	EcoWorldSystem& system;
	EcoWorldInfos& worldInfos;
	EcoWorldModifiers& modifiers;

	IBatimentSceneNode* sceneNode;

	BatimentID ID;
	BatimentInfos infos;
	const StaticBatimentInfos& staticInfos;

	// L'index du bâtiment dans la carte système
	core::vector2di index;

	// La rotation en Y du batiment
	float rotation;

	// Le nombre de jours écoulés depuis le début de la construction du bâtiment (l'"âge" du bâtiment)
	u32 daysPassed;

	// Le jour où l'ordre de destruction a été lancé (si égal à 0, aucun ordre de construction n'a été lancé : le bâtiment n'est pas en train d'être détruit)
	u32 destroyingDay;

	// La box contenant le batiment dans le plan 2D (conservée en mémoire pour accélérer certains calculs du système)
	obbox2df box;

public:
	// Accesseurs inline
	const EcoWorldSystem& getSystem() const				{ return system; }
	BatimentInfos& getInfos()							{ return infos; }
	const BatimentInfos& getInfos() const				{ return infos; }
	const StaticBatimentInfos& getStaticInfos() const	{ return staticInfos; }
	BatimentID getID() const							{ return ID; }
	bool isDestroying() const							{ return (destroyingDay != (u32)(-1)); }
	bool isConstructing() const							{ return (!isDestroying() && daysPassed < staticInfos.tempsC && staticInfos.tempsC != 0); }
	bool isConstructingOrDestroying() const				{ return ((destroyingDay != (u32)(-1)) || (daysPassed < staticInfos.tempsC && staticInfos.tempsC != 0)); }
	u32 getDestroyingDay() const						{ return destroyingDay; }
	u32 getDaysPassed() const							{ return daysPassed; }
	const core::vector2di& getIndex() const				{ return index; }
	float getRotation() const							{ return rotation; }
	const obbox2df& getBoundingBox() const				{ return box; }

	IBatimentSceneNode* getSceneNode()					{ return sceneNode; }
	void setSceneNode(IBatimentSceneNode* node)			{ sceneNode = node; }

	// Lance la destruction du bâtiment
	void destroy(u32 currentDay)
	{
		destroyingDay = currentDay;	// On peut aussi annuler la destruction du bâtiment en indiquant currentDay = (u32)(-1), mais ces conséquences sont encores imprévues

		if (destroyingDay != (u32)(-1))
		{
			// Met à jour les informations du monde si ce bâtiment va être détruit
			onDestroy();

			// Pendant sa destruction, le bâtiment devient complètement inactif : on réinitialise ses informations
			infos.reset();
		}
	}
};

#endif
