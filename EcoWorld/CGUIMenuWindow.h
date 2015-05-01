#ifndef DEF_MENU_WINDOW
#define DEF_MENU_WINDOW

#include "global.h"
#include "CGUIWindow.h"
#include "CGUITexturedModalScreen.h"

using namespace gui;

// Fen�tre affich�e lorsque l'utilisateur appuie sur Echap pendant le jeu (menu du jeu)
class CGUIMenuWindow : public CGUIWindow
{
public:
	// L'action choisie par l'utilisateur
	enum GUI_MENU_WINDOW_ACTION
	{
		GMWA_nonChoisi,				// L'utilisateur n'a pas encore choisi d'action
		GMWA_annuler,				// L'utilisateur veut fermer cette bo�te de dialogue
		GMWA_creerNouvellePartie,	// L'utilisateur veut cr�er une nouvelle partie
		GMWA_recommencerPartie,		// L'utilisateur veut recommencer cette partie
		GMWA_charger,				// L'utilisateur veut charger une partie
		GMWA_sauvegarder,			// L'utilisateur veut sauvegarder la partie
		GMWA_retourMenuPrincipal,	// L'utilisateur veut revenir au menu principal
		GMWA_quitter				// L'utilisateur veut quitter le jeu
	};

	CGUIMenuWindow(IGUIEnvironment* environment, IGUIElement* parent, int id, core::recti rectangle, ITimer* timer);

	virtual bool OnEvent(const SEvent& event);

	// Fonction draw surcharg�e pour dessiner le titre de cette fen�tre au centre de sa barre de titre
	virtual void draw();

	virtual void OnPostRender(u32 timeMs);

protected:
	ITimer* deviceTimer;					// Le timer du jeu

	bool closeMenu;							// True si ce menu doit �tre ferm� au prochain appel de OnPostRender

	bool hasChangedPauseBeforeShowingMenuWindow;	// True si la pause a �t� chang�e avant l'affichage de ce menu

	IGUIButton* annulerBouton;				// Le bouton de la fen�tre du menu du jeu pour annuler et quitter cette bo�te de dialogue
	IGUIButton* creerNouvellePartieBouton;	// Le bouton de la fen�tre du menu du jeu pour cr�er une nouvelle partie
	IGUIButton* recommencerPartieBouton;	// Le bouton de la fen�tre du menu du jeu pour recommencer la partie en cours
	IGUIButton* chargerBouton;				// Le bouton de la fen�tre du menu du jeu pour charger une partie
	IGUIButton* sauvegarderBouton;			// Le bouton de la fen�tre du menu du jeu pour sauvegarder la partie
	IGUIButton* retourMenuPrincipalBouton;	// Le bouton de la fen�tre du menu du jeu pour revenir au menu principal
	IGUIButton* quitterBouton;				// Le bouton de la fen�tre du menu du jeu pour quitter le jeu
	CGUITexturedModalScreen* modalScreen;	// Le modal screen qui emp�che les autres �l�ments de la GUI d'avoir acc�s aux events, et qui s'affiche avec une texture de fond

	GUI_MENU_WINDOW_ACTION askedAction;		// L'action actuellement demand�e par le joueur, lors de l'affichage de la fen�tre de confirmation
	IGUIWindow* confirmationWindow;			// La bo�te de dialogue de confirmation demandant si le joueur est s�r de vouloir continuer l'action demand�e si elle entra�ne une perte de la partie en cours

	GUI_MENU_WINDOW_ACTION action;	// L'action choisie par l'utilisateur (peut �tre remis � z�ro avec setHasChangedPause())

	// Demande une confirmation � l'utilisateur pour continuer l'action en cours
	void askUserConfirmation(CGUIMenuWindow::GUI_MENU_WINDOW_ACTION m_action, const wchar_t* caption, const wchar_t* text);

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

public:
	//! Sets the visible state of this element.
	virtual void setVisible(bool visible)
	{
		CGUIWindow::setVisible(visible);

		if (visible)
		{
			// Place cette fen�tre et son modal screen au premier plan
			if (modalScreen && modalScreen->getParent())
				modalScreen->getParent()->bringToFront(modalScreen);
			if (Parent)
				Parent->bringToFront(this);

			// Donne le focus � cette fen�tre
			Environment->setFocus(this);
		}
	}

	// Accesseurs inline :

	// Retourne/Indique le choix effectu� par l'utilisateur
	GUI_MENU_WINDOW_ACTION getUserAction() const { return action; }
	void setUserAction(GUI_MENU_WINDOW_ACTION act) { action = act; }

	// Retourne/Indique si l'�tat de la pause a �t� chang� avant l'affichage de ce menu
	bool getHasChangedPause() const { return hasChangedPauseBeforeShowingMenuWindow; }
	void setHasChangedPause(bool hasChangedPause) { hasChangedPauseBeforeShowingMenuWindow = hasChangedPause; }
};

#endif
