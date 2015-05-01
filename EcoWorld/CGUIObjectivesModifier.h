#ifndef DEF_C_GUI_OBJECTIVES_MODIFIER
#define DEF_C_GUI_OBJECTIVES_MODIFIER

#include "global.h"
#include "Objectives.h"

using namespace gui;

// Elément de la GUI permettant la visualisation/modification des objectifs à définir pour une partie
class CGUIObjectivesModifier : public IGUIElement
{
public:
	// Constructeur
	CGUIObjectivesModifier(IGUIEnvironment* environment, IGUIElement* parent, int id, const core::recti& rectangle);

	// Appelé lors d'un évènement
	virtual bool OnEvent(const SEvent& event);

	// Réinitialise la liste actuelle des objectifs
	void reset();

	// Rempli la liste des objectifs avec la liste passée en paramêtre
	void fillList(const core::list<ObjectiveData>& objectives);

protected:
	// La liste des données des objectifs représentés dans cette liste
	core::list<ObjectiveData> objectives;



	// Les éléments de la GUI permettant de visualiser ces objectifs :
	IGUIListBox* listBox;				// La liste permettant de visualiser les objectifs actuellement définis
	IGUIButton* addButton;				// Le bouton pour ajouter un objectif à la liste
	IGUIButton* removeButton;			// Le bouton pour supprimer un objectif de la liste
	IGUIComboBox* typeComboBox;			// La liste permettant de choisir le type d'un objectif
	IGUIComboBox* winComboBox;			// La liste permettant de choisir si un objectif est nécessaire pour gagner ou pour ne pas perdre
	IGUIComboBox* comparisonOpComboBox;	// La liste permettant de choisir l'opérateur de comparaison pour un objectif
	IGUIEditBox* dataFEditBox;			// La zone de texte permettant d'indiquer la valeur numérique de cet objectif
	IGUIComboBox* dataIDComboBox;		// La liste permettant d'indiquer la valeur de l'ID du bâtiment concerné par cet objectif



	// Fonctions permettant de gérer la modification des objectifs :
	void enableGUIElementsFromObjectiveType(Objectives::E_OBJECTIVE_TYPE type);	// Active/désactive certains éléments de la GUI suivant le type de l'objectif actuel
	void setCurrentSelectedObjective(int index);								// Indique le numéro de l'objectif actuellement sélectionné
	void addObjective();														// Ajoute un objectif à la liste et le sélectionne
	void removeObjective(int index);											// Supprime un objectif de cette liste et modifie l'objectif sélectionné de la liste de la GUI
	void changeObjectiveType(Objectives::E_OBJECTIVE_TYPE type);				// Change le type de cet objectif
	void changeObjectiveWin(bool winObjective);									// Change cet objectif en mode gagnant ou éliminatoire
	void changeComparisonOp(Objectives::E_COMPARISON_OPERATOR comparisonOp);	// Change l'opérateur de comparaison utilisé pour cet objectif
	void changeDataF(float dataF);												// Change les données numériques de cet objectif
	void changeDataID(BatimentID dataID);										// Change les données d'ID de cet objectif



	// Obtient les informations sur un certain objectif de cette liste (défini d'après son index dans la liste de la GUI)
	ObjectiveData getObjective(int index) const
	{
		// Vérifie que cet index est valide, sinon on retourne un objectif par défaut
		if (index < 0 || index >= (int)(objectives.size()))
			return ObjectiveData();

		// Retourne l'objectif avec l'index indiqué
		core::list<ObjectiveData>::ConstIterator it = objectives.begin();
		it += index;
		return (*it);
	}

public:
	// Détermine si une EditBox de cet élément a le focus
	bool isEditBoxFocused() const
	{
		if (!Environment || !dataFEditBox)
			return false;
		return (Environment->getFocus() == dataFEditBox);
	}

	// Accesseurs inline :

	// Obtient la liste des données des objectifs représentés dans cette liste
	const core::list<ObjectiveData>& getObjectives() const	{ return objectives; }
};

#endif
