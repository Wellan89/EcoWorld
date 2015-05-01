// Site : http://forum.irrlicht.fr/viewtopic.php?id=336
// Cette version a été modifiée

#ifndef _CTerrain_Copland
#define _CTerrain_Copland

//#include <IRR\irrlicht.h>
//using namespace irr;
#include "global.h"

class MyShaderCallBack_copland : public video::IShaderConstantSetCallBack
{
public:
    scene::ISceneManager* smgr;
    f32 Height;
	//f32 Fog;
    //video::SColorf FogColor;

    void setSplatScene(scene::ISceneManager* Scene,
		f32 height
		//,f32 fog,video::SColorf fogColor
		)
    {
        smgr=Scene;
        Height = height;
        //Fog = fog;
        //FogColor = fogColor;
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
			const int tex[] = { 0, 1, 2, 3 };
            services->setPixelShaderConstant("tex0", (float*)(&(tex[0])), 1);
            services->setPixelShaderConstant("tex1", (float*)(&(tex[1])), 1);
            services->setPixelShaderConstant("tex2", (float*)(&(tex[2])), 1);
            services->setPixelShaderConstant("tex3", (float*)(&(tex[3])), 1);
        }

        services->setVertexShaderConstant("TerrainHeight",reinterpret_cast<f32*>(&Height),1);
        //services->setVertexShaderConstant("FogDistance",reinterpret_cast<f32*>(&Fog),1);

        //core::vector3df pos = smgr->getActiveCamera()->getPosition();
        //services->setVertexShaderConstant("cameraPos", reinterpret_cast<f32*>(&pos), 3);
        
        //core::matrix4 world = driver->getTransform(video::ETS_WORLD);
        //services->setVertexShaderConstant("mWorld",world.pointer(),16);

        //services->setPixelShaderConstant("fog",reinterpret_cast<f32*>(&FogColor),4);



		// Ajouté : Gestion de la première lumière dynamique directionnelle  (d'après EcoWorldRenderer.cpp : NormalMappingShaderCallBack::OnSetConstants) :

		// lightDirection et diffuseColor (déterminés d'après la première lumière dynamique disponible)
		if (driver->getDynamicLightCount() > 0)
		{
			const video::SLight& light = driver->getDynamicLight(0);
			const core::vector3df lightDir(-light.Direction);
			services->setVertexShaderConstant("lightDirection", reinterpret_cast<const float*>(&lightDir), 3);
			services->setVertexShaderConstant("diffuseColor", reinterpret_cast<const float*>(&light.DiffuseColor), 4);
		}
		else
		{
			const core::vector3df lightDirection(-1.0f, 0.0f, 0.0f);
			const video::SColorf diffuseColor(0.0f, 0.0f, 0.0f, 1.0f);
			services->setVertexShaderConstant("lightDirection", reinterpret_cast<const float*>(&lightDirection), 3);
			services->setVertexShaderConstant("diffuseColor", reinterpret_cast<const float*>(&diffuseColor), 4);
		}

		// ambientColor
		const video::SColorf ambientColor = smgr->getAmbientLight();
		services->setPixelShaderConstant("ambientColor", reinterpret_cast<const float*>(&ambientColor), 4);
    }
};

class CTerrain_Copland : public scene::ISceneNode
{
    public:
        const enum TerrainQuality {High=1,Medium=2,Low=4,ExtraLow=8};
		CTerrain_Copland(video::IImage* Heightmap,TerrainQuality Quality,f32 ScaleTexture, const core::vector3df& pos, const core::vector3df& scale,scene::ISceneNode* parent,scene::ISceneManager* smgr,s32 id);	// Constructeur modifié
		~CTerrain_Copland();
        void ActivateSplattingTextures(//scene::ISceneManager* smgr,
			f32 Height
			//,f32 Fog,video::SColorf FogColor
			);
        f32 getHeight(f32 x,f32 z);
        virtual void OnRegisterSceneNode();
        virtual void render();
        virtual void setPosition(const core::vector3df &Pos);
        virtual void setScale(const core::vector3df &Scale);
    private:
        core::aabbox3df Box;
        video::SMaterial Material;
        scene::SMeshBufferLightMap** CTTileBuffer;
        void calculateNormals (scene::SMeshBufferLightMap* pMeshBuffer, u32 Size);
        //bool Debug;
        f32 RenderDistance;
        scene::SMesh* TerrainMesh;
        u16 NbsTiles;
		MyShaderCallBack_copland* mc;

		// Ajouté :
		scene::ITriangleSelector** TriangleSelectors;
		scene::IMetaTriangleSelector* metaTriSelector;

public:	// Ajouté :
		u32 getMaterialCount() const { return 1; }
		virtual video::SMaterial& getMaterial(u32 i) { return Material; }
		virtual const core::aabbox3df& getBoundingBox() const { return Box; }
		void createTriangleSelectors();
		virtual scene::ITriangleSelector* getTriangleSelector() const { return metaTriSelector; };
		virtual void setTriangleSelector(scene::ITriangleSelector* selector) { createTriangleSelectors(); };

		// Fonctions passées inline :

		void setColorTexture(video::ITexture* tex)
		{
		   /* for (u32 i=0;i<NbsTiles;++i)
			{
				if (CTTileBuffer[i])
				{
					CTTileBuffer[i]->Material.MaterialType = video::EMT_SOLID;
					CTTileBuffer[i]->Material.setTexture(0, SceneManager->getVideoDriver()->getTexture(FileName));
				}
			}*/

			// Ajouté :
			Material.MaterialType = video::EMT_SOLID;
			Material.setTexture(0, tex);
		}
		void setDetailTexture(video::ITexture* tex)
		{
			/*for (u32 i=0;i<NbsTiles;++i)
			{
				if (CTTileBuffer[i])
				{
					CTTileBuffer[i]->Material.MaterialType = video::EMT_DETAIL_MAP;
					CTTileBuffer[i]->Material.setTexture(1, SceneManager->getVideoDriver()->getTexture(FileName));
				}
			}*/

			// Ajouté :
			if (tex)
			{
				Material.setTexture(1, tex);
				Material.MaterialType = video::EMT_DETAIL_MAP;
			}
		}
		void SetTextureSplat(u32 NumTex, video::ITexture* tex)
		{
			/*for (u32 i=0;i<NbsTiles;++i)
			{
				if (CTTileBuffer[i])
				{
					CTTileBuffer[i]->Material.setTexture(NumTex, SceneManager->getVideoDriver()->getTexture(FileName));
				}
			}*/

			// Ajouté :
			Material.setTexture(NumTex, tex);
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
		/*void setDebugMode(bool Enable)
		{
			Debug=Enable;
		}*/
		void setRenderDistance(f32 Distance)
		{
			RenderDistance = Distance * Distance;
		}
		scene::IMesh* getMesh()
		{
			return (scene::IMesh*)TerrainMesh;
		}
};

#endif
