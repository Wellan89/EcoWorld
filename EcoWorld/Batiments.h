#ifndef DEF_BATIMENTS
#define DEF_BATIMENTS

#include "global.h"
#include "Ressources.h"
#include "EcoWorldModifiers.h"

// Les différents identificateurs (ID) des batiments
enum BatimentID
{
	// Attention :
	// L'ordre des ID (leur valeur numérique) influe aussi sur l'ordre de mise à jour des bâtiments lors de la mise à jour du système,
	// il faut donc veiller à placer judicieusement ce nouvel ID dans la liste
	// (par exemple, l'ID de la décharge doit être inférieur à l'ID de l'usine d'incinération des déchets,
	// car la décharge doit traîter les déchets avant l'usine d'incinération des déchets (donc être mise à jour avant) : cette dernière pollue plus pour un effet identique)

	BI_aucun,

#ifndef KID_VERSION	// En mode enfant : on désactive la maison de base et les routes qui sont inutiles
	// Tests
	BI_maison,
	BI_route,
#endif

	// Maisons
	BI_maison_individuelle,
	BI_maison_basse_consommation,
	BI_maison_avec_panneaux_solaires,
	BI_grande_maison_individuelle,
	BI_chalet,

	// Immeubles
	BI_immeuble_individuel,
	BI_immeuble_basse_consommation,
	BI_immeuble_avec_panneaux_solaires,
	BI_grand_immeuble_individuel,

	// Buildings
	BI_building_individuel,
	BI_building_basse_consommation,
	BI_building_avec_panneaux_solaires,
	BI_grand_building_individuel,

	// Production d'énergie
	BI_hydrolienne,
	BI_eolienne,
	BI_panneau_solaire,
	BI_centrale_charbon,

	// Production d'eau
	BI_pompe_extraction_eau,

	// Usines
	BI_usine_verre_grande,	// Verrerie : Usine de verre (grande)
	BI_usine_verre_petite,	// Verrerie : Usine de verre (petite)
	BI_usine_ciment_grande,	// Cimenterie : Usine de ciment (grande)
	BI_usine_ciment_petite,	// Cimenterie : Usine de ciment (petite)
	BI_usine_tuiles_grande,	// Tuilerie : Usine de tuiles (grande)
	BI_usine_tuiles_petite,	// Tuilerie : Usine de tuiles (petite)
#ifdef KID_VERSION	// En mode enfant : on réactiver le papier comme ressource consommée par les habitants pour leur satisfaction
	BI_usine_papier_grande,	// Papeterie : Usine de papier (grande)		// Désactivé en mode normal : Le papier a été supprimé de la liste des ressources
	BI_usine_papier_petite,	// Papeterie : Usine de papier (petite)		// Désactivé en mode normal : Le papier a été supprimé de la liste des ressources
#else					// En mode enfant : On désactive l'usine à tout faire qui est devenue inutile
	BI_usine_tout,			// Usine de test : fabrique toutes les ressources
#endif

	// Gestion de l'effet de serre et des déchets
	BI_decharge,
	BI_usine_incineration_dechets,

	// Arbres
	BI_arbre_aspen,			// Tremble
	BI_arbre_oak,			// Chêne
	BI_arbre_pine,			// Pin
	BI_arbre_willow,		// Saule

	BI_COUNT
};



// La rentabilité de la centrale à charbon : Electricité produite (en 1 jour) = CENTRALE_CHARBON_RENTABILITE * Bois consommé (en 1 jour)
#define CENTRALE_CHARBON_RENTABILITE				5.0f		// 5 WJ / kg de bois consommé
#define CENTRALE_CHARBON_RENTABILITE_OPP_INV		-0.2f		// CENTRALE_CHARBON_RENTABILITE_OPP_INV = -1 / CENTRALE_CHARBON_RENTABILITE
// La consommation en eau (en L/kg) par rapport au bois consommé
#define CENTRALE_CHARBON_CONSOMMATION_EAU_FACTEUR	0.25f
// Le bois maximal que peut transformer la centrale à charbon par jour
#define CENTRALE_CHARBON_BOIS_MAX					60000.0f
// L'électricité maximale que peut produire la centrale à charbon (donc par jour)
#define CENTRALE_CHARBON_ENERGIE_MAX				300000.0f	// CENTRALE_CHARBON_ENERGIE_MAX = CENTRALE_CHARBON_BOIS_MAX * CENTRALE_CHARBON_RENTABILITE

// La rentabilité de l'usine d'incinération des déchets : ES rejetée = USINE_INCINERATION_RENTABILITE * Déchets absorbés
#define USINE_INCINERATION_RENTABILITE				0.5f
// Les déchets maximaux que peut absorber l'usine d'incinération des déchets par jour
#define USINE_INCINERATION_DECHETS_MAX				500.0f
#define USINE_INCINERATION_DECHETS_MAX_OPP_INV		-0.002f		// USINE_INCINERATION_DECHETS_MAX_OPP_INV = -1 / USINE_INCINERATION_DECHETS_MAX

// La facture d'électricité (en € / WJ (1 WJ : Consommation d'1 Watt pendant 1 jour)) (avec TVA)
#define FACTURE_ELECTRICITE							(0.00192f / 3.0f)	// 0.00192 € / WM => (0.00192 / 3) € / WJ

// La facture d'eau (en € / L) (avec TVA)
#define FACTURE_EAU									0.0225f



// Contient les informations statiques (constantes) sur les batiments (leur "fiche d'identité" de base)
class StaticBatimentInfos
{
public:
	// Structure simple pour contenir l'ID d'une ressource et sa consommation
	struct RessourceConsommation
	{
		Ressources::RessourceID ID;	// L'ID de la ressource actuelle
		float production;			// Sa production/consommation (si produite : positif ; si consommée : négatif)

		RessourceConsommation(Ressources::RessourceID ressourceID = Ressources::RI_eau, float quantite = 0.0f) : ID(ressourceID), production(quantite) { }
	};

	// Ajoute la quantité spécifiée à la consommation de ressources avec l'ID spécifié, dans le tableau de ressources spécifié
	// Retourne la nouvelle consommation de cette ressource
	static float getRessourceConsommation(Ressources::RessourceID ressourceID, const core::array<RessourceConsommation>& ressources)
	{
		// Cherche la consommation de cette ressource, si elle existe
		float consommation = 0.0f;
		const u32 ressourcesSize = ressources.size();
		for (u32 i = 0; i < ressourcesSize; ++i)
			if (ressources[i].ID == ressourceID)
				consommation = ressources[i].production;

		// Et on retourne la consommation trouvée (0 si non trouvée)
		return consommation;
	}

	// Ajoute la quantité spécifiée à la consommation de ressources avec l'ID spécifié, dans le tableau de ressources spécifié
	static void addRessourceConsommation(float quantite, Ressources::RessourceID ressourceID, core::array<RessourceConsommation>& ressources)
	{
		if (quantite == 0.0f)
			return;

		// Cherche la consommation de cette ressource, si elle existe
		RessourceConsommation* ressPtr = NULL;
		const u32 ressourcesSize = ressources.size();
		for (u32 i = 0; i < ressourcesSize; ++i)
			if (ressources[i].ID == ressourceID)
				ressPtr = (&(ressources[i]));

		if (ressPtr)	// Si la consommation de cette ressource existe déjà, on ajoute directement la quantité spécifiée
			ressPtr->production += quantite;
		else			// Sinon, on crée la consommation de cette ressource avec la quantité spécifiée
			ressources.push_back(RessourceConsommation(ressourceID, quantite));
	}



	/*
	Signification des lettres après les variables :
	- C : Une seule fois (à la construction)
	- J : Tous les jours
	- M : Tous les mois
	- A : Tous les ans
	- D : Une seule fois (à la destruction)
	*/

	float prixC;		// Le prix de ce bâtiment à la construction								// (€)
	float effetSerreC;	// L'effet de serre rejetée à la construction de ce bâtiment			// (kg)
	float dechetsC;		// Les dechets rejetés à la construction de ce bâtiment					// (kg)

