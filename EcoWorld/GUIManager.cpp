#include "GUIManager.h"
#include "Game.h"
#include "CGUIFileSelector.h"
#include "CGUIMenuWindow.h"
#include "CGUIRessourcesWindow.h"
#include "CGUIInformationsWindow.h"
#include "CGUINotesWindow.h"
#include "CGUIMiniMapWindow.h"
#include "CGUIMonthTableWindow.h"
#include "CGUIObjectivesWindow.h"
#include "CGUIObjectivesModifier.h"
#include "CGUISortTable.h"
#include "CGUICameraAnimatorWindow.h"
#include "CGUITexturedSkin.h"
#include "CGUIOptionsMenu.h"
#include "CLoadingScreen.h"
#include "Batiments.h"
#include "EcoWorldModifiers.h"
#include "Weathers.h"

GUIManager::GUIManager() : texturedSkin(NULL)
{
	// Crée un skin par défaut pour la gui
	IGUISkin* const skin = game->gui->createSkin(EGST_WINDOWS_METALLIC);
	game->gui->setSkin(skin);
	skin->drop();

	// Obtient la transparence par défaut du skin
	for (int i = 0; i < EGDC_COUNT ; ++i)
		skinDefaultAplha[i] = skin->getColor((EGUI_DEFAULT_COLOR)i).getAlpha();

	// Remet la gui à zéro
	resetGUI();
}
GUIManager::~GUIManager()
{
	// Détruit l'écran de chargement
	if (guiElements.globalGUI.loadingScreen)
		delete guiElements.globalGUI.loadingScreen;
}
void GUIManager::resetGUI()
{
	// Détruit le loadingScreen
	if (guiElements.globalGUI.loadingScreen)
		delete guiElements.globalGUI.loadingScreen;
	guiElements.globalGUI.loadingScreen = NULL;

	// Supprime l'image de prévisualisation du terrain du cache de textures du driver
	if (guiElements.newGameMenuGUI.terrainApercuImage)
	{
		video::ITexture* const newGamePreviewImage = guiElements.newGameMenuGUI.terrainApercuImage->getImage();
		if (newGamePreviewImage)
		{
			guiElements.newGameMenuGUI.terrainApercuImage->setImage(NULL);
			game->driver->removeTexture(newGamePreviewImage);
		}
	}

	// Efface tous les éléments de la gui
	game->gui->clear();

	// Remet les pointeurs à NULL
	for (int i = 0; i < EGN_COUNT; ++i)
		GUIs[i] = NULL;
	guiElements.reset();
}
void GUIManager::createGUISkin(const io::path& guiSkinFile, int guiTransparency)
{
	IGUISkin* skin = NULL;

	// Vérifie que le fichier de configuration spécifié existe
	bool needCreateDefaultSkin = true;
	if (game->fileSystem->existFile(guiSkinFile))
	{
		texturedSkin = new CGUITexturedSkin(game->gui, game->fileSystem);

		if (texturedSkin)
		{
			// Modifie les paramêtres de création de texture du game->driver : force leur création en mode 32 bits et avec la composante Alpha sans niveaux de mip map :
			// Augmente la qualité visuelle de la texture de la gui et permet ainsi l'utilisation de la fonction CGUITexturedSkin::SetSkinAlpha qui ne supporte les textures qu'en mode 32 bits
			const bool lastDriverTextureCreationStates[6] = {
				game->driver->getTextureCreationFlag(video::ETCF_ALWAYS_16_BIT),
				game->driver->getTextureCreationFlag(video::ETCF_ALWAYS_32_BIT),
				game->driver->getTextureCreationFlag(video::ETCF_OPTIMIZED_FOR_QUALITY),
				game->driver->getTextureCreationFlag(video::ETCF_OPTIMIZED_FOR_SPEED),
				game->driver->getTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS),
				game->driver->getTextureCreationFlag(video::ETCF_NO_ALPHA_CHANNEL), };
			game->driver->setTextureCreationFlag(video::ETCF_ALWAYS_16_BIT, false);
			game->driver->setTextureCreationFlag(video::ETCF_ALWAYS_32_BIT, true);
			game->driver->setTextureCreationFlag(video::ETCF_OPTIMIZED_FOR_QUALITY, false);
			game->driver->setTextureCreationFlag(video::ETCF_OPTIMIZED_FOR_SPEED, false);
			game->driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, false);
			game->driver->setTextureCreationFlag(video::ETCF_NO_ALPHA_CHANNEL, false);

			if (texturedSkin->setSkin(guiSkinFile))	// Vérifie que le chargement du fichier de configuration a bien réussi, sinon ce skin n'a aucun intêret
			{
				game->gui->setSkin(texturedSkin);
				skin = texturedSkin;

				// Obtient la transparence par défaut du skin
				for (int i = 0; i < EGDC_COUNT ; ++i)
					skinDefaultAplha[i] = texturedSkin->getColor((EGUI_DEFAULT_COLOR)i).getAlpha();

				needCreateDefaultSkin = false;	// Indique qu'il n'est pas nécessaire de créer un skin par défaut : la création du skin personnalisé a réussi
				texturedSkin->drop();
			}
			else
			{
				texturedSkin->drop();
				texturedSkin = NULL;
			}

			// Restaure les paramêtres de création de texture du game->driver
			game->driver->setTextureCreationFlag(video::ETCF_ALWAYS_16_BIT, lastDriverTextureCreationStates[0]);
			game->driver->setTextureCreationFlag(video::ETCF_ALWAYS_32_BIT, lastDriverTextureCreationStates[1]);
			game->driver->setTextureCreationFlag(video::ETCF_OPTIMIZED_FOR_QUALITY, lastDriverTextureCreationStates[2]);
			game->driver->setTextureCreationFlag(video::ETCF_OPTIMIZED_FOR_SPEED, lastDriverTextureCreationStates[3]);
			game->driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, lastDriverTextureCreationStates[4]);
			game->driver->setTextureCreationFlag(video::ETCF_NO_ALPHA_CHANNEL, lastDriverTextureCreationStates[5]);
		}
	}

	if (needCreateDefaultSkin)
	{
		// Crée un skin par défaut si le skin personnalisé n'a pu être créé
		skin = game->gui->createSkin(gui::EGST_WINDOWS_METALLIC);
		game->gui->setSkin(skin);
		skin->drop();

		// Obtient la transparence par défaut du skin
		for (int i = 0; i < EGDC_COUNT ; ++i)
			skinDefaultAplha[i] = skin->getColor((EGUI_DEFAULT_COLOR)i).getAlpha();

		// Change la couleur par défaut des polices des boutons
		skin->setColor(EGDC_BUTTON_TEXT, video::SColor(255, 50, 50, 50));

		texturedSkin = NULL;
	}

	// Charge la police du skin suivant la résolution de l'écran (seule la largeur est prise en compte)
	const core::dimension2du screenSize = game->driver->getScreenSize();
	u32 fontSize = 800;
	if (screenSize.Width <= 640)		// <= 640 x 480
		fontSize = 640;
	else if (screenSize.Width <= 800)	// <= 800 x 600
		fontSize = 800;
	else if (screenSize.Width <= 1024)	// <= 1024 x 768
		fontSize = 1024;
	else if (screenSize.Width <= 1280)	// <= 1280 x 800
		fontSize = 1280;
	else								// <= 1440 x 900
		fontSize = 1440;
	sprintf_SS("font_%d.xml", fontSize);
	IGUIFont* font = game->gui->getFont(text_SS);

	// Si la police n'a pas pu être chargée, on utilise la police par défaut de la gui
	if (!font)
		font = game->gui->getBuiltInFont();

	// Assigne toutes les polices du skin avec celle-ci si elle est valide
	if (font)
		for (int i = 0; i < EGDF_COUNT; ++i)
			skin->setFont(font, (EGUI_DEFAULT_FONT)i);

	// Indique les textes par défaut des boutons
	skin->setDefaultText(EGDT_MSG_BOX_CANCEL, L"Annuler");
	skin->setDefaultText(EGDT_MSG_BOX_NO, L"Non");
	skin->setDefaultText(EGDT_MSG_BOX_OK, L"OK");
	skin->setDefaultText(EGDT_MSG_BOX_YES, L"Oui");
	skin->setDefaultText(EGDT_WINDOW_CLOSE, L"Fermer");
	skin->setDefaultText(EGDT_WINDOW_RESTORE, L"Restaurer");
	skin->setDefaultText(EGDT_WINDOW_MINIMIZE, L"Réduire");
	skin->setDefaultText(EGDT_WINDOW_MAXIMIZE, L"Agrandir");

	// Indique la transparence de la gui
	setGUITransparency(guiTransparency);
}
void GUIManager::setGUITransparency(int transparency)
{
	// Utilise le skin de la gui déjà créé (même si ce n'est pas celui retourné par gui->getSkin() : même si ce n'est pas le skin actuel)
	IGUISkin* const skin = (texturedSkin ? texturedSkin : game->gui->getSkin());

	const bool isValidTransparency = (transparency >= 0 && transparency <= 255);

	video::SColor tmpColor;
	for (int i = 0; i < EGDC_COUNT; ++i)
	{
		tmpColor = skin->getColor((EGUI_DEFAULT_COLOR)i);

		if (isValidTransparency)	// La couleur par défaut sera rétablie si la valeur de transparence n'est pas valide
			tmpColor.setAlpha(max(transparency, (int)(skinDefaultAplha[i])));	// Si la couleur est moins transparente par défaut, on ne la modifie pas (la transparence minimale du skin d'Irrlicht par défaut est de 101)

		skin->setColor((EGUI_DEFAULT_COLOR)i, tmpColor);
	}

	// Change la couleur du texte de pause suivant le skin de la GUI
	if (guiElements.globalGUI.pauseTexte)
		guiElements.globalGUI.pauseTexte->setOverrideColor(skin ? skin->getColor(gui::EGDC_ACTIVE_CAPTION) : video::SColor(0xffffffff));

	// A partir d'ici, on suppose la transparence valide : on la rétablit dans les limites
	transparency = core::clamp(transparency, 0, 255);

	// Modifie manuellement la transparence de certains élements de la gui n'utilisant pas les couleurs du skin
	const u32 buttonTextTransparency = max((u32)transparency, skinDefaultAplha[EGDC_BUTTON_TEXT]);	// Prend la couleur la plus opaque des deux
	if (guiElements.globalGUI.pauseTexte)
	{
		tmpColor = guiElements.globalGUI.pauseTexte->getOverrideColor();				// Récupère la couleur de base de l'élement
		tmpColor.setAlpha(buttonTextTransparency);										// Indique la transparence de la couleur
		guiElements.globalGUI.pauseTexte->setOverrideColor(tmpColor);					// Indique la nouvelle couleur de l'élement
	}
	if (guiElements.gameEndGUI.endTexte)
	{
		tmpColor = guiElements.gameEndGUI.endTexte->getOverrideColor();					// Récupère la couleur de base de l'élement
		tmpColor.setAlpha(buttonTextTransparency);										// Indique la transparence de la couleur
		guiElements.gameEndGUI.endTexte->setOverrideColor(tmpColor);					// Indique la nouvelle couleur de l'élement
	}
	if (guiElements.gameGUI.budgetTexte)
	{
		tmpColor = guiElements.gameGUI.budgetTexte->getOverrideColor();					// Récupère la couleur de base de l'élement
		tmpColor.setAlpha(buttonTextTransparency);										// Indique la transparence de la couleur
		guiElements.gameGUI.budgetTexte->setOverrideColor(tmpColor);					// Indique la nouvelle couleur de l'élement
	}
	if (guiElements.gameGUI.energieTexte)
	{
		tmpColor = guiElements.gameGUI.energieTexte->getOverrideColor();				// Récupère la couleur de base de l'élement
		tmpColor.setAlpha(buttonTextTransparency);										// Indique la transparence de la couleur
		guiElements.gameGUI.energieTexte->setOverrideColor(tmpColor);					// Indique la nouvelle couleur de l'élement
	}
	if (guiElements.gameGUI.effetSerreTexte)
	{
		tmpColor = guiElements.gameGUI.effetSerreTexte->getOverrideColor();				// Récupère la couleur de base de l'élement
		tmpColor.setAlpha(buttonTextTransparency);										// Indique la transparence de la couleur
		guiElements.gameGUI.effetSerreTexte->setOverrideColor(tmpColor);				// Indique la nouvelle couleur de l'élement
	}
	if (guiElements.gameGUI.dechetsTexte)
	{
		tmpColor = guiElements.gameGUI.dechetsTexte->getOverrideColor();				// Récupère la couleur de base de l'élement
		tmpColor.setAlpha(buttonTextTransparency);										// Indique la transparence de la couleur
		guiElements.gameGUI.dechetsTexte->setOverrideColor(tmpColor);					// Indique la nouvelle couleur de l'élement
	}
	if (guiElements.gameGUI.populationTexte)
	{
		tmpColor = guiElements.gameGUI.populationTexte->getOverrideColor();				// Récupère la couleur de base de l'élement
		tmpColor.setAlpha(buttonTextTransparency);										// Indique la transparence de la couleur
		guiElements.gameGUI.populationTexte->setOverrideColor(tmpColor);				// Indique la nouvelle couleur de l'élement
	}
	if (guiElements.gameGUI.dateTexte)
	{
		tmpColor = guiElements.gameGUI.dateTexte->getOverrideColor();					// Récupère la couleur de base de l'élement
		tmpColor.setAlpha(buttonTextTransparency);										// Indique la transparence de la couleur
		guiElements.gameGUI.dateTexte->setOverrideColor(tmpColor);						// Indique la nouvelle couleur de l'élement
	}
	if (guiElements.gameGUI.budgetInfosTexte)
	{
		tmpColor = guiElements.gameGUI.budgetInfosTexte->getOverrideColor();			// Récupère la couleur de base de l'élement
		tmpColor.setAlpha(buttonTextTransparency);										// Indique la transparence de la couleur
		guiElements.gameGUI.budgetInfosTexte->setOverrideColor(tmpColor);				// Indique la nouvelle couleur de l'élement
	}
	if (guiElements.gameGUI.energieInfosTexte)
	{
		tmpColor = guiElements.gameGUI.energieInfosTexte->getOverrideColor();			// Récupère la couleur de base de l'élement
		tmpColor.setAlpha(buttonTextTransparency);										// Indique la transparence de la couleur
		guiElements.gameGUI.energieInfosTexte->setOverrideColor(tmpColor);				// Indique la nouvelle couleur de l'élement
	}
	if (guiElements.gameGUI.effetSerreInfosTexte)
	{
		tmpColor = guiElements.gameGUI.effetSerreInfosTexte->getOverrideColor();		// Récupère la couleur de base de l'élement
		tmpColor.setAlpha(buttonTextTransparency);										// Indique la transparence de la couleur
		guiElements.gameGUI.effetSerreInfosTexte->setOverrideColor(tmpColor);			// Indique la nouvelle couleur de l'élement
	}
	if (guiElements.gameGUI.dechetsInfosTexte)
	{
		tmpColor = guiElements.gameGUI.dechetsInfosTexte->getOverrideColor();			// Récupère la couleur de base de l'élement
		tmpColor.setAlpha(buttonTextTransparency);										// Indique la transparence de la couleur
		guiElements.gameGUI.dechetsInfosTexte->setOverrideColor(tmpColor);				// Indique la nouvelle couleur de l'élement
	}
	if (guiElements.gameGUI.populationInfosTexte)
	{
		tmpColor = guiElements.gameGUI.populationInfosTexte->getOverrideColor();		// Récupère la couleur de base de l'élement
		tmpColor.setAlpha(buttonTextTransparency);										// Indique la transparence de la couleur
		guiElements.gameGUI.populationInfosTexte->setOverrideColor(tmpColor);			// Indique la nouvelle couleur de l'élement
	}
	if (guiElements.gameGUI.dateInfosTexte)
	{
		tmpColor = guiElements.gameGUI.dateInfosTexte->getOverrideColor();				// Récupère la couleur de base de l'élement
		tmpColor.setAlpha(buttonTextTransparency);										// Indique la transparence de la couleur
		guiElements.gameGUI.dateInfosTexte->setOverrideColor(tmpColor);					// Indique la nouvelle couleur de l'élement
	}
	if (guiElements.gameGUI.texteVitesseJeuRapide)
	{
		tmpColor = guiElements.gameGUI.texteVitesseJeuRapide->getOverrideColor();		// Récupère la couleur de base de l'élement
		tmpColor.setAlpha(buttonTextTransparency);										// Indique la transparence de la couleur
		guiElements.gameGUI.texteVitesseJeuRapide->setOverrideColor(tmpColor);			// Indique la nouvelle couleur de l'élement
	}
	if (guiElements.gameGUI.texteVitesseJeuLent)
	{
		tmpColor = guiElements.gameGUI.texteVitesseJeuLent->getOverrideColor();			// Récupère la couleur de base de l'élement
		tmpColor.setAlpha(buttonTextTransparency);										// Indique la transparence de la couleur
		guiElements.gameGUI.texteVitesseJeuLent->setOverrideColor(tmpColor);			// Indique la nouvelle couleur de l'élement
	}

/*
#if 0
	// Enlève la transparence de la gui
	skin->setColor(EGDC_ACTIVE_BORDER,		// Valeur de base (A, R, G, B) : 101, 16, 14, 115
		video::SColor(255, 32, 30, 100));

	skin->setColor(EGDC_INACTIVE_BORDER,	// Valeur de base (A, R, G, B) : 101, 165, 165, 165
		video::SColor(255, 140, 140, 140));

	skin->setColor(EGDC_3D_FACE,			// Valeur de base (A, R, G, B) : 101, 210, 210, 210
		video::SColor(255, 180, 180, 180));

	skin->setColor(EGDC_3D_SHADOW,			// Valeur de base (A, R, G, B) : 101, 130, 130, 130
		video::SColor(255, 170, 170, 170));
#endif
*/
}
void GUIManager::createGUIs()
{
	IGUIElement* const rootElement = game->gui->getRootGUIElement();

	// Crée la gui globale
	createGlobalGUI(rootElement);

	// Crée tous les menus de la gui
	const core::recti screenRect = core::recti(core::vector2di(0, 0), game->driver->getScreenSize());
	for (int i = 0; i < EGN_COUNT; ++i)
	{
		if (i == EGN_optionsMenuGUI)
		{
			// Le menu options dispose de sa propre classe CGUIOptionsMenu : on crée directement une instance de cette classe, et on l'assigne aussi au pointeur directement typé du menu options dans les éléments de la GUI
			GUIs[EGN_optionsMenuGUI] = guiElements.optionsMenuGUI.optionsMenu = new CGUIOptionsMenu(game->gui, rootElement, -1, screenRect);
		}
		else
		{
			GUIs[i] = new IGUIElement(EGUIET_ELEMENT, game->gui, rootElement, -1, screenRect);

			switch (i)
			{
			case EGN_mainMenuGUI:
				createMainMenuGUI(GUIs[i]);
				break;
			case EGN_newGameMenuGUI:
				createNewGameMenuGUI(GUIs[i]);
				break;
#ifdef USE_RAKNET
			case EGN_multiplayerMenuGUI:
				createMultiplayerMenuGUI(GUIs[i]);
				break;
#endif
			case EGN_gameEndGUI:
				createGameEndGUI(GUIs[i]);
				break;
			case EGN_gameGUI:
				createGameGUI(GUIs[i]);
				break;

			default:
				LOG_DEBUG("GUIManager::createGUIs : ID de la GUI inconnue : i = " << i, ELL_WARNING);
				break;
			}
		}

		GUIs[i]->drop();
	}

	// Change la transparence de la gui
	setGUITransparency(gameConfig.guiTransparency);

	// Masque toutes les GUIs pour éviter que le joueur ne les voie toutes se chevaucher
	hideAllGUIs();
}
void GUIManager::createGlobalGUI(IGUIElement* parent)
{
	const core::recti parentRect = parent->getRelativePosition();
	const core::dimension2di parentSize = parentRect.getSize();

	// Crée les éléments de base de la gui :

	// Crée le texte de la pause
	if (!guiElements.globalGUI.pauseTexte)
	{
		IGUISkin* const skin = game->gui->getSkin();
		const core::dimension2di halfParentSize = parentSize / 2;
		guiElements.globalGUI.pauseTexte = game->gui->addStaticText(L"PAUSE", core::recti(
			halfParentSize.Width - 100, halfParentSize.Height - 100,
			halfParentSize.Width + 100, halfParentSize.Height + 100), false, true, parent);
		guiElements.globalGUI.pauseTexte->setOverrideFont(game->gui->getFont("bigfont.png"));
		guiElements.globalGUI.pauseTexte->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER);			// Indique l'alignement du texte
		guiElements.globalGUI.pauseTexte->setOverrideColor(
			skin ? skin->getColor(gui::EGDC_ACTIVE_CAPTION) : video::SColor(0xffffffff));		// Change la couleur du texte
		guiElements.globalGUI.pauseTexte->setVisible(game->isPaused);							// Rend le texte visible suivant la pause actuelle
	}

	// Crée l'écran de chargement
	if (!guiElements.globalGUI.loadingScreen)
		guiElements.globalGUI.loadingScreen = new CLoadingScreen();

	// Crée les deux boîtes de dialogue pour ouvrir et sauvegarder une partie :

	for (u32 i = 0; i < 2; ++i)
	{
		const bool open = (i == 0);

		// Crée la banque d'icônes pour cette boîte de dialogue
		IGUISpriteBank* fileDialogSpriteBank = game->gui->addEmptySpriteBank(open ? "IconsOpen" : "IconsSave");

		// Crée cette boîte de dialogue
		CGUIFileSelector* fileDialog = new CGUIFileSelector(open ? L"Charger une partie" : L"Enregistrer la partie", game->gui, parent, -1,
			open ? CGUIFileSelector::EFST_OPEN_DIALOG : CGUIFileSelector::EFST_SAVE_DIALOG,
			getAbsoluteRectangle(0.2f, 0.15f, 0.8f, 0.85f, parentRect), fileDialogSpriteBank);
		if (fileDialog)
		{
			// Change le dossier de départ de la boîte de dialogue dans le dossier des sauvegardes
			fileDialog->setCurrentDirectory("./Saves");

			// Ajoute les textures spécifiées aux sprites puis les utilise dans la liste des fichiers
			fileDialog->setCustomFileIcon(game->driver->getTexture("file.png"));
			fileDialog->setCustomDirectoryIcon(game->driver->getTexture("folder.png"));

			// Ajoute le filtre des fichiers pour les parties EcoWorld ("*.ewg"), en lui associant une icône
			fileDialog->addFileFilter(L"EcoWorld Game", L"ewg",
#ifdef CAN_USE_ROMAIN_DARCEL_RESSOURCES
				(gameConfig.mainIconsSet == GameConfiguration::EMIS_NEXT_GEN ? game->driver->getTexture("world2.png") : game->driver->getTexture("world.png"))
#else
				game->driver->getTexture("world.png")
#endif
				);

			// Masque cette fenêtre
			fileDialog->hide();

			// Retiens cette fenêtre puis la libère
			if (open)
				guiElements.globalGUI.openFileDialog = fileDialog;
			else
				guiElements.globalGUI.saveFileDialog = fileDialog;
			fileDialog->drop();
		}
	}
}
void GUIManager::createMainMenuGUI(IGUIElement* parent)
{
	const core::recti parentRect = parent->getRelativePosition();

#if 1	// Nouveau titre avec taille variable, entre 2048x512 et 256x64
	bool scaleImage = false;
	const core::dimension2du titleSize = calculateMainTitleSize(game->driver->getScreenSize(), &scaleImage);
	const core::dimension2di parentSize = parentRect.getSize();
	const core::vector2di mainTitlePos((parentSize.Width - (int)titleSize.Width) / 2, (parentSize.Height - (int)titleSize.Height) / 14);
	const core::dimension2di mainTitleSize(min((int)titleSize.Width, parentSize.Width), min((int)titleSize.Height, parentSize.Height));

	// Crée l'image du titre principal (on ne charge pas l'image ici, elle devra être chargée après dans le programme)
	guiElements.mainMenuGUI.mainTitle = game->gui->addImage(core::recti(mainTitlePos, mainTitleSize), parent);
	guiElements.mainMenuGUI.mainTitle->setUseAlphaChannel(true);	// Active le canal Alpha pour la transparence de l'image
	guiElements.mainMenuGUI.mainTitle->setScaleImage(scaleImage);	// Active la mise à l'échelle si nécessaire (produit parfois des résultats peu appréciables, cette option sera donc désactivée la plupart du temps)
#else	// Ancien titre avec taille fixée de 512x128
	// Vérifie si le rectangle contenant l'image dépasse la taille de l'écran, et si c'est le cas, on la redimensionne
	const core::dimension2di titleSize(512, 128);	// Dimensions de l'image de titre
	const core::dimension2di parentSize = parentRect.getSize();
	core::recti mainTitleRect = core::recti(
		core::vector2di((parentSize.Width - titleSize.Width) / 2, (int)((float)parentSize.Height * 0.05f)),
		titleSize);
	bool scaleImage = false;
	{
		// Diminue la largeur de l'image si elle est trop grande
		if (mainTitleRect.getWidth() > parentSize.Width)
		{
			mainTitleRect.UpperLeftCorner.X = 0;
			mainTitleRect.LowerRightCorner.X = parentSize.Width;

			// Recentre le rectangle en X
			mainTitleRect.UpperLeftCorner.X = (parentSize.Width - mainTitleRect.getWidth()) / 2;

			// Indique que l'image devra être redimensionnée
			scaleImage = true;
		}

		// Diminue la hauteur de l'image si elle est trop grande
		if (mainTitleRect.getHeight() > parentSize.Height)
		{
			mainTitleRect.UpperLeftCorner.Y = 0;
			mainTitleRect.LowerRightCorner.Y = parentSize.Height;

			// Met l'ordonnée du rectangle à zéro
			mainTitleRect.UpperLeftCorner.Y = 0;

			// Indique que l'image devra être redimensionnée
			scaleImage = true;
		}
	}

	// Crée l'image du titre principal
	guiElements.mainMenuGUI.mainTitle = game->gui->addImage(game->driver->getTexture("MainTitle.png"), core::vector2di(0, 0), true, parent);
	guiElements.mainMenuGUI.mainTitle->setRelativePosition(mainTitleRect);
	guiElements.mainMenuGUI.mainTitle->setScaleImage(scaleImage);
#endif

	// Crée les boutons
#if 0
	// Les boutons occupent ici le centre de l'écran
	const float minX = 0.3f, maxX = 0.7f, minY = 0.3f, maxY = 0.15f, ecartBoutonY = 0.05f;
	const float tailleBoutonY = (1.0f - minY - maxY - ecartBoutonY * 4.0f) * 0.2f;	// (...) / 5.0f

	guiElements.mainMenuGUI.newGameBouton = game->gui->addButton(getAbsoluteRectangle(minX, minY, maxX, minY + tailleBoutonY, parentRect),
		parent, -1, L"Nouvelle partie", L"Commence une nouvelle partie");
	guiElements.mainMenuGUI.chargerBouton = game->gui->addButton(getAbsoluteRectangle(minX, minY + ecartBoutonY + tailleBoutonY, maxX, minY + ecartBoutonY + tailleBoutonY * 2, parentRect),
		parent, -1, L"Charger une partie", L"Charge une partie enregistrée");
#ifdef USE_RAKNET
	guiElements.mainMenuGUI.multiplayerBouton = game->gui->addButton(getAbsoluteRectangle(minX, minY + ecartBoutonY * 2 + tailleBoutonY * 2, maxX, minY + ecartBoutonY * 2 + tailleBoutonY * 3, parentRect),
		parent, -1, L"Multijoueur", L"Créer ou rejoindre une partie multijoueur");
#else
	guiElements.mainMenuGUI.multiplayerBouton = game->gui->addButton(getAbsoluteRectangle(minX, minY + ecartBoutonY * 2 + tailleBoutonY * 2, maxX, minY + ecartBoutonY * 2 + tailleBoutonY * 3, parentRect),
		parent, -1, L"Multijoueur", L"Le mode multijoueur n'est pas disponible avec cette version du jeu");
	guiElements.mainMenuGUI.multiplayerBouton->setEnabled(false);
#endif
	guiElements.mainMenuGUI.optionsBouton = game->gui->addButton(getAbsoluteRectangle(minX, minY + ecartBoutonY * 3 + tailleBoutonY * 3, maxX, minY + ecartBoutonY * 3 + tailleBoutonY * 4, parentRect),
		parent, -1, L"Options", L"Modifie les options du jeu");
	guiElements.mainMenuGUI.quitterBouton = game->gui->addButton(getAbsoluteRectangle(minX, minY + ecartBoutonY * 4 + tailleBoutonY * 4, maxX, minY + ecartBoutonY * 4 + tailleBoutonY * 5, parentRect),
		parent, -1, L"Quitter", L"Quitte le jeu");
#else
	// Les boutons occupent ici la partie inférieure de l'écran
	const float minX = 0.07f, maxX = 0.07f, minY = 0.82f, maxY = 0.9f, ecartBoutonX = 0.03f;
	const float tailleBoutonX = (1.0f - minX - maxX - ecartBoutonX * 4.0f) * 0.2f;	// (...) / 5.0f

	guiElements.mainMenuGUI.newGameBouton = game->gui->addButton(getAbsoluteRectangle(minX, minY, minX + tailleBoutonX, maxY, parentRect),
		parent, -1, L"Nouvelle partie", L"Commence une nouvelle partie");
	guiElements.mainMenuGUI.chargerBouton = game->gui->addButton(getAbsoluteRectangle(minX + ecartBoutonX + tailleBoutonX, minY, minX + ecartBoutonX + tailleBoutonX * 2, maxY, parentRect),
		parent, -1, L"Charger une partie", L"Charge une partie enregistrée");
#ifdef USE_RAKNET
	guiElements.mainMenuGUI.multiplayerBouton = game->gui->addButton(getAbsoluteRectangle(minX + ecartBoutonX * 2 + tailleBoutonX * 2, minY, minX + ecartBoutonX * 2 + tailleBoutonX * 3, maxY, parentRect),
		parent, -1, L"Multijoueur", L"Créer ou rejoindre une partie multijoueur");
#else
	guiElements.mainMenuGUI.multiplayerBouton = game->gui->addButton(getAbsoluteRectangle(minX + ecartBoutonX * 2 + tailleBoutonX * 2, minY, minX + ecartBoutonX * 2 + tailleBoutonX * 3, maxY, parentRect),
		parent, -1, L"Multijoueur", L"Le mode multijoueur n'est pas disponible avec cette version du jeu");
	guiElements.mainMenuGUI.multiplayerBouton->setEnabled(false);
#endif
	guiElements.mainMenuGUI.optionsBouton = game->gui->addButton(getAbsoluteRectangle(minX + ecartBoutonX * 3 + tailleBoutonX * 3, minY, minX + ecartBoutonX * 3 + tailleBoutonX * 4, maxY, parentRect),
		parent, -1, L"Options", L"Modifie les options du jeu");
	guiElements.mainMenuGUI.quitterBouton = game->gui->addButton(getAbsoluteRectangle(minX + ecartBoutonX * 4 + tailleBoutonX * 4, minY, minX + ecartBoutonX * 4 + tailleBoutonX * 5, maxY, parentRect),
		parent, -1, L"Quitter", L"Quitte le jeu");
#endif
}
void GUIManager::loadMainMenuTitles(const core::dimension2du& screenSize)
{
	if (!game->driver)
		return;

	// Modifie les paramêtres de création de texture du game->driver : force sa création en mode 32 bits et avec la composante Alpha sans niveaux de mip map, pour augmenter sa qualité visuelle
	const bool lastDriverTextureCreationStates[6] = {
		game->driver->getTextureCreationFlag(video::ETCF_ALWAYS_16_BIT),
		game->driver->getTextureCreationFlag(video::ETCF_ALWAYS_32_BIT),
		game->driver->getTextureCreationFlag(video::ETCF_OPTIMIZED_FOR_QUALITY),
		game->driver->getTextureCreationFlag(video::ETCF_OPTIMIZED_FOR_SPEED),
		game->driver->getTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS),
		game->driver->getTextureCreationFlag(video::ETCF_NO_ALPHA_CHANNEL), };
	game->driver->setTextureCreationFlag(video::ETCF_ALWAYS_16_BIT, false);
	game->driver->setTextureCreationFlag(video::ETCF_ALWAYS_32_BIT, true);
	game->driver->setTextureCreationFlag(video::ETCF_OPTIMIZED_FOR_QUALITY, false);
	game->driver->setTextureCreationFlag(video::ETCF_OPTIMIZED_FOR_SPEED, false);
	game->driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, false);
	game->driver->setTextureCreationFlag(video::ETCF_NO_ALPHA_CHANNEL, false);

	// Suffixe pour déterminer l'adresse des textures du menu principal
	const u32 titleSize = calculateMainTitleSize(screenSize).Width;

	// Chargement des 4 textures à la suite sous la forme : MainTitle_taille_id.bmp (ex : MainTitle_2048_1.bmp)
	for (u32 i = 1; i < 5; ++i)	// i <= 4
	{
#ifdef CAN_USE_ROMAIN_DARCEL_RESSOURCES
		if (gameConfig.mainIconsSet == GameConfiguration::EMIS_NEXT_GEN)
			sprintf_SS("MainTitle2_%u_%u.png", titleSize, i);
		else
#endif
			sprintf_SS("MainTitle_%u_%u.bmp", titleSize, i);
		game->driver->getTexture(text_SS);
	}

	// Restaure les paramêtres de création de texture du game->driver
	game->driver->setTextureCreationFlag(video::ETCF_ALWAYS_16_BIT, lastDriverTextureCreationStates[0]);
	game->driver->setTextureCreationFlag(video::ETCF_ALWAYS_32_BIT, lastDriverTextureCreationStates[1]);
	game->driver->setTextureCreationFlag(video::ETCF_OPTIMIZED_FOR_QUALITY, lastDriverTextureCreationStates[2]);
	game->driver->setTextureCreationFlag(video::ETCF_OPTIMIZED_FOR_SPEED, lastDriverTextureCreationStates[3]);
	game->driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, lastDriverTextureCreationStates[4]);
	game->driver->setTextureCreationFlag(video::ETCF_NO_ALPHA_CHANNEL, lastDriverTextureCreationStates[5]);
}
video::ITexture* GUIManager::getMainMenuTitleFromWeather(WeatherID weather, const core::dimension2du& screenSize)
{
	if (!game->driver)
		return NULL;

	// Crée la texture du titre d'après le temps actuel :

	// Choisis l'ID du titre du menu principal d'arpsè le temps actuel
	const u32 titleID = (weather == WI_sunny ? 1 : (int)weather);

	// Choisis l'adresse du fichier de texture du titre du menu principal
	const u32 titleSize = calculateMainTitleSize(screenSize).Width;
#ifdef CAN_USE_ROMAIN_DARCEL_RESSOURCES
	if (gameConfig.mainIconsSet == GameConfiguration::EMIS_NEXT_GEN)
		sprintf_SS("MainTitle2_%u_%u.png", titleSize, titleID);
	else
#endif
		sprintf_SS("MainTitle_%u_%u.bmp", titleSize, titleID);

	// Renvoit la texture du titre choisie
	return game->driver->getTexture(text_SS);
}
void GUIManager::chooseMainMenuTitleFromWeather(WeatherID weather, const core::dimension2du& screenSize)
{
	if (!game->driver || !guiElements.mainMenuGUI.mainTitle)
		return;

	// Crée la texture du titre d'après le temps actuel :

	// Charge la texture du titre et l'indique à la gui (elle a normalement déjà été préchargée, cette étape devrait être rapide)
	guiElements.mainMenuGUI.mainTitle->setImage(getMainMenuTitleFromWeather(weather, screenSize));
}
void GUIManager::createNewGameMenuGUI(IGUIElement* parent)
{
	const core::recti parentRect = parent->getRelativePosition();

	const float positionMainBoutonsY = 0.85f,
		tailleMainBoutonsX = 0.1f,
		tailleMainBoutonsY = 0.05f;

	// Crée le groupe d'onglets principal et ses onglets
	guiElements.newGameMenuGUI.mainTabGroup = game->gui->addTabControl(getAbsoluteRectangle(0.03f, 0.03f, 0.97f, 0.82f, parentRect), parent, true, true);

	// Règle la taille des onglets (ne fonctionne pas encore)	// TODO : Revoir ces valeurs pour que cela fonctionne quelle que soit la résolution choisie
	guiElements.newGameMenuGUI.mainTabGroup->setTabExtraWidth((int)(0.1f * (float)parentRect.getWidth()));	// Valeur par défaut : 20
	guiElements.newGameMenuGUI.mainTabGroup->setTabHeight((int)(0.03f * (float)parentRect.getHeight()));	// Valeur par défaut : 32

	// Crée les onglets
	guiElements.newGameMenuGUI.terrainTab = guiElements.newGameMenuGUI.mainTabGroup->addTab(L"Terrain");
	guiElements.newGameMenuGUI.objectivesTab = guiElements.newGameMenuGUI.mainTabGroup->addTab(L"Objectifs");
#ifdef USE_RAKNET
	guiElements.newGameMenuGUI.multiplayerTab = guiElements.newGameMenuGUI.mainTabGroup->addTab(L"Multijoueur");
#endif

	const core::recti parentTabRect = guiElements.newGameMenuGUI.terrainTab->getRelativePosition();

	// Remplit l'onglet du choix du terrain :
	game->gui->addStaticText(L"Terrains disponibles :", getAbsoluteRectangle(0.05f, 0.05f, 0.45f, 0.1f, parentTabRect),
		false, true, guiElements.newGameMenuGUI.terrainTab)->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);
	guiElements.newGameMenuGUI.terrainListBox = game->gui->addListBox(getAbsoluteRectangle(0.05f, 0.1f, 0.45f, 0.9f, parentTabRect), guiElements.newGameMenuGUI.terrainTab, -1, true);
	guiElements.newGameMenuGUI.terrainListBox->setToolTipText(L"Choisissez le terrain sur lequel vous désirez construire votre ville");

	game->gui->addStaticText(L"Description du terrain :", getAbsoluteRectangle(0.5f, 0.05f, 0.95f, 0.1f, parentTabRect),
		false, true, guiElements.newGameMenuGUI.terrainTab)->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);
	guiElements.newGameMenuGUI.terrainDescriptionTexte = game->gui->addStaticText(L"", getAbsoluteRectangle(0.5f, 0.1f, 0.95f, 0.35f, parentTabRect), true, true, guiElements.newGameMenuGUI.terrainTab);
	guiElements.newGameMenuGUI.terrainDescriptionTexte->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_UPPERLEFT);

	// Calcul de la taille du rectangle relatif de guiElements.newGameMenuGUI.terrainApercuImage pour qu'il soit un carré :
	/*
		ParentRect(0.03f, 0.03f, 0.97f, 0.82f)	;	Rect(0.5f, 0.45f, x, y) :

		(0.97f - 0.03f) * (x - 0.5f)	=	(0.82f - 0.03f) * (y - 0.45f)
		<=>			0.94f * x - 0.47	=	0.79f * y - 0.3555
		<=>					0.94f * x	=	0.79f * y + 0.1145
		<=>							x	=	(y * 0.79f + 0.1145) / 0.94f
		<=>							y	=	(x * 0.94f - 0.1145) / 0.79f

		Pour y = 0.9f :
		x = (0.9f * 0.79f + 0.1145) / 0.94f
		x = 0.87819148936170212765957446808511 ~= 0.8781915
	*/
	game->gui->addStaticText(L"Aperçu du terrain :", getAbsoluteRectangle(0.5f, 0.4f, 0.9f, 0.45f, parentTabRect),
		false, true, guiElements.newGameMenuGUI.terrainTab)->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);
	guiElements.newGameMenuGUI.terrainApercuImage = game->gui->addImage(getAbsoluteRectangle(0.5f, 0.45f, 0.8781915f, 0.9f, parentTabRect), guiElements.newGameMenuGUI.terrainTab);
	guiElements.newGameMenuGUI.terrainApercuImage->setScaleImage(true);
	guiElements.newGameMenuGUI.terrainApercuImage->setUseAlphaChannel(true);

	// Remplit l'onglet du choix des objectifs :
	// Crée la liste permettant de choisir la difficulté de cette partie
	game->gui->addStaticText(L"Difficulté du jeu :", getAbsoluteRectangle(0.05f, 0.05f, 0.17f, 0.1f, parentTabRect),
		false, true, guiElements.newGameMenuGUI.objectivesTab)->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);
	guiElements.newGameMenuGUI.difficultyComboBox = game->gui->addComboBox(getAbsoluteRectangle(0.18f, 0.05f, 0.37f, 0.1f, parentTabRect), guiElements.newGameMenuGUI.objectivesTab);
	guiElements.newGameMenuGUI.difficultyComboBox->setToolTipText(L"Réglez ici le niveau de difficulté de cette partie");
	guiElements.newGameMenuGUI.difficultyComboBox->addItem(L"Facile", (u32)EcoWorldModifiers::ED_facile);
	guiElements.newGameMenuGUI.difficultyComboBox->setSelected(guiElements.newGameMenuGUI.difficultyComboBox->addItem(L"Normal", (u32)EcoWorldModifiers::ED_normal));
	guiElements.newGameMenuGUI.difficultyComboBox->addItem(L"Difficile", (u32)EcoWorldModifiers::ED_difficile);

	// Crée la liste modifiable des objectifs
	if (!guiElements.newGameMenuGUI.objectivesModifier)
	{
		guiElements.newGameMenuGUI.objectivesModifier = new CGUIObjectivesModifier(game->gui, guiElements.newGameMenuGUI.objectivesTab, -1,
			getAbsoluteRectangle(0.05f, 0.15f, 0.95f, 0.9f, parentTabRect));
		guiElements.newGameMenuGUI.objectivesModifier->drop();
	}

