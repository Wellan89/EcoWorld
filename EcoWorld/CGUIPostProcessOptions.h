#ifndef DEF_C_GUI_POST_PROCESS_OPTIONS
#define DEF_C_GUI_POST_PROCESS_OPTIONS

#include "global.h"

using namespace gui;

// Elément de la GUI permettant le paramêtres des effets post-rendu du jeu
class CGUIPostProcessOptions : public IGUIElement
{
public:
	// Constructeur
	CGUIPostProcessOptions(IGUIEnvironment* environment, IGUIElement* parent, int id, const core::recti& rectangle);

	// Appelé lors d'un évènement
	virtual bool OnEvent(const SEvent& event);

	// Met à jour l'affichage du menu des effets post-rendu suivant la configuration du jeu spécifiée
	void updateFromGameConfig(const GameConfiguration& gameCfg);

	// Met à jour la liste des effets post-rendu disponibles
	void updateAvalaibleEffectsList();

protected:
	// Met à jour la liste des effets post-rendu activés suivant la configuration du jeu spécifiée
	void updateEnabledEffectsList(const GameConfiguration& gameCfg);

	// Met à jour ces éléments de la GUI suivant l'activation générale de PostProcess
	void updateEnabledElements(bool postProcessEnabled);



	// La liste des effets post-rendu activés
	core::list<core::stringw> enabledEffects;



	// Les éléments de la GUI permettant de gérer les effets post-rendu :
	IGUICheckBox* postProcessEnabledCheckBox;	// La case à cocher indiquant si les effets post-rendu sont activés ou non
	IGUICheckBox* cameraShakingEnabledCheckBox;	// La case à cocher indiquant si les tremblements de la caméra lors de la destruction d'un bâtiment sont activés ou non
	IGUIStaticText* avalaibleEffectsStaticText;	// Le texte de description au-dessus de la liste des effets post-rendu disponibles
	IGUIStaticText* enabledEffectsStaticText;	// Le texte de description au-dessus de la liste des effets post-rendu activés
	IGUIListBox* avalaibleEffectsListBox;		// La liste permettant d'afficher quels effets post-rendu sont disponibles
	IGUIListBox* enabledEffectsListBox;			// La liste permettant d'afficher quels effets post-rendu sont activés, ainsi que leur ordre de rendu
	IGUIButton* addButton;						// Le bouton pour ajouter un effet post-rendu à la fin de la liste des effets activés
	IGUIButton* removeButton;					// Le bouton pour supprimer un effet post-rendu de la liste des effets activés
	IGUIButton* putUpButton;					// Le bouton pour monter un effet post-rendu de la liste des effets activés
	IGUIButton* putDownButton;					// Le bouton pour descendre un effet post-rendu de la liste des effets activés



	// Fonctions permettant de gérer les effets post-rendu :
	void setCurrentSelectedAvalaibleEffect(int avalaibleEffectIndex);	// Met à jour les éléments de la GUI suivant l'effet disponible actuellement sélectionné
	void setCurrentSelectedEnabledEffect(int enabledEffectIndex);		// Met à jour les éléments de la GUI suivant l'effet activé actuellement sélectionné
	void addEffect(int avalaibleEffectIndex);							// Ajoute un effet post-rendu disponible à la liste des effets activés, juste après l'effet actuellement sélectionné
	void removeEffect(int enabledEffectIndex);							// Supprime un effet post-rendu activé de la liste des effets activés
	void putUpEffect(int enabledEffectIndex);							// Monte un effet post-rendu activé dans la liste des effets activés
	void putDownEffect(int enabledEffectIndex);							// Descend un effet post-rendu activé dans la liste des effets activés

public:
	// Met à jour ce menu suivant l'état d'activation des shaders
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

	// Obtient la case à cocher indiquant si les effets post-rendu sont activés ou non
	const IGUICheckBox* getPostProcessEnabledCheckBox() const	{ return postProcessEnabledCheckBox; }

	// Obtient l'état d'activation des effets post-rendu
	bool getPostProcessEffectsEnabled() const
	{
		if (postProcessEnabledCheckBox)
			return postProcessEnabledCheckBox->isChecked();
		return false;
	}

	// Obtient l'état d'activation des tremblements de la caméra
	bool getCameraShakingEnabled() const
	{
		if (cameraShakingEnabledCheckBox)
			return cameraShakingEnabledCheckBox->isChecked();
		return false;
	}

	// Détermine si le rendu de la profondeur de la scène est nécessaire pour les effets activés
	bool needDepthPass() const;

	// Obtient la liste des effets post-rendu actuellement activés, dans leur ordre de rendu
	const core::list<core::stringw>& getEnabledEffects() const	{ return enabledEffects; }
};

#endif