	u32 unlockPopulationNeeded;	// La population minimale du monde nécessaire pour débloquer la construction de ce bâtiment au joueur	// (Personnes)

	float prixD;		// Le prix de la destruction de ce bâtiment								// (€)
	float effetSerreD;	// L'effet de serre rejetée à la destruction de ce bâtiment				// (kg)
	float dechetsD;		// Les dechets rejetés à la destruction de ce bâtiment					// (kg)

	float energie;			// L'énergie nécessaire pour que ce bâtiment fonctionne à 100%		// (W)
	float eauConsommationJ;	// L'eau nécessaire à ce bâtiment par jour							// (L)

	float effetSerreJ;		// L'effet de serre moyen que rejette ce bâtiment par jour			// (kg)
	float dechetsJ;			// Les déchets moyens que rejette ce bâtiment par jour				// (kg)

	float entretienM;		// L'entretien moyen que coûte ce bâtiment par mois					// (€)

	float loyerM;			// Le loyer moyen que rapporte ce bâtiment par mois (réservé aux habitations !)		// (€)
	float impotsA;			// Les impôts moyens que rapporte ce bâtiment par mois (réservé aux habitations !)	// (€)

	u32 habitantsC;			// Le nombre d'habitant que contient ce bâtiment à sa construction																		// (Personnes)
	u32 habitantsMax;		// Le nombre d'habitant maximal que peut contenir ce bâtiment																			// (Personnes)
	float habitantsJ;		// La progression de l'arrivée du prochain habitant par jour : un nouvel habitant arrive tous les 1/habitantsJ jours à vitesse maximale	// (Pourcent entre 0 et 1)
							// Cela détermine aussi la vitesse de déménagement des habitants (elle est égale à cette vitesse d'emménagenemt)
							// De plus, ces deux vitesses sont dépendantes de la satisfaction des habitants (plus ils sont satisfaits, plus ils emménagent vite, et inversement pour leur déménagement)

	core::array<RessourceConsommation> ressourcesC;	// Les ressources nécessaires à la construction de ce bâtiment	// Unité varie suivant le type de ressource
	core::array<RessourceConsommation> ressourcesJ;	// Les ressources consommées/produites par jour par ce bâtiment	// Unité varie suivant le type de ressource
	core::array<RessourceConsommation> ressourcesD;	// Les ressources nécessaires à la destruction de ce bâtiment	// Unité varie suivant le type de ressource

	u32 tempsC;				// Le temps de construction du bâtiment (si égal à 0 : instantané)	// (Jours)
	u32 tempsD;				// Le temps de destruction du bâtiment (si égal à 0 : instantané)	// (Jours)
	u32 dureeVie;			// La durée de vie moyenne du bâtiment (si égal à 0 : infinie)		// (Jours)
	u32 dureeVieVariation;	// La variation de la durée de vie du bâtiment : recalculée aléatoirement dans la plage [dureeVie - variation ; dureeVie + variation] (si égal à 0 : non aléatoire ; non utilisé si dureeVie = 0)	// (Jours)
							// Note : On doit toujours avoir : dureeVieVariation <= dureeVie

	bool isHabitation;		// Indique si ce bâtiment est considéré comme une habitation
	bool isTree;			// Indique si ce bâtiment est considéré comme un arbre
	bool isUsine;			// Indique si ce bâtiment est considéré comme une usine

	core::dimension2du taille;		// La taille du bâtiment en unités du système de jeu
	core::dimension2df halfTaille;	// La taille du bâtiment / 2 en unités du système de jeu (utilisé pour connaître les coins du bâtiment grâce à son centre)