#ifdef USE_RAKNET
	// Remplit l'onglet du mode multijoueur :
	guiElements.newGameMenuGUI.multiplayerEnabledCheckBox = game->gui->addCheckBox(false, getAbsoluteRectangle(0.05f, 0.05f, 0.95f, 0.1f, parentTabRect),
		guiElements.newGameMenuGUI.multiplayerTab, -1, L"Mode multijoueur activé");
	guiElements.newGameMenuGUI.multiplayerEnabledCheckBox->setToolTipText(L"Cochez cette case pour permettre à d'autres joueurs de rejoindre votre partie en mode multijoueur");
#endif

	// Crée les boutons du bas
	guiElements.newGameMenuGUI.retourBouton = game->gui->addButton(getAbsoluteRectangle(0.15f - tailleMainBoutonsX * 0.5f, positionMainBoutonsY, 0.15f + tailleMainBoutonsX * 0.5f, positionMainBoutonsY + tailleMainBoutonsY, parentRect),
		parent, -1, L"Retour", L"Retourne au menu principal");
	guiElements.newGameMenuGUI.commencerBouton = game->gui->addButton(getAbsoluteRectangle(0.85f - tailleMainBoutonsX * 0.5f, positionMainBoutonsY, 0.85f + tailleMainBoutonsX * 0.5f, positionMainBoutonsY + tailleMainBoutonsY, parentRect),
		parent, -1, L"Commencer", L"ERREUR");	// L"ERREUR : Fonction GUIManager::updateListTerrainsNewGameGUI() non appelée !"
	guiElements.newGameMenuGUI.commencerBouton->setEnabled(false);	// Désactive ce bouton par défaut

	// Met à jour la liste des terrains
	// Désactivé : Effectué automatiquement dans Game::switchToNextScene lors de l'affichage de ce menu
	//updateListTerrainsNewGameGUI(game->fileSystem, gameEventReceiver);
}
void GUIManager::updateListTerrainsNewGameGUI(IEventReceiver* gameEventReceiver)
{
	if (!guiElements.newGameMenuGUI.terrainListBox)
		return;

	const u32 lastSelectedItem = guiElements.newGameMenuGUI.terrainListBox->getSelected();
	const core::stringw lastSelectedTerrain = lastSelectedItem != -1 ? guiElements.newGameMenuGUI.terrainListBox->getListItem(lastSelectedItem) : L"";

	// Désélectionne la valeur actuellement sélectionnée
	guiElements.newGameMenuGUI.terrainListBox->setSelected(-1);

	// Efface les anciennes valeurs de la liste
	guiElements.newGameMenuGUI.terrainListBox->clear();

	// Ajoute tous les fichiers du système de fichier d'Irrlicht avec l'extension ".ewt" dans la liste des terrains (inspiré depuis Game::addGameArchives)
	const u32 fileArchiveCount = game->fileSystem->getFileArchiveCount();
	for (u32 i = 0; i < fileArchiveCount; ++i)
	{
		const io::IFileList* const fileList = game->fileSystem->getFileArchive(i)->getFileList();
		const u32 fileCount = fileList->getFileCount();
		for (u32 j = 0; j < fileCount; ++j)
		{
			if (fileList->isDirectory(j))	continue;						// Vérifie que le fichier actuel n'est pas un dossier

			const io::path& fileName = fileList->getFileName(j);			// Obtient le nom actuel du fichier
			const int extPos = fileName.findLast('.');						// Trouve le dernier '.' du nom du fichier, pour vérifier son extension
			if (extPos < 0)	continue;										// Vérifie que le nom du fichier contient bien un '.'
			const io::path extension = fileName.subString(extPos + 1, 4);	// Obtient l'extension du fichier à 4 caractères maximum (on supprime aussi le '.' de l'extension)
			if (!extension.equals_ignore_case("ewt"))	continue;			// Vérifie que l'extension du fichier actuel est bien ".ewt"

			// Le fichier est bien un terrain :

			// Enlève l'extension ".ewt" du nom du fichier
			core::stringw terrainName = fileName;
			terrainName.remove(L".ewt");

			// Parcours la liste pour vérifier qu'un terrain de ce nom n'a pas déjà été ajouté :
			// (évite d'avoir deux occurrences de terrain avec un nom identique malgré un fichier différent : on ne conserve que le premier des deux)
			bool alreadyExists = false;
			const u32 terrainItemCount = guiElements.newGameMenuGUI.terrainListBox->getItemCount();
			for (u32 k = 0; k < terrainItemCount; ++k)
			{
				if (terrainName == guiElements.newGameMenuGUI.terrainListBox->getListItem(k))
				{
					alreadyExists = true;
					break;
				}
			}

			// Ajoute ce terrain à la liste des terrains
			if (!alreadyExists)
				guiElements.newGameMenuGUI.terrainListBox->addItem(terrainName.c_str());
		}
	}

	// Active ou désactive le bouton "Commencer" si des terrains sont disponibles
	if (guiElements.newGameMenuGUI.commencerBouton)
	{
		if (guiElements.newGameMenuGUI.terrainListBox->getItemCount() > 0)
		{
			guiElements.newGameMenuGUI.commencerBouton->setEnabled(true);
			guiElements.newGameMenuGUI.commencerBouton->setToolTipText(L"Commence une nouvelle partie avec les paramêtres sélectionnés");
		}
		else
		{
			guiElements.newGameMenuGUI.commencerBouton->setToolTipText(L"Impossible de commencer une partie si aucun terrain n'est disponible");
			guiElements.newGameMenuGUI.commencerBouton->setEnabled(false);
		}
	}

	// Vérifie que l'on veut sélectionner un élement
	const u32 listItemCount = guiElements.newGameMenuGUI.terrainListBox->getItemCount();
	if (listItemCount > 0)
	{
		if (lastSelectedTerrain.size() > 0)
		{
			// Cherche le nom du terrain dans la liste et le sélectionne (s'il n'a pas été trouvé, on sélectionnera le premier élement de la liste)
			int ID = 0;
			for (u32 i = 0; i < listItemCount; ++i)
			{
				if (lastSelectedTerrain.equals_ignore_case(guiElements.newGameMenuGUI.terrainListBox->getListItem(i)))
				{
					ID = i;
					break;
				}
			}

			guiElements.newGameMenuGUI.terrainListBox->setSelected(ID);
		}
		else
		{
			// Sinon on sélectionne le premier élement
			guiElements.newGameMenuGUI.terrainListBox->setSelected(0);
		}
	}

	// Envoie finalement un event à Game pour lui indiquer qu'on a changé le terrain sélectionné de la liste des terrains
	// et qu'il doit ainsi mettre à jour l'image de prévisualisation
	if (gameEventReceiver)
	{
		SEvent event;
		event.EventType = EET_GUI_EVENT;
		event.GUIEvent.EventType = EGET_LISTBOX_CHANGED;
		event.GUIEvent.Caller = guiElements.newGameMenuGUI.terrainListBox;
		event.GUIEvent.Element = NULL;
		gameEventReceiver->OnEvent(event);
	}
}
#ifdef USE_RAKNET
void GUIManager::createMultiplayerMenuGUI(IGUIElement* parent)
{
	const core::recti parentRect = parent->getRelativePosition();

	const float positionMainBoutonsY = 0.85f,
		tailleMainBoutonsX = 0.1f,
		tailleMainBoutonsY = 0.05f;

	// Ajoute le tableau affichant les parties multijoueurs
	game->gui->addStaticText(L"Parties multijoueurs disponibles :", getAbsoluteRectangle(0.05f, 0.05f, 0.95f, 0.1f, parentRect),
		false, true, parent)->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);
	guiElements.multiplayerMenuGUI.multiplayerGamesTable = new CGUISortTable(game->gui, parent, -1,
		getAbsoluteRectangle(0.05f, 0.1f, 0.95f, 0.8f, parentRect), true, true, false);
	guiElements.multiplayerMenuGUI.multiplayerGamesTable->drop();
	guiElements.multiplayerMenuGUI.multiplayerGamesTable->setToolTipText(L"Choisissez ici la partie multijoueur que vous souhaitez rejoindre");

	// Crée les colonnes de ce tableau
	guiElements.multiplayerMenuGUI.multiplayerGamesTable->addColumn(L"Adresse IP de l'hôte");
	guiElements.multiplayerMenuGUI.multiplayerGamesTable->addColumn(L"Nom du terrain");
	guiElements.multiplayerMenuGUI.multiplayerGamesTable->addColumn(L"Nombre de participants actuel / maximal");
	guiElements.multiplayerMenuGUI.multiplayerGamesTable->addColumn(L"Version du jeu de l'hôte");

	// Indique la méthode de tri de chaque colonne
	const int columnCount = guiElements.multiplayerMenuGUI.multiplayerGamesTable->getColumnCount();
	for (int i = 0; i < columnCount; ++i)
		guiElements.multiplayerMenuGUI.multiplayerGamesTable->setColumnOrdering(i, EGCO_FLIP_ASCENDING_DESCENDING);

	// Indique la première colonne comme active (sans trier les lignes du tableau, car il n'y en a pas)
	guiElements.multiplayerMenuGUI.multiplayerGamesTable->setActiveColumn(0);

	// Crée les boutons du bas
	guiElements.multiplayerMenuGUI.retourBouton = game->gui->addButton(getAbsoluteRectangle(0.15f - tailleMainBoutonsX * 0.5f, positionMainBoutonsY, 0.15f + tailleMainBoutonsX * 0.5f, positionMainBoutonsY + tailleMainBoutonsY, parentRect),
		parent, -1, L"Retour", L"Retourne au menu principal");
	guiElements.multiplayerMenuGUI.rejoindreBouton = game->gui->addButton(getAbsoluteRectangle(0.85f - tailleMainBoutonsX * 0.5f, positionMainBoutonsY, 0.85f + tailleMainBoutonsX * 0.5f, positionMainBoutonsY + tailleMainBoutonsY, parentRect),
		parent, -1, L"Rejoindre", L"Aucune partie multijoueur n'est disponible");
	guiElements.multiplayerMenuGUI.rejoindreBouton->setEnabled(false);	// Désactive ce bouton par défaut
}
void GUIManager::updateMultiplayerGamesTableNewGameGUI(const core::list<MultiplayerGameListInfos>& gamesListInfos)
{
	if (!guiElements.multiplayerMenuGUI.multiplayerGamesTable)
		return;

	// Obtient les couleurs pour les cellules du tableau
	const video::SColor defaultColor = game->gui->getSkin() ? game->gui->getSkin()->getColor(EGDC_BUTTON_TEXT) : video::SColor(255, 0, 0, 0);
	const video::SColor greenColor(defaultColor.getAlpha(), defaultColor.getRed(), 150, defaultColor.getBlue());
	const video::SColor redColor(defaultColor.getAlpha(), 200, defaultColor.getGreen(), defaultColor.getBlue());

	const int lastSelectedRow = guiElements.multiplayerMenuGUI.multiplayerGamesTable->getSelected();
	//const core::stringw lastSelectedGame = lastSelectedRow != -1 ? guiElements.multiplayerMenuGUI.multiplayerGamesTable->getCellText(lastSelectedRow, 0) : L"";

	// Désélectionne la valeur actuellement sélectionnée
	guiElements.multiplayerMenuGUI.multiplayerGamesTable->setSelected(-1);

	// Efface toutes les lignes du tableau
	guiElements.multiplayerMenuGUI.multiplayerGamesTable->clearRows();

	const core::list<MultiplayerGameListInfos>::ConstIterator END = gamesListInfos.end();
	for (core::list<MultiplayerGameListInfos>::ConstIterator it = gamesListInfos.begin(); it != END; ++it)
	{
		// Obtient les informations sur cette partie
		const MultiplayerGameListInfos& gameInfos = (*it);

		// Ajoute une ligne pour ce bâtiment
		const u32 row = guiElements.multiplayerMenuGUI.multiplayerGamesTable->addRow(guiElements.multiplayerMenuGUI.multiplayerGamesTable->getRowCount());
		u32 column = 0;



		// Indique les valeurs des cellules de cette ligne :

		// Détermine la couleur des textes ayant rapport au nombre de participants : rouge si la partie est remplie, vert sinon
		const video::SColor& participantsColor = (gameInfos.currentParticipants == gameInfos.maxParticipants ? redColor : greenColor);

		// Adresse IP de l'hôte
		{
			guiElements.multiplayerMenuGUI.multiplayerGamesTable->setCellText(row, column, gameInfos.hostIP.c_str());
			guiElements.multiplayerMenuGUI.multiplayerGamesTable->setCellData(row, column, guiElements.multiplayerMenuGUI.multiplayerGamesTable->getTriTypePointer(CGUISortTable::ETT_IP_ADDRESS));
		} column++;

		// Nom du terrain
		{
			// Indique la couleur de la cellule suivant si le terrain existe ou non :
			bool existsTerrain = game->fileSystem->existFile(gameInfos.terrainName);
			if (!existsTerrain)
			{
				// Si le terrain n'a pas été trouvé, on lui ajoute l'extension ".ewt" puis on le recherche à nouveau
				io::path tmpTerrainName = gameInfos.terrainName;
				tmpTerrainName.append(".ewt");
				existsTerrain = game->fileSystem->existFile(tmpTerrainName);
			}

			guiElements.multiplayerMenuGUI.multiplayerGamesTable->setCellText(row, column, gameInfos.terrainName.c_str(),
				(existsTerrain ? greenColor : redColor));
			guiElements.multiplayerMenuGUI.multiplayerGamesTable->setCellData(row, column, guiElements.multiplayerMenuGUI.multiplayerGamesTable->getTriTypePointer(CGUISortTable::ETT_ALPHABETICAL));
		} column++;

		// Nombre de participants actuel / maximal
		{
			swprintf_SS(L"%u / %u", gameInfos.currentParticipants, gameInfos.maxParticipants);
			guiElements.multiplayerMenuGUI.multiplayerGamesTable->setCellText(row, column, textW_SS, participantsColor);
			guiElements.multiplayerMenuGUI.multiplayerGamesTable->setCellData(row, column, guiElements.multiplayerMenuGUI.multiplayerGamesTable->getTriTypePointer(CGUISortTable::ETT_VALUE));
		} column++;

		// Version du jeu de l'hôte
		{
			guiElements.multiplayerMenuGUI.multiplayerGamesTable->setCellText(row, column, gameInfos.hostGameVersion,
				(gameInfos.hostGameVersion.equals_ignore_case(ECOWORLD_VERSION) ? greenColor : redColor));
			guiElements.multiplayerMenuGUI.multiplayerGamesTable->setCellData(row, column, guiElements.multiplayerMenuGUI.multiplayerGamesTable->getTriTypePointer(CGUISortTable::ETT_IP_ADDRESS));	// Ce type de tri peut aussi gérer les versions de jeu
		} column++;
	}

	// Active ou désactive le bouton "Rejoindre" si des parties sont disponibles
	if (guiElements.multiplayerMenuGUI.rejoindreBouton)
	{
		if (guiElements.multiplayerMenuGUI.multiplayerGamesTable->getRowCount() > 0)
		{
			guiElements.multiplayerMenuGUI.rejoindreBouton->setEnabled(true);
			guiElements.multiplayerMenuGUI.rejoindreBouton->setToolTipText(L"Rejoindre cette partie");
		}
		else
		{
			guiElements.multiplayerMenuGUI.rejoindreBouton->setToolTipText(L"Aucune partie multijoueur n'est disponible");
			guiElements.multiplayerMenuGUI.rejoindreBouton->setEnabled(false);
		}
	}

	// TODO : Calculer et indiquer automatiquement la largeur de chaque colonne suivant la taille du texte dans leurs cellules
	//			(sans jamais diminuer la taille des colonnes, toujours en les agrandissant si nécessaire)

	// Trie le tableau avec les nouvelles lignes ajoutées
	guiElements.multiplayerMenuGUI.multiplayerGamesTable->orderRows(-1, guiElements.multiplayerMenuGUI.multiplayerGamesTable->getActiveColumnOrdering());

	// Sélectionne le premier élément de la liste si aucun n'est sélectionné
	if (lastSelectedRow < 0)
		guiElements.multiplayerMenuGUI.multiplayerGamesTable->setSelected(0);
	else	// Sinon, on restaure la dernière ligne sélectionnée du tableau
		guiElements.multiplayerMenuGUI.multiplayerGamesTable->setSelected(lastSelectedRow);
}
#endif
void GUIManager::createGameEndGUI(IGUIElement* parent)
{
	const core::recti parentRect = parent->getRelativePosition();

	guiElements.gameEndGUI.fondImage = game->gui->addImage(parentRect, parent);

	guiElements.gameEndGUI.endTexte = game->gui->addStaticText(L"ERREUR",	// L"ERREUR : endTexte non modifié !"
		getAbsoluteRectangle(0.1f, 0.1f, 0.9f, 0.7f, parentRect), false, true, parent);
	guiElements.gameEndGUI.endTexte->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER);
	guiElements.gameEndGUI.endTexte->setOverrideFont(game->gui->getFont("bigfont.png"));
	guiElements.gameEndGUI.endTexte->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER);			// Indique l'alignement du texte
	guiElements.gameEndGUI.endTexte->setOverrideColor(video::SColor(255, 255, 255, 255));	// Change la couleur du texte

	// Crée les boutons pour continuer la partie ou retourner au menu principal :

	guiElements.gameEndGUI.continuerPartieBouton = game->gui->addButton(getAbsoluteRectangle(0.35f, 0.72f, 0.65f, 0.8f, parentRect),
		parent, -1, L"Continuer la partie", L"Cliquez sur ce bouton si vous voulez continuer cette partie");
	guiElements.gameEndGUI.continuerPartieBouton->setVisible(false);	// Masque ce bouton par défaut

	guiElements.gameEndGUI.retourMenuPrincipalBouton = game->gui->addButton(getAbsoluteRectangle(0.35f, 0.82f, 0.65f, 0.9f, parentRect),
		parent, -1, L"Retourner au menu principal", L"Cliquez sur ce bouton pour retourner au menu principal");
}
void GUIManager::createGameGUI(IGUIElement* parent)
{
	const core::recti parentRect = parent->getRelativePosition();

	// Crée le tableau de commande du haut
	guiElements.gameGUI.tableauHaut = game->gui->addImage(getAbsoluteRectangle(0.0f, 0.0f, 1.0f, 0.03f, parentRect), parent);

	// budgetTexte
	guiElements.gameGUI.budgetTexte = game->gui->addStaticText(L"Budget : 0.00 €",
		getAbsoluteRectangle(0.0f, 0.0f, 0.2f, 1.0f, guiElements.gameGUI.tableauHaut->getAbsoluteClippingRect()),
		true, true, guiElements.gameGUI.tableauHaut, -1, false);
	guiElements.gameGUI.budgetTexte->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER); // Indique l'alignement du texte
	guiElements.gameGUI.budgetTexte->setOverrideColor(video::SColor(255, 255, 255, 255)); // Change la couleur du texte

	// energieTexte
	guiElements.gameGUI.energieTexte = game->gui->addStaticText(L"Energie : 0 W",
		getAbsoluteRectangle(0.2f, 0.0f, 0.35f, 1.0f, guiElements.gameGUI.tableauHaut->getAbsoluteClippingRect()),
		true, true, guiElements.gameGUI.tableauHaut, -1, false);
	guiElements.gameGUI.energieTexte->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER); // Indique l'alignement du texte
	guiElements.gameGUI.energieTexte->setOverrideColor(video::SColor(255, 255, 255, 255)); // Change la couleur du texte

	// effetSerreTexte
	guiElements.gameGUI.effetSerreTexte = game->gui->addStaticText(L"Effet de serre : 0 kg",
		getAbsoluteRectangle(0.35f, 0.0f, 0.5f, 1.0f, guiElements.gameGUI.tableauHaut->getAbsoluteClippingRect()),
		true, true, guiElements.gameGUI.tableauHaut, -1, false);
	guiElements.gameGUI.effetSerreTexte->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER); // Indique l'alignement du texte
	guiElements.gameGUI.effetSerreTexte->setOverrideColor(video::SColor(255, 255, 255, 255)); // Change la couleur du texte

	// dechetsTexte
	guiElements.gameGUI.dechetsTexte = game->gui->addStaticText(L"Déchets : 0 kg",
		getAbsoluteRectangle(0.5f, 0.0f, 0.65f, 1.0f, guiElements.gameGUI.tableauHaut->getAbsoluteClippingRect()),
		true, true, guiElements.gameGUI.tableauHaut, -1, false);
	guiElements.gameGUI.dechetsTexte->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER); // Indique l'alignement du texte
	guiElements.gameGUI.dechetsTexte->setOverrideColor(video::SColor(255, 255, 255, 255)); // Change la couleur du texte

	// populationTexte
	guiElements.gameGUI.populationTexte = game->gui->addStaticText(L"0 Habitants",
		getAbsoluteRectangle(0.65f, 0.0f, 0.8f, 1.0f, guiElements.gameGUI.tableauHaut->getAbsoluteClippingRect()),
		true, true, guiElements.gameGUI.tableauHaut, -1, false);
	guiElements.gameGUI.populationTexte->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER); // Indique l'alignement du texte
	guiElements.gameGUI.populationTexte->setOverrideColor(video::SColor(255, 255, 255, 255)); // Change la couleur du texte

	// dateTexte
	guiElements.gameGUI.dateTexte = game->gui->addStaticText(L"1 janvier 2000",
		getAbsoluteRectangle(0.8f, 0.0f, 1.0f, 1.0f, guiElements.gameGUI.tableauHaut->getAbsoluteClippingRect()),
		true, true, guiElements.gameGUI.tableauHaut, -1, false);
	guiElements.gameGUI.dateTexte->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER); // Indique l'alignement du texte
	guiElements.gameGUI.dateTexte->setOverrideColor(video::SColor(255, 255, 255, 255)); // Change la couleur du texte

	// budgetInfosTexte
	guiElements.gameGUI.budgetInfosTexte = game->gui->addStaticText(L"Evolution (M) : +0.00 €\r\nEvolution (A) : +0.00 €",
		getAbsoluteRectangle(0.0f, 0.03f, 0.2f, 0.09f, parentRect),
		true, true, parent, -1, true);
	guiElements.gameGUI.budgetInfosTexte->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER); // Indique l'alignement du texte
	guiElements.gameGUI.budgetInfosTexte->setOverrideColor(video::SColor(255, 255, 255, 255)); // Change la couleur du texte
	guiElements.gameGUI.budgetInfosTexte->setVisible(false);

	// energieInfosTexte
	guiElements.gameGUI.energieInfosTexte = game->gui->addStaticText(L"Energie disponible : 100 %",
		getAbsoluteRectangle(0.2f, 0.03f, 0.35f, 0.06f, parentRect),
		true, true, parent, -1, true);
	guiElements.gameGUI.energieInfosTexte->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER); // Indique l'alignement du texte
	guiElements.gameGUI.energieInfosTexte->setOverrideColor(video::SColor(255, 255, 255, 255)); // Change la couleur du texte
	guiElements.gameGUI.energieInfosTexte->setVisible(false);

	// effetSerreInfosTexte
	guiElements.gameGUI.effetSerreInfosTexte = game->gui->addStaticText(L"Evolution (M) : +0 kg\r\nTaxe (M) : -0.00 €",
		getAbsoluteRectangle(0.35f, 0.03f, 0.5f, 0.09f, parentRect),
		true, true, parent, -1, true);
	guiElements.gameGUI.effetSerreInfosTexte->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER); // Indique l'alignement du texte
	guiElements.gameGUI.effetSerreInfosTexte->setOverrideColor(video::SColor(255, 255, 255, 255)); // Change la couleur du texte
	guiElements.gameGUI.effetSerreInfosTexte->setVisible(false);

	// dechetsInfosTexte
	guiElements.gameGUI.dechetsInfosTexte = game->gui->addStaticText(L"Evolution (M) : +0 kg\r\nTaxe (M) : -0.00 €",
		getAbsoluteRectangle(0.5f, 0.03f, 0.65f, 0.09f, parentRect),
		true, true, parent, -1, true);
	guiElements.gameGUI.dechetsInfosTexte->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER); // Indique l'alignement du texte
	guiElements.gameGUI.dechetsInfosTexte->setOverrideColor(video::SColor(255, 255, 255, 255)); // Change la couleur du texte
	guiElements.gameGUI.dechetsInfosTexte->setVisible(false);

	// populationInfosTexte
	guiElements.gameGUI.populationInfosTexte = game->gui->addStaticText(L"Satisfaction : 100 %\r\nSatisfaction réelle : 100 %",
		getAbsoluteRectangle(0.65f, 0.03f, 0.8f, 0.09f, parentRect),
		true, true, parent, -1, true);
	guiElements.gameGUI.populationInfosTexte->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER); // Indique l'alignement du texte
	guiElements.gameGUI.populationInfosTexte->setOverrideColor(video::SColor(255, 255, 255, 255)); // Change la couleur du texte
	guiElements.gameGUI.populationInfosTexte->setVisible(false);

	// dateInfosTexte
	guiElements.gameGUI.dateInfosTexte = game->gui->addStaticText(L"0 années 0 mois 0 jours\r\nVitesse du jeu : 1.0\r\n1 jour <=> 2.0 sec",
		getAbsoluteRectangle(0.8f, 0.03f, 0.95f, 0.12f, parentRect),
		true, true, parent, -1, true);
	guiElements.gameGUI.dateInfosTexte->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER); // Indique l'alignement du texte
	guiElements.gameGUI.dateInfosTexte->setOverrideColor(video::SColor(255, 255, 255, 255)); // Change la couleur du texte
	guiElements.gameGUI.dateInfosTexte->setVisible(false);

	// Crée le menu de droite
	guiElements.gameGUI.menuDroite = game->gui->addImage(getAbsoluteRectangle(0.95f, 0.03f, 1.0f, 0.8f, parentRect), parent);
	const core::recti menuDroiteClippingRect = guiElements.gameGUI.menuDroite->getAbsoluteClippingRect();

	// Crée les textes complètant la scroll bar de la vitesse du jeu :

	// Texte "+ Rapide"
	guiElements.gameGUI.texteVitesseJeuRapide = game->gui->addStaticText(L"+\r\nRapide", getAbsoluteRectangle(0.05f, 0.02f, 0.95f, 0.08f, menuDroiteClippingRect),
		false, true, guiElements.gameGUI.menuDroite, -1, false);
	guiElements.gameGUI.texteVitesseJeuRapide->setTextAlignment(EGUIA_CENTER, EGUIA_LOWERRIGHT);		// Indique l'alignement du texte
	guiElements.gameGUI.texteVitesseJeuRapide->setOverrideColor(video::SColor(255, 255, 255, 255));	// Change la couleur du texte

	// Texte "- Rapide"
	guiElements.gameGUI.texteVitesseJeuLent = game->gui->addStaticText(L"-\r\nRapide", getAbsoluteRectangle(0.05f, 0.6f, 0.95f, 0.66f, menuDroiteClippingRect),
		false, true, guiElements.gameGUI.menuDroite, -1, false);
	guiElements.gameGUI.texteVitesseJeuLent->setTextAlignment(EGUIA_CENTER, EGUIA_LOWERRIGHT);		// Indique l'alignement du texte
	guiElements.gameGUI.texteVitesseJeuLent->setOverrideColor(video::SColor(255, 255, 255, 255));	// Change la couleur du texte

	// Crée la scroll bar de la vitesse du jeu du menu de droite
	guiElements.gameGUI.vitesseJeuScrollBar = game->gui->addScrollBar(false, getAbsoluteRectangle(0.25f, 0.09f, 0.75f, 0.59f, menuDroiteClippingRect),
		guiElements.gameGUI.menuDroite, -1);
	guiElements.gameGUI.vitesseJeuScrollBar->setToolTipText(L"Règle la vitesse du jeu");
	guiElements.gameGUI.vitesseJeuScrollBar->setMin(0);			// Position min (voir GUIManager::getGameGUITimerSpeed())
	guiElements.gameGUI.vitesseJeuScrollBar->setMax(6);			// Position max (voir GUIManager::getGameGUITimerSpeed())
	guiElements.gameGUI.vitesseJeuScrollBar->setSmallStep(1);	// Pas minimal
	guiElements.gameGUI.vitesseJeuScrollBar->setLargeStep(10);	// Pas maximal

	// Indique la valeur actuelle de la vitesse du jeu
	setGameGUITimerSpeed(game->deviceTimer->getSpeed());

	// Crée les boutons du menu de droite
	guiElements.gameGUI.objectivesBouton = game->gui->addButton(getAbsoluteRectangle(0.1f, 0.7f, 0.9f, 0.74f, menuDroiteClippingRect),
		guiElements.gameGUI.menuDroite, -1, L"Obj", L"Afficher les objectifs définis pour cette partie");
	guiElements.gameGUI.monthTableBouton = game->gui->addButton(getAbsoluteRectangle(0.1f, 0.76f, 0.9f, 0.8f, menuDroiteClippingRect),
		guiElements.gameGUI.menuDroite, -1, L"Récap", L"Afficher le tableau récapitulatif de ce mois");
	guiElements.gameGUI.ressourcesBouton = game->gui->addButton(getAbsoluteRectangle(0.1f, 0.82f, 0.9f, 0.86f, menuDroiteClippingRect),
		guiElements.gameGUI.menuDroite, -1, L"Ress", L"Afficher le menu Ressources");
	guiElements.gameGUI.FPSCameraBouton = game->gui->addButton(getAbsoluteRectangle(0.1f, 0.88f, 0.9f, 0.92f, menuDroiteClippingRect),
		guiElements.gameGUI.menuDroite, -1, L"Visit", L"Passer en mode Visiteur");
	guiElements.gameGUI.menuBouton = game->gui->addButton(getAbsoluteRectangle(0.1f, 0.94f, 0.9f, 0.98f, menuDroiteClippingRect),
		guiElements.gameGUI.menuDroite, -1, L"Menu", L"Afficher le menu du jeu");

	// Crée le groupe d'onglets du bas et ses onglets
	guiElements.gameGUI.tabGroupBas = game->gui->addTabControl(getAbsoluteRectangle(0.0f, 0.8f, 1.0f, 1.0f, parentRect), parent, true, true);

	// Règle la taille des onglets (ne fonctionne pas encore parfaitement)
	// TODO : Revoir ces valeurs pour que cela fonctionne quelle que soit la résolution choisie
	guiElements.gameGUI.tabGroupBas->setTabExtraWidth((int)(0.053f * (float)parentRect.getWidth()));	// Valeur par défaut : 20
	guiElements.gameGUI.tabGroupBas->setTabHeight((int)(0.03f * (float)parentRect.getHeight()));	// Valeur par défaut : 32

	// Crée les onglets du bas
	guiElements.gameGUI.habitationsTab = guiElements.gameGUI.tabGroupBas->addTab(L"Habitations (1/2)");
	guiElements.gameGUI.habitations2Tab = guiElements.gameGUI.tabGroupBas->addTab(L"Habitations (2/2)");
	guiElements.gameGUI.usinesTab = guiElements.gameGUI.tabGroupBas->addTab(L"Usines");
	guiElements.gameGUI.energieTab = guiElements.gameGUI.tabGroupBas->addTab(L"Production d'énergie");
	guiElements.gameGUI.dechetsTab = guiElements.gameGUI.tabGroupBas->addTab(L"Gestion des déchets");
	guiElements.gameGUI.arbresTab = guiElements.gameGUI.tabGroupBas->addTab(L"Arbres");

