/*  
    Written by Asger Feldthaus
    
    February 2007
*/

// Cette version a �t� modifi�e :
// - Correction de bugs de rendu par la modification des fonctions CTreeSceneNode::OnRegisterSceneNode() et CTreeSceneNode::render()
// - Correction de bugs de par l'impl�mentation des fonctions setPosition() et setScale()
// - Plusieurs optimisations, dont une optimisation importante par le remplacement de tableaux (core::array) par des listes (core::list) diminuant la consommation m�moire et fournissant un code plus rapide
// - Le billboard material a �t� supprim� pour utiliser le mat�riau g�n�ral de l'arbre � la place, mais adapt� au billboard

#ifndef _C_TREE_SCENE_NODE_H_
#define _C_TREE_SCENE_NODE_H_

#include "global.h"
//#include <IRR/irrlicht.h>
#include "CBillboardGroupSceneNode.h"
#include "CTreeGenerator.h"

namespace irr
{
namespace scene
{

/*!
    \brief A tree with three levels of detail.
    
    CTreeSceneNode is a tree with three levels of detail. The highest LOD is the mesh with full detail. The second LOD is another mesh with fewer polygons.
    The last LOD is a billboard. The leaves are displayed in the two first levels of detail.
    
    Call setup() to initialize the scene node.
*/
class CTreeSceneNode : public ISceneNode
{
public:
    //! Standard constructor for scene nodes. Call setup() to initialize the scene node.
    CTreeSceneNode( ISceneNode* parent, ISceneManager* manager, s32 id=-1,
        const core::vector3df& position = core::vector3df(0,0,0),
        const core::vector3df& rotation = core::vector3df(0,0,0),
        const core::vector3df& scale = core::vector3df(1,1,1) );

    virtual ~CTreeSceneNode();

    //! Sets the meshes used by the scene node, the leaf node used and the billboard texture.
    //! \param highLod: The mesh to use when the tree is close to the camera. Must not be 0!
    //! \param midLod: The mesh to use when the camera is at mediocre distance. If 0 highLod is used instead.
    //! \param leafNode: The scene node displaying the leaves on the tree. If 0 no leaves will be displayed.
    //! \param billboardTexture: The texture to display on the billboard when the tree is very far from the camera. If 0 it will not turn into a billboard.
    void setup( const STreeMesh* highLOD, const STreeMesh* midLOD, video::ITexture* billboardTexture=0 );

    //! Generates meshes and leaves for the scene node automatically, and specifies the billboard texture.
    //! \param seed: The random number seed used to generate the tree.
    //! \param billboardTexture: The texture to display on the billboard when the tree is very far from the camera. If 0 it will not turn into a billboard.
    void setup( CTreeGenerator* generator, s32 seed=0, video::ITexture* billboardTexture=0 );

    //! Specifies where the tree should switch to medium LOD, and where it should switch to a billboard.
    //! \param midRange: If the camera is closer than this, it will use high LOD. If it is greater than this, but lower than farRange, it will use medium LOD.
    //! \param farRange: If the camera is further away than this, a billboard will be displayed instead of the tree.
    void setDistances( f32 midRange, f32 farRange );

    virtual void OnRegisterSceneNode();

    virtual void render();

	// Fonctions impl�ment�es :

	//! Sets the position of the node relative to its parent.
	/** Note that the position is relative to the parent.
	\param newpos New relative position of the scene node. */
	virtual void setPosition(const core::vector3df& newpos);

	//! Sets the relative scale of the scene node.
	/** \param scale New scale of the node, relative to its parent. */
	virtual void setScale(const core::vector3df& scale);

private:
	void updateBillboard();
	void updateSizeAndRange();

	video::SMaterial TreeMaterial;
	video::SMaterial BillboardMaterial;

	f32 MidRange;
	f32 FarRange;
	f32 Size;

	CBillboardGroupSceneNode* LeafNode;

	SMeshBuffer* HighLODMeshBuffer;
	SMeshBuffer* MidLODMeshBuffer;

	SMeshBuffer BillboardMeshBuffer;

	f32 DistSQ;

public:	// Fonctions adapt�es inline
	virtual video::SMaterial& getMaterial(u32 i)
	{
		if (i == 1)
			return BillboardMaterial;
		return TreeMaterial;
	}

	virtual u32 getMaterialCount() const { return 2; }

	virtual const core::aabbox3df& getBoundingBox() const { return HighLODMeshBuffer->BoundingBox; }

	u32 getVertexCount() const { return HighLODMeshBuffer->getVertexCount(); }

	//! Returns the leaf node associated with the tree. Useful for settings the correct texture and material settings.
	CBillboardGroupSceneNode* getLeafNode() { return LeafNode; }
};

} // namespace scene
} // namespace irr

#endif
