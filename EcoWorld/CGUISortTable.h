#ifndef DEF_C_GUI_SORT_TABLE
#define DEF_C_GUI_SORT_TABLE

#include "global.h"
#include "CGUITable.h"

using namespace gui;

// Tableau bas� sur la classe CGUITable d'Irrlicht SVN 1.8.0-alpha, mais permettant de personnaliser le mode de tri de ses lignes, et non plus avoir un tri forc�ment bas� sur leur valeur alphab�tique (ASCII)
class CGUISortTable : public CGUITable
{
public:
	// Enum indiquant les diff�rents types de tri � utiliser pour chaque cellule
	enum E_TRI_TYPES
	{
		ETT_ALPHABETICAL,	// Tri par ordre alphab�tique du texte des cellules (d'apr�s les tables de caract�re ASCII)
		ETT_VALUE,			// Tri par valeur de chaque cellule (le texte de chaque cellule est converti en float puis cette valeur est compar�e aux autres cellules (g�re aussi le texte "-"))
		ETT_TIME,			// Tri par le temps indiqu� dans chaque cellule (n�cessite que le texte de la cellule soit sous la forme d'un temps ("X ann�es Y mois Z jours") (g�re aussi les textes "Infini" et "-"))
		ETT_IP_ADDRESS,		// Tri par adresse IP indiqu�e dans chaque cellule (n�cessite que le texte de la cellule soit sous la forme d'une adresse IP ("W.X.Y.Z" : ex : "127.0.0.1") (g�re aussi les comparaison de versions ce cette mani�re : ex : "1.0.0.0"))

		ETT_COUNT			// Nombre d'�lements dans cet enum, ne pas utiliser
							// Attention : Penser � mettre � jour main.cpp lors de l'initialisation statique des types de tris � chaque fois que cette valeur augmente !
	};

	// Constructeur
	CGUISortTable(IGUIEnvironment* environment, IGUIElement* parent, int id, const core::recti& rectangle,
		bool clip = true, bool drawBack = false, bool moveOverSelect = true);

	// R�organise les lignes du tableau d'apr�s leur valeur num�rique :
	//! This tells the table to start ordering all the rows. You
	//! need to explicitly tell the table to reorder the rows when
	//! a new row is added or the cells data is changed. This makes
	//! the system more flexible and doesn't make you pay the cost
	//! of ordering when adding a lot of rows.
	//! \param columnIndex: When set to -1 the active column is used.
	virtual void orderRows(int columnIndex = -1, EGUI_ORDERING_MODE mode = EGOM_NONE);

protected:
	// Le type des tris pour pouvoir obtenir des pointeurs sur ces valeurs (statiques pour �viter d'allouer ces donn�es � chaque tableau cr��)
	static const E_TRI_TYPES triTypes[ETT_COUNT];

	// Compare la valeur de deux cellules (d'apr�s leur valeur num�rique) pour d�terminer laquelle doit �tre au-dessus de l'autre
	// Retourne true si la cellule 1 doit �tre au-dessus de la cellule 2, false sinon
	bool compareCells(const Cell& cell1, const Cell& cell2);

	// Retourne la valeur num�rique (sous forme de float) d'une cha�ne de caract�res
	// (fonctionnera m�me si elle contient des lettres ou des caract�res sp�ciaux, mais retournera 0.0f en cas d'erreur)
	float getFloatValue(const core::stringw& valueStr);

	// Retourne la valeur num�rique (sous forme de float) d'une cha�ne de caract�res sous la forme d'un temps �coul� ("X ann�es Y mois Z jours")
	// (fonctionnera m�me si elle n'est pas sous cette forme, mais retournera 0.0f en cas d'erreur)
	float getTimeValue(const core::stringw& valueStr);

	// Retourne la valeur num�rique (sous forme de float) d'une cha�ne de caract�res sous la forme d'une adresse IP ("W.X.Y.Z" : ex : "127.0.0.1"),
	// en choisissant dans le second param�tre quel membre de l'adresse IP utiliser (0 pour W, 1 pour X, 2 pour Y, 3 pour Z, ... si d'autres membres existent)
	// (fonctionnera m�me si elle n'est pas sous cette forme, mais retournera 0.0f en cas d'erreur)
	float getIPAddressMemberValue(const core::stringw& valueStr, u32 member);

public:
	// Retourne un pointeur pour ce type de tri, qui pourra �tre utilis� pour �tre assign� au membre "Data" d'une "Cell", et ainsi d�finir le type de tri de cette cellule
	static void* getTriTypePointer(E_TRI_TYPES type)
	{
		if (type >= 0 && type < ETT_COUNT)
			return (void*)(&(triTypes[type]));
		return NULL;
	}



	// Fonctions surcharg�es de IGUIElement (mais non virtuelles !!!) :
	// Pallie � un probl�me de CGUITable : Recalcule les positions des scrolls bars lors du redimensionnement de ce contr�le

	//! Sets the relative rectangle of this element.
	/** \param r The absolute position to set */
	void setRelativePosition(const core::recti& r)
	{
		IGUIElement::setRelativePosition(r);
		refreshControls();
	}
	//! Sets the relative rectangle of this element, maintaining its current width and height
	/** \param position The new relative position to set. Width and height will not be changed. */
	void setRelativePosition(const core::vector2di& position)
	{
		IGUIElement::setRelativePosition(position);
		refreshControls();
	}
	//! Sets the relative rectangle of this element as a proportion of its parent's area.
	/** \note This method used to be 'void setRelativePosition(const core::rect<f32>& r)'
	\param r  The rectangle to set, interpreted as a proportion of the parent's area.
	Meaningful values are in the range [0...1], unless you intend this element to spill
	outside its parent. */
	void setRelativePositionProportional(const core::rectf& r)
	{
		IGUIElement::setRelativePositionProportional(r);
		refreshControls();
	}
};

#endif
