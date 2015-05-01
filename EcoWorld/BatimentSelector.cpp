#include "BatimentSelector.h"
#include "Game.h"
#include "RTSCamera.h"
#include "GUIManager.h"
#include "CGUIInformationsWindow.h"
#include "EcoWorldRenderer.h"
#include "EcoWorldSystem.h"
#include "CPostProcessManager.h"
#ifdef USE_IRRKLANG
#include "IrrKlangManager.h"
#endif
#ifdef USE_RAKNET
#include "RakNetManager.h"
#endif

// D�termine si un point est en-dehors du syst�me de jeu
#define IS_CORNER_OUT_OF_TERRAIN(corner)	((corner).X < 0 || (corner).Y < 0 || (corner).X >= TAILLE_CARTE || (corner).Y >= TAILLE_CARTE)

inline bool BatimentSelector::isPlacingBat() const
{
	return  (placerBatiment.currentBatiment != BI_aucun
		|| (game->guiManager->guiElements.gameGUI.detruireBouton && game->guiManager->guiElements.gameGUI.detruireBouton->isPressed()));
}
bool BatimentSelector::OnEvent(const SEvent& event)
{
	// G�re les �v�nements du jeu pour le placement et la s�lection de b�timents :

	const bool isCreatingOrDestroyingBatiment = (placerBatiment.currentBatiment != BI_aucun
		|| (game->guiManager->guiElements.gameGUI.detruireBouton && game->guiManager->guiElements.gameGUI.detruireBouton->isPressed()));
#ifdef CAN_BUILD_WITH_SELECTION_RECT
	const bool isRectConstructing = (placerBatiment.rectSelection != core::recti(-1, -1, -1, -1));
#endif

	// Placement des b�timents (Souris)
	if (event.EventType == EET_MOUSE_INPUT_EVENT)
	{
		if (event.MouseInput.Event == EMIE_RMOUSE_PRESSED_DOWN && !game->isMouseOnGUI && isPlacingOrSelectingBat())
		{
#ifdef CAN_BUILD_WITH_SELECTION_RECT
			if (isRectConstructing)
			{
				// On a cliqu� avec le bouton droit et on est en train de faire une s�lection rectangulaire :
				// on annule la s�lection rectangulaire et on d�truit la liste des b�timents de pr�visualisation
				placerBatiment.rectSelection = core::recti(-1, -1, -1, -1);
				placerBatiment.nbBatimentsPreviewRect = 0;
				const u32 listeBatimentsPreviewRectSize = placerBatiment.listeBatimentsPreviewRect.size();
				for (u32 i = 0; i < listeBatimentsPreviewRectSize; ++i)
				{
					scene::ISceneNode* const batiment = placerBatiment.listeBatimentsPreviewRect[i];
					batiment->setVisible(false);
					game->sceneManager->addToDeletionQueue(batiment);

					// Si PostProcess est activ�, on lui indique qu'il ne doit plus prendre en compte ce b�timent dans le flou de profondeur
					if (game->postProcessManager)
						game->postProcessManager->removeNodeFromDepthPass(batiment);

					// Si XEffects est activ�, on lui indique qu'il ne doit plus afficher l'ombre de ce b�timent
					if (game->xEffects)
						game->xEffects->removeShadowFromNode(batiment);
				}
				placerBatiment.listeBatimentsPreviewRect.clear();

				// D�sactivable (�lement de jouabilit�) : De plus, on annule la cr�ation du batiment en cours
				annulerSelection();
			}
			else
#endif
			{
				// On a cliqu� avec le bouton droit et un batiment est sur le point d'�tre cr�e (ou on est en train de d�truire des batiments) :
				// on annule la cr�ation du batiment
				annulerSelection();
			}
		}
		else if (event.MouseInput.Event == EMIE_MOUSE_WHEEL && !game->isMouseOnGUI && event.MouseInput.Control && placerBatiment.currentBatiment != BI_aucun)
		{
			// Ctrl + Molette de la souris :
			// on modifie la rotation du batiment de pr�visualisation de 45� (ou de 90� en mode de construction rectangulaire)
			placerBatiment.currentBatimentRotation += event.MouseInput.Wheel * (isRectConstructing ? 90.0f : 45.0f);

			// Replace la rotation entre 0� et 360�
			while (placerBatiment.currentBatimentRotation < 0.0f)		placerBatiment.currentBatimentRotation += 360.0f;
			while (placerBatiment.currentBatimentRotation >= 360.0f)	placerBatiment.currentBatimentRotation -= 360.0f;
		}
#ifdef CAN_BUILD_WITH_SELECTION_RECT
		else if (event.MouseInput.Event == EMIE_MOUSE_MOVED && isRectConstructing)
		{
			// Si on est en train de faire une s�lection rectangulaire et que la souris a boug� :
			// on actualise le rectangle de s�lection actuel
			placerBatiment.rectSelection.LowerRightCorner.set(event.MouseInput.X, event.MouseInput.Y);
			//placerBatiment.rectSelection.repair();
		}
#endif
		else if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN && !game->isMouseOnGUI	// Clic gauche de la souris (appuy�) en dehors de la GUI
			&& !game->cameraRTS->getIsCameraFPSEnabled())
		{
			// Si on est en train de cr�er un b�timent : on valide l'action
			if (isCreatingOrDestroyingBatiment)
			{
#ifdef CAN_BUILD_WITH_SELECTION_RECT
				if (StaticBatimentInfos::canCreateBySelectionRect(placerBatiment.currentBatiment))
				{
					// Le batiment actuel peut �tre cr�� par rectangle : on commence un rectangle de s�lection
					placerBatiment.rectSelection.UpperLeftCorner.set(event.MouseInput.X, event.MouseInput.Y);
					placerBatiment.rectSelection.LowerRightCorner.set(event.MouseInput.X, event.MouseInput.Y);
				}
				else
#endif
					// On a cliqu� avec le bouton gauche : on cr�e le batiment et le place, ou on d�truit le batiment
					placerBatiment.validateAction();
			}
			else	// Sinon : on valide la s�lection du b�timent point�
				selectBatiment.selectBatiment();
		}
#ifdef CAN_BUILD_WITH_SELECTION_RECT
		else if (event.MouseInput.Event == EMIE_LMOUSE_LEFT_UP && isRectConstructing)
		{
			// On a relach� le bouton gauche de la souris et on �tait en train de faire une s�lection rectangulaire :
			// on termine la s�lection en pla�ant le batiment choisi
			placerBatiment.rectSelection.LowerRightCorner.set(event.MouseInput.X, event.MouseInput.Y);

			placerBatiment.validateAction();	// La m�thode d�terminera automatiquement qu'on �tait en train de cr�er une s�lection rectangulaire

			placerBatiment.rectSelection = core::recti(-1, -1, -1, -1);
			placerBatiment.nbBatimentsPreviewRect = 0;
		}
#endif
	}
		// Raccourcis claviers :
	else if (event.EventType == EET_KEY_INPUT_EVENT)
	{
		// Sp�cifique au jeu :
		if ((event.KeyInput.Key == KEY_LEFT || event.KeyInput.Key == KEY_RIGHT) && event.KeyInput.Control && !event.KeyInput.Shift
			&& !event.KeyInput.PressedDown && placerBatiment.currentBatiment != BI_aucun)
		{
			// Ctrl + Fl�che gauche ou droite :
			// on modifie la rotation du batiment de pr�visualisation de 45�
			placerBatiment.currentBatimentRotation += 45.0f * (event.KeyInput.Key == KEY_LEFT ? 1.0f : -1.0f);

			// Replace la rotation entre 0 et 360�
			while (placerBatiment.currentBatimentRotation < 0.0f)		placerBatiment.currentBatimentRotation += 360.0f;
			while (placerBatiment.currentBatimentRotation >= 360.0f)	placerBatiment.currentBatimentRotation -= 360.0f;
		}
		else if ((event.KeyInput.Key == KEY_UP || event.KeyInput.Key == KEY_DOWN) && event.KeyInput.Control && !event.KeyInput.Shift
			&& !event.KeyInput.PressedDown && placerBatiment.currentBatiment != BI_aucun)
		{
			// Ctrl + Fl�che haut ou bas :
			// on modifie la rotation du batiment de pr�visualisation de 180�
			placerBatiment.currentBatimentRotation += 180.0f * (event.KeyInput.Key == KEY_UP ? 1.0f : -1.0f);

			// Replace la rotation entre 0 et 360�
			while (placerBatiment.currentBatimentRotation < 0.0f)		placerBatiment.currentBatimentRotation += 360.0f;
			while (placerBatiment.currentBatimentRotation >= 360.0f)	placerBatiment.currentBatimentRotation -= 360.0f;
		}
		else if (event.KeyInput.Key == KEY_DELETE && !event.KeyInput.PressedDown && game->guiManager->guiElements.gameGUI.detruireBouton && game->canHandleKeysEvent)
		{
			// DEL (ou SUPPR)

			// On peut aussi activer/d�sactiver le bouton pour d�truire un b�timent par un appui sur la touche DEL :

			// Retiens si ce bouton va �tre press� d'apr�s son �tat actuel (annulerSelection() va rendre ce bouton non press� !)
			const bool newPressed = !game->guiManager->guiElements.gameGUI.detruireBouton->isPressed();

			// Annule la s�lection actuelle (cr�ation/destruction/s�lection d'un b�timent)
			annulerSelection();

			// Rend � nouveau ce bouton press�
			game->guiManager->guiElements.gameGUI.detruireBouton->setPressed(newPressed);
		}
	}

	return false;
}
void BatimentSelector::annulerSelection()
{
	// Annule la construction du batiment
	if (placerBatiment.currentBatiment != BI_aucun)
	{
		// Rend tous les boutons non press�s
		const core::list<gui::IGUIButton*>::Iterator END = game->guiManager->guiElements.gameGUI.listeBoutonsBatiments.end();
		for (core::list<gui::IGUIButton*>::Iterator it = game->guiManager->guiElements.gameGUI.listeBoutonsBatiments.begin(); it != END; ++it)
		{
			gui::IGUIButton* const currentBouton = (*it);
			if (currentBouton)
				currentBouton->setPressed(false);
		}

		// Indique qu'il n'y a plus de b�timent en construction (moveBatiment sera r�initialis� dans BatimentSelector::PlacerBatiment::updatePrevisualisationBat())
		placerBatiment.currentBatiment = BI_aucun;
	}

	// D�selectionne le b�timent actuellement s�lectionn�
	selectBatiment.deselectBatiment();

	// Rend aussi le bouton pour d�truire un batiment non press�
	if (game->guiManager->guiElements.gameGUI.detruireBouton)
		game->guiManager->guiElements.gameGUI.detruireBouton->setPressed(false);

	// Remet le batiment s�lectionn� pour la destruction � sa couleur normale et masque son texte (-> n'est plus fait dans placerBatiment.resetPreviewErrors)
	if (placerBatiment.pointedToDestroyBatiment)
	{
		Game::resetBatimentColor(placerBatiment.pointedToDestroyBatiment);
		placerBatiment.pointedToDestroyBatiment->setNomBatimentVisible(false);
		placerBatiment.pointedToDestroyBatiment = NULL;
	}

	// R�initialise les appels � placerBatiment.frameUpdate() et selectBatiment.frameUpdate()
	placerBatiment.lastUpdateParams.reset();
	selectBatiment.lastUpdateParams.reset();
}
void BatimentSelector::reset()
{
	// R�initialise les deux modules de placement et de s�lection des b�timents
	placerBatiment.reset();
	selectBatiment.reset();
}
void BatimentSelector::pushBackGamePointers(core::list<CBatimentSceneNode**>& gameSceneNodesPointers)
{
	// Ajoute les pointeurs du jeu susceptibles de pointer sur un scene node invalide (sur le point de se d�truire) � la liste fournie
	gameSceneNodesPointers.push_back(&placerBatiment.pointedToDestroyBatiment);
	gameSceneNodesPointers.push_back(&placerBatiment.samePosBatiment);
	gameSceneNodesPointers.push_back(&selectBatiment.selectedBatiment);
	gameSceneNodesPointers.push_back(&selectBatiment.pointedBatiment);
}
void BatimentSelector::update()
{
	// Met � jour les deux modules de placement et de s�lection des b�timents
	placerBatiment.frameUpdate();
	if (placerBatiment.currentBatiment == BI_aucun)
		selectBatiment.frameUpdate();

	// Met � jour la GUI du jeu d'apr�s le placement et la s�lection actuelle, si elle est visible
	if (game->guiManager->isGUIVisible(GUIManager::EGN_gameGUI))
		updateGameGUI();
}
void BatimentSelector::updateGameGUI()
{
	GUIManager* const guiManager = game->guiManager;

	// Met � jour les informations sur le b�timent actuellement s�lectionn�
	if (guiManager->guiElements.gameGUI.informationsWindow)
	{
		// Indique le b�timent actuellement s�lectionn� � la fen�tre d'informations
		guiManager->guiElements.gameGUI.informationsWindow->setSelectedBatiment(selectBatiment.selectedBatiment ? selectBatiment.selectedBatiment->getAssociatedBatiment() : NULL);

		// Indique si la fen�tre d'informations devra �tre visible ou non
		bool informationsWindowVisible = false;

		// Construction :
		if (placerBatiment.currentBatiment != BI_aucun)
		{
#ifdef CAN_BUILD_WITH_SELECTION_RECT
			if (placerBatiment.rectSelection != core::recti(-1, -1, -1, -1) && placerBatiment.nbBatimentsPreviewRect > 1)	// Construction de b�timents en rectangle avec plus d'un b�timent pr�vu
				guiManager->guiElements.gameGUI.informationsWindow->updateInfosTexte(
					CGUIInformationsWindow::EIT_MULTI_CONSTRUCTION, placerBatiment.currentBatiment, placerBatiment.nbBatimentsPreviewRect, NULL);
			else	// Construction simple d'un b�timent
#endif
				guiManager->guiElements.gameGUI.informationsWindow->updateInfosTexte(
					CGUIInformationsWindow::EIT_CONSTRUCTION, placerBatiment.currentBatiment, 0, NULL);

			informationsWindowVisible = true;
		}
		// Destruction :
		else if (placerBatiment.pointedToDestroyBatiment && placerBatiment.pointedToDestroyBatiment->getAssociatedBatiment())
		{
			guiManager->guiElements.gameGUI.informationsWindow->updateInfosTexte(
				CGUIInformationsWindow::EIT_DESTRUCTION, BI_aucun, 0, placerBatiment.pointedToDestroyBatiment->getAssociatedBatiment());
			informationsWindowVisible = true;
		}
		// S�lection :
		else if (selectBatiment.selectedBatiment && selectBatiment.selectedBatiment->getAssociatedBatiment())
		{
			guiManager->guiElements.gameGUI.informationsWindow->updateInfosTexte(
				CGUIInformationsWindow::EIT_SELECTION, BI_aucun, 0, selectBatiment.selectedBatiment->getAssociatedBatiment());
			informationsWindowVisible = true;
		}

		// Rend la fen�tre d'informations visible si n�cessaire
		guiManager->guiElements.gameGUI.informationsWindow->setVisible(informationsWindowVisible);
	}
}
scene::ISceneNode* BatimentSelector::getPointedBat(const core::vector2di& mousePos)
{
	// Si la cam�ra, le renderer ou le triangle selector des b�timents ne sont pas valides, on quitte
	scene::ICameraSceneNode* const camera = game->sceneManager->getActiveCamera();
	if (!camera || !game->renderer->getBatimentsTriangleSelector())
		return NULL;

	const bool validMousePos = (mousePos.X >= 0 && mousePos.Y >= 0);

	scene::ISceneNode* node = NULL;
	core::triangle3df triangle;
	core::vector3df position;

	// Obtient le node vis� par la souris
	scene::ISceneCollisionManager* const collisionMgr = game->sceneManager->getSceneCollisionManager();
	const core::line3df ligne = collisionMgr->getRayFromScreenCoordinates(validMousePos ? mousePos : game->mouseState.position, camera);
	const bool collision = collisionMgr->getCollisionPoint(ligne, game->renderer->getBatimentsTriangleSelector(), position, triangle, node);
	if (!collision)
		return NULL;

	// Si le type de ce node est un node de b�timent, on le renvoie
	if (node->getType() == ESNT_BATIMENT_NODE)
		return node;

	// Sinon, on recherche si un parent de ce node n'est pas un b�timent
	scene::ISceneNode* parentNode = node->getParent();
	node = NULL;
	while (parentNode && !node)
	{
		if (parentNode->getType() == ESNT_BATIMENT_NODE)
			node = parentNode;
		else
			parentNode = parentNode->getParent();
	}
	return node;
}
core::vector3df BatimentSelector::getPointedTerrainPos(const core::vector2di& mousePos)
{
	// Si la cam�ra, le renderer ou le triangle selector du terrain ne sont pas valides, on quitte
	scene::ICameraSceneNode* const camera = game->sceneManager->getActiveCamera();
	core::vector3df position(-1.0f);
	if (!camera || !game->renderer)
		return position;
	if (!game->renderer->getTerrainTriangleSelector())
		return position;

	const bool validMousePos = (mousePos.X >= 0 && mousePos.Y >= 0);

	scene::ISceneNode* node = 0;
	core::triangle3df triangle;

	// Obtient la position vis�e par la souris
	scene::ISceneCollisionManager* const collisionMgr = game->sceneManager->getSceneCollisionManager();
	const core::line3df ligne = collisionMgr->getRayFromScreenCoordinates(validMousePos ? mousePos : game->mouseState.position, camera);
	const bool collision = collisionMgr->getCollisionPoint(
		ligne, game->renderer->getTerrainTriangleSelector(), position, triangle, node);

	return (collision ? position : core::vector3df(-1.0f));
}
void BatimentSelector::PlacerBatiment::frameUpdate()
{
	// Met � jour le b�timent de pr�visualisation
	updatePrevisualisationBat();

	// D�termine si le joueur veut d�truire ou construire des batiments
	if (game->guiManager->guiElements.gameGUI.detruireBouton && game->guiManager->guiElements.gameGUI.detruireBouton->isPressed())
	{
		// R�initialise les param�tres pour la construction
		lastUpdateParams.batimentID = BI_aucun;
		lastUpdateParams.batimentRotation = -1.0f;
#ifdef CAN_BUILD_WITH_SELECTION_RECT
		// R�initialise les param�tres pour la construction en zone rectangulaire
		lastUpdateParams.rectSelection = core::recti(-1, -1, -1, -1);
		lastUpdateParams.pointedFirstCorner.set(-1, -1);
#if !defined(FORCE_RECTANGLE) || !defined(ONLY_TAKE_MAIN_CORNERS)
		lastUpdateParams.pointedSecondCorner.set(-1, -1);
		lastUpdateParams.pointedThirdCorner.set(-1, -1);
#endif
		lastUpdateParams.pointedLastCorner.set(-1, -1);
#endif

		// Pr�visualise la destruction simple du batiment
		previewDestroySimpleBat();
	}
#ifdef CAN_BUILD_WITH_SELECTION_RECT
	else if (rectSelection != core::recti(-1, -1, -1, -1))
	{
		// Pr�visualise la cr�ation en zone rectangulaire du batiment
		previewCreateRectBats(currentBatiment);
	}
#endif
	else
	{
#ifdef CAN_BUILD_WITH_SELECTION_RECT
		// R�initialise les param�tres pour la construction en zone rectangulaire
		lastUpdateParams.rectSelection = core::recti(-1, -1, -1, -1);
		lastUpdateParams.pointedFirstCorner.set(-1, -1);
#if !defined(FORCE_RECTANGLE) || !defined(ONLY_TAKE_MAIN_CORNERS)
		lastUpdateParams.pointedSecondCorner.set(-1.0f, -1.0f, -1.0f);
		lastUpdateParams.pointedThirdCorner.set(-1.0f, -1.0f, -1.0f);
#endif
		lastUpdateParams.pointedLastCorner.set(-1, -1);
#endif

		// Pr�visualise la cr�ation simple du batiment
		previewCreateSimpleBat(currentBatiment, moveBatiment);
	}
}
void BatimentSelector::PlacerBatiment::validateAction()
{
	// R�initialise la GUI d�e � la pr�visualisation des batiments
	resetPreviewErrors(moveBatiment);

	// D�termine si le joueur veut d�truire ou construire des batiments
	if (game->guiManager->guiElements.gameGUI.detruireBouton && game->guiManager->guiElements.gameGUI.detruireBouton->isPressed())
		destroySimpleBat();					// D�truit le batiment
#ifdef CAN_BUILD_WITH_SELECTION_RECT
	else if (rectSelection != core::recti(-1, -1, -1, -1))
		createRectBats(currentBatiment);	// Cr�e le batiment en zone rectangulaire
#endif
	else
		createSimpleBat(currentBatiment);	// Cr�e le batiment

	// R�initialise tous les param�tres de frameUpdate()
	lastUpdateParams.reset();
}
void BatimentSelector::PlacerBatiment::updatePrevisualisationBat()
{
	// Recr�e le b�timent de pr�visualisation si besoin
	static BatimentID lastBatiment = currentBatiment;
	if (lastBatiment != currentBatiment)	// Si le batiment choisi est diff�rent du batiment actuel
	{
		lastBatiment = currentBatiment;
		if (moveBatiment)
		{
			// Si PostProcess est activ�, on lui indique qu'il ne doit plus prendre en compte ce b�timent dans le flou de profondeur
			if (game->postProcessManager)
				game->postProcessManager->removeNodeFromDepthPass(moveBatiment);

			// Si XEffects est activ�, on lui indique qu'il ne doit plus afficher l'ombre de ce b�timent
			if (game->xEffects)
				game->xEffects->removeShadowFromNode(moveBatiment);

			moveBatiment->setVisible(false);
			game->sceneManager->addToDeletionQueue(moveBatiment); // On supprime l'ancien batiment
		}
		moveBatiment = NULL; // Et on met sa valeur � NULL pour qu'un autre soit cr��

		if (samePosBatiment)
		{
			Game::resetBatimentColor(samePosBatiment);
			samePosBatiment = NULL;
		}
	}

	// On doit cr�er un nouveau b�timent de pr�visualisation s'il n'et pas d�j� cr�� et qu'un type de b�timent est s�lectionn� pour �tre cr��
	if (!moveBatiment && currentBatiment != BI_aucun)
	{
		// Cr�e le b�timent de pr�visualisation
		moveBatiment = game->renderer->loadBatiment(currentBatiment, 0, false, true);

		// Ajoute un bloc de b�ton sous le b�timent de pr�visualisation si n�cessaire
		if (moveBatiment)
		{
			if (StaticBatimentInfos::needConcreteUnderBatiment(currentBatiment))
			{
				// Le d�nivel� ici indiqu� est arbitraire : il peut �tre augment� ou diminu� suivant les besoins des terrains
				scene::ISceneNode* const concreteNode = game->renderer->loadConcreteBatiment(40.0f, currentBatiment, moveBatiment, false, true);

				// Modifie l'agrandissement du node de b�ton pour que son agrandissement final soit de 1.0f (�vite qu'il ne soit affect� par l'agrandissement du b�timent de pr�visualisation)
				const core::vector3df& moveBatimentScaling = moveBatiment->getScale();
				concreteNode->setScale(core::vector3df(1.0f / moveBatimentScaling.X, 1.0f / moveBatimentScaling.Y, 1.0f / moveBatimentScaling.Z));
			}

#if 0		// Permet de le d�sactiver : peu efficace lorsque la cam�ra est �loign�e, et trompe l�g�rement le joueur sur la taille r�elle du b�timent
			// Agrandit un petit peu le b�timent par rapport � sa taille normale pour qu'il soit "par-dessus" les autres b�timents du m�me type :
			if (currentBatiment != BI_route)
				moveBatiment->setScale(moveBatiment->getScale() * core::vector3df(1.05f, 1.05f, 1.05f));
			else // Pour les routes, on augmente plus la hauteur, et moins la largeur
				moveBatiment->setScale(moveBatiment->getScale() * core::vector3df(1.001f, 1.5f, 1.001f));
#endif
		}
	}

	// Masque le b�timent de pr�visualisation si on est en mode de cr�ation rectangulaire
	if (moveBatiment && rectSelection != core::recti(-1, -1, -1, -1))
		moveBatiment->setVisible(false);
}
void BatimentSelector::PlacerBatiment::previewCreateSimpleBat(BatimentID batimentID, scene::ISceneNode* batiment, core::vector2di index)
{
	// Si le batiment � placer n'est pas valide, on quitte
	if (!batiment)
		return;

	// D�termine si cette fonction a �t� appel�e par la pr�visualisation rectangulaire : si le b�timent qu'on doit construire est cr�� par rectangle de s�lection ET que le b�timent fourni n'est pas le b�timent de pr�visualisation normal
	const bool calledByRectCreation = (StaticBatimentInfos::canCreateBySelectionRect(batimentID)) && (batiment != moveBatiment);

	if (!calledByRectCreation)	// V�rifie si les param�tres ont chang� si on n'a pas �t� appel� par la pr�visualisation rectangulaire
	{
		// Si aucun param�tre n'a chang� depuis le dernier appel, on quitte aussi
		const core::vector3df camPos = game->sceneManager->getActiveCamera() ? game->sceneManager->getActiveCamera()->getAbsolutePosition() : core::vector3df(-1.0f);
		if (!lastUpdateParams.hasConstructSimpleParamsChanged(game->system.getTime().getTotalJours(), batimentID, currentBatimentRotation, game->mouseState.position, camPos))
			return;

		// Met � jour les param�tres pour le dernier appel � cette fonction
		lastUpdateParams.systemTotalDays = game->system.getTime().getTotalJours();
		lastUpdateParams.batimentID = batimentID;
		lastUpdateParams.batimentRotation = currentBatimentRotation;
		lastUpdateParams.mousePos = game->mouseState.position;
		lastUpdateParams.camPos = camPos;

		// R�initialise la GUI d�e � la pr�visualisation des batiments
		resetPreviewErrors(moveBatiment);
	}

	core::vector3df position;
	bool visible = true;

	// V�rifie que l'index fourni est bien valide, sauf si on a �t� appel� par la pr�visualisation de b�timents en rectangle : dans ce cas, on conserve cet index quoi qu'il arrive
	if ((index.X >= 0 && index.Y >= 0 && index.X < TAILLE_CARTE && index.Y < TAILLE_CARTE) || calledByRectCreation)
		position = EcoWorldSystem::getPositionFromIndex(index);	// Obtient la position du node suivant son index
	else
	{
		// Obtient la position du terrain point�e par la souris
		position = getPointedTerrainPos();

		// La souris est en dehors du terrain si : position == core::vector3df(-1.0f)
		if (position == core::vector3df(-1.0f))
			visible = false;
		else
		{
			visible = true;

			// Arrondi la position � TAILLE_OBJETS en X et en Z
			position = EcoWorldSystem::getRoundPosition(position);

			// Obtient l'index du b�timent
			index = EcoWorldSystem::getIndexFromPosition(position);
		}
	}

	// Lorsque le batiment de pr�visualisation est sur un batiment d�j� existant et du m�me ID avec la m�me rotation actuelle sans qu'il ne se construise ou d�truise,
	// on modifie la couleur du batiment d�j� existant et on masque le batiment de pr�visualisation.
	// Sinon, on modifie effectivement le batiment de pr�visualisation.
	const Batiment* const samePosBat =
		(index.X >= 0 && index.Y >= 0 && index.X < TAILLE_CARTE && index.Y < TAILLE_CARTE) ? game->system.carte[index.X][index.Y].batiment : NULL;
	if (samePosBat && !calledByRectCreation			// TODO : Faire fonctionner ce syst�me aussi en mode cr�ation rectangulaire
		&& samePosBat->getID() == batimentID && samePosBat->getRotation() == currentBatimentRotation
		&& !samePosBat->isConstructingOrDestroying())
	{
		samePosBatiment = reinterpret_cast<CBatimentSceneNode*>(game->system.carte[index.X][index.Y].batiment->getSceneNode());
		batiment->setVisible(false);
	}
	else
	{
		batiment->setVisible(visible);
		if (visible)
		{
			// Obtient la position du terrain en Y d'apr�s le terrain actuel
			game->renderer->getNewBatimentPos(position, batimentID, currentBatimentRotation);

			// Place le batiment
			batiment->setPosition(position);
			batiment->setRotation(core::vector3df(0.0f, currentBatimentRotation, 0.0f));
		}
	}

	// V�rifie si on peut placer le batiment
	bool outRessources[Ressources::RI_COUNT];
#ifndef _DEBUG
	const	// Le const d�sactiv� permet de d�sactiver l'erreur EBCE_NOT_ENOUGH_POPULATION en mode DEBUG
#endif
		int error = game->system.canCreateBatiment(batimentID, index, currentBatimentRotation, outRessources);

#ifdef _DEBUG
	// /!\ Attention /!\ 
	// En mode DEBUG uniquement : On d�sactive l'erreur EBCE_NOT_ENOUGH_POPULATION pour permettre le fonctionnement du code de d�bogage pour d�bloquer tous les b�timents
	if (game->unlockAllBatiments)
		error &= ~EBCE_NOT_ENOUGH_POPULATION;	// Passe le bit repr�sent� par EBCE_NOT_ENOUGH_POPULATION � 0 (~ : op�rateur NOT bit � bit)
#endif

	// On n'affiche pas l'erreur du manque de place si le b�timent peut �tre cr�� par rectangle de s�lection
	//if (canCreateBatimentBySelectionRect(batimentID))
	//	error &= ~EBCE_PLACE;	// Passe le bit repr�sent� par EBCE_PLACE � 0 (~ : op�rateur NOT bit � bit)

	// Affiche les messages � l'utilisateur indiquant pourquoi on n'a pas pu placer le b�timent
	showSimpleCreationErrors(NULL, samePosBatiment ? samePosBatiment : batiment, error, outRessources);
}
void BatimentSelector::PlacerBatiment::previewDestroySimpleBat(core::vector2di index)
{
	// D�termine si cette fonction a �t� appel�e par la pr�visualisation rectangulaire
	//const bool calledByRectDestruction = ;	// TODO

	//if (!calledByRectDestruction)	// V�rifie si les param�tres ont chang� si on n'a pas �t� appel� par la pr�visualisation rectangulaire
	//{
	// V�rifie que les param�tres ont bien chang� depuis le dernier appel � cette fonction
		const core::vector3df camPos = game->sceneManager->getActiveCamera() ? game->sceneManager->getActiveCamera()->getAbsolutePosition() : core::vector3df(-1.0f);
		if (!lastUpdateParams.hasDestroySimpleParamsChanged(game->system.getTime().getTotalJours(), game->mouseState.position, camPos))
			return;

		// Met � jour les param�tres pour le dernier appel � cette fonction
		lastUpdateParams.systemTotalDays = game->system.getTime().getTotalJours();
		lastUpdateParams.mousePos = game->mouseState.position;
		lastUpdateParams.camPos = camPos;

		// R�initialise la GUI d�e � la pr�visualisation des batiments
		resetPreviewErrors(moveBatiment);
	//}

	scene::ISceneNode* node = NULL;

	// V�rifie que l'index fourni est bien valide
	if (index.X >= 0 && index.Y >= 0 && index.X < TAILLE_CARTE && index.Y < TAILLE_CARTE)
	{
		// Obtient le b�timent � l'index sp�cifi� puis son node associ�
		Batiment* batiment = game->system.carte[index.X][index.Y].batiment;
		if (batiment)
			node = batiment->getSceneNode();
	}
	else	// Si l'index du b�timent n'est pas valide, on obtient le b�timent point� avec la souris
	{
		node = getPointedBat();

		// Obtient l'index du batiment associ� � ce node
		const CBatimentSceneNode* batNode = Game::getBatimentNode(node);
		if (batNode && batNode->getAssociatedBatiment())
			index = batNode->getAssociatedBatiment()->getIndex();
		else
			return;
	}

	// V�rifie que le node sp�cifi� existe et que le node a bien chang� depuis le dernier appel, sinon on quitte
	if (!node || node == pointedToDestroyBatiment)
		return;

	// Remet le dernier batiment s�lectionn� pour la destruction � sa couleur normale et masque son texte (-> n'est plus fait dans resetPreviewErrors)
	if (pointedToDestroyBatiment)
	{
		Game::resetBatimentColor(pointedToDestroyBatiment);
		pointedToDestroyBatiment->setNomBatimentVisible(false);
		pointedToDestroyBatiment = NULL;
	}

	// Obtient le b�timent point� pour la destruction :
	pointedToDestroyBatiment = Game::getBatimentNode(node);

	// Change la couleur du b�timent vis� et affiche son nom
	if (pointedToDestroyBatiment)
	{
		Game::changeBatimentColor(pointedToDestroyBatiment, video::SColor(255, 255, 0, 0));	// Destruction : Rouge
		pointedToDestroyBatiment->setNomBatimentVisible(true);
	}

	// V�rifie qu'on peut d�truire ce batiment
	bool outRessources[Ressources::RI_COUNT];
	const int error = game->system.canDestroyBatiment(index, outRessources);

	// Affiche les messages � l'utilisateur indiquant pourquoi on n'a pas pu d�truire le b�timent
	showSimpleDestructionErrors(NULL, node, error, outRessources);
}
void BatimentSelector::PlacerBatiment::createSimpleBat(BatimentID batimentID, core::vector2di index)
{
	// Si la cam�ra ou le renderer ne sont pas valides, on quitte
	scene::ICameraSceneNode* const camera = game->sceneManager->getActiveCamera();
	if (!camera)
		return;

#ifdef USE_IRRKLANG	// Seulement utilis� pour IrrKlang pour le moment
	// D�termine si cette fonction a �t� appel�e par la cr�ation rectangulaire
	const bool calledByRectCreation = StaticBatimentInfos::canCreateBySelectionRect(batimentID);
#endif

	core::vector3df position;
	bool visible = true;

	// V�rifie que l'index fourni est bien valide
	if (index.X >= 0 && index.Y >= 0 && index.X < TAILLE_CARTE && index.Y < TAILLE_CARTE)
		position = EcoWorldSystem::getPositionFromIndex(index);	// Obtient la position du node suivant son index
	else
	{
		// Obtient la position du terrain point�e par la souris
		position = getPointedTerrainPos();

		// La souris est en dehors du terrain si : position == core::vector3df(-1.0f)
		if (position == core::vector3df(-1.0f))
			visible = false;
		else
		{
			visible = true;

			// Arrondi la position � TAILLE_OBJETS en X et en Z
			position = EcoWorldSystem::getRoundPosition(position);

			// Obtient l'index du b�timent
			index = EcoWorldSystem::getIndexFromPosition(position);
		}
	}

	// Si on ne peut placer le batiment, on quitte
	if (!visible)
		return;

	// D�termine si on peut placer ce b�timent
	bool outRessources[Ressources::RI_COUNT];
#ifndef _DEBUG
	const	// Le const d�sactiv� permet de d�sactiver l'erreur EBCE_NOT_ENOUGH_POPULATION en mode DEBUG
#endif
		int error = game->system.canCreateBatiment(batimentID, index, currentBatimentRotation, outRessources);

#ifdef _DEBUG
	// /!\ Attention /!\ 
	// En mode DEBUG uniquement : On d�sactive l'erreur EBCE_NOT_ENOUGH_POPULATION pour permettre le fonctionnement du code de d�bogage pour d�bloquer tous les b�timents
	if (game->unlockAllBatiments)
		error &= ~EBCE_NOT_ENOUGH_POPULATION;	// Passe le bit repr�sent� par EBCE_NOT_ENOUGH_POPULATION � 0 (~ : op�rateur NOT bit � bit)
#endif

	if (error != EBCE_AUCUNE && error != EBCE_ENERGIE)	// V�rifie que l'erreur n'est pas aussi d�e � un manque d'�nergie, qui n'est plus une erreur fatale
	{
		// On n'affiche pas l'erreur du manque de place si le b�timent peut �tre cr�� par glissement de la souris ou par rectangle de s�lection
		//if (calledByRectCreation)
		//	error &= ~EBCE_PLACE; // Passe le bit repr�sent� par EBCE_PLACE � 0 (~ : op�rateur NOT bit � bit)

		// Affiche les messages � l'utilisateur indiquant pourquoi on n'a pas pu placer le b�timent
		showSimpleCreationErrors(&position, NULL, error, outRessources);

#ifdef USE_IRRKLANG
		// Joue le son de la construction d'un b�timent refus�e si on n'a pas �t� appel� par la construction rectangulaire
		if (!calledByRectCreation)
			ikMgr.playGameSound(IrrKlangManager::EGSE_BATIMENT_CREATE_NEGATIVE);
#endif
	}
	else	// Le syst�me de jeu n'a retourn� aucune erreur : on peut donc ajouter ce b�timent
	{
#ifdef USE_RAKNET
		// Indique � RakNet qu'on veut cr�er un b�timent � cet emplacement, s'il est activ�
		if (rkMgr.isNetworkPlaying()) // && !calledByRectCreation	// V�rifie qu'on n'a pas �t� appel� par la destruction en rectangle, qui est elle-m�me charg�e d'envoyer l'ordres de destruction � RakNet
		{
			RakNetManager::RakNetGamePacket packet(RakNetManager::EGPT_BATIMENT_CONSTRUCTED);
			packet.gameConstructInfos.batimentID = batimentID;
			packet.gameConstructInfos.indexX = index.X;
			packet.gameConstructInfos.indexY = index.Y;
			packet.gameConstructInfos.rotation = currentBatimentRotation;

			// Pr�-calcule la dur�e de vie de ce b�timent pour pouvoir l'envoyer aux clients afin qu'elle soit synchronis�e
			const StaticBatimentInfos& staticBatInfos = StaticBatimentInfos::getInfos(batimentID);
			packet.gameConstructInfos.dureeVie = BatimentInfos::computeDureeVie(staticBatInfos.dureeVie, staticBatInfos.dureeVieVariation);

			rkMgr.sendPackets.push_back(packet);
		}
		else
#endif
		{
			// Ajoute le b�timent au syst�me de jeu (il sera aussi ajout� au renderer)
			game->system.addBatiment(batimentID, index, currentBatimentRotation, outRessources, &position);

#ifdef _DEBUG
			// En mode DEBUG : On v�rifie que le b�timent existe bien
			if (!game->system.carte[index.X][index.Y].batiment)
				LOG_DEBUG("Game::createSimpleBat : Le systeme de jeu n'a retourne aucune erreur lors de l'ajout du batiment, pourtant il n'existe pas sur la carte du systeme :" << endl
					<< "    game->system.carte[" << index.X << "][" << index.Y << "].batiment = " << game->system.carte[index.X][index.Y].batiment, ELL_WARNING);
#endif

#ifdef USE_IRRKLANG
			// Joue le son de la construction d'un b�timent accept�e si on n'a pas �t� appel� par la construction rectangulaire
			if (!calledByRectCreation)
				ikMgr.playGameSound(IrrKlangManager::EGSE_BATIMENT_CREATE_POSITIVE);
#endif
		}
	}
}
void BatimentSelector::PlacerBatiment::destroySimpleBat(core::vector2di index)
{
#ifdef USE_IRRKLANG	// Seulement utilis� pour IrrKlang pour le moment
	// D�termine si cette fonction a �t� appel�e par la destruction rectangulaire
	//const bool calledByRectDestruction = ;	// TODO : Destruction rectangulaire non impl�ment�e pour le moment
#endif

	scene::ISceneNode* node = NULL;

	// V�rifie que l'index fourni est bien valide
	if (index.X >= 0 && index.Y >= 0 && index.X < TAILLE_CARTE && index.Y < TAILLE_CARTE)
	{
		// Obtient le b�timent � l'index sp�cifi� puis son node associ�
		Batiment* batiment = game->system.carte[index.X][index.Y].batiment;
		if (batiment)
			node = batiment->getSceneNode();
	}
	else	// Si l'index du b�timent n'est pas valide, on obtient le b�timent point� avec la souris
	{
		node = getPointedBat();

		// Obtient l'index du batiment associ� � ce node
		CBatimentSceneNode* const batNode = Game::getBatimentNode(node);
		if (batNode && batNode->getAssociatedBatiment())
			index = batNode->getAssociatedBatiment()->getIndex();
		else
			return;
	}

	// V�rifie que le node sp�cifi� existe
	if (!node)
		return;

	// Remet le dernier batiment s�lectionn� pour la destruction � sa couleur normale et masque son texte (-> n'est plus fait dans resetPreviewErrors)
	if (pointedToDestroyBatiment)
	{
		Game::resetBatimentColor(pointedToDestroyBatiment);
		pointedToDestroyBatiment->setNomBatimentVisible(false);
		pointedToDestroyBatiment = NULL;
	}

	// Obtient le b�timent point� pour la destruction :
	pointedToDestroyBatiment = Game::getBatimentNode(node);

	// D�termine si on peut d�truire ce batiment
	bool outRessources[Ressources::RI_COUNT];
	const int error = game->system.canDestroyBatiment(index, outRessources);

	// S'il y a une erreur � d�truire ce batiment, on l'affiche (le b�timent n'a pas �t� d�truit)
	if (error != EBDE_AUCUNE)
	{
		// Affiche les messages � l'utilisateur indiquant pourquoi on n'a pas pu d�truire le b�timent
		const core::vector3df messagesPos = node->getAbsolutePosition();
		showSimpleDestructionErrors(&messagesPos, node, error, outRessources);

#ifdef USE_IRRKLANG
		// Joue le son de la destruction d'un b�timent refus�e si on n'a pas �t� appel� par la destruction rectangulaire
		//if (!calledByRectDestruction)
			ikMgr.playGameSound(IrrKlangManager::EGSE_BATIMENT_DESTROY_NEGATIVE);
#endif
	}
	else	// Le syst�me de jeu n'a retourn� aucune erreur : on peut donc supprimer ce b�timent
	{
#ifdef USE_RAKNET
		// Indique � RakNet qu'on veut supprimer ce b�timent, s'il est activ�
		if (rkMgr.isNetworkPlaying()) // && !calledByRectDestruction	// V�rifie qu'on n'a pas �t� appel� par la destruction en rectangle, qui est elle-m�me charg�e d'envoyer l'ordres de destruction � RakNet
		{
			RakNetManager::RakNetGamePacket packet(RakNetManager::EGPT_BATIMENT_DESTROYED);
			packet.gameDestroyInfos.indexX = index.X;
			packet.gameDestroyInfos.indexY = index.Y;
			rkMgr.sendPackets.push_back(packet);
		}
		else
#endif
		{
			// Supprime le b�timent du syst�me de jeu (il sera aussi supprim� du renderer)
			game->system.destroyBatiment(index, outRessources);

#ifdef _DEBUG
			// En mode DEBUG : On v�rifie que le b�timent n'existe plus
			if (!game->system.carte[index.X][index.Y].batiment)
				LOG_DEBUG("Game::destroySimpleBat : Le systeme de jeu n'a retourne aucune erreur lors de la destruction du batiment, pourtant existe toujours sur la carte du systeme :" << endl
					<< "    game->system.carte[" << index.X << "][" << index.Y << "].batiment = " << game->system.carte[index.X][index.Y].batiment, ELL_WARNING);
#endif

#ifdef USE_IRRKLANG
			// Joue le son de la destruction d'un b�timent accept�e si on n'a pas �t� appel� par la destruction rectangulaire
			//if (!calledByRectDestruction)
				ikMgr.playGameSound(IrrKlangManager::EGSE_BATIMENT_DESTROY_POSITIVE);
#endif
		}
	}
}
#ifdef CAN_BUILD_WITH_SELECTION_RECT
void BatimentSelector::PlacerBatiment::previewCreateRectBats(BatimentID batimentID)
{
	// V�rifie que le rectangle de s�lection est bien valide et qu'on a acc�s au terrain, � son triangle selector et � la cam�ra
	scene::ICameraSceneNode* camera = game->sceneManager->getActiveCamera();
	if (!camera || !game->renderer || rectSelection == core::recti(-1, -1, -1, -1))
		return;
	// Si aucun param�tre n'a chang� depuis le dernier appel ou que le triangle selector du terrain n'est pas valide, on quitte aussi
	if (!game->renderer->getTerrainManager().getTerrainTriangleSelector()
		|| !lastUpdateParams.hasConstructRectParamsChanged(game->system.getTime().getTotalJours(), batimentID, currentBatimentRotation,
			game->mouseState.position, camera->getAbsolutePosition(), rectSelection))
		return;

	// Met � jour les param�tres pour le dernier appel � cette fonction
	const bool sameID = (lastUpdateParams.batimentID == batimentID);
	lastUpdateParams.mousePos = game->mouseState.position;
	lastUpdateParams.camPos = camera->getAbsolutePosition();
	lastUpdateParams.rectSelection = rectSelection;

	// Les 4 coins des indices correspondant au rectangle de s�lection actuel
	core::vector2di coin1(-9999, -9999),
#if !defined(FORCE_RECTANGLE) || !defined(ONLY_TAKE_MAIN_CORNERS)
		coin2(-9999, -9999), coin3(-9999, -9999),
#endif
		coin4(-9999, -9999);

	/*
	Organisation des coins :
		longueur X
		 <--->
		1 --- 2    /\
		 \     \   |  largeur Y (+ �cart Y)
		  \     \  |
		   3 --- 4 \/
		<->
		�cart X
	*/

	// Obtient les coins des indices
	{
		core::vector3df position;

		// Coin 1
		{
			// Obtient la position du terrain point�e par la souris
			position = getPointedTerrainPos(rectSelection.UpperLeftCorner);

			// La position est en dehors du terrain si : position == core::vector3df(-1.0f)
			if (position != core::vector3df(-1.0f))
			{
				// Obtient le premier indice suivant la position actuelle du point de collision
				coin1 = EcoWorldSystem::getIndexFromPosition(position);
			}
		}

#if !defined(FORCE_RECTANGLE) || !defined(ONLY_TAKE_MAIN_CORNERS)
		// Coin 2
		{
			// Obtient la position du terrain point�e par la souris
			position = getPointedTerrainPos(core::vector2di(rectSelection.LowerRightCorner.X, rectSelection.UpperLeftCorner.Y));

			// La position est en dehors du terrain si : position == core::vector3df(-1.0f)
			if (position != core::vector3df(-1.0f))
			{
				// Obtient le premier indice suivant la position actuelle du point de collision
				coin2 = EcoWorldSystem::getIndexFromPosition(position);
			}
		}

		// Coin 3
		{
			// Obtient la position du terrain point�e par la souris
			position = getPointedTerrainPos(core::vector2di(rectSelection.UpperLeftCorner.X, rectSelection.LowerRightCorner.Y));

			// La position est en dehors du terrain si : position == core::vector3df(-1.0f)
			if (position != core::vector3df(-1.0f))
			{
				// Obtient le premier indice suivant la position actuelle du point de collision
				coin3 = EcoWorldSystem::getIndexFromPosition(position);
			}
		}
#endif

		// Coin 4
		{
			// Obtient la position du terrain point�e par la souris
			position = getPointedTerrainPos(rectSelection.LowerRightCorner);

			// La position est en dehors du terrain si : position == core::vector3df(-1.0f)
			if (position != core::vector3df(-1.0f))
			{
				// Obtient le premier indice suivant la position actuelle du point de collision
				coin4 = EcoWorldSystem::getIndexFromPosition(position);
			}
		}
	}

	// V�rifie que tous les coins sont valides, sinon on quitte
	// On v�rifie aussi qu'au moins un des deux points est dans le terrain, pour �viter d'afficher des b�timents en-dehors du terrain.
	if ((coin1 == core::vector2di(-9999, -9999)
#if !defined(FORCE_RECTANGLE) || !defined(ONLY_TAKE_MAIN_CORNERS)
		|| coin2 == core::vector2di(-9999, -9999) || coin3 == core::vector2di(-9999, -9999)
#endif
		|| coin4 == core::vector2di(-9999, -9999))
		|| (IS_CORNER_OUT_OF_TERRAIN(coin1)
#if !defined(FORCE_RECTANGLE) || !defined(ONLY_TAKE_MAIN_CORNERS)
		&& IS_CORNER_OUT_OF_TERRAIN(coin2) && IS_CORNER_OUT_OF_TERRAIN(coin3)
#endif
		&& IS_CORNER_OUT_OF_TERRAIN(coin4)))
		return;

	// Tourne les coins pour qu'ils correspondent � leurs caract�ristiques
	{
#ifdef FORCE_RECTANGLE
#ifdef ONLY_TAKE_MAIN_CORNERS
		// Ne prend en compte que les coins 1 et 4, sinon on se retrouve g�n�ralement avec un rectangle des index trop grand
		// On force aussi les coins � se retrouver dans le terrain, pour �viter que certains b�timents se retrouvent en-dehors du terrain.
		const int minX = core::clamp(min(coin1.X, coin4.X), 0, TAILLE_CARTE);
		const int minY = core::clamp(min(coin1.Y, coin4.Y), 0, TAILLE_CARTE);
		const int maxX = core::clamp(max(coin1.X, coin4.X), 0, TAILLE_CARTE);
		const int maxY = core::clamp(max(coin1.Y, coin4.Y), 0, TAILLE_CARTE);
#else
		// Prend en compte les 4 coins du quadrilat�re des index
		// On force aussi les coins � se retrouver dans le terrain, pour �viter que certains b�timents se retrouvent en-dehors du terrain.
		const int minX = core::clamp(core::min_(min(coin1.X, coin2.X), coin3.X, coin4.X), 0, TAILLE_CARTE);
		const int minY = core::clamp(core::min_(min(coin1.Y, coin2.Y), coin3.Y, coin4.Y), 0, TAILLE_CARTE);
		const int maxX = core::clamp(core::max_(max(coin1.X, coin2.X), coin3.X, coin4.X), 0, TAILLE_CARTE);
		const int maxY = core::clamp(core::max_(max(coin1.Y, coin2.Y), coin3.Y, coin4.Y), 0, TAILLE_CARTE);
#endif

		// Force la zone s�lectionn�e � �tre un rectangle en for�ant chaque coin suivant les valeurs min et max en X et en Y

		// Le coin 1 est celui qui a la plus faible valeur en X et en Y
		coin1.set(minX, minY);

#ifndef ONLY_TAKE_MAIN_CORNERS
		// Le coin 2 est celui qui a la plus grande valeur en X et la plus faible valeur en Y
		coin2.set(maxX, minY);

		// Le coin 3 est celui qui a la plus faible valeur en X et la plus grande valeur en Y
		coin3.set(minX, maxY);
#endif

		// Le coin 4 est celui qui a la plus grande valeur en X et en Y
		coin4.set(maxX, maxY);
#else
		// Prend en compte les 4 coins du quadrilat�re des index
		const int minX = core::min_(min(coin1.X, coin2.X), coin3.X, coin4.X);
		const int minY = core::min_(min(coin1.Y, coin2.Y), coin3.Y, coin4.Y);
		const int maxX = core::max_(max(coin1.X, coin2.X), coin3.X, coin4.X);
		const int maxY = core::max_(max(coin1.Y, coin2.Y), coin3.Y, coin4.Y);

		// Tourne les coins pour qu'ils correspondent � leurs caract�ristiques (ne fonctionne que si la zone s�lectionn�e est d�j� un rectangle !)
		core::vector2di tmpCoin;

		// Le coin 1 est celui qui a la plus faible valeur en X et en Y
		{
			tmpCoin.set(minX, minY);

			// Recherche le coin qui correspond au coin temporaire
			if (coin2 == tmpCoin)
			{
				// Inverse les coins 1 et 2
				coin2 = coin1;
				coin1 = tmpCoin;
			}
			else if (coin3 == tmpCoin)
			{
				// Inverse les coins 1 et 3
				coin3 = coin1;
				coin1 = tmpCoin;
			}
			else if (coin4 == tmpCoin)
			{
				// Inverse les coins 1 et 4
				coin4 = coin1;
				coin1 = tmpCoin;
			}
#ifdef _DEBUG
			else if (coin1 == tmpCoin) // Le coin 1 est d�j� le bon, on ne fait rien
			{ }
			else
				LOG_DEBUG("Game::previewCreateRectBats : Impossible de determiner le coin 1 : Aucun coin ne correspond aux conditions requises", ELL_WARNING);
#endif
		}

		// Le coin 2 est celui qui a la plus grande valeur en X et la plus faible valeur en Y
		{
			tmpCoin.set(maxX, minY);

			// Recherche le coin qui correspond au coin temporaire
			if (coin1 == tmpCoin)
			{
				// Inverse les coins 2 et 1
				coin1 = coin2;
				coin2 = tmpCoin;
			}
			else if (coin3 == tmpCoin)
			{
				// Inverse les coins 2 et 3
				coin3 = coin2;
				coin2 = tmpCoin;
			}
			else if (coin4 == tmpCoin)
			{
				// Inverse les coins 2 et 4
				coin4 = coin2;
				coin2 = tmpCoin;
			}
#ifdef _DEBUG
			else if (coin2 == tmpCoin) // Le coin 2 est d�j� le bon, on ne fait rien
			{ }
			else
				LOG_DEBUG("Game::previewCreateRectBats : Impossible de determiner le coin 2 : Aucun coin ne correspond aux conditions requises", ELL_WARNING);
#endif
		}

		// Le coin 3 est celui qui a la plus faible valeur en X et la plus grande valeur en Y
		{
			tmpCoin.set(minX, maxY);

			// Recherche le coin qui correspond au coin temporaire
			if (coin1 == tmpCoin)
			{
				// Inverse les coins 3 et 1
				coin1 = coin3;
				coin3 = tmpCoin;
			}
			else if (coin2 == tmpCoin)
			{
				// Inverse les coins 3 et 2
				coin2 = coin3;
				coin3 = tmpCoin;
			}
			else if (coin4 == tmpCoin)
			{
				// Inverse les coins 3 et 4
				coin4 = coin3;
				coin3 = tmpCoin;
			}
#ifdef _DEBUG
			else if (coin3 == tmpCoin) // Le coin 3 est d�j� le bon, on ne fait rien
			{ }
			else
				LOG_DEBUG("Game::previewCreateRectBats : Impossible de determiner le coin 3 : Aucun coin ne correspond aux conditions requises", ELL_WARNING);
#endif
		}

		// Le coin 4 est celui qui a la plus grande valeur en X et en Y
		{
			tmpCoin.set(maxX, maxY);

			// Recherche le coin qui correspond au coin temporaire
			if (coin1 == tmpCoin)
			{
				// Inverse les coins 4 et 1
				coin1 = coin4;
				coin4 = tmpCoin;
			}
			else if (coin2 == tmpCoin)
			{
				// Inverse les coins 4 et 2
				coin2 = coin4;
				coin4 = tmpCoin;
			}
			else if (coin3 == tmpCoin)
			{
				// Inverse les coins 4 et 3
				coin3 = coin4;
				coin4 = tmpCoin;
			}
#ifdef _DEBUG
			else if (coin4 == tmpCoin) // Le coin 4 est d�j� le bon, on ne fait rien
			{ }
			else
				LOG_DEBUG("Game::previewCreateRectBats : Impossible de determiner le coin 4 : Aucun coin ne correspond aux conditions requises", ELL_WARNING);
#endif
		}
#endif
	}

	// En mode cr�ation rectangulaire, on force les rotations des b�timents � �tre des multiples de 90�
	currentBatimentRotation = core::round_((currentBatimentRotation / 90.0f) - 0.05f) * 90.0f;	// -0.05f � l'arrondi pour qu'un angle de 45� (et jusqu'� 49.5�) soit arrondi � 0� et non pas � 90�

	// V�rifie que les coins ou d'autres param�tres ont bien chang� depuis le dernier appel
	if (!lastUpdateParams.hasConstructRectCornersChanged(game->system.getTime().getTotalJours(), batimentID, currentBatimentRotation, coin1,
#if !defined(FORCE_RECTANGLE) || !defined(ONLY_TAKE_MAIN_CORNERS)
		coin2, coin3,
#endif
		coin4))
		return;

	// Stoppe le timer pour �viter que le temps de jeu ne se d�roule pendant le placement de ces b�timents
	const bool wasTimerStopped = !(game->deviceTimer->isStopped());
	if (wasTimerStopped)
		game->deviceTimer->stop();

	// Met � jour les coins et d'autres param�tres pour le dernier appel � cette fonction
	lastUpdateParams.systemTotalDays = game->system.getTime().getTotalJours();
	lastUpdateParams.batimentID = batimentID;
	lastUpdateParams.batimentRotation = currentBatimentRotation;
	lastUpdateParams.pointedFirstCorner = coin1;
#if !defined(FORCE_RECTANGLE) || !defined(ONLY_TAKE_MAIN_CORNERS)
	lastUpdateParams.pointedSecondCorner = coin2;
	lastUpdateParams.pointedThirdCorner = coin3;
#endif
	lastUpdateParams.pointedLastCorner = coin4;

	// R�initialise la GUI d�e � la pr�visualisation des batiments
	resetPreviewErrors(moveBatiment);

	// Variable d'it�ration utilis�e pour parcourir la liste des batiments de pr�visualisation d�j� cr��s
	u32 i = 0;
	const u32 previewRectSize = listeBatimentsPreviewRect.size();
	if (!sameID)
	{
		// D�truit tous les batiments de pr�visualisation pr�cedemment cr��s si leur ID est diff�rent de l'ID des batiments � cr�er
		for (i = 0; i < previewRectSize; ++i)
		{
			scene::ISceneNode* const batiment = listeBatimentsPreviewRect[i];
			batiment->setVisible(false);
			game->sceneManager->addToDeletionQueue(batiment);

			// Si PostProcess est activ�, on lui indique qu'il ne doit plus prendre en compte ce b�timent dans le flou de profondeur
			if (game->postProcessManager)
				game->postProcessManager->removeNodeFromDepthPass(batiment);

			// Si XEffects est activ�, on lui indique qu'il ne doit plus afficher l'ombre de ce b�timent
			if (game->xEffects)
				game->xEffects->removeShadowFromNode(batiment);
		}
		listeBatimentsPreviewRect.clear();
	}
	else
	{
		// Remet tous les batiments de pr�visualisation � leur couleur normale et les masque
		for (i = 0; i < previewRectSize; ++i)
		{
			scene::ISceneNode* const batiment = listeBatimentsPreviewRect[i];
			Game::resetBatimentColor(batiment);
			batiment->setVisible(false);
		}
	}

	// Obtient la taille du b�timent :
	// Si le b�timent n'est pas tourn� d'un multiple de 180� (dans tous les cas, la rotation des b�timents a �t� forc�e comme multiple de 90�),
	// alors il est perpendiculaire � l'axe des abscisses, et on inverse donc ses informations de taille :
	const core::dimension2du& batimentRealSize = StaticBatimentInfos::getInfos(batimentID).taille;
	const bool swapSize = core::equals(fmodf(currentBatimentRotation, 180.0f), 90.0f);	// Si currentBatimentRotation % 180.0f == 90.0f
	const core::dimension2du batimentTaille(swapSize ? batimentRealSize.Height : batimentRealSize.Width, swapSize ? batimentRealSize.Width : batimentRealSize.Height);

#ifndef FORCE_RECTANGLE	// TODO : A r�optimiser si utilis� !
	const int longueurX = coin2.X - coin1.X;
	const int largeurY = coin3.Y - coin1.Y;
	const int ecartX = coin4.X - coin2.X;
	const int ecartY = coin4.Y - coin3.Y;

	// Compte le nombre de b�timents � cr�er
	// TODO : Trouver une fonction plus efficace que l'incr�mentation � l'int�rieur des boucles
	nbBatimentsPreviewRect = 0;

	float ecartXParUniteY = (float)ecartX / (float)largeurY;
	float ecartYParUniteX = (float)ecartY / (float)longueurX;

	// Affiche les batiments contenus entre les 4 coins du parall�logramme de s�lection
	bool longueurNegative = (longueurX < 0);
	bool largeurNegative = (largeurY < 0);
	i = 0;
	for (int x = 0; abs(x) <= abs(longueurX); longueurNegative ? x -= batimentTaille.Width : x += batimentTaille.Width)
	{
		for (int y = 0; abs(y) <= abs(largeurY); largeurNegative ? y -= batimentTaille.Height : y += batimentTaille.Height)
		{
			// Le batiment � placer
			scene::ISceneNode* bat = NULL;

			// Si des batiments sont encore disponibles, on les utilise pour la pr�visualisation actuelle
			const u32 previewRectSizeUpdated = listeBatimentsPreviewRect.size();
			if (i < previewRectSizeUpdated)
			{
				bat = listeBatimentsPreviewRect[i];
				++i;
			}
			else
			{
				// Cr�e un nouveau batiment de ce type
				bat = game->renderer->loadBatiment(batimentID);
				listeBatimentsPreviewRect.push_back(bat);
				i = previewRectSizeUpdated + 1;		// Evite de r�utiliser ce batiment pour le placement
			}

			if (bat)
			{
				bat->setVisible(true);

				// Place ce batiment � sa position pour sa pr�visualisation
				previewCreateSimpleBat(batimentID, bat, core::vector2di(
					x + coin1.X + core::round32(ecartXParUniteY * (float)y),
					y + coin1.Y + core::round32(ecartYParUniteX * (float)x)), false);
			}

			++nbBatimentsPreviewRect;	// Incr�mente le nombre de b�timents � cr�er
		}
	}
#else
	const int longueurX = coin4.X - coin1.X;	// longueurX est toujours positif, car coin1.X = min(coin1.X, coin4.X) et coin4.X = max(coin1.X, coin4.X)
	const int largeurY = coin4.Y - coin1.Y;		// largeurY est toujours positif,  car coin1.Y = min(coin1.Y, coin4.Y) et coin4.Y = max(coin1.Y, coin4.Y)

	// Compte le nombre de b�timents � cr�er
	nbBatimentsPreviewRect = ((((u32)longueurX) / batimentTaille.Width) + 1) * ((((u32)largeurY) / batimentTaille.Height) + 1);

	// V�rifie que le nombre de b�timents � cr�er n'est pas trop grand, sinon on ne les cr�e pas
	if (nbBatimentsPreviewRect > 200)
	{
		// Red�marre le timer du jeu
		if (wasTimerStopped)
			game->deviceTimer->start();

		LOG_DEBUG("Game::previewCreateRectBats : Trop de batiments en rectangle a placer :" << endl
			<< "                                 nbBatimentsPreviewRect = " << nbBatimentsPreviewRect << "     => Abandon", ELL_WARNING);
		return;
	}

	// V�rifie que la liste listeBatimentsPreviewRect est assez grande, sinon on lui indique quelle place est n�cessaire
	if (listeBatimentsPreviewRect.allocated_size() < nbBatimentsPreviewRect)
		listeBatimentsPreviewRect.reallocate(nbBatimentsPreviewRect);

	// Affiche les batiments contenus entre les 4 coins du rectangle de s�lection
	i = 0;
	for (int x = 0; x <= longueurX; x += batimentTaille.Width)
	{
		for (int y = 0; y <= largeurY; y += batimentTaille.Height)
		{
			// Le batiment � placer
			scene::ISceneNode* bat = NULL;

			// Si des batiments sont encore disponibles, on les utilise pour la pr�visualisation actuelle
			const u32 previewRectSizeUpdated = listeBatimentsPreviewRect.size();
			if (i < previewRectSizeUpdated)
			{
				bat = listeBatimentsPreviewRect[i];
				++i;
			}
			else
			{
				// Cr�e un nouveau batiment de ce type
				bat = game->renderer->loadBatiment(batimentID, NULL, false, true);
				listeBatimentsPreviewRect.push_back(bat);
				i = previewRectSizeUpdated + 1;		// Evite de r�utiliser ce batiment pour le placement (on en cr�era un nouveau � la nouvelle it�ration)
			}

			if (bat)
			{
				bat->setVisible(true);

				// Place ce nouveau batiment � sa position pour sa pr�visualisation
				previewCreateSimpleBat(batimentID, bat, core::vector2di(x + coin1.X, y + coin1.Y));
			}
		}
	}
#endif

	// D�truit tous les batiments de pr�visualisation non utilis�s (ils sont encore masqu�s)
	const u32 previewRectSizeUpdated = listeBatimentsPreviewRect.size();
	u32 firstIndex = previewRectSizeUpdated;
	for (i = 0; i < previewRectSizeUpdated; ++i)
	{
		// V�rifie si ce batiment est masqu�
		if (!listeBatimentsPreviewRect[i]->isVisible())
		{
			// S'il est masqu�, on le supprime et on note l'index du premier �lement masqu�
			scene::ISceneNode* const batiment = listeBatimentsPreviewRect[i];
			game->sceneManager->addToDeletionQueue(batiment);
			firstIndex = min(firstIndex, i);

			// Si PostProcess est activ�, on lui indique qu'il ne doit plus prendre en compte ce b�timent dans le flou de profondeur
			if (game->postProcessManager)
				game->postProcessManager->removeNodeFromDepthPass(batiment);

			// Si XEffects est activ�, on lui indique qu'il ne doit plus afficher l'ombre de ce b�timent
			if (game->xEffects)
				game->xEffects->removeShadowFromNode(batiment);
		}
	}

	// Supprime tous les �lements masqu�s de la liste (ils le sont tous � partir du rang firstIndex)
	if (firstIndex < previewRectSizeUpdated)
		listeBatimentsPreviewRect.erase(firstIndex, previewRectSizeUpdated - firstIndex);

	// V�rifie les erreurs de cr�ation d�es au nombre multiple de b�timents
	bool outRessources[Ressources::RI_COUNT];
	const int error = game->system.canCreateMultiBatiments(batimentID, nbBatimentsPreviewRect, outRessources);

	// Affiche les messages � l'utilisateur indiquant pourquoi on n'a pas pu placer tous les b�timents
	showRectCreationErrors(NULL, &listeBatimentsPreviewRect, error, outRessources);

	// Red�marre le timer du jeu
	if (wasTimerStopped)
		game->deviceTimer->start();
}
void BatimentSelector::PlacerBatiment::createRectBats(BatimentID batimentID)
{
	// V�rifie que le rectangle de s�lection est bien valide, qu'on a acc�s au terrain, � son triangle selector et � la cam�ra
	if (rectSelection == core::recti(-1, -1, -1, -1) || !game->renderer)
		return;
	scene::ICameraSceneNode* const camera = game->sceneManager->getActiveCamera();
	if (!game->renderer->getTerrainManager().getTerrainTriangleSelector() || !camera)
		return;

	// Les 4 coins des indices correspondant au rectangle de s�lection actuel
	core::vector2di coin1,
#if !defined(FORCE_RECTANGLE) || !defined(ONLY_TAKE_MAIN_CORNERS)
		coin2, coin3,
#endif
		coin4;

	/*
	Organisation des coins :
		longueur X
		 <--->
		1 --- 2    /\
		 \     \   |  largeur Y (+ �cart Y)
		  \     \  |
		   3 --- 4 \/
		<->
		�cart X
	*/

	// Obtient les coins des indices
	{
		core::vector3df position;

		// Coin 1
		{
			// Obtient la position du terrain point�e par la souris
			position = getPointedTerrainPos(rectSelection.UpperLeftCorner);

			// La position est en dehors du terrain si : position == core::vector3df(-1.0f)
			if (position != core::vector3df(-1.0f))
			{
				// Obtient le premier indice suivant la position actuelle du point de collision
				coin1 = EcoWorldSystem::getIndexFromPosition(position);
			}
		}

#if !defined(FORCE_RECTANGLE) || !defined(ONLY_TAKE_MAIN_CORNERS)
		// Coin 2
		{
			// Obtient la position du terrain point�e par la souris
			position = getPointedTerrainPos(core::vector2di(rectSelection.LowerRightCorner.X, rectSelection.UpperLeftCorner.Y));

			// La position est en dehors du terrain si : position == core::vector3df(-1.0f)
			if (position != core::vector3df(-1.0f))
			{
				// Obtient le premier indice suivant la position actuelle du point de collision
				coin2 = EcoWorldSystem::getIndexFromPosition(position);
			}
		}

		// Coin 3
		{
			// Obtient la position du terrain point�e par la souris
			position = getPointedTerrainPos(core::vector2di(rectSelection.UpperLeftCorner.X, rectSelection.LowerRightCorner.Y));

			// La position est en dehors du terrain si : position == core::vector3df(-1.0f)
			if (position != core::vector3df(-1.0f))
			{
				// Obtient le premier indice suivant la position actuelle du point de collision
				coin3 = EcoWorldSystem::getIndexFromPosition(position);
			}
		}
#endif

		// Coin 4
		{
			// Obtient la position du terrain point�e par la souris
			position = getPointedTerrainPos(rectSelection.LowerRightCorner);

			// La position est en dehors du terrain si : position == core::vector3df(-1.0f)
			if (position != core::vector3df(-1.0f))
			{
				// Obtient le premier indice suivant la position actuelle du point de collision
				coin4 = EcoWorldSystem::getIndexFromPosition(position);
			}
		}
	}

	// V�rifie que tous les coins sont valides, sinon on quitte
	// On v�rifie aussi qu'au moins un des deux points est dans le terrain, pour �viter d'afficher des b�timents en-dehors du terrain.
	if ((coin1 == core::vector2di(-9999, -9999)
#if !defined(FORCE_RECTANGLE) || !defined(ONLY_TAKE_MAIN_CORNERS)
		|| coin2 == core::vector2di(-9999, -9999) || coin3 == core::vector2di(-9999, -9999)
#endif
		|| coin4 == core::vector2di(-9999, -9999))
		|| (IS_CORNER_OUT_OF_TERRAIN(coin1)
#if !defined(FORCE_RECTANGLE) || !defined(ONLY_TAKE_MAIN_CORNERS)
		&& IS_CORNER_OUT_OF_TERRAIN(coin2) && IS_CORNER_OUT_OF_TERRAIN(coin3)
#endif
		&& IS_CORNER_OUT_OF_TERRAIN(coin4)))
		return;

	// Stoppe le timer pour �viter que le temps de jeu ne se d�roule pendant le placement de ces b�timents
	const bool wasTimerStopped = !(game->deviceTimer->isStopped());
	if (wasTimerStopped)
		game->deviceTimer->stop();

	// Tourne les coins pour qu'ils correspondent � leurs caract�ristiques
	{
#ifdef FORCE_RECTANGLE
#ifdef ONLY_TAKE_MAIN_CORNERS
		// Ne prend en compte que les coins 1 et 4, sinon on se retrouve g�n�ralement avec un rectangle des index trop grand
		// On force aussi les coins � se retrouver dans le terrain, pour �viter que certains b�timents se retrouvent en-dehors du terrain.
		const int minX = core::clamp(min(coin1.X, coin4.X), 0, TAILLE_CARTE);
		const int minY = core::clamp(min(coin1.Y, coin4.Y), 0, TAILLE_CARTE);
		const int maxX = core::clamp(max(coin1.X, coin4.X), 0, TAILLE_CARTE);
		const int maxY = core::clamp(max(coin1.Y, coin4.Y), 0, TAILLE_CARTE);
#else
		// Prend en compte les 4 coins du quadrilat�re des index
		// On force aussi les coins � se retrouver dans le terrain, pour �viter que certains b�timents se retrouvent en-dehors du terrain.
		const int minX = core::clamp(core::min_(min(coin1.X, coin2.X), coin3.X, coin4.X), 0, TAILLE_CARTE);
		const int minY = core::clamp(core::min_(min(coin1.Y, coin2.Y), coin3.Y, coin4.Y), 0, TAILLE_CARTE);
		const int maxX = core::clamp(core::max_(max(coin1.X, coin2.X), coin3.X, coin4.X), 0, TAILLE_CARTE);
		const int maxY = core::clamp(core::max_(max(coin1.Y, coin2.Y), coin3.Y, coin4.Y), 0, TAILLE_CARTE);
#endif

		// Force la zone s�lectionn�e � �tre un rectangle en for�ant chaque coin suivant les valeurs min et max en X et en Y

		// Le coin 1 est celui qui a la plus faible valeur en X et en Y
		coin1.set(minX, minY);

#ifndef ONLY_TAKE_MAIN_CORNERS
		// Le coin 2 est celui qui a la plus grande valeur en X et la plus faible valeur en Y
		coin2.set(maxX, minY);

		// Le coin 3 est celui qui a la plus faible valeur en X et la plus grande valeur en Y
		coin3.set(minX, maxY);
#endif

		// Le coin 4 est celui qui a la plus grande valeur en X et en Y
		coin4.set(maxX, maxY);
#else
		// Prend en compte les 4 coins du quadrilat�re des index
		const int minX = core::min_(min(coin1.X, coin2.X), coin3.X, coin4.X);
		const int minY = core::min_(min(coin1.Y, coin2.Y), coin3.Y, coin4.Y);
		const int maxX = core::max_(max(coin1.X, coin2.X), coin3.X, coin4.X);
		const int maxY = core::max_(max(coin1.Y, coin2.Y), coin3.Y, coin4.Y);

		// Tourne les coins pour qu'ils correspondent � leurs caract�ristiques (ne fonctionne que si la zone s�lectionn�e est d�j� un rectangle !)
		core::vector2di tmpCoin;

		// Le coin 1 est celui qui a la plus faible valeur en X et en Y
		{
			tmpCoin.set(minX, minY);

			// Recherche le coin qui correspond au coin temporaire
			if (coin2 == tmpCoin)
			{
				// Inverse les coins 1 et 2
				coin2 = coin1;
				coin1 = tmpCoin;
			}
			else if (coin3 == tmpCoin)
			{
				// Inverse les coins 1 et 3
				coin3 = coin1;
				coin1 = tmpCoin;
			}
			else if (coin4 == tmpCoin)
			{
				// Inverse les coins 1 et 4
				coin4 = coin1;
				coin1 = tmpCoin;
			}
#ifdef _DEBUG
			else if (coin1 == tmpCoin) // Le coin 1 est d�j� le bon, on ne fait rien
			{ }
			else
				LOG_DEBUG("Game::createRectBats : Impossible de determiner le coin 1 : Aucun coin ne correspond aux conditions requises", ELL_WARNING);
#endif
		}

		// Le coin 2 est celui qui a la plus grande valeur en X et la plus faible valeur en Y
		{
			tmpCoin.set(maxX, minY);

			// Recherche le coin qui correspond au coin temporaire
			if (coin1 == tmpCoin)
			{
				// Inverse les coins 2 et 1
				coin1 = coin2;
				coin2 = tmpCoin;
			}
			else if (coin3 == tmpCoin)
			{
				// Inverse les coins 2 et 3
				coin3 = coin2;
				coin2 = tmpCoin;
			}
			else if (coin4 == tmpCoin)
			{
				// Inverse les coins 2 et 4
				coin4 = coin2;
				coin2 = tmpCoin;
			}
#ifdef _DEBUG
			else if (coin2 == tmpCoin) // Le coin 2 est d�j� le bon, on ne fait rien
			{ }
			else
				LOG_DEBUG("Game::createRectBats : Impossible de determiner le coin 2 : Aucun coin ne correspond aux conditions requises", ELL_WARNING);
#endif
		}

		// Le coin 3 est celui qui a la plus faible valeur en X et la plus grande valeur en Y
		{
			tmpCoin.set(minX, maxY);

			// Recherche le coin qui correspond au coin temporaire
			if (coin1 == tmpCoin)
			{
				// Inverse les coins 3 et 1
				coin1 = coin3;
				coin3 = tmpCoin;
			}
			else if (coin2 == tmpCoin)
			{
				// Inverse les coins 3 et 2
				coin2 = coin3;
				coin3 = tmpCoin;
			}
			else if (coin4 == tmpCoin)
			{
				// Inverse les coins 3 et 4
				coin4 = coin3;
				coin3 = tmpCoin;
			}
#ifdef _DEBUG
			else if (coin3 == tmpCoin) // Le coin 3 est d�j� le bon, on ne fait rien
			{ }
			else
				LOG_DEBUG("Game::createRectBats : Impossible de determiner le coin 3 : Aucun coin ne correspond aux conditions requises", ELL_WARNING);
#endif
		}

		// Le coin 4 est celui qui a la plus grande valeur en X et en Y
		{
			tmpCoin.set(maxX, maxY);

			// Recherche le coin qui correspond au coin temporaire
			if (coin1 == tmpCoin)
			{
				// Inverse les coins 4 et 1
				coin1 = coin4;
				coin4 = tmpCoin;
			}
			else if (coin2 == tmpCoin)
			{
				// Inverse les coins 4 et 2
				coin2 = coin4;
				coin4 = tmpCoin;
			}
			else if (coin3 == tmpCoin)
			{
				// Inverse les coins 4 et 3
				coin3 = coin4;
				coin4 = tmpCoin;
			}
#ifdef _DEBUG
			else if (coin4 == tmpCoin) // Le coin 4 est d�j� le bon, on ne fait rien
			{ }
			else
				LOG_DEBUG("Game::createRectBats : Impossible de determiner le coin 4 : Aucun coin ne correspond aux conditions requises", ELL_WARNING);
#endif
		}
#endif
	}

	// En mode cr�ation rectangulaire, on force les rotations des b�timents � �tre des multiples de 90�
	currentBatimentRotation = core::round_((currentBatimentRotation / 90.0f) - 0.05f) * 90.0f;	// -0.05f � l'arrondi pour qu'un angle de 45� (et jusqu'� 49.5�) soit arrondi � 0� et non pas � 90�

	// Obtient la taille du b�timent :
	// Si le b�timent n'est pas tourn� d'un multiple de 180� (dans tous les cas, la rotation des b�timents a �t� forc�e comme multiple de 90�),
	// alors il est perpendiculaire � l'axe des abscisses, et on inverse donc ses informations de taille :
	const core::dimension2du& batimentRealSize = StaticBatimentInfos::getInfos(batimentID).taille;
	const bool swapSize = core::equals(fmodf(currentBatimentRotation, 180.0f), 90.0f);	// Si currentBatimentRotation % 180.0f == 90.0f
	const core::dimension2du batimentTaille(swapSize ? batimentRealSize.Height : batimentRealSize.Width, swapSize ? batimentRealSize.Width : batimentRealSize.Height);

	// Compte le nombre de b�timents � cr�er :
#ifndef FORCE_RECTANGLE	// TODO : A r�optimiser si utilis� !
	const int longueurX = coin2.X - coin1.X;
	const int largeurY = coin3.Y - coin1.Y;

	// Compte le nombre de b�timents � cr�er
	// TODO : Trouver une fonction plus efficace
	nbBatimentsPreviewRect = 0;
	for (int x = 0; abs(x) <= abs(longueurX); longueurNegative ? x -= batimentTaille.Width : x += batimentTaille.Width)
		for (int y = 0; abs(y) <= abs(largeurY); largeurNegative ? y -= batimentTaille.Height : y += batimentTaille.Height)
			++nbBatimentsPreviewRect;
#else
	const int longueurX = coin4.X - coin1.X;	// longueurX est toujours positif, car coin1.X = min(coin1.X, coin4.X) et coin4.X = max(coin1.X, coin4.X)
	const int largeurY = coin4.Y - coin1.Y;		// largeurY est toujours positif,  car coin1.Y = min(coin1.Y, coin4.Y) et coin4.Y = max(coin1.Y, coin4.Y)

	// Compte le nombre de b�timents plac�s dans ce rectangle
	nbBatimentsPreviewRect = ((((u32)longueurX) / batimentTaille.Width) + 1) * ((((u32)largeurY) / batimentTaille.Height) + 1);
#endif

	// V�rifie que le nombre de b�timents � cr�er n'est pas trop grand, sinon on ne les cr�e pas :
	// Evite ainsi de perdre la partie en cours en cas de manque de m�moire : 200 arbres consomment environ 30 Mo de m�moire vive !
	if (nbBatimentsPreviewRect > 200)
	{
		// Red�marre le timer du jeu
		if (wasTimerStopped)
			game->deviceTimer->start();

		// Affiche un message d'erreur � l'utilisateur
		swprintf_SS(L"Placement des b�timents annul� :\r\nLe nombre de b�timents � placer est trop grand : %u b�timents !", nbBatimentsPreviewRect);
		game->showErrorMessageBox(L"Avertissement", textW_SS);

		return;
	}

	// V�rifie les erreurs de cr�ation d�es au nombre multiple de b�timents
	bool outRessources[Ressources::RI_COUNT];
	const int error = game->system.canCreateMultiBatiments(batimentID, nbBatimentsPreviewRect, outRessources);

	// D�termine si cette erreur est une erreur fatale
	const bool fatalError = (error != EBMCE_AUCUNE && error != EBMCE_ENERGIE);

	// D�truit tous les batiments de pr�visualisation pr�cedemment cr��s
	const u32 previewRectSize = listeBatimentsPreviewRect.size();
	for (u32 i = 0; i < previewRectSize; ++i)
	{
		scene::ISceneNode* const batiment = listeBatimentsPreviewRect[i];
		batiment->setVisible(false);
		game->sceneManager->addToDeletionQueue(batiment);

		// Si PostProcess est activ�, on lui indique qu'il ne doit plus prendre en compte ce b�timent dans le flou de profondeur
		if (game->postProcessManager)
			game->postProcessManager->removeNodeFromDepthPass(batiment);

		// Si XEffects est activ�, on lui indique qu'il ne doit plus afficher l'ombre de ce b�timent
		if (game->xEffects)
			game->xEffects->removeShadowFromNode(batiment);
	}
	listeBatimentsPreviewRect.clear();

#ifndef FORCE_RECTANGLE	// A r�optimiser si utilis� !
	const int ecartX = coin4.X - coin2.X;
	const int ecartY = coin4.Y - coin3.Y;
	float ecartXParUniteY = (float)ecartX / (float)largeurY;
	float ecartYParUniteX = (float)ecartY / (float)longueurX;

	// Cr�e les batiments entre les 4 coins du parall�logramme de s�lection
	bool longueurNegative = (longueurX < 0);
	bool largeurNegative = (largeurY < 0);
	for (int x = 0; abs(x) <= abs(longueurX); longueurNegative ? x -= batimentTaille.Width : x += batimentTaille.Width)
	{
		for (int y = 0; abs(y) <= abs(largeurY); largeurNegative ? y -= batimentTaille.Height : y += batimentTaille.Height)
		{
			const core::vector2di index(
				x + coin1.X + core::round32(ecartXParUniteY * (float)y),
				y + coin1.Y + core::round32(ecartYParUniteX * (float)x));

			if (!fatalError)	// Si ce n'est pas une erreur fatale, on essaie de placer ce b�timent
				createSimpleBat(batimentID, index);
			else
			{
				// Sinon on affiche les erreurs sp�cifiques qu'aurait rencontr� le b�timent si on l'avait plac� ici :

				// Obtient les erreurs qu'aurait renvoy� le syst�me si on avait essay� de placer le b�timent ici
				int errorBat = game->system.canCreateBatiment(batimentID, index, currentBatimentRotation, NULL);

				// Evite d'afficher certaines erreurs g�r�es dans la pr�visualisation en rectangle :
				// Budget insuffisant, Energie insuffisante et Ressources insuffisantes
				errorBat &= ~(EBCE_BUDGET | EBCE_ENERGIE | EBCE_RESSOURCE); // Passe les bits repr�sent�s par EBCE_BUDGET, EBCE_ENERGIE et EBCE_RESSOURCE � 0 (~ : op�rateur NOT bit � bit)

				// Affiche les erreurs de cr�ation de ce b�timent :
				if (errorBat != EBCE_AUCUNE)
				{
					// Calcule la position de ce b�timent
					core::vector3df batPos = EcoWorldSystem::getPositionFromIndex(index);

					// Obtient la position du terrain en Y
					batPos.Y = game->renderer->getTerrainHeight(batPos.X, batPos.Z);

					// Interdit � la position de tomber en-dessous du niveau de l'eau si elle est visible
					if (game->renderer->getTerrainManager().getTerrainInfos().water.visible)
						batPos.Y = max(game->renderer->getTerrainManager().getTerrainInfos().water.height, batPos.Y);

					// Ajoute un peu de hauteur au b�timent pour �viter les effets d�sagr�ables avec le sol
					batPos.Y += HAUTEUR_AJOUTEE_BATIMENTS;

					showSimpleCreationErrors(&batPos, NULL, errorBat, NULL);
				}
			}
		}
	}
#else
	// Cr�e les batiments entre les 4 coins du rectangle de s�lection
	for (int x = 0; x <= longueurX; x += batimentTaille.Width)
	{
		for (int y = 0; y <= largeurY; y += batimentTaille.Height)
		{
			const core::vector2di index(x + coin1.X, y + coin1.Y);

			if (!fatalError)	// Si ce n'est pas une erreur fatale, on essaie de placer ce b�timent
				createSimpleBat(batimentID, index);
			else
			{
				// Sinon on affiche les erreurs sp�cifiques qu'aurait rencontr� le b�timent si on l'avait plac� ici :

				// Obtient les erreurs qu'aurait renvoy� le syst�me si on avait essay� de placer le b�timent ici
				int errorBat = game->system.canCreateBatiment(batimentID, index, currentBatimentRotation, NULL);

				// Evite d'afficher certaines erreurs g�r�es dans la pr�visualisation en rectangle :
				// Budget insuffisant, Energie insuffisante et Ressources insuffisantes
				errorBat &= ~(EBCE_BUDGET | EBCE_ENERGIE | EBCE_RESSOURCE); // Passe les bits repr�sent�s par EBCE_BUDGET, EBCE_ENERGIE et EBCE_RESSOURCE � 0 (~ : op�rateur NOT bit � bit)

				// Affiche les erreurs de cr�ation de ce b�timent :
				if (errorBat != EBCE_AUCUNE)
				{
					// Calcule la position de ce b�timent
					core::vector3df batPos = EcoWorldSystem::getPositionFromIndex(index);

					// Obtient la position du terrain en Y
					batPos.Y = game->renderer->getTerrainHeight(batPos.X, batPos.Z);

					// Interdit � la position de tomber en-dessous du niveau de l'eau si elle est visible
					if (game->renderer->getTerrainManager().getTerrainInfos().water.visible)
						batPos.Y = max(game->renderer->getTerrainManager().getTerrainInfos().water.height, batPos.Y);

					// Ajoute un peu de hauteur au b�timent pour �viter les effets d�sagr�ables avec le sol
					batPos.Y += HAUTEUR_AJOUTEE_BATIMENTS;

					showSimpleCreationErrors(&batPos, NULL, errorBat, NULL);
				}
			}
		}
	}
#endif

	// Affiche les messages � l'utilisateur indiquant pourquoi on n'a pas pu placer tous les b�timents
	if (error != EBMCE_AUCUNE)
	{
		// Calcule la position moyenne des b�timents :
		core::vector3df medBatimentPosition;

		// On calcule la position moyenne des b�timents aux 2 coins oppos�s du rectangle
		{
			core::vector3df batPos;

			// Coin 1 :
			{
				// Calcule la position du b�timent
				batPos = EcoWorldSystem::getPositionFromIndex(coin1);

				// Obtient la position du terrain en Y
				batPos.Y = game->renderer->getTerrainHeight(batPos.X, batPos.Z);

				// Interdit � la position de tomber en-dessous du niveau de l'eau si elle est visible
				if (game->renderer->getTerrainManager().getTerrainInfos().water.visible)
					batPos.Y = max(game->renderer->getTerrainManager().getTerrainInfos().water.height, batPos.Y);

				// Calcule la position moyenne des b�timents
				medBatimentPosition += batPos;
			}

			// Coin 4 :
			{
				// Calcule la position du b�timent
				batPos = EcoWorldSystem::getPositionFromIndex(coin4);

				// Obtient la position du terrain en Y
				batPos.Y = game->renderer->getTerrainHeight(batPos.X, batPos.Z);

				// Interdit � la position de tomber en-dessous du niveau de l'eau si elle est visible
				if (game->renderer->getTerrainManager().getTerrainInfos().water.visible)
					batPos.Y = max(game->renderer->getTerrainManager().getTerrainInfos().water.height, batPos.Y);

				// Calcule la position moyenne des b�timents
				medBatimentPosition += batPos;
			}

			medBatimentPosition *= 0.5f;						// Fait la moyenne des deux coins
			medBatimentPosition += HAUTEUR_AJOUTEE_BATIMENTS;	// D� � l'ajout de hauteur aux b�timents pour �viter les effets d�sagr�ables avec le sol
		}

		// Affiche les erreurs de cr�ation des b�timents : les messages seront cr��s � cette position moyenne
		showRectCreationErrors(&medBatimentPosition, NULL, error, outRessources);
	}

#ifdef USE_IRRKLANG
	// Joue le son de la cr�ation d'un b�timent accept�e ou refus�e
	if (!fatalError)
		ikMgr.playGameSound(IrrKlangManager::EGSE_BATIMENT_CREATE_POSITIVE);
	else
		ikMgr.playGameSound(IrrKlangManager::EGSE_BATIMENT_CREATE_NEGATIVE);
#endif

	// Red�marre le timer
	if (wasTimerStopped)
		game->deviceTimer->start();
}
void BatimentSelector::PlacerBatiment::showRectCreationErrors(const core::vector3df* messagesPosition, core::array<scene::ISceneNode*>* batiments, int error, bool outRessources[])
{
	if (error == EBMCE_AUCUNE)
		return;

	// TODO : Utiliser la sortie "outRessources" pour savoir quelles ressources sont manquantes

	// Obtient la transparence par d�faut des textes
	const u32 textTransparency = game->guiManager->getTextTransparency();

#ifdef _DEBUG
	// V�rifie que l'erreur peut �tre trait�e ici, sinon, on affiche un message
	if (!( ((error & EBMCE_BUDGET) != 0)
		|| ((error & EBMCE_ENERGIE) != 0)
		|| ((error & EBMCE_BATIMENT_ID) != 0)
		|| ((error & EBMCE_RESSOURCE) != 0)))
		LOG_DEBUG("Game::showRectCreationErrors : Impossible d'ajouter les batiments au systeme de jeu : Erreur inconnue : error = " << error, ELL_WARNING);
#endif

	if (((error & EBMCE_RESSOURCE) != 0) && batiments)
	{
		const u32 size = batiments->size();
		for (u32 i = 0; i < size; ++i)
		{
			// On n'a pas les ressources suffisantes :
			// On rend tous les batiments rouges
			game->changeBatimentColor((*batiments)[i], video::SColor(255, 255, 0, 0));	// Construction impossible : Rouge
		}
	}
	if (game->guiManager->guiElements.gameGUI.budgetTexte)
	{
		if ((error & EBMCE_BUDGET) != 0)
		{
			// Le budget est insuffisant : Affiche son texte en rouge
			game->guiManager->guiElements.gameGUI.budgetTexte->setOverrideColor(video::SColor(textTransparency, 255, 50, 50));
		}
		else
		{
			// Le budget est suffisant : Affiche son texte en blanc
			game->guiManager->guiElements.gameGUI.budgetTexte->setOverrideColor(video::SColor(textTransparency, 255, 255, 255));
		}
	}
	if (game->guiManager->guiElements.gameGUI.energieTexte)
	{
		if ((error & EBMCE_ENERGIE) != 0)
		{
			// L'�nergie est insuffisante : Affiche son texte en rouge
			game->guiManager->guiElements.gameGUI.energieTexte->setOverrideColor(video::SColor(textTransparency, 255, 50, 50));
		}
		else
		{
			// L'�nergie est suffisante : Affiche son texte en blanc
			game->guiManager->guiElements.gameGUI.energieTexte->setOverrideColor(video::SColor(textTransparency, 255, 255, 255));
		}
	}
#ifdef _DEBUG
	if (((error & EBMCE_BATIMENT_ID) != 0))
		LOG_DEBUG("Game::showRectCreationErrors : Impossible d'ajouter le batiment au systeme de jeu car l'ID du batiment n'est pas valide : error = " << error, ELL_WARNING);
#endif

	// Affiche les messages � l'utilisateur indiquant pourquoi on n'a pas pu placer le b�timent
	if (messagesPosition)
	{
		core::stringw text = "";
		if ((error & EBMCE_BUDGET) != 0)
			text.append(L"Budget insuffisant\n");
		//if ((error & EBMCE_ENERGIE) != 0)				// Le manque d'�nergie n'est plus une erreur fatale pour pouvoir placer des b�timents : on se contente d'afficher son texte en rouge dans ce cas
		//	text.append(L"Energie insuffisante\n");
		if ((error & EBMCE_RESSOURCE) != 0)
			text.append(L"Ressources insuffisantes\n");

		if (text.size() > 1)
		{
			core::vector3df newPos(*messagesPosition);

			if (batiments)
				newPos.Y += ((*batiments)[0]->getTransformedBoundingBox().MaxEdge.Y * 0.5f) + 5.0f;
			else
				newPos.Y += 40.0f;

			core::vector3df endPos(newPos);
			endPos.Y += 15.0f;

			scene::ITextSceneNode* textNode = game->sceneManager->addTextSceneNode(game->gui->getSkin()->getFont(), text.c_str(),
				video::SColor(255, 255, 0, 0), 0, newPos);

			scene::ISceneNodeAnimator* anim = game->sceneManager->createFlyStraightAnimator(newPos, endPos, 2500);
			textNode->addAnimator(anim);
			anim->drop();

			anim = game->sceneManager->createDeleteAnimator(2500);
			textNode->addAnimator(anim);
			anim->drop();
		}
	}
}
#endif
void BatimentSelector::PlacerBatiment::resetPreviewErrors(scene::ISceneNode* batiment)
{
	// Remet le batiment � sa couleur normale
	Game::resetBatimentColor(batiment);

	// Remet le dernier batiment s�lectionn� pour la destruction � sa couleur normale et masque son texte
	// D�sactiv� : Permet de conserver le b�timent point� pour la destruction rouge, pour savoir quel b�timent est d�crit dans la fen�tre d'informations
	//resetBatimentColor(pointedToDestroyBatiment);	pointedToDestroyBatiment = NULL;
	//pointedToDestroyBatiment->setNomBatimentVisible(indeterminate);
	//pointedToDestroyBatiment = NULL;

	// Masque le batiment de pr�visualisation
	if (moveBatiment)
		moveBatiment->setVisible(false);

	// Restaure le b�timent qui �tait � la m�me position que le b�timent de pr�visualisation
	if (samePosBatiment)
	{
		Game::resetBatimentColor(samePosBatiment);
		samePosBatiment = NULL;
	}

	// Les textes sont remis � leur couleur normale dans la fonction Game::updateGameGUI() (-> seulement si on n'est pas en train de construire/d�truire un b�timent !)
}
void BatimentSelector::PlacerBatiment::showSimpleCreationErrors(const core::vector3df* messagesPosition, scene::ISceneNode* batiment, int error, bool outRessources[])
{
	if (error == EBCE_AUCUNE)
		return;

	// TODO : Utiliser la sortie "outRessources" pour savoir quelles ressources sont manquantes

	// Obtient la transparence par d�faut des textes
	const u32 textTransparency = game->guiManager->getTextTransparency();

#ifdef _DEBUG
	// V�rifie que l'erreur peut �tre trait�e ici, sinon, on affiche un message
	if (!( ((error & EBCE_BUDGET) != 0)
		|| ((error & EBCE_ENERGIE) != 0)
		|| ((error & EBCE_PLACE) != 0)
		|| ((error & EBCE_TERRAIN_NON_CONSTRUCTIBLE) != 0)
		|| ((error & EBCE_TERRAIN_OUT) != 0)
		|| ((error & EBCE_NEED_DEEP_WATER) != 0)
		|| ((error & EBCE_RESSOURCE) != 0)
		|| ((error & EBCE_NOT_ENOUGH_POPULATION) != 0)
		|| ((error & EBCE_BATIMENT_ID) != 0)))
		LOG_DEBUG("Game::showSimpleCreationErrors : Impossible d'ajouter le batiment au systeme de jeu : Erreur inconnue : error = " << error, ELL_WARNING);
#endif

	if (((error & EBCE_TERRAIN_OUT) != 0) || ((error & EBCE_PLACE) != 0) || ((error & EBCE_NEED_DEEP_WATER) != 0)
		|| ((error & EBCE_RESSOURCE) != 0) || ((error & EBCE_TERRAIN_NON_CONSTRUCTIBLE) != 0) || ((error & EBCE_NOT_ENOUGH_POPULATION) != 0) && batiment)
	{
		// On ne peut pas placer le batiment � cet endroit ou on n'a pas les ressources ou la population suffisantes :
		// On rend le batiment rouge
		Game::changeBatimentColor(batiment, video::SColor(255, 255, 0, 0));	// Construction impossible : Rouge
	}
	if (game->guiManager->guiElements.gameGUI.budgetTexte)
	{
		if ((error & EBCE_BUDGET) != 0)
		{
			// Le budget est insuffisant : Affiche son texte en rouge
			game->guiManager->guiElements.gameGUI.budgetTexte->setOverrideColor(video::SColor(textTransparency, 255, 50, 50));
		}
		else
		{
			// Le budget est suffisant : Affiche son texte en blanc
			game->guiManager->guiElements.gameGUI.budgetTexte->setOverrideColor(video::SColor(textTransparency, 255, 255, 255));
		}
	}
	if (game->guiManager->guiElements.gameGUI.energieTexte)
	{
		if ((error & EBCE_ENERGIE) != 0)
		{
			// L'�nergie est insuffisante : Affiche son texte en rouge
			game->guiManager->guiElements.gameGUI.energieTexte->setOverrideColor(video::SColor(textTransparency, 255, 50, 50));
		}
		else
		{
			// L'�nergie est suffisante : Affiche son texte en blanc
			game->guiManager->guiElements.gameGUI.energieTexte->setOverrideColor(video::SColor(textTransparency, 255, 255, 255));
		}
	}
#ifdef _DEBUG
	if (((error & EBCE_BATIMENT_ID) != 0))
		LOG_DEBUG("Game::showSimpleCreationErrors : Impossible d'ajouter le batiment au systeme de jeu car l'ID du batiment n'est pas valide : error = " << error, ELL_WARNING);
#endif

	// Affiche les messages � l'utilisateur indiquant pourquoi on n'a pas pu placer le b�timent
	if (messagesPosition)
	{
		core::stringw text = "";
		if ((error & EBCE_TERRAIN_OUT) != 0)
			text.append(L"Hors des limites du terrain\r\n");
		if ((error & EBCE_PLACE) != 0)
			text.append(L"Place insuffisante\r\n");
		if ((error & EBCE_TERRAIN_NON_CONSTRUCTIBLE) != 0)
			text.append(L"Terrain non constructible\r\n");
		if ((error & EBCE_NEED_DEEP_WATER) != 0)
			text.append(L"N�cessite de l'eau profonde\r\n");
		if ((error & EBCE_BUDGET) != 0)
			text.append(L"Budget insuffisant\r\n");
		//if ((error & EBCE_ENERGIE) != 0)				// Le manque d'�nergie n'est plus une erreur fatale pour pouvoir placer un b�timent : on se contente d'afficher son texte en rouge dans ce cas
		//	text.append(L"Energie insuffisante\r\n");
		if ((error & EBCE_RESSOURCE) != 0)
			text.append(L"Ressources insuffisantes\r\n");
		if ((error & EBCE_NOT_ENOUGH_POPULATION) != 0)
			text.append(L"Population insuffisante\r\n");

		if (text.size() > 1)
		{
			core::vector3df newPos(*messagesPosition);

			if (batiment)
				newPos.Y += (batiment->getTransformedBoundingBox().MaxEdge.Y * 0.5f) + 5.0f;
			else
				newPos.Y += 40.0f;

			core::vector3df endPos(newPos);
			endPos.Y += 15.0f;

			scene::ITextSceneNode* textNode = game->sceneManager->addTextSceneNode(game->gui->getSkin()->getFont(), text.c_str(),
				video::SColor(255, 255, 0, 0), 0, newPos);

			scene::ISceneNodeAnimator* anim = game->sceneManager->createFlyStraightAnimator(newPos, endPos, 2500);
			textNode->addAnimator(anim);
			anim->drop();

			anim = game->sceneManager->createDeleteAnimator(2500);
			textNode->addAnimator(anim);
			anim->drop();
		}
	}
}
void BatimentSelector::PlacerBatiment::showSimpleDestructionErrors(const core::vector3df* messagesPosition, const scene::ISceneNode* batiment, int error, bool outRessources[])
{
	if (error == EBDE_AUCUNE)
		return;

	// TODO : Utiliser la sortie "outRessources" pour savoir quelles ressources sont manquantes

	// Obtient la transparence par d�faut des textes
	const u32 textTransparency = game->guiManager->getTextTransparency();

#ifdef _DEBUG
	// V�rifie que l'erreur peut �tre trait�e ici, sinon, on affiche un message
	if (!( ((error & EBDE_BUDGET) != 0)
		|| ((error & EBDE_RESSOURCE) != 0)
		|| ((error & EBDE_INVALID_INDEX) != 0)
		|| ((error & EBDE_ALREADY_DESTRUCTING) != 0)))
		LOG_DEBUG("Game::showSimpleDestructionErrors : Impossible de detruire le batiment : Erreur inconnue : error = " << error, ELL_WARNING);
#endif
	if (game->guiManager->guiElements.gameGUI.budgetTexte)
	{
		if ((error & EBDE_BUDGET) != 0)
		{
			// Le budget est insuffisant : Affiche son texte en rouge
			game->guiManager->guiElements.gameGUI.budgetTexte->setOverrideColor(video::SColor(textTransparency, 255, 50, 50));
		}
		else
		{
			// Le budget est suffisant : Affiche son texte en blanc
			game->guiManager->guiElements.gameGUI.budgetTexte->setOverrideColor(video::SColor(textTransparency, 255, 255, 255));
		}
	}
#ifdef _DEBUG
	if (((error & EBDE_INVALID_INDEX) != 0))
		LOG_DEBUG("Game::showSimpleDestructionErrors : Impossible de supprimer le batiment du systeme de jeu car l'index du batiment n'est pas valide : error = " << error, ELL_WARNING);
#endif

	// Affiche les messages � l'utilisateur indiquant pourquoi on n'a pas pu d�truire le b�timent
	if (messagesPosition)
	{
		core::stringw text = "";
		if ((error & EBDE_BUDGET) != 0)
			text.append(L"Budget insuffisant\r\n");
		if ((error & EBDE_RESSOURCE) != 0)
			text.append(L"Ressources insuffisantes\r\n");
		if ((error & EBDE_ALREADY_DESTRUCTING) != 0)
			text.append(L"Destruction d�j� commenc�e\r\n");

		if (text.size() > 1)
		{
			core::vector3df newPos(*messagesPosition);

			if (batiment)
				newPos.Y += (batiment->getTransformedBoundingBox().MaxEdge.Y * 0.5f) + 5.0f;
			else
				newPos.Y += 40.0f;

			core::vector3df endPos(newPos);
			endPos.Y += 15.0f;

			scene::ITextSceneNode* textNode = game->sceneManager->addTextSceneNode(game->gui->getSkin()->getFont(), text.c_str(),
				video::SColor(255, 255, 0, 0), 0, newPos);

			scene::ISceneNodeAnimator* anim = game->sceneManager->createFlyStraightAnimator(newPos, endPos, 2500);
			textNode->addAnimator(anim);
			anim->drop();

			anim = game->sceneManager->createDeleteAnimator(2500);
			textNode->addAnimator(anim);
			anim->drop();
		}
	}
}
void BatimentSelector::SelectBatiment::frameUpdate()
{
	// V�rifie qu'on a acc�s au triangle selector des b�timents et � la cam�ra, qu'on n'est pas en train de cr�er ou d�truire un b�timent
	// et que des param�tres ont chang� depuis le dernier appel
	scene::ICameraSceneNode* const camera = game->sceneManager->getActiveCamera();
	if (!game->renderer->getBatimentsTriangleSelector() || !camera)
		return;
	if (game->guiManager->guiElements.gameGUI.detruireBouton && game->guiManager->guiElements.gameGUI.detruireBouton->isPressed())
		return;
	if (!lastUpdateParams.hasParamsChanged(game->mouseState.position, camera->getAbsolutePosition()))
		return;

	// Met � jour les param�tres pour le dernier appel � cette fonction
	lastUpdateParams.mousePos = game->mouseState.position;
	lastUpdateParams.camPos = camera->getAbsolutePosition();

	// Obtient le b�timent actuellement point�
	CBatimentSceneNode* const newPointedBatiment = Game::getBatimentNode(getPointedBat());

	// Si le dernier b�timent point� est le m�me que celui-ci, on annule
	if (pointedBatiment == newPointedBatiment)
		return;

	// Restaure le dernier b�timent point� s'il est valide et masque son texte
	if (pointedBatiment != selectedBatiment && pointedBatiment)
	{
		Game::resetBatimentColor(pointedBatiment);
		pointedBatiment->setNomBatimentVisible(false);
	}

	// Montre le nouveau b�timent point� et affiche son nom s'il est valide et qu'il est diff�rent du b�timent actuellement s�lectionn�
	if (newPointedBatiment != selectedBatiment && newPointedBatiment)
	{
		Game::changeBatimentColor(newPointedBatiment, video::SColor(255, 255, 255, 0));	// Point� : Jaune
		newPointedBatiment->setNomBatimentVisible(true);
	}

	// Indique le dernier b�timent point�
	pointedBatiment = newPointedBatiment;
}
void BatimentSelector::SelectBatiment::selectBatiment()
{
	// V�rifie qu'on a acc�s au triangle selector des b�timents et � la cam�ra
	scene::ICameraSceneNode* const camera = game->sceneManager->getActiveCamera();
	if (!game->renderer->getBatimentsTriangleSelector() || !camera)
		return;

	// S�lectionne le b�timent actuellement point�
	CBatimentSceneNode* const newSelectedBatiment = Game::getBatimentNode(getPointedBat());

	// Si le b�timent actuellement s�lectionn� est le m�me que celui-ci, on annule
	if (selectedBatiment == newSelectedBatiment)
		return;

	// Si le b�timent actuellement s�lectionn� est le m�me que le dernier b�timent point�, on oublie le dernier b�timent point�
	if (newSelectedBatiment == pointedBatiment && pointedBatiment)
		pointedBatiment = NULL;

	// Restaure le dernier b�timent s�lectionn� s'il est valide
	if (selectedBatiment)
	{
		Game::resetBatimentColor(selectedBatiment);
		selectedBatiment->setNomBatimentVisible(false);
	}

	if (newSelectedBatiment)	// On a s�lectionn� un nouveau b�timent
	{
		// Montre le nouveau b�timent s�lectionn� s'il est valide
		Game::changeBatimentColor(newSelectedBatiment, video::SColor(255, 0, 255, 0));	// S�lectionn� : Vert
		newSelectedBatiment->setNomBatimentVisible(true);

#ifdef USE_IRRKLANG
		// Joue le son de la s�lection d'un b�timent
		ikMgr.playGameSound(IrrKlangManager::EGSE_BATIMENT_SELECTED);
#endif
	}
#ifdef USE_IRRKLANG
	else						// Si on n'a pas s�lectionn� de nouveau b�timent, alors c'est qu'on a d�selectionn� le pr�c�dent
	{
		// Joue le son de la d�selection d'un b�timent
		ikMgr.playGameSound(IrrKlangManager::EGSE_BATIMENT_DESELECTED);
	}
#endif

	// Indique le scene node actuellement s�lectionn�
	selectedBatiment = newSelectedBatiment;

	// R�initialise tous les param�tres de frameUpdate()
	lastUpdateParams.reset();
}
void BatimentSelector::SelectBatiment::deselectBatiment()
{
	// Restaure le dernier b�timent point� s'il est valide et diff�rent du b�timent s�lectionn�
	if (pointedBatiment != selectedBatiment && pointedBatiment)
	{
		Game::resetBatimentColor(pointedBatiment);
		pointedBatiment->setNomBatimentVisible(false);
	}

	// Restaure le b�timent s�lectionn� s'il est valide
	if (selectedBatiment)
	{
		Game::resetBatimentColor(selectedBatiment);
		selectedBatiment->setNomBatimentVisible(false);

#ifdef USE_IRRKLANG
		// Joue le son de la d�selection d'un b�timent
		ikMgr.playGameSound(IrrKlangManager::EGSE_BATIMENT_DESELECTED);
#endif
	}

	// D�selectionne le b�timent et oublie le b�timent point�
	selectedBatiment = NULL;
	pointedBatiment = NULL;
}
