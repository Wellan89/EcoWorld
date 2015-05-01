#include "CGUIOptionsMenu.h"
#include "CGUIMessageBox.h"
#include "CGUITexturedModalScreen.h"
#include "CGUIPostProcessOptions.h"
#include "GUIManager.h"
#include "Game.h"	// Pour avoir acc�s au timer du jeu

CGUIOptionsMenu::CGUIOptionsMenu(IGUIEnvironment* environment, IGUIElement* parent, int id, const core::recti& rectangle)
 : IGUIElement(EGUIET_ELEMENT, environment, parent, id, rectangle)
{
	// Initialise le menu options
	reset();

	// Cr�e ce menu dans la GUI d'Irrlicht
	createMenu();
}
bool CGUIOptionsMenu::OnEvent(const SEvent& event)
{
	// On ne g�re ici que les �v�nements de la GUI
	if (IsVisible && event.EventType == EET_GUI_EVENT && event.GUIEvent.Caller)
	{
		if (event.GUIEvent.EventType == EGET_BUTTON_CLICKED)	// Appui sur un bouton
		{
			if (event.GUIEvent.Caller == retourBouton)
			{
				// Appui sur le bouton "Retour" :

				// Retourne � la sc�ne du menu principal
				game->switchToNextScene(Game::ECS_MAIN_MENU);
			}
			else if (event.GUIEvent.Caller == retablirBouton)
			{
				// Appui sur le bouton "R�tablir" :

				// Actualise le menu options avec les valeurs actuelles de la configuration du jeu
				updateFromGameConfig(gameConfig);
			}
			else if (event.GUIEvent.Caller == defautBouton)
			{
				// Appui sur le bouton "Par d�faut" :

				// Actualise le menu options avec les valeurs par d�faut de la configuration du jeu
				updateFromGameConfig(GameConfiguration());
			}
			else if (event.GUIEvent.Caller == appliquerBouton)
			{
				// Appui sur le bouton "Appliquer" :

				// Applique les options s�lectionn�es
				applySelectedOptions();
			}
		}
		else if (event.GUIEvent.Caller == confirmationWindow)	// Ev�nement de la fen�tre de confirmation des param�tres
		{
			if (event.GUIEvent.EventType == EGET_MESSAGEBOX_CANCEL || event.GUIEvent.EventType == EGET_MESSAGEBOX_NO
				|| event.GUIEvent.EventType == EGET_MESSAGEBOX_OK || event.GUIEvent.EventType == EGET_MESSAGEBOX_YES)
			{
				// Si l'utilisateur a cliqu� sur un bouton (le bouton Oui est inclus) :
				// On indique que la fen�tre de confirmation est ferm�e
				confirmationWindow = NULL;

				if (event.GUIEvent.EventType != EGET_MESSAGEBOX_YES)
				{
					// Si on n'a pas cliqu� sur le bouton "Oui" :
					// Restaure les anciennes options : r�tabli la GUI du menu options avec les anciennes options, puis applique ces options (sans afficher � nouveau la fen�tre de confirmation)
					updateFromGameConfig(lastGameConfig);
					applySelectedOptions(false);
				}

				// Indique � la configuration du jeu qu'il n'est plus utile de sauvegarder les anciennes options de configuration
				gameState.saveLastGameConfig = false;

				return true;
			}
		}
		else if (event.GUIEvent.EventType == EGET_COMBO_BOX_CHANGED && event.GUIEvent.Caller == driverTypeComboBox)
		{
			// Changement du type de driver :

			// Actualise ce menu avec le nouveau driver
			updateElements_driverChanged();
		}
		else if (event.GUIEvent.EventType == EGET_CHECKBOX_CHANGED)
		{
			if (event.GUIEvent.Caller == audioEnabledCheckBox)
			{
				// Changement de l'activation de l'audio :

				// Actualise ce menu avec l'activation actuelle de l'audio
				updateElements_audioEnabledChanged();
			}
			else if (event.GUIEvent.Caller == shadersEnabledCheckBox)
			{
				// Changement de l'activation des shaders :

				// Actualise ce menu avec l'activation actuelle des shaders
				updateElements_shadersEnabledChanged();
			}
			else if (event.GUIEvent.Caller == waterShaderEnabledCheckBox)
			{
				// Changement de l'activation du shader de l'eau :

				// Actualise ce menu avec l'activation actuelle du shader de l'eau
				updateElements_waterShaderEnabledChanged();
			}
			else if (postProcessOptions && event.GUIEvent.Caller == postProcessOptions->getPostProcessEnabledCheckBox())
			{
				// Changement de l'activation des effets post-rendu :

				// Actualise ce menu avec l'activation actuelle des effets post-rendu
				updateElements_postProcessEnabledChanged();
			}
		}
	}

	return IGUIElement::OnEvent(event);
}
void CGUIOptionsMenu::OnPostRender(u32 timeMs)
{
	if (IsVisible && confirmationWindow)
	{
		// V�rifie que les 15 secondes imparties � l'utilisateur pour confirmer le choix de ces param�tres ne sont pas �coul�es
		const u32 diffTimeMs = game->deviceTimer->getRealTime() - confirmationWindowShowRealTimeMs;
		if (diffTimeMs > 15000)
		{
			// Ferme cette bo�te de dialogue
			confirmationWindow->remove();
			confirmationWindow = NULL;

			// Restaure les anciennes options : r�tabli la GUI du menu options avec les anciennes options, puis applique ces options (sans afficher � nouveau la fen�tre de confirmation)
			updateFromGameConfig(lastGameConfig);
			applySelectedOptions(false);

			// Indique � la configuration du jeu qu'il n'est plus utile de sauvegarder les anciennes options de configuration
			gameState.saveLastGameConfig = false;
		}
		else
		{
			// Met � jour le texte de la fen�tre de confirmation avec le temps r�ellement �coul�
			swprintf_SS(L"Voulez-vous conserver ces param�tres ?\r\nLes anciens param�tres seront r�tablis dans %u secondes.", 15 - (diffTimeMs / 1000));
			confirmationWindow->getStaticText()->setText(textW_SS);
		}
	}

	IGUIElement::OnPostRender(timeMs);
}
void CGUIOptionsMenu::showConfirmationWindow()
{
	if (confirmationWindow)
		return;

	// Cr�e la fen�tre de confirmation des param�tres actuels, avec un fond modal obscurci
	confirmationWindow = CGUIMessageBox::addMessageBox(Environment, L"Confirmation des param�tres",
		L"Voulez-vous conserver ces param�tres ?\r\nLes anciens param�tres seront r�tablis dans 15 secondes.",
		EMBF_YES | EMBF_NO, this, true);

	// Indique le temps r�el auquel cette fen�tre de confirmation a �t� cr��e
	confirmationWindowShowRealTimeMs = game->deviceTimer->getRealTime();

	// Bug fix :
	// Lorsque la fen�tre d'Irrlicht est ferm�e alors que la fen�tre de confirmation des param�tres est affich�e,
	// la configuration du jeu qui est sauvegard�e lors de sa fermeture est celle qui est actuellement en train d'�tre test�e.
	// Ainsi, on indique � la configuration du jeu d'enregistrer les anciennes options de configuration au cas o� la fen�tre du jeu serait ferm�e.
	gameState.saveLastGameConfig = true;
}
void CGUIOptionsMenu::setVisible(bool visible)
{
	if (visible)
	{
		// Ce menu est affich� :

		// Met � jour la liste des skins disponibles pour la GUI et s�lectionne le skin actuel
		updateGUISkinsList(gameConfig.guiSkinFile);

		// Actualise ce menu avec les valeurs actuelles de gameConfig
		updateFromGameConfig(gameConfig);
	}
	IsVisible = visible;
}
void CGUIOptionsMenu::createMenu()
{
	const core::recti parentRect = Parent->getRelativePosition();

	// Constantes pour simplifier la modification de la taille/position des �l�ments de ce menu
	const float
		positionTexteX = 0.03f,
		tailleTexteX = 0.27f,
		positionValeurX = 0.3f,
		tailleValeurX = 0.3f,

		minY = 0.03f,
		ecartY = 0.02f,
		tailleY = 0.04f,

		soundsX = 0.05f,
		soundsMaxX = 0.95f,
		soundsEcartX = 0.045f,
		soundsTailleTexteX = 0.27f,
		soundsTailleScrollBarX = 0.04f,
		sounsPositionY = 0.05f,
		soundsTailleScrollBarY = 0.8f,

		positionMainBoutonsY = 0.85f,
		tailleMainBoutonsX = 0.1f,
		tailleMainBoutonsY = 0.05f;

	const float
		soundsEcartScrollBarX = (soundsTailleTexteX - soundsTailleScrollBarX) * 0.5f,
		halfTailleMainBoutonsX = tailleMainBoutonsX * 0.5f;
	int indexY = 0;	// Utilis� pour simplifier l'ajout d'options dans le menu graphique



	// Cr�e le groupe d'onglets principal et ses onglets
	mainTabGroup = Environment->addTabControl(getAbsoluteRectangle(0.03f, 0.03f, 0.97f, 0.82f, Parent->getRelativePosition()), this, true, true);

	// R�gle la taille des onglets (ne fonctionne pas encore)	// TODO : Revoir ces valeurs pour que cela fonctionne quelle que soit la r�solution choisie
	mainTabGroup->setTabExtraWidth((int)(0.1f * (float)parentRect.getWidth()));	// Valeur par d�faut : 20
	mainTabGroup->setTabHeight((int)(0.03f * (float)parentRect.getHeight()));	// Valeur par d�faut : 32

	// Cr�e les onglets
	generalTab = mainTabGroup->addTab(L"G�n�ral");
	shadersTab = mainTabGroup->addTab(L"Shaders et ombres");
	postProcessTab = mainTabGroup->addTab(L"Effets post-rendu");
	audioTab = mainTabGroup->addTab(L"Audio");
	const core::recti parentTabRect = generalTab->getRelativePosition();



	// Partie g�n�ral/graphique :
	Environment->addStaticText(L"Type de driver :",		getAbsoluteRectangle(positionTexteX,												minY + (ecartY + tailleY) * indexY,			positionTexteX + tailleTexteX,														minY + ecartY * indexY + tailleY * (indexY + 1), parentTabRect),
		false, true, generalTab);
	driverTypeComboBox =
		Environment->addComboBox(							getAbsoluteRectangle(positionValeurX,												minY + (ecartY + tailleY) * indexY,			positionValeurX + tailleValeurX,													minY + ecartY * indexY + tailleY * (indexY + 1), parentTabRect),
		generalTab); ++indexY;
	driverTypeComboBox->setToolTipText(L"Choisissez ici le type de driver qui sera utilis� pour le rendu 3D");
	driverTypeComboBox->addItem(L"OpenGL", video::EDT_OPENGL);
	driverTypeComboBox->addItem(L"Direct3D 9.0c", video::EDT_DIRECT3D9);
	switch (gameConfig.deviceParams.DriverType)
	{
	case video::EDT_OPENGL:			driverTypeComboBox->setSelected(0);														break;
	case video::EDT_DIRECT3D9:		driverTypeComboBox->setSelected(1);														break;
	case video::EDT_DIRECT3D8:		driverTypeComboBox->setSelected(
										driverTypeComboBox->addItem(L"Direct3D 8.1", video::EDT_DIRECT3D8));				break;
	case video::EDT_BURNINGSVIDEO:	driverTypeComboBox->setSelected(
										driverTypeComboBox->addItem(L"Burning's Video", video::EDT_BURNINGSVIDEO));			break;
	case video::EDT_SOFTWARE:		driverTypeComboBox->setSelected(
										driverTypeComboBox->addItem(L"Irrlicht Software Renderer", video::EDT_SOFTWARE));	break;
	}

	Environment->addStaticText(L"R�solution :",			getAbsoluteRectangle(positionTexteX,												minY + (ecartY + tailleY) * indexY,			positionTexteX + tailleTexteX,														minY + ecartY * indexY + tailleY * (indexY + 1), parentTabRect),
		false, true, generalTab);
	resolutionComboBox =
		Environment->addComboBox(						getAbsoluteRectangle(positionValeurX,												minY + (ecartY + tailleY) * indexY,			positionValeurX + tailleValeurX,													minY + ecartY * indexY + tailleY * (indexY + 1), parentTabRect),
		generalTab); ++indexY;
	resolutionComboBox->setToolTipText(L"Choisissez ici la r�solution de l'affichage 3D ainsi que la fid�lit� dans le rendu des couleurs");

	// Charge les r�solutions
	{
		// Constantes utilis�es pour trier l'ordre d'affichage des r�solutions dans la liste :
		// La taille en pixels de la r�solution choisie doit v�rifier : MIN_RESOLUTION_WIDTH <= largeur <= MAX_RESOLUTION_WIDTH et : MIN_RESOLUTION_HEIGHT <= hauteur <= MAX_RESOLUTION_HEIGHT
		// Et sa profondeur de couleur en bits doit v�rifier : MIN_COLOR_DEPHT <= bits <= MAX_COLOR_DEPHT
		// Si toutes ces conditions sont v�rifi�es, cette r�solution appara�tra au d�but de la liste, sinon elle ne sera ajout�e qu'� la fin
#define MIN_RESOLUTION_WIDTH	800
#define MIN_RESOLUTION_HEIGHT	600

#define MAX_RESOLUTION_WIDTH	1920
#define MAX_RESOLUTION_HEIGHT	1080

#define MIN_COLOR_DEPHT			16
#define MAX_COLOR_DEPHT			32

		int activeResIndex = -1;
		video::IVideoModeList* const listeModesVideo = game->device->getVideoModeList();
		const int videoModeCount = listeModesVideo->getVideoModeCount();
		int i;	// Variable d'it�ration
		for (i = 0; i < videoModeCount; ++i)
		{
			const core::dimension2du currentRes = listeModesVideo->getVideoModeResolution(i);
			const int currentColorDepht = listeModesVideo->getVideoModeDepth(i);

			// Les r�solutions comprises entre minRes et maxRes et qui ont une profondeur de couleur comprise entre minColorDepht et maxColorDepht
			// apparaissent en premier dans la liste
			if (currentRes.Width >= MIN_RESOLUTION_WIDTH && currentRes.Width <= MAX_RESOLUTION_WIDTH
				&& currentRes.Height >= MIN_RESOLUTION_HEIGHT && currentRes.Height <= MAX_RESOLUTION_HEIGHT
				&& currentColorDepht >= MIN_COLOR_DEPHT && currentColorDepht <= MAX_COLOR_DEPHT)
			{
				swprintf_SS(L"%u x %u : %d bits",
					currentRes.Width, currentRes.Height, currentColorDepht);

				// Note : codage de la r�solution et de la profondeur de couleur :
				// Profondeur max :	64 bits
				// Hauteur max :	4096 pixels
				// => Largeur max :	4294967295 / (4096 * 64) = 16383 pixels (car UINT_MAX = 4294967295)
				const u32 index = resolutionComboBox->addItem(textW_SS, (currentRes.Width * 4096 + currentRes.Height) * 64 + currentColorDepht);

				// Si la r�solution qu'on vient d'ajouter est la r�solution actuelle du device
				if (currentRes == gameConfig.deviceParams.WindowSize)
					activeResIndex = (int)index; // On note son index
			}
		}
		// Ajoute les r�solutions non comprises entre minRes et maxRes ou qui n'ont pas une profondeur de couleur comprise entre minColorDepht et maxColorDepht en dernier
		for (i = 0; i < videoModeCount; ++i)
		{
			const core::dimension2du currentRes = listeModesVideo->getVideoModeResolution(i);
			const int currentColorDepht = listeModesVideo->getVideoModeDepth(i);

			if (currentRes.Width < MIN_RESOLUTION_WIDTH || currentRes.Width > MAX_RESOLUTION_WIDTH
				|| currentRes.Height < MIN_RESOLUTION_HEIGHT || currentRes.Height > MAX_RESOLUTION_HEIGHT
				|| currentColorDepht < MIN_COLOR_DEPHT || currentColorDepht > MAX_COLOR_DEPHT)
			{
				swprintf_SS(L"%u x %u : %d bits",
					currentRes.Width, currentRes.Height, currentColorDepht);

				const u32 index = resolutionComboBox->addItem(textW_SS, (currentRes.Width * 4096 + currentRes.Height) * 64 + currentColorDepht);

				// Si la r�solution qu'on vient d'ajouter est la r�solution actuelle du device
				if (currentRes == gameConfig.deviceParams.WindowSize && currentColorDepht == gameConfig.deviceParams.Bits)
					activeResIndex = (int)index; // On note son index
			}
		}

		if (activeResIndex >= 0)	// Si on a trouv� la r�solution actuelle, on la s�lectionne
			resolutionComboBox->setSelected(activeResIndex);
		else						// Sinon, on ajoute la r�solution actuelle en dernier et on la s�lectionne
		{
			swprintf_SS(L"%u x %u : %d bits",
				gameConfig.deviceParams.WindowSize.Width, gameConfig.deviceParams.WindowSize.Height, gameConfig.deviceParams.Bits);

			resolutionComboBox->setSelected(
				(int)resolutionComboBox->addItem(textW_SS, (gameConfig.deviceParams.WindowSize.Width * 4096 + gameConfig.deviceParams.WindowSize.Height) * 64 + gameConfig.deviceParams.Bits));
		}
	}

	Environment->addStaticText(L"Anticr�nelage :",							getAbsoluteRectangle(positionTexteX,							minY + (ecartY + tailleY) * indexY,			positionTexteX + tailleTexteX,														minY + ecartY * indexY + tailleY * (indexY + 1), parentTabRect),
		false, true, generalTab);
	antialiasingComboBox =
		Environment->addComboBox(												getAbsoluteRectangle(positionValeurX,							minY + (ecartY + tailleY) * indexY,			positionValeurX + tailleValeurX,													minY + ecartY * indexY + tailleY * (indexY + 1), parentTabRect),
		generalTab); ++indexY;
	antialiasingComboBox->setToolTipText(L"R�glez le niveau d'anticr�nelage appliqu� aux rendus 3D");
	antialiasingComboBox->addItem(L"D�sactiv�", 0);
	antialiasingComboBox->addItem(L"2x", 2);
	antialiasingComboBox->addItem(L"4x", 4);
	antialiasingComboBox->addItem(L"8x", 8);
	switch (gameConfig.deviceParams.AntiAlias)
	{
	case 0:		antialiasingComboBox->setSelected(0);													break;
	case 2:		antialiasingComboBox->setSelected(1);													break;
	case 4:		antialiasingComboBox->setSelected(2);													break;
	case 8:		antialiasingComboBox->setSelected(3);													break;
	default: {	swprintf_SS(L"Inconnu (%d)", gameConfig.deviceParams.AntiAlias);
				antialiasingComboBox->setSelected(
					antialiasingComboBox->addItem(textW_SS, gameConfig.deviceParams.AntiAlias)); }		break;
	}

	fullScreenCheckBox =
		Environment->addCheckBox(gameConfig.deviceParams.Fullscreen,			getAbsoluteRectangle(positionTexteX,							minY + (ecartY + tailleY) * indexY,			positionValeurX + tailleValeurX,													minY + ecartY * indexY + tailleY * (indexY + 1), parentTabRect),
		generalTab, -1, L"Plein �cran"); ++indexY;
	fullScreenCheckBox->setToolTipText(L"Cochez cette case pour activer le mode plein �cran");

	vSyncCheckBox =
		Environment->addCheckBox(gameConfig.deviceParams.Vsync,				getAbsoluteRectangle(positionTexteX,							minY + (ecartY + tailleY) * indexY,			positionValeurX + tailleValeurX,													minY + ecartY * indexY + tailleY * (indexY + 1), parentTabRect),
		generalTab, -1, L"Synchronisation Verticale"); ++indexY;
	vSyncCheckBox->setToolTipText(L"Cochez cette case pour activer la synchronisation verticale");

	++indexY;

	Environment->addStaticText(L"Qualit� des textures :",						getAbsoluteRectangle(positionTexteX,							minY + (ecartY + tailleY) * indexY,			positionTexteX + tailleTexteX,														minY + ecartY * indexY + tailleY * (indexY + 1), parentTabRect),
		false, true, generalTab);
	texturesQualityComboBox =
		Environment->addComboBox(												getAbsoluteRectangle(positionValeurX,							minY + (ecartY + tailleY) * indexY,			positionValeurX + tailleValeurX,													minY + ecartY * indexY + tailleY * (indexY + 1), parentTabRect),
		generalTab); ++indexY;
	texturesQualityComboBox->setToolTipText(L"Modifiez ici la qualit� des textures utilis�e");
	texturesQualityComboBox->addItem(L"Tr�s basse", (int)GameConfiguration::ETQ_VERY_LOW);
	texturesQualityComboBox->addItem(L"Basse", (int)GameConfiguration::ETQ_LOW);
	texturesQualityComboBox->addItem(L"Moyenne", (int)GameConfiguration::ETQ_MEDIUM);
	texturesQualityComboBox->addItem(L"Haute", (int)GameConfiguration::ETQ_HIGH);
	switch (gameConfig.texturesQuality)
	{
	case GameConfiguration::ETQ_VERY_LOW:	texturesQualityComboBox->setSelected(0);									break;
	case GameConfiguration::ETQ_LOW:		texturesQualityComboBox->setSelected(1);									break;
	case GameConfiguration::ETQ_MEDIUM:		texturesQualityComboBox->setSelected(2);									break;
	case GameConfiguration::ETQ_HIGH:		texturesQualityComboBox->setSelected(3);									break;
	default: {	swprintf_SS(L"Inconnu (%d)", (int)gameConfig.texturesQuality);
				texturesQualityComboBox->setSelected(
				texturesQualityComboBox->addItem(textW_SS, (int)gameConfig.texturesQuality)); }							break;
	}

	Environment->addStaticText(L"Filtrage des textures :",					getAbsoluteRectangle(positionTexteX,							minY + (ecartY + tailleY) * indexY,			positionTexteX + tailleTexteX,														minY + ecartY * indexY + tailleY * (indexY + 1), parentTabRect),
		false, true, generalTab);
	texturesFilterComboBox =
		Environment->addComboBox(												getAbsoluteRectangle(positionValeurX,							minY + (ecartY + tailleY) * indexY,			positionValeurX + tailleValeurX,													minY + ecartY * indexY + tailleY * (indexY + 1), parentTabRect),
		generalTab); ++indexY;
	texturesFilterComboBox->setToolTipText(L"Modifiez ici le type de filtrage des textures");
	texturesFilterComboBox->addItem(L"Aucun", (int)GameConfiguration::ETQ_VERY_LOW);
	texturesFilterComboBox->addItem(L"Bilin�aire", (int)GameConfiguration::ETQ_LOW);
	texturesFilterComboBox->addItem(L"Trilin�aire", (int)GameConfiguration::ETQ_MEDIUM);
	texturesFilterComboBox->addItem(L"Anisotropique", (int)GameConfiguration::ETQ_HIGH);
	if (gameConfig.anisotropicFilter > 0)
		texturesFilterComboBox->setSelected(3);
	else if (gameConfig.trilinearFilterEnabled)
		texturesFilterComboBox->setSelected(2);
	else if (gameConfig.bilinearFilterEnabled)
		texturesFilterComboBox->setSelected(1);
	else
		texturesFilterComboBox->setSelected(0);

	// D�sactiv� car peu utilis� en r�alit�
	//powerOf2TexturesCheckBox =
	//	Environment->addCheckBox(gameConfig.usePowerOf2Textures,				getAbsoluteRectangle(positionTexteX,							minY + (ecartY + tailleY)* indexY,			positionValeurX + tailleValeurX,													minY + ecartY * indexY + tailleY * (indexY + 1), parentTabRect),
	//	generalTab, -1, L"Textures en puissances de deux"); ++indexY;
	//powerOf2TexturesCheckBox->setToolTipText(L"Activez cette option si vous rencontrez des probl�mes d'affichage des textures sur certains batiments");

	++indexY;

	Environment->addStaticText(L"Transparence de l'interface :",				getAbsoluteRectangle(positionTexteX,							minY + (ecartY + tailleY)* indexY,			positionTexteX + tailleTexteX,														minY + ecartY * indexY + tailleY * (indexY + 1), parentTabRect),
		false, true, generalTab);
	guiTransparencyScrollBar =
		Environment->addScrollBar(true,										getAbsoluteRectangle(positionValeurX,							minY + (ecartY + tailleY)* indexY,			positionValeurX + tailleValeurX,													minY + ecartY * indexY + tailleY * (indexY + 1), parentTabRect),
		generalTab); ++indexY;
	guiTransparencyScrollBar->setToolTipText(L"D�placez le curseur pour r�gler la transparence de l'interface utilisateur");
	{
		const int guiTransparency = gameConfig.guiTransparency;
		guiTransparencyScrollBar->setMin(guiTransparency >= 100 ? 100 : -1);
		guiTransparencyScrollBar->setMax(guiTransparency <= 255 ? 255 : 256);
		guiTransparencyScrollBar->setSmallStep(5);
		guiTransparencyScrollBar->setLargeStep(25);
	}

	Environment->addStaticText(L"Apparence de l'interface :",					getAbsoluteRectangle(positionTexteX,							minY + (ecartY + tailleY) * indexY,			positionTexteX + tailleTexteX,														minY + ecartY * indexY + tailleY * (indexY + 1), parentTabRect),
		false, true, generalTab);
	guiSkinsComboBox =
		Environment->addComboBox(												getAbsoluteRectangle(positionValeurX,							minY + (ecartY + tailleY) * indexY,			positionValeurX + tailleValeurX,													minY + ecartY * indexY + tailleY * (indexY + 1), parentTabRect),
		generalTab); ++indexY;
	guiSkinsComboBox->setToolTipText(L"Choisissez ici l'apparence de l'interface utilisateur");

	Environment->addStaticText(L"Valeur gamma :",								getAbsoluteRectangle(positionTexteX,							minY + (ecartY + tailleY)* indexY,			positionTexteX + tailleTexteX,														minY + ecartY * indexY + tailleY * (indexY + 1), parentTabRect),
		false, true, generalTab);
	gammaScrollBar =
		Environment->addScrollBar(true,										getAbsoluteRectangle(positionValeurX,							minY + (ecartY + tailleY)* indexY,			positionValeurX + tailleValeurX,													minY + ecartY * indexY + tailleY * (indexY + 1), parentTabRect),
		generalTab); ++indexY;
	gammaScrollBar->setToolTipText(L"D�placez le curseur pour r�gler le niveau de correction gamma de votre moniteur");
	{
		const int gammaValue = (int)(((gameConfig.gamma.red + gameConfig.gamma.green+ gameConfig.gamma.blue) / 3.0f) * 10.0f);
		gammaScrollBar->setMin(gammaValue >= 5 ? 5 : gammaValue);
		gammaScrollBar->setMax(gammaValue <= 35 ? 35 : gammaValue);
		gammaScrollBar->setSmallStep(1);
		gammaScrollBar->setLargeStep(5);
	}

	// Partie sauvegarde automatique :
	++indexY;

	Environment->addStaticText(L"Sauvegardes automatiques :",					getAbsoluteRectangle(positionTexteX,							minY + (ecartY + tailleY)* indexY,			positionTexteX + tailleTexteX,														minY + ecartY * indexY + tailleY * (indexY + 1), parentTabRect),
		false, true, generalTab);
	autoSaveFrequencyComboBox =
		Environment->addComboBox(												getAbsoluteRectangle(positionValeurX,							minY + (ecartY + tailleY)* indexY,			positionValeurX + tailleValeurX,													minY + ecartY * indexY + tailleY * (indexY + 1), parentTabRect),
		generalTab); ++indexY;
	autoSaveFrequencyComboBox->setToolTipText(L"Choisissez ici la fr�quence des sauvegardes automatiques");
	autoSaveFrequencyComboBox->addItem(L"D�sactiv�", 0);
	autoSaveFrequencyComboBox->addItem(L"Toutes les 5 minutes", 300);
	autoSaveFrequencyComboBox->addItem(L"Toutes les 15 minutes", 900);
	autoSaveFrequencyComboBox->addItem(L"Toutes les 30 minutes", 1800);
	autoSaveFrequencyComboBox->addItem(L"Toutes les heures", 3600);
	switch (gameConfig.autoSaveFrequency)
	{
	case 0:			autoSaveFrequencyComboBox->setSelected(0);														break;
	case 300:		autoSaveFrequencyComboBox->setSelected(1);														break;
	case 900:		autoSaveFrequencyComboBox->setSelected(2);														break;
	case 1800:		autoSaveFrequencyComboBox->setSelected(3);														break;
	case 3600:		autoSaveFrequencyComboBox->setSelected(4);														break;
	default: {		swprintf_SS(L"Toutes les %u secondes", gameConfig.autoSaveFrequency);
					autoSaveFrequencyComboBox->setSelected(
							autoSaveFrequencyComboBox->addItem(textW_SS, gameConfig.autoSaveFrequency)); }			break;
	}



	// Partie shaders et ombres :
	indexY = 0;
	// Ombres temps r�el d'Irrlicht d�sactiv�es : compl�tement bugg�es lorsque certains shaders sont activ�s
#if 0
	stencilShadowsCheckBox =
		Environment->addCheckBox(gameConfig.stencilShadowsEnabled,			getAbsoluteRectangle(positionTexteX,							minY + (ecartY + tailleY) * indexY,			positionValeurX + tailleValeurX,													minY + ecartY * indexY + tailleY * (indexY + 1), parentTabRect),
		shadersTab, -1, L"Ombres pr�cises"); ++indexY;
	stencilShadowsCheckBox->setToolTipText(L"Cochez cette case pour afficher les ombres pr�cises des b�timents");

	++indexY;
#endif

	shadersEnabledCheckBox =
		Environment->addCheckBox(gameConfig.shadersEnabled,					getAbsoluteRectangle(positionTexteX,							minY + (ecartY + tailleY) * indexY,			positionValeurX + tailleValeurX,													minY + ecartY * indexY + tailleY * (indexY + 1), parentTabRect),
		shadersTab, -1, L"Shaders"); ++indexY;
	shadersEnabledCheckBox->setToolTipText(L"Cochez cette case pour permettre l'utilisation des shaders (am�liore la qualit� graphique du jeu)");

	skyShaderEnabledCheckBox =
		Environment->addCheckBox(gameConfig.skyDomeShadersEnabled,			getAbsoluteRectangle(positionTexteX,							minY + (ecartY + tailleY) * indexY,			positionValeurX + tailleValeurX,													minY + ecartY * indexY + tailleY * (indexY + 1), parentTabRect),
		shadersTab, -1, L"Shaders du ciel"); ++indexY;
	skyShaderEnabledCheckBox->setToolTipText(L"Cochez cette case pour activer les shaders du ciel : am�liore les transitions entre les diff�rents temps du ciel");

	normalMappingShaderEnabledCheckBox =
		Environment->addCheckBox(gameConfig.normalMappingEnabled,				getAbsoluteRectangle(positionTexteX,							minY + (ecartY + tailleY) * indexY,			positionValeurX + tailleValeurX,													minY + ecartY * indexY + tailleY * (indexY + 1), parentTabRect),
		shadersTab, -1, L"Shaders des b�timents"); ++indexY;
	normalMappingShaderEnabledCheckBox->setToolTipText(L"Cochez cette case pour activer les shaders des b�timents : ajoute des effets de relief � certains b�timents du jeu");

	terrainShaderEnabledCheckBox =
		Environment->addCheckBox(gameConfig.terrainsShadersEnabled,			getAbsoluteRectangle(positionTexteX,							minY + (ecartY + tailleY) * indexY,			positionValeurX + tailleValeurX,													minY + ecartY * indexY + tailleY * (indexY + 1), parentTabRect),
		shadersTab, -1, L"Shaders des terrains"); ++indexY;
	terrainShaderEnabledCheckBox->setToolTipText(L"Cochez cette case pour activer les shaders des terrains : am�liore le rendu de certains terrains");

	++indexY;

	waterShaderEnabledCheckBox =
		Environment->addCheckBox(gameConfig.waterShaderEnabled,				getAbsoluteRectangle(positionTexteX,							minY + (ecartY + tailleY) * indexY,			positionValeurX + tailleValeurX,													minY + ecartY * indexY + tailleY * (indexY + 1), parentTabRect),
		shadersTab, -1, L"Shaders de l'eau"); ++indexY;
	waterShaderEnabledCheckBox->setToolTipText(L"Cochez cette case pour activer les shaders de l'eau : am�liore le rendu de l'eau");

	// D�sactiv� : la case � cocher pour l'animation de la surface de l'eau dans le sens qu'elle n'est vraiment apr�ciable que si les shaders sont d�sactiv�s
	// (la fonction CGUIOptionsMenu::applySelectedOptions() a �t� pr�vue pour g�rer correctement cette d�sactivation)
	// R�-activ� !
	animatedWaterCheckBox =
		Environment->addCheckBox(gameConfig.animatedWater,					getAbsoluteRectangle(positionTexteX,							minY + (ecartY + tailleY) * indexY,			positionValeurX + tailleValeurX,													minY + ecartY * indexY + tailleY * (indexY + 1), parentTabRect),
		shadersTab, -1, L"Animation de l'eau"); ++indexY;
	animatedWaterCheckBox->setToolTipText(L"Cochez cette case pour activer l'animation de la surface de l'eau");

	waterQualityTexte = Environment->addStaticText(L"Qualit� de l'eau :",		getAbsoluteRectangle(positionTexteX,							minY + (ecartY + tailleY) * indexY,			positionTexteX + tailleTexteX,														minY + ecartY * indexY + tailleY * (indexY + 1), parentTabRect),
		false, true, shadersTab);
	waterShaderQualityComboBox =
		Environment->addComboBox(												getAbsoluteRectangle(positionValeurX,							minY + (ecartY + tailleY) * indexY,			positionValeurX + tailleValeurX,													minY + ecartY * indexY + tailleY * (indexY + 1), parentTabRect),
		shadersTab); ++indexY;
	waterShaderQualityComboBox->setToolTipText(L"Modifiez ici la qualit� du rendu des objets sous l'eau et des objets r�fl�chis");
	waterShaderQualityComboBox->addItem(L"Tr�s basse", 128);
	waterShaderQualityComboBox->addItem(L"Basse", 256);
	waterShaderQualityComboBox->addItem(L"Moyenne", 512);
	waterShaderQualityComboBox->addItem(L"Haute", 1024);
	const u32 rttSizeData = (gameConfig.waterShaderRenderTargetSize.Width + gameConfig.waterShaderRenderTargetSize.Height) / 2;
	switch (rttSizeData)
	{
	case 128:	waterShaderQualityComboBox->setSelected(0);							break;
	case 256:	waterShaderQualityComboBox->setSelected(1);							break;
	case 512:	waterShaderQualityComboBox->setSelected(2);							break;
	case 1024:	waterShaderQualityComboBox->setSelected(3);							break;
	default: {	swprintf_SS(L"Inconnu (%u)", rttSizeData);
				waterShaderQualityComboBox->setSelected(
					waterShaderQualityComboBox->addItem(textW_SS, rttSizeData)); }	break;
	}

#ifdef _DEBUG
	// TODO : Pour le moment, ombres de XEffects seulement disponibles en mode DEBUG
	++indexY;

	xEffectsShadowsEnabledCheckBox =
		Environment->addCheckBox(gameConfig.terrainsShadersEnabled,			getAbsoluteRectangle(positionTexteX,							minY + (ecartY + tailleY) * indexY,			positionValeurX + tailleValeurX,													minY + ecartY * indexY + tailleY * (indexY + 1), parentTabRect),
		shadersTab, -1, L"Ombres"); ++indexY;
	xEffectsShadowsEnabledCheckBox->setToolTipText(L"Cochez cette case pour afficher les ombres des b�timents");
#endif



	// Partie effets post-rendu :
	//indexY = 0;
	// Note : La soustraction de la position de son coin sup�rieur gauche au rectangle indiquant la position de cet �l�ment est n�cessaire pour �viter un d�calage en bas � gauche de cet �l�ment
	postProcessOptions = new CGUIPostProcessOptions(Environment, postProcessTab, -1, parentTabRect - parentTabRect.UpperLeftCorner);
	postProcessOptions->drop();



	// Partie audio :
	indexY = 0;
	audioEnabledCheckBox =
		Environment->addCheckBox(gameConfig.audioEnabled,	getAbsoluteRectangle((soundsMaxX - soundsX) * 0.5f + soundsX,						sounsPositionY,								soundsMaxX,																			sounsPositionY + tailleY, parentTabRect),
		audioTab, -1, L"Audio activ�");
#ifdef USE_IRRKLANG
	audioEnabledCheckBox->setToolTipText(L"Cochez cette case pour activer les musiques et sons");
#else
	audioEnabledCheckBox->setEnabled(false);
	audioEnabledCheckBox->setToolTipText(L"Les musiques et sons ne sont pas disponibles avec cette version du jeu");
#endif

	mainVolumeTexte =
		Environment->addStaticText(L"Volume principal",		getAbsoluteRectangle(soundsX,													sounsPositionY + ecartY + tailleY,				soundsX + soundsTailleTexteX,														sounsPositionY + ecartY + tailleY * 2.0f, parentTabRect),
		false, true, audioTab);
	mainVolumeTexte->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER);
