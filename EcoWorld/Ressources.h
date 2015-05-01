#ifndef DEF_RESSOURCES
#define DEF_RESSOURCES

#ifndef KID_VERSION	// En mode enfant : on d�sactive les ressources ne pouvant pas �tre produites
//#define KEEP_UNUSED_RESSOURCES	// Si d�fini, les ressources d�clar�es ici mais inutilis�es dans le jeu seront tout de m�me compil�es
#endif

// Espace de nom contenant des �num�rations et des fonctions statiques pour la gestion des ressources dans EcoWorld
namespace Ressources
{
	enum RessourceID
	{
		// Mat�riaux
		RI_eau,				// (L)
		RI_bois,			// (kg) 0.08 �/kg
		RI_verre,			// (kg) 1.33 �/kg
		RI_ciment,			// (kg) 0.15 �/kg
		RI_tuiles,			// (kg) 1 �/kg
#ifndef KID_VERSION	// En mode enfant : on d�sactive les ressources ne pouvant pas �tre produites
		RI_pierre,			// (kg) Parpaing : 0.06 �/kg
		RI_fer,				// (kg) 0.11 �/kg (voire "m�tal" ?)
		//RI_plastique,		// (kg)
		RI_sable,			// (kg) 0.025 �/kg <- A revoir
		RI_panneauxPhotovoltaiques,	// (nb) 1000 �/nb

		// Semi mat�riaux
		RI_quartz,			// (kg)
		RI_potasse,			// (kg)
		//RI_blocsPierre,		// (kg)
		//RI_troncsAbres,		// (st) -> Utilis� : kg
		RI_argile,			// (kg)
		RI_calcaire,		// (kg)

		// Nourriture
		RI_viande,			// (kg)
		//RI_volaille,		// (kg)
		RI_poisson,			// (kg)
		//RI_fruits,			// (kg)
		//RI_legumes,			// (kg)
		//RI_pates,			// (kg)
		//RI_riz,				// (kg)
		RI_pain,			// (kg) (1 pain <-> 400 g)
		RI_huile,			// (L)
		RI_vin,				// (L)
		RI_lait,			// (L)
		//RI_biere,			// (L)
		//RI_chocolat,		// (kg)
		//RI_biscuit,			// (kg)
		//RI_sel,				// (kg)
		//RI_poivre,			// (kg)
		//RI_sucre,			// (kg)
		//RI_gateau,			// (kg)
		//RI_miel,			// (kg)
		//RI_confiture,		// (kg)
		//RI_moutarde,		// (kg)
		//RI_the,				// (kg)

		// Semi Nourriture
		//RI_veau,			// (nb)
#ifdef KEEP_UNUSED_RESSOURCES
		RI_vache,			// (nb)
#endif
		//RI_taureau,			// (nb)
		//RI_agneau,			// (nb)
#ifdef KEEP_UNUSED_RESSOURCES
		RI_mouton,			// (nb)
#endif
		//RI_brebis,			// (nb)
		//RI_bouc,			// (nb)
		//RI_poulet,			// (nb)
		//RI_poule,			// (nb)
		//RI_poussin,			// (nb)
		//RI_cheval,			// (kg) Incoh�rent ?! -> Utilis� : nb
		//RI_jument,			// (nb)
		//RI_poney,			// (nb)
		//RI_porc,			// (nb)
		//RI_truie,			// (nb)
		//RI_porcelet,		// (nb)
		//RI_boeuf,			// (kg)
		//RI_truite,			// (kg)
		//RI_saumon,			// (kg)
		//RI_thon,			// (kg)
		//RI_moule,			// (kg)
		//RI_crevette,		// (kg)
		RI_pomme,			// (kg)
		RI_poire,			// (kg)
		//RI_prune,			// (kg)
		//RI_cerise,			// (kg)
		//RI_noix,			// (kg)
		RI_banane,			// (kg)
		//RI_peche,			// (kg)
		RI_orange,			// (kg)
		RI_raisin,			// (kg)
		RI_tomate,			// (kg)
		//RI_potiron,			// (kg)
		//RI_melon,			// (kg)
		RI_haricots,		// (kg)
		//RI_haricotsCoco,	// (kg)
		//RI_radis,			// (kg)
		//RI_courgette,		// (kg)
		RI_pommesTerre,		// (kg)
		RI_salade,			// (kg)
		RI_carotte,			// (kg)
#ifdef KEEP_UNUSED_RESSOURCES
		RI_ble,				// (kg)
		RI_tournesol,		// (kg)
#endif
		//RI_orge,			// (kg)
#ifdef KEEP_UNUSED_RESSOURCES
		RI_avoine,			// (kg)
#endif
		//RI_mais,			// (kg)
		//RI_cafe,			// (kg)
		//RI_beurre,			// (kg)
		//RI_oeuf,			// (kg)
		//RI_cremeFraiche,	// (kg)

