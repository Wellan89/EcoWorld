#ifndef H_XEFFECTS_CB
#define H_XEFFECTS_CB

#include "EffectHandler.h"

using namespace irr;
using namespace scene;
using namespace video;
using namespace core;

class DepthShaderCB : public video::IShaderConstantSetCallBack
{
public:
	DepthShaderCB() { }

	virtual void OnSetConstants(video::IMaterialRendererServices* services, s32 userData)
	{
		IVideoDriver* const driver = services->getVideoDriver();

		core::matrix4 worldViewProj = driver->getTransform(video::ETS_PROJECTION);			
		worldViewProj *= driver->getTransform(video::ETS_VIEW);
		worldViewProj *= driver->getTransform(video::ETS_WORLD);
		services->setVertexShaderConstant("mWorldViewProj", worldViewProj.pointer(), 16);

		services->setPixelShaderConstant("MaxD", &FarLink, 1);
	}

	f32 FarLink;
};

class ShadowShaderCB : public video::IShaderConstantSetCallBack
{
public:
	ShadowShaderCB(E_FILTER_TYPE FilterType) : sendMapResInv(FilterType > 0), MapResInv(1.0f) { }

	//virtual void OnSetMaterial(const SMaterial& material) {}

	virtual void OnSetConstants(video::IMaterialRendererServices* services, s32 userData)
	{
		IVideoDriver* const driver = services->getVideoDriver();

		const core::matrix4& world = driver->getTransform(video::ETS_WORLD);

		core::matrix4 worldViewProj = driver->getTransform(video::ETS_PROJECTION);			
		worldViewProj *= driver->getTransform(video::ETS_VIEW);
		worldViewProj *= world;
		services->setVertexShaderConstant("mWorldViewProj", worldViewProj.pointer(), 16);

		worldViewProj = ProjLink;			
		worldViewProj *= ViewLink;
		worldViewProj *= world;
		services->setVertexShaderConstant("mWorldViewProj2", worldViewProj.pointer(), 16);

		services->setVertexShaderConstant("LightDir", reinterpret_cast<f32*>(&LightDir.X), 3);

		services->setPixelShaderConstant("MaxD", reinterpret_cast<f32*>(&FarLink), 1);
		if (sendMapResInv)
			services->setPixelShaderConstant("MapResInv", &MapResInv, 1);

		services->setPixelShaderConstant("AmbientColour", reinterpret_cast<f32*>(&AmbientColour.r), 4);

		services->setPixelShaderConstant("LightColour", reinterpret_cast<f32*>(&LightColour.r), 4);
	}

	video::SColorf LightColour, AmbientColour;
	core::matrix4 ProjLink;
	core::matrix4 ViewLink;
	core::vector3df LightDir;
	f32 FarLink, MapResInv;

	bool sendMapResInv;
};


class ScreenQuadCB : public irr::video::IShaderConstantSetCallBack
{
public:
	ScreenQuadCB(EffectHandler* effectIn) : effect(effectIn) { }

	EffectHandler* effect;

	virtual void OnSetConstants(irr::video::IMaterialRendererServices* services, irr::s32 userData)
	{
		irr::video::IVideoDriver* const driver = services->getVideoDriver();

		if (driver->getDriverType() == irr::video::EDT_OPENGL)
		{
			irr::u32 TexVar = 0;
			services->setPixelShaderConstant("ColorMapSampler", (irr::f32*)(&TexVar), 1); 

			TexVar = 1;
			services->setPixelShaderConstant("ScreenMapSampler", (irr::f32*)(&TexVar), 1); 

			TexVar = 2;
			services->setPixelShaderConstant("DepthMapSampler", (irr::f32*)(&TexVar), 1); 

			TexVar = 3;
			services->setPixelShaderConstant("UserMapSampler", (irr::f32*)(&TexVar), 1);
		}

		const irr::core::dimension2du& currentRTTSize = driver->getCurrentRenderTargetSize();
		const irr::f32 screenX = (irr::f32)currentRTTSize.Width,
			screenY = (irr::f32)currentRTTSize.Height;
		services->setVertexShaderConstant("screenX", &screenX, 1);
		services->setVertexShaderConstant("screenY", &screenY, 1);

		const core::recti& viewPort = driver->getViewPort();
		irr::scene::ISceneManager* const smgr = effect->getActiveSceneManager();
		irr::scene::ICameraSceneNode* const cam = smgr->getActiveCamera();
		ISceneCollisionManager* const collisionManager = smgr->getSceneCollisionManager();
		const irr::core::line3df sLines[4] =
		{
			collisionManager->getRayFromScreenCoordinates(viewPort.UpperLeftCorner, cam),
			collisionManager->getRayFromScreenCoordinates(irr::core::vector2di(viewPort.LowerRightCorner.X, viewPort.UpperLeftCorner.Y), cam),
			collisionManager->getRayFromScreenCoordinates(irr::core::vector2di(viewPort.UpperLeftCorner.X, viewPort.LowerRightCorner.Y), cam),
			collisionManager->getRayFromScreenCoordinates(viewPort.LowerRightCorner, cam)
		};

		services->setVertexShaderConstant("LineStarts0", &sLines[0].start.X, 3);
		services->setVertexShaderConstant("LineStarts1", &sLines[1].start.X, 3);
		services->setVertexShaderConstant("LineStarts2", &sLines[2].start.X, 3);
		services->setVertexShaderConstant("LineStarts3", &sLines[3].start.X, 3);

		services->setVertexShaderConstant("LineEnds0", &sLines[0].end.X, 3);
		services->setVertexShaderConstant("LineEnds1", &sLines[1].end.X, 3);
		services->setVertexShaderConstant("LineEnds2", &sLines[2].end.X, 3);
		services->setVertexShaderConstant("LineEnds3", &sLines[3].end.X, 3);

		if (uniformDescriptors.size())
		{
			for (irr::core::map<irr::core::stringc, SUniformDescriptor>::Iterator mapIter = uniformDescriptors.getIterator(); mapIter.getNode(); mapIter++)
			{
				if (mapIter.getNode()->getValue().fPointer)
					services->setPixelShaderConstant(mapIter.getNode()->getKey().c_str(), mapIter.getNode()->getValue().fPointer, mapIter.getNode()->getValue().paramCount);
			}
		}
	}

	struct SUniformDescriptor
	{
		SUniformDescriptor() : fPointer(0), paramCount(0) { }
		SUniformDescriptor(const irr::f32* fPointerIn, irr::u32 paramCountIn) : fPointer(fPointerIn), paramCount(paramCountIn) { }

		const irr::f32* fPointer;
		irr::u32 paramCount;
	};

	irr::core::map<irr::core::stringc, SUniformDescriptor> uniformDescriptors;
};

#endif
