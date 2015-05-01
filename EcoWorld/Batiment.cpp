#include "Batiment.h"
#include "EcoWorldSystem.h"
#include "EcoWorldModifiers.h"

Batiment::Batiment(BatimentID batimentID, EcoWorldSystem& m_system, EcoWorldInfos& m_worldInfos, EcoWorldModifiers& m_modifiers,
	const core::vector2di m_index, float m_rotation)
 : system(m_system), ID(batimentID), infos(batimentID), worldInfos(m_worldInfos), modifiers(m_modifiers), staticInfos(StaticBatimentInfos::getInfos(batimentID)),
 index(m_index), rotation(m_rotation), daysPassed(0), destroyingDay((u32)(-1)), sceneNode(NULL)
{
	// Calcule la Bounding Box de ce b�timent
	box = obbox2df(core::rectf((float)index.X - staticInfos.halfTaille.Width, (float)index.Y - staticInfos.halfTaille.Height,
		(float)index.X + staticInfos.halfTaille.Width, (float)index.Y + staticInfos.halfTaille.Height),
		rotation);
}
void Batiment::onConstruct()
{
	// Ajoute les co�ts de construction au monde
	worldInfos.budget += staticInfos.prixC * modifiers.prixFactor;
	worldInfos.effetSerre += staticInfos.effetSerreC * modifiers.effetSerreFactor;
	worldInfos.dechets += staticInfos.dechetsC * modifiers.dechetsFactor;

	const u32 ressourcesCSize = staticInfos.ressourcesC.size();
	for (u32 i = 0; i < ressourcesCSize; ++i)
		worldInfos.ressources[staticInfos.ressourcesC[i].ID] += staticInfos.ressourcesC[i].production;


	// Si ce b�timent est une maison n'ayant pas de temps de construction, on lui assigne directement comme nombre d'habitants le nombre d'habitants � sa construction.
	// Le cas o� l'habitation a un temps de construction non nul est g�r� dans Batiment::update.
	if (staticInfos.isHabitation && staticInfos.tempsC == 0)
		infos.habitants = staticInfos.habitantsC;
}
void Batiment::onDestroy()
{
	// Ajoute les co�ts de destruction au monde
	worldInfos.budget += staticInfos.prixD;
	worldInfos.effetSerre += staticInfos.effetSerreD;
	worldInfos.dechets += staticInfos.dechetsD;

	const u32 ressourcesDSize = staticInfos.ressourcesD.size();
	for (u32 i = 0; i < ressourcesDSize; ++i)
	{
		const Ressources::RessourceID currentRessourceID = staticInfos.ressourcesD[i].ID;
		if (staticInfos.isTree && currentRessourceID == Ressources::RI_bois)	// Si ce b�timent est un arbre, et que la ressource � g�rer est le bois � sa destruction :
			worldInfos.ressources[Ressources::RI_bois] += staticInfos.ressourcesD[i].production * min((float)daysPassed / 360.0f, 1.0f);	// Un arbre fournit du bois � sa destruction proportionnellement � son �ge (jusqu'� un an) : plus il est grand, plus il fournit de bois
		else
			worldInfos.ressources[currentRessourceID] += staticInfos.ressourcesD[i].production;
	}
}
void Batiment::save(io::IAttributes* out) const
{
	if (!out)
		return;

	// Nouvelle m�thode d'enregistrement des b�timents : on n'enregistre plus leur ID mais leur nom, pour �viter une compl�te invalidation des sauvegardes en cas de changement des ID des b�timents
	//out->addInt("ID", (int)ID);

	// Enregistre le nom de ce b�timent, donn� par ses informations statiques
	out->addString("Name", staticInfos.name);

	out->addPosition2d("Index", index);
	out->addFloat("Rotation", rotation);

	// Enregistre les informations sur l'�ge et les dates de ce b�timent
	out->addInt("DaysPassed", daysPassed);
	out->addInt("DestroyingDay", destroyingDay);

	// Enregistre les informations variables sur ce b�timent
	out->addInt("DureeVie", infos.dureeVie);
	out->addFloat("CurrentEnergieConsommationM", infos.currentEnergieConsommationM);
	out->addFloat("CurrentEauConsommationM", infos.currentEauConsommationM);
	out->addFloat("PourcentageProduction", infos.pourcentageProduction);
	out->addInt("Habitants", (int)infos.habitants);
	out->addFloat("NextHabitantPercent", infos.nextHabitantPercent);

	// Les infos du b�timent seront enregistr�es par le syst�me
}
void Batiment::load(io::IAttributes* in)
{
	if (!in)
		return;

	// Les infos du b�timent ont d�j� �t� lues par le syst�me, il ne reste plus qu'� les r�cup�rer
	// L'index, la rotation et l'ID de ce b�timent ont d�j� �t� lus et assign�s par le syst�me

	// Charge les informations sur l'�ge et les dates de ce b�timent
	if (in->existsAttribute("DaysPassed"))					daysPassed = in->getAttributeAsInt("DaysPassed");
	if (in->existsAttribute("DestroyingDay"))				destroyingDay = in->getAttributeAsInt("DestroyingDay");

	// Charge les informations variables sur ce b�timent
	if (in->existsAttribute("DureeVie"))					infos.dureeVie = in->getAttributeAsInt("DureeVie");
	if (in->existsAttribute("CurrentEnergieConsommationM"))	infos.currentEnergieConsommationM = in->getAttributeAsFloat("CurrentEnergieConsommationM");
	if (in->existsAttribute("CurrentEauConsommationM"))		infos.currentEauConsommationM = in->getAttributeAsFloat("CurrentEauConsommationM");
	if (in->existsAttribute("PourcentageProduction"))		infos.pourcentageProduction = in->getAttributeAsFloat("PourcentageProduction");
	if (in->existsAttribute("Habitants"))					infos.habitants = (u32)abs(in->getAttributeAsInt("Habitants"));
	if (in->existsAttribute("NextHabitantPercent"))			infos.nextHabitantPercent = in->getAttributeAsFloat("NextHabitantPercent");
}
void Batiment::updateDaysPassed(bool* outNormalBatimentDestroy, bool* outOldBatimentDestroy)
{
	// Note : On ne r�initialise pas les valeurs de outNormalBatimentDestroy et de outOldBatimentDestroy au cas o� un b�timent aurait d�j� �t� d�truit lors d'une mise � jour pr�c�dente d'un b�timent

	// Met � jour l'�ge du b�timent (on lui ajoute 1 jour de plus)
	++daysPassed;

	// V�rifie que le b�timent n'est pas en train d'�tre d�truit
	if (isDestroying())
	{
		// V�rifie que le b�timent n'est pas d�truit
		if (system.getTime().getTotalJours() >= (staticInfos.tempsD + destroyingDay) || staticInfos.tempsD == 0)
		{
			// Le b�timent est effectivement d�truit : on indique au syst�me qu'il doit �tre supprim�
			system.addBatimentToDeleteList(this);

			// Indique qu'un b�timent a �t� d�truit volontairement
			if (outNormalBatimentDestroy)
				(*outNormalBatimentDestroy) = true;
		}
	}

	// V�rifie que la dur�e de vie du b�timent n'est pas termin�e
	else if (infos.dureeVie != 0 && daysPassed >= infos.dureeVie)
	{
		// Le b�timent doit effectivement �tre d�truit : on indique au syst�me qu'il doit �tre supprim�
		system.addBatimentToDeleteList(this);

		// Indique qu'un b�timent a �t� d�truit involontairement : � cause de sa dur�e de vie
		if (outOldBatimentDestroy)
			(*outOldBatimentDestroy) = true;
	}
}
void Batiment::updateBatimentInfos(bool firstCall, const float pourcentageRessourcesDisponibles[Ressources::RI_COUNT])
{
	// V�rifie que le b�timent n'est pas en train d'�tre construit ou d�truit, sinon on quitte
	if (isConstructingOrDestroying())
		return;

	// Retiens certaines valeurs sp�cifiques � ce b�timent pour d�terminer si une mise � jour des informations sur la population et l'�nergie du monde sera n�cessaire � la fin de cette fonction
	const float lastEnergie = infos.energie;
	const u32 lastHabitants = infos.habitants;



	// Met � jour les informations sp�cifiques de ce b�timent pour ce jour :

	// Calcule l'�nergie consomm� par ce b�timent d'apr�s son pourcentage de production demand� par l'utilisateur
	// Note : L'�nergie consomm�e par les habitations doit �tre constante, ind�pendamment du nombre d'habitants des habitations !
	// En effet, le nombre d'habitants des maisons est d�j� lui-m�me d�pendant de l'�nergie du monde disponible
	// (par la vitesse d'emm�nagement des habitants, d�pendante de la satisfaction r�elle, elle-m�me d�pendante de l'�nergie disponible),
	// donc si ce cas venait � se produire ces deux valeurs seraient mutuellement d�pendantes et provoqueraient des erreurs dans les mises � jour du monde !
	if (ID != BI_centrale_charbon)	// Exception : Ne change pas l'�nergie de la centrale � charbon, ce qui provoquerait des erreurs dans son calcul de l'�nergie du monde n�cessaire lors de l'appel � la m�thode updateBatimentInfosFromBatimentID
		infos.energie = staticInfos.energie * infos.pourcentageProduction;

	// Calcule l'eau consomm�e actuellement par ce b�timent, limit�e par le pourcentage d'eau disponible
	infos.eauConsommationJ = staticInfos.eauConsommationJ * pourcentageRessourcesDisponibles[Ressources::RI_eau];

	// V�rifie si ce b�timent est une habitation
	if (staticInfos.isHabitation)
	{
		// Si c'est la premi�re mise � jour de cette maison, on lui assigne comme nombre d'habitants le nombre d'habitants � sa construction.
		// Ici, on ne g�re que le cas o� l'habitation a un temps de construction non nul.
		if (daysPassed == staticInfos.tempsC && staticInfos.tempsC != 0)
			infos.habitants = staticInfos.habitantsC;

		// Si c'est le premier appel de cette fonction ce jour, on actualise le nombre d'habitants dans cette maison
		if (firstCall)
		{
			// Calcule la vitesse d'emm�nagement/de d�m�nagement des habitants dans cette maison :
			// A 0% de satisfaction :	La vitesse est -staticInfos.habitantsJ (d�m�nagement le plus rapide).
			// A 20% de satisfaction :	La vitesse est 0.0f (aucune modification).
			// A 40% de satisfaction :	La vitesse est staticInfos.habitantsJ (emm�nagement le plus rapide).
			const float minSatisfaction = HABITANTS_MIN_SATISFACTION * modifiers.minSatisfactionFactor;
			const float emmenagementSpeed = core::clamp((worldInfos.popRealSatisfaction - minSatisfaction) / minSatisfaction, -1.0f, 1.0f) * staticInfos.habitantsJ;

			// Calcule le pourcentage de progression du prochain habitant et augmente ou diminue le nombre d'habitants si n�cessaire
			infos.nextHabitantPercent += emmenagementSpeed;
			while (infos.nextHabitantPercent >= 1.0f)	// Le while permet de g�rer le cas o� plusieurs habitants ont emm�nag�s en m�me temps
			{
				if (infos.habitants < staticInfos.habitantsMax)
					++infos.habitants;
				infos.nextHabitantPercent -= 1.0f;
			}
			while (infos.nextHabitantPercent <= 0.0f)	// Le while permet de g�rer le cas o� plusieurs habitants ont d�m�nag�s en m�me temps
			{
				if (infos.habitants > 1)
					--infos.habitants;
				infos.nextHabitantPercent += 1.0f;
			}

			// Si on est au minimum ou au maximum d'habitants dans cette maison
			if (infos.habitants <= 1 || infos.habitants >= staticInfos.habitantsMax)
			{
				// Force le nombre d'habitants dans cette maison � �tre compris entre 1 habitant et le maximum d'habitants dans cette maison
				infos.habitants = core::clamp(infos.habitants, (u32)1, staticInfos.habitantsMax);

				// R�initialise le pourcentage de progression du prochain habitant
				infos.nextHabitantPercent = 0.0f;
			}
		}

		// Rend l'eau consomm�e actuellement par cette maison d�pendante de son nombre d'habitants actuel
		const float habitantsFactor = (float)(infos.habitants) / (float)(staticInfos.habitantsMax);
		infos.eauConsommationJ *= habitantsFactor;

		// Calcule le loyer r�el de cette habitation
		// Le loyer des habitations est d�pendant de la satisfaction r�elle de la population et du nombre d'habitants actuel dans la maison
		infos.loyerM = staticInfos.loyerM * worldInfos.popRealSatisfaction * habitantsFactor;

		// Calcule les imp�ts annuels de cette habitation, d�pendants du nombre d'habitants actuel dans la maison
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

	// V�rifie si ce b�timent est un arbre
	if (staticInfos.isTree && daysPassed < 360)
	{
		// Les arbres absorbent une quantit� d'effet de serre relative � leur anciennet� (quadratiquement entre 0 et 1 an) :
		// facteur = f(j) = (j / 400)� + 0.19
		float facteur = daysPassed * 0.0025f;	// 1 / 400 = 0.0025
		facteur *= facteur;
		facteur += 0.19f;
		infos.effetSerreJ = staticInfos.effetSerreJ * facteur;
	}
	else
		infos.effetSerreJ = staticInfos.effetSerreJ;

	// Indique les d�chets produits/absorb�s par jour d'apr�s les informations statiques sur ce b�timent
	infos.dechetsJ = staticInfos.dechetsJ;

	// Calcule l'entretien de ce b�timent d'apr�s son pourcentage de production demand� par l'utilisateur
	infos.entretienM = staticInfos.entretienM * (infos.pourcentageProduction * 0.5f + 0.5f);

	// Calcule le pourcentage de production actuel de ce b�timent :
	// (limit� par la ressource disponible la plus basse, ainsi que par le pourcentage de production maximal demand� par l'utilisateur)
	infos.pourcentageProductionActuel = infos.pourcentageProduction;
	const u32 ressourcesSize = infos.ressourcesJ.size();
	if (ressourcesSize)
	{
#ifdef KID_VERSION
		// V�rifie si ce b�timent est un arbre
		if (staticInfos.isTree)
		{
			// Ce b�timent est un arbre :

			// En mode enfant, un arbre produit du bois chaque jour proportionnellement � son �ge (jusqu'� un an) : plus il est grand, plus il fournit de bois.
			// A un an (et plus) (� sa taille maximale) : 5 kg/arbre/J.
			// En effet, il est pour le moment impossible de produire du bois autrement que par la destruction manuelle des arbres !

			// Met � jour la production/consommation r�elle de chaque ressource produite par cet arbre (juste du bois normalement, mais on parcours toutes les ressources au cas o� d'autres ressources auraient �t� ajout�es)
			for (u32 i = 0; i < ressourcesSize; ++i)
				infos.ressourcesJ[i].production = staticInfos.ressourcesJ[i].production * min((float)daysPassed / 360.0f, 1.0f);
		}
		else
#endif
		{
			// Ce b�timent est s�rement une usine :

			// D�termine le pourcentage de production actuel suivant la disponibilit� des ressources consomm�es par ce b�timent
			for (u32 i = 0; i < ressourcesSize; ++i)
				if (staticInfos.ressourcesJ[i].production < 0.0f)	// Une ressource consomm�e et non disponible compl�tement limite le pourcentage de production actuel
					infos.pourcentageProductionActuel = min(pourcentageRessourcesDisponibles[infos.ressourcesJ[i].ID], infos.pourcentageProductionActuel);

			// Le pourcentage de production actuel est aussi directement limit� par le pourcentage d'�nergie disponible du monde (uniquement si ce b�timent consomme de l'�nergie)
			if (staticInfos.energie < 0.0f)
				infos.pourcentageProductionActuel *= worldInfos.pourcentageEnergieDisponible;

			// Met � jour la production/consommation r�elle de chaque ressource pour ce jour
			for (u32 i = 0; i < ressourcesSize; ++i)
				infos.ressourcesJ[i].production = staticInfos.ressourcesJ[i].production * infos.pourcentageProductionActuel;
		}
	}


	// Met � jour les informations sp�cifiques de ce b�timent suivant son ID
	updateBatimentInfosFromBatimentID(pourcentageRessourcesDisponibles);


	// Si certaines informations de ce b�timent ayant un impact sur les donn�es du monde (l'�nergie produite/consomm�e par ce b�timent et son nombre d'habitants) ont �t� modifi�es :
	if (infos.energie != lastEnergie || infos.habitants != lastHabitants)
	{
		// On met � jour les informations du monde
		worldInfos.updateDonnees();

#ifdef _DEBUG
		// V�rification de d�bogage :
		// V�rifie que l'�nergie consomm�e par les habitations reste constante (voir ci-dessus pour plus d'informations, lors du calcul de l'�nergie consomm� par ce b�timent)
		if (staticInfos.isHabitation && infos.energie != lastEnergie
			&& daysPassed != staticInfos.tempsC)	// V�rifie aussi que cette habitation ne vient pas d'�tre construite : dans ce cas il est normal que son �nergie soit initialis�e � sa bonne valeur
			LOG_DEBUG("Batiment::updateBatimentInfos(...) : Ce batiment est une habitation et son �nergie consomm�e n'est pas constante : ID = " << ID << " ; energie = " << infos.energie << " ; lastEnergie = " << lastEnergie, ELL_ERROR);
#endif
	}
}
void Batiment::updateBatimentInfosFromBatimentID(const float pourcentageRessourcesDisponibles[Ressources::RI_COUNT])
{
	// D�termine quel est l'ID actuel du b�timent
	switch (ID)
	{
	case BI_panneau_solaire:
		// La production d'�nergie des panneaux solaires est relative au facteur d'�nergie lumineuse du temps actuel
		infos.energie = staticInfos.energie * system.getWeatherManager().getCurrentWeatherInfos().energyFactor;
		break;

	case BI_centrale_charbon:
		// Calcule l'�nergie produite par la centrale � charbon
		infos.energie = core::min_(min(
			max(infos.energie - worldInfos.energie, 0.0f),					// La centrale � charbon ne doit produire que l'�lectricit� n�cessaire pour ne pas g�cher de bois (on n'oublie pas de soustraire la production actuelle d'�nergie de la centrale � charbon pour conna�tre l'�nergie totale qu'elle doit produire)
			CENTRALE_CHARBON_ENERGIE_MAX * infos.pourcentageProduction),	// La centrale � charbon ne produit pas plus d'�nergie qu'elle ne peut en produire au maximum (aussi limit� par son pourcentage de production d�sir�)
			worldInfos.ressources[Ressources::RI_bois] * pourcentageRessourcesDisponibles[Ressources::RI_bois] * CENTRALE_CHARBON_RENTABILITE,	// La centrale � charbon ne produit pas plus d'�nergie qu'il ne reste de bois
			worldInfos.ressources[Ressources::RI_eau] * pourcentageRessourcesDisponibles[Ressources::RI_eau] * CENTRALE_CHARBON_RENTABILITE / CENTRALE_CHARBON_CONSOMMATION_EAU_FACTEUR);	// La centrale � charbon ne produit pas plus d'�nergie qu'il ne reste d'eau pour son refroidissement

		// Caclule la consommation de bois de la centrale : le premier �lement de ce tableau de ressources repr�sente le bois
		infos.ressourcesJ[0].production = infos.energie * CENTRALE_CHARBON_RENTABILITE_OPP_INV;

		// Calcule la consommation d'eau de la centrale (proportionnel � la consommation de bois) : le deuxi�me �lement de ce tableau de ressources repr�sente l'eau
		infos.ressourcesJ[1].production = infos.ressourcesJ[0].production * CENTRALE_CHARBON_CONSOMMATION_EAU_FACTEUR;

		// Calcule le pourcentage de production actuel de la centrale par rapport � sa production maximale
		infos.pourcentageProductionActuel = infos.energie / CENTRALE_CHARBON_ENERGIE_MAX;

		// Son entretien est relatif � son pourcentage de production d�sir� � 50 %
		infos.entretienM = staticInfos.entretienM * (infos.pourcentageProduction * 0.5f + 0.5f);
		break;

	case BI_usine_incineration_dechets:
		// L'usine d'incin�ration des d�chets produits proportionnellement de l'effet de serre par rapport aux d�chets br�l�s
		// et essaie toujours de br�ler les d�chets le plus rapidement possible.
		// Les d�chets maximaux qu'elle peut br�ler sont aussi r�gl�s par le pourcentage de production de l'usine demand� par l'utilisateur.
		const float dechetsMax = USINE_INCINERATION_DECHETS_MAX * infos.pourcentageProduction * worldInfos.pourcentageEnergieDisponible;
		infos.dechetsJ = -min(worldInfos.dechets, dechetsMax);
		infos.effetSerreJ = infos.dechetsJ * -USINE_INCINERATION_RENTABILITE;

		// Calcule le pourcentage de production actuel de l'usine par rapport aux d�chets maximaux qu'elle peut �liminer
		infos.pourcentageProductionActuel = infos.dechetsJ * USINE_INCINERATION_DECHETS_MAX_OPP_INV;

		// Sa consommation d'�nergie est relative � son pourcentage de production demand� � 50 %
		infos.energie = staticInfos.energie * (infos.pourcentageProduction * 0.5f + 0.5f);

		// Son entretien est relatif � son pourcentage de production d�sir� � 50 %
		infos.entretienM = staticInfos.entretienM * (infos.pourcentageProduction * 0.5f + 0.5f);
		break;
	}
}
void Batiment::update(bool moisEcoule, bool anneeEcoulee)
{
	// V�rifie que le b�timent n'est pas en train d'�tre construit ou d�truit, sinon on quitte
	if (isConstructingOrDestroying())
		return;

	u32 i = 0;	// Variable d'it�ration



	// Met � jour les informations du syst�me :

	// Par an
	// Ajoute les imp�ts si c'est une habitation, qui d�pendent du nombre d'habitants actuel dans la maison
	if (anneeEcoulee && staticInfos.isHabitation)
		worldInfos.budget += infos.impotsA;

	// Par mois
	if (moisEcoule)
	{
		worldInfos.budget -= infos.entretienM;

		// Ajoute le loyer et les factures d'�lectricit� et d'eau (aux habitations seulement !)
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

	// Par jour (effectu� obligatoirement apr�s "Par mois" pour que les variables infos.currentEnergieConsommationM et infos.currentEauConsommationM qui viennent d'�tre r�initialis�es � 0.0f soient recalcul�es)
	{
		// Ajoute l'effet de serre et les d�chets au monde
		worldInfos.effetSerre += infos.effetSerreJ;
		worldInfos.dechets += infos.dechetsJ;

#ifdef _DEBUG
		// V�rifie que la consommation d'eau de ce b�timent n'est jamais n�gative
		if (infos.eauConsommationJ < 0.0f)
			LOG_DEBUG("Batiment::update(...) : La consommation d'eau de ce batiment est negative : infos.eauConsommationJ = " << infos.eauConsommationJ << " ; ID = " << ID <<" !", ELL_WARNING);
#endif

		// Retire l'eau consomm�e du monde
		worldInfos.ressources[Ressources::RI_eau] -= infos.eauConsommationJ;
		infos.currentEauConsommationM += infos.eauConsommationJ;				// Se souvient de l'eau consomm�e ce mois-ci, pour le calcul de la facture d'eau

		// Calcule la consommation d'�nergie ce jour-ci, pour pouvoir l'ajouter au total de ce mois, et ainsi pouvoir calculer la facture d'�lectricit� de ce b�timent en fin de mois
		if (infos.energie < 0.0f)
			infos.currentEnergieConsommationM -= infos.energie * worldInfos.pourcentageEnergieDisponible;	// -= : car infos.energie < 0.0f

		// Retire/Ajoute les ressources produites/consomm�es par ce b�timent (g�n�ralement une usine dans ce cas)
		const u32 ressourcesSize = infos.ressourcesJ.size();
		for (i = 0; i < ressourcesSize; ++i)
		{
			const Ressources::RessourceID ressID = infos.ressourcesJ[i].ID;
			worldInfos.ressources[ressID] += infos.ressourcesJ[i].production;
		}
	}



	// Met � jour les �volutions des informations du monde :
	float budgetEvolutionM = -infos.entretienM;
	if (staticInfos.isHabitation)	// Si c'est une habitation, ajoute le loyer et les factures d'�lectricit� et d'eau
		budgetEvolutionM += infos.loyerM + infos.currentEnergieConsommationM * FACTURE_ELECTRICITE + infos.currentEauConsommationM * FACTURE_EAU;
	worldInfos.budgetEvolutionM += budgetEvolutionM;
	worldInfos.budgetEvolutionA += budgetEvolutionM * 12.0f + staticInfos.impotsA;
	worldInfos.effetSerreEvolutionM += infos.effetSerreJ * 30.0f;
	worldInfos.dechetsEvolutionM += infos.dechetsJ * 30.0f;
}
