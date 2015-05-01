#include "CShaderMaterial.h"
#include "CShaderPreprocessor.h"
#include "CPostProcessManager.h"

CShaderMaterial::CShaderMaterial(IrrlichtDevice* device, CPostProcessManager& postProcessManager, u32 maxMRTs, const core::stringw& Name, const io::path& vs_file, const core::stringc& vs_entry_, video::E_VERTEX_SHADER_TYPE vs_type_,
	const io::path& ps_file, const core::stringc& ps_entry_, video::E_PIXEL_SHADER_TYPE ps_type_, video::E_MATERIAL_TYPE baseMaterial_)
	: driver(device->getVideoDriver()), timer(device->getTimer()), postProcess(postProcessManager), RenderTarget(maxMRTs), name(Name),
	PixelShaderFlags(0), VertexShaderFlags(0), vs_entry(vs_entry_), vs_type(vs_type_), ps_entry(ps_entry_), ps_type(ps_type_), baseMaterial(baseMaterial_)
{
	// Initialise le tableau des noms des textures
	for (u32 i = 0; i < maxMRTs; ++i)
		RenderTarget.push_back(core::stringw());

	// create destination path for (driver specific) shader files
	const core::stringc ppfx("ppfx_");
	vs_path = ppfx + vs_file;
	ps_path = ppfx + ps_file;
	if (driver->getDriverType() != video::EDT_OPENGL)
	{
		vs_path += core::stringc("_vs.hlsl");
		ps_path += core::stringc("_ps.hlsl");
	}
	else
	{
		vs_path += core::stringc("_vs.glsl");
		ps_path += core::stringc("_ps.glsl");
	}

	// set the default texturenames and texture wrap
	// these are overridden by the effect.xml configuration
	const core::stringc texStr("texture");
	for (u32 i = 0; i < video::MATERIAL_MAX_TEXTURES; ++i)
	{
		TextureName[i] = texStr + core::stringc(i);
		Material.TextureLayer[i].TextureWrapU = video::ETC_REPEAT;
		Material.TextureLayer[i].TextureWrapV = video::ETC_REPEAT;
	}

	// set material parameters
	Material.MaterialType = (video::E_MATERIAL_TYPE)-1;
	Material.Lighting = false;
	Material.BackfaceCulling = false;
	Material.ZBuffer = video::ECFN_NEVER;
}
void CShaderMaterial::compileShader()
{
	if (isCompiled())	return;

	LOG("PostProcess: Compiling shader material \"" << core::stringc(name).c_str() << "\" : Vertex shader \"" << vs_path.c_str() << "\" and pixel shader \"" << ps_path.c_str() << "\"", ELL_DEBUG);

	// create shader material
	Material.MaterialType = (video::E_MATERIAL_TYPE)postProcess.getShaderPreprocessor()->addHighLevelShaderMaterialFromFiles(
		vs_path, vs_entry.c_str(), vs_type, ps_path, ps_entry.c_str(), ps_type, this, baseMaterial, 0, false, false);

	// Textures :
	for (u32 i = 0; i < video::MATERIAL_MAX_TEXTURES; ++i)
	{
		const core::stringw& path = TexturePath[i];
		if (!path.empty())
		{
			// Détermine si cette texture est une texture RTT, puis charge la texture correspondante
			const bool isRTTTexture = postProcess.isRTTTexture(path);
			if (isRTTTexture)
				Material.TextureLayer[i].Texture = postProcess.getRTTTexture(path);
			else
				Material.TextureLayer[i].Texture = driver->getTexture(path);
		}
		else
			Material.TextureLayer[i].Texture = NULL;
	}

	// Crée aussi les textures de rendu final de ce shader
	const u32 renderTargetSize = RenderTarget.size();
	for (u32 i = 0; i < renderTargetSize; ++i)
	{
		const core::stringw& target = RenderTarget[i];
		if (!target.empty())
			postProcess.getRTTTexture(target);
	}
}
bool CShaderMaterial::needDepthPass() const
{
	// Détermine si une texture source est la texture RTT nommée "rttDepth"
	for (u32 i = 0; i < video::MATERIAL_MAX_TEXTURES; ++i)
	{
		const core::stringw& path = TexturePath[i];
		if (!path.empty())
		{
			// Vérifie que cette texture est bien nommée "rttDepth"
			if (path == RTT_NAME_DEPTH)
			{
				// Détermine si cette texture est bien une texture RTT
				if (postProcess.isRTTTexture(path))
					return true;	// Dans ce cas, cet effet post-rendu nécessite effectivement le rendu de la profondeur de la scène
			}
		}
	}

	// Aucune texture correspondante n'a pu être trouvée : cet effet post-rendu ne nécessite pas le rendu de la profondeur de la scène
	return false;
}
void CShaderMaterial::setupDriverForRender()
{
	// Met à jour les textures de ce matériau ayant pour nom "auxIn" et "auxOut" car leur pointeur est susceptible de varier (ils sont échangés à chaque fin d'effet)
	// Note : On ne permet pas à la fonction postProcess.getRTTTexture de créer la texture RTT correspondante si elle n'a été trouvée :
	// si c'est le cas, c'est que le driver n'a pas réussi à la créer la première fois, et ce ne serait qu'une perte de temps de réessayer
	for (u32 i = 0; i < video::MATERIAL_MAX_TEXTURES; ++i)
	{
		const core::stringw& path = TexturePath[i];
		if (path == RTT_NAME_IN)
			Material.TextureLayer[i].Texture = postProcess.getRTTTexture(RTT_NAME_IN, false);
		else if (path == RTT_NAME_OUT)
			Material.TextureLayer[i].Texture = postProcess.getRTTTexture(RTT_NAME_OUT, false);
	}

	// Crée la liste des textures de rendu cibles
	// Note : cette liste n'est pas stockable dans cette classe car elle n'est pas constante :
	// les textures auxIn et auxOut sont échangées en permanence, et c'est pourquoi cette liste doit être recréée avant chaque rendu
	const u32 renderTargetSize = RenderTarget.size();
	core::array<video::IRenderTarget> renderTargetTextures(renderTargetSize);
	for (u32 i = 0; i < renderTargetSize; ++i)
	{
		const core::stringw& target = RenderTarget[i];
		if (!target.empty())
		{
			video::ITexture* const texture = postProcess.getRTTTexture(target, false);
			if (texture)
				renderTargetTextures.push_back(video::IRenderTarget(texture));
		}
	}

	// Indique le matériau de ce shader au driver
	driver->setMaterial(Material);

	// Indique les textures de rendu cibles au driver
	const u32 renderTargetTexturesSize = renderTargetTextures.size();
	if (renderTargetTexturesSize > 0)
	{
		if (renderTargetTexturesSize > 1)
			driver->setRenderTarget(renderTargetTextures);					// Plusieurs textures de rendu cibles : indique au driver les textures cibles
		else
			driver->setRenderTarget(renderTargetTextures[0].RenderTexture);	// Une seule texture de rendu cible : indique au driver la texture cible
	}
	else
		driver->setRenderTarget(video::ERT_FRAME_BUFFER);					// Aucune texture de rendu cible : on affiche le résultat du rendu à l'écran
}
void CShaderMaterial::OnSetConstants(video::IMaterialRendererServices* services, s32 userdata)
{
	// set the constants for the pixelshader
	if (PixelShaderConstant.size())
	{
		core::map<core::stringc, f32>::Iterator psParamIter = PixelShaderConstant.getIterator();
		for(; !psParamIter.atEnd(); psParamIter++)
		{
			const f32 value = psParamIter.getNode()->getValue();
			services->setPixelShaderConstant(psParamIter.getNode()->getKey().c_str(), &value, 1);
		}
	}

	// set the constants for the vertexshader
	if (VertexShaderConstant.size())
	{
		core::map<core::stringc, f32>::Iterator vsParamIter = VertexShaderConstant.getIterator();
		for(; !vsParamIter.atEnd(); vsParamIter++)
		{
			const f32 value = vsParamIter.getNode()->getValue();
			services->setVertexShaderConstant(vsParamIter.getNode()->getKey().c_str(), &value, 1);
		}
	}

	// set the elapsed time if the shader wants it
	const bool pixelShaderFlagTime = getPixelShaderFlag(ESC_TIME);
	const bool vertexShaderFlagTime = getVertexShaderFlag(ESC_TIME);
	if (pixelShaderFlagTime || vertexShaderFlagTime)
	{
		const f32 elapsedTime = timer->getTime() * 0.001f;
		if (pixelShaderFlagTime)
			services->setPixelShaderConstant("ElapsedTime", &elapsedTime, 1);
		if (vertexShaderFlagTime)
			services->setVertexShaderConstant("ElapsedTime", &elapsedTime, 1);
	}

	// set a random value if the shader wants it
	const bool pixelShaderFlagRand = getPixelShaderFlag(ESC_RANDOM);
	const bool vertexShaderFlagRand = getVertexShaderFlag(ESC_RANDOM);
	if (pixelShaderFlagRand || vertexShaderFlagRand)
	{
		const f32 random = (f32)rand() / (f32)RAND_MAX;
		if (pixelShaderFlagRand)
			services->setPixelShaderConstant("RandomValue", &random, 1);
		if (vertexShaderFlagRand)
			services->setVertexShaderConstant("RandomValue", &random, 1);
	}

	// set the projection matrix if the shader wants it
	const core::matrix4 projMatrix = driver->getTransform(video::ETS_PROJECTION);
	if (getPixelShaderFlag(ESC_PROJECTION))
		services->setPixelShaderConstant("ProjMatrix", projMatrix.pointer(), 16);
	if (getVertexShaderFlag(ESC_PROJECTION))
		services->setVertexShaderConstant("ProjMatrix", projMatrix.pointer(), 16);

	// set the view matrix if the shader wants it
	const core::matrix4 viewMatrix = driver->getTransform(video::ETS_VIEW);
	if (getPixelShaderFlag(ESC_VIEW))
		services->setPixelShaderConstant("ViewMatrix", viewMatrix.pointer(), 16);
	if (getVertexShaderFlag(ESC_VIEW))
		services->setVertexShaderConstant("ViewMatrix", viewMatrix.pointer(), 16);

	// set the world matrix if the shader wants it
	const core::matrix4 worldMatrix = driver->getTransform(video::ETS_WORLD);
	if (getPixelShaderFlag(ESC_WORLD))
		services->setPixelShaderConstant("WorldMatrix", worldMatrix.pointer(), 16);
	if (getVertexShaderFlag(ESC_WORLD))
		services->setVertexShaderConstant("WorldMatrix", worldMatrix.pointer(), 16);

	// set the world view matrix if the shader wants it
	const bool pixelShaderFlagWorldViewMat = getPixelShaderFlag(ESC_WORLDVIEW);
	const bool vertexShaderFlagWorldViewMat = getVertexShaderFlag(ESC_WORLDVIEW);
	if (pixelShaderFlagWorldViewMat || vertexShaderFlagWorldViewMat)
	{
		core::matrix4 worldViewMatrix = viewMatrix;
		worldViewMatrix *= worldMatrix;
		if (pixelShaderFlagWorldViewMat)
			services->setPixelShaderConstant("WorldViewMatrix", worldViewMatrix.pointer(), 16);
		if (vertexShaderFlagWorldViewMat)
			services->setVertexShaderConstant("WorldViewMatrix", worldViewMatrix.pointer(), 16);
	}

	// set the world view projection matrix if the shader wants it
	const bool pixelShaderFlagWorldViewProjMat = getPixelShaderFlag(ESC_WORLDVIEWPROJ);
	const bool vertexShaderFlagWorldViewProjMat = getVertexShaderFlag(ESC_WORLDVIEWPROJ);
	if (pixelShaderFlagWorldViewProjMat || vertexShaderFlagWorldViewProjMat)
	{
		core::matrix4 worldViewProjMatrix = projMatrix;
		worldViewProjMatrix *= viewMatrix;
		worldViewProjMatrix *= worldMatrix;
		if (pixelShaderFlagWorldViewProjMat)
			services->setPixelShaderConstant("WorldViewProjMatrix", worldViewProjMatrix.pointer(), 16);
		if (vertexShaderFlagWorldViewProjMat)
			services->setVertexShaderConstant("WorldViewProjMatrix", worldViewProjMatrix.pointer(), 16);
	}

	// set buffer dimensions
	if (Material.TextureLayer[0].Texture)
	{
		const bool pixelShaderFlagBufferWidth = getPixelShaderFlag(ESC_BUFFERWIDTH);
		const bool vertexShaderFlagBufferWidth = getVertexShaderFlag(ESC_BUFFERWIDTH);
		const bool pixelShaderFlagBufferHeight = getPixelShaderFlag(ESC_BUFFERHEIGHT);
		const bool vertexShaderFlagBufferHeight = getVertexShaderFlag(ESC_BUFFERHEIGHT);
		const bool shaderFlagWidth = (pixelShaderFlagBufferWidth || vertexShaderFlagBufferWidth);
		const bool shaderFlagHeight = (pixelShaderFlagBufferHeight || vertexShaderFlagBufferHeight);		
		if (shaderFlagWidth || shaderFlagHeight)
		{
			const core::dimension2du& matTextureSize = Material.TextureLayer[0].Texture->getSize();

			// set buffer width if the shader wants it
			if (shaderFlagWidth)
			{
				const f32 width = (f32)matTextureSize.Width;
				if (pixelShaderFlagBufferWidth)
					services->setPixelShaderConstant("BufferWidth", &width, 1);
				if (vertexShaderFlagBufferWidth)
					services->setVertexShaderConstant("BufferWidth", &width, 1);
			}

			// set buffer height if the shader wants it
			if (shaderFlagHeight)
			{
				const f32 height = (f32)matTextureSize.Height;
				if (pixelShaderFlagBufferHeight)
					services->setPixelShaderConstant("BufferHeight", &height, 1);
				if (vertexShaderFlagBufferHeight)
					services->setVertexShaderConstant("BufferHeight", &height, 1);
			}
		}
	}

	if (driver->getDriverType() != video::EDT_OPENGL)
	{
		// set buffer size for DirectX vertex Shader (vertexshader performs 0.5 texel offset for correct texture sampling,  this is not necessary for OpenGL)
		setVertexShaderFlag(ESC_BUFFERWIDTH);
		setVertexShaderFlag(ESC_BUFFERHEIGHT);
	}
	else
	{
		// set texture names for OpenGL Shaders (this is not necessary for DirectX)
		for (u32 i = 0; i<video::MATERIAL_MAX_TEXTURES; ++i)
		{
			if (Material.TextureLayer[i].Texture)
				services->setPixelShaderConstant(TextureName[i].c_str(), (const f32*) &i, 1);
		}
	}
}
