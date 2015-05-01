#include "CGUIResizableWindow.h"

CGUIResizableWindow::CGUIResizableWindow(IGUIEnvironment* environment, IGUIElement* parent, int id, const core::recti& rectangle)
: CGUIWindow(environment, parent, id, rectangle), resizableImage(0), mousePosResizingStart(-1, -1), windowStartSize(0, 0)
{
#ifdef _DEBUG
	setDebugName("CGUIResizableWindow");
#endif

	// Cr�e l'ic�ne pour le redimensionnement
	resizableImage = environment->addImage(environment->getVideoDriver()->getTexture("resize icon.bmp"),
		core::vector2di(0, 0), true, this);
	resizableImage->grab();

	// Place l'ic�ne pour le redimensionnement en bas de la fen�tre
	recalculateResizableImagePos();
}
CGUIResizableWindow::~CGUIResizableWindow()
{
	if (resizableImage)
		resizableImage->drop();
}
bool CGUIResizableWindow::OnEvent(const SEvent& event)
{
	// On ne g�re ici que les �v�nements venant de la souris
	if (event.EventType == EET_MOUSE_INPUT_EVENT && isEnabled())
	{
		const core::vector2di mousePos(event.MouseInput.X, event.MouseInput.Y);

		// Si le bouton gauche de la souris vient d'�tre press� et que la souris est sur l'ic�ne pour le redimensionnement
		if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN
			&& resizableImage->getAbsolutePosition().isPointInside(mousePos))
		{
			// On commence le redimensionnement
			mousePosResizingStart.set(mousePos);
			windowStartSize = RelativeRect.getSize();

			// Evite le d�placement de la fen�tre en m�me temps
			return true;
		}
		// Si le bouton gauche de la souris vient d'�tre relach�
		else if (event.MouseInput.Event == EMIE_LMOUSE_LEFT_UP)
		{
			// On recalcule la taille finale de la fen�tre et on arr�te le redimensionnement
			recalculateWindowSize(core::vector2di(mousePos));
			mousePosResizingStart.set(-1, -1);
		}
		// Si la souris a boug� et qu'on est en train d'effectuer un redimensionnement
		else if (event.MouseInput.Event == EMIE_MOUSE_MOVED && mousePosResizingStart != core::vector2di(-1, -1))
		{
			// Si le bouton gauche de la souris n'est pas press�, on termine le redimensionnement
			if (!event.MouseInput.isLeftPressed())
			{
				recalculateWindowSize(core::vector2di(mousePos));
				mousePosResizingStart.set(-1, -1);
			}
			else
			{
				// On calcule la taille temporaire de la fen�tre
				recalculateWindowSize(core::vector2di(mousePos));
			}
		}
	}

	return CGUIWindow::OnEvent(event);
}
void CGUIResizableWindow::OnWindowResize()
{
	// Replace l'ic�ne pour le redimensionnement
	recalculateResizableImagePos();

	// Replace les ic�nes de base de la fen�tre :
	{
		// Constantes d'apr�s CGUIWindow.cpp (Constructeur CGUIWindow::CGUIWindow(...))
		const int buttonw = 15;
		int posx = RelativeRect.getWidth() - buttonw - 4;

		// CloseButton
		if (CloseButton && CloseButton->isVisible())
		{
			CloseButton->setRelativePosition(core::recti(posx, 3, posx + buttonw, 3 + buttonw));
			posx -= buttonw + 2;
		}

		// RestoreButton
		if (RestoreButton && RestoreButton->isVisible())
		{
			RestoreButton->setRelativePosition(core::recti(posx, 3, posx + buttonw, 3 + buttonw));
			posx -= buttonw + 2;
		}

		// MinButton
		if (MinButton && MinButton->isVisible())
		{
			MinButton->setRelativePosition(core::recti(posx, 3, posx + buttonw, 3 + buttonw));
			posx -= buttonw + 2;
		}
	}
}
void CGUIResizableWindow::recalculateResizableImagePos()
{
	// Constantes utilis�es pour le calcul de la nouvelle position de l'ic�ne
	const core::dimension2di windowSize = RelativeRect.getSize();
	const core::dimension2di iconSize(11, 11);
	const int ecart = 2;	// L'�cart de l'ic�ne par rapport aux bords de la fen�tre

	// Indique la nouvelle position de l'ic�ne
	resizableImage->setRelativePosition(core::vector2di(
		windowSize.Width - iconSize.Width - ecart,
		windowSize.Height - iconSize.Height - ecart));
}
void CGUIResizableWindow::recalculateWindowSize(const core::vector2di& mousePos)
{
	// On ne peut pas calculer la nouvelle taille de la fen�tre si on n'est pas en train d'effectuer un redimnesionnement
	if (mousePosResizingStart == core::vector2di(-1, -1))
		return;

	// Calcule la nouvelle taille que doit avoir la fen�tre
	const core::dimension2di newWindowSize(
		windowStartSize.Width + mousePos.X - mousePosResizingStart.X,
		windowStartSize.Height + mousePos.Y - mousePosResizingStart.Y);

	// Indique la nouvelle taille de la fen�tre
	setRelativePosition(core::recti(RelativeRect.UpperLeftCorner, newWindowSize));

	// Met � jour la zone disponible pour la zone cliente de la fen�tre
	updateClientRect();

	// Appelle le callback indiquant que la fen�tre a �t� redimensionn�e
	OnWindowResize();
}
