#ifndef DEF_ECO_WORLD_SYSTEM
#define DEF_ECO_WORLD_SYSTEM

#include "global.h"
#include "Utils.h"
#include "Batiments.h"
#include "EcoWorldInfos.h"
#include "EcoWorldModifiers.h"
#include "WeatherManager.h"
#include "Objectives.h"
#include "Interfaces.h"

class Batiment;
class CLoadingScreen;

class EcoWorldSystem
{
public:
	// Constructeur et destructeur
	EcoWorldSystem();
	~EcoWorldSystem();

	// Fonction pour savoir si on peut créer un batiment à un endroit spécifié (et si non, en indique la cause, voir E_BATIMENT_CREATING_ERROR)
	int canCreateBatiment(BatimentID batimentID, const core::vector2di& index, float rotation, bool outRessources[] = 0) const;

	// Fonction pour savoir si on peut créer plusieurs batiments à un endroit spécifié (et si non, en indique la cause, voir E_BATIMENT_MULTI_CREATING_ERROR)
	int canCreateMultiBatiments(BatimentID batimentID, int batimentCount, bool outRessources[] = 0) const;

	// Ajoute un batiment au système à la position spécifiée
	// rendererPosition et rendererDenivele seront envoyés au renderer pour la création du bâtiment
	// Retourne EBCE_AUCUN si l'ajout est valide, sinon la cause de l'erreur (voir E_BATIMENT_CREATING_ERROR)
	// Si forceAdding est à true, le placement du bâtiment ne sera pas vérifié :
	//	il sera forcé, dans les limites matérielles possibles (ex : il ne sera pas placé s'il est en-dehors de la carte du monde !)
	//	Cet argument n'est à utiliser qu'en mode jeu client dans une partie multijoueur, car il pourrait nuire au bon fonctionnement du jeu
	// forceDureeVie :	La durée de vie forcée de ce bâtiment lorsque forceAdding est à true
	int addBatiment(BatimentID batimentID, const core::vector2di& index, float rotation, bool outRessources[] = 0, const core::vector3df* rendererPosition = NULL, const float* rendererDenivele = NULL
#ifdef USE_RAKNET
		, bool forceAdding = false, u32 forceDureeVie = 0
#endif
		);

	// Fonction pour savoir si on peut détruire un batiment (et si non, en indique la cause, voir E_BATIMENT_DESTROYING_ERROR)
	int canDestroyBatiment(const core::vector2di& index, bool outRessources[] = 0) const;

	// Détruit un batiment (lance son ordre de destruction)
	// Retourne EBCE_AUCUN si la destruction est valide, sinon la cause de l'erreur (voir E_BATIMENT_DESTROYING_ERROR)
	// Si forceDestroying est à true, la destruction du bâtiment ne sera pas vérifiée :
	//	elle sera forcée dans les limites matérielles possibles (ex : il ne sera pas détruit si son index est en-dehors de la carte du monde !)
	//	Cet argument n'est à utiliser qu'en mode jeu client dans une partie multijoueur, car il pourrait nuire au bon fonctionnement du jeu
	int destroyBatiment(const core::vector2di& index, bool outRessources[] = 0
#ifdef USE_RAKNET
		, bool forceDestroying = false
#endif
		);

	// Ajoute un bâtiment à la liste de suppression, pour qu'il soit supprimé durant la mise à jour du monde
	// Note : Aucune vérification si la suppression du bâtiment est valide ne sera effectuée, le bâtiment sera simplement supprimé du système dès que possible
	void addBatimentToDeleteList(Batiment* batiment);

	// Actualise le monde pour le menu principal (en réalité, met seulement à jour le temps du jeu et le weather manager)
	void updateMainMenu(float elapsedTime);

