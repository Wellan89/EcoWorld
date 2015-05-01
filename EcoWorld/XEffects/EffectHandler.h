#ifndef H_XEFFECTS_EFFECT_HANDLER
#define H_XEFFECTS_EFFECT_HANDLER

// Cette version de XEffects a été largement modifiée :
// simplifiée (suppression de fonctions non utilisées), sécurisée et optimisée,
// suppression du préprocesseur de shaders (gérés nativement par OpenGL et DirectX, et ajout d'un préprocesseur de shader simplifié à chaque shader chargé dans EcoWorld),
// et suppression du support du PostProcessing et du DepthPass (maintenant géré par PostProcess)

#include "../global.h"

class CShaderPreprocessor;
class CScreenQuad;

/// Shadow mode enums, sets whether a node recieves shadows, casts shadows, or both.
/// If the mode is ESM_CAST, it will not be affected by shadows or lighting.
enum E_SHADOW_MODE
{
	ESM_RECEIVE,
	ESM_CAST,
	ESM_BOTH,
	ESM_EXCLUDE,
	ESM_COUNT
};

/// Various filter types, up to 16 samples PCF.
enum E_FILTER_TYPE
{
	EFT_NONE,
	EFT_4PCF,
	EFT_8PCF,
	EFT_12PCF,
	EFT_16PCF,
	EFT_COUNT
};

// Classe ajoutée, permettant de gérer les matériaux d'un node et de ses enfants, et d'afficher ce node et tous ses enfants s'ils sont visibles
class SNodeMaterialBuffer
{
protected:
	const core::list<scene::ISceneNode*>::ConstIterator END;		// Automatiquement initialisé en pointant sur NULL
	scene::ISceneNode* node;
	core::list<video::E_MATERIAL_TYPE> materialTypesBuffer;
	core::list<video::ITexture*> textures0Buffer;