		// Arbres et plantes
		//RI_cacaoyer,		// (nb)
		//RI_poivrier,		// (nb)
		//RI_canneSucre,		// (nb)

		// V�tement
		RI_vetements,		// (kg)
		//RI_pantalon,		// (nb)
		//RI_short,			// (nb)
		//RI_pantacourt,		// (nb)
		//RI_chausette,		// (nb)
		//RI_basket,			// (nb)
		//RI_vesteEte,		// (nb) (Veste d'�t�)
		//RI_manteau,			// (nb)
		//RI_teeShirt,		// (nb)
		//RI_pullOver,		// (nb)
		//RI_chemise,			// (nb)
		//RI_casquette,		// (nb)
		//RI_chapeau,			// (nb)

		// Semi V�tement
		//RI_lin,				// (kg)
#ifdef KEEP_UNUSED_RESSOURCES
		RI_indigo,			// (kg)
#endif
		//RI_cuir,			// (kg)
#ifdef KEEP_UNUSED_RESSOURCES
		RI_laine,			// (kg)
#endif

		// Outils d'�cole
		//RI_crayon,			// (nb)
		//RI_stylo,			// (nb)
		//RI_ciseaux,			// (nb)
		//RI_trousse,			// (nb)
		//RI_regle,			// (nb)
		//RI_equerre,			// (nb)
		//RI_rapporteur,		// (nb)
		//RI_crayonCouleur,	// (nb)
#else				// En mode enfant : on r�activer le papier comme ressource consomm�e par les habitants pour leur satisfaction
		RI_papier,			// (kg)
#endif
#ifndef KID_VERSION	// En mode enfant : on d�sactive les ressources ne pouvant pas �tre produites
		//RI_cahier,			// (nb)
		//RI_compas,			// (nb)
		//RI_gomme,			// (nb)

		// Moyens de locomotion
		//RI_velo,			// (nb)
		//RI_bus,				// (nb)

		// Divers
		//RI_tondeuse,		// (nb)
		//RI_television,		// (nb)
		//RI_ordinateur,		// (nb)
		//RI_telephone_portable,	// (nb)
		//RI_assiette,		// (nb)
		//RI_couteau,			// (nb)
		//RI_fourchette,		// (nb)
		//RI_cuillere,		// (nb)
		//RI_casserole,		// (nb)
		//RI_poele,			// (nb)
		//RI_cocoteMinute,	// (nb)
		//RI_tasse,			// (nb)
		//RI_bol,				// (nb)
#endif

		RI_COUNT
	};