#ifndef USE_IRRKLANG
	mainVolumeTexte->setEnabled(false);
	mainVolumeTexte->setToolTipText(L"Les musiques et sons ne sont pas disponibles avec cette version du jeu");
#endif
	mainVolumeScrollBar =
		Environment->addScrollBar(false,						getAbsoluteRectangle(soundsX + soundsEcartScrollBarX,									sounsPositionY + (ecartY + tailleY) * 2.0f,	soundsX + soundsEcartScrollBarX + soundsTailleScrollBarX,										sounsPositionY + (ecartY + tailleY) * 2.0f + soundsTailleScrollBarY, parentTabRect),
		audioTab);
#ifdef USE_IRRKLANG
	mainVolumeScrollBar->setToolTipText(L"D�placez le curseur pour r�gler le volume audio g�n�ral");
#else
	mainVolumeScrollBar->setEnabled(false);
	mainVolumeScrollBar->setToolTipText(L"Les musiques et sons ne sont pas disponibles avec cette version du jeu");
#endif
	mainVolumeScrollBar->setMin(0);
	mainVolumeScrollBar->setMax(10);
	mainVolumeScrollBar->setSmallStep(1);
	mainVolumeScrollBar->setLargeStep(5);

	musicVolumeTexte =
		Environment->addStaticText(L"Volume des musiques",	getAbsoluteRectangle(soundsX + soundsEcartX + soundsTailleTexteX,				sounsPositionY + ecartY + tailleY,				soundsX + soundsEcartX + soundsTailleTexteX * 2.0f,									sounsPositionY + ecartY + tailleY * 2.0f, parentTabRect),
		false, true, audioTab);
	musicVolumeTexte->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER);