	// Actualise le monde
	// elapsedTime :				Temps écoulé depuis la dernière mise à jour
	// outNormalBatimentDestroy :	Si fourni, valeur de sortie indiquant si la destruction d'un bâtiment vient d'être terminée (destruction volontaire du joueur) (note : valeur initialisée à l'appel de la fonction)
	// outOldBatimentDestroy :		Si fourni, valeur de sortie indiquant si un bâtiment vient d'être détruit par vieillesse (destruction involontaire du joueur : durée de vie terminée)) (note : valeur initialisée à l'appel de la fonction)
	void update(float elapsedTime, bool* outNormalBatimentDestroy = NULL, bool* outOldBatimentDestroy = NULL);

	// Remet le système à zéro :
	// si resetTerrain est à false, toutes les informations concernant le terrain de jeu (i.e. sa constructibilité) seront conservées
	void reset(bool resetTerrain = true);

	// Initialise le système de jeu pour la création d'une nouvelle partie (attention : le système devra avoir préalablement été remis à zéro !)
	// difficulty :		La difficulté de cette nouvelle partie (influe les modifiers de jeu)
	// objectives :		La liste des objectifs personnalisés associés à cette nouvelle partie
	void createNewGame(EcoWorldModifiers::E_DIFFICULTY difficulty, const core::list<ObjectiveData>& objectivesList);

	// Retourne les objectifs par défaut pour la création d'une nouvelle partie
	static core::list<ObjectiveData> getNewGameDefaultObjectives();

	// Enregistre la partie dans un fichier
	void save(io::IAttributes* out, io::IXMLWriter* writer) const;

	// Charge la partie à partir d'un fichier (le système sera remis à zéro, mais les informations concernant le terrain de jeu seront conservées : elles doivent déjà avoir été initialisées)
	void load(io::IAttributes* in, io::IXMLReader* reader,
		CLoadingScreen* loadingScreen = NULL, float loadMin = 100.0f, float loadMax = 150.0f);

protected:
	// Met à jour le système sans se préocupper du temps écoulé réel (lors de l'appel de cette fonction, on considérera qu'un jour se sera automatiquement écoulé depuis la dernière mise à jour du système)
	// moisEcoule :					Indique si un mois s'est écoulé depuis la dernière mise à jour
	// anneeEcoulee :				Indique si une année s'est écoulée depuis la dernière mise à jour
	// outNormalBatimentDestroy :	Si fourni, valeur de sortie indiquant si la destruction d'un bâtiment vient d'être terminée (destruction volontaire du joueur) (attention : valeur non initialisée à l'appel de la fonction)
	// outOldBatimentDestroy :		Si fourni, valeur de sortie indiquant si un bâtiment vient d'être détruit par vieillesse (destruction involontaire du joueur : durée de vie terminée)) (attention : valeur non initialisée à l'appel de la fonction)
	void updateSystem(bool moisEcoule, bool anneeEcoulee, bool* outNormalBatimentDestroy = NULL, bool* outOldBatimentDestroy = NULL);

	// Met à jour le temps de vie des bâtiments ainsi que leur temps de construction/destruction
	// outNormalBatimentDestroy :	Si fourni, valeur de sortie indiquant si la destruction d'un bâtiment vient d'être terminée (destruction volontaire du joueur) (attention : valeur non initialisée à l'appel de la fonction)
	// outOldBatimentDestroy :		Si fourni, valeur de sortie indiquant si un bâtiment vient d'être détruit par vieillesse (destruction involontaire du joueur : durée de vie terminée)) (attention : valeur non initialisée à l'appel de la fonction)
	void updateBatimentDaysPassed(bool* outNormalBatimentDestroy = NULL, bool* outOldBatimentDestroy = NULL);

