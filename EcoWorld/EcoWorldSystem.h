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

	// Fonction pour savoir si on peut cr�er un batiment � un endroit sp�cifi� (et si non, en indique la cause, voir E_BATIMENT_CREATING_ERROR)
	int canCreateBatiment(BatimentID batimentID, const core::vector2di& index, float rotation, bool outRessources[] = 0) const;

	// Fonction pour savoir si on peut cr�er plusieurs batiments � un endroit sp�cifi� (et si non, en indique la cause, voir E_BATIMENT_MULTI_CREATING_ERROR)
	int canCreateMultiBatiments(BatimentID batimentID, int batimentCount, bool outRessources[] = 0) const;

	// Ajoute un batiment au syst�me � la position sp�cifi�e
	// rendererPosition et rendererDenivele seront envoy�s au renderer pour la cr�ation du b�timent
	// Retourne EBCE_AUCUN si l'ajout est valide, sinon la cause de l'erreur (voir E_BATIMENT_CREATING_ERROR)
	// Si forceAdding est � true, le placement du b�timent ne sera pas v�rifi� :
	//	il sera forc�, dans les limites mat�rielles possibles (ex : il ne sera pas plac� s'il est en-dehors de la carte du monde !)
	//	Cet argument n'est � utiliser qu'en mode jeu client dans une partie multijoueur, car il pourrait nuire au bon fonctionnement du jeu
	// forceDureeVie :	La dur�e de vie forc�e de ce b�timent lorsque forceAdding est � true
	int addBatiment(BatimentID batimentID, const core::vector2di& index, float rotation, bool outRessources[] = 0, const core::vector3df* rendererPosition = NULL, const float* rendererDenivele = NULL
#ifdef USE_RAKNET
		, bool forceAdding = false, u32 forceDureeVie = 0
#endif
		);

	// Fonction pour savoir si on peut d�truire un batiment (et si non, en indique la cause, voir E_BATIMENT_DESTROYING_ERROR)
	int canDestroyBatiment(const core::vector2di& index, bool outRessources[] = 0) const;

	// D�truit un batiment (lance son ordre de destruction)
	// Retourne EBCE_AUCUN si la destruction est valide, sinon la cause de l'erreur (voir E_BATIMENT_DESTROYING_ERROR)
	// Si forceDestroying est � true, la destruction du b�timent ne sera pas v�rifi�e :
	//	elle sera forc�e dans les limites mat�rielles possibles (ex : il ne sera pas d�truit si son index est en-dehors de la carte du monde !)
	//	Cet argument n'est � utiliser qu'en mode jeu client dans une partie multijoueur, car il pourrait nuire au bon fonctionnement du jeu
	int destroyBatiment(const core::vector2di& index, bool outRessources[] = 0
#ifdef USE_RAKNET
		, bool forceDestroying = false
#endif
		);

	// Ajoute un b�timent � la liste de suppression, pour qu'il soit supprim� durant la mise � jour du monde
	// Note : Aucune v�rification si la suppression du b�timent est valide ne sera effectu�e, le b�timent sera simplement supprim� du syst�me d�s que possible
	void addBatimentToDeleteList(Batiment* batiment);

	// Actualise le monde pour le menu principal (en r�alit�, met seulement � jour le temps du jeu et le weather manager)
	void updateMainMenu(float elapsedTime);

	// Actualise le monde
	// elapsedTime :				Temps �coul� depuis la derni�re mise � jour
	// outNormalBatimentDestroy :	Si fourni, valeur de sortie indiquant si la destruction d'un b�timent vient d'�tre termin�e (destruction volontaire du joueur) (note : valeur initialis�e � l'appel de la fonction)
	// outOldBatimentDestroy :		Si fourni, valeur de sortie indiquant si un b�timent vient d'�tre d�truit par vieillesse (destruction involontaire du joueur : dur�e de vie termin�e)) (note : valeur initialis�e � l'appel de la fonction)
	void update(float elapsedTime, bool* outNormalBatimentDestroy = NULL, bool* outOldBatimentDestroy = NULL);

	// Remet le syst�me � z�ro :
	// si resetTerrain est � false, toutes les informations concernant le terrain de jeu (i.e. sa constructibilit�) seront conserv�es
	void reset(bool resetTerrain = true);

	// Initialise le syst�me de jeu pour la cr�ation d'une nouvelle partie (attention : le syst�me devra avoir pr�alablement �t� remis � z�ro !)
	// difficulty :		La difficult� de cette nouvelle partie (influe les modifiers de jeu)
	// objectives :		La liste des objectifs personnalis�s associ�s � cette nouvelle partie
	void createNewGame(EcoWorldModifiers::E_DIFFICULTY difficulty, const core::list<ObjectiveData>& objectivesList);

	// Retourne les objectifs par d�faut pour la cr�ation d'une nouvelle partie
	static core::list<ObjectiveData> getNewGameDefaultObjectives();

	// Enregistre la partie dans un fichier
	void save(io::IAttributes* out, io::IXMLWriter* writer) const;

	// Charge la partie � partir d'un fichier (le syst�me sera remis � z�ro, mais les informations concernant le terrain de jeu seront conserv�es : elles doivent d�j� avoir �t� initialis�es)
	void load(io::IAttributes* in, io::IXMLReader* reader,
		CLoadingScreen* loadingScreen = NULL, float loadMin = 100.0f, float loadMax = 150.0f);

