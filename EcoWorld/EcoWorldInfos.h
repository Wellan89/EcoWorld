#ifndef DEF_ECO_WORLD_INFOS
#define DEF_ECO_WORLD_INFOS



// La satisfaction r�elle minimale n�cessaire avant que les habitants ne d�m�nagent des maisons (en pourcentage) :
// Les vitesses d'emm�nagement et de d�m�nagement des habitants sont maintenant aussi d�pendantes de leur satisfaction r�elle (plus ils sont satisfaits, plus ils emm�nagent vite, et inversement pour leur d�m�nagement).
// En-dessous de 20% de satisfaction, les habitants d�m�nagent des maisons. Le seuil de satisfaction (20% par d�faut) est d�pendant de la difficult� du jeu (de 10% en mode Facile � 40% en mode Difficile actuellement).
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

	// R�initialise toutes les informations du monde
	void reset();

	// R�initialise les �volutions du monde pour qu'elles soient recalcul�es (une seule fois par frame, avant la mise � jour des b�timents !)
	void resetEvolutions();

	// Met � jour la population et l'�nergie, et met aussi � jour la satisfaction r�elle de la population suivant le pourcentage d'�nergie disponible
	void updateDonnees();

	// Met � jour les taxes sur l'effet de serre et les d�chets
	// A chaque appel de cette fonction, on consid�rera qu'un jour suppl�mentaire s'est �coul�
	void updateTaxes();

	// Met � jour les ressources consomm�es par les habitants
	// A chaque appel de cette fonction, on consid�rera qu'un jour suppl�mentaire s'est �coul�
	void updateRessourcesConsommation();

	void load(io::IAttributes* in, io::IXMLReader* reader);
	void save(io::IAttributes* out, io::IXMLWriter* writer) const;

	float budget;
	float budgetEvolutionM;
	float budgetEvolutionA;
	float energie;						// Le niveau d'�nergie actuel
	float pourcentageEnergieDisponible;	// Le pourcentage d'�nergie disponible pour les b�timents utilisant de l'�nergie
	float effetSerre;
	float effetSerreEvolutionM;
	float dechets;
	float dechetsEvolutionM;

	u32 population;
	float popSatisfaction;				// La satisfaction de la population d�e uniquement aux ressources fournies aux habitants (augmentable en assouvissant leurs besoins)
	float popRealSatisfaction;			// La satisfaction r�elle de la population d�e aussi au poucentage d'�nergie disponible fourni aux maisons (facteur entre *0.5 � 0% d'�nergie � *1 � 100% d'�nergie)

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
