#ifndef _POSTPROCESSMANAGER_H
#define _POSTPROCESSMANAGER_H

// Cette version a été très largement modifiée : simplifiée, sécurisée et optimisée + Compilation retardée des shaders (on ne compile pas tous les shaders dès le chargement, mais dès qu'ils sont nécessaires)



// Le nom de l'effet à utiliser pour le rendu final sur l'écran
#define EFFECT_NAME_FINAL	L"Render To Screen"

// Le nom de l'effet à utiliser pour l'effet de tremblement de la caméra (cet effet devra impérativement être défini dans PostProcessEffectConfig.xml)
#define EFFECT_NAME_SHAKE	L"Shaking"

// Le nom de la texture RTT d'entrée par défaut pour les effets post-rendu
#define RTT_NAME_IN			L"auxIn"

// Le nom de la texture RTT de sortie par défaut pour les effets post-rendu
#define RTT_NAME_OUT		L"auxOut"

// Le nom de la texture RTT dans laquelle sera rendue la profondeur de la scène
#define RTT_NAME_DEPTH		L"rttDepth"



#include "global.h"

class CShaderPreprocessor;
class CScreenQuad;
class CShaderMaterial;

class CEffectChain : public core::array<CShaderMaterial*>
{
protected:
	// name of the effect
	core::stringw name;

	// Ajouté : True si les buffers auxIn et auxOut devront être échangés avant le rendu du prochain effet, false sinon
	bool swapBuffers;

public:
	// set the name of the effect
	void setName(const core::stringw& Name)	{ name = Name; }

	// get the name of the effect
	const core::stringw& getName() const	{ return name; }

	// Ajouté :
	CEffectChain() : swapBuffers(true)		{ }

	void setSwapBuffers(bool swap)			{ swapBuffers = swap; }
	bool getSwapBuffers() const				{ return swapBuffers; }
};

// Ajouté :
// Structure d'informations sur une texture RTT, utilisée pour sa création retardée
struct RTTInfos
{
	core::dimension2du size;
	video::ECOLOR_FORMAT colorFormat;

	RTTInfos()
		: size(64, 64), colorFormat(video::ECF_A8R8G8B8)	{ }
	RTTInfos(u32 width, u32 height, video::ECOLOR_FORMAT ColorFormat)
		: size(width, height), colorFormat(ColorFormat)		{ }
};

class CPostProcessManager
{
public:
	// constructor
	CPostProcessManager(IrrlichtDevice* Device, CShaderPreprocessor* ShaderPreprocessor, CScreenQuad* ScreenQuad);

	// destructor
	~CPostProcessManager();

protected:
	// the irrlicht device
	IrrlichtDevice* device;
	video::IVideoDriver* driver;

	// additional render target textures (defined in rtt.xml)
	core::map<core::stringw, video::ITexture*> RenderTargetMap;

	// Ajouté : Informations sur les textures RTT pour leur création décalée
	core::map<core::stringw, RTTInfos> RTTInfosMap;

	// the effect chains that store the post process passes (defined in effect.xml)
	//CEffectChain EffectChain[EPPE_COUNT];
	core::map<core::stringw, CEffectChain*> EffectChain;
	
	// list of nodes for depth pass
	core::array<scene::ISceneNode*> DepthPassNodes;

	// material for depth pass
	CShaderMaterial* DepthMaterial;

	// Ajouté : Nombre maximal de textures RTT cibles pour un même rendu supporté par le driver
	const u32 maxMRTs;

	// Ajouté : Préprocesseur de shaders
	CShaderPreprocessor* shaderPreprocessor;

	// Ajouté : Ecran de rendu
	CScreenQuad* screenQuad;

	// performs aux buffer swaping
	void SwapAuxBuffers();

	// loads the rtt configuration and fills the RenderTargetMap
	void loadRTTConfig();

	// loads the effects configuration and fills the EffectChain
	void loadEffectConfig();
	
	// True si l'effet de rendu sur écran a déjà été affiché durant cette frame, false sinon.
	// S'il a déjà été affiché, cet effet ne sera pas à nouveau automatiquement à l'appel de la fonction CPostProcessManager::update() :
	// il est ainsi possible d'afficher un résultat sur l'écran, puis de ré-appliquer des effets post-rendu non visibles après (ce dernier résultat pouvant être utilisé pour les frames suivantes par exemple)
	bool alreadyRendered;

public:
	// prepares the postprocessing by setting the aux0 buffer as the scene render target
	// use this function before calling smgr->drawAll()
	// Ajouté : retourne la texture de rendu (RTT) dans laquelle devra être affichée la scène
	video::ITexture* prepare();

	// renders the desired effect
	// Modifié : Prend maintenant un core::stringw en paramêtre spécifiant le nom de l'effet à rendre sur l'écran
	//void render(E_POSTPROCESS_EFFECT effect);
	void render(const core::stringw& effect);

	// performs depth and normal generation pass
	void renderDepth(const video::SColor& defaultDepth = video::SColor(0xffffffff));

	// renders the aux buffer to the framebuffer
	// call this function after applying all desired effects and before calling gui->drawAll()
	void update();
	
	// removes all nodes from depth pass
	void clearDepthPass();

	// adds node to the depth pass
	void addNodeToDepthPass(scene::ISceneNode *node);

	// removes a node from the depth pass
	void removeNodeFromDepthPass(scene::ISceneNode *node);



	// Ajouté :

	// Obtient le préprocesseur de shader
	CShaderPreprocessor* getShaderPreprocessor()								{ return shaderPreprocessor; }

	// Obtient la liste des effets disponibles
	const core::map<core::stringw, CEffectChain*>& getAvalaibleEffects() const	{ return EffectChain; }

	// Compile et vérifie les shaders particuliers : shader permettant le rendu final sur l'écran, shader de tremblement de la caméra et shader permettant le rendu de la profondeur de la scène ;
	// et crée toutes les textures nécessaires
	void compileParticularShaders(bool compileShakeEffect, bool compileDepthMaterial);

	// Compile et vérifie tous les effets spécifiés, et crée toutes les textures nécessaires
	void compileEffects(const core::array<core::stringw>& effects);

	// Compile et vérifie l'effet spécifié, et crée toutes les textures nécessaires
	void compileEffect(const core::stringw& effect);

	// Détermine si le nom d'une texture correspond à une texture RTT ou non
	bool isRTTTexture(const core::stringw& id) const							{ return (RTTInfosMap.find(id) != NULL); }

	// Crée/Obtient une texture RTT d'après son id
	// allowCreateTexture :	True si la texture devra être créée si elle n'a pas déjà été trouvée dans la liste des textures existantes, false sinon
	video::ITexture* getRTTTexture(const core::stringw& id, bool allowCreateTexture = true);

	// Détermine si un effet nécessite le rendu de la profondeur de la scène (depth pass)
	bool needDepthPass(const core::stringw& effect);
};

#endif
