#include "Objectives.h"
#include "EcoWorldSystem.h"
#include "Batiment.h"
#include "Game.h"

Objectives::Objectives(EcoWorldSystem* m_system) : system(m_system)
{
	// Réinitialise tous les objectifs
	reset();
}
void Objectives::reset()
{
	// Efface la liste des objectifs pour cette partie
	objectives.clear();

	// Indique que la partie n'est ni gagnée, ni perdue
	gameWon = false;
	gameLost = false;

	// Indique que le joueur n'a pas demandé à continuer la partie si elle est gagnée
	continueGame = false;
}
void Objectives::update()
{
	// Vérifie tous les objectifs :

	gameWon = true;
	gameLost = false;

	bool winObjectiveTested = false;	// Permet de vérifier qu'au moins un objectif gagnant a été testé : si aucun objectif gagnant n'a été testé, le jeu n'est pas gagné

	const core::list<ObjectiveData>::Iterator END = objectives.end();
	for (core::list<ObjectiveData>::Iterator it = objectives.begin(); it != END; ++it)
	{
		// Obtient l'objectif actuel
		ObjectiveData& objective = (*it);

		// Détermine si cet objectif est vérifié
		objective.isFulfilled = verifyObjective(objective);

		if (objective.winObjective)	// Objectif gagnant
		{
			// Indique qu'au moins un objectif gagnant a été testé
			winObjectiveTested = true;

			// Si cet objectif n'est pas vérifié, cette partie ne peut pas être gagnée
			gameWon &= objective.isFulfilled;
		}
		else						// Objectif éliminatoire
		{
			// Si cet objectif n'est pas vérifié, cette partie est perdue
			gameLost |= !(objective.isFulfilled);
		}
	}

	// Si aucun objectif gagnant n'a été testé, le jeu n'est pas gagné
	gameWon &= winObjectiveTested;

	// Réintialise la volonté du joueur de continuer la partie si elle n'est plus gagnée dorénavant
	// Désactivé : Peu cohérent, et peut gêner le joueur
	//continueGame &= gameWon;
}
void Objectives::addObjective(E_OBJECTIVE_TYPE type, bool winObjective, E_COMPARISON_OPERATOR comparisonOp, float dataF, BatimentID dataID)
{
	// Ajoute un objectif à la liste des objectifs
	objectives.push_back(ObjectiveData(type, winObjective, comparisonOp, dataF, dataID));
}
void Objectives::addObjective(const ObjectiveData& objective)
{
	// Ajoute un objectif à la liste des objectifs
	objectives.push_back(objective);
}
void Objectives::addObjectives(const core::list<ObjectiveData> objectivesList)
{
	// Ajoute cette liste d'objectifs à cette partie
	const core::list<ObjectiveData>::ConstIterator END = objectivesList.end();
	for (core::list<ObjectiveData>::ConstIterator it = objectivesList.begin(); it != END; ++it)
		objectives.push_back(*it);
}
bool Objectives::compareValues(float data1, float data2, E_COMPARISON_OPERATOR comparisonOp)
{
	switch (comparisonOp)
	{
	case ECO_EQUALS:				// ==
		return (data1 == data2);
	case ECO_DIFFERENT:				// !=
		return (data1 != data2);
	case ECO_LESS_EQUALS:			// <=
		return (data1 <= data2);
	case ECO_LESS:					// <
		return (data1 < data2);
	case ECO_MORE_EQUALS:			// >=
		return (data1 >= data2);
	case ECO_MORE:					// >
		return (data1 > data2);
	}

	// Type d'opérateur inconnu : on retourne toujours true, ce qui a pour effet d'annuler l'objectif contenant cet opérateur
	LOG_DEBUG("Objectives::compareValues(" << data1 << ", " << data2 << ", " << comparisonOp << ") : Type d'operateur de comparaison inconnu : comparisonOp = " << comparisonOp, ELL_WARNING);

	return true;
}
bool Objectives::verifyObjective(const ObjectiveData& objective) const
{
	if (!system)
		return true;

	switch (objective.type)
	{
	case EOT_BUDGET:
		return compareValues(system->getInfos().budget, objective.dataF, objective.comparisonOp);
	case EOT_EFFET_SERRE:
		return compareValues(system->getInfos().effetSerre, objective.dataF, objective.comparisonOp);
	case EOT_DECHETS:
		return compareValues(system->getInfos().dechets, objective.dataF, objective.comparisonOp);
	case EOT_TIME_PASSED:
		return compareValues(system->getTime().getTotalTime(), fabs(objective.dataF), objective.comparisonOp);

	case EOT_BATIMENT_NB:
		{
			// Compte le nombre de bâtiments du système ayant ce type d'ID :
			// TODO : Optimiser cette recherche en n'utilisant non pas la liste de tous les bâtiments du jeu, mais les listes des bâtiments pour les mises à jour du système (ex : listeBatsMaisons au lieu de listeAllBats)

			float nbBats = 0.0f;

			const core::list<Batiment*>& listeAllBats = system->getAllBatiments();
			const core::list<Batiment*>::ConstIterator END = listeAllBats.end();
			for (core::list<Batiment*>::ConstIterator it = listeAllBats.begin(); it != END; ++it)
			{
				const Batiment* const batiment = (*it);
				if (batiment->getID() == objective.dataID)			// Vérifie que ce bâtiment est bien du type demandé
					if (!batiment->isConstructingOrDestroying())	// Vérifie que ce bâtiment n'est pas en train d'être construit ou détruit
						++nbBats;
			}

			return compareValues(nbBats, fabs(objective.dataF), objective.comparisonOp);
		}
	}

	// Type d'objectif inconnu : on retourne toujours true, ce qui a pour effet d'annuler cet objectif
	LOG_DEBUG("Objectives::verifyObjective(objective) : Type d'objectif inconnu : objective.type = " << objective.type, ELL_WARNING);

	return true;
}
void Objectives::save(io::IAttributes* out, io::IXMLWriter* writer) const
{
	if (!out || !writer)
		return;

	// Ajoute si le joueur veut continuer la partie s'il l'a gagnée
	out->addBool("ContinueGame", continueGame);

	// Ajoute le nombre total d'objectifs
	out->addInt("ObjectivesCount", (int)(objectives.size()));

	// Ajoute chaque objectif indépendamment
	int objectiveCount = 0;
	core::stringc elementName;
	const core::list<ObjectiveData>::ConstIterator END = objectives.end();
	for (core::list<ObjectiveData>::ConstIterator it = objectives.begin(); it != END; ++it)
	{
		// Obtient le préfixe des élements de cet objectif
		sprintf_SS("Objective%d_", objectiveCount);

		// Ajoute les infos de cet objectif s'ils sont différents de leur valeur par défaut :

		elementName = text_SS;
		elementName.append("Type");
		out->addInt(elementName.c_str(), (int)((*it).type));

		elementName = text_SS;
		elementName.append("WinObjective");
		out->addBool(elementName.c_str(), (*it).winObjective);

		elementName = text_SS;
		elementName.append("ComparisonOp");
		out->addInt(elementName.c_str(), (int)((*it).comparisonOp));

		const float dataF = (*it).dataF;
		if (dataF != 0.0f)
		{
			elementName = text_SS;
			elementName.append("DataF");
			out->addFloat(elementName.c_str(), (*it).dataF);
		}

		const BatimentID dataID = (*it).dataID;
		if (dataID != BI_aucun)
		{
			elementName = text_SS;
			elementName.append("DataID");
			out->addInt(elementName.c_str(), (*it).dataID);
		}

		// Incrémente le compteur des objectifs
		++objectiveCount;
	}

	// Enregistre les infos des objectifs
	out->write(writer, false, L"Objectives");
	out->clear();
}
void Objectives::load(io::IAttributes* in, io::IXMLReader* reader)
{
	if (!in || !reader)
		return;

	// Réinitialise les objectifs avant le chargement
	reset();

	// Lit les informations sur les objectifs depuis le fichier
	core::stringc elementName;
	reader->resetPosition();
	if (in->read(reader, false, L"Objectives"))
	{
		// Lit si le joueur veut continuer la partie s'il l'a gagnée
		if (in->existsAttribute("ContinueGame"))					continueGame = in->getAttributeAsBool("ContinueGame");

		if (in->existsAttribute("ObjectivesCount"))
		{
			// Lit le nombre total d'objectifs
			const int objectivesCount = in->getAttributeAsInt("ObjectivesCount");

			// Lit les informations de chaque objectif indépendamment :
			for (int i = 0; i < objectivesCount; ++i)
			{
				// Obtient le préfixe des élements de cet objectif
				sprintf_SS("Objective%d_", i);

				// Vérifie que le type de cet objectif est disponible (élément nécessaire à sa création)
				elementName = text_SS;
				elementName.append("Type");
				if (in->existsAttribute(elementName.c_str()))
				{
					ObjectiveData objective;
					objective.type = (E_OBJECTIVE_TYPE)(in->getAttributeAsInt(elementName.c_str()));

					elementName = text_SS;
					elementName.append("WinObjective");
					if (in->existsAttribute(elementName.c_str()))	objective.winObjective = in->getAttributeAsBool(elementName.c_str());

					elementName = text_SS;
					elementName.append("ComparisonOp");
					if (in->existsAttribute(elementName.c_str()))	objective.comparisonOp = (E_COMPARISON_OPERATOR)(in->getAttributeAsInt(elementName.c_str()));

					elementName = text_SS;
					elementName.append("DataF");
					if (in->existsAttribute(elementName.c_str()))	objective.dataF = in->getAttributeAsFloat(elementName.c_str());

					elementName = text_SS;
					elementName.append("DataID");
					if (in->existsAttribute(elementName.c_str()))	objective.dataID = (BatimentID)(in->getAttributeAsInt(elementName.c_str()));

					// Ajoute cet objectif à la liste des objectifs
					objectives.push_back(objective);
				}
			}
		}

		in->clear();
	}
}
core::stringw Objectives::getComparisonOperatorStr(E_COMPARISON_OPERATOR comparisonOp, bool masculin, bool singulier)
{
	switch (comparisonOp)
	{
	case ECO_EQUALS:				// ==
		if (masculin)
		{
			if (singulier)
				return core::stringw(L"égal");
			else
				return core::stringw(L"égaux");
		}
		else
		{
			if (singulier)
				return core::stringw(L"égale");
			else
				return core::stringw(L"égales");
		}
	case ECO_DIFFERENT:				// !=
		if (masculin)
		{
			if (singulier)
				return core::stringw(L"différent");
			else
				return core::stringw(L"différents");
		}
		else
		{
			if (singulier)
				return core::stringw(L"différente");
			else
				return core::stringw(L"différentes");
		}
	case ECO_LESS_EQUALS:			// <=
		if (masculin)
		{
			if (singulier)
				return core::stringw(L"inférieur ou égal");
			else
				return core::stringw(L"inférieurs ou égaux");
		}
		else
		{
			if (singulier)
				return core::stringw(L"inférieure ou égale");
			else
				return core::stringw(L"inférieures ou égales");
		}
	case ECO_LESS:					// <
		if (masculin)
		{
			if (singulier)
				return core::stringw(L"inférieur");
			else
				return core::stringw(L"inférieurs");
		}
		else
		{
			if (singulier)
				return core::stringw(L"inférieure");
			else
				return core::stringw(L"inférieures");
		}
	case ECO_MORE_EQUALS:			// >=
		if (masculin)
		{
			if (singulier)
				return core::stringw(L"supérieur ou égal");
			else
				return core::stringw(L"supérieurs ou égaux");
		}
		else
		{
			if (singulier)
				return core::stringw(L"supérieure ou égale");
			else
				return core::stringw(L"supérieures ou égales");
		}
	case ECO_MORE:					// >
		if (masculin)
		{
			if (singulier)
				return core::stringw(L"supérieur");
			else
				return core::stringw(L"supérieurs");
		}
		else
		{
			if (singulier)
				return core::stringw(L"supérieure");
			else
				return core::stringw(L"supérieures");
		}
	}

	LOG_DEBUG("Objectives::getComparisonOperatorStr(" << comparisonOp << ") : Type d'operateur de comparaison inconnu : comparisonOp = " << comparisonOp, ELL_WARNING);

	return core::stringw();
}
core::stringw ObjectiveData::getObjectiveDescription() const
{
	core::stringw desc;

	switch (type)
	{
	case Objectives::EOT_BUDGET:
		if (winObjective)
			desc = L"Le budget doit devenir ";
		else
			desc = L"Le budget doit rester ";
		desc.append(Objectives::getComparisonOperatorStr(comparisonOp, true, true));
		if (comparisonOp == Objectives::ECO_DIFFERENT)
			desc.append(L" de ");
		else
			desc.append(L" à ");
		swprintf_SS(L"%.2f €.", dataF);
		desc.append(textW_SS);
		break;
	case Objectives::EOT_EFFET_SERRE:
		if (winObjective)
			desc = L"L'effet de serre doit devenir ";
		else
			desc = L"L'effet de serre doit rester ";
		desc.append(Objectives::getComparisonOperatorStr(comparisonOp, true, true));
		if (comparisonOp == Objectives::ECO_DIFFERENT)
			desc.append(L" de ");
		else
			desc.append(L" à ");
		swprintf_SS(L"%.2f kg.", dataF);
		desc.append(textW_SS);
		break;
	case Objectives::EOT_DECHETS:
		if (winObjective)
			desc = L"Les déchets doivent devenir ";
		else
			desc = L"Les déchets doivent rester ";
		desc.append(Objectives::getComparisonOperatorStr(comparisonOp, true, false));
		if (comparisonOp == Objectives::ECO_DIFFERENT)
			desc.append(L" de ");
		else
			desc.append(L" à ");
		swprintf_SS(L"%.2f kg.", dataF);
		desc.append(textW_SS);
		break;
	case Objectives::EOT_TIME_PASSED:	// TODO : Améliorer le texte "Le temps écoulé..." pour le rendre plus facilement compréhensible
		if (winObjective)
			desc = L"Le temps écoulé doit devenir ";
		else
			desc = L"Le temps écoulé doit rester ";
		desc.append(Objectives::getComparisonOperatorStr(comparisonOp, true, true));
		if (comparisonOp == Objectives::ECO_DIFFERENT)
			desc.append(L" de ");
		else
			desc.append(L" à ");
		swprintf_SS(L"%.2f s (", abs(dataF));
		desc.append(textW_SS);
		Game::appendDays(desc, (u32)(abs(dataF) * DAY_TIME_INV));
		desc.append(L").");
		break;

	case Objectives::EOT_BATIMENT_NB:	// TODO : Améliorer le texte "Le nombre de bâtiments construits de type "ID" ..." pour le rendre plus facilement compréhensible et moins long
		desc = L"Le nombre de bâtiments construits de type \"";
		desc.append(StaticBatimentInfos::getInfos(dataID).name);
		if (winObjective)
			desc.append(L"\" doit devenir ");
		else
			desc.append(L"\" doit rester ");
		desc.append(Objectives::getComparisonOperatorStr(comparisonOp, true, true));
		if (comparisonOp == Objectives::ECO_DIFFERENT)
			desc.append(L" de ");
		else
			desc.append(L" à ");
		swprintf_SS(L"%u.", (u32)(abs(dataF)));
		desc.append(textW_SS);
		break;

	default:
		LOG_DEBUG("ObjectiveData::getObjectiveDescription() : Type d'objectif inconnu : type = " << type, ELL_WARNING);
		break;
	}

	return desc;
}