#ifndef USE_IRRKLANG
	musicVolumeTexte->setEnabled(false);
	musicVolumeTexte->setToolTipText(L"Les musiques et sons ne sont pas disponibles avec cette version du jeu");
#endif
	musicVolumeScrollBar =
		Environment->addScrollBar(false,					getAbsoluteRectangle(soundsX + soundsEcartScrollBarX + soundsEcartX + soundsTailleTexteX,	sounsPositionY + (ecartY + tailleY) * 2.0f,	soundsX + soundsEcartScrollBarX + soundsEcartX + soundsTailleTexteX + soundsTailleScrollBarX,	sounsPositionY + (ecartY + tailleY) * 2.0f + soundsTailleScrollBarY, parentTabRect),
		audioTab);
#ifdef USE_IRRKLANG
	musicVolumeScrollBar->setToolTipText(L"D�placez le curseur pour r�gler le volume des musiques");
#else
	musicVolumeScrollBar->setEnabled(false);
	musicVolumeScrollBar->setToolTipText(L"Les musiques et sons ne sont pas disponibles avec cette version du jeu");
#endif
	musicVolumeScrollBar->setMin(0);
	musicVolumeScrollBar->setMax(10);
	musicVolumeScrollBar->setSmallStep(1);
	musicVolumeScrollBar->setLargeStep(5);

	soundVolumeTexte =
		Environment->addStaticText(L"Volume des sons",		getAbsoluteRectangle(soundsX + (soundsEcartX + soundsTailleTexteX) * 2.0f,		sounsPositionY + ecartY + tailleY,			soundsX + soundsEcartX * 2.0f + soundsTailleTexteX * 3.0f,									sounsPositionY + ecartY + tailleY * 2.0f, parentTabRect),
		false, true, audioTab);
	soundVolumeTexte->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER);
