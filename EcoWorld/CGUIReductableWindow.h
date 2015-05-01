// Fenêtre réductible et redimensionnable basée sur la classe CGUIWindow d'Irrlicht 1.7.1

#ifndef DEF_C_GUI_REDUCTABLE_WINDOW
#define DEF_C_GUI_REDUCTABLE_WINDOW

#include "global.h"
#include "CGUIResizableWindow.h"

using namespace gui;

class CGUIReductableWindow : public CGUIResizableWindow
{
public:
	// Constructeur
	CGUIReductableWindow(IGUIEnvironment* environment, IGUIElement* parent, int id, const core::recti& rectangle);

	// Appelé si un évènement arrive
	virtual bool OnEvent(const SEvent& event);

	// Appelé lorsque la fenêtre a été redimensionnée
	virtual void OnWindowResize();

	// Appelé lorsque la fenêtre a été réduite ou agrandie
	virtual void OnWindowMinimizedChanged();

	// Indique le nouveau texte de cette fenêtre
	virtual void setText(const wchar_t* text);

	// Indique la taille minimale de cette fenêtre
	virtual void setMinSize(core::dimension2du size);

	// Permet de réduire ou d'agrandir la fenêtre
	void changeMinimizedState();

protected:
	bool isReducted;						// True si la taille de la fenêtre est réduite à sa barre de titre (si elle est minimisée)
	core::dimension2du minSize;				// La taille minimale de la fenêtre lorsqu'elle est agrandie
	core::dimension2di maximizedDimensions;	// Les dimensions de la fenêtre lorsqu'elle est agrandie
	core::vector2di dragStartPosition;		// La position de départ lorsque l'utilisateur a cliqué sur la fenêtre (si il a déplacé la fenêtre, on ne l'agrandit ni ne la réduit pas)
	core::stringw windowText;				// Le texte normal de la fenêtre (sans les symboles d'agrandissement ou de réduction)

public:
	// Accesseurs inline :

	// Retourne si la fenêtre est minimisée
	bool isMinimized() const				{ return isReducted; }

	// Minimise ou agrandis la fenêtre
	void setMinimized(bool minimized)		{ if (isReducted != minimized) { changeMinimizedState(); } }

	// Retourne la taille de la fenêtre lorsqu'elle est agrandie
	core::recti getMaximizedRect() const	{ return core::recti(RelativeRect.UpperLeftCorner, maximizedDimensions); }
};

#endif
