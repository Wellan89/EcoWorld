// Copyright (C) 2002-2011 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

// Pris de Irrlicht SVN 1.8.0-alpha (Cette version a �t� l�g�rement modifi�e : ajout d'un accesseur pour le texte statique de la fen�tre, ajout d'une option permettant d'ajouter un fond modal obscurci � cette fen�tre)

#ifndef __C_GUI_MESSAGE_BOX_H_INCLUDED__
#define __C_GUI_MESSAGE_BOX_H_INCLUDED__

#include "global.h"
#include "CGUIWindow.h"
using namespace gui;
class CGUITexturedModalScreen;

//#include "IrrCompileConfig.h"
//#ifdef _IRR_COMPILE_WITH_GUI_

//#include "CGUIWindow.h"
//#include "IGUIStaticText.h"
//#include "IGUIImage.h"
//#include "irrArray.h"

//namespace irr
//{
//namespace gui
//{
	class CGUIMessageBox : public CGUIWindow
	{
	public:
		//! constructor
		CGUIMessageBox(IGUIEnvironment* environment, const wchar_t* caption,
			const wchar_t* text, s32 flag,
			IGUIElement* parent, s32 id, core::rect<s32> rectangle, video::ITexture* image,
			bool addTexturedModalScreen);

		//! destructor
		virtual ~CGUIMessageBox();

		//! called if an event happened.
		virtual bool OnEvent(const SEvent& event);

		////! Writes attributes of the element.
		//virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const;

		////! Reads attributes of the element
		//virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options);

	private:

		void refreshControls();
		void setButton(IGUIButton*& button, bool isAvailable, const core::rect<s32> & btnRect, const wchar_t * text, IGUIElement*& focusMe);

		IGUIButton* OkButton;
		IGUIButton* CancelButton;
		IGUIButton* YesButton;
		IGUIButton* NoButton;
		IGUIStaticText* StaticText;
		IGUIImage * Icon;
		video::ITexture * IconTexture;

		s32 Flags;
		core::stringw MessageText;
		bool Pressed;



	public:
		// Ajout� :

		IGUIStaticText* getStaticText()	{ return StaticText; }

		// G�re automatiquement l'ajout d'une fen�tre de message � la GUI du jeu
		static CGUIMessageBox* addMessageBox(IGUIEnvironment* environment, const wchar_t* caption, const wchar_t* text, s32 flags, IGUIElement* parent, bool addTexturedModalScreen)
		{
			// Calcule la position de cette message box : au centre de son parent
			const core::recti parentAbsPos = parent->getAbsolutePosition();
			const core::dimension2du parentHalfDim(parentAbsPos.getWidth() / 2, parentAbsPos.getHeight() / 2);
			const core::recti windowRect(parentHalfDim.Width - 1, parentHalfDim.Height - 1, parentHalfDim.Width + 1, parentHalfDim.Height + 1);

			CGUIMessageBox* const messageBox = new CGUIMessageBox(environment, caption, text, flags, parent,
				-1, windowRect, NULL, addTexturedModalScreen);

			// Emp�che cette fen�tre d'�tre coup�e par le rectangle de son parent :
			// elle peut ainsi �tre dessin�e sur tout l'�cran (mais pas d�plac�e sur tout l'�cran !)
			messageBox->setNotClipped(true);

			// Indique cette fen�tre est un sous-�l�ment d'un menu
			messageBox->setSubElement(true);

			messageBox->drop();
			return messageBox;
		}
	};

//} // end namespace gui
//} // end namespace irr

//#endif // _IRR_COMPILE_WITH_GUI_

#endif
