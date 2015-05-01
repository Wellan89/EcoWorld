/*  
    Written by Asger Feldthaus
    
    February 2007
*/

#include "CTreeSceneNode.h"

namespace irr
{
namespace scene
{

//const f32 CAMERA_UPDATE_DISTANCE = 100.0f;

CTreeSceneNode::CTreeSceneNode( ISceneNode* parent, ISceneManager* manager, s32 id,
        const core::vector3df& position,
        const core::vector3df& rotation,
        const core::vector3df& scale )
    : ISceneNode( parent, manager, id, position, rotation, scale )
{
    MidRange = 500.0f;
    FarRange = 1300.0f;
    Size = 0.0f;

	HighLODMeshBuffer = 0;
    MidLODMeshBuffer = 0;
    LeafNode = 0;

	BillboardMaterial.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;	// Optimisation non significative : Nouveau (désactivé) : EMT_TRANSPARENT_ALPHA_CHANNEL_REF
    BillboardMaterial.Lighting = true;	// false	// Permet d'activer la lumière sur les arbres en billboard
    BillboardMaterial.FogEnable = true;	// Ajouté : Permet d'activer le brouillard

	BillboardMeshBuffer.Indices.push_back( 2 );
    BillboardMeshBuffer.Indices.push_back( 1 );
    BillboardMeshBuffer.Indices.push_back( 0 );
    
    BillboardMeshBuffer.Indices.push_back( 0 );
    BillboardMeshBuffer.Indices.push_back( 3 );
    BillboardMeshBuffer.Indices.push_back( 2 );
    
    BillboardMeshBuffer.Vertices.push_back( video::S3DVertex( 0,0,0,  0,1,0, video::SColor(255,255,255,255), 0,1 ) );
    BillboardMeshBuffer.Vertices.push_back( video::S3DVertex( 0,0,0,  0,1,0, video::SColor(255,255,255,255), 0,0 ) );
    BillboardMeshBuffer.Vertices.push_back( video::S3DVertex( 0,0,0,  0,1,0, video::SColor(255,255,255,255), 1,0 ) );
    BillboardMeshBuffer.Vertices.push_back( video::S3DVertex( 0,0,0,  0,1,0, video::SColor(255,255,255,255), 1,1 ) );

	setPosition(position);
	setScale(scale);
}

CTreeSceneNode::~CTreeSceneNode()
{
    if ( HighLODMeshBuffer )
        HighLODMeshBuffer->drop();
    
    if ( MidLODMeshBuffer )
        MidLODMeshBuffer->drop();
}

void CTreeSceneNode::setup( const STreeMesh* highLOD, const STreeMesh* midLOD, video::ITexture* billboardTexture )
{
    if ( HighLODMeshBuffer )
        HighLODMeshBuffer->drop();
    
    if ( MidLODMeshBuffer )
        MidLODMeshBuffer->drop();
    
    if ( LeafNode )
        LeafNode->remove();
    
    HighLODMeshBuffer = highLOD->MeshBuffer;
    MidLODMeshBuffer = midLOD->MeshBuffer;

	const u32 leavesSize = highLOD->Leaves.size();
    if ( leavesSize )
    {
		LeafNode = new CBillboardGroupSceneNode( this, SceneManager );

		// Ajouté : Alloue la mémoire nécessaire au tableau de billboards des feuilles
		LeafNode->allocateBillboardsArray(leavesSize);

		const core::list<STreeLeaf>::ConstIterator END = highLOD->Leaves.end();
        for (core::list<STreeLeaf>::ConstIterator it = highLOD->Leaves.begin(); it != END; ++it)
        {
			const STreeLeaf& leaf = (*it);
            if ( leaf.HasAxis )
                LeafNode->addBillboardWithAxis( leaf.Position, leaf.Size, leaf.Axis, leaf.Roll, leaf.Color );
            else
                LeafNode->addBillboard( leaf.Position, leaf.Size, leaf.Roll, leaf.Color );
        }

		LeafNode->drop();
    }
	else
		LeafNode = NULL;

	BillboardMaterial.TextureLayer[0].Texture = billboardTexture;
    
    if ( HighLODMeshBuffer )
    {
        HighLODMeshBuffer->grab();
        Size = HighLODMeshBuffer->BoundingBox.getExtent().getLength() * 0.5f;

		// Ajouté : Active les VBO (uniquement disponible sous OpenGL) pour améliorer les performances
		HighLODMeshBuffer->setHardwareMappingHint(scene::EHM_STATIC);	
    }
    
    if ( MidLODMeshBuffer )
	{
        MidLODMeshBuffer->grab();

		// Ajouté : Active les VBO (uniquement disponible sous OpenGL) pour améliorer les performances
		MidLODMeshBuffer->setHardwareMappingHint(scene::EHM_STATIC);	
	}
}

void CTreeSceneNode::setup( CTreeGenerator* generator, s32 seed, video::ITexture* billboardTexture )
{
    STreeMesh* highLOD = generator->generateTree( 8, seed, true, 0 );
    STreeMesh* midLOD = generator->generateTree( 4, seed, false, 1 );
    
    setup( highLOD, midLOD, billboardTexture );
    
    highLOD->drop();
    midLOD->drop();
}

void CTreeSceneNode::setDistances( f32 midRange, f32 farRange )
{
    MidRange = midRange * midRange;
    FarRange = farRange * farRange;
}

/*
void CTreeSceneNode::OnRegisterSceneNode()
{
    if ( IsVisible )
    {
        ICameraSceneNode* camera = SceneManager->getActiveCamera();
        
        DistSQ = 0.0f;
        if ( camera )
        {
            core::vector3df campos = camera->getAbsolutePosition();
            
            core::vector3df center = HighLODMeshBuffer->BoundingBox.getCenter();
            AbsoluteTransformation.rotateVect(center);
            
            center += getAbsolutePosition();
            
            DistSQ = (campos - center).getLengthSQ();
        }
        
        if ( LeafNode )
        {
            f32 far = FarRange + Size;
            LeafNode->setVisible( far*far >= DistSQ );
        }
        
        SceneManager->registerNodeForRendering( this );
    }
    ISceneNode::OnRegisterSceneNode();
}

void CTreeSceneNode::render()
{
    video::IVideoDriver* driver = SceneManager->getVideoDriver();
    
    f32 far = FarRange + Size;
    f32 mid = MidRange + Size;
    
	if ( far*far < DistSQ && BillboardMaterial.TextureLayer[0].Texture != 0 )
    {
        driver->setTransform( video::ETS_WORLD, core::matrix4() );

        driver->setMaterial( BillboardMaterial );
        
        ICameraSceneNode* camera = SceneManager->getActiveCamera();
        
        core::vector3df view = camera->getAbsolutePosition() - getAbsolutePosition();
        
        if ( view.getDistanceFromSQ( LastViewVec ) >= CAMERA_UPDATE_DISTANCE*CAMERA_UPDATE_DISTANCE )
        {
            updateBillboard();
            LastViewVec = view;
        }
        
        driver->drawMeshBuffer( &BillboardMeshBuffer );
    }
    else
    if ( mid*mid < DistSQ && MidLODMeshBuffer != 0 )
    {
        driver->setTransform( video::ETS_WORLD, AbsoluteTransformation );
        
        driver->setMaterial( TreeMaterial );
        
        driver->drawMeshBuffer( MidLODMeshBuffer );
    }
    else
    {
        driver->setTransform( video::ETS_WORLD, AbsoluteTransformation );
        
        driver->setMaterial( TreeMaterial );
        
        driver->drawMeshBuffer( HighLODMeshBuffer );
    }
}
*/

// Fonctions corrigées :
void CTreeSceneNode::OnRegisterSceneNode()
{
    if ( IsVisible )
    {
        DistSQ = 0.0f;

        const ICameraSceneNode* const camera = SceneManager->getActiveCamera();
        if (camera)
        {
			core::vector3df center = HighLODMeshBuffer->BoundingBox.getCenter();
            AbsoluteTransformation.rotateVect(center);
			center += getAbsolutePosition();

			DistSQ = (camera->getAbsolutePosition() - center).getLengthSQ();
        }

		if (DistSQ < FarRange)	// On ne dessinera un solide que si on affiche les niveaux de détail haut et moyen
		{
			SceneManager->registerNodeForRendering(this, scene::ESNRP_SOLID);

			// Rend les feuilles visibles
			if (LeafNode)
				LeafNode->setVisible(true);
		}
		else					// Sinon, on dessinera un billboard
		{
			SceneManager->registerNodeForRendering(this, scene::ESNRP_TRANSPARENT);

			// Rend les feuilles invisibles
			if (LeafNode)
				LeafNode->setVisible(false);
		}
    }
    ISceneNode::OnRegisterSceneNode();
}

void CTreeSceneNode::render()
{
    video::IVideoDriver* const driver = SceneManager->getVideoDriver();

	const scene::E_SCENE_NODE_RENDER_PASS currentRenderPass = SceneManager->getSceneNodeRenderPass();

	if (currentRenderPass == ESNRP_SOLID)	// Dessine les matériaux solides
	{
		driver->setMaterial( TreeMaterial );

		if (DistSQ < FarRange && MidRange < DistSQ && MidLODMeshBuffer)
		{
			// Dessine le niveau de détail moyen
			driver->setTransform( video::ETS_WORLD, AbsoluteTransformation );

			driver->drawMeshBuffer( MidLODMeshBuffer );
		}
		else if (DistSQ < MidRange && HighLODMeshBuffer)
		{
			// Dessine le niveau de détail haut
			driver->setTransform( video::ETS_WORLD, AbsoluteTransformation );

			driver->drawMeshBuffer( HighLODMeshBuffer );
		}
	}
	else if (currentRenderPass == ESNRP_TRANSPARENT)	// Dessine les matériaux transparents
	{
		if ( FarRange < DistSQ && BillboardMaterial.TextureLayer[0].Texture)
		{
			// Met à jour le billboard de l'arbre
			updateBillboard();

			// Dessine le billboard général de l'arbre (niveau de détail bas)
			driver->setMaterial( BillboardMaterial );
			driver->setTransform( video::ETS_WORLD, core::matrix4() );
			driver->drawMeshBuffer(&BillboardMeshBuffer);
		}

		// On ne dessine pas les billboards des niveaux de détail moyens et haut car ils sont dessinés par le node Leafs
	}
}

void CTreeSceneNode::updateBillboard()
{
    const ICameraSceneNode* const camera = SceneManager->getActiveCamera();
    if (!camera)
        return;

	const core::vector3df pos = getAbsolutePosition();
    core::vector3df view = pos - camera->getAbsolutePosition();
    view.normalize();

    core::vector3df up = core::vector3df(0,1,0);
   
    AbsoluteTransformation.rotateVect(up);
    up.normalize();

    core::vector3df left = view.crossProduct(up);
    left.normalize();

    core::vector3df extent = HighLODMeshBuffer->BoundingBox.getExtent();
    
    core::vector3df yscale = core::vector3df( 0, 1, 0 );
    AbsoluteTransformation.rotateVect( yscale );    // Find the y scale and apply to the height

    up *= extent.Y * yscale.getLength();

    core::vector3df xz = core::vector3df( 1, 0, 1 );    // Find the xz scale and apply to the width

    AbsoluteTransformation.rotateVect(xz);    
    extent.Y = 0.0f;
    f32 len = extent.getLength() * xz.getLength() / 1.4f;   // Divide by 1.4f to compensate for the extra length of xz.

	left *= (len * 0.5f);

	/*
        1--2
        |  |
        0--3
    */

	BillboardMeshBuffer.Vertices[0].Pos = pos - left;
    BillboardMeshBuffer.Vertices[1].Pos = pos - left + up;
    BillboardMeshBuffer.Vertices[2].Pos = pos + left + up;
    BillboardMeshBuffer.Vertices[3].Pos = pos + left;
}

// Fonctions implémentées :
void CTreeSceneNode::setPosition(const core::vector3df& newpos)
{
	RelativeTranslation = newpos;

	ISceneNode::updateAbsolutePosition();
	if (LeafNode)
		LeafNode->updateAbsolutePosition();
}
void CTreeSceneNode::setScale(const core::vector3df& scale)
{
	RelativeScale = scale;

	Size = HighLODMeshBuffer ? (HighLODMeshBuffer->BoundingBox.getExtent() * scale).getLength() * 0.5f : 1.0f;
}

} // namespace scene
} // namespace irr
