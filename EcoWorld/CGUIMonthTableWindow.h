#ifndef DEF_C_GUI_MONTH_TABLE_WINDOW
#define DEF_C_GUI_MONTH_TABLE_WINDOW

#include "global.h"
#include "CGUICenteredTitleWindow.h"

using namespace gui;

class CGUISortTable;
class EcoWorldSystem;
class Batiment;

// Fenêtre affichant un tableau récapitulatif du mois actuel et des mois précédents : Tableau récapitulatif des fins de mois
class CGUIMonthTableWindow : public CGUICenteredTitleWindow
{
public:
	// Constructeur et destructeur
	CGUIMonthTableWindow(const EcoWorldSystem& m_system, IGUIEnvironment* environment, IGUIElement* parent, int id, const core::recti& rectangle);
	virtual ~CGUIMonthTableWindow();

	virtual bool OnEvent(const SEvent& event);

	virtual void OnWindowResize();

	// Met à jour ce tableau avec les informations du système
	// forceUpdate :	Si à true, la mise à jour se fera obligatoirement, sinon elle ne se fera que si au moins un jour supplémentaire s'est écoulé depuis sa dernière mise à jour
	void update(bool forceUpdate = false);

protected:
	// Ajoute une ligne dans le tableau pour un bâtiment spécifique
	void addBatimentRow(const Batiment* batiment);

	// Ajoute une ligne dans le tableau pour plusieurs bâtiments du même type
	void addMultiBatimentRow(const core::list<const Batiment*>& batList);

	// L'élément de la GUI affichant le tableau récapitulatif de ce mois
	CGUISortTable* table;

	// Le système de jeu
	const EcoWorldSystem& system;

	// Le jour du système à la date de la dernière mise à jour de cette fenêtre
	u32 lastSystemDay;
};

#endif
