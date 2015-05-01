/***************************************************************
*                Terrain Engine issue of					   *
*							N3xtD engine				       *
**************************************************************** 
*File Created: 29.01.11                                        *
*Author:			                                           *
*Contens: Implementation of HeightMap algorithme               *
***************************************************************/

// Cette version a été modifiée

//#include <iostream>
//#include <stdio.h>

#include "TerrainHeight.h"
//#pragma warning( disable : 4996 ) // disable deprecated warning 


using namespace irr;

//******************************
// Variables globales 
//******************************
#define		TMapHauteur(x, y) (m_pHeightMap[x+y*m_NbPointX])
#define		TMapHauteurLisser(x, y) (pMapHauteurLisser[x+y*m_NbPointX])


//=========================================================
//---------------------------------------------------------
// Name: constructor()
// 
//---------------------------------------------------------
CHeightMap_N3xtD::CHeightMap_N3xtD()
{
	m_pHeightMap = NULL;
	m_iWidth     = 0;
	m_iHeight    = 0;
	m_fMinHeight = 0.0f;
    m_fMaxHeight = 0.0f;
	heightmap	 = NULL;
}
CHeightMap_N3xtD::CHeightMap_N3xtD( int Width, int Height )
{
	m_iWidth  = Width;
	m_iHeight = Height;
	m_pHeightMap = new float[ Width * Height ];
	m_fMinHeight = 0.0f;
    m_fMaxHeight = 0.0f;
	heightmap	 = NULL;
}
CHeightMap_N3xtD::~CHeightMap_N3xtD()
{
	Destroy();
}
//=========================================================
//---------------------------------------------------------
// Name: Create()
// 
//---------------------------------------------------------
void CHeightMap_N3xtD::Create( int Width, int Height )
{
	m_iWidth  = Width;
	m_iHeight = Height;
	m_pHeightMap = new float[ Width * Height ];
	memset(m_pHeightMap, 0, sizeof(float) * Width * Height);
	m_fMinHeight = 0.0f;
    m_fMaxHeight = 0.0f;
}
//=========================================================
//---------------------------------------------------------
// Name: Destroy()
// 
//---------------------------------------------------------
void CHeightMap_N3xtD::Destroy()
{
	if (m_pHeightMap)
		delete[] m_pHeightMap;
	m_pHeightMap = NULL;
	m_iWidth = m_iHeight = 0;
}
//=========================================================
//---------------------------------------------------------
// Name: Scale()
// 
//---------------------------------------------------------
void CHeightMap_N3xtD::Scale( float fScale )
{
	float *hbuf;
	int x,y;

   if( m_pHeightMap == NULL || ( m_iWidth <= 0 || m_iHeight <= 0 ))
	   return;

	hbuf = m_pHeightMap;

	for( x = 0; x < m_iWidth; ++x )
	{
		for( y = 0; y < m_iHeight; ++y )
		{
			   hbuf[ y * m_iWidth + x ] *= fScale;
		}
	}
	   GetMinMax();
}
//=============================================================================
//-----------------------------------------------------------------------------
// Name: ScaleHeightmap_()
// changement d'echelle de la surface HeightMap
//-----------------------------------------------------------------------------
float* CHeightMap_N3xtD::ScaleHeightmap_( float *Src, int SrcWidth, int SrcHeight, 
					               int DstWidth, int DstHeight )
{
	float x_step, y_step;
	float *Dst;

	Dst = new float[ DstWidth * DstHeight ];

	x_step = (float)SrcWidth  / (float)DstWidth;
	y_step = (float)SrcHeight / (float)DstHeight;

	float fXT, fYT, fXPos, fYPos;
	int x,y, iX, iY;
	iX = iY = 0;
	fXT = fYT = 0.0f;
	fXPos = fYPos = 0.0f;

	for( y = 0; y < DstHeight; ++y )
	{
		fXPos = 0.0f;
	    for(x = 0; x < DstWidth; ++x)
		{
			Dst[ y * DstWidth + x ] = Src[ iY * SrcWidth + iX ];

			(float)fXPos += (float)x_step;
			(float)fXT    = (float)fXPos;
			(int)iX       = (int)fXT;
		}
		(float)fYPos += (float)y_step;
		(float)fYT    = (float)fYPos;
		(int)iY       = (int)fYT;
	}
   return (float*)Dst;
}
//=============================================================================
//-----------------------------------------------------------------------------
// Name: LoadHeightMap()
// charge le fichier image représentative du HeightMap
//-----------------------------------------------------------------------------
bool CHeightMap_N3xtD::LoadHeightMap(scene::ISceneManager* mgr, io::path fname, float HScale )
{
	m_fHeightScale = HScale;

	if(fname.size() > 1)
	{ // FILE HEIGHTMAP
		float *buffer = LoadTexture32(fname, 1.0f, mgr);
		const int width  = widthT; 
		const int height = heightT;
		m_iWidth   = width;
		m_iHeight  = height;

		const int aera = width * height;
		float *fHeightMap = new float[ aera ];
		memset( fHeightMap, 0, sizeof(float) * aera );
	    for(int x = 0; x < width; ++x)
		    for(int y = 0; y < height; ++y)
			{
				const int index = (y * width + x);
				fHeightMap[ index ] = buffer[index] * HScale; 
			}
		delete[] buffer ;
		delete[] m_pHeightMap;
	    if( width != m_iWidth || height != m_iHeight )
		{
		   m_pHeightMap = ScaleHeightmap_( fHeightMap, width, height, m_iWidth, m_iHeight );
		   delete[] fHeightMap;
		}
		else 
			m_pHeightMap = fHeightMap;
	}
	// NOISE PERLIN
	else
	{
		const int width  = widthT = m_iWidth; 
		const int height = heightT = m_iHeight;
	    for(int x = 0; x < width; ++x)
		    for(int y = 0; y < height; ++y)
			{
				const int index = (y * width + x);
				m_pHeightMap[index] = m_pHeightMap[index] * HScale; 
			}

	    if( width != m_iWidth || height != m_iHeight )
		   m_pHeightMap = ScaleHeightmap_( m_pHeightMap, width, height, m_iWidth, m_iHeight );
	}
	GetMinMax();
	return true;
}
bool CHeightMap_N3xtD::LoadHeightMap(scene::ISceneManager* mgr, video::IImage* heightMap, float HScale )
{
	m_fHeightScale = HScale;

	float *buffer = LoadTexture32(heightMap, HScale, mgr);
	m_iWidth	= widthT;
	m_iHeight	= heightT;

	if (m_pHeightMap)
		delete[] m_pHeightMap;
	m_pHeightMap = buffer;
	GetMinMax();

	return true;
}
//=============================================================================
//-----------------------------------------------------------------------------
// Name: GetMinMax()
// recupère les deux sommet (positif et négatif)
//-----------------------------------------------------------------------------
void CHeightMap_N3xtD::GetMinMax()
{
  float *hbuf = m_pHeightMap;
  int x,y;
  float miny, maxy, h;

  miny = hbuf[ 0 ];
  maxy = hbuf[ 0 ];
	for( x = 0; x < m_iWidth; ++x )
	{
		for( y = 0; y < m_iHeight; ++y )
		{
			   h = hbuf[ y * m_iWidth + x ];
			   if( h < miny ) miny = h;
			   if( h > maxy ) maxy = h;
		}
	}
  m_fMinHeight = (float) miny;
  m_fMaxHeight = (float) maxy;
}
//=============================================================================
//-----------------------------------------------------------------------------
// Name: SmoothMap()
// On va lisser le terrain en hauteur pour evite les effets d'escalier à cause 
//	des valeurs entière
//-----------------------------------------------------------------------------
void CHeightMap_N3xtD::SmoothMap()
{
	int m_NbPointX=m_iWidth;
	int m_NbPointY=m_iHeight;
	float* pMapHauteurLisser = new float[m_NbPointX * m_NbPointY];

	// Lissage de l'image sans les bordures
	for(int j=1; j<m_NbPointY-1; ++j)
	{
		for(int i=1; i<m_NbPointX-1; ++i)
		{
			TMapHauteurLisser(i, j) =  (TMapHauteur(i    ,     j)
									+	TMapHauteur(i    , (j-1))
									+	TMapHauteur(i    , (j+1))
									+	TMapHauteur((i-1),     j)
									+	TMapHauteur((i+1),     j)
									+	TMapHauteur((i-1), (j-1))
									+	TMapHauteur((i+1), (j+1))
									+	TMapHauteur((i+1), (j-1))
									+	TMapHauteur((i-1), (j+1))) / 9.0f;
		}
	}
	// Lissage bordure gauche
	for(int j=1; j<m_NbPointY-1; ++j)
	{
		TMapHauteurLisser(0, j) =  (TMapHauteur(0,     j)
								+   TMapHauteur(0, (j-1))
								+   TMapHauteur(0, (j+1))
								+   TMapHauteur(1,     j)
								+   TMapHauteur(1, (j-1))
								+   TMapHauteur(1, (j+1))) / 6.0f;
	}
	// Lissage bordure droite
	for(int j=1; j<m_NbPointY-1; ++j)
	{
		TMapHauteurLisser((m_NbPointX-1), j) =  (TMapHauteur((m_NbPointX-1),     j)
											 +   TMapHauteur((m_NbPointX-1), (j-1))
								             +   TMapHauteur((m_NbPointX-1), (j+1))
								             +   TMapHauteur((m_NbPointX-2),     j)
								             +   TMapHauteur((m_NbPointX-2), (j-1))
								             +   TMapHauteur((m_NbPointX-2), (j+1))) / 6.0f;
	}
	// Lissage bordure haut
	for(int i=1; i<m_NbPointX-1; ++i)
	{
		TMapHauteurLisser(i, 0) =  (TMapHauteur(i    , 0)
								+   TMapHauteur((i+1), 0)
								+   TMapHauteur((i-1), 0)
								+   TMapHauteur(i    , 1)
								+   TMapHauteur((i+1), 1)
								+   TMapHauteur((i-1), 1)) / 6.0f;
	}
	// Lissage bordure bas
	for(int i=1; i<m_NbPointX-1; ++i)
	{
		TMapHauteurLisser(i, (m_NbPointY-1)) =  (TMapHauteur(i    , (m_NbPointY-1))
								             +   TMapHauteur((i+1), (m_NbPointY-1))
								             +   TMapHauteur((i-1), (m_NbPointY-1))
								             +   TMapHauteur(i    , (m_NbPointY-2))
								             +   TMapHauteur((i+1), (m_NbPointY-2))
								             +   TMapHauteur((i-1), (m_NbPointY-2))) / 6.0f;
	}
	// Lissage des angles de l'image
	TMapHauteurLisser(0, 0) = (TMapHauteur(0, 0) + TMapHauteur(0, 1) + TMapHauteur(1, 0) + TMapHauteur(1, 1)) / 4.0f;
	TMapHauteurLisser((m_NbPointX-1), 0) = (TMapHauteur((m_NbPointX-1), 0) + TMapHauteur((m_NbPointX-1), 1) + TMapHauteur((m_NbPointX-2), 0) + TMapHauteur((m_NbPointX-2), 1)) / 4.0f;
	TMapHauteurLisser(0, (m_NbPointY-1)) = (TMapHauteur(0, (m_NbPointY-1)) + TMapHauteur(1, (m_NbPointY-1)) + TMapHauteur(0, (m_NbPointY-2)) + TMapHauteur(1, (m_NbPointY-2))) / 4.0f;
	TMapHauteurLisser((m_NbPointX-1), (m_NbPointY-1)) = (TMapHauteur((m_NbPointX-1), (m_NbPointY-1)) + TMapHauteur((m_NbPointX-2), (m_NbPointY-1)) + TMapHauteur((m_NbPointX-1), (m_NbPointY-2)) + TMapHauteur((m_NbPointX-2), (m_NbPointY-2))) / 4.0f;

	// On libère l'ancien tableau et on le remplace par le nouveau
	delete[] m_pHeightMap;
	m_pHeightMap = pMapHauteurLisser;
}
//=============================================================================
//-----------------------------------------------------------------------------
// Name: SetBorders()
// met en place des bord, plus jolie au niveau du rendu
//-----------------------------------------------------------------------------
void CHeightMap_N3xtD::SetBorders( float fHeight )
{
	float *hbuf;
	int x,y;

   if( m_pHeightMap == NULL || ( m_iWidth <= 0 || m_iHeight <= 0 ))
	   return;

	hbuf = m_pHeightMap;

	for( x = 0; x < m_iWidth; ++x )
	{
		for( y = 0; y < m_iHeight; ++y )
		{
		   if( x == 0 || x == (m_iWidth-1) || y == 0 || y == (m_iHeight-1) )
			   hbuf[ y * m_iWidth + x ] = fHeight;
		}
	}
	GetMinMax();
}
//=============================================================================
//-----------------------------------------------------------------------------
// Name: GetHeightValue()
// récupère une hauteur terrain en fonction de coordonnées X et Y
//-----------------------------------------------------------------------------
void CHeightMap_N3xtD::SetHeightValue( int iX, int iY, float value )
{
	if( iX >= m_iWidth ) 
		iX = m_iWidth-1;
	if( iY >= m_iHeight )
		iY = m_iHeight-1;
	m_pHeightMap[ iY * m_iWidth + iX ] = value;
}
//=============================================================================
//-----------------------------------------------------------------------------
// Name: GetHeightValue()
// récupère une hauteur terrain en fonction de coordonnées X et Y
//-----------------------------------------------------------------------------
float CHeightMap_N3xtD::GetHeightValue( int iX, int iY )
{
	if( iX >= m_iWidth ) 
		iX = m_iWidth-1;
	if( iY >= m_iHeight )
		iY = m_iHeight-1;
	return m_pHeightMap[ iY * m_iWidth + iX ];
}
//=============================================================================
//-----------------------------------------------------------------------------
// Name: GetHeightValue()
// récupère une hauteur terrain en fonction de coordonnées X et Y
//-----------------------------------------------------------------------------
float CHeightMap_N3xtD::GetHeightValue( float fX, float fY )
{
	int iX = (int)fX;
	int iY = (int)fY;
	if( iX >= m_iWidth ) 
		iX = m_iWidth-1;
	if( iY >= m_iHeight )
		iY = m_iHeight-1;
	return (float) m_pHeightMap[ iY * m_iWidth + iX ];
}
//=============================================================================
//-----------------------------------------------------------------------------
// Name: GetInterpolateHeight()
// récupère une hauteur terrain en fonction de coordonnées X et Y avec interpolation
// (non tributaire de l'echelle du terrain)
//-----------------------------------------------------------------------------
float CHeightMap_N3xtD::GetInterpolateHeight( float fX, float fY )
{
    int iX, iY;
	float dX, dY;

    iX = (int)fX;
	iY = (int)fY;
    dX = fX - (float)iX;
	dY = fY - (float)iY;

	if( (iX >(m_iWidth-1)) || (iY >(m_iHeight-1)) ) 		return GetHeightValue( fX, fY );
	if( (iX<0) || (iY<0) ) return GetHeightValue( 0, 0 );

	float hY, hY2, Height;
	int   idY=1;
	int   idX=1;

	if( iX==(m_iWidth-1)) idX=0;
	if( iY==(m_iHeight-1)) idY=0;

	hY =  m_pHeightMap[ iY * m_iWidth + iX ] + 
	( m_pHeightMap[ (iY+idY) * m_iWidth + iX ] - m_pHeightMap[ iY * m_iWidth + iX ] ) * dY;


	hY2= m_pHeightMap[ iY * m_iWidth + iX + idX ] + 
	( m_pHeightMap[ (iY+idY) * m_iWidth + iX + idX ] - m_pHeightMap[ iY * m_iWidth + iX + idX ] ) * dY;


	Height = hY + ( hY2 - hY ) * dX;
	return Height;
}
//=============================================================================
//-----------------------------------------------------------------------------
// Name: GetVertexNormal()
// 
//---------------------------------------------------------------------------------------
core::vector3df CHeightMap_N3xtD::GetVertexNormal( float fX, float fY, float fSizeScale )
{
   core::vector3df vNorm = core::vector3df( 0,0,0 );

   float iX, iY;
   iX = fX;
   iY = fY;

   if( (iX > 0) && (iX < ( m_iWidth - 1 )))
   {
	   vNorm.X = GetInterpolateHeight( iX - 1, iY ) - GetInterpolateHeight( iX + 1, iY );
   } 
   else if( iX > 0 )
   {
	   vNorm.X = 2.0f * GetInterpolateHeight( iX - 1, iY ) - GetInterpolateHeight( iX, iY );
   } 
   else
   {
	   vNorm.X = 2.0f * GetInterpolateHeight( iX, iY ) - GetInterpolateHeight( iX + 1, iY );
   }

   if( (iY > 0) && (iY < ( m_iHeight - 1 )))
   {
	   vNorm.Z = GetInterpolateHeight( iX, iY - 1 ) - GetInterpolateHeight( iX, iY + 1 );
   } 
   else if( iY > 0 )
   {
	   vNorm.Z = 2.0f * GetInterpolateHeight( iX, iY - 1 ) - GetInterpolateHeight( iX, iY );
   } 
   else
   {
	   vNorm.Z = 2.0f * GetInterpolateHeight( iX, iY ) - GetInterpolateHeight( iX, iY + 1 );
   }

   vNorm.Y = 2.0f * fSizeScale;

   vNorm.normalize();
	vNorm= -vNorm;

   return vNorm;
}
//=============================================================================
//-----------------------------------------------------------------------------
// Name: LoadTexture()
// methode pour le chargement du fichier image représentant les hauteurs
//-----------------------------------------------------------------------------
float* CHeightMap_N3xtD::LoadTexture32(io::path filename, float scale, scene::ISceneManager* mgr)
{	
  //Get the Heightmap
	heightmap = mgr->getVideoDriver()->createImageFromFile(filename);
	if (!heightmap)
		return NULL;

  //Get dimension of heightmap
  widthT = heightmap->getDimension().Width;
  heightT = heightmap->getDimension().Height;

	float *img = new float[widthT * heightT];

	for(int u=0;u<heightT;u++) 
	{ // pour chaque ligne de la texture
		for(int v=0;v<widthT;v++) 
		{  // pour chaque pixel de la ligne
			//video::SColor pixelcolor(heightmap->getPixel(v,u));
			//vertex.pos.y = (f32) pixelcolor.getluminance()/10.0f;
			//float color = pixelcolor.getLuminance();//			 0.3f*getRed() + 0.59f*getGreen() + 0.11f*getBlue();
			img[v+u*heightT] = heightmap->getPixel(v,u).getLuminance() * scale;
		}
	}

	heightmap->drop();

	return img;
}

