// Fen�tre r�ductible et redimensionnable bas�e sur la classe CGUIWindow d'Irrlicht 1.7.1

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

	// Appel� si un �v�nement arrive
	virtual bool OnEvent(const SEvent& event);

	// Appel� lorsque la fen�tre a �t� redimensionn�e
	virtual void OnWindowResize();

	// Appel� lorsque la fen�tre a �t� r�duite ou agrandie
	virtual void OnWindowMinimizedChanged();

	// Indique le nouveau texte de cette fen�tre
	virtual void setText(const wchar_t* text);

	// Indique la taille minimale de cette fen�tre
	virtual void setMinSize(core::dimension2du size);

	// Permet de r�duire ou d'agrandir la fen�tre
	void changeMinimizedState();

protected:
	bool isReducted;						// True si la taille de la fen�tre est r�duite � sa barre de titre (si elle est minimis�e)
	core::dimension2du minSize;				// La taille minimale de la fen�tre lorsqu'elle est agrandie
	core::dimension2di maximizedDimensions;	// Les dimensions de la fen�tre lorsqu'elle est agrandie
	core::vector2di dragStartPosition;		// La position de d�part lorsque l'utilisateur a cliqu� sur la fen�tre (si il a d�plac� la fen�tre, on ne l'agrandit ni ne la r�duit pas)
	core::stringw windowText;				// Le texte normal de la fen�tre (sans les symboles d'agrandissement ou de r�duction)

public:
	// Accesseurs inline :

	// Retourne si la fen�tre est minimis�e
	bool isMinimized() const				{ return isReducted; }

	// Minimise ou agrandis la fen�tre
	void setMinimized(bool minimized)		{ if (isReducted != minimized) { changeMinimizedState(); } }

	// Retourne la taille de la fen�tre lorsqu'elle est agrandie
	core::recti getMaximizedRect() const	{ return core::recti(RelativeRect.UpperLeftCorner, maximizedDimensions); }
};

#endif