	BatimentID ID;			// L'ID système de ce bâtiment
	const wchar_t* name;	// Le nom externe de ce bâtiment

protected:
	// Recalcule certaines valeurs d'après des fonctions numériques
	void calculateFunctionsData()
	{
		// Détermine si ce bâtiment est une habitation, un arbre ou une usine :
		isHabitation = (
#ifndef KID_VERSION	// En mode enfant : on désactive la maison de base et les routes qui sont inutiles
			ID == BI_maison ||
#endif
			ID == BI_maison_individuelle || ID == BI_maison_basse_consommation
			|| ID == BI_maison_avec_panneaux_solaires || ID == BI_grande_maison_individuelle || ID == BI_chalet
			|| ID == BI_immeuble_individuel || ID == BI_immeuble_basse_consommation
			|| ID == BI_immeuble_avec_panneaux_solaires || ID == BI_grand_immeuble_individuel
			|| ID == BI_building_individuel || ID == BI_building_basse_consommation
			|| ID == BI_building_avec_panneaux_solaires || ID == BI_grand_building_individuel);
		isTree = (ID == BI_arbre_aspen || ID == BI_arbre_oak || ID == BI_arbre_pine || ID == BI_arbre_willow);
		isUsine = (ID == BI_usine_verre_petite || ID == BI_usine_verre_grande
			|| ID == BI_usine_ciment_petite || ID == BI_usine_ciment_grande
			|| ID == BI_usine_tuiles_petite || ID == BI_usine_tuiles_grande
#ifdef KID_VERSION	// En mode enfant : on réactiver le papier comme ressource consommée par les habitants pour leur satisfaction
			|| ID == BI_usine_papier_petite || ID == BI_usine_papier_grande
#else				// En mode enfant : On désactive l'usine à tout faire qui est devenue inutile
			|| ID == BI_usine_tout
#endif
			);

		// Prix de destruction : 10% du prix de construction :
		prixD = prixC * 0.1f;

		// Temps de construction : 10 000 € du prix de construction = 1 mois de construction :	// abs(prixC) * 3.0f / 1000.0f
		tempsC = core::ceil32(abs(prixC) * 0.003f);

		// Temps de destruction : 10 000 € du prix de destruction = 1 mois de destruction :		// abs(prixD) * 3.0f / 1000.0f
		tempsD = core::ceil32(tempsC * 0.1f);	// Car prixD = prixC * 0.1f

		// Effet de serre (construction et destruction) :
		/*
			Description :
			5 ouvriers travaillent sur un chantier à la fois. Ils rejettent chacun 30 kg d'ES par mois de travail, soit en tout 150kg/M.
			On multiplie par 3 cette consommation pour les rejets des machines (arbitraire), ce qui fait donc :
					450 kg d'ES par mois du temps de construction !
			Ou :	15 kg d'ES par jour du temps de construction !

			Idem pour les rejets d'ES à la destruction, avec le temps de destruction.
			Or tempsD = tempsC / 10, donc effetSerreD = effetSerreC / 10.
		*/
		effetSerreC = tempsC * 15.0f;
		effetSerreD = effetSerreC * 0.1f;

		// Déchets et ressources : voir après les usines (elles ajoutent des ressources supplémentaires à la construction !)

		// Habitations :
		if (isHabitation)
		{
			const float habitantsMaxF = (float)habitantsMax;

			// Effet de serre par jour : 0.5 kg par personne et par jour :
			effetSerreJ = habitantsMaxF * 0.5f;

			// Déchets par jour : 1 kg par personne et par jour :
			// Référence (M6) : 390 kg de déchets par an par habitant en moyenne
			dechetsJ = habitantsMaxF;

			// Energie consommée : 1000 W par habitant :
			// (On vérifie tout de même que ce bâtiment n'a pas de panneaux solaires)
			if (ID != BI_maison_avec_panneaux_solaires && ID != BI_immeuble_avec_panneaux_solaires && ID != BI_building_avec_panneaux_solaires)
				energie = habitantsMaxF * -1000.0f;

			// Consommation d'eau par jour : 400 L par personne et par mois :
			eauConsommationJ = habitantsMaxF * 40.0f / 3.0f;

			// Loyer par mois : 200 € par habitant :
			loyerM = habitantsMaxF * 200.0f;

			// Impôts par an : 150% du loyer par mois :
			impotsA = loyerM * 1.5f;

			// Habitations basse consommation :
			if (ID == BI_maison_basse_consommation || ID == BI_immeuble_basse_consommation || ID == BI_building_basse_consommation)
			{
				// Energie consommée diminuée de 25% :
				energie *= 0.75f;

				// Consommation d'eau par jour diminuée de 25 % :
				eauConsommationJ *= 0.75f;
			}

			// Vérifie que les habitants présents dans ce bâtiment à sa construction sont d'au moins 1 habitant, puisque le nombre minimum d'habitants dans une maison est de 1 habitant
			habitantsC = max(habitantsC, (u32)1);
		}
		else
		{
			// Ces valeurs ne sont valides que pour les habitations :
			habitantsC = 0;
			habitantsMax = 0;
			habitantsJ = 0.0f;
			loyerM = 0.0f;
			impotsA = 0.0f;
		}

		// Usines :
		// Certains paramêtres des usines sont basés sur les informations de la maison individuelle :
		// on vérifie que ses informations ont bien été initialisées avec son ID
		if (isUsine)
		{
			// Détermine si cette usine est une grande usine
			const bool isGrandeUsine = (ID == BI_usine_verre_grande
				|| ID == BI_usine_ciment_grande || ID == BI_usine_tuiles_grande
#ifdef KID_VERSION	// En mode enfant : on réactiver le papier comme ressource consommée par les habitants pour leur satisfaction
				|| ID == BI_usine_papier_grande
#else				// En mode enfant : On désactive l'usine à tout faire qui est devenue inutile
				|| ID == BI_usine_tout
#endif
				);

			if (batimentInfos[BI_maison_individuelle].ID == BI_maison_individuelle)
			{
				const StaticBatimentInfos& maisonIndividuelleInfos = batimentInfos[BI_maison_individuelle];

				// Si cette usine est une grande usine, on applique des augmentations des prix et consommations par rapport aux petites usines :
				const float multiplierCD = (isGrandeUsine ? 2.0f : 1.0f);	// L'augmentation des prix à la construction/destruction
				const float multiplierJMA = (isGrandeUsine ? 2.5f : 1.0f);	// L'augmentation des consommations par jour/mois/an, ainsi que l'augmentation de l'énergie consommée

				// Prix à la construction : 150 000 € :
				prixC = -150000.0f * multiplierCD;

				// Entretien mensuel : 1 500 € :
				entretienM = 1500.0f * multiplierJMA;

				// Energie nécessaire : 20 000 W :
				energie = -20000.0f * multiplierJMA;

				// Effet de serre à la construction : Effet de serre à la construction de la maison individuelle * 5 :
				effetSerreC = maisonIndividuelleInfos.effetSerreC * 5.0f * multiplierCD;

				// Effet de serre par jour : Effet de serre par jour de la maison individuelle * 20 :
				effetSerreJ = maisonIndividuelleInfos.effetSerreJ * 20.0f * multiplierJMA;

				// Effet de serre à la destruction : Effet de serre à la destruction de la maison individuelle * 20 :
				effetSerreD = maisonIndividuelleInfos.effetSerreD * 20.0f * multiplierCD;

				// Déchets à la construction : Déchets à la construction de la maison individuelle * 3 :
				dechetsC = maisonIndividuelleInfos.dechetsC * 3.0f * multiplierCD;

				// Déchets par jour : Déchets par jour de la maison individuelle * 10 :
				dechetsJ = maisonIndividuelleInfos.dechetsJ * 10.0f * multiplierJMA;

				// Consommation d'eau par jour : Consommation d'eau par jour de la maison individuelle * 3 :
				eauConsommationJ = maisonIndividuelleInfos.eauConsommationJ * 3.0f * multiplierJMA;

				// Bois nécessaire à la construction : Bois nécessaire à la construction de la maison individuelle * 3 :
				addRessourceConsommation(3.0f * getRessourceConsommation(Ressources::RI_bois, maisonIndividuelleInfos.ressourcesC) * multiplierCD, Ressources::RI_bois, ressourcesC);

				// Verre nécessaire à la construction : Verre nécessaire à la construction de la maison individuelle * 3 :
				addRessourceConsommation(3.0f * getRessourceConsommation(Ressources::RI_verre, maisonIndividuelleInfos.ressourcesC) * multiplierCD, Ressources::RI_verre, ressourcesC);

				// Ciment nécessaire à la construction : Ciment nécessaire à la construction de la maison individuelle * 3 :
				addRessourceConsommation(3.0f * getRessourceConsommation(Ressources::RI_ciment, maisonIndividuelleInfos.ressourcesC) * multiplierCD, Ressources::RI_ciment, ressourcesC);

				// Tuiles nécessaire à la construction : Tuiles nécessaire à la construction de la maison individuelle * 1 :
				addRessourceConsommation(getRessourceConsommation(Ressources::RI_tuiles, maisonIndividuelleInfos.ressourcesC) * multiplierCD, Ressources::RI_tuiles, ressourcesC);

#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
				// Pierre nécessaire à la construction : Pierre nécessaire à la construction de la maison individuelle * 3 :
				addRessourceConsommation(3.0f * getRessourceConsommation(Ressources::RI_pierre, maisonIndividuelleInfos.ressourcesC) * multiplierCD, Ressources::RI_pierre, ressourcesC);

				// Sable nécessaire à la construction : Sable nécessaire à la construction de la maison individuelle * 3 :
				addRessourceConsommation(3.0f * getRessourceConsommation(Ressources::RI_sable, maisonIndividuelleInfos.ressourcesC) * multiplierCD, Ressources::RI_sable, ressourcesC);
#endif
			}

			// Détermine la population nécessaire, la durée de vie et la taille des usines :
			if (isGrandeUsine)	// Grandes usines
			{
				unlockPopulationNeeded = 150;

				dureeVie = 21600;			// 60 ans
				dureeVieVariation = 4320;	// ± 12 ans (20 %)

				taille.set(10, 10);
			}
			else				// Petites usines
			{
				unlockPopulationNeeded = 0;

				dureeVie = 14400;			// 40 ans
				dureeVieVariation = 2880;	// ± 8 ans (20 %)

				taille.set(5, 5);
			}

#ifndef KID_VERSION	// En mode enfant : On désactive l'usine à tout faire qui est devenue inutile
			// Informations spéciales pour l'usine à tout faire, considérée comme une grande usine :
			if (ID == BI_usine_tout)
			{
				entretienM = 10000.0f;
				unlockPopulationNeeded = 0;
			}
#endif
		}

		// Obtient le ciment nécessaire à la construction (nécessaire aux calcul de la consommation d'eau et de fer)
		const float cimentConsommationC = getRessourceConsommation(Ressources::RI_ciment, ressourcesC);

		// Eau nécessaire à la construction et à la destruction, ainsi que fer nécessaire à la construction :
		if (cimentConsommationC < 0.0f)	// Seulement valide pour les consommations de ciment !
		{
			// Eau nécessaire à la construction : 80% de la masse du ciment nécessaire à la construction :
			addRessourceConsommation(cimentConsommationC * 0.8f, Ressources::RI_eau, ressourcesC);

			// Eau nécessaire à la destruction : 10% de l'eau nécessaire à la construction : soit 8% de la masse du ciment nécessaire à la construction :
			addRessourceConsommation(cimentConsommationC * 0.08f, Ressources::RI_eau, ressourcesD);

#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
			// Fer nécessaire à la construction : 10% de la masse du ciment nécessaire à la construction :
			addRessourceConsommation(cimentConsommationC * 0.1f, Ressources::RI_fer, ressourcesC);
#endif
		}

		// Déchets à la destruction : Somme des ressources nécessaires à la construction, en excluant l'eau :
		dechetsD = 0.0f;
		const u32 ressourcesCSize = ressourcesC.size();
		for (u32 i = 0; i < ressourcesCSize; ++i)
			if (ressourcesC[i].ID != Ressources::RI_eau)
				if (ressourcesC[i].production < 0.0f)	// Vérifie que cette ressource est consommée à la construction
					dechetsD -= ressourcesC[i].production;

		// Déchets à la construction : 5% des déchets à la destruction :
		dechetsC = dechetsD * 0.05f;

		// Demi-taille du bâtiment :
		halfTaille = core::vector2df(taille.Width * 0.5f, taille.Height * 0.5f);
	}

