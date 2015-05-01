#ifndef DEF_SHADER_PREPROCESSOR
#define DEF_SHADER_PREPROCESSOR

#include "global.h"

// Simple préprocesseur de shader appliqué à tous les shaders avant leur compilation par DirectX ou OpenGL
// En pratique, cette classe ajoute simplement certaines définitions de préprocesseur (déclarées par des directives "#define") au début des shaders d'après les paramêtres du driver actuels
class CShaderPreprocessor
{
public:
	// Constructeur inline
	CShaderPreprocessor(io::IFileSystem* FileSystem, video::IGPUProgrammingServices* Gpu) : fileSystem(FileSystem), gpu(Gpu)
	{
		// Génère l'en-tête général des shaders
		//generateShadersHeader();

		// Génère l'en-tête de XEffects
		generateXEffectsShadersHeader();
	}

	// Traîte un shader depuis un fichier puis le compile
	int addHighLevelShaderMaterialFromFiles(
		const io::path& vertexShaderProgramFileName,
		const char* vertexShaderEntryPointName,
		video::E_VERTEX_SHADER_TYPE vsCompileTarget,
		const io::path& pixelShaderProgramFileName,
		const char* pixelShaderEntryPointName,
		video::E_PIXEL_SHADER_TYPE psCompileTarget,
		video::IShaderConstantSetCallBack* callback,
		video::E_MATERIAL_TYPE baseMaterial,
		int userData = 0,
		bool xEffectsShader = false, bool xEffectsVerticalVSMBlur = false);

	// Génère l'en-tête général des shaders
	// A appeler dès la création du driver d'Irrlicht et son paramêtrage
	//void generateShadersHeader();

	// Génère l'en-tête pour les shaders de XEffects
	// A appeler une fois que la configuration du jeu a été chargée ou modifiée
	void generateXEffectsShadersHeader();

	// Génère l'en-tête complet d'un certain shader
	// Note : Efface auparavant la chaîne outHeader (ne l'ajoute pas à sa fin !)
	void generateFullShaderHeader(core::stringc& outHeader, video::E_VERTEX_SHADER_TYPE vsCompileTarget, video::E_PIXEL_SHADER_TYPE psCompileTarget,
		bool xEffectsShader, bool xEffectsVerticalVSMBlur);

protected:
	// Charge le texte contenu par un shader depuis un fichier : ajoute ce texte à la fin de la chaîne de caractères outShader (n'efface pas cette chaîne !)
	// Attention : le cas filename = outShader (la même chaîne de caractère est passée en argument : ex : getShaderFile(shader, shader); ) n'est pas autorisé !
	void getShaderFile(const io::path& filename, core::stringc& outShader);

	// Le système de fichier d'Irrlicht
	io::IFileSystem* fileSystem;

	// L'interface de shaders d'Irrlicht
	video::IGPUProgrammingServices* gpu;

	// L'en-tête à ajouter au début de chaque shader avant leur compilation
	// Cette chaîne reste constante pour un même driver, quel que soit le shader à compiler
	// Désactivé : Encore aucune information générale à envoyer aux shaders
	//core::stringc shadersHeader;

	// L'en-tête à ajouter au début de chaque shader de XEffects avant leur compilation
	// Cette chaîne reste constante pour un même driver, quel que soit le shader à compiler
	core::stringc xEffectsShadersHeader;
};

#endif