protected:
	// Met � jour le syst�me sans se pr�ocupper du temps �coul� r�el (lors de l'appel de cette fonction, on consid�rera qu'un jour se sera automatiquement �coul� depuis la derni�re mise � jour du syst�me)
	// moisEcoule :					Indique si un mois s'est �coul� depuis la derni�re mise � jour
	// anneeEcoulee :				Indique si une ann�e s'est �coul�e depuis la derni�re mise � jour
	// outNormalBatimentDestroy :	Si fourni, valeur de sortie indiquant si la destruction d'un b�timent vient d'�tre termin�e (destruction volontaire du joueur) (attention : valeur non initialis�e � l'appel de la fonction)
	// outOldBatimentDestroy :		Si fourni, valeur de sortie indiquant si un b�timent vient d'�tre d�truit par vieillesse (destruction involontaire du joueur : dur�e de vie termin�e)) (attention : valeur non initialis�e � l'appel de la fonction)
	void updateSystem(bool moisEcoule, bool anneeEcoulee, bool* outNormalBatimentDestroy = NULL, bool* outOldBatimentDestroy = NULL);

	// Met � jour le temps de vie des b�timents ainsi que leur temps de construction/destruction
	// outNormalBatimentDestroy :	Si fourni, valeur de sortie indiquant si la destruction d'un b�timent vient d'�tre termin�e (destruction volontaire du joueur) (attention : valeur non initialis�e � l'appel de la fonction)
	// outOldBatimentDestroy :		Si fourni, valeur de sortie indiquant si un b�timent vient d'�tre d�truit par vieillesse (destruction involontaire du joueur : dur�e de vie termin�e)) (attention : valeur non initialis�e � l'appel de la fonction)
	void updateBatimentDaysPassed(bool* outNormalBatimentDestroy = NULL, bool* outOldBatimentDestroy = NULL);

	// Calcule le pourcentage disponible de chaque ressources pour cette mise � jour, et met � jour les informations des b�timents (se charge des appels � la fonction Batiment::updateBatimentInfos).
	// Utilit� : Il reste 500 kg de sable, il y a une usine qui en consomme 250 kg par jour, une autre qui en consomme 750 kg par jour :
	//			le total de sable n�cessaire est donc de 250 + 750 = 1000 kg de sable, chaque usine aura donc 500 / 1000 = 50 % de sable disponible,
	//			la premi�re usine recevra donc 0.5 * 250 = 125 kg de sable, et la seconde recevra 0.5 * 750 = 375 kg de sable.
	// Ce syst�me permet de r�partir les ressources manquantes entre chaque usine pour que chacune en ait une quantit� relative � sa consommation (optimise ainsi les pourcentages de production des usines).
	// A chaque appel de cette fonction, on consid�rera qu'un jour suppl�mentaire s'est �coul�.
	void calculatePourcentageRessourcesDisponiblesAndUpdateBatimentsInfos();

	EcoWorldInfos infos;
	EcoWorldModifiers modifiers;
	WeatherManager weatherManager;
	ISystemRenderer* systemRenderer;

	// Les objectifs � remplir pour la partie en cours
	Objectives objectives;

	::Time time;			// Le temps total du jeu actuel
	float lastUpdateTime;	// Le temps total du jeu lors de la derni�re mise � jour du syst�me

	// La quantit� de ressources produites/consomm�es en une journ�e (recalcul�e � chaque actualisation du monde)
	float ressourcesProduites[Ressources::RI_COUNT];

	// Tableau servant simplement � stocker le pourcentage disponible de chaque ressource
	// pourrait �tre d�clar� dans EcoWorldSystem::update(), mais ici on �vite les allocations/d�sallocations successives d'un grand tableau)
	// A ne pas utiliser en dehors de la fonction EcoWorldSystem::update() !
	float pourcentageRessourcesDisponibles[Ressources::RI_COUNT];

	// La liste contenant tous les b�timents ajout�s au syst�me
	core::list<Batiment*> listeAllBats;

	// Les listes des b�timents pour leur mise � jour
	core::list<Batiment*> listeBatsProductionElectricite;
	core::list<Batiment*> listeBatsProductionEau;
	core::list<Batiment*> listeBatsUsines;
	core::list<Batiment*> listeBatsMaisons;
	core::list<Batiment*> listeBatsOther;
	core::list<Batiment*> listeBatsDechetsES;

	// Ajoute un b�timent nouvellement cr�� aux listes des b�timents
	void addBatimentToLists(Batiment* batiment);

	// Supprime un b�timent qui devrait se trouver dans les listes des b�timents
	void deleteBatimentInLists(Batiment* batiment);

	// La liste indiquant les b�timents � supprimer � la fin de la prochaine mise � jour du monde
	core::list<Batiment*> deleteList;

	// Supprime directement un b�timent du syst�me (sans aucune v�rification)
	void deleteBatiment(Batiment* batiment);