	// Indique les informations constantes spécifiques à ce bâtiment suivant son ID
	void setID(BatimentID batimentID)
	{
		prixC = 0.0f;
		effetSerreC = 0.0f;
		dechetsC = 0.0f;

		unlockPopulationNeeded = 0;

		prixD = 0.0f;
		dechetsD = 0.0f;
		effetSerreD = 0.0f;

		energie = 0.0f;
		eauConsommationJ = 0.0f;

		effetSerreJ = 0.0f;
		dechetsJ = 0.0f;

		entretienM = 0.0f;
		loyerM = 0.0f;
		impotsA = 0.0f;

		habitantsC = 0;
		habitantsMax = 0;
		habitantsJ = 0.0f;

		ressourcesC.clear();
		ressourcesJ.clear();
		ressourcesD.clear();

		tempsC = 0;
		tempsD = 0;
		dureeVie = 0;
		dureeVieVariation = 0;

		isHabitation = false;
		isTree = false;
		isUsine = false;

		taille.set(1, 1);

		ID = batimentID;
		name = L"Bâtiment inconnu";

		if (batimentID == BI_aucun)
			return;



		// Indique les informations statiques sur ce bâtiment d'après son ID
		switch (batimentID)
		{
#ifndef KID_VERSION	// En mode enfant : on désactive la maison de base et les routes qui sont inutiles
			// Tests
		case BI_maison: // Permet d'augmenter considérablement les revenus du joueur
			prixC = -10000.0f;

			habitantsC = 10;
			habitantsMax = 100;
			habitantsJ = 1.0f;		// +1 habitant chaque jour

			dureeVie = 360;			// 1 an
			dureeVieVariation = 0;	// ± 0 ans (0 %) (exact)

			taille.set(1, 1);
			name = L"Maison de test";
			break;
		case BI_route:
			prixC = -100.0f;
			entretienM = 1.0f;

			dureeVie = 0;			// 0 ans (infini)
			dureeVieVariation = 0;	// ± 0 ans (0 %) (exact)

			taille.set(1, 1);
			name = L"Route";
			break;
#endif

			// Maisons
		case BI_maison_individuelle:
			prixC = -60000.0f;
			entretienM = 25.0f;		// 300.0f / 12.0f

			habitantsC = 2;
			habitantsMax = 4;
			habitantsJ = 0.2f;		// +1 habitant tous les 5 jours

			ressourcesC.reallocate(8);
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_bois,	-1150.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_verre,	-100.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_ciment,	-2900.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_tuiles,	-3000.0f));
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_pierre,	-12000.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_fer,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_sable,	-6800.0f));
#endif

			ressourcesD.reallocate(1);
			ressourcesD.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place

			dureeVie = 36000;			// 100 ans
			dureeVieVariation = 7200;	// ± 20 ans (20 %)

			taille.set(2, 1);
			name = L"Maison individuelle";
			break;
		case BI_maison_basse_consommation:
			prixC = -70000.0f;
			entretienM = 25.0f;		// 300.0f / 12.0f

			unlockPopulationNeeded = 10;

			habitantsC = 2;
			habitantsMax = 4;
			habitantsJ = 0.2f;		// +1 habitant tous les 5 jours

			ressourcesC.reallocate(8);
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_bois,	-1150.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_verre,	-100.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_ciment,	-2900.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_tuiles,	-3000.0f));
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_pierre,	-12000.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_fer,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_sable,	-6800.0f));
#endif

			ressourcesD.reallocate(1);
			ressourcesD.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place

			dureeVie = 36000;			// 100 ans
			dureeVieVariation = 7200;	// ± 20 ans (20 %)

			taille.set(2, 1);
			name = L"Maison basse consommation";
			break;
		case BI_maison_avec_panneaux_solaires:
			prixC = -80000.0f;
			entretienM = 25.0f;			// 300.0f / 12.0f

			unlockPopulationNeeded = 20;

			habitantsC = 2;
			habitantsMax = 4;
			habitantsJ = 0.2f;	// +1 habitant tous les 10 jours

			ressourcesC.reallocate(9);
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_bois,	-1150.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_verre,	-100.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_ciment,	-2900.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_tuiles,	-3000.0f));
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_pierre,	-12000.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_fer,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_sable,	-6800.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_panneauxPhotovoltaiques,	-7.0f));
#endif

			ressourcesD.reallocate(1);
			ressourcesD.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place

			dureeVie = 36000;			// 100 ans
			dureeVieVariation = 7200;	// ± 20 ans (20 %)

			taille.set(2, 1);
			name = L"Maison avec panneaux solaires";
			break;
		case BI_grande_maison_individuelle:
			prixC = -90000.0f;
			entretienM = 100.0f / 3.0f;	// 400.0f / 12.0f

			unlockPopulationNeeded = 30;

			habitantsC = 3;
			habitantsMax = 6;
			habitantsJ = 0.2f;	// +1 habitant tous les 5 jours

			ressourcesC.reallocate(8);
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_bois,	-1265.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_verre,	-180.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_ciment,	-3190.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_tuiles,	-3300.0f));
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_pierre,	-13200.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_fer,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_sable,	-7480.0f));
#endif

			ressourcesD.reallocate(1);
			ressourcesD.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place

			dureeVie = 36000;			// 100 ans
			dureeVieVariation = 7200;	// ± 20 ans (20 %)

			taille.set(2, 1);
			name = L"Grande maison individuelle";
			break;
		case BI_chalet:
			prixC = -75000.0f;
			entretienM = 25.0f;			 // 300.0f / 12.0f

			unlockPopulationNeeded = 20;

			habitantsC = 2;
			habitantsMax = 4;
			habitantsJ = 0.2f;	// +1 habitant tous les 5 jours

			ressourcesC.reallocate(7);
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_bois,	-9550.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_verre,	-100.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_ciment,	-500.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_tuiles,	-3000.0f));
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
			//ressourcesC.push_back(RessourceConsommation(Ressources::RI_pierre,	-0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_fer,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			//ressourcesC.push_back(RessourceConsommation(Ressources::RI_tavaillon,	-2100.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_sable,	-1000.0f));
