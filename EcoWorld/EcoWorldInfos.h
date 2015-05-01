#ifndef DEF_ECO_WORLD_INFOS
#define DEF_ECO_WORLD_INFOS



// La satisfaction réelle minimale nécessaire avant que les habitants ne déménagent des maisons (en pourcentage) :
// Les vitesses d'emménagement et de déménagement des habitants sont maintenant aussi dépendantes de leur satisfaction réelle (plus ils sont satisfaits, plus ils emménagent vite, et inversement pour leur déménagement).
// En-dessous de 20% de satisfaction, les habitants déménagent des maisons. Le seuil de satisfaction (20% par défaut) est dépendant de la difficulté du jeu (de 10% en mode Facile à 40% en mode Difficile actuellement).
#define HABITANTS_MIN_SATISFACTION	0.2f	// 20% de satisfaction minimale



#include "global.h"
#include "Ressources.h"

class EcoWorldSystem;
class EcoWorldModifiers;

// Les informations globales sur le monde actuel
class EcoWorldInfos
{
public:
	enum TaxeID
	{
		TI_effetSerre,
		TI_dechets,

		TI_COUNT
	};

	// Constructeur
	EcoWorldInfos(EcoWorldSystem* m_system = NULL, EcoWorldModifiers* m_modifiers = NULL);

	// Réinitialise toutes les informations du monde
	void reset();

	// Réinitialise les évolutions du monde pour qu'elles soient recalculées (une seule fois par frame, avant la mise à jour des bâtiments !)
	void resetEvolutions();

	// Met à jour la population et l'énergie, et met aussi à jour la satisfaction réelle de la population suivant le pourcentage d'énergie disponible
	void updateDonnees();

	// Met à jour les taxes sur l'effet de serre et les déchets
	// A chaque appel de cette fonction, on considèrera qu'un jour supplémentaire s'est écoulé
	void updateTaxes();

	// Met à jour les ressources consommées par les habitants
	// A chaque appel de cette fonction, on considèrera qu'un jour supplémentaire s'est écoulé
	void updateRessourcesConsommation();

	void load(io::IAttributes* in, io::IXMLReader* reader);
	void save(io::IAttributes* out, io::IXMLWriter* writer) const;

	float budget;
	float budgetEvolutionM;
	float budgetEvolutionA;
	float energie;						// Le niveau d'énergie actuel
	float pourcentageEnergieDisponible;	// Le pourcentage d'énergie disponible pour les bâtiments utilisant de l'énergie
	float effetSerre;
	float effetSerreEvolutionM;
	float dechets;
	float dechetsEvolutionM;

	u32 population;
	float popSatisfaction;				// La satisfaction de la population dûe uniquement aux ressources fournies aux habitants (augmentable en assouvissant leurs besoins)
	float popRealSatisfaction;			// La satisfaction réelle de la population dûe aussi au poucentage d'énergie disponible fourni aux maisons (facteur entre *0.5 à 0% d'énergie à *1 à 100% d'énergie)

	float ressources[Ressources::RI_COUNT];

	float taxes[TI_COUNT];
	float taxesTotales;

protected:
	EcoWorldSystem* system;
	EcoWorldModifiers* modifiers;

public:
	void setSystem(EcoWorldSystem* m_system) { system = m_system; }
	void setModifiers(EcoWorldModifiers* m_modifiers) { modifiers = m_modifiers; }
};

#endif
