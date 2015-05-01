#include "RealisticWater.h"
#include "CShaderPreprocessor.h"
#include "EcoWorldRenderer.h"
#include "Game.h"
#ifdef USE_SPARK
#include "SparkManager.h"
#endif

RealisticWaterSceneNode::RealisticWaterSceneNode(
#ifdef USE_SPARK
												 SparkManager& sparkManager,
#endif
												 f32 width, f32 height, video::ITexture* bumpTexture, bool animatedSurface,
												 core::dimension2du waterSubdivisions, float waveHeight, float waveSpeed, float waveLenght, 
												 core::dimension2du renderTargetSize, scene::ISceneNode* parent, s32 id
												 )
 :	scene::ISceneNode(parent, game->sceneManager, id),
#ifdef USE_SPARK
	sparkMgr(sparkManager),
#endif
	RefractionMap(0), ReflectionMap(0), shaderCallBack(0), ShaderMaterial(-1)
{
#ifdef _DEBUG
	// Ajouté
	setDebugName("RealisticWaterSceneNode");
#endif

	shaderCallBack = new RealisticWaterShaderCallBack();

	scene::ICameraSceneNode* const CurrentCamera = game->sceneManager->getActiveCamera(); //get current camera
	Camera = SceneManager->addCameraSceneNode(); //create new camera
	
	Camera->setNearValue(CurrentCamera->getNearValue());
    Camera->setFarValue(CurrentCamera->getFarValue());
	Camera->setFOV(CurrentCamera->getFOV());
	SceneManager->setActiveCamera(CurrentCamera); //set back the previous camera	

	if (!animatedSurface)
	{
		// Version de base : surface de l'eau plane
		scene::IAnimatedMesh* WaterMesh = game->sceneManager->addHillPlaneMesh("realisticwater", core::dimension2df(width, height), core::dimension2du(1, 1));
		WaterSceneNode = SceneManager->addMeshSceneNode(WaterMesh, this);

		// Supprime la mesh du cache du scene manager pour que la prochaine mesh d'eau (avec ce nom) qu'on ait besoin soit recréée, au cas où sa taille serait différente
		game->sceneManager->getMeshCache()->removeMesh(WaterMesh);
	}
	else
	{
		// Version modifiée : surface de l'eau animée par des vagues (visible seulement sur les coupures entre l'eau et le terrain)
		scene::IAnimatedMesh* WaterMesh = game->sceneManager->addHillPlaneMesh("realisticwater",
			core::dimension2df(width / (float)waterSubdivisions.Width, height / (float)waterSubdivisions.Height), waterSubdivisions);
		WaterSceneNode = SceneManager->addWaterSurfaceSceneNode(WaterMesh, waveHeight, waveSpeed, waveLenght, this);

		// Supprime la mesh du cache du scene manager pour que la prochaine mesh d'eau (avec ce nom) qu'on ait besoin soit recréée, au cas où sa taille serait différente
		game->sceneManager->getMeshCache()->removeMesh(WaterMesh);
	}

	if (game->driver->getDriverType() != video::EDT_OPENGL)
	{
		ShaderMaterial = game->shaderPreprocessor->addHighLevelShaderMaterialFromFiles(
			"Water_vs.hlsl", "main", video::EVST_VS_1_1,
			"Water_ps.hlsl", "main", video::EPST_PS_2_0,
			shaderCallBack, video::EMT_LIGHTMAP);
	}
	else
	{
		ShaderMaterial = game->shaderPreprocessor->addHighLevelShaderMaterialFromFiles(
			"Water_vs.glsl", "main", video::EVST_VS_1_1,
			"Water_ps.glsl", "main", video::EPST_PS_1_1,
			shaderCallBack, video::EMT_LIGHTMAP);
	}

	WaterSceneNode->setMaterialType((video::E_MATERIAL_TYPE)ShaderMaterial);

	WaterSceneNode->setMaterialTexture(0, bumpTexture);

	//const bool flagMipMaps = game->driver->getTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS);
	//const bool flag32Bit = game->driver->getTextureCreationFlag(video::ETCF_ALWAYS_32_BIT);
	//game->driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, false);
	//game->driver->setTextureCreationFlag(video::ETCF_ALWAYS_32_BIT, true);

	RefractionMap = game->driver->addRenderTargetTexture(renderTargetSize, "rw_1");
	ReflectionMap = game->driver->addRenderTargetTexture(renderTargetSize, "rw_2");

	//game->driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, flagMipMaps);
	//game->driver->setTextureCreationFlag(video::ETCF_ALWAYS_32_BIT, flag32Bit);

	WaterSceneNode->setMaterialTexture(1, RefractionMap);
	WaterSceneNode->setMaterialTexture(2, ReflectionMap);

	// Ajouté : Modifie les paramêtres de clamp des textures pour éviter qu'elles ne soient répétées :
	// évite qu'une réflexion du ciel ne se retrouve dans le coin opposé de l'écran par répétition des textures
	video::SMaterial& mat= WaterSceneNode->getMaterial(0);
	mat.TextureLayer[1].TextureWrapU = video::ETC_CLAMP;
	mat.TextureLayer[1].TextureWrapV = video::ETC_CLAMP;
	mat.TextureLayer[2].TextureWrapU = video::ETC_CLAMP;
	mat.TextureLayer[2].TextureWrapV = video::ETC_CLAMP;
}
RealisticWaterSceneNode::~RealisticWaterSceneNode()
{
	if (shaderCallBack)
		shaderCallBack->drop();
}
// frame
void RealisticWaterSceneNode::OnRegisterSceneNode()
{
	if (IsVisible)
		SceneManager->registerNodeForRendering(this, scene::ESNRP_TRANSPARENT_EFFECT);

	ISceneNode::OnRegisterSceneNode();
}
void RealisticWaterSceneNode::preRender(video::ITexture* screenRenderTarget)
{
	if (IsVisible)
	{
#ifdef USE_SPARK
		// Masque le système de particules de SPARK
		const bool lastParticleSystemVisible = sparkMgr.isParticleSystemVisible();
		sparkMgr.setParticleSystemVisible(false);
#endif

		// Masque les textes au-dessus des bâtiments
		const tribool lastNomsBatimentsVisible = game->renderer->getNomsBatimentsVisible();
		game->renderer->setNomsBatimentsVisible(false);

		setVisible(false); //hide the water

		//refraction
		game->driver->setRenderTarget(RefractionMap, true, true, video::SColor(0x0)); //render to refraction

		//core::plane3d<f32> refractionClipPlane(0,RelativeTranslation.Y+5,0, 0,-1,0); //refraction clip plane
		//game->driver->setClipPlane(0,refractionClipPlane,true);

		SceneManager->drawAll(); //draw the scene

		//game->driver->enableClipPlane(0,false);

		//reflection
		game->driver->setRenderTarget(ReflectionMap, true, true, video::SColor(0x0)); //render to reflection

		scene::ICameraSceneNode* const CurrentCamera = SceneManager->getActiveCamera(); //get current camera
        // Added by Christian Clavet to update the camera FOV (Zooming)
        Camera->setFarValue(CurrentCamera->getFarValue());
	    Camera->setFOV(CurrentCamera->getFOV());
	    // end 
		Camera->setNearValue(CurrentCamera->getNearValue());	// Ajouté
		Camera->setFarValue(CurrentCamera->getFarValue());		// Ajouté
		core::vector3df position = CurrentCamera->getPosition();
		position.Y = -position.Y + 2.0f * RelativeTranslation.Y; //position of the water
		Camera->setPosition(position);

		core::vector3df target = CurrentCamera->getTarget();
		target.Y = -target.Y + 2.0f * RelativeTranslation.Y;
		Camera->setTarget(target);

		SceneManager->setActiveCamera(Camera); //set the reflection camera

		//core::plane3d<f32> reflectionClipPlane(0,RelativeTranslation.Y,0, 0,1,0);
		//game->driver->setClipPlane(0,reflectionClipPlane,true);

		SceneManager->drawAll(); //draw the scene

		//game->driver->enableClipPlane(0,false);

		// set back old render target
		game->driver->setRenderTarget(screenRenderTarget, false, true, video::SColor(0x0));	// Modifié : Efface le Z-Buffer pour éviter des incohérences avec les rendus suivants lorsque l'antialiasing est désactivé

		SceneManager->setActiveCamera(CurrentCamera);

		setVisible(true); //show it again

		// Restaure les textes au-dessus des bâtiments
		game->renderer->setNomsBatimentsVisible(lastNomsBatimentsVisible);

#ifdef USE_SPARK
		// Restaure le système de particules de SPARK
		sparkMgr.setParticleSystemVisible(lastParticleSystemVisible);
#endif
	}
}
void RealisticWaterSceneNode::RealisticWaterShaderCallBack::OnSetConstants(video::IMaterialRendererServices* services, s32 userData)
{
	const scene::ICameraSceneNode* const Camera = game->sceneManager->getActiveCamera();

	const core::matrix4& projection = game->driver->getTransform(video::ETS_PROJECTION);
	const core::matrix4& view = game->driver->getTransform(video::ETS_VIEW);			
	const core::matrix4& world = game->driver->getTransform(video::ETS_WORLD);

	services->setVertexShaderConstant("View", view.pointer(), 16);

	core::matrix4 worldReflectionViewProj = projection;
	worldReflectionViewProj *= Camera->getViewMatrix();
	worldReflectionViewProj *= world;
	services->setVertexShaderConstant("WorldReflectionViewProj", worldReflectionViewProj.pointer(), 16);

	// Attention : valeur constante !
	const f32 WaveLengthInv = 10.0f;	// 1.0f / 0.1f
	services->setVertexShaderConstant("WaveLengthInv", &WaveLengthInv, 1);

	const f32 time = (game->deviceTimer->getTime() % 1000000) * 0.00001f;	// 1.0f / 100000.0f = 0.00001f
	const core::vector2df windTime = Wind * time;
	services->setVertexShaderConstant("WindTime", &windTime.X, 2);

	const core::vector3df& CameraPosition = Camera->getPosition();
	services->setPixelShaderConstant("CameraPosition", &CameraPosition.X, 3);

	services->setPixelShaderConstant("WaveHeight", &WaveHeight, 1);

	services->setPixelShaderConstant("WaterColor", &WaterColor.r, 4);

	services->setPixelShaderConstant("ColorBlendFactor", &ColorBlendFactor, 1);

	if (game->driver->getDriverType() != video::EDT_OPENGL)
	{
		core::matrix4 worldViewProj = projection;			
		worldViewProj *= view;
		worldViewProj *= world;
		services->setVertexShaderConstant("WorldViewProj", worldViewProj.pointer(), 16);
	}
	else
	{
		const int d[] = { 0, 1, 2 }; // sampler2d IDs
		services->setPixelShaderConstant("WaterBump", (float*)&d[0], 1);
		services->setPixelShaderConstant("RefractionMap", (float*)&d[1], 1);
		services->setPixelShaderConstant("ReflectionMap", (float*)&d[2], 1);
	}
}
