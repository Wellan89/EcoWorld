#ifndef C_GUI_TEXTURED_SKIN_H
#define C_GUI_TEXTURED_SKIN_H

// Cette version a été modifiée : Conversion vers Irrlicht SVN 1.8.0-alpha, activation des rectangles de clipping des textures, changement des couleurs et des tailles par défaut du skin autorisé depuis le fichier de configuration du skin

//#include <IRR/irrlicht.h>
#include "global.h"


namespace irr {
namespace gui {


enum SKIN_TEXURE_COORD_ENUM
{
	ESTC_ENUM_INVALID = -1,
	ESTC_BUTTON_NORMAL_LEFT = 0,
	ESTC_BUTTON_NORMAL_MIDDLE,
	ESTC_BUTTON_NORMAL_RIGHT,
	ESTC_BUTTON_PRESSED_LEFT,
	ESTC_BUTTON_PRESSED_MIDDLE,
	ESTC_BUTTON_PRESSED_RIGHT,
	ESTC_CHECK_BOX_UNCHECKED,
	ESTC_CHECK_BOX_CHECKED,
	ESTC_MENU_PANE,
	ESTC_SUNKEN_PANE,
	ESTC_TAB_BODY,
	ESTC_TAB_BUTTON_ACTIVE,
	ESTC_TAB_BUTTON_INACTIVE,
	ESTC_TOOLBAR_LEFT,
	ESTC_TOOLBAR_MIDDLE,
	ESTC_TOOLBAR_RIGHT,
	ESTC_WINDOW_UPPER_LEFT_CORNER,
	ESTC_WINDOW_UPPER_RIGHT_CORNER,
	ESTC_WINDOW_LOWER_LEFT_CORNER,
	ESTC_WINDOW_LOWER_RIGHT_CORNER,
	ESTC_WINDOW_UPPER_EDGE,
	ESTC_WINDOW_LOWER_EDGE,
	ESTC_WINDOW_LEFT_EDGE,
	ESTC_WINDOW_RIGHT_EDGE,
	ESTC_WINDOW_INTERIOR,
	ESTC_WINDOW_TITLEBAR,
	ESTC_ENUM_COUNT
};


class CGUITexturedSkin : public IGUISkin
{
protected:
	// Name of this skin
	core::stringc skinName;
	core::stringc skinFilename;

	// Store pointer to required irrlicht classes
	IGUIEnvironment * pGuiEnv;
	video::IVideoDriver * pVideo;
	io::IFileSystem * pFileSystem;

	// The file name of the texture used
	core::stringc skinTextureFilename;
	// Pointer to the texture that is used by the current skin
	video::ITexture * pSkinTexture;
	// Pointer to the image that is used by the current skin
	video::IImage * pSkinImage;
	// The texture coordinates for the various components of the skin
	//core::array<core::rect<s32> > skinPartTypeRectArray;
	core::rect<s32> skinTexCoords[ESTC_ENUM_COUNT];

	// Whether to tile the texture for interior of the window, or to stretch it
	bool tileWindowInterior;

	// Font used by this skin
	// WARNING: This skin does not come with a default built-in font
	// Create and assign one to it before use.
	IGUIFont* fonts[EGDF_COUNT];
	IGUISpriteBank* SpriteBank;
	u32 icons[EGDI_COUNT];

	// Array of skin-defined colors
	video::SColor colors[EGDC_COUNT];
	// Array of skin-defined text
	core::stringw texts[EGDT_COUNT];
	// Array of skin-defined sizes
	s32 sizes[EGDS_COUNT];


	// This function will parse the skin definition xml file
	bool readSkinXml(const io::path& guiSkinXmlFile);

	// Case insensitive compare of current xml node name to supplied name
	bool isNodeName(io::IXMLReaderUTF8 * pXml, const c8 * pNodeName);

	// Finds a sub-node of the current xml node that has a name specified in the
	// nodeList array. currNodeDepth is the number of sublevels from the starting
	// node, used for repeatedly calling this function to parse an entire set of
	// nodes
	// Returns the nodeList array index of the next node name that is found
	s32 findInterestedSubNode(io::IXMLReaderUTF8 * pXml,
								   const core::array<c8*>& nodeList,
								   s32 & currNodeDepth);

