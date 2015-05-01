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

	// Indique le texte par défaut de cette fenêtre
	setText(L"Tableau récapitulatif");

	// Indique la taille minimale de cette fenêtre
	setMinSize(core::dimension2du(300, 120));

	// Crée le tableau à la position désirée
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
	table->addColumn(L"Déchets (J)");
	table->addColumn(L"Entretien réel (M)");
	table->addColumn(L"Habitants");
	table->addColumn(L"Loyer réel (M)");
	table->addColumn(L"Facture d'électricité (M)");
	table->addColumn(L"Facture d'eau (M)");
	table->addColumn(L"Impôts (A)");
	table->addColumn(L"Temps de vie");
	table->addColumn(L"Durée de vie restante");
	table->addColumn(L"Pourcentage de production désiré");
	table->addColumn(L"Pourcentage de production réel");

	// Indique la méthode de tri de chaque colonne
	const int columnCount = table->getColumnCount();
	for (int i = 0; i < columnCount; ++i)
		table->setColumnOrdering(i, EGCO_FLIP_ASCENDING_DESCENDING);

	// Rempli le tableau d'après le système de jeu
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
			if (event.KeyInput.PressedDown) // La touche a été pressée : on appuie sur le bouton
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
			else // La touche a été relevée : on active l'action du bouton
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
		sendEvent.GUIEvent.EventType = EGET_ELEMENT_CLOSED; // Indique que cette fenêtre a été fermée

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

	// Vérifie que le système a été remis à jour depuis le dernier appel à cette fonction
	if (lastSystemDay == system.getTime().getTotalJours() && !forceUpdate)
		return;
	lastSystemDay = system.getTime().getTotalJours();

	// Regroupe certains bâtiments dans la même ligne :
	// Routes, Arbres (Trembles, Chênes, Pins, Saules), Panneaux Solaires, Eoliennes, Hydroliennes
#ifndef KID_VERSION	// En mode enfant : on désactive les routes qui sont inutiles
	core::list<const Batiment*> batListRoutes;
#endif
	core::list<const Batiment*> batListTrembles;
	core::list<const Batiment*> batListChenes;
	core::list<const Batiment*> batListPins;
	core::list<const Batiment*> batListSaules;
	core::list<const Batiment*> batListPanneauxSolaires;
	core::list<const Batiment*> batListEoliennes;
	core::list<const Batiment*> batListHydroliennes;

	// Se souviens de la ligne actuellement sélectionnée pour pouvoir la restaurer après la mise à jour du tableau
	const int lastSelectedRow = table->getSelected();

	// Efface toutes les lignes du tableau
	table->clearRows();

	// Parcours tous les bâtiments du système
	const core::list<Batiment*>& allBatiments = system.getAllBatiments();
	const Batiment* batiment = NULL;
	const core::list<Batiment*>::ConstIterator END = allBatiments.end();
	for (core::list<Batiment*>::ConstIterator it = allBatiments.begin(); it != END; ++it)
	{
		batiment = (*it);
		if (!batiment)
			continue;

		// Ajoute les bâtiments à leur listes respectives s'ils doivent être regroupés dans la même ligne
		switch (batiment->getID())
		{
#ifndef KID_VERSION	// En mode enfant : on désactive les routes qui sont inutiles
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
			// Sinon, ajoute une ligne normale pour ce bâtiment
			addBatimentRow(batiment);
			break;
		}

	}

	// Ajoute les lignes pour les bâtiments multiples
