/***************************************************************
*                Terrain Engine issue of					   *
*							N3XTD engine					   *
*															   *
**************************************************************** 
*File Created: 29.01.11                                        *
*Author:			                                           *
*Contens: Implementation of HeightMap headers	               *
***************************************************************/

// Cette version a été modifiée

#ifndef _CHEIGHT_N3xtD
#define _CHEIGHT_N3xtD

//#include <IRR\irrlicht.h>
//using namespace irr;
#include "global.h"

/**
* CHeightMap class .
* Class to manage the height map for terrain construct.
* @param none.
* @return none.
*/
class CHeightMap_N3xtD
{
  private:
		void SetBorders( float fHeight );
		float* LoadTexture32(io::path filename, float scale, scene::ISceneManager* mgr);
		float* LoadTexture32(video::IImage* heightmap, float scale, scene::ISceneManager* mgr);	// Surcharge ajoutée
		void GetMinMax();
		core::vector3df GetVertexNormal( float fX, float fY, float fSizeScale );
		float* ScaleHeightmap_( float *Src, int SrcWidth, int SrcHeight, int DstWidth, int DstHeight );

  public:
		int		heightT,widthT;			// Variables globales déplacées

	   float	*m_pHeightMap;			/*!< tableau global des hauteurs terrain. */	
	   int		m_iWidth;				/*!< largeur du heightMap. */
	   int		m_iHeight;				/*!< hauteur du heightMap. */
	   float	m_fMinHeight;			/*!< Min value du Height. */
	   float	m_fMaxHeight;			/*!< Max value du Height. */
	   float	m_fHeightScale;			/*!< height image scale. */
	   video::IImage	*heightmap;

	// ------------------------------
	//	Constructeurs - Destructeurs
	// ------------------------------/
   ~CHeightMap_N3xtD();

	/**
	* constructor().
	*/
    CHeightMap_N3xtD();

	/**
	* constructor().
	*/
	CHeightMap_N3xtD( int Width, int Height );

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

	/**
	* CreateRandomHeightMap().
	*/
	bool CreateRandomHeightMap(int mSize, int seed, float noiseSize, float persistence, int octaves);


	/**
	* SetHeightValue().
	*/
	void SetHeightValue( int iX, int iY, float value );

	/**
	* saveHeightMap().
	*/
	bool saveHeightMap(io::path filename, scene::ISceneManager* mgr);

};


#endif