	void changeNodeMaterialType(scene::ISceneNode* changeNode, video::E_MATERIAL_TYPE newMatType)
	{
		const u32 matCount = changeNode->getMaterialCount();
		for (u32 i = 0; i < matCount; ++i)
		{
			video::E_MATERIAL_TYPE& matType = changeNode->getMaterial(i).MaterialType;
			materialTypesBuffer.push_back(matType);
			matType = newMatType;
		}

		for (core::list<scene::ISceneNode*>::ConstIterator it = changeNode->getChildren().begin(); it != END; ++it)
			changeNodeMaterialType(*it, newMatType);
	}
	void changeNodeMaterialType_1(scene::ISceneNode* changeNode, video::E_MATERIAL_TYPE Depth, video::E_MATERIAL_TYPE DepthT)
	{
		const u32 matCount = changeNode->getMaterialCount();
		for (u32 i = 0; i < matCount; ++i)
		{
			video::E_MATERIAL_TYPE& matType = changeNode->getMaterial(i).MaterialType;
			materialTypesBuffer.push_back(matType);
			matType = (matType == video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF ? DepthT : Depth);
		}

		for (core::list<scene::ISceneNode*>::ConstIterator it = changeNode->getChildren().begin(); it != END; ++it)
			changeNodeMaterialType_1(*it, Depth, DepthT);
	}
	void changeNodeMaterialType_2(scene::ISceneNode* changeNode, video::E_MATERIAL_TYPE WhiteWash, video::E_MATERIAL_TYPE WhiteWashTRef, video::E_MATERIAL_TYPE WhiteWashTAdd, video::E_MATERIAL_TYPE WhiteWashTAlpha)
	{
		const u32 matCount = changeNode->getMaterialCount();
		for (u32 i = 0; i < matCount; ++i)
		{
			video::E_MATERIAL_TYPE& matType = changeNode->getMaterial(i).MaterialType;
			materialTypesBuffer.push_back(matType);
			switch(matType)
			{
			case video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF:
				matType = WhiteWashTRef;
				break;
			case video::EMT_TRANSPARENT_ADD_COLOR:
				matType = WhiteWashTAdd;
				break;
			case video::EMT_TRANSPARENT_ALPHA_CHANNEL:
				matType = WhiteWashTAlpha;
				break;
			default:
				matType = WhiteWash;
				break;
			}
		}

		for (core::list<scene::ISceneNode*>::ConstIterator it = changeNode->getChildren().begin(); it != END; ++it)
			changeNodeMaterialType_2(*it, WhiteWash, WhiteWashTRef, WhiteWashTAdd, WhiteWashTAlpha);
	}
	void changeNodeMaterialTypeAndTexture0(scene::ISceneNode* changeNode, video::E_MATERIAL_TYPE newMatType, video::ITexture* newTexture0)
	{
		const u32 matCount = changeNode->getMaterialCount();
		for (u32 i = 0; i < matCount; ++i)
		{
			video::SMaterial& mat = changeNode->getMaterial(i);
			materialTypesBuffer.push_back(mat.MaterialType);
			textures0Buffer.push_back(mat.TextureLayer[0].Texture);
			mat.MaterialType = newMatType;
			mat.TextureLayer[0].Texture = newTexture0;
		}

		for (core::list<scene::ISceneNode*>::ConstIterator it = changeNode->getChildren().begin(); it != END; ++it)
			changeNodeMaterialTypeAndTexture0(*it, newMatType, newTexture0);
	}
	void restoreMaterialType(scene::ISceneNode* restoreNode, core::list<video::E_MATERIAL_TYPE>::Iterator& currentIt)
	{
		const u32 matCount = restoreNode->getMaterialCount();
		for (u32 i = 0; i < matCount; ++currentIt, ++i)
			restoreNode->getMaterial(i).MaterialType = *currentIt;

		for (core::list<scene::ISceneNode*>::ConstIterator it = restoreNode->getChildren().begin(); it != END; ++it)
			restoreMaterialType(*it, currentIt);
	}
	void restoreMaterialTypeAndTexture0(scene::ISceneNode* restoreNode, core::list<video::E_MATERIAL_TYPE>::Iterator& currentItMat, core::list<video::ITexture*>::Iterator& currentItTex)
	{
		const u32 matCount = restoreNode->getMaterialCount();
		for (u32 i = 0; i < matCount; ++currentItMat, ++currentItTex, ++i)
		{
			video::SMaterial& mat = restoreNode->getMaterial(i);
			mat.MaterialType = *currentItMat;
			mat.TextureLayer[0].Texture = *currentItTex;
		}

		for (core::list<scene::ISceneNode*>::ConstIterator it = restoreNode->getChildren().begin(); it != END; ++it)
			restoreMaterialTypeAndTexture0(*it, currentItMat, currentItTex);
	}
	void renderNodeAndChildren(scene::ISceneNode* renderNode)
	{
		if (renderNode && renderNode->isVisible())
		{
			renderNode->render();

			for (core::list<scene::ISceneNode*>::ConstIterator it = renderNode->getChildren().begin(); it != END; ++it)
				renderNodeAndChildren(*it);
		}
	}

public:
	SNodeMaterialBuffer(scene::ISceneNode* Node) : node(Node)	{ }

	void changeNodeMaterialType(video::E_MATERIAL_TYPE newMatType)
	{ changeNodeMaterialType(node, newMatType); }
	void changeNodeMaterialType_1(video::E_MATERIAL_TYPE Depth, video::E_MATERIAL_TYPE DepthT)
	{ changeNodeMaterialType_1(node, Depth, DepthT); }
	void changeNodeMaterialType_2(video::E_MATERIAL_TYPE WhiteWash, video::E_MATERIAL_TYPE WhiteWashTRef, video::E_MATERIAL_TYPE WhiteWashTAdd, video::E_MATERIAL_TYPE WhiteWashTAlpha)
	{ changeNodeMaterialType_2(node, WhiteWash, WhiteWashTRef, WhiteWashTAdd, WhiteWashTAlpha); }
	void changeNodeMaterialTypeAndTexture0(video::E_MATERIAL_TYPE newMatType, video::ITexture* newTexture0)
	{ changeNodeMaterialTypeAndTexture0(node, newMatType, newTexture0); }
	void restoreMaterialType()
	{ restoreMaterialType(node, materialTypesBuffer.begin()); }
	void restoreMaterialTypeAndTexture0()
	{ restoreMaterialTypeAndTexture0(node, materialTypesBuffer.begin(), textures0Buffer.begin()); }
	void renderNodeAndChildren()
	{ renderNodeAndChildren(node); }
};

