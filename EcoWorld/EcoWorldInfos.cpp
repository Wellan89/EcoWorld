#include "EcoWorldInfos.h"
#include "EcoWorldSystem.h"
#include "EcoWorldModifiers.h"
#include "Batiment.h"

EcoWorldInfos::EcoWorldInfos(EcoWorldSystem* m_system, EcoWorldModifiers* m_modifiers) : system(m_system), modifiers(m_modifiers)
{
	reset();
}
void EcoWorldInfos::reset()
{
	// R�initialise les informations du monde
	budget = 0.0f;
	budgetEvolutionM = 0.0f;
	budgetEvolutionA = 0.0f;
	energie = 0.0f;
	pourcentageEnergieDisponible = 1.0f;
	effetSerre = 0.0f;
	effetSerreEvolutionM = 0.0f;
	dechets = 0.0f;
	dechetsEvolutionM = 0.0f;
	population = 0;
	popSatisfaction = 1.0f;
	popRealSatisfaction = 1.0f;

	// R�initialise les ressources
	int i;
	for (i = 0; i < Ressources::RI_COUNT; ++i)
		ressources[i] = 0.0f;

	// R�initialise les taxes
	taxesTotales = 0.0f;
	for (i = 0; i < TI_COUNT; ++i)
		taxes[i] = 0.0f;
}
void EcoWorldInfos::save(io::IAttributes* out, io::IXMLWriter* writer) const
{
	if (!out || !writer)
		return;

	out->addFloat("Budget", budget);
	out->addFloat("EffetSerre", effetSerre);
	out->addFloat("Dechets", dechets);

	out->write(writer, false, L"WorldInfos");
	out->clear();

	for (int i = 0; i < Ressources::RI_COUNT; ++i)
		out->addFloat(Ressources::getRessourceNameChar((Ressources::RessourceID)i), ressources[i]);

	out->write(writer, false, L"Ressources");
	out->clear();
}
void EcoWorldInfos::load(io::IAttributes* in, io::IXMLReader* reader)
{
	if (!in || !reader)
		return;

	// Remet les informations du monde � z�ro avant le chargement
	reset();

	reader->resetPosition();
	if (in->read(reader, false, L"WorldInfos"))
	{
		if (in->existsAttribute("Budget"))				budget = in->getAttributeAsFloat("Budget");
		if (in->existsAttribute("EffetSerre"))			effetSerre = in->getAttributeAsFloat("EffetSerre");
		if (in->existsAttribute("Dechets"))				dechets = in->getAttributeAsFloat("Dechets");

		in->clear();
	}

	reader->resetPosition();
	if (in->read(reader, false, L"Ressources"))
	{
		for (int i = 0; i < Ressources::RI_COUNT; ++i)
		{
			const char* ressourceName = Ressources::getRessourceNameChar((Ressources::RessourceID)i);
			if (in->existsAttribute(ressourceName))		ressources[i] = in->getAttributeAsFloat(ressourceName);
		}

		in->clear();
	}

	// Met � jour toutes les autres donn�es
	updateDonnees();
	//updateTaxes();					// Ne fonctionne plus depuis qu'un jour est automatiquement �coul� � chaque appel de ces fonctions
	//updateRessourcesConsommation();
}
void EcoWorldInfos::resetEvolutions()
{
	// Remet les �volutions � 0 pour qu'elles soient recalcul�es
	budgetEvolutionM = 0.0f;
	budgetEvolutionA = 0.0f;
	effetSerreEvolutionM = 0.0f;
	dechetsEvolutionM = 0.0f;
}
void EcoWorldInfos::updateDonnees()
{
	// Outils pour le calcul du pourcentage d'�nergie disponible
	float energieConsommeeTotale = 0.0f;	// L'�nergie totale consomm�e
	float energieProduiteTotale = 0.0f;		// L'�nergie totale produite

	// Recalcule l'�nergie et la population
	if (system)
	{
		energie = 0.0f;
		population = 0;

		const core::list<Batiment*>& listeAllBats = system->getAllBatiments();
		const core::list<Batiment*>::ConstIterator END = listeAllBats.end();
		Batiment* batiment = NULL;
		for (core::list<Batiment*>::ConstIterator it = listeAllBats.begin(); it != END; ++it)
		{
			batiment = (*it);
			if (!batiment->isConstructingOrDestroying())
			{
				population += batiment->getInfos().habitants;

				const float energieBat = batiment->getInfos().energie;
				if (energieBat > 0.0f)
				{
					// Accorde un bonus d'�nergie si elle est positive : si le batiment en produit
					const float energieFactor = (modifiers ? modifiers->energieFactor : 1.0f);
					energieProduiteTotale += energieBat * energieFactor;
					energie += energieBat * energieFactor;
				}
				else
				{
					energieConsommeeTotale -= energieBat;	// Le signe - sert � rendre cette valeur positive
					energie += energieBat;
				}
			}
		}
	}

	// Calcule le pourcentage d'�nergie disponible pour les b�timents
	if (energieConsommeeTotale > 0.0f)	// Si on consomme de l'�nergie, on peut faire ce calcul
		pourcentageEnergieDisponible = min(energieProduiteTotale / energieConsommeeTotale, 1.0f);	// On a bien s�r un maximum de 1.0f
	else								// Si on ne consomme pas d'�nergie, toute l'�nergie est disponible
		pourcentageEnergieDisponible = 1.0f;

	// Calcule la satisfaction r�elle de la population, d�pendante de la satisfaction g�n�rale de la popuation et du pourcentage d'�nergie disponible (facteur entre *0.5f � 0% d'�nergie � *1.0f � 100% d'�nergie)
	popRealSatisfaction = popSatisfaction * (pourcentageEnergieDisponible * 0.5f + 0.5f);
}
void EcoWorldInfos::updateTaxes()
{
	// V�rifie que l'effet de serre et les d�chets sont bien positifs
	if (effetSerre < 0.0f)
		effetSerre = 0.0f;
	if (dechets < 0.0f)
		dechets = 0.0f;

	float tmp = 0.0f;
	float taxesFactor = modifiers ? modifiers->taxesFactor : 1.0f;

	// Taxe de l'ES :
	// Formule : f(x) = ((x - 50000) / 1000) ^2				pour x entre 50 000 et 1 000 000
	//			 f(x) = f(50 000) = 0						pour x < 50 000
	//			 f(x) = f(1 000 000) = 902 500				pour x > 1 000 000
	if (effetSerre > 50000.0f)
	{
		if (effetSerre < 1000000.0f)
		{
			tmp = (effetSerre - 50000.0f) * 0.001f;
			taxes[TI_effetSerre] = (tmp * tmp) * taxesFactor;
		}
		else
			taxes[TI_effetSerre] = 902500.0f;	// = f(1 000 000)
	}
	else
		taxes[TI_effetSerre] = 0.0f;			// = f(50 000)

	// Taxe des d�chets :
	// Formule : f(x) = (((x - 100000) / 2000) ^2) * 2		pour x entre 100 000 et 2 000 000
	//			 f(x) = f(100 000) = 0						pour x < 100 000
	//			 f(x) = f(2 000 000) = 1 805 000			pour x > 2 000 000
	if (dechets > 100000.0f)
	{
		if (dechets < 2000000.0f)
		{
			tmp = (dechets - 100000.0f) * 0.0005f;
			taxes[TI_dechets] = ((tmp * tmp) * 2) * taxesFactor;
		}
		else
			taxes[TI_dechets] = 1805000.0f;		// = f(2 000 000)
	}
	else
		taxes[TI_dechets] = 0.0f;				// = f(100 000)

	// Calcule les taxes totales
	taxesTotales = 0.0f;
	for (int i = 0; i < TI_COUNT; ++i)
		taxesTotales += taxes[i];

	// Diminue le budget suivant les taxes
	budgetEvolutionM -= taxesTotales;
	budgetEvolutionA -= taxesTotales * 12.0f;
	if (taxesTotales != 0.0f)
		budget -= taxesTotales / 30.0f;
}
void EcoWorldInfos::updateRessourcesConsommation()
{
	// V�rifie qu'il y a au moins un habitant dans le monde
	if (population == 0)
	{
		// Si il n'y a aucun habitant dans le monde, alors la satisfaction de la population est de 100%
		popSatisfaction = 1.0f;
		popRealSatisfaction = 1.0f;
		return;
	}

	// Replace toutes les ressources n�gatives � 0.0f avant le calcul des ressources consomm�es
	//	(m�me si normalement tout a �t� prot�g� pour que �a ne puisse pas se produire, il reste encore actuellement un risque :
	//	voir EcoWorldSystem.cpp : calculatePourcentageRessourcesDisponibles())
	//	(si des ressources sont d�j� n�gatives, elles pourraient influencer le calcul de la satisfaction de la population)
	for (int i = 0; i < Ressources::RI_COUNT; ++i)
		ressources[i] = max(ressources[i], 0.0f);

	// Repr�sente le nombre total de ressources diff�rentes sur lesquelles repose la satisfaction de la population (c'est donc un nombre entier, mais �crit sous forme de float)
#ifndef KID_VERSION	// En mode enfant : on d�sactive les ressources ne pouvant pas �tre produites
#define NB_RESSOURCES	18.0f
#else				// En mode enfant : on r�activer le papier comme ressource consomm�e par les habitants pour leur satisfaction
#define NB_RESSOURCES	2.0f
#endif

	const float factor = (modifiers ? modifiers->ressourcesConsommationFactor : 1.0f) * population / 30.0f;
	popSatisfaction = 0.0f;


	// Consommation des ressources par mois (!) par habitant (en unit� de ressources) :
	ressources[Ressources::RI_eau] -= 45.0f				* factor; // Seulement la consommation en boisson !

#ifndef KID_VERSION	// En mode enfant : on d�sactive les ressources ne pouvant pas �tre produites
	ressources[Ressources::RI_viande] -= 6.0f			* factor;
	//ressources[Ressources::RI_veau] -= 1.0f				* factor;
	//ressources[Ressources::RI_agneau] -= 1.0f			* factor;
	//ressources[Ressources::RI_poulet] -= 3.0f			* factor;
	//ressources[Ressources::RI_cheval] -= 1.0f			* factor;
	//ressources[Ressources::RI_porc] -= 1.0f				* factor;
	//ressources[Ressources::RI_boeuf] -= 1.0f			* factor;

	ressources[Ressources::RI_poisson] -= 1.0f			* factor;
	//ressources[Ressources::RI_truite] -= 2.0f			* factor;
	//ressources[Ressources::RI_saumon] -= 1.0f			* factor;
	//ressources[Ressources::RI_thon] -= 0.3f				* factor;
	//ressources[Ressources::RI_moule] -= 0.5f			* factor;
	//ressources[Ressources::RI_crevette] -= 0.25f		* factor;

	ressources[Ressources::RI_pomme] -= 1.5f			* factor;
	ressources[Ressources::RI_poire] -= 1.5f			* factor;
	//ressources[Ressources::RI_prune] -= 1.0f			* factor;
	//ressources[Ressources::RI_cerise] -= 1.0f			* factor;
	//ressources[Ressources::RI_noix] -= 1.0f				* factor;
	ressources[Ressources::RI_banane] -= 1.5f			* factor;
	//ressources[Ressources::RI_peche] -= 1.0f			* factor;
	ressources[Ressources::RI_orange] -= 1.5f			* factor;
	ressources[Ressources::RI_raisin] -= 0.5f			 * factor;

	ressources[Ressources::RI_tomate] -= 2.0f			* factor;
	//ressources[Ressources::RI_potiron] -= 1.0f			* factor;
	//ressources[Ressources::RI_melon] -= 1.0f			* factor;
	ressources[Ressources::RI_haricots] -= 1.0f			* factor;
	//ressources[Ressources::RI_haricotsCoco] -= 1.0f		* factor;
	//ressources[Ressources::RI_radis] -= 4.0f			* factor;
	//ressources[Ressources::RI_courgette] -= 1.0f		* factor;
	ressources[Ressources::RI_pommesTerre] -= 3.0f		* factor;
	ressources[Ressources::RI_salade] -= 5.0f			* factor;
	ressources[Ressources::RI_carotte] -= 1.5f			* factor;

	ressources[Ressources::RI_pain] -= 6.0f				* factor;	// 6 kg de pain : 15 pains * 400 g
	ressources[Ressources::RI_huile] -= 1.0f			* factor;
	ressources[Ressources::RI_lait] -= 15.0f			* factor;
	//ressources[Ressources::RI_moutarde] -= 0.15f		* factor;
	//ressources[Ressources::RI_biscuit] -= 0.5f			* factor;
	//ressources[Ressources::RI_confiture] -= 0.1f		* factor;
	//ressources[Ressources::RI_sel] -= 0.1f				* factor;
	//ressources[Ressources::RI_poivre] -= 0.12f			* factor;
	ressources[Ressources::RI_vin] -= 3.0f				* factor;
	//ressources[Ressources::RI_sucre] -= 0.15f			* factor;
	//ressources[Ressources::RI_miel] -= 0.1f				* factor;

	//ressources[Ressources::RI_cafe] -= 1.0f / 3.0f		* factor;
	//ressources[Ressources::RI_chocolat] -= 1.0f / 12.0f	* factor;
	//ressources[Ressources::RI_the] -= 1.0f / 12.0f		* factor;

	ressources[Ressources::RI_vetements] -= 1.5f		* factor;
#else				// En mode enfant : on r�activer le papier comme ressource consomm�e par les habitants pour leur satisfaction
	ressources[Ressources::RI_papier] -= 30.0f			* factor;	// Valeur volontairement assign�e assez haute (peu r�aliste : 30 kg/habitant/M !) pour ajouter un peu de difficult� � cette version du jeu
#endif


	// Recalcule la satisfaction de la population par la quantit� de ressources n�gatives
	// Ancienne formule :				popSatisfaction -= max(-ressources[RessourceID], 0.0f) / ressourceQuantityNeeded;
	// Nouvelle formule �quivalente :	popSatisfaction += min(ressources[RessourceID], 0.0f) / ressourceQuantityNeeded;
	popSatisfaction += min(ressources[Ressources::RI_eau], 0.0f) / 45.0f;			// Eau

#ifndef KID_VERSION	// En mode enfant : on d�sactive les ressources ne pouvant pas �tre produites
	popSatisfaction += min(ressources[Ressources::RI_viande], 0.0f) / 6.0f;			// Viande
	//popSatisfaction += min(ressources[Ressources::RI_veau], 0.0f) / 1.0f;			// Veau
	//popSatisfaction += min(ressources[Ressources::RI_agneau], 0.0f) / 1.0f;		// Agneau
	//popSatisfaction += min(ressources[Ressources::RI_poulet], 0.0f) / 3.0f;		// Poulet
	//popSatisfaction += min(ressources[Ressources::RI_cheval], 0.0f) / 1.0f;		// Cheval
	//popSatisfaction += min(ressources[Ressources::RI_porc], 0.0f) / 1.0f;			// Porc
	//popSatisfaction += min(ressources[Ressources::RI_boeuf], 0.0f) / 1.0f;		// Boeuf

	popSatisfaction += min(ressources[Ressources::RI_poisson], 0.0f);				// Poisson
	//popSatisfaction += min(ressources[Ressources::RI_truite], 0.0f) / 2.0f;		// Truite
	//popSatisfaction += min(ressources[Ressources::RI_saumon], 0.0f) / 1.0f;		// Saumon
	//popSatisfaction += min(ressources[Ressources::RI_thon], 0.0f) / 0.3f;			// Thon
	//popSatisfaction += min(ressources[Ressources::RI_moule], 0.0f) / 0.5f;		// Moule
	//popSatisfaction += min(ressources[Ressources::RI_crevette], 0.0f) / 0.25f;	// Crevette

	popSatisfaction += min(ressources[Ressources::RI_pomme], 0.0f) / 1.5f;			// Pomme
	popSatisfaction += min(ressources[Ressources::RI_poire], 0.0f) / 1.5f;			// Poire
	//popSatisfaction += min(ressources[Ressources::RI_prune], 0.0f) / 1.0f;		// Prune
	//popSatisfaction += min(ressources[Ressources::RI_cerise], 0.0f) / 1.0f;		// Cerise
	//popSatisfaction += min(ressources[Ressources::RI_noix], 0.0f) / 1.0f;			// Noix
	popSatisfaction += min(ressources[Ressources::RI_banane], 0.0f) / 1.5f;			// Banane
	//popSatisfaction += min(ressources[Ressources::RI_peche], 0.0f) / 1.0f;		// P�che
	popSatisfaction += min(ressources[Ressources::RI_orange], 0.0f) / 1.5f;			// Orange
	popSatisfaction += min(ressources[Ressources::RI_raisin], 0.0f) * 2.0f;			// Raisin

	popSatisfaction += min(ressources[Ressources::RI_tomate], 0.0f) * 0.5f;			// Tomate
	//popSatisfaction += min(ressources[Ressources::RI_potiron], 0.0f) / 1.0f;		// Potiron
	//popSatisfaction += min(ressources[Ressources::RI_melon], 0.0f) / 1.0f;		// Melon
	popSatisfaction += min(ressources[Ressources::RI_haricots], 0.0f);				// Haricots
	//popSatisfaction += min(ressources[Ressources::RI_haricotsCoco], 0.0f) / 1.0f;	// Haricots coco
	//popSatisfaction += min(ressources[Ressources::RI_radis], 0.0f) / 4.0f;		// Radis
	//popSatisfaction += min(ressources[Ressources::RI_courgette], 0.0f) / 1.0f;	// Courgette
	popSatisfaction += min(ressources[Ressources::RI_pommesTerre], 0.0f) / 3.0f;	// Pommes de terre
	popSatisfaction += min(ressources[Ressources::RI_salade], 0.0f) * 0.2f;			// Salade
	popSatisfaction += min(ressources[Ressources::RI_carotte], 0.0f) / 1.5f;		// Carotte

	popSatisfaction += min(ressources[Ressources::RI_pain], 0.0f) / 6.0f;			// Pain
	popSatisfaction += min(ressources[Ressources::RI_huile], 0.0f);					// Huile
	popSatisfaction += min(ressources[Ressources::RI_lait], 0.0f) / 15.0f;			// Lait
	//popSatisfaction += min(ressources[Ressources::RI_moutarde], 0.0f) / 0.15f;	// Moutarde
	//popSatisfaction += min(ressources[Ressources::RI_biscuit], 0.0f) / 0.5f;		// Biscuit
	//popSatisfaction += min(ressources[Ressources::RI_confiture], 0.0f) / 0.1f;	// Confiture
	//popSatisfaction += min(ressources[Ressources::RI_sel], 0.0f) / 0.1f;			// Sel
	//popSatisfaction += min(ressources[Ressources::RI_poivre], 0.0f) / 0.12f;		// Poivre
	popSatisfaction += min(ressources[Ressources::RI_vin], 0.0f) / 3.0f;			// Vin
	//popSatisfaction += min(ressources[Ressources::RI_sucre], 0.0f) / 0.15f;		// Sucre
	//popSatisfaction += min(ressources[Ressources::RI_miel], 0.0f) / 0.1f;			// Miel

	//popSatisfaction += min(ressources[Ressources::RI_cafe], 0.0f) / (1.0f / 3.0f);		// Caf�
	//popSatisfaction += min(ressources[Ressources::RI_chocolat], 0.0f) / (1.0f / 12.0f);	// Chocolat
	//popSatisfaction += min(ressources[Ressources::RI_the], 0.0f) / (1.0f / 12.0f);		// Th�

	popSatisfaction += min(ressources[Ressources::RI_vetements], 0.0f) / 1.5f;		// V�tements
#else				// En mode enfant : on r�activer le papier comme ressource consomm�e par les habitants pour leur satisfaction
	popSatisfaction += min(ressources[Ressources::RI_papier], 0.0f) / 30.0f;		// Papier
#endif


	// On divise la satisfaction de la population par le facteur de consommation des ressources et par le nombre de ressources n�cessaires
	popSatisfaction /= (factor * NB_RESSOURCES);

	// On lui ajoute 1 pour rendre ce nombre compris entre -1 et 0 dans la plage 0 et 1
	popSatisfaction += 1.0f;

	// Calcule la satisfaction r�elle de la population d�pendante de la satisfaction g�n�rale de la popuation et du pourcentage d'�nergie disponible (facteur entre *0.5f � 0% d'�nergie � *1.0f � 100% d'�nergie)
	popRealSatisfaction = popSatisfaction * (pourcentageEnergieDisponible * 0.5f + 0.5f);



	// Replace toutes les ressources n�gatives � 0.0f
	for (int i = 0; i < Ressources::RI_COUNT; ++i)
		ressources[i] = max(ressources[i], 0.0f);
}
