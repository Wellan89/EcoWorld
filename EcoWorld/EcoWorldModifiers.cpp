#include "EcoWorldModifiers.h"
#include "Batiments.h"

EcoWorldModifiers::EcoWorldModifiers()
{
	// Indique la difficulté normale par défaut
	setDifficulty(ED_normal);
}
void EcoWorldModifiers::setDifficulty(E_DIFFICULTY newDifficulty)
{
	// Conserve les paramêtres de difficulté actuels si la difficulté désirée est personnalisée
	if (newDifficulty == ED_personnalisee)
		return;

	difficulty = newDifficulty;

	prixFactor = 1.0f;
	loyerFactor = 1.0f;
	taxesFactor = 1.0f;
	energieFactor = 1.0f;
	effetSerreFactor = 1.0f;
	dechetsFactor = 1.0f;
	ressourcesConsommationFactor = 1.0f;
	ressourcesProductionFactor = 1.0f;
	constructionTimeFactor = 1.0f;
	dureeVieFactor = 1.0f;
	unlockPopulationNeededFactor = 1.0f;
	minSatisfactionFactor = 1.0f;
	startBudgetFactor = 1.0f;
	startRessourcesFactor = 1.0f;

	switch (difficulty)
	{
	case ED_facile:
		prixFactor = 0.5f;
		loyerFactor = 2.0f;
		taxesFactor = 0.5f;
		energieFactor = 2.0f;
		effetSerreFactor = 0.5f;
		dechetsFactor = 0.5f;
		ressourcesConsommationFactor = 0.5f;
		ressourcesProductionFactor = 2.0f;
		constructionTimeFactor = 0.5f;
		dureeVieFactor = 2.0f;
		unlockPopulationNeededFactor = 0.5f;
		minSatisfactionFactor = 0.5f;
		startBudgetFactor = 2.0f;
		startRessourcesFactor = 2.0f;
		break;

	case ED_difficile:
		prixFactor = 2.0f;
		loyerFactor = 0.5f;
		taxesFactor = 2.0f;
		energieFactor = 0.5f;
		effetSerreFactor = 2.0f;
		dechetsFactor = 2.0f;
		ressourcesConsommationFactor = 2.0f;
		ressourcesProductionFactor = 0.5f;
		constructionTimeFactor = 2.0f;
		dureeVieFactor = 0.5f;
		unlockPopulationNeededFactor = 2.0f;
		minSatisfactionFactor = 2.0f;
		startBudgetFactor = 0.5f;
		startRessourcesFactor = 0.5f;
		break;

	case ED_normal:
		// Les valeurs actuelles (par défaut) des modifiers représentent les valeurs de la difficulté normale
		break;
	default:
		LOG_DEBUG("EcoWorldModifiers::setDifficulty(" << difficulty << ") : Niveau de difficulte inconnu : difficulty = " << difficulty, ELL_WARNING);
		break;
	}

	// Recrée les informations statiques sur les batiments d'après les modifiers de jeu
	StaticBatimentInfos::init(*this);
}
void EcoWorldModifiers::save(io::IAttributes* out, io::IXMLWriter* writer) const
{
	if (!out || !writer)
		return;

	out->addFloat("prixFactor", prixFactor);
	out->addFloat("loyerFactor", loyerFactor);
	out->addFloat("taxesFactor", taxesFactor);
	out->addFloat("energieFactor", energieFactor);
	out->addFloat("effetSerreFactor", effetSerreFactor);
	out->addFloat("dechetsFactor", dechetsFactor);
	out->addFloat("ressourcesConsommationFactor", ressourcesConsommationFactor);
	out->addFloat("ressourcesProductionFactor", ressourcesProductionFactor);
	out->addFloat("constructionTimeFactor", constructionTimeFactor);
	out->addFloat("dureeVieFactor", dureeVieFactor);
	out->addFloat("unlockPopulationNeededFactor", unlockPopulationNeededFactor);
	out->addFloat("minSatisfactionFactor", minSatisfactionFactor);
	// Inutiles car affectés au budget et ressources de départ :
	//out->addFloat("startBudgetFactor", startBudgetFactor);
	//out->addFloat("startRessourcesFactor", startRessourcesFactor);

	out->write(writer, false, L"Modifiers");
	out->clear();
}
void EcoWorldModifiers::load(io::IAttributes* in, io::IXMLReader* reader)
{
	if (!in || !reader)
		return;

	// Remet les modifiers à leur valeur normale avant le chargement
	setDifficulty(ED_normal);
	difficulty = ED_personnalisee;

	reader->resetPosition();
	if (in->read(reader, false, L"Modifiers"))
	{
		if (in->existsAttribute("prixFactor"))						prixFactor = in->getAttributeAsFloat("prixFactor");
		if (in->existsAttribute("loyerFactor"))						loyerFactor = in->getAttributeAsFloat("loyerFactor");
		if (in->existsAttribute("taxesFactor"))						taxesFactor = in->getAttributeAsFloat("taxesFactor");
		if (in->existsAttribute("energieFactor"))					energieFactor = in->getAttributeAsFloat("energieFactor");
		if (in->existsAttribute("effetSerreFactor"))				effetSerreFactor = in->getAttributeAsFloat("effetSerreFactor");
		if (in->existsAttribute("dechetsFactor"))					dechetsFactor = in->getAttributeAsFloat("dechetsFactor");
		if (in->existsAttribute("ressourcesConsommationFactor"))	ressourcesConsommationFactor = in->getAttributeAsFloat("ressourcesConsommationFactor");
		if (in->existsAttribute("ressourcesProductionFactor"))		ressourcesProductionFactor = in->getAttributeAsFloat("ressourcesProductionFactor");
		if (in->existsAttribute("constructionTimeFactor"))			constructionTimeFactor = in->getAttributeAsFloat("constructionTimeFactor");
		if (in->existsAttribute("dureeVieFactor"))					dureeVieFactor = in->getAttributeAsFloat("dureeVieFactor");
		if (in->existsAttribute("unlockPopulationNeededFactor"))	unlockPopulationNeededFactor = in->getAttributeAsFloat("unlockPopulationNeededFactor");
		if (in->existsAttribute("minSatisfactionFactor"))			minSatisfactionFactor = in->getAttributeAsFloat("minSatisfactionFactor");
		// Inutiles car affectés au budget et ressources de départ :
		//if (in->existsAttribute("startBudgetFactor"))				startBudgetFactor = in->getAttributeAsFloat("startBudgetFactor");
		//if (in->existsAttribute("startRessourcesFactor"))			startRessourcesFactor = in->getAttributeAsFloat("startRessourcesFactor");

		in->clear();
	}

	// Recrée les informations statiques sur les batiments d'après les modifiers de jeu
	StaticBatimentInfos::init(*this);
}