struct SShadowLight
{
	/// Shadow light constructor. The first parameter is the square shadow map resolution.
	/// This should be a power of 2 number, and within reasonable size to achieve optimal
	/// performance and quality. Recommended sizes are 512 to 4096 subject to your target
	/// hardware and quality requirements. The next two parameters are position and target,
	/// the next one is the light color. The next two are very important parameters,
	/// the far value and the near value. The higher the near value, and the lower the
	/// far value, the better the depth precision of the shadows will be, however it will
	/// cover a smaller volume. The next is the FOV, if the light was to be considered
	/// a camera, this would be similar to setting the camera's field of view. The last
	/// parameter is whether the light is directional or not, if it is, an orthogonal
	/// projection matrix will be created instead of a perspective one.
	SShadowLight(	const irr::u32 shadowMapResolution,
					const irr::core::vector3df& position, 
					const irr::core::vector3df& target,
					const irr::video::SColorf& lightColour = irr::video::SColor(0xffffffff), 
					irr::f32 nearValue = 10.0, irr::f32 farValue = 100.0,
					irr::f32 fov = 90.0 * irr::core::DEGTORAD64, bool directional = false)
					:	pos(position), tar(target), farPlane(directional ? 1.0f : farValue), diffuseColour(lightColour), mapRes(shadowMapResolution)
	{
		//nearValue = nearValue <= 0.0f ? 0.1f : nearValue;

		updateViewMatrix();

		if (directional)
			projMat.buildProjectionMatrixOrthoLH(fov, fov, nearValue, farValue);
		else
			projMat.buildProjectionMatrixPerspectiveFovLH(fov, 1.0f, nearValue, farValue);
	}

	/// Sets the light's position.
	void setPosition(const irr::core::vector3df& position)
	{
		pos = position;
		//updateViewMatrix();
	}

	/// Sets the light's target.
	void setTarget(const irr::core::vector3df& target)
	{
		tar = target;
		//updateViewMatrix();
	}

	/// Gets the light's position.
	const irr::core::vector3df& getPosition() const					{ return pos; }

	/// Gets the light's target.
	const irr::core::vector3df& getTarget()  const					{ return tar; }

	/// Sets the light's view matrix.
	/*void setViewMatrix(const irr::core::matrix4& matrix)
	{
		viewMat = matrix;
		irr::core::matrix4 vInverse;
		viewMat.getInverse(vInverse);
		pos = vInverse.getTranslation();
	}*/

	/// Sets the light's projection matrix.
	void setProjectionMatrix(const irr::core::matrix4& matrix)		{ projMat = matrix; }

	/// Gets the light's view matrix.
	irr::core::matrix4& getViewMatrix()								{ return viewMat; }

	/// Gets the light's projection matrix.
	irr::core::matrix4& getProjectionMatrix()						{ return projMat; }

	/// Gets the light's far value.
	irr::f32 getFarValue() const									{ return farPlane; }

	/// Gets the light's color.
	const irr::video::SColorf& getLightColor() const				{ return diffuseColour; }

	/// Sets the light's color.
	void setLightColor(const irr::video::SColorf& lightColour)		{ diffuseColour = lightColour; }

	/// Sets the shadow map resolution for this light.
	void setShadowMapResolution(const irr::u32 shadowMapResolution)	{ mapRes = shadowMapResolution; }

	/// Gets the shadow map resolution for this light.
	irr::u32 getShadowMapResolution() const							{ return mapRes; }

//private:
	void updateViewMatrix()
	{
		viewMat.buildCameraLookAtMatrixLH(pos, tar,
			(core::equals((pos - tar).dotProduct(core::vector3df(1.0f, 0.0f, 1.0f)), 0.0f) ?
				core::vector3df(0.0f, 0.0f, 1.0f) : core::vector3df(0.0f, 1.0f, 0.0f))); 
	}

