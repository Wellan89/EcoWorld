#ifndef _SHADERMATERIAL_H
#define _SHADERMATERIAL_H

#include "global.h"
class CPostProcessManager;

// built in shader constants
// combine using bitwise or
enum E_SHADER_CONSTANT
{
	ESC_TIME =			0x1,
	ESC_RANDOM =		0x2,
	ESC_BUFFERWIDTH =	0x4,
	ESC_BUFFERHEIGHT =	0x8,
	ESC_PROJECTION =	0x10,
	ESC_VIEW =			0x20,
	ESC_WORLD =			0x40,
	ESC_WORLDVIEW =		0x80,
	ESC_WORLDVIEWPROJ =	0x100
};

class CShaderMaterial : public video::IShaderConstantSetCallBack
{
public:
	// constructor
	CShaderMaterial(IrrlichtDevice* device, CPostProcessManager& postProcessManager, u32 maxMRTs, const core::stringw& Name,
		const io::path& vs_file, const core::stringc& vs_entry_, video::E_VERTEX_SHADER_TYPE vs_type_,
		const io::path& ps_file, const core::stringc& ps_entry_, video::E_PIXEL_SHADER_TYPE ps_type_,
		video::E_MATERIAL_TYPE baseMaterial_);

protected:
	// the irrlicht video driver
	video::IVideoDriver* driver;
	ITimer* timer;

	// shader constant maps
	core::map<core::stringc, f32> PixelShaderConstant;
	core::map<core::stringc, f32> VertexShaderConstant;

	// texture names
	core::stringc TextureName[video::MATERIAL_MAX_TEXTURES];

	// material
	video::SMaterial Material;

	// additional built in shader constants
	u32 PixelShaderFlags;
	u32 VertexShaderFlags;

	// Ajouté :
	
	CPostProcessManager& postProcess;

	core::stringw name;

	// Paramêtres nécessaires pour la compilation retardée du shader
	io::path vs_path;
	core::stringc vs_entry;
	video::E_VERTEX_SHADER_TYPE vs_type;
	io::path ps_path;
	core::stringc ps_entry;
	video::E_PIXEL_SHADER_TYPE ps_type;
	video::E_MATERIAL_TYPE baseMaterial;

	// Chemin des textures du matériau de ce shader
	core::stringw TexturePath[video::MATERIAL_MAX_TEXTURES];

	// Nom des textures RTT cibles de rendu de ce shader
	core::array<core::stringw> RenderTarget;

public:
	// OnSetConstants callback to transfer shader constant to the gpu program
	virtual void OnSetConstants(video::IMaterialRendererServices* services, s32 userdata);
	
	// sets pixel shader constant
	void setPixelShaderConstant(const core::stringc& name, f32 value) { PixelShaderConstant[name] = value; }
	// returns pixel shader constant
	f32 getPixelShaderConstant(const core::stringc& name) { return PixelShaderConstant[name]; }
	// returns a pixel shader constant iterator
	core::map<core::stringc, f32>::Iterator getPixelShaderConstantIterator() { return PixelShaderConstant.getIterator(); }
	// sets pixel shader flag (bitwise or combination)
	void setPixelShaderFlag(E_SHADER_CONSTANT flag, bool enabled=true) { PixelShaderFlags = (PixelShaderFlags & (~flag)) | ((((u32)!enabled)-1) & flag); }
	// returns pixel shader flag
	bool getPixelShaderFlag(E_SHADER_CONSTANT flag) const { return (PixelShaderFlags& flag)!=0; }

	// set vertex shader constants
	void setVertexShaderConstant(const core::stringc& name, f32 value) { VertexShaderConstant[name] = value; }
	// returns vertex shader constant
	f32 getVertexShaderConstant(const core::stringc& name) { return VertexShaderConstant[name]; }
	// returns a vertex shader constant iterator
	core::map<core::stringc, f32>::Iterator getVertexShaderConstantIterator() { return VertexShaderConstant.getIterator(); }
	// sets vertex shader flag
	void setVertexShaderFlag(E_SHADER_CONSTANT flag, bool enabled=true) { VertexShaderFlags = (VertexShaderFlags & (~flag)) | ((((u32)!enabled)-1) & flag); }
	// returns vertex shader flag
	bool getVertexShaderFlag(E_SHADER_CONSTANT flag) const { return (VertexShaderFlags& flag)!=0; }
	
	// sets texture name (used for opengl gpu programs)
	void setTextureName(u32 index, const core::stringc& name) { TextureName[index] = name; }

	// returns the SMaterial struct of the shader material
	video::SMaterial& getMaterial() { return Material; }

	// returns the material type of the shader material
	video::E_MATERIAL_TYPE getMaterialType() const { return Material.MaterialType; }



	// Ajouté :
	
	const core::stringw& getName() const								{ return name; }

	// Indique si ce shader a été compilé ou non (attention : ce shader sera considéré comme non compilé si sa compilation a échouée)
	bool isCompiled() const												{ return (Material.MaterialType != -1); }

	// Indique le chemin d'une texture, pour permettre son chargement ultérieur
	// Attention : Les changements de textures sont interdits une fois le shader compilé !
	void setTexturePath(u32 index, const core::stringw& path)			{ if (index < video::MATERIAL_MAX_TEXTURES && !isCompiled()) { TexturePath[index] = path; } }

	// Indique le nom d'une texture RTT cible de ce shader
	// Attention : Les changements de textures sont interdits une fois le shader compilé !
	void setTextureTarget(u32 index, const core::stringw& renderTarget)	{ if (index < RenderTarget.size() && !isCompiled()) { RenderTarget[index] = renderTarget; } }

	// Compile ce shader et charge et assigne les textures correspondantes à son matériau
	void compileShader();

	// Détermine si ce shader nécessite le rendu de la profondeur de la scène (depth pass)
	bool needDepthPass() const;

	// Avant le rendu de ce shader : met à jour les textures de rendu cibles du driver et son matériau
	void setupDriverForRender();
};

#endif
