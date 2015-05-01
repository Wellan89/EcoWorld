/***************************************************************
*                Terrain Engine issue of					   *
*							Dreamotion3D engine				   *
*															   *
**************************************************************** 
*File Created: 29.03.08                                        *
*Author:			                                           *
*Contens: Implementation of HeightMap headers	               *
***************************************************************/

// Cette version a été modifiée

#ifndef _CHEIGHT_Tmyke
#define _CHEIGHT_Tmyke

#include "global.h"
//#include <IRR/irrlicht.h>

using namespace irr;

/**
* CHeightMap_tmyke class .
* Class to manage the height map for terrain construct.
* @param none.
* @return none.
*/
class CHeightMap_tmyke
{
  private:
		//void SetBorders( float fHeight );											// Désactivé car non utilisé
		float* LoadTexture32(io::path filename, scene::ISceneManager* mgr);
		float* LoadTexture32(video::IImage* heightmap, scene::ISceneManager* mgr);	// Surcharge ajoutée
		void GetMinMax();
		//core::vector3df GetVertexNormal( float fX, float fY, float fSizeScale );	// Désactivé car non utilisé
		float* ScaleHeightmap_( float *Src, int SrcWidth, int SrcHeight, int DstWidth, int DstHeight );

  public:

	   float	*m_pHeightMap;			/*!< tableau global des hauteurs terrain. */	
	   int		m_iWidth;				/*!< largeur du heightMap. */
	   int		m_iHeight;				/*!< hauteur du heightMap. */
	   float	m_fMinHeight;			/*!< Min value du Height. */
	   float	m_fMaxHeight;			/*!< Max value du Height. */


		// ------------------------------
		//	Constructeurs - Destructeurs
		// ------------------------------/
	   ~CHeightMap_tmyke();

	/**
	* constructor().
	*/
    CHeightMap_tmyke();

	/**
	* constructor().
	*/
	CHeightMap_tmyke( int Width, int Height );

	/**
	* Destroy().
	*/
	void Destroy();

	/**
	* Create().
	*/
	void Create( int Width, int Height );

	/**
	* LoadHeightMap().
	*/
	bool LoadHeightMap(scene::ISceneManager* mgr, io::path filename, float HScale );
	bool LoadHeightMap(scene::ISceneManager* mgr, video::IImage* heightMap, float HScale );	// Surcharge ajoutée

	/**
	* Scale().
	*/
	void Scale( float fScale );

	/**
	* SmoothMap().
	*/
	void SmoothMap();

	/**
	* GetHeightValue().
	*/
	float GetHeightValue( int iX, int iY );

	/**
	* GetHeightValue().
	*/
	float GetHeightValue( float fX, float fY );

	/**
	* GetInterpolateHeight().
	*/
	float GetInterpolateHeight( float fX, float fY );
};


#endif