/***************************************************************
*                Terrain Engine issue of					   *
*							Dreamotion3D engine				   *
*															   *
**************************************************************** 
*File Created: 29.03.08                                        *
*Author:			                                           *
*Contens: Implementation of QuadTree   algorithme              *
***************************************************************/

// Cette version a été modifiée

#include "CQuad.h"
#include "CList.h"
#include "CHeight.h"
#include "CTerrainNode_Tmyke.h"
//#pragma warning( disable : 4996 ) // disable deprecated warning 


using namespace irr;
using namespace core;


//*******************************************************************************
//********* PROTOTYPES LOCAUX ***************************************************
//*******************************************************************************
float distance3(vector3df* p1, vector3df *p2)
{
	//return sqrtf(powf(p2->X-p1->X,2)+powf(p2->Y-p1->Y,2)+powf(p2->Z-p1->Z,2));

	// Nouveau (Ancien ci-dessus) :
	return (p2->getDistanceFrom(*p1));
}


/*********************************************************************************
//********************************************************************************
//********************************************************************************/
//-------------------------------------------------
// CQuadNode()
//-------------------------------------------------
// constructeur 
//
// *parent : parent dans l'arbre quadtree
// *mgr    : récupération du pointeur ISceneManager
CQuadNode::CQuadNode(CQuadNode *Parent, CTerrainNode *Terrain, scene::ISceneManager* mgr) : selector(0)
{
    m_pParent      = Parent;
	for(int i = 0; i < 4; ++i)
	{
	  m_pChildren[i] = NULL;
  	  m_iCorners[i].set(0.0f,0.0f,0.0f);
	}
	m_pQuadTerrain    = Terrain;
	//m_cCenter         = vector3df( 0,0,0 );
	mbuffer=NULL;
}
//-------------------------------------------------
// ~CQuadNode()
//-------------------------------------------------
// Destructeur 
CQuadNode::~CQuadNode()
{
	if (selector)
		selector->drop();
}
//-------------------------------------------------
// Destroy()
//-------------------------------------------------
// fonction de libération des ressources
void CQuadNode::Destroy()
{
	m_pQuadTerrain->DelQuad.add(this);
	if (m_pChildren[0] != NULL )
	{
		for (int i = 0; i < 4; ++i)
			m_pChildren[i]->Destroy();
	} 
	else if (mbuffer)
		delete mbuffer;

	if (selector)
		selector->drop();
	selector = NULL;
}
//-------------------------------------------------
// SetBox()
//-------------------------------------------------
// établit la boite englobante de chaque élément
// de l'arbre
void CQuadNode::SetBox()
{
	vector3df max = vector3df(0,0,0);
	vector3df min = vector3df(999990,999990,999990);
	for(int i=0; i<4; ++i)
	{
		if(m_iCorners[i].X<min.X) min.X =  m_iCorners[i].X;
		if(m_iCorners[i].Y<min.Y) min.Y =  m_iCorners[i].Y;
		if(m_iCorners[i].Z<min.Z) min.Z =  m_iCorners[i].Z;
		
		if(m_iCorners[i].X>max.X) max.X =  m_iCorners[i].X;
		if(m_iCorners[i].Y>max.Y) max.Y =  m_iCorners[i].Y;
		if(m_iCorners[i].Z>max.Z) max.Z =  m_iCorners[i].Z;
	}
	min.Y=0.0f;
	max.Y=0.0f;
	Box.MaxEdge = max;
	Box.MinEdge = min;
	//Box = aabbox3d<f32>(min.X,  min.Y, min.Z, max.X, max.Y, max.Z);
}
//-------------------------------------------------
// SetMaxBox()
//-------------------------------------------------
// redéfinit rétrospectivement la boite englobante
// de chaque élément avec la valeur max en hauteur
void CQuadNode::SetMaxBox()
{	
	Box.MaxEdge.Y = m_pQuadTerrain->max_h;
  if( m_pChildren[0] != NULL )
  {	// c'est un noeud
	  if( m_pChildren[0] )
		  m_pChildren[0]->SetMaxBox();
	  if( m_pChildren[1] )
		  m_pChildren[1]->SetMaxBox();
	  if( m_pChildren[2] )
		  m_pChildren[2]->SetMaxBox();
	  if( m_pChildren[3] )
		  m_pChildren[3]->SetMaxBox();
  }  
}
//-------------------------------------------------
// Subdivide()
//-------------------------------------------------
// Subdivision récursive des éléments de l'arbre pour
// arriver au plus petit élément souhaité
bool CQuadNode::Subdivide()
{
	vector3df iCorners[4];
    float x0 = ( m_iCorners[0].X + m_iCorners[1].X ) * 0.5f;
	float z0 = ( m_iCorners[0].Z + m_iCorners[1].Z ) * 0.5f;

    float x1 = ( m_iCorners[1].X + m_iCorners[2].X ) * 0.5f;
	float z1 = ( m_iCorners[1].Z + m_iCorners[2].Z ) * 0.5f;

    float x2 = ( m_iCorners[3].X + m_iCorners[2].X ) * 0.5f;
	float z2 = ( m_iCorners[3].Z + m_iCorners[2].Z ) * 0.5f;

    float x3 = ( m_iCorners[3].X + m_iCorners[0].X ) * 0.5f;
	float z3 = ( m_iCorners[3].Z + m_iCorners[0].Z ) * 0.5f;

	const vector3df center = (m_iCorners[0]+m_iCorners[1]+m_iCorners[2]+m_iCorners[3]) * 0.25f; 
	//radius = distance3( &m_iCorners[0], &center);

	if( ( m_iCorners[1].X - m_iCorners[0].X ) <= (m_pQuadTerrain->m_iSize/(float)m_pQuadTerrain->m_rc))
					return false;   //on ne peut pas subdiviser plus

	// children 0
	m_pChildren[ 0 ] = new CQuadNode( this, m_pQuadTerrain );
	 if( !m_pChildren[0] )
	 {	 printf(" <<ERROR>> CQuad(Subdivide): Subdivide: out of memory  \r\n");                         
		 return false;
	 }
	 iCorners[0] = m_iCorners[0];
	 iCorners[1] = vector3df(x0,0.0,z0);
	 iCorners[2] = center;
	 iCorners[3] = vector3df(x3,0.0,z3);
	 m_pChildren[ 0 ]->Allocate( iCorners);
	 m_pChildren[ 0 ]->SetBox();

     m_pChildren[ 1 ] = new CQuadNode(this, m_pQuadTerrain );
	 if( !m_pChildren[1] )
	 {	 printf(" <<ERROR>> CQuad(Subdivide): Subdivide: out of memory  \r\n");                         
		 return false;
	 }
	 iCorners[0] = vector3df(x0,0.0,z0);
	 iCorners[1] = m_iCorners[1];
	 iCorners[2] = vector3df(x1,0.0,z1);
	 iCorners[3] = center;
	 m_pChildren[ 1 ]->Allocate( iCorners);
	 m_pChildren[ 1 ]->SetBox();

     m_pChildren[ 2 ] = new CQuadNode( this, m_pQuadTerrain );
	 if( !m_pChildren[2] )
	 {   printf(" <<ERROR>> CQuad(Subdivide): Subdivide: out of memory  \r\n");                         
		 return false;
	 }
	 iCorners[0] = center;
	 iCorners[1] = vector3df(x1,0.0,z1);
	 iCorners[2] = m_iCorners[2];
	 iCorners[3] = vector3df(x2,0.0,z2);
	 m_pChildren[ 2 ]->Allocate( iCorners );
	 m_pChildren[ 2 ]->SetBox();

     m_pChildren[ 3 ] = new CQuadNode( this, m_pQuadTerrain );
	 if( !m_pChildren[3] )
	 {	 printf(" <<ERROR>> CQuad(Subdivide): Subdivide: out of memory  \r\n");                         
		 return false;
	 }
	 iCorners[0] = vector3df(x3,0.0,z3);
	 iCorners[1] = center;
	 iCorners[2] = vector3df(x2,0.0,z2);
	 iCorners[3] = m_iCorners[3];
	 m_pChildren[ 3 ]->Allocate( iCorners );
	 m_pChildren[ 3 ]->SetBox();

  return true;
}
//-------------------------------------------------
// Allocate()
//-------------------------------------------------
// Allocation des valeurs de base pour les cellules
// filles
// Corners[4] : prochaines dimension à transmettre
bool CQuadNode::Allocate( vector3df Corners[4])
{
	for(int i = 0; i < 4; ++i)
	{
	  m_pChildren[i] = NULL;
	  m_iCorners[i]  = Corners[i];
	}
	//m_cCenter = (m_iCorners[0]+m_iCorners[1]+m_iCorners[2]+m_iCorners[3]) * 0.25f;
	return true;
}
//-------------------------------------------------
// BuildQuadTree()
//-------------------------------------------------
// point d'entré pour la construction de l'arbre
void CQuadNode::BuildQuadTree()
{
    if( Subdivide() )
	{
		m_pChildren[0]->BuildQuadTree();
		m_pChildren[1]->BuildQuadTree();
		m_pChildren[2]->BuildQuadTree();
		m_pChildren[3]->BuildQuadTree();
	}	
	//else
	//	m_pQuadTerrain->numSheet++;
}
//-------------------------------------------------
// CalculYCorner()
//-------------------------------------------------
// calcul des coins d'un quad
void CQuadNode::CalculYCorner()
{
	const float f = 1.0f / m_pQuadTerrain->m_rk;

  for(u8 i=0; i<4; ++i)
	  m_iCorners[i].Y = m_pQuadTerrain->m_pHeightMap->GetHeightValue( m_iCorners[i].X*f, m_iCorners[i].Z*f);

  if( m_pChildren[0] )
  {	// c'est un noeud
	  if( m_pChildren[0] )
		  m_pChildren[0]->CalculYCorner();
	  if( m_pChildren[1] )
		  m_pChildren[1]->CalculYCorner();
	  if( m_pChildren[2] )
		  m_pChildren[2]->CalculYCorner();
	  if( m_pChildren[3] )
		  m_pChildren[3]->CalculYCorner();
  } 
  else
	  return;
}
//-------------------------------------------------
// Create3D_Quad()
//-------------------------------------------------
// point d'entré pour la creation géometrique des
// éléments terminaux de l'abre
#define DIFF 0xFFFFFFFF
void CQuadNode::Create3D_Quad(scene::ISceneManager *mgr)
{ 
	if( m_pChildren[0] )
	{	// c'est un noeud
	  if( m_pChildren[0] )
		  m_pChildren[0]->Create3D_Quad(mgr);
	  if( m_pChildren[1] )
		  m_pChildren[1]->Create3D_Quad(mgr);
	  if( m_pChildren[2] )
		  m_pChildren[2]->Create3D_Quad(mgr);
	  if( m_pChildren[3] )
		  m_pChildren[3]->Create3D_Quad(mgr);
	}  
	else
	{ // c'est un Sheet, creation du 3D Buffer
			CreateQuadMesh(mgr);
	}
	// calcul le centre geométrique des coins
	//vector3df coin = 	m_iCorners[0]+m_iCorners[1]+m_iCorners[2]+m_iCorners[3]; 
	//coin = coin * 0.25f;
	//d_mCorners = coin;
}
//-------------------------------------------------
// CreateQuadMesh()
//-------------------------------------------------
// construction 3D d'un élément noeud terminal.
// volontairement décomposé, pour une meilleurs
// compréhension
void CQuadNode::CreateQuadMesh(scene::ISceneManager* mgr)
{
	// Fonction globalement modifiée au niveau du texturage des vertices + modifiée pour supporter des tailles de terrain de 2^n+1

	// instanciation du mbuffer
	mbuffer=new scene::SMeshBufferLightMap();

	// réservaton des espaces nescessaire dans les buffer Vertices et Indices
	const int heightMapLargMin1 = m_pQuadTerrain->m_larg - 1;
	mbuffer->Indices.set_used(heightMapLargMin1 * heightMapLargMin1 * 6);
	mbuffer->Vertices.set_used(heightMapLargMin1 * heightMapLargMin1 * 4);

	// coin de départ
	const vector3df& origin = m_iCorners[0];

	// calcul du 'pas' pour les texture map
	const float pa  = 1.0f/(float)(m_pQuadTerrain->m_rc*heightMapLargMin1);
	const float pa2 = 1.0f/(float)heightMapLargMin1;

	// calcul du 'pas' entre les faces
	const float ps = abs(m_iCorners[0].X-m_iCorners[1].X)/(float)heightMapLargMin1;

	const float index = 1.0f/(m_pQuadTerrain->m_iSize/(float)m_pQuadTerrain->m_rc);
	const int icx = (int)(m_iCorners[0].X * index);		// icz = (...)
	const int icz = (int)(m_iCorners[0].Z * index);		// icx = (...)

	const float facteur = 1.0f/m_pQuadTerrain->m_rk;

	/*float ii = 1.0f - (float)(icx * m_pQuadTerrain->m_larg) * pa - pa;
	float jj = 0.0f + (float)(icz * m_pQuadTerrain->m_larg) * pa;
	float i2 = 1.0f - pa2;
	float j2 = 0.0f;*/

	float ii = (float)(icx * heightMapLargMin1) * pa;
	float jj = 1.0f - (float)(icz * heightMapLargMin1) * pa - pa;
	float i2 = 0.0f;
	float j2 = 1.0f - pa2;

	// on a enfin les éléments pour constituer la geométrie
	int num = 0;
	short p = 0;	// maintenant p sert d'index	
	for (short i = 0; i < heightMapLargMin1; ++i)
	{
		for (short j = 0; j < heightMapLargMin1; ++j)
		{
			// triangle - P0
			const float ju = j*ps;
			const float iu = i*ps;
			mbuffer->Vertices[p].Pos.X = origin.X+ju+ps;
			mbuffer->Vertices[p].Pos.Z = origin.Z+iu;
			mbuffer->Vertices[p].Pos.Y = m_pQuadTerrain->m_pHeightMap->GetHeightValue( mbuffer->Vertices[p].Pos.X*facteur,mbuffer->Vertices[p].Pos.Z*facteur );
			/*mbuffer->Vertices[p].TCoords.X = ii+pa;
			mbuffer->Vertices[p].TCoords.Y = jj+pa;
			mbuffer->Vertices[p].TCoords2.X = i2+pa2;
			mbuffer->Vertices[p].TCoords2.Y = j2+pa2;*/
			mbuffer->Vertices[p].TCoords.X = ii+pa;
			mbuffer->Vertices[p].TCoords.Y = jj+pa;
			mbuffer->Vertices[p].TCoords2.X = i2+pa2;
			mbuffer->Vertices[p].TCoords2.Y = j2+pa2;
			mbuffer->Vertices[p].Color = DIFF;
			p++;
			// triangle - P1
			mbuffer->Vertices[p].Pos.X = origin.X+ju;
			mbuffer->Vertices[p].Pos.Z = origin.Z+iu;
			mbuffer->Vertices[p].Pos.Y = m_pQuadTerrain->m_pHeightMap->GetHeightValue( mbuffer->Vertices[p].Pos.X*facteur,mbuffer->Vertices[p].Pos.Z*facteur );
			/*mbuffer->Vertices[p].TCoords.X = ii+pa;
			mbuffer->Vertices[p].TCoords.Y = jj;
			mbuffer->Vertices[p].TCoords2.X = i2+pa2;
			mbuffer->Vertices[p].TCoords2.Y = j2;*/
			mbuffer->Vertices[p].TCoords.X = ii;
			mbuffer->Vertices[p].TCoords.Y = jj+pa;
			mbuffer->Vertices[p].TCoords2.X = i2;
			mbuffer->Vertices[p].TCoords2.Y = j2+pa2;
			mbuffer->Vertices[p].Color = DIFF;
			p++;
			// triangle - P2
			mbuffer->Vertices[p].Pos.X = origin.X+ju;
			mbuffer->Vertices[p].Pos.Z = origin.Z+iu+ps;
			mbuffer->Vertices[p].Pos.Y = m_pQuadTerrain->m_pHeightMap->GetHeightValue( mbuffer->Vertices[p].Pos.X*facteur,mbuffer->Vertices[p].Pos.Z*facteur );
			/*mbuffer->Vertices[p].TCoords.X = ii;
			mbuffer->Vertices[p].TCoords.Y = jj;
			mbuffer->Vertices[p].TCoords2.X = i2;
			mbuffer->Vertices[p].TCoords2.Y = j2;*/
			mbuffer->Vertices[p].TCoords.X = ii;
			mbuffer->Vertices[p].TCoords.Y = jj;
			mbuffer->Vertices[p].TCoords2.X = i2;
			mbuffer->Vertices[p].TCoords2.Y = j2;
			mbuffer->Vertices[p].Color = DIFF;
			p++;
			// triangle	- P3
			mbuffer->Vertices[p].Pos.X = origin.X+ju+ps;
			mbuffer->Vertices[p].Pos.Z = origin.Z+iu+ps;
			mbuffer->Vertices[p].Pos.Y = m_pQuadTerrain->m_pHeightMap->GetHeightValue( mbuffer->Vertices[p].Pos.X*facteur,mbuffer->Vertices[p].Pos.Z*facteur );
			/*mbuffer->Vertices[p].TCoords.X = ii;
			mbuffer->Vertices[p].TCoords.Y = jj+pa;
			mbuffer->Vertices[p].TCoords2.X = i2;
			mbuffer->Vertices[p].TCoords2.Y = j2+pa2;*/
			mbuffer->Vertices[p].TCoords.X = ii+pa;
			mbuffer->Vertices[p].TCoords.Y = jj;
			mbuffer->Vertices[p].TCoords2.X = i2+pa2;
			mbuffer->Vertices[p].TCoords2.Y = j2;
		  	mbuffer->Vertices[p].Color = DIFF;
			p++;

			/*jj += pa;
			j2 += pa2;
			if (j2 > 0.99f) { j2 = 0.0f; i2 -= pa2; }*/

			ii += pa;
			i2 += pa2;
			if (i2 > 0.99f) { i2 = 0.0f; j2 -= pa2; }

			mbuffer->Indices[num] =   0 + p-4 ;	// 0
			mbuffer->Indices[num+1] = 1 + p-4 ;	// 1
			mbuffer->Indices[num+2] = 2 + p-4 ;	// 2
			mbuffer->Indices[num+3] = 0 + p-4 ;	// 0
			mbuffer->Indices[num+4] = 2 + p-4 ;	// 2
			mbuffer->Indices[num+5] = 3 + p-4 ;	// 3
			num += 6;
		}
		/*jj = (float)(icz * m_pQuadTerrain->m_larg) * pa;
		ii -= pa;*/

		ii = (float)(icx * heightMapLargMin1) * pa;
		jj -= pa;
	}

	// Ajouté : Permet d'activer les VBO sous OpenGL
	mbuffer->setHardwareMappingHint(scene::EHM_STATIC);

	mbuffer->recalculateBoundingBox();
	Box = mbuffer->BoundingBox;
	// on fait recalculer les normal smooth par Irrlicht (pas très concluant pour le moment)
	scene::IMeshManipulator *manipulator = mgr->getMeshManipulator();
	manipulator->recalculateNormals(mbuffer, true);
	// récupération de la hauteur maxi du terrain
	Box.MinEdge.Y=0;
	if(Box.MaxEdge.Y > m_pQuadTerrain->max_h) m_pQuadTerrain->max_h = Box.MaxEdge.Y;
}
//-------------------------------------------------
// AffecteQuad()
//-------------------------------------------------
// Pour facilité la manipulation des éléments nodes 
// 3D une fois l'arbre créé, on les cole dans un tableau
// facilement consultable
/*void CQuadNode::AffecteQuad()
{	
  if( m_pChildren[0] )
  {	// c'est un noeud
	  if( m_pChildren[0] )
		  m_pChildren[0]->AffecteQuad();
	  if( m_pChildren[1] )
		  m_pChildren[1]->AffecteQuad();
	  if( m_pChildren[2] )
		  m_pChildren[2]->AffecteQuad();
	  if( m_pChildren[3] )
		  m_pChildren[3]->AffecteQuad();
  }  
  else
  {	// calcul	
	int x = (int) m_iCorners[1].X;
	int z = (int) m_iCorners[3].Z;
	int f = (int) m_pQuadTerrain->m_iSize / m_pQuadTerrain->m_rc;
	x = (x / f)-1;
	z = (z / f)-1;
	x = x + (z * m_pQuadTerrain->m_rc);
	m_pQuadTerrain->grille[x] = this;
  }
}*/
//-------------------------------------------------
// RenderQuad()
//-------------------------------------------------
// la pass de rendu 
void CQuadNode::RenderQuad( scene::ISceneManager* mgr , int FrustumCode)
{
  //float rr=0;

  // parcours de l'arbre pour représenter  
  if( !m_pParent )
  {
	  FrustumCode = 1; //FRUSTUM_INTERSECT; 
  } 
  else
  {
 	if( FrustumCode )	//FRUSTUM_ALLIN
	{	
		// Modifié pour prendre en compte les transformations du terrain
		//FrustumCode = m_pQuadTerrain->_isVisible(Box);
		core::aabbox3df box_ = Box;
		m_pQuadTerrain->getAbsoluteTransformation().transformBoxEx(box_);
		FrustumCode = m_pQuadTerrain->_isVisible(box_);

		if( !FrustumCode  && m_pParent )		return; 
	}
  }

  if( m_pChildren[0] )
  {	// c'est un noeud
	  if( m_pChildren[0] )
	  		m_pChildren[0]->RenderQuad(mgr, FrustumCode );
	  if( m_pChildren[1] )
			m_pChildren[1]->RenderQuad(mgr, FrustumCode );
	  if( m_pChildren[2] )
			m_pChildren[2]->RenderQuad(mgr, FrustumCode );
	  if( m_pChildren[3] )
			m_pChildren[3]->RenderQuad(mgr, FrustumCode );
  }  
  else
  {		//m_pQuadTerrain->nQuad++;
        //rendu  du mesh;
		video::IVideoDriver* driver = mgr->getVideoDriver();

		//DEBUG ZONE
		/*if(Debug)
		{
		    video::SMaterial mat;
			mat.AmbientColor = video::SColor(255,255,255,255);
		    mat.DiffuseColor = video::SColor(255,255,255,255);
		    mat.EmissiveColor = video::SColor(255,0,255,0);
		    driver->setMaterial(mat);
			driver->draw3DBox(Box, video::SColor(255,255,255,255));
		}*/
		//driver->setMaterial(Material);	// Désactivé pour prendre le material de CTerrainNode.cpp au lieu de celui de ce quad
		/*driver->drawIndexedTriangleList(&mbuffer->Vertices[0],
										mbuffer->getVertexCount(),
										&mbuffer->Indices[0], 
										mbuffer->getIndexCount()/3 );*/
		driver->drawMeshBuffer(mbuffer);	// Permet d'activer les VBO sous OpenGL

  }
}
void CQuadNode::createTriangleSelectors(scene::ISceneManager* smgr, scene::IMetaTriangleSelector* metaTriSelector)
{
	if (!smgr)
		return;

	if (m_pChildren[0])
	{
		if(m_pChildren[0])
		  m_pChildren[0]->createTriangleSelectors(smgr, metaTriSelector);
		if(m_pChildren[1])
		  m_pChildren[1]->createTriangleSelectors(smgr, metaTriSelector);
		if(m_pChildren[2])
		  m_pChildren[2]->createTriangleSelectors(smgr, metaTriSelector);
		if(m_pChildren[3])
		  m_pChildren[3]->createTriangleSelectors(smgr, metaTriSelector);
	}  
	else
	{
		scene::SMesh mesh;
		mesh.addMeshBuffer(mbuffer);

		// L'utilisation d'un octree triangle selector ici n'est pas toujours justifiée :
		selector = smgr->createOctreeTriangleSelector(&mesh, m_pQuadTerrain);//smgr->createTriangleSelector(&mesh, m_pQuadTerrain)

		if (metaTriSelector)
			metaTriSelector->addTriangleSelector(selector);
	}
}
