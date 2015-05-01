// Copyright (C) 2002-2011 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h
// Code for this scene node has been contributed by Anders la Cour-Harbo (alc)

#include "CEcoWorldSkyDomeSceneNode.h"

/*	horiRes and vertRes:
	Controls the number of faces along the horizontal axis (30 is a good value)
	and the number of faces along the vertical axis (8 is a good value).

	texturePercentage:
	Only the top texturePercentage of the image is used, e.g. 0.8 uses the top 80% of the image,
	1.0 uses the entire image. This is useful as some landscape images have a small banner
	at the bottom that you don't want.

	spherePercentage:
	This controls how far around the sphere the sky dome goes. For value 1.0 you get exactly the upper
	hemisphere, for 1.1 you get slightly more, and for 2.0 you get a full sphere. It is sometimes useful
	to use a value slightly bigger than 1 to avoid a gap between some ground place and the sky. This
	parameters stretches the image to fit the chosen "sphere-size". */
CEcoWorldSkyDomeSceneNode::CEcoWorldSkyDomeSceneNode(video::E_MATERIAL_TYPE skyDomeMaterial, ISceneNode* parent, ISceneManager* mgr, s32 id,
	u32 horiRes, u32 vertRes, f32 texturePercentage, f32 spherePercentage, f32 radius)
 : ISceneNode(parent, mgr, id), Buffer(0), HorizontalResolution(horiRes), VerticalResolution(vertRes), SpherePercentage(spherePercentage), Radius(radius)
{
#ifdef _DEBUG
	setDebugName("CEcoWorldSkyDomeSceneNode");
#endif

	setAutomaticCulling(scene::EAC_OFF);

	Buffer = new SMeshBufferLightMap();
	Buffer->Material.MaterialType = skyDomeMaterial;
	Buffer->Material.Lighting = false;
	Buffer->Material.FogEnable = false;
	Buffer->Material.ZBuffer = video::ECFN_NEVER;
	Buffer->Material.ZWriteEnable = false;
	Buffer->Material.AntiAliasing = video::EAAM_OFF;
	Buffer->BoundingBox.MaxEdge.set(0,0,0);
	Buffer->BoundingBox.MinEdge.set(0,0,0);

	// regenerate the mesh
	generateMesh(texturePercentage);
}
CEcoWorldSkyDomeSceneNode::~CEcoWorldSkyDomeSceneNode()
{
	if (Buffer)
		Buffer->drop();
}
void CEcoWorldSkyDomeSceneNode::generateMesh(f32 TexturePercentage)
{
	Buffer->Vertices.clear();
	Buffer->Indices.clear();

	const f32 azimuth_step = (core::PI * 2.f) / (f32)HorizontalResolution;
	if (SpherePercentage < 0.f)
		SpherePercentage = -SpherePercentage;
	if (SpherePercentage > 2.f)
		SpherePercentage = 2.f;
	const f32 elevation_step = SpherePercentage * core::HALF_PI / (f32)VerticalResolution;

	Buffer->Vertices.reallocate( (HorizontalResolution + 1) * (VerticalResolution + 1) );
	Buffer->Indices.reallocate(3 * (2*VerticalResolution - 1) * HorizontalResolution);

	video::S3DVertex2TCoords vtx;
	vtx.Color.set(255,255,255,255);
	vtx.Normal.set(0.0f,-1.f,0.0f);

	const f32 tcV = TexturePercentage / VerticalResolution;
	f32 azimuth = 0.0f;
	for (u32 k = 0; k <= HorizontalResolution; ++k)
	{
		f32 elevation = core::HALF_PI;
		const f32 tcU = (f32)k / (f32)HorizontalResolution;
		const f32 sinA = sinf(azimuth);
		const f32 cosA = cosf(azimuth);
		for (u32 j = 0; j <= VerticalResolution; ++j)
		{
			const f32 cosEr = Radius * cosf(elevation);
			vtx.Pos.set(cosEr*sinA, Radius*sinf(elevation), cosEr*cosA);
			vtx.TCoords.set(tcU, j*tcV);
			vtx.TCoords2.set(tcU, j*tcV);

			vtx.Normal = -vtx.Pos;
			vtx.Normal.normalize();

			Buffer->Vertices.push_back(vtx);
			elevation -= elevation_step;
		}
		azimuth += azimuth_step;
	}

	for (u32 k = 0; k < HorizontalResolution; ++k)
	{
		Buffer->Indices.push_back(VerticalResolution + 2 + (VerticalResolution + 1)*k);
		Buffer->Indices.push_back(1 + (VerticalResolution + 1)*k);
		Buffer->Indices.push_back(0 + (VerticalResolution + 1)*k);

		for (u32 j = 1; j < VerticalResolution; ++j)
		{
			Buffer->Indices.push_back(VerticalResolution + 2 + (VerticalResolution + 1)*k + j);
			Buffer->Indices.push_back(1 + (VerticalResolution + 1)*k + j);
			Buffer->Indices.push_back(0 + (VerticalResolution + 1)*k + j);

			Buffer->Indices.push_back(VerticalResolution + 1 + (VerticalResolution + 1)*k + j);
			Buffer->Indices.push_back(VerticalResolution + 2 + (VerticalResolution + 1)*k + j);
			Buffer->Indices.push_back(0 + (VerticalResolution + 1)*k + j);
		}
	}
	Buffer->setHardwareMappingHint(scene::EHM_STATIC);
}
void CEcoWorldSkyDomeSceneNode::render()
{
	video::IVideoDriver* const driver = SceneManager->getVideoDriver();
	scene::ICameraSceneNode* const camera = SceneManager->getActiveCamera();
	if (!camera || !driver)
		return;

	// Désactivé : Vérification de la caméra perspective : Optimisation
	//if (!camera->isOrthogonal())
	//{
	core::matrix4 mat(AbsoluteTransformation);
	mat.setTranslation(camera->getAbsolutePosition());

	driver->setTransform(video::ETS_WORLD, mat);

	driver->setMaterial(Buffer->Material);
	driver->drawMeshBuffer(Buffer);
	//}

	// Désactivé : Mode débogage : Optimisation
/*
	// for debug purposes only:
	if ( DebugDataVisible )
	{
		video::SMaterial m;
		m.Lighting = false;
		driver->setMaterial(m);

		if ( DebugDataVisible & scene::EDS_NORMALS )
		{
			// draw normals
			const f32 debugNormalLength = SceneManager->getParameters()->getAttributeAsFloat(DEBUG_NORMAL_LENGTH);
			const video::SColor debugNormalColor = SceneManager->getParameters()->getAttributeAsColor(DEBUG_NORMAL_COLOR);
			driver->drawMeshBufferNormals(Buffer, debugNormalLength, debugNormalColor);
		}

		// show mesh
		if ( DebugDataVisible & scene::EDS_MESH_WIRE_OVERLAY )
		{
			m.Wireframe = true;
			driver->setMaterial(m);

			driver->drawMeshBuffer(Buffer);
		}
	}
*/
}
void CEcoWorldSkyDomeSceneNode::OnRegisterSceneNode()
{
	if (IsVisible)
		SceneManager->registerNodeForRendering(this, ESNRP_SKY_BOX);

	// Désactivé : Normalement, aucun enfant ne nous est assigné : Optimisation
	//ISceneNode::OnRegisterSceneNode();
}
void CEcoWorldSkyDomeSceneNode::setLastWeather(video::ITexture* texture, float texturePercentage, float textureOffset)
{
	// Ancien temps : Texture 0

	Buffer->Material.TextureLayer[0].Texture = texture;

	const f32 tcV = texturePercentage / VerticalResolution;
	for (u32 k = 0; k <= HorizontalResolution; ++k)
	{
		const f32 tcU = ((f32)k / (f32)HorizontalResolution) + textureOffset;
		for (u32 j = 0; j <= VerticalResolution; ++j)
			Buffer->Vertices[k * (VerticalResolution + 1) + j].TCoords.set(tcU, j * tcV);
	}
	Buffer->setDirty(EBT_VERTEX);
}
void CEcoWorldSkyDomeSceneNode::setNewWeather(video::ITexture* texture, float texturePercentage, float textureOffset)
{
	// Nouveau temps : Texture 1

	Buffer->Material.TextureLayer[1].Texture = texture;

	const f32 tcV = texturePercentage / VerticalResolution;
	for (u32 k = 0; k <= HorizontalResolution; ++k)
	{
		const f32 tcU = ((f32)k / (f32)HorizontalResolution) + textureOffset;
		for (u32 j = 0; j <= VerticalResolution; ++j)
			Buffer->Vertices[k * (VerticalResolution + 1) + j].TCoords2.set(tcU, j * tcV);
	}
	Buffer->setDirty(EBT_VERTEX);
}
