/***************************************************************
*                Terrain Engine issue of					   *
*							N3XTD engine					   *
*															   *
**************************************************************** 
*File Created: 29.01.11                                        *
*Author:			                                           *
*Contens: Implementation of TerrainNode headers	               *
***************************************************************/

// Cette version a été modifiée

#ifndef _CTerrainSceneNode_N3xtD
#define _CTerrainSceneNode_N3xtD


//#include <IRR\irrlicht.h>
//using namespace irr;
#include "global.h"

#include "TerrainHeight.h"


namespace irr
{
namespace scene
{

// Classe largement modifiée : beaucoup d'options inutiles pour ce jeu supprimées ; Shader modifié : Le nombre maximal de textures est 4, ce shader en demandait 7 ! ; Gestion de la première lumière dynamique directionnelle
/*
Ancienne organisation des textures avec ce shader :
0 : Alpha texture (Masque en rouge, vert, bleu)
1 : Black Texture
2 : Green Texture
3 : Blue Texture
4 : Red Texture
5 : Detail Texture
6 : Lightmap Texture

Nouvelle organisation des textures :
0 : Mask texture (Masque en teintes de gris)
1 : Black Texture
2 : White Texture
3 : Detail Texture
*/
class MyShaderCallBack : public video::IShaderConstantSetCallBack
{
public:
  scene::ISceneManager* smgr;
  //f32 Fog;
  //video::SColorf FogColor;
  //float diffusePower;
  //core::vector3df dirL;
  //bool lightEnable;

  void setSplatScene(scene::ISceneManager* Scene//, f32 fog,video::SColorf fogColor
	  )
  {
    smgr = Scene;
    //Fog = fog;
    //FogColor = fogColor;
	//diffusePower = 0.91f;
	//dirL=core::vector3df(0,1,1);
	//lightEnable=false;
  }

  virtual void OnSetConstants(video::IMaterialRendererServices* services, s32 userData)
  {
    video::IVideoDriver* driver = services->getVideoDriver();

    if (driver->getDriverType() != video::EDT_OPENGL)
	{
		core::matrix4 worldViewProj(driver->getTransform(video::ETS_PROJECTION));
		worldViewProj *= driver->getTransform(video::ETS_VIEW);
		worldViewProj *= driver->getTransform(video::ETS_WORLD);
		services->setVertexShaderConstant("mWorldViewProj", worldViewProj.pointer(), 16);
	}
	else
    {
		const int tex[] = { 0, 1, 2, 3
			// , 4, 5, 6
			};
		services->setPixelShaderConstant("tex0", (float*)(&(tex[0])), 1);
		services->setPixelShaderConstant("tex1", (float*)(&(tex[1])), 1);
		services->setPixelShaderConstant("tex2", (float*)(&(tex[2])), 1);
		services->setPixelShaderConstant("tex3", (float*)(&(tex[3])), 1);
		//services->setPixelShaderConstant("tex4", (float*)(&(tex[4])), 1);
		//services->setPixelShaderConstant("tex5", (float*)(&(tex[5])), 1);
		//services->setPixelShaderConstant("tex6", (float*)(&(tex[6])), 1);
    }

	//if(Fog>0) 
	//{	
	//	core::matrix4 world = driver->getTransform(video::ETS_WORLD);
	//	services->setVertexShaderConstant("mWorld", world.pointer(),16);
	//}

    //if(lightEnable)	services->setVertexShaderConstant("dirToLight", reinterpret_cast<f32*>(&dirL), 3);

	//core::vector3df scaletex(16,16,16);
    //services->setVertexShaderConstant("scaletex", reinterpret_cast<f32*>(&scaletex), 3);

	//if(Fog>0)	services->setVertexShaderConstant("FogDistance",reinterpret_cast<f32*>(&Fog),1);

	//core::vector3df pos = smgr->getActiveCamera()->getPosition();
	//if(Fog>0) 	services->setVertexShaderConstant("cameraPos", reinterpret_cast<f32*>(&pos), 3);

	//if(Fog>0) 	services->setPixelShaderConstant("fog",reinterpret_cast<f32*>(&FogColor),4);
    //services->setPixelShaderConstant("diffusePower",reinterpret_cast<f32*>(&diffusePower),1);



	// Ajouté : Gestion de la première lumière dynamique directionnelle (d'après EcoWorldRenderer.cpp : NormalMappingShaderCallBack::OnSetConstants) :

	// lightDirection et diffuseColor (déterminés d'après la première lumière dynamique disponible)
	if (driver->getDynamicLightCount() > 0)
	{
		const video::SLight& light = driver->getDynamicLight(0);
		const core::vector3df lightDir(-light.Direction);
		services->setVertexShaderConstant("lightDirection", reinterpret_cast<const float*>(&lightDir), 3);
		services->setVertexShaderConstant("diffuseColor", reinterpret_cast<const float*>(&light.DiffuseColor), 3);
	}
	else
	{
		const core::vector3df lightDirection(-1.0f, 0.0f, 0.0f);
		const video::SColorf diffuseColor(0.0f, 0.0f, 0.0f, 1.0f);
		services->setVertexShaderConstant("lightDirection", reinterpret_cast<const float*>(&lightDirection), 3);
		services->setVertexShaderConstant("diffuseColor", reinterpret_cast<const float*>(&diffuseColor), 3);
	}

	// ambientColor
	const video::SColorf ambientColor = smgr->getAmbientLight();
	services->setPixelShaderConstant("ambientColor", reinterpret_cast<const float*>(&ambientColor), 4);
  }
};

class CTerrainSceneNode_N3xtD : public scene::ISceneNode
{
  public:
    enum TerrainQuality {High=1,Medium=2,Low=4,ExtraLow=8};
	CTerrainSceneNode_N3xtD(video::IImage* heightMap,TerrainQuality Quality, const core::vector3df& pos, const core::vector3df &terrainScale, f32 scaletexture, bool smooth,scene::ISceneNode* parent,scene::ISceneManager* smgr,s32 id=0);	// Surcharge ajoutée
	CTerrainSceneNode_N3xtD(int mSize, int seed, float noiseSize, float persistence, int octaves, TerrainQuality Quality, const core::vector3df &terrainScale, f32 ScaleTexture, bool smooth,scene::ISceneNode* parent,scene::ISceneManager* smgr,s32 id=0);
    ~CTerrainSceneNode_N3xtD();
    void Create(TerrainQuality quality, f32 ScaleTexture, bool smooth, scene::ISceneNode* parent,scene::ISceneManager* smgr,s32 id);
    void ActivateSplattingTextures(//scene::ISceneManager* smgr, bool LightDefine, f32 Fog,video::SColorf FogColor
		);
    float getHeight(float x,float z, bool interpolate=true);
    virtual void OnRegisterSceneNode();
    virtual void render();
    virtual void setPosition(const core::vector3df &Pos);
    virtual void setScale(const core::vector3df &Scale);
	void calculateBoundingBox();
	//void setFogValue(const float fog) { mc->Fog = fog; }
	//void setFogColor(const video::SColor &color) { mc->FogColor = color; }
	//void setDiffusePower(const float diffusepower) { mc->diffusePower = diffusepower; }
	//void setLightDir(const core::vector3df &dir) { mc->dirL = dir; }
	bool RaycastPick();
	void setGeometry(bool calcNormal);
	void setHeight(float x, float z, float value);

