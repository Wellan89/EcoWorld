#ifndef DEF_ECO_WORLD_MODIFIERS
#define DEF_ECO_WORLD_MODIFIERS

#include "global.h"

// Classe permettant de modifier la diffilcuté du jeu en augmentant ou diminuant proportionnellement certains facteurs du jeu
class EcoWorldModifiers
{
public:
	// Les difficultés prédéfinies du jeu, ainsi que la difficulté personnalisée
	enum E_DIFFICULTY
	{
		ED_facile,
		ED_normal,
		ED_difficile,

		ED_personnalisee	// Difficulté personnalisée (utilisée pour indiquer une difficulté inconnue durant le chargement du jeu)
	} difficulty;

	// Constructeur
	EcoWorldModifiers();

	// Modifie la difficulté du jeu
	void setDifficulty(E_DIFFICULTY newDifficulty);

	// Enregistre les données de ces modifiers dans un fichier
	void save(io::IAttributes* out, io::IXMLWriter* writer) const;

	// Charge les données de ces modifiers à partir d'un fichier
	void load(io::IAttributes* in, io::IXMLReader* reader);

	float prixFactor;					// Ce facteur s'applique aux prix de construction et de destruction des batiments
	float loyerFactor;					// Ce facteur s'applique au loyer des habitations
	float taxesFactor;					// Ce facteur s'applique aux taxes de l'effet de serre et des déchets
	float energieFactor;				// Ce facteur augmente l'énergie produite par les batiments (il n'augmente pas l'énergie consommée !)
	float effetSerreFactor;				// Ce facteur s'applique à l'augmentation de l'effet de serre
	float dechetsFactor;				// Ce facteur s'applique à l'augmentation des déchets
	float ressourcesConsommationFactor;	// Ce facteur s'applique aux ressources consommées par les habitants
	float ressourcesProductionFactor;	// Ce facteur s'applique aux ressources produites par jour par les usines (il n'augmente pas la consommation d'autres ressources des usines utilisées pour produire cette ressource)
	float constructionTimeFactor;		// Ce facteur s'applique aux temps de construction/destruction des bâtiments
	float dureeVieFactor;				// Ce facteur s'applique à la durée de vie des bâtiments
	float unlockPopulationNeededFactor;	// Ce facteur s'applique à la population minimale pour débloquer les bâtiments
	float minSatisfactionFactor;		// Ce facteur s'applique à la satisfaction minimale avant que les habitants ne déménagent des maisons
	float startBudgetFactor;			// Ce facteur s'applique au budget de départ du joueur
	float startRessourcesFactor;		// Ce facteur s'applique aux ressources de départ du joueur
};

#endif
