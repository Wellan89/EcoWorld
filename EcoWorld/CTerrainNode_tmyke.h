/***************************************************************
*                Terrain Engine issue of					   *
*							Dreamotion3D engine				   *
*															   *
**************************************************************** 
*File Created: 29.03.08                                        *
*Author:			                                           *
*Contens: Implementation of TerrainNode headers	               *
***************************************************************/

// Cette version a été modifiée

#ifndef _CTERRAIN_Tmyke
#define _CTERRAIN_Tmyke

#include "global.h"
//#include <IRR/irrlicht.h>

#include "CList.h"
#include "CHeight.h"
#include "CQuad.h"

using namespace irr;

// class externes
//class CQuadNode;

//-----------------------------------------------------------------------------
// Name: Plane class
// Desc: petite classe plane personnalisée pour pour ce codage.
//-----------------------------------------------------------------------------
class Plane
{
public:
	float a;
	float b;
	float c;
	float d;

	Plane():  a(0),b(0),c(0),d(0) {}
	Plane(float aa, float bb, float cc, float dd): a(aa), b(bb), c(cc), d(dd){}
	Plane operator-(const Plane& other) const { return Plane(a - other.a, b - other.b, c - other.c, d - other.d); }
	Plane operator+(const Plane& other) const { return Plane(a + other.a, b + other.b, c + other.c, d + other.d); }
	float operator[](unsigned i) const 	{ switch(i)	{case 0: return a; case 1: return b; case 2: return c; case 3: return d;default: return 0;	}	}
	void Normalize() { float m;	m = sqrt( a*a + b*b + c*c + d*d );	a = a / m;	b = b / m;	c = c/ m;	d = d /m;	}
	float DotCoord(float *V) {	return (a * V[0] + b * V[1] + c * V[2] + d);	}
};


//-------------------------------------------------
// CTerrainNode()
//-------------------------------------------------
// classe TerrainNode, dérivée de ISceneNode 
class CTerrainNode : public scene::ISceneNode
{
protected:
		scene::ISceneManager*   t_mgr;
		video::SMaterial	    Material;
		core::aabbox3df			Box;

		// Ajouté :
		scene::IMetaTriangleSelector* metaTriSelector;

   public:

		CList<CQuadNode> DelQuad;			
		CHeightMap_tmyke *m_pHeightMap;	/*!< pointeur vers l'objet heightmap. */

		// Modifié : int -> float :

		float           m_iSize;		/*!< taille du terrain. */
		//float          m_iSizeQuad;	/*!< taille de chaque quad (tile) du terrain (m_iSize / m_rc). */
		//int			numSheet;		/*!< nombre de sheet. */
		int				m_larg;			/*!< largeur de chaque quad, en nombre de  faces. */
		int				m_rc;			/*!< taille du terrain en Quad x Quad  (m_rc * m_rc). */   
		float			m_rk;			/*!< m_iSize / HeightMap Width. */
		//int			m_cote;			/*!< HeightMap / m_rc. */
		//int			numQuad;		/*!< nombre total de quad définit. */
		float			max_h;			/*!< point haut maxi du terrain. */

		CQuadNode       *m_pQuadTree;	/*!< pointeur sur le quad originel. */
		//CQuadNode		**grille;		/*!< table de tous les Quad 3D définis. */
		//int			max_grille;		/*!< taille de la grille. */
		//int			nQuad;			/*!< nombre de quad rendu par renderpass. */

		::Plane			mFrustumPlane[6];	/*!< plan pour le frustum. */


	//************************************************
	//***************** methods
	//************************************************
	~CTerrainNode();

	// constructeur
	CTerrainNode(scene::ISceneNode* parent, scene::ISceneManager* mgr, s32 id);
	// SetValues
	void SetValues(int numQuads=8, int numFaces=16, float sizeTerrain=2048.0f);	// (int, int, int)
	// LoadHeightMap
	bool LoadHeightMap(io::path filename, float HScale );
	bool LoadHeightMap(video::IImage* heightMap, float HScale );	// Surcharge ajoutée
	// Construct
	void Construct(io::path filename, float scale);
	void Construct(video::IImage* heightMap, float scale);	// Surcharge ajoutée
	// OnRegisterSceneNode
	virtual void OnRegisterSceneNode();
	// render
	virtual void render();
	//setMaterialTexture
	void setMaterialTexture(u32 textureLayer, video::ITexture* texture);
	//setMaterialType
	void setMaterialType(video::E_MATERIAL_TYPE newType);
	//setMaterialFlag
	void setMaterialFlag(video::E_MATERIAL_FLAG flag, bool newvalue);
	//GetTerrainHeight
	float GetTerrainHeight(float x, float z);
	// GetQuadNode
	//CQuadNode* GetQuadNode(int num);
	//CQuadNode* GetQuadNode(int x, int y);
	//Update Cull Info, attention  avec la camera courante.
	void UpdateCullInfo();
	// test de visibilité d'un mesh
	int _isVisible(const core::aabbox3df& box);
	// TextureTerrain
	//void TextureTerrain(int nquad, int stage, int n,  video::ITexture* texture);	// Fonction supprimée : Ne fonctionne plus depuis que les matériaux individuels des quads ont été supprimés

	// Ajouté :
	virtual u32 getMaterialCount() const { return 1; }
	virtual video::SMaterial& getMaterial(u32 i) { return Material; }
	virtual const core::aabbox3df& getBoundingBox() const { return Box; }
	void createTriangleSelectors();
	virtual scene::ITriangleSelector* getTriangleSelector() const { return metaTriSelector; };
	virtual void setTriangleSelector(scene::ITriangleSelector* selector) { createTriangleSelectors(); };
};

#endif