	// Calcule le pourcentage disponible de chaque ressources pour cette mise à jour, et met à jour les informations des bâtiments (se charge des appels à la fonction Batiment::updateBatimentInfos).
	// Utilité : Il reste 500 kg de sable, il y a une usine qui en consomme 250 kg par jour, une autre qui en consomme 750 kg par jour :
	//			le total de sable nécessaire est donc de 250 + 750 = 1000 kg de sable, chaque usine aura donc 500 / 1000 = 50 % de sable disponible,
	//			la première usine recevra donc 0.5 * 250 = 125 kg de sable, et la seconde recevra 0.5 * 750 = 375 kg de sable.
	// Ce système permet de répartir les ressources manquantes entre chaque usine pour que chacune en ait une quantité relative à sa consommation (optimise ainsi les pourcentages de production des usines).
	// A chaque appel de cette fonction, on considèrera qu'un jour supplémentaire s'est écoulé.
	void calculatePourcentageRessourcesDisponiblesAndUpdateBatimentsInfos();

	EcoWorldInfos infos;
	EcoWorldModifiers modifiers;
	WeatherManager weatherManager;
	ISystemRenderer* systemRenderer;

	// Les objectifs à remplir pour la partie en cours
	Objectives objectives;

	::Time time;			// Le temps total du jeu actuel
	float lastUpdateTime;	// Le temps total du jeu lors de la dernière mise à jour du système

	// La quantité de ressources produites/consommées en une journée (recalculée à chaque actualisation du monde)
	float ressourcesProduites[Ressources::RI_COUNT];

	// Tableau servant simplement à stocker le pourcentage disponible de chaque ressource
	// pourrait être déclaré dans EcoWorldSystem::update(), mais ici on évite les allocations/désallocations successives d'un grand tableau)
	// A ne pas utiliser en dehors de la fonction EcoWorldSystem::update() !
	float pourcentageRessourcesDisponibles[Ressources::RI_COUNT];

	// La liste contenant tous les bâtiments ajoutés au système
	core::list<Batiment*> listeAllBats;

	// Les listes des bâtiments pour leur mise à jour
	core::list<Batiment*> listeBatsProductionElectricite;
	core::list<Batiment*> listeBatsProductionEau;
	core::list<Batiment*> listeBatsUsines;
	core::list<Batiment*> listeBatsMaisons;
	core::list<Batiment*> listeBatsOther;
	core::list<Batiment*> listeBatsDechetsES;

	// Ajoute un bâtiment nouvellement créé aux listes des bâtiments
	void addBatimentToLists(Batiment* batiment);

	// Supprime un bâtiment qui devrait se trouver dans les listes des bâtiments
	void deleteBatimentInLists(Batiment* batiment);

	// La liste indiquant les bâtiments à supprimer à la fin de la prochaine mise à jour du monde
	core::list<Batiment*> deleteList;

	// Supprime directement un bâtiment du système (sans aucune vérification)
	void deleteBatiment(Batiment* batiment);

public:
	// Détermine si le joueur a gagné la partie
	bool isGameWon() const
	{
		return objectives.isGameWon();
	}
	// Détermine si le joueur a perdu la partie
	bool isGameLost() const
	{
		return objectives.isGameLost();
	}
	// Détermine si la partie est terminée
	bool isGameFinished() const
	{
		return objectives.isGameFinished();
	}
	// Indique si le joueur souhaite continuer la partie en cours même si elle est gagnée
	void setContinueGame(bool continueGame)
	{
		objectives.setContinueGame(continueGame);
	}

#ifdef USE_RAKNET
	// Synchronise le temps total actuel du jeu avec le temps spécifié
	void synchroniseGameTime(float totalTime)
	{
		time.setTotalTime(totalTime);
		if (lastUpdateTime > totalTime)
			lastUpdateTime = totalTime;
	}
#endif

#ifdef _DEBUG
	// Outils de débogage :

	// Ajoute un certain montant au budget du joueur
	void addBudget(float amount) { infos.budget += amount; }
	// Retire un certain montant du budget du joueur
	void subtractBudget(float amount) { infos.budget -= amount; }
	// Ajoute le temps spécifié au temps système
	void addTime(float timeS) { time += timeS; }
	// Ajoute un certain nombre de toutes les ressources au système
	void addRessources(float quantity) { for (u32 i = 0; i < Ressources::RI_COUNT; ++i) { infos.ressources[i] += quantity; infos.ressources[i] = max(infos.ressources[i], 0.0f); } }
#endif

