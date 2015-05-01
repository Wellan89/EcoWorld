// Copyright (C) 2002-2011 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_TEXT_SCENE_NODE_CUSTOM_H_INCLUDED__
#define __C_TEXT_SCENE_NODE_CUSTOM_H_INCLUDED__

// Cette version, prise de Irrlicht SVN 1.8.0, a été légèrement modifiée : pass de rendu modifié de ESNRP_TRANSPARENT à ESNRP_TRANSPARENT_EFFECT, pour être rendu après tous les nodes
#include "global.h"
using namespace scene;

//#include "ITextSceneNode.h"
//#include "IBillboardTextSceneNode.h"
//#include "IGUIFont.h"
//#include "IGUIFontBitmap.h"
//#include "ISceneCollisionManager.h"
//#include "SMesh.h"

//namespace irr
//{
//namespace scene
//{

	class CTextSceneNodeCustom : public ITextSceneNode
	{
	public:

		//! constructor
		CTextSceneNodeCustom(ISceneNode* parent, ISceneManager* mgr, s32 id,
			gui::IGUIFont* font,
			const core::vector3df& position = core::vector3df(0,0,0), const wchar_t* text=0,
			video::SColor color=video::SColor(100,0,0,0));

		//! destructor
		virtual ~CTextSceneNodeCustom();

		virtual void OnRegisterSceneNode();

		//! renders the node.
		virtual void render();

		//! Returns type of the scene node
		virtual ESCENE_NODE_TYPE getType() const { return ESNT_TEXT; }

	private:

		core::stringw Text;
		video::SColor Color;
		gui::IGUIFont* Font;
		scene::ISceneCollisionManager* Coll;
		core::aabbox3d<f32> Box;

	public:
		//! returns the axis aligned bounding box of this node
		virtual const core::aabbox3d<f32>& getBoundingBox() const
		{
			return Box;
		}

		//! sets the text string
		virtual void setText(const wchar_t* text)
		{
			Text = text;
		}

		//! sets the color of the text
		virtual void setTextColor(video::SColor color)
		{
			Color = color;
		}
	};

//} // end namespace scene
//} // end namespace irr

#endif