	inline bool testFRUSTUM_BOX(const scene::SViewFrustum &frust, const core::aabbox3df &box)
	{
		bool result=false;
		core::vector3df edges[8];
		box.getEdges(edges);

		u32 j;
		for (s32 i=0; i<scene::SViewFrustum::VF_PLANE_COUNT; ++i)
		{
			bool boxInFrustum=false;
			for (j=0; j<8; ++j)
			{
				if (frust.planes[i].classifyPointRelation(edges[j]) != core::ISREL3D_FRONT)
				{
					boxInFrustum=true;
					break;
				}
			}

			if (!boxInFrustum)
			{
				result = true;
				break;
			}
		}
		return result;
	}

  private:
    void calculateNormals (scene::SMeshBufferLightMap* pMeshBuffer, u32 Size);

	MyShaderCallBack* mc;
    core::aabbox3df Box;
    video::SMaterial Material;
    scene::SMeshBufferLightMap** CTTileBuffer;
    //bool Debug;
    scene::SMesh* TerrainMesh;
    u16 NbsTiles;
	u16 WTiles;
	u32 SOTiles;
	s32 SizeOfTiles;
	TerrainQuality Quality;
	float ScaleTexture;
	core::vector3df terrainPosition;
	scene::ILightSceneNode* dirLight;
	core::vector3df TerrainScale;

	// Ajouté :
	scene::ITriangleSelector** TriangleSelectors;
	scene::IMetaTriangleSelector* metaTriSelector;

  public:
	CHeightMap_N3xtD	mHeightmap;

	// Ajouté :
	virtual u32 getMaterialCount() const { return 1; }
	virtual const core::aabbox3df& getBoundingBox() const { return Box; }
	virtual video::SMaterial& getMaterial(u32 i) { return Material; }
	void createTriangleSelectors();
	virtual scene::ITriangleSelector* getTriangleSelector() const { return metaTriSelector; };
	virtual void setTriangleSelector(scene::ITriangleSelector* selector) { createTriangleSelectors(); };

	// Fonctions passées inline :

