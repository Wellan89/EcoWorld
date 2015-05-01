#ifndef DEF_ECO_WORLD_MODIFIERS
#define DEF_ECO_WORLD_MODIFIERS

#include "global.h"

// Classe permettant de modifier la diffilcut� du jeu en augmentant ou diminuant proportionnellement certains facteurs du jeu
class EcoWorldModifiers
{
public:
	// Les difficult�s pr�d�finies du jeu, ainsi que la difficult� personnalis�e
	enum E_DIFFICULTY
	{
		ED_facile,
		ED_normal,
		ED_difficile,

		ED_personnalisee	// Difficult� personnalis�e (utilis�e pour indiquer une difficult� inconnue durant le chargement du jeu)
	} difficulty;

	// Constructeur
	EcoWorldModifiers();

	// Modifie la difficult� du jeu
	void setDifficulty(E_DIFFICULTY newDifficulty);

	// Enregistre les donn�es de ces modifiers dans un fichier
	void save(io::IAttributes* out, io::IXMLWriter* writer) const;

	// Charge les donn�es de ces modifiers � partir d'un fichier
	void load(io::IAttributes* in, io::IXMLReader* reader);

	float prixFactor;					// Ce facteur s'applique aux prix de construction et de destruction des batiments
	float loyerFactor;					// Ce facteur s'applique au loyer des habitations
	float taxesFactor;					// Ce facteur s'applique aux taxes de l'effet de serre et des d�chets
	float energieFactor;				// Ce facteur augmente l'�nergie produite par les batiments (il n'augmente pas l'�nergie consomm�e !)
	float effetSerreFactor;				// Ce facteur s'applique � l'augmentation de l'effet de serre
	float dechetsFactor;				// Ce facteur s'applique � l'augmentation des d�chets
	float ressourcesConsommationFactor;	// Ce facteur s'applique aux ressources consomm�es par les habitants
	float ressourcesProductionFactor;	// Ce facteur s'applique aux ressources produites par jour par les usines (il n'augmente pas la consommation d'autres ressources des usines utilis�es pour produire cette ressource)
	float constructionTimeFactor;		// Ce facteur s'applique aux temps de construction/destruction des b�timents
	float dureeVieFactor;				// Ce facteur s'applique � la dur�e de vie des b�timents
	float unlockPopulationNeededFactor;	// Ce facteur s'applique � la population minimale pour d�bloquer les b�timents
	float minSatisfactionFactor;		// Ce facteur s'applique � la satisfaction minimale avant que les habitants ne d�m�nagent des maisons
	float startBudgetFactor;			// Ce facteur s'applique au budget de d�part du joueur
	float startRessourcesFactor;		// Ce facteur s'applique aux ressources de d�part du joueur
};

#endif
