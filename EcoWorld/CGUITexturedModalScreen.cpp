#include "CGUITexturedModalScreen.h"

CGUITexturedModalScreen::CGUITexturedModalScreen(IGUIElement* parent, int id, IGUIEnvironment* environment, IGUIElement* child,
												 video::ITexture* backgroundTexture, bool useAlphaChannel, bool scale)
 : CGUIModalScreen(environment, parent, id), backgroundImage(NULL)
{
#ifdef _DEBUG
	setDebugName("CGUITexturedModalScreen");
#endif

	// Ajoute l'enfant spécifié à nos enfants
	if (child)
		addChild(child);

	// Crée l'image de fond
	backgroundImage = environment->addImage(backgroundTexture, core::vector2di(0, 0), useAlphaChannel, this);

	if (scale || !backgroundTexture)
	{
		// Indique à l'image de fond qu'elle doit occuper tout l'écran (la même place que cet élement)
		backgroundImage->setRelativePosition(RelativeRect);

		// Indique à l'image qu'elle doit être agrandie
		if (scale)
			backgroundImage->setScaleImage(scale);
	}
	else
	{
		// Centre l'image sur l'écran
		const core::dimension2du imageHalfSize = (backgroundTexture->getOriginalSize() / 2);
		const core::vector2di screenCenter = RelativeRect.getCenter();
		backgroundImage->setRelativePosition(core::recti(
			screenCenter.X - imageHalfSize.Width, screenCenter.Y - imageHalfSize.Height,
			screenCenter.X + imageHalfSize.Width, screenCenter.Y + imageHalfSize.Height));
	}

	// Masque l'image de fond
	backgroundImage->setVisible(false);

	// On se replace au premier plan (l'image de fond qui vient d'être créée est passée devant)
	if (parent)
		parent->bringToFront(this);
}
void CGUITexturedModalScreen::draw()
{
	if (!isVisible())
		return;

	// Affiche l'image de fond pour pouvoir la dessiner, puis la masque aussitôt
	if (backgroundImage)
	{
		backgroundImage->setVisible(true);
		backgroundImage->draw();
		backgroundImage->setVisible(false);
	}

	CGUIModalScreen::draw();
}
