#ifndef DEF_C_GUI_OPTIONS_MENU
#define DEF_C_GUI_OPTIONS_MENU

#include "global.h"

class CGUIMessageBox;
class CGUIPostProcessOptions;

using namespace gui;

// Classe du menu options : Menu de la GUI gérant la configuration du jeu
class CGUIOptionsMenu : public IGUIElement
{
public:
	// Constructeur
	CGUIOptionsMenu(IGUIEnvironment* environment, IGUIElement* parent, int id, const core::recti& rectangle);

	// Appelé lorsqu'un évènement arrive sur un des éléments de ce menu
	virtual bool OnEvent(const SEvent& event);

	// Affiche/Masque ce menu
	virtual void setVisible(bool visible);

	// Appelé à chaque frame après le rendu de cet élément
	virtual void OnPostRender(u32 timeMs);

	// Affiche la fenêtre de confirmation des paramêtres par l'utilisateur, pour vérifier que le jeu est bien capable de se lancer avec ces paramêtres
	void showConfirmationWindow();

	// Crée le menu options dans la GUI d'Irrlicht
	void createMenu();

	// Met à jour la liste des skins de la GUI disponibles et sélectionne automatiquement le skin spécifié (à appeler à chaque affichage de cette GUI)
	void updateGUISkinsList(const io::path& guiSkinFileToSelect = "");

	// Trouve le nom du skin de la GUI spécifié avec l'adresse de son fichier de configuration
	core::stringw findGUISkinName(const io::path& guiSkinFilename);

	// Trouve l'adresse du fichier de configuration avec le nom du skin de la GUI spécifié (étape inverse de findGUISkinFilename)
	io::path findGUISkinFilename(const core::stringw& guiSkinName);

	// Actualise les paramêtres du jeu avec les options choisies dans ce menu
	// canShowConfirmationWindow :	True si la fenêtre de confirmation des paramêtres peut être affichée après l'application des nouveaux paramêtres (si nécessaire),
	//								False sinon (utile dans le cas où d'anciens paramêtres sont restaurés)
	void applySelectedOptions(bool canShowConfirmationWindow = true);

	// Met à jour l'affichage du menu options suivant la configuration du jeu spécifiée
	void updateFromGameConfig(const GameConfiguration& gameCfg);



	// Met à jour les éléments du menu options lorsque l'utilisateur a changé un paramêtre important dans ce menu :
	// canModifyValues :	False si les valeurs actuellement indiquées dans ce menu doivent être conservées, true si elles peuvent être modifiées (par défaut)

	// Met à jour ce menu lorsque le driver de rendu a été changé
	void updateElements_driverChanged(bool canModifyValues = true);
	// Met à jour ce menu lorsque l'activation de l'audio a été changé
	void updateElements_audioEnabledChanged();
	// Met à jour ce menu lorsque l'activation des shaders a été changé
	void updateElements_shadersEnabledChanged(bool canModifyValues = true);
	// Met à jour ce menu lorsque l'activation du shader de l'eau a été changé
	void updateElements_waterShaderEnabledChanged(bool canModifyValues = true);
	// Met à jour ce menu lorsque l'activation des effets post-rendu a changé
	void updateElements_postProcessEnabledChanged(bool canModifyValues = true);

protected:
	// Les éléments de la GUI pour le menu options :

	// TODO : Eléments à ajouter :
	// - Qualité des ombres XEffects (basse, normale, haute) : xEffectsUseVSMShadows, xEffectsUse32BitDepthBuffers, xEffectsShadowMapResolution ... (à revoir) :
	//		IGUIComboBox* xEffectsShadowsQualityComboBox;		// La liste des qualités des ombres de XEffects disponibles
	// - Options d'IrrKlang...

	IGUITabControl* mainTabGroup;				// Le groupe d'onglet principal du menu permettant d'afficher plusieurs écrans d'options
	IGUITab* generalTab;						// L'onglet gérant les paramêtres de création du device et graphiques, ainsi que ceux de la GUI et du jeu
	IGUITab* shadersTab;						// L'onglet gérant les paramêtres des shaders et des ombres (Irrlicht ou XEffects)
	IGUITab* postProcessTab;					// L'onglet gérant les paramêtres des effets post-rendu
	IGUITab* audioTab;							// L'onglet gérant les paramêtres audios

	IGUIComboBox* driverTypeComboBox;			// La liste des drivers disponibles
	IGUIComboBox* resolutionComboBox;			// La liste des résolutions disponibles
	IGUIComboBox* antialiasingComboBox;			// La liste des qualités de l'antialiasing disponibles
	IGUICheckBox* fullScreenCheckBox;			// La case à cocher pour activer le mode plein écran
	IGUICheckBox* vSyncCheckBox;				// La case à cocher pour activer la synchronisation verticale

	IGUIComboBox* texturesQualityComboBox;		// La liste des qualités de textures disponibles
	IGUIComboBox* texturesFilterComboBox;		// La liste des filtres de textures disponibles
	IGUICheckBox* powerOf2TexturesCheckBox;		// La case à cocher pour activer les textures en puissance de deux

	IGUIScrollBar* guiTransparencyScrollBar;	// La barre pour régler la transparence de la gui
	IGUIComboBox* guiSkinsComboBox;				// La liste des skins de la GUI disponibles

	IGUIScrollBar* gammaScrollBar;				// La barre pour régler la valeur du gamma pour toutes les composantes de couleur (RGB)