#ifndef USE_IRRKLANG
	soundVolumeTexte->setEnabled(false);
	soundVolumeTexte->setToolTipText(L"Les musiques et sons ne sont pas disponibles avec cette version du jeu");
#endif
	soundVolumeScrollBar = 
		Environment->addScrollBar(false,	getAbsoluteRectangle(soundsX + soundsEcartScrollBarX + soundsEcartX * 2.0f + soundsTailleTexteX * 2.0f,	sounsPositionY + (ecartY + tailleY) * 2.0f,soundsX + soundsEcartScrollBarX + (soundsEcartX + soundsTailleTexteX) * 2.0f + soundsTailleScrollBarX,	sounsPositionY + (ecartY + tailleY) * 2.0f + soundsTailleScrollBarY, parentTabRect),
		audioTab);
#ifdef USE_IRRKLANG
	soundVolumeScrollBar->setToolTipText(L"D�placez le curseur pour r�gler le volume des sons");
#else
	soundVolumeScrollBar->setEnabled(false);
	soundVolumeScrollBar->setToolTipText(L"Les musiques et sons ne sont pas disponibles avec cette version du jeu");
#endif
	soundVolumeScrollBar->setMin(0);
	soundVolumeScrollBar->setMax(10);
	soundVolumeScrollBar->setSmallStep(1);
	soundVolumeScrollBar->setLargeStep(5);



	// Boutons du bas :
	retourBouton = Environment->addButton(getAbsoluteRectangle(0.15f - halfTailleMainBoutonsX, positionMainBoutonsY, 0.15f + halfTailleMainBoutonsX, positionMainBoutonsY + tailleMainBoutonsY, parentRect),
		this, -1, L"Retour", L"Retourne au menu principal");
	retablirBouton = Environment->addButton(getAbsoluteRectangle(0.38f - halfTailleMainBoutonsX, positionMainBoutonsY, 0.38f + halfTailleMainBoutonsX, positionMainBoutonsY + tailleMainBoutonsY, parentRect),
		this, -1, L"R�tablir", L"R�tablit les options actuelles");
	defautBouton = Environment->addButton(getAbsoluteRectangle(0.62f - halfTailleMainBoutonsX, positionMainBoutonsY, 0.62f + halfTailleMainBoutonsX, positionMainBoutonsY + tailleMainBoutonsY, parentRect),
		this, -1, L"Par d�faut", L"Restaure les valeurs par d�faut");
	appliquerBouton = Environment->addButton(getAbsoluteRectangle(0.85f - halfTailleMainBoutonsX, positionMainBoutonsY, 0.85f + halfTailleMainBoutonsX, positionMainBoutonsY + tailleMainBoutonsY, parentRect),
		this, -1, L"Appliquer", L"Applique les changements");



	// Met � jour la liste des skins disponibles pour la gui
	// D�sactiv� : Effectu� automatiquement dans CGUIOptionsMenu lors de l'affichage de ce menu
	//updateGUISkinsListOptionsGUI(game->fileSystem, gameConfig.guiSkinFile);
}
void CGUIOptionsMenu::updateFromGameConfig(const GameConfiguration& gameCfg)
{
	// Partie g�n�ral/graphique :
	if (driverTypeComboBox)
	{
		const int activeDriverIndex = driverTypeComboBox->getIndexForItemData((u32)gameCfg.deviceParams.DriverType);

		if (activeDriverIndex != -1)
			driverTypeComboBox->setSelected(activeDriverIndex);
		else
		{
			switch (gameCfg.deviceParams.DriverType)
			{
			case video::EDT_DIRECT3D8:		driverTypeComboBox->setSelected(
												driverTypeComboBox->addItem(L"Direct3D 8.1", video::EDT_DIRECT3D8));				break;
			case video::EDT_BURNINGSVIDEO:	driverTypeComboBox->setSelected(
												driverTypeComboBox->addItem(L"Burning's Video", video::EDT_BURNINGSVIDEO));			break;
			case video::EDT_SOFTWARE:		driverTypeComboBox->setSelected(
												driverTypeComboBox->addItem(L"Irrlicht Software Renderer", video::EDT_SOFTWARE));	break;
			default:						driverTypeComboBox->setSelected(-1);													break;
			}
		}
	}

	if (resolutionComboBox)
	{
		const core::dimension2du& activeRes = gameCfg.deviceParams.WindowSize;
		const int activeResData = (activeRes.Width * 4096 + activeRes.Height) * 64 + gameCfg.deviceParams.Bits;
		const int activeResIndex = resolutionComboBox->getIndexForItemData(activeResData);

		if (activeResIndex != -1)
			resolutionComboBox->setSelected(activeResIndex);
		else
		{
			swprintf_SS(L"%d x %d : %d bits", activeRes.Width, activeRes.Height, gameCfg.deviceParams.Bits);

			resolutionComboBox->setSelected(resolutionComboBox->addItem(textW_SS, activeResData));
		}
	}

	if (fullScreenCheckBox)
		fullScreenCheckBox->setChecked(gameCfg.deviceParams.Fullscreen);

	if (antialiasingComboBox)
	{
		const int activeAntialiasingIndex = antialiasingComboBox->getIndexForItemData(gameCfg.deviceParams.AntiAlias);

		if (activeAntialiasingIndex != -1)
			antialiasingComboBox->setSelected(activeAntialiasingIndex);
		else
		{
			swprintf_SS(L"Inconnu (%d)", gameCfg.deviceParams.AntiAlias);

			antialiasingComboBox->setSelected(antialiasingComboBox->addItem(textW_SS, gameCfg.deviceParams.AntiAlias));
		}
	}

	if (vSyncCheckBox)
		vSyncCheckBox->setChecked(gameCfg.deviceParams.Vsync);

	if (texturesQualityComboBox)
	{
		const int texturesQualityIndex = texturesQualityComboBox->getIndexForItemData(gameCfg.texturesQuality);

		if (texturesQualityIndex != -1)
			texturesQualityComboBox->setSelected(texturesQualityIndex);
		else
		{
			swprintf_SS(L"Inconnu (%d)", (int)gameCfg.texturesQuality);

			texturesQualityComboBox->setSelected(
					texturesQualityComboBox->addItem(textW_SS, (int)gameCfg.texturesQuality));
		}
	}

	if (gameCfg.anisotropicFilter > 0)
		texturesFilterComboBox->setSelected(3);
	else if (gameCfg.trilinearFilterEnabled)
		texturesFilterComboBox->setSelected(2);
	else if (gameCfg.bilinearFilterEnabled)
		texturesFilterComboBox->setSelected(1);
	else
		texturesFilterComboBox->setSelected(0);

	if (powerOf2TexturesCheckBox)
		powerOf2TexturesCheckBox->setChecked(gameCfg.usePowerOf2Textures);

	if (guiTransparencyScrollBar)
	{
		const int guiTransparency = gameCfg.guiTransparency;

		guiTransparencyScrollBar->setPos(
			guiTransparency >= guiTransparencyScrollBar->getMin() ?
				(guiTransparency <= guiTransparencyScrollBar->getMax() ?
					guiTransparency											// Min <= guiTransparency <= Max
					: guiTransparencyScrollBar->getMax())					// guiTransparency > Max
				: guiTransparencyScrollBar->getMin());						// guiTransparency < Min
	}

	// Met � jour la liste des skins disponibles et s�lectionne le skin actuellement choisi dans la configuration du jeu
	if (guiSkinsComboBox)
		updateGUISkinsList(gameCfg.guiSkinFile);

	if (gammaScrollBar)
	{
#if 0
		gammaScrollBar->setPos((int)(((gameCfg.gamma.red + gameCfg.gamma.green + gameCfg.gamma.blue) / 3.0f) * 10.0f));
#else
		const int gammaValue = (int)(((gameCfg.gamma.red + gameCfg.gamma.green + gameCfg.gamma.blue) / 3.0f) * 10.0f);

		gammaScrollBar->setPos(
			gammaValue >= gammaScrollBar->getMin() ?
				(gammaValue <= gammaScrollBar->getMax() ?
					gammaValue								// Min <= gammaValue <= Max
					: gammaScrollBar->getMax())				// gammaValue > Max
				: gammaScrollBar->getMin());				// gammaValue < Min
#endif
	}

	// Partie sauvegarde automatique :
	if (autoSaveFrequencyComboBox)
	{
		const int autoSaveFrequencyIndex = autoSaveFrequencyComboBox->getIndexForItemData(gameCfg.autoSaveFrequency);

		if (autoSaveFrequencyIndex != -1)
			autoSaveFrequencyComboBox->setSelected(autoSaveFrequencyIndex);
		else
		{
			swprintf_SS(L"Toutes les %u secondes", gameCfg.autoSaveFrequency);

			autoSaveFrequencyComboBox->setSelected(autoSaveFrequencyComboBox->addItem(textW_SS, gameCfg.autoSaveFrequency));
		}
	}



	// Partie shaders et ombres :
	if (stencilShadowsCheckBox)
		stencilShadowsCheckBox->setChecked(gameCfg.stencilShadowsEnabled);

	if (shadersEnabledCheckBox)
		shadersEnabledCheckBox->setChecked(gameCfg.shadersEnabled);

	if (waterShaderEnabledCheckBox)
		waterShaderEnabledCheckBox->setChecked(gameCfg.waterShaderEnabled);

	if (waterShaderQualityComboBox)
	{
		const u32 rttSizeData = (gameCfg.waterShaderRenderTargetSize.Width + gameCfg.waterShaderRenderTargetSize.Height) / 2;
		const int rttSizeIndex = waterShaderQualityComboBox->getIndexForItemData(rttSizeData);

		if (rttSizeIndex != -1)
			waterShaderQualityComboBox->setSelected(rttSizeIndex);
		else
		{
			swprintf_SS(L"Inconnue (%d)", rttSizeData);
			waterShaderQualityComboBox->setSelected(waterShaderQualityComboBox->addItem(textW_SS, rttSizeData));
		}
	}

	if (animatedWaterCheckBox)
		animatedWaterCheckBox->setChecked(gameCfg.animatedWater);

	if (skyShaderEnabledCheckBox)
		skyShaderEnabledCheckBox->setChecked(gameCfg.skyDomeShadersEnabled);

	if (normalMappingShaderEnabledCheckBox)
		normalMappingShaderEnabledCheckBox->setChecked(gameCfg.normalMappingEnabled);

	if (terrainShaderEnabledCheckBox)
		terrainShaderEnabledCheckBox->setChecked(gameCfg.terrainsShadersEnabled);

	if (xEffectsShadowsEnabledCheckBox)
		xEffectsShadowsEnabledCheckBox->setChecked(gameCfg.useXEffectsShadows);



	// Partie effets post-rendu :
	postProcessOptions->updateFromGameConfig(gameCfg);



	// Partie audio :
	if (audioEnabledCheckBox)
		audioEnabledCheckBox->setChecked(gameConfig.audioEnabled);
	if (mainVolumeScrollBar)
		mainVolumeScrollBar->setPos(mainVolumeScrollBar->getMax() - (int)(gameConfig.mainVolume * 10.0f));		// Pour les scroll bar des volumes, la position du curseur est invers�e
	if (musicVolumeScrollBar)
		musicVolumeScrollBar->setPos(musicVolumeScrollBar->getMax() - (int)(gameConfig.musicVolume * 10.0f));	// Pour les scroll bar des volumes, la position du curseur est invers�e
	if (soundVolumeScrollBar)
		soundVolumeScrollBar->setPos(soundVolumeScrollBar->getMax() - (int)(gameConfig.soundVolume * 10.0f));	// Pour les scroll bar des volumes, la position du curseur est invers�e



	// Met � jour ce menu avec tous les nouveaux param�tres importants modifi�s, tout en conservant les valeurs qui viennent d'�tre fix�es
	updateElements_driverChanged(false);
	updateElements_audioEnabledChanged();
	updateElements_shadersEnabledChanged(false);
	updateElements_waterShaderEnabledChanged(false);
	updateElements_postProcessEnabledChanged(false);
}
void CGUIOptionsMenu::applySelectedOptions(bool canShowConfirmationWindow)
{
	// Permet de se souvenir des anciennes options du jeu, au cas o� les nouvelles options am�neraient � un �chec du red�marrage
	lastGameConfig = gameConfig;

	bool haveToRestart = false,
		haveToReloadXEffects = false,
		haveToChangeGameConfigMaterial = false,
		haveToRecreateTextures = false,
		haveToReloadMainMenu = false,
		haveToChangeGUITransparency = false,
		haveToChangeGUISkin = false,
		haveToChangeGammaRamp = false
#ifdef USE_IRRKLANG
		, haveToChangeAudioEnabled = false,
		haveToChangeMusicVolume = false,
		haveToChangeSoundVolume = false
#endif
		;

	SIrrlichtCreationParameters& deviceParams = gameConfig.deviceParams;




	// Partie g�n�ral/graphique :
	if (driverTypeComboBox)
	{
		const int selected = driverTypeComboBox->getSelected();
		if (selected != -1)
		{
			const video::E_DRIVER_TYPE newDriverType = (video::E_DRIVER_TYPE)driverTypeComboBox->getItemData(selected);

			haveToRestart |= (deviceParams.DriverType != newDriverType);
			deviceParams.DriverType = newDriverType;
		}
	}

	if (resolutionComboBox)
	{
		const int selected = resolutionComboBox->getSelected();
		if (selected != -1)
		{
			const u32 itemData = resolutionComboBox->getItemData(selected);
			const u32 resData = itemData / 64;
			const core::dimension2du newResolution(resData / 4096, resData % 4096);
			const u32 newColorDepht = itemData % 64;

			haveToRestart |= ((deviceParams.WindowSize != newResolution) || (deviceParams.Bits != newColorDepht));
			deviceParams.WindowSize = newResolution;
			deviceParams.Bits = newColorDepht;
		}
	}

	if (fullScreenCheckBox)
	{
		haveToRestart |= (deviceParams.Fullscreen != fullScreenCheckBox->isChecked());
		deviceParams.Fullscreen = fullScreenCheckBox->isChecked();
	}

	if (antialiasingComboBox)
	{
		const int selected = antialiasingComboBox->getSelected();
		if (selected != -1)
		{
			const u8 newAntialiasing = (u8)antialiasingComboBox->getItemData(selected);
			haveToRestart |= (deviceParams.AntiAlias != newAntialiasing);
			deviceParams.AntiAlias = newAntialiasing;
		}
	}

	if (vSyncCheckBox)
	{
		haveToRestart |= (deviceParams.Vsync != vSyncCheckBox->isChecked());
		deviceParams.Vsync = vSyncCheckBox->isChecked();
	}

	if (texturesQualityComboBox)
	{
		const int selected = texturesQualityComboBox->getSelected();
		if (selected != -1)
		{
			const GameConfiguration::E_TEXTURE_QUALITY newTexturesQuality =
				(GameConfiguration::E_TEXTURE_QUALITY)texturesQualityComboBox->getItemData(selected);
			haveToRecreateTextures |= (gameConfig.texturesQuality != newTexturesQuality);
			gameConfig.texturesQuality = newTexturesQuality;
		}
	}

	if (texturesFilterComboBox)
	{
		const int filterType = texturesFilterComboBox->getSelected();
		switch (filterType)
		{
		case 3:		// Anisotropique
			haveToChangeGameConfigMaterial |= (gameConfig.anisotropicFilter != 255 || !gameConfig.trilinearFilterEnabled || !gameConfig.bilinearFilterEnabled);
			gameConfig.anisotropicFilter = 255;
			gameConfig.trilinearFilterEnabled = true;
			gameConfig.bilinearFilterEnabled = true;
			break;
		case 2:		// Trilin�aire
			haveToChangeGameConfigMaterial |= (gameConfig.anisotropicFilter != 0 || !gameConfig.trilinearFilterEnabled || !gameConfig.bilinearFilterEnabled);
			gameConfig.anisotropicFilter = 0;
			gameConfig.trilinearFilterEnabled = true;
			gameConfig.bilinearFilterEnabled = true;
			break;
		case 1:		// Bilin�aire
			haveToChangeGameConfigMaterial |= (gameConfig.anisotropicFilter != 0 || gameConfig.trilinearFilterEnabled || !gameConfig.bilinearFilterEnabled);
			gameConfig.anisotropicFilter = 0;
			gameConfig.trilinearFilterEnabled = false;
			gameConfig.bilinearFilterEnabled = true;
			break;
		default:	// case 0 : Aucun
			haveToChangeGameConfigMaterial |= (gameConfig.anisotropicFilter != 0 || gameConfig.trilinearFilterEnabled || gameConfig.bilinearFilterEnabled);
			gameConfig.anisotropicFilter = 0;
			gameConfig.trilinearFilterEnabled = false;
			gameConfig.bilinearFilterEnabled = false;
			break;
		}
	}

	if (powerOf2TexturesCheckBox)
	{
		const bool powerOf2TexturesChecked = powerOf2TexturesCheckBox->isChecked();
		haveToReloadMainMenu |= (gameConfig.usePowerOf2Textures != powerOf2TexturesChecked);
		gameConfig.usePowerOf2Textures = powerOf2TexturesChecked;
	}

	if (guiTransparencyScrollBar)
	{
		const int guiTransparency = guiTransparencyScrollBar->getPos();
		haveToChangeGUITransparency = (gameConfig.guiTransparency != guiTransparency);
		gameConfig.guiTransparency = guiTransparency;
	}

	if (guiSkinsComboBox)
	{
		// Obtient le nom du fichier du skin de la GUI actuellement s�lectionn�
		const int selectedSkin = guiSkinsComboBox->getSelected();
		const io::path newSkinPath = ((selectedSkin == -1) ? "" : (findGUISkinFilename(guiSkinsComboBox->getItem(guiSkinsComboBox->getSelected()))));

		// D�termine si on doit changer le skin de la GUI
		haveToChangeGUISkin = !(newSkinPath.equals_ignore_case(gameConfig.guiSkinFile));
		if (haveToChangeGUISkin)
			gameConfig.guiSkinFile = newSkinPath;
	}

	if (gammaScrollBar)
	{
		const float gammaValue = gammaScrollBar->getPos() * 0.1f;
		haveToChangeGammaRamp = (gameConfig.gamma.red != gammaValue || gameConfig.gamma.green != gammaValue || gameConfig.gamma.blue != gammaValue);

		gameConfig.gamma.red = gammaValue;
		gameConfig.gamma.green = gammaValue;
		gameConfig.gamma.blue = gammaValue;
		gameConfig.gamma.brightness = 0.0f;
		gameConfig.gamma.contrast = 0.0f;
	}

	// Partie sauvegarde automatique :
	if (autoSaveFrequencyComboBox)
	{
		const int selectedFrequency = autoSaveFrequencyComboBox->getSelected();
		if (selectedFrequency != -1)
			gameConfig.autoSaveFrequency = (u32)autoSaveFrequencyComboBox->getItemData(selectedFrequency);
	}



	// Partie shaders et ombres :
	if (stencilShadowsCheckBox)
	{
		const bool stencilShadowsChecked = stencilShadowsCheckBox->isChecked();
#if 0
		// Laisse le stencil buffer du device activ� s'il y �tait et que les ombres viennent d'�tre d�sactiv�es
		// sinon (si les ombres viennent d'�tre activ�es), on l'active obligatoirement
		// -> Le stencil buffer reste activ� lorsque les ombres ont �t� activ�es puis d�sactiv�es juste apr�s
		haveToReloadMainMenu = (gameConfig.stencilShadowsEnabled != stencilShadowsChecked);
		haveToRestart |= (!deviceParams.Stencilbuffer && stencilShadowsChecked);
		deviceParams.Stencilbuffer |= stencilShadowsChecked;
#else
		// Fait obligatoirement concorder l'�tat du stencil buffer avec celui des ombres

		haveToReloadMainMenu = (gameConfig.stencilShadowsEnabled != stencilShadowsChecked);
		haveToRestart |= (deviceParams.Stencilbuffer != stencilShadowsChecked);
		deviceParams.Stencilbuffer = stencilShadowsChecked;		// Permet de d�sactiver le stencil buffer si les ombres sont d�sactiv�es
#endif
		gameConfig.stencilShadowsEnabled = stencilShadowsChecked;
	}

	if (shadersEnabledCheckBox)
	{
		const bool shadersEnabled = shadersEnabledCheckBox->isChecked();
		haveToReloadMainMenu |= (gameConfig.shadersEnabled != shadersEnabled);
		gameConfig.shadersEnabled = shadersEnabled;
	}

	if (waterShaderEnabledCheckBox)
	{
		const bool waterShaderEnabled = waterShaderEnabledCheckBox->isChecked();
		haveToReloadMainMenu |= (gameConfig.waterShaderEnabled != waterShaderEnabled);
		gameConfig.waterShaderEnabled = waterShaderEnabled;
	}

	if (waterShaderQualityComboBox)
	{
		const int selected = waterShaderQualityComboBox->getSelected();
		if (selected != -1)
		{
			const u32 newRenderTargetSize = waterShaderQualityComboBox->getItemData(selected);
			haveToReloadMainMenu |= (gameConfig.waterShaderRenderTargetSize.Width != newRenderTargetSize || gameConfig.waterShaderRenderTargetSize.Height != newRenderTargetSize);
			gameConfig.waterShaderRenderTargetSize.set(newRenderTargetSize, newRenderTargetSize);
		}
	}

	if (animatedWaterCheckBox)
	{
		const bool animatedWater = animatedWaterCheckBox->isChecked();
		haveToReloadMainMenu |= (gameConfig.animatedWater != animatedWater);
		gameConfig.animatedWater = animatedWater;
	}
	else	// Si la case � cocher pour animer l'eau n'existe pas, on anime l'eau seulement si le shader de l'eau est d�sactiv�
	{
		const bool animatedWater = !gameConfig.waterShaderEnabled;
		haveToReloadMainMenu |= (gameConfig.animatedWater != animatedWater);
		gameConfig.animatedWater = animatedWater;
	}

	if (skyShaderEnabledCheckBox)
	{
		const bool skyShaderEnabled = skyShaderEnabledCheckBox->isChecked();
		haveToReloadMainMenu |= (gameConfig.skyDomeShadersEnabled != skyShaderEnabled);
		gameConfig.skyDomeShadersEnabled = skyShaderEnabled;
	}

	if (normalMappingShaderEnabledCheckBox)
	{
		const bool normalMappingShaderEnabled = normalMappingShaderEnabledCheckBox->isChecked();
		haveToReloadMainMenu |= (gameConfig.normalMappingEnabled != normalMappingShaderEnabled);
		gameConfig.normalMappingEnabled = normalMappingShaderEnabled;
	}

	if (terrainShaderEnabledCheckBox)
	{
		const bool terrainShaderEnabled = terrainShaderEnabledCheckBox->isChecked();
		haveToReloadMainMenu |= (gameConfig.terrainsShadersEnabled != terrainShaderEnabled);
		gameConfig.terrainsShadersEnabled = terrainShaderEnabled;
	}

	if (xEffectsShadowsEnabledCheckBox)
	{
		const bool xEffectsShadowsEnabled = xEffectsShadowsEnabledCheckBox->isChecked();
		haveToReloadXEffects = (gameConfig.useXEffectsShadows != xEffectsShadowsEnabled);
		gameConfig.useXEffectsShadows = xEffectsShadowsEnabled;
	}



	// Partie effets post-rendu :
	if (postProcessOptions)
	{
		const bool postProcessEffectsEnabled = postProcessOptions->getPostProcessEffectsEnabled();
		haveToReloadMainMenu |= (gameConfig.usePostProcessEffects != postProcessEffectsEnabled);
		gameConfig.usePostProcessEffects = postProcessEffectsEnabled;

		const bool postProcessCameraShakingEnabled = postProcessOptions->getCameraShakingEnabled();
		haveToReloadMainMenu |= (gameConfig.postProcessShakeCameraOnDestroying != postProcessCameraShakingEnabled);
		gameConfig.postProcessShakeCameraOnDestroying = postProcessCameraShakingEnabled;

		const bool postProcessNeedDepthPass = postProcessOptions->needDepthPass();
		haveToReloadMainMenu |= (gameConfig.postProcessUseDepthRendering != postProcessNeedDepthPass);
		gameConfig.postProcessUseDepthRendering = postProcessNeedDepthPass;

		// Indique les nouveaux effets post-rendu activ�s � la configuration du jeu :
		{
			const core::list<core::stringw>& enabledEffects = postProcessOptions->getEnabledEffects();
			core::array<core::stringw>& gameConfigEffects = gameConfig.postProcessEffects;
			const u32 enabledEffectsSize = enabledEffects.size();
			const u32 gameConfigEffectsSize = gameConfigEffects.size();
			const core::list<core::stringw>::ConstIterator END = enabledEffects.end();

			// D�termine si on doit recr�er le menu principal
			haveToReloadMainMenu |= (enabledEffectsSize != gameConfigEffectsSize);
			if (!haveToReloadMainMenu)
			{
				u32 i = 0;
				for (core::list<core::stringw>::ConstIterator it = enabledEffects.begin(); it != END; ++it, ++i)
					haveToReloadMainMenu |= ((*it) != gameConfigEffects[i]);
			}

			// R��crit ces effets dans la configuration du jeu
			gameConfigEffects.clear();
			gameConfigEffects.reallocate(enabledEffectsSize);
			for (core::list<core::stringw>::ConstIterator it = enabledEffects.begin(); it != END; ++it)
				gameConfigEffects.push_back(*it);
		}
	}



	// Partie audio :
#ifdef USE_IRRKLANG
	if (audioEnabledCheckBox)
	{
		const bool audioEnabled = audioEnabledCheckBox->isChecked();
		haveToChangeAudioEnabled |= (gameConfig.audioEnabled != audioEnabled);
		gameConfig.audioEnabled = audioEnabled;
	}

	if (mainVolumeScrollBar)
	{
		const float newVolume = (mainVolumeScrollBar->getMax() - mainVolumeScrollBar->getPos()) * 0.1f; // Pour les scroll bar des volumes, la position du curseur est invers�e
		if (gameConfig.mainVolume != newVolume)
		{
			gameConfig.mainVolume = newVolume;

			// Demande � IrrKlang de recalculer son volume principal d'apr�s les options de configuration actuelles
			// Cette action est faite automatiquement car elle est tr�s rapide
			ikMgr.recalculateMainVolume();
		}
	}

	if (musicVolumeScrollBar)
	{
		// Pour les scroll bar des volumes, la position du curseur est invers�e
		const float newVolume = (musicVolumeScrollBar->getMax() - musicVolumeScrollBar->getPos()) * 0.1f;
		haveToChangeMusicVolume = (gameConfig.musicVolume != newVolume);
		gameConfig.musicVolume = newVolume;
	}

	if (soundVolumeScrollBar)
	{
		// Pour les scroll bar des volumes, la position du curseur est invers�e
		const float newVolume = (soundVolumeScrollBar->getMax() - soundVolumeScrollBar->getPos()) * 0.1f;
		haveToChangeSoundVolume = (gameConfig.soundVolume != newVolume);
		gameConfig.soundVolume = newVolume;
	}



	// Applique les options audio qui ont chang�es :

	if (haveToChangeAudioEnabled)
	{
		// D�marre IrrKlang si il n'a pas encore �t� charg� jusque l�
		ikMgr.init(true);

		// Joue la musique du menu principal
		if (gameConfig.audioEnabled)
			ikMgr.playMainMenuMusics();

#if 1
		ikMgr.setAllSoundsPaused(!gameConfig.audioEnabled);
#else
		if (gameConfig.audioEnabled)
			ikMgr.setAllSoundsPaused(false);
		else
			ikMgr.stopAllSounds();
#endif
	}

	if (haveToChangeMusicVolume)
		ikMgr.recalculateMusicsVolume();

	if (haveToChangeSoundVolume)
		ikMgr.recalculateSoundsVolume();
#endif



	// Applique les options g�n�rales qui ont chang�es :

	// D�termine si la fen�tre de confirmation des param�tres doit �tre affich�e : lors de la modification de param�tres importants
	bool showConfirmWindow = (canShowConfirmationWindow && (haveToRestart || haveToRecreateTextures || haveToReloadMainMenu || haveToChangeGUISkin));

	if (haveToRestart)
	{
		// Demande le red�marrage et reviens automatiquement � ce menu
		LOG(endl << endl << "Redemarrage en cours pour activer les nouvelles options graphiques..." << endl << endl, ELL_INFORMATION);
		gameState.restart = true;

		gameState.lastWeather = game->system.getWeatherManager().getCurrentWeatherID();	// Indique lors du red�marrage quel temps doit �tre utilis�
		gameState.lastDeviceTime = game->deviceTimer->getTime();						// Indique le temps actuel du device
		gameState.keepIrrKlang = true;													// Indique qu'on doit conserver IrrKlang si le mode audio est activ�
		gameState.restartToOptionsMenu = true;											// Indique qu'une fois red�marr�, on doit revenir au menu options
		gameState.optionsMenuTab = mainTabGroup->getActiveTab();						// Indique quel onglet devra �tre activ� une fois revenu au menu options
		gameState.showOptionsConfirmationWindow = showConfirmWindow;					// Indique qu'une fois arriv� au menu options, on doit demander � l'utilisateur s'il veut conserver ces param�tres

		// Arr�te tous les sons d'IrrKlang sauf la musique du menu principal
		// D�sactiv� tant qu'il n'y a aucun son dans le menu principal : permet d'entendre le son du clic sur le bouton "Appliquer"
		//ikMgr.stopAllSoundsExceptType(IrrKlangManager::EST_mainMenuMusic);

		return;
	}

	if (haveToReloadXEffects)
	{
		// D�truit XEffects pour qu'il soit recr�� au prochain chargement du menu principal
		if (game->xEffects)
		{
			delete game->xEffects;
			game->xEffects = NULL;
		}

		// Et indique qu'on doit changer le mat�riau de remplacement du driver et recr�er le menu principal
		haveToChangeGameConfigMaterial = true;
		haveToReloadMainMenu = true;
	}

	// Applique les nouvelles options du mat�riau de remplacement du driver, ainsi que ses param�tres de cr�ation de texture
	if (haveToChangeGameConfigMaterial || haveToRecreateTextures)
	{
		game->applyGameConfigDriverOptions();

		if (haveToRecreateTextures)
		{
			// Supprime toutes les textures
			while (game->driver->getTextureCount())	game->driver->removeTexture(game->driver->getTextureByIndex(0));

			// Indique que l'image de pr�visualisation du terrain n'est maintenant plus valide
			if (game->guiManager->guiElements.newGameMenuGUI.terrainApercuImage)
				game->guiManager->guiElements.newGameMenuGUI.terrainApercuImage->setImage(NULL);

			// Et indique qu'on doit recharger le menu principal ainsi que le skin de la GUI (sa texture aussi vient d'�tre d�truite) :
			haveToReloadMainMenu = true;
			haveToChangeGUISkin = true;
		}
	}

	if (haveToReloadMainMenu)
	{
		// Recr�e le menu principal :
		gameState.lastWeather = game->system.getWeatherManager().getCurrentWeatherID();	// Permet de conserver le temps actuel lors de la cr�ation du menu principal
		gameState.lastDeviceTime = game->deviceTimer->getTime();						// Permet de conserver le temps du device lors de la cr�ation du menu principal
		gameState.keepIrrKlang = true;													// Permet de conserver irrKlang lors de la cr�ation du menu principal
		gameState.restartToOptionsMenu = true;											// Indique qu'on devra ensuite revenir au menu options (cette option ne red�marre pas le device)
		gameState.optionsMenuTab = -1;													// Indique qu'il ne sera pas n�cessaire d'activer un certain onglet une fois revenu au menu options, puisque ce menu ne sera pas r�initialis�
		gameState.showOptionsConfirmationWindow = false;								// Indique qu'une fois arriv� au menu options, il ne sera pas n�cessaire de demander � l'utilisateur s'il veut conserver ces param�tres, car cela est fait � la fin de cette fonction

		game->createMainMenu();
	}

	if (haveToChangeGUISkin)
	{
		// Recr�e le skin de la GUI
		game->guiManager->createGUISkin(gameConfig.guiSkinFile, gameConfig.guiTransparency);

		// Indique qu'il n'est plus n�cessaire de modifier la transparence de l'interface utilisateur puisque cela vient d'�tre fait
		haveToChangeGUITransparency = false;
	}

	if (haveToChangeGUITransparency)
	{
		// Indique la nouvelle transparence de l'interface utilisateur
		game->guiManager->setGUITransparency(gameConfig.guiTransparency);
	}

	if (haveToChangeGammaRamp)
	{
		// Change le niveau gamma du moniteur si n�cessaire
		game->setNewGammaRamp(gameConfig.gamma);
	}

	// Actualise le menu options avec ces param�tres
	// D�sactiv� : inutile normalement, puisque ces param�tres viennent d'�tre charg�s
	//updateFromGameConfig(gameConfig);

	// Affiche la fen�tre de confirmation de ces param�tres � l'utilisateur si n�cessaire
	if (showConfirmWindow)
		showConfirmationWindow();
}
void CGUIOptionsMenu::updateElements_driverChanged(bool canModifyValues)
{
	const int driverType = driverTypeComboBox ? driverTypeComboBox->getSelected() : gameConfig.deviceParams.DriverType;

	// On ne peut pas activer les shaders si le driver choisi est diff�rent de OpenGL ou DirectX 9
	if (shadersEnabledCheckBox)
	{
		if (driverType == 0 || driverType == 1)	// Le driver actuellement choisi est bien OpenGL ou DirectX 9 : les shaders sont autoris�s
		{
			shadersEnabledCheckBox->setEnabled(true);
			shadersEnabledCheckBox->setToolTipText(L"Cochez cette case pour activer les shaders (am�liore notamment la qualit� de l'eau et les textures de certains terrains)");
		}
		else	// Un autre driver est actuellement s�lectionn� : on d�sactive les shaders
		{
			if (canModifyValues)
			{
				shadersEnabledCheckBox->setChecked(false);
				updateElements_shadersEnabledChanged();	// Indique que l'�tat de la case � cocher des shaders a �t� modifi�
			}
			shadersEnabledCheckBox->setEnabled(false);
			shadersEnabledCheckBox->setToolTipText(L"Les shaders ne sont pas compatibles avec le driver actuellement s�lectionn�");
		}
	}
}
void CGUIOptionsMenu::updateElements_audioEnabledChanged()
{
#ifdef USE_IRRKLANG
	const bool audioEnabled = audioEnabledCheckBox ? audioEnabledCheckBox->isChecked() : false;

	if (mainVolumeTexte)
		mainVolumeTexte->setEnabled(audioEnabled);

	if (mainVolumeScrollBar)
	{
		mainVolumeScrollBar->setEnabled(audioEnabled);

		mainVolumeScrollBar->setToolTipText(audioEnabled ?
			L"D�placez le curseur pour r�gler le volume audio g�n�ral"
			: L"Vous devez activer les musiques et les sons avant de pouvoir modifier les param�tres audio");
	}

	if (musicVolumeTexte)
		musicVolumeTexte->setEnabled(audioEnabled);

	if (musicVolumeScrollBar)
	{
		musicVolumeScrollBar->setEnabled(audioEnabled);

		musicVolumeScrollBar->setToolTipText(audioEnabled ?
			L"D�placez le curseur pour r�gler le volume des musiques"
			: L"Vous devez activer les musiques et les sons avant de pouvoir modifier les param�tres audio");
	}

	if (soundVolumeTexte)
		soundVolumeTexte->setEnabled(audioEnabled);

	if (soundVolumeScrollBar)
	{
		soundVolumeScrollBar->setEnabled(audioEnabled);

		soundVolumeScrollBar->setToolTipText(audioEnabled ?
			L"D�placez le curseur pour r�gler le volume des sons"
			: L"Vous devez activer les musiques et les sons avant de pouvoir modifier les param�tres audio");
	}
#endif
}
void CGUIOptionsMenu::updateElements_shadersEnabledChanged(bool canModifyValues)
{
	const bool shadersEnabled = shadersEnabledCheckBox ? shadersEnabledCheckBox->isChecked() : gameConfig.shadersEnabled;

	// D�sactive la partie shaders si les shaders sont globalement d�sactiv�s
	if (shadersEnabled)
	{
		if (skyShaderEnabledCheckBox)
			skyShaderEnabledCheckBox->setEnabled(true);
		if (normalMappingShaderEnabledCheckBox)
			normalMappingShaderEnabledCheckBox->setEnabled(true);
		if (terrainShaderEnabledCheckBox)
			terrainShaderEnabledCheckBox->setEnabled(true);
		if (waterShaderEnabledCheckBox)
			waterShaderEnabledCheckBox->setEnabled(true);
		if (xEffectsShadowsEnabledCheckBox)
			xEffectsShadowsEnabledCheckBox->setEnabled(true);
	}
	else
	{
		if (skyShaderEnabledCheckBox)
		{
			if (canModifyValues)
				skyShaderEnabledCheckBox->setChecked(false);
			skyShaderEnabledCheckBox->setEnabled(false);
		}
		if (normalMappingShaderEnabledCheckBox)
		{
			if (canModifyValues)
				normalMappingShaderEnabledCheckBox->setChecked(false);
			normalMappingShaderEnabledCheckBox->setEnabled(false);
		}
		if (terrainShaderEnabledCheckBox)
		{
			if (canModifyValues)
				terrainShaderEnabledCheckBox->setChecked(false);
			terrainShaderEnabledCheckBox->setEnabled(false);
		}
		if (waterShaderEnabledCheckBox)
		{
			if (canModifyValues)
			{
				waterShaderEnabledCheckBox->setChecked(false);
				updateElements_waterShaderEnabledChanged();	// Indique que l'�tat de la case � cocher du shader de l'eau a �t� modifi�
			}
			waterShaderEnabledCheckBox->setEnabled(false);
		}
		if (xEffectsShadowsEnabledCheckBox)
		{
			if (canModifyValues)
				xEffectsShadowsEnabledCheckBox->setChecked(false);
			xEffectsShadowsEnabledCheckBox->setEnabled(false);
		}
	}

	// Indique aux options des effets post-rendu si les shaders sont activ�s ou non
	if (postProcessOptions)
	{
		postProcessOptions->setShadersEnabled(shadersEnabled, canModifyValues);
		if (canModifyValues)
			updateElements_postProcessEnabledChanged();	// Indique que l'�tat de la case � cocher de l'activation des effets post-rendu a �t� modifi�
	}
}
void CGUIOptionsMenu::updateElements_waterShaderEnabledChanged(bool canModifyValues)
{
	const bool waterShaderEnabled = waterShaderEnabledCheckBox ? waterShaderEnabledCheckBox->isChecked() : gameConfig.waterShaderEnabled;

	// Active l'animation de l'eau si le shader de l'eau est d�sactiv� et qu'on peut modifier les valeurs de la GUI
	if (canModifyValues && animatedWaterCheckBox)
		animatedWaterCheckBox->setChecked(!waterShaderEnabled);

	// D�sactive le texte de description et la combo box permettant de s�lectionner la qualit� de l'eau si le shader de l'eau est d�sactiv�
	if (waterQualityTexte)
		waterQualityTexte->setEnabled(waterShaderEnabled);
	if (waterShaderQualityComboBox)
		waterShaderQualityComboBox->setEnabled(waterShaderEnabled);
}
void CGUIOptionsMenu::updateElements_postProcessEnabledChanged(bool canModifyValues)
{
	const bool postProcessEnabled = postProcessOptions->getPostProcessEffectsEnabled();

	// PostProcess n'est pas compatible avec l'antialiasing (cela cr�e des probl�mes de Z-Buffer : voir ici : http://irrlicht.sourceforge.net/forum/viewtopic.php?t=33217) :
	// On d�sactive l'antialiasing lorsqu'il est activ�
	if (antialiasingComboBox)
	{
		if (postProcessEnabled)
		{
			if (canModifyValues)
				antialiasingComboBox->setSelected(max(antialiasingComboBox->getIndexForItemData(0), 0));
			antialiasingComboBox->setEnabled(false);
		}
		else
			antialiasingComboBox->setEnabled(true);
	}
}
void CGUIOptionsMenu::updateGUISkinsList(const io::path& guiSkinFileToSelect)
{
	if (!game->fileSystem || !guiSkinsComboBox)
		return;

	// D�s�lectionne la valeur actuellement s�lectionn�e
	guiSkinsComboBox->setSelected(-1);

	// Efface les anciennes valeurs de la liste
	guiSkinsComboBox->clear();

	// Ajoute tous les fichiers du syst�me de fichier d'Irrlicht avec l'extension ".xml" et contenant "Environment Skin" dans la liste des skins de la gui (inspir� depuis Game::addGameArchives)
	const u32 fileArchiveCount = game->fileSystem->getFileArchiveCount();
	for (u32 i = 0; i < fileArchiveCount; ++i)
	{
		const io::IFileList* const fileList = game->fileSystem->getFileArchive(i)->getFileList();
		const u32 fileCount = fileList->getFileCount();
		for (u32 j = 0; j < fileCount; ++j)
		{
			if (fileList->isDirectory(j))	continue;						// V�rifie que le fichier actuel n'est pas un dossier

			const io::path& fileName = fileList->getFileName(j);				// Obtient le nom actuel du fichier
			const int extPos = fileName.findLast('.');						// Trouve le dernier '.' du nom du fichier, pour v�rifier son extension
			if (extPos < 0)	continue;										// V�rifie que le nom du fichier contient bien un '.'
			const io::path extension = fileName.subString(extPos + 1, 4);	// Obtient l'extension du fichier � 4 caract�res maximum (on supprime aussi le '.' de l'extension)
			if (!extension.equals_ignore_case("xml"))	continue;			// V�rifie que l'extension du fichier actuel est bien ".xml"
			if (fileName.find("gui skin") < 0)		continue;			// V�rifie que le nom du fichier contient bien "Environment Skin" (en minuscule car les noms de fichier retourn�s par la liste des fichiers d'Irrlicht sont en minuscule)

			// Le fichier est bien un skin pour la gui : On demande son nom r�el :
			const core::stringw skinName = findGUISkinName(fileName);
			if (skinName != "")
			{
				// Parcours la liste pour v�rifier qu'un skin de ce nom n'a pas d�j� �t� ajout� :
				// (�vite d'avoir deux occurrences de skin avec un nom identique malgr� un fichier de configuration diff�rent : on ne conserve que le premier des deux)
				bool alreadyExists = false;
				const u32 skinItemCount = guiSkinsComboBox->getItemCount();
				for (u32 k = 0; k < skinItemCount; ++k)
				{
					if (skinName == guiSkinsComboBox->getItem(k))
					{
						alreadyExists = true;
						break;
					}
				}

				// Ajoute ce skin � la liste des skins de la gui
				if (!alreadyExists)
					guiSkinsComboBox->addItem(skinName.c_str());
			}
		}
	}

	// V�rifie que l'on veut s�lectionner un �lement
	if (guiSkinsComboBox->getItemCount() > 0)
	{
		if (guiSkinFileToSelect != "")
		{
			// Obtient le nom r�el du skin de la gui d�sir�
			const core::stringw guiSkinNameToSelect = findGUISkinName(guiSkinFileToSelect);

			if (guiSkinNameToSelect != "")
			{
				// Cherche le nom du skin dans la liste et le s�lectionne (s'il n'a pas �t� trouv�, on s�lectionnera le premier �lement de la liste)
				int ID = 0;
				const u32 skinElementsCount = guiSkinsComboBox->getItemCount();
				for (u32 i = 0; i < skinElementsCount; ++i)
				{
					if (guiSkinNameToSelect == guiSkinsComboBox->getItem(i))
					{
						ID = i;
						break;
					}
				}
				guiSkinsComboBox->setSelected(ID);
			}
			else
			{
				// Sinon on ajoute cet �l�ment comme un nouvel �lement dans la liste
				guiSkinsComboBox->setSelected(guiSkinsComboBox->addItem(
					(core::stringw(guiSkinFileToSelect)).c_str()));
			}
		}
		else
		{
			// Sinon s�lectionne le premier �lement
			guiSkinsComboBox->setSelected(0);
		}
	}
}
core::stringw CGUIOptionsMenu::findGUISkinName(const io::path& guiSkinFilename)
{
	if (!game->fileSystem)
		return core::stringw();

	// Obtient le nom r�el d'un skin de la gui (stock� dans son fichier de configuration) : Inspir� depuis CGUITexturedSkin::readSkinXml :

	io::IXMLReaderUTF8* const fileReader = game->fileSystem->createXMLReaderUTF8(guiSkinFilename);
	if (!fileReader)
		return core::stringw();

	core::stringw guiSkinName;
	const core::stringc guiSkinNodeStr("guiskin");
	while (fileReader->read())
	{
		if (fileReader->getNodeType() == io::EXN_ELEMENT)
		{
			if (guiSkinNodeStr.equals_ignore_case(fileReader->getNodeName()))	// Node : "guiskin"
			{
				guiSkinName = fileReader->getAttributeValueSafe("name");		// Nom de l'�lement : "name"
				if (guiSkinName.size() > 0)
					break;
			}
		}
	}
	fileReader->drop();

	return guiSkinName;
}
io::path CGUIOptionsMenu::findGUISkinFilename(const core::stringw& guiSkinName)
{
	if (!game->fileSystem)
		return io::path();

	// Parcours tous les fichiers du syst�me de fichier d'Irrlicht avec l'extension ".xml" et contenant "Environment Skin" (inspir� depuis Game::addGameArchives)
	const u32 fileArchiveCount = game->fileSystem->getFileArchiveCount();
	for (u32 i = 0; i < fileArchiveCount; ++i)
	{
		const io::IFileList* const fileList = game->fileSystem->getFileArchive(i)->getFileList();
		const u32 fileCount = fileList->getFileCount();
		for (u32 j = 0; j < fileCount; ++j)
		{
			if (fileList->isDirectory(j))	continue;						// V�rifie que le fichier actuel n'est pas un dossier

			const io::path& fileName = fileList->getFileName(j);			// Obtient le nom actuel du fichier
			const int extPos = fileName.findLast('.');						// Trouve le dernier '.' du nom du fichier, pour v�rifier son extension
			if (extPos < 0)	continue;										// V�rifie que le nom du fichier contient bien un '.'
			const io::path extension = fileName.subString(extPos + 1, 4);	// Obtient l'extension du fichier � 4 caract�res maximum (on supprime aussi le '.' de l'extension)
			if (!extension.equals_ignore_case("xml"))	continue;			// V�rifie que l'extension du fichier actuel est bien ".xml"
			if (fileName.find("gui skin") < 0)			continue;			// V�rifie que le nom du fichier contient bien "Environment Skin" (en minuscule car les noms de fichier retourn�s par la liste des fichiers d'Irrlicht sont en minuscule)

			// Le fichier est bien un skin pour la gui : On demande son nom r�el :
			const core::stringw skinName = findGUISkinName(fileName);
			if (skinName != "")
			{
				// V�rifie que le nom du skin est bien celui demand�
				if (skinName == guiSkinName)
					return fileName;			// On a trouv� le skin avec ce nom : on renvoie le nom de son fichier
			}
		}
	}

	// Le fichier n'a pas pu �tre trouv� : on retourne le nom du skin de la gui
	return io::path(guiSkinName);
}
