#ifndef DEF_INFORMATIONS_WINDOW
#define DEF_INFORMATIONS_WINDOW

#include "global.h"
#include "CGUIReductableWindow.h"
#include "CGUITextBox.h"
#include "Batiments.h"
#include "Batiment.h"

using namespace gui;

class EcoWorldSystem;

class CGUIInformationsWindow : public CGUIReductableWindow
{
public:
	// Indique quel type d'information doit être affiché par cette fenêtre
	enum E_INFORMATION_TYPE
	{
		EIT_CONSTRUCTION,		// Informations sur la construction
		EIT_MULTI_CONSTRUCTION,	// Informations sur la construction de plusieurs bâtiments
		EIT_DESTRUCTION,		// Informations sur la destruction
		EIT_SELECTION			// Informations sur la sélection
	};

	CGUIInformationsWindow(EcoWorldSystem& m_system, IGUIEnvironment* environment, IGUIElement* parent, int id, const core::recti& rectangle);

	virtual void OnWindowResize();

	virtual bool OnEvent(const SEvent& event);

	// Met à jour la fenêtre d'informations avec ce bâtiment :
	// infosType :		Type des informations à afficher
	// batimentID :		ID du bâtiment à construire (seulement nécessaire pour les infos sur la construction)
	// batimentCount :	Nombre de bâtiments à construire (seulement nécessaire pour les infos sur la construction multiple)
	// batiment :		Informations sur le bâtiment actuel (seulement nécessaire pour les infos sur la destruction/sélection)
	void updateInfosTexte(E_INFORMATION_TYPE infosType, BatimentID batimentID, u32 batimentCount, Batiment* batiment);

	// Indique le texte à afficher dans la zone d'informations
	void setInfosTexte(const wchar_t* texte);

	// Indique le bâtiment actuellement sélectionné
	void setSelectedBatiment(Batiment* bat);

protected:
	// Met à jour la fenêtre d'informations suivant le type d'informations qu'elle doit afficher
	void updateConstructionText(BatimentID batimentID);
	void updateMultiConstructionText(BatimentID batimentID, u32 batimentCount);
	void updateDestructionText(Batiment* batiment);
	void updateSelectionText(Batiment* batiment);

	// Optimisations pour les fonctions ci-dessus :
	E_INFORMATION_TYPE lastInfosType;	// Le type des dernières informations affichées
	BatimentInfos lastBatimentInfos;	// Les dernières informations sur le batiment sélectionné
	u32 lastMultiBatCount;				// Le dernier nombre de jours bâtiments à créer dans les informations sur la construction de bâtiments multiples
	u32 lastBatDaysPassed;				// Le dernier nombre de jours écoulés du bâtiment sélectionné

	EcoWorldSystem& system;

	// Le bâtiment actuellement sélectionné
	Batiment* selectedBatiment;

	IGUIStaticText* pourcentageProductionText;		// Les texte au-dessus de la scroll bar du pourcentage de production
	IGUIScrollBar* pourcentageProductionScrollBar;	// La scroll bar permettant de régler le pourcentage de production du bâtiment actuel
	CGUITextBox* textBox;							// Le texte personnalisé contenant une zone de texte + une scroll bar permettant de le faire défiler

	bool showPourcentageProductionZone() const	// Détermine si on doit afficher la zone pour régler le pourcentage de production du bâtiment actuellement sélectionné
	{
		if (!selectedBatiment)
			return false;

		if (!StaticBatimentInfos::needPourcentageProduction(selectedBatiment->getID()))
			return false;

		// Vérifie que le bâtiment sélectionné n'est pas en construction ou en destruction :
		// dans ce cas, l'affichage de la barre de pourcentage de production n'est pas nécessaire
		return (!(selectedBatiment->isConstructing() || selectedBatiment->isDestroying()));
	}

	core::recti getAbsoluteRectangle(double x, double y, double x2, double y2)
	{
		return getAbsoluteRectangle(x, y, x2, y2, ClientRect);
	}
	core::recti getAbsoluteRectangle(double x, double y, double x2, double y2, const core::recti& parentRect)
	{
		return getAbsoluteRectangle(x, y, x2, y2, core::dimension2du(
				parentRect.LowerRightCorner.X - parentRect.UpperLeftCorner.X,
				parentRect.LowerRightCorner.Y - parentRect.UpperLeftCorner.Y));
	}
	core::recti getAbsoluteRectangle(double x, double y, double x2, double y2, const core::dimension2du& parentSize)
	{
		return core::recti((int)(x * parentSize.Width), (int)(y * parentSize.Height),
			(int)(x2 * parentSize.Width), (int)(y2 * parentSize.Height));
	}
};

#endif
