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
	// Indique quel type d'information doit �tre affich� par cette fen�tre
	enum E_INFORMATION_TYPE
	{
		EIT_CONSTRUCTION,		// Informations sur la construction
		EIT_MULTI_CONSTRUCTION,	// Informations sur la construction de plusieurs b�timents
		EIT_DESTRUCTION,		// Informations sur la destruction
		EIT_SELECTION			// Informations sur la s�lection
	};

	CGUIInformationsWindow(EcoWorldSystem& m_system, IGUIEnvironment* environment, IGUIElement* parent, int id, const core::recti& rectangle);

	virtual void OnWindowResize();

	virtual bool OnEvent(const SEvent& event);

	// Met � jour la fen�tre d'informations avec ce b�timent :
	// infosType :		Type des informations � afficher
	// batimentID :		ID du b�timent � construire (seulement n�cessaire pour les infos sur la construction)
	// batimentCount :	Nombre de b�timents � construire (seulement n�cessaire pour les infos sur la construction multiple)
	// batiment :		Informations sur le b�timent actuel (seulement n�cessaire pour les infos sur la destruction/s�lection)
	void updateInfosTexte(E_INFORMATION_TYPE infosType, BatimentID batimentID, u32 batimentCount, Batiment* batiment);

	// Indique le texte � afficher dans la zone d'informations
	void setInfosTexte(const wchar_t* texte);

	// Indique le b�timent actuellement s�lectionn�
	void setSelectedBatiment(Batiment* bat);

protected:
	// Met � jour la fen�tre d'informations suivant le type d'informations qu'elle doit afficher
	void updateConstructionText(BatimentID batimentID);
	void updateMultiConstructionText(BatimentID batimentID, u32 batimentCount);
	void updateDestructionText(Batiment* batiment);
	void updateSelectionText(Batiment* batiment);

	// Optimisations pour les fonctions ci-dessus :
	E_INFORMATION_TYPE lastInfosType;	// Le type des derni�res informations affich�es
	BatimentInfos lastBatimentInfos;	// Les derni�res informations sur le batiment s�lectionn�
	u32 lastMultiBatCount;				// Le dernier nombre de jours b�timents � cr�er dans les informations sur la construction de b�timents multiples
	u32 lastBatDaysPassed;				// Le dernier nombre de jours �coul�s du b�timent s�lectionn�

	EcoWorldSystem& system;

	// Le b�timent actuellement s�lectionn�
	Batiment* selectedBatiment;

	IGUIStaticText* pourcentageProductionText;		// Les texte au-dessus de la scroll bar du pourcentage de production
	IGUIScrollBar* pourcentageProductionScrollBar;	// La scroll bar permettant de r�gler le pourcentage de production du b�timent actuel
	CGUITextBox* textBox;							// Le texte personnalis� contenant une zone de texte + une scroll bar permettant de le faire d�filer

	bool showPourcentageProductionZone() const	// D�termine si on doit afficher la zone pour r�gler le pourcentage de production du b�timent actuellement s�lectionn�
	{
		if (!selectedBatiment)
			return false;

		if (!StaticBatimentInfos::needPourcentageProduction(selectedBatiment->getID()))
			return false;

		// V�rifie que le b�timent s�lectionn� n'est pas en construction ou en destruction :
		// dans ce cas, l'affichage de la barre de pourcentage de production n'est pas n�cessaire
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
