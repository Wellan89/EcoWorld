#include "CGUIMenuWindow.h"
#include "CGUITexturedModalScreen.h"
#include "CGUIMessageBox.h"

CGUIMenuWindow::CGUIMenuWindow(IGUIEnvironment* environment, IGUIElement* parent, int id, core::recti rectangle, ITimer* timer)
 : CGUIWindow(environment, parent, id, rectangle), action(GMWA_nonChoisi), closeMenu(false), hasChangedPauseBeforeShowingMenuWindow(false), deviceTimer(timer), annulerBouton(NULL),
 creerNouvellePartieBouton(NULL), recommencerPartieBouton(NULL), chargerBouton(NULL), sauvegarderBouton(NULL), retourMenuPrincipalBouton(NULL),
 quitterBouton(NULL), modalScreen(NULL), askedAction(GMWA_nonChoisi), confirmationWindow(NULL)
{
#ifdef _DEBUG
	setDebugName("CGUIMenuWindow");
#endif

	setText(L"Menu");

	// On ajoute la taille de la barre de titre aux rectangles des boutons
	// car elle est prise en compte dans la surface de placement des boutons
	const core::vector2di titleBarSize(0, 20);

	const double ecartHaut = 0.05;
	const double ecartBoutons = 0.04;
	const double tailleBoutons = (1.0 - ecartHaut * 2.0 - ecartBoutons * 6.0) / 7.0;

	annulerBouton = environment->addButton(getAbsoluteRectangle(
		0.1, ecartHaut, 0.9, ecartHaut + tailleBoutons) + titleBarSize,
		this, -1, L"Retourner au jeu", L"Ferme cette boîte de dialogue");
	creerNouvellePartieBouton = environment->addButton(getAbsoluteRectangle(
		0.1, ecartHaut + ecartBoutons + tailleBoutons, 0.9, ecartHaut + ecartBoutons + tailleBoutons * 2) + titleBarSize,
		this, -1, L"Créer une nouvelle partie", L"Crée une nouvelle partie");
	recommencerPartieBouton = environment->addButton(getAbsoluteRectangle(
		0.1, ecartHaut + ecartBoutons * 2 + tailleBoutons * 2, 0.9, ecartHaut + ecartBoutons * 2 + tailleBoutons * 3) + titleBarSize,
		this, -1, L"Recommencer la partie", L"Recommence cette partie");
	chargerBouton = environment->addButton(getAbsoluteRectangle(
		0.1, ecartHaut + ecartBoutons * 3 + tailleBoutons * 3, 0.9, ecartHaut + ecartBoutons * 3 + tailleBoutons * 4) + titleBarSize,
		this, -1, L"Charger une partie", L"Charge une partie enregistrée");
	sauvegarderBouton = environment->addButton(getAbsoluteRectangle(
		0.1, ecartHaut + ecartBoutons * 4 + tailleBoutons * 4, 0.9, ecartHaut + ecartBoutons * 4 + tailleBoutons * 5) + titleBarSize,
		this, -1, L"Sauvegarder la partie", L"Sauvegarde la partie en cours");
	retourMenuPrincipalBouton = environment->addButton(getAbsoluteRectangle(
		0.1, ecartHaut + ecartBoutons * 5 + tailleBoutons * 5, 0.9, ecartHaut + ecartBoutons * 5 + tailleBoutons * 6) + titleBarSize,
		this, -1, L"Retourner au menu principal", L"Retourne au menu principal");
	quitterBouton = environment->addButton(getAbsoluteRectangle(
		0.1, ecartHaut + ecartBoutons * 6 + tailleBoutons * 6, 0.9, ecartHaut + ecartBoutons * 6 + tailleBoutons * 7) + titleBarSize,
		this, -1, L"Quitter", L"Quitte le jeu");

	modalScreen = new CGUITexturedModalScreen(parent, -1, environment, this);
	modalScreen->drop();
}
bool CGUIMenuWindow::OnEvent(const SEvent& event)
{
	if (IsVisible && isEnabled() && Parent)
	{
		if (event.EventType == EET_GUI_EVENT && event.GUIEvent.Caller)
		{
			if (event.GUIEvent.EventType == EGET_BUTTON_CLICKED)
			{
				if (event.GUIEvent.Caller == CloseButton || event.GUIEvent.Caller == annulerBouton)
				{
					// Ferme directement ce menu
					action = GMWA_annuler;
					closeMenu = true;

					// Lorsque c'est le bouton pour fermer la fenêtre qui a été cliqué, on retourne true pour annuler cet évènement et éviter que la fenêtre ne se ferme réellement
					if (event.GUIEvent.Caller == CloseButton)
						return true;
				}
				else if (event.GUIEvent.Caller == sauvegarderBouton)
				{
					// Sauvegarde directement la partie
					action = GMWA_sauvegarder;
					closeMenu = true;
				}
				else if (event.GUIEvent.Caller == creerNouvellePartieBouton && !confirmationWindow)
				{
#if 1
					// Demande une confirmation avant de revenir au menu de création de partie
					askUserConfirmation(GMWA_creerNouvellePartie,
						//L"Créer une nouvelle partie", L"Voulez-vous vraiment créer une nouvelle partie ?"
						L"Créer une nouvelle partie", L"Votre progression sera perdue. Continuer ?"
						);
#else
					// Reviens directement au menu de création de partie
					action = GMWA_creerNouvellePartie;
					closeMenu = true;
#endif
				}
				else if (event.GUIEvent.Caller == recommencerPartieBouton && !confirmationWindow)
				{
#if 1
					// Demande une confirmation avant de revenir au menu de création de partie
					askUserConfirmation(GMWA_recommencerPartie,
						//L"Recommencer la partie", L"Voulez-vous vraiment recommencer cette partie ?"
						L"Recommencer la partie", L"Votre progression sera perdue. Continuer ?"
						);
#else
					// Reviens directement au menu de création de partie
					action = GMWA_recommencerPartie;
					closeMenu = true;
#endif
				}
				else if (event.GUIEvent.Caller == chargerBouton && !confirmationWindow)
				{
#if 1
					// Demande une confirmation avant de charger une partie sauvegardée
					askUserConfirmation(GMWA_charger,
						//L"Charger une partie", L"Voulez-vous vraiment charger une partie sauvegardée ?"
						L"Charger une partie", L"Votre progression sera perdue. Continuer ?"
						);
#else
					// Charge directement une partie
					action = GMWA_charger;
					closeMenu = true;
#endif
				}
				else if (event.GUIEvent.Caller == retourMenuPrincipalBouton && !confirmationWindow)
				{
#if 1
					// Demande une confirmation avant de retourner au menu principal
					askUserConfirmation(GMWA_retourMenuPrincipal,
						//L"Retourner au menu principal", L"Voulez-vous vraiment retourner au menu principal ?"
						L"Retourner au menu principal", L"Votre progression sera perdue. Continuer ?"
						);
#else
					// Retourne au menu principal directement sans prévenir
					action = GMWA_retourMenuPrincipal;
					closeMenu = true;
#endif
				}
				else if (event.GUIEvent.Caller == quitterBouton && !confirmationWindow)
				{
#if 1
					// Demande une confirmation avant de quitter
					askUserConfirmation(GMWA_quitter,
						//L"Quitter", L"Voulez-vous vraiment quitter le jeu ?"
						L"Quitter", L"Votre progression sera perdue. Continuer ?"
						);
#else
					// Quitte directement sans prévenir
					action = GMWA_quitter;
					closeMenu = true;
#endif
				}
			}
			else if (event.GUIEvent.Caller == confirmationWindow)
			{
				if (event.GUIEvent.EventType == EGET_MESSAGEBOX_CANCEL || event.GUIEvent.EventType == EGET_MESSAGEBOX_NO
					|| event.GUIEvent.EventType == EGET_MESSAGEBOX_OK || event.GUIEvent.EventType == EGET_MESSAGEBOX_YES)
				{
					// Si l'utilisateur a cliqué sur un bouton (le bouton Oui est inclus) :
					// On indique que la fenêtre de confirmation est fermée
					confirmationWindow = NULL;

					if (event.GUIEvent.EventType != EGET_MESSAGEBOX_YES)
						askedAction = GMWA_nonChoisi;	// Evite que l'action demandée ne soit validée si l'utilisateur a refusé de continuer
														// Dans le cas contraire, l'action sera automatiquement validée au prochain appel de cette fonction

					return true;
				}
			}
		}
		else if (event.EventType == EET_KEY_INPUT_EVENT && !confirmationWindow)
		{
			// Si on appuie sur les touches Entrée/Echap, on simule un appui sur le bouton annuler/fermer
			if (event.KeyInput.PressedDown) // La touche a été pressée : on appuie sur le bouton
			{
				switch (event.KeyInput.Key)
				{
				case KEY_RETURN:
					if (annulerBouton)
						annulerBouton->setPressed(true);
					break;

				case KEY_ESCAPE:
					if (CloseButton->isVisible())
						CloseButton->setPressed(true);
					else if (annulerBouton)
						annulerBouton->setPressed(true);
					break;

				default: break;
				}
			}
			else // La touche a été relevée : on active l'action du bouton
			{
				switch (event.KeyInput.Key)
				{
				case KEY_RETURN:
				case KEY_ESCAPE:
					action = GMWA_annuler;
					closeMenu = true;
					break;

				default: break;
				}
			}
		}
	}

	return CGUIWindow::OnEvent(event);
}
void CGUIMenuWindow::OnPostRender(u32 timeMs)
{
	// Si une action est demandée et que la fenêtre de confirmation est fermée, on continue l'action demandée
	if (askedAction != GMWA_nonChoisi && !confirmationWindow)
	{
		action = askedAction;
		askedAction = GMWA_nonChoisi;
		closeMenu = true;
	}

	// Ferme ce menu si nécessaire
	if (closeMenu)
	{
		// Indique au parent de cette fenêtre qu'elle a été fermée
		SEvent sendEvent;
		sendEvent.EventType = EET_GUI_EVENT;
		sendEvent.GUIEvent.Caller = this;
		sendEvent.GUIEvent.Element = NULL;
		sendEvent.GUIEvent.EventType = EGET_ELEMENT_CLOSED; // Indique que cette fenêtre a été fermée

		Parent->OnEvent(sendEvent);

		// Masque cette fenêtre
		setVisible(false);

		// Evite que cette fenêtre ne se referme à la prochaine mise à jour
		closeMenu = false;

		// Remet l'état des boutons à leur état normal
		if (CloseButton)
			CloseButton->setPressed(false);
		if (annulerBouton)
			annulerBouton->setPressed(false);
		if (creerNouvellePartieBouton)
			creerNouvellePartieBouton->setPressed(false);
		if (chargerBouton)
			chargerBouton->setPressed(false);
		if (sauvegarderBouton)
			sauvegarderBouton->setPressed(false);
		if (retourMenuPrincipalBouton)
			retourMenuPrincipalBouton->setPressed(false);
		if (quitterBouton)
			quitterBouton->setPressed(false);

		// La fenêtre de confirmation est obligatoirement fermée pour qu'on ait pu arriver ici : on n'a donc pas besoin de la fermer ici
	}
}
void CGUIMenuWindow::askUserConfirmation(CGUIMenuWindow::GUI_MENU_WINDOW_ACTION m_action, const wchar_t* caption, const wchar_t* text)
{
	// Si la fenêtre de confirmation est déjà créée, on annule
	if (confirmationWindow)
		return;

	// Indique qu'aucune action n'est choisie pendant la création de la fenêtre de confirmation, pour éviter des event pendant que confirmationWindow est toujours NULL, et évite ainsi que l'action soit validée
	// Désactivé : Ne peut arriver en mode single thread !
	//askedAction = GMWA_nonChoisi;

	// Crée la fenêtre demandant une confirmation à l'utilisateur
	confirmationWindow = CGUIMessageBox::addMessageBox(Environment, caption, text, EMBF_YES | EMBF_NO, this, true);

	// Indique que la fenêtre demandant une confirmation à l'utilisateur est crée et indique l'action demandée
	askedAction = m_action;
}
void CGUIMenuWindow::draw()
{
	// Basé sur la fonction draw() de la classe CGUIWindow d'Irrlicht SVN 1.8.0-alpha :
	// Dessine le titre de cette fenêtre au centre de sa barre de titre

	if (IsVisible)
	{
		IGUISkin* const skin = Environment->getSkin();

		// update each time because the skin is allowed to change this always.
		updateClientRect();

		if ( CurrentIconColor != skin->getColor(isEnabled() ? EGDC_WINDOW_SYMBOL : EGDC_GRAY_WINDOW_SYMBOL) )
			refreshSprites();

		core::recti rect = AbsoluteRect;

		// draw body fast
		if (DrawBackground)
		{
			rect = skin->draw3DWindowBackground(this, DrawTitlebar,
					skin->getColor(IsActive ? EGDC_ACTIVE_BORDER : EGDC_INACTIVE_BORDER),
					AbsoluteRect, &AbsoluteClippingRect);

			if (DrawTitlebar && Text.size())
			{
				rect.UpperLeftCorner.X += skin->getSize(EGDS_TITLEBARTEXT_DISTANCE_X);
				rect.UpperLeftCorner.Y += skin->getSize(EGDS_TITLEBARTEXT_DISTANCE_Y);
				rect.LowerRightCorner.X -= skin->getSize(EGDS_WINDOW_BUTTON_WIDTH) + 5;

				IGUIFont* const font = skin->getFont(EGDF_WINDOW);
				if (font)
				{
					font->draw(Text.c_str(), rect,
							skin->getColor(IsActive ? EGDC_ACTIVE_CAPTION:EGDC_INACTIVE_CAPTION),
							true, true, &AbsoluteClippingRect);
				}
			}
		}
	}

	IGUIElement::draw();
}