	static const wchar_t* getRessourceName(RessourceID id)
	{
		// Cr�e les noms des ressources
		static const wchar_t* ressourcesNames[RI_COUNT] =
		{
			// Mat�riaux
			L"Eau",
			L"Bois",
			L"Verre",
			L"Ciment",
			L"Tuiles",
#ifndef KID_VERSION	// En mode enfant : on d�sactive les ressources ne pouvant pas �tre produites
			L"Pierre",
			L"Fer",
			//L"Plastique",
			L"Sable",
			L"Panneaux solaires", // L"Panneaux photovolta�ques"

			// Semi mat�riaux
			L"Quartz",
			L"Potasse",
			//L"Blocs de pierre",
			//L"Troncs d'arbre",
			L"Argile",
			L"Calcaire",

			// Nourriture
			L"Viande",
			//L"Volaille",
			L"Poisson",
			//L"Fruits",
			//L"L�gumes",
			//L"P�tes",
			//L"Riz",
			L"Pain",
			L"Huile",
			L"Vin",
			L"Lait",
			//L"Bi�re",
			//L"Chocolat",
			//L"Biscuit",
			//L"Sel",
			//L"Poivre",
			//L"Sucre",
			//L"G�teau",
			//L"Miel",
			//L"Confiture",
			//L"Moutarde",
			//L"Th�",

			// Semi Nourriture
			//L"Veau",
#ifdef KEEP_UNUSED_RESSOURCES
			L"Vache",
#endif
			//L"Taureau",
			//L"Agneau",
#ifdef KEEP_UNUSED_RESSOURCES
			L"Mouton",
#endif
			//L"Brebis",
			//L"Bouc",
			//L"Poulet",
			//L"Poule",
			//L"Poussin",
			//L"Cheval",
			//L"Jument",
			//L"Poney",
			//L"Porc",
			//L"Truie",
			//L"Porcelet",
			//L"Boeuf",
			//L"Truite",
			//L"Saumon",
			//L"Thon",
			//L"Moule",
			//L"Crevette",
			L"Pomme",
			L"Poire",
			//L"Prune",
			//L"Cerise",
			//L"Noix",
			L"Banane",
			//L"P�che",
			L"Orange",
			L"Raisin",
			L"Tomate",
			//L"Potiron",
			//L"Melon",
			L"Haricots",
			//L"Haricots coco",
			//L"Radis",
			//L"Courgette",
			L"Pomme de terre",
			L"Salade",
			L"Carotte",
#ifdef KEEP_UNUSED_RESSOURCES
			L"Bl�",
			L"Tournesol",
#endif
			//L"Orge",
#ifdef KEEP_UNUSED_RESSOURCES
			L"Avoine",
#endif
			//L"Ma�s",
			//L"Caf�",
			//L"Beurre",
			//L"Oeuf",
			//L"Cr�me fra�che",

			// Arbres et plantes
			//L"Cacaoyer",
			//L"Poivrier",
			//L"Canne � sucre",

			// V�tement
			L"V�tements",
			//L"Pantalon",
			//L"Short",
			//L"Pantacourt",
			//L"Chausette",
			//L"Basket",
			//L"Veste d'�t�",
			//L"Manteau",
			//L"Tee-shirt",
			//L"Pull-over",
			//L"Chemise",
			//L"Casquette",
			//L"Chapeau",

			// Semi V�tement
			//L"Lin",
#ifdef KEEP_UNUSED_RESSOURCES
			L"Indigo",
#endif
			//L"Cuir",
#ifdef KEEP_UNUSED_RESSOURCES
			L"Laine",
#endif

			// Outils d'�cole
			//L"Crayon",
			//L"Stylo",
			//L"Ciseaux",
			//L"Trousse",
			//L"R�gle",
			//L"Equerre",
			//L"Rapporteur",
			//L"Crayon de couleur",
#else				// En mode enfant : on r�activer le papier comme ressource consomm�e par les habitants pour leur satisfaction
			L"Papier",
#endif
#ifndef KID_VERSION	// En mode enfant : on d�sactive les ressources ne pouvant pas �tre produites
			//L"Cahier",
			//L"Compas",
			//L"Gomme",

			// Moyens de locomotion
			//L"V�lo",
			//L"Bus",

			// Divers
			//L"Tondeuse",
			//L"T�l�vision",
			//L"Ordinateur",
			//L"T�l�phone portable",
			//L"Assiette",
			//L"Couteau",
			//L"Fourchette",
			//L"Cuill�re",
			//L"Casserole",
			//L"Po�le",
			//L"Cocote minute",
			//L"Tasse",
			//L"Bol",
#endif
		};

		return ressourcesNames[id];
	}

