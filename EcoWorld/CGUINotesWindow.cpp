#include "CGUINotesWindow.h"

CGUINotesWindow::CGUINotesWindow(IGUIEnvironment *environment, IGUIElement *parent, int id, core::recti rectangle)
 : CGUIReductableWindow(environment, parent, id, rectangle), notesEditBox(0)
{
#ifdef _DEBUG
	setDebugName("CGUIInformationsWindow");
#endif

	setText(L"Notes");

	// Masque le bouton pour fermer la fen�tre
	if (CloseButton)
		CloseButton->setVisible(false);

	// Calcule la position de la zone de texte
	const core::recti notesEditBoxRect(
		ClientRect.UpperLeftCorner.X + 10, ClientRect.UpperLeftCorner.Y + 10,
		ClientRect.LowerRightCorner.X - 15, ClientRect.LowerRightCorner.Y - 15);

	// Cr�e la zone de texte contenant les notes
	notesEditBox = environment->addEditBox(L"", notesEditBoxRect, true, this);
	notesEditBox->setTextAlignment(gui::EGUIA_UPPERLEFT, gui::EGUIA_UPPERLEFT);
	notesEditBox->setAutoScroll(true);
	notesEditBox->setMultiLine(true);
	notesEditBox->setWordWrap(true);
	//notesEditBox->setMax(60000);	// Moins de libert� pour le joueur si on limite le nombre de caract�res
	notesEditBox->setMax(0);

	// Indique la taille minimale de cette fen�tre
	setMinSize(core::dimension2du(150, 100));
}
void CGUINotesWindow::OnWindowResize()
{
	// On ne g�re ici que le cas o� la fen�tre n'est pas r�duite
	if (!isReducted)
	{
		// Replace la zone de notes
		const core::recti notesEditBoxRect(
			ClientRect.UpperLeftCorner.X + 10, ClientRect.UpperLeftCorner.Y + 10,
			ClientRect.LowerRightCorner.X - 15, ClientRect.LowerRightCorner.Y - 15);

		notesEditBox->setRelativePosition(notesEditBoxRect);
	}

	CGUIReductableWindow::OnWindowResize();
}
