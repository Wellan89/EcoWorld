#ifndef DEF_OBJECTIVES
#define DEF_OBJECTIVES

#include "global.h"
#include "Batiments.h"

class EcoWorldSystem;
struct ObjectiveData;

// Classe permettant de g�rer les objectifs d'une partie
class Objectives
{
public:
	// Enum indiquant les diff�rents types d'objectifs disponibles et leur description
	enum E_OBJECTIVE_TYPE
	{
		EOT_BUDGET,				// Le budget du joueur doit v�rifier :			budget 'comparisonOp' dataF		pour que cette condition soit v�rifi�e
		EOT_EFFET_SERRE,		// L'effet de serre du joueur doit v�rifier :	ES 'comparisonOp' dataF			pour que cette condition soit v�rifi�e
		EOT_DECHETS,			// Les d�chets du joueur doivent v�rifier :		d�chets 'comparisonOp' dataF	pour que cette condition soit v�rifi�e
		EOT_TIME_PASSED,		// Le temps �coul� du syst�me doit v�rifier :	temps 'comparisonOp' dataF		pour que cette condition soit v�rifi�e

		EOT_BATIMENT_NB,		// Le nombre de b�timents du joueur du type dataID doivent v�rifier :	nbBatsID 'comparisonOp' dataF	pour que cette condition soit v�rifi�e
	};

	// Enum permettant de d�finir les diff�rents op�rateurs de comparaison pour les objectifs
	enum E_COMPARISON_OPERATOR
	{
		ECO_EQUALS,			// ==
		ECO_DIFFERENT,		// !=
		ECO_LESS_EQUALS,	// <=
		ECO_LESS,			// <
		ECO_MORE_EQUALS,	// >=
		ECO_MORE,			// >
	};

	// Retourne un op�rateur de comparaison sous forme de cha�ne de caract�re
	// masculin est utilis� pour conna�tre le genre de l'objet compar�
	static core::stringw getComparisonOperatorStr(E_COMPARISON_OPERATOR comparisonOp, bool masculin, bool singulier);

	// Constructeur
	Objectives(EcoWorldSystem* m_system = NULL);

	// R�initialise et efface tous les objectifs
	void reset();

	// Met � jour les objectifs actuels et d�termine si le jeu est gagn� ou perdu
	void update();

	// Ajoute un objectif � cette partie :
	// type :			Le type d'objectif � remplir
	// winObjective :	Si true, cet objectif devra �tre compl�t� pour que la partie puisse �tre gagn�e ; Si false, la partie sera perdue si cet objectif n'est pas constamment v�rifi�

	// Pour les param�tres suivants (parfois facultatifs), se r�f�rer � la description de l'objectif dans E_OBJECTIFS_TYPE pour savoir quels param�tres envoyer lors de l'utilisation de ces objectifs :
	// comparisonOp :	L'op�rateur de comparaison � utiliser pour comparer la valeur syst�me r�elle de cet objectif � sa valeur n�cessaire objectiveDataF
	// data* :			Diff�rents param�tres suppl�mentaires qui seront envoy�s � cet objectif
	void addObjective(E_OBJECTIVE_TYPE type, bool winObjective, E_COMPARISON_OPERATOR comparisonOp = ECO_MORE_EQUALS, float dataF = 0.0f, BatimentID dataID = BI_aucun);

	// Ajoute un objectif � cette partie
	void addObjective(const ObjectiveData& objective);

	// Ajoute une liste d'objectifs � cette partie
	void addObjectives(const core::list<ObjectiveData> objectivesList);

	// V�rifie si un objectif est v�rifi�
	bool verifyObjective(const ObjectiveData& objective) const;

	// Charge les donn�es des objectifs � partir d'un fichier
	void load(io::IAttributes* in, io::IXMLReader* reader);

	// Enregistre les donn�es des objectifs dans un fichier
	void save(io::IAttributes* out, io::IXMLWriter* writer) const;

protected:
	// Un pointeur vers le syst�me principal de jeu
	EcoWorldSystem* system;

	// Les variables permettant de se souvenir si le jeu est gagn� ou perdu depuis la derni�re mise � jour du syst�me
	bool gameWon;
	bool gameLost;

	bool continueGame;	// True si le joueur a demand� � continuer la partie en cours m�me si elle est gagn�e, false sinon

	// La liste des objectifs � v�rifier pour cette partie
	core::list<ObjectiveData> objectives;

	// Compare deux valeurs sous forme de float d'apr�s un op�rateur de comparaison
	static bool compareValues(float data1, float data2, E_COMPARISON_OPERATOR comparisonOp);

public:
	// Accesseurs inline :

	// Modifie le syst�me de jeu actuel
	void setSystem(EcoWorldSystem* m_system)				{ system = m_system; }

	// La liste des objectifs pour cette partie
	const core::list<ObjectiveData>& getObjectives() const	{ return objectives; }

	// Retourne si la partie est gagn�e pour le joueur
	bool isGameWon() const									{ return gameWon; }

	// Retourne si la partie est perdue pour le joueur
	bool isGameLost() const									{ return gameLost; }

	// Retourne si la partie en cours est termin�e
	// TODO : Permettre au joueur de continuer la partie en cours m�me si elle est gagn�e (mais pas si elle est perdue)
	bool isGameFinished() const								{ return ((gameWon && !continueGame) || gameLost); }

	// Indique si le joueur veut continuer la partie en cours, m�me si elle est gagn�e
	void setContinueGame(bool continueG)					{ continueGame = continueG; }
};

// Structure permettant de stocker les informations d'un objectif
struct ObjectiveData
{
	// Les donn�es sur cet objectif
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

	// Op�rateur d'assignement
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

	// La description de cet objectif sous forme de cha�ne de caract�re
	core::stringw getObjectiveDescription() const;
};

#endif
