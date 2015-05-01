#ifndef DEF_C_GUI_OPTIONS_MENU
#define DEF_C_GUI_OPTIONS_MENU

#include "global.h"

class CGUIMessageBox;
class CGUIPostProcessOptions;

using namespace gui;

// Classe du menu options : Menu de la GUI g�rant la configuration du jeu
class CGUIOptionsMenu : public IGUIElement
{
public:
	// Constructeur
	CGUIOptionsMenu(IGUIEnvironment* environment, IGUIElement* parent, int id, const core::recti& rectangle);

	// Appel� lorsqu'un �v�nement arrive sur un des �l�ments de ce menu
	virtual bool OnEvent(const SEvent& event);

	// Affiche/Masque ce menu
	virtual void setVisible(bool visible);

	// Appel� � chaque frame apr�s le rendu de cet �l�ment
	virtual void OnPostRender(u32 timeMs);

	// Affiche la fen�tre de confirmation des param�tres par l'utilisateur, pour v�rifier que le jeu est bien capable de se lancer avec ces param�tres
	void showConfirmationWindow();

	// Cr�e le menu options dans la GUI d'Irrlicht
	void createMenu();

	// Met � jour la liste des skins de la GUI disponibles et s�lectionne automatiquement le skin sp�cifi� (� appeler � chaque affichage de cette GUI)
	void updateGUISkinsList(const io::path& guiSkinFileToSelect = "");

	// Trouve le nom du skin de la GUI sp�cifi� avec l'adresse de son fichier de configuration
	core::stringw findGUISkinName(const io::path& guiSkinFilename);

	// Trouve l'adresse du fichier de configuration avec le nom du skin de la GUI sp�cifi� (�tape inverse de findGUISkinFilename)
	io::path findGUISkinFilename(const core::stringw& guiSkinName);

	// Actualise les param�tres du jeu avec les options choisies dans ce menu
	// canShowConfirmationWindow :	True si la fen�tre de confirmation des param�tres peut �tre affich�e apr�s l'application des nouveaux param�tres (si n�cessaire),
	//								False sinon (utile dans le cas o� d'anciens param�tres sont restaur�s)
	void applySelectedOptions(bool canShowConfirmationWindow = true);

	// Met � jour l'affichage du menu options suivant la configuration du jeu sp�cifi�e
	void updateFromGameConfig(const GameConfiguration& gameCfg);



	// Met � jour les �l�ments du menu options lorsque l'utilisateur a chang� un param�tre important dans ce menu :
	// canModifyValues :	False si les valeurs actuellement indiqu�es dans ce menu doivent �tre conserv�es, true si elles peuvent �tre modifi�es (par d�faut)

	// Met � jour ce menu lorsque le driver de rendu a �t� chang�
	void updateElements_driverChanged(bool canModifyValues = true);
	// Met � jour ce menu lorsque l'activation de l'audio a �t� chang�
	void updateElements_audioEnabledChanged();
	// Met � jour ce menu lorsque l'activation des shaders a �t� chang�
	void updateElements_shadersEnabledChanged(bool canModifyValues = true);
	// Met � jour ce menu lorsque l'activation du shader de l'eau a �t� chang�
	void updateElements_waterShaderEnabledChanged(bool canModifyValues = true);
	// Met � jour ce menu lorsque l'activation des effets post-rendu a chang�
	void updateElements_postProcessEnabledChanged(bool canModifyValues = true);

protected:
	// Les �l�ments de la GUI pour le menu options :

	// TODO : El�ments � ajouter :
	// - Qualit� des ombres XEffects (basse, normale, haute) : xEffectsUseVSMShadows, xEffectsUse32BitDepthBuffers, xEffectsShadowMapResolution ... (� revoir) :
	//		IGUIComboBox* xEffectsShadowsQualityComboBox;		// La liste des qualit�s des ombres de XEffects disponibles
	// - Options d'IrrKlang...

	IGUITabControl* mainTabGroup;				// Le groupe d'onglet principal du menu permettant d'afficher plusieurs �crans d'options
	IGUITab* generalTab;						// L'onglet g�rant les param�tres de cr�ation du device et graphiques, ainsi que ceux de la GUI et du jeu
	IGUITab* shadersTab;						// L'onglet g�rant les param�tres des shaders et des ombres (Irrlicht ou XEffects)
	IGUITab* postProcessTab;					// L'onglet g�rant les param�tres des effets post-rendu
	IGUITab* audioTab;							// L'onglet g�rant les param�tres audios

