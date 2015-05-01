#include "CGUIMonthTableWindow.h"
#include "CGUISortTable.h"
#include "EcoWorldSystem.h"
#include "Batiment.h"
#include "Game.h"

CGUIMonthTableWindow::CGUIMonthTableWindow(const EcoWorldSystem& m_system, IGUIEnvironment* environment, IGUIElement* parent, int id, const core::recti& rectangle)
: CGUICenteredTitleWindow(environment, parent, id, rectangle), system(m_system), table(0), lastSystemDay(0)
{
#ifdef _DEBUG
	setDebugName("CGUIMonthTableWindow");
#endif

	// Indique le texte par d�faut de cette fen�tre
	setText(L"Tableau r�capitulatif");

	// Indique la taille minimale de cette fen�tre
	setMinSize(core::dimension2du(300, 120));

	// Cr�e le tableau � la position d�sir�e
	table = new CGUISortTable(environment, this, -1, core::recti(
			ClientRect.UpperLeftCorner.X + 10, ClientRect.UpperLeftCorner.Y + 10,
			ClientRect.LowerRightCorner.X - 15, ClientRect.LowerRightCorner.Y - 15),
		true, true, false);
	table->setDrawFlags(EGTDF_ROWS | EGTDF_COLUMNS | EGTDF_ACTIVE_ROW);

	// Ajoute les colonnes du tableau
	table->addColumn(L"Nom");
	table->addColumn(L"Etat");
	table->addColumn(L"Temps restant");
	table->addColumn(L"Energie");
	table->addColumn(L"Consommation d'eau (J)");
	table->addColumn(L"Effet de Serre (J)");
	table->addColumn(L"D�chets (J)");
	table->addColumn(L"Entretien r�el (M)");
	table->addColumn(L"Habitants");
	table->addColumn(L"Loyer r�el (M)");
	table->addColumn(L"Facture d'�lectricit� (M)");
	table->addColumn(L"Facture d'eau (M)");
	table->addColumn(L"Imp�ts (A)");
	table->addColumn(L"Temps de vie");
	table->addColumn(L"Dur�e de vie restante");
	table->addColumn(L"Pourcentage de production d�sir�");
	table->addColumn(L"Pourcentage de production r�el");

	// Indique la m�thode de tri de chaque colonne
	const int columnCount = table->getColumnCount();
	for (int i = 0; i < columnCount; ++i)
		table->setColumnOrdering(i, EGCO_FLIP_ASCENDING_DESCENDING);

	// Rempli le tableau d'apr�s le syst�me de jeu
	update();

	// Indique la colonne active et trie les lignes du tableau
	table->setActiveColumn(0, true);
}
CGUIMonthTableWindow::~CGUIMonthTableWindow()
{
	if (table)
		table->drop();
}
bool CGUIMonthTableWindow::OnEvent(const SEvent& event)
{
	bool closeMenu = false;

	if (isEnabled() && Parent)
	{
		if (event.EventType == EET_KEY_INPUT_EVENT)
		{
			if (event.KeyInput.PressedDown) // La touche a �t� press�e : on appuie sur le bouton
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
			else // La touche a �t� relev�e : on active l'action du bouton
			{
				switch (event.KeyInput.Key)
				{
				case KEY_RETURN:
				case KEY_ESCAPE:
					closeMenu = true;
					break;

				default: break;
				}
			}
		}
		else if (event.EventType == EET_GUI_EVENT)
		{
			if (event.GUIEvent.EventType == EGET_BUTTON_CLICKED && event.GUIEvent.Caller == CloseButton)
				closeMenu = true;
		}
	}

	// Ferme ce menu
	if (closeMenu && Parent)
	{
		SEvent sendEvent;
		sendEvent.EventType = EET_GUI_EVENT;
		sendEvent.GUIEvent.Caller = this;
		sendEvent.GUIEvent.Element = NULL;
		sendEvent.GUIEvent.EventType = EGET_ELEMENT_CLOSED; // Indique que cette fen�tre a �t� ferm�e

		Parent->OnEvent(sendEvent);

		return true;
	}

	return CGUICenteredTitleWindow::OnEvent(event);
}
void CGUIMonthTableWindow::OnWindowResize()
{
	// Replace le tableau
	if (table)
		table->setRelativePosition(core::recti(
			ClientRect.UpperLeftCorner.X + 10, ClientRect.UpperLeftCorner.Y + 10,
			ClientRect.LowerRightCorner.X - 15, ClientRect.LowerRightCorner.Y - 15));

	CGUICenteredTitleWindow::OnWindowResize();
}
void CGUIMonthTableWindow::update(bool forceUpdate)
{
	if (!table)
		return;

	// V�rifie que le syst�me a �t� remis � jour depuis le dernier appel � cette fonction
	if (lastSystemDay == system.getTime().getTotalJours() && !forceUpdate)
		return;
	lastSystemDay = system.getTime().getTotalJours();

	// Regroupe certains b�timents dans la m�me ligne :
	// Routes, Arbres (Trembles, Ch�nes, Pins, Saules), Panneaux Solaires, Eoliennes, Hydroliennes
#ifndef KID_VERSION	// En mode enfant : on d�sactive les routes qui sont inutiles
	core::list<const Batiment*> batListRoutes;
#endif
	core::list<const Batiment*> batListTrembles;
	core::list<const Batiment*> batListChenes;
	core::list<const Batiment*> batListPins;
	core::list<const Batiment*> batListSaules;
	core::list<const Batiment*> batListPanneauxSolaires;
	core::list<const Batiment*> batListEoliennes;
	core::list<const Batiment*> batListHydroliennes;

	// Se souviens de la ligne actuellement s�lectionn�e pour pouvoir la restaurer apr�s la mise � jour du tableau
	const int lastSelectedRow = table->getSelected();

	// Efface toutes les lignes du tableau
	table->clearRows();

	// Parcours tous les b�timents du syst�me
	const core::list<Batiment*>& allBatiments = system.getAllBatiments();
	const Batiment* batiment = NULL;
	const core::list<Batiment*>::ConstIterator END = allBatiments.end();
	for (core::list<Batiment*>::ConstIterator it = allBatiments.begin(); it != END; ++it)
	{
		batiment = (*it);
		if (!batiment)
			continue;

		// Ajoute les b�timents � leur listes respectives s'ils doivent �tre regroup�s dans la m�me ligne
		switch (batiment->getID())
		{
#ifndef KID_VERSION	// En mode enfant : on d�sactive les routes qui sont inutiles
		case BI_route:
			batListRoutes.push_back(batiment);
			break;
#endif
		case BI_arbre_aspen:
			batListTrembles.push_back(batiment);
			break;
		case BI_arbre_oak:
			batListChenes.push_back(batiment);
			break;
		case BI_arbre_pine:
			batListPins.push_back(batiment);
			break;
		case BI_arbre_willow:
			batListSaules.push_back(batiment);
			break;
		case BI_panneau_solaire:
			batListPanneauxSolaires.push_back(batiment);
			break;
		case BI_eolienne:
			batListEoliennes.push_back(batiment);
			break;
		case BI_hydrolienne:
			batListHydroliennes.push_back(batiment);
			break;

		default:
			// Sinon, ajoute une ligne normale pour ce b�timent
			addBatimentRow(batiment);
			break;
		}

	}

	// Ajoute les lignes pour les b�timents multiples
#ifndef KID_VERSION	// En mode enfant : on d�sactive les routes qui sont inutiles
	addMultiBatimentRow(batListRoutes);
#endif
	addMultiBatimentRow(batListTrembles);
	addMultiBatimentRow(batListChenes);
	addMultiBatimentRow(batListPins);
	addMultiBatimentRow(batListSaules);
	addMultiBatimentRow(batListPanneauxSolaires);
	addMultiBatimentRow(batListEoliennes);
	addMultiBatimentRow(batListHydroliennes);

	// TODO : Calculer et indiquer automatiquement la largeur de chaque colonne suivant la taille du texte dans leurs cellules
	//			(sans jamais diminuer la taille des colonnes, toujours en les agrandissant si n�cessaire)

	// Trie le tableau avec les nouvelles lignes ajout�es
	table->orderRows(-1, table->getActiveColumnOrdering());

	// Restaure la derni�re ligne s�lectionn�e du tableau
	table->setSelected(lastSelectedRow);
}
void CGUIMonthTableWindow::addBatimentRow(const Batiment* batiment)
{
	if (!batiment || !table)
		return;

	// Obtient les couleurs pour les cellules du tableau
	const video::SColor defaultColor = Environment->getSkin() ? Environment->getSkin()->getColor(EGDC_BUTTON_TEXT) : video::SColor(255, 0, 0, 0);
	const video::SColor greenColor(defaultColor.getAlpha(), defaultColor.getRed(), 150, defaultColor.getBlue());
	const video::SColor redColor(defaultColor.getAlpha(), 200, defaultColor.getGreen(), defaultColor.getBlue());

	// Obtient les informations de base sur le b�timent
	const StaticBatimentInfos& staticBatimentInfos = batiment->getStaticInfos();
	const BatimentInfos& batimentInfos = batiment->getInfos();

	// Ajoute une ligne pour ce b�timent
	const u32 row = table->addRow(table->getRowCount());
	u32 column = 0;

	// Indique les valeurs des cellules de cette ligne :

	// Nom
	{
		table->setCellText(row, column, staticBatimentInfos.name);
		table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_ALPHABETICAL));
	} column++;

	// Etat
	{
		table->setCellText(row, column, (batiment->isConstructing() ? L"En construction" : (batiment->isDestroying() ? L"En destruction" : L"Normal")));
		table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_ALPHABETICAL));
	} column++;

	// Temps restant
	if (batiment->isConstructingOrDestroying() || batimentInfos.dureeVie != 0)
	{
		u32 value = 0;
		if (batiment->isConstructing())
			value = (staticBatimentInfos.tempsC - batiment->getDaysPassed());
		else if (batiment->isDestroying())
			value = (staticBatimentInfos.tempsD - (system.getTime().getTotalJours() - batiment->getDestroyingDay()));
		else
			value = batimentInfos.dureeVie - batiment->getDaysPassed();

		core::stringw str;
		Game::appendDays(str, value);
		table->setCellText(row, column, str);
	}
	else
		table->setCellText(row, column, L"Infini", greenColor);
	table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_TIME));
	column++;

	// Energie
	{
		const float value = batimentInfos.energie;
		swprintf_SS(L"%+.0f W", value);
		if (value == 0.0f)
			table->setCellText(row, column, textW_SS);
		else
			table->setCellText(row, column, textW_SS, (value < 0.0f ? redColor : greenColor));
		table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	} column++;

	// Consommation d'eau (J)
	{
		const float value = batimentInfos.eauConsommationJ;
		swprintf_SS(L"%.0f L", value);
		if (value == 0.0f)
			table->setCellText(row, column, textW_SS);
		else
			table->setCellText(row, column, textW_SS, redColor);
		table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	} column++;

	// Effet de Serre (J)
	{
		const float value = batimentInfos.effetSerreJ;
		if (value < 10.0f)	// Utilis� pour les faibles absorptions de l'effet de serre des arbres
			swprintf_SS(L"%.2f kg", value);
		else
			swprintf_SS(L"%.0f kg", value);

		if (value == 0.0f)
			table->setCellText(row, column, textW_SS);
		else
			table->setCellText(row, column, textW_SS, (value < 0.0f ? greenColor : redColor));
		table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	} column++;

	// D�chets (J)
	{
		const float value = batimentInfos.dechetsJ;
		swprintf_SS(L"%.0f kg", value);
		if (value == 0.0f)
			table->setCellText(row, column, textW_SS);
		else
			table->setCellText(row, column, textW_SS, (value < 0.0f ? greenColor : redColor));
		table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	} column++;

	// Entretien r�el (M)
	{
		const float value = batimentInfos.entretienM;
		swprintf_SS(L"%.2f �", value);
		if (value == 0.0f)
			table->setCellText(row, column, textW_SS);
		else
			table->setCellText(row, column, textW_SS, redColor);
		table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	} column++;

	// Habitants
	if (staticBatimentInfos.isHabitation)
	{
		const u32 value = batimentInfos.habitants;
		swprintf_SS(L"%u", value);
		if (value == 0)
			table->setCellText(row, column, textW_SS);
		else
			table->setCellText(row, column, textW_SS, greenColor);
	}
	else
	{
		table->setCellText(row, column, L"-");
		table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	} column++;

	// Loyer r�el (M)
	if (staticBatimentInfos.isHabitation)
	{
		const float value = batimentInfos.loyerM;
		swprintf_SS(L"%.2f �", value);
		if (value == 0.0f)
			table->setCellText(row, column, textW_SS);
		else
			table->setCellText(row, column, textW_SS, greenColor);
	}
	else
		table->setCellText(row, column, L"-");
	table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	column++;

	// Facture d'�lectricit� (M)
	if (staticBatimentInfos.isHabitation && staticBatimentInfos.energie > 0.0f)
	{
		const float value = batimentInfos.currentEnergieConsommationM * FACTURE_ELECTRICITE;
		swprintf_SS(L"%.2f �", value);
		if (value == 0.0f)
			table->setCellText(row, column, textW_SS);
		else
			table->setCellText(row, column, textW_SS, greenColor);
	}
	else
		table->setCellText(row, column, L"-");
	table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	column++;

	// Facture d'eau (M)
	if (staticBatimentInfos.isHabitation && staticBatimentInfos.eauConsommationJ > 0.0f)
	{
		const float value = batimentInfos.currentEauConsommationM * FACTURE_EAU;
		swprintf_SS(L"%.2f �", value);
		if (value == 0.0f)
			table->setCellText(row, column, textW_SS);
		else
			table->setCellText(row, column, textW_SS, greenColor);
	}
	else
		table->setCellText(row, column, L"-");
	table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	column++;

	// Imp�ts (A)
	if (staticBatimentInfos.isHabitation)
	{
		const float value = batimentInfos.impotsA;
		swprintf_SS(L"%.2f �", value);
		if (value == 0.0f)
			table->setCellText(row, column, textW_SS);
		else
			table->setCellText(row, column, textW_SS, greenColor);
		table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	} column++;

	// Temps de vie
	{
		core::stringw str;
		Game::appendDays(str, batiment->getDaysPassed());
		table->setCellText(row, column, str);
		table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_TIME));
	} column++;

	// Dur�e de vie
	if (batimentInfos.dureeVie != 0)
	{
		core::stringw str;
		Game::appendDays(str, batimentInfos.dureeVie);
		table->setCellText(row, column, str);
	}
	else
		table->setCellText(row, column, L"Infinie", greenColor);
	table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_TIME));
	column++;

	const bool needPourcentageProduction = StaticBatimentInfos::needPourcentageProduction(batiment->getID());

	// Pourcentage de production
	if (needPourcentageProduction)
	{
		const float value = batimentInfos.pourcentageProduction * 100.0f;
		swprintf_SS(L"%.0f %%", value);
		table->setCellText(row, column, textW_SS);
	}
	else
		table->setCellText(row, column, L"-");
	table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	column++;

	// Pourcentage de production r�el
	if (needPourcentageProduction)
	{
		const float value = batimentInfos.pourcentageProductionActuel * 100.0f;
		swprintf_SS(L"%.0f %%", value);
		if (batimentInfos.pourcentageProductionActuel == batimentInfos.pourcentageProduction
			|| batiment->isConstructingOrDestroying())
			table->setCellText(row, column, textW_SS);
		else
			table->setCellText(row, column, textW_SS, redColor);	// Texte rouge si la production actuelle du b�timent est diff�rente (donc forc�ment inf�rieure) � la production demand�e
	}
	else
		table->setCellText(row, column, L"-");
	table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	column++;
}
void CGUIMonthTableWindow::addMultiBatimentRow(const core::list<const Batiment*>& batList)
{
	if (batList.empty())
		return;

	// L'iterator sur cette liste de b�timents
	core::list<const Batiment*>::ConstIterator it = batList.begin();

	if (batList.size() == 1)
	{
		// Si la liste des b�timents ne contient qu'un seul �l�ment, on ne cr�e qu'une ligne simple pour celui-ci
		addBatimentRow(*it);
		return;
	}

	// Obtient les couleurs pour les cellules du tableau
	const video::SColor defaultColor = Environment->getSkin() ? Environment->getSkin()->getColor(EGDC_BUTTON_TEXT) : video::SColor(255, 0, 0, 0);
	const video::SColor greenColor(defaultColor.getAlpha(), defaultColor.getRed(), 150, defaultColor.getBlue());
	const video::SColor redColor(defaultColor.getAlpha(), 200, defaultColor.getGreen(), defaultColor.getBlue());

	// Obtient les informations de base sur le b�timent bas�es sur le premier b�timent de la liste
	const StaticBatimentInfos& staticBatimentInfos = (*it)->getStaticInfos();
	BatimentInfos totalBatimentInfos = (*it)->getInfos();
	const u32 batCount = batList.size();

	// Bool�ens pour retenir l'�tat des diff�rents b�timents
	bool isBatimentConstructing((*it)->isConstructing());						// True si des b�timents sont en construction
	bool isBatimentDestroying((*it)->isDestroying());							// True si des b�timents sont en destruction
	bool isBatimentNormal(!isBatimentConstructing && !isBatimentDestroying);	// True si des b�timents sont en mode de vie normal

	// Calcule les informations totales sur ces b�timents (on ignore le premier b�timent car il est d�j� ajout� : c'est celui qui nous sert � obtenir les informations de base)
	for (++it; it != batList.end(); ++it)
	{
		// Met � jour l'�tat des diff�rents b�timents
		if ((*it)->isConstructing())
			isBatimentConstructing = true;
		else if ((*it)->isDestroying())
			isBatimentDestroying = true;
		else
			isBatimentNormal = true;

		// Obtient les informations de base sur ce b�timent
		const BatimentInfos& batimentInfos = (*it)->getInfos();

		// Energie
		totalBatimentInfos.energie += batimentInfos.energie;

		// Consommation d'�lectricit� (M)
		totalBatimentInfos.currentEnergieConsommationM += batimentInfos.currentEnergieConsommationM;

		// Consommation d'eau (M)
		totalBatimentInfos.currentEauConsommationM += batimentInfos.currentEauConsommationM;

		// Effet de Serre (J)
		totalBatimentInfos.effetSerreJ += batimentInfos.effetSerreJ;

		// D�chets (J)
		totalBatimentInfos.dechetsJ += batimentInfos.dechetsJ;

		// Entretien r�el (M)
		totalBatimentInfos.entretienM += batimentInfos.entretienM;

		if (staticBatimentInfos.isHabitation)
		{
			// Habitants
			totalBatimentInfos.habitants += batimentInfos.habitants;

			// Loyer r�el (M)
			totalBatimentInfos.loyerM += batimentInfos.loyerM;
		}
	}

	// Ajoute une ligne pour ces b�timents
	const u32 row = table->addRow(table->getRowCount());
	u32 column = 0;

	// Indique les valeurs des cellules de cette ligne :

	// Nom
	{
		swprintf_SS(L"%s (%u)", staticBatimentInfos.name, batCount);
		table->setCellText(row, column, textW_SS);
		table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_ALPHABETICAL));
	} column++;

	// Etat
	{
		core::stringw str;
		if (isBatimentConstructing)
		{
			str.append(L"En construction");
			if (isBatimentNormal || isBatimentDestroying)
				str.append(L" / ");
		}
		if (isBatimentNormal)
			str.append(L"Normal");
		if (isBatimentDestroying)
		{
			if (isBatimentConstructing || isBatimentNormal)
				str.append(L" / ");
			str.append(L"En destruction");
		}
		table->setCellText(row, column, str.c_str());
		table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_ALPHABETICAL));
	} column++;

	// Temps restant
	// TODO
	{
		table->setCellText(row, column, L"-");
		table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_TIME));
	} column++;

	// Energie
	{
		const float value = totalBatimentInfos.energie;
		swprintf_SS(L"%+.0f W", value);
		if (value == 0.0f)
			table->setCellText(row, column, textW_SS);
		else
			table->setCellText(row, column, textW_SS, (value < 0.0f ? redColor : greenColor));
		table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	} column++;

	// Consommation d'eau (J)
	{
		const float value = totalBatimentInfos.eauConsommationJ;
		swprintf_SS(L"%.0f L", value);
		if (value == 0.0f)
			table->setCellText(row, column, textW_SS);
		else
			table->setCellText(row, column, textW_SS, redColor);
		table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	} column++;

	// Effet de Serre (J)
	{
		const float value = totalBatimentInfos.effetSerreJ;
		if (value < 10.0f)	// Utilis� pour les faibles absorptions de l'effet de serre des arbres
			swprintf_SS(L"%.2f kg", value);
		else
			swprintf_SS(L"%.0f kg", value);

		if (value == 0.0f)
			table->setCellText(row, column, textW_SS);
		else
			table->setCellText(row, column, textW_SS, (value < 0.0f ? greenColor : redColor));
		table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	} column++;

	// D�chets (J)
	{
		const float value = totalBatimentInfos.dechetsJ;
		swprintf_SS(L"%.0f kg", value);
		if (value == 0.0f)
			table->setCellText(row, column, textW_SS);
		else
			table->setCellText(row, column, textW_SS, (value < 0.0f ? greenColor : redColor));
		table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	} column++;

	// Entretien r�el (M)
	{
		const float value = totalBatimentInfos.entretienM;
		swprintf_SS(L"%.2f �", value);
		if (value == 0.0f)
			table->setCellText(row, column, textW_SS);
		else
			table->setCellText(row, column, textW_SS, redColor);
		table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	} column++;

	// Habitants
	if (staticBatimentInfos.isHabitation)
	{
		const u32 value = totalBatimentInfos.habitants;
		swprintf_SS(L"%u", value);
		if (value == 0)
			table->setCellText(row, column, textW_SS);
		else
			table->setCellText(row, column, textW_SS, greenColor);
	}
	else
		table->setCellText(row, column, L"-");
	table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	column++;

	// Loyer r�el (M)
	if (staticBatimentInfos.isHabitation)
	{
		const float value = totalBatimentInfos.loyerM;
		swprintf_SS(L"%.2f �", value);
		if (value == 0.0f)
			table->setCellText(row, column, textW_SS);
		else
			table->setCellText(row, column, textW_SS, greenColor);
	}
	else
		table->setCellText(row, column, L"-");
	table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	column++;

	// Facture d'�lectricit� (M)
	if (staticBatimentInfos.isHabitation && staticBatimentInfos.energie < 0.0f)
	{
		const float value = totalBatimentInfos.currentEnergieConsommationM * FACTURE_ELECTRICITE;
		swprintf_SS(L"%.2f �", value);
		if (value == 0.0f)
			table->setCellText(row, column, textW_SS);
		else
			table->setCellText(row, column, textW_SS, greenColor);
	}
	else
		table->setCellText(row, column, L"-");
	table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	column++;

	// Facture d'eau (M)
	if (staticBatimentInfos.isHabitation && staticBatimentInfos.eauConsommationJ > 0.0f)
	{
		const float value = totalBatimentInfos.currentEauConsommationM * FACTURE_EAU;
		swprintf_SS(L"%.2f �", value);
		if (value == 0.0f)
			table->setCellText(row, column, textW_SS);
		else
			table->setCellText(row, column, textW_SS, greenColor);
	}
	else
		table->setCellText(row, column, L"-");
	table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	column++;

	// Imp�ts (A)
	{
		const float value = staticBatimentInfos.impotsA * (float)batCount;
		swprintf_SS(L"%.2f �", value);
		if (value == 0.0f)
			table->setCellText(row, column, textW_SS);
		else
			table->setCellText(row, column, textW_SS, greenColor);
		table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	} column++;

	// Temps de vie
	// TODO
	{
		table->setCellText(row, column, L"-");
		table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_TIME));
	} column++;

	// Dur�e de vie
	// TODO
	{
		table->setCellText(row, column, L"-");
		table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_TIME));
	} column++;

	// Pourcentage de production
	// TODO
	{
		table->setCellText(row, column, L"-");
		table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	} column++;

	// Pourcentage de production r�el
	// TODO
	{
		table->setCellText(row, column, L"-");
		table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	} column++;
}