	// Ajouté :
	// Renvoie la direction opposée et normalisée de la lumière, pour l'envoi vers les shaders
	irr::core::vector3df getDirectionOpp() const					{ return (tar - pos).normalize(); }

protected:
	irr::video::SColorf diffuseColour;
	irr::core::vector3df pos, tar;
	irr::f32 farPlane;
	irr::core::matrix4 viewMat, projMat;
	irr::u32 mapRes;
};

// This is a general interface that can be overidden if you want to perform operations before or after
// a specific post-processing effect. You will be passed an instance of the EffectHandler.
// The function names themselves should be self-explanatory ;)
class EffectHandler;
class IPostProcessingRenderCallback
{
public:
	virtual void OnPreRender(EffectHandler* effect) = 0;
	virtual void OnPostRender(EffectHandler* effect) = 0;
};

// Shader callback prototypes.
class DepthShaderCB;
class ShadowShaderCB;
class ScreenQuadCB;

/// Main effect handling class, use this to apply shadows and effects.
class EffectHandler
{
public:

	/*	EffectHandler constructor. Initializes the EffectHandler.

		Parameters:
		irrlichtDevice: Current Irrlicht device.
		screenRTTSize: Size of screen render target for post processing. Default is screen size.
		useVSMShadows: Shadows will use VSM filtering. It is recommended to only use EFT_NONE when this is enabled.
		useRoundSpotlights: Shadow lights will have a soft round spot light mask. Default is false.
		use32BitDepthBuffers: XEffects will use 32-bit depth buffers if this is true, otherwise 16-bit. Default is false.
	*/
	EffectHandler(irr::IrrlichtDevice* irrlichtDevice, const irr::core::dimension2du& screenRTTSize,
		const bool useVSMShadows, const bool useRoundSpotLights, const bool use32BitDepthBuffers,
		E_FILTER_TYPE FilterType, CShaderPreprocessor* shaderPreprocessor, CScreenQuad* screenQuad);	// Ajoutés
	
	/// Destructor.
	~EffectHandler();

	/// Adds a shadow light. Check out the shadow light constructor for more information.
	void addShadowLight(const SShadowLight& shadowLight)
	{
		LightList.push_back(shadowLight);
	}

	/// Retrieves a reference to a shadow light. You may get the max amount from getShadowLightCount.
	SShadowLight& getShadowLight(irr::u32 index)
	{
		return LightList[index];
	}

	/// Retrieves the current number of shadow lights.
	const irr::u32 getShadowLightCount() const
	{
		return LightList.size();
	}

	/// Retrieves the shadow map texture for the specified square shadow map resolution.
	/// Only one shadow map is kept for each resolution, so if multiple lights are using
	/// the same resolution, you will only see the last light drawn's output.
	/// The secondary param specifies whether to retrieve the secondary shadow map used in blurring.
	irr::video::ITexture* getShadowMapTexture(const irr::u32 resolution, const bool secondary = false);

	/// Removes shadows from a scene node.
	void removeShadowFromNode(irr::scene::ISceneNode* node)
	{
		SShadowNode tmpShadowNode = {node, ESM_RECEIVE};
		irr::s32 i = ShadowNodeArray.binary_search(tmpShadowNode);

		if(i != -1)
			ShadowNodeArray.erase(i);
	}

	// Excludes a scene node from lighting calculations, avoiding any side effects that may
	// occur from XEffect's light modulation on this particular scene node.
	void excludeNodeFromLightingCalculations(irr::scene::ISceneNode* node)
	{
		SShadowNode tmpShadowNode = {node, ESM_EXCLUDE};
		ShadowNodeArray.push_back(tmpShadowNode);
	}