	// Accesseurs inline
	const EcoWorldInfos& getInfos() const					{ return infos; }
	const EcoWorldModifiers& getModifiers() const			{ return modifiers; }
	const ::Time& getTime() const							{ return time; }
	const WeatherManager& getWeatherManager() const			{ return weatherManager; }
	WeatherManager& getWeatherManager()						{ return weatherManager; }
	const Objectives& getObjectives() const					{ return objectives; }		// Obtient la liste des objectifs définis pour la partie en cours
	Objectives& getObjectives()								{ return objectives; }		// Obtient la liste des objectifs définis pour la partie en cours

	// Obtient la production d'une certaine ressource : La quantité de ressources produites/consommées en une journée (recalculée à chaque actualisation du monde)
	float getRessourcesProduction(Ressources::RessourceID ressourceID) const { if (ressourceID >= 0 && ressourceID < Ressources::RI_COUNT) { return ressourcesProduites[ressourceID]; } return 0.0f; }

	// Obtient la liste de tous les bâtiments du jeu
	const core::list<Batiment*>& getAllBatiments() const	{ return listeAllBats; }

	// Obtient le renderer du système de jeu
	const ISystemRenderer* getSystemRenderer() const		{ return systemRenderer; }

	// Indique le renderer du système de jeu
	void setSystemRenderer(ISystemRenderer* renderer)		{ systemRenderer = renderer; }

	// La carte du monde
	TerrainInfo carte[TAILLE_CARTE][TAILLE_CARTE];



	// Fonctions inline

	// Ces trois fonctions ont un algorithme de calcul qui peut sembler complexe, mais qui a été longtemps réfléchi et est actuellement certifié (06/04/11).
	// Elles fonctionnent seulement si TAILLE_CARTE est pair !

	// Retourne une position en coordonnées 3D arrondie à TAILLE_OBJETS en X et en Z suivant une position en coordonnées 3D
	static core::vector3df getRoundPosition(const core::vector3df& position)
	{
		const float INV_TAILLE_OBJETS = 1.0f / TAILLE_OBJETS;

		return core::vector3df(
			(core::floor32(position.X * INV_TAILLE_OBJETS + 1.0f) - 0.5f) * TAILLE_OBJETS,
			position.Y,
			(core::floor32(position.Z * INV_TAILLE_OBJETS + 1.0f) - 0.5f) * TAILLE_OBJETS);
	}
	// Retourne un index en coordonnées de la carte suivant une position en coordonnées 3D
	static core::vector2di getIndexFromPosition(const core::vector3df& position)
	{
		// -> Valide avec TAILLE_CARTE divisible par 2 (obligatoire !)

		const float INV_TAILLE_OBJETS = 1.0f / TAILLE_OBJETS;

		// On n'oublie pas d'ajouter la moitié de la taille de la carte à l'index car une partie des positions est dans les valeurs négatives
		return core::vector2di(
			(int)(core::floor32(position.X * INV_TAILLE_OBJETS + 1.0f) + (TAILLE_CARTE - 2) * 0.5f),
			(int)(core::floor32(position.Z * INV_TAILLE_OBJETS + 1.0f) + (TAILLE_CARTE - 2) * 0.5f));
	}
	// Retourne une position en coordonnées 3D arrondie à TAILLE_OBJETS en X et en Z de la carte suivant un index en coordonnées de la carte (la position en Y sera égale à 0.0f)
	static core::vector3df getPositionFromIndex(const core::vector2di& index)
	{
		// -> Valide avec TAILLE_CARTE divisible par 2 (obligatoire !)

		// On n'oublie pas de soustraire la moitié de la taille de la carte à l'index
		return core::vector3df(
			((float)index.X - (TAILLE_CARTE - 1) * 0.5f) * TAILLE_OBJETS,
			0.0f,
			((float)index.Y - (TAILLE_CARTE - 1) * 0.5f) * TAILLE_OBJETS);
	}
};

#endif
