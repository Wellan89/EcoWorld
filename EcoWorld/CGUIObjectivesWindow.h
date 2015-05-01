#ifndef DEF_C_GUI_OBJECTIVES_WINDOW
#define DEF_C_GUI_OBJECTIVES_WINDOW

#include "global.h"
#include "CGUICenteredTitleWindow.h"

using namespace gui;

class EcoWorldSystem;
class Objectives;
struct ObjectiveData;

// Fenêtre affichant un tableau des objectifs pour la partie actuelle
class CGUIObjectivesWindow : public CGUICenteredTitleWindow
{
public:
	// Constructeur
	CGUIObjectivesWindow(const EcoWorldSystem& m_system, IGUIEnvironment* environment, IGUIElement* parent, int id, const core::recti& rectangle);

	virtual bool OnEvent(const SEvent& event);

	virtual void OnWindowResize();

	// Met à jour ce tableau avec les informations du système
	// forceUpdate :	Si à true, la mise à jour se fera obligatoirement, sinon elle ne se fera que si au moins un jour supplémentaire s'est écoulé depuis sa dernière mise à jour
	void update(bool forceUpdate = false);

protected:
	// Ajoute une ligne dans le tableau pour un objectif
	void addObjectiveRow(const ObjectiveData& objective);

	// L'élément de la GUI affichant le tableau des objectifs
	IGUITable* table;

	// Le système de jeu
	const EcoWorldSystem& system;

	// Le jour du système à la date de la dernière mise à jour de cette fenêtre
	u32 lastSystemDay;
};

#endif
