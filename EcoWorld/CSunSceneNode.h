#ifndef DEF_SUN_SCENE_NODE
#define DEF_SUN_SCENE_NODE

#include "global.h"

// Classe CSunSceneNode permettant l'affichage d'un soleil (ou d'une lune) avec l'effet qu'il se trouve à l'infini (en utilisant une distance constante avec la caméra)
// Pour son bon fonctionnement, ce node ne devrait avoir ni enfants, ni parent différent du root scene node du scene manager
// Cette classe est basée sur les classes CBillboardSceneNode et CSkyDomeSceneNode d'Irrlicht SVN 1.8.0-alpha
class CSunSceneNode : public scene::IBillboardSceneNode
{
public:
	// Constructeur
	CSunSceneNode(const core::dimension2df& size, const video::SColor& color, scene::ISceneNode* parent, scene::ISceneManager* mgr, int id = -1);

	//! pre render event
	virtual void OnRegisterSceneNode();

	//! render
	virtual void render();

	//! sets the size of the billboard
	virtual void setSize(const core::dimension2df& size);
 
	//! Set the color of all vertices of the billboard
	//! \param overallColor: the color to set
	virtual void setColor(const video::SColor& overallColor);
	
	//! Set the color of the top and bottom vertices of the billboard
	//! \param topColor: the color to set the top vertices
	//! \param bottomColor: the color to set the bottom vertices
	virtual void setColor(const video::SColor& topColor, const video::SColor& bottomColor);

	//! Gets the color of the top and bottom vertices of the billboard
	//! \param[out] topColor: stores the color of the top vertices
	//! \param[out] bottomColor: stores the color of the bottom vertices
	virtual void getColor(video::SColor& topColor, video::SColor& bottomColor) const;

protected:
	// Depuis CBillboardSceneNode :
	core::dimension2df Size;
	core::aabbox3df BBox;
	video::SMaterial Material;

	video::S3DVertex vertices[4];
	u16 indices[6];

public:	// Accesseurs inline :

	//! Returns type of the scene node
	virtual scene::ESCENE_NODE_TYPE getType() const { return scene::ESNT_BILLBOARD; }

	//! returns the axis aligned bounding box of this node
	virtual const core::aabbox3df& getBoundingBox() const { return BBox; }

	virtual video::SMaterial& getMaterial(u32 i) { return Material; }
	
	//! returns amount of materials used by this scene node.
	virtual u32 getMaterialCount() const { return 1; }

	//! Sets the widths of the top and bottom edges of the billboard independently.
	virtual void setSize(float height, float bottomEdgeWidth, float topEdgeWidth)
	{
		setSize(core::dimension2df(bottomEdgeWidth, height));
	}

	//! gets the size of the billboard
	virtual const core::dimension2df& getSize() const { return Size; }

	//! Gets the widths of the top and bottom edges of the billboard.
	virtual void getSize(float& height, float& bottomEdgeWidth, float& topEdgeWidth) const
	{
		height = Size.Height;
		topEdgeWidth = bottomEdgeWidth = Size.Width;
	}
};

#endif
