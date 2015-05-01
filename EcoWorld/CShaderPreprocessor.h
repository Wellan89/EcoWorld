#ifndef DEF_SHADER_PREPROCESSOR
#define DEF_SHADER_PREPROCESSOR

#include "global.h"

// Simple pr�processeur de shader appliqu� � tous les shaders avant leur compilation par DirectX ou OpenGL
// En pratique, cette classe ajoute simplement certaines d�finitions de pr�processeur (d�clar�es par des directives "#define") au d�but des shaders d'apr�s les param�tres du driver actuels
class CShaderPreprocessor
{
public:
	// Constructeur inline
	CShaderPreprocessor(io::IFileSystem* FileSystem, video::IGPUProgrammingServices* Gpu) : fileSystem(FileSystem), gpu(Gpu)
	{
		// G�n�re l'en-t�te g�n�ral des shaders
		//generateShadersHeader();

		// G�n�re l'en-t�te de XEffects
		generateXEffectsShadersHeader();
	}

	// Tra�te un shader depuis un fichier puis le compile
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

	// G�n�re l'en-t�te g�n�ral des shaders
	// A appeler d�s la cr�ation du driver d'Irrlicht et son param�trage
	//void generateShadersHeader();

	// G�n�re l'en-t�te pour les shaders de XEffects
	// A appeler une fois que la configuration du jeu a �t� charg�e ou modifi�e
	void generateXEffectsShadersHeader();

	// G�n�re l'en-t�te complet d'un certain shader
	// Note : Efface auparavant la cha�ne outHeader (ne l'ajoute pas � sa fin !)
	void generateFullShaderHeader(core::stringc& outHeader, video::E_VERTEX_SHADER_TYPE vsCompileTarget, video::E_PIXEL_SHADER_TYPE psCompileTarget,
		bool xEffectsShader, bool xEffectsVerticalVSMBlur);

protected:
	// Charge le texte contenu par un shader depuis un fichier : ajoute ce texte � la fin de la cha�ne de caract�res outShader (n'efface pas cette cha�ne !)
	// Attention : le cas filename = outShader (la m�me cha�ne de caract�re est pass�e en argument : ex : getShaderFile(shader, shader); ) n'est pas autoris� !
	void getShaderFile(const io::path& filename, core::stringc& outShader);

	// Le syst�me de fichier d'Irrlicht
	io::IFileSystem* fileSystem;

	// L'interface de shaders d'Irrlicht
	video::IGPUProgrammingServices* gpu;

	// L'en-t�te � ajouter au d�but de chaque shader avant leur compilation
	// Cette cha�ne reste constante pour un m�me driver, quel que soit le shader � compiler
	// D�sactiv� : Encore aucune information g�n�rale � envoyer aux shaders
	//core::stringc shadersHeader;

	// L'en-t�te � ajouter au d�but de chaque shader de XEffects avant leur compilation
	// Cette cha�ne reste constante pour un m�me driver, quel que soit le shader � compiler
	core::stringc xEffectsShadersHeader;
};

#endif
