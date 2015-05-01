#ifndef DEF_RESSOURCES_WINDOW
#define DEF_RESSOURCES_WINDOW

#include "global.h"
#include "Ressources.h"
#include "CGUICenteredTitleWindow.h"

using namespace gui;

class EcoWorldSystem;

class CGUIRessourcesWindow : public CGUICenteredTitleWindow
{
public:
	CGUIRessourcesWindow(const EcoWorldSystem& m_system, IGUIEnvironment* environment, IGUIElement* parent, int id, core::recti rectangle);

	virtual void OnWindowResize();

	virtual bool OnEvent(const SEvent& event);

	// Met à jour les ressources d'après le système de jeu
	// forceUpdate :	Si à true, la mise à jour se fera obligatoirement, sinon elle ne se fera que si au moins un jour supplémentaire s'est écoulé depuis sa dernière mise à jour
	void update(u32 textTransparency, bool forceUpdate = false);

protected:
	struct RessourceTab
	{
		IGUITab* tab;					// L'onglet de la GUI
		IGUIScrollBar* scrollBar;		// La scroll bar de l'onglet
		u32 elementsCount;				// Le nombre de textes dans cet onglet

		RessourceTab() : tab(0), scrollBar(0), elementsCount(0)	{ }
	};

	// Le système de jeu
	const EcoWorldSystem& system;

	// Le jour du système à la date de la dernière mise à jour de cette fenêtre
	u32 lastSystemDay;

	// Le groupe d'onglets
	IGUITabControl* tabGroup;

	// Les onglets pour trier les ressources
	RessourceTab materiauxTab;
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
	RessourceTab nourritureTab;
#ifdef KEEP_UNUSED_RESSOURCES
	RessourceTab vetementsTab;
#endif
	//RessourceTab arbresTab;	// Onglets désactivés : leurs ressources ont été supprimées
#else				// En mode enfant : on réactiver le papier comme ressource consommée par les habitants pour leur satisfaction
	RessourceTab diversTab;
#endif

	// La hauteur de chaque texte lorsque la scroll bar est à 0
	int ressourcesTextHeight[Ressources::RI_COUNT];

	// Les textes qui indiquent les noms des ressources dans le menu des ressources
	IGUIStaticText* ressourcesNameText[Ressources::RI_COUNT];

	// Les textes qui indiquent la quantité des ressources dans le menu des ressources
	IGUIStaticText* ressourcesQuantityText[Ressources::RI_COUNT];

	// Recalcule la position de chaque texte du menu suivant la position de la scroll bar
	void recalculateTextPos();

	// Retourne l'onglet actif
	RessourceTab* getActiveTab()
	{
		if (tabGroup)
		{
			switch (tabGroup->getActiveTab())
			{
			case 0: return &materiauxTab;
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
			case 1: return &nourritureTab;
#ifdef KEEP_UNUSED_RESSOURCES
			case 2: return &vetementsTab;
#endif
			//case 3: return &arbresTab;
			//case 4: return &diversTab;
#else				// En mode enfant : on réactiver le papier comme ressource consommée par les habitants pour leur satisfaction
			case 1: return &diversTab;
#endif
			}
		}
		LOG_DEBUG("CGUIRessourcesWindow::getActiveTab() : L'onglet actuel est inconnu : tabGroup = " << tabGroup << " ; tabGroup->getActiveTab() = " << (tabGroup ? core::stringc(tabGroup->getActiveTab()).c_str() : "???"), ELL_WARNING);
		return NULL;
	}
};

#endif
