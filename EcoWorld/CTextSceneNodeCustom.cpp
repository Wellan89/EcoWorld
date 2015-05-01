// Copyright (C) 2002-2011 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CTextSceneNodeCustom.h"
//#include "ISceneManager.h"
//#include "IVideoDriver.h"
//#include "ICameraSceneNode.h"
//#include "IGUISpriteBank.h"
//#include "SMeshBuffer.h"
//#include "os.h"


//namespace irr
//{
//namespace scene
//{


//! constructor
CTextSceneNodeCustom::CTextSceneNodeCustom(ISceneNode* parent, ISceneManager* mgr, s32 id,
			gui::IGUIFont* font,
			const core::vector3df& position, const wchar_t* text,
			video::SColor color)
	: ITextSceneNode(parent, mgr, id, position), Text(text), Color(color),
		Font(font), Coll(mgr->getSceneCollisionManager())

{
#ifdef _DEBUG
	setDebugName("CTextSceneNodeCustom");
#endif

	if (Font)
		Font->grab();

	setAutomaticCulling(scene::EAC_OFF);
}

//! destructor
CTextSceneNodeCustom::~CTextSceneNodeCustom()
{
	if (Font)
		Font->drop();
}

void CTextSceneNodeCustom::OnRegisterSceneNode()
{
	if (IsVisible)
		SceneManager->registerNodeForRendering(this, ESNRP_TRANSPARENT_EFFECT);

	ISceneNode::OnRegisterSceneNode();
}

//! renders the node.
void CTextSceneNodeCustom::render()
{
	if (!Font || !Coll)
		return;

	const core::vector2di pos = Coll->getScreenCoordinatesFrom3DPosition(getAbsolutePosition(), SceneManager->getActiveCamera());

	const core::recti r(pos, core::dimension2di(1,1));
	Font->draw(Text.c_str(), r, Color, true, true);
}


//} // end namespace scene
//} // end namespace irr

