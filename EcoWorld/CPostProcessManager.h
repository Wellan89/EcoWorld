#ifndef _POSTPROCESSMANAGER_H
#define _POSTPROCESSMANAGER_H

// Cette version a �t� tr�s largement modifi�e : simplifi�e, s�curis�e et optimis�e + Compilation retard�e des shaders (on ne compile pas tous les shaders d�s le chargement, mais d�s qu'ils sont n�cessaires)



// Le nom de l'effet � utiliser pour le rendu final sur l'�cran
#define EFFECT_NAME_FINAL	L"Render To Screen"

// Le nom de l'effet � utiliser pour l'effet de tremblement de la cam�ra (cet effet devra imp�rativement �tre d�fini dans PostProcessEffectConfig.xml)
#define EFFECT_NAME_SHAKE	L"Shaking"

// Le nom de la texture RTT d'entr�e par d�faut pour les effets post-rendu
#define RTT_NAME_IN			L"auxIn"

// Le nom de la texture RTT de sortie par d�faut pour les effets post-rendu
#define RTT_NAME_OUT		L"auxOut"

// Le nom de la texture RTT dans laquelle sera rendue la profondeur de la sc�ne
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

	// Ajout� : True si les buffers auxIn et auxOut devront �tre �chang�s avant le rendu du prochain effet, false sinon
	bool swapBuffers;

public:
	// set the name of the effect
	void setName(const core::stringw& Name)	{ name = Name; }

	// get the name of the effect
	const core::stringw& getName() const	{ return name; }

	// Ajout� :
	CEffectChain() : swapBuffers(true)		{ }

	void setSwapBuffers(bool swap)			{ swapBuffers = swap; }
	bool getSwapBuffers() const				{ return swapBuffers; }
};

// Ajout� :
// Structure d'informations sur une texture RTT, utilis�e pour sa cr�ation retard�e
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

	// Ajout� : Informations sur les textures RTT pour leur cr�ation d�cal�e
	core::map<core::stringw, RTTInfos> RTTInfosMap;

	// the effect chains that store the post process passes (defined in effect.xml)
	//CEffectChain EffectChain[EPPE_COUNT];
	core::map<core::stringw, CEffectChain*> EffectChain;
	
	// list of nodes for depth pass
	core::array<scene::ISceneNode*> DepthPassNodes;

	// material for depth pass
	CShaderMaterial* DepthMaterial;

	// Ajout� : Nombre maximal de textures RTT cibles pour un m�me rendu support� par le driver
	const u32 maxMRTs;

	// Ajout� : Pr�processeur de shaders
	CShaderPreprocessor* shaderPreprocessor;

	// Ajout� : Ecran de rendu
	CScreenQuad* screenQuad;

	// performs aux buffer swaping
	void SwapAuxBuffers();

	// loads the rtt configuration and fills the RenderTargetMap
	void loadRTTConfig();

	// loads the effects configuration and fills the EffectChain
	void loadEffectConfig();
	
	// True si l'effet de rendu sur �cran a d�j� �t� affich� durant cette frame, false sinon.
	// S'il a d�j� �t� affich�, cet effet ne sera pas � nouveau automatiquement � l'appel de la fonction CPostProcessManager::update() :
	// il est ainsi possible d'afficher un r�sultat sur l'�cran, puis de r�-appliquer des effets post-rendu non visibles apr�s (ce dernier r�sultat pouvant �tre utilis� pour les frames suivantes par exemple)
	bool alreadyRendered;

public:
	// prepares the postprocessing by setting the aux0 buffer as the scene render target
	// use this function before calling smgr->drawAll()
	// Ajout� : retourne la texture de rendu (RTT) dans laquelle devra �tre affich�e la sc�ne
	video::ITexture* prepare();

	// renders the desired effect
	// Modifi� : Prend maintenant un core::stringw en param�tre sp�cifiant le nom de l'effet � rendre sur l'�cran
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



	// Ajout� :

	// Obtient le pr�processeur de shader
	CShaderPreprocessor* getShaderPreprocessor()								{ return shaderPreprocessor; }

	// Obtient la liste des effets disponibles
	const core::map<core::stringw, CEffectChain*>& getAvalaibleEffects() const	{ return EffectChain; }

	// Compile et v�rifie les shaders particuliers : shader permettant le rendu final sur l'�cran, shader de tremblement de la cam�ra et shader permettant le rendu de la profondeur de la sc�ne ;
	// et cr�e toutes les textures n�cessaires
	void compileParticularShaders(bool compileShakeEffect, bool compileDepthMaterial);

	// Compile et v�rifie tous les effets sp�cifi�s, et cr�e toutes les textures n�cessaires
	void compileEffects(const core::array<core::stringw>& effects);

	// Compile et v�rifie l'effet sp�cifi�, et cr�e toutes les textures n�cessaires
	void compileEffect(const core::stringw& effect);

	// D�termine si le nom d'une texture correspond � une texture RTT ou non
	bool isRTTTexture(const core::stringw& id) const							{ return (RTTInfosMap.find(id) != NULL); }

	// Cr�e/Obtient une texture RTT d'apr�s son id
	// allowCreateTexture :	True si la texture devra �tre cr��e si elle n'a pas d�j� �t� trouv�e dans la liste des textures existantes, false sinon
	video::ITexture* getRTTTexture(const core::stringw& id, bool allowCreateTexture = true);

	// D�termine si un effet n�cessite le rendu de la profondeur de la sc�ne (depth pass)
	bool needDepthPass(const core::stringw& effect);
};

#endif