	IGUIComboBox* autoSaveFrequencyComboBox;	// La liste des fréquences pour la sauvegarde automatique

	IGUICheckBox* stencilShadowsCheckBox;				// La case à cocher pour activer les ombres d'Irrlicht
	IGUICheckBox* shadersEnabledCheckBox;				// La case à cocher pour activer les shaders
	IGUICheckBox* skyShaderEnabledCheckBox;				// La case à cocher pour activer le shader de ciel
	IGUICheckBox* normalMappingShaderEnabledCheckBox;	// La case à cocher pour activer le shader de normal mapping
	IGUICheckBox* terrainShaderEnabledCheckBox;			// La case à cocher pour activer le shader de terrain
	IGUICheckBox* waterShaderEnabledCheckBox;			// La case à cocher pour activer le shader de l'eau
	IGUICheckBox* animatedWaterCheckBox;				// La case à cocher pour activer l'animation de la surface de l'eau
	IGUIStaticText* waterQualityTexte;					// Le texte à gauche de la liste des qualités de l'eau
	IGUIComboBox* waterShaderQualityComboBox;			// La liste des qualités de la réflexion et de la transparence de l'eau (modifie la taille des textures RTT de l'eau)

	IGUICheckBox* xEffectsShadowsEnabledCheckBox;		// La case à cocher pour activer les ombres de XEffects

	CGUIPostProcessOptions* postProcessOptions;			// L'élément de la GUI gérant les options des effets post-rendu			

	IGUICheckBox* audioEnabledCheckBox;			// La case à cocher pour activer le mode audio
	IGUIStaticText* mainVolumeTexte;			// Le texte au dessus de la barre pour régler le volume audio principal
	IGUIScrollBar* mainVolumeScrollBar;			// La barre pour régler le volume audio principal
	IGUIStaticText* musicVolumeTexte;			// Le texte au dessus de la barre pour régler le volume de la musique
	IGUIScrollBar* musicVolumeScrollBar;		// La barre pour régler le volume de la musique
	IGUIStaticText* soundVolumeTexte;			// Le texte au dessus de la barre pour régler le volume des sons
	IGUIScrollBar* soundVolumeScrollBar;		// La barre pour régler le volume des sons
	
	CGUIMessageBox* confirmationWindow;			// La fenêtre qui s'affiche pour demander à l'utilisateur de confirmer les nouveaux paramêtres appliqués (NULL si cette fenêtre n'est pas affichée)
	u32 confirmationWindowShowRealTimeMs;		// Le temps réel en milisecondes à laquelle la fenêtre de confirmation a été affichée

public:	// Les boutons de ce menu sont rendus publics, car ils sont parfois utilisés par la classe Game
	IGUIButton* appliquerBouton;				// Le bouton pour appliquer les changements
	IGUIButton* defautBouton;					// Le bouton pour remettre les options par défaut
	IGUIButton* retablirBouton;					// Le bouton pour rétablir les options actuelles dans ce menu
	IGUIButton* retourBouton;					// Le bouton pour revenir au menu principal

//public:
	// Fonctions inline :

	// Réinitialise le menu options (réinitialise les pointeurs mémoire vers les éléments de cette GUI, mais ne les supprime pas directement de la GUI d'Irrlicht !)
	void reset()
	{
		mainTabGroup = NULL;
		generalTab = NULL;
		shadersTab = NULL;
		postProcessTab = NULL;
		audioTab = NULL;

		driverTypeComboBox = NULL;
		resolutionComboBox = NULL;
		antialiasingComboBox = NULL;
		fullScreenCheckBox = NULL;
		vSyncCheckBox = NULL;

		texturesQualityComboBox = NULL;
		texturesFilterComboBox = NULL;
		powerOf2TexturesCheckBox = NULL;

		guiTransparencyScrollBar = NULL;
		guiSkinsComboBox = NULL;

		gammaScrollBar = NULL;

		autoSaveFrequencyComboBox = NULL;

		stencilShadowsCheckBox = NULL;
		shadersEnabledCheckBox = NULL;
		skyShaderEnabledCheckBox = NULL;
		normalMappingShaderEnabledCheckBox = NULL;
		terrainShaderEnabledCheckBox = NULL;
		waterShaderEnabledCheckBox = NULL;
		animatedWaterCheckBox = NULL;
		waterQualityTexte = NULL;
		waterShaderQualityComboBox = NULL;

		xEffectsShadowsEnabledCheckBox = NULL;

		postProcessOptions = NULL;

		audioEnabledCheckBox = NULL;
		mainVolumeTexte = NULL;
		mainVolumeScrollBar = NULL;
		musicVolumeTexte = NULL;
		musicVolumeScrollBar = NULL;
		soundVolumeTexte = NULL;
		soundVolumeScrollBar = NULL;

		confirmationWindow = NULL;
		confirmationWindowShowRealTimeMs = 0;

		appliquerBouton = NULL;
		defautBouton = NULL;
		retablirBouton = NULL;
		retourBouton = NULL;
	}

	// Indique si la fenêtre de confirmation des paramêtres est affichée ou non
	bool isConfirmationWindowShown() const	{ return (confirmationWindow != NULL); }

	// Active l'onglet correspondant de ce menu
	void setActiveTab(int activeTab)
	{
		if (mainTabGroup)
			mainTabGroup->setActiveTab(activeTab);
	}
};

#endif