#ifdef MENU_INFERIEUR_HAUT
#ifndef MENU_INFERIEUR_REACTIF
	guiElements.gameGUI.reduireTab = guiElements.gameGUI.tabGroupBas->addTab(L"-");
#endif
#else
#ifndef MENU_INFERIEUR_REACTIF
	guiElements.gameGUI.reduireTab = guiElements.gameGUI.tabGroupBas->addTab(L"+");
#endif

	// Descends le menu inférieur par défaut
	const int minPosY = guiElements.gameGUI.tabGroupBas->getParent()->getRelativePosition().LowerRightCorner.Y - guiElements.gameGUI.tabGroupBas->getTabHeight();
	guiElements.gameGUI.tabGroupBas->setRelativePosition(
		core::vector2di(guiElements.gameGUI.tabGroupBas->getRelativePosition().UpperLeftCorner.X, minPosY));
#endif

	// Crée le bouton pour détruire les batiments
	guiElements.gameGUI.detruireBouton = game->gui->addButton(getAbsoluteRectangle(0.9f, 0.0f, 1.0f, 0.15f, guiElements.gameGUI.tabGroupBas->getAbsolutePosition()),
		guiElements.gameGUI.tabGroupBas, -1, L"Détruire", L"Détruire des bâtiments");
	guiElements.gameGUI.detruireBouton->setIsPushButton(true);



	// Crée les boutons des onglets du bas :
	const float minX = 0.01f, minY = 0.075f,
		tailleBoutonX = 0.23f, tailleBoutonY = 0.25f,
		ecartBoutonX = 0.02f, ecartBoutonY = 0.05f;

	guiElements.gameGUI.listeBoutonsBatiments.clear();



	// Habitations (1/1) :

	// Maisons
	addBoutonBatiment(
		minX, minY, minX + tailleBoutonX, minY + tailleBoutonY,
		BI_maison_individuelle,
		guiElements.gameGUI.habitationsTab);
	addBoutonBatiment(
		minX + tailleBoutonX + ecartBoutonX, minY, minX + tailleBoutonX * 2 + ecartBoutonX, minY + tailleBoutonY,
		BI_maison_basse_consommation,
		guiElements.gameGUI.habitationsTab);
	addBoutonBatiment(
		minX + tailleBoutonX * 2 + ecartBoutonX * 2, minY, minX + tailleBoutonX * 3 + ecartBoutonX * 2, minY + tailleBoutonY,
		BI_maison_avec_panneaux_solaires,
		guiElements.gameGUI.habitationsTab);
	addBoutonBatiment(
		minX + tailleBoutonX * 3 + ecartBoutonX * 3, minY, minX + tailleBoutonX * 4 + ecartBoutonX * 3, minY + tailleBoutonY,
		BI_grande_maison_individuelle,
		guiElements.gameGUI.habitationsTab);
	// Immeubles
	addBoutonBatiment(
		minX, minY + tailleBoutonY + ecartBoutonY, minX + tailleBoutonX, minY + tailleBoutonY * 2 + ecartBoutonY,
		BI_immeuble_individuel,
		guiElements.gameGUI.habitationsTab);
	addBoutonBatiment(
		minX + tailleBoutonX + ecartBoutonX, minY + tailleBoutonY + ecartBoutonY, minX + tailleBoutonX * 2 + ecartBoutonX, minY + tailleBoutonY * 2 + ecartBoutonY,
		BI_immeuble_basse_consommation,
		guiElements.gameGUI.habitationsTab);
	addBoutonBatiment(
		minX + tailleBoutonX * 2 + ecartBoutonX * 2, minY + tailleBoutonY + ecartBoutonY, minX + tailleBoutonX * 3 + ecartBoutonX * 2, minY + tailleBoutonY * 2 + ecartBoutonY,
		BI_immeuble_avec_panneaux_solaires,
		guiElements.gameGUI.habitationsTab);
	addBoutonBatiment(
		minX + tailleBoutonX * 3 + ecartBoutonX * 3, minY + tailleBoutonY + ecartBoutonY, minX + tailleBoutonX * 4 + ecartBoutonX * 3, minY + tailleBoutonY * 2 + ecartBoutonY,
		BI_grand_immeuble_individuel,
		guiElements.gameGUI.habitationsTab);
	// Buildings
	addBoutonBatiment(
		minX, minY + tailleBoutonY * 2 + ecartBoutonY * 2, minX + tailleBoutonX, minY + tailleBoutonY * 3 + ecartBoutonY * 2,
		BI_building_individuel,
		guiElements.gameGUI.habitationsTab);
	addBoutonBatiment(
		minX + tailleBoutonX + ecartBoutonX, minY + tailleBoutonY * 2 + ecartBoutonY * 2, minX + tailleBoutonX * 2 + ecartBoutonX, minY + tailleBoutonY * 3 + ecartBoutonY * 2,
		BI_building_basse_consommation,
		guiElements.gameGUI.habitationsTab);
	addBoutonBatiment(
		minX + tailleBoutonX * 2 + ecartBoutonX * 2, minY + tailleBoutonY * 2 + ecartBoutonY * 2, minX + tailleBoutonX * 3 + ecartBoutonX * 2, minY + tailleBoutonY * 3 + ecartBoutonY * 2,
		BI_building_avec_panneaux_solaires,
		guiElements.gameGUI.habitationsTab);
	addBoutonBatiment(
		minX + tailleBoutonX * 3 + ecartBoutonX * 3, minY + tailleBoutonY * 2 + ecartBoutonY * 2, minX + tailleBoutonX * 4 + ecartBoutonX * 3, minY + tailleBoutonY * 3 + ecartBoutonY * 2,
		BI_grand_building_individuel,
		guiElements.gameGUI.habitationsTab);

	// Habitations (2/2) :
