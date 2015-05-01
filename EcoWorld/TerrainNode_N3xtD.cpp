/***************************************************************
*                Terrain Engine issue of					   *
*							N3XTD engine					   *
*															   *
**************************************************************** 
*File Created: 29.01.11                                        *
*Author:			                                           *
*Contens: Implementation of TerrainNode source	               *
***************************************************************/

// Cette version a été modifiée

#include "TerrainNode_N3xtD.h"



namespace irr
{
namespace scene
{

	CTerrainSceneNode_N3xtD::CTerrainSceneNode_N3xtD(video::IImage* heightMap, TerrainQuality Quality, const core::vector3df& pos, const core::vector3df &terrainScale, f32 ScaleTexture, bool smooth, scene::ISceneNode* parent,scene::ISceneManager* smgr,s32 id)
: scene::ISceneNode(parent, smgr, id)
{
	mc=NULL;
	SizeOfTiles = 0;
	terrainPosition = pos;
	TerrainScale = terrainScale;
	mHeightmap.LoadHeightMap(smgr, heightMap, (terrainScale.Y == 0.0f ? 1.0f : terrainScale.Y));
	Create(Quality, ScaleTexture, smooth, parent, smgr, id);
	//setAutomaticCulling(scene::EAC_FRUSTUM_BOX);
	setAutomaticCulling(scene::EAC_OFF);
	dirLight=NULL;
}

CTerrainSceneNode_N3xtD::CTerrainSceneNode_N3xtD(int mSize, int seed, float noiseSize, float persistence, int octaves,
				   TerrainQuality Quality, const core::vector3df &terrainScale, f32 ScaleTexture, bool smooth,scene::ISceneNode* parent,scene::ISceneManager* smgr,s32 id)
: scene::ISceneNode(parent, smgr, id)
{
	mc=NULL;
	SizeOfTiles = 0;
	TerrainScale = terrainScale;
	mHeightmap.CreateRandomHeightMap(mSize,  seed, noiseSize, persistence, octaves);
	mHeightmap.LoadHeightMap(smgr, "", (terrainScale.Y == 0.0f ? 1.0f : terrainScale.Y) );
	Create(Quality, ScaleTexture, smooth, parent, smgr, id);
	//this->setAutomaticCulling(scene::EAC_FRUSTUM_BOX);
	setAutomaticCulling(scene::EAC_OFF);
	dirLight=NULL;
}



void CTerrainSceneNode_N3xtD::Create(TerrainQuality quality, f32 scaletexture, bool smooth, scene::ISceneNode* parent,scene::ISceneManager* smgr,s32 id)
{

    //Test if the number of quality is correct
    if(Quality != 1 && Quality != 2 && Quality != 4 && Quality != 8)
    {
        //if not force to medium quality
        Quality = Medium;
    }

    //Set the Debug to false
    //Debug=false;

	Quality = quality;
	ScaleTexture = scaletexture;
    //Get the Heightmap
	 if(smooth) mHeightmap.SmoothMap();

    //Get dimension of heightmap
	const u16 Size = mHeightmap.m_iWidth;	//Heightmap->getDimension().Width;

    //Set the size of Tiles for Terrain
    SizeOfTiles = 0;

    //Switch Size for calculate the Size of Tile
    switch (Size)	// Modifié pour gérer des tailles de 2^n et de 2^n+1
    {
        case 64:
        case 65:
            SizeOfTiles=(Size/4)+1;
            NbsTiles = 4*4;
            break;
        case 128:
		case 129:
            SizeOfTiles=(Size/8)+1;
            NbsTiles = 8*8;
            break;
        case 256:
        case 257:
			SizeOfTiles=(Size/16)+1;
            NbsTiles = 16*16;
            break;
        case 512:
        case 513:
            SizeOfTiles=(Size/16)+1;
            NbsTiles = 16*16;
            break;
        case 768:
        case 769:
            SizeOfTiles=(Size/24)+1;
            NbsTiles = 24*24;
            break;
        case 1024:
        case 1025:
            SizeOfTiles=(Size/32)+1;
            NbsTiles = 32*32;
            break;
        case 2048:
        case 2049:
            SizeOfTiles=(Size/32)+1;
            NbsTiles = 32*32;
            break;
        default:
            SizeOfTiles=(Size/16)+1;
            NbsTiles = 16*16;
            break;
    }

    //Create the Mesh for the Terrain Mesh
    TerrainMesh = new scene::SMesh();

    //Calculate the quality factor
    SOTiles = irr::core::ceil32((f32)SizeOfTiles/(f32)Quality);

    //Init the array of MeshBuffer
    CTTileBuffer=new scene::SMeshBufferLightMap* [NbsTiles];
    for (u32 i =0;i < NbsTiles;++i) CTTileBuffer[i]=NULL;

	// Ajouté : Initialise le tableau des triangle selectors
	metaTriSelector = smgr->createMetaTriangleSelector();
	TriangleSelectors = new scene::ITriangleSelector* [NbsTiles];
	for (u16 i = 0; i < NbsTiles; ++i)
		TriangleSelectors[i] = NULL;

	// Ajouté : indique les materiaux par défaut de ce node
	Material.Lighting = true;	// false
    Material.BackfaceCulling = true;
    Material.GouraudShading = true;
    Material.FogEnable = true;	// false

	setGeometry(true);
}


void CTerrainSceneNode_N3xtD::createTriangleSelectors()
{
	metaTriSelector->removeAllTriangleSelectors();
	for (u16 i = 0; i < NbsTiles; ++i)
	{
		scene::SMesh mesh;
		mesh.addMeshBuffer(CTTileBuffer[i]);

		if (!TriangleSelectors[i])
			TriangleSelectors[i] = SceneManager->createOctreeTriangleSelector(&mesh, this);//SceneManager->createTriangleSelector(&mesh, this)
		metaTriSelector->addTriangleSelector(TriangleSelectors[i]);
	}
}
void CTerrainSceneNode_N3xtD::setGeometry(bool calcNormal)
{
	const u16 Size = mHeightmap.m_iWidth;

    //Start the loop to create Buffers
    u32 TileX=0,TileZ=0;
    const f32 tdSize = 1.0f/(f32)(Size-1);
    for (u32 i =0;i < NbsTiles;++i)
    {
        if (!CTTileBuffer[i]) 
		{
			CTTileBuffer[i]=new scene::SMeshBufferLightMap();
			CTTileBuffer[i]->Vertices.set_used(SizeOfTiles*SizeOfTiles);
			CTTileBuffer[i]->Indices.set_used(SizeOfTiles*SizeOfTiles*6);
		}
        u32 Index=0;
        u16 NbsIndices=0,NbsVertices=0;

        for(u32 x=TileX;x<(TileX+SizeOfTiles);x+=Quality)
        {
            for (u32 z=TileZ;z<(TileZ+SizeOfTiles);z+=Quality)
            {
                if (NbsVertices < (SOTiles*SOTiles)-SOTiles-1)
                {
                    Index = NbsVertices;
                    u32 TestValue = ( (((x-TileX)/Quality)+1) * ((z-TileZ)/Quality) + ((x-TileX)/Quality) );
                    if (Index != TestValue || (x-TileX==0 && z < TileZ+SizeOfTiles-Quality))
                    {
                        CTTileBuffer[i]->Indices[NbsIndices++]=Index;
                        CTTileBuffer[i]->Indices[NbsIndices++]=Index+1;
                        CTTileBuffer[i]->Indices[NbsIndices++]=Index+SOTiles+1;
                        CTTileBuffer[i]->Indices[NbsIndices++]=Index;
                        CTTileBuffer[i]->Indices[NbsIndices++]=Index+SOTiles+1;
                        CTTileBuffer[i]->Indices[NbsIndices++]=Index+SOTiles;
                    }
                }
                /*video::S3DVertex2TCoords Vertex;
                Vertex.Normal.set(0,1,0);
                Vertex.Pos.set((f32)x*TerrainScale.X, mHeightmap.GetHeightValue((int)x,(int)z), (f32)z*TerrainScale.Z);
                Vertex.TCoords.set( (f32)x*tdSize, (f32)z*tdSize);
                Vertex.TCoords2.set( (f32)x*tdSize, (f32)z*tdSize)*ScaleTexture;*/

				// Modifié : Pris de CTerrain_paged.cpp et adapté : Permet de tourner le terrain de 180°
				video::S3DVertex2TCoords Vertex;
                Vertex.Normal.set(0,1,0);
				Vertex.Pos.set((f32)x*TerrainScale.X + terrainPosition.X, mHeightmap.GetHeightValue((int)x,(int)(Size-1-z)) + terrainPosition.Y, (f32)z*TerrainScale.Z + terrainPosition.Z);
                Vertex.TCoords.set((f32)x*tdSize, (f32)(Size-1-z)*tdSize);
                Vertex.TCoords2.set((f32)x*tdSize*ScaleTexture, (f32)(Size-1-z)*tdSize*ScaleTexture);	
				Vertex.Color.set(255, 255, 255, 255);

				CTTileBuffer[i]->Vertices[NbsVertices]=Vertex;

                NbsVertices++;
            }
        }

		/*CTTileBuffer[i]->Material.Lighting = false;

        CTTileBuffer[i]->Material.BackfaceCulling = true;
        CTTileBuffer[i]->Material.GouraudShading=true;
        CTTileBuffer[i]->Material.FogEnable=false;*/

        CTTileBuffer[i]->Vertices.set_used(NbsVertices);
        CTTileBuffer[i]->Indices.set_used(NbsIndices);

        //Calculate the normals
        if (calcNormal) calculateNormals(CTTileBuffer[i], SOTiles);

        //Recalculate the bounding box
        CTTileBuffer[i]->recalculateBoundingBox();

		// Ajouté : Permet d'activer les VBO sous OpenGL
		CTTileBuffer[i]->setHardwareMappingHint(scene::EHM_STATIC);

        //Add the buffer to the Terrain Mesh
        TerrainMesh->addMeshBuffer(CTTileBuffer[i]);

        TileX+=SizeOfTiles-1;
        if (TileX >= Size - (u32)1)	// Modifié (tailles 2^n) : "if (TileX >= Size)" (cette version fonctionne à la fois en tailles 2^n et 2^n+1)
        {
            TileX=0;

			TileZ+=SizeOfTiles-1;
            if (TileZ >= Size - (u32)1)	// Modifié (tailles 2^n) : "if (TileX >= Size)" (cette version fonctionne à la fois en tailles 2^n et 2^n+1)
            {
                TileZ=0;
            }
        }
    }

	calculateBoundingBox();
}



CTerrainSceneNode_N3xtD::~CTerrainSceneNode_N3xtD()
{
	// Ajouté :
	metaTriSelector->drop();
	for (u16 i = 0; i < NbsTiles; ++i)
	{
		if (TriangleSelectors[i])
			TriangleSelectors[i]->drop();

        if (CTerrainSceneNode_N3xtD::CTTileBuffer[i])
            CTerrainSceneNode_N3xtD::CTTileBuffer[i]->drop();
	}
	delete[] TriangleSelectors;
    
    delete[] CTerrainSceneNode_N3xtD::CTTileBuffer;

    if (CTerrainSceneNode_N3xtD::TerrainMesh)
        CTerrainSceneNode_N3xtD::TerrainMesh->drop();
}

void CTerrainSceneNode_N3xtD::OnRegisterSceneNode()
{
    if (IsVisible)
        SceneManager->registerNodeForRendering(this);

    ISceneNode::OnRegisterSceneNode();
}





void CTerrainSceneNode_N3xtD::render()
{
	const scene::ICameraSceneNode* const cam = SceneManager->getActiveCamera();
    //cam->updateAbsolutePosition();
    scene::SViewFrustum frustum = *(cam->getViewFrustum());
    video::IVideoDriver* const Driver = SceneManager->getVideoDriver();

	//transform the frustum to the node's current absolute transformation
	const core::matrix4 invTrans(AbsoluteTransformation, core::matrix4::EM4CONST_INVERSE);	// Ces lignes proviennent de testFRUSTUM_BOX, elles ont été placées ici ce qui a grandement diminué le temps de calcul de cette fonction
	frustum.transform(invTrans);

    /*if(Debug)
    {
        Driver->setTransform(video::ETS_WORLD,AbsoluteTransformation);
            video::SMaterial Mat;
            Mat.AmbientColor = video::SColor(255, 255, 255, 255);
            Mat.DiffuseColor = video::SColor(255, 255, 255, 255);
            Mat.EmissiveColor = video::SColor(255,0,255,0);
            Driver->setMaterial(Mat);
            Driver->draw3DBox(getBoundingBox(),video::SColor(255, 255, 255, 255));
    }*/

    Driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);
    Driver->setMaterial(Material);	// Optimisation
    for (u32 i=0;i<NbsTiles;++i)
    {
        //if (CTTileBuffer[i])	// Optimisation
        {
            //DEBUG HERE
            /*if(Debug)
            {
                video::SMaterial Mat;
                Mat.AmbientColor = video::SColor(255, 255, 255, 255);
                Mat.DiffuseColor = video::SColor(255, 255, 255, 255);
                Mat.EmissiveColor = video::SColor(255,0,255,0);
                Driver->setMaterial(Mat);
                Driver->draw3DBox(CTTileBuffer[i]->getBoundingBox(),video::SColor(255, 255, 255, 255));
            }*/
            if (!testFRUSTUM_BOX(frustum, CTTileBuffer[i]->getBoundingBox()))
            {
				//Driver->setMaterial(video::SMaterial(CTTileBuffer[i]->Material));
				//Driver->drawIndexedTriangleList(&CTTileBuffer[i]->Vertices[0],CTTileBuffer[i]->getVertexCount(),&CTTileBuffer[i]->Indices[0], CTTileBuffer[i]->getIndexCount()/3 );
				Driver->drawMeshBuffer(CTTileBuffer[i]);	// Permet d'utiliser les VBO pour OpenGL
			}
        }
    }
}

void CTerrainSceneNode_N3xtD::setPosition(const core::vector3df &Pos)
{
	terrainPosition = Pos;
    for (u32 i=0;i<NbsTiles;++i)
    {
        if (CTTileBuffer[i])
        {
            for (u32 j=0;j<CTTileBuffer[i]->getVertexCount();++j)
            {
                CTTileBuffer[i]->Vertices[j].Pos+=Pos;
            }
            CTTileBuffer[i]->recalculateBoundingBox();
        }
    }
	calculateBoundingBox();
}


void CTerrainSceneNode_N3xtD::calculateBoundingBox()
{
	// Recalcule rapidement la bounding box globale du terrain (la seule valeur fausse se trouve en Y)
	Box.MinEdge = CTTileBuffer[0]->getBoundingBox().MinEdge;
	Box.MaxEdge = CTTileBuffer[NbsTiles-1]->getBoundingBox().MaxEdge;

	/*Box.MinEdge = core::vector3df(9999999,9999999,9999999);
	Box.MaxEdge = core::vector3df(-9999999,-9999999,-9999999);
    for (u32 i=0;i<NbsTiles;++i)
    {
        if (CTTileBuffer[i])
        {
            const core::aabbox3df& tbox = CTTileBuffer[i]->getBoundingBox();
			if(tbox.MaxEdge.X>Box.MaxEdge.X) Box.MaxEdge.X = tbox.MaxEdge.X;
			if(tbox.MaxEdge.Y>Box.MaxEdge.Y) Box.MaxEdge.Y = tbox.MaxEdge.Y;
			if(tbox.MaxEdge.Z>Box.MaxEdge.Z) Box.MaxEdge.Z = tbox.MaxEdge.Z;
			if(tbox.MinEdge.X<Box.MinEdge.X) Box.MinEdge.X = tbox.MinEdge.X;
			if(tbox.MinEdge.Y<Box.MinEdge.Y) Box.MinEdge.Y = tbox.MinEdge.Y;
			if(tbox.MinEdge.Z<Box.MinEdge.Z) Box.MinEdge.Z = tbox.MinEdge.Z;
        }
    }*/
}


void CTerrainSceneNode_N3xtD::setScale(const core::vector3df &Scale)
{
    for (u32 i=0;i<NbsTiles;++i)
    {
        if (CTTileBuffer[i])
        {
            for (u32 j=0;j<CTTileBuffer[i]->getVertexCount();++j)
            {
                CTTileBuffer[i]->Vertices[j].Pos*=Scale;
            }
            CTTileBuffer[i]->recalculateBoundingBox();
        }
    }
	calculateBoundingBox();
}

void CTerrainSceneNode_N3xtD::ActivateSplattingTextures(//scene::ISceneManager* smgr, bool LightDefine, f32 Fog,video::SColorf FogColor
	)
{
    video::IVideoDriver* driver = SceneManager->getVideoDriver();
    int newMaterialType1 = -1;
    video::IGPUProgrammingServices* gpu = driver->getGPUProgrammingServices();

	if (!gpu)
		return;

	// Les shaders ont été largement modifiés et sont maintenant disponibles dans des fichiers externes

#if 0
	// Ancien : Shaders compilés :
    if (driver->getDriverType() != video::EDT_OPENGL)
    {
		core::stringc cf;
		cf += "float4x4 mWorldViewProj;\n";
	//if(LightDefine)
		//cf += "float3 dirToLight;\n";
		//cf += "float3 scaletex;\n";
	/*if(Fog>0)
	{
		cf += "float4x4 mWorld;\n";
		cf += "float3 cameraPos;\n";
		cf += "float FogDistance;\n";
	}*/
		//cf += "float3 lightPos;\n";

		cf += "struct VS_OUTPUT\n";
		cf += "{\n";
		cf += "float4 Position : POSITION;\n";
		cf += "float4 Diffuse : COLOR0;\n";
		cf += "float2 TexCoord0 : TEXCOORD0;\n";
		cf += "float2 TexCoord1 : TEXCOORD1;\n" ;
		//cf += "float  shade : TEXCOORD2;\n";
		//cf += "float2  detail : TEXCOORD3;\n";
		cf += "};\n";

		cf += "VS_OUTPUT vertexMain( in float4 vPosition : POSITION,in float3 vNormal : NORMAL, \n";
		cf += "float2 texCoord0 : TEXCOORD0, float2 texCoord1 : TEXCOORD1)\n";
		cf += "{\n";
		cf += "VS_OUTPUT Output;\n";
		cf += "Output.Position = mul(vPosition, mWorldViewProj);\n";

	/*if(Fog>0)
	{
		cf += "float4 position = mul(vPosition, mWorld);\n";
		cf += "float dist = sqrt ( pow(position.x - cameraPos.x, 2.0f) +\n";
		cf += "pow(position.y - cameraPos.y, 2.0f) +\n";
		cf += "pow(position.z - cameraPos.z, 2.0f));\n";
		cf += "Output.Diffuse = float4(vPosition.y , dist / FogDistance, 0.0f, 0.0f);\n";
	}
	else*/
		cf += "Output.Diffuse = float4(vPosition.y , 0.0f, 0.0f, 0.0f);\n";

	/*if(LightDefine)	
	{
		cf +="Output.shade = max(0.0f, dot(normalize(vNormal), dirToLight));\n";
		cf +="Output.shade = 0.2f + Output.shade * 0.8f;\n";
	}
	else*/
		//cf +="Output.shade = 0.9f;\n";

		cf += "Output.TexCoord0 = texCoord0;\n";
		cf += "Output.TexCoord1 = texCoord1;\n";
		//cf += "Output.detail = texCoord1 * scaletex;\n";
		cf += "return Output;\n";
		cf += "}\n";


		cf += "struct PS_OUTPUT\n";
		cf += "{\n";
		cf += "float4 RGBColor : COLOR0;\n";
		cf += "};\n";

		//cf += "sampler2D tex[8];\n";
		cf += "sampler2D tex[4];\n";	// Ajouté
		//if(Fog>0)	cf += "float4 fog;\n";
		//cf += "float  diffusePower;\n";

		//cf += "PS_OUTPUT pixelMain(float2 TexCoord0 : TEXCOORD0, float2 TexCoord1 : TEXCOORD1, float shade : TEXCOORD2, float2 detail : TEXCOORD3, float4 Diffuse : COLOR0)\n";
		cf += "PS_OUTPUT pixelMain(float2 TexCoord0 : TEXCOORD0, float2 TexCoord1 : TEXCOORD1, float4 Diffuse : COLOR0)\n";	// Ajouté
		cf += "{\n";
		cf += "PS_OUTPUT Output;\n";

		cf += "float4 dcol = {0.5f, 0.5f, 0.5f, 0.5f};\n";	// Ajouté

		//cf += "float heightpercent = Diffuse.x;\n";
		//cf += "float dist = Diffuse.y;\n";

		//cf += "float3 a  = tex2D(tex[0], TexCoord0).rgb;\n";
		cf += "float4 a  = tex2D(tex[0], TexCoord0);\n";
		cf += "float4 c1 = tex2D(tex[1], TexCoord1);\n";
		cf += "float4 c2 = tex2D(tex[2], TexCoord1);\n";
		cf += "float4 d  = tex2D(tex[3], TexCoord1);\n";	// Ajouté
		//cf += "float4 c3 = tex2D(tex[3], TexCoord1);\n";
		//cf += "float4 c4 = tex2D(tex[4], TexCoord1);\n";
		//cf += "float4 d  = tex2D(tex[5], detail);\n";
		//cf += "float4 l  = tex2D(tex[6], TexCoord0);\n";

		//cf += "float4 texColor = lerp(c1, c2, a.g);\n";
		//cf += "texColor = lerp(texColor, c3, a.b);\n";
		//cf += "texColor = lerp(texColor, c4, a.r);\n";
		//cf += "Output.RGBColor = d * texColor * shade * l;\n";
		//cf += "Output.RGBColor *= 1.5f;\n";
		cf += "Output.RGBColor = lerp(c1, c2, 0.3f*a.r + 0.59f*a.g + 0.11f*a.b);\n";	// Ajouté
		cf += "Output.RGBColor += d - dcol;\n";	// Ajouté

	/*if(Fog>0)
	{
		cf += "if (dist >= 0.5f)\n";
		cf += "{\n";
		cf += "Output.RGBColor *= (1.0f - (dist-0.5f));\n";
		cf += "Output.RGBColor += (fog  * (dist-0.5f));\n";
		cf += "}\n";
	}*/
		//cf += "Output.RGBColor *= diffusePower;\n";

		cf += "return Output;\n" ;
		cf += "}\n";

        mc = new MyShaderCallBack();
        //mc->setSplatScene(//SceneManager,Fog,FogColor
		//	);
        newMaterialType1 = gpu->addHighLevelShaderMaterial(
			cf.c_str(), "vertexMain", video::EVST_VS_2_0,
            cf.c_str(), "pixelMain", video::EPST_PS_2_0,
            mc, video::EMT_SOLID,0);
        mc->drop();
		//if(LightDefine)	mc->lightEnable = true;
    }
    else
    {
		core::stringc cf;//Shaderpixel;
		cf += "//\n";
		cf += "// Structure definitions\n";
		cf += "//\n";
		cf += "struct VS_OUTPUT {\n";
		cf += "vec4 Position;\n";
		cf += "vec4 Diffuse;\n";
		cf += "vec2 TexCoord0;\n";
		cf += "vec2 TexCoord1;\n";
		//cf += "vec2 shade;\n";
		//cf += "vec2 detail;\n";
		cf += "};\n";

		cf += "struct PS_OUTPUT {\n";
		cf += "vec4 RGBColor;\n";
		cf += "};\n";


		cf += "//\n";
		cf += "// Global variable definitions\n";
		cf += "//\n";

	//if(Fog>0)	cf += "uniform vec4 fog;\n";
		//cf += "uniform float diffusePower;\n";
		cf += "uniform sampler2D tex0;\n";
		cf += "uniform sampler2D tex1;\n";
		cf += "uniform sampler2D tex2;\n";
		cf += "uniform sampler2D tex3;\n";
		//cf += "uniform sampler2D tex4;\n";
		//cf += "uniform sampler2D tex5;\n";
		//cf += "uniform sampler2D tex6;\n";


		cf += "//\n";
		cf += "// Function declarations\n";
		cf += "//\n";
		//cf += "PS_OUTPUT pixelMain( in vec2 TexCoord0, in vec2 TexCoord1, in vec2 shade, in vec2 detail, in vec4 Diffuse  );\n";
		cf += "PS_OUTPUT pixelMain( in vec2 TexCoord0, in vec2 TexCoord1, in vec4 Diffuse  );\n";	// Ajouté
		cf += "//\n";
		cf += "// Function definitions\n";
		cf += "//\n";
		//cf += "PS_OUTPUT pixelMain( in vec2 TexCoord0, in vec2 TexCoord1, in vec2 shade, in vec2 detail, in vec4 Diffuse ) {\n";
		cf += "PS_OUTPUT pixelMain( in vec2 TexCoord0, in vec2 TexCoord1, in vec4 Diffuse ) {\n";	// Ajouté
		cf += "vec4 dcol;";	// Ajouté
		cf += "float heightpercent;\n";
		cf += "float dist;\n";
		//cf += "vec3 a;\n";
		cf += "vec4 a;\n";		// Ajouté
		cf += "vec4 c1;\n";
		cf += "vec4 c2;\n";
		//cf += "vec4 c3;\n";
		//cf += "vec4 c4;\n";
		cf += "vec4 d;\n";
		//cf += "vec4 l;\n";
		//cf += "vec4 texColor;\n";
		cf += "PS_OUTPUT Output;\n";
		cf += "dcol = vec4(0.5, 0.5, 0.5, 0.5);";	// Ajouté
		cf += "heightpercent = Diffuse.x ;\n";
		cf += "dist = Diffuse.y ;\n";
		//cf += "a = texture2D(  tex0, TexCoord0).rgb;\n";
		cf += "a = texture2D(  tex0, TexCoord0);\n";	// Ajouté
		cf += "c1 = texture2D( tex1, TexCoord1);\n";
		cf += "c2 = texture2D( tex2, TexCoord1);\n";
		cf += "d = texture2D(  tex3, TexCoord1);\n";	// Ajouté
		//cf += "c3 = texture2D( tex3, TexCoord1);\n";
		//cf += "c4 = texture2D( tex4, TexCoord1);\n";
		//cf += "d = texture2D(  tex5, detail);\n";
		//cf += "l = texture2D(  tex6, TexCoord0);\n";
		//cf += "texColor = mix( c1, c2, a.g);\n";
		//cf += "texColor = mix( texColor, c3, a.b);\n";
		//cf += "texColor = mix( texColor, c4, a.r);\n";

		//cf += "Output.RGBColor =  d * texColor * shade.x * l;\n";
		//cf += "Output.RGBColor *= 1.50000;\n";
		cf += "Output.RGBColor = mix(c1, c2, 0.3*a.r + 0.59*a.g + 0.11*a.b);\n";	// Ajouté
		cf += "Output.RGBColor += d - dcol;\n";	// Ajouté

	/*if(Fog>0)
	{
		cf += "if ( dist >= 0.500000 ){\n";
		cf += "Output.RGBColor *= (1.00000 - (dist - 0.500000));\n";
		cf += "Output.RGBColor += (fog * (dist - 0.500000));\n";
		cf += "}\n";
	}*/
		//cf += "Output.RGBColor *= diffusePower;\n";
		cf += "return Output;\n";
		cf += "}\n";

		cf += "//\n";
		cf += "// Translator's entry point\n";
		cf += "//\n";
		cf += "void main() {\n";
		cf += "PS_OUTPUT xlat_retVal;\n";
		//cf += "xlat_retVal = pixelMain(  vec2(gl_TexCoord[0]), vec2(gl_TexCoord[1]), float(gl_TexCoord[2]), vec2(gl_TexCoord[3]), vec4(gl_Color));\n";
		cf += "xlat_retVal = pixelMain(  vec2(gl_TexCoord[0]), vec2(gl_TexCoord[1]), vec4(gl_Color));\n";	// Ajouté
		cf += "gl_FragData[0] = vec4( xlat_retVal.RGBColor);\n";
		cf += "}\n";

		//-------------------------------------------
 		core::stringc cv;
		cv += "//\n";
		cv += "// Structure definitions\n";
		cv += "//\n" ;

		cv += "struct VS_OUTPUT {\n";
		cv += "vec4 Position;\n";
		cv += "vec4 Diffuse;\n";
		cv += "vec2 TexCoord0;\n";
		cv += "vec2 TexCoord1;\n";
		//cv += "vec2 shade;\n";
		//cv += "vec2 detail;\n";
		cv += "};\n";

		cv += "struct PS_OUTPUT {\n";
		cv += "vec4 RGBColor;\n";
		cv += "};\n";

		cv += "//\n";
		cv += "// Global variable definitions\n";
		cv += "//\n" ;

	/*if(Fog>0)
	{
		cv += "uniform float FogDistance;\n";
		cv += "uniform vec3 cameraPos;\n";
		cv += "uniform mat4 mWorld;\n";
	}*/
		cv += "uniform mat4 mWorldViewProj;\n";
		//cv += "uniform vec3 scaletex;\n";
	//if(LightDefine)	
		//cv += "uniform vec3 dirToLight;\n";	


		cv += "//\n";
		cv += "// Function declarations\n";
		cv += "//\n";

		cv += "VS_OUTPUT vertexMain( in vec4 vPosition, in vec3 vNormal, in vec2 texCoord0, in vec2 texCoord1);\n" ;

		cv += "//\n";
		cv += "// Function definitions\n";
		cv += "//\n";

		cv += "VS_OUTPUT vertexMain( in vec4 vPosition, in vec3 vNormal, in vec2 texCoord0, in vec2 texCoord1) {\n";
		cv += "VS_OUTPUT Output;\n";
		cv += "vec4 position;\n";
		cv += "Output.Position = ( vPosition * mWorldViewProj );\n";
	/*if(Fog>0)
	{
		cv += "float dist;\n";
		cv += "position = ( vPosition * mWorld );\n";
		cv += "dist = sqrt( ((pow( (position.x  - cameraPos.x ), 2.00000) + pow( (position.y  - cameraPos.y ), 2.00000)) + pow( (position.z  - cameraPos.z ), 2.00000)) );\n";
		cv += "Output.Diffuse = vec4( vPosition.y , (dist / FogDistance), 0.000000, 0.000000);\n";
 	}
	else*/
		cv += "Output.Diffuse = vec4(vPosition.y , 0.0f, 0.0f, 0.0f);\n";
           
	/*if(LightDefine)	
	{
		cv += "Output.shade.x = max( 0.000000, dot( normalize( vNormal ), dirToLight));\n";
		cv += "Output.shade.x = (0.200000 + (Output.shade.x * 0.800000));\n";
	}
	else*/
		//cv +="Output.shade.x = 0.9200000;\n";

		cv += "Output.TexCoord0 = texCoord0;\n";
		cv += "Output.TexCoord1 = texCoord1;\n";
		//cv += "Output.detail = texCoord1 * vec2( scaletex);\n";
		cv += "return Output;\n";
		cv += "}\n";

		cv += "//\n";
		cv += "// Translator's entry point\n";
		cv += "//\n";
		cv += "void main() {\n";
		cv += "VS_OUTPUT xlat_retVal;\n";

		cv += "xlat_retVal = vertexMain( vec4(gl_Vertex), vec3(gl_Normal), vec2(gl_MultiTexCoord0), vec2(gl_MultiTexCoord1));\n";

		cv += "gl_Position = vec4( xlat_retVal.Position);\n";
		cv += "gl_FrontColor = vec4( xlat_retVal.Diffuse);\n";
		cv += "gl_TexCoord[0] = vec4( xlat_retVal.TexCoord0, 0.0, 0.0);\n";
		cv += "gl_TexCoord[1] = vec4( xlat_retVal.TexCoord1, 0.0, 0.0);\n";
		//cv += "gl_TexCoord[2] = vec4( xlat_retVal.shade,  0.0, 0.0);\n";
		//cv += "gl_TexCoord[3] = vec4( xlat_retVal.detail, 0.0, 0.0);\n";
		cv += "}\n";

        mc = new MyShaderCallBack();
        //mc->setSplatScene(//SceneManager,Fog,FogColor
		//	);
        newMaterialType1 = gpu->addHighLevelShaderMaterial(
			cv.c_str(), "main", video::EVST_VS_1_1,
			cf.c_str(), "main", video::EPST_PS_1_1,
            mc, video::EMT_SOLID,0);
        mc->drop();
		//if(LightDefine)		mc->lightEnable = true;
    }
#else
	// Nouveau : Shaders externes :
    if (driver->getDriverType() != video::EDT_OPENGL)
    {
        mc = new MyShaderCallBack();
        mc->setSplatScene(SceneManager//,Fog,FogColor
			);
        newMaterialType1 = gpu->addHighLevelShaderMaterialFromFiles(
			"Terrain_N3xtD.hlsl", "vertexMain", video::EVST_VS_1_1,
            "Terrain_N3xtD.hlsl", "pixelMain", video::EPST_PS_1_4,
            mc, video::EMT_SOLID, 0);
        mc->drop();
		//if(LightDefine)	mc->lightEnable = true;
    }
    else
    {
        mc = new MyShaderCallBack();
        mc->setSplatScene(SceneManager//,Fog,FogColor
			);
        newMaterialType1 = gpu->addHighLevelShaderMaterialFromFiles(
			"Terrain_N3xtD_vs.glsl", "main", video::EVST_VS_1_1,
			"Terrain_N3xtD_ps.glsl", "main", video::EPST_PS_1_1,
            mc, video::EMT_SOLID, 0);
        mc->drop();
		//if(LightDefine)		mc->lightEnable = true;
    }
#endif

	if (newMaterialType1 != -1)
		setMaterialType((video::E_MATERIAL_TYPE)newMaterialType1);
}

float CTerrainSceneNode_N3xtD::getHeight(float x, float z, bool interpolate)
{
	// Transforme x et z avec la position et l'agrandissement du terrain
	x -= terrainPosition.X;
	z -= terrainPosition.Z;
	x /= TerrainScale.X;
	z /= TerrainScale.Z;

	// Ajouté car le terrain est maintenant retourné de 180° :
	//x = mHeightmap.m_iWidth - 1 - x;
	z = mHeightmap.m_iHeight - 1 - z;

	if (interpolate)
		return (mHeightmap.GetInterpolateHeight(x, z)) + terrainPosition.Y;

	return mHeightmap.GetHeightValue(x, z) + terrainPosition.Y;
}

void CTerrainSceneNode_N3xtD::setHeight(float x, float z, float value)
{
	mHeightmap.SetHeightValue((int)((x-terrainPosition.X)/TerrainScale.X), (int)((z-terrainPosition.Z)/TerrainScale.Z),  value);
}

//Take from the irrlicht source code
// Modifié : Fonction débuggée !
void CTerrainSceneNode_N3xtD::calculateNormals(scene::SMeshBufferLightMap* pMeshBuffer, u32 Size)
{
    u32 count;
    //s32 Size = 4;
    core::vector3df a, b, c, t;

    for (u32 x=0; x<Size; ++x)
        for (u32 z=0; z<Size; ++z)
        {
            count = 0;
            core::vector3df normal;

			const u32 currentIdx = x * Size + z;
			const u32 upIdx = x * Size + z - 1;
			const u32 downIdx = x * Size + z + 1;
			const u32 leftIdx = (x - 1) * Size + z;
			const u32 rightIdx = (x + 1) * Size + z;
			const u32 upLeftIdx = (x - 1) * Size + z - 1;
			const u32 upRightIdx = (x + 1) * Size + z - 1;
			const u32 downLeftIdx = (x - 1) * Size + z + 1;
			const u32 downRightIdx = (x + 1) * Size + z + 1;

            // top left
            if (x>0 && z>0)
            {
                a = pMeshBuffer->Vertices[upLeftIdx].Pos;
                b = pMeshBuffer->Vertices[leftIdx].Pos;
                c = pMeshBuffer->Vertices[currentIdx].Pos;
                b -= a;
                c -= a;
                t = b.crossProduct ( c );
                t.normalize ( );
                normal += t;

                a = pMeshBuffer->Vertices[upLeftIdx].Pos;
                b = pMeshBuffer->Vertices[currentIdx].Pos;	// Modifié : Ancien : upIdx !
                c = pMeshBuffer->Vertices[upIdx].Pos;		// Modifié : Ancien : currentIdx !
                b -= a;
                c -= a;
                t = b.crossProduct ( c );
                t.normalize ( );
                normal += t;

                count += 2;
            }

            // top right
            if (x>0 && z<Size-1)
            {
                a = pMeshBuffer->Vertices[leftIdx].Pos;
                b = pMeshBuffer->Vertices[downLeftIdx].Pos;
                c = pMeshBuffer->Vertices[downIdx].Pos;
                b -= a;
                c -= a;
                t = b.crossProduct ( c );
                t.normalize ( );
                normal += t;

                a = pMeshBuffer->Vertices[leftIdx].Pos;
                b = pMeshBuffer->Vertices[downIdx].Pos;
                c = pMeshBuffer->Vertices[currentIdx].Pos;
                b -= a;
                c -= a;
                t = b.crossProduct ( c );
                t.normalize ( );
                normal += t;

                count += 2;
            }

            // bottom right
            if (x<Size-1 && z<Size-1)
            {
                a = pMeshBuffer->Vertices[currentIdx].Pos;	// Modifié : Ancien : downIdx !
                b = pMeshBuffer->Vertices[downIdx].Pos;
                c = pMeshBuffer->Vertices[downRightIdx].Pos;
                b -= a;
                c -= a;
                t = b.crossProduct ( c );
                t.normalize ( );
                normal += t;

                a = pMeshBuffer->Vertices[downIdx].Pos;
                b = pMeshBuffer->Vertices[downRightIdx].Pos;
                c = pMeshBuffer->Vertices[rightIdx].Pos;
                b -= a;
                c -= a;
                t = b.crossProduct ( c );
                t.normalize ( );
                normal += t;

                count += 2;
            }

            // bottom left
            if (x<Size-1 && z>0)
            {
                a = pMeshBuffer->Vertices[upIdx].Pos;
                b = pMeshBuffer->Vertices[currentIdx].Pos;
                c = pMeshBuffer->Vertices[rightIdx].Pos;
                b -= a;
                c -= a;
                t = b.crossProduct ( c );
                t.normalize ( );
                normal += t;

                a = pMeshBuffer->Vertices[upIdx].Pos;
                b = pMeshBuffer->Vertices[rightIdx].Pos;
                c = pMeshBuffer->Vertices[upRightIdx].Pos;
                b -= a;
                c -= a;
                t = b.crossProduct ( c );
                t.normalize ( );
                normal += t;

                count += 2;
            }

            if ( count != 0 )
                normal.normalize ( );
            else
                normal.set( 0.0f, 1.0f, 0.0f );

            pMeshBuffer->Vertices[currentIdx].Normal = normal;
        }
}

}
}