	// For converting a string value of the current node from x1,y1,x2,y2 format
	// into a rect, that is returned
	core::rect<s32> decodeImageCoord(io::IXMLReaderUTF8 * pXml);

	video::SColor decodeColor(io::IXMLReaderUTF8 * pXml);	// Ajouté : Format : a,r,g,b
	u32 decodeSize(io::IXMLReaderUTF8 * pXml);				// Ajouté : Format : s

	virtual void SetSkinAlpha( u32 a );

public:
	CGUITexturedSkin(IGUIEnvironment * pEnv, io::IFileSystem * ifs);
	virtual ~CGUITexturedSkin();

	// Assign a new skin definition to this skin. Can be done during runtime to
	// change the skin.
	bool setSkin(const io::path& guiSkinXmlFile);

	// Implementation of IGUISkin interface
	virtual void draw3DButtonPanePressed(IGUIElement*element,
										 const core::rect<s32> &rect,
										 const core::rect<s32> *clip=0);

	virtual void draw3DButtonPaneStandard(IGUIElement *element,
										  const core::rect<s32> &rect,
										  const core::rect<s32> *clip=0);

	virtual void draw3DMenuPane(IGUIElement *element,
								const core::rect<s32> &rect,
								const core::rect<s32> *clip=0);

	virtual void draw3DSunkenPane(IGUIElement *element,
								  video::SColor bgcolor,
								  bool flat, bool fillBackGround,
								  const core::rect<s32> &rect,
								  const core::rect<s32> *clip=0);

	virtual void draw3DTabBody(IGUIElement *element, bool border, bool background,
							   const core::rect<s32> &rect,
							   const core::rect<s32> *clip=0, s32 tabHeight = -1, gui::EGUI_ALIGNMENT alignment=EGUIA_UPPERLEFT);	// Fonction modifiée pour Irrlicht 1.7.2

	virtual void draw3DTabButton(IGUIElement *element, bool active,
								 const core::rect<s32> &rect,
								 const core::rect<s32> *clip=0, gui::EGUI_ALIGNMENT alignment=EGUIA_UPPERLEFT);	// Fonction modifiée pour Irrlicht 1.7.2

	virtual void draw3DToolBar(IGUIElement *element,
							   const core::rect<s32> &rect,
							   const core::rect<s32> *clip=0);

	virtual core::rect<s32> draw3DWindowBackground(IGUIElement *element,
															 bool drawTitleBar,
															 video::SColor titleBarColor,
															 const core::rect<s32> &rect,
															 const core::rect<s32> *clip=0,
															core::rect<s32>* checkClientArea=0);	// Fonction modifiée pour Irrlicht 1.7.2

	virtual video::SColor getColor(EGUI_DEFAULT_COLOR color) const;
	virtual void setColor(EGUI_DEFAULT_COLOR which, video::SColor newColor);

	virtual s32 getSize(EGUI_DEFAULT_SIZE size) const;
	virtual void setSize(EGUI_DEFAULT_SIZE which, s32 size);

	virtual const wchar_t * getDefaultText(EGUI_DEFAULT_TEXT text) const;
	virtual void setDefaultText(EGUI_DEFAULT_TEXT which, const wchar_t *newText);

	virtual IGUIFont * getFont(EGUI_DEFAULT_FONT which=EGDF_DEFAULT) const;
	virtual void setFont(IGUIFont *font, EGUI_DEFAULT_FONT which);

	virtual IGUISpriteBank* getSpriteBank() const;
	virtual void setSpriteBank(IGUISpriteBank* bank);

	virtual u32 getIcon(EGUI_DEFAULT_ICON icon) const;
	virtual void setIcon(EGUI_DEFAULT_ICON icon, u32 index);

	virtual void drawIcon(IGUIElement* element, EGUI_DEFAULT_ICON icon,
		const core::position2di position, u32 starttime=0, u32 currenttime=0,
		bool loop=false, const core::rect<s32>* clip=0);
	virtual void draw2DRectangle(IGUIElement* element, const video::SColor &color,
		const core::rect<s32>& pos, const core::rect<s32>* clip = 0);

	virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const;
	virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options);
};


}	// end namespace gui
}	// end namespace irr


#endif
