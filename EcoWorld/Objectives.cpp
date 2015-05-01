#include "Objectives.h"
#include "EcoWorldSystem.h"
#include "Batiment.h"
#include "Game.h"

Objectives::Objectives(EcoWorldSystem* m_system) : system(m_system)
{
	// R�initialise tous les objectifs
	reset();
}
void Objectives::reset()
{
	// Efface la liste des objectifs pour cette partie
	objectives.clear();

	// Indique que la partie n'est ni gagn�e, ni perdue
	gameWon = false;
	gameLost = false;

	// Indique que le joueur n'a pas demand� � continuer la partie si elle est gagn�e
	continueGame = false;
}
void Objectives::update()
{
	// V�rifie tous les objectifs :

	gameWon = true;
	gameLost = false;

	bool winObjectiveTested = false;	// Permet de v�rifier qu'au moins un objectif gagnant a �t� test� : si aucun objectif gagnant n'a �t� test�, le jeu n'est pas gagn�

	const core::list<ObjectiveData>::Iterator END = objectives.end();
	for (core::list<ObjectiveData>::Iterator it = objectives.begin(); it != END; ++it)
	{
		// Obtient l'objectif actuel
		ObjectiveData& objective = (*it);

		// D�termine si cet objectif est v�rifi�
		objective.isFulfilled = verifyObjective(objective);

		if (objective.winObjective)	// Objectif gagnant
		{
			// Indique qu'au moins un objectif gagnant a �t� test�
			winObjectiveTested = true;

			// Si cet objectif n'est pas v�rifi�, cette partie ne peut pas �tre gagn�e
			gameWon &= objective.isFulfilled;
		}
		else						// Objectif �liminatoire
		{
			// Si cet objectif n'est pas v�rifi�, cette partie est perdue
			gameLost |= !(objective.isFulfilled);
		}
	}

	// Si aucun objectif gagnant n'a �t� test�, le jeu n'est pas gagn�
	gameWon &= winObjectiveTested;

	// R�intialise la volont� du joueur de continuer la partie si elle n'est plus gagn�e dor�navant
	// D�sactiv� : Peu coh�rent, et peut g�ner le joueur
	//continueGame &= gameWon;
}
void Objectives::addObjective(E_OBJECTIVE_TYPE type, bool winObjective, E_COMPARISON_OPERATOR comparisonOp, float dataF, BatimentID dataID)
{
	// Ajoute un objectif � la liste des objectifs
	objectives.push_back(ObjectiveData(type, winObjective, comparisonOp, dataF, dataID));
}
void Objectives::addObjective(const ObjectiveData& objective)
{
	// Ajoute un objectif � la liste des objectifs
	objectives.push_back(objective);
}
void Objectives::addObjectives(const core::list<ObjectiveData> objectivesList)
{
	// Ajoute cette liste d'objectifs � cette partie
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

	// Type d'op�rateur inconnu : on retourne toujours true, ce qui a pour effet d'annuler l'objectif contenant cet op�rateur
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
			// Compte le nombre de b�timents du syst�me ayant ce type d'ID :
			// TODO : Optimiser cette recherche en n'utilisant non pas la liste de tous les b�timents du jeu, mais les listes des b�timents pour les mises � jour du syst�me (ex : listeBatsMaisons au lieu de listeAllBats)

			float nbBats = 0.0f;

			const core::list<Batiment*>& listeAllBats = system->getAllBatiments();
			const core::list<Batiment*>::ConstIterator END = listeAllBats.end();
			for (core::list<Batiment*>::ConstIterator it = listeAllBats.begin(); it != END; ++it)
			{
				const Batiment* const batiment = (*it);
				if (batiment->getID() == objective.dataID)			// V�rifie que ce b�timent est bien du type demand�
					if (!batiment->isConstructingOrDestroying())	// V�rifie que ce b�timent n'est pas en train d'�tre construit ou d�truit
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

	// Ajoute si le joueur veut continuer la partie s'il l'a gagn�e
	out->addBool("ContinueGame", continueGame);

	// Ajoute le nombre total d'objectifs
	out->addInt("ObjectivesCount", (int)(objectives.size()));

	// Ajoute chaque objectif ind�pendamment
	int objectiveCount = 0;
	core::stringc elementName;
	const core::list<ObjectiveData>::ConstIterator END = objectives.end();
	for (core::list<ObjectiveData>::ConstIterator it = objectives.begin(); it != END; ++it)
	{
		// Obtient le pr�fixe des �lements de cet objectif
		sprintf_SS("Objective%d_", objectiveCount);

		// Ajoute les infos de cet objectif s'ils sont diff�rents de leur valeur par d�faut :

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

		// Incr�mente le compteur des objectifs
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

	// R�initialise les objectifs avant le chargement
	reset();

	// Lit les informations sur les objectifs depuis le fichier
	core::stringc elementName;
	reader->resetPosition();
	if (in->read(reader, false, L"Objectives"))
	{
		// Lit si le joueur veut continuer la partie s'il l'a gagn�e
		if (in->existsAttribute("ContinueGame"))					continueGame = in->getAttributeAsBool("ContinueGame");

		if (in->existsAttribute("ObjectivesCount"))
		{
			// Lit le nombre total d'objectifs
			const int objectivesCount = in->getAttributeAsInt("ObjectivesCount");

			// Lit les informations de chaque objectif ind�pendamment :
			for (int i = 0; i < objectivesCount; ++i)
			{
				// Obtient le pr�fixe des �lements de cet objectif
				sprintf_SS("Objective%d_", i);

				// V�rifie que le type de cet objectif est disponible (�l�ment n�cessaire � sa cr�ation)
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

					// Ajoute cet objectif � la liste des objectifs
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
				return core::stringw(L"�gal");
			else
				return core::stringw(L"�gaux");
		}
		else
		{
			if (singulier)
				return core::stringw(L"�gale");
			else
				return core::stringw(L"�gales");
		}
	case ECO_DIFFERENT:				// !=
		if (masculin)
		{
			if (singulier)
				return core::stringw(L"diff�rent");
			else
				return core::stringw(L"diff�rents");
		}
		else
		{
			if (singulier)
				return core::stringw(L"diff�rente");
			else
				return core::stringw(L"diff�rentes");
		}
	case ECO_LESS_EQUALS:			// <=
		if (masculin)
		{
			if (singulier)
				return core::stringw(L"inf�rieur ou �gal");
			else
				return core::stringw(L"inf�rieurs ou �gaux");
		}
		else
		{
			if (singulier)
				return core::stringw(L"inf�rieure ou �gale");
			else
				return core::stringw(L"inf�rieures ou �gales");
		}
	case ECO_LESS:					// <
		if (masculin)
		{
			if (singulier)
				return core::stringw(L"inf�rieur");
			else
				return core::stringw(L"inf�rieurs");
		}
		else
		{
			if (singulier)
				return core::stringw(L"inf�rieure");
			else
				return core::stringw(L"inf�rieures");
		}
	case ECO_MORE_EQUALS:			// >=
		if (masculin)
		{
			if (singulier)
				return core::stringw(L"sup�rieur ou �gal");
			else
				return core::stringw(L"sup�rieurs ou �gaux");
		}
		else
		{
			if (singulier)
				return core::stringw(L"sup�rieure ou �gale");
			else
				return core::stringw(L"sup�rieures ou �gales");
		}
	case ECO_MORE:					// >
		if (masculin)
		{
			if (singulier)
				return core::stringw(L"sup�rieur");
			else
				return core::stringw(L"sup�rieurs");
		}
		else
		{
			if (singulier)
				return core::stringw(L"sup�rieure");
			else
				return core::stringw(L"sup�rieures");
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
			desc.append(L" � ");
		swprintf_SS(L"%.2f �.", dataF);
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
			desc.append(L" � ");
		swprintf_SS(L"%.2f kg.", dataF);
		desc.append(textW_SS);
		break;
	case Objectives::EOT_DECHETS:
		if (winObjective)
			desc = L"Les d�chets doivent devenir ";
		else
			desc = L"Les d�chets doivent rester ";
		desc.append(Objectives::getComparisonOperatorStr(comparisonOp, true, false));
		if (comparisonOp == Objectives::ECO_DIFFERENT)
			desc.append(L" de ");
		else
			desc.append(L" � ");
		swprintf_SS(L"%.2f kg.", dataF);
		desc.append(textW_SS);
		break;
	case Objectives::EOT_TIME_PASSED:	// TODO : Am�liorer le texte "Le temps �coul�..." pour le rendre plus facilement compr�hensible
		if (winObjective)
			desc = L"Le temps �coul� doit devenir ";
		else
			desc = L"Le temps �coul� doit rester ";
		desc.append(Objectives::getComparisonOperatorStr(comparisonOp, true, true));
		if (comparisonOp == Objectives::ECO_DIFFERENT)
			desc.append(L" de ");
		else
			desc.append(L" � ");
		swprintf_SS(L"%.2f s (", abs(dataF));
		desc.append(textW_SS);
		Game::appendDays(desc, (u32)(abs(dataF) * DAY_TIME_INV));
		desc.append(L").");
		break;

	case Objectives::EOT_BATIMENT_NB:	// TODO : Am�liorer le texte "Le nombre de b�timents construits de type "ID" ..." pour le rendre plus facilement compr�hensible et moins long
		desc = L"Le nombre de b�timents construits de type \"";
		desc.append(StaticBatimentInfos::getInfos(dataID).name);
		if (winObjective)
			desc.append(L"\" doit devenir ");
		else
			desc.append(L"\" doit rester ");
		desc.append(Objectives::getComparisonOperatorStr(comparisonOp, true, true));
		if (comparisonOp == Objectives::ECO_DIFFERENT)
			desc.append(L" de ");
		else
			desc.append(L" � ");
		swprintf_SS(L"%u.", (u32)(abs(dataF)));
		desc.append(textW_SS);
		break;

	default:
		LOG_DEBUG("ObjectiveData::getObjectiveDescription() : Type d'objectif inconnu : type = " << type, ELL_WARNING);
		break;
	}

	return desc;
}
