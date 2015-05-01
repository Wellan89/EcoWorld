/*
 * Copyright (c) 2007, elvman
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY elvman ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <copyright holder> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// Cette version a été largement modifiée :
//	- Fuites mémoires supprimées : Il fallait séparer les deux interfaces scene::ISceneNode et video::IShaderConstantSetCallBack, pour qu'elles ne se "grab" pas mutuellement
//	- Gestion des matériaux (nécessaire pour l'activation du brouillard et de certains paramêtres de rendu)
//	- Optimisation des shaders + Prise en compte de la hauteur réelle de l'eau

#ifndef DEF_REALISTIC_WATER
#define DEF_REALISTIC_WATER

//#include <IRR\irrlicht.h>
//using namespace irr;
#include "global.h"

#ifdef USE_SPARK
class SparkManager;
#endif

class RealisticWaterSceneNode : public scene::ISceneNode
{
public:
	RealisticWaterSceneNode(
#ifdef USE_SPARK
		SparkManager& sparkManager,
#endif
		f32 width, f32 height, video::ITexture* bumpTexture, bool animatedSurface = false,
		core::dimension2du waterSubdivisions = core::dimension2du(50, 50), float waveHeight = 2.0f, float waveSpeed = 300.0f, float waveLenght = 10.0f, 
		core::dimension2du renderTargetSize=core::dimension2du(512, 512), scene::ISceneNode* parent = 0, s32 id = -1);
	virtual ~RealisticWaterSceneNode();

	// frame
	virtual void OnRegisterSceneNode();

	virtual void render()	{ }

	// Ajouté : Prépare le rendu de l'eau (à effectuer avant l'appel à sceneManager->drawAll())
	void preRender(video::ITexture* screenRenderTarget = 0);

protected:
#ifdef USE_SPARK
	SparkManager& sparkMgr;
#endif

	scene::ICameraSceneNode*		Camera;
	scene::ISceneNode*				WaterSceneNode;

	video::ITexture*				RefractionMap;
	video::ITexture*				ReflectionMap;

	int ShaderMaterial;

	class RealisticWaterShaderCallBack : public video::IShaderConstantSetCallBack
	{
	public:
		core::vector2df					Wind;
		f32								WaveHeight;
		video::SColorf					WaterColor;
		f32								ColorBlendFactor;

		RealisticWaterShaderCallBack() : Wind(0.0f, 20.0f), WaveHeight(0.3f), WaterColor(0.1f, 0.1f, 0.6f, 1.0f), ColorBlendFactor(0.2f)	{ }

		virtual void OnSetConstants(video::IMaterialRendererServices* services, s32 userData);
	};

	RealisticWaterShaderCallBack*	shaderCallBack;

public:
	// returns the axis aligned bounding box of terrain
	const core::aabbox3df& getBoundingBox() const	{ return WaterSceneNode->getBoundingBox(); }

	// Fonnctions passées inline :
	void setWindForce(const f32 windForce)
	{
		shaderCallBack->Wind.normalize();
		shaderCallBack->Wind *= windForce;
	}
	void setWindDirection(const core::vector2df& windDirection)
	{
		const float windForce = shaderCallBack->Wind.getLength();
		shaderCallBack->Wind = windDirection;
		shaderCallBack->Wind.normalize();
		shaderCallBack->Wind *= windForce;
	}
	void setWaveHeight(const f32 waveHeight)
	{
		shaderCallBack->WaveHeight = waveHeight;
	}
	void setWaterColor(const video::SColorf& waterColor)
	{
		shaderCallBack->WaterColor = waterColor;
	}
	void setColorBlendFactor(const f32 colorBlendFactor)
	{
		shaderCallBack->ColorBlendFactor = colorBlendFactor;
	}
};

#endif