#ifndef KID_VERSION	// En mode normal : cet onglet comprend le chalet, la maison de base, les routes et l'usine à tout faire
	addBoutonBatiment(
		0.2f, 0.1f, 0.45f, 0.45f,
		BI_chalet,
		guiElements.gameGUI.habitations2Tab);
	addBoutonBatiment(
		0.55f, 0.1f, 0.8f, 0.45f,
		BI_maison,
		guiElements.gameGUI.habitations2Tab);
	addBoutonBatiment(
		0.2f, 0.55f, 0.45f, 0.9f,
		BI_usine_tout,
		guiElements.gameGUI.habitations2Tab);
	addBoutonBatiment(
		0.55f, 0.55f, 0.8f, 0.9f,
		BI_route,
		guiElements.gameGUI.habitations2Tab);
#else				// En mode enfant : seul le chalet fait partie de cet onglet
	addBoutonBatiment(
		0.3f, 0.2f, 0.7f, 0.8f,
		BI_chalet,
		guiElements.gameGUI.habitations2Tab);
#endif

	// Usines + Production d'eau :
	addBoutonBatiment(
		0.375f, minY, 0.625f, minY + tailleBoutonY,
		BI_pompe_extraction_eau,
		guiElements.gameGUI.usinesTab);
#ifndef KID_VERSION	// En mode normal : on désactive l'usine de papier qui est inutile : les usines sont centrées sur 3 colonnes
	addBoutonBatiment(
		0.05f, minY + tailleBoutonY + ecartBoutonY, 0.3f, minY + tailleBoutonY * 2 + ecartBoutonY,
		BI_usine_verre_petite,
		guiElements.gameGUI.usinesTab);
	addBoutonBatiment(
		0.05f, minY + tailleBoutonY * 2 + ecartBoutonY * 2, 0.3f, minY + tailleBoutonY * 3 + ecartBoutonY * 2,
		BI_usine_verre_grande,
		guiElements.gameGUI.usinesTab);
	addBoutonBatiment(
		0.375f, minY + tailleBoutonY + ecartBoutonY, 0.625f, minY + tailleBoutonY * 2 + ecartBoutonY,
		BI_usine_ciment_petite,
		guiElements.gameGUI.usinesTab);
	addBoutonBatiment(
		0.375f, minY + tailleBoutonY * 2 + ecartBoutonY * 2, 0.625f, minY + tailleBoutonY * 3 + ecartBoutonY * 2,
		BI_usine_ciment_grande,
		guiElements.gameGUI.usinesTab);
	addBoutonBatiment(
		0.7f, minY + tailleBoutonY + ecartBoutonY, 0.95f, minY + tailleBoutonY * 2 + ecartBoutonY,
		BI_usine_tuiles_petite,
		guiElements.gameGUI.usinesTab);
	addBoutonBatiment(
		0.7f, minY + tailleBoutonY * 2 + ecartBoutonY * 2, 0.95f, minY + tailleBoutonY * 3 + ecartBoutonY * 2,
		BI_usine_tuiles_grande,
		guiElements.gameGUI.usinesTab);