#endif

			ressourcesD.reallocate(1);
			ressourcesD.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place

			dureeVie = 36000;			// 100 ans
			dureeVieVariation = 7200;	// ± 20 ans (20 %)

			taille.set(3, 2);	// TODO : Changer à (9, 4) / 3 lorsque le chalet aura son toit étendu sur les facades avant et arrière
								// TODO : Normalement : (9, 4) / 3 !
			name = L"Chalet";
			break;

			// Immeubles
		case BI_immeuble_individuel:
			prixC = -110000.0f;
			entretienM = 175.0f / 3.0f;	// 700.0f / 12.0f

			unlockPopulationNeeded = 50;

			habitantsC = 12;
			habitantsMax = 24;
			habitantsJ = 0.3f;	// +1 habitant tous les 3.33 jours

			ressourcesC.reallocate(7);
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_bois,	-390.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_verre,	-340.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_ciment,	-10700.0f));
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_pierre,	-36000.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_fer,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_sable,	-24400.0f));
#endif

			ressourcesD.reallocate(1);
			ressourcesD.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place

			dureeVie = 34200;			// 95 ans
			dureeVieVariation = 6840;	// ± 19 ans (20 %)

			taille.set(2, 1);
			name = L"Immeuble individuel";
			break;
		case BI_immeuble_basse_consommation:
			prixC = -125000.0f;
			entretienM = 175.0f / 3.0f;	// 700.0f / 12.0f

			unlockPopulationNeeded = 100;

			habitantsC = 12;
			habitantsMax = 24;
			habitantsJ = 0.3f;	// +1 habitant tous les 3.33 jours

			ressourcesC.reallocate(7);
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_bois,	-390.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_verre,	-340.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_ciment,	-10700.0f));
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_pierre,	-36000.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_fer,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_sable,	-24400.0f));
#endif

			ressourcesD.reallocate(1);
			ressourcesD.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place

			dureeVie = 34200;			// 95 ans
			dureeVieVariation = 6840;	// ± 19 ans (20 %)

			taille.set(2, 1);
			name = L"Immeuble basse consommation";
			break;
		case BI_immeuble_avec_panneaux_solaires:
			prixC = -135000.0f;
			entretienM = 175.0f / 3.0f;	// 700.0f / 12.0f

			unlockPopulationNeeded = 150;

			habitantsC = 12;
			habitantsMax = 24;
			habitantsJ = 0.3f;	// +1 habitant tous les 3.33 jours

			ressourcesC.reallocate(8);
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_bois,	-390.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_verre,	-340.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_ciment,	-10700.0f));
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_pierre,	-36000.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_fer,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_sable,	-24400.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_panneauxPhotovoltaiques,	-15.0f));
#endif

			ressourcesD.reallocate(1);
			ressourcesD.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place

			dureeVie = 34200;			// 95 ans
			dureeVieVariation = 6840;	// ± 19 ans (20 %)

			taille.set(2, 1);
			name = L"Immeuble avec panneaux solaires";
			break;
		case BI_grand_immeuble_individuel:
			prixC = -140000.0f;
			entretienM = 75.0f;			// 900.0f / 12.0f

			unlockPopulationNeeded = 200;

			habitantsC = 16;
			habitantsMax = 32;
			habitantsJ = 0.3f;	// +1 habitant tous les 3.33 jours

			ressourcesC.reallocate(7);
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_bois,	-510.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_verre,	-460.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_ciment,	-14100.0f));
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_pierre,	-48000.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_fer,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_sable,	-33360.0f));
#endif

			ressourcesD.reallocate(1);
			ressourcesD.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place

			dureeVie = 34200;			// 95 ans
			dureeVieVariation = 6840;	// ± 19 ans (20 %)

			taille.set(2, 1);
			name = L"Grand immeuble individuel";
			break;

			// Buildings
		case BI_building_individuel:
			prixC = -150000.0f;
			entretienM = 275.0f / 3.0f;	// 1100.0f / 12.0f

			unlockPopulationNeeded = 250;

			habitantsC = 20;
			habitantsMax = 40;
			habitantsJ = 0.4f;	// +1 habitant tous les 2.5 jours

			ressourcesC.reallocate(7);
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_bois,	-850.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_verre,	-800.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_ciment,	-19250.0f));
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_pierre,	-66000.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_fer,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_sable,	-44000.0f));
#endif

			ressourcesD.reallocate(1);
			ressourcesD.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place

			dureeVie = 32400;			// 90 ans
			dureeVieVariation = 6480;	// ± 18 ans (20 %)

			taille.set(4, 2);
			name = L"Building individuel";
			break;
		case BI_building_basse_consommation:
			prixC = -160000.0f;
			entretienM = 275.0f / 3.0f;	// 1100.0f / 12.0f

			unlockPopulationNeeded = 350;

			habitantsC = 20;
			habitantsMax = 40;
			habitantsJ = 0.4f;	// +1 habitant tous les 2.5 jours

			ressourcesC.reallocate(7);
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_bois,	-850.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_verre,	-800.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_ciment,	-19250.0f));
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_pierre,	-66000.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_fer,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_sable,	-44000.0f));
#endif

			ressourcesD.reallocate(1);
			ressourcesD.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place

			dureeVie = 32400;			// 90 ans
			dureeVieVariation = 6480;	// ± 18 ans (20 %)

			taille.set(4, 2);
			name = L"Building basse consommation";
			break;
		case BI_building_avec_panneaux_solaires:
			prixC = -170000.0f;
			entretienM = 275.0f / 3.0f;	// 1100.0f / 12.0f

			unlockPopulationNeeded = 450;

			habitantsC = 20;
			habitantsMax = 40;
			habitantsJ = 0.4f;	// +1 habitant tous les 2.5 jours

			ressourcesC.reallocate(8);
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_bois,	-830.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_verre,	-780.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_ciment,	-19550.0f));
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_pierre,	-69000.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_fer,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_sable,	-44850.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_panneauxPhotovoltaiques,	-57.0f));
#endif

			ressourcesD.reallocate(1);
			ressourcesD.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place

			dureeVie = 32400;			// 90 ans
			dureeVieVariation = 6480;	// ± 18 ans (20 %)

			taille.set(4, 2);
			name = L"Building avec panneaux solaires";
			break;
		case BI_grand_building_individuel:
			prixC = -180000.0f;
			entretienM = 325.0f / 3.0f;	// 1300.0f / 12.0f

			unlockPopulationNeeded = 600;

			habitantsC = 28;	// Modifié : Ancien : 24
			habitantsMax = 56;	// Modifié : Ancien : 48
			habitantsJ = 0.4f;	// +1 habitant tous les 2.5 jours

			ressourcesC.reallocate(7);
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_bois,	-990.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_verre,	-940.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_ciment,	-24035.0f));
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_pierre,	-82800.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_fer,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_sable,	-54970.0f));
#endif

			ressourcesD.reallocate(1);
			ressourcesD.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place

			dureeVie = 32400;			// 90 ans
			dureeVieVariation = 6480;	// ± 18 ans (20 %)

			taille.set(4, 3);
			name = L"Grand building individuel";
			break;

			// Production d'énergie
		case BI_centrale_charbon:
			prixC = -120000.0f;
			energie = CENTRALE_CHARBON_ENERGIE_MAX;
			entretienM = 500.0f;

			effetSerreJ = 200.0f;
			dechetsJ = 50.0f;

			// TODO : Ressources C et D à calculer

			ressourcesJ.reallocate(2);
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_bois,	-CENTRALE_CHARBON_BOIS_MAX));													// Consommation en bois d'alimentation
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_eau,		-CENTRALE_CHARBON_BOIS_MAX * CENTRALE_CHARBON_CONSOMMATION_EAU_FACTEUR));		// Consommation en eau de refroidissement

			dureeVie = 10800;			// 30 ans
			dureeVieVariation = 2160;	// ± 6 ans (20 %)

			taille.set(10, 10);
			name = L"Centrale à charbon";
			break;
		case BI_panneau_solaire:
			prixC = -5000.0f;
			energie = 4000.0f;		// 4000 W en plein soleil
			entretienM = 200.0f;

			ressourcesC.reallocate(2);
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_bois,					-200.0f));
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_panneauxPhotovoltaiques,	-4.0f));
#else				// En mode enfant : les panneaux solaires consomment du verre à leur construction pour qu'il soit un peu plus difficile d'en construire beaucoup
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_verre,					-50.0f));
#endif

			dureeVie = 3600;			// 10 ans
			dureeVieVariation = 720;	// ± 2 ans (20 %)

			name = L"Panneau solaire";
			break;
		case BI_eolienne:
			prixC = -80000.0f;
			energie = 40000.0f;
			entretienM = 100.0f;

			unlockPopulationNeeded = 50;

			// TODO : Ressources C et D à calculer

			dureeVie = 25200;			// 70 ans
			dureeVieVariation = 5040;	// ± 14 ans (20 %)

			taille.set(6, 4);
			name = L"Eolienne";
			break;
		case BI_hydrolienne:
			prixC = -90000.0f;
			energie = 60000.0f;
			entretienM = 100.0f;

			unlockPopulationNeeded = 200;

			// TODO : Ressources C et D à calculer

			dureeVie = 21600;			// 60 ans
			dureeVieVariation = 4320;	// ± 12 ans (20 %)

			taille.set(8, 6);
			name = L"Hydrolienne";
			break;

			// Production d'eau
		case BI_pompe_extraction_eau:
			prixC = -80000.0f;
			energie = -18000.0f;
			entretienM = 1200.0f;

			// TODO : Ressources C et D à calculer

			ressourcesJ.reallocate(1);
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_eau,		1000.0f));		// Production d'eau

			dureeVie = 18000;			// 50 ans
			dureeVieVariation = 3600;	// ± 10 ans (20 %)

			taille.set(4, 4);
			name = L"Pompe d'extraction d'eau";
			break;

