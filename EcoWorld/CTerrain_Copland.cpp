#include "CTerrain_Copland.h"
#include "CShaderPreprocessor.h"
#include "Game.h"	// Permet l'accès au préprocesseur de shaders

// Cette version a été modifiée

CTerrain_Copland::CTerrain_Copland(video::IImage *Heightmap,TerrainQuality Quality,f32 ScaleTexture, const core::vector3df& pos, const core::vector3df& scale,scene::ISceneNode* parent,scene::ISceneManager* smgr,s32 id): scene::ISceneNode(parent, smgr, id), mc(0)
{
    //Test if the number of quality is correct
    if (Quality != 1 && Quality != 2 && Quality != 4 && Quality != 8)
    {
        //if not force to medium quality
        Quality = Medium;
    }

    //Set the Debug to false
    //Debug=false;

    //Get dimension of heightmap
    const u16 Size = Heightmap->getDimension().Width;

    //Set the size of Tiles for Terrain
    s32 SizeOfTiles = 0;

    //Switch Size for calculate the Size of Tile
    switch (Size)	// Modifié pour gérer des tailles de 2^n+1, ce qui résoud ainsi un bug sur les bords du terrain
    {
         case 65:
            SizeOfTiles=(Size/4)+1;
            NbsTiles = 4*4;
            break;
        case 129:
            SizeOfTiles=(Size/8)+1;
            NbsTiles = 8*8;
            break;
        case 257:
			SizeOfTiles=(Size/16)+1;
            NbsTiles = 16*16;
            break;
        case 513:
            SizeOfTiles=(Size/16)+1;
            NbsTiles = 16*16;
            break;
        case 769:
            SizeOfTiles=(Size/24)+1;
            NbsTiles = 24*24;
            break;
        case 1025:
            SizeOfTiles=(Size/32)+1;
            NbsTiles = 32*32;
            break;
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
    const u32 SOTiles = irr::core::ceil32((f32)SizeOfTiles/(f32)Quality);

	// Ajouté :
	const int halfQuality = ((Quality+1)/2);

    //Init the array of MeshBuffer
    CTTileBuffer=new scene::SMeshBufferLightMap* [NbsTiles];

	// Ajouté : Initialise le tableau des triangle selectors
	metaTriSelector = smgr->createMetaTriangleSelector();
	TriangleSelectors = new scene::ITriangleSelector* [NbsTiles];
	for (u16 i = 0; i < NbsTiles; ++i)
		TriangleSelectors[i] = NULL;

	// Ajouté : Initialise le Material général du terrain
	Material.Lighting = true;
	Material.BackfaceCulling = true;
	Material.GouraudShading = true;
	Material.FogEnable = false;

    //Start the loop to create Buffers
    u32 TileX=0,TileZ=0;
    const f32 tdSize = 1.0f/(f32)(Size-1);
    for (u32 i =0;i < NbsTiles;++i)
    {
        CTTileBuffer[i]=new scene::SMeshBufferLightMap();
        CTTileBuffer[i]->Vertices.set_used(SizeOfTiles*SizeOfTiles);
        CTTileBuffer[i]->Indices.set_used(SizeOfTiles*SizeOfTiles*6);

        u32 Index=0;
        u16 NbsIndices=0,NbsVertices=0;

        for(u32 x=TileX;x<(TileX+SizeOfTiles);x+=Quality)
        {
            for (u32 z=TileZ;z<(TileZ+SizeOfTiles);z+=Quality)
            {
                if (NbsVertices < (SOTiles*SOTiles)-SOTiles-1)
                {
                    Index = NbsVertices;
                    u32 TestValue = ( (((x-(int)TileX)/Quality)+1) * ((z-(int)TileZ)/Quality) + ((x-(int)TileX)/Quality) );
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
                //video::SColor pixelColor(Heightmap->getPixel(x,z));
				//Vertex.Pos.set((f32)x, (f32) pixelColor.getLuminance()/10.0f, (f32)z);
				Vertex.Pos.set((f32)x, Heightmap->getPixel(x,z).getLuminance(), (f32)z);	// Ajouté
                Vertex.TCoords.set( (f32)(x*tdSize), (f32)(z*tdSize));
                Vertex.TCoords2.set( (f32)(x*tdSize)*ScaleTexture, (f32)(z*tdSize)*ScaleTexture);*/

				// Modifié : Pris de CTerrain_paged.cpp et adapté : Permet de tourner le terrain de 180°
				video::S3DVertex2TCoords Vertex;
                Vertex.Normal.set(0,1,0);
				Vertex.Pos.set((f32)x * scale.X + pos.X, (Heightmap->getPixel((int)x,(int)(Size-1-z)).getLuminance()) * scale.Y + pos.Y, (f32)z * scale.Z + pos.Z);
                Vertex.TCoords.set((f32)x*tdSize, (f32)(Size-1-z)*tdSize);
                Vertex.TCoords2.set((f32)x*tdSize*ScaleTexture, (f32)(Size-1-z)*tdSize*ScaleTexture);	
				Vertex.Color.set(255, 255, 255, 255);

                CTTileBuffer[i]->Vertices[NbsVertices]=Vertex;

                NbsVertices++;
            }
        }

		/*
        CTTileBuffer[i]->Material.Lighting = true;
        //CTTileBuffer[i]->Material.Wireframe = true;
        CTTileBuffer[i]->Material.BackfaceCulling = true;
        CTTileBuffer[i]->Material.GouraudShading=true;
        CTTileBuffer[i]->Material.FogEnable=false;

        CTTileBuffer[i]->Material.DiffuseColor.set(255, 255, 255, 255);
        CTTileBuffer[i]->Material.AmbientColor.set(255, 255, 255, 255);
        CTTileBuffer[i]->Material.EmissiveColor.set(255, 255, 255, 255);
		*/

        CTTileBuffer[i]->Vertices.set_used(NbsVertices);
        CTTileBuffer[i]->Indices.set_used(NbsIndices);

        for(s32 j = 0; j < halfQuality; ++j) 
        {
            for(u32 index = 2; index < (SOTiles * SOTiles - 2); ++index) 
            {
                //A[i] = (1/8)*(A[i-2] + 2*A[i-1] + 2*A[i] + 2*A[i+1] + A[i+2]);
                CTTileBuffer[i]->Vertices[index].Pos.Y += (1/8)*
					(CTTileBuffer[i]->Vertices[index-2].Pos.Y +
					2*CTTileBuffer[i]->Vertices[index-1].Pos.Y +
					2*CTTileBuffer[i]->Vertices[index].Pos.Y +
					2*CTTileBuffer[i]->Vertices[index+1].Pos.Y +
					CTTileBuffer[i]->Vertices[index+2].Pos.Y);
            }
        }

		for (s32 k = 0; k < halfQuality; ++k) 
        { 
            for(u32 index = SOTiles; index < (SOTiles * (SOTiles - 1)); ++index) 
            { 
                CTTileBuffer[i]->Vertices[index].Pos.Y = (CTTileBuffer[i]->Vertices[index - SOTiles].Pos.Y + CTTileBuffer[i]->Vertices[index + SOTiles].Pos.Y ) * 0.5f; 
            } 
        }

        //Calculate the normals
        calculateNormals(CTTileBuffer[i],SOTiles);

        //Recalculate the bounding box
        CTTileBuffer[i]->recalculateBoundingBox();

		// Ajouté :
		CTTileBuffer[i]->BoundingBox.MinEdge.Y = 0.0f;

		// Ajouté depuis le Terrain Paged : Active les VBO (uniquement disponible sous OpenGL)
		// active VBO for performance
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
    AutomaticCullingState = scene::EAC_OFF;
}

CTerrain_Copland::~CTerrain_Copland()
{
	// Ajouté :
	metaTriSelector->drop();
	for (u16 i = 0; i < NbsTiles; ++i)
	{
		if (TriangleSelectors[i])
			TriangleSelectors[i]->drop();

        if (CTerrain_Copland::CTTileBuffer[i])
            CTerrain_Copland::CTTileBuffer[i]->drop();
	}
	delete[] TriangleSelectors;

    delete[] CTerrain_Copland::CTTileBuffer;

    if (CTerrain_Copland::TerrainMesh)
        CTerrain_Copland::TerrainMesh->drop();
}

void CTerrain_Copland::createTriangleSelectors()
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
void CTerrain_Copland::OnRegisterSceneNode()
{
    if (IsVisible)
        SceneManager->registerNodeForRendering(this, scene::ESNRP_SOLID);

    ISceneNode::OnRegisterSceneNode();
}

void CTerrain_Copland::render()
{
    const scene::ICameraSceneNode* cam = SceneManager->getActiveCamera();
    const scene::SViewFrustum* frustum = cam->getViewFrustum();
    video::IVideoDriver* Driver = SceneManager->getVideoDriver();
    core::vector3df Pos = cam->getPosition();
    //cam->updateAbsolutePosition();

    Pos.Y = RelativeTranslation.Y;	// Pos.Y = 0.0f;

	Driver->setTransform(video::ETS_WORLD, core::IdentityMatrix);	// Driver->setTransform(video::ETS_WORLD,AbsoluteTransformation);
	Driver->setMaterial(Material);	// Optimisation
    for (u32 i=0;i<NbsTiles;++i)
    {
        if (CTTileBuffer[i])
        {
            //DEBUG HERE
            /*if (Debug)
            {
                video::SMaterial Mat;
                Mat.AmbientColor = video::SColor(255, 255, 255, 255);
                Mat.DiffuseColor = video::SColor(255, 255, 255, 255);
                Mat.EmissiveColor = video::SColor(255, 0, 255, 0);
                Driver->setMaterial(Mat);
                Driver->draw3DBox(CTTileBuffer[i]->getBoundingBox(),video::SColor(255, 255, 255, 255));
            }*/

            if (frustum->getBoundingBox().intersectsWithBox(CTTileBuffer[i]->getBoundingBox()))
            {
                f32 ActualDistance = CTTileBuffer[i]->BoundingBox.getCenter().getDistanceFromSQ(Pos); // Optimisation : Ancien : CTTileBuffer[i]->BoundingBox.getCenter().getDistanceFrom(Pos)
                if (ActualDistance < RenderDistance)
                {
                    //Driver->setMaterial(video::SMaterial(CTTileBuffer[i]->Material));
					// Driver->drawIndexedTriangleList est désactivé pour permettre l'activation des VBO sous OpenGL (seule la fonction Driver->drawMeshBuffer le permet)
                    //Driver->drawIndexedTriangleList(&CTTileBuffer[i]->Vertices[0],CTTileBuffer[i]->getVertexCount(),&CTTileBuffer[i]->Indices[0], CTTileBuffer[i]->getIndexCount()/3 );
                    Driver->drawMeshBuffer(CTTileBuffer[i]);
                }
            }
        }
    }
}

void CTerrain_Copland::setPosition(const core::vector3df &Pos)
{
 	Box.reset(0, 0, 0);
   for (u32 i=0;i<NbsTiles;++i)
    {
        if (CTTileBuffer[i])
        {
            for (u32 j=0;j<CTTileBuffer[i]->getVertexCount();++j)
            {
                CTTileBuffer[i]->Vertices[j].Pos+=Pos;
            }
            CTTileBuffer[i]->recalculateBoundingBox();
			//Box.addInternalBox(CTTileBuffer[i]->getBoundingBox());	// Ajouté : recalcule la bounding box globale du terrain
        }
    }
}

void CTerrain_Copland::setScale(const core::vector3df &Scale)
{
	Box.reset(0, 0, 0);
    for (u32 i=0;i<NbsTiles;++i)
    {
        if (CTTileBuffer[i])
        {
            for (u32 j=0;j<CTTileBuffer[i]->getVertexCount();++j)
            {
                CTTileBuffer[i]->Vertices[j].Pos*=Scale;
            }
            CTTileBuffer[i]->recalculateBoundingBox();
			//Box.addInternalBox(CTTileBuffer[i]->getBoundingBox());	// Ajouté : recalcule la bounding box globale du terrain
        }
    }
}

void CTerrain_Copland::ActivateSplattingTextures(//scene::ISceneManager* smgr,
												 f32 Height
												 //,f32 Fog,video::SColorf FogColor
												 )
{
    video::IVideoDriver* driver = SceneManager->getVideoDriver();
    int newMaterialType1 = -1;

	// Les shaders ont été largement modifiés et sont maintenant disponibles dans des fichiers externes

#if 0
	// Ancien : Shaders compilés :

	video::IGPUProgrammingServices* gpu = driver->getGPUProgrammingServices();
	if (!gpu)
		return;

    if (driver->getDriverType() != video::EDT_OPENGL)
    {
		//Merci à DeusXL pour son shader :D
        const c8 ShaderFileName[] =
            "float4x4 mWorldViewProj;\n" \
            "float4x4 mWorld;\n" \
            //"float3 cameraPos;\n" \//
            "float TerrainHeight;\n" \
            //"float FogDistance;\n" \//

            "struct VS_OUTPUT\n" \
            "{\n" \
            "float4 Position : POSITION;\n" \
            "float4 Diffuse : COLOR0;\n" \
            "float2 TexCoord1 : TEXCOORD1;\n" \
            "};\n" \

            "VS_OUTPUT vertexMain( in float4 vPosition : POSITION,in float3 vNormal : NORMAL,float2 texCoord1 : TEXCOORD1)\n" \
            "{\n" \
            "VS_OUTPUT Output;\n" \
            "Output.Position = mul(vPosition, mWorldViewProj);\n" \
            //"float4 position = mul(vPosition, mWorld);\n" \//
            //"float dist = sqrt ( pow(position.x - cameraPos.x, 2.0f) +\n" \//
            //"pow(position.y - cameraPos.y, 2.0f) +\n" \//
            //"pow(position.z - cameraPos.z, 2.0f));\n" \//
            //"Output.Diffuse = float4(vPosition.y / TerrainHeight, dist / FogDistance, 0.0f, 0.0f);\n" \//
            "Output.Diffuse = float4(vPosition.y / TerrainHeight, 0.0f, 0.0f, 0.0f);\n" \
            "Output.TexCoord1 = texCoord1;\n" \
            "return Output;\n" \
            "}\n" \

            "struct PS_OUTPUT\n" \
            "{\n" \
            "float4 RGBColor : COLOR0;\n" \
            "};\n" \

            "sampler2D tex[4];\n" \
            //"float4 fog;\n" \//
            "PS_OUTPUT pixelMain( float2 TexCoord1 : TEXCOORD1,float4 Position : POSITION,float4 Diffuse : COLOR0 )\n" \
            "{\n" \
            "PS_OUTPUT Output;\n" \
            "float heightpercent = Diffuse.x;\n" \
            //"float dist = Diffuse.y;\n" \//

            //"float4 grasscolor = tex2D(tex[0], TexCoord1 * 5.0f) * pow((1.0f - heightpercent), 4.0f);\n" \//
            //"float4 rockcolor = tex2D(tex[1], TexCoord1 * 5.0f) * pow((1.0f - abs(0.5f - heightpercent)), 4.0f);\n" \//
            //"float4 snowcolor = tex2D(tex[2], TexCoord1 * 5.0f) * pow(heightpercent,4.0f);\n" \//
            //"float4 detailcolor = tex2D(tex[3], TexCoord1 * 5.0f) * pow((1.0f - abs(0.7f - heightpercent)), 4.0f);\n" \//
            "float4 grasscolor = tex2D(tex[0], TexCoord1) * pow((1.0f - heightpercent), 4.0f);\n" \
            "float4 rockcolor = tex2D(tex[1], TexCoord1) * pow((1.0f - abs(0.5f - heightpercent)), 4.0f);\n" \
            "float4 snowcolor = tex2D(tex[2], TexCoord1) * pow(heightpercent,4.0f);\n" \
            "float4 detailcolor = tex2D(tex[3], TexCoord1) * pow((1.0f - abs(0.7f - heightpercent)), 4.0f);\n" \
            "Output.RGBColor = (grasscolor + rockcolor + snowcolor  + detailcolor);\n" \

            //"if (dist >= 0.5f)\n" \//
            //"{\n" \//
            //"Output.RGBColor *= (1.0f - (dist-0.5f));\n" \//
            //"Output.RGBColor += (fog  * (dist-0.5f));\n" \//
            //"}\n" \//

            "return Output;\n" \
            "}\n";

        mc = new MyShaderCallBack_copland();
        mc->setSplatScene(//smgr,
			Height
			//,Fog,FogColor
			);
        newMaterialType1 = gpu->addHighLevelShaderMaterial(
            ShaderFileName, "vertexMain", video::EVST_VS_2_0,
            ShaderFileName, "pixelMain", video::EPST_PS_2_0,
            mc, video::EMT_SOLID,0);
		mc->drop();
    }
    else
    {
        const c8 ShaderFragment[] =
            "//\n" \
            "// Structure definitions\n" \
            "//\n" \
            "struct VS_OUTPUT {\n" \
            "vec4 Position;\n" \
            "vec4 Diffuse;\n" \
            "vec2 TexCoord1;\n" \
            "};\n" \

            "struct PS_OUTPUT {\n" \
            "vec4 RGBColor;\n" \
            "};\n" \


            "//\n" \
            "// Global variable definitions\n" \
            "//\n" \

            //"uniform vec4 fog;\n" \//
            "uniform sampler2D texgrass,texrock,texsnow,texdetail;\n" \

            "//\n" \
            "// Function declarations\n" \
            "//\n" \
            "PS_OUTPUT pixelMain( in vec2 TexCoord1, in vec4 Position, in vec4 Diffuse );\n" \
            "//\n" \
            "// Function definitions\n" \
            "//\n" \
            "PS_OUTPUT pixelMain( in vec2 TexCoord1, in vec4 Position, in vec4 Diffuse ) {\n" \
            "float heightpercent;\n" \
            "float dist;\n" \
            "vec4 grasscolor;\n" \
            "vec4 rockcolor;\n" \
            "vec4 snowcolor;\n" \
            "vec4 detailcolor;\n" \
            "PS_OUTPUT Output;\n" \

            "heightpercent = Diffuse.x ;\n" \
            //"dist = Diffuse.y ;\n" \//
            //"grasscolor = (texture2D( texgrass, (TexCoord1 * 5.00000)) * pow( (1.00000 - heightpercent), 4.00000));\n" \//
            //"rockcolor = (texture2D( texrock, (TexCoord1 * 5.00000)) * pow( (1.00000 - abs( (0.500000 - heightpercent) )), 4.00000));\n" \//
            //"snowcolor = (texture2D( texsnow, (TexCoord1 * 5.00000)) * pow( heightpercent, 4.00000));\n" \//
            //"detailcolor = (texture2D( texdetail, (TexCoord1 * 5.00000)) * pow( (1.00000 - abs( (0.700000 - heightpercent) )), 4.00000));\n" \//
            "grasscolor = (texture2D( texgrass, TexCoord1) * pow( (1.00000 - heightpercent), 4.00000));\n" \
            "rockcolor = (texture2D( texrock, TexCoord1) * pow( (1.00000 - abs( (0.500000 - heightpercent) )), 4.00000));\n" \
            "snowcolor = (texture2D( texsnow, TexCoord1) * pow( heightpercent, 4.00000));\n" \
            "detailcolor = (texture2D( texdetail, TexCoord1) * pow( (1.00000 - abs( (0.700000 - heightpercent) )), 4.00000));\n" \
            "Output.RGBColor = (((grasscolor + rockcolor) + snowcolor) + detailcolor);\n" \
            //"if ( (dist >= 0.500000) ){\n" \//
            //"Output.RGBColor *= (1.00000 - (dist - 0.500000));\n" \//
            //"Output.RGBColor += (fog * (dist - 0.500000));\n" \//
            //"}\n" \//
            "return Output;\n" \
            "}\n" \

            "//\n" \
            "// Translator's entry point\n" \
            "//\n" \
            "void main() {\n" \
            "PS_OUTPUT xlat_retVal;\n" \
            "xlat_retVal = pixelMain( vec2(gl_TexCoord[1]), vec4(gl_FragCoord), vec4(gl_Color));\n" \
            "gl_FragData[0] = vec4( xlat_retVal.RGBColor);\n" \
            "}\n";

        const c8 ShaderVertex[]=
            "//\n" \
            "// Structure definitions\n" \
            "//\n" \

            "struct VS_OUTPUT {\n" \
            "vec4 Position;\n" \
            "vec4 Diffuse;\n" \
            "vec2 TexCoord1;\n" \
            "};\n" \

            "struct PS_OUTPUT {\n" \
            "vec4 RGBColor;\n" \
            "};\n" \

            "//\n" \
            "// Global variable definitions\n" \
            "//\n" \

            //"uniform float FogDistance;\n" \//
            "uniform float TerrainHeight;\n" \
            //"uniform vec3 cameraPos;\n" \//
            //"uniform mat4 mWorld;\n" \//
            "uniform mat4 mWorldViewProj;\n" \

            "//\n" \
            "// Function declarations\n" \
            "//\n" \

            "VS_OUTPUT vertexMain( in vec4 vPosition, in vec3 vNormal, in vec2 texCoord1 );\n" \

            "//\n" \
            "// Function definitions\n" \
            "//\n" \

            "VS_OUTPUT vertexMain( in vec4 vPosition, in vec3 vNormal, in vec2 texCoord1 ) {\n" \
            "VS_OUTPUT Output;\n" \
            "vec4 position;\n" \
            //"float dist;\n" \//

            "Output.Position = ( vPosition * mWorldViewProj );\n" \
            //"position = ( vPosition * mWorld );\n" \//
            //"dist = sqrt( ((pow( (position.x  - cameraPos.x ), 2.00000) + pow( (position.y  - cameraPos.y ), 2.00000)) + pow( (position.z  - cameraPos.z ), 2.00000)) );\n" \//
            //"Output.Diffuse = vec4( (vPosition.y  / TerrainHeight), (dist / FogDistance), 0.000000, 0.000000);\n" \//
            "Output.Diffuse = vec4( (vPosition.y  / TerrainHeight), 0.000000, 0.000000, 0.000000);\n" \
            "Output.TexCoord1 = texCoord1;\n" \
            "return Output;\n" \
            "}\n" \


            "//\n" \
            "// Translator's entry point\n" \
            "//\n" \
            "void main() {\n" \
            "VS_OUTPUT xlat_retVal;\n" \

            "xlat_retVal = vertexMain( vec4(gl_Vertex), vec3(gl_Normal), vec2(gl_MultiTexCoord1));\n" \

            "gl_Position = vec4( xlat_retVal.Position);\n" \
            "gl_FrontColor = vec4( xlat_retVal.Diffuse);\n" \
            "gl_TexCoord[1] = vec4( xlat_retVal.TexCoord1, 0.0, 0.0);\n" \
            "}\n";

        mc = new MyShaderCallBack_copland();
        mc->setSplatScene(//smgr,
			Height
			//,Fog,FogColor
			);
        newMaterialType1 = gpu->addHighLevelShaderMaterial(
            ShaderVertex, "main", video::EVST_VS_1_1,
            ShaderFragment, "main", video::EPST_PS_1_1,
            mc, video::EMT_SOLID,0);
		mc->drop();
    }
#else
	// Nouveau : Shaders externes :
    if (driver->getDriverType() != video::EDT_OPENGL)
    {
        mc = new MyShaderCallBack_copland();
        mc->setSplatScene(SceneManager,
			Height
			//,Fog,FogColor
			);
        newMaterialType1 = game->shaderPreprocessor->addHighLevelShaderMaterialFromFiles(
            "Terrain_Copland.hlsl", "vertexMain", video::EVST_VS_1_1,
            "Terrain_Copland.hlsl", "pixelMain", video::EPST_PS_2_0,
            mc, video::EMT_SOLID);
		mc->drop();
    }
    else
    {
        mc = new MyShaderCallBack_copland();
        mc->setSplatScene(SceneManager,
			Height
			//,Fog,FogColor
			);
        newMaterialType1 = game->shaderPreprocessor->addHighLevelShaderMaterialFromFiles(
            "Terrain_Copland_vs.glsl", "main", video::EVST_VS_1_1,
            "Terrain_Copland_ps.glsl", "main", video::EPST_PS_1_1,
            mc, video::EMT_SOLID);
		mc->drop();
    }
#endif

	if (newMaterialType1 != -1)
		setMaterialType((video::E_MATERIAL_TYPE)newMaterialType1);
}

f32 CTerrain_Copland::getHeight(f32 x,f32 z)
{
	// Cette fonction a été grandement optimisée !

	int iBuffer = -1;
	const core::vector3df point(x, 0.0f, z);
    for (u16 i = 0; i < NbsTiles; ++i)
    {
        if (CTTileBuffer[i])
        {
			if (CTTileBuffer[i]->getBoundingBox().isPointInside(point))
            {
				iBuffer = (int)i;
                break;
            }
        }
    }

	if (iBuffer >= 0)
	{
		const core::line3df line(x, -200000.0f, z, x, 200000.0f, z);

		core::vector3df intersection;
		core::triangle3df tri;
		scene::ISceneNode* node;

		if (SceneManager->getSceneCollisionManager()->getCollisionPoint(line, TriangleSelectors[iBuffer], intersection, tri, node))
			return intersection.Y;
	}

    return -999999.9f;
}

//Take from the irrlicht source code
// Modifié : Fonction débuggée !
void CTerrain_Copland::calculateNormals(scene::SMeshBufferLightMap* pMeshBuffer, u32 Size)
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