#else				// En mode enfant : on réactiver le papier comme ressource consommée par les habitants pour leur satisfaction : les usines sont centrées sur 4 colonnes
	addBoutonBatiment(
		minX, minY + tailleBoutonY + ecartBoutonY, minX + tailleBoutonX, minY + tailleBoutonY * 2 + ecartBoutonY,
		BI_usine_verre_petite,
		guiElements.gameGUI.usinesTab);
	addBoutonBatiment(
		minX, minY + tailleBoutonY * 2 + ecartBoutonY * 2, minX + tailleBoutonX, minY + tailleBoutonY * 3 + ecartBoutonY * 2,
		BI_usine_verre_grande,
		guiElements.gameGUI.usinesTab);
	addBoutonBatiment(
		minX + tailleBoutonX + ecartBoutonX, minY + tailleBoutonY + ecartBoutonY, minX + tailleBoutonX * 2 + ecartBoutonX, minY + tailleBoutonY * 2 + ecartBoutonY,
		BI_usine_ciment_petite,
		guiElements.gameGUI.usinesTab);
	addBoutonBatiment(
		minX + tailleBoutonX + ecartBoutonX, minY + tailleBoutonY * 2 + ecartBoutonY * 2, minX + tailleBoutonX * 2 + ecartBoutonX, minY + tailleBoutonY * 3 + ecartBoutonY * 2,
		BI_usine_ciment_grande,
		guiElements.gameGUI.usinesTab);
	addBoutonBatiment(
		minX + tailleBoutonX * 2 + ecartBoutonX * 2, minY + tailleBoutonY + ecartBoutonY, minX + tailleBoutonX * 3 + ecartBoutonX * 2, minY + tailleBoutonY * 2 + ecartBoutonY,
		BI_usine_tuiles_petite,
		guiElements.gameGUI.usinesTab);
	addBoutonBatiment(
		minX + tailleBoutonX * 2 + ecartBoutonX * 2, minY + tailleBoutonY * 2 + ecartBoutonY * 2, minX + tailleBoutonX * 3 + ecartBoutonX * 2, minY + tailleBoutonY * 3 + ecartBoutonY * 2,
		BI_usine_tuiles_grande,
		guiElements.gameGUI.usinesTab);
	addBoutonBatiment(
		minX + tailleBoutonX * 3 + ecartBoutonX * 3, minY + tailleBoutonY + ecartBoutonY, minX + tailleBoutonX * 4 + ecartBoutonX * 3, minY + tailleBoutonY * 2 + ecartBoutonY,
		BI_usine_papier_petite,
		guiElements.gameGUI.usinesTab);
	addBoutonBatiment(
		minX + tailleBoutonX * 3 + ecartBoutonX * 3, minY + tailleBoutonY * 2 + ecartBoutonY * 2, minX + tailleBoutonX * 4 + ecartBoutonX * 3, minY + tailleBoutonY * 3 + ecartBoutonY * 2,
		BI_usine_papier_grande,
		guiElements.gameGUI.usinesTab);
