#include "CGUITexturedSkin.h"

// Cette version a été modifiée

using namespace irr;
using namespace core;
using namespace video;
using namespace io;
using namespace gui;


namespace irr {
namespace gui {


CGUITexturedSkin::CGUITexturedSkin(IGUIEnvironment * pEnv, io::IFileSystem * ifs)
{
	pGuiEnv = pEnv;
	pVideo = pEnv->getVideoDriver();
	pFileSystem = ifs;

	SpriteBank = 0;
	pSkinTexture = 0;
	pSkinImage = 0;

	IGUIFont* builtinfont = pEnv->getBuiltInFont();
	IGUIFontBitmap* bitfont = 0;
	if (builtinfont && builtinfont->getType() == EGFT_BITMAP)
		bitfont = (IGUIFontBitmap*)builtinfont;
	if (bitfont)
		setSpriteBank( bitfont->getSpriteBank() );

	for (u32 i=0; i<EGDF_COUNT; ++i)
		fonts[i] = 0;

	/*
	colors[EGDC_3D_DARK_SHADOW]   = video::SColor(101,50,50,50);
	colors[EGDC_3D_SHADOW]        = video::SColor(101,130,130,130);
	colors[EGDC_3D_FACE]          = video::SColor(101,210,210,210);
	colors[EGDC_3D_HIGH_LIGHT]    = video::SColor(101,255,255,255);
	colors[EGDC_3D_LIGHT]         =	video::SColor(101,210,210,210);
	colors[EGDC_ACTIVE_BORDER]    = video::SColor(101,16,14,115);
	colors[EGDC_ACTIVE_CAPTION]   = video::SColor(200,255,255,255);
	colors[EGDC_APP_WORKSPACE]    = video::SColor(101,100,100,100);
	colors[EGDC_BUTTON_TEXT]      = video::SColor(240,10,10,10);
	colors[EGDC_GRAY_TEXT]        = video::SColor(240,130,130,130);
	colors[EGDC_HIGH_LIGHT]       = video::SColor(101,8,36,107);
	colors[EGDC_HIGH_LIGHT_TEXT]  = video::SColor(240,255,255,255);
	colors[EGDC_INACTIVE_BORDER]  = video::SColor(101,165,165,165);
	colors[EGDC_INACTIVE_CAPTION] = video::SColor(101,210,210,210);
	colors[EGDC_TOOLTIP]          = video::SColor(200,0,0,0);
	colors[EGDC_TOOLTIP_BACKGROUND]=video::SColor(200,255,255,225);
	colors[EGDC_SCROLLBAR]        = video::SColor(101,230,230,230);
	colors[EGDC_WINDOW]           = video::SColor(101,255,255,255);
	colors[EGDC_WINDOW_SYMBOL]    = video::SColor(200,10,10,10);
	colors[EGDC_ICON]             = video::SColor(200,255,255,255);
	colors[EGDC_ICON_HIGH_LIGHT]  = video::SColor(200,8,36,107);
	*/

	// Modifié : valeurs Alpha réinitialisées : à 255 : Valeur bloquée ; à 0 : Valeur réglable par l'utilisateur :
	colors[EGDC_3D_DARK_SHADOW]   = video::SColor(0,50,50,50);
	colors[EGDC_3D_SHADOW]        = video::SColor(0,130,130,130);
	colors[EGDC_3D_FACE]          = video::SColor(0,210,210,210);
	colors[EGDC_3D_HIGH_LIGHT]    = video::SColor(0,255,255,255);
	colors[EGDC_3D_LIGHT]         =	video::SColor(0,210,210,210);
	colors[EGDC_ACTIVE_BORDER]    = video::SColor(0,16,14,115);
	colors[EGDC_ACTIVE_CAPTION]   = video::SColor(255,255,255,255);
	colors[EGDC_APP_WORKSPACE]    = video::SColor(0,100,100,100);
	colors[EGDC_BUTTON_TEXT]      = video::SColor(255,10,10,10);
	colors[EGDC_GRAY_TEXT]        = video::SColor(255,130,130,130);
	colors[EGDC_HIGH_LIGHT]       = video::SColor(0,8,36,107);
	colors[EGDC_HIGH_LIGHT_TEXT]  = video::SColor(255,255,255,255);
	colors[EGDC_INACTIVE_BORDER]  = video::SColor(0,165,165,165);
	colors[EGDC_INACTIVE_CAPTION] = video::SColor(255,210,210,210);
	colors[EGDC_TOOLTIP]          = video::SColor(0,0,0,0);
	colors[EGDC_TOOLTIP_BACKGROUND]=video::SColor(0,255,255,225);
	colors[EGDC_SCROLLBAR]        = video::SColor(0,230,230,230);
	colors[EGDC_WINDOW]           = video::SColor(0,255,255,255);
	colors[EGDC_WINDOW_SYMBOL]    = video::SColor(0,10,10,10);
	colors[EGDC_ICON]             = video::SColor(0,255,255,255);
	colors[EGDC_ICON_HIGH_LIGHT]  = video::SColor(0,8,36,107);

	sizes[EGDS_SCROLLBAR_SIZE] = 14;
	sizes[EGDS_MENU_HEIGHT] = 30;
	sizes[EGDS_WINDOW_BUTTON_WIDTH] = 13;	// 15
	sizes[EGDS_CHECK_BOX_WIDTH] = 20;		// 18
	sizes[EGDS_MESSAGE_BOX_WIDTH] = 500;
	sizes[EGDS_MESSAGE_BOX_HEIGHT] = 200;
	sizes[EGDS_BUTTON_WIDTH] = 80;
	sizes[EGDS_BUTTON_HEIGHT] = 30;
	sizes[EGDS_TEXT_DISTANCE_X] = 2;
	sizes[EGDS_TEXT_DISTANCE_Y] = 0;
	// Ajouté (Manquants) : (d'après Irrlicht 1.7.2 : CGUISkin.cpp : EGST_WINDOWS_METALLIC)
	sizes[EGDS_TITLEBARTEXT_DISTANCE_X] = 2;
	sizes[EGDS_TITLEBARTEXT_DISTANCE_Y] = 0;
	sizes[EGDS_MESSAGE_BOX_GAP_SPACE] = 15;
	sizes[EGDS_MESSAGE_BOX_MIN_TEXT_WIDTH] = 0;
	sizes[EGDS_MESSAGE_BOX_MAX_TEXT_WIDTH] = 500;
	sizes[EGDS_MESSAGE_BOX_MIN_TEXT_HEIGHT] = 0;
	sizes[EGDS_MESSAGE_BOX_MAX_TEXT_HEIGHT] = 99999;

	texts[EGDT_MSG_BOX_OK] = L"OK";
	texts[EGDT_MSG_BOX_CANCEL] = L"Cancel";
	texts[EGDT_MSG_BOX_YES] = L"Yes";
	texts[EGDT_MSG_BOX_NO] = L"No";
	texts[EGDT_WINDOW_CLOSE] = L"Close";
	texts[EGDT_WINDOW_RESTORE] = L"Restore";
	texts[EGDT_WINDOW_MINIMIZE] = L"Minimize";
	texts[EGDT_WINDOW_MAXIMIZE] = L"Maximize";

	icons[EGDI_WINDOW_MAXIMIZE] = 225;
	icons[EGDI_WINDOW_RESTORE] = 226;
	icons[EGDI_WINDOW_CLOSE] = 227;
	icons[EGDI_WINDOW_MINIMIZE] = 228;
	icons[EGDI_CURSOR_UP] = 229;
	icons[EGDI_CURSOR_DOWN] = 230;
	icons[EGDI_CURSOR_LEFT] = 231;
	icons[EGDI_CURSOR_RIGHT] = 232;
	icons[EGDI_MENU_MORE] = 232;
	icons[EGDI_CHECK_BOX_CHECKED] = 0;
	icons[EGDI_DROP_DOWN] = 234;
	icons[EGDI_SMALL_CURSOR_UP] = 235;
	icons[EGDI_SMALL_CURSOR_DOWN] = 236;
	icons[EGDI_RADIO_BUTTON_CHECKED] = 237;
	icons[EGDI_MORE_LEFT] = 238;
	icons[EGDI_MORE_RIGHT] = 239;
	icons[EGDI_MORE_UP] = 240;
	icons[EGDI_MORE_DOWN] = 241;
	icons[EGDI_WINDOW_RESIZE] = 242;
	icons[EGDI_EXPAND] = 243;
	icons[EGDI_COLLAPSE] = 244;
	icons[EGDI_FILE] = 245;
	icons[EGDI_DIRECTORY] = 246;
}


CGUITexturedSkin::~CGUITexturedSkin()
{
	for (u32 i=0; i<EGDF_COUNT; ++i)
	{
		if (fonts[i])
			fonts[i]->drop();
	}

	if (SpriteBank)
		SpriteBank->drop();

	if ( pSkinTexture )
	{
		pSkinTexture->drop();
		pSkinTexture = 0;
	}

	if ( pSkinImage )
	{
		pSkinImage->drop();
		pSkinImage = 0;
	}
}


// Protected helper functions...


bool CGUITexturedSkin::readSkinXml(const io::path& guiSkinXmlFile )
{
	IXMLReaderUTF8 * const pXml = pFileSystem->createXMLReaderUTF8( guiSkinXmlFile );
	if ( !pXml )
	{
		return false;
	}

	skinFilename = guiSkinXmlFile;

	while ( pXml->read() )
	{
		if ( EXN_ELEMENT == pXml->getNodeType() )
		{
			if ( isNodeName( pXml, "guiskin" ) )
			{
				skinName = pXml->getAttributeValue( "name" );
				skinTextureFilename = pXml->getAttributeValue( "texture" );
			}
			else if ( isNodeName( pXml, "buttonNormal" ) )	// Normal button
			{
				core::array< c8 * > nodeList(4);
				nodeList.push_back( "left" );
				nodeList.push_back( "middle" );
				nodeList.push_back( "right" );
				nodeList.push_back( "imageCoord" );
				s32 texCoordIndex = ESTC_ENUM_INVALID;
				s32 nodeDepth = 0;

				for ( s32 findResult = findInterestedSubNode( pXml, nodeList, nodeDepth );
					      findResult != -1;
						  findResult = findInterestedSubNode( pXml, nodeList, nodeDepth ) )
				{
					switch ( findResult )
					{
					case 0:
						texCoordIndex = ESTC_BUTTON_NORMAL_LEFT;
						break;

					case 1:
						texCoordIndex = ESTC_BUTTON_NORMAL_MIDDLE;
						break;

					case 2:
						texCoordIndex = ESTC_BUTTON_NORMAL_RIGHT;
						break;

					case 3:
						if ( ESTC_ENUM_INVALID != texCoordIndex )
						{
							skinTexCoords[texCoordIndex] = decodeImageCoord( pXml );
						}
						break;
					}
				}
			}
			else if ( isNodeName( pXml, "buttonPressed" ) )		// Pressed button
			{
				core::array< c8 * > nodeList(4);
				nodeList.push_back( "left" );
				nodeList.push_back( "middle" );
				nodeList.push_back( "right" );
				nodeList.push_back( "imageCoord" );
				s32 texCoordIndex = ESTC_ENUM_INVALID;
				s32 nodeDepth = 0;

				for ( s32 findResult = findInterestedSubNode( pXml, nodeList, nodeDepth );
						  findResult != -1;
						  findResult = findInterestedSubNode( pXml, nodeList, nodeDepth ) )
				{
					switch ( findResult )
					{
					case 0:
						texCoordIndex = ESTC_BUTTON_PRESSED_LEFT;
						break;

					case 1:
						texCoordIndex = ESTC_BUTTON_PRESSED_MIDDLE;
						break;

					case 2:
						texCoordIndex = ESTC_BUTTON_PRESSED_RIGHT;
						break;

					case 3:
						if ( ESTC_ENUM_INVALID != texCoordIndex )
						{
							skinTexCoords[texCoordIndex] = decodeImageCoord( pXml );
						}
						break;
					}
				}
			}
			else if ( isNodeName( pXml, "checkBox" ) )		// Tab button
			{
				core::array< c8 * > nodeList(3);
				nodeList.push_back( "unchecked" );
				nodeList.push_back( "checked" );
				nodeList.push_back( "imageCoord" );
				s32 checkBoxType = ESTC_CHECK_BOX_UNCHECKED;

				s32 nodeDepth = 0;

				for ( s32 findResult = findInterestedSubNode( pXml, nodeList, nodeDepth );
					findResult != -1;
					findResult = findInterestedSubNode( pXml, nodeList, nodeDepth ) )
				{
					switch ( findResult )
					{
					case 0:
						checkBoxType = ESTC_CHECK_BOX_UNCHECKED;
						break;

					case 1:
						checkBoxType = ESTC_CHECK_BOX_CHECKED;
						break;

					case 2:
						skinTexCoords[checkBoxType] = decodeImageCoord( pXml );
						break;
					}
				}
			}
			else if ( isNodeName( pXml, "menuPane" ) )		// Menu pane
			{
				core::array< c8 * > nodeList(1);
				nodeList.push_back( "imageCoord" );
				s32 nodeDepth = 0;

				for ( s32 findResult = findInterestedSubNode( pXml, nodeList, nodeDepth );
						  findResult != -1;
						  findResult = findInterestedSubNode( pXml, nodeList, nodeDepth ) )
				{
					if ( 0 == findResult )
					{
						skinTexCoords[ESTC_MENU_PANE] = decodeImageCoord( pXml );
					}
				}
			}
			else if ( isNodeName( pXml, "sunkenPane" ) )	// Sunken pane
			{
				core::array< c8 * > nodeList(1);
				nodeList.push_back( "imageCoord" );
				s32 nodeDepth = 0;

				for ( s32 findResult = findInterestedSubNode( pXml, nodeList, nodeDepth );
						  findResult != -1;
						  findResult = findInterestedSubNode( pXml, nodeList, nodeDepth ) )
				{
					if ( 0 == findResult )
					{
						skinTexCoords[ESTC_SUNKEN_PANE] = decodeImageCoord( pXml );
					}
				}
			}
			else if ( isNodeName( pXml, "tabBody" ) )		// Tab body
			{
				core::array< c8 * > nodeList(1);
				nodeList.push_back( "imageCoord" );
				s32 nodeDepth = 0;

				for ( s32 findResult = findInterestedSubNode( pXml, nodeList, nodeDepth );
						  findResult != -1;
						  findResult = findInterestedSubNode( pXml, nodeList, nodeDepth ) )
				{
					if ( 0 == findResult )
					{
						skinTexCoords[ESTC_TAB_BODY] = decodeImageCoord( pXml );
					}
				}
			}
			else if ( isNodeName( pXml, "tabButton" ) )		// Tab button
			{
				core::array< c8 * > nodeList(3);
				nodeList.push_back( "active" );
				nodeList.push_back( "inactive" );
				nodeList.push_back( "imageCoord" );
				s32 buttonType = ESTC_TAB_BUTTON_ACTIVE;

				s32 nodeDepth = 0;

				for ( s32 findResult = findInterestedSubNode( pXml, nodeList, nodeDepth );
						  findResult != -1;
						  findResult = findInterestedSubNode( pXml, nodeList, nodeDepth ) )
				{
					switch ( findResult )
					{
					case 0:
						buttonType = ESTC_TAB_BUTTON_ACTIVE;
						break;

					case 1:
						buttonType = ESTC_TAB_BUTTON_INACTIVE;
						break;

					case 2:
						skinTexCoords[buttonType] = decodeImageCoord( pXml );
						break;
					}
				}
			}
			else if ( isNodeName( pXml, "toolBar" ) )		// Toolbar
			{
				core::array< c8 * > nodeList(4);
				nodeList.push_back( "left" );
				nodeList.push_back( "middle" );
				nodeList.push_back( "right" );
				nodeList.push_back( "imageCoord" );
				s32 texCoordIndex = ESTC_ENUM_INVALID;
				s32 nodeDepth = 0;

				for ( s32 findResult = findInterestedSubNode( pXml, nodeList, nodeDepth );
						  findResult != -1;
						  findResult = findInterestedSubNode( pXml, nodeList, nodeDepth ) )
				{
					switch ( findResult )
					{
					case 0:
						texCoordIndex = ESTC_TOOLBAR_LEFT;
						break;

					case 1:
						texCoordIndex = ESTC_TOOLBAR_MIDDLE;
						break;

					case 2:
						texCoordIndex = ESTC_TOOLBAR_RIGHT;
						break;

					case 3:
						if ( ESTC_ENUM_INVALID != texCoordIndex )
						{
							skinTexCoords[texCoordIndex] = decodeImageCoord( pXml );
						}
						break;
					}
				}
			}
			else if ( isNodeName( pXml, "window" ) )		// Window
			{
				core::array< c8 * > nodeList(11);
				nodeList.push_back( "upperLeftCorner" );
				nodeList.push_back( "upperRightCorner" );
				nodeList.push_back( "lowerLeftCorner" );
				nodeList.push_back( "lowerRightCorner" );
				nodeList.push_back( "upperEdge" );
				nodeList.push_back( "lowerEdge" );
				nodeList.push_back( "leftEdge" );
				nodeList.push_back( "rightEdge" );
				nodeList.push_back( "interior" );
				nodeList.push_back( "titleBar" );
				nodeList.push_back( "imageCoord" );
				s32 texCoordIndex = ESTC_ENUM_INVALID;
				s32 nodeDepth = 0;

				for ( s32 findResult = findInterestedSubNode( pXml, nodeList, nodeDepth );
						  findResult != -1;
						  findResult = findInterestedSubNode( pXml, nodeList, nodeDepth ) )
				{
					switch ( findResult )
					{
					case 0:
						texCoordIndex = ESTC_WINDOW_UPPER_LEFT_CORNER;
						break;

					case 1:
						texCoordIndex = ESTC_WINDOW_UPPER_RIGHT_CORNER;
						break;

					case 2:
						texCoordIndex = ESTC_WINDOW_LOWER_LEFT_CORNER;
						break;

					case 3:
						texCoordIndex = ESTC_WINDOW_LOWER_RIGHT_CORNER;
						break;

					case 4:
						texCoordIndex = ESTC_WINDOW_UPPER_EDGE;
						break;

					case 5:
						texCoordIndex = ESTC_WINDOW_LOWER_EDGE;
						break;

					case 6:
						texCoordIndex = ESTC_WINDOW_LEFT_EDGE;
						break;

					case 7:
						texCoordIndex = ESTC_WINDOW_RIGHT_EDGE;
						break;

					case 8:
						texCoordIndex = ESTC_WINDOW_INTERIOR;
						tileWindowInterior = stringc( pXml->getAttributeValue( "tile" ) ).equals_ignore_case( stringc( "true" ) );
						break;

					case 9:
						texCoordIndex = ESTC_WINDOW_TITLEBAR;
						break;

					case 10:
						if ( ESTC_ENUM_INVALID != texCoordIndex )
						{
							skinTexCoords[texCoordIndex] = decodeImageCoord( pXml );
						}
						break;
					}
				}
			}
			else if ( isNodeName( pXml, "colors" ) )		// Ajouté : Skin Colors
			{
				core::array< c8* > nodeList(EGDC_COUNT + 1);
				nodeList.push_back( "3DDarkShadow" );
				nodeList.push_back( "3DShadow" );
				nodeList.push_back( "3DFace" );
				nodeList.push_back( "3DHighLight" );
				nodeList.push_back( "3DLight" );
				nodeList.push_back( "activeBorder" );
				nodeList.push_back( "activeCaption" );
				nodeList.push_back( "appWorkspace" );
				nodeList.push_back( "buttonText" );
				nodeList.push_back( "grayText" );
				nodeList.push_back( "highLight" );
				nodeList.push_back( "highLightText" );
				nodeList.push_back( "inactiveBorder" );
				nodeList.push_back( "inactiveCaption" );
				nodeList.push_back( "toolTip" );
				nodeList.push_back( "toolTipBackground" );
				nodeList.push_back( "scrollBar" );
				nodeList.push_back( "window" );
				nodeList.push_back( "windowSymbol" );
				nodeList.push_back( "icon" );
				nodeList.push_back( "iconHighLight" );
				nodeList.push_back( "grayWindowSymbol" );
				nodeList.push_back( "editable" );
				nodeList.push_back( "grayEditable" );
				nodeList.push_back( "focusedEditable" );
				nodeList.push_back( "color" );
				int colorIndex = ESTC_ENUM_INVALID,
					nodeDepth = 0;

				for ( int findResult = findInterestedSubNode( pXml, nodeList, nodeDepth );
						  findResult != -1;
						  findResult = findInterestedSubNode( pXml, nodeList, nodeDepth ) )
				{
					if (findResult >= 0 && findResult < EGDC_COUNT)
						colorIndex = findResult;
					else if (colorIndex >= 0)
						colors[colorIndex] = decodeColor(pXml);
				}
			}
			else if ( isNodeName( pXml, "sizes" ) )		// Ajouté : Skin Sizes
			{
				core::array< c8* > nodeList(EGDS_COUNT + 1);
				nodeList.push_back( "scrollBarSize" );
				nodeList.push_back( "menuHeight" );
				nodeList.push_back( "windowButtonWidth" );
				nodeList.push_back( "checkBoxWidth" );
				nodeList.push_back( "messageBoxWidth" );
				nodeList.push_back( "messageBoxHeight" );
				nodeList.push_back( "buttonWidth" );
				nodeList.push_back( "buttonHeight" );
				nodeList.push_back( "textDistanceX" );
				nodeList.push_back( "textDistanceY" );
				nodeList.push_back( "titleBarDistanceX" );
				nodeList.push_back( "titleBarDistanceY" );
				nodeList.push_back( "messageBoxGapSpace" );
				nodeList.push_back( "messageBoxMinTextWidth" );
				nodeList.push_back( "messageBoxMaxTextWidth" );
				nodeList.push_back( "messageBoxMinTextHeight" );
				nodeList.push_back( "messageBoxMaxTextHeight" );
				nodeList.push_back( "size" );
				int sizeIndex = ESTC_ENUM_INVALID,
					nodeDepth = 0;

				for ( int findResult = findInterestedSubNode( pXml, nodeList, nodeDepth );
						  findResult != -1;
						  findResult = findInterestedSubNode( pXml, nodeList, nodeDepth ) )
				{
					if (findResult >= 0 && findResult < EGDS_COUNT)
						sizeIndex = findResult;
					else if (sizeIndex >= 0)
						sizes[sizeIndex] = (int)(decodeSize(pXml));
				}
			}
		}
	}	// end while pXml->read()

	// Drop the Xml reader to free it
	pXml->drop();

	return true;
}


bool CGUITexturedSkin::isNodeName( IXMLReaderUTF8 * pXml, const c8 * pNodeName )
{
	// do case insensitive string compare with current node name
	if ( stringc( pNodeName ).equals_ignore_case( stringc( pXml->getNodeName() ) ) )
	{
		return true;
	}
	return false;
}


s32 CGUITexturedSkin::findInterestedSubNode( IXMLReaderUTF8 * pXml, const core::array< c8* >& nodeList, s32 & currNodeDepth )
{
	while ( pXml->read() )
	{
		if ( EXN_ELEMENT == pXml->getNodeType() )
		{
			currNodeDepth++;
			for ( u32 i = 0; i < nodeList.size(); ++i )
			{
				if ( isNodeName( pXml, nodeList[i] ) )
					return (s32)i;
			}
		}
		else if ( EXN_ELEMENT_END == pXml->getNodeType() )
		{
			currNodeDepth--;
			if ( currNodeDepth < 0 )
				break;
		}
	}
	return -1;
}


core::rect<s32> CGUITexturedSkin::decodeImageCoord( IXMLReaderUTF8 * pXml )
{
	core::rect<s32> returnRect;
	const c8 * pCoordStr = 0;
	u32 index = 0;
	u32 tempValue = 0;
	// This stores number of coordinate values converted
	u32 numCoordObtained = 0;

	if ( pXml->read() && EXN_TEXT == pXml->getNodeType() )
		pCoordStr = pXml->getNodeData();

	if (!pCoordStr)
		return returnRect;

	while ( pCoordStr[index] != '\0' && numCoordObtained < 4)
	{
		if ( pCoordStr[index] >= '0' && pCoordStr[index] <= '9' )
		{
			tempValue *= 10;
			tempValue += (u32) (pCoordStr[index] - '0');
		}
		else if ( pCoordStr[index] == ',' )
		{
			switch ( numCoordObtained )
			{
			case 0:
				returnRect.UpperLeftCorner.X = tempValue;
				numCoordObtained++;
				break;

			case 1:
				returnRect.UpperLeftCorner.Y = tempValue;
				numCoordObtained++;
				break;

			case 2:
				returnRect.LowerRightCorner.X = tempValue;
				numCoordObtained++;
				break;

			case 3:
				returnRect.LowerRightCorner.Y = tempValue;
				numCoordObtained++;
				break;
			}

			tempValue = 0;
		}

		++index;
	}

	// This will return the number when the end of string is reached
	returnRect.LowerRightCorner.Y = tempValue;

	return returnRect;
}
video::SColor CGUITexturedSkin::decodeColor( IXMLReaderUTF8 * pXml )
{
	video::SColor returnColor(255, 255, 255, 255);
	const char* pCoordStr = 0;
	u32 index = 0,
		tempValue = 0,
		numColorsObtained = 0;

	if ( pXml->read() && pXml->getNodeType() == EXN_TEXT )
		pCoordStr = pXml->getNodeData();

	if (!pCoordStr)
		return returnColor;

	while ( pCoordStr[index] != '\0' && numColorsObtained < 4 )
	{
		if ( pCoordStr[index] >= '0' && pCoordStr[index] <= '9' )
		{
			tempValue *= 10;
			tempValue += (u32) (pCoordStr[index] - '0');
		}
		else if ( pCoordStr[index] == ',' )
		{
			switch ( numColorsObtained )
			{
			case 0:
				returnColor.setAlpha(core::clamp<u32>(tempValue, 0, 255));
				numColorsObtained++;
				break;

			case 1:
				returnColor.setRed(core::clamp<u32>(tempValue, 0, 255));
				numColorsObtained++;
				break;

			case 2:
				returnColor.setGreen(core::clamp<u32>(tempValue, 0, 255));
				numColorsObtained++;
				break;

			case 3:
				returnColor.setBlue(core::clamp<u32>(tempValue, 0, 255));
				numColorsObtained++;
				break;
			}

			tempValue = 0;
		}

		++index;
	}

	// This will return the number when the end of string is reached
	returnColor.setBlue(core::clamp<u32>(tempValue, 0, 255));

	return returnColor;
}
u32 CGUITexturedSkin::decodeSize( IXMLReaderUTF8 * pXml )
{
	const char* pCoordStr = 0;

	if ( pXml->read() && pXml->getNodeType() == EXN_TEXT )
		pCoordStr = pXml->getNodeData();

	if (!pCoordStr)
		return 0;

	u32 returnValue = 0;
	u32 index = 0;

	while ( pCoordStr[index] != '\0' )
	{
		if ( pCoordStr[index] >= '0' && pCoordStr[index] <= '9' )
		{
			returnValue *= 10;
			returnValue += (u32) (pCoordStr[index] - '0');
		}

		++index;
	}

	return returnValue;
}



// Public function...


bool CGUITexturedSkin::setSkin(const io::path& guiSkinXmlFile )
{
	if ( !readSkinXml( guiSkinXmlFile ) )
	{
		// fail to load or parse xml file
		return false;
	}
	if ( pSkinTexture )
	{
		// release existing skin texture
		pSkinTexture->drop();
		//pSkinTexture = 0;
	}
	pSkinTexture = pVideo->getTexture( skinTextureFilename.c_str() );
	if ( !pSkinTexture )
	{
		// fail to load texture
		return false;
	}
	pSkinTexture->grab();

	if ( pSkinImage )
	{
		// release existing skin image
		pSkinImage->drop();
		//pSkinImage = 0;
	}
	pSkinImage = pVideo->createImageFromFile( skinTextureFilename.c_str() );
	if ( !pSkinImage )
	{
		pSkinTexture->drop();
		return false;
	}

	return true;
}


void CGUITexturedSkin::draw3DButtonPanePressed(IGUIElement*element, const core::rect<s32> &rect, const core::rect<s32> *clip)
{
	// Draw the left side
	const core::recti& buttonPressedLeft = skinTexCoords[ESTC_BUTTON_PRESSED_LEFT];
	f32 leftHtRatio = (f32)rect.getHeight() / buttonPressedLeft.getHeight();
	core::rect<s32> leftDestRect = rect;
	s32 leftWidth = (s32)(buttonPressedLeft.getWidth() * leftHtRatio);
	leftDestRect.LowerRightCorner.X = rect.UpperLeftCorner.X + (leftWidth<=rect.getWidth()?leftWidth:rect.getWidth());
	pVideo->draw2DImage(pSkinTexture,leftDestRect,buttonPressedLeft,clip,0,true);

	// Draw the right side
	const core::recti& buttonPressedRight = skinTexCoords[ESTC_BUTTON_PRESSED_RIGHT];
	f32 rightHtRatio = (f32)rect.getHeight() / buttonPressedRight.getHeight();
	core::rect<s32> rightDestRect = rect;
	s32 rightWidth = (s32)(buttonPressedRight.getWidth() * rightHtRatio);
	rightDestRect.UpperLeftCorner.X = rect.LowerRightCorner.X - (rightWidth<=rect.getWidth()?rightWidth:rect.getWidth());
	pVideo->draw2DImage(pSkinTexture,rightDestRect,buttonPressedRight,clip,0,true);

	// Draw the middle
	const core::recti& buttonPressedMiddle = skinTexCoords[ESTC_BUTTON_PRESSED_MIDDLE];
	core::rect<s32> middleDestRect = rect;
	middleDestRect.UpperLeftCorner.X = leftDestRect.LowerRightCorner.X;
	middleDestRect.LowerRightCorner.X = rightDestRect.UpperLeftCorner.X;
	pVideo->draw2DImage(pSkinTexture,middleDestRect,buttonPressedMiddle,clip,0,true);
}


void CGUITexturedSkin::draw3DButtonPaneStandard(IGUIElement *element, const core::rect<s32> &rect, const core::rect<s32> *clip)
{
	// Fonction largement optimisée

	// Draw the left side
	const core::recti& buttonNormalLeft = skinTexCoords[ESTC_BUTTON_NORMAL_LEFT];
	const int leftWidth = (int)((float)(buttonNormalLeft.getWidth() * rect.getHeight()) / (float)buttonNormalLeft.getHeight());
	const core::recti leftDestRect(rect.UpperLeftCorner.X,
		rect.UpperLeftCorner.Y,
		rect.UpperLeftCorner.X + min(leftWidth, rect.getWidth()),
		rect.LowerRightCorner.Y);
	pVideo->draw2DImage(pSkinTexture,leftDestRect,buttonNormalLeft,clip,0,true);

	// Draw the right side
	const core::recti& buttonNormalRight = skinTexCoords[ESTC_BUTTON_NORMAL_RIGHT];
	const int rightWidth = (int)((float)(buttonNormalRight.getWidth() * rect.getHeight()) / (float)buttonNormalRight.getHeight());
	const core::recti rightDestRect(rect.LowerRightCorner.X - min(rightWidth, rect.getWidth()),
		rect.UpperLeftCorner.Y,
		rect.LowerRightCorner.X,
		rect.LowerRightCorner.Y);
	pVideo->draw2DImage(pSkinTexture,rightDestRect,buttonNormalRight,clip,0,true);

	// Draw the middle
	const core::recti middleDestRect(leftDestRect.LowerRightCorner.X,
		rect.UpperLeftCorner.Y,
		rightDestRect.UpperLeftCorner.X,
		rect.LowerRightCorner.Y);
	pVideo->draw2DImage(pSkinTexture,middleDestRect,skinTexCoords[ESTC_BUTTON_NORMAL_MIDDLE],clip,0,true);
}


void CGUITexturedSkin::draw3DMenuPane(IGUIElement *element, const core::rect<s32> &rect, const core::rect<s32> *clip)
{
	pVideo->draw2DImage(pSkinTexture,rect,skinTexCoords[ESTC_MENU_PANE],clip,0,true);
}


void CGUITexturedSkin::draw3DSunkenPane(IGUIElement *element, video::SColor bgcolor, bool flat, bool fillBackGround, const core::rect<s32> &rect, const core::rect<s32> *clip)
{
	s32 type = ESTC_SUNKEN_PANE;
	if ( EGUIET_CHECK_BOX == element->getType() )
	{
		if ( ( (IGUICheckBox*)element)->isChecked() )
			type = ESTC_CHECK_BOX_CHECKED;
		else
			type = ESTC_CHECK_BOX_UNCHECKED;
	}
	pVideo->draw2DImage(pSkinTexture,rect,skinTexCoords[type],clip,0,true);
}


void CGUITexturedSkin::draw3DTabBody(IGUIElement *element, bool border, bool background, const core::rect<s32> &rect, const core::rect<s32> *clip, s32 tabHeight, gui::EGUI_ALIGNMENT alignment)
{
	// Fonction modifiée pour Irrlicht 1.7.2

	const core::recti& tabBody = skinTexCoords[ESTC_TAB_BODY];
	core::rect<s32> destRect = rect;

	// A faire : utiliser "alignment" :
	/*
	if (alignment == EGUIA_UPPERLEFT)
	{
	}
	else
	{
	}
	*/

	if (tabHeight == -1)
		destRect.UpperLeftCorner.Y += getSize(EGDS_BUTTON_HEIGHT);
	else
		destRect.UpperLeftCorner.Y += tabHeight;

	pVideo->draw2DImage(pSkinTexture,destRect,tabBody,clip,0,true);
}


void CGUITexturedSkin::draw3DTabButton(IGUIElement *element, bool active, const core::rect<s32> &rect, const core::rect<s32> *clip, gui::EGUI_ALIGNMENT alignment)
{
	// Fonction modifiée pour Irrlicht 1.7.2

	s32 buttonType = active ? ESTC_TAB_BUTTON_ACTIVE : ESTC_TAB_BUTTON_INACTIVE;
	const core::recti& tabButton = skinTexCoords[buttonType];

	// A faire : utiliser "alignment" :
	/*
	if (alignment == EGUIA_UPPERLEFT)
	{
	}
	else
	{
	}
	*/

	pVideo->draw2DImage(pSkinTexture,rect,tabButton,clip,0,true);
}


void CGUITexturedSkin::draw3DToolBar(IGUIElement *element, const core::rect<s32> &rect, const core::rect<s32> *clip)
{
	// Draw the left side
	const core::recti& toolBarLeft = skinTexCoords[ESTC_TOOLBAR_LEFT];
	f32 leftHtRatio = (f32)rect.getHeight() / (float)toolBarLeft.getHeight();
	core::rect<s32> leftDestRect = rect;
	s32 leftWidth = (s32)(toolBarLeft.getWidth() * leftHtRatio);
	leftDestRect.LowerRightCorner.X = rect.UpperLeftCorner.X + (leftWidth<=rect.getWidth()?leftWidth:rect.getWidth());
	pVideo->draw2DImage(pSkinTexture,leftDestRect,toolBarLeft,clip,0,true);

	// Draw the right side
	const core::recti& toolBarRight = skinTexCoords[ESTC_TOOLBAR_RIGHT];
	f32 rightHtRatio = (f32)rect.getHeight() / (float)toolBarRight.getHeight();
	core::rect<s32> rightDestRect = rect;
	s32 rightWidth = (s32)(toolBarRight.getWidth() * rightHtRatio);
	rightDestRect.UpperLeftCorner.X = rect.LowerRightCorner.X - (rightWidth<=rect.getWidth()?rightWidth:rect.getWidth());
	pVideo->draw2DImage(pSkinTexture,rightDestRect,toolBarRight,clip,0,true);

	// Draw the middle
	const core::recti&toolBarMiddle = skinTexCoords[ESTC_TOOLBAR_MIDDLE];
	core::rect<s32> middleDestRect = rect;
	middleDestRect.UpperLeftCorner.X = leftDestRect.LowerRightCorner.X;
	middleDestRect.LowerRightCorner.X = rightDestRect.UpperLeftCorner.X;
	pVideo->draw2DImage(pSkinTexture,middleDestRect,toolBarMiddle,clip,0,true);
}


core::rect<s32> CGUITexturedSkin::draw3DWindowBackground(IGUIElement *element, bool drawTitleBar, video::SColor titleBarColor, const core::rect<s32> &rect, const core::rect<s32> *clip, core::rect<s32>* checkClientArea)
{
	// Fonction modifiée pour Irrlicht 1.7.2

	if (!pVideo)
	{
		if (checkClientArea)
			*checkClientArea = rect;
		return rect;
	}

	// Nouvelles constantes utilisées pour le dessin des bords de la fenêtre ainsi que de la barre de titre pour assurer une compatibilité entre les skins d'Irrlicht et celui-ci
	const int borderSize = 3, titleBarSizeY = 18;

	// Draw top left corner
	const core::recti& topLeftCorner = skinTexCoords[ESTC_WINDOW_UPPER_LEFT_CORNER];
	core::rect<s32> topLeftCornerDest = rect;
	//topLeftCornerDest.LowerRightCorner.X = rect.UpperLeftCorner.X + topLeftCorner.getWidth();
	//topLeftCornerDest.LowerRightCorner.Y = rect.UpperLeftCorner.Y + topLeftCorner.getHeight();
	topLeftCornerDest.LowerRightCorner.X = rect.UpperLeftCorner.X + borderSize;
	topLeftCornerDest.LowerRightCorner.Y = rect.UpperLeftCorner.Y + borderSize;
	if (!checkClientArea)
		pVideo->draw2DImage(pSkinTexture,topLeftCornerDest,topLeftCorner,clip,0,true);

	// Draw top right corner
	const core::recti& topRightCorner = skinTexCoords[ESTC_WINDOW_UPPER_RIGHT_CORNER];
	core::rect<s32> topRightCornerDest = rect;
	//topRightCornerDest.UpperLeftCorner.X = rect.LowerRightCorner.X - topRightCorner.getWidth();
	//topRightCornerDest.LowerRightCorner.Y = rect.UpperLeftCorner.Y + topRightCorner.getHeight();
	topRightCornerDest.UpperLeftCorner.X = rect.LowerRightCorner.X - borderSize;
	topRightCornerDest.LowerRightCorner.Y = rect.UpperLeftCorner.Y + borderSize;
	if (!checkClientArea)
		pVideo->draw2DImage(pSkinTexture,topRightCornerDest,topRightCorner,clip,0,true);

	// Draw bottom left corner
	const core::recti& bottomLeftCorner = skinTexCoords[ESTC_WINDOW_LOWER_LEFT_CORNER];
	core::rect<s32> bottomLeftCornerDest = rect;
	//bottomLeftCornerDest.LowerRightCorner.X = rect.UpperLeftCorner.X + bottomLeftCorner.getWidth();
	//bottomLeftCornerDest.UpperLeftCorner.Y = rect.LowerRightCorner.Y - bottomLeftCorner.getHeight();
	bottomLeftCornerDest.LowerRightCorner.X = rect.UpperLeftCorner.X + borderSize;
	bottomLeftCornerDest.UpperLeftCorner.Y = rect.LowerRightCorner.Y - borderSize;
	if (!checkClientArea)
		pVideo->draw2DImage(pSkinTexture,bottomLeftCornerDest,bottomLeftCorner,clip,0,true);

	// Draw top right corner
	const core::recti& bottomRightCorner = skinTexCoords[ESTC_WINDOW_LOWER_RIGHT_CORNER];
	core::rect<s32> bottomRightCornerDest = rect;
	//bottomRightCornerDest.UpperLeftCorner.X = rect.LowerRightCorner.X - bottomRightCorner.getWidth();
	//bottomRightCornerDest.UpperLeftCorner.Y = rect.LowerRightCorner.Y - bottomRightCorner.getHeight();
	bottomRightCornerDest.UpperLeftCorner.X = rect.LowerRightCorner.X - borderSize;
	bottomRightCornerDest.UpperLeftCorner.Y = rect.LowerRightCorner.Y - borderSize;
	if (!checkClientArea)
		pVideo->draw2DImage(pSkinTexture,bottomRightCornerDest,bottomRightCorner,clip,0,true);

	// Draw top edge
	const core::recti& topEdge = skinTexCoords[ESTC_WINDOW_UPPER_EDGE];
	core::rect<s32> topEdgeDest = rect;
	//topEdgeDest.UpperLeftCorner.X = rect.UpperLeftCorner.X + topLeftCorner.getWidth();
	//topEdgeDest.LowerRightCorner.X = rect.LowerRightCorner.X - topRightCorner.getWidth();
	//topEdgeDest.LowerRightCorner.Y = rect.UpperLeftCorner.Y + topEdge.getHeight();
	topEdgeDest.UpperLeftCorner.X = rect.UpperLeftCorner.X + borderSize;
	topEdgeDest.LowerRightCorner.X = rect.LowerRightCorner.X - borderSize;
	topEdgeDest.LowerRightCorner.Y = rect.UpperLeftCorner.Y + borderSize;
	if (!checkClientArea)
		pVideo->draw2DImage(pSkinTexture,topEdgeDest,topEdge,clip,0,true);

	// Draw bottom edge
	const core::recti& bottomEdge = skinTexCoords[ESTC_WINDOW_LOWER_EDGE];
	core::rect<s32> bottomEdgeDest = rect;
	//bottomEdgeDest.UpperLeftCorner.X = rect.UpperLeftCorner.X + bottomLeftCorner.getWidth();
	//bottomEdgeDest.UpperLeftCorner.Y = rect.LowerRightCorner.Y - bottomEdge.getHeight();
	//bottomEdgeDest.LowerRightCorner.X = rect.LowerRightCorner.X - bottomRightCorner.getWidth();
	bottomEdgeDest.UpperLeftCorner.X = rect.UpperLeftCorner.X + borderSize;
	bottomEdgeDest.UpperLeftCorner.Y = rect.LowerRightCorner.Y - borderSize;
	bottomEdgeDest.LowerRightCorner.X = rect.LowerRightCorner.X - borderSize;
	if (!checkClientArea)
		pVideo->draw2DImage(pSkinTexture,bottomEdgeDest,bottomEdge,clip,0,true);

	// Draw left edge
	const core::recti& leftEdge = skinTexCoords[ESTC_WINDOW_LEFT_EDGE];
	core::rect<s32> leftEdgeDest = rect;
	//leftEdgeDest.UpperLeftCorner.Y = rect.UpperLeftCorner.Y + topLeftCorner.getHeight();
	//leftEdgeDest.LowerRightCorner.X = rect.UpperLeftCorner.X + leftEdge.getWidth();
	//leftEdgeDest.LowerRightCorner.Y = rect.LowerRightCorner.Y - bottomLeftCorner.getHeight();
	leftEdgeDest.UpperLeftCorner.Y = rect.UpperLeftCorner.Y + borderSize;
	leftEdgeDest.LowerRightCorner.X = rect.UpperLeftCorner.X + borderSize;
	leftEdgeDest.LowerRightCorner.Y = rect.LowerRightCorner.Y - borderSize;
	if (!checkClientArea)
		pVideo->draw2DImage(pSkinTexture,leftEdgeDest,leftEdge,clip,0,true);

	// Draw right edge
	const core::recti& rightEdge = skinTexCoords[ESTC_WINDOW_RIGHT_EDGE];
	core::rect<s32> rightEdgeDest = rect;
	//rightEdgeDest.UpperLeftCorner.X = rect.LowerRightCorner.X - rightEdge.getWidth();
	//rightEdgeDest.UpperLeftCorner.Y = rect.UpperLeftCorner.Y + topRightCorner.getHeight();
	//rightEdgeDest.LowerRightCorner.Y = rect.LowerRightCorner.Y - bottomRightCorner.getHeight();
	rightEdgeDest.UpperLeftCorner.X = rect.LowerRightCorner.X - borderSize;
	rightEdgeDest.UpperLeftCorner.Y = rect.UpperLeftCorner.Y + borderSize;
	rightEdgeDest.LowerRightCorner.Y = rect.LowerRightCorner.Y - borderSize;
	if (!checkClientArea)
		pVideo->draw2DImage(pSkinTexture,rightEdgeDest,rightEdge,clip,0,true);

	// Draw interior
	const core::recti& interior = skinTexCoords[ESTC_WINDOW_INTERIOR];
	core::rect<s32> interiorDest = rect;
	//interiorDest.UpperLeftCorner.X = rect.UpperLeftCorner.X + leftEdge.getWidth();
	//interiorDest.UpperLeftCorner.Y = rect.UpperLeftCorner.Y + topEdge.getHeight();
	//interiorDest.LowerRightCorner.X = rect.LowerRightCorner.X - rightEdge.getWidth();
	//interiorDest.LowerRightCorner.Y = rect.LowerRightCorner.Y - bottomEdge.getHeight();
	interiorDest.UpperLeftCorner.X = rect.UpperLeftCorner.X + borderSize;
	interiorDest.UpperLeftCorner.Y = rect.UpperLeftCorner.Y + borderSize;
	interiorDest.LowerRightCorner.X = rect.LowerRightCorner.X - borderSize;
	interiorDest.LowerRightCorner.Y = rect.LowerRightCorner.Y - borderSize;
	if (!checkClientArea)
		pVideo->draw2DImage(pSkinTexture,interiorDest,interior,clip,0,true);
	else
		*checkClientArea = interiorDest;

	if (drawTitleBar)
	{
		// Draw title bar
		const core::recti& titleBar = skinTexCoords[ESTC_WINDOW_TITLEBAR];
		core::rect<s32> titleBarDest = rect;
		titleBarDest.UpperLeftCorner.X = rect.UpperLeftCorner.X + 3;
		titleBarDest.UpperLeftCorner.Y = rect.UpperLeftCorner.Y + 3;
		//titleBarDest.LowerRightCorner.X = rect.UpperLeftCorner.X + titleBar.getWidth();
		//titleBarDest.LowerRightCorner.Y = rect.UpperLeftCorner.Y + titleBar.getHeight();
		titleBarDest.LowerRightCorner.Y = rect.UpperLeftCorner.Y + titleBarSizeY;

		if (!checkClientArea)
			pVideo->draw2DImage(pSkinTexture,titleBarDest,titleBar,clip,0,true);
		else
			(*checkClientArea).UpperLeftCorner.Y = titleBarDest.LowerRightCorner.Y;

		return titleBarDest;
	}

	return rect;
}


SColor CGUITexturedSkin::getColor(EGUI_DEFAULT_COLOR color) const
{
	if ((u32)color < EGDC_COUNT)
		return colors[color];
	else
		return video::SColor();
}


void CGUITexturedSkin::setColor(EGUI_DEFAULT_COLOR which, SColor newColor)
{
	if ((u32)which < EGDC_COUNT)
		colors[which] = newColor;

	if ( 0 == which )
	{
		SetSkinAlpha( newColor.getAlpha() );
	}
}


s32 CGUITexturedSkin::getSize(EGUI_DEFAULT_SIZE size) const
{
	if ((u32)size < EGDS_COUNT)
		return sizes[size];
	else
		return 0;
}


void CGUITexturedSkin::setSize(EGUI_DEFAULT_SIZE which, s32 size)
{
	if ((u32)which < EGDS_COUNT)
		sizes[which] = size;
}


const wchar_t * CGUITexturedSkin::getDefaultText(EGUI_DEFAULT_TEXT text) const
{
	if ((u32)text < EGDT_COUNT)
		return texts[text].c_str();
	else
		return texts[0].c_str();
}


void CGUITexturedSkin::setDefaultText(EGUI_DEFAULT_TEXT which, const wchar_t *newText)
{
	if ((u32)which < EGDT_COUNT)
		texts[which] = newText;
}


IGUIFont * CGUITexturedSkin::getFont(EGUI_DEFAULT_FONT which) const
{
	if (((u32)which < EGDS_COUNT) && fonts[which])
		return fonts[which];
	else
		return fonts[EGDF_DEFAULT];
}


void CGUITexturedSkin::setFont(IGUIFont *font, EGUI_DEFAULT_FONT which)
{
	if ((u32)which >= EGDS_COUNT)
		return;

	if (fonts[which])
		fonts[which]->drop();

	fonts[which] = font;

	if (fonts[which])
		fonts[which]->grab();
}


IGUISpriteBank* CGUITexturedSkin::getSpriteBank() const
{
	return SpriteBank;
}


void CGUITexturedSkin::setSpriteBank(IGUISpriteBank* bank)
{
	if (SpriteBank)
		SpriteBank->drop();

	if (bank)
		bank->grab();

	SpriteBank = bank;
}


u32 CGUITexturedSkin::getIcon(EGUI_DEFAULT_ICON icon) const
{
	if ((u32)icon < EGDI_COUNT)
		return icons[icon];
	else
		return 0;
}


void CGUITexturedSkin::setIcon(EGUI_DEFAULT_ICON icon, u32 index)
{
	if ((u32)icon < EGDI_COUNT)
		icons[icon] = index;
}


void CGUITexturedSkin::drawIcon(IGUIElement* element, EGUI_DEFAULT_ICON icon,
								const core::position2di position,
								u32 starttime, u32 currenttime,
								bool loop, const core::rect<s32>* clip)
{
	if (!SpriteBank)
		return;

	SpriteBank->draw2DSprite(icons[icon], position, clip,
		video::SColor(255,0,0,0), starttime, currenttime, loop, true);
}


void CGUITexturedSkin::draw2DRectangle(IGUIElement* element,
									   const video::SColor &color, const core::rect<s32>& pos,
									   const core::rect<s32>* clip)
{
	pVideo->draw2DRectangle(color, pos, clip);
}


void CGUITexturedSkin::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const
{
	out->addString( "SkinFilename", skinFilename.c_str() );
}


void CGUITexturedSkin::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
{
	core::stringc file = in->getAttributeAsString( "SkinFilename" );
	setSkin( file.c_str() );
}

void CGUITexturedSkin::SetSkinAlpha( u32 a )
{
	if ( !pSkinImage || !pSkinTexture )
	{
		return;
	}
	if ( ECF_A8R8G8B8 != pSkinImage->getColorFormat() || pSkinImage->getColorFormat() != pSkinTexture->getColorFormat() )
	{
		// Working with 32 bit colour format only for now
		return;
	}

	const core::dimension2du& imageSize = pSkinImage->getDimension();
	const u32 pixelMask = pSkinImage->getRedMask() | pSkinImage->getGreenMask() | pSkinImage->getBlueMask();
	const u32 alphaBits = (a << 24 | a << 16 | a << 8 | a) & pSkinImage->getAlphaMask();
	//u32 imagePitch = pSkinImage->getPitch();
	//u32 texturePitch = pSkinTexture->getPitch();
	const u32 imagePixelSize = pSkinImage->getBytesPerPixel();

	u8 * pImageData = (u8 *) pSkinImage->lock();
	u8 * pTextureData = (u8 *) pSkinTexture->lock();
	for ( u32 y = 0; y < imageSize.Height; ++y )
	{
		for ( u32 x = 0; x < imageSize.Width; ++x )
		{
			u32 * pImagePixel = (u32*)pImageData;
			u32 * pTexturePixel = (u32*)pTextureData;
			if ( (*pImagePixel & pSkinImage->getAlphaMask()) != 0 )
				*pTexturePixel = (*pImagePixel & pixelMask) | alphaBits;

			pImageData += imagePixelSize;
			pTextureData += imagePixelSize;
		}
		// Somehow incrementing the pointer by pitch results in out of bounds memory access, while not using it works fine
		//pImageData += imagePitch;
		//pTextureData += texturePitch;
	}
	pSkinImage->unlock();
	pSkinTexture->unlock();
}


}	// end namespace gui
}	// end namespace irr
