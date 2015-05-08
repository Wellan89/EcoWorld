#ifndef DEF_GUI_MANAGER
#define DEF_GUI_MANAGER



// Si défini, le menu inférieur de construction de la GUI du jeu montera et descendra suivant si la souris est sur lui ou non,
// sinon il ne montera et descendra que par les appuis sur son onglet "+" ou "-"
#define MENU_INFERIEUR_REACTIF

#ifdef MENU_INFERIEUR_REACTIF
// Le temps d'attente en ms du menu inférieur avant que celui-ci ne commence à descendre après que la souris l'ait quitté
#define MENU_INFERIEUR_WAIT_TIME	2000

#else

// Si défini, le menu inférieur de construction de la GUI du jeu sera placé en position haute (ouvert) par défaut,
// sinon il sera placé en position basse (fermé)
#define MENU_INFERIEUR_HAUT
#endif



#include "global.h"
#include "GameConfiguration.h"

enum BatimentID;
enum WeatherID;

class CGUIFileSelector;
class CGUIMenuWindow;
class CGUIRessourcesWindow;
class CGUIInformationsWindow;
class CGUINotesWindow;
class CGUIMiniMapWindow;
class CGUIMonthTableWindow;
class CGUIObjectivesWindow;
class CGUIObjectivesModifier;
class CGUISortTable;
class CGUICameraAnimatorWindow;
class CGUIOptionsMenu;
class CLoadingScreen;
class EcoWorldRenderer;
class EcoWorldSystem;

namespace irr
{
	namespace gui
	{
		class CGUITexturedSkin;
	}
}

using namespace gui;

#ifdef USE_RAKNET
// Structure permettant de stocker des informations sur une partie multijoueurs
struct MultiplayerGameListInfos
{
	core::stringc hostIP;			// L'adresse IP de l'hôte

	core::stringc terrainName;		// Le nom du terrain de l'hôte
	u8 currentParticipants;			// Le nombre actuel de participants
	u8 maxParticipants;				// Le nombre maximal de participants
	core::stringc hostGameVersion;	// La version d'EcoWorld de l'hôte

	MultiplayerGameListInfos() : currentParticipants(0), maxParticipants(0)	{ }
};
#endif

class GUIManager
{
public:
	enum E_GUI_NAME
	{
		EGN_mainMenuGUI,		// La gui du menu principal
		EGN_optionsMenuGUI,		// La gui du menu des options
		EGN_newGameMenuGUI,		// La gui du menu pour créer une nouvelle partie
#ifdef USE_RAKNET
		EGN_multiplayerMenuGUI,	// La gui du menu pour rejoindre une partie mutlijoueur
#endif
		EGN_gameEndGUI,			// La gui de l'écran qui s'affiche lorsque le jeu est terminé (perdu ou gagné)
		EGN_gameGUI,			// La gui du jeu

		EGN_COUNT
	};

	// Constructeur et destructeur
	GUIManager();
	~GUIManager();

	// Remet la GUI à zéro, et la réinitialise
	void resetGUI();

	// Recrée le skin de la GUI avec le fichier de paramêtres spécifié, et assigne sa transparence
	void createGUISkin(const io::path& guiSkinFile, int guiTransparency);

	// Crée toutes les GUIs
	void createGUIs();

	// Charge toutes les image des titres du menu principal pour une résolution d'écran donnée
	void loadMainMenuTitles(const core::dimension2du& screenSize);

	// Obtient l'image du titre du menu principal selon un temps donné
	video::ITexture* getMainMenuTitleFromWeather(WeatherID weather, const core::dimension2du& screenSize);

	// Change l'image du titre du menu principal suivant le temps actuel du menu principal
	void chooseMainMenuTitleFromWeather(WeatherID weather, const core::dimension2du& screenSize);

	// Met à jour la liste des terrains disponibles dans la GUI de création d'une nouvelle partie (à appeler à chaque affichage de cette GUI)
	// Si gameEventReceiver est spécifié, un event sera lancé à la fin de cette fonction pour avertir du changement de terrain sélectionné dans la liste des terrains
	void updateListTerrainsNewGameGUI(IEventReceiver* gameEventReceiver = NULL);

#ifdef USE_RAKNET
	// Met à jour la liste des parties multijoueurs disponibles dans la GUI pour rejoindre une partie multijoueur (à appeler à chaque affichage de cette GUI)
	void updateMultiplayerGamesTableNewGameGUI(const core::list<MultiplayerGameListInfos>& gamesListInfos);
#endif

