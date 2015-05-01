/*  
    Written by Asger Feldthaus
    
    February 2007
*/

#ifndef DEF_C_BILLBOARD_GROUP_SCENE_NODE
#define DEF_C_BILLBOARD_GROUP_SCENE_NODE

#include "CBillboardGroupSceneNode.h"

namespace irr
{
namespace scene
{

CBillboardGroupSceneNode::CBillboardGroupSceneNode( ISceneNode* parent, ISceneManager* manager, s32 id,
        const core::vector3df& position,
        const core::vector3df& rotation,
        const core::vector3df& scale )
    : ISceneNode( parent, manager, id, position, rotation, scale ), LastCamDir(-1.0f), lastAbsPos(-1.0f), lastAbsScale(-1.0f)
{
    //FarDistance = 256.0f;
    //Radius = 0.0f;

	MeshBuffer = new SMeshBuffer();
    MeshBuffer->Material.Lighting = true;	// false	// Permet d'activer la lumière sur les feuilles des arbres
    MeshBuffer->Material.FogEnable = true;	// Ajouté : Permet d'activer le brouillard

	// Ajouté : Indique au mesh buffer les conseils de stockage pour la carte graphique des informations sur ses indices et ses vertices
	MeshBuffer->setHardwareMappingHint(scene::EHM_DYNAMIC, scene::EBT_VERTEX);	// Vertices régulièrement modifiées : elles se déplacent pour toujours être face à la caméra
	MeshBuffer->setHardwareMappingHint(scene::EHM_STATIC, scene::EBT_INDEX);		// Indices jamais modifiés : ils sont créés à l'initialisation et restent constants entre les rendus
}

CBillboardGroupSceneNode::~CBillboardGroupSceneNode()
{
	if (MeshBuffer)
		MeshBuffer->drop();
}

void CBillboardGroupSceneNode::addBillboard( const core::vector3df& position, const core::dimension2df& size, f32 roll, const video::SColor& vertexColor )
{
    Billboards.push_back( SBillboard( position, size, roll, vertexColor ) );

	const s32 vertexIndex = MeshBuffer->Vertices.size();

	const f32 maxDimension = max(size.Width, size.Height);

	const core::vector3df vDim = core::vector3df(maxDimension);

	BoundingBox.addInternalBox( core::aabbox3df(
            position - vDim,
            position + vDim
        ));
    
    //Radius = BoundingBox.getExtent().getLength() * 0.5f;
    
    // Don't bother setting correct positions here, they are updated in updateBillboards anyway
	const core::vector3df normal(0.0f, 1.0f, 0.0f);
    MeshBuffer->Vertices.push_back( video::S3DVertex( position, normal, vertexColor, core::vector2df(0,0) ) );
    MeshBuffer->Vertices.push_back( video::S3DVertex( position, normal, vertexColor, core::vector2df(1,0) ) );
    MeshBuffer->Vertices.push_back( video::S3DVertex( position, normal, vertexColor, core::vector2df(1,1) ) );
    MeshBuffer->Vertices.push_back( video::S3DVertex( position, normal, vertexColor, core::vector2df(0,1) ) );
    
    /*
        Vertices are placed like this:
            0---1
            |   |
            3---2    
    */
    
    MeshBuffer->Indices.push_back( vertexIndex );
    MeshBuffer->Indices.push_back( vertexIndex + 1 );
    MeshBuffer->Indices.push_back( vertexIndex + 2 );
    
    MeshBuffer->Indices.push_back( vertexIndex + 2 );
    MeshBuffer->Indices.push_back( vertexIndex + 3 );
    MeshBuffer->Indices.push_back( vertexIndex );
}

void CBillboardGroupSceneNode::addBillboardWithAxis( const core::vector3df& position, const core::dimension2df& size, const core::vector3df& axis, f32 roll, const video::SColor& vertexColor )
{
    const s32 index = Billboards.size();

	addBillboard( position, size, roll, vertexColor );

	Billboards[index].HasAxis = true;
	Billboards[index].Axis = axis;
}

/*
void CBillboardGroupSceneNode::OnRegisterSceneNode()
{
    if ( IsVisible )
    {
        SceneManager->registerNodeForRendering( this );
    }
}
*/

// Fonction corrigée
void CBillboardGroupSceneNode::OnRegisterSceneNode()
{
    if (IsVisible)
		SceneManager->registerNodeForRendering(this, scene::ESNRP_TRANSPARENT);
	ISceneNode::OnRegisterSceneNode();
}

void CBillboardGroupSceneNode::render()
{
    updateBillboards();

	video::IVideoDriver* const driver = SceneManager->getVideoDriver();

	driver->setTransform( video::ETS_WORLD, core::matrix4() );

	driver->setMaterial( MeshBuffer->Material );

	driver->drawMeshBuffer( MeshBuffer );
}

void CBillboardGroupSceneNode::applyVertexShadows( const core::vector3df& lightDir, f32 intensity, f32 ambient )
{
	const u32 billboardsSize = Billboards.size();
    for ( u32 i=0; i<billboardsSize; ++i )
    {
        core::vector3df normal = Billboards[i].Position;
        
        normal.normalize();
        
        const f32 light = core::clamp(ambient - lightDir.dotProduct(normal) * intensity, 0.0f, 1.0f);
        
        /*video::SColor color;
        
        color.setRed( (u8)(Billboards[i].Color.getRed() * light) );
        color.setGreen( (u8)(Billboards[i].Color.getGreen() * light) );
        color.setBlue( (u8)(Billboards[i].Color.getBlue() * light) );
        color.setAlpha( Billboards[i].Color.getAlpha() );*/
        
        for ( s32 j=0; j<4; ++j )
        {
            //MeshBuffer->Vertices[i*4+j].Color = color;
            MeshBuffer->Vertices[i*4+j].Color.set(Billboards[i].Color.getAlpha(), (u8)(Billboards[i].Color.getRed() * light),
				(u8)(Billboards[i].Color.getGreen() * light), (u8)(Billboards[i].Color.getBlue() * light));
        }
    }
}

//! Gives vertices back their original color.
void CBillboardGroupSceneNode::resetVertexShadows()
{
	const u32 billboardsSize = Billboards.size();
    for ( u32 i=0; i<billboardsSize; ++i )
    {
        for ( u32 j=0; j<4; ++j )
        {
            MeshBuffer->Vertices[i*4+j].Color = Billboards[i].Color;
        }
    }
}

void CBillboardGroupSceneNode::updateBillboards()
{
    const ICameraSceneNode* const camera = SceneManager->getActiveCamera();
	if ( !camera )
        return;
    
    core::vector3df camPos = camera->getAbsolutePosition();
        
    core::vector3df ref = core::vector3df(0,1,0);
    camera->getAbsoluteTransformation().rotateVect(ref);
    
    core::vector3df view, right, up;
    
    
    core::vector3df center = BoundingBox.getCenter();
    AbsoluteTransformation.transformVect(center);
    
    const core::vector3df camDir = camPos - center;
    
	// Test pour déterminer si l'arbre est à une distance assez éloignée de la caméra désactivé :
	// Dans les conditions d'EcoWorld, l'arbre est déjà affiché sous sa forme de billboard (et on ne voit donc plus ses feuilles : ce node n'est donc plus affiché) avant que l'éloignement de la caméra ne soit détecté par ce test
	/*
    bool farAway = false;
	const float tmp = FarDistance + Radius;
    if ( camDir.getLengthSQ() >= tmp * tmp )
    {
        farAway = true;
        view = center - camPos;
        view.normalize();
        
        right = ref.crossProduct( view );
        
        up = view.crossProduct( right );
    }
	*/
    
    core::vector3df rotatedCamDir = camDir;
    AbsoluteTransformation.inverseRotateVect( rotatedCamDir );
    
	// Optimisation améliorée : les feuilles des arbres étaient parfois complètement mal placées, alors on vérifie juste si la position de la caméra est différente avec un très léger seuil
    //if ( farAway )
	//	if ((rotatedCamDir - LastCamDir).getLengthSQ() < 1000.0f )
	//		return;

	// On quitte seulement si la position de la caméra est très proche de son ancienne position (aux précisions des calculs près en fait),
	// et que la position absolue et la taille absolue de ce node sont identiques.
	const core::vector3df absPosition = AbsoluteTransformation.getTranslation();
	const core::vector3df absScale = AbsoluteTransformation.getScale();
	if (rotatedCamDir.equals(LastCamDir) && absPosition.equals(lastAbsPos) && absScale.equals(lastAbsScale))
		return;

    LastCamDir = rotatedCamDir;
	lastAbsPos = absPosition;
    lastAbsScale = absScale;

    // Update the position of every billboard
	const u32 billboardsSize = Billboards.size();
    for ( u32 i=0; i<billboardsSize; ++i )
    {
        //if ( !farAway )
        //{
            core::vector3df pos = Billboards[i].Position;
            
            AbsoluteTransformation.transformVect( pos );
            
            view = pos - camPos;
            
            view.normalize();
        //}
        
        core::vector3df thisRight = right;
        core::vector3df thisUp = up;
        
        if ( Billboards[i].HasAxis )
        {
            core::vector3df axis = Billboards[i].Axis;
            
            AbsoluteTransformation.rotateVect(axis);
            
            thisRight = axis.crossProduct( view );
            
            thisUp = axis;
        }
        else //if ( !farAway )
        {
            thisRight = ref.crossProduct( view );
            
            thisUp = view.crossProduct( thisRight );
        }
        
        const f32 rollrad = Billboards[i].Roll * core::DEGTORAD;
        const f32 cos_roll = cosf( rollrad );
        const f32 sin_roll = sinf( rollrad );
        
        core::vector3df a =  cos_roll * thisRight + sin_roll * thisUp;
        core::vector3df b = -sin_roll * thisRight + cos_roll * thisUp;
        
		// Modifié : Permet de mettre à jour la taille des feuilles avec l'agrandissement actuel de ce node (déjà effectué si le billboard actuel a des axes) :
		if (Billboards[i].HasAxis)
		{
			a *= Billboards[i].Size.Width * 0.5f;
			b *= Billboards[i].Size.Height * 0.5f;
		}
		else
		{
			// Utilisation de la valeur absolue de l'agrandissement, car scale peut être négatif, et les feuilles ne sont pas visibles si leur taille est négative
			a *= Billboards[i].Size.Width * 0.5f * abs(absScale.X);		// a *= Billboards[i].Size.Width * 0.5f
			b *= Billboards[i].Size.Height * 0.5f * abs(absScale.Y);	// b *= Billboards[i].Size.Height * 0.5f
		}
        
        const s32 vertexIndex = i * 4;  // 4 vertices per billboard
        
        core::vector3df billPos = Billboards[i].Position;
        
        AbsoluteTransformation.transformVect(billPos);
        
        MeshBuffer->Vertices[vertexIndex  ].Pos = billPos - a + b;
        MeshBuffer->Vertices[vertexIndex+1].Pos = billPos + a + b;
        MeshBuffer->Vertices[vertexIndex+2].Pos = billPos + a - b;
        MeshBuffer->Vertices[vertexIndex+3].Pos = billPos - a - b;
    }

	// Indique que les vertices de ce MeshBuffer sont à remettre à jour
	MeshBuffer->setDirty(scene::EBT_VERTEX);
}

} // namespace scene
} // namespace irr

#endif
