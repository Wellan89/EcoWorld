#include "CGUIReductableWindow.h"

CGUIReductableWindow::CGUIReductableWindow(IGUIEnvironment* environment, IGUIElement* parent, int id, const core::recti& rectangle)
 : CGUIResizableWindow(environment, parent, id, rectangle),
 windowText(""), maximizedDimensions(rectangle.getSize()), isReducted(false), dragStartPosition(-1, -1), minSize(0, 0)
{
#ifdef _DEBUG
	setDebugName("CGUIReductableWindow");
#endif

	// Met à jour le texte de la fenêtre avec les symboles de réduction/agrandissement
	setText(L"");
}
bool CGUIReductableWindow::OnEvent(const SEvent& event)
{
	if (event.EventType == EET_MOUSE_INPUT_EVENT && isEnabled())
	{
		if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN)
		{
			// On retient la position actuelle de la fenêtre
			dragStartPosition = AbsoluteRect.UpperLeftCorner;
		}
		else if (event.MouseInput.Event == EMIE_LMOUSE_LEFT_UP)
		{
			// Vérifie que l'utilisateur n'a pas déplacé la fenêtre (si c'est le cas, on ne l'agrandit ni ne la réduit pas)
			if (AbsoluteRect.UpperLeftCorner == dragStartPosition)
			{
				const int minimizeZoneX = Environment->getSkin()->getFont(EGDF_WINDOW)->getDimension(L"<__").Width + 
					Environment->getSkin()->getFont(EGDF_WINDOW)->getDimension(windowText.c_str()).Width;
				const int titleBarSizeY = 20;

				core::recti titleBarRect(AbsoluteRect);
				titleBarRect.LowerRightCorner.X = titleBarRect.UpperLeftCorner.X + minimizeZoneX;
				titleBarRect.LowerRightCorner.Y = titleBarRect.UpperLeftCorner.Y + titleBarSizeY;

				// Vérifie que le clic de la souris est sur la barre de titre de la fenêtre
				if (titleBarRect.isPointInside(core::vector2di(event.MouseInput.X, event.MouseInput.Y)))
					changeMinimizedState(); // Agrandit ou réduit la fenêtre
			}
			else if (isReducted)
			{
				// Si la fenêtre est réduite et que l'utilisateur a fini de la déplacer, on lui retire le focus
				//if (Environment->getFocus() == this)
					Environment->setFocus(NULL);
			}
		}
	}

	return CGUIResizableWindow::OnEvent(event);
}
void CGUIReductableWindow::OnWindowResize()
{
	// Indique la nouvelle taille agrandie de la fenêtre
	maximizedDimensions = AbsoluteRect.getSize();

	CGUIResizableWindow::OnWindowResize();
}
void CGUIReductableWindow::OnWindowMinimizedChanged()
{
	// Met à jour le texte de la fenêtre pour afficher les symboles de réduction/agrandissement
	setText(windowText.c_str());
}
void CGUIReductableWindow::setText(const wchar_t* text)
{
	// Indique le nouveau titre de la fenêtre en y ajoutant les symboles d'agrandissement/de réduction
	core::stringw newText = isReducted ? L"> " : L"< ";
	if (text)
		newText.append(text);
	CGUIResizableWindow::setText(newText.c_str());

	// Se souviens du titre désiré
	if (text)
		windowText = text;
	else
		windowText = L"";

	// Si la fenêtre est réduite, on met à jour sa largeur
	if (isReducted)
	{
		const int minimizeZoneX = Environment->getSkin()->getFont(EGDF_WINDOW)->getDimension(L"<__").Width +
			Environment->getSkin()->getFont(EGDF_WINDOW)->getDimension(windowText.c_str()).Width;
		const int titleBarSizeY = 20;

		// Calcule la nouvelle taille de la fenêtre
		core::recti newWindowRect(RelativeRect);

		//newWindowRect.LowerRightCorner.X = newWindowRect.LowerRightCorner.X;					// Nouvelle largeur : Largeur normale de la fenêtre (Inutile)
		newWindowRect.LowerRightCorner.X = newWindowRect.UpperLeftCorner.X + minimizeZoneX;		// Nouvelle largeur : Largeur du texte de titre de la fenêtre (Commenter pour désactiver)
		newWindowRect.LowerRightCorner.Y = newWindowRect.UpperLeftCorner.Y + titleBarSizeY;		// Nouvelle hauteur : Hauteur de la barre de titre de la fenêtre

		// Réduit la fenêtre
		IGUIElement::setRelativePosition(newWindowRect);
	}
}
void CGUIReductableWindow::setMinSize(core::dimension2du size)
{
	minSize = size;
	if (!isReducted)
		CGUIResizableWindow::setMinSize(size);
}
void CGUIReductableWindow::changeMinimizedState()
{
	const int minimizeZoneX = Environment->getSkin()->getFont(EGDF_WINDOW)->getDimension(L"<__").Width +
		Environment->getSkin()->getFont(EGDF_WINDOW)->getDimension(windowText.c_str()).Width;
	const int titleBarSizeY = 19;

	if (!isReducted)
	{
		// Supprime la taille minimale de cette fenêtre
		minSize = MinSize;
		CGUIResizableWindow::setMinSize(core::dimension2du(1, 1));

		// Calcule la nouvelle taille de la fenêtre
		core::recti newWindowRect(RelativeRect);

		//newWindowRect.LowerRightCorner.X = newWindowRect.LowerRightCorner.X;					// Nouvelle largeur : Largeur normale de la fenêtre (Inutile)
		newWindowRect.LowerRightCorner.X = newWindowRect.UpperLeftCorner.X + minimizeZoneX;		// Nouvelle largeur : Largeur du texte de titre de la fenêtre (Commenter pour désactiver)
		newWindowRect.LowerRightCorner.Y = newWindowRect.UpperLeftCorner.Y + titleBarSizeY;		// Nouvelle hauteur : Hauteur de la barre de titre de la fenêtre

		// Réduit la fenêtre
		IGUIElement::setRelativePosition(newWindowRect);

		// Supprime le focus s'il appartient à ce menu
		//if (Environment->getFocus() == this)
			Environment->setFocus(NULL);

		// Masque l'icône pour le redimensionnement
		if (resizableImage)
			resizableImage->setVisible(false);

		isReducted = true;
	}
	else
	{
		// Agrandit la fenêtre en conservant sa position et en lui donnant son ancienne dimension
		IGUIElement::setRelativePosition(core::recti(RelativeRect.UpperLeftCorner, maximizedDimensions));

		// Restaure la taille minimale de cette fenêtre
		CGUIResizableWindow::setMinSize(minSize);

		// Affiche l'icône pour le redimensionnement
		if (resizableImage)
			resizableImage->setVisible(true);

		isReducted = false;
	}

	// Met à jour la zone disponible pour la zone cliente de la fenêtre
	updateClientRect();

	// Indique que la fenêtre a été minimisée/agrandie
	OnWindowMinimizedChanged();
}