	void setDiffuseColor(const video::SColor &color)
	{
		/*for (u32 i=0;i<NbsTiles;++i)
		{
			if (CTTileBuffer[i])
			{
				CTTileBuffer[i]->Material.DiffuseColor=color;
			}
		}*/
		// Ajouté :
		Material.DiffuseColor=color;
	}
	void setAmbientColor(const video::SColor &color)
	{
		/*for (u32 i=0;i<NbsTiles;++i)
		{
			if (CTTileBuffer[i])
			{
				CTTileBuffer[i]->Material.AmbientColor=color;
			}
		}*/
		// Ajouté :
		Material.AmbientColor=color;
	}
	void setEmissiveColor(const video::SColor &color)
	{
		/*for (u32 i=0;i<NbsTiles;++i)
		{
			if (CTTileBuffer[i])
			{
				CTTileBuffer[i]->Material.EmissiveColor=color;
			}
		}*/
		// Ajouté :
		Material.EmissiveColor=color;
	}
	void setSpecularColor(const video::SColor &color)
	{
		/*for (u32 i=0;i<NbsTiles;++i)
		{
			if (CTTileBuffer[i])
			{
				CTTileBuffer[i]->Material.SpecularColor=color;
			}
		}*/
		// Ajouté :
		Material.SpecularColor=color;
	}

	void setColorTexture(video::ITexture* tex)
	{
		Material.setTexture(0, tex);
		setMaterialType(video::EMT_SOLID);
	}
	video::ITexture* setColorTexture(io::path FileName)
	{
		video::ITexture* tex = SceneManager->getVideoDriver()->getTexture(FileName);
	   /*for (u32 i=0;i<NbsTiles;++i)
		{
			if (CTTileBuffer[i])
			{
				CTTileBuffer[i]->Material.MaterialType = video::EMT_SOLID;
				CTTileBuffer[i]->Material.setTexture(0, tex);
			}
		}*/
		// Ajouté :
		Material.setTexture(0, tex);
		setMaterialType(video::EMT_SOLID);

		return tex;
	}
	void setDetailTexture(video::ITexture* tex)
	{
		if (tex)
		{
			Material.setTexture(1, tex);
			setMaterialType(video::EMT_DETAIL_MAP);
		}
	}
	video::ITexture* setDetailTexture(io::path FileName)
	{
		video::ITexture* tex = SceneManager->getVideoDriver()->getTexture(FileName);
	   /*for (u32 i=0;i<NbsTiles;++i)
		{
			if (CTTileBuffer[i])
			{
				CTTileBuffer[i]->Material.setTexture(5, tex);
			}
		}*/
   		// Ajouté :
		//Material.setTexture(5, tex);
		if (tex)
		{
			Material.setTexture(1, tex);
			setMaterialType(video::EMT_DETAIL_MAP);
		}

		return tex;
	}
	video::ITexture* SetTextureTerrain(u32 NumTex, io::path FileName)
	{
		video::ITexture* tex = SceneManager->getVideoDriver()->getTexture(FileName);
		/*for (u32 i=0;i<NbsTiles;++i)
		{
			if (CTTileBuffer[i])
			{
				CTTileBuffer[i]->Material.setTexture(NumTex, tex);
			}
		}*/
		// Ajouté :
		Material.setTexture(NumTex, tex);

		return tex;
	}
	void SetTextureTerrain(u32 NumTex, video::ITexture* tex)
	{
		/*for (u32 i=0;i<NbsTiles;++i)
		{
			if (CTTileBuffer[i])
			{
				CTTileBuffer[i]->Material.setTexture(NumTex, tex);

			}
		}*/
		// Ajouté :
		Material.setTexture(NumTex, tex);
	}
	video::ITexture* SetLightMapTexture(io::path FileName)
	{
		video::ITexture* tex = SceneManager->getVideoDriver()->getTexture(FileName);
		/*for (u32 i=0;i<NbsTiles;++i)
		{
			if (CTTileBuffer[i])
			{
				CTTileBuffer[i]->Material.setTexture(6, tex);	// Modifié : n° de texture : 5
			}
		}*/
		// Ajouté :
		Material.setTexture(6, tex);

		return tex;
	}
	void SetLightMapTexture(video::ITexture* tex, int size)
	{
		if(!tex)
		{
			video::IImage *img = SceneManager->getVideoDriver()->createImage( video::ECF_A8R8G8B8, core::dimension2du(size,size));	//core::dimension2du(256,256)
			for(int x=0; x<size; ++x)
				for(int y=0; y<size; ++y)
					img->setPixel( x,y, video::SColor(0xffffffff), false);

			tex	= SceneManager->getVideoDriver()->addTexture("terrainLightMap", img);
		}
		/*for (u32 i=0;i<NbsTiles;++i)
		{
			if (CTTileBuffer[i])
			{
				CTTileBuffer[i]->Material.setTexture(6, tex);
			}
		}*/
		// Ajouté :
		Material.setTexture(6, tex);
	}
	/*void setDebugMode(bool Enable)
	{
		Debug=Enable;
	}*/
	bool saveHeightMap(io::path filename)
	{
		return mHeightmap.saveHeightMap(filename, SceneManager);
	}
	virtual void setMaterialType(video::E_MATERIAL_TYPE Mat)
	{
		/*for (u32 i=0;i < NbsTiles;++i)
		{
			if (CTTileBuffer[i])
			{
				CTTileBuffer[i]->Material.MaterialType = Mat;
			}
		}*/
		// Ajouté :
		Material.MaterialType = Mat;
	}
	scene::IMesh* getMesh()
	{
		return (scene::IMesh*)TerrainMesh;
	}
};

}
}


#endif
