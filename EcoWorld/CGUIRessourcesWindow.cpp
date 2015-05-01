#include "CGUIRessourcesWindow.h"
#include "EcoWorldSystem.h"

CGUIRessourcesWindow::CGUIRessourcesWindow(const EcoWorldSystem& m_system, IGUIEnvironment *environment, IGUIElement *parent, int id, core::recti rectangle)
 : CGUICenteredTitleWindow(environment, parent, id, rectangle), tabGroup(0), system(m_system), lastSystemDay(0)
{
#ifdef _DEBUG
	setDebugName("CGUIRessourcesWindow");
#endif

	setText(L"Vue générale des ressources");

	// Indique la taille minimale de la fenêtre
	// TODO : Revoir ces valeurs
	const float facteur = environment->getVideoDriver()->getScreenSize().Width * 0.0014f;
	setMinSize(core::dimension2du(core::floor32(400 * facteur), core::floor32(200 * facteur)));

	// Variables temporaires
	RessourceTab* currentTab = NULL;	// Un pointeur temporaire sur l'un des onglets
	int i;								// Un entier temporaire qui servira d'itérateur

	for (i = 0; i < Ressources::RI_COUNT; ++i)
	{
		ressourcesTextHeight[i] = 0;
		ressourcesNameText[i] = NULL;
		ressourcesQuantityText[i] = NULL;
	}

	// La hauteur des zones de texte
	const int textHeight = environment->getSkin()->getFont()->getDimension(L"A").Height + 1;

	// Crée les onglets pour trier les ressources
	tabGroup = environment->addTabControl(core::recti(10, ClientRect.UpperLeftCorner.Y + 10, ClientRect.LowerRightCorner.X - 15, ClientRect.LowerRightCorner.Y - 15), this);
	materiauxTab.tab = tabGroup->addTab(L"Matériaux de construction");
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
#ifdef KEEP_UNUSED_RESSOURCES
	nourritureTab.tab = tabGroup->addTab(L"Nourriture");
	vetementsTab.tab = tabGroup->addTab(L"Vêtements");
#else
	nourritureTab.tab = tabGroup->addTab(L"Produits de consommation");
#endif
	//arbresTab.tab = tabGroup->addTab(L"Arbres et plantes");
	//diversTab.tab = tabGroup->addTab(L"Divers");
#else				// En mode enfant : on réactiver le papier comme ressource consommée par les habitants pour leur satisfaction
	diversTab.tab = tabGroup->addTab(L"Consommables");
#endif

	// Règle la hauteur et la largeur des onglets du tabGroup
	// TODO : Régler précisément les tailles des onglets en largeur
	tabGroup->setTabExtraWidth((int)(0.1f * (float)(ClientRect.LowerRightCorner.X - ClientRect.UpperLeftCorner.X)));	// Valeur par défaut : 20
	//tabGroup->setTabHeight((int)((float)textHeight * 1.4f));															// Valeur par défaut : 32
	tabGroup->setTabHeight(max(																							// Valeur par défaut : 32
		(int)((float)(ClientRect.LowerRightCorner.Y - ClientRect.UpperLeftCorner.Y) * 0.06f),
		(int)((float)textHeight * 1.2f)));



	// Crée les textes pour les ressources
	currentTab = NULL;
	for (i = 0; i < Ressources::RI_COUNT; ++i)
	{
		currentTab = NULL;
		const Ressources::RessourceGroup group = Ressources::getRessourceGroup((Ressources::RessourceID)i);
		switch (group)
		{
		case Ressources::RG_materiaux:
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
		case Ressources::RG_semiMateriaux:
#endif
			currentTab = &materiauxTab;
			break;

#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
		case Ressources::RG_nourriture:
		case Ressources::RG_semiNourriture:
#ifndef KEEP_UNUSED_RESSOURCES
		case Ressources::RG_vetements:
#endif
			currentTab = &nourritureTab;
			break;

		//case Ressources::RG_arbres:
		//	currentTab = &arbresTab;
		//	break;

#ifdef KEEP_UNUSED_RESSOURCES
		case Ressources::RG_vetements:
		case Ressources::RG_semiVetements:
			currentTab = &vetementsTab;
			break;
#endif

		//case Ressources::RG_locomotion:
		//case Ressources::RG_divers:
		//	currentTab = &diversTab;
		//	break;
#else				// En mode enfant : on réactiver le papier comme ressource consommée par les habitants pour leur satisfaction
		case Ressources::RG_outilsEcole:
			currentTab = &diversTab;
			break;
#endif

		default:
			LOG_DEBUG("CGUIRessourcesWindow::CGUIRessourcesWindow() : Groupe de ressource inconnu : group = " << group, ELL_WARNING);

#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
			//currentTab = &diversTab;
			currentTab = &nourritureTab;
#else				// En mode enfant : on réactiver le papier comme ressource consommée par les habitants pour leur satisfaction
			currentTab = &diversTab;
#endif
			break;
		};

		if (currentTab)
		{
			// Calule le décalage en x suivant la valeur de j (x varie maintenant entre 0.0f et 0.64f, au lieu de 0.0f et 0.66...6f)
			const float x = (float)(currentTab->elementsCount % 3) * 0.32f;	// (float)(currentTab->elementsCount % 3) / 3.0f
			const core::rectf rect = core::rectf(
				(float)(ClientRect.UpperLeftCorner.X + 5),
				(float)(ClientRect.UpperLeftCorner.Y + 5),
				(float)(ClientRect.LowerRightCorner.X - 15),
				(float)(ClientRect.LowerRightCorner.Y - 15));

			core::stringw itemText(Ressources::getRessourceName((Ressources::RessourceID)i));
			itemText.append(L" :");

			ressourcesTextHeight[i] = textHeight * (currentTab->elementsCount / 3);

			// Schéma de l'organisation des textes des ressources en X (en valeurs relatives, arrondies au centième) :
			// 0.02 <-> 0.16 ; 0.16 <-> 0.31 | 0.34 <-> 0.48 ; 0.48 <-> 0.63 | 0.66 <-> 0.80 ; 0.80 <-> 0.95

			// Crée le texte pour le nom de la ressource
			ressourcesNameText[i] = environment->addStaticText(itemText.c_str(), core::recti(
					(int)((0.02f + x) * rect.LowerRightCorner.X), ressourcesTextHeight[i],
					(int)((0.16f + x) * rect.LowerRightCorner.X), ressourcesTextHeight[i] + textHeight),
				false, false, currentTab->tab, -1, false);
			ressourcesNameText[i]->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_UPPERLEFT); // Indique l'alignement du texte

			// Crée le texte pour la quantité de la ressource
			ressourcesQuantityText[i] = environment->addStaticText(L"0", core::recti(
					(int)((0.16f + x) * rect.LowerRightCorner.X), ressourcesTextHeight[i],
					(int)((0.31f + x) * rect.LowerRightCorner.X), ressourcesTextHeight[i] + textHeight),
				false, false, currentTab->tab, -1, false);
			ressourcesQuantityText[i]->setTextAlignment(EGUIA_LOWERRIGHT, EGUIA_UPPERLEFT); // Indique l'alignement du texte

			++currentTab->elementsCount;
		}
#ifdef _DEBUG
		else
			LOG_DEBUG("CGUIRessourcesWindow::CGUIRessourcesWindow() : L'onglet actuel est NULL : currentTab = " << currentTab << " ; ressourceID = i = " << i, ELL_WARNING);
#endif
	}

	// Crée les scrolls bars
	int scrollBarMax = 0;
	currentTab = NULL;
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
#ifdef KEEP_UNUSED_RESSOURCES
	for (i = 0; i < 3; ++i)	// i < 5
