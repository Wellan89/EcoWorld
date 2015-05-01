#ifndef DEF_C_GUI_POST_PROCESS_OPTIONS
#define DEF_C_GUI_POST_PROCESS_OPTIONS

#include "global.h"

using namespace gui;

// El�ment de la GUI permettant le param�tres des effets post-rendu du jeu
class CGUIPostProcessOptions : public IGUIElement
{
public:
	// Constructeur
	CGUIPostProcessOptions(IGUIEnvironment* environment, IGUIElement* parent, int id, const core::recti& rectangle);

	// Appel� lors d'un �v�nement
	virtual bool OnEvent(const SEvent& event);

	// Met � jour l'affichage du menu des effets post-rendu suivant la configuration du jeu sp�cifi�e
	void updateFromGameConfig(const GameConfiguration& gameCfg);

	// Met � jour la liste des effets post-rendu disponibles
	void updateAvalaibleEffectsList();

protected:
	// Met � jour la liste des effets post-rendu activ�s suivant la configuration du jeu sp�cifi�e
	void updateEnabledEffectsList(const GameConfiguration& gameCfg);

	// Met � jour ces �l�ments de la GUI suivant l'activation g�n�rale de PostProcess
	void updateEnabledElements(bool postProcessEnabled);



	// La liste des effets post-rendu activ�s
	core::list<core::stringw> enabledEffects;



	// Les �l�ments de la GUI permettant de g�rer les effets post-rendu :
	IGUICheckBox* postProcessEnabledCheckBox;	// La case � cocher indiquant si les effets post-rendu sont activ�s ou non
	IGUICheckBox* cameraShakingEnabledCheckBox;	// La case � cocher indiquant si les tremblements de la cam�ra lors de la destruction d'un b�timent sont activ�s ou non
	IGUIStaticText* avalaibleEffectsStaticText;	// Le texte de description au-dessus de la liste des effets post-rendu disponibles
	IGUIStaticText* enabledEffectsStaticText;	// Le texte de description au-dessus de la liste des effets post-rendu activ�s
	IGUIListBox* avalaibleEffectsListBox;		// La liste permettant d'afficher quels effets post-rendu sont disponibles
	IGUIListBox* enabledEffectsListBox;			// La liste permettant d'afficher quels effets post-rendu sont activ�s, ainsi que leur ordre de rendu
	IGUIButton* addButton;						// Le bouton pour ajouter un effet post-rendu � la fin de la liste des effets activ�s
	IGUIButton* removeButton;					// Le bouton pour supprimer un effet post-rendu de la liste des effets activ�s
	IGUIButton* putUpButton;					// Le bouton pour monter un effet post-rendu de la liste des effets activ�s
	IGUIButton* putDownButton;					// Le bouton pour descendre un effet post-rendu de la liste des effets activ�s



	// Fonctions permettant de g�rer les effets post-rendu :
	void setCurrentSelectedAvalaibleEffect(int avalaibleEffectIndex);	// Met � jour les �l�ments de la GUI suivant l'effet disponible actuellement s�lectionn�
	void setCurrentSelectedEnabledEffect(int enabledEffectIndex);		// Met � jour les �l�ments de la GUI suivant l'effet activ� actuellement s�lectionn�
	void addEffect(int avalaibleEffectIndex);							// Ajoute un effet post-rendu disponible � la liste des effets activ�s, juste apr�s l'effet actuellement s�lectionn�
	void removeEffect(int enabledEffectIndex);							// Supprime un effet post-rendu activ� de la liste des effets activ�s
	void putUpEffect(int enabledEffectIndex);							// Monte un effet post-rendu activ� dans la liste des effets activ�s
	void putDownEffect(int enabledEffectIndex);							// Descend un effet post-rendu activ� dans la liste des effets activ�s

public:
	// Met � jour ce menu suivant l'�tat d'activation des shaders
	void setShadersEnabled(bool shadersEnabled, bool canModifyValues)
	{
		if (postProcessEnabledCheckBox)
		{
			if (shadersEnabled)
			{
				postProcessEnabledCheckBox->setEnabled(true);
				updateEnabledElements(postProcessEnabledCheckBox->isChecked());
			}
			else
			{
				if (canModifyValues)
					postProcessEnabledCheckBox->setChecked(false);
				postProcessEnabledCheckBox->setEnabled(false);
				updateEnabledElements(false);
			}
		}
	}



	// Accesseurs inline :

	// Obtient la case � cocher indiquant si les effets post-rendu sont activ�s ou non
	const IGUICheckBox* getPostProcessEnabledCheckBox() const	{ return postProcessEnabledCheckBox; }

	// Obtient l'�tat d'activation des effets post-rendu
	bool getPostProcessEffectsEnabled() const
	{
		if (postProcessEnabledCheckBox)
			return postProcessEnabledCheckBox->isChecked();
		return false;
	}

	// Obtient l'�tat d'activation des tremblements de la cam�ra
	bool getCameraShakingEnabled() const
	{
		if (cameraShakingEnabledCheckBox)
			return cameraShakingEnabledCheckBox->isChecked();
		return false;
	}

	// D�termine si le rendu de la profondeur de la sc�ne est n�cessaire pour les effets activ�s
	bool needDepthPass() const;

	// Obtient la liste des effets post-rendu actuellement activ�s, dans leur ordre de rendu
	const core::list<core::stringw>& getEnabledEffects() const	{ return enabledEffects; }
};

#endif
