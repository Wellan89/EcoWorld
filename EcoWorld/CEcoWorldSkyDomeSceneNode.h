#ifndef DEF_C_ECO_WORLD_SKY_DOME_SCENE_NODE
#define DEF_C_ECO_WORLD_SKY_DOME_SCENE_NODE

#include "global.h"

enum WeatherID;

using namespace scene;

// Scene Node permettant d'afficher le ciel (en skydome) d'EcoWorld : ce scene node gère automatiquement les shaders du ciel, si disponibles
// Pour son bon fonctionnement, ce node ne devrait avoir ni enfants, ni parent différent du root scene node du scene manager
// Cette classe est largement basée sur la classe CSkyDomeSceneNode d'Irrlicht SVN 1.8.0-alpha
class CEcoWorldSkyDomeSceneNode : public ISceneNode
{
public:
	// Constructeur et destructeur
	CEcoWorldSkyDomeSceneNode(video::E_MATERIAL_TYPE skyDomeMaterial, ISceneNode* parent, ISceneManager* smgr, s32 id = -1,
		u32 horiRes = 32, u32 vertRes = 16, f32 texturePercentage = 1.0f, f32 spherePercentage = 2.0f, f32 radius = 1000.0f);
	virtual ~CEcoWorldSkyDomeSceneNode();

	virtual void OnRegisterSceneNode();
	virtual void render();

	// Indique l'ancien ou le nouveau temps désigné par le Weather Manager
	void setLastWeather(video::ITexture* texture, float texturePercentage, float textureOffset);
	void setNewWeather(video::ITexture* texture, float texturePercentage, float textureOffset);

protected:
	void generateMesh(f32 TexturePercentage);
	SMeshBufferLightMap* Buffer;
	u32 HorizontalResolution, VerticalResolution;
	f32 SpherePercentage, Radius;

public:
	// Accesseurs inline :

	// Surcharges de scene::ISceneNode
	const core::aabbox3df& getBoundingBox() const	{ return Buffer->BoundingBox; }
	virtual video::SMaterial& getMaterial(u32 i)	{ return Buffer->Material; }
	virtual u32 getMaterialCount() const			{ return 1; }
	virtual ESCENE_NODE_TYPE getType() const		{ return ESNT_SKY_DOME; }
};

#endif