#ifndef KID_VERSION	// En mode enfant : on désactive les routes qui sont inutiles
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
	//			(sans jamais diminuer la taille des colonnes, toujours en les agrandissant si nécessaire)

	// Trie le tableau avec les nouvelles lignes ajoutées
	table->orderRows(-1, table->getActiveColumnOrdering());

	// Restaure la dernière ligne sélectionnée du tableau
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

	// Obtient les informations de base sur le bâtiment
	const StaticBatimentInfos& staticBatimentInfos = batiment->getStaticInfos();
	const BatimentInfos& batimentInfos = batiment->getInfos();

	// Ajoute une ligne pour ce bâtiment
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
		if (value < 10.0f)	// Utilisé pour les faibles absorptions de l'effet de serre des arbres
			swprintf_SS(L"%.2f kg", value);
		else
			swprintf_SS(L"%.0f kg", value);

		if (value == 0.0f)
			table->setCellText(row, column, textW_SS);
		else
			table->setCellText(row, column, textW_SS, (value < 0.0f ? greenColor : redColor));
		table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	} column++;

	// Déchets (J)
	{
		const float value = batimentInfos.dechetsJ;
		swprintf_SS(L"%.0f kg", value);
		if (value == 0.0f)
			table->setCellText(row, column, textW_SS);
		else
			table->setCellText(row, column, textW_SS, (value < 0.0f ? greenColor : redColor));
		table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	} column++;

	// Entretien réel (M)
	{
		const float value = batimentInfos.entretienM;
		swprintf_SS(L"%.2f €", value);
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

	// Loyer réel (M)
	if (staticBatimentInfos.isHabitation)
	{
		const float value = batimentInfos.loyerM;
		swprintf_SS(L"%.2f €", value);
		if (value == 0.0f)
			table->setCellText(row, column, textW_SS);
		else
			table->setCellText(row, column, textW_SS, greenColor);
	}
	else
		table->setCellText(row, column, L"-");
	table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	column++;

	// Facture d'électricité (M)
	if (staticBatimentInfos.isHabitation && staticBatimentInfos.energie > 0.0f)
	{
		const float value = batimentInfos.currentEnergieConsommationM * FACTURE_ELECTRICITE;
		swprintf_SS(L"%.2f €", value);
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
		swprintf_SS(L"%.2f €", value);
		if (value == 0.0f)
			table->setCellText(row, column, textW_SS);
		else
			table->setCellText(row, column, textW_SS, greenColor);
	}
	else
		table->setCellText(row, column, L"-");
	table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	column++;

	// Impôts (A)
	if (staticBatimentInfos.isHabitation)
	{
		const float value = batimentInfos.impotsA;
		swprintf_SS(L"%.2f €", value);
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

	// Durée de vie
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

	// Pourcentage de production réel
	if (needPourcentageProduction)
	{
		const float value = batimentInfos.pourcentageProductionActuel * 100.0f;
		swprintf_SS(L"%.0f %%", value);
		if (batimentInfos.pourcentageProductionActuel == batimentInfos.pourcentageProduction
			|| batiment->isConstructingOrDestroying())
			table->setCellText(row, column, textW_SS);
		else
			table->setCellText(row, column, textW_SS, redColor);	// Texte rouge si la production actuelle du bâtiment est différente (donc forcément inférieure) à la production demandée
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

	// L'iterator sur cette liste de bâtiments
	core::list<const Batiment*>::ConstIterator it = batList.begin();

	if (batList.size() == 1)
	{
		// Si la liste des bâtiments ne contient qu'un seul élément, on ne crée qu'une ligne simple pour celui-ci
		addBatimentRow(*it);
		return;
	}

	// Obtient les couleurs pour les cellules du tableau
	const video::SColor defaultColor = Environment->getSkin() ? Environment->getSkin()->getColor(EGDC_BUTTON_TEXT) : video::SColor(255, 0, 0, 0);
	const video::SColor greenColor(defaultColor.getAlpha(), defaultColor.getRed(), 150, defaultColor.getBlue());
	const video::SColor redColor(defaultColor.getAlpha(), 200, defaultColor.getGreen(), defaultColor.getBlue());

	// Obtient les informations de base sur le bâtiment basées sur le premier bâtiment de la liste
	const StaticBatimentInfos& staticBatimentInfos = (*it)->getStaticInfos();
	BatimentInfos totalBatimentInfos = (*it)->getInfos();
	const u32 batCount = batList.size();

	// Booléens pour retenir l'état des différents bâtiments
	bool isBatimentConstructing((*it)->isConstructing());						// True si des bâtiments sont en construction
	bool isBatimentDestroying((*it)->isDestroying());							// True si des bâtiments sont en destruction
	bool isBatimentNormal(!isBatimentConstructing && !isBatimentDestroying);	// True si des bâtiments sont en mode de vie normal

	// Calcule les informations totales sur ces bâtiments (on ignore le premier bâtiment car il est déjà ajouté : c'est celui qui nous sert à obtenir les informations de base)
	for (++it; it != batList.end(); ++it)
	{
		// Met à jour l'état des différents bâtiments
		if ((*it)->isConstructing())
			isBatimentConstructing = true;
		else if ((*it)->isDestroying())
			isBatimentDestroying = true;
		else
			isBatimentNormal = true;

		// Obtient les informations de base sur ce bâtiment
		const BatimentInfos& batimentInfos = (*it)->getInfos();

		// Energie
		totalBatimentInfos.energie += batimentInfos.energie;

		// Consommation d'électricité (M)
		totalBatimentInfos.currentEnergieConsommationM += batimentInfos.currentEnergieConsommationM;

		// Consommation d'eau (M)
		totalBatimentInfos.currentEauConsommationM += batimentInfos.currentEauConsommationM;

		// Effet de Serre (J)
		totalBatimentInfos.effetSerreJ += batimentInfos.effetSerreJ;

		// Déchets (J)
		totalBatimentInfos.dechetsJ += batimentInfos.dechetsJ;

		// Entretien réel (M)
		totalBatimentInfos.entretienM += batimentInfos.entretienM;

		if (staticBatimentInfos.isHabitation)
		{
			// Habitants
			totalBatimentInfos.habitants += batimentInfos.habitants;

			// Loyer réel (M)
			totalBatimentInfos.loyerM += batimentInfos.loyerM;
		}
	}

	// Ajoute une ligne pour ces bâtiments
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
		if (value < 10.0f)	// Utilisé pour les faibles absorptions de l'effet de serre des arbres
			swprintf_SS(L"%.2f kg", value);
		else
			swprintf_SS(L"%.0f kg", value);

		if (value == 0.0f)
			table->setCellText(row, column, textW_SS);
		else
			table->setCellText(row, column, textW_SS, (value < 0.0f ? greenColor : redColor));
		table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	} column++;

	// Déchets (J)
	{
		const float value = totalBatimentInfos.dechetsJ;
		swprintf_SS(L"%.0f kg", value);
		if (value == 0.0f)
			table->setCellText(row, column, textW_SS);
		else
			table->setCellText(row, column, textW_SS, (value < 0.0f ? greenColor : redColor));
		table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	} column++;

	// Entretien réel (M)
	{
		const float value = totalBatimentInfos.entretienM;
		swprintf_SS(L"%.2f €", value);
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

	// Loyer réel (M)
	if (staticBatimentInfos.isHabitation)
	{
		const float value = totalBatimentInfos.loyerM;
		swprintf_SS(L"%.2f €", value);
		if (value == 0.0f)
			table->setCellText(row, column, textW_SS);
		else
			table->setCellText(row, column, textW_SS, greenColor);
	}
	else
		table->setCellText(row, column, L"-");
	table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	column++;

	// Facture d'électricité (M)
	if (staticBatimentInfos.isHabitation && staticBatimentInfos.energie < 0.0f)
	{
		const float value = totalBatimentInfos.currentEnergieConsommationM * FACTURE_ELECTRICITE;
		swprintf_SS(L"%.2f €", value);
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
		swprintf_SS(L"%.2f €", value);
		if (value == 0.0f)
			table->setCellText(row, column, textW_SS);
		else
			table->setCellText(row, column, textW_SS, greenColor);
	}
	else
		table->setCellText(row, column, L"-");
	table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	column++;

	// Impôts (A)
	{
		const float value = staticBatimentInfos.impotsA * (float)batCount;
		swprintf_SS(L"%.2f €", value);
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

	// Durée de vie
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

	// Pourcentage de production réel
	// TODO
	{
		table->setCellText(row, column, L"-");
		table->setCellData(row, column, table->getTriTypePointer(CGUISortTable::ETT_VALUE));
	} column++;
}