	// Modifie la transparence de la GUI
	void setGUITransparency(int transparency);

	// Réinitialise la GUI du jeu (c'est-à-dire EGN_gameGUI seulement !) : Remet les fenêtres à leur place et à leur taille par défaut
	void resetGameGUI();

	// Enregistre les données de la GUI dans un fichier
	void save(io::IAttributes* out, io::IXMLWriter* writer, bool menuInferieurHaut) const;

	// Charge les données de la GUI depuis un fichier
	void load(io::IAttributes* in, io::IXMLReader* reader);

protected:
	CGUITexturedSkin* texturedSkin;	// Le skin personnalisé de la GUI

	void initGUI(); // Initialise les paramêtres de base de la GUI

	void createGlobalGUI(IGUIElement* parent);				// Crée les éléments de base de la GUI (la GUI globale)
	void createMainMenuGUI(IGUIElement* parent);			// Crée le menu principal
	void createNewGameMenuGUI(IGUIElement* parent);			// Crée le menu pour commencer une nouvelle partie
#ifdef USE_RAKNET
	void createMultiplayerMenuGUI(IGUIElement* parent);		// Crée le menu pour rejoindre une partie mutlijoueur
#endif
	void createGameEndGUI(IGUIElement* parent);				// Crée l'écran qui s'affiche lorsque le jeu est terminé (perdu ou gagné)
	void createGameGUI(IGUIElement* parent);

	// Fonction pour ajouter un bouton qui servira à construire un batiment
	void addBoutonBatiment(float x, float y, float x2, float y2, BatimentID batimentID, IGUIElement* parent);

