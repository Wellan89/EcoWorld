#ifndef DEF_OBJECTIVES
#define DEF_OBJECTIVES

#include "global.h"
#include "Batiments.h"

class EcoWorldSystem;
struct ObjectiveData;

// Classe permettant de gérer les objectifs d'une partie
class Objectives
{
public:
	// Enum indiquant les différents types d'objectifs disponibles et leur description
	enum E_OBJECTIVE_TYPE
	{
		EOT_BUDGET,				// Le budget du joueur doit vérifier :			budget 'comparisonOp' dataF		pour que cette condition soit vérifiée
		EOT_EFFET_SERRE,		// L'effet de serre du joueur doit vérifier :	ES 'comparisonOp' dataF			pour que cette condition soit vérifiée
		EOT_DECHETS,			// Les déchets du joueur doivent vérifier :		déchets 'comparisonOp' dataF	pour que cette condition soit vérifiée
		EOT_TIME_PASSED,		// Le temps écoulé du système doit vérifier :	temps 'comparisonOp' dataF		pour que cette condition soit vérifiée

		EOT_BATIMENT_NB,		// Le nombre de bâtiments du joueur du type dataID doivent vérifier :	nbBatsID 'comparisonOp' dataF	pour que cette condition soit vérifiée
	};

	// Enum permettant de définir les différents opérateurs de comparaison pour les objectifs
	enum E_COMPARISON_OPERATOR
	{
		ECO_EQUALS,			// ==
		ECO_DIFFERENT,		// !=
		ECO_LESS_EQUALS,	// <=
		ECO_LESS,			// <
		ECO_MORE_EQUALS,	// >=
		ECO_MORE,			// >
	};

	// Retourne un opérateur de comparaison sous forme de chaîne de caractère
	// masculin est utilisé pour connaître le genre de l'objet comparé
	static core::stringw getComparisonOperatorStr(E_COMPARISON_OPERATOR comparisonOp, bool masculin, bool singulier);

	// Constructeur
	Objectives(EcoWorldSystem* m_system = NULL);

	// Réinitialise et efface tous les objectifs
	void reset();

	// Met à jour les objectifs actuels et détermine si le jeu est gagné ou perdu
	void update();

	// Ajoute un objectif à cette partie :
	// type :			Le type d'objectif à remplir
	// winObjective :	Si true, cet objectif devra être complété pour que la partie puisse être gagnée ; Si false, la partie sera perdue si cet objectif n'est pas constamment vérifié

	// Pour les paramêtres suivants (parfois facultatifs), se référer à la description de l'objectif dans E_OBJECTIFS_TYPE pour savoir quels paramêtres envoyer lors de l'utilisation de ces objectifs :
	// comparisonOp :	L'opérateur de comparaison à utiliser pour comparer la valeur système réelle de cet objectif à sa valeur nécessaire objectiveDataF
	// data* :			Différents paramêtres supplémentaires qui seront envoyés à cet objectif
	void addObjective(E_OBJECTIVE_TYPE type, bool winObjective, E_COMPARISON_OPERATOR comparisonOp = ECO_MORE_EQUALS, float dataF = 0.0f, BatimentID dataID = BI_aucun);

	// Ajoute un objectif à cette partie
	void addObjective(const ObjectiveData& objective);

	// Ajoute une liste d'objectifs à cette partie
	void addObjectives(const core::list<ObjectiveData> objectivesList);

	// Vérifie si un objectif est vérifié
	bool verifyObjective(const ObjectiveData& objective) const;

	// Charge les données des objectifs à partir d'un fichier
	void load(io::IAttributes* in, io::IXMLReader* reader);

	// Enregistre les données des objectifs dans un fichier
	void save(io::IAttributes* out, io::IXMLWriter* writer) const;

protected:
	// Un pointeur vers le système principal de jeu
	EcoWorldSystem* system;

	// Les variables permettant de se souvenir si le jeu est gagné ou perdu depuis la dernière mise à jour du système
	bool gameWon;
	bool gameLost;

	bool continueGame;	// True si le joueur a demandé à continuer la partie en cours même si elle est gagnée, false sinon

	// La liste des objectifs à vérifier pour cette partie
	core::list<ObjectiveData> objectives;

	// Compare deux valeurs sous forme de float d'après un opérateur de comparaison
	static bool compareValues(float data1, float data2, E_COMPARISON_OPERATOR comparisonOp);

public:
	// Accesseurs inline :

	// Modifie le système de jeu actuel
	void setSystem(EcoWorldSystem* m_system)				{ system = m_system; }

	// La liste des objectifs pour cette partie
	const core::list<ObjectiveData>& getObjectives() const	{ return objectives; }

	// Retourne si la partie est gagnée pour le joueur
	bool isGameWon() const									{ return gameWon; }

	// Retourne si la partie est perdue pour le joueur
	bool isGameLost() const									{ return gameLost; }

	// Retourne si la partie en cours est terminée
	// TODO : Permettre au joueur de continuer la partie en cours même si elle est gagnée (mais pas si elle est perdue)
	bool isGameFinished() const								{ return ((gameWon && !continueGame) || gameLost); }

	// Indique si le joueur veut continuer la partie en cours, même si elle est gagnée
	void setContinueGame(bool continueG)					{ continueGame = continueG; }
};

// Structure permettant de stocker les informations d'un objectif
struct ObjectiveData
{
	// Les données sur cet objectif
	Objectives::E_OBJECTIVE_TYPE type;
	bool winObjective;
	Objectives::E_COMPARISON_OPERATOR comparisonOp;
	float dataF;
	BatimentID dataID;

	bool isFulfilled;	// True si cet objectif est actuellement rempli

	// Constructeur
	ObjectiveData(Objectives::E_OBJECTIVE_TYPE Type = Objectives::EOT_BUDGET, bool WinObjective = true, Objectives::E_COMPARISON_OPERATOR ComparisonOp = Objectives::ECO_MORE_EQUALS, float DataF = 0.0f, BatimentID DataID = BI_aucun)
		: type(Type), winObjective(WinObjective), comparisonOp(ComparisonOp), dataF(DataF), dataID(DataID), isFulfilled(false)
	{ }

	// Opérateur d'assignement
	ObjectiveData& operator=(const ObjectiveData& other)
	{
		type = other.type;
		winObjective = other.winObjective;
		comparisonOp = other.comparisonOp;
		dataF = other.dataF;
		dataID = other.dataID;
		isFulfilled = other.isFulfilled;
		return (*this);
	}

	// La description de cet objectif sous forme de chaîne de caractère
	core::stringw getObjectiveDescription() const;
};

#endif
