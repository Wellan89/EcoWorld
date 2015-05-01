#include "CGUIObjectivesModifier.h"
#include "EcoWorldSystem.h"
#include <sstream>	// Pour l'utilisation de la structure wistringstream de la STL (permet la conversion chaîne de caractères -> float)

CGUIObjectivesModifier::CGUIObjectivesModifier(IGUIEnvironment* environment, IGUIElement* parent, int id, const core::recti& rectangle)
 : IGUIElement(EGUIET_ELEMENT, environment, parent, id, rectangle), listBox(NULL)
{
#ifdef _DEBUG
	setDebugName("CGUIObjectivesModifier");
#endif

	// Crée les éléments de la GUI permettant de visualiser/modifier la liste des objectifs :
	// TODO : Ajouter des textes statiques et revoir les textes des tooltips pour aider le joueur dans le paramêtrage des objectifs

	environment->addStaticText(L"Objectifs actuels :", getAbsoluteRectangle(0.0f, 0.0f, 0.62f, 0.08f, rectangle), false, true, this)->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);

	listBox = environment->addListBox(getAbsoluteRectangle(0.0f, 0.08f, 0.62f, 1.0f, rectangle), this, -1, true);
	listBox->setToolTipText(L"Sélectionnez ici les objectifs à modifier");

	addButton = environment->addButton(getAbsoluteRectangle(0.64f, 0.08f, 0.81f, 0.16f, rectangle), this, -1,
		L"Ajouter un objectif", L"Cliquez sur ce bouton pour ajouter un objectif");
	removeButton = environment->addButton(getAbsoluteRectangle(0.83f, 0.08f, 1.0f, 0.16f, rectangle), this, -1,
		L"Supprimer cet objectif", L"Cliquez sur ce bouton pour supprimer l'objectif sélectionné");

	winComboBox = environment->addComboBox(getAbsoluteRectangle(0.64f, 0.18f, 1.0f, 0.26f, rectangle), this);
	winComboBox->setToolTipText(L"Indiquez ici si cet objectif est nécessaire pour gagner cette partie ou s'il est éliminatoire");
	winComboBox->addItem(L"Objectif gagnant", 1);		// winObjective = true
	winComboBox->addItem(L"Objectif éliminatoire", 0);	// winObjective = false

	typeComboBox = environment->addComboBox(getAbsoluteRectangle(0.64f, 0.28f, 1.0f, 0.36f, rectangle), this);
	typeComboBox->setToolTipText(L"Indiquez ici la valeur du jeu que cet objectif vérifiera");
	typeComboBox->addItem(L"Budget", Objectives::EOT_BUDGET);
	typeComboBox->addItem(L"Effet de serre", Objectives::EOT_EFFET_SERRE);
	typeComboBox->addItem(L"Déchets", Objectives::EOT_DECHETS);
	typeComboBox->addItem(L"Temps écoulé", Objectives::EOT_TIME_PASSED);
	typeComboBox->addItem(L"Nombre de bâtiments", Objectives::EOT_BATIMENT_NB);

	comparisonOpComboBox = environment->addComboBox(getAbsoluteRectangle(0.64f, 0.38f, 1.0f, 0.46f, rectangle), this);
	comparisonOpComboBox->setToolTipText(L"Indiquez ici quel est l'opérateur de comparaison avec lequel la valeur numérique de cet objectif sera comparée");
	comparisonOpComboBox->addItem(L"Egal à (=)", Objectives::ECO_EQUALS);
	comparisonOpComboBox->addItem(L"Différent de (!=)", Objectives::ECO_DIFFERENT);
	comparisonOpComboBox->addItem(L"Inférieur ou égal à (<=)", Objectives::ECO_LESS_EQUALS);
	comparisonOpComboBox->addItem(L"Inférieur à (<)", Objectives::ECO_LESS);
	comparisonOpComboBox->addItem(L"Supérieur ou égal à (>=)", Objectives::ECO_MORE_EQUALS);
	comparisonOpComboBox->addItem(L"Supérieur à (>)", Objectives::ECO_MORE);

	// TODO : Implémenter une EditBox n'acceptant que les entrées numériques
	dataFEditBox = environment->addEditBox(L"0.00", getAbsoluteRectangle(0.64f, 0.48f, 1.0f, 0.56f, rectangle), true, this);
	dataFEditBox->setToolTipText(L"Indiquez ici quelle est la valeur numérique avec laquelle cet objectif sera comparé");

	dataIDComboBox = environment->addComboBox(getAbsoluteRectangle(0.64f, 0.58f, 1.0f, 0.66f, rectangle), this);
	dataIDComboBox->setToolTipText(L"Indiquez ici quel est le type des bâtiments vérifiés par objectif");

	// Ajoute tous les bâtiments du jeu en tant qu'élement à la liste des ID :
	for (int i = 0; i < BI_COUNT; ++i)
		if (i != BI_aucun)
			dataIDComboBox->addItem(StaticBatimentInfos::getInfos((BatimentID)i).name, i);
		else
			dataIDComboBox->addItem(L"Aucun", BI_aucun);

	// Réinitialise la liste des objectifs
	//reset();

	// Rempli la liste des objectifs avec les objectifs par défaut lors de la création d'une nouvelle partie
	fillList(EcoWorldSystem::getNewGameDefaultObjectives());

	// Sélectionne le premier objectif par défaut
	listBox->setSelected(0);
	setCurrentSelectedObjective(listBox->getSelected());
}
void CGUIObjectivesModifier::reset()
{
	// Efface la liste des objectifs
	objectives.clear();

	// Efface la liste actuelle
	listBox->clear();

	// Indique qu'aucun objectif n'est sélectionné
	setCurrentSelectedObjective(-1);
}
void CGUIObjectivesModifier::fillList(const core::list<ObjectiveData>& objectivesList)
{
	// Ajoute les objectifs à la liste actuelle
	const core::list<ObjectiveData>::ConstIterator END = objectivesList.end();
	for (core::list<ObjectiveData>::ConstIterator it = objectivesList.begin(); it != END; ++it)
	{
		// Ajoute cet objectif à la liste des objectifs
		objectives.push_back(*it);

		// Crée un nouvel élement dans la liste pour cet objectif
		listBox->addItem((*it).getObjectiveDescription().c_str());
	}
}
bool CGUIObjectivesModifier::OnEvent(const SEvent& event)
{
	// On ne gère ici que les évènements de la GUI
	if (event.EventType == EET_GUI_EVENT && IsEnabled)
	{
		switch (event.GUIEvent.EventType)
		{
		case EGET_LISTBOX_CHANGED:
		case EGET_LISTBOX_SELECTED_AGAIN:
			if (event.GUIEvent.Caller == listBox)
			{
				// Indique l'objectif actuellement sélectionné
				setCurrentSelectedObjective(listBox->getSelected());
			}
			break;

		case EGET_BUTTON_CLICKED:
			if (event.GUIEvent.Caller == addButton)
			{
				// Ajoute un objectif à la liste
				addObjective();
			}
			else if (event.GUIEvent.Caller == removeButton)
			{
				// Supprime cet objectif de la liste
				removeObjective(listBox->getSelected());
			}
			break;

		case EGET_COMBO_BOX_CHANGED:
			if (event.GUIEvent.Caller == typeComboBox)
			{
				// Change le type de cet objectif
				changeObjectiveType((Objectives::E_OBJECTIVE_TYPE)typeComboBox->getItemData(typeComboBox->getSelected()));
			}
			else if (event.GUIEvent.Caller == winComboBox)
			{
				// Change cet objectif en mode gagnant ou éliminatoire
				changeObjectiveWin(winComboBox->getItemData(winComboBox->getSelected()) != 0);
			}
			else if (event.GUIEvent.Caller == comparisonOpComboBox)
			{
				// Change l'opérateur de comparaison utilisé pour cet objectif
				changeComparisonOp((Objectives::E_COMPARISON_OPERATOR)comparisonOpComboBox->getItemData(comparisonOpComboBox->getSelected()));
			}
			else if (event.GUIEvent.Caller == dataIDComboBox)
			{
				// Change les données d'ID de cet objectif
				changeDataID((BatimentID)dataIDComboBox->getItemData(dataIDComboBox->getSelected()));
			}
			break;

		case EGET_ELEMENT_FOCUS_LOST:
		case EGET_EDITBOX_ENTER:
			if (event.GUIEvent.Caller == dataFEditBox)
			{
				// Converti la chaîne de caractères retournée par la zone de texte en float :

				// Utilise la structure wistringstream de la STL pour convertir cette chaîne en float
				float dataF = 0.0f;
				wistringstream str(dataFEditBox->getText());
				str >> dataF;

				// Si la conversion a échouée, on indique une valeur par défaut
				if (str.fail())
					dataF = 0.0f;

				// Change les données numériques de cet objectif
				changeDataF(dataF);
			}
			break;
		}
	}

	return IGUIElement::OnEvent(event);
}
void CGUIObjectivesModifier::enableGUIElementsFromObjectiveType(Objectives::E_OBJECTIVE_TYPE type)
{
	// Active tous les élements permettant de modifier cet objectif :
	typeComboBox->setEnabled(true);
	winComboBox->setEnabled(true);
	comparisonOpComboBox->setEnabled(true);
	dataFEditBox->setEnabled(true);
	dataIDComboBox->setEnabled(true);

	switch (type)
	{
	case Objectives::EOT_BUDGET:
	case Objectives::EOT_EFFET_SERRE:
	case Objectives::EOT_DECHETS:
	case Objectives::EOT_TIME_PASSED:
		// Désactive et réinitialise la zone permettant de choisir la donnée ID de cet objectif
		dataIDComboBox->setEnabled(false);
		dataIDComboBox->setSelected(0);		// Sélectionne l'élément "Aucun"
		break;

	case Objectives::EOT_BATIMENT_NB:
		break;

	default:
#ifdef _DEBUG
		cout << "CGUIObjectivesModifier::enableGUIElementsFromObjectiveType(" << type << ") : Type d'objectif inconnu : type = " << type << endl;
#endif
		break;
	}
}
void CGUIObjectivesModifier::setCurrentSelectedObjective(int index)
{
	// On suppose ici que l'objectif est déjà sélectionné dans la liste de la GUI

	// Vérifie qu'un objectif est réellement sélectionné
	if (index < 0)
	{
		// Sinon, désactive et réinitialise tous les éléments de la GUI, dont le bouton pour supprimer un objectif :

		removeButton->setEnabled(false);

		typeComboBox->setEnabled(false);
		typeComboBox->setSelected(0);

		winComboBox->setEnabled(false);
		winComboBox->setSelected(0);

		comparisonOpComboBox->setEnabled(false);
		comparisonOpComboBox->setSelected(0);

		dataFEditBox->setEnabled(false);
		dataFEditBox->setText(L"0.00");

		dataIDComboBox->setEnabled(false);
		dataIDComboBox->setSelected(0);

		return;
	}

	// Active le bouton pour supprimer cet objectif
	removeButton->setEnabled(true);

	// Obtient l'objectif actuellement sélectionné
	const ObjectiveData objectiveData = getObjective(index);

	// Modifie les informations des zones de modification de cet objectif :
	typeComboBox->setSelected(typeComboBox->getIndexForItemData(objectiveData.type));
	winComboBox->setSelected(objectiveData.winObjective ? 0 : 1);
	comparisonOpComboBox->setSelected(comparisonOpComboBox->getIndexForItemData(objectiveData.comparisonOp));

	swprintf_SS(L"%.2f", objectiveData.dataF);
	dataFEditBox->setText(textW_SS);

	dataIDComboBox->setSelected(dataIDComboBox->getIndexForItemData(objectiveData.dataID));

	// Active/désactive certains éléments de la GUI suivant le type de cet objectif
	enableGUIElementsFromObjectiveType(objectiveData.type);
}
void CGUIObjectivesModifier::addObjective()
{
	// Crée un objectif par défaut
	ObjectiveData objective;

	// Ajoute cet objectif à la liste des objectifs
	objectives.push_back(objective);

	// Ajoute un élément représentant cet objectif dans la liste des objectifs de la GUI et le sélectionné
	listBox->setSelected(listBox->addItem(objective.getObjectiveDescription().c_str()));

	// Indique qu'un nouvel objectif a été sélectionné
	setCurrentSelectedObjective(listBox->getSelected());
}
void CGUIObjectivesModifier::removeObjective(int index)
{
	if (index < 0 || (u32)index >= objectives.size())
		return;

	// Supprime cet objectif de la liste des objectifs de la GUI
	listBox->removeItem((u32)index);

	// Sélectionne l'élément d'index inférieur dans la liste des objectifs de la GUI s'il existe, sinon celui avec cet index
	listBox->setSelected((index == 0 && (u32)index < objectives.size()) ? index : index - 1);

	// Indique qu'un nouvel objectif a été sélectionné
	setCurrentSelectedObjective(listBox->getSelected());

	// Supprime cet objectif de la liste des objectifs
	core::list<ObjectiveData>::Iterator it = objectives.begin();
	if (index > 0)	it += index;
	objectives.erase(it);
}
void CGUIObjectivesModifier::changeObjectiveType(Objectives::E_OBJECTIVE_TYPE type)
{
	// On suppose ici que le bon type d'objectif est déjà sélectionné dans les éléments de la GUI

	const int index = listBox->getSelected();
	if (index < 0 || (u32)index >= objectives.size())
		return;

	// Obtient l'objectif sélectionné
	core::list<ObjectiveData>::Iterator it = objectives.begin();
	it += index;

	// Modifie le type de cet objectif
	(*it).type = type;

	// Met à jour le texte dans la liste de cet objectif
	listBox->setItem(index, (*it).getObjectiveDescription().c_str(), -1);

	// Active/désactive certains éléments de la GUI suivant le type de l'objectif actuel
	enableGUIElementsFromObjectiveType(type);
}
void CGUIObjectivesModifier::changeObjectiveWin(bool winObjective)
{
	// On suppose ici que le bon mode gagnant ou éliminatoire est déjà sélectionné dans les éléments de la GUI

	const int index = listBox->getSelected();
	if (index < 0 || (u32)index >= objectives.size())
		return;

	// Obtient l'objectif sélectionné
	core::list<ObjectiveData>::Iterator it = objectives.begin();
	it += index;

	// Modifie le mode gagnant ou éliminatoire de cet objectif
	(*it).winObjective = winObjective;

	// Met à jour le texte dans la liste de cet objectif
	listBox->setItem(index, (*it).getObjectiveDescription().c_str(), -1);
}
void CGUIObjectivesModifier::changeComparisonOp(Objectives::E_COMPARISON_OPERATOR comparisonOp)
{
	// On suppose ici que le bon opérateur de comparaison est déjà sélectionné dans les éléments de la GUI

	const int index = listBox->getSelected();
	if (index < 0 || (u32)index >= objectives.size())
		return;

	// Obtient l'objectif sélectionné
	core::list<ObjectiveData>::Iterator it = objectives.begin();
	it += index;

	// Modifie l'opérateur de comparaison de cet objectif
	(*it).comparisonOp = comparisonOp;

	// Met à jour le texte dans la liste de cet objectif
	listBox->setItem(index, (*it).getObjectiveDescription().c_str(), -1);
}
void CGUIObjectivesModifier::changeDataF(float dataF)
{
	// On suppose ici que les bonnes données numériques sont déjà indiquées dans les éléments de la GUI

	const int index = listBox->getSelected();
	if (index < 0 || (u32)index >= objectives.size())
		return;

	// Obtient l'objectif sélectionné
	core::list<ObjectiveData>::Iterator it = objectives.begin();
	it += index;

	// Modifie les données numériques de cet objectif
	(*it).dataF = dataF;

	// Met à jour le texte dans la liste de cet objectif
	listBox->setItem(index, (*it).getObjectiveDescription().c_str(), -1);
}
void CGUIObjectivesModifier::changeDataID(BatimentID dataID)
{
	// On suppose ici que le bon ID est déjà sélectionné dans les éléments de la GUI

	const int index = listBox->getSelected();
	if (index < 0 || (u32)index >= objectives.size())
		return;

	// Obtient l'objectif sélectionné
	core::list<ObjectiveData>::Iterator it = objectives.begin();
	it += index;

	// Modifie l'ID objectif
	(*it).dataID = dataID;

	// Met à jour le texte dans la liste de cet objectif
	listBox->setItem(index, (*it).getObjectiveDescription().c_str(), -1);
}
