// Fenêtre redimensionnable basée sur la classe CGUIWindow d'Irrlicht 1.7.1

#ifndef DEF_C_GUI_RESIZABLE_WINDOW
#define DEF_C_GUI_RESIZABLE_WINDOW

#include "global.h"
#include "CGUIWindow.h"

using namespace gui;

class CGUIResizableWindow : public CGUIWindow
{
public:
	// Constructeur et destructeur
	CGUIResizableWindow(IGUIEnvironment* environment, IGUIElement* parent, int id, const core::recti& rectangle);
	virtual ~CGUIResizableWindow();

	// Appelé si un évènement arrive
	virtual bool OnEvent(const SEvent& event);

	// Doit être surchargé par le parent de cette fenêtre pour être eficace :
	// Appelé lorsque la fenêtre a été redimensionnée : replace tous les éléments de la fenêtre à leur position et taille normale
	virtual void OnWindowResize();

protected:
	IGUIImage* resizableImage;

	core::vector2di mousePosResizingStart;	// Indique la position de la souris au début du redimensionnement, si différent de (-1, -1), un redimensionnement est en cours
	core::dimension2di windowStartSize;

	// Place l'icône pour le redimensionnement en bas à droite de la fenêtre
	void recalculateResizableImagePos();

	// Recalcule la nouvelle taille de la fenêtre suivant la position actuelle de la souris
	void recalculateWindowSize(const core::vector2di& mousePos);

public:
	// Surcharges de IGUIElement (mais non virtuelles !!!) pour que les classes parents de celle-ci gèrent bien le redimensionnement de leur fenêtre principale :

	//! Sets the relative rectangle of this element.
	/** \param r The absolute position to set */
	void setRelativePosition(const core::recti& r)
	{
		IGUIElement::setRelativePosition(r);
		updateClientRect();
		OnWindowResize();
	}
	//! Sets the relative rectangle of this element, maintaining its current width and height
	/** \param position The new relative position to set. Width and height will not be changed. */
	void setRelativePosition(const core::vector2di& position)
	{
		IGUIElement::setRelativePosition(position);
		updateClientRect();
		OnWindowResize();
	}
	//! Sets the relative rectangle of this element as a proportion of its parent's area.
	/** \note This method used to be 'void setRelativePosition(const core::rect<f32>& r)'
	\param r  The rectangle to set, interpreted as a proportion of the parent's area.
	Meaningful values are in the range [0...1], unless you intend this element to spill
	outside its parent. */
	void setRelativePositionProportional(const core::rectf& r)
	{
		IGUIElement::setRelativePositionProportional(r);
		updateClientRect();
		OnWindowResize();
	}


	// Accesseurs inline
	virtual IGUIImage* getResizableImage() const { return resizableImage; };
};

#endif