	/// Updates the effects handler. This must be done between IVideoDriver::beginScene and IVideoDriver::endScene.
	/// This function now replaces smgr->drawAll(). So place it where smgr->drawAll() would normally go. Please note
	/// that the clear colour from IVideoDriver::beginScene is not preserved, so you must instead specify the clear
	/// colour using EffectHandler::setClearColour(Colour).
	/// A render target may be passed as the output target, else rendering will commence on the backbuffer.
	void update(irr::video::ITexture* outputTarget = 0);

	/// Adds a shadow to the scene node. The filter type specifies how many shadow map samples
	/// to take, a higher value can produce a smoother or softer result. The shadow mode can
	/// be either ESM_BOTH, ESM_CAST, or ESM_RECEIVE. ESM_BOTH casts and receives shadows,
	/// ESM_CAST only casts shadows, and is unaffected by shadows or lighting, and ESM_RECEIVE
	/// only receives but does not cast shadows.
	void addShadowToNode(irr::scene::ISceneNode* node, E_SHADOW_MODE shadowMode = ESM_BOTH);

	/// Returns the device time divided by 100, for use with the shader callbacks.
	irr::f32 getTime()
	{ 
		return device->getTimer()->getTime() * 0.01f;
	}

	/// Sets the scene clear colour, for when the scene is cleared before smgr->drawAll().
	void setClearColour(const irr::video::SColor& ClearCol)
	{
		ClearColour = ClearCol;
	}

	/// Returns the screen quad scene node. This is not required in any way, but some advanced users may want to adjust
	/// its material settings accordingly.
	/*CScreenQuad* getScreenQuad()
	{
		return ScreenQuad;
	}*/

	/// Sets the active scene manager.
	void setActiveSceneManager(irr::scene::ISceneManager* smgrIn)
	{
		smgr = smgrIn;
	}

	/// Gets the active scene manager.
	irr::scene::ISceneManager* getActiveSceneManager()
	{
		return smgr;
	}
	
	/// Sets the global ambient color for shadowed scene nodes.
	void setAmbientColor(const irr::video::SColor& ambientColour)
	{
		AmbientColour = ambientColour;
	}

	/// Gets the global ambient color.
	const irr::video::SColor& getAmbientColor() const
	{
		return AmbientColour;
	}

	/// Sets a new screen render target resolution.
	void setScreenRenderTargetResolution(const irr::core::dimension2du& resolution);

	/// Returns the device that this EffectHandler was initialized with.
	//irr::IrrlichtDevice* getIrrlichtDevice() {return device;}

private:
	struct SShadowNode
	{
		bool operator<(const SShadowNode& other) const
		{
			return node < other.node;
		}

		irr::scene::ISceneNode* node;

		E_SHADOW_MODE shadowMode;
	};

	irr::IrrlichtDevice* device;
	irr::video::IVideoDriver* driver;
	irr::scene::ISceneManager* smgr;
	irr::core::dimension2du mapRes;

	irr::s32 Depth;
	irr::s32 DepthT;
	irr::s32 DepthWiggle;
	irr::s32 Shadow;
	irr::s32 LightModulate;
	irr::s32 Simple;
	irr::s32 WhiteWash;
	irr::s32 WhiteWashTRef;
	irr::s32 WhiteWashTAdd;
	irr::s32 WhiteWashTAlpha;
	irr::s32 VSMBlurH;
	irr::s32 VSMBlurV;
	E_FILTER_TYPE filterType;

	DepthShaderCB* depthMC;
	ShadowShaderCB* shadowMC;

	irr::core::array<SShadowLight> LightList;
	irr::core::array<SShadowNode> ShadowNodeArray;

	irr::core::dimension2du ScreenRTTSize;
	irr::video::SColor ClearColour;
	irr::video::SColor AmbientColour;

	CScreenQuad* ScreenQuad;
	video::SMaterial screenQuadMat;
	video::ITexture* screenRTT[2];

	bool shadowsUnsupported;
	bool use32BitDepth;
	bool useVSM;

public:
	// Ajouté :
	void reset()
	{
		ShadowNodeArray.clear();
	}
};

#endif

// Copyright (C) 2007-2009 Ahmed Hilali
