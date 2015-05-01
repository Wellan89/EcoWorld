/***************************************************************
*                Terrain Engine issue of					   *
*							Dreamotion3D engine				   *
*															   *
**************************************************************** 
*File Created: 29.03.08                                        *
*Author:			                                           *
*Contens: Implementation of QuadTree   headers	               *
***************************************************************/

// Cette version a été modifiée

#ifndef _CQUADN_Tmyke
#define _CQUADN_Tmyke

#include "global.h"
//#include <IRR/irrlicht.h>

#define TRIANGULATE_UP    1
#define TRIANGULATE_DOWN  2
#define TRIANGULATE_LEFT  4
#define TRIANGULATE_RIGHT 8

using namespace irr;
using namespace core;

// class externes
class CTerrainNode;


//-------------------------------------------------
// CQuadNode()
//-------------------------------------------------
// classe  
class CQuadNode	
{
private:

		CQuadNode*				m_pChildren[4];		/*!< 4 enfants par Quad. */
		CQuadNode*				m_pParent;			/*!< le parent de ce quad. */

		//vector3df				d_mCorners;			/*!< coin de référence pour les calcul. */
		//vector3df				m_cCenter;			/*!< centre du quad. */
		//float					radius;				/*!< radius du Quad. */

		// Ajouté :
		scene::ITriangleSelector* selector;			// Le triangle selector de ce quad, valide lorsque ce quad n'est pas un parent d'autre quads

public:
		core::aabbox3df			Box;				/*!< boundingBox du Quad. */
		vector3df				m_iCorners[4];		/*!< dimension des coins de ce quad. */
		CTerrainNode*			m_pQuadTerrain;		/*!< objet CTerrainNode propriétaire de ce Quad. */
		scene::SMeshBufferLightMap* mbuffer;

	//---------------------------------------------------------
	// procedure et fonctions
	//---------------------------------------------------------
public:
	~CQuadNode();

	// constructeur().
    CQuadNode(CQuadNode *Parent, CTerrainNode *Terrain, scene::ISceneManager* mgr=NULL);

	// destruction
	void Destroy();

	// CreateQuadMesh().
	void CreateQuadMesh(scene::ISceneManager* mgr);

	// Create3D_Quad().
	void Create3D_Quad(scene::ISceneManager* mgr);

	// Subdivide().
	bool Subdivide();

	// Allocate().
	bool Allocate(vector3df Corners[4]);

	// BuildQuadTree().
	void BuildQuadTree(); 

	// CalculYCorner().
	void CalculYCorner();

	// SetBox().
	void SetBox();

	// AffecteQuad().
	//void AffecteQuad();

	// SetMaxBox().
	void SetMaxBox();

	// RenderQuad
	void RenderQuad(scene::ISceneManager* mgr, int FrustumCode);

	// getBoundingBox
	const core::aabbox3df& getBoundingBox() const { return Box; }

	// Ajouté :
	// Crée récursivement les triangle selector de ce quad ou de tous ses enfants si c'est un parent
	// Le meta triangle selector fourni contiendra tous les triangle selector créés
	void createTriangleSelectors(scene::ISceneManager* smgr, scene::IMetaTriangleSelector* metaTriSelector);
};

#endif
