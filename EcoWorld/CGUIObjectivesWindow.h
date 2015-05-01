#ifndef DEF_C_GUI_OBJECTIVES_WINDOW
#define DEF_C_GUI_OBJECTIVES_WINDOW

#include "global.h"
#include "CGUICenteredTitleWindow.h"

using namespace gui;

class EcoWorldSystem;
class Objectives;
struct ObjectiveData;

// Fen�tre affichant un tableau des objectifs pour la partie actuelle
class CGUIObjectivesWindow : public CGUICenteredTitleWindow
{
public:
	// Constructeur
	CGUIObjectivesWindow(const EcoWorldSystem& m_system, IGUIEnvironment* environment, IGUIElement* parent, int id, const core::recti& rectangle);

	virtual bool OnEvent(const SEvent& event);

	virtual void OnWindowResize();

	// Met � jour ce tableau avec les informations du syst�me
	// forceUpdate :	Si � true, la mise � jour se fera obligatoirement, sinon elle ne se fera que si au moins un jour suppl�mentaire s'est �coul� depuis sa derni�re mise � jour
	void update(bool forceUpdate = false);

protected:
	// Ajoute une ligne dans le tableau pour un objectif
	void addObjectiveRow(const ObjectiveData& objective);

	// L'�l�ment de la GUI affichant le tableau des objectifs
	IGUITable* table;

	// Le syst�me de jeu
	const EcoWorldSystem& system;

	// Le jour du syst�me � la date de la derni�re mise � jour de cette fen�tre
	u32 lastSystemDay;
};

#endif