	// Calcule la taille du titre du menu principal d'après la résolution actuelle
	// Si needScaling est renseigné, il sera passé à true si un redimensionnement de l'image est nécessaire (seulement si l'image la plus petite est plus grande que la résolution actuelle)
	static core::dimension2du calculateMainTitleSize(const core::dimension2du& screenSize, bool* needScaling = NULL)
	{
		if (needScaling)
			(*needScaling) = false;

		// Tailles possibles (pack standard) :
		//	2048 x 512
		//	1536 x 384
		//	1024 x 256
		//	 512 x 128
		//	 256 x  64
#ifdef CAN_USE_ROMAIN_DARCEL_RESSOURCES
		const core::dimension2du sizes_standard[5] = {
#else
		const core::dimension2du sizes[5] = {
#endif
			core::dimension2du( 256,  64),
			core::dimension2du( 512, 128),
			core::dimension2du(1024, 256),
			core::dimension2du(1536, 384),
			core::dimension2du(2048, 512) };

#ifdef CAN_USE_ROMAIN_DARCEL_RESSOURCES
		// Tailles possibles maximales (pack next-gen) :
		//	2048 x 720
		//	1536 x 540
		//	1024 x 360
		//	 512 x 180
		//	 256 x  90
		const core::dimension2du sizes_nextGen[5] = {
			core::dimension2du( 256,  90),
			core::dimension2du( 512, 180),
			core::dimension2du(1024, 360),
			core::dimension2du(1536, 540),
			core::dimension2du(2048, 720) };

		const core::dimension2du* const& sizes = (gameConfig.mainIconsSet == GameConfiguration::EMIS_NEXT_GEN ? sizes_nextGen : sizes_standard);
#endif

		// Si la largeur est inférieure à la largeur la plus petite ou la hauteur est inférieure à la hauteur la plus petite, on renvoie la plus petite taille possible
		if (screenSize.Width < sizes[0].Width || screenSize.Height < sizes[0].Height)
		{
			if (needScaling)
				(*needScaling) = true;
			return sizes[0];
		}

		// Trouve la première taille supérieure à la taille actuelle de l'écran et renvoie la précédente (on a ainsi la taille la plus grande possible)
		for (int i = 0; i < 5; ++i)
			if (sizes[i].Width > screenSize.Width || sizes[i].Height > screenSize.Height)
				return sizes[core::clamp(i - 1, 0, 4)];	// Retire 1 et replace tout de même i dans les limites pour plus de sûreté (pour i = 0 : sizes[i - 1] = sizes[-1] !)

		// Si on arrive ici, c'est que la taille de l'écran est supérieure à la taille maximale du titre, on renvoie donc la taille la plus grande possible
		return sizes[4];
	}

	u32 skinDefaultAplha[EGDC_COUNT];	// La transparence par défaut de chaque élément du skin

	IGUIElement* GUIs[EGN_COUNT];		// La liste des éléments parents des différentes GUIs

public:
	// Accesseurs inline
	bool isGUIVisible(E_GUI_NAME name)
	{
		if (name < EGN_COUNT && GUIs[name])
			return GUIs[name]->isVisible();

		return false;
	}
	void setGUIVisible(E_GUI_NAME name, bool value)
	{
		if (name < EGN_COUNT && GUIs[name])
			GUIs[name]->setVisible(value);
	}
	void setOnlyGUIVisible(E_GUI_NAME name)
	{
		for (u32 i = 0; i < EGN_COUNT; ++i)
			if (GUIs[i])
				GUIs[i]->setVisible(i == name);
	}
	void hideAllGUIs()
	{
		for (u32 i = 0; i < EGN_COUNT; ++i)
			if (GUIs[i])
				GUIs[i]->setVisible(false);
	}

	IGUIElement* getGUI(E_GUI_NAME name)
	{
		if (name < EGN_COUNT)
			return GUIs[name];

		return NULL;
	}

	// Retourne la transparence par défaut du skin pour une couleur
	u32 getDefaultColorTransparency(EGUI_DEFAULT_COLOR color) const
	{
		if (color < 0 || color >= EGDC_COUNT)
			return 255;

		return skinDefaultAplha[color];
	}

	// Retourne la transparence actuelle des textes de la GUI
	u32 getTextTransparency() const
	{
		// Prend la valeur de transparence la plus opaque entre celle de la configuration du jeu et celle par défaut des textes (dépendant de GUIManager::setGUITransparency(int transparency))
		return max((u32)(core::clamp(gameConfig.guiTransparency, 0, 255)), skinDefaultAplha[EGDC_BUTTON_TEXT]);
	}

	// Les éléments de la GUI
	struct GUIElements
	{
		// La gui globale
		struct GlobalGUI
		{
			IGUIStaticText* pauseTexte;			// Le texte qui indique quand on est en Pause

			CLoadingScreen* loadingScreen;		// L'écran de chargement global

			IGUIWindow* errorMessageBox;		// Une boîte de dialogue informant l'utilisateur d'une erreur (attention : ce pointeur peut être nul si aucune boîte de dialogue n'est affichée !)

			CGUIFileSelector* saveFileDialog;	// La boîte de dialogue qui demande à l'utilisateur de sauvergarder un fichier
			CGUIFileSelector* openFileDialog;	// La boîte de dialogue qui demande à l'utilisateur d'ouvrir un fichier
		} globalGUI;

		// La gui du menu principal
		struct MainMenuGUI
		{
			IGUIImage* mainTitle;			// L'image qui sera affichée au dessus du menu contenant le titre du jeu
			IGUIButton* newGameBouton;		// Le bouton pour créer une nouvelle partie dans le menu principal
			IGUIButton* chargerBouton;		// Le bouton pour charger une partie dans le menu principal
			IGUIButton* multiplayerBouton;	// Le bouton pour rejoindre une partie multijoueur dans le menu principal
			IGUIButton* optionsBouton;		// Le bouton pour modifier les options dans le menu principal
			IGUIButton* quitterBouton;		// Le bouton pour quitter dans le menu principal
#ifdef ASK_BEFORE_QUIT
			IGUIWindow* quitterWindow;		// La fenêtre qui demande si le joueur est sûr de vouloir quitter lors du menu principal
#endif
		} mainMenuGUI;

		// La gui du menu des options
		struct OptionsMenuGUI
		{
			CGUIOptionsMenu* optionsMenu;	// L'élément de la GUI spécialisé dans la gestion du menu options, égal à GUIs[EGN_optionsMenuGUI] (avec l'avantage d'être déjà correctement typé)
		} optionsMenuGUI;

		// La gui du menu pour créer une nouvelle partie
		struct NewGameMenuGUI
		{
			IGUITabControl* mainTabGroup;				// Le groupe d'onglet principal du menu permettant d'afficher plusieurs écrans de paramêtres
			IGUITab* terrainTab;						// L'onglet gérant le choix du terrain
			IGUITab* objectivesTab;						// L'onglet gérant le choix de la difficulté et les objectifs de la partie
#ifdef USE_RAKNET
			IGUITab* multiplayerTab;					// L'onglet permettant de paramêtrer cette partie en mode multijoueur
#endif

			IGUIListBox* terrainListBox;				// La liste des terrains disponibles
			IGUIStaticText* terrainDescriptionTexte;	// Le texte indiquant la description du terrain actuellement sélectionné
			IGUIImage* terrainApercuImage;				// L'image d'aperçu du terrain actuellement sélectionné

			IGUIComboBox* difficultyComboBox;			// La liste des difficultés
			CGUIObjectivesModifier* objectivesModifier;	// L'élément de la GUI permettant de définir et modifier les objectifs pour cette partie

#ifdef USE_RAKNET
			IGUICheckBox* multiplayerEnabledCheckBox;	// La case à cocher permettant d'activer le mode multijoueur
#endif

			IGUIButton* commencerBouton;				// Le bouton pour commencer la partie
			IGUIButton* retourBouton;					// Le bouton pour revenir au menu principal
		} newGameMenuGUI;

#ifdef USE_RAKNET
		// La gui du menu pour pour rejoindre une partie mutlijoueur
		struct MultiplayerMenuGUI
		{
			CGUISortTable* multiplayerGamesTable;	// Le tableau des parties multijoueurs actuellement disponibles

			IGUIButton* rejoindreBouton;			// Le bouton pour rejoindre la partie sélectionnée
			IGUIButton* retourBouton;				// Le bouton pour revenir au menu principal
		} multiplayerMenuGUI;
#endif

		// La gui de l'écran qui s'affiche lorsque le jeu est terminé (perdu ou gagné : le texte endTexte sera modifié en conséquence)
		struct GameEndGUI
		{
			IGUIImage* fondImage;							// L'image qui sert à assombrir le fond de la GUI
			IGUIStaticText* endTexte;						// Le texte informant le joueur s'il a perdu ou gagné
			IGUIButton* continuerPartieBouton;				// Le bouton proposant au joueur de continuer la partie en cours (seulement proposé s'il a gagné cette partie)
			IGUIButton* retourMenuPrincipalBouton;			// Le bouton proposant au joueur de retourner au menu principal
		} gameEndGUI;

		// La gui du jeu
		struct GameGUI
		{
			IGUIImage* tableauHaut;							// Le tableau du haut
			IGUIStaticText* budgetTexte;					// Le texte affichant le budget
			IGUIStaticText* energieTexte;					// Le texte affichant l'énergie
			IGUIStaticText* effetSerreTexte;				// Le texte affichant l'effet de serre
			IGUIStaticText* dechetsTexte;					// Le texte affichant les déchets
			IGUIStaticText* populationTexte;				// Le texte affichant la population
			IGUIStaticText* dateTexte;						// Le texte affichant la date

			IGUIStaticText* budgetInfosTexte;				// Le texte affichant des informations avancées sur le budget
			IGUIStaticText* energieInfosTexte;				// Le texte affichant des informations avancées sur l'énergie
			IGUIStaticText* effetSerreInfosTexte;			// Le texte affichant des informations avancées sur l'effet de serre
			IGUIStaticText* dechetsInfosTexte;				// Le texte affichant des informations avancées sur les déchets
			IGUIStaticText* populationInfosTexte;			// Le texte affichant des informations avancées sur la population
			IGUIStaticText* dateInfosTexte;					// Le texte affichant des informations avancées sur la date

			IGUIImage* menuDroite;							// Le menu à droite de l'écran
			IGUIStaticText* texteVitesseJeuRapide;			// Le texte complètant la scroll bar de la vitesse du jeu, indiquant le côté le plus rapide
			IGUIStaticText* texteVitesseJeuLent;			// Le texte complètant la scroll bar de la vitesse du jeu, indiquant le côté le plus lent
			IGUIScrollBar* vitesseJeuScrollBar;				// La scroll bar pour modifier la vitesse du jeu
			IGUIButton* objectivesBouton;					// Le bouton pour afficher le tableau des objectifs de la partie actuelle
			IGUIButton* monthTableBouton;					// Le bouton pour afficher le tableau récapitulatif des fins de mois
			IGUIButton* ressourcesBouton;					// Le bouton pour afficher le menu des ressources
			IGUIButton* FPSCameraBouton;					// Le bouton pour passer en vue caméra FPS dans la caméra RTS
			IGUIButton* menuBouton;							// Le bouton pour afficher le menu "Echap"

			IGUITabControl* tabGroupBas;					// Le groupe d'onglets du bas (le menu inférieur)
			IGUITab* habitationsTab;						// L'onglet des habitations
			IGUITab* habitations2Tab;						// Le deuxième onglet des habitations (chalet ; et maison de test, route et usine de test en mode normal)
			IGUITab* usinesTab;								// L'onglet des usines
			IGUITab* energieTab;							// L'onglet des bâtiments de production d'énergie
			IGUITab* dechetsTab;							// L'onglet des bâtiments de gestion des déchets
			IGUITab* arbresTab;								// L'onglet des arbres (gestion de l'effet de serre)
#ifndef MENU_INFERIEUR_REACTIF
			IGUITab* reduireTab;							// L'onglet qui réduit ou agrandit le menu inférieur
#endif

			core::list<IGUIButton*> listeBoutonsBatiments;	// La liste des boutons des batiments

			IGUIButton* detruireBouton;						// Le bouton pour détruire les batiments

			CGUIMenuWindow* menuWindow;						// Le menu qui s'affiche lorsque l'utilisateur appuye sur Echap
			CGUIObjectivesWindow* objectivesWindow;			// La fenêtre affichant les objectifs de la partie actuelle
			CGUIMonthTableWindow* monthTableWindow;			// La fenêtre affichant le récapitulatif des fins de mois
			CGUIRessourcesWindow* ressourcesWindow;			// La fenêtre du menu des ressources
			CGUIInformationsWindow* informationsWindow;		// La fenêtre des informations sur la sélection actuelle
			CGUINotesWindow* notesWindow;					// La fenêtre où l'utilisateur peut enregistrer des notes
			CGUIMiniMapWindow* miniMapWindow;				// La fenêtre affichant la mini carte du terrain actuel
			CGUICameraAnimatorWindow* cameraAnimatorWindow;	// La fenêtre permettant à l'utilisateur d'affecter une animation personnalisée à la caméra RTS du jeu
		} gameGUI;

		void reset()
		{
			// La gui globale
			{
				globalGUI.pauseTexte = NULL;

				globalGUI.loadingScreen = NULL;

				globalGUI.errorMessageBox = NULL;

				globalGUI.saveFileDialog = NULL;
				globalGUI.openFileDialog = NULL;
			}

			// La gui du menu principal
			{
				mainMenuGUI.mainTitle = NULL;
				mainMenuGUI.newGameBouton = NULL;
				mainMenuGUI.chargerBouton = NULL;
				mainMenuGUI.multiplayerBouton = NULL;
				mainMenuGUI.optionsBouton = NULL;
				mainMenuGUI.quitterBouton = NULL;
#ifdef ASK_BEFORE_QUIT
				mainMenuGUI.quitterWindow = NULL;
#endif
			}

			// La gui du menu des options
			{
				optionsMenuGUI.optionsMenu = NULL;
			}

			// La gui du menu pour créer une nouvelle partie
			{
				newGameMenuGUI.mainTabGroup = NULL;
				newGameMenuGUI.terrainTab = NULL;
				newGameMenuGUI.objectivesTab = NULL;
#ifdef USE_RAKNET
				newGameMenuGUI.multiplayerTab = NULL;
#endif

				newGameMenuGUI.terrainListBox = NULL;
				newGameMenuGUI.terrainDescriptionTexte = NULL;
				newGameMenuGUI.terrainApercuImage = NULL;

				newGameMenuGUI.difficultyComboBox = NULL;
				newGameMenuGUI.objectivesModifier = NULL;

#ifdef USE_RAKNET
				newGameMenuGUI.multiplayerEnabledCheckBox = NULL;
#endif

				newGameMenuGUI.commencerBouton = NULL;
				newGameMenuGUI.retourBouton = NULL;
			}

#ifdef USE_RAKNET
			// La gui du menu pour pour rejoindre une partie mutlijoueur
			{
				multiplayerMenuGUI.multiplayerGamesTable = NULL;

				multiplayerMenuGUI.rejoindreBouton = NULL;
				multiplayerMenuGUI.retourBouton = NULL;
			}
#endif

			// La gui de l'écran qui s'affiche lorsque le jeu est terminé (perdu ou gagné)
			{
				gameEndGUI.fondImage = NULL;
				gameEndGUI.endTexte = NULL;
				gameEndGUI.continuerPartieBouton = NULL;
				gameEndGUI.retourMenuPrincipalBouton = NULL;
			}

			// La gui du jeu
			{
				gameGUI.tableauHaut = NULL;
				gameGUI.budgetTexte = NULL;
				gameGUI.energieTexte = NULL;
				gameGUI.effetSerreTexte = NULL;
				gameGUI.dechetsTexte = NULL;
				gameGUI.populationTexte = NULL;
				gameGUI.dateTexte = NULL;

				gameGUI.budgetInfosTexte = NULL;
				gameGUI.energieInfosTexte = NULL;
				gameGUI.effetSerreInfosTexte = NULL;
				gameGUI.dechetsInfosTexte = NULL;
				gameGUI.populationInfosTexte = NULL;
				gameGUI.dateInfosTexte = NULL;

				gameGUI.menuDroite = NULL;
				gameGUI.texteVitesseJeuRapide = NULL;
				gameGUI.texteVitesseJeuLent = NULL;
				gameGUI.vitesseJeuScrollBar = NULL;
				gameGUI.objectivesBouton = NULL;
				gameGUI.monthTableBouton = NULL;
				gameGUI.ressourcesBouton = NULL;
				gameGUI.FPSCameraBouton = NULL;
				gameGUI.menuBouton = NULL;

				gameGUI.tabGroupBas = NULL;
				gameGUI.habitationsTab = NULL;
				gameGUI.habitations2Tab = NULL;
				gameGUI.usinesTab = NULL;
				gameGUI.energieTab = NULL;
				gameGUI.dechetsTab = NULL;
				gameGUI.arbresTab = NULL;
#ifndef MENU_INFERIEUR_REACTIF
				gameGUI.reduireTab = NULL;
#endif

				gameGUI.listeBoutonsBatiments.clear();

				gameGUI.detruireBouton = NULL;

				gameGUI.menuWindow = NULL;
				gameGUI.objectivesWindow = NULL;
				gameGUI.monthTableWindow = NULL;
				gameGUI.ressourcesWindow = NULL;
				gameGUI.informationsWindow = NULL;
				gameGUI.notesWindow = NULL;
				gameGUI.miniMapWindow = NULL;
				gameGUI.cameraAnimatorWindow = NULL;
			}
		}
		GUIElements()	{ reset(); }
	} guiElements;

	// Retourne la vitesse du timer du jeu sélectionnée dans la GUI du jeu (d'après sa scroll bar de réglage)
	float getGameGUITimerSpeed() const
	{
		if (!guiElements.gameGUI.vitesseJeuScrollBar)
			return 1.0f;

		// Obtient la position actuelle de la scroll bar
		// Attention : ne pas oublier que la position de la scroll bar est inversée
		const int pos = (guiElements.gameGUI.vitesseJeuScrollBar->getMax() - guiElements.gameGUI.vitesseJeuScrollBar->getPos());
		switch (pos)
		{
		case 0:
			return 0.1f;
		case 1:
			return 0.2f;
		case 2:
			return 0.5f;
		case 3:
			return 1.0f;
		case 4:
			return 2.0f;
		case 5:
			return 5.0f;
		case 6:
			return 10.0f;
		case 7:
			return 20.0f;	// Mode Boost uniquement (autorise des vitesses de jeu plus élevées)
		case 8:
			return 50.0f;	// Mode Boost uniquement (autorise des vitesses de jeu plus élevées)
		}

		// Position de la scroll bar inconnue : on retourne 1.0f par défaut
		LOG_DEBUG("GUIManager::getGameGUITimerSpeed() : Position de la scroll bar de la vitesse du jeu inconnue : pos = " << pos, ELL_WARNING);
		return 1.0f;
	}
	// Indique la vitesse du timer du jeu sélectionnée dans la GUI du jeu
	void setGameGUITimerSpeed(float speed)
	{
		IGUIScrollBar* const vitesseJeuScrollBar = guiElements.gameGUI.vitesseJeuScrollBar;
		if (!vitesseJeuScrollBar)
			return;

		// Attention : ne pas oublier ici que la position de la scroll bar est inversée ! (aussi par rapport à la fonction getGameGUITimerSpeed())

		if (vitesseJeuScrollBar->getMax() > 6)	// Mode Boost (autorise des vitesses de jeu plus élevées)
		{
			if (speed > 35.0f)	// (50.0f + 20.0f) / 2.0f
				vitesseJeuScrollBar->setPos(0);					// Valeur : 50.0f	(35.0f < speed)
			else if (speed > 15.0f)	// (20.0f + 10.0f) / 2.0f
				vitesseJeuScrollBar->setPos(1);					// Valeur : 20.0f	(15.0f < speed < 35.0f)
			else if (speed > 7.5f)	// (10.0f + 5.0f) / 2.0f
				vitesseJeuScrollBar->setPos(2);					// Valeur : 10.0f	(7.5f < speed < 15.0f)
			else if (speed > 3.5f)	// (5.0f + 2.0f) / 2.0f
				vitesseJeuScrollBar->setPos(3);					// Valeur : 5.0f	(3.5f < speed < 7.5f)
			else if (speed > 1.5f)	// (2.0f + 1.0f) / 2.0f
				vitesseJeuScrollBar->setPos(4);					// Valeur : 2.0f	(1.5f < speed < 3.5f)
			else if (speed > 0.75f)	// (1.0f + 0.5f) / 2.0f
				vitesseJeuScrollBar->setPos(5);					// Valeur : 1.0f	(0.75f < speed < 1.5f)
			else if (speed > 0.35f)	// (0.5f + 0.2f) / 2.0f
				vitesseJeuScrollBar->setPos(6);					// Valeur : 0.5f	(0.35f < speed < 0.75f)
			else if (speed > 0.15f)	// (0.2f + 0.1f) / 2.0f
				vitesseJeuScrollBar->setPos(7);					// Valeur : 0.2f	(0.15f < speed < 0.35f)
			else
				vitesseJeuScrollBar->setPos(8);					// Valeur : 0.1f	(speed < 0.15f)
		}
		else									// Mode normal
		{
			if (speed > 7.5f)		// (10.0f + 5.0f) / 2.0f
				vitesseJeuScrollBar->setPos(0);					// Valeur : 10.0f	(7.5f < speed)
			else if (speed > 3.5f)	// (5.0f + 2.0f) / 2.0f
				vitesseJeuScrollBar->setPos(1);					// Valeur : 5.0f	(3.5f < speed < 7.5f)
			else if (speed > 1.5f)	// (2.0f + 1.0f) / 2.0f
				vitesseJeuScrollBar->setPos(2);					// Valeur : 2.0f	(1.5f < speed < 3.5f)
			else if (speed > 0.75f)	// (1.0f + 0.5f) / 2.0f
				vitesseJeuScrollBar->setPos(3);					// Valeur : 1.0f	(0.75f < speed < 1.5f)
			else if (speed > 0.35f)	// (0.5f + 0.2f) / 2.0f
				vitesseJeuScrollBar->setPos(4);					// Valeur : 0.5f	(0.35f < speed < 0.75f)
			else if (speed > 0.15f)	// (0.2f + 0.1f) / 2.0f
				vitesseJeuScrollBar->setPos(5);					// Valeur : 0.2f	(0.15f < speed < 0.35f)
			else
				vitesseJeuScrollBar->setPos(6);					// Valeur : 0.1f	(speed < 0.15f)
		}
	}
};

#endif
