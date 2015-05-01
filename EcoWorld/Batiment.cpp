#include "Batiment.h"
#include "EcoWorldSystem.h"
#include "EcoWorldModifiers.h"

Batiment::Batiment(BatimentID batimentID, EcoWorldSystem& m_system, EcoWorldInfos& m_worldInfos, EcoWorldModifiers& m_modifiers,
	const core::vector2di m_index, float m_rotation)
 : system(m_system), ID(batimentID), infos(batimentID), worldInfos(m_worldInfos), modifiers(m_modifiers), staticInfos(StaticBatimentInfos::getInfos(batimentID)),
 index(m_index), rotation(m_rotation), daysPassed(0), destroyingDay((u32)(-1)), sceneNode(NULL)
{
	// Calcule la Bounding Box de ce bâtiment
	box = obbox2df(core::rectf((float)index.X - staticInfos.halfTaille.Width, (float)index.Y - staticInfos.halfTaille.Height,
		(float)index.X + staticInfos.halfTaille.Width, (float)index.Y + staticInfos.halfTaille.Height),
		rotation);
}
void Batiment::onConstruct()
{
	// Ajoute les coûts de construction au monde
	worldInfos.budget += staticInfos.prixC * modifiers.prixFactor;
	worldInfos.effetSerre += staticInfos.effetSerreC * modifiers.effetSerreFactor;
	worldInfos.dechets += staticInfos.dechetsC * modifiers.dechetsFactor;

	const u32 ressourcesCSize = staticInfos.ressourcesC.size();
	for (u32 i = 0; i < ressourcesCSize; ++i)
		worldInfos.ressources[staticInfos.ressourcesC[i].ID] += staticInfos.ressourcesC[i].production;


	// Si ce bâtiment est une maison n'ayant pas de temps de construction, on lui assigne directement comme nombre d'habitants le nombre d'habitants à sa construction.
	// Le cas où l'habitation a un temps de construction non nul est géré dans Batiment::update.
	if (staticInfos.isHabitation && staticInfos.tempsC == 0)
		infos.habitants = staticInfos.habitantsC;
}
void Batiment::onDestroy()
{
	// Ajoute les coûts de destruction au monde
	worldInfos.budget += staticInfos.prixD;
	worldInfos.effetSerre += staticInfos.effetSerreD;
	worldInfos.dechets += staticInfos.dechetsD;

	const u32 ressourcesDSize = staticInfos.ressourcesD.size();
	for (u32 i = 0; i < ressourcesDSize; ++i)
	{
		const Ressources::RessourceID currentRessourceID = staticInfos.ressourcesD[i].ID;
		if (staticInfos.isTree && currentRessourceID == Ressources::RI_bois)	// Si ce bâtiment est un arbre, et que la ressource à gérer est le bois à sa destruction :
			worldInfos.ressources[Ressources::RI_bois] += staticInfos.ressourcesD[i].production * min((float)daysPassed / 360.0f, 1.0f);	// Un arbre fournit du bois à sa destruction proportionnellement à son âge (jusqu'à un an) : plus il est grand, plus il fournit de bois
		else
			worldInfos.ressources[currentRessourceID] += staticInfos.ressourcesD[i].production;
	}
}
void Batiment::save(io::IAttributes* out) const
{
	if (!out)
		return;

	// Nouvelle méthode d'enregistrement des bâtiments : on n'enregistre plus leur ID mais leur nom, pour éviter une complète invalidation des sauvegardes en cas de changement des ID des bâtiments
	//out->addInt("ID", (int)ID);

	// Enregistre le nom de ce bâtiment, donné par ses informations statiques
	out->addString("Name", staticInfos.name);

	out->addPosition2d("Index", index);
	out->addFloat("Rotation", rotation);

	// Enregistre les informations sur l'âge et les dates de ce bâtiment
	out->addInt("DaysPassed", daysPassed);
	out->addInt("DestroyingDay", destroyingDay);

	// Enregistre les informations variables sur ce bâtiment
	out->addInt("DureeVie", infos.dureeVie);
	out->addFloat("CurrentEnergieConsommationM", infos.currentEnergieConsommationM);
	out->addFloat("CurrentEauConsommationM", infos.currentEauConsommationM);
	out->addFloat("PourcentageProduction", infos.pourcentageProduction);
	out->addInt("Habitants", (int)infos.habitants);
	out->addFloat("NextHabitantPercent", infos.nextHabitantPercent);

	// Les infos du bâtiment seront enregistrées par le système
}
void Batiment::load(io::IAttributes* in)
{
	if (!in)
		return;

	// Les infos du bâtiment ont déjà été lues par le système, il ne reste plus qu'à les récupérer
	// L'index, la rotation et l'ID de ce bâtiment ont déjà été lus et assignés par le système

	// Charge les informations sur l'âge et les dates de ce bâtiment
	if (in->existsAttribute("DaysPassed"))					daysPassed = in->getAttributeAsInt("DaysPassed");
	if (in->existsAttribute("DestroyingDay"))				destroyingDay = in->getAttributeAsInt("DestroyingDay");

	// Charge les informations variables sur ce bâtiment
	if (in->existsAttribute("DureeVie"))					infos.dureeVie = in->getAttributeAsInt("DureeVie");
	if (in->existsAttribute("CurrentEnergieConsommationM"))	infos.currentEnergieConsommationM = in->getAttributeAsFloat("CurrentEnergieConsommationM");
	if (in->existsAttribute("CurrentEauConsommationM"))		infos.currentEauConsommationM = in->getAttributeAsFloat("CurrentEauConsommationM");
	if (in->existsAttribute("PourcentageProduction"))		infos.pourcentageProduction = in->getAttributeAsFloat("PourcentageProduction");
	if (in->existsAttribute("Habitants"))					infos.habitants = (u32)abs(in->getAttributeAsInt("Habitants"));
	if (in->existsAttribute("NextHabitantPercent"))			infos.nextHabitantPercent = in->getAttributeAsFloat("NextHabitantPercent");
}
void Batiment::updateDaysPassed(bool* outNormalBatimentDestroy, bool* outOldBatimentDestroy)
{
	// Note : On ne réinitialise pas les valeurs de outNormalBatimentDestroy et de outOldBatimentDestroy au cas où un bâtiment aurait déjà été détruit lors d'une mise à jour précédente d'un bâtiment

	// Met à jour l'âge du bâtiment (on lui ajoute 1 jour de plus)
	++daysPassed;

	// Vérifie que le bâtiment n'est pas en train d'être détruit
	if (isDestroying())
	{
		// Vérifie que le bâtiment n'est pas détruit
		if (system.getTime().getTotalJours() >= (staticInfos.tempsD + destroyingDay) || staticInfos.tempsD == 0)
		{
			// Le bâtiment est effectivement détruit : on indique au système qu'il doit être supprimé
			system.addBatimentToDeleteList(this);

			// Indique qu'un bâtiment a été détruit volontairement
			if (outNormalBatimentDestroy)
				(*outNormalBatimentDestroy) = true;
		}
	}

	// Vérifie que la durée de vie du bâtiment n'est pas terminée
	else if (infos.dureeVie != 0 && daysPassed >= infos.dureeVie)
	{
		// Le bâtiment doit effectivement être détruit : on indique au système qu'il doit être supprimé
		system.addBatimentToDeleteList(this);

		// Indique qu'un bâtiment a été détruit involontairement : à cause de sa durée de vie
		if (outOldBatimentDestroy)
			(*outOldBatimentDestroy) = true;
	}
}
void Batiment::updateBatimentInfos(bool firstCall, const float pourcentageRessourcesDisponibles[Ressources::RI_COUNT])
{
	// Vérifie que le bâtiment n'est pas en train d'être construit ou détruit, sinon on quitte
	if (isConstructingOrDestroying())
		return;

	// Retiens certaines valeurs spécifiques à ce bâtiment pour déterminer si une mise à jour des informations sur la population et l'énergie du monde sera nécessaire à la fin de cette fonction
	const float lastEnergie = infos.energie;
	const u32 lastHabitants = infos.habitants;



	// Met à jour les informations spécifiques de ce bâtiment pour ce jour :

	// Calcule l'énergie consommé par ce bâtiment d'après son pourcentage de production demandé par l'utilisateur
	// Note : L'énergie consommée par les habitations doit être constante, indépendamment du nombre d'habitants des habitations !
	// En effet, le nombre d'habitants des maisons est déjà lui-même dépendant de l'énergie du monde disponible
	// (par la vitesse d'emménagement des habitants, dépendante de la satisfaction réelle, elle-même dépendante de l'énergie disponible),
	// donc si ce cas venait à se produire ces deux valeurs seraient mutuellement dépendantes et provoqueraient des erreurs dans les mises à jour du monde !
	if (ID != BI_centrale_charbon)	// Exception : Ne change pas l'énergie de la centrale à charbon, ce qui provoquerait des erreurs dans son calcul de l'énergie du monde nécessaire lors de l'appel à la méthode updateBatimentInfosFromBatimentID
		infos.energie = staticInfos.energie * infos.pourcentageProduction;

	// Calcule l'eau consommée actuellement par ce bâtiment, limitée par le pourcentage d'eau disponible
	infos.eauConsommationJ = staticInfos.eauConsommationJ * pourcentageRessourcesDisponibles[Ressources::RI_eau];

	// Vérifie si ce bâtiment est une habitation
	if (staticInfos.isHabitation)
	{
		// Si c'est la première mise à jour de cette maison, on lui assigne comme nombre d'habitants le nombre d'habitants à sa construction.
		// Ici, on ne gère que le cas où l'habitation a un temps de construction non nul.
		if (daysPassed == staticInfos.tempsC && staticInfos.tempsC != 0)
			infos.habitants = staticInfos.habitantsC;

		// Si c'est le premier appel de cette fonction ce jour, on actualise le nombre d'habitants dans cette maison
		if (firstCall)
		{
			// Calcule la vitesse d'emménagement/de déménagement des habitants dans cette maison :
			// A 0% de satisfaction :	La vitesse est -staticInfos.habitantsJ (déménagement le plus rapide).
			// A 20% de satisfaction :	La vitesse est 0.0f (aucune modification).
			// A 40% de satisfaction :	La vitesse est staticInfos.habitantsJ (emménagement le plus rapide).
			const float minSatisfaction = HABITANTS_MIN_SATISFACTION * modifiers.minSatisfactionFactor;
			const float emmenagementSpeed = core::clamp((worldInfos.popRealSatisfaction - minSatisfaction) / minSatisfaction, -1.0f, 1.0f) * staticInfos.habitantsJ;

			// Calcule le pourcentage de progression du prochain habitant et augmente ou diminue le nombre d'habitants si nécessaire
			infos.nextHabitantPercent += emmenagementSpeed;
			while (infos.nextHabitantPercent >= 1.0f)	// Le while permet de gérer le cas où plusieurs habitants ont emménagés en même temps
			{
				if (infos.habitants < staticInfos.habitantsMax)
					++infos.habitants;
				infos.nextHabitantPercent -= 1.0f;
			}
			while (infos.nextHabitantPercent <= 0.0f)	// Le while permet de gérer le cas où plusieurs habitants ont déménagés en même temps
			{
				if (infos.habitants > 1)
					--infos.habitants;
				infos.nextHabitantPercent += 1.0f;
			}

			// Si on est au minimum ou au maximum d'habitants dans cette maison
			if (infos.habitants <= 1 || infos.habitants >= staticInfos.habitantsMax)
			{
				// Force le nombre d'habitants dans cette maison à être compris entre 1 habitant et le maximum d'habitants dans cette maison
				infos.habitants = core::clamp(infos.habitants, (u32)1, staticInfos.habitantsMax);

				// Réinitialise le pourcentage de progression du prochain habitant
				infos.nextHabitantPercent = 0.0f;
			}
		}

		// Rend l'eau consommée actuellement par cette maison dépendante de son nombre d'habitants actuel
		const float habitantsFactor = (float)(infos.habitants) / (float)(staticInfos.habitantsMax);
		infos.eauConsommationJ *= habitantsFactor;

		// Calcule le loyer réel de cette habitation
		// Le loyer des habitations est dépendant de la satisfaction réelle de la population et du nombre d'habitants actuel dans la maison
		infos.loyerM = staticInfos.loyerM * worldInfos.popRealSatisfaction * habitantsFactor;

		// Calcule les impôts annuels de cette habitation, dépendants du nombre d'habitants actuel dans la maison
		infos.impotsA = staticInfos.impotsA * habitantsFactor;
	}
	else
	{
		// Ces valeurs ne sont valides que pour les habitations
		infos.loyerM = 0.0f;
		infos.impotsA = 0.0f;
		infos.habitants = 0;
		infos.nextHabitantPercent = 0.0f;
	}

	// Vérifie si ce bâtiment est un arbre
	if (staticInfos.isTree && daysPassed < 360)
	{
		// Les arbres absorbent une quantité d'effet de serre relative à leur ancienneté (quadratiquement entre 0 et 1 an) :
		// facteur = f(j) = (j / 400)² + 0.19
		float facteur = daysPassed * 0.0025f;	// 1 / 400 = 0.0025
		facteur *= facteur;
		facteur += 0.19f;
		infos.effetSerreJ = staticInfos.effetSerreJ * facteur;
	}
	else
		infos.effetSerreJ = staticInfos.effetSerreJ;

	// Indique les déchets produits/absorbés par jour d'après les informations statiques sur ce bâtiment
	infos.dechetsJ = staticInfos.dechetsJ;

	// Calcule l'entretien de ce bâtiment d'après son pourcentage de production demandé par l'utilisateur
	infos.entretienM = staticInfos.entretienM * (infos.pourcentageProduction * 0.5f + 0.5f);

	// Calcule le pourcentage de production actuel de ce bâtiment :
	// (limité par la ressource disponible la plus basse, ainsi que par le pourcentage de production maximal demandé par l'utilisateur)
	infos.pourcentageProductionActuel = infos.pourcentageProduction;
	const u32 ressourcesSize = infos.ressourcesJ.size();
	if (ressourcesSize)
	{
#ifdef KID_VERSION
		// Vérifie si ce bâtiment est un arbre
		if (staticInfos.isTree)
		{
			// Ce bâtiment est un arbre :

			// En mode enfant, un arbre produit du bois chaque jour proportionnellement à son âge (jusqu'à un an) : plus il est grand, plus il fournit de bois.
			// A un an (et plus) (à sa taille maximale) : 5 kg/arbre/J.
			// En effet, il est pour le moment impossible de produire du bois autrement que par la destruction manuelle des arbres !

			// Met à jour la production/consommation réelle de chaque ressource produite par cet arbre (juste du bois normalement, mais on parcours toutes les ressources au cas où d'autres ressources auraient été ajoutées)
			for (u32 i = 0; i < ressourcesSize; ++i)
				infos.ressourcesJ[i].production = staticInfos.ressourcesJ[i].production * min((float)daysPassed / 360.0f, 1.0f);
		}
		else
#endif
		{
			// Ce bâtiment est sûrement une usine :

			// Détermine le pourcentage de production actuel suivant la disponibilité des ressources consommées par ce bâtiment
			for (u32 i = 0; i < ressourcesSize; ++i)
				if (staticInfos.ressourcesJ[i].production < 0.0f)	// Une ressource consommée et non disponible complètement limite le pourcentage de production actuel
					infos.pourcentageProductionActuel = min(pourcentageRessourcesDisponibles[infos.ressourcesJ[i].ID], infos.pourcentageProductionActuel);

			// Le pourcentage de production actuel est aussi directement limité par le pourcentage d'énergie disponible du monde (uniquement si ce bâtiment consomme de l'énergie)
			if (staticInfos.energie < 0.0f)
				infos.pourcentageProductionActuel *= worldInfos.pourcentageEnergieDisponible;

			// Met à jour la production/consommation réelle de chaque ressource pour ce jour
			for (u32 i = 0; i < ressourcesSize; ++i)
				infos.ressourcesJ[i].production = staticInfos.ressourcesJ[i].production * infos.pourcentageProductionActuel;
		}
	}


	// Met à jour les informations spécifiques de ce bâtiment suivant son ID
	updateBatimentInfosFromBatimentID(pourcentageRessourcesDisponibles);


	// Si certaines informations de ce bâtiment ayant un impact sur les données du monde (l'énergie produite/consommée par ce bâtiment et son nombre d'habitants) ont été modifiées :
	if (infos.energie != lastEnergie || infos.habitants != lastHabitants)
	{
		// On met à jour les informations du monde
		worldInfos.updateDonnees();

#ifdef _DEBUG
		// Vérification de débogage :
		// Vérifie que l'énergie consommée par les habitations reste constante (voir ci-dessus pour plus d'informations, lors du calcul de l'énergie consommé par ce bâtiment)
		if (staticInfos.isHabitation && infos.energie != lastEnergie
			&& daysPassed != staticInfos.tempsC)	// Vérifie aussi que cette habitation ne vient pas d'être construite : dans ce cas il est normal que son énergie soit initialisée à sa bonne valeur
			LOG_DEBUG("Batiment::updateBatimentInfos(...) : Ce batiment est une habitation et son énergie consommée n'est pas constante : ID = " << ID << " ; energie = " << infos.energie << " ; lastEnergie = " << lastEnergie, ELL_ERROR);
#endif
	}
}
void Batiment::updateBatimentInfosFromBatimentID(const float pourcentageRessourcesDisponibles[Ressources::RI_COUNT])
{
	// Détermine quel est l'ID actuel du bâtiment
	switch (ID)
	{
	case BI_panneau_solaire:
		// La production d'énergie des panneaux solaires est relative au facteur d'énergie lumineuse du temps actuel
		infos.energie = staticInfos.energie * system.getWeatherManager().getCurrentWeatherInfos().energyFactor;
		break;

	case BI_centrale_charbon:
		// Calcule l'énergie produite par la centrale à charbon
		infos.energie = core::min_(min(
			max(infos.energie - worldInfos.energie, 0.0f),					// La centrale à charbon ne doit produire que l'électricité nécessaire pour ne pas gâcher de bois (on n'oublie pas de soustraire la production actuelle d'énergie de la centrale à charbon pour connaître l'énergie totale qu'elle doit produire)
			CENTRALE_CHARBON_ENERGIE_MAX * infos.pourcentageProduction),	// La centrale à charbon ne produit pas plus d'énergie qu'elle ne peut en produire au maximum (aussi limité par son pourcentage de production désiré)
			worldInfos.ressources[Ressources::RI_bois] * pourcentageRessourcesDisponibles[Ressources::RI_bois] * CENTRALE_CHARBON_RENTABILITE,	// La centrale à charbon ne produit pas plus d'énergie qu'il ne reste de bois
			worldInfos.ressources[Ressources::RI_eau] * pourcentageRessourcesDisponibles[Ressources::RI_eau] * CENTRALE_CHARBON_RENTABILITE / CENTRALE_CHARBON_CONSOMMATION_EAU_FACTEUR);	// La centrale à charbon ne produit pas plus d'énergie qu'il ne reste d'eau pour son refroidissement

		// Caclule la consommation de bois de la centrale : le premier élement de ce tableau de ressources représente le bois
		infos.ressourcesJ[0].production = infos.energie * CENTRALE_CHARBON_RENTABILITE_OPP_INV;

		// Calcule la consommation d'eau de la centrale (proportionnel à la consommation de bois) : le deuxième élement de ce tableau de ressources représente l'eau
		infos.ressourcesJ[1].production = infos.ressourcesJ[0].production * CENTRALE_CHARBON_CONSOMMATION_EAU_FACTEUR;

		// Calcule le pourcentage de production actuel de la centrale par rapport à sa production maximale
		infos.pourcentageProductionActuel = infos.energie / CENTRALE_CHARBON_ENERGIE_MAX;

		// Son entretien est relatif à son pourcentage de production désiré à 50 %
		infos.entretienM = staticInfos.entretienM * (infos.pourcentageProduction * 0.5f + 0.5f);
		break;

	case BI_usine_incineration_dechets:
		// L'usine d'incinération des déchets produits proportionnellement de l'effet de serre par rapport aux déchets brûlés
		// et essaie toujours de brûler les déchets le plus rapidement possible.
		// Les déchets maximaux qu'elle peut brûler sont aussi réglés par le pourcentage de production de l'usine demandé par l'utilisateur.
		const float dechetsMax = USINE_INCINERATION_DECHETS_MAX * infos.pourcentageProduction * worldInfos.pourcentageEnergieDisponible;
		infos.dechetsJ = -min(worldInfos.dechets, dechetsMax);
		infos.effetSerreJ = infos.dechetsJ * -USINE_INCINERATION_RENTABILITE;

		// Calcule le pourcentage de production actuel de l'usine par rapport aux déchets maximaux qu'elle peut éliminer
		infos.pourcentageProductionActuel = infos.dechetsJ * USINE_INCINERATION_DECHETS_MAX_OPP_INV;

		// Sa consommation d'énergie est relative à son pourcentage de production demandé à 50 %
		infos.energie = staticInfos.energie * (infos.pourcentageProduction * 0.5f + 0.5f);

		// Son entretien est relatif à son pourcentage de production désiré à 50 %
		infos.entretienM = staticInfos.entretienM * (infos.pourcentageProduction * 0.5f + 0.5f);
		break;
	}
}
void Batiment::update(bool moisEcoule, bool anneeEcoulee)
{
	// Vérifie que le bâtiment n'est pas en train d'être construit ou détruit, sinon on quitte
	if (isConstructingOrDestroying())
		return;

	u32 i = 0;	// Variable d'itération



	// Met à jour les informations du système :

	// Par an
	// Ajoute les impôts si c'est une habitation, qui dépendent du nombre d'habitants actuel dans la maison
	if (anneeEcoulee && staticInfos.isHabitation)
		worldInfos.budget += infos.impotsA;

	// Par mois
	if (moisEcoule)
	{
		worldInfos.budget -= infos.entretienM;

		// Ajoute le loyer et les factures d'électricité et d'eau (aux habitations seulement !)
		if (staticInfos.isHabitation)
		{
			worldInfos.budget += infos.loyerM;

			if (infos.currentEnergieConsommationM > 0.0f)
				worldInfos.budget += infos.currentEnergieConsommationM * FACTURE_ELECTRICITE;
			if (infos.currentEauConsommationM > 0.0f)
				worldInfos.budget += infos.currentEauConsommationM * FACTURE_EAU;
		}
		infos.currentEnergieConsommationM = 0.0f;
		infos.currentEauConsommationM = 0.0f;
	}

	// Par jour (effectué obligatoirement après "Par mois" pour que les variables infos.currentEnergieConsommationM et infos.currentEauConsommationM qui viennent d'être réinitialisées à 0.0f soient recalculées)
	{
		// Ajoute l'effet de serre et les déchets au monde
		worldInfos.effetSerre += infos.effetSerreJ;
		worldInfos.dechets += infos.dechetsJ;

#ifdef _DEBUG
		// Vérifie que la consommation d'eau de ce bâtiment n'est jamais négative
		if (infos.eauConsommationJ < 0.0f)
			LOG_DEBUG("Batiment::update(...) : La consommation d'eau de ce batiment est negative : infos.eauConsommationJ = " << infos.eauConsommationJ << " ; ID = " << ID <<" !", ELL_WARNING);
#endif

		// Retire l'eau consommée du monde
		worldInfos.ressources[Ressources::RI_eau] -= infos.eauConsommationJ;
		infos.currentEauConsommationM += infos.eauConsommationJ;				// Se souvient de l'eau consommée ce mois-ci, pour le calcul de la facture d'eau

		// Calcule la consommation d'énergie ce jour-ci, pour pouvoir l'ajouter au total de ce mois, et ainsi pouvoir calculer la facture d'électricité de ce bâtiment en fin de mois
		if (infos.energie < 0.0f)
			infos.currentEnergieConsommationM -= infos.energie * worldInfos.pourcentageEnergieDisponible;	// -= : car infos.energie < 0.0f

		// Retire/Ajoute les ressources produites/consommées par ce bâtiment (généralement une usine dans ce cas)
		const u32 ressourcesSize = infos.ressourcesJ.size();
		for (i = 0; i < ressourcesSize; ++i)
		{
			const Ressources::RessourceID ressID = infos.ressourcesJ[i].ID;
			worldInfos.ressources[ressID] += infos.ressourcesJ[i].production;
		}
	}



	// Met à jour les évolutions des informations du monde :
	float budgetEvolutionM = -infos.entretienM;
	if (staticInfos.isHabitation)	// Si c'est une habitation, ajoute le loyer et les factures d'électricité et d'eau
		budgetEvolutionM += infos.loyerM + infos.currentEnergieConsommationM * FACTURE_ELECTRICITE + infos.currentEauConsommationM * FACTURE_EAU;
	worldInfos.budgetEvolutionM += budgetEvolutionM;
	worldInfos.budgetEvolutionA += budgetEvolutionM * 12.0f + staticInfos.impotsA;
	worldInfos.effetSerreEvolutionM += infos.effetSerreJ * 30.0f;
	worldInfos.dechetsEvolutionM += infos.dechetsJ * 30.0f;
}