	static const char* getRessourceNameChar(RessourceID id)
	{
		// Cr�e les noms des ressources (sans accents ici !)
		static const char* ressourcesNamesChar[RI_COUNT] =
		{
			// Mat�riaux
			"Eau",
			"Bois",
			"Verre",
			"Ciment",
			"Tuiles",
#ifndef KID_VERSION	// En mode enfant : on d�sactive les ressources ne pouvant pas �tre produites
			"Pierre",
			"Fer",
			//"Plastique",
			"Sable",
			"Panneaux solaires", // "Panneaux photovolta�ques"

			// Semi mat�riaux
			"Quartz",
			"Potasse",
			//"Blocs de pierre",
			//"Troncs d'arbre",
			"Argile",
			"Calcaire",

			// Nourriture
			"Viande",
			//"Volaille",
			"Poisson",
			//"Fruits",
			//"Legumes",
			//"Pates",
			//"Riz",
			"Pain",
			"Huile",
			"Vin",
			"Lait",
			//"Biere",
			//"Chocolat",
			//"Biscuit",
			//"Sel",
			//"Poivre",
			//"Sucre",
			//"Gateau",
			//"Miel",
			//"Confiture",
			//"Moutarde",
			//"The",

			// Semi Nourriture
			//"Veau",
#ifdef KEEP_UNUSED_RESSOURCES
			"Vache",
#endif
			//"Taureau",
			//"Agneau",
#ifdef KEEP_UNUSED_RESSOURCES
			"Mouton",
#endif
			//"Brebis",
			//"Bouc",
			//"Poulet",
			//"Poule",
			//"Poussin",
			//"Cheval",
			//"Jument",
			//"Poney",
			//"Porc",
			//"Truie",
			//"Porcelet",
			//"Boeuf",
			//"Truite",
			//"Saumon",
			//"Thon",
			//"Moule",
			//"Crevette",
			"Pomme",
			"Poire",
			//"Prune",
			//"Cerise",
			//"Noix",
			"Banane",
			//"Peche",
			"Orange",
			"Raisin",
			"Tomate",
			//"Potiron",
			//"Melon",
			"Haricots",
			//"Haricots coco",
			//"Radis",
			//"Courgette",
			"Pomme de terre",
			"Salade",
			"Carotte",
#ifdef KEEP_UNUSED_RESSOURCES
			"Ble",
			"Tournesol",
#endif
			//"Orge",
#ifdef KEEP_UNUSED_RESSOURCES
			"Avoine",
#endif
			//"Mais",
			//"Cafe",
			//"Beurre",
			//"Oeuf",
			//"Creme fraiche",

			// Arbres et plantes
			//"Cacaoyer",
			//"Poivrier",
			//"Canne � sucre",

			// V�tement
			"Vetements",
			//"Pantalon",
			//"Short",
			//"Pantacourt",
			//"Chausette",
			//"Basket",
			//"Veste d'ete",
			//"Manteau",
			//"Tee-shirt",
			//"Pull-over",
			//"Chemise",
			//"Casquette",
			//"Chapeau",

			// Semi V�tement
			//"Lin",
#ifdef KEEP_UNUSED_RESSOURCES
			"Indigo",
#endif
			//"Cuir",
#ifdef KEEP_UNUSED_RESSOURCES
			"Laine",
#endif

			// Outils d'�cole
			//"Crayon",
			//"Stylo",
			//"Ciseaux",
			//"Trousse",
			//"Regle",
			//"Equerre",
			//"Rapporteur",
			//"Crayon de couleur",
#else				// En mode enfant : on r�activer le papier comme ressource consomm�e par les habitants pour leur satisfaction
			"Papier",
#endif
#ifndef KID_VERSION	// En mode enfant : on d�sactive les ressources ne pouvant pas �tre produites
			//"Cahier",
			//"Compas",
			//"Gomme",

			// Moyens de locomotion
			//"Velo",
			//"Bus",

			// Divers
			//"Tondeuse",
			//"Television",
			//"Ordinateur",
			//"Telephone portable",
			//"Assiette",
			//"Couteau",
			//"Fourchette",
			//"Cuillere",
			//"Casserole",
			//"Poele",
			//"Cocote minute",
			//"Tasse",
			//"Bol",
#endif
		};

		return ressourcesNamesChar[id];
	}