public:
	// D�termine si le joueur a gagn� la partie
	bool isGameWon() const
	{
		return objectives.isGameWon();
	}
	// D�termine si le joueur a perdu la partie
	bool isGameLost() const
	{
		return objectives.isGameLost();
	}
	// D�termine si la partie est termin�e
	bool isGameFinished() const
	{
		return objectives.isGameFinished();
	}
	// Indique si le joueur souhaite continuer la partie en cours m�me si elle est gagn�e
	void setContinueGame(bool continueGame)
	{
		objectives.setContinueGame(continueGame);
	}

#ifdef USE_RAKNET
	// Synchronise le temps total actuel du jeu avec le temps sp�cifi�
	void synchroniseGameTime(float totalTime)
	{
		time.setTotalTime(totalTime);
		if (lastUpdateTime > totalTime)
			lastUpdateTime = totalTime;
	}
#endif

#ifdef _DEBUG
	// Outils de d�bogage :

	// Ajoute un certain montant au budget du joueur
	void addBudget(float amount) { infos.budget += amount; }
	// Retire un certain montant du budget du joueur
	void subtractBudget(float amount) { infos.budget -= amount; }
	// Ajoute le temps sp�cifi� au temps syst�me
	void addTime(float timeS) { time += timeS; }
	// Ajoute un certain nombre de toutes les ressources au syst�me
	void addRessources(float quantity) { for (u32 i = 0; i < Ressources::RI_COUNT; ++i) { infos.ressources[i] += quantity; infos.ressources[i] = max(infos.ressources[i], 0.0f); } }
