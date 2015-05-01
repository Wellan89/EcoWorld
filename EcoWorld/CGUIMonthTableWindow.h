#ifndef DEF_C_GUI_MONTH_TABLE_WINDOW
#define DEF_C_GUI_MONTH_TABLE_WINDOW

#include "global.h"
#include "CGUICenteredTitleWindow.h"

using namespace gui;

class CGUISortTable;
class EcoWorldSystem;
class Batiment;

// Fen�tre affichant un tableau r�capitulatif du mois actuel et des mois pr�c�dents : Tableau r�capitulatif des fins de mois
class CGUIMonthTableWindow : public CGUICenteredTitleWindow
{
public:
	// Constructeur et destructeur
	CGUIMonthTableWindow(const EcoWorldSystem& m_system, IGUIEnvironment* environment, IGUIElement* parent, int id, const core::recti& rectangle);
	virtual ~CGUIMonthTableWindow();

	virtual bool OnEvent(const SEvent& event);

	virtual void OnWindowResize();

	// Met � jour ce tableau avec les informations du syst�me
	// forceUpdate :	Si � true, la mise � jour se fera obligatoirement, sinon elle ne se fera que si au moins un jour suppl�mentaire s'est �coul� depuis sa derni�re mise � jour
	void update(bool forceUpdate = false);

protected:
	// Ajoute une ligne dans le tableau pour un b�timent sp�cifique
	void addBatimentRow(const Batiment* batiment);

	// Ajoute une ligne dans le tableau pour plusieurs b�timents du m�me type
	void addMultiBatimentRow(const core::list<const Batiment*>& batList);

	// L'�l�ment de la GUI affichant le tableau r�capitulatif de ce mois
	CGUISortTable* table;

	// Le syst�me de jeu
	const EcoWorldSystem& system;

	// Le jour du syst�me � la date de la derni�re mise � jour de cette fen�tre
	u32 lastSystemDay;
};

#endif