	static const wchar_t* getRessourceUnit(RessourceID id)
	{
		// Cr�e les unit�s des ressources
		static const wchar_t* ressourcesUnits[RI_COUNT] =
		{
			// Mat�riaux
			L"L",
			L"kg",
			L"kg",
			L"kg",
			L"kg",
#ifndef KID_VERSION	// En mode enfant : on d�sactive les ressources ne pouvant pas �tre produites
			L"kg",
			L"kg",
			//L"kg",
			L"kg",
			L"",

			// Semi mat�riaux
			L"kg",
			L"kg",
			//L"kg",
			//L"kg",
			L"kg",
			L"kg",

			// Nourriture
			L"kg",
			//L"kg",
			L"kg",
			//L"kg",
			//L"kg",
			//L"kg",
			//L"kg",
			L"kg",
			L"L",
			L"L",
			L"L",
			//L"L",
			//L"kg",
			//L"kg",
			//L"kg",
			//L"kg",
			//L"kg",
			//L"kg",
			//L"kg",
			//L"kg",
			//L"kg",
			//L"kg",

			// Semi Nourriture
			//L"",
#ifdef KEEP_UNUSED_RESSOURCES
			L"",
#endif
			//L"",
			//L"",
#ifdef KEEP_UNUSED_RESSOURCES
			L"",
#endif
			//L"",
			//L"",
			//L"",
			//L"",
			//L"",
			//L"",
			//L"",
			//L"",
			//L"",
			//L"",
			//L"",
			//L"",
			//L"kg",
			//L"kg",
			//L"kg",
			//L"kg",
			//L"kg",
			L"kg",
			L"kg",
			//L"kg",
			//L"kg",
			//L"kg",
			L"kg",
			//L"kg",
			L"kg",
			L"kg",
			L"kg",
			//L"kg",
			//L"kg",
			L"kg",
			//L"kg",
			//L"kg",
			//L"kg",
			L"kg",
			L"kg",
			L"kg",
#ifdef KEEP_UNUSED_RESSOURCES
			L"kg",
			L"kg",
#endif
			//L"kg",
#ifdef KEEP_UNUSED_RESSOURCES
			L"kg",
#endif
			//L"kg",
			//L"kg",
			//L"kg",
			//L"kg",
			//L"kg",

			// Arbres et plantes
			//L"",
			//L"",
			//L"",

			// V�tement
			L"kg",
			//L"",
			//L"",
			//L"",
			//L"",
			//L"",
			//L"",
			//L"",
			//L"",
			//L"",
			//L"",
			//L"",
			//L"",
			
			// Semi V�tement
			//L"kg",
#ifdef KEEP_UNUSED_RESSOURCES
			L"kg",
#endif
			//L"kg",
#ifdef KEEP_UNUSED_RESSOURCES
			L"kg",
#endif

			// Outils d'�cole
			//L"",
			//L"",
			//L"",
			//L"",
			//L"",
			//L"",
			//L"",
			//L"",
#else				// En mode enfant : on r�activer le papier comme ressource consomm�e par les habitants pour leur satisfaction
			L"kg",
#endif
#ifndef KID_VERSION	// En mode enfant : on d�sactive les ressources ne pouvant pas �tre produites
			//L"",
			//L"",
			//L"",

			// Moyens de locomotion
			//L"",
			//L"",

			// Divers
			//L"",
			//L"",
			//L"",
			//L"",
			//L"",
			//L"",
			//L"",
			//L"",
			//L"",
			//L"",
			//L"",
			//L"",
			//L"",
#endif
		};

		return ressourcesUnits[id];
	}

	enum RessourceGroup
	{
		RG_materiaux,
#ifndef KID_VERSION	// En mode enfant : on d�sactive les ressources ne pouvant pas �tre produites
		RG_semiMateriaux,
		RG_nourriture,
		RG_semiNourriture,
		//RG_arbres,
		RG_vetements,
#ifdef KEEP_UNUSED_RESSOURCES
		RG_semiVetements,
#endif
#else				// En mode enfant : on r�activer le papier comme ressource consomm�e par les habitants pour leur satisfaction
		RG_outilsEcole,
#endif
#ifndef KID_VERSION	// En mode enfant : on d�sactive les ressources ne pouvant pas �tre produites
		//RG_locomotion,
		//RG_divers
#endif
	};

