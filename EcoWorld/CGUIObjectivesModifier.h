#ifndef DEF_C_GUI_OBJECTIVES_MODIFIER
#define DEF_C_GUI_OBJECTIVES_MODIFIER

#include "global.h"
#include "Objectives.h"

using namespace gui;

// El�ment de la GUI permettant la visualisation/modification des objectifs � d�finir pour une partie
class CGUIObjectivesModifier : public IGUIElement
{
public:
	// Constructeur
	CGUIObjectivesModifier(IGUIEnvironment* environment, IGUIElement* parent, int id, const core::recti& rectangle);

	// Appel� lors d'un �v�nement
	virtual bool OnEvent(const SEvent& event);

	// R�initialise la liste actuelle des objectifs
	void reset();

	// Rempli la liste des objectifs avec la liste pass�e en param�tre
	void fillList(const core::list<ObjectiveData>& objectives);

protected:
	// La liste des donn�es des objectifs repr�sent�s dans cette liste
	core::list<ObjectiveData> objectives;



	// Les �l�ments de la GUI permettant de visualiser ces objectifs :
	IGUIListBox* listBox;				// La liste permettant de visualiser les objectifs actuellement d�finis
	IGUIButton* addButton;				// Le bouton pour ajouter un objectif � la liste
	IGUIButton* removeButton;			// Le bouton pour supprimer un objectif de la liste
	IGUIComboBox* typeComboBox;			// La liste permettant de choisir le type d'un objectif
	IGUIComboBox* winComboBox;			// La liste permettant de choisir si un objectif est n�cessaire pour gagner ou pour ne pas perdre
	IGUIComboBox* comparisonOpComboBox;	// La liste permettant de choisir l'op�rateur de comparaison pour un objectif
	IGUIEditBox* dataFEditBox;			// La zone de texte permettant d'indiquer la valeur num�rique de cet objectif
	IGUIComboBox* dataIDComboBox;		// La liste permettant d'indiquer la valeur de l'ID du b�timent concern� par cet objectif



	// Fonctions permettant de g�rer la modification des objectifs :
	void enableGUIElementsFromObjectiveType(Objectives::E_OBJECTIVE_TYPE type);	// Active/d�sactive certains �l�ments de la GUI suivant le type de l'objectif actuel
	void setCurrentSelectedObjective(int index);								// Indique le num�ro de l'objectif actuellement s�lectionn�
	void addObjective();														// Ajoute un objectif � la liste et le s�lectionne
	void removeObjective(int index);											// Supprime un objectif de cette liste et modifie l'objectif s�lectionn� de la liste de la GUI
	void changeObjectiveType(Objectives::E_OBJECTIVE_TYPE type);				// Change le type de cet objectif
	void changeObjectiveWin(bool winObjective);									// Change cet objectif en mode gagnant ou �liminatoire
	void changeComparisonOp(Objectives::E_COMPARISON_OPERATOR comparisonOp);	// Change l'op�rateur de comparaison utilis� pour cet objectif
	void changeDataF(float dataF);												// Change les donn�es num�riques de cet objectif
	void changeDataID(BatimentID dataID);										// Change les donn�es d'ID de cet objectif



	// Obtient les informations sur un certain objectif de cette liste (d�fini d'apr�s son index dans la liste de la GUI)
	ObjectiveData getObjective(int index) const
	{
		// V�rifie que cet index est valide, sinon on retourne un objectif par d�faut
		if (index < 0 || index >= (int)(objectives.size()))
			return ObjectiveData();

		// Retourne l'objectif avec l'index indiqu�
		core::list<ObjectiveData>::ConstIterator it = objectives.begin();
		it += index;
		return (*it);
	}

public:
	// D�termine si une EditBox de cet �l�ment a le focus
	bool isEditBoxFocused() const
	{
		if (!Environment || !dataFEditBox)
			return false;
		return (Environment->getFocus() == dataFEditBox);
	}

	// Accesseurs inline :

	// Obtient la liste des donn�es des objectifs repr�sent�s dans cette liste
	const core::list<ObjectiveData>& getObjectives() const	{ return objectives; }
};

#endif
