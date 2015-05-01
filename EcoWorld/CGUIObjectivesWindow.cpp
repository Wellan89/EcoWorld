#include "CGUIObjectivesWindow.h"
#include "EcoWorldSystem.h"
#include "Objectives.h"

CGUIObjectivesWindow::CGUIObjectivesWindow(const EcoWorldSystem& m_system, IGUIEnvironment* environment, IGUIElement* parent, int id, const core::recti& rectangle)
: CGUICenteredTitleWindow(environment, parent, id, rectangle), system(m_system), table(0), lastSystemDay(0)
{
#ifdef _DEBUG
	setDebugName("CGUIObjectivesWindow");
#endif

	// Indique le texte par défaut de cette fenêtre
	setText(L"Objectifs");

	// Indique la taille minimale de cette fenêtre
	// TODO : A régler
	setMinSize(core::dimension2du(300, 120));

	// Crée le tableau à la position désirée
	table = environment->addTable(core::recti(
			ClientRect.UpperLeftCorner.X + 10, ClientRect.UpperLeftCorner.Y + 10,
			ClientRect.LowerRightCorner.X - 15, ClientRect.LowerRightCorner.Y - 15),
			this, -1, true);
	table->setDrawFlags(EGTDF_ROWS | EGTDF_COLUMNS | EGTDF_ACTIVE_ROW);

	// Ajoute les colonnes du tableau
	// TODO : Améliorer les textes de ce tableau (nom des colonnes + textes des colonnes)
	table->addColumn(L"Description de l'objectif");
	table->addColumn(L"Type");
	table->addColumn(L"Vérifié");

	// Indique la méthode de tri de chaque colonne
	const int columnCount = table->getColumnCount();
	for (int i = 0; i < columnCount; ++i)
		table->setColumnOrdering(i, EGCO_FLIP_ASCENDING_DESCENDING);

	// Rempli le tableau d'après les objectifs actuels
	update();

	// Indique la colonne active et trie les lignes du tableau
	table->setActiveColumn(0, true);
}
bool CGUIObjectivesWindow::OnEvent(const SEvent& event)
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
void CGUIObjectivesWindow::OnWindowResize()
{
	// Replace le tableau
	if (table)
		table->setRelativePosition(core::recti(
			ClientRect.UpperLeftCorner.X + 10, ClientRect.UpperLeftCorner.Y + 10,
			ClientRect.LowerRightCorner.X - 15, ClientRect.LowerRightCorner.Y - 15));

	CGUICenteredTitleWindow::OnWindowResize();
}
void CGUIObjectivesWindow::update(bool forceUpdate)
{
	if (!table)
		return;

	// Vérifie que le système a été remis à jour depuis le dernier appel à cette fonction
	if (lastSystemDay == system.getTime().getTotalJours() && !forceUpdate)
		return;
	lastSystemDay = system.getTime().getTotalJours();

	// Se souviens de la ligne actuellement sélectionnée pour pouvoir la restaurer après la mise à jour du tableau
	const int lastSelectedRow = table->getSelected();

	// Efface toutes les lignes du tableau
	// TODO : Trouver un meilleur moyen de mettre à jour ce tableau qu'en supprimant toutes les lignes à chaque mise à jour (lent)
	//			-> Les objectifs sont toujours les mêmes au cours du jeu, ce qui peut être un avantage
	table->clearRows();

	// Parcours tous les objectifs de la partie actuelle
	const core::list<ObjectiveData>& objectives = system.getObjectives().getObjectives();
	for (core::list<ObjectiveData>::ConstIterator it = objectives.begin(); it != objectives.end(); ++it)
	{
		// Ajoute cet objectif au tableau
		addObjectiveRow(*it);
	}

	// TODO : Calculer et indiquer automatiquement la largeur de chaque colonne suivant la taille du texte dans leurs cellules
	//			(sans jamais diminuer la taille des colonnes, toujours en les agrandissant si nécessaire)

	// Trie le tableau avec les nouvelles lignes ajoutées
	table->orderRows(-1, table->getActiveColumnOrdering());

	// Restaure la dernière ligne sélectionnée du tableau
	table->setSelected(lastSelectedRow);
}
void CGUIObjectivesWindow::addObjectiveRow(const ObjectiveData& objective)
{
	if (!table)
		return;

	// Obtient les couleurs pour les cellules du tableau
	const video::SColor defaultColor = Environment->getSkin() ? Environment->getSkin()->getColor(EGDC_BUTTON_TEXT) : video::SColor(255, 0, 0, 0);
	const video::SColor greenColor(defaultColor.getAlpha(), defaultColor.getRed(), 150, defaultColor.getBlue());
	const video::SColor redColor(defaultColor.getAlpha(), 200, defaultColor.getGreen(), defaultColor.getBlue());

	// Ajoute une ligne pour cet objectif
	const u32 row = table->addRow(table->getRowCount());
	u32 column = 0;

	// Indique les valeurs des cellules de cette ligne :

	// Description de l'objectif
	{
		table->setCellText(row, column, objective.getObjectiveDescription());
	} column++;

	// Type
	{
		table->setCellText(row, column, (objective.winObjective ? L"Gagnant" : L"Eliminatoire"));
	} column++;

	// Vérifié
	{
		const bool verifiedObjective = (system.getObjectives().verifyObjective(objective));
		table->setCellText(row, column, (verifiedObjective ? L"Oui" : L"Non"), (verifiedObjective ? greenColor : redColor));
	} column++;
}