	static RessourceGroup getRessourceGroup(RessourceID id)
	{
		// Cr�e les groupes des ressources
		static const RessourceGroup ressourcesGroups[RI_COUNT] =
		{
			// Mat�riaux
			RG_materiaux,
			RG_materiaux,
			RG_materiaux,
			RG_materiaux,
			RG_materiaux,
#ifndef KID_VERSION	// En mode enfant : on d�sactive les ressources ne pouvant pas �tre produites
			RG_materiaux,
			RG_materiaux,
			//RG_materiaux,
			RG_materiaux,
			RG_materiaux,

			// Semi mat�riaux
			RG_semiMateriaux,
			RG_semiMateriaux,
			//RG_semiMateriaux,
			//RG_semiMateriaux,
			RG_semiMateriaux,
			RG_semiMateriaux,

			// Nourriture
			RG_nourriture,
			//RG_nourriture,
			RG_nourriture,
			//RG_nourriture,
			//RG_nourriture,
			//RG_nourriture,
			//RG_nourriture,
			RG_nourriture,
			RG_nourriture,
			RG_nourriture,
			RG_nourriture,
			//RG_nourriture,
			//RG_nourriture,
			//RG_nourriture,
			//RG_nourriture,
			//RG_nourriture,
			//RG_nourriture,
			//RG_nourriture,
			//RG_nourriture,
			//RG_nourriture,
			//RG_nourriture,
			//RG_nourriture,

			// Semi Nourriture
			//RG_semiNourriture,
#ifdef KEEP_UNUSED_RESSOURCES
			RG_semiNourriture,
#endif
			//RG_semiNourriture,
			//RG_semiNourriture,
#ifdef KEEP_UNUSED_RESSOURCES
			RG_semiNourriture,
#endif
			//RG_semiNourriture,
			//RG_semiNourriture,
			//RG_semiNourriture,
			//RG_semiNourriture,
			//RG_semiNourriture,
			//RG_semiNourriture,
			//RG_semiNourriture,
			//RG_semiNourriture,
			//RG_semiNourriture,
			//RG_semiNourriture,
			//RG_semiNourriture,
			//RG_semiNourriture,
			//RG_semiNourriture,
			//RG_semiNourriture,
			//RG_semiNourriture,
			//RG_semiNourriture,
			//RG_semiNourriture,
			RG_semiNourriture,
			RG_semiNourriture,
			//RG_semiNourriture,
			//RG_semiNourriture,
			//RG_semiNourriture,
			RG_semiNourriture,
			//RG_semiNourriture,
			RG_semiNourriture,
			RG_semiNourriture,
			RG_semiNourriture,
			//RG_semiNourriture,
			//RG_semiNourriture,
			RG_semiNourriture,
			//RG_semiNourriture,
			//RG_semiNourriture,
			//RG_semiNourriture,
			RG_semiNourriture,
			RG_semiNourriture,
			RG_semiNourriture,
#ifdef KEEP_UNUSED_RESSOURCES
			RG_semiNourriture,
			RG_semiNourriture,
#endif
			//RG_semiNourriture,
#ifdef KEEP_UNUSED_RESSOURCES
			RG_semiNourriture,
#endif
			//RG_semiNourriture,
			//RG_semiNourriture,
			//RG_semiNourriture,
			//RG_semiNourriture,
			//RG_semiNourriture,

			// Arbres et plantes
			//RG_arbres,
			//RG_arbres,
			//RG_arbres,

			// V�tement
			RG_vetements,
			//RG_vetements,
			//RG_vetements,
			//RG_vetements,
			//RG_vetements,
			//RG_vetements,
			//RG_vetements,
			//RG_vetements,
			//RG_vetements,
			//RG_vetements,
			//RG_vetements,
			//RG_vetements,
			//RG_vetements,

			// Semi V�tement
			//RG_semiVetements,
#ifdef KEEP_UNUSED_RESSOURCES
			RG_vetements,
#endif
			//RG_vetements,
#ifdef KEEP_UNUSED_RESSOURCES
			RG_vetements,
#endif

			// Outils d'�cole
			//RG_outilsEcole,
			//RG_outilsEcole,
			//RG_outilsEcole,
			//RG_outilsEcole,
			//RG_outilsEcole,
			//RG_outilsEcole,
			//RG_outilsEcole,
			//RG_outilsEcole,
#else				// En mode enfant : on r�activer le papier comme ressource consomm�e par les habitants pour leur satisfaction
			RG_outilsEcole,
#endif
#ifndef KID_VERSION	// En mode enfant : on d�sactive les ressources ne pouvant pas �tre produites
			//RG_outilsEcole,
			//RG_outilsEcole,
			//RG_outilsEcole,

			// Moyens de locomotion
			//RG_locomotion,
			//RG_locomotion,

			// Divers
			//RG_divers,
			//RG_divers,
			//RG_divers,
			//RG_divers,
			//RG_divers,
			//RG_divers,
			//RG_divers,
			//RG_divers,
			//RG_divers,
			//RG_divers,
			//RG_divers,
			//RG_divers,
			//RG_divers,
#endif
		};

		return ressourcesGroups[id];
	}
};

#endif
