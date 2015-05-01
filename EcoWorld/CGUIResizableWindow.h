// Fen�tre redimensionnable bas�e sur la classe CGUIWindow d'Irrlicht 1.7.1

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

	// Appel� si un �v�nement arrive
	virtual bool OnEvent(const SEvent& event);

	// Doit �tre surcharg� par le parent de cette fen�tre pour �tre eficace :
	// Appel� lorsque la fen�tre a �t� redimensionn�e : replace tous les �l�ments de la fen�tre � leur position et taille normale
	virtual void OnWindowResize();

protected:
	IGUIImage* resizableImage;

	core::vector2di mousePosResizingStart;	// Indique la position de la souris au d�but du redimensionnement, si diff�rent de (-1, -1), un redimensionnement est en cours
	core::dimension2di windowStartSize;

	// Place l'ic�ne pour le redimensionnement en bas � droite de la fen�tre
	void recalculateResizableImagePos();

	// Recalcule la nouvelle taille de la fen�tre suivant la position actuelle de la souris
	void recalculateWindowSize(const core::vector2di& mousePos);

public:
	// Surcharges de IGUIElement (mais non virtuelles !!!) pour que les classes parents de celle-ci g�rent bien le redimensionnement de leur fen�tre principale :

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
