#include "CGUITexturedModalScreen.h"

CGUITexturedModalScreen::CGUITexturedModalScreen(IGUIElement* parent, int id, IGUIEnvironment* environment, IGUIElement* child,
												 video::ITexture* backgroundTexture, bool useAlphaChannel, bool scale)
 : CGUIModalScreen(environment, parent, id), backgroundImage(NULL)
{
#ifdef _DEBUG
	setDebugName("CGUITexturedModalScreen");
#endif

	// Ajoute l'enfant sp�cifi� � nos enfants
	if (child)
		addChild(child);

	// Cr�e l'image de fond
	backgroundImage = environment->addImage(backgroundTexture, core::vector2di(0, 0), useAlphaChannel, this);

	if (scale || !backgroundTexture)
	{
		// Indique � l'image de fond qu'elle doit occuper tout l'�cran (la m�me place que cet �lement)
		backgroundImage->setRelativePosition(RelativeRect);

		// Indique � l'image qu'elle doit �tre agrandie
		if (scale)
			backgroundImage->setScaleImage(scale);
	}
	else
	{
		// Centre l'image sur l'�cran
		const core::dimension2du imageHalfSize = (backgroundTexture->getOriginalSize() / 2);
		const core::vector2di screenCenter = RelativeRect.getCenter();
		backgroundImage->setRelativePosition(core::recti(
			screenCenter.X - imageHalfSize.Width, screenCenter.Y - imageHalfSize.Height,
			screenCenter.X + imageHalfSize.Width, screenCenter.Y + imageHalfSize.Height));
	}

	// Masque l'image de fond
	backgroundImage->setVisible(false);

	// On se replace au premier plan (l'image de fond qui vient d'�tre cr��e est pass�e devant)
	if (parent)
		parent->bringToFront(this);
}
void CGUITexturedModalScreen::draw()
{
	if (!isVisible())
		return;

	// Affiche l'image de fond pour pouvoir la dessiner, puis la masque aussit�t
	if (backgroundImage)
	{
		backgroundImage->setVisible(true);
		backgroundImage->draw();
		backgroundImage->setVisible(false);
	}

	CGUIModalScreen::draw();
}