#ifndef KID_VERSION	// En mode enfant : On désactive l'usine à tout faire qui est devenue inutile
			// Usines
		case BI_usine_tout:	// Permet d'augmenter considérablement les ressources du joueur
			for (int i = 0; i < Ressources::RI_COUNT; ++i)
				if (i == Ressources::RI_eau)
					ressourcesJ.push_back(RessourceConsommation((Ressources::RessourceID)i, 1000.0f));
				else
					ressourcesJ.push_back(RessourceConsommation((Ressources::RessourceID)i, 100.0f));

			dureeVie = 3600;		// 10 ans
			dureeVieVariation = 0;	// ± 0 ans (0 %) (exact)

			name = L"Usine à tout faire";
			break;
#endif
		case BI_usine_verre_petite:
			// Ces valeurs seront assignées plus loin dans calculateFunctionsData(), mais on leur réserve tout de même de la place
			ressourcesC.reallocate(7);
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));	
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_bois,	0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_verre,	0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_ciment,	0.0f));
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_pierre,	0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_fer,		0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_sable,	0.0f));
#endif

			ressourcesJ.reallocate(5);
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_sable,	-165.0f));
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_potasse,	-82.5f));
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_quartz,	-66.0f));
#endif
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_eau,		-16.5f));
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_verre,	300.0f));

			// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesD.reallocate(1);
			ressourcesD.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));

			name = L"Usine de verre (Petite)";
			break;
		case BI_usine_verre_grande:
			// Ces valeurs seront assignées plus loin dans calculateFunctionsData(), mais on leur réserve tout de même de la place
			ressourcesC.reallocate(7);
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));	
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_bois,	0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_verre,	0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_ciment,	0.0f));
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_pierre,	0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_fer,		0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_sable,	0.0f));
#endif

			ressourcesJ.reallocate(5);
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_sable,	-495.0f));
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_potasse,	-247.5f));
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_quartz,	-198.0f));
#endif
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_eau,		-49.5f));
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_verre,	900.0f));

			// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesD.reallocate(1);
			ressourcesD.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));

			name = L"Usine de verre (Grande)";
			break;
		case BI_usine_ciment_petite:
			// Ces valeurs seront assignées plus loin dans calculateFunctionsData(), mais on leur réserve tout de même de la place
			ressourcesC.reallocate(7);
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));	
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_bois,	0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_verre,	0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_ciment,	0.0f));
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_pierre,	0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_fer,		0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_sable,	0.0f));
#endif

			ressourcesJ.reallocate(5);
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_calcaire,-165.0f));
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_argile,	-165.0f));
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_potasse,	-264.0f));
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_quartz,	-60.0f));
#else				// En mode enfant : Cette usine consomme de l'eau pour avoir tout de même une ressource nécessaire pour son fonctionnement
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_eau,		-20.0f));
#endif
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_ciment,	600.0f));

			// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesD.reallocate(1);
			ressourcesD.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place

			name = L"Usine de ciment (Petite)";
			break;
		case BI_usine_ciment_grande:
			// Ces valeurs seront assignées plus loin dans calculateFunctionsData(), mais on leur réserve tout de même de la place
			ressourcesC.reallocate(7);
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));	
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_bois,	0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_verre,	0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_ciment,	0.0f));
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_pierre,	0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_fer,		0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_sable,	0.0f));
#endif

			ressourcesJ.reallocate(5);
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_calcaire,-495.0f));
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_argile,	-495.0f));
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_potasse,	-792.0f));
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_quartz,	-180.0f));
#else				// En mode enfant : Cette usine consomme de l'eau pour avoir tout de même une ressource nécessaire pour son fonctionnement
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_eau,		-60.0f));
#endif
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_ciment,	1800.0f));

			// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesD.reallocate(1);
			ressourcesD.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place

			name = L"Usine de ciment (Grande)";
			break;
		case BI_usine_tuiles_petite:
			// Ces valeurs seront assignées plus loin dans calculateFunctionsData(), mais on leur réserve tout de même de la place
			ressourcesC.reallocate(7);
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));	
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_bois,	0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_verre,	0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_ciment,	0.0f));
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_pierre,	0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_fer,		0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_sable,	0.0f));
#endif

			ressourcesJ.reallocate(4);
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_ciment,	-1320.0f));
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_argile,	-660.0f));
#endif
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_eau,		-1320.0f));
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_tuiles,	3000.0f));

			// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesD.reallocate(1);
			ressourcesD.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place

			name = L"Usine de tuiles (Petite)";
			break;
		case BI_usine_tuiles_grande:
			// Ces valeurs seront assignées plus loin dans calculateFunctionsData(), mais on leur réserve tout de même de la place
			ressourcesC.reallocate(7);
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));	
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_bois,	0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_verre,	0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_ciment,	0.0f));
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_pierre,	0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_fer,		0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_sable,	0.0f));
#endif

			ressourcesJ.reallocate(4);
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_ciment,	-3960.0f));
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_argile,	-1980.0f));
#endif
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_eau,		-3960.0f));
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_tuiles,	9000.0f));

			// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesD.reallocate(1);
			ressourcesD.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place

			name = L"Usine de tuiles (Grande)";
			break;
#ifdef KID_VERSION	// En mode enfant : on réactiver le papier comme ressource consommée par les habitants pour leur satisfaction
		case BI_usine_papier_petite:
			// Ces valeurs seront assignées plus loin dans calculateFunctionsData(), mais on leur réserve tout de même de la place
			ressourcesC.reallocate(7);
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));	
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_bois,	0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_verre,	0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_ciment,	0.0f));
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_pierre,	0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_fer,		0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_sable,	0.0f));
#endif

			ressourcesJ.reallocate(3);
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_bois,	-990.0f));
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_eau,		-660.0f));
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_papier,	1500.0f));

			// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesD.reallocate(1);
			ressourcesD.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place

			name = L"Usine de papier (Petite)";
			break;
		case BI_usine_papier_grande:
			// Ces valeurs seront assignées plus loin dans calculateFunctionsData(), mais on leur réserve tout de même de la place
			ressourcesC.reallocate(7);
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));	
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_bois,	0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_verre,	0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_ciment,	0.0f));
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_pierre,	0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_fer,		0.0f));
			ressourcesC.push_back(RessourceConsommation(Ressources::RI_sable,	0.0f));
#endif

			ressourcesJ.reallocate(3);
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_bois,	-2970.0f));
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_eau,		-1980.0f));
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_papier,	4500.0f));

			// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place
			ressourcesD.reallocate(1);
			ressourcesD.push_back(RessourceConsommation(Ressources::RI_eau,		0.0f));		// Cette valeur sera assignée plus loin dans calculateFunctionsData(), mais on lui réserve tout de même de la place

			name = L"Usine de papier (Grande)";
			break;
#endif

			// Gestion des déchets et de l'effet de serre
		case BI_decharge:
			prixC = -5000.0f;
			entretienM = 50.0f;

			effetSerreJ = 1.0f;
			dechetsJ = -10.0f;

			dureeVie = 72000;			// 200 ans
			dureeVieVariation = 14400;	// ± 40 ans (20 %)

			taille.set(5, 5);
			name = L"Décharge";
			break;
		case BI_usine_incineration_dechets:
			prixC = -40000.0f;
			energie = -20000.0f;
			entretienM = 700.0f;

			unlockPopulationNeeded = 30;

			effetSerreJ = USINE_INCINERATION_DECHETS_MAX * USINE_INCINERATION_RENTABILITE;
			dechetsJ = -USINE_INCINERATION_DECHETS_MAX;

			dureeVie = 18000;			// 50 ans
			dureeVieVariation = 3600;	// ± 10 ans (20 %)

			taille.set(10, 10);
			name = L"Usine d'incinération des déchets";
			break;

			// Arbres
		case BI_arbre_aspen:
		case BI_arbre_oak:
		case BI_arbre_pine:
		case BI_arbre_willow:
			prixC = -60.0f;

			// Effet de serre absorbé par jour : Dépendant de l'âge de l'arbre
			effetSerreJ = -2.0f;

