#include "CPostProcessManager.h"
#include "CShaderPreprocessor.h"
#include "CShaderMaterial.h"
#include "CScreenQuad.h"
#include "GameConfiguration.h"

// Classe ajoutée permettant de gérer les matériaux d'un node et de ses enfants (basée sur la classe semblable SNodeMaterialBuffer utilisée dans la version modifiée de XEffects),
// et d'afficher ce node et tous ses enfants s'ils sont visibles
class PP_SNodeMaterialBuffer
{
protected:
	const core::list<scene::ISceneNode*>::ConstIterator END;		// Automatiquement initialisé en pointant sur NULL
	scene::ISceneNode* node;
	core::list<video::E_MATERIAL_TYPE> materialTypesBuffer;

	void changeNodeMaterialType(scene::ISceneNode* changeNode, video::E_MATERIAL_TYPE newMatType)
	{
		if (changeNode)
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
	}
	void restoreMaterialType(scene::ISceneNode* restoreNode, core::list<video::E_MATERIAL_TYPE>::Iterator& currentIt)
	{
		if (restoreNode)
		{
			const u32 matCount = restoreNode->getMaterialCount();
			for (u32 i = 0; i < matCount; ++currentIt, ++i)
				restoreNode->getMaterial(i).MaterialType = *currentIt;

			for (core::list<scene::ISceneNode*>::ConstIterator it = restoreNode->getChildren().begin(); it != END; ++it)
				restoreMaterialType(*it, currentIt);
		}
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
	PP_SNodeMaterialBuffer(scene::ISceneNode* Node) : node(Node)	{ }

	void changeNodeMaterialType(video::E_MATERIAL_TYPE newMatType)
	{ changeNodeMaterialType(node, newMatType); }
	void restoreMaterialType()
	{ restoreMaterialType(node, materialTypesBuffer.begin()); }
	void renderNodeAndChildren()
	{ renderNodeAndChildren(node); }
};

CPostProcessManager::CPostProcessManager(IrrlichtDevice* Device, CShaderPreprocessor* ShaderPreprocessor, CScreenQuad* ScreenQuad)
 : device(Device), driver(device->getVideoDriver()), DepthMaterial(NULL), shaderPreprocessor(ShaderPreprocessor), screenQuad(ScreenQuad), alreadyRendered(false),
 maxMRTs((u32)core::clamp(driver->getDriverAttributes().getAttributeAsInt("MaxMultipleRenderTargets"), 1, 4))
{
	// load the rtt and effect configuration 
	loadRTTConfig();
	loadEffectConfig();

	LOG("PostProcess: Configuration loaded", ELL_INFORMATION);
}
CPostProcessManager::~CPostProcessManager()
{
	// drop all PostProcess Objects
	for (core::map<core::stringw, CEffectChain*>::Iterator iter = EffectChain.getIterator(); !iter.atEnd(); iter++)
	{
		CEffectChain* const effect = (*iter).getValue();
		const u32 effectChainSize = effect->size();
		for(u32 j = 0; j < effectChainSize; ++j)
		{
			CShaderMaterial* const shaderMaterial = (*effect)[j];
			if (shaderMaterial)
				shaderMaterial->drop();
		}
		delete effect;
	}
	EffectChain.clear();

	// drop the depth material
	if (DepthMaterial)
		DepthMaterial->drop();
}
void CPostProcessManager::compileParticularShaders(bool compileShakeEffect, bool compileDepthMaterial)
{
	// Shader de rendu final
	compileEffect(EFFECT_NAME_FINAL);

	// Shader de tremblement de la caméra
	if (compileShakeEffect)
		compileEffect(EFFECT_NAME_SHAKE);

	// Crée toujours les deux textures de rendu par défaut
	getRTTTexture(RTT_NAME_IN);
	getRTTTexture(RTT_NAME_OUT);

	// Shader de rendu de la profondeur de la scène
	if (compileDepthMaterial && !DepthMaterial)
	{
		LOG("PostProcess: Compiling material \"Depth\" for depth pass", ELL_INFORMATION);

		// prepare the depth material for depth pass
		DepthMaterial = new CShaderMaterial(device, *this, maxMRTs, L"Depth", "depth", "main", video::EVST_VS_1_1, "depth", "main", video::EPST_PS_2_0, video::EMT_SOLID);
		if (DepthMaterial)
		{
			DepthMaterial->setVertexShaderFlag(ESC_WORLD);
			if (driver->getDriverType() != video::EDT_OPENGL)
				DepthMaterial->setVertexShaderFlag(ESC_WORLDVIEWPROJ);
			DepthMaterial->setVertexShaderConstant("MaxDistanceInv", 1.0f / CAMERA_FAR_VALUE);

			DepthMaterial->compileShader();
			if (!DepthMaterial->isCompiled())
				LOG("PostProcess: Warning : Could not compile shader material \"Depth\" for depth pass !", ELL_WARNING);
		}
		else
			LOG("PostProcess: Warning : NULL shader material \"Depth\" for depth pass !", ELL_WARNING);

		// Crée la texture de rendu pour la profondeur de la scène
		getRTTTexture(RTT_NAME_DEPTH);
	}
}
void CPostProcessManager::compileEffects(const core::array<core::stringw>& effects)
{
	// Compile chaque effet indépendamment
	const u32 effectsSize = effects.size();
	for (u32 i = 0; i < effectsSize; ++i)
		compileEffect(effects[i]);
}
void CPostProcessManager::compileEffect(const core::stringw& effect)
{
	bool haveAlreadyLoggedCompilingEffect = false;
	const core::map<core::stringw, CEffectChain*>::Node* const effNode = EffectChain.find(effect);
	if (effNode)
	{
		const CEffectChain* const effChain = effNode->getValue();
		if (effChain)
		{
			const u32 effectChainSize = effChain->size();
			if (effectChainSize)
			{
				for (u32 i = 0; i < effectChainSize; ++i)
				{
					CShaderMaterial* const shaderMaterial = (*effChain)[i];
					if (shaderMaterial)
					{
						if (!shaderMaterial->isCompiled())
						{
							if (!haveAlreadyLoggedCompilingEffect)
							{
								LOG("PostProcess: Compiling effect \"" << core::stringc(effect).c_str() << "\"", ELL_INFORMATION);
								haveAlreadyLoggedCompilingEffect = true;
							}
							shaderMaterial->compileShader();
							if (!shaderMaterial->isCompiled())
								LOG("PostProcess: Warning : Could not compile shader material of pass \"" << core::stringc(shaderMaterial->getName()).c_str() << "\" (" << (i + 1) << " of " << effectChainSize << ") for effect \"" << core::stringc(effect).c_str() << "\" !", ELL_WARNING);
						}
					}
					else
						LOG("PostProcess: Warning : NULL shader material of pass " << (i + 1) << " of " << effectChainSize << " for effect \"" << core::stringc(effect).c_str() << "\" !", ELL_WARNING);
				}
			}
			else
				LOG("PostProcess: Warning : Empty effect chain for effect \"" << core::stringc(effect).c_str() << "\" !", ELL_WARNING);
		}
		else
			LOG("PostProcess: Warning : NULL effect chain for effect \"" << core::stringc(effect).c_str() << "\" !", ELL_WARNING);
	}
	else
		LOG("PostProcess: Warning : Cannot find effect \"" << core::stringc(effect).c_str() << "\" !", ELL_WARNING);
}
video::ITexture* CPostProcessManager::getRTTTexture(const core::stringw& id, bool allowCreateTexture)
{
	// Vérifie que la texture RTT n'est pas déjà créée : dans ce cas, on la renvoie directement
	const core::map<core::stringw, video::ITexture*>::Node* const rttNode = RenderTargetMap.find(id);
	if (rttNode)
	{
		video::ITexture* const rttTexture = rttNode->getValue();
		if (rttTexture)
			return rttTexture;
	}

	if (allowCreateTexture)
	{
		// Si la texture RTT n'est pas encore créée, on la crée d'après ses informations puis on la renvoie
		const core::map<core::stringw, RTTInfos>::Node* const infosNode = RTTInfosMap.find(id);
		if (infosNode)
		{
			LOG("PostProcess: Creating RTT texture \"" << core::stringc(id).c_str() << "\"", ELL_INFORMATION);

			//// store driver flags
			//const bool flagMipMaps = driver->getTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS);
			//const bool flag32Bit = driver->getTextureCreationFlag(video::ETCF_ALWAYS_32_BIT);

			//// set new flags for rtt creation
			//driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, false);
			//driver->setTextureCreationFlag(video::ETCF_ALWAYS_32_BIT, true);

			const RTTInfos& infos = infosNode->getValue();
			video::ITexture* const rttTexture = driver->addRenderTargetTexture(infos.size, id, infos.colorFormat);
			RenderTargetMap[id] = rttTexture;

			//// restore driver flags
			//driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, flagMipMaps);
			//driver->setTextureCreationFlag(video::ETCF_ALWAYS_32_BIT, flag32Bit);

			if (!rttTexture)
				LOG("PostProcess: Warning : Could not create RTT texture \"" << core::stringc(id).c_str() << "\" !", ELL_WARNING);

			return rttTexture;
		}
	}

	// Impossible de trouver les informations sur cette texture RTT : on retourne alors NULL
	return NULL;
}
void CPostProcessManager::SwapAuxBuffers()
{
	// swap the in and out buffers
	video::ITexture* const tmp = RenderTargetMap[RTT_NAME_IN];
	RenderTargetMap[RTT_NAME_IN] = RenderTargetMap[RTT_NAME_OUT];
	RenderTargetMap[RTT_NAME_OUT] = tmp;
}
video::ITexture* CPostProcessManager::prepare()
{
	// Indique que cette frame n'a pas encore été affichée sur l'écran
	alreadyRendered = false;

	// set auxIn RTT as RenderTarget
	video::ITexture* const renderTarget = RenderTargetMap[RTT_NAME_IN];
	driver->setRenderTarget(renderTarget);
	return renderTarget;
}
void CPostProcessManager::render(const core::stringw& effect)
{
	if (effect.empty() || !screenQuad)
		return;

	// run through the effect chain
	const core::map<core::stringw, CEffectChain*>::Node* const effNode = EffectChain.find(effect);
	if (effNode)
	{
		const CEffectChain* const effChain = effNode->getValue();
		if (effChain)
		{
			if (effChain->getName() == EFFECT_NAME_FINAL)
				alreadyRendered = true;

			const u32 effectChainSize = effChain->size();
			if (effectChainSize)
			{
				for (u32 i = 0; i < effectChainSize; ++i)
				{
					// Obtient le shader permettant l'affichage de cet effet post-rendu
					CShaderMaterial* const shaderMaterial = (*effChain)[i];
					if (shaderMaterial)
					{
						// Assigne les textures de rendu cible du driver et les matériaux du shader
						shaderMaterial->setupDriverForRender();

						// Affiche l'écran de rendu avec les matériaux de ce shader
						screenQuad->render();
					}
				}

				// swap the in and out buffers
				if (effChain->getSwapBuffers())
					SwapAuxBuffers();
			}
		}
	}
}
void CPostProcessManager::update()
{
	// Affiche l'effet de rendu sur l'écran si il n'a pas déjà été affiché
	if (!alreadyRendered)
		render(EFFECT_NAME_FINAL);

	// Restaure toujours la cible de rendu du driver vers l'écran (sans l'effacer !), au cas où l'effet Render To Screen aurait été dévié pour afficher le résultat des effets post-rendu dans une texture RTT :
	// dans ce cas, la GUI serait alors elle-aussi déviée dans cette texture, et resterait invisible au joueur
	driver->setRenderTarget(video::ERT_FRAME_BUFFER, false, false);
}
void CPostProcessManager::clearDepthPass()
{
	// clear all nodes from the depth pass
	DepthPassNodes.clear();
}
void CPostProcessManager::addNodeToDepthPass(scene::ISceneNode *node)
{
	// Ajouté :
	if (!node)	return;

	// add node to the depth pass array
	if (DepthPassNodes.binary_search(node) == -1)
		DepthPassNodes.push_back(node);
}
void CPostProcessManager::removeNodeFromDepthPass(scene::ISceneNode *node)
{
	// Ajouté :
	if (!node)	return;

	// remove node from depth pass array
	const s32 index = DepthPassNodes.binary_search(node);
	if (index != -1) 
		DepthPassNodes.erase(index);
}
void CPostProcessManager::renderDepth(const video::SColor& defaultDepth)
{
	// Ajouté :
	if (!DepthMaterial)		return;

	const u32 depthPassNodesSize = DepthPassNodes.size();
	if (depthPassNodesSize)
	{
		// set depth render target texture as render target 
		driver->setRenderTarget(RenderTargetMap[RTT_NAME_DEPTH], true, true, defaultDepth);

		// Ajouté : Désactive le brouillard
		video::SOverrideMaterial& overrideMat = driver->getOverrideMaterial();
		overrideMat.EnableFlags |= video::EMF_FOG_ENABLE;
		overrideMat.Material.FogEnable = false;

		// animate and render the camera to ensure correct depth and normal information
		scene::ICameraSceneNode* const camera = device->getSceneManager()->getActiveCamera();
		if (camera)
		{
			// Désactivé : Mise à jour de la caméra inutile puisque le rendu de profondeur est effectué après le rendu de la scène : la caméra a déjà été mise à jour
			//camera->OnRegisterSceneNode(); 
			//camera->OnAnimate(Device->getTimer()->getTime()); 
			camera->render(); 
			DepthMaterial->setVertexShaderConstant("MaxDistanceInv", 1.0f / camera->getFarValue());
		}

		// render all nodes that are stored in the depth pass array
		for (u32 i = 0; i < depthPassNodesSize; ++i)
		{
			// get the scene node from the array
			scene::ISceneNode* const node = DepthPassNodes[i];

			if (node && node->isVisible())
			{
				// save the scene node material
				PP_SNodeMaterialBuffer matBuffer(node);

				// apply the depth material
				matBuffer.changeNodeMaterialType((video::E_MATERIAL_TYPE)DepthMaterial->getMaterialType());

				// render the node and its children
				matBuffer.renderNodeAndChildren();

				// reset the scene node to the original material
				matBuffer.restoreMaterialType();
			}
		}

		// Ajouté : Restaure le brouillard
		overrideMat.EnableFlags &= ~video::EMF_FOG_ENABLE;

		// Ajouté : Restaure le render target normal
		driver->setRenderTarget(RenderTargetMap[RTT_NAME_OUT], false, false);
	}
}
bool CPostProcessManager::needDepthPass(const core::stringw& effect)
{
	const core::map<core::stringw, CEffectChain*>::Node* const effNode = EffectChain.find(effect);
	if (effNode)
	{
		const CEffectChain* const effChain = effNode->getValue();
		if (effChain)
		{
			const u32 effectChainSize = effChain->size();
			if (effectChainSize)
			{
				for (u32 i = 0; i < effectChainSize; ++i)
				{
					CShaderMaterial* const shaderMaterial = (*effChain)[i];
					if (shaderMaterial && shaderMaterial->needDepthPass())
						return true;
				}
			}
		}
	}
	return false;
}
void CPostProcessManager::loadRTTConfig()
{
	// create a xml reader
	io::IXMLReader* const xmlReader = device->getFileSystem()->createXMLReader("PostProcessRTTConfig.xml");
	if (!xmlReader)
	{
		LOG("PostProcess: Warning : Could not load RTT config file \"PostProcessRTTConfig.xml\" !", ELL_WARNING);
		return;
	}

	// we'll be looking for the rendertarget tag in the xml
	const core::stringw renderTargetTag(L"RenderTarget");

    while (xmlReader->read())
    {    
		if (xmlReader->getNodeType() == io::EXN_ELEMENT)
		{
			// we are in the setup section and we find a rendertarget to parse
			if (renderTargetTag.equals_ignore_case(xmlReader->getNodeName()))
			{
				// get the rtt parameters
				const core::stringw id = xmlReader->getAttributeValueSafe(L"id");
				u32 width = (u32) xmlReader->getAttributeValueAsInt(L"width");
				u32 height = (u32) xmlReader->getAttributeValueAsInt(L"height");
				const f32 scale = (f32) xmlReader->getAttributeValueAsFloat(L"scale");
				const video::ECOLOR_FORMAT colorFormat = (video::ECOLOR_FORMAT) xmlReader->getAttributeValueAsInt(L"colorFormat");

				// set width and height of the rtt
				if (scale > 0.0f)
				{
					width =  (u32) (scale * driver->getScreenSize().Width);
					height =  (u32) (scale * driver->getScreenSize().Height);
				}
				if (width == 0)
					width=driver->getScreenSize().Width;
				if (height == 0)
					height=driver->getScreenSize().Height;

				// add the rendertarget with its properties and store it in the render target map
				RenderTargetMap[id] = NULL;
				RTTInfosMap[id] = RTTInfos(width, height, colorFormat);
			}
		}
	}
	delete xmlReader;    
}
void CPostProcessManager::loadEffectConfig()
{
	// create a xml reader
	io::IXMLReader* const xmlReader = device->getFileSystem()->createXMLReader("PostProcessEffectConfig.xml");
	if (!xmlReader)
	{
		LOG("PostProcess: Warning : Could not load effect config file \"PostProcessEffectConfig.xml\" !", ELL_WARNING);
		return;
	}

	// we'll be looking for these tags in the xml file
	const core::stringw effectTag(L"Effect");
	const core::stringw shaderPostProcessTag(L"ShaderPostProcess");
	const core::stringw renderSourceTag(L"RenderSource");
	const core::stringw renderTargetTag(L"RenderTarget");
	const core::stringw psConstant(L"PixelShaderConstant");
	const core::stringw vsConstant(L"VertexShaderConstant");
	const core::stringw textureTag(L"Texture");

	// each effect chain is a sequence of postprocesses
	CEffectChain* currentEffectChain = NULL;
	CShaderMaterial* currentShaderMaterial = NULL;

    while (xmlReader->read())
    {    
		if (xmlReader->getNodeType() == io::EXN_ELEMENT)
		{
			// we are in the effect section and we find a effect to parse
			const core::stringw nodeName = xmlReader->getNodeName();
			if (effectTag.equals_ignore_case(nodeName))
			{
				const core::stringw name = xmlReader->getAttributeValueSafe(L"effectName");
				const core::map<core::stringw, CEffectChain*>::Node* effNode = EffectChain.find(name);
				if (effNode)
					currentEffectChain = effNode->getValue();
				else
				{
					EffectChain[name] = currentEffectChain = new CEffectChain();
					currentEffectChain->setName(name);
				}
				currentShaderMaterial = NULL;

				if (currentEffectChain)
				{
					const bool swapBuffers = xmlReader->getAttributeValueAsInt(L"noBufferSwap") == 0;
					currentEffectChain->setSwapBuffers(swapBuffers);
				}
			}

			// we are in the shader post process section and have a valid currentEffect
			else if (currentEffectChain && shaderPostProcessTag.equals_ignore_case(nodeName))
			{
				// get the postprocess name
				const core::stringw name = xmlReader->getAttributeValueSafe(L"name");

				// get vertex shader config
				const core::stringw vsFile = xmlReader->getAttributeValueSafe(L"vsFile");
				core::stringw vsEntry = L"main";
				if (xmlReader->getAttributeValue(L"vsEntry"))
					vsEntry = xmlReader->getAttributeValueSafe(L"vsEntry");	
				const video::E_VERTEX_SHADER_TYPE vsType = (video::E_VERTEX_SHADER_TYPE) xmlReader->getAttributeValueAsInt(L"vsType");

				// get pixel shader config
				const core::stringw psFile = xmlReader->getAttributeValueSafe(L"psFile");
				core::stringw psEntry = L"main";
				if (xmlReader->getAttributeValue(L"psEntry"))
					psEntry = xmlReader->getAttributeValueSafe(L"psEntry");	
				const video::E_PIXEL_SHADER_TYPE psType = (video::E_PIXEL_SHADER_TYPE) xmlReader->getAttributeValueAsInt(L"psType");
				const video::E_MATERIAL_TYPE baseMaterial = (video::E_MATERIAL_TYPE) xmlReader->getAttributeValueAsInt(L"baseMaterial");

				// get additional built in shader constants for vertexshader
				const bool vsUseElapsedTime = xmlReader->getAttributeValueAsInt(L"vsUseElapsedTime") != 0;
				const bool vsUseRandom = xmlReader->getAttributeValueAsInt(L"vsUseRandom") != 0;
				const bool vsUseBufferWidth = xmlReader->getAttributeValueAsInt(L"vsUseBufferWidth") != 0;
				const bool vsUseBufferHeight = xmlReader->getAttributeValueAsInt(L"vsUseBufferHeight") != 0;
				const bool vsUseProjection = xmlReader->getAttributeValueAsInt(L"vsUseProjection") != 0;
				const bool vsUseView = xmlReader->getAttributeValueAsInt(L"vsUseView") != 0;
				const bool vsUseWorld = xmlReader->getAttributeValueAsInt(L"vsUseWorld") != 0;
				const bool vsUseWorldView = xmlReader->getAttributeValueAsInt(L"vsUseWorldView") != 0;
				const bool vsUseWorldViewProj = xmlReader->getAttributeValueAsInt(L"vsUseWorldViewProj") != 0;

				// get additional built in shader constants for pixelshader
				const bool psUseElapsedTime = xmlReader->getAttributeValueAsInt(L"psUseElapsedTime") != 0;
				const bool psUseRandom = xmlReader->getAttributeValueAsInt(L"psUseRandom") != 0;
				const bool psUseBufferWidth = xmlReader->getAttributeValueAsInt(L"psUseBufferWidth") != 0;
				const bool psUseBufferHeight = xmlReader->getAttributeValueAsInt(L"psUseBufferHeight") != 0;
				const bool psUseProjection = xmlReader->getAttributeValueAsInt(L"psUseProjection") != 0;
				const bool psUseView = xmlReader->getAttributeValueAsInt(L"psUseView") != 0;
				const bool psUseWorld = xmlReader->getAttributeValueAsInt(L"psUseWorld") != 0;
				const bool psUseWorldView = xmlReader->getAttributeValueAsInt(L"psUseWorldView") != 0;
				const bool psUseWorldViewProj = xmlReader->getAttributeValueAsInt(L"psUseWorldViewProj") != 0;

				// create a new shader post process material
				currentShaderMaterial = new CShaderMaterial(device, *this, maxMRTs, name, vsFile, vsEntry, vsType, psFile, psEntry, psType, baseMaterial);

				if (currentShaderMaterial)
				{
					// set pixel shader flags
					currentShaderMaterial->setPixelShaderFlag(ESC_TIME, psUseElapsedTime);
					currentShaderMaterial->setPixelShaderFlag(ESC_RANDOM, psUseRandom);
					currentShaderMaterial->setPixelShaderFlag(ESC_BUFFERWIDTH, psUseBufferWidth);
					currentShaderMaterial->setPixelShaderFlag(ESC_BUFFERHEIGHT, psUseBufferHeight);
					currentShaderMaterial->setPixelShaderFlag(ESC_PROJECTION, psUseProjection);
					currentShaderMaterial->setPixelShaderFlag(ESC_VIEW, psUseView);
					currentShaderMaterial->setPixelShaderFlag(ESC_WORLD, psUseWorld);
					currentShaderMaterial->setPixelShaderFlag(ESC_WORLDVIEW, psUseWorldView);
					currentShaderMaterial->setPixelShaderFlag(ESC_WORLDVIEWPROJ, psUseWorldViewProj);

					// set vertex shader flags
					currentShaderMaterial->setVertexShaderFlag(ESC_TIME, vsUseElapsedTime);
					currentShaderMaterial->setVertexShaderFlag(ESC_RANDOM, vsUseRandom);
					currentShaderMaterial->setVertexShaderFlag(ESC_BUFFERWIDTH, vsUseBufferWidth);
					currentShaderMaterial->setVertexShaderFlag(ESC_BUFFERHEIGHT, vsUseBufferHeight);
					currentShaderMaterial->setVertexShaderFlag(ESC_PROJECTION, vsUseProjection);
					currentShaderMaterial->setVertexShaderFlag(ESC_VIEW, vsUseView);
					currentShaderMaterial->setVertexShaderFlag(ESC_WORLD, vsUseWorld);
					currentShaderMaterial->setVertexShaderFlag(ESC_WORLDVIEW, vsUseWorldView);
					currentShaderMaterial->setVertexShaderFlag(ESC_WORLDVIEWPROJ, vsUseWorldViewProj);

					// push back the post process into the effect chain
					currentEffectChain->push_back(currentShaderMaterial);
				}
			}

			else if (currentShaderMaterial)
			{
				// read vertex shader constants from the xml-file
				if (vsConstant.equals_ignore_case(nodeName))
				{
					// add the defined constants to the postprocess
					const core::stringw name = xmlReader->getAttributeValueSafe(L"name");
					const f32 value = xmlReader->getAttributeValueAsFloat(L"value");
					currentShaderMaterial->setVertexShaderConstant(name, value);
				}

				// read pixel shader constants from the xml-file
				else if (psConstant.equals_ignore_case(nodeName))
				{
					// add the defined constants to the postprocess
					const core::stringw name = xmlReader->getAttributeValueSafe(L"name");
					const f32 value = xmlReader->getAttributeValueAsFloat(L"value");
					currentShaderMaterial->setPixelShaderConstant(name, value);
				}

				// read input texture properties from the xml-file
				else if (textureTag.equals_ignore_case(nodeName))
				{
					// read texture properties
					const u32 index = (u32) xmlReader->getAttributeValueAsInt(L"index");
					if (index < video::MATERIAL_MAX_TEXTURES)
					{
						const core::stringw texPath = xmlReader->getAttributeValueSafe(L"path");
						const core::stringw texName = xmlReader->getAttributeValueSafe(L"name");
						const video::E_TEXTURE_CLAMP texClamp = (video::E_TEXTURE_CLAMP)xmlReader->getAttributeValueAsInt(L"textureClamp");
						const s8 texLODBias = (s8) xmlReader->getAttributeValueAsInt(L"lodBias");
						bool bilinearFilter = true;
						bool trilinearFilter = false;
						bool anisotropicFilter = false;
						if (xmlReader->getAttributeValue(L"bilinearFilter"))
							bilinearFilter = xmlReader->getAttributeValueAsInt(L"bilinearFilter") != 0;
						if (xmlReader->getAttributeValue(L"trilinearFilter"))
							trilinearFilter = xmlReader->getAttributeValueAsInt(L"trilinearFilter") != 0;
						if (xmlReader->getAttributeValue(L"anisotropicFilter"))
							anisotropicFilter = xmlReader->getAttributeValueAsInt(L"anisotropicFilter") != 0;

						// set texture properties
						video::SMaterialLayer& currentShaderMaterialTextureLayer = currentShaderMaterial->getMaterial().TextureLayer[index];
						currentShaderMaterialTextureLayer.TextureWrapU = texClamp;
						currentShaderMaterialTextureLayer.TextureWrapV = texClamp;
						currentShaderMaterialTextureLayer.LODBias = texLODBias;
						currentShaderMaterialTextureLayer.BilinearFilter = bilinearFilter;
						currentShaderMaterialTextureLayer.TrilinearFilter = trilinearFilter;
						currentShaderMaterialTextureLayer.AnisotropicFilter = anisotropicFilter;
						
						// set texture name (used for glsl)
						if (!texName.empty())
							currentShaderMaterial->setTextureName(index, texName);
						if (!texPath.empty())
							currentShaderMaterial->setTexturePath(index, texPath);
					}
				}

				else if (currentEffectChain)
				{
					// read render target for the postprocess from the xml-file
					if (renderTargetTag.equals_ignore_case(nodeName))
					{
						for (u32 i = 0; i < maxMRTs; ++i)
						{
							swprintf_SS(L"path%u", i);

							// set render target of the postprocess
							const core::stringw texPath = xmlReader->getAttributeValueSafe(textW_SS);
							currentShaderMaterial->setTextureTarget(i, texPath);
						}
					}
				}
			}
		}
	}
	delete xmlReader;    
}