	IGUIComboBox* driverTypeComboBox;			// La liste des drivers disponibles
	IGUIComboBox* resolutionComboBox;			// La liste des r�solutions disponibles
	IGUIComboBox* antialiasingComboBox;			// La liste des qualit�s de l'antialiasing disponibles
	IGUICheckBox* fullScreenCheckBox;			// La case � cocher pour activer le mode plein �cran
	IGUICheckBox* vSyncCheckBox;				// La case � cocher pour activer la synchronisation verticale

	IGUIComboBox* texturesQualityComboBox;		// La liste des qualit�s de textures disponibles
	IGUIComboBox* texturesFilterComboBox;		// La liste des filtres de textures disponibles
	IGUICheckBox* powerOf2TexturesCheckBox;		// La case � cocher pour activer les textures en puissance de deux

	IGUIScrollBar* guiTransparencyScrollBar;	// La barre pour r�gler la transparence de la gui
	IGUIComboBox* guiSkinsComboBox;				// La liste des skins de la GUI disponibles

	IGUIScrollBar* gammaScrollBar;				// La barre pour r�gler la valeur du gamma pour toutes les composantes de couleur (RGB)

	IGUIComboBox* autoSaveFrequencyComboBox;	// La liste des fr�quences pour la sauvegarde automatique

	IGUICheckBox* stencilShadowsCheckBox;				// La case � cocher pour activer les ombres d'Irrlicht
	IGUICheckBox* shadersEnabledCheckBox;				// La case � cocher pour activer les shaders
	IGUICheckBox* skyShaderEnabledCheckBox;				// La case � cocher pour activer le shader de ciel
	IGUICheckBox* normalMappingShaderEnabledCheckBox;	// La case � cocher pour activer le shader de normal mapping
	IGUICheckBox* terrainShaderEnabledCheckBox;			// La case � cocher pour activer le shader de terrain
	IGUICheckBox* waterShaderEnabledCheckBox;			// La case � cocher pour activer le shader de l'eau
	IGUICheckBox* animatedWaterCheckBox;				// La case � cocher pour activer l'animation de la surface de l'eau
	IGUIStaticText* waterQualityTexte;					// Le texte � gauche de la liste des qualit�s de l'eau
	IGUIComboBox* waterShaderQualityComboBox;			// La liste des qualit�s de la r�flexion et de la transparence de l'eau (modifie la taille des textures RTT de l'eau)

	IGUICheckBox* xEffectsShadowsEnabledCheckBox;		// La case � cocher pour activer les ombres de XEffects

	CGUIPostProcessOptions* postProcessOptions;			// L'�l�ment de la GUI g�rant les options des effets post-rendu			

	IGUICheckBox* audioEnabledCheckBox;			// La case � cocher pour activer le mode audio
	IGUIStaticText* mainVolumeTexte;			// Le texte au dessus de la barre pour r�gler le volume audio principal
	IGUIScrollBar* mainVolumeScrollBar;			// La barre pour r�gler le volume audio principal
	IGUIStaticText* musicVolumeTexte;			// Le texte au dessus de la barre pour r�gler le volume de la musique
	IGUIScrollBar* musicVolumeScrollBar;		// La barre pour r�gler le volume de la musique
	IGUIStaticText* soundVolumeTexte;			// Le texte au dessus de la barre pour r�gler le volume des sons
	IGUIScrollBar* soundVolumeScrollBar;		// La barre pour r�gler le volume des sons
	
	CGUIMessageBox* confirmationWindow;			// La fen�tre qui s'affiche pour demander � l'utilisateur de confirmer les nouveaux param�tres appliqu�s (NULL si cette fen�tre n'est pas affich�e)
	u32 confirmationWindowShowRealTimeMs;		// Le temps r�el en milisecondes � laquelle la fen�tre de confirmation a �t� affich�e

public:	// Les boutons de ce menu sont rendus publics, car ils sont parfois utilis�s par la classe Game
	IGUIButton* appliquerBouton;				// Le bouton pour appliquer les changements
	IGUIButton* defautBouton;					// Le bouton pour remettre les options par d�faut
	IGUIButton* retablirBouton;					// Le bouton pour r�tablir les options actuelles dans ce menu
	IGUIButton* retourBouton;					// Le bouton pour revenir au menu principal

//public:
	// Fonctions inline :

	// R�initialise le menu options (r�initialise les pointeurs m�moire vers les �l�ments de cette GUI, mais ne les supprime pas directement de la GUI d'Irrlicht !)
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

	// Indique si la fen�tre de confirmation des param�tres est affich�e ou non
	bool isConfirmationWindowShown() const	{ return (confirmationWindow != NULL); }

	// Active l'onglet correspondant de ce menu
	void setActiveTab(int activeTab)
	{
		if (mainTabGroup)
			mainTabGroup->setActiveTab(activeTab);
	}
};

#endif