#endif

	// Production d'énergie :
	addBoutonBatiment(
		0.2f, 0.1f, 0.45f, 0.45f,
		BI_panneau_solaire,
		guiElements.gameGUI.energieTab);
	addBoutonBatiment(
		0.55f, 0.1f, 0.8f, 0.45f,
		BI_centrale_charbon,
		guiElements.gameGUI.energieTab);
	addBoutonBatiment(
		0.2f, 0.55f, 0.45f, 0.9f,
		BI_eolienne,
		guiElements.gameGUI.energieTab);
	addBoutonBatiment(
		0.55f, 0.55f, 0.8f, 0.9f,
		BI_hydrolienne,
		guiElements.gameGUI.energieTab);

	// Gestion des déchets :
	addBoutonBatiment(
		0.2f, 0.2f, 0.45f, 0.8f,
		BI_decharge,
		guiElements.gameGUI.dechetsTab);
	addBoutonBatiment(
		0.55f, 0.2f, 0.8f, 0.8f,
		BI_usine_incineration_dechets,
		guiElements.gameGUI.dechetsTab);

	// Arbres (gestion de l'effet de serre) :
	addBoutonBatiment(
		0.2f, 0.1f, 0.45f, 0.45f,
		BI_arbre_aspen,
		guiElements.gameGUI.arbresTab);
	addBoutonBatiment(
		0.55f, 0.1f, 0.8f, 0.45f,
		BI_arbre_oak,
		guiElements.gameGUI.arbresTab);
	addBoutonBatiment(
		0.2f, 0.55f, 0.45f, 0.9f,
		BI_arbre_pine,
		guiElements.gameGUI.arbresTab);
	addBoutonBatiment(
		0.55f, 0.55f, 0.8f, 0.9f,
		BI_arbre_willow,
		guiElements.gameGUI.arbresTab);



	// Crée les fenêtres de la GUI du jeu :

	// Obtient le rectangle par défaut pour la taille d'une fenêtre :
	// ce rectangle sera assigné aux fenêtres normalement invisibles, qui devront prendre par défaut la majeure partie de l'écran lorsqu'elles seront visibles
	const core::recti defaultWindowRect = getAbsoluteRectangle(0.01f, 0.07f, 0.94f, 0.79f, parentRect);

	// Crée la fenêtre de la mini carte
	if (!guiElements.gameGUI.miniMapWindow)
	{
		// Calcule la taille d'un bord de la mini carte (elle doit être carrée, on prend donc la taille minimale entre celle des deux bords)
		const core::dimension2di parentSize = parentRect.getSize();
		const int size = min((int)(0.285f * parentSize.Width) - 25,	// (int)(0.285f * parentSize.Width) - (10 + 15)
			(int)(0.3f * parentSize.Height) - 45);					// (int)((0.8f - 0.5f) * parentSize.Height) - (20 + 10 + 15)

		// Calcule le rectangle qu'occupe la mini carte
		// (on y ajoute aussi la taille de la barre de titre en Y : 20 px, et ses écarts de 5 px en haut et à gauche, et de 15 px en bas et à droite)
		const int miniMapY = (int)(0.8f * parentSize.Height);
		const core::recti miniMapRect(0, miniMapY - size - 40, size + 20, miniMapY);

		guiElements.gameGUI.miniMapWindow = new CGUIMiniMapWindow(game->sceneManager, game->renderer, game->gui, parent, -1, miniMapRect);
		guiElements.gameGUI.miniMapWindow->setMinimized(true);
		guiElements.gameGUI.miniMapWindow->drop();
	}

	// Crée la fenêtre d'informations
	if (!guiElements.gameGUI.informationsWindow)
	{
		guiElements.gameGUI.informationsWindow = new CGUIInformationsWindow(game->system, game->gui, parent, -1,
			getAbsoluteRectangle(0.0f, 0.03f, 0.285f, 0.5f, parentRect));
		guiElements.gameGUI.informationsWindow->setVisible(false);
		guiElements.gameGUI.informationsWindow->drop();
	}

	// Crée la fenêtre de notes
	if (!guiElements.gameGUI.notesWindow)
	{
		guiElements.gameGUI.notesWindow = new CGUINotesWindow(game->gui, parent, -1,
			getAbsoluteRectangle(0.65f, 0.03f, 0.95f, 0.4f, parentRect));
		guiElements.gameGUI.notesWindow->setMinimized(true);
		guiElements.gameGUI.notesWindow->drop();
	}

	// Crée la fenêtre du menu des ressources
	if (!guiElements.gameGUI.ressourcesWindow)
	{
		guiElements.gameGUI.ressourcesWindow = new CGUIRessourcesWindow(game->system, game->gui, parent, -1,
			defaultWindowRect);
		guiElements.gameGUI.ressourcesWindow->setVisible(false);
		guiElements.gameGUI.ressourcesWindow->drop();
	}

	// Crée la fenêtre du tableau récapitulatif des fins de mois
	if (!guiElements.gameGUI.monthTableWindow)
	{
		guiElements.gameGUI.monthTableWindow = new CGUIMonthTableWindow(game->system, game->gui, parent, -1,
			defaultWindowRect);
		guiElements.gameGUI.monthTableWindow->setVisible(false);
		guiElements.gameGUI.monthTableWindow->drop();
	}

	// Crée la fenêtre des objectifs de la partie actuelle
	if (!guiElements.gameGUI.objectivesWindow)
	{
		guiElements.gameGUI.objectivesWindow = new CGUIObjectivesWindow(game->system, game->gui, parent, -1,
			defaultWindowRect);
		guiElements.gameGUI.objectivesWindow->setVisible(false);
		guiElements.gameGUI.objectivesWindow->drop();
	}

	// Crée la fenêtre permettant à l'utilisateur d'affecter une animation personnalisée à la caméra RTS du jeu
	if (!guiElements.gameGUI.cameraAnimatorWindow)
	{
		guiElements.gameGUI.cameraAnimatorWindow = new CGUICameraAnimatorWindow(game->gui, parent, -1,
			defaultWindowRect);
		guiElements.gameGUI.cameraAnimatorWindow->setVisible(false);
		guiElements.gameGUI.cameraAnimatorWindow->drop();
	}

	// Crée la fenêtre du menu du jeu (qui s'affiche quand on appuie sur Echap)
	if (!guiElements.gameGUI.menuWindow)
	{
		guiElements.gameGUI.menuWindow = new CGUIMenuWindow(game->gui, parent, -1,
			getAbsoluteRectangle(0.3f, 0.2f, 0.7f, 0.8f, parentRect), game->deviceTimer);	// 0.34f, 0.31f, 0.66f, 0.69f	// 0.38f, 0.38f, 0.62f, 0.62f
		guiElements.gameGUI.menuWindow->setVisible(false);
		guiElements.gameGUI.menuWindow->drop();
	}
}
void GUIManager::addBoutonBatiment(float x, float y, float x2, float y2, BatimentID batimentID, IGUIElement* parent)
{
	if (!game->gui)
		return;

	if (!parent)
		parent = game->gui->getRootGUIElement();

	// TODO : Ajouter un tooltip text aux boutons de construction
	IGUIButton* boutonBatiment = game->gui->addButton(
		getAbsoluteRectangle(x, y, x2, y2, parent->getRelativePosition()), parent,
		batimentID, StaticBatimentInfos::getInfos(batimentID).name, 0);
	boutonBatiment->setIsPushButton(true);
	guiElements.gameGUI.listeBoutonsBatiments.push_back(boutonBatiment);
}
void GUIManager::resetGameGUI()
{
	if (!GUIs[EGN_gameGUI])
		return;

	const core::recti parentRect = GUIs[EGN_gameGUI]->getRelativePosition();

	// Replace le groupe d'onglets du bas
	if (guiElements.gameGUI.tabGroupBas)
	{
#ifdef MENU_INFERIEUR_HAUT
		// Monte le menu inférieur par défaut
		const int maxPosY = core::floor32(0.8f * (float)game->driver->getScreenSize().Height);
		guiElements.gameGUI.tabGroupBas->setRelativePosition(
			core::vector2di(guiElements.gameGUI.tabGroupBas->getRelativePosition().UpperLeftCorner.X, maxPosY));

#ifndef MENU_INFERIEUR_REACTIF
		if (guiElements.gameGUI.reduireTab)
			guiElements.gameGUI.reduireTab->setText(L"-");
#endif
#else
		// Descends le menu inférieur par défaut
		const int minPosY = guiElements.gameGUI.tabGroupBas->getParent()->getRelativePosition().LowerRightCorner.Y - guiElements.gameGUI.tabGroupBas->getTabHeight();
		guiElements.gameGUI.tabGroupBas->setRelativePosition(
			core::vector2di(guiElements.gameGUI.tabGroupBas->getRelativePosition().UpperLeftCorner.X, minPosY));

#ifndef MENU_INFERIEUR_REACTIF
		if (guiElements.gameGUI.reduireTab)
			guiElements.gameGUI.reduireTab->setText(L"+");
#endif
#endif

		// Active le premier onglet
		guiElements.gameGUI.tabGroupBas->setActiveTab(0);
	}



	// Replace les fenêtres de la GUI du jeu :

	// Obtient le rectangle par défaut pour la taille d'une fenêtre :
	// ce rectangle sera assigné aux fenêtres normalement invisibles, qui devront prendre par défaut la majeure partie de l'écran lorsqu'elles seront visibles
	const core::recti defaultWindowRect = getAbsoluteRectangle(0.01f, 0.07f, 0.94f, 0.79f, parentRect);

	// Replace la fenêtre de la mini carte
	if (guiElements.gameGUI.miniMapWindow)
	{
		// Calcule la taille d'un bord de la mini carte (elle doit être carrée, on prend donc la taille minimale entre celle des deux bords)
		const core::dimension2di parentSize = parentRect.getSize();
		const int size = min((int)(0.285f * parentSize.Width) - 25,	// (int)(0.285f * parentSize.Width) - (10 + 15)
			(int)(0.3f * parentSize.Height) - 45);					// (int)((0.8f - 0.5f) * parentSize.Height) - (20 + 10 + 15)

		// Calcule le rectangle qu'occupe la mini carte
		// (on y ajoute aussi la taille de la barre de titre en Y : 20 px, et ses écarts de 5 px en haut et à gauche, et de 15 px en bas et à droite)
		const int miniMapY = (int)(0.8f * parentSize.Height);
		const core::recti miniMapRect(0, miniMapY - size - 40, size + 20, miniMapY);

		guiElements.gameGUI.miniMapWindow->setMinimized(false);
		guiElements.gameGUI.miniMapWindow->setRelativePosition(miniMapRect);
		guiElements.gameGUI.miniMapWindow->setMinimized(true);
	}

	// Replace la fenêtre d'informations
	if (guiElements.gameGUI.informationsWindow)
	{
		guiElements.gameGUI.informationsWindow->setMinimized(false);
		guiElements.gameGUI.informationsWindow->setRelativePosition(
			getAbsoluteRectangle(0.0f, 0.03f, 0.285f, 0.5f, parentRect));
	}

	// Replace la fenêtre de notes
	if (guiElements.gameGUI.notesWindow)
	{
		guiElements.gameGUI.notesWindow->setMinimized(false);
		guiElements.gameGUI.notesWindow->setRelativePosition(
			getAbsoluteRectangle(0.65f, 0.03f, 0.95f, 0.4f, parentRect));
		guiElements.gameGUI.notesWindow->setMinimized(true);
	}

	// Replace la fenêtre du menu des ressources
	if (guiElements.gameGUI.ressourcesWindow)
	{
		guiElements.gameGUI.ressourcesWindow->setRelativePosition(
			defaultWindowRect);
		guiElements.gameGUI.ressourcesWindow->setVisible(false);
	}

	// Replace la fenêtre du tableau récapitulatif des fins de mois
	if (guiElements.gameGUI.monthTableWindow)
	{
		guiElements.gameGUI.monthTableWindow->setRelativePosition(
			defaultWindowRect);
		guiElements.gameGUI.monthTableWindow->setVisible(false);
	}

	// Replace la fenêtre des objectifs de la partie actuelle
	if (!guiElements.gameGUI.objectivesWindow)
	{
		guiElements.gameGUI.objectivesWindow->setRelativePosition(
			defaultWindowRect);
		guiElements.gameGUI.objectivesWindow->setVisible(false);
	}

	// Replace la fenêtre permettant à l'utilisateur d'affecter une animation personnalisée à la caméra RTS du jeu
	if (!guiElements.gameGUI.cameraAnimatorWindow)
	{
		guiElements.gameGUI.cameraAnimatorWindow->setRelativePosition(
			defaultWindowRect);
		guiElements.gameGUI.cameraAnimatorWindow->setVisible(false);
	}

	// Replace la fenêtre du menu du jeu (qui s'affiche quand on appuie sur Echap)
	if (guiElements.gameGUI.menuWindow)
	{
		guiElements.gameGUI.menuWindow->setRelativePosition(
			getAbsoluteRectangle(0.3f, 0.2f, 0.7f, 0.8f, parentRect));	// 0.34f, 0.31f, 0.66f, 0.69f	// 0.38f, 0.38f, 0.62f, 0.62f
		guiElements.gameGUI.menuWindow->setVisible(false);
	}
}
void GUIManager::save(io::IAttributes* out, io::IXMLWriter* writer, bool menuInferieurHaut) const
{
	if (!out || !writer)
		return;

	// Ajoute les informations sur la fenêtre de la mini carte
	if (guiElements.gameGUI.miniMapWindow)
	{
		out->addRect("MiniMapWindowRect", guiElements.gameGUI.miniMapWindow->getMaximizedRect());
		out->addBool("MiniMapWindowMinimized", guiElements.gameGUI.miniMapWindow->isMinimized());
	}

	// Ajoute les informations sur la fenêtre d'informations
	if (guiElements.gameGUI.informationsWindow)
	{
		out->addRect("InformationsWindowRect", guiElements.gameGUI.informationsWindow->getMaximizedRect());
		out->addBool("InformationsWindowMinimized", guiElements.gameGUI.informationsWindow->isMinimized());
	}

	// Ajoute les informations sur la fenêtre de notes
	if (guiElements.gameGUI.notesWindow)
	{
		out->addRect("NotesWindowRect", guiElements.gameGUI.notesWindow->getMaximizedRect());
		out->addBool("NotesWindowMinimized", guiElements.gameGUI.notesWindow->isMinimized());
		out->addString("NotesText", guiElements.gameGUI.notesWindow->getNotesTexte());
	}

	// Ajoute les informations sur la fenêtre du menu des ressources
	if (guiElements.gameGUI.ressourcesWindow)
	{
		out->addRect("RessourcesWindowRect", guiElements.gameGUI.ressourcesWindow->getRelativePosition());
		out->addBool("RessourcesWindowVisible", guiElements.gameGUI.ressourcesWindow->isVisible());
	}

	// Ajoute les informations sur le tableau récapitulatif des fins de mois
	if (guiElements.gameGUI.monthTableWindow)
	{
		out->addRect("MonthTableWindowRect",	 guiElements.gameGUI.monthTableWindow->getRelativePosition());
		out->addBool("MonthTableWindowVisible", guiElements.gameGUI.monthTableWindow->isVisible());
	}

	// Ajoute les informations sur le tableau affichant les objectifs de la partie actuelle
	if (guiElements.gameGUI.objectivesWindow)
	{
		out->addRect("ObjectivesWindowRect",	 guiElements.gameGUI.objectivesWindow->getRelativePosition());
		out->addBool("ObjectivesWindowVisible", guiElements.gameGUI.objectivesWindow->isVisible());
	}

	// Ajoute la position du menu inférieur
	out->addBool("MenuInferieurHaut", menuInferieurHaut);

	// Ajoute la visibilité de la gui du jeu
	out->addBool("GameGUIVisible", GUIs[EGN_gameGUI] ? GUIs[EGN_gameGUI]->isVisible() : true);

	// Ecrit les informations de la gui dans le fichier
	out->write(writer, false, L"GUI");
	out->clear();
}
void GUIManager::load(io::IAttributes* in, io::IXMLReader* reader)
{
	if (!in || !reader)
		return;

	// Réinitialise la gui du jeu avant son chargement
	resetGameGUI();

	// Lit les informations de la gui à partir du fichier
	reader->resetPosition();
	if (in->read(reader, false, L"GUI"))
	{
		// Lit les informations sur la fenêtre de la mini carte
		if (guiElements.gameGUI.miniMapWindow)
		{
			guiElements.gameGUI.miniMapWindow->setMinimized(false);
			if (in->existsAttribute("MiniMapWindowRect"))			guiElements.gameGUI.miniMapWindow->setRelativePosition(in->getAttributeAsRect("MiniMapWindowRect"));
			guiElements.gameGUI.miniMapWindow->setMinimized(true);	// Fenêtre minimisée par défaut
			if (in->existsAttribute("MiniMapWindowMinimized"))		guiElements.gameGUI.miniMapWindow->setMinimized(in->getAttributeAsBool("MiniMapWindowMinimized"));
		}

		// Lit les informations sur la fenêtre d'informations
		if (guiElements.gameGUI.informationsWindow)
		{
			guiElements.gameGUI.informationsWindow->setMinimized(false);
			if (in->existsAttribute("InformationsWindowRect"))		guiElements.gameGUI.informationsWindow->setRelativePosition(in->getAttributeAsRect("InformationsWindowRect"));
			if (in->existsAttribute("InformationsWindowMinimized"))	guiElements.gameGUI.informationsWindow->setMinimized(in->getAttributeAsBool("InformationsWindowMinimized"));
		}

		// Lit les informations sur la fenêtre de notes
		if (guiElements.gameGUI.notesWindow)
		{
			guiElements.gameGUI.notesWindow->setMinimized(false);
			if (in->existsAttribute("NotesWindowRect"))				guiElements.gameGUI.notesWindow->setRelativePosition(in->getAttributeAsRect("NotesWindowRect"));
			guiElements.gameGUI.miniMapWindow->setMinimized(true);	// Fenêtre minimisée par défaut
			if (in->existsAttribute("NotesWindowMinimized"))			guiElements.gameGUI.notesWindow->setMinimized(in->getAttributeAsBool("NotesWindowMinimized"));
			if (in->existsAttribute("NotesText"))					guiElements.gameGUI.notesWindow->setNotesTexte(in->getAttributeAsStringW("NotesText").c_str());
		}

		// Lit les informations sur la fenêtre du menu des ressources
		if (guiElements.gameGUI.ressourcesWindow)
		{
			if (in->existsAttribute("RessourcesWindowRect"))			guiElements.gameGUI.ressourcesWindow->setRelativePosition(in->getAttributeAsRect("RessourcesWindowRect"));
			if (in->existsAttribute("RessourcesWindowVisible"))		guiElements.gameGUI.ressourcesWindow->setVisible(in->getAttributeAsBool("RessourcesWindowVisible"));
		}

		// Lit les informations sur le tableau récapitulatif des fins de mois
		if (guiElements.gameGUI.monthTableWindow)
		{
			if (in->existsAttribute("MonthTableWindowRect"))			guiElements.gameGUI.monthTableWindow->setRelativePosition(in->getAttributeAsRect("MonthTableWindowRect"));
			if (in->existsAttribute("MonthTableWindowVisible"))		guiElements.gameGUI.monthTableWindow->setVisible(in->getAttributeAsBool("MonthTableWindowVisible"));
		}

		// Lit les informations sur le tableau affichant les objectifs de la partie actuelle
		if (guiElements.gameGUI.objectivesWindow)
		{
			if (in->existsAttribute("ObjectivesWindowRect"))			guiElements.gameGUI.objectivesWindow->setRelativePosition(in->getAttributeAsRect("ObjectivesWindowRect"));
			if (in->existsAttribute("ObjectivesWindowVisible"))		guiElements.gameGUI.objectivesWindow->setVisible(in->getAttributeAsBool("ObjectivesWindowVisible"));
		}

		// Lit la position du menu inférieur
		if (guiElements.gameGUI.tabGroupBas && in->existsAttribute("MenuInferieurHaut"))
		{
			const bool menuInferieurHaut = in->getAttributeAsBool("MenuInferieurHaut");
			if (menuInferieurHaut)
			{
				// Monte le menu inférieur
				const int maxPosY = core::floor32(0.8f * (float)game->driver->getScreenSize().Height);
				guiElements.gameGUI.tabGroupBas->setRelativePosition(
					core::vector2di(guiElements.gameGUI.tabGroupBas->getRelativePosition().UpperLeftCorner.X, maxPosY));

#ifndef MENU_INFERIEUR_REACTIF
				if (guiElements.gameGUI.reduireTab)
					guiElements.gameGUI.reduireTab->setText(L"-");
#endif
			}
			else
			{
				// Descends le menu inférieur
				const int minPosY = guiElements.gameGUI.tabGroupBas->getParent()->getRelativePosition().LowerRightCorner.Y - guiElements.gameGUI.tabGroupBas->getTabHeight();
				guiElements.gameGUI.tabGroupBas->setRelativePosition(
					core::vector2di(guiElements.gameGUI.tabGroupBas->getRelativePosition().UpperLeftCorner.X, minPosY));

#ifndef MENU_INFERIEUR_REACTIF
				if (guiElements.gameGUI.reduireTab)
					guiElements.gameGUI.reduireTab->setText(L"+");
#endif
			}

			// Active le premier onglet
			guiElements.gameGUI.tabGroupBas->setActiveTab(0);
		}

		// Lit la visibilité de la gui du jeu
		if (GUIs[EGN_gameGUI] && in->existsAttribute("GameGUIVisible"))
			GUIs[EGN_gameGUI]->setVisible(in->getAttributeAsBool("GameGUIVisible"));

		in->clear();
	}
}