#ifdef KID_VERSION
			// En mode enfant, un arbre produit du bois chaque jour proportionnellement à son âge (jusqu'à un an) : plus il est grand, plus il fournit de bois.
			// A un an (et plus) (à sa taille maximale) : 5 kg/arbre/J.
			// En effet, il est pour le moment impossible de produire du bois autrement que par la destruction manuelle des arbres !
			ressourcesJ.reallocate(1);
			ressourcesJ.push_back(RessourceConsommation(Ressources::RI_bois,	5.0f));
#endif

			// Un arbre fournit du bois à sa destruction proportionnellement à son âge (jusqu'à un an) : plus il est grand, plus il fournit de bois.
			// Attention : un arbre devrait toujours produire un peu de bois à sa destruction.
			// A un an (et plus) (à sa taille maximale) : 1 st/arbre <-> 500 kg/arbre.
			ressourcesD.reallocate(1);
			ressourcesD.push_back(RessourceConsommation(Ressources::RI_bois,	500.0f));

			dureeVie = 54000;			// 150 ans
			dureeVieVariation = 21600;	// ± 60 ans (40 %)

			taille.set(1, 1);
			switch (batimentID)
			{
			case BI_arbre_aspen:	name = L"Tremble";		break;
			case BI_arbre_oak:		name = L"Chêne";		break;
			case BI_arbre_pine:		name = L"Pin";			break;
			case BI_arbre_willow:	name = L"Saule";		break;
			default:				name = L"Arbre";		break;
			}
			break;



		default:
			LOG_DEBUG("StaticBatimentInfos::setID(" << batimentID << ") : Type de batiment inconnu : " << batimentID, ELL_WARNING);
			break;
		}

		calculateFunctionsData();
	}

	// Met à jour les informations sur ce bâtiment avec les modifiers de jeu
	void updateInfosWithModifiers(const EcoWorldModifiers& modifiers)
	{
		prixC *= modifiers.prixFactor;
		prixD *= modifiers.prixFactor;
		// Désactivé : Permet de conserver des temps cohérents entre les difficultés :
		//tempsC *= modifiers.prixFactor;	// Le temps de construction est dépendant du prix du bâtiment
		//tempsD *= modifiers.prixFactor;	// Le temps de destruction est dépendant du prix du bâtiment

		loyerM *= modifiers.loyerFactor;

		if (energie > 0.0f)
			energie *= modifiers.energieFactor;

		effetSerreC *= modifiers.effetSerreFactor;
		effetSerreJ *= modifiers.effetSerreFactor;
		effetSerreD *= modifiers.effetSerreFactor;

		dechetsC *= modifiers.dechetsFactor;
		dechetsJ *= modifiers.dechetsFactor;
		dechetsD *= modifiers.dechetsFactor;

		const u32 ressourcesSize = ressourcesJ.size();
		for (u32 i = 0; i < ressourcesSize; ++i)
			if (ressourcesJ[i].production > 0.0f)
				ressourcesJ[i].production *= modifiers.ressourcesProductionFactor;

		tempsC = (u32)floorf(abs((float)tempsC * modifiers.constructionTimeFactor));
		tempsD = (u32)floorf(abs((float)tempsD * modifiers.constructionTimeFactor));

		dureeVie = (u32)floorf(abs((float)dureeVie * modifiers.dureeVieFactor));
		dureeVieVariation = (u32)floorf(abs((float)dureeVieVariation * modifiers.dureeVieFactor));

		unlockPopulationNeeded = (u32)floorf(abs((float)unlockPopulationNeeded * modifiers.unlockPopulationNeededFactor));
	}

	// Constructeur et destructeur protégés (impossible d'instancier cette classe)
	StaticBatimentInfos() : ID(BI_aucun)	{ setID(BI_aucun); }

	// La liste des informations sur les batiments
	static StaticBatimentInfos batimentInfos[BI_COUNT];

	static u32 tailleMaxBats;	// La taille maximale que peuvent avoir les bâtiments, utilisé pour certaines optimisations
								// Cette taille est maintenant calculée dynamiquement lors de l'appel à StaticBatimentInfos::init()
public:
	// Retourne la taille maximale des bâtiments
	static u32 getTailleMaxBatiments()		{ return tailleMaxBats; }

	// Crée toutes les informations sur les batiments d'après les modifiers de jeu
	static void init(const EcoWorldModifiers& modifiers)
	{
		// Crée les informations statiques de tous les bâtiments
		tailleMaxBats = 1;
		for (int i = 0; i < BI_COUNT; ++i)
		{
			batimentInfos[i].setID((BatimentID)i);
			batimentInfos[i].updateInfosWithModifiers(modifiers);

			// Recalcule la taille maximale des bâtiments
			tailleMaxBats = core::max_(batimentInfos[i].taille.Width, batimentInfos[i].taille.Height, tailleMaxBats);
		}
	}

	// Retourne les informations statiques d'un batiment
	const static StaticBatimentInfos& getInfos(BatimentID batimentID)
	{
		// Vérifie que batimentID est l'ID d'un batiment valide
		if ((int)batimentID <= 0 || (int)batimentID >= (int)BI_COUNT)
			return batimentInfos[BI_aucun];

		// Si l'ID du batiment n'est pas valide avec celle qu'il devrait avoir (à cause d'un non-appel à la fonction init() par exemple)
		if (batimentInfos[batimentID].ID != batimentID)
			batimentInfos[batimentID].setID(batimentID); // On recrée ses informations de base, mais elles pourraient ne pas correspondre aux modifiers de difficulté du jeu !

		return batimentInfos[batimentID];
	}



	// Détermine si ce bâtiment nécessite le réglage de son pourcentage de production par l'utilisateur
	static bool needPourcentageProduction(BatimentID ID)
	{
		const StaticBatimentInfos& batInfos = getInfos(ID);
		return (batInfos.isUsine
			|| ID == BI_centrale_charbon || ID == BI_pompe_extraction_eau || ID == BI_usine_incineration_dechets
			);
	}

	// Détermine si ce bâtiment nécessite de l'eau profonde pour être construit (ces bâtiments ne nécessitent donc pas de terrain constructible)
	static bool needDeepWater(BatimentID ID)
	{
		return (ID == BI_hydrolienne);
	}

	// Détermine si ce batiment nécessite un bloc de béton sous son placement 3D sur le terrain pour poser ses fondations (en ainsi ne pas faire croire qu'il "vole")
	static bool needConcreteUnderBatiment(BatimentID ID)
	{
		const StaticBatimentInfos& batInfos = getInfos(ID);
		return (!batInfos.isTree && !needDeepWater(ID)
#ifndef KID_VERSION	// En mode enfant : on désactive les routes qui sont inutiles
			&& ID != BI_route
#endif
			);
	}

	// Détermine si ce batiment peut être créé par rectangle de sélection
	static bool canCreateBySelectionRect(BatimentID ID)
	{
		const StaticBatimentInfos& batInfos = getInfos(ID);
		return (batInfos.isTree
			|| ID == BI_panneau_solaire || ID == BI_eolienne || ID == BI_hydrolienne
#ifndef KID_VERSION	// En mode enfant : on désactive les routes qui sont inutiles
			|| ID == BI_route
#endif
			);
	}
};

// Contient les informations réelles sur les batiments (les informations actuelles pour leur affichage)
class BatimentInfos
{
public:
	/*
	Signification des lettres après les variables :
	- C : Une seule fois (à la construction)
	- J : Tous les jours
	- M : Tous les mois
	- A : Tous les ans
	- D : Une seule fois (à la destruction)
	*/

	// Les factures d'eau et d'électricité, le loyer et les habitants ne doivent être utilisés que pour les habitations

	float energie;				// L'électricité réelle que consomme ce bâtiment (énergie par jour)	// (W : WJ/J)
	float eauConsommationJ;		// L'eau réelle que consomme ce bâtiment par jour					// (L/J)

	float effetSerreJ;			// L'effet de serre réel que rejette ce bâtiment par jour			// (kg)
	float dechetsJ;				// Les déchets réels que rejette ce bâtiment par jour				// (kg)

