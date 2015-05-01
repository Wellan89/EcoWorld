#include "CGUIReductableWindow.h"

CGUIReductableWindow::CGUIReductableWindow(IGUIEnvironment* environment, IGUIElement* parent, int id, const core::recti& rectangle)
 : CGUIResizableWindow(environment, parent, id, rectangle),
 windowText(""), maximizedDimensions(rectangle.getSize()), isReducted(false), dragStartPosition(-1, -1), minSize(0, 0)
{
#ifdef _DEBUG
	setDebugName("CGUIReductableWindow");
#endif

	// Met � jour le texte de la fen�tre avec les symboles de r�duction/agrandissement
	setText(L"");
}
bool CGUIReductableWindow::OnEvent(const SEvent& event)
{
	if (event.EventType == EET_MOUSE_INPUT_EVENT && isEnabled())
	{
		if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN)
		{
			// On retient la position actuelle de la fen�tre
			dragStartPosition = AbsoluteRect.UpperLeftCorner;
		}
		else if (event.MouseInput.Event == EMIE_LMOUSE_LEFT_UP)
		{
			// V�rifie que l'utilisateur n'a pas d�plac� la fen�tre (si c'est le cas, on ne l'agrandit ni ne la r�duit pas)
			if (AbsoluteRect.UpperLeftCorner == dragStartPosition)
			{
				const int minimizeZoneX = Environment->getSkin()->getFont(EGDF_WINDOW)->getDimension(L"<__").Width + 
					Environment->getSkin()->getFont(EGDF_WINDOW)->getDimension(windowText.c_str()).Width;
				const int titleBarSizeY = 20;

				core::recti titleBarRect(AbsoluteRect);
				titleBarRect.LowerRightCorner.X = titleBarRect.UpperLeftCorner.X + minimizeZoneX;
				titleBarRect.LowerRightCorner.Y = titleBarRect.UpperLeftCorner.Y + titleBarSizeY;

				// V�rifie que le clic de la souris est sur la barre de titre de la fen�tre
				if (titleBarRect.isPointInside(core::vector2di(event.MouseInput.X, event.MouseInput.Y)))
					changeMinimizedState(); // Agrandit ou r�duit la fen�tre
			}
			else if (isReducted)
			{
				// Si la fen�tre est r�duite et que l'utilisateur a fini de la d�placer, on lui retire le focus
				//if (Environment->getFocus() == this)
					Environment->setFocus(NULL);
			}
		}
	}

	return CGUIResizableWindow::OnEvent(event);
}
void CGUIReductableWindow::OnWindowResize()
{
	// Indique la nouvelle taille agrandie de la fen�tre
	maximizedDimensions = AbsoluteRect.getSize();

	CGUIResizableWindow::OnWindowResize();
}
void CGUIReductableWindow::OnWindowMinimizedChanged()
{
	// Met � jour le texte de la fen�tre pour afficher les symboles de r�duction/agrandissement
	setText(windowText.c_str());
}
void CGUIReductableWindow::setText(const wchar_t* text)
{
	// Indique le nouveau titre de la fen�tre en y ajoutant les symboles d'agrandissement/de r�duction
	core::stringw newText = isReducted ? L"> " : L"< ";
	if (text)
		newText.append(text);
	CGUIResizableWindow::setText(newText.c_str());

	// Se souviens du titre d�sir�
	if (text)
		windowText = text;
	else
		windowText = L"";

	// Si la fen�tre est r�duite, on met � jour sa largeur
	if (isReducted)
	{
		const int minimizeZoneX = Environment->getSkin()->getFont(EGDF_WINDOW)->getDimension(L"<__").Width +
			Environment->getSkin()->getFont(EGDF_WINDOW)->getDimension(windowText.c_str()).Width;
		const int titleBarSizeY = 20;

		// Calcule la nouvelle taille de la fen�tre
		core::recti newWindowRect(RelativeRect);

		//newWindowRect.LowerRightCorner.X = newWindowRect.LowerRightCorner.X;					// Nouvelle largeur : Largeur normale de la fen�tre (Inutile)
		newWindowRect.LowerRightCorner.X = newWindowRect.UpperLeftCorner.X + minimizeZoneX;		// Nouvelle largeur : Largeur du texte de titre de la fen�tre (Commenter pour d�sactiver)
		newWindowRect.LowerRightCorner.Y = newWindowRect.UpperLeftCorner.Y + titleBarSizeY;		// Nouvelle hauteur : Hauteur de la barre de titre de la fen�tre

		// R�duit la fen�tre
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
		// Supprime la taille minimale de cette fen�tre
		minSize = MinSize;
		CGUIResizableWindow::setMinSize(core::dimension2du(1, 1));

		// Calcule la nouvelle taille de la fen�tre
		core::recti newWindowRect(RelativeRect);

		//newWindowRect.LowerRightCorner.X = newWindowRect.LowerRightCorner.X;					// Nouvelle largeur : Largeur normale de la fen�tre (Inutile)
		newWindowRect.LowerRightCorner.X = newWindowRect.UpperLeftCorner.X + minimizeZoneX;		// Nouvelle largeur : Largeur du texte de titre de la fen�tre (Commenter pour d�sactiver)
		newWindowRect.LowerRightCorner.Y = newWindowRect.UpperLeftCorner.Y + titleBarSizeY;		// Nouvelle hauteur : Hauteur de la barre de titre de la fen�tre

		// R�duit la fen�tre
		IGUIElement::setRelativePosition(newWindowRect);

		// Supprime le focus s'il appartient � ce menu
		//if (Environment->getFocus() == this)
			Environment->setFocus(NULL);

		// Masque l'ic�ne pour le redimensionnement
		if (resizableImage)
			resizableImage->setVisible(false);

		isReducted = true;
	}
	else
	{
		// Agrandit la fen�tre en conservant sa position et en lui donnant son ancienne dimension
		IGUIElement::setRelativePosition(core::recti(RelativeRect.UpperLeftCorner, maximizedDimensions));

		// Restaure la taille minimale de cette fen�tre
		CGUIResizableWindow::setMinSize(minSize);

		// Affiche l'ic�ne pour le redimensionnement
		if (resizableImage)
			resizableImage->setVisible(true);

		isReducted = false;
	}

	// Met � jour la zone disponible pour la zone cliente de la fen�tre
	updateClientRect();

	// Indique que la fen�tre a �t� minimis�e/agrandie
	OnWindowMinimizedChanged();
}
