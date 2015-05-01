/***************************************************************
*                Terrain Engine issue of					   *
*							Dreamotion3D engine				   *
*															   *
**************************************************************** 
*File Created: 29.03.08                                        *
*Author:			                                           *
*Contens: Implementation of TerrainNode algorithme             *
***************************************************************/

// Cette version a été modifiée

#include <stdio.h>
#include "CTerrainNode_Tmyke.h"

//#pragma warning( disable : 4996 ) // disable deprecated warning 

using namespace irr;

/****************************************************************/
/****************************************************************/
//=========================================================
//---------------------------------------------------------
// Name: constructor()
// 
//---------------------------------------------------------
CTerrainNode::CTerrainNode(scene::ISceneNode* parent, scene::ISceneManager* mgr, s32 id)
										: scene::ISceneNode(parent, mgr, id)
{
	//m_iSize		= 0.0f;
	//numSheet		= 0;
	m_pHeightMap	= new CHeightMap_tmyke();
	max_h			= 0.0f;
	t_mgr			= mgr;
	// valeurs de base par defaut
	m_rc			= 8;
	m_larg			= 16;
	m_iSize			= 2048.0f;

	// Ajouté :
	//m_rk = 0.0f;
	m_pQuadTree = NULL;

	setVisible(true);
	//Material.setFlag(video::EMF_LIGHTING, true);

	Material.Lighting = true;
	Material.BackfaceCulling = true;
	Material.GouraudShading = true;
	Material.FogEnable = true;		// false;

	//setAutomaticCulling(scene::EAC_OFF);

	// Ajouté :
	metaTriSelector = mgr->createMetaTriangleSelector();
}
CTerrainNode::~CTerrainNode()
{
	m_pHeightMap->Destroy();
	delete m_pHeightMap;
	//delete[] grille;

	m_pQuadTree->Destroy();
	for(int i=0; i<DelQuad.nbrelement; ++i)
	{
		CQuadNode *quad = DelQuad.get(i);
		delete quad;
	}
	DelQuad.destroy();

	// Ajouté :
	if (metaTriSelector)
		metaTriSelector->drop();
}
//=========================================================
//---------------------------------------------------------
// Name: SetValues()
// 
// numQuads    : largeur du terrain en nombre de Quads 
//               (valeur multiple de 2^n, 2,4,8,16,32,64,128,256,etc...)
// numFaces    : largeur de chaque Quad en nombre de faces 
//               (résolution de chaque Quad, valeur multiple de 2^n, 2,4,8,16,32,64,128,256,etc...)
// sizeTerrain : taille générale du terrain
//---------------------------------------------------------
void CTerrainNode::SetValues(int numQuads, int numFaces, float sizeTerrain)
{
	m_rc = numQuads;
	m_larg = numFaces;
	m_iSize = sizeTerrain;
}
//=========================================================
//---------------------------------------------------------
// Name: Construct()
// 
//---------------------------------------------------------
void CTerrainNode::Construct(io::path filename, float scale)
{
	LoadHeightMap( filename, scale);


	// intialisation, et construction du noeud de départ
   max_h=0.0f;
   m_pQuadTree = new CQuadNode( NULL, this );
   m_pQuadTree->m_iCorners[0] = vector3df(0.0f,0.0f,0.0f);
   m_pQuadTree->m_iCorners[1] = vector3df(m_iSize ,0.0f,0.0f);
   m_pQuadTree->m_iCorners[2] = vector3df(m_iSize ,0.0f,m_iSize );
   m_pQuadTree->m_iCorners[3] = vector3df(0.0f,0.0f,m_iSize);
   m_pQuadTree->Box.MinEdge = vector3df(0.0f,0.0f,0.0f);
   //m_pQuadTree->Box.MaxEdge = vector3df((float)m_iSize,(float)m_iSize/10.0f,(float)m_iSize);
   m_pQuadTree->Box.MaxEdge = vector3df(m_iSize,1.0f,m_iSize);	// Ajouté
   m_pQuadTree->BuildQuadTree();

	// calcul de quelques valeurs de bases:
	//m_cote = m_pHeightMap->m_iWidth / m_rc;		// raport largeur du HeightMap / m_rc  (par exemple un height de 1024 et 16 quad de cote = 64)
	m_rk = m_iSize / (float)m_pHeightMap->m_iWidth;
	//m_iSizeQuad = m_iSize / m_rc;
	m_pQuadTree->CalculYCorner();

	// init la grille maintenant des Quad 3D
	//int n = m_rc * m_rc;
	//grille = new CQuadNode*[n];
	//max_grille = n;
	//memset(grille,0, sizeof(CQuadNode*)*n);
	//m_pQuadTree->AffecteQuad();
	//numQuad=n;

	// le bounding Box de TerrainNode lui même
	// égale à celui du Quad Supprème
	Box = m_pQuadTree->Box;
	// élargie le boundingBox pour éviter l'effet d'écartement visuel par le rendu
	//Box.MaxEdge *=1.5;	// Modifié : désactivé car la bounding box ne correspondait plus au terrain
	//Box.MinEdge *=1.5;

	m_pQuadTree->Create3D_Quad(t_mgr);
	m_pQuadTree->SetMaxBox();
	//setPosition( vector3df(0,0,0) );
}
void CTerrainNode::Construct(video::IImage* heightMap, float scale)
{
	LoadHeightMap( heightMap, scale);


	// intialisation, et construction du noeud de départ
   max_h=0.0f;
   m_pQuadTree = new CQuadNode( NULL, this );
   m_pQuadTree->m_iCorners[0] = vector3df(0.0f,0.0f,0.0f);
   m_pQuadTree->m_iCorners[1] = vector3df(m_iSize ,0.0f,0.0f);
   m_pQuadTree->m_iCorners[2] = vector3df(m_iSize ,0.0f,m_iSize );
   m_pQuadTree->m_iCorners[3] = vector3df(0.0f,0.0f,m_iSize);
   m_pQuadTree->Box.MinEdge = vector3df(0.0f,0.0f,0.0f);
   //m_pQuadTree->Box.MaxEdge = vector3df((float)m_iSize,(float)m_iSize/10.0f,(float)m_iSize);
   m_pQuadTree->Box.MaxEdge = vector3df(m_iSize,1.0f,m_iSize);	// Ajouté
   m_pQuadTree->BuildQuadTree();

	// calcul de quelques valeurs de bases:
	//m_cote = m_pHeightMap->m_iWidth / m_rc;		// raport largeur du HeightMap / m_rc  (par exemple un height de 1024 et 16 quad de cote = 64)
	m_rk = m_iSize / (float)m_pHeightMap->m_iWidth;
	//m_iSizeQuad = m_iSize / m_rc;
	m_pQuadTree->CalculYCorner();

	// init la grille maintenant des Quad 3D
	//const int n = m_rc * m_rc;
	//grille = new CQuadNode*[n];
	//max_grille = n;
	//memset(grille,0, sizeof(CQuadNode*)*n);
	//m_pQuadTree->AffecteQuad();
	//numQuad=n;

	// le bounding Box de TerrainNode lui même
	// égale à celui du Quad Supprème
	Box = m_pQuadTree->Box;
	// élargie le boundingBox pour éviter l'effet d'écartement visuel par le rendu
	//Box.MaxEdge *=1.5;	// Modifié : désactivé car la bounding box ne correspondait plus au terrain
	//Box.MinEdge *=1.5;

	m_pQuadTree->Create3D_Quad(t_mgr);
	m_pQuadTree->SetMaxBox();
	//setPosition( vector3df(0,0,0) );
}
void CTerrainNode::createTriangleSelectors()
{
	// Crée le triangle selector d'après le premier quad en lui passant notre meta triangle selector en paramêtre
	m_pQuadTree->createTriangleSelectors(t_mgr, metaTriSelector);
}
//=========================================================
//---------------------------------------------------------
// Name: LoadHeightMap()
// 
// desc: chargement de l'image permettant la constitution
// du relief.
//---------------------------------------------------------
bool CTerrainNode::LoadHeightMap(io::path fname, float HScale)
{
	if( !m_pHeightMap->LoadHeightMap( t_mgr, fname, HScale ))
	 {	printf(" <<ERROR>> CTerrainNode(LoadHeightMap)couldn't load heightmap  \r\n");                         
		return false;
	}
	//m_pHeightMap->SetBorders(-20.0);
	m_pHeightMap->SmoothMap();

	return true;
}
bool CTerrainNode::LoadHeightMap(video::IImage* heightMap, float HScale)
{
	if( !m_pHeightMap->LoadHeightMap( t_mgr, heightMap, HScale ))
	 {	printf(" <<ERROR>> CTerrainNode(LoadHeightMap)couldn't load heightmap  \r\n");                         
		return false;
	}
	//m_pHeightMap->SetBorders(-20.0);
	m_pHeightMap->SmoothMap();

	return true;
}
//=========================================================
//---------------------------------------------------------
// Name: Render()
// Desc: routine de rendu standard
//	
//---------------------------------------------------------
void CTerrainNode::render()
{
	// mise à jour des info camera frustum
	UpdateCullInfo();

	video::IVideoDriver* const driver = t_mgr->getVideoDriver();
	driver->setMaterial(Material);
	driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);	// Modifié : ancien : this->getRelativeTransformation()

	//nQuad=0;
	m_pQuadTree->RenderQuad(t_mgr, 0);
}
//=========================================================
//---------------------------------------------------------
// Name: OnRegisterSceneNode()
// Desc: 
//	
//---------------------------------------------------------
void CTerrainNode::OnRegisterSceneNode()
{
	if(IsVisible)
		SceneManager->registerNodeForRendering(this);

	ISceneNode::OnRegisterSceneNode();
}
//=========================================================
//---------------------------------------------------------
// Name: setMaterialTexture()
// Desc: travail sur la textures des Quad du Terrain
//	
//---------------------------------------------------------
void CTerrainNode::setMaterialTexture(u32 textureLayer, video::ITexture* texture)
{
	if (textureLayer >= video::MATERIAL_MAX_TEXTURES)    return;

    Material.setTexture(textureLayer, texture);
	// on applique a tous les quad maintenant
	/*for(int j=0 ; j<max_grille; ++j)
	{
		CQuadNode  *pQuad = this->grille[j];
		pQuad->Material.setTexture(textureLayer, texture);
	}*/
}
//=========================================================
//---------------------------------------------------------
// Name: setMaterialType()
// Desc: travail sur le type de aterial des Quad du Terrain
//	
//---------------------------------------------------------
void CTerrainNode::setMaterialType(video::E_MATERIAL_TYPE newType)
{
    Material.MaterialType = newType;
	// on applique a tous les quad maintenant
	/*for(int j=0 ; j<max_grille; ++j)
	{
		CQuadNode  *pQuad = this->grille[j];
		pQuad->Material.MaterialType = newType;
	}*/
}
//=========================================================
//---------------------------------------------------------
// Name: setMaterialFlag()
// Desc: travail sur les flag des materials des Quad du Terrain
//	
//---------------------------------------------------------
void CTerrainNode::setMaterialFlag(video::E_MATERIAL_FLAG flag, bool newvalue)
{
	Material.setFlag(flag, newvalue);
	// on applique a tous les quad maintenant
	/*for(int j=0 ; j<max_grille; ++j)
	{
		CQuadNode  *pQuad = this->grille[j];
		pQuad->Material.setFlag(flag, newvalue);
	}*/
}
//======================================================================================
//--------------------------------------------------------------------------------------
// name:	DM_GetTerrainHeight()
//
// Retourne la hauteur du terrain en fonction des coordonnées x et z fournies.
//----------------------------------------------------------------------------------------------------------------
float CTerrainNode::GetTerrainHeight(float x, float z)
{
	//return this->m_pHeightMap->GetInterpolateHeight(x/m_rk,  z/m_rk);

	// Modifié pour prendre en compte le déplacement et l'agrandissement du terrain
	// (seulement les valeurs relatives : les valeurs absolues ne fonctionnent que lorsque updateAbsolutePosition a déjà été appelé
	//	-> ne fonctionne donc pas pendant le chargement d'une partie)
	return m_pHeightMap->GetInterpolateHeight((x - RelativeTranslation.X) / (m_rk * RelativeScale.X),  (z - RelativeTranslation.Z) / (m_rk * RelativeScale.Z)) * RelativeScale.Y + RelativeTranslation.Y;
}
//======================================================================================
//--------------------------------------------------------------------------------------
// name:	GetQuadNode()
//
// Retourne le pointeur d'un Quad souhaité
//----------------------------------------------------------------------------------------------------------------
/*CQuadNode* CTerrainNode::GetQuadNode(int num)
{
	return this->grille[num];
}
CQuadNode* CTerrainNode::GetQuadNode(int x, int y)
{
	return this->grille[x + y*m_rc];
}*/
//-----------------------------------------------------------------------------
// Name: UpdateCullInfo()
// Desc: Update Cull Info, attention  avec la camera courante
//-----------------------------------------------------------------------------
void CTerrainNode::UpdateCullInfo()
{
	scene::ICameraSceneNode* cam = t_mgr->getActiveCamera();
	core::matrix4 pMatProj, pMatView;
	pMatProj = cam->getProjectionMatrix();
	pMatView = cam->getViewMatrix();
	core::matrix4 VPI = pMatProj * pMatView;

	Plane col0(VPI(0,0), VPI(1,0), VPI(2,0), VPI(3,0));
	Plane col1(VPI(0,1), VPI(1,1), VPI(2,1), VPI(3,1));
	Plane col2(VPI(0,2), VPI(1,2), VPI(2,2), VPI(3,2));
	Plane col3(VPI(0,3), VPI(1,3), VPI(2,3), VPI(3,3));
	// construit les 6 plane du Frustum view
	mFrustumPlane[0] = col2;          // near
	mFrustumPlane[1] = (col3 - col2); // far
	mFrustumPlane[2] = (col3 + col0); // left
	mFrustumPlane[3] = (col3 - col0); // right
	mFrustumPlane[4] = (col3 - col1); // top
	mFrustumPlane[5] = (col3 + col1); // bottom
	for(int i = 0; i < 6; ++i)		
		mFrustumPlane[i].Normalize();
}
//-----------------------------------------------------------------------------
// Name: _isVisible()
// Desc: test de visibilité d'un mesh
//-----------------------------------------------------------------------------
int CTerrainNode::_isVisible(const core::aabbox3df& box)
{
	// TODO : Comprendre pourquoi le tableau P n'est pas utilisé, et déterminé l'utilité qu'il aurait dû avoir !
	//float P[3];	// Désactivé : Non utilisé
	float Q[3];
	// la classe vector3d ne possédant pas d'opérateur de surcharge []....
	float Min[3];	memcpy( Min, &box.MinEdge.X, sizeof(float)*3);
	float Max[3];	memcpy( Max, &box.MaxEdge.X, sizeof(float)*3);
	for(int i = 0; i < 6; ++i)
	{
		// pour chaque coordonnées x, y, z...
		for(int j = 0; j < 3; ++j)
		{
			// etabli le poiont PQ dans la même direction que le plan normal sur l'axe.
			if( mFrustumPlane[i][j] >= 0.0f )
			{
				//P[j] = Min[j];
				Q[j] = Max[j];
			}
			else 
			{
				//P[j] = Max[j];
				Q[j] = Min[j];
			}
		}
		if( mFrustumPlane[i].DotCoord(Q) < 0.0f  ) // en dehor, exit procedure
			return false;
	}
	return true;
}
//OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO
//-----------------------------------------------------------------------------
// Name: TextureTerrain()
// Desc:
//
//	nquad = quad d'origine pour le dépard de la texturation
//	layer = étage de texture concerné (de 0 ou 1 )
//	n	  = largeur en quad de la texturation
//-----------------------------------------------------------------------------
//OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO
/*void CTerrainNode::TextureTerrain(int nquad, int layer, int n,  video::ITexture* texture)
{
	float i2,j2;
	int j,i,kx, ky;
	// recupère le qued en X et Z
	int fact = this->m_rc;
	int x = nquad / fact;
	int z = nquad - (x*fact);
	int	larg = this->m_larg;

	CQuadNode   *pQuad;
	pQuad = this->grille[nquad];
	pQuad->Material.setTexture(layer, texture);
	
	CQuadNode *QuadR, *Quadd;
	// quad de reference
	QuadR = this->grille[nquad];
	int id;
	for(j=0 ; j<n; ++j)
	{
		for(i=0; i<n; ++i)
		{	id = nquad+i + j*fact;
			// test les bornes
			if( (id<0) || (id> this->max_grille-1) )
			{
				// erreur
				return;
			}
			Quadd = this->grille[id];
			Quadd->Material.setTexture(layer, texture);
		}
	}
	// nouvelle buté
	float pa = 1.0f / (float)n;	
	float pa2= 1.0f / ( (float)QuadR->m_pQuadTerrain->m_larg * (float)n );
	layer = layer * 2;
	for(j=0 ; j<n; ++j)
	{
		for(i=0; i<n; ++i)
		{
			Quadd = this->grille[nquad+ i + j*fact];
			i2 = 1.0f - (float)j * pa ;
			i2 = i2 - pa2;
			j2 = (float)i * pa;
			// accès vertices
			int pp=0;
			int cur=0;
			scene::SMeshBufferLightMap* vbuffer = Quadd->mbuffer;
			for(kx=0; kx<larg; k++x)
			{
				for(ky=0; ky<larg; k++y)
				{
					if(layer)	{vbuffer->Vertices[cur].TCoords2.X = i2+pa2;	vbuffer->Vertices[cur].TCoords2.Y = j2+pa2;}
					else		{vbuffer->Vertices[cur].TCoords.X = i2+pa2;		vbuffer->Vertices[cur].TCoords.Y = j2+pa2;}
					cur++;

					if(layer)	{vbuffer->Vertices[cur].TCoords2.X = i2+pa2; vbuffer->Vertices[cur].TCoords2.Y = j2;	}
					else		{vbuffer->Vertices[cur].TCoords.X = i2+pa2;	 vbuffer->Vertices[cur].TCoords.Y = j2;	}
					cur++;

					if(layer)	{vbuffer->Vertices[cur].TCoords2.X = i2;	vbuffer->Vertices[cur].TCoords2.Y = j2;	}
					else		{vbuffer->Vertices[cur].TCoords.X = i2;		vbuffer->Vertices[cur].TCoords.Y = j2;	}
					cur++;

					if(layer)	{vbuffer->Vertices[cur].TCoords2.X = i2;	vbuffer->Vertices[cur].TCoords2.Y = j2+pa2;	}
					else		{vbuffer->Vertices[cur].TCoords.X = i2;		vbuffer->Vertices[cur].TCoords.Y = j2+pa2;	}
					cur++;
				
					j2 = j2 + pa2;
				}
				j2 = (float)i * pa;
				i2=i2-pa2;
			}
		}
	}
}*/