#else
	for (i = 0; i < 2; ++i)	// i < 5
#endif
#else
	for (i = 0; i < 2; ++i)	// i < 5
#endif
	{
		switch (i)
		{
		case 0:	currentTab = &materiauxTab;		break;
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
		case 1:	currentTab = &nourritureTab;	break;
#ifdef KEEP_UNUSED_RESSOURCES
		case 2:	currentTab = &vetementsTab;		break;
#endif
		//case 3:	currentTab = &arbresTab;		break;
#else				// En mode enfant : on réactiver le papier comme ressource consommée par les habitants pour leur satisfaction
		case 4:	currentTab = &diversTab;		break;
#endif
		}
		if (!currentTab)
		{
			LOG_DEBUG("CGUIRessourcesWindow::CGUIRessourcesWindow() : L'onglet actuel est NULL : currentTab = " << currentTab << " ; i = " << i, ELL_WARNING);
			continue;
		}

		// Calcule la valeur maximale de la scroll bar
		scrollBarMax = textHeight * ((currentTab->elementsCount / 3) + 6) - ClientRect.LowerRightCorner.Y; // textHeight * ((currentTab->elementsCount / 3) + 7) - ClientRect.LowerRightCorner.Y

		// Crée la scroll bar
		currentTab->scrollBar = environment->addScrollBar(false,
			getAbsoluteRectangle(0.97f, 0.02f, 0.99f, 0.98f, currentTab->tab->getAbsoluteClippingRect()), currentTab->tab, -1);
		currentTab->scrollBar->setSubElement(true);
		currentTab->scrollBar->setTabStop(false);

		// Indique le comportement de la scroll bar si la fenêtre est redimensionnée
		currentTab->scrollBar->setAlignment(EGUIA_LOWERRIGHT, EGUIA_LOWERRIGHT, EGUIA_UPPERLEFT, EGUIA_LOWERRIGHT);

		// Indique le minimum, maximum et les valeurs de déplacement de la scroll bar
		currentTab->scrollBar->setMin(0);
		currentTab->scrollBar->setMax(scrollBarMax > 0 ? scrollBarMax : 0);
		currentTab->scrollBar->setPos(0);
		currentTab->scrollBar->setSmallStep(textHeight);
		currentTab->scrollBar->setLargeStep(textHeight * 5);

		// Vérifie que la scroll bar est utile
		if (scrollBarMax > 0)
			currentTab->scrollBar->setVisible(true);
		else
			currentTab->scrollBar->setVisible(false);
	}

	// Recalcule la position des textes du menu
	// Désactivé : Inutile puisque les textes de ce menu viennent d'être créés (serait utile seulement si la valeur de la scroll bar de défilement avait été modifiée (différente de 0))
	//recalculateTextPos();
}
void CGUIRessourcesWindow::OnWindowResize()
{
	// Variables temporaires
	RessourceTab* currentTab = NULL;	// Un pointeur temporaire sur l'un des onglets
	int i;								// Un entier temporaire qui servira d'itérateur

	// La hauteur des zones de texte
	const int textHeight = Environment->getSkin()->getFont()->getDimension(L"A").Height + 1;

	// Replace le groupe d'onglets
	tabGroup->setRelativePosition(core::recti(10, ClientRect.UpperLeftCorner.Y + 10, ClientRect.LowerRightCorner.X - 15, ClientRect.LowerRightCorner.Y - 15));

	// Règle la hauteur et la largeur des onglets du tabGroup
	// TODO : Régler précisément les tailles des onglets en largeur
	tabGroup->setTabExtraWidth((int)(0.1f * (float)(ClientRect.LowerRightCorner.X - ClientRect.UpperLeftCorner.X)));	// Valeur par défaut : 20
	//tabGroup->setTabHeight((int)((float)textHeight * 1.4f));															// Valeur par défaut : 32
	tabGroup->setTabHeight(max(																							// Valeur par défaut : 32
		(int)((float)(ClientRect.LowerRightCorner.Y - ClientRect.UpperLeftCorner.Y) * 0.06f),
		(int)((float)textHeight * 1.2f)));



	// Réinitialise le nombre d'éléments dans chaque onglet, car il sera recompté et utilisé ici pour connaître le nombre d'éléments déplacés de chaque onglet
	materiauxTab.elementsCount = 0;
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
	nourritureTab.elementsCount = 0;
#ifdef KEEP_UNUSED_RESSOURCES
	vetementsTab.elementsCount = 0;
#endif
	//arbresTab.elementsCount = 0;
#else				// En mode enfant : on réactiver le papier comme ressource consommée par les habitants pour leur satisfaction
	diversTab.elementsCount = 0;
#endif

	// Replace les textes pour les ressources
	for (i = 0; i < Ressources::RI_COUNT; ++i)
	{
		currentTab = NULL;
		const Ressources::RessourceGroup group = Ressources::getRessourceGroup((Ressources::RessourceID)i);
		switch (group)
		{
		case Ressources::RG_materiaux:
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
		case Ressources::RG_semiMateriaux:
#endif
			currentTab = &materiauxTab;
			break;

#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
		case Ressources::RG_nourriture:
		case Ressources::RG_semiNourriture:
#ifndef KEEP_UNUSED_RESSOURCES
		case Ressources::RG_vetements:
#endif
			currentTab = &nourritureTab;
			break;

		//case Ressources::RG_arbres:
		//	currentTab = &arbresTab;
		//	break;

#ifdef KEEP_UNUSED_RESSOURCES
		case Ressources::RG_vetements:
		case Ressources::RG_semiVetements:
			currentTab = &vetementsTab;
			break;
#endif

		//case Ressources::RG_locomotion:
		//case Ressources::RG_divers:
		//	currentTab = &diversTab;
		//	break;
#else				// En mode enfant : on réactiver le papier comme ressource consommée par les habitants pour leur satisfaction
		case Ressources::RG_outilsEcole:
			currentTab = &diversTab;
			break;
#endif

		default:
			LOG_DEBUG("CGUIRessourcesWindow::OnWindowResize() : Groupe de ressource inconnu : group = " << group, ELL_WARNING);

#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
			//currentTab = &diversTab;
			currentTab = &nourritureTab;
#else				// En mode enfant : on réactiver le papier comme ressource consommée par les habitants pour leur satisfaction
			currentTab = &diversTab;
#endif
			break;
		};

		if (currentTab)
		{
			// Calule le décalage en x suivant la valeur de j (x varie maintenant entre 0.0f et 0.64f, au lieu de 0.0f et 0.66...6f)
			const float x = (float)(currentTab->elementsCount % 3) * 0.32f;	// (float)(currentTab->elementsCount % 3) / 3.0f
			const core::rectf rect = core::rectf(
				(float)(ClientRect.UpperLeftCorner.X + 5),
				(float)(ClientRect.UpperLeftCorner.Y + 5),
				(float)(ClientRect.LowerRightCorner.X - 15),
				(float)(ClientRect.LowerRightCorner.Y - 15));

			core::stringw itemText(Ressources::getRessourceName((Ressources::RessourceID)i));
			itemText.append(L" :");

			ressourcesTextHeight[i] = textHeight * (currentTab->elementsCount / 3);

			// Schéma de l'organisation des textes des ressources en X (en valeurs relatives, arrondies au centième) :
			// 0.02 <-> 0.16 ; 0.17 <-> 0.31 | 0.34 <-> 0.48 ; 0.49 <-> 0.63 | 0.66 <-> 0.80 ; 0.81 <-> 0.95

			// Replace le texte pour le nom de la ressource
			ressourcesNameText[i]->setRelativePosition(core::recti(
					(int)((0.02f + x) * rect.LowerRightCorner.X), ressourcesTextHeight[i],
					(int)((0.16f + x) * rect.LowerRightCorner.X), ressourcesTextHeight[i] + textHeight));

			// Replace le texte pour la quantité de la ressource
			ressourcesQuantityText[i]->setRelativePosition(core::recti(
					(int)((0.16f + x) * rect.LowerRightCorner.X), ressourcesTextHeight[i],
					(int)((0.31f + x) * rect.LowerRightCorner.X), ressourcesTextHeight[i] + textHeight));

			++currentTab->elementsCount;
		}
#ifdef _DEBUG
		else
			LOG_DEBUG("CGUIRessourcesWindow::OnWindowResize() : L'onglet actuel est NULL : currentTab = " << currentTab << " ; ressourceID = i = " << i, ELL_WARNING);
#endif
	}

	// Replace les scrolls bars
	int scrollBarMax = 0;
	currentTab = NULL;
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
#ifdef KEEP_UNUSED_RESSOURCES
	for (i = 0; i < 3; ++i)	// i < 5
#else
	for (i = 0; i < 2; ++i)	// i < 5
#endif
#else
	for (i = 0; i < 2; ++i)	// i < 5
#endif
	{
		switch (i)
		{
		case 0:	currentTab = &materiauxTab;		break;
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
		case 1:	currentTab = &nourritureTab;	break;
#ifdef KEEP_UNUSED_RESSOURCES
		case 2:	currentTab = &vetementsTab;		break;
#endif
		//case 3:	currentTab = &arbresTab;		break;
#else				// En mode enfant : on réactiver le papier comme ressource consommée par les habitants pour leur satisfaction
		case 4:	currentTab = &diversTab;		break;
#endif
		}
		if (!currentTab)
		{
			LOG_DEBUG("CGUIRessourcesWindow::OnWindowResize() : L'onglet actuel est NULL : currentTab = " << currentTab << " ; i = " << i, ELL_WARNING);
			continue;
		}

		// Calcule la valeur maximale de la scroll bar
		scrollBarMax = textHeight * ((currentTab->elementsCount / 3) + 6) - ClientRect.LowerRightCorner.Y;

		// Replace la scroll bar
		currentTab->scrollBar->setRelativePosition(getAbsoluteRectangle(0.97f, 0.02f, 0.99f, 0.98f, currentTab->tab->getAbsoluteClippingRect()));

		// Indique le minimum, maximum et les valeurs de déplacement de la scroll bar
		currentTab->scrollBar->setMin(0);
		currentTab->scrollBar->setMax(scrollBarMax > 0 ? scrollBarMax : 0);
		currentTab->scrollBar->setPos(0);
		currentTab->scrollBar->setSmallStep(textHeight);
		currentTab->scrollBar->setLargeStep(textHeight * 5);

		// Vérifie que la scroll bar est utile
		if (scrollBarMax > 0)
			currentTab->scrollBar->setVisible(true);
		else
			currentTab->scrollBar->setVisible(false);
	}

	CGUICenteredTitleWindow::OnWindowResize();
}
bool CGUIRessourcesWindow::OnEvent(const SEvent &event)
{
	bool closeMenu = false;

	if (isEnabled() && Parent)
	{
		if (event.EventType == EET_KEY_INPUT_EVENT)
		{
			if (event.KeyInput.PressedDown)	// La touche a été pressée : on appuie sur le bouton
			{
				switch (event.KeyInput.Key)
				{
				case KEY_RETURN:
				case KEY_ESCAPE:
					if (CloseButton->isVisible())
						CloseButton->setPressed(true);
					break;

				default: break;
				}
			}
			else	// La touche a été relevée : on active l'action du bouton
			{
				switch (event.KeyInput.Key)
				{
				case KEY_RETURN:
				case KEY_ESCAPE:
					closeMenu = true;
					break;

				default:	break;
				}
			}
		}
		else if (event.EventType == EET_GUI_EVENT)
		{
			if (event.GUIEvent.EventType == EGET_BUTTON_CLICKED && event.GUIEvent.Caller == CloseButton)
				closeMenu = true;

			// La position d'une scroll bar a changé : on recalcule la position des textes du menu
			else if (event.GUIEvent.EventType == EGET_SCROLL_BAR_CHANGED)
				recalculateTextPos();

			// On a changé d'onglet : on recalcule la position des textes du menu suivant la scroll bar de l'onglet actif
			else if (event.GUIEvent.EventType == EGET_TAB_CHANGED && event.GUIEvent.Caller == tabGroup)
				recalculateTextPos();
		}
	}

	// Ferme ce menu
	if (closeMenu && Parent)
	{
		SEvent sendEvent;
		sendEvent.EventType = EET_GUI_EVENT;
		sendEvent.GUIEvent.Caller = this;
		sendEvent.GUIEvent.Element = NULL;
		sendEvent.GUIEvent.EventType = EGET_ELEMENT_CLOSED; // Indique que cette fenêtre a été fermée

		Parent->OnEvent(sendEvent);

		return true;
	}

	return CGUICenteredTitleWindow::OnEvent(event);
}
void CGUIRessourcesWindow::recalculateTextPos()
{
	// La hauteur des zones de texte
	const int textHeight = Environment->getSkin()->getFont()->getDimension(L"A").Height + 1;

	const RessourceTab* const activeTab = getActiveTab();
	if (!activeTab)
		return;
	const int scrollBarPos = (activeTab->scrollBar ? (activeTab->scrollBar->isVisible() ? activeTab->scrollBar->getPos() : 0) : 0);

	for (int i = 0; i < Ressources::RI_COUNT; ++i)
	{
		// Vérifie que cette ressource a bien été associée à l'onglet actif
		if (ressourcesNameText[i]->getParent() == activeTab->tab)
		{
			// Replace le texte du nom de la ressource
			core::recti pos = ressourcesNameText[i]->getRelativePosition();
			ressourcesNameText[i]->setRelativePosition(core::recti(
				pos.UpperLeftCorner.X, ressourcesTextHeight[i] - scrollBarPos,
				pos.LowerRightCorner.X, ressourcesTextHeight[i] + textHeight - scrollBarPos));

			// Replace le texte de la quantité de la ressource
			pos = ressourcesQuantityText[i]->getRelativePosition();
			ressourcesQuantityText[i]->setRelativePosition(core::recti(
				pos.UpperLeftCorner.X, ressourcesTextHeight[i] - scrollBarPos,
				pos.LowerRightCorner.X, ressourcesTextHeight[i] + textHeight - scrollBarPos));	
		}
	}
}
void CGUIRessourcesWindow::update(u32 textTransparency, bool forceUpdate)
{
	// Vérifie que le système a été remis à jour depuis le dernier appel à cette fonction
	if (lastSystemDay == system.getTime().getTotalJours() && !forceUpdate)
		return;
	lastSystemDay = system.getTime().getTotalJours();

	// Obtient les couleurs pour les textes des ressources
	const video::SColor defaultColor = Environment->getSkin() ? Environment->getSkin()->getColor(EGDC_BUTTON_TEXT) : video::SColor(textTransparency, 0, 0, 0);
	const video::SColor greenColor(textTransparency, defaultColor.getRed(), 150, defaultColor.getBlue());
	const video::SColor redColor(textTransparency, 200, defaultColor.getGreen(), defaultColor.getBlue());

	// Pour toutes les ressources :
	for (u32 i = 0; i < Ressources::RI_COUNT; ++i)
	{
		// Actualise les quantités des ressources
		if (ressourcesQuantityText[i])
		{
			// N'affiche plus les quantités de ressources en tonnes (10^3), mais bien en kg (10^0)
			//swprintf_SS(L"%d ", core::floor32(system.getInfos().ressources[i] * 0.001f));
			swprintf_SS(L"%d ", core::floor32(system.getInfos().ressources[i]));

			core::stringw ressourceText(textW_SS);
			ressourceText.append(Ressources::getRessourceUnit((Ressources::RessourceID)i));

			ressourcesQuantityText[i]->setText(ressourceText.c_str());
		}

		// Met à jour la couleur du texte suivant l'évolution actuelle de la ressource
		if (ressourcesNameText[i] || ressourcesQuantityText[i])
		{
			const float ressourceProduction = system.getRessourcesProduction((Ressources::RessourceID)i);
			if (ressourceProduction == 0.0f)		// Aucune évolution de la ressource (couleur par défaut : noir)
			{
				if (ressourcesNameText[i])
					ressourcesNameText[i]->enableOverrideColor(false);
				if (ressourcesQuantityText[i])
					ressourcesQuantityText[i]->enableOverrideColor(false);
			}
			else if (ressourceProduction > 0.0f)	// Augmentation de la ressource (couleur : vert)
			{
				if (ressourcesNameText[i])
					ressourcesNameText[i]->setOverrideColor(greenColor);
				if (ressourcesQuantityText[i])
					ressourcesQuantityText[i]->setOverrideColor(greenColor);
			}
			else									// Diminution de la ressource (couleur : rouge)
			{
				if (ressourcesNameText[i])
					ressourcesNameText[i]->setOverrideColor(redColor);
				if (ressourcesQuantityText[i])
					ressourcesQuantityText[i]->setOverrideColor(redColor);
			}
		}
	}
}