float* CHeightMap_N3xtD::LoadTexture32(video::IImage* heightmap, float scale, scene::ISceneManager* mgr)
{	
  //Get dimension of heightmap
  widthT = heightmap->getDimension().Width;
  heightT = heightmap->getDimension().Height;

	float *img = new float[widthT * heightT];

	for(int u=0;u<heightT;u++) 
	{ // pour chaque ligne de la texture
		for(int v=0;v<widthT;v++) 
		{  // pour chaque pixel de la ligne
			//video::SColor pixelcolor(heightmap->getPixel(v,u));
			//vertex.pos.y = (f32) pixelcolor.getluminance()/10.0f;
			//float color = pixelcolor.getLuminance();//			 0.3f*getRed() + 0.59f*getGreen() + 0.11f*getBlue();
			img[v+u*heightT] = heightmap->getPixel(v,u).getLuminance() * scale;
		}
	}

	return img;
}

//=============================================================================
//-----------------------------------------------------------------------------
// Name: 
// 
//-----------------------------------------------------------------------------
float Noise(int x)
{
	x = (x<<13) ^ x;
	return (1.0f - ((x * (x*x * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.f);    
}

float CosInterpolate(float v1, float v2, float a)
{
	float angle = a * 3.14159265358f;
	float prc = (1.0f - cos(angle)) * 0.5f;
	return  v1*(1.0f - prc) + v2*prc;
}
//=============================================================================
//-----------------------------------------------------------------------------
// Name: CreateRandomHeightMap()
// 
//-----------------------------------------------------------------------------
bool CHeightMap_N3xtD::CreateRandomHeightMap(int mSize, int seed, float noiseSize, float persistence, int octaves)
{
	m_pHeightMap = new float[ mSize * mSize ];
	memset( m_pHeightMap, 0,  sizeof(float) * mSize * mSize );

	//For each map node
	for(int y=0;y<mSize;++y)
		for(int x=0;x<mSize;++x)
		{
			//Scale x & y to the range of 0.0 - mSize
			float xf = ((float)x / (float)mSize) * noiseSize;
			float yf = ((float)y / (float)mSize) * noiseSize;

			float total = 0;			

			// For each octave
			for(int i =0; i < octaves; ++i)
			{
				//Calculate frequency and amplitude (different for each octave)
				float freq = pow(2.0f, i);
				float amp = pow(persistence, i);

				//Calculate the x,y noise coordinates
				float tx = xf * freq;
				float ty = yf * freq;
				int tx_int = (int)tx;
				int ty_int = (int)ty;

				//Calculate the fractions of x & y
				float fracX = tx - tx_int;
				float fracY = ty - ty_int;

				//Get the noise of this octave for each of these 4 points
				float v1 = Noise(tx_int + ty_int * 57 + seed);
				float v2 = Noise(tx_int+ 1 + ty_int * 57 + seed);
				float v3 = Noise(tx_int + (ty_int+1) * 57 + seed);
				float v4 = Noise(tx_int + 1 + (ty_int+1) * 57 + seed);

				//Smooth in the X-axis
				float i1 = CosInterpolate(v1 , v2 , fracX);
				float i2 = CosInterpolate(v3 , v4 , fracX);

				//Smooth in the Y-axis
				total += CosInterpolate(i1 , i2 , fracY) * amp;
			}

			int b = (int)(128.f + total * 128.f);
			if(b < 0)b = 0;
			if(b > 255)b = 255;

			// Save to height map table
			//float height = (b / 255.0f)*512.0f;// * mMaxHeight;			
			m_pHeightMap[x + y * mSize] = 2.0f*(float)b;//height;								
		}
		m_iWidth = m_iHeight = mSize;
		return 1;
}
//=============================================================================
//-----------------------------------------------------------------------------
// Name: saveHeightMap()
// 
//-----------------------------------------------------------------------------
bool CHeightMap_N3xtD::saveHeightMap(io::path filename, scene::ISceneManager* mgr)
{
	video::IImage *img =  mgr->getVideoDriver()->createImage(video::ECF_A8R8G8B8, core::dimension2du(widthT, heightT));

	for(int u=0;u<heightT;u++) 
	{ // pour chaque ligne de la texture
		for(int v=0;v<widthT;v++) 
		{  // pour chaque pixel de la ligne
			const float color = m_pHeightMap[v+u*heightT]/m_fHeightScale;
			img->setPixel(v,u, video::SColor((u32)color, (u32)color, (u32)color, (u32)color));
		}
	}
	return mgr->getVideoDriver()->writeImageToFile(img, filename);
}
