#include "CGUIInformationsWindow.h"
#include "EcoWorldSystem.h"
#include "Game.h"
#ifdef USE_RAKNET
#include "RakNetManager.h"
#endif

CGUIInformationsWindow::CGUIInformationsWindow(EcoWorldSystem& m_system, IGUIEnvironment* environment, IGUIElement* parent, int id, const core::recti& rectangle)
 : CGUIReductableWindow(environment, parent, id, rectangle), system(m_system), textBox(0), lastInfosType(EIT_CONSTRUCTION), lastBatimentInfos(BI_aucun),
 lastBatDaysPassed(0), selectedBatiment(0), pourcentageProductionText(0), pourcentageProductionScrollBar(0), lastMultiBatCount(0)
{
#ifdef _DEBUG
	setDebugName("CGUIInformationsWindow");
#endif

	setText(L"Informations");

	// Masque le bouton pour fermer la fen�tre
	if (CloseButton)
		CloseButton->setVisible(false);

	// Indique la taille minimale de cette fen�tre
	setMinSize(core::dimension2du(175, 200));

	// On ajoute la taille de la barre de titre aux rectangles des boutons et des textes
	// car elle est prise en compte dans la surface de placement des boutons
	const int titleBarSizeY = 19;

	// Cr�e le texte d'informations
	textBox = new CGUITextBox(environment->getSkin()->getFont(), L"", environment,
		core::recti(10, 10 + titleBarSizeY, ClientRect.LowerRightCorner.X - 5, ClientRect.LowerRightCorner.Y - 15), this, -1);
	textBox->setScrollbarRight(true);	// Scroll bar � droite
	textBox->setScrollModeLines(false);	// D�filement par pixels (et non par lignes)
	textBox->drop();

	// Cr�e le texte au-dessus de la scroll bar du pourcentage de production
	pourcentageProductionText = environment->addStaticText(L"Pourcentage de production :", core::recti(
			ClientRect.UpperLeftCorner.X + 5, ClientRect.LowerRightCorner.Y - 55,
			ClientRect.LowerRightCorner.X - 20, ClientRect.LowerRightCorner.Y - 30),
		false, true, this);
	pourcentageProductionText->setVisible(false);
	pourcentageProductionText->setTextAlignment(gui::EGUIA_UPPERLEFT, gui::EGUIA_UPPERLEFT);

	// Cr�e la scroll bar du pourcentage de production
	pourcentageProductionScrollBar = environment->addScrollBar(true, core::recti(
		ClientRect.UpperLeftCorner.X + 5, ClientRect.LowerRightCorner.Y - 30,
		ClientRect.LowerRightCorner.X - 20, ClientRect.LowerRightCorner.Y - 10), this);
	pourcentageProductionScrollBar->setVisible(false);
	pourcentageProductionScrollBar->setMin(0);
	pourcentageProductionScrollBar->setMax(100);
	pourcentageProductionScrollBar->setSmallStep(5);
	pourcentageProductionScrollBar->setLargeStep(20);
	pourcentageProductionScrollBar->setPos(100);
}
void CGUIInformationsWindow::OnWindowResize()
{
	if (isReducted)
		return;

	// On ajoute la taille de la barre de titre aux rectangles des boutons et des textes
	// car elle est prise en compte dans la surface de placement des boutons
	const int titleBarSizeY = 19;

	const bool showPourcentageProduction = showPourcentageProductionZone();

	if (textBox)
	{
		// Retiens la valeur actuelle de la scroll bar du texte d'informations
		int scrollBarPos = (textBox->getScrollBar() ? textBox->getScrollBar()->getPos() : 0);

		// Replace le texte d'informations
		textBox->setRelativePosition(core::recti(10, 10 + titleBarSizeY, ClientRect.LowerRightCorner.X - 5,
			showPourcentageProduction ? ClientRect.LowerRightCorner.Y - 60 : ClientRect.LowerRightCorner.Y - 15));

		// Restaure l'ancienne valeur de la scroll bar (elle a �t� remise � z�ro) et recalcule la position du texte
		if (textBox->getScrollBar())
		{
			scrollBarPos = core::clamp(scrollBarPos, textBox->getScrollBar()->getMin(), textBox->getScrollBar()->getMax());
			textBox->getScrollBar()->setPos(scrollBarPos);
			textBox->setVisibleText(scrollBarPos);
		}
	}

	// Replace le texte au-dessus de la scroll bar du pourcentage de production
	if (pourcentageProductionText)
	{
		pourcentageProductionText->setRelativePosition(core::recti(
			ClientRect.UpperLeftCorner.X + 5, ClientRect.LowerRightCorner.Y - 55,
			ClientRect.LowerRightCorner.X - 20, ClientRect.LowerRightCorner.Y - 30));
		pourcentageProductionText->setVisible(showPourcentageProduction);
	}

	// Replace la scroll bar du pourcentage de production
	if (pourcentageProductionScrollBar)
	{
		pourcentageProductionScrollBar->setRelativePosition(core::recti(
			ClientRect.UpperLeftCorner.X + 5, ClientRect.LowerRightCorner.Y - 30,
			ClientRect.LowerRightCorner.X - 20, ClientRect.LowerRightCorner.Y - 10));
		pourcentageProductionScrollBar->setVisible(showPourcentageProduction);
	}

	CGUIReductableWindow::OnWindowResize();
}
bool CGUIInformationsWindow::OnEvent(const SEvent& event)
{
	if (event.EventType == EET_GUI_EVENT && isEnabled())
	{
		if (event.GUIEvent.EventType == EGET_SCROLL_BAR_CHANGED && event.GUIEvent.Caller == pourcentageProductionScrollBar && selectedBatiment)
		{
			// La scroll bar pour modifier le pourcentage de production a chang� : on actualise le pourcentage de production du b�timent :

			const float pourcentageProduction = pourcentageProductionScrollBar->getPos() * 0.01f;

#ifdef USE_RAKNET
			// Indique � RakNet qu'on veut modifier le pourcentage de production d'un b�timent, s'il est activ�
			if (rkMgr.isNetworkPlaying())
			{
				RakNetManager::RakNetGamePacket packet(RakNetManager::EGPT_BATIMENT_PRODUCTION_PERCENTAGE_CHANGED);
				const core::vector2di& index = selectedBatiment->getIndex();
				packet.gameBatPPChangeInfos.indexX = (INDEX_TYPE)index.X;
				packet.gameBatPPChangeInfos.indexY = (INDEX_TYPE)index.Y;
				packet.gameBatPPChangeInfos.newProductionPercentage = pourcentageProduction;
				rkMgr.sendPackets.push_back(packet);
			}
			else
#endif
				selectedBatiment->getInfos().pourcentageProduction = pourcentageProduction;
		}
	}

	return CGUIReductableWindow::OnEvent(event);
}
void CGUIInformationsWindow::setInfosTexte(const wchar_t* texte)
{
	if (!textBox)
		return;

	// Retiens la valeur actuelle de la scroll bar
	int scrollBarPos = (textBox->getScrollBar() ? textBox->getScrollBar()->getPos() : 0);

	// Indique le texte � afficher � la bo�te de texte
	textBox->setText(texte);

	// Restaure l'ancienne valeur de la scroll bar (elle a �t� remise � z�ro) et recalcule la position du texte
	if (textBox->getScrollBar() && textBox->getScrollBar()->isVisible())
	{
		scrollBarPos = core::clamp(scrollBarPos, textBox->getScrollBar()->getMin(), textBox->getScrollBar()->getMax());
		textBox->getScrollBar()->setPos(scrollBarPos);
		textBox->setVisibleText(scrollBarPos);
	}
}
void CGUIInformationsWindow::setSelectedBatiment(Batiment* bat)
{
	if (selectedBatiment == bat)
		return;

	selectedBatiment = bat;

	// D�termine automatiquement si on doit afficher la zone du pourcentage de production en simulant un redimensionnement de la fen�tre
	OnWindowResize();
}
void CGUIInformationsWindow::updateInfosTexte(CGUIInformationsWindow::E_INFORMATION_TYPE infosType, BatimentID batimentID, u32 batimentCount, Batiment* batiment)
{
	if (!IsVisible)
		return;

	switch (infosType)
	{
	case EIT_CONSTRUCTION:			updateConstructionText(batimentID);							break;
	case EIT_MULTI_CONSTRUCTION:	updateMultiConstructionText(batimentID, batimentCount);		break;
	case EIT_DESTRUCTION:			updateDestructionText(batiment);							break;
	case EIT_SELECTION:				updateSelectionText(batiment);								break;

	default:
#ifdef _DEBUG
		cout << "CGUIInformationsWindow::updateInfosTexte(" << infosType << ", " << batimentID << ", " << batiment << ") :" << endl
			<< "        infosType a une valeur inconnue : infosType = " << infosType << endl;
#endif
		break;
	}
}
void CGUIInformationsWindow::updateConstructionText(BatimentID batimentID)
{
	if (batimentID == BI_aucun)
	{
#ifdef _DEBUG
		cout << "CGUIInformationsWindow::updateConstructionText(" << batimentID << ") : L'ID du batiment fourni n'est pas valide : batimentID = " << batimentID << endl;
#endif
		return;
	}

	// V�rifie que les informations sur le b�timent ont bien chang�es avant de mettre � jour le texte d'informations
	if (lastInfosType == EIT_CONSTRUCTION && lastBatimentInfos.ID == batimentID)
		return;

	// Obtient les informations sur le batiment actuellement s�lectionn� et sur les modifiers de jeu
	const StaticBatimentInfos& staticBatimentInfos = StaticBatimentInfos::getInfos(batimentID);

	// Modifie le titre de la fen�tre
	swprintf_SS(L"Construction : %s", staticBatimentInfos.name);
	setText(textW_SS);

	// Si cette fen�tre est r�duite, on met seulement � jour le titre de la fen�tre, puis on quitte
	if (isReducted)
		return;

	// Met � jour les derni�res informations sur le b�timent
	lastInfosType = EIT_CONSTRUCTION;
	lastBatimentInfos.ID = batimentID;	// D�sactiv� : lastBatimentInfos.setID(batimentID) : lastBatimentInfos n'est pas utilis� dans cette fonction

	core::stringw infosTexte;	// La chaine de caract�res qui contiendra le texte d'informations
	u32 i = 0;					// Variable temporaire utilis�e pour les it�rations

	// Cr�e le texte d'informations
	{
		swprintf_SS(L"Prix (C) : %.2f �\r\n", -staticBatimentInfos.prixC);
		infosTexte.append(textW_SS);

		//swprintf_SS(L"Prix (D) : %.2f �\r\n", -staticBatimentInfos.prixD);
		//infosTexte.append(textW_SS);

		if (staticBatimentInfos.unlockPopulationNeeded != 0)	// Population n�cessaire
		{
			swprintf_SS(L"Population n�cessaire : %u habitants\r\n", staticBatimentInfos.unlockPopulationNeeded);
			infosTexte.append(textW_SS);
		}

		if (staticBatimentInfos.energie < 0.0f)			// Consommation d'�nergie
		{
			swprintf_SS(L"Consommation d'�nergie : %.0f W\r\n", -staticBatimentInfos.energie);
			infosTexte.append(textW_SS);
		}
		else if (staticBatimentInfos.energie > 0.0f)	// Production d'�nergie
		{
			swprintf_SS(L"Production d'�nergie : %.0f W\r\n", staticBatimentInfos.energie);
			infosTexte.append(textW_SS);
		}

		infosTexte.trim();
		infosTexte.append("\r\n\r\n");

		if (staticBatimentInfos.tempsC)
		{
			// Ajoute le nombre de jour du temps de construction :
			infosTexte.append(L"Temps de construction : ");
			Game::appendDays(infosTexte, staticBatimentInfos.tempsC);
			infosTexte.append(L"\r\n");
		}
		//if (staticBatimentInfos.tempsD)
		//{
		//	// Ajoute le nombre de jour du temps de destruction :
		//	infosTexte.append(L"Temps de destruction : ");
		//	Game::appendDays(infosTexte, staticBatimentInfos.tempsD);
		//	infosTexte.append(L"\r\n");
		//}

		if (staticBatimentInfos.dureeVie)
		{
			// Ajoute le nombre de jours de la dur�e de vie :
			infosTexte.append(L"Dur�e de vie : ");
			Game::appendDays(infosTexte, staticBatimentInfos.dureeVie);

			if (staticBatimentInfos.dureeVieVariation)
			{
				// Ajoute le nombre de jours de la variation de la dur�e de vie :
				infosTexte.append(L" (� ");
				Game::appendDays(infosTexte, staticBatimentInfos.dureeVieVariation);
				infosTexte.append(L")\r\n");
			}
			else	// Ajoute simplement le retour � la ligne
				infosTexte.append(L"\r\n");
		}
		else
			infosTexte.append(L"Dur�e de vie : Infinie\r\n");

		infosTexte.trim();
		infosTexte.append("\r\n\r\n");

		if (staticBatimentInfos.entretienM)
		{
			swprintf_SS(L"Entretien (M) : %.2f �\r\n", staticBatimentInfos.entretienM);
			infosTexte.append(textW_SS);
		}
		if (staticBatimentInfos.eauConsommationJ)
		{
			swprintf_SS(L"Consommation d'eau (J) : %.0f L\r\n", staticBatimentInfos.eauConsommationJ);
			infosTexte.append(textW_SS);
		}

		infosTexte.trim();
		infosTexte.append("\r\n\r\n");

		swprintf_SS(L"Effet de serre (C) : %.0f kg\r\n", staticBatimentInfos.effetSerreC);
		infosTexte.append(textW_SS);
		if (staticBatimentInfos.effetSerreJ < 10.0f)	// Utilis� pour les faibles absorptions de l'effet de serre des arbres
			swprintf_SS(L"Effet de serre (J) : %.2f kg\r\n", staticBatimentInfos.effetSerreJ);
		else
			swprintf_SS(L"Effet de serre (J) : %.0f kg\r\n", staticBatimentInfos.effetSerreJ);
		infosTexte.append(textW_SS);
		//swprintf_SS(L"Effet de serre (D) : %.0f kg\r\n", staticBatimentInfos.effetSerreD);
		//infosTexte.append(textW_SS);

		swprintf_SS(L"D�chets (C) : %.0f kg\r\n", staticBatimentInfos.dechetsC);
		infosTexte.append(textW_SS);
		swprintf_SS(L"D�chets (J) : %.0f kg\r\n", staticBatimentInfos.dechetsJ);
		infosTexte.append(textW_SS);
		//swprintf_SS(L"D�chets (D) : %.0f kg\r\n", staticBatimentInfos.dechetsD);
		//infosTexte.append(textW_SS);

		infosTexte.trim();
		infosTexte.append("\r\n\r\n");

		if (staticBatimentInfos.isHabitation)
		{
			if (staticBatimentInfos.habitantsMax > 0)
			{
				swprintf_SS(L"Habitants : %u / %u\r\n",
					staticBatimentInfos.habitantsC, staticBatimentInfos.habitantsMax);
				infosTexte.append(textW_SS);
			}
			if (staticBatimentInfos.loyerM != 0.0f)
			{
				swprintf_SS(L"Loyer (M) : %.2f �\r\n",
					staticBatimentInfos.loyerM);
				infosTexte.append(textW_SS);
			}
			if (staticBatimentInfos.energie < 0.0f)
			{
				swprintf_SS(L"Facture d'�lectricit� (M) : %.2f �\r\n",
					-staticBatimentInfos.energie * 30.0f * FACTURE_ELECTRICITE);
				infosTexte.append(textW_SS);
			}
			if (staticBatimentInfos.eauConsommationJ > 0.0f)
			{
				swprintf_SS(L"Facture d'eau (M) : %.2f �\r\n",
					staticBatimentInfos.eauConsommationJ * 30.0f * FACTURE_EAU);
				infosTexte.append(textW_SS);
			}
			if (staticBatimentInfos.impotsA != 0.0f)
			{
				swprintf_SS(L"Imp�ts (A) : %.2f �\r\n",
					staticBatimentInfos.impotsA);
				infosTexte.append(textW_SS);
			}
		}

		// Affiche les ressources n�cessaires � la construction
		if (staticBatimentInfos.ressourcesC.size())
		{
			infosTexte.trim();
			infosTexte.append("\r\n\r\n");
			infosTexte.append("Ressources (C) :\r\n");
			const u32 ressourcesCSize = staticBatimentInfos.ressourcesC.size();
			for (i = 0; i < ressourcesCSize; ++i)
			{
				infosTexte.append(Ressources::getRessourceName(staticBatimentInfos.ressourcesC[i].ID));
				swprintf_SS(L" : %+.0f ", staticBatimentInfos.ressourcesC[i].production);
				infosTexte.append(textW_SS);
				infosTexte.append(Ressources::getRessourceUnit(staticBatimentInfos.ressourcesC[i].ID));
				infosTexte.append("\r\n");
			}
		}

		// Affiche les ressources produites et consomm�es chaque jour
		if (staticBatimentInfos.ressourcesJ.size())
		{
			infosTexte.trim();
			infosTexte.append("\r\n\r\n");
			infosTexte.append("Ressources (J) :\r\n");
			const u32 ressourcesJSize = staticBatimentInfos.ressourcesJ.size();
			for (i = 0; i < ressourcesJSize; ++i)
			{
				infosTexte.append(Ressources::getRessourceName(staticBatimentInfos.ressourcesJ[i].ID));
				swprintf_SS(L" : %+.0f ",
					staticBatimentInfos.ressourcesJ[i].production);
				infosTexte.append(textW_SS);
				infosTexte.append(Ressources::getRessourceUnit(staticBatimentInfos.ressourcesJ[i].ID));
				infosTexte.append("\r\n");
			}
		}

		/*
		// Affiche les ressources n�cessaires � la destruction
		if (staticBatimentInfos.ressourcesD.size())
		{
			infosTexte.trim();
			infosTexte.append("\r\n\r\n");
			infosTexte.append("Ressources (D) :\r\n");
			const u32 ressourcesDSize = staticBatimentInfos.ressourcesD.size();
			for (i = 0; i < ressourcesDSize; ++i)
			{
				infosTexte.append(Ressources::getRessourceName(staticBatimentInfos.ressourcesD[i].ID));
				swprintf_SS(L" : %+.0f ", staticBatimentInfos.ressourcesD[i].production);
				infosTexte.append(textW_SS);
				infosTexte.append(Ressources::getRessourceUnit(staticBatimentInfos.ressourcesD[i].ID));
				infosTexte.append("\r\n");
			}
		}
		*/
	}

	// Si le texte d'informations commence ou se termine par des caract�res vides, on les supprime
	infosTexte.trim();

	// Indique le texte d'informations � afficher
	setInfosTexte(infosTexte.c_str());
}
void CGUIInformationsWindow::updateMultiConstructionText(BatimentID batimentID, u32 batimentCount)
{
	if (batimentID == BI_aucun)
	{
#ifdef _DEBUG
		cout << "CGUIInformationsWindow::updateMultiConstructionText(" << batimentID << ") : L'ID du batiment fourni n'est pas valide : batimentID = " << batimentID << endl;
#endif
		return;
	}

	// V�rifie que les informations sur le b�timent ont bien chang�es avant de mettre � jour le texte d'informations
	if (lastInfosType == EIT_MULTI_CONSTRUCTION && lastBatimentInfos.ID == batimentID && lastMultiBatCount == batimentCount)
		return;

	// Obtient les informations sur le batiment actuellement s�lectionn� et sur les modifiers de jeu
	const StaticBatimentInfos& staticBatimentInfos = StaticBatimentInfos::getInfos(batimentID);

	// Modifie le titre de la fen�tre
	swprintf_SS(L"Construction : %s (%u)", staticBatimentInfos.name, batimentCount);
	setText(textW_SS);

	// Si cette fen�tre est r�duite, on met seulement � jour le titre de la fen�tre, puis on quitte
	if (isReducted)
		return;

	// Met � jour les derni�res informations sur le b�timent
	lastInfosType = EIT_MULTI_CONSTRUCTION;
	lastBatimentInfos.ID = batimentID;	// D�sactiv� : lastBatimentInfos.setID(batimentID) : lastBatimentInfos n'est pas utilis� dans cette fonction
	lastMultiBatCount = batimentCount;

	core::stringw infosTexte;	// La chaine de caract�res qui contiendra le texte d'informations
	u32 i = 0;					// Variable temporaire utilis�e pour les it�rations

	const float batimentCountF = (float)batimentCount;	// Conversion en float de batimentCount

	// Cr�e le texte d'informations
	{
		swprintf_SS(L"Prix total (C) : %.2f �\r\n", -staticBatimentInfos.prixC * batimentCountF);
		infosTexte.append(textW_SS);

		//swprintf_SS(L"Prix total (D) : %.2f �\r\n", -staticBatimentInfos.prixD * batimentCountF);
		//infosTexte.append(textW_SS);

		if (staticBatimentInfos.unlockPopulationNeeded != 0)	// Population n�cessaire
		{
			swprintf_SS(L"Population n�cessaire : %u habitants\r\n", staticBatimentInfos.unlockPopulationNeeded);
			infosTexte.append(textW_SS);
		}

		if (staticBatimentInfos.energie < 0.0f)			// Consommation d'�nergie
		{
			swprintf_SS(L"Consommation d'�nergie totale : %.0f W\r\n", -staticBatimentInfos.energie * batimentCountF);
			infosTexte.append(textW_SS);
		}
		else if (staticBatimentInfos.energie > 0.0f)	// Production d'�nergie
		{
			swprintf_SS(L"Production d'�nergie totale : %.0f W\r\n", staticBatimentInfos.energie * batimentCountF);
			infosTexte.append(textW_SS);
		}

		infosTexte.trim();
		infosTexte.append("\r\n\r\n");

		if (staticBatimentInfos.tempsC)
		{
			// Ajoute le nombre de jour du temps de construction :
			infosTexte.append(L"Temps de construction : ");
			Game::appendDays(infosTexte, staticBatimentInfos.tempsC);
			infosTexte.append(L"\r\n");
		}
		//if (staticBatimentInfos.tempsD)
		//{
		//	// Ajoute le nombre de jour du temps de destruction :
		//	infosTexte.append(L"Temps de destruction : ");
		//	Game::appendDays(infosTexte, staticBatimentInfos.tempsD);
		//	infosTexte.append(L"\r\n");
		//}

		if (staticBatimentInfos.dureeVie)
		{
			// Ajoute le nombre de jours de la dur�e de vie :
			infosTexte.append(L"Dur�e de vie : ");
			Game::appendDays(infosTexte, staticBatimentInfos.dureeVie);

			if (staticBatimentInfos.dureeVieVariation)
			{
				// Ajoute le nombre de jours de la variation de la dur�e de vie :
				infosTexte.append(L" (� ");
				Game::appendDays(infosTexte, staticBatimentInfos.dureeVieVariation);
				infosTexte.append(L")\r\n");
			}
			else	// Ajoute simplement le retour � la ligne
				infosTexte.append(L"\r\n");
		}
		else
			infosTexte.append(L"Dur�e de vie : Infinie\r\n");

		infosTexte.trim();
		infosTexte.append("\r\n\r\n");

		if (staticBatimentInfos.entretienM)
		{
			swprintf_SS(L"Entretien total (M) : %.2f �\r\n", staticBatimentInfos.entretienM * batimentCountF);
			infosTexte.append(textW_SS);
		}
		if (staticBatimentInfos.eauConsommationJ)
		{
			swprintf_SS(L"Consommation d'eau totale (J) : %.0f L\r\n", staticBatimentInfos.eauConsommationJ * batimentCountF);
			infosTexte.append(textW_SS);
		}

		infosTexte.trim();
		infosTexte.append("\r\n\r\n");

		swprintf_SS(L"Effet de serre total (C) : %.0f kg\r\n", staticBatimentInfos.effetSerreC * batimentCountF);
		infosTexte.append(textW_SS);
		if (staticBatimentInfos.effetSerreJ < 10.0f)	// Utilis� pour les faibles absorptions de l'effet de serre des arbres
			swprintf_SS(L"Effet de serre total (J) : %.2f kg\r\n", staticBatimentInfos.effetSerreJ * batimentCountF);
		else
			swprintf_SS(L"Effet de serre total (J) : %.0f kg\r\n", staticBatimentInfos.effetSerreJ * batimentCountF);
		infosTexte.append(textW_SS);
		//swprintf_SS(L"Effet de serre total (D) : %.0f kg\r\n", staticBatimentInfos.effetSerreD * batimentCountF);
		//infosTexte.append(textW_SS);

		swprintf_SS(L"D�chets totaux (C) : %.0f kg\r\n", staticBatimentInfos.dechetsC * batimentCountF);
		infosTexte.append(textW_SS);
		swprintf_SS(L"D�chets totaux (J) : %.0f kg\r\n", staticBatimentInfos.dechetsJ * batimentCountF);
		infosTexte.append(textW_SS);
		//swprintf_SS(L"D�chets totaux (D) : %.0f kg\r\n", staticBatimentInfos.dechetsD * batimentCountF);
		//infosTexte.append(textW_SS);

		infosTexte.trim();
		infosTexte.append("\r\n\r\n");

		if (staticBatimentInfos.isHabitation)
		{
			if (staticBatimentInfos.habitantsMax > 0)
			{
				swprintf_SS(L"Habitants totaux : %u / %u\r\n",
					staticBatimentInfos.habitantsC * batimentCount, staticBatimentInfos.habitantsMax * batimentCount);
				infosTexte.append(textW_SS);
			}
			if (staticBatimentInfos.loyerM != 0.0f)
			{
				swprintf_SS(L"Loyer total (M) : %.2f �\r\n",
					staticBatimentInfos.loyerM * batimentCountF);
				infosTexte.append(textW_SS);
			}
			if (staticBatimentInfos.energie < 0.0f)
			{
				swprintf_SS(L"Facture d'�lectricit� totale (M) : %.2f �\r\n",
					-staticBatimentInfos.energie * 30.0f * FACTURE_ELECTRICITE * batimentCountF);
				infosTexte.append(textW_SS);
			}
			if (staticBatimentInfos.eauConsommationJ > 0.0f)
			{
				swprintf_SS(L"Facture d'eau totale (M) : %.2f �\r\n",
					staticBatimentInfos.eauConsommationJ * 30.0f * FACTURE_EAU * batimentCountF);
				infosTexte.append(textW_SS);
			}
			if (staticBatimentInfos.impotsA != 0.0f)
			{
				swprintf_SS(L"Imp�ts totaux (A) : %.2f �\r\n",
					staticBatimentInfos.impotsA * batimentCountF);
				infosTexte.append(textW_SS);
			}
		}

		// Affiche les ressources n�cessaires � la construction
		if (staticBatimentInfos.ressourcesC.size())
		{
			infosTexte.trim();
			infosTexte.append("\r\n\r\n");
			infosTexte.append("Ressources totales (C) :\r\n");
			const u32 ressourcesCSize = staticBatimentInfos.ressourcesC.size();
			for (i = 0; i < ressourcesCSize; ++i)
			{
				infosTexte.append(Ressources::getRessourceName(staticBatimentInfos.ressourcesC[i].ID));
				swprintf_SS(L" : %+.0f ", staticBatimentInfos.ressourcesC[i].production * batimentCountF);
				infosTexte.append(textW_SS);
				infosTexte.append(Ressources::getRessourceUnit(staticBatimentInfos.ressourcesC[i].ID));
				infosTexte.append("\r\n");
			}
		}

		// Affiche les ressources produites et consomm�es chaque jour
		if (staticBatimentInfos.ressourcesJ.size())
		{
			infosTexte.trim();
			infosTexte.append("\r\n\r\n");
			infosTexte.append("Ressources totales (J) :\r\n");
			const u32 ressourcesJSize = staticBatimentInfos.ressourcesJ.size();
			for (i = 0; i < ressourcesJSize; ++i)
			{
				infosTexte.append(Ressources::getRessourceName(staticBatimentInfos.ressourcesJ[i].ID));
				swprintf_SS(L" : %+.0f ", staticBatimentInfos.ressourcesJ[i].production * batimentCountF);
				infosTexte.append(textW_SS);
				infosTexte.append(Ressources::getRessourceUnit(staticBatimentInfos.ressourcesJ[i].ID));
				infosTexte.append("\r\n");
			}
		}

		/*
		// Affiche les ressources n�cessaires � la destruction
		if (staticBatimentInfos.ressourcesD.size())
		{
			infosTexte.trim();
			infosTexte.append("\r\n\r\n");
			infosTexte.append("Ressources totales (D) :\r\n");
			for (i = 0; i < staticBatimentInfos.ressourcesD.size(); ++i)
			{
				infosTexte.append(Ressources::getRessourceName(staticBatimentInfos.ressourcesD[i].ID));
				swprintf_SS(L" : %+.0f ", staticBatimentInfos.ressourcesD[i].production * batimentCountF);
				infosTexte.append(textW_SS);
				infosTexte.append(Ressources::getRessourceUnit(staticBatimentInfos.ressourcesD[i].ID));
				infosTexte.append("\r\n");
			}
		}
		*/
	}

	// Si le texte d'informations commence ou se termine par des caract�res vides, on les supprime
	infosTexte.trim();

	// Indique le texte d'informations � afficher
	setInfosTexte(infosTexte.c_str());
}
void CGUIInformationsWindow::updateDestructionText(Batiment* batiment)
{
	if (!batiment)
	{
#ifdef _DEBUG
		cout << "CGUIInformationsWindow::updateDestructionText(" << batiment << ") : Le batiment fourni n'est pas valide : batiment = " << batiment << endl;
#endif
		return;
	}

	// V�rifie que les informations sur le b�timent ont bien chang�es avant de mettre � jour le texte d'informations
	if (lastInfosType == EIT_DESTRUCTION && batiment->getDaysPassed() == lastBatDaysPassed && batiment->getInfos() == lastBatimentInfos)
		return;

	// Obtient les informations sur le batiment actuellement s�lectionn� et sur les modifiers de jeu
	const StaticBatimentInfos& staticBatimentInfos = batiment->getStaticInfos();

	// Modifie le titre de la fen�tre
	swprintf_SS(L"Destruction : %s", staticBatimentInfos.name);
	setText(textW_SS);

	// Si cette fen�tre est r�duite, on met seulement � jour le titre de la fen�tre, puis on quitte
	if (isReducted)
		return;

	// Met � jour les derni�res informations sur le b�timent
	lastInfosType = EIT_SELECTION;
	lastBatDaysPassed = batiment->getDaysPassed();
	lastBatimentInfos = batiment->getInfos();

	core::stringw infosTexte;	// La chaine de caract�res qui contiendra le texte d'informations
	u32 i = 0;					// Variable temporaire utilis�e pour les it�rations

	// Cr�e le texte d'informations
	{
		// D�termine si le b�timent est en cours de destruction : dans ce cas, on n'affiche pas d'autres informations que son temps de destruction
		if (batiment->isDestroying())
		{
			infosTexte.append("Destruction : Restant : ");
			Game::appendDays(infosTexte, (staticBatimentInfos.tempsD - (system.getTime().getTotalJours() - batiment->getDestroyingDay())));
			infosTexte.append(L"\r\n\r\n");
		}
		else
		{
			// D�termine si le b�timent est en cours de construction
			if (batiment->isConstructing())
			{
				infosTexte.append("Construction : Restant : ");
				Game::appendDays(infosTexte, (staticBatimentInfos.tempsC - batiment->getDaysPassed()));
				infosTexte.append(L"\r\n\r\n");
			}

			swprintf_SS(L"Prix (D) : %.2f �\r\n", -staticBatimentInfos.prixD);
			infosTexte.append(textW_SS);

			if (staticBatimentInfos.tempsD)
			{
				// Ajoute le nombre de jour du temps de destruction :
				infosTexte.append(L"Temps de destruction : ");
				Game::appendDays(infosTexte, staticBatimentInfos.tempsD);
				infosTexte.append(L"\r\n");
			}

			if (staticBatimentInfos.entretienM)
			{
				swprintf_SS(L"Entretien en moins (M) : %.2f �\r\n", lastBatimentInfos.entretienM);
				infosTexte.append(textW_SS);
			}

			if (staticBatimentInfos.energie < 0.0f)			// Ce b�timent consomme de l'�nergie
			{
				swprintf_SS(L"Energie lib�r�e : %.0f W\r\n", fabs(lastBatimentInfos.energie));	// "fabs" est utilis� pour toujours enlever le signe "-", m�me lorsque le r�sultat est nul
				infosTexte.append(textW_SS);
			}
			else if (staticBatimentInfos.energie > 0.0f)	// Ce b�timent produit de l'�nergie
			{
				swprintf_SS(L"Energie perdue : %.0f W\r\n", fabs(lastBatimentInfos.energie));	// "fabs" est utilis� pour toujours enlever le signe "-", m�me lorsque le r�sultat est nul
				infosTexte.append(textW_SS);
			}

			if (staticBatimentInfos.eauConsommationJ)
			{
				swprintf_SS(L"Consommation d'eau en moins (J) : %.0f L\r\n", lastBatimentInfos.eauConsommationJ);
				infosTexte.append(textW_SS);
			}

			infosTexte.trim();
			infosTexte.append("\r\n\r\n");

			// Ajoute le nombre de jours du temps de vie :
			infosTexte.append(L"Temps de vie : ");
			Game::appendDays(infosTexte, batiment->getDaysPassed());
			infosTexte.append("\r\n");

			if (lastBatimentInfos.dureeVie)
			{
				// Ajoute le nombre de jours de la dur�e de vie :
				infosTexte.append(L"Dur�e de vie : ");
				Game::appendDays(infosTexte, lastBatimentInfos.dureeVie);
				infosTexte.append(L"\r\n");

				// Ajoute le nombre de jour de la dur�e de vie restante :
				const u32 remainingDays = lastBatimentInfos.dureeVie - batiment->getDaysPassed();
				infosTexte.append(L"Dur�e de vie restante : ");
				Game::appendDays(infosTexte, remainingDays);

				// Ajoute le pourcentage restant de la dur�e de vie totale :
				swprintf_SS(L" (%.0f %% de la dur�e de vie totale)\r\n",	// '%%' : Symbole %
					((float)(remainingDays) / (float)(lastBatimentInfos.dureeVie)) * 100.0f);
				infosTexte.append(textW_SS);
			}
			else
				infosTexte.append(L"Dur�e de vie : Infinie\r\n");

			infosTexte.trim();
			infosTexte.append("\r\n\r\n");

			if (lastBatimentInfos.effetSerreJ < 10.0f)	// Utilis� pour les faibles absorptions de l'effet de serre des arbres
				swprintf_SS(L"Effet de serre en moins (J) : %.2f kg\r\n", lastBatimentInfos.effetSerreJ);
			else
				swprintf_SS(L"Effet de serre en moins (J) : %.0f kg\r\n", lastBatimentInfos.effetSerreJ);
			infosTexte.append(textW_SS);
			swprintf_SS(L"Effet de serre (D) : %.0f kg\r\n", staticBatimentInfos.effetSerreD);
			infosTexte.append(textW_SS);

			swprintf_SS(L"D�chets en moins (J) : %.0f kg\r\n", lastBatimentInfos.dechetsJ);
			infosTexte.append(textW_SS);
			swprintf_SS(L"D�chets (D) : %.0f kg\r\n", staticBatimentInfos.dechetsD);
			infosTexte.append(textW_SS);

			infosTexte.trim();
			infosTexte.append("\r\n\r\n");

			if (staticBatimentInfos.isHabitation)
			{
				if (staticBatimentInfos.habitantsMax > 0)
				{
					swprintf_SS(L"Habitants d�m�nageant : %u / %u\r\n",
						lastBatimentInfos.habitants, staticBatimentInfos.habitantsMax);
					infosTexte.append(textW_SS);
				}
				if (staticBatimentInfos.loyerM != 0.0f)
				{
					swprintf_SS(L"Loyer en moins (M) : %.2f �\r\n",
						fabs(lastBatimentInfos.loyerM));	// "fabs" est utilis� pour toujours enlever le signe "-", m�me lorsque le r�sultat est nul
					infosTexte.append(textW_SS);
				}
				if (staticBatimentInfos.energie < 0.0f)
				{
					swprintf_SS(L"Facture d'�lectricit� en moins (M) : %.2f �\r\n",
						lastBatimentInfos.currentEnergieConsommationM * FACTURE_ELECTRICITE);
					infosTexte.append(textW_SS);
				}
				if (staticBatimentInfos.eauConsommationJ > 0.0f)
				{
					swprintf_SS(L"Facture d'eau en moins (M) : %.2f �\r\n",
						lastBatimentInfos.currentEauConsommationM * FACTURE_EAU);
					infosTexte.append(textW_SS);
				}
				if (staticBatimentInfos.impotsA != 0.0f)
				{
					swprintf_SS(L"Imp�ts en moins (A) : %.2f �\r\n",
						 //batiment->isConstructingOrDestroying()) ? 0.0f :
						 lastBatimentInfos.impotsA);
					infosTexte.append(textW_SS);
				}
			}

			if (StaticBatimentInfos::needPourcentageProduction(batiment->getID()))
			{
				if (lastBatimentInfos.pourcentageProduction != lastBatimentInfos.pourcentageProductionActuel)
				{
					swprintf_SS(L"Pourcentage de production d�sir� : %.0f %%\r\n", lastBatimentInfos.pourcentageProduction * 100.0f);
					infosTexte.append(textW_SS);
					swprintf_SS(L"Pourcentage de production r�el : %.0f %%\r\n", lastBatimentInfos.pourcentageProductionActuel * 100.0f);
					infosTexte.append(textW_SS);
				}
				else
				{
					swprintf_SS(L"Pourcentage de production : %.0f %%\r\n", lastBatimentInfos.pourcentageProduction * 100.0f);
					infosTexte.append(textW_SS);
				}
			}

			// Affiche les ressources produites et consomm�es chaque jour
			if (lastBatimentInfos.ressourcesJ.size())
			{
				infosTexte.trim();
				infosTexte.append("\r\n\r\n");
				infosTexte.append("Ressources en moins (J) :\r\n");
				const u32 ressourcesJSize = lastBatimentInfos.ressourcesJ.size();
				for (i = 0; i < ressourcesJSize; ++i)
				{
					infosTexte.append(Ressources::getRessourceName(lastBatimentInfos.ressourcesJ[i].ID));
					swprintf_SS(L" : %+.0f ", lastBatimentInfos.ressourcesJ[i].production);
					infosTexte.append(textW_SS);
					infosTexte.append(Ressources::getRessourceUnit(lastBatimentInfos.ressourcesJ[i].ID));
					infosTexte.append("\r\n");
				}
			}

			// Affiche les ressources n�cessaires � la destruction
			if (staticBatimentInfos.ressourcesD.size())
			{
				infosTexte.trim();
				infosTexte.append("\r\n\r\n");
				infosTexte.append("Ressources (D) :\r\n");
				const u32 ressourcesDSize = staticBatimentInfos.ressourcesD.size();
				for (i = 0; i < ressourcesDSize; ++i)
				{
					infosTexte.append(Ressources::getRessourceName(staticBatimentInfos.ressourcesD[i].ID));
					if (staticBatimentInfos.isTree && i == Ressources::RI_bois)	// Si ce b�timent est un arbre, et que la ressource � g�rer est le bois � sa destruction :
					{
						// Prend en compte le fait qu'un arbre fournit du bois � sa destruction proportionnellement � son �ge (jusqu'� un an) : plus il est grand, plus il fournit de bois
						swprintf_SS(L" : %+.0f ", staticBatimentInfos.ressourcesD[i].production * min((float)(batiment->getDaysPassed()) / 360.0f, 1.0f));
					}
					else
						swprintf_SS(L" : %+.0f ", staticBatimentInfos.ressourcesD[i].production);
					infosTexte.append(textW_SS);
					infosTexte.append(Ressources::getRessourceUnit(staticBatimentInfos.ressourcesD[i].ID));
					infosTexte.append("\r\n");
				}
			}
		}
	}

	// Si le texte d'informations commence ou se termine par des caract�res vides, on les supprime
	infosTexte.trim();

	// Indique le texte d'informations � afficher
	setInfosTexte(infosTexte.c_str());
}
void CGUIInformationsWindow::updateSelectionText(Batiment* batiment)
{
	if (!batiment)
	{
#ifdef _DEBUG
		cout << "CGUIInformationsWindow::updateSelectionText(" << batiment << ") : Le batiment fourni n'est pas valide : batiment = " << batiment << endl;
#endif
		return;
	}

	// V�rifie que les informations sur le b�timent ont bien chang�es avant de mettre � jour le texte d'informations
	if (lastInfosType == EIT_SELECTION && batiment->getDaysPassed() == lastBatDaysPassed && batiment->getInfos() == lastBatimentInfos)
		return;

	// Demande le redimensionnement de la fen�tre pour v�rifier si on n�cessite l'affichage de la barre de production :
	// Si le b�timent vient d'�tre construit, l'appel � cette fonction est n�cessaire pour la r�afficher
	OnWindowResize();

	// Obtient les informations sur le batiment actuellement s�lectionn�
	const StaticBatimentInfos& staticBatimentInfos = batiment->getStaticInfos();

	// Modifie le titre de la fen�tre
	swprintf_SS(L"S�lection : %s", staticBatimentInfos.name);
	setText(textW_SS);

	// Si cette fen�tre est r�duite, on met seulement � jour le titre de la fen�tre, puis on quitte
	if (isReducted)
		return;

	// Met � jour les derni�res informations sur le b�timent
	lastInfosType = EIT_SELECTION;
	lastBatDaysPassed = batiment->getDaysPassed();
	lastBatimentInfos = batiment->getInfos();

	core::stringw infosTexte;	// La chaine de caract�res qui contiendra le texte d'informations
	u32 i = 0;					// Variable temporaire utilis�e pour les it�rations

	// Obtient les informations sur le monde
	const EcoWorldInfos& worldInfos = system.getInfos();

	// Cr�e le texte d'informations
	{
		// D�termine si le b�timent est en cours de destruction
		if (batiment->isDestroying())
		{
			infosTexte.append("Destruction : Restant : ");
			Game::appendDays(infosTexte, (staticBatimentInfos.tempsD - (system.getTime().getTotalJours() - batiment->getDestroyingDay())));
			infosTexte.append(L"\r\n\r\n");
		}
		else
		{
			// D�termine si le b�timent est en cours de construction
			if (batiment->isConstructing())
			{
				infosTexte.append("Construction : Restant : ");
				Game::appendDays(infosTexte, (staticBatimentInfos.tempsC - batiment->getDaysPassed()));
				infosTexte.append(L"\r\n\r\n");
			}
			else
			{
				if (staticBatimentInfos.entretienM)
				{
					swprintf_SS(L"Entretien (M) : %.2f �\r\n", lastBatimentInfos.entretienM);
					infosTexte.append(textW_SS);
				}

				if (staticBatimentInfos.energie < 0.0f)			// Ce b�timent consomme de l'�nergie
				{
					if (staticBatimentInfos.energie != lastBatimentInfos.energie)
					{
						swprintf_SS(L"Consommation d'�nergie maximale : %.0f W\r\n",
							-staticBatimentInfos.energie);
						infosTexte.append(textW_SS);
					}
					swprintf_SS(L"Consommation d'�nergie actuelle : %.0f W\r\n",
						fabs(-lastBatimentInfos.energie * worldInfos.pourcentageEnergieDisponible));	// "fabs" est utilis� pour toujours enlever le signe "-", m�me lorsque le r�sultat est nul
					infosTexte.append(textW_SS);
				}
				else if (staticBatimentInfos.energie > 0.0f)	// Ce b�timent produit de l'�nergie
				{
					if (staticBatimentInfos.energie != lastBatimentInfos.energie)
					{
						swprintf_SS(L"Production d'�nergie maximale : %.0f W\r\n",
							fabs(staticBatimentInfos.energie));
						infosTexte.append(textW_SS);
					}
					swprintf_SS(L"Production d'�nergie actuelle : %.0f W\r\n",
						fabs(lastBatimentInfos.energie));												// "fabs" est utilis� pour toujours enlever le signe "-", m�me lorsque le r�sultat est nul
					infosTexte.append(textW_SS);
				}

				if (staticBatimentInfos.eauConsommationJ)
				{
					swprintf_SS(L"Consommation d'eau actuelle (J) : %.0f L\r\n", lastBatimentInfos.eauConsommationJ);
					infosTexte.append(textW_SS);
				}

				infosTexte.trim();
				infosTexte.append("\r\n\r\n");
			}

			// Ajoute le nombre de jours du temps de vie :
			infosTexte.append(L"Temps de vie : ");
			Game::appendDays(infosTexte, batiment->getDaysPassed());
			infosTexte.append("\r\n");

			if (lastBatimentInfos.dureeVie)
			{
				// Ajoute le nombre de jours de la dur�e de vie :
				infosTexte.append(L"Dur�e de vie : ");
				Game::appendDays(infosTexte, lastBatimentInfos.dureeVie);
				infosTexte.append(L"\r\n");

				// Ajoute le nombre de jour de la dur�e de vie restante :
				const u32 remainingDays = lastBatimentInfos.dureeVie - batiment->getDaysPassed();
				infosTexte.append(L"Dur�e de vie restante : ");
				Game::appendDays(infosTexte, remainingDays);

				// Ajoute le pourcentage restant de la dur�e de vie totale :
				swprintf_SS(L" (%.0f %% de la dur�e de vie totale)\r\n",	// '%%' : Symbole %
					((float)(remainingDays) / (float)(lastBatimentInfos.dureeVie)) * 100.0f);
				infosTexte.append(textW_SS);
			}
			else
				infosTexte.append(L"Dur�e de vie : Infinie\r\n");

			infosTexte.trim();
			infosTexte.append("\r\n\r\n");

			if (!batiment->isConstructing())
			{
				if (lastBatimentInfos.effetSerreJ < 10.0f)	// Utilis� pour les faibles absorptions de l'effet de serre des arbres
					swprintf_SS(L"Effet de serre (J) : %.2f kg\r\n", lastBatimentInfos.effetSerreJ);
				else
					swprintf_SS(L"Effet de serre (J) : %.0f kg\r\n", lastBatimentInfos.effetSerreJ);
				infosTexte.append(textW_SS);

				swprintf_SS(L"D�chets (J) : %.0f kg\r\n", lastBatimentInfos.dechetsJ);
				infosTexte.append(textW_SS);

				infosTexte.trim();
				infosTexte.append("\r\n\r\n");

				if (staticBatimentInfos.isHabitation)
				{
					if (staticBatimentInfos.habitantsMax > 0)
					{
						swprintf_SS(L"Habitants : %u / %u\r\n",
							lastBatimentInfos.habitants, staticBatimentInfos.habitantsMax);
						infosTexte.append(textW_SS);
					}
					if (staticBatimentInfos.loyerM != 0.0f)
					{
						swprintf_SS(L"Loyer r�el (M) : %.2f �\r\n", fabs(lastBatimentInfos.loyerM));	// "fabs" est utilis� pour toujours enlever le signe "-", m�me lorsque le r�sultat est nul
						infosTexte.append(textW_SS);
					}
					if (staticBatimentInfos.energie < 0.0f)
					{
						swprintf_SS(L"Facture d'�lectricit� (M) : %.2f �\r\n",
							lastBatimentInfos.currentEnergieConsommationM * FACTURE_ELECTRICITE);
						infosTexte.append(textW_SS);
					}
					if (staticBatimentInfos.eauConsommationJ > 0.0f)
					{
						swprintf_SS(L"Facture d'eau (M) : %.2f �\r\n",
							lastBatimentInfos.currentEauConsommationM * FACTURE_EAU);
						infosTexte.append(textW_SS);
					}
					if (staticBatimentInfos.impotsA != 0.0f)
					{
						swprintf_SS(L"Imp�ts (A) : %.2f �\r\n",
							//batiment->isConstructingOrDestroying()) ? 0.0f :
							lastBatimentInfos.impotsA);
						infosTexte.append(textW_SS);
					}
				}

				if (StaticBatimentInfos::needPourcentageProduction(batiment->getID()))
				{
					if (lastBatimentInfos.pourcentageProduction != lastBatimentInfos.pourcentageProductionActuel)
					{
						swprintf_SS(L"Pourcentage de production d�sir� : %.0f %%\r\n", lastBatimentInfos.pourcentageProduction * 100.0f);
						infosTexte.append(textW_SS);
						swprintf_SS(L"Pourcentage de production r�el : %.0f %%\r\n", lastBatimentInfos.pourcentageProductionActuel * 100.0f);
						infosTexte.append(textW_SS);
					}
					else
					{
						swprintf_SS(L"Pourcentage de production : %.0f %%\r\n", lastBatimentInfos.pourcentageProduction * 100.0f);
						infosTexte.append(textW_SS);
					}
				}

				// Affiche les ressources produites et consomm�es chaque jour
				if (lastBatimentInfos.ressourcesJ.size())
				{
					infosTexte.trim();
					infosTexte.append("\r\n\r\n");
					infosTexte.append("Ressources (J) :\r\n");
					const u32 ressourcesJSize = lastBatimentInfos.ressourcesJ.size();
					for (i = 0; i < ressourcesJSize; ++i)
					{
						infosTexte.append(Ressources::getRessourceName(lastBatimentInfos.ressourcesJ[i].ID));
						swprintf_SS(L" : %+.0f ", lastBatimentInfos.ressourcesJ[i].production);
						infosTexte.append(textW_SS);
						infosTexte.append(Ressources::getRessourceUnit(lastBatimentInfos.ressourcesJ[i].ID));
						infosTexte.append("\r\n");
					}
				}
			}
		}
	}

	// Si le texte d'informations commence ou se termine par des caract�res vides, on les supprime
	infosTexte.trim();

	// Indique le texte d'informations � afficher
	setInfosTexte(infosTexte.c_str());
}