	float loyerM;				// Le loyer réel que rapporte ce bâtiment par mois (réservé aux habitations !)		// (€)
	float impotsA;				// Les impôts réels que rapporte ce bâtiment par mois (réservé aux habitations !)	// (€)

	float entretienM;			// L'entretien réel que coûte ce bâtiment par mois					// (€)

	u32 habitants;				// Le nombre réel d'habitants que contient ce bâtiment				// (Personnes)
	float nextHabitantPercent;	// Le pourcentage (entre 0 et 1) d'arrivée du prochain habitant		// (Pourcent entre 0 et 1)
								// -> Utilisé comme compteur : une fois à 1, l'habitant suivant emménage, et le compteur est remis à zéro

	float pourcentageProduction;		// Le pourcentage de production choisi par l'utilisateur (permet d'économiser de l'argent et des ressources sur une usine qui produit trop)
	float pourcentageProductionActuel;	// Le pourcentage de production réel de cette usine (influencé par les ressources disponibles entre-autres) (ne dépasse jamais le pourcentage de production demandé par l'utilisateur)

	float currentEnergieConsommationM;	// La quantité d'énergie (sous forme d'électricité) consommée et non encore payée (incrémentée à chaque jour avec l'électricité consommée, remise à zéro à la fin du mois lorsque la maison paye ses factures au joueur)
	float currentEauConsommationM;		// La quantité d'eau totale consommée et non encore payée (incrémentée à chaque jour avec l'eau consommée, remise à zéro à la fin du mois lorsque la maison paye ses factures au joueur)

	u32 dureeVie;				// La durée de vie réelle de ce bâtiment (recalculée aléatoirement d'après les informations statiques sur ce bâtiment)

	core::array<StaticBatimentInfos::RessourceConsommation> ressourcesJ;	// Les ressources actuellement consommées/produites par jour par ce bâtiment	// Unité varie suivant le type de ressource

	BatimentID ID;

	// Constructeur
	BatimentInfos(BatimentID batimentID) : ID(batimentID)
	{
		setID(batimentID);
	}

	// Opérateur d'assignement
	BatimentInfos& operator=(const BatimentInfos& other)
	{
		energie = other.energie;
		eauConsommationJ = other.eauConsommationJ;
		effetSerreJ = other.effetSerreJ;
		dechetsJ = other.dechetsJ;
		loyerM = other.loyerM;
		impotsA = other.impotsA;
		entretienM = other.entretienM;

		habitants = other.habitants;
		nextHabitantPercent = other.nextHabitantPercent;

		pourcentageProduction = other.pourcentageProduction;
		pourcentageProductionActuel = other.pourcentageProductionActuel;

		currentEnergieConsommationM = other.currentEnergieConsommationM;
		currentEauConsommationM = other.currentEauConsommationM;

		dureeVie = other.dureeVie;

		const u32 size = other.ressourcesJ.size();
		ressourcesJ.clear();
		ressourcesJ.reallocate(size);
		for (u32 i = 0; i < size; ++i)
			ressourcesJ.push_back(other.ressourcesJ[i]);

		ID = other.ID;

		return *this;
	}

	// Opérateurs de comparaison (utilisés uniquement par la GUI du jeu pour vérifier s'il est nécessaire de remettre à jour la fenêtre d'informations ou non)
	bool operator==(const BatimentInfos& other) const
	{
		if (ID != other.ID)
			return false;

		if (energie != other.energie
			|| eauConsommationJ != other.eauConsommationJ
			|| effetSerreJ != other.effetSerreJ
			|| dechetsJ != other.dechetsJ
			|| loyerM != other.loyerM
			|| impotsA != other.impotsA
			|| entretienM != other.entretienM
			|| habitants != other.habitants
			//|| nextHabitantPercent != other.nextHabitantPercent		// Non utilisé dans l'affichage dans la fenêtre d'informations
			|| pourcentageProduction != other.pourcentageProduction
			|| pourcentageProductionActuel != other.pourcentageProductionActuel
			|| currentEnergieConsommationM != other.currentEnergieConsommationM
			|| currentEauConsommationM != other.currentEauConsommationM
			|| dureeVie != other.dureeVie)
			return false;

		const u32 size = ressourcesJ.size();
		if (size != other.ressourcesJ.size())
			return false;

		for (u32 i = 0; i < size; ++i)
			if (ressourcesJ[i].ID != other.ressourcesJ[i].ID || ressourcesJ[i].production != other.ressourcesJ[i].production)
				return false;

		return true;
	}
	bool operator!=(const BatimentInfos& other) const { return (!((*this) == other)); }

	// Fonction statique permettant de calculer la durée de vie d'un bâtiment suivant les informations statiques sur sa durée de vie et sa variation
	static u32 computeDureeVie(u32 staticInfosDureeVie, u32 staticInfosDureeVieVariation)
	{
		if (staticInfosDureeVie > 0 && staticInfosDureeVieVariation > 0
			&& staticInfosDureeVie >= staticInfosDureeVieVariation)	// Vérifie que le résultat (staticInfosDureeVie - staticInfosDureeVieVariation) sera toujours positif pour pouvoir être contenu dans un u32
			return (max<u32>((staticInfosDureeVie + ((u32)rand() % (2 * staticInfosDureeVieVariation + 1))) - staticInfosDureeVieVariation, 1));
		return staticInfosDureeVie;
	}

	// Recalcule certaines valeurs d'après des fonctions numériques
	void calculateFunctionsData()
	{
		// Recalcule aléatoirement la durée de vie de ce bâtiment
		{
			const StaticBatimentInfos& staticInfos = StaticBatimentInfos::getInfos(ID);
			/*
			dureeVie = staticInfos.dureeVie;
			if (dureeVie > 0 && staticInfos.dureeVieVariation > 0)
				dureeVie += max((long)((rand() % (2 * staticInfos.dureeVieVariation + 1)) - staticInfos.dureeVieVariation), -((long)(dureeVie - 1)));
			*/
			dureeVie = computeDureeVie(staticInfos.dureeVie, staticInfos.dureeVieVariation);
		}
	}

	// Fonction de réinitialisation, destinée à être appelée par les bâtiments eux-mêmes pour indiquer qu'ils sont complètement inactifs pendant leur construction/destruction.
	// On n'efface pas la durée de vie du bâtiment ni son ID, qui doivent rester constants durant toute la vie du bâtiment.
	void reset()
	{
		energie = 0.0f;
		eauConsommationJ = 0.0f;
		effetSerreJ = 0.0f;
		dechetsJ = 0.0f;
		loyerM = 0.0f;
		impotsA = 0.0f;
		entretienM = 0.0f;

		habitants = 0;
		nextHabitantPercent = 0.0f;

		pourcentageProduction = 1.0f;
		pourcentageProductionActuel = 0.0f;

		currentEnergieConsommationM = 0.0f;
		currentEauConsommationM = 0.0f;

		// Remet la production de ressources par jour à 0.0f, mais sans modifier la structure du tableau des ressources
		const u32 ressourcesSize = ressourcesJ.size();
		for (u32 i = 0; i < ressourcesSize; ++i)
			ressourcesJ[i].production = 0.0f;
	}

	void setID(BatimentID batimentID)
	{
		// Initialise le tableau des ressources par jour (utilisé par reset())
		ressourcesJ.clear();
		const StaticBatimentInfos& staticInfos = StaticBatimentInfos::getInfos(batimentID);
		if (batimentID != BI_aucun)
		{
			const u32 size = staticInfos.ressourcesJ.size();
			ressourcesJ.reallocate(size);
			for (u32 i = 0; i < size; ++i)
				ressourcesJ.push_back(staticInfos.ressourcesJ[i]);
		}

		// Réinitialise les informations sur le bâtiment (elles seront assignées par le bâtiment lui-même une fois sa construction terminée)
		reset();
		dureeVie = 0;
		ID = batimentID;

		if (batimentID != BI_aucun)
		{
			// Recalcule les données pouvant être initialisées grâce à des fonctions
			calculateFunctionsData();
		}
	}
};

#endif