#endif

	// Accesseurs inline
	const EcoWorldInfos& getInfos() const					{ return infos; }
	const EcoWorldModifiers& getModifiers() const			{ return modifiers; }
	const ::Time& getTime() const							{ return time; }
	const WeatherManager& getWeatherManager() const			{ return weatherManager; }
	WeatherManager& getWeatherManager()						{ return weatherManager; }
	const Objectives& getObjectives() const					{ return objectives; }		// Obtient la liste des objectifs d�finis pour la partie en cours
	Objectives& getObjectives()								{ return objectives; }		// Obtient la liste des objectifs d�finis pour la partie en cours

	// Obtient la production d'une certaine ressource : La quantit� de ressources produites/consomm�es en une journ�e (recalcul�e � chaque actualisation du monde)
	float getRessourcesProduction(Ressources::RessourceID ressourceID) const { if (ressourceID >= 0 && ressourceID < Ressources::RI_COUNT) { return ressourcesProduites[ressourceID]; } return 0.0f; }

	// Obtient la liste de tous les b�timents du jeu
	const core::list<Batiment*>& getAllBatiments() const	{ return listeAllBats; }

	// Obtient le renderer du syst�me de jeu
	const ISystemRenderer* getSystemRenderer() const		{ return systemRenderer; }

	// Indique le renderer du syst�me de jeu
	void setSystemRenderer(ISystemRenderer* renderer)		{ systemRenderer = renderer; }

	// La carte du monde
	TerrainInfo carte[TAILLE_CARTE][TAILLE_CARTE];



	// Fonctions inline

	// Ces trois fonctions ont un algorithme de calcul qui peut sembler complexe, mais qui a �t� longtemps r�fl�chi et est actuellement certifi� (06/04/11).
	// Elles fonctionnent seulement si TAILLE_CARTE est pair !

	// Retourne une position en coordonn�es 3D arrondie � TAILLE_OBJETS en X et en Z suivant une position en coordonn�es 3D
	static core::vector3df getRoundPosition(const core::vector3df& position)
	{
		const float INV_TAILLE_OBJETS = 1.0f / TAILLE_OBJETS;

		return core::vector3df(
			(core::floor32(position.X * INV_TAILLE_OBJETS + 1.0f) - 0.5f) * TAILLE_OBJETS,
			position.Y,
			(core::floor32(position.Z * INV_TAILLE_OBJETS + 1.0f) - 0.5f) * TAILLE_OBJETS);
	}
	// Retourne un index en coordonn�es de la carte suivant une position en coordonn�es 3D
	static core::vector2di getIndexFromPosition(const core::vector3df& position)
	{
		// -> Valide avec TAILLE_CARTE divisible par 2 (obligatoire !)

		const float INV_TAILLE_OBJETS = 1.0f / TAILLE_OBJETS;

		// On n'oublie pas d'ajouter la moiti� de la taille de la carte � l'index car une partie des positions est dans les valeurs n�gatives
		return core::vector2di(
			(int)(core::floor32(position.X * INV_TAILLE_OBJETS + 1.0f) + (TAILLE_CARTE - 2) * 0.5f),
			(int)(core::floor32(position.Z * INV_TAILLE_OBJETS + 1.0f) + (TAILLE_CARTE - 2) * 0.5f));
	}
	// Retourne une position en coordonn�es 3D arrondie � TAILLE_OBJETS en X et en Z de la carte suivant un index en coordonn�es de la carte (la position en Y sera �gale � 0.0f)
	static core::vector3df getPositionFromIndex(const core::vector2di& index)
	{
		// -> Valide avec TAILLE_CARTE divisible par 2 (obligatoire !)

		// On n'oublie pas de soustraire la moiti� de la taille de la carte � l'index
		return core::vector3df(
			((float)index.X - (TAILLE_CARTE - 1) * 0.5f) * TAILLE_OBJETS,
			0.0f,
			((float)index.Y - (TAILLE_CARTE - 1) * 0.5f) * TAILLE_OBJETS);
	}
};

#endif
