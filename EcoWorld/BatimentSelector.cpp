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

// Détermine si un point est en-dehors du système de jeu
#define IS_CORNER_OUT_OF_TERRAIN(corner)	((corner).X < 0 || (corner).Y < 0 || (corner).X >= TAILLE_CARTE || (corner).Y >= TAILLE_CARTE)

inline bool BatimentSelector::isPlacingBat() const
{
	return  (placerBatiment.currentBatiment != BI_aucun
		|| (game->guiManager->guiElements.gameGUI.detruireBouton && game->guiManager->guiElements.gameGUI.detruireBouton->isPressed()));
}
bool BatimentSelector::OnEvent(const SEvent& event)
{
	// Gère les évènements du jeu pour le placement et la sélection de bâtiments :

	const bool isCreatingOrDestroyingBatiment = (placerBatiment.currentBatiment != BI_aucun
		|| (game->guiManager->guiElements.gameGUI.detruireBouton && game->guiManager->guiElements.gameGUI.detruireBouton->isPressed()));
#ifdef CAN_BUILD_WITH_SELECTION_RECT
	const bool isRectConstructing = (placerBatiment.rectSelection != core::recti(-1, -1, -1, -1));
#endif

	// Placement des bâtiments (Souris)
	if (event.EventType == EET_MOUSE_INPUT_EVENT)
	{
		if (event.MouseInput.Event == EMIE_RMOUSE_PRESSED_DOWN && !game->isMouseOnGUI && isPlacingOrSelectingBat())
		{
#ifdef CAN_BUILD_WITH_SELECTION_RECT
			if (isRectConstructing)
			{
				// On a cliqué avec le bouton droit et on est en train de faire une sélection rectangulaire :
				// on annule la sélection rectangulaire et on détruit la liste des bâtiments de prévisualisation
				placerBatiment.rectSelection = core::recti(-1, -1, -1, -1);
				placerBatiment.nbBatimentsPreviewRect = 0;
				const u32 listeBatimentsPreviewRectSize = placerBatiment.listeBatimentsPreviewRect.size();
				for (u32 i = 0; i < listeBatimentsPreviewRectSize; ++i)
				{
					scene::ISceneNode* const batiment = placerBatiment.listeBatimentsPreviewRect[i];
					batiment->setVisible(false);
					game->sceneManager->addToDeletionQueue(batiment);

					// Si PostProcess est activé, on lui indique qu'il ne doit plus prendre en compte ce bâtiment dans le flou de profondeur
					if (game->postProcessManager)
						game->postProcessManager->removeNodeFromDepthPass(batiment);

					// Si XEffects est activé, on lui indique qu'il ne doit plus afficher l'ombre de ce bâtiment
					if (game->xEffects)
						game->xEffects->removeShadowFromNode(batiment);
				}
				placerBatiment.listeBatimentsPreviewRect.clear();

				// Désactivable (élement de jouabilité) : De plus, on annule la création du batiment en cours
				annulerSelection();
			}
			else
#endif
			{
				// On a cliqué avec le bouton droit et un batiment est sur le point d'être crée (ou on est en train de détruire des batiments) :
				// on annule la création du batiment
				annulerSelection();
			}
		}
		else if (event.MouseInput.Event == EMIE_MOUSE_WHEEL && !game->isMouseOnGUI && event.MouseInput.Control && placerBatiment.currentBatiment != BI_aucun)
		{
			// Ctrl + Molette de la souris :
			// on modifie la rotation du batiment de prévisualisation de 45° (ou de 90° en mode de construction rectangulaire)
			placerBatiment.currentBatimentRotation += event.MouseInput.Wheel * (isRectConstructing ? 90.0f : 45.0f);

			// Replace la rotation entre 0° et 360°
			while (placerBatiment.currentBatimentRotation < 0.0f)		placerBatiment.currentBatimentRotation += 360.0f;
			while (placerBatiment.currentBatimentRotation >= 360.0f)	placerBatiment.currentBatimentRotation -= 360.0f;
		}
#ifdef CAN_BUILD_WITH_SELECTION_RECT
		else if (event.MouseInput.Event == EMIE_MOUSE_MOVED && isRectConstructing)
		{
			// Si on est en train de faire une sélection rectangulaire et que la souris a bougé :
			// on actualise le rectangle de sélection actuel
			placerBatiment.rectSelection.LowerRightCorner.set(event.MouseInput.X, event.MouseInput.Y);
			//placerBatiment.rectSelection.repair();
		}
#endif
		else if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN && !game->isMouseOnGUI	// Clic gauche de la souris (appuyé) en dehors de la GUI
			&& !game->cameraRTS->getIsCameraFPSEnabled())
		{
			// Si on est en train de créer un bâtiment : on valide l'action
			if (isCreatingOrDestroyingBatiment)
			{
#ifdef CAN_BUILD_WITH_SELECTION_RECT
				if (StaticBatimentInfos::canCreateBySelectionRect(placerBatiment.currentBatiment))
				{
					// Le batiment actuel peut être créé par rectangle : on commence un rectangle de sélection
					placerBatiment.rectSelection.UpperLeftCorner.set(event.MouseInput.X, event.MouseInput.Y);
					placerBatiment.rectSelection.LowerRightCorner.set(event.MouseInput.X, event.MouseInput.Y);
				}
				else
#endif
					// On a cliqué avec le bouton gauche : on crée le batiment et le place, ou on détruit le batiment
					placerBatiment.validateAction();
			}
			else	// Sinon : on valide la sélection du bâtiment pointé
				selectBatiment.selectBatiment();
		}
#ifdef CAN_BUILD_WITH_SELECTION_RECT
		else if (event.MouseInput.Event == EMIE_LMOUSE_LEFT_UP && isRectConstructing)
		{
			// On a relaché le bouton gauche de la souris et on était en train de faire une sélection rectangulaire :
			// on termine la sélection en plaçant le batiment choisi
			placerBatiment.rectSelection.LowerRightCorner.set(event.MouseInput.X, event.MouseInput.Y);

			placerBatiment.validateAction();	// La méthode déterminera automatiquement qu'on était en train de créer une sélection rectangulaire

			placerBatiment.rectSelection = core::recti(-1, -1, -1, -1);
			placerBatiment.nbBatimentsPreviewRect = 0;
		}
#endif
	}
		// Raccourcis claviers :
	else if (event.EventType == EET_KEY_INPUT_EVENT)
	{
		// Spécifique au jeu :
		if ((event.KeyInput.Key == KEY_LEFT || event.KeyInput.Key == KEY_RIGHT) && event.KeyInput.Control && !event.KeyInput.Shift
			&& !event.KeyInput.PressedDown && placerBatiment.currentBatiment != BI_aucun)
		{
			// Ctrl + Flèche gauche ou droite :
			// on modifie la rotation du batiment de prévisualisation de 45°
			placerBatiment.currentBatimentRotation += 45.0f * (event.KeyInput.Key == KEY_LEFT ? 1.0f : -1.0f);

			// Replace la rotation entre 0 et 360°
			while (placerBatiment.currentBatimentRotation < 0.0f)		placerBatiment.currentBatimentRotation += 360.0f;
			while (placerBatiment.currentBatimentRotation >= 360.0f)	placerBatiment.currentBatimentRotation -= 360.0f;
		}
		else if ((event.KeyInput.Key == KEY_UP || event.KeyInput.Key == KEY_DOWN) && event.KeyInput.Control && !event.KeyInput.Shift
			&& !event.KeyInput.PressedDown && placerBatiment.currentBatiment != BI_aucun)
		{
			// Ctrl + Flèche haut ou bas :
			// on modifie la rotation du batiment de prévisualisation de 180°
			placerBatiment.currentBatimentRotation += 180.0f * (event.KeyInput.Key == KEY_UP ? 1.0f : -1.0f);

			// Replace la rotation entre 0 et 360°
			while (placerBatiment.currentBatimentRotation < 0.0f)		placerBatiment.currentBatimentRotation += 360.0f;
			while (placerBatiment.currentBatimentRotation >= 360.0f)	placerBatiment.currentBatimentRotation -= 360.0f;
		}
		else if (event.KeyInput.Key == KEY_DELETE && !event.KeyInput.PressedDown && game->guiManager->guiElements.gameGUI.detruireBouton && game->canHandleKeysEvent)
		{
			// DEL (ou SUPPR)

			// On peut aussi activer/désactiver le bouton pour détruire un bâtiment par un appui sur la touche DEL :

			// Retiens si ce bouton va être pressé d'après son état actuel (annulerSelection() va rendre ce bouton non pressé !)
			const bool newPressed = !game->guiManager->guiElements.gameGUI.detruireBouton->isPressed();

			// Annule la sélection actuelle (création/destruction/sélection d'un bâtiment)
			annulerSelection();

			// Rend à nouveau ce bouton pressé
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
		// Rend tous les boutons non pressés
		const core::list<gui::IGUIButton*>::Iterator END = game->guiManager->guiElements.gameGUI.listeBoutonsBatiments.end();
		for (core::list<gui::IGUIButton*>::Iterator it = game->guiManager->guiElements.gameGUI.listeBoutonsBatiments.begin(); it != END; ++it)
		{
			gui::IGUIButton* const currentBouton = (*it);
			if (currentBouton)
				currentBouton->setPressed(false);
		}

		// Indique qu'il n'y a plus de bâtiment en construction (moveBatiment sera réinitialisé dans BatimentSelector::PlacerBatiment::updatePrevisualisationBat())
		placerBatiment.currentBatiment = BI_aucun;
	}

	// Déselectionne le bâtiment actuellement sélectionné
	selectBatiment.deselectBatiment();

	// Rend aussi le bouton pour détruire un batiment non pressé
	if (game->guiManager->guiElements.gameGUI.detruireBouton)
		game->guiManager->guiElements.gameGUI.detruireBouton->setPressed(false);

	// Remet le batiment sélectionné pour la destruction à sa couleur normale et masque son texte (-> n'est plus fait dans placerBatiment.resetPreviewErrors)
	if (placerBatiment.pointedToDestroyBatiment)
	{
		Game::resetBatimentColor(placerBatiment.pointedToDestroyBatiment);
		placerBatiment.pointedToDestroyBatiment->setNomBatimentVisible(false);
		placerBatiment.pointedToDestroyBatiment = NULL;
	}

	// Réinitialise les appels à placerBatiment.frameUpdate() et selectBatiment.frameUpdate()
	placerBatiment.lastUpdateParams.reset();
	selectBatiment.lastUpdateParams.reset();
}
void BatimentSelector::reset()
{
	// Réinitialise les deux modules de placement et de sélection des bâtiments
	placerBatiment.reset();
	selectBatiment.reset();
}
void BatimentSelector::pushBackGamePointers(core::list<CBatimentSceneNode**>& gameSceneNodesPointers)
{
	// Ajoute les pointeurs du jeu susceptibles de pointer sur un scene node invalide (sur le point de se détruire) à la liste fournie
	gameSceneNodesPointers.push_back(&placerBatiment.pointedToDestroyBatiment);
	gameSceneNodesPointers.push_back(&placerBatiment.samePosBatiment);
	gameSceneNodesPointers.push_back(&selectBatiment.selectedBatiment);
	gameSceneNodesPointers.push_back(&selectBatiment.pointedBatiment);
}
void BatimentSelector::update()
{
	// Met à jour les deux modules de placement et de sélection des bâtiments
	placerBatiment.frameUpdate();
	if (placerBatiment.currentBatiment == BI_aucun)
		selectBatiment.frameUpdate();

	// Met à jour la GUI du jeu d'après le placement et la sélection actuelle, si elle est visible
	if (game->guiManager->isGUIVisible(GUIManager::EGN_gameGUI))
		updateGameGUI();
}
void BatimentSelector::updateGameGUI()
{
	GUIManager* const guiManager = game->guiManager;

	// Met à jour les informations sur le bâtiment actuellement sélectionné
	if (guiManager->guiElements.gameGUI.informationsWindow)
	{
		// Indique le bâtiment actuellement sélectionné à la fenêtre d'informations
		guiManager->guiElements.gameGUI.informationsWindow->setSelectedBatiment(selectBatiment.selectedBatiment ? selectBatiment.selectedBatiment->getAssociatedBatiment() : NULL);

		// Indique si la fenêtre d'informations devra être visible ou non
		bool informationsWindowVisible = false;

		// Construction :
		if (placerBatiment.currentBatiment != BI_aucun)
		{
#ifdef CAN_BUILD_WITH_SELECTION_RECT
			if (placerBatiment.rectSelection != core::recti(-1, -1, -1, -1) && placerBatiment.nbBatimentsPreviewRect > 1)	// Construction de bâtiments en rectangle avec plus d'un bâtiment prévu
				guiManager->guiElements.gameGUI.informationsWindow->updateInfosTexte(
					CGUIInformationsWindow::EIT_MULTI_CONSTRUCTION, placerBatiment.currentBatiment, placerBatiment.nbBatimentsPreviewRect, NULL);
			else	// Construction simple d'un bâtiment
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
		// Sélection :
		else if (selectBatiment.selectedBatiment && selectBatiment.selectedBatiment->getAssociatedBatiment())
		{
			guiManager->guiElements.gameGUI.informationsWindow->updateInfosTexte(
				CGUIInformationsWindow::EIT_SELECTION, BI_aucun, 0, selectBatiment.selectedBatiment->getAssociatedBatiment());
			informationsWindowVisible = true;
		}

		// Rend la fenêtre d'informations visible si nécessaire
		guiManager->guiElements.gameGUI.informationsWindow->setVisible(informationsWindowVisible);
	}
}
scene::ISceneNode* BatimentSelector::getPointedBat(const core::vector2di& mousePos)
{
	// Si la caméra, le renderer ou le triangle selector des bâtiments ne sont pas valides, on quitte
	scene::ICameraSceneNode* const camera = game->sceneManager->getActiveCamera();
	if (!camera || !game->renderer->getBatimentsTriangleSelector())
		return NULL;

	const bool validMousePos = (mousePos.X >= 0 && mousePos.Y >= 0);

	scene::ISceneNode* node = NULL;
	core::triangle3df triangle;
	core::vector3df position;

	// Obtient le node visé par la souris
	scene::ISceneCollisionManager* const collisionMgr = game->sceneManager->getSceneCollisionManager();
	const core::line3df ligne = collisionMgr->getRayFromScreenCoordinates(validMousePos ? mousePos : game->mouseState.position, camera);
	const bool collision = collisionMgr->getCollisionPoint(ligne, game->renderer->getBatimentsTriangleSelector(), position, triangle, node);
	if (!collision)
		return NULL;

	// Si le type de ce node est un node de bâtiment, on le renvoie
	if (node->getType() == ESNT_BATIMENT_NODE)
		return node;

	// Sinon, on recherche si un parent de ce node n'est pas un bâtiment
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
	// Si la caméra, le renderer ou le triangle selector du terrain ne sont pas valides, on quitte
	scene::ICameraSceneNode* const camera = game->sceneManager->getActiveCamera();
	core::vector3df position(-1.0f);
	if (!camera || !game->renderer)
		return position;
	if (!game->renderer->getTerrainTriangleSelector())
		return position;

	const bool validMousePos = (mousePos.X >= 0 && mousePos.Y >= 0);

	scene::ISceneNode* node = 0;
	core::triangle3df triangle;

	// Obtient la position visée par la souris
	scene::ISceneCollisionManager* const collisionMgr = game->sceneManager->getSceneCollisionManager();
	const core::line3df ligne = collisionMgr->getRayFromScreenCoordinates(validMousePos ? mousePos : game->mouseState.position, camera);
	const bool collision = collisionMgr->getCollisionPoint(
		ligne, game->renderer->getTerrainTriangleSelector(), position, triangle, node);

	return (collision ? position : core::vector3df(-1.0f));
}
void BatimentSelector::PlacerBatiment::frameUpdate()
{
	// Met à jour le bâtiment de prévisualisation
	updatePrevisualisationBat();

	// Détermine si le joueur veut détruire ou construire des batiments
	if (game->guiManager->guiElements.gameGUI.detruireBouton && game->guiManager->guiElements.gameGUI.detruireBouton->isPressed())
	{
		// Réinitialise les paramêtres pour la construction
		lastUpdateParams.batimentID = BI_aucun;
		lastUpdateParams.batimentRotation = -1.0f;
#ifdef CAN_BUILD_WITH_SELECTION_RECT
		// Réinitialise les paramêtres pour la construction en zone rectangulaire
		lastUpdateParams.rectSelection = core::recti(-1, -1, -1, -1);
		lastUpdateParams.pointedFirstCorner.set(-1, -1);
#if !defined(FORCE_RECTANGLE) || !defined(ONLY_TAKE_MAIN_CORNERS)
		lastUpdateParams.pointedSecondCorner.set(-1, -1);
		lastUpdateParams.pointedThirdCorner.set(-1, -1);
#endif
		lastUpdateParams.pointedLastCorner.set(-1, -1);
#endif

		// Prévisualise la destruction simple du batiment
		previewDestroySimpleBat();
	}
#ifdef CAN_BUILD_WITH_SELECTION_RECT
	else if (rectSelection != core::recti(-1, -1, -1, -1))
	{
		// Prévisualise la création en zone rectangulaire du batiment
		previewCreateRectBats(currentBatiment);
	}
#endif
	else
	{
#ifdef CAN_BUILD_WITH_SELECTION_RECT
		// Réinitialise les paramêtres pour la construction en zone rectangulaire
		lastUpdateParams.rectSelection = core::recti(-1, -1, -1, -1);
		lastUpdateParams.pointedFirstCorner.set(-1, -1);
#if !defined(FORCE_RECTANGLE) || !defined(ONLY_TAKE_MAIN_CORNERS)
		lastUpdateParams.pointedSecondCorner.set(-1.0f, -1.0f, -1.0f);
		lastUpdateParams.pointedThirdCorner.set(-1.0f, -1.0f, -1.0f);
#endif
		lastUpdateParams.pointedLastCorner.set(-1, -1);
#endif

		// Prévisualise la création simple du batiment
		previewCreateSimpleBat(currentBatiment, moveBatiment);
	}
}
void BatimentSelector::PlacerBatiment::validateAction()
{
	// Réinitialise la GUI dûe à la prévisualisation des batiments
	resetPreviewErrors(moveBatiment);

	// Détermine si le joueur veut détruire ou construire des batiments
	if (game->guiManager->guiElements.gameGUI.detruireBouton && game->guiManager->guiElements.gameGUI.detruireBouton->isPressed())
		destroySimpleBat();					// Détruit le batiment
#ifdef CAN_BUILD_WITH_SELECTION_RECT
	else if (rectSelection != core::recti(-1, -1, -1, -1))
		createRectBats(currentBatiment);	// Crée le batiment en zone rectangulaire
#endif
	else
		createSimpleBat(currentBatiment);	// Crée le batiment

	// Réinitialise tous les paramêtres de frameUpdate()
	lastUpdateParams.reset();
}
void BatimentSelector::PlacerBatiment::updatePrevisualisationBat()
{
	// Recrée le bâtiment de prévisualisation si besoin
	static BatimentID lastBatiment = currentBatiment;
	if (lastBatiment != currentBatiment)	// Si le batiment choisi est différent du batiment actuel
	{
		lastBatiment = currentBatiment;
		if (moveBatiment)
		{
			// Si PostProcess est activé, on lui indique qu'il ne doit plus prendre en compte ce bâtiment dans le flou de profondeur
			if (game->postProcessManager)
				game->postProcessManager->removeNodeFromDepthPass(moveBatiment);

			// Si XEffects est activé, on lui indique qu'il ne doit plus afficher l'ombre de ce bâtiment
			if (game->xEffects)
				game->xEffects->removeShadowFromNode(moveBatiment);

			moveBatiment->setVisible(false);
			game->sceneManager->addToDeletionQueue(moveBatiment); // On supprime l'ancien batiment
		}
		moveBatiment = NULL; // Et on met sa valeur à NULL pour qu'un autre soit créé

		if (samePosBatiment)
		{
			Game::resetBatimentColor(samePosBatiment);
			samePosBatiment = NULL;
		}
	}

	// On doit créer un nouveau bâtiment de prévisualisation s'il n'et pas déjà créé et qu'un type de bâtiment est sélectionné pour être créé
	if (!moveBatiment && currentBatiment != BI_aucun)
	{
		// Crée le bâtiment de prévisualisation
		moveBatiment = game->renderer->loadBatiment(currentBatiment, 0, false, true);

		// Ajoute un bloc de béton sous le bâtiment de prévisualisation si nécessaire
		if (moveBatiment)
		{
			if (StaticBatimentInfos::needConcreteUnderBatiment(currentBatiment))
			{
				// Le dénivelé ici indiqué est arbitraire : il peut être augmenté ou diminué suivant les besoins des terrains
				scene::ISceneNode* const concreteNode = game->renderer->loadConcreteBatiment(40.0f, currentBatiment, moveBatiment, false, true);

				// Modifie l'agrandissement du node de béton pour que son agrandissement final soit de 1.0f (évite qu'il ne soit affecté par l'agrandissement du bâtiment de prévisualisation)
				const core::vector3df& moveBatimentScaling = moveBatiment->getScale();
				concreteNode->setScale(core::vector3df(1.0f / moveBatimentScaling.X, 1.0f / moveBatimentScaling.Y, 1.0f / moveBatimentScaling.Z));
			}

#if 0		// Permet de le désactiver : peu efficace lorsque la caméra est éloignée, et trompe légèrement le joueur sur la taille réelle du bâtiment
			// Agrandit un petit peu le bâtiment par rapport à sa taille normale pour qu'il soit "par-dessus" les autres bâtiments du même type :
			if (currentBatiment != BI_route)
				moveBatiment->setScale(moveBatiment->getScale() * core::vector3df(1.05f, 1.05f, 1.05f));
			else // Pour les routes, on augmente plus la hauteur, et moins la largeur
				moveBatiment->setScale(moveBatiment->getScale() * core::vector3df(1.001f, 1.5f, 1.001f));
#endif
		}
	}

	// Masque le bâtiment de prévisualisation si on est en mode de création rectangulaire
	if (moveBatiment && rectSelection != core::recti(-1, -1, -1, -1))
		moveBatiment->setVisible(false);
}
void BatimentSelector::PlacerBatiment::previewCreateSimpleBat(BatimentID batimentID, scene::ISceneNode* batiment, core::vector2di index)
{
	// Si le batiment à placer n'est pas valide, on quitte
	if (!batiment)
		return;

	// Détermine si cette fonction a été appelée par la prévisualisation rectangulaire : si le bâtiment qu'on doit construire est créé par rectangle de sélection ET que le bâtiment fourni n'est pas le bâtiment de prévisualisation normal
	const bool calledByRectCreation = (StaticBatimentInfos::canCreateBySelectionRect(batimentID)) && (batiment != moveBatiment);

	if (!calledByRectCreation)	// Vérifie si les paramêtres ont changé si on n'a pas été appelé par la prévisualisation rectangulaire
	{
		// Si aucun paramêtre n'a changé depuis le dernier appel, on quitte aussi
		const core::vector3df camPos = game->sceneManager->getActiveCamera() ? game->sceneManager->getActiveCamera()->getAbsolutePosition() : core::vector3df(-1.0f);
		if (!lastUpdateParams.hasConstructSimpleParamsChanged(game->system.getTime().getTotalJours(), batimentID, currentBatimentRotation, game->mouseState.position, camPos))
			return;

		// Met à jour les paramêtres pour le dernier appel à cette fonction
		lastUpdateParams.systemTotalDays = game->system.getTime().getTotalJours();
		lastUpdateParams.batimentID = batimentID;
		lastUpdateParams.batimentRotation = currentBatimentRotation;
		lastUpdateParams.mousePos = game->mouseState.position;
		lastUpdateParams.camPos = camPos;

		// Réinitialise la GUI dûe à la prévisualisation des batiments
		resetPreviewErrors(moveBatiment);
	}

	core::vector3df position;
	bool visible = true;

	// Vérifie que l'index fourni est bien valide, sauf si on a été appelé par la prévisualisation de bâtiments en rectangle : dans ce cas, on conserve cet index quoi qu'il arrive
	if ((index.X >= 0 && index.Y >= 0 && index.X < TAILLE_CARTE && index.Y < TAILLE_CARTE) || calledByRectCreation)
		position = EcoWorldSystem::getPositionFromIndex(index);	// Obtient la position du node suivant son index
	else
	{
		// Obtient la position du terrain pointée par la souris
		position = getPointedTerrainPos();

		// La souris est en dehors du terrain si : position == core::vector3df(-1.0f)
		if (position == core::vector3df(-1.0f))
			visible = false;
		else
		{
			visible = true;

			// Arrondi la position à TAILLE_OBJETS en X et en Z
			position = EcoWorldSystem::getRoundPosition(position);

			// Obtient l'index du bâtiment
			index = EcoWorldSystem::getIndexFromPosition(position);
		}
	}

	// Lorsque le batiment de prévisualisation est sur un batiment déjà existant et du même ID avec la même rotation actuelle sans qu'il ne se construise ou détruise,
	// on modifie la couleur du batiment déjà existant et on masque le batiment de prévisualisation.
	// Sinon, on modifie effectivement le batiment de prévisualisation.
	const Batiment* const samePosBat =
		(index.X >= 0 && index.Y >= 0 && index.X < TAILLE_CARTE && index.Y < TAILLE_CARTE) ? game->system.carte[index.X][index.Y].batiment : NULL;
	if (samePosBat && !calledByRectCreation			// TODO : Faire fonctionner ce système aussi en mode création rectangulaire
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
			// Obtient la position du terrain en Y d'après le terrain actuel
			game->renderer->getNewBatimentPos(position, batimentID, currentBatimentRotation);

			// Place le batiment
			batiment->setPosition(position);
			batiment->setRotation(core::vector3df(0.0f, currentBatimentRotation, 0.0f));
		}
	}

	// Vérifie si on peut placer le batiment
	bool outRessources[Ressources::RI_COUNT];
#ifndef _DEBUG
	const	// Le const désactivé permet de désactiver l'erreur EBCE_NOT_ENOUGH_POPULATION en mode DEBUG
#endif
		int error = game->system.canCreateBatiment(batimentID, index, currentBatimentRotation, outRessources);

#ifdef _DEBUG
	// /!\ Attention /!\ 
	// En mode DEBUG uniquement : On désactive l'erreur EBCE_NOT_ENOUGH_POPULATION pour permettre le fonctionnement du code de débogage pour débloquer tous les bâtiments
	if (game->unlockAllBatiments)
		error &= ~EBCE_NOT_ENOUGH_POPULATION;	// Passe le bit représenté par EBCE_NOT_ENOUGH_POPULATION à 0 (~ : opérateur NOT bit à bit)
#endif

	// On n'affiche pas l'erreur du manque de place si le bâtiment peut être créé par rectangle de sélection
	//if (canCreateBatimentBySelectionRect(batimentID))
	//	error &= ~EBCE_PLACE;	// Passe le bit représenté par EBCE_PLACE à 0 (~ : opérateur NOT bit à bit)

	// Affiche les messages à l'utilisateur indiquant pourquoi on n'a pas pu placer le bâtiment
	showSimpleCreationErrors(NULL, samePosBatiment ? samePosBatiment : batiment, error, outRessources);
}
void BatimentSelector::PlacerBatiment::previewDestroySimpleBat(core::vector2di index)
{
	// Détermine si cette fonction a été appelée par la prévisualisation rectangulaire
	//const bool calledByRectDestruction = ;	// TODO

	//if (!calledByRectDestruction)	// Vérifie si les paramêtres ont changé si on n'a pas été appelé par la prévisualisation rectangulaire
	//{
	// Vérifie que les paramêtres ont bien changé depuis le dernier appel à cette fonction
		const core::vector3df camPos = game->sceneManager->getActiveCamera() ? game->sceneManager->getActiveCamera()->getAbsolutePosition() : core::vector3df(-1.0f);
		if (!lastUpdateParams.hasDestroySimpleParamsChanged(game->system.getTime().getTotalJours(), game->mouseState.position, camPos))
			return;

		// Met à jour les paramêtres pour le dernier appel à cette fonction
		lastUpdateParams.systemTotalDays = game->system.getTime().getTotalJours();
		lastUpdateParams.mousePos = game->mouseState.position;
		lastUpdateParams.camPos = camPos;

		// Réinitialise la GUI dûe à la prévisualisation des batiments
		resetPreviewErrors(moveBatiment);
	//}

	scene::ISceneNode* node = NULL;

	// Vérifie que l'index fourni est bien valide
	if (index.X >= 0 && index.Y >= 0 && index.X < TAILLE_CARTE && index.Y < TAILLE_CARTE)
	{
		// Obtient le bâtiment à l'index spécifié puis son node associé
		Batiment* batiment = game->system.carte[index.X][index.Y].batiment;
		if (batiment)
			node = batiment->getSceneNode();
	}
	else	// Si l'index du bâtiment n'est pas valide, on obtient le bâtiment pointé avec la souris
	{
		node = getPointedBat();

		// Obtient l'index du batiment associé à ce node
		const CBatimentSceneNode* batNode = Game::getBatimentNode(node);
		if (batNode && batNode->getAssociatedBatiment())
			index = batNode->getAssociatedBatiment()->getIndex();
		else
			return;
	}

	// Vérifie que le node spécifié existe et que le node a bien changé depuis le dernier appel, sinon on quitte
	if (!node || node == pointedToDestroyBatiment)
		return;

	// Remet le dernier batiment sélectionné pour la destruction à sa couleur normale et masque son texte (-> n'est plus fait dans resetPreviewErrors)
	if (pointedToDestroyBatiment)
	{
		Game::resetBatimentColor(pointedToDestroyBatiment);
		pointedToDestroyBatiment->setNomBatimentVisible(false);
		pointedToDestroyBatiment = NULL;
	}

	// Obtient le bâtiment pointé pour la destruction :
	pointedToDestroyBatiment = Game::getBatimentNode(node);

	// Change la couleur du bâtiment visé et affiche son nom
	if (pointedToDestroyBatiment)
	{
		Game::changeBatimentColor(pointedToDestroyBatiment, video::SColor(255, 255, 0, 0));	// Destruction : Rouge
		pointedToDestroyBatiment->setNomBatimentVisible(true);
	}

	// Vérifie qu'on peut détruire ce batiment
	bool outRessources[Ressources::RI_COUNT];
	const int error = game->system.canDestroyBatiment(index, outRessources);

	// Affiche les messages à l'utilisateur indiquant pourquoi on n'a pas pu détruire le bâtiment
	showSimpleDestructionErrors(NULL, node, error, outRessources);
}
void BatimentSelector::PlacerBatiment::createSimpleBat(BatimentID batimentID, core::vector2di index)
{
	// Si la caméra ou le renderer ne sont pas valides, on quitte
	scene::ICameraSceneNode* const camera = game->sceneManager->getActiveCamera();
	if (!camera)
		return;

#ifdef USE_IRRKLANG	// Seulement utilisé pour IrrKlang pour le moment
	// Détermine si cette fonction a été appelée par la création rectangulaire
	const bool calledByRectCreation = StaticBatimentInfos::canCreateBySelectionRect(batimentID);
#endif

	core::vector3df position;
	bool visible = true;

	// Vérifie que l'index fourni est bien valide
	if (index.X >= 0 && index.Y >= 0 && index.X < TAILLE_CARTE && index.Y < TAILLE_CARTE)
		position = EcoWorldSystem::getPositionFromIndex(index);	// Obtient la position du node suivant son index
	else
	{
		// Obtient la position du terrain pointée par la souris
		position = getPointedTerrainPos();

		// La souris est en dehors du terrain si : position == core::vector3df(-1.0f)
		if (position == core::vector3df(-1.0f))
			visible = false;
		else
		{
			visible = true;

			// Arrondi la position à TAILLE_OBJETS en X et en Z
			position = EcoWorldSystem::getRoundPosition(position);

			// Obtient l'index du bâtiment
			index = EcoWorldSystem::getIndexFromPosition(position);
		}
	}

	// Si on ne peut placer le batiment, on quitte
	if (!visible)
		return;

	// Détermine si on peut placer ce bâtiment
	bool outRessources[Ressources::RI_COUNT];
#ifndef _DEBUG
	const	// Le const désactivé permet de désactiver l'erreur EBCE_NOT_ENOUGH_POPULATION en mode DEBUG
#endif
		int error = game->system.canCreateBatiment(batimentID, index, currentBatimentRotation, outRessources);

#ifdef _DEBUG
	// /!\ Attention /!\ 
	// En mode DEBUG uniquement : On désactive l'erreur EBCE_NOT_ENOUGH_POPULATION pour permettre le fonctionnement du code de débogage pour débloquer tous les bâtiments
	if (game->unlockAllBatiments)
		error &= ~EBCE_NOT_ENOUGH_POPULATION;	// Passe le bit représenté par EBCE_NOT_ENOUGH_POPULATION à 0 (~ : opérateur NOT bit à bit)
#endif

	if (error != EBCE_AUCUNE && error != EBCE_ENERGIE)	// Vérifie que l'erreur n'est pas aussi dûe à un manque d'énergie, qui n'est plus une erreur fatale
	{
		// On n'affiche pas l'erreur du manque de place si le bâtiment peut être créé par glissement de la souris ou par rectangle de sélection
		//if (calledByRectCreation)
		//	error &= ~EBCE_PLACE; // Passe le bit représenté par EBCE_PLACE à 0 (~ : opérateur NOT bit à bit)

		// Affiche les messages à l'utilisateur indiquant pourquoi on n'a pas pu placer le bâtiment
		showSimpleCreationErrors(&position, NULL, error, outRessources);

#ifdef USE_IRRKLANG
		// Joue le son de la construction d'un bâtiment refusée si on n'a pas été appelé par la construction rectangulaire
		if (!calledByRectCreation)
			ikMgr.playGameSound(IrrKlangManager::EGSE_BATIMENT_CREATE_NEGATIVE);
#endif
	}
	else	// Le système de jeu n'a retourné aucune erreur : on peut donc ajouter ce bâtiment
	{
#ifdef USE_RAKNET
		// Indique à RakNet qu'on veut créer un bâtiment à cet emplacement, s'il est activé
		if (rkMgr.isNetworkPlaying()) // && !calledByRectCreation	// Vérifie qu'on n'a pas été appelé par la destruction en rectangle, qui est elle-même chargée d'envoyer l'ordres de destruction à RakNet
		{
			RakNetManager::RakNetGamePacket packet(RakNetManager::EGPT_BATIMENT_CONSTRUCTED);
			packet.gameConstructInfos.batimentID = batimentID;
			packet.gameConstructInfos.indexX = index.X;
			packet.gameConstructInfos.indexY = index.Y;
			packet.gameConstructInfos.rotation = currentBatimentRotation;

			// Pré-calcule la durée de vie de ce bâtiment pour pouvoir l'envoyer aux clients afin qu'elle soit synchronisée
			const StaticBatimentInfos& staticBatInfos = StaticBatimentInfos::getInfos(batimentID);
			packet.gameConstructInfos.dureeVie = BatimentInfos::computeDureeVie(staticBatInfos.dureeVie, staticBatInfos.dureeVieVariation);

			rkMgr.sendPackets.push_back(packet);
		}
		else
#endif
		{
			// Ajoute le bâtiment au système de jeu (il sera aussi ajouté au renderer)
			game->system.addBatiment(batimentID, index, currentBatimentRotation, outRessources, &position);

#ifdef _DEBUG
			// En mode DEBUG : On vérifie que le bâtiment existe bien
			if (!game->system.carte[index.X][index.Y].batiment)
				LOG_DEBUG("Game::createSimpleBat : Le systeme de jeu n'a retourne aucune erreur lors de l'ajout du batiment, pourtant il n'existe pas sur la carte du systeme :" << endl
					<< "    game->system.carte[" << index.X << "][" << index.Y << "].batiment = " << game->system.carte[index.X][index.Y].batiment, ELL_WARNING);
#endif

#ifdef USE_IRRKLANG
			// Joue le son de la construction d'un bâtiment acceptée si on n'a pas été appelé par la construction rectangulaire
			if (!calledByRectCreation)
				ikMgr.playGameSound(IrrKlangManager::EGSE_BATIMENT_CREATE_POSITIVE);
#endif
		}
	}
}
void BatimentSelector::PlacerBatiment::destroySimpleBat(core::vector2di index)
{
#ifdef USE_IRRKLANG	// Seulement utilisé pour IrrKlang pour le moment
	// Détermine si cette fonction a été appelée par la destruction rectangulaire
	//const bool calledByRectDestruction = ;	// TODO : Destruction rectangulaire non implémentée pour le moment
#endif

	scene::ISceneNode* node = NULL;

	// Vérifie que l'index fourni est bien valide
	if (index.X >= 0 && index.Y >= 0 && index.X < TAILLE_CARTE && index.Y < TAILLE_CARTE)
	{
		// Obtient le bâtiment à l'index spécifié puis son node associé
		Batiment* batiment = game->system.carte[index.X][index.Y].batiment;
		if (batiment)
			node = batiment->getSceneNode();
	}
	else	// Si l'index du bâtiment n'est pas valide, on obtient le bâtiment pointé avec la souris
	{
		node = getPointedBat();

		// Obtient l'index du batiment associé à ce node
		CBatimentSceneNode* const batNode = Game::getBatimentNode(node);
		if (batNode && batNode->getAssociatedBatiment())
			index = batNode->getAssociatedBatiment()->getIndex();
		else
			return;
	}

	// Vérifie que le node spécifié existe
	if (!node)
		return;

	// Remet le dernier batiment sélectionné pour la destruction à sa couleur normale et masque son texte (-> n'est plus fait dans resetPreviewErrors)
	if (pointedToDestroyBatiment)
	{
		Game::resetBatimentColor(pointedToDestroyBatiment);
		pointedToDestroyBatiment->setNomBatimentVisible(false);
		pointedToDestroyBatiment = NULL;
	}

	// Obtient le bâtiment pointé pour la destruction :
	pointedToDestroyBatiment = Game::getBatimentNode(node);

	// Détermine si on peut détruire ce batiment
	bool outRessources[Ressources::RI_COUNT];
	const int error = game->system.canDestroyBatiment(index, outRessources);

	// S'il y a une erreur à détruire ce batiment, on l'affiche (le bâtiment n'a pas été détruit)
	if (error != EBDE_AUCUNE)
	{
		// Affiche les messages à l'utilisateur indiquant pourquoi on n'a pas pu détruire le bâtiment
		const core::vector3df messagesPos = node->getAbsolutePosition();
		showSimpleDestructionErrors(&messagesPos, node, error, outRessources);

#ifdef USE_IRRKLANG
		// Joue le son de la destruction d'un bâtiment refusée si on n'a pas été appelé par la destruction rectangulaire
		//if (!calledByRectDestruction)
			ikMgr.playGameSound(IrrKlangManager::EGSE_BATIMENT_DESTROY_NEGATIVE);
#endif
	}
	else	// Le système de jeu n'a retourné aucune erreur : on peut donc supprimer ce bâtiment
	{
#ifdef USE_RAKNET
		// Indique à RakNet qu'on veut supprimer ce bâtiment, s'il est activé
		if (rkMgr.isNetworkPlaying()) // && !calledByRectDestruction	// Vérifie qu'on n'a pas été appelé par la destruction en rectangle, qui est elle-même chargée d'envoyer l'ordres de destruction à RakNet
		{
			RakNetManager::RakNetGamePacket packet(RakNetManager::EGPT_BATIMENT_DESTROYED);
			packet.gameDestroyInfos.indexX = index.X;
			packet.gameDestroyInfos.indexY = index.Y;
			rkMgr.sendPackets.push_back(packet);
		}
		else
#endif
		{
			// Supprime le bâtiment du système de jeu (il sera aussi supprimé du renderer)
			game->system.destroyBatiment(index, outRessources);

#ifdef _DEBUG
			// En mode DEBUG : On vérifie que le bâtiment n'existe plus
			if (!game->system.carte[index.X][index.Y].batiment)
				LOG_DEBUG("Game::destroySimpleBat : Le systeme de jeu n'a retourne aucune erreur lors de la destruction du batiment, pourtant existe toujours sur la carte du systeme :" << endl
					<< "    game->system.carte[" << index.X << "][" << index.Y << "].batiment = " << game->system.carte[index.X][index.Y].batiment, ELL_WARNING);
#endif

#ifdef USE_IRRKLANG
			// Joue le son de la destruction d'un bâtiment acceptée si on n'a pas été appelé par la destruction rectangulaire
			//if (!calledByRectDestruction)
				ikMgr.playGameSound(IrrKlangManager::EGSE_BATIMENT_DESTROY_POSITIVE);
#endif
		}
	}
}
#ifdef CAN_BUILD_WITH_SELECTION_RECT
void BatimentSelector::PlacerBatiment::previewCreateRectBats(BatimentID batimentID)
{
	// Vérifie que le rectangle de sélection est bien valide et qu'on a accès au terrain, à son triangle selector et à la caméra
	scene::ICameraSceneNode* camera = game->sceneManager->getActiveCamera();
	if (!camera || !game->renderer || rectSelection == core::recti(-1, -1, -1, -1))
		return;
	// Si aucun paramêtre n'a changé depuis le dernier appel ou que le triangle selector du terrain n'est pas valide, on quitte aussi
	if (!game->renderer->getTerrainManager().getTerrainTriangleSelector()
		|| !lastUpdateParams.hasConstructRectParamsChanged(game->system.getTime().getTotalJours(), batimentID, currentBatimentRotation,
			game->mouseState.position, camera->getAbsolutePosition(), rectSelection))
		return;

	// Met à jour les paramêtres pour le dernier appel à cette fonction
	const bool sameID = (lastUpdateParams.batimentID == batimentID);
	lastUpdateParams.mousePos = game->mouseState.position;
	lastUpdateParams.camPos = camera->getAbsolutePosition();
	lastUpdateParams.rectSelection = rectSelection;

	// Les 4 coins des indices correspondant au rectangle de sélection actuel
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
		 \     \   |  largeur Y (+ écart Y)
		  \     \  |
		   3 --- 4 \/
		<->
		écart X
	*/

	// Obtient les coins des indices
	{
		core::vector3df position;

		// Coin 1
		{
			// Obtient la position du terrain pointée par la souris
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
			// Obtient la position du terrain pointée par la souris
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
			// Obtient la position du terrain pointée par la souris
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
			// Obtient la position du terrain pointée par la souris
			position = getPointedTerrainPos(rectSelection.LowerRightCorner);

			// La position est en dehors du terrain si : position == core::vector3df(-1.0f)
			if (position != core::vector3df(-1.0f))
			{
				// Obtient le premier indice suivant la position actuelle du point de collision
				coin4 = EcoWorldSystem::getIndexFromPosition(position);
			}
		}
	}

	// Vérifie que tous les coins sont valides, sinon on quitte
	// On vérifie aussi qu'au moins un des deux points est dans le terrain, pour éviter d'afficher des bâtiments en-dehors du terrain.
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

	// Tourne les coins pour qu'ils correspondent à leurs caractéristiques
	{
#ifdef FORCE_RECTANGLE
#ifdef ONLY_TAKE_MAIN_CORNERS
		// Ne prend en compte que les coins 1 et 4, sinon on se retrouve généralement avec un rectangle des index trop grand
		// On force aussi les coins à se retrouver dans le terrain, pour éviter que certains bâtiments se retrouvent en-dehors du terrain.
		const int minX = core::clamp(min(coin1.X, coin4.X), 0, TAILLE_CARTE);
		const int minY = core::clamp(min(coin1.Y, coin4.Y), 0, TAILLE_CARTE);
		const int maxX = core::clamp(max(coin1.X, coin4.X), 0, TAILLE_CARTE);
		const int maxY = core::clamp(max(coin1.Y, coin4.Y), 0, TAILLE_CARTE);
#else
		// Prend en compte les 4 coins du quadrilatère des index
		// On force aussi les coins à se retrouver dans le terrain, pour éviter que certains bâtiments se retrouvent en-dehors du terrain.
		const int minX = core::clamp(core::min_(min(coin1.X, coin2.X), coin3.X, coin4.X), 0, TAILLE_CARTE);
		const int minY = core::clamp(core::min_(min(coin1.Y, coin2.Y), coin3.Y, coin4.Y), 0, TAILLE_CARTE);
		const int maxX = core::clamp(core::max_(max(coin1.X, coin2.X), coin3.X, coin4.X), 0, TAILLE_CARTE);
		const int maxY = core::clamp(core::max_(max(coin1.Y, coin2.Y), coin3.Y, coin4.Y), 0, TAILLE_CARTE);
#endif

		// Force la zone sélectionnée à être un rectangle en forçant chaque coin suivant les valeurs min et max en X et en Y

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
		// Prend en compte les 4 coins du quadrilatère des index
		const int minX = core::min_(min(coin1.X, coin2.X), coin3.X, coin4.X);
		const int minY = core::min_(min(coin1.Y, coin2.Y), coin3.Y, coin4.Y);
		const int maxX = core::max_(max(coin1.X, coin2.X), coin3.X, coin4.X);
		const int maxY = core::max_(max(coin1.Y, coin2.Y), coin3.Y, coin4.Y);

		// Tourne les coins pour qu'ils correspondent à leurs caractéristiques (ne fonctionne que si la zone sélectionnée est déjà un rectangle !)
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
			else if (coin1 == tmpCoin) // Le coin 1 est déjà le bon, on ne fait rien
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
			else if (coin2 == tmpCoin) // Le coin 2 est déjà le bon, on ne fait rien
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
			else if (coin3 == tmpCoin) // Le coin 3 est déjà le bon, on ne fait rien
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
			else if (coin4 == tmpCoin) // Le coin 4 est déjà le bon, on ne fait rien
			{ }
			else
				LOG_DEBUG("Game::previewCreateRectBats : Impossible de determiner le coin 4 : Aucun coin ne correspond aux conditions requises", ELL_WARNING);
#endif
		}
#endif
	}

	// En mode création rectangulaire, on force les rotations des bâtiments à être des multiples de 90°
	currentBatimentRotation = core::round_((currentBatimentRotation / 90.0f) - 0.05f) * 90.0f;	// -0.05f à l'arrondi pour qu'un angle de 45° (et jusqu'à 49.5°) soit arrondi à 0° et non pas à 90°

	// Vérifie que les coins ou d'autres paramêtres ont bien changé depuis le dernier appel
	if (!lastUpdateParams.hasConstructRectCornersChanged(game->system.getTime().getTotalJours(), batimentID, currentBatimentRotation, coin1,
#if !defined(FORCE_RECTANGLE) || !defined(ONLY_TAKE_MAIN_CORNERS)
		coin2, coin3,
#endif
		coin4))
		return;

	// Stoppe le timer pour éviter que le temps de jeu ne se déroule pendant le placement de ces bâtiments
	const bool wasTimerStopped = !(game->deviceTimer->isStopped());
	if (wasTimerStopped)
		game->deviceTimer->stop();

	// Met à jour les coins et d'autres paramêtres pour le dernier appel à cette fonction
	lastUpdateParams.systemTotalDays = game->system.getTime().getTotalJours();
	lastUpdateParams.batimentID = batimentID;
	lastUpdateParams.batimentRotation = currentBatimentRotation;
	lastUpdateParams.pointedFirstCorner = coin1;
#if !defined(FORCE_RECTANGLE) || !defined(ONLY_TAKE_MAIN_CORNERS)
	lastUpdateParams.pointedSecondCorner = coin2;
	lastUpdateParams.pointedThirdCorner = coin3;
#endif
	lastUpdateParams.pointedLastCorner = coin4;

	// Réinitialise la GUI dûe à la prévisualisation des batiments
	resetPreviewErrors(moveBatiment);

	// Variable d'itération utilisée pour parcourir la liste des batiments de prévisualisation déjà créés
	u32 i = 0;
	const u32 previewRectSize = listeBatimentsPreviewRect.size();
	if (!sameID)
	{
		// Détruit tous les batiments de prévisualisation précedemment créés si leur ID est différent de l'ID des batiments à créer
		for (i = 0; i < previewRectSize; ++i)
		{
			scene::ISceneNode* const batiment = listeBatimentsPreviewRect[i];
			batiment->setVisible(false);
			game->sceneManager->addToDeletionQueue(batiment);

			// Si PostProcess est activé, on lui indique qu'il ne doit plus prendre en compte ce bâtiment dans le flou de profondeur
			if (game->postProcessManager)
				game->postProcessManager->removeNodeFromDepthPass(batiment);

			// Si XEffects est activé, on lui indique qu'il ne doit plus afficher l'ombre de ce bâtiment
			if (game->xEffects)
				game->xEffects->removeShadowFromNode(batiment);
		}
		listeBatimentsPreviewRect.clear();
	}
	else
	{
		// Remet tous les batiments de prévisualisation à leur couleur normale et les masque
		for (i = 0; i < previewRectSize; ++i)
		{
			scene::ISceneNode* const batiment = listeBatimentsPreviewRect[i];
			Game::resetBatimentColor(batiment);
			batiment->setVisible(false);
		}
	}

	// Obtient la taille du bâtiment :
	// Si le bâtiment n'est pas tourné d'un multiple de 180° (dans tous les cas, la rotation des bâtiments a été forcée comme multiple de 90°),
	// alors il est perpendiculaire à l'axe des abscisses, et on inverse donc ses informations de taille :
	const core::dimension2du& batimentRealSize = StaticBatimentInfos::getInfos(batimentID).taille;
	const bool swapSize = core::equals(fmodf(currentBatimentRotation, 180.0f), 90.0f);	// Si currentBatimentRotation % 180.0f == 90.0f
	const core::dimension2du batimentTaille(swapSize ? batimentRealSize.Height : batimentRealSize.Width, swapSize ? batimentRealSize.Width : batimentRealSize.Height);

#ifndef FORCE_RECTANGLE	// TODO : A réoptimiser si utilisé !
	const int longueurX = coin2.X - coin1.X;
	const int largeurY = coin3.Y - coin1.Y;
	const int ecartX = coin4.X - coin2.X;
	const int ecartY = coin4.Y - coin3.Y;

	// Compte le nombre de bâtiments à créer
	// TODO : Trouver une fonction plus efficace que l'incrémentation à l'intérieur des boucles
	nbBatimentsPreviewRect = 0;

	float ecartXParUniteY = (float)ecartX / (float)largeurY;
	float ecartYParUniteX = (float)ecartY / (float)longueurX;

	// Affiche les batiments contenus entre les 4 coins du parallélogramme de sélection
	bool longueurNegative = (longueurX < 0);
	bool largeurNegative = (largeurY < 0);
	i = 0;
	for (int x = 0; abs(x) <= abs(longueurX); longueurNegative ? x -= batimentTaille.Width : x += batimentTaille.Width)
	{
		for (int y = 0; abs(y) <= abs(largeurY); largeurNegative ? y -= batimentTaille.Height : y += batimentTaille.Height)
		{
			// Le batiment à placer
			scene::ISceneNode* bat = NULL;

			// Si des batiments sont encore disponibles, on les utilise pour la prévisualisation actuelle
			const u32 previewRectSizeUpdated = listeBatimentsPreviewRect.size();
			if (i < previewRectSizeUpdated)
			{
				bat = listeBatimentsPreviewRect[i];
				++i;
			}
			else
			{
				// Crée un nouveau batiment de ce type
				bat = game->renderer->loadBatiment(batimentID);
				listeBatimentsPreviewRect.push_back(bat);
				i = previewRectSizeUpdated + 1;		// Evite de réutiliser ce batiment pour le placement
			}

			if (bat)
			{
				bat->setVisible(true);

				// Place ce batiment à sa position pour sa prévisualisation
				previewCreateSimpleBat(batimentID, bat, core::vector2di(
					x + coin1.X + core::round32(ecartXParUniteY * (float)y),
					y + coin1.Y + core::round32(ecartYParUniteX * (float)x)), false);
			}

			++nbBatimentsPreviewRect;	// Incrémente le nombre de bâtiments à créer
		}
	}
#else
	const int longueurX = coin4.X - coin1.X;	// longueurX est toujours positif, car coin1.X = min(coin1.X, coin4.X) et coin4.X = max(coin1.X, coin4.X)
	const int largeurY = coin4.Y - coin1.Y;		// largeurY est toujours positif,  car coin1.Y = min(coin1.Y, coin4.Y) et coin4.Y = max(coin1.Y, coin4.Y)

	// Compte le nombre de bâtiments à créer
	nbBatimentsPreviewRect = ((((u32)longueurX) / batimentTaille.Width) + 1) * ((((u32)largeurY) / batimentTaille.Height) + 1);

	// Vérifie que le nombre de bâtiments à créer n'est pas trop grand, sinon on ne les crée pas
	if (nbBatimentsPreviewRect > 200)
	{
		// Redémarre le timer du jeu
		if (wasTimerStopped)
			game->deviceTimer->start();

		LOG_DEBUG("Game::previewCreateRectBats : Trop de batiments en rectangle a placer :" << endl
			<< "                                 nbBatimentsPreviewRect = " << nbBatimentsPreviewRect << "     => Abandon", ELL_WARNING);
		return;
	}

	// Vérifie que la liste listeBatimentsPreviewRect est assez grande, sinon on lui indique quelle place est nécessaire
	if (listeBatimentsPreviewRect.allocated_size() < nbBatimentsPreviewRect)
		listeBatimentsPreviewRect.reallocate(nbBatimentsPreviewRect);

	// Affiche les batiments contenus entre les 4 coins du rectangle de sélection
	i = 0;
	for (int x = 0; x <= longueurX; x += batimentTaille.Width)
	{
		for (int y = 0; y <= largeurY; y += batimentTaille.Height)
		{
			// Le batiment à placer
			scene::ISceneNode* bat = NULL;

			// Si des batiments sont encore disponibles, on les utilise pour la prévisualisation actuelle
			const u32 previewRectSizeUpdated = listeBatimentsPreviewRect.size();
			if (i < previewRectSizeUpdated)
			{
				bat = listeBatimentsPreviewRect[i];
				++i;
			}
			else
			{
				// Crée un nouveau batiment de ce type
				bat = game->renderer->loadBatiment(batimentID, NULL, false, true);
				listeBatimentsPreviewRect.push_back(bat);
				i = previewRectSizeUpdated + 1;		// Evite de réutiliser ce batiment pour le placement (on en créera un nouveau à la nouvelle itération)
			}

			if (bat)
			{
				bat->setVisible(true);

				// Place ce nouveau batiment à sa position pour sa prévisualisation
				previewCreateSimpleBat(batimentID, bat, core::vector2di(x + coin1.X, y + coin1.Y));
			}
		}
	}
#endif

	// Détruit tous les batiments de prévisualisation non utilisés (ils sont encore masqués)
	const u32 previewRectSizeUpdated = listeBatimentsPreviewRect.size();
	u32 firstIndex = previewRectSizeUpdated;
	for (i = 0; i < previewRectSizeUpdated; ++i)
	{
		// Vérifie si ce batiment est masqué
		if (!listeBatimentsPreviewRect[i]->isVisible())
		{
			// S'il est masqué, on le supprime et on note l'index du premier élement masqué
			scene::ISceneNode* const batiment = listeBatimentsPreviewRect[i];
			game->sceneManager->addToDeletionQueue(batiment);
			firstIndex = min(firstIndex, i);

			// Si PostProcess est activé, on lui indique qu'il ne doit plus prendre en compte ce bâtiment dans le flou de profondeur
			if (game->postProcessManager)
				game->postProcessManager->removeNodeFromDepthPass(batiment);

			// Si XEffects est activé, on lui indique qu'il ne doit plus afficher l'ombre de ce bâtiment
			if (game->xEffects)
				game->xEffects->removeShadowFromNode(batiment);
		}
	}

	// Supprime tous les élements masqués de la liste (ils le sont tous à partir du rang firstIndex)
	if (firstIndex < previewRectSizeUpdated)
		listeBatimentsPreviewRect.erase(firstIndex, previewRectSizeUpdated - firstIndex);

	// Vérifie les erreurs de création dûes au nombre multiple de bâtiments
	bool outRessources[Ressources::RI_COUNT];
	const int error = game->system.canCreateMultiBatiments(batimentID, nbBatimentsPreviewRect, outRessources);

	// Affiche les messages à l'utilisateur indiquant pourquoi on n'a pas pu placer tous les bâtiments
	showRectCreationErrors(NULL, &listeBatimentsPreviewRect, error, outRessources);

	// Redémarre le timer du jeu
	if (wasTimerStopped)
		game->deviceTimer->start();
}
void BatimentSelector::PlacerBatiment::createRectBats(BatimentID batimentID)
{
	// Vérifie que le rectangle de sélection est bien valide, qu'on a accès au terrain, à son triangle selector et à la caméra
	if (rectSelection == core::recti(-1, -1, -1, -1) || !game->renderer)
		return;
	scene::ICameraSceneNode* const camera = game->sceneManager->getActiveCamera();
	if (!game->renderer->getTerrainManager().getTerrainTriangleSelector() || !camera)
		return;

	// Les 4 coins des indices correspondant au rectangle de sélection actuel
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
		 \     \   |  largeur Y (+ écart Y)
		  \     \  |
		   3 --- 4 \/
		<->
		écart X
	*/

	// Obtient les coins des indices
	{
		core::vector3df position;

		// Coin 1
		{
			// Obtient la position du terrain pointée par la souris
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
			// Obtient la position du terrain pointée par la souris
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
			// Obtient la position du terrain pointée par la souris
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
			// Obtient la position du terrain pointée par la souris
			position = getPointedTerrainPos(rectSelection.LowerRightCorner);

			// La position est en dehors du terrain si : position == core::vector3df(-1.0f)
			if (position != core::vector3df(-1.0f))
			{
				// Obtient le premier indice suivant la position actuelle du point de collision
				coin4 = EcoWorldSystem::getIndexFromPosition(position);
			}
		}
	}

	// Vérifie que tous les coins sont valides, sinon on quitte
	// On vérifie aussi qu'au moins un des deux points est dans le terrain, pour éviter d'afficher des bâtiments en-dehors du terrain.
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

	// Stoppe le timer pour éviter que le temps de jeu ne se déroule pendant le placement de ces bâtiments
	const bool wasTimerStopped = !(game->deviceTimer->isStopped());
	if (wasTimerStopped)
		game->deviceTimer->stop();

	// Tourne les coins pour qu'ils correspondent à leurs caractéristiques
	{
#ifdef FORCE_RECTANGLE
#ifdef ONLY_TAKE_MAIN_CORNERS
		// Ne prend en compte que les coins 1 et 4, sinon on se retrouve généralement avec un rectangle des index trop grand
		// On force aussi les coins à se retrouver dans le terrain, pour éviter que certains bâtiments se retrouvent en-dehors du terrain.
		const int minX = core::clamp(min(coin1.X, coin4.X), 0, TAILLE_CARTE);
		const int minY = core::clamp(min(coin1.Y, coin4.Y), 0, TAILLE_CARTE);
		const int maxX = core::clamp(max(coin1.X, coin4.X), 0, TAILLE_CARTE);
		const int maxY = core::clamp(max(coin1.Y, coin4.Y), 0, TAILLE_CARTE);
#else
		// Prend en compte les 4 coins du quadrilatère des index
		// On force aussi les coins à se retrouver dans le terrain, pour éviter que certains bâtiments se retrouvent en-dehors du terrain.
		const int minX = core::clamp(core::min_(min(coin1.X, coin2.X), coin3.X, coin4.X), 0, TAILLE_CARTE);
		const int minY = core::clamp(core::min_(min(coin1.Y, coin2.Y), coin3.Y, coin4.Y), 0, TAILLE_CARTE);
		const int maxX = core::clamp(core::max_(max(coin1.X, coin2.X), coin3.X, coin4.X), 0, TAILLE_CARTE);
		const int maxY = core::clamp(core::max_(max(coin1.Y, coin2.Y), coin3.Y, coin4.Y), 0, TAILLE_CARTE);
#endif

		// Force la zone sélectionnée à être un rectangle en forçant chaque coin suivant les valeurs min et max en X et en Y

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
		// Prend en compte les 4 coins du quadrilatère des index
		const int minX = core::min_(min(coin1.X, coin2.X), coin3.X, coin4.X);
		const int minY = core::min_(min(coin1.Y, coin2.Y), coin3.Y, coin4.Y);
		const int maxX = core::max_(max(coin1.X, coin2.X), coin3.X, coin4.X);
		const int maxY = core::max_(max(coin1.Y, coin2.Y), coin3.Y, coin4.Y);

		// Tourne les coins pour qu'ils correspondent à leurs caractéristiques (ne fonctionne que si la zone sélectionnée est déjà un rectangle !)
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
			else if (coin1 == tmpCoin) // Le coin 1 est déjà le bon, on ne fait rien
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
			else if (coin2 == tmpCoin) // Le coin 2 est déjà le bon, on ne fait rien
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
			else if (coin3 == tmpCoin) // Le coin 3 est déjà le bon, on ne fait rien
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
			else if (coin4 == tmpCoin) // Le coin 4 est déjà le bon, on ne fait rien
			{ }
			else
				LOG_DEBUG("Game::createRectBats : Impossible de determiner le coin 4 : Aucun coin ne correspond aux conditions requises", ELL_WARNING);
#endif
		}
#endif
	}

	// En mode création rectangulaire, on force les rotations des bâtiments à être des multiples de 90°
	currentBatimentRotation = core::round_((currentBatimentRotation / 90.0f) - 0.05f) * 90.0f;	// -0.05f à l'arrondi pour qu'un angle de 45° (et jusqu'à 49.5°) soit arrondi à 0° et non pas à 90°

	// Obtient la taille du bâtiment :
	// Si le bâtiment n'est pas tourné d'un multiple de 180° (dans tous les cas, la rotation des bâtiments a été forcée comme multiple de 90°),
	// alors il est perpendiculaire à l'axe des abscisses, et on inverse donc ses informations de taille :
	const core::dimension2du& batimentRealSize = StaticBatimentInfos::getInfos(batimentID).taille;
	const bool swapSize = core::equals(fmodf(currentBatimentRotation, 180.0f), 90.0f);	// Si currentBatimentRotation % 180.0f == 90.0f
	const core::dimension2du batimentTaille(swapSize ? batimentRealSize.Height : batimentRealSize.Width, swapSize ? batimentRealSize.Width : batimentRealSize.Height);

	// Compte le nombre de bâtiments à créer :
#ifndef FORCE_RECTANGLE	// TODO : A réoptimiser si utilisé !
	const int longueurX = coin2.X - coin1.X;
	const int largeurY = coin3.Y - coin1.Y;

	// Compte le nombre de bâtiments à créer
	// TODO : Trouver une fonction plus efficace
	nbBatimentsPreviewRect = 0;
	for (int x = 0; abs(x) <= abs(longueurX); longueurNegative ? x -= batimentTaille.Width : x += batimentTaille.Width)
		for (int y = 0; abs(y) <= abs(largeurY); largeurNegative ? y -= batimentTaille.Height : y += batimentTaille.Height)
			++nbBatimentsPreviewRect;
#else
	const int longueurX = coin4.X - coin1.X;	// longueurX est toujours positif, car coin1.X = min(coin1.X, coin4.X) et coin4.X = max(coin1.X, coin4.X)
	const int largeurY = coin4.Y - coin1.Y;		// largeurY est toujours positif,  car coin1.Y = min(coin1.Y, coin4.Y) et coin4.Y = max(coin1.Y, coin4.Y)

	// Compte le nombre de bâtiments placés dans ce rectangle
	nbBatimentsPreviewRect = ((((u32)longueurX) / batimentTaille.Width) + 1) * ((((u32)largeurY) / batimentTaille.Height) + 1);
#endif

	// Vérifie que le nombre de bâtiments à créer n'est pas trop grand, sinon on ne les crée pas :
	// Evite ainsi de perdre la partie en cours en cas de manque de mémoire : 200 arbres consomment environ 30 Mo de mémoire vive !
	if (nbBatimentsPreviewRect > 200)
	{
		// Redémarre le timer du jeu
		if (wasTimerStopped)
			game->deviceTimer->start();

		// Affiche un message d'erreur à l'utilisateur
		swprintf_SS(L"Placement des bâtiments annulé :\r\nLe nombre de bâtiments à placer est trop grand : %u bâtiments !", nbBatimentsPreviewRect);
		game->showErrorMessageBox(L"Avertissement", textW_SS);

		return;
	}

	// Vérifie les erreurs de création dûes au nombre multiple de bâtiments
	bool outRessources[Ressources::RI_COUNT];
	const int error = game->system.canCreateMultiBatiments(batimentID, nbBatimentsPreviewRect, outRessources);

	// Détermine si cette erreur est une erreur fatale
	const bool fatalError = (error != EBMCE_AUCUNE && error != EBMCE_ENERGIE);

	// Détruit tous les batiments de prévisualisation précedemment créés
	const u32 previewRectSize = listeBatimentsPreviewRect.size();
	for (u32 i = 0; i < previewRectSize; ++i)
	{
		scene::ISceneNode* const batiment = listeBatimentsPreviewRect[i];
		batiment->setVisible(false);
		game->sceneManager->addToDeletionQueue(batiment);

		// Si PostProcess est activé, on lui indique qu'il ne doit plus prendre en compte ce bâtiment dans le flou de profondeur
		if (game->postProcessManager)
			game->postProcessManager->removeNodeFromDepthPass(batiment);

		// Si XEffects est activé, on lui indique qu'il ne doit plus afficher l'ombre de ce bâtiment
		if (game->xEffects)
			game->xEffects->removeShadowFromNode(batiment);
	}
	listeBatimentsPreviewRect.clear();

#ifndef FORCE_RECTANGLE	// A réoptimiser si utilisé !
	const int ecartX = coin4.X - coin2.X;
	const int ecartY = coin4.Y - coin3.Y;
	float ecartXParUniteY = (float)ecartX / (float)largeurY;
	float ecartYParUniteX = (float)ecartY / (float)longueurX;

	// Crée les batiments entre les 4 coins du parallélogramme de sélection
	bool longueurNegative = (longueurX < 0);
	bool largeurNegative = (largeurY < 0);
	for (int x = 0; abs(x) <= abs(longueurX); longueurNegative ? x -= batimentTaille.Width : x += batimentTaille.Width)
	{
		for (int y = 0; abs(y) <= abs(largeurY); largeurNegative ? y -= batimentTaille.Height : y += batimentTaille.Height)
		{
			const core::vector2di index(
				x + coin1.X + core::round32(ecartXParUniteY * (float)y),
				y + coin1.Y + core::round32(ecartYParUniteX * (float)x));

			if (!fatalError)	// Si ce n'est pas une erreur fatale, on essaie de placer ce bâtiment
				createSimpleBat(batimentID, index);
			else
			{
				// Sinon on affiche les erreurs spécifiques qu'aurait rencontré le bâtiment si on l'avait placé ici :

				// Obtient les erreurs qu'aurait renvoyé le système si on avait essayé de placer le bâtiment ici
				int errorBat = game->system.canCreateBatiment(batimentID, index, currentBatimentRotation, NULL);

				// Evite d'afficher certaines erreurs gérées dans la prévisualisation en rectangle :
				// Budget insuffisant, Energie insuffisante et Ressources insuffisantes
				errorBat &= ~(EBCE_BUDGET | EBCE_ENERGIE | EBCE_RESSOURCE); // Passe les bits représentés par EBCE_BUDGET, EBCE_ENERGIE et EBCE_RESSOURCE à 0 (~ : opérateur NOT bit à bit)

				// Affiche les erreurs de création de ce bâtiment :
				if (errorBat != EBCE_AUCUNE)
				{
					// Calcule la position de ce bâtiment
					core::vector3df batPos = EcoWorldSystem::getPositionFromIndex(index);

					// Obtient la position du terrain en Y
					batPos.Y = game->renderer->getTerrainHeight(batPos.X, batPos.Z);

					// Interdit à la position de tomber en-dessous du niveau de l'eau si elle est visible
					if (game->renderer->getTerrainManager().getTerrainInfos().water.visible)
						batPos.Y = max(game->renderer->getTerrainManager().getTerrainInfos().water.height, batPos.Y);

					// Ajoute un peu de hauteur au bâtiment pour éviter les effets désagréables avec le sol
					batPos.Y += HAUTEUR_AJOUTEE_BATIMENTS;

					showSimpleCreationErrors(&batPos, NULL, errorBat, NULL);
				}
			}
		}
	}
#else
	// Crée les batiments entre les 4 coins du rectangle de sélection
	for (int x = 0; x <= longueurX; x += batimentTaille.Width)
	{
		for (int y = 0; y <= largeurY; y += batimentTaille.Height)
		{
			const core::vector2di index(x + coin1.X, y + coin1.Y);

			if (!fatalError)	// Si ce n'est pas une erreur fatale, on essaie de placer ce bâtiment
				createSimpleBat(batimentID, index);
			else
			{
				// Sinon on affiche les erreurs spécifiques qu'aurait rencontré le bâtiment si on l'avait placé ici :

				// Obtient les erreurs qu'aurait renvoyé le système si on avait essayé de placer le bâtiment ici
				int errorBat = game->system.canCreateBatiment(batimentID, index, currentBatimentRotation, NULL);

				// Evite d'afficher certaines erreurs gérées dans la prévisualisation en rectangle :
				// Budget insuffisant, Energie insuffisante et Ressources insuffisantes
				errorBat &= ~(EBCE_BUDGET | EBCE_ENERGIE | EBCE_RESSOURCE); // Passe les bits représentés par EBCE_BUDGET, EBCE_ENERGIE et EBCE_RESSOURCE à 0 (~ : opérateur NOT bit à bit)

				// Affiche les erreurs de création de ce bâtiment :
				if (errorBat != EBCE_AUCUNE)
				{
					// Calcule la position de ce bâtiment
					core::vector3df batPos = EcoWorldSystem::getPositionFromIndex(index);

					// Obtient la position du terrain en Y
					batPos.Y = game->renderer->getTerrainHeight(batPos.X, batPos.Z);

					// Interdit à la position de tomber en-dessous du niveau de l'eau si elle est visible
					if (game->renderer->getTerrainManager().getTerrainInfos().water.visible)
						batPos.Y = max(game->renderer->getTerrainManager().getTerrainInfos().water.height, batPos.Y);

					// Ajoute un peu de hauteur au bâtiment pour éviter les effets désagréables avec le sol
					batPos.Y += HAUTEUR_AJOUTEE_BATIMENTS;

					showSimpleCreationErrors(&batPos, NULL, errorBat, NULL);
				}
			}
		}
	}
#endif

	// Affiche les messages à l'utilisateur indiquant pourquoi on n'a pas pu placer tous les bâtiments
	if (error != EBMCE_AUCUNE)
	{
		// Calcule la position moyenne des bâtiments :
		core::vector3df medBatimentPosition;

		// On calcule la position moyenne des bâtiments aux 2 coins opposés du rectangle
		{
			core::vector3df batPos;

			// Coin 1 :
			{
				// Calcule la position du bâtiment
				batPos = EcoWorldSystem::getPositionFromIndex(coin1);

				// Obtient la position du terrain en Y
				batPos.Y = game->renderer->getTerrainHeight(batPos.X, batPos.Z);

				// Interdit à la position de tomber en-dessous du niveau de l'eau si elle est visible
				if (game->renderer->getTerrainManager().getTerrainInfos().water.visible)
					batPos.Y = max(game->renderer->getTerrainManager().getTerrainInfos().water.height, batPos.Y);

				// Calcule la position moyenne des bâtiments
				medBatimentPosition += batPos;
			}

			// Coin 4 :
			{
				// Calcule la position du bâtiment
				batPos = EcoWorldSystem::getPositionFromIndex(coin4);

				// Obtient la position du terrain en Y
				batPos.Y = game->renderer->getTerrainHeight(batPos.X, batPos.Z);

				// Interdit à la position de tomber en-dessous du niveau de l'eau si elle est visible
				if (game->renderer->getTerrainManager().getTerrainInfos().water.visible)
					batPos.Y = max(game->renderer->getTerrainManager().getTerrainInfos().water.height, batPos.Y);

				// Calcule la position moyenne des bâtiments
				medBatimentPosition += batPos;
			}

			medBatimentPosition *= 0.5f;						// Fait la moyenne des deux coins
			medBatimentPosition += HAUTEUR_AJOUTEE_BATIMENTS;	// Dû à l'ajout de hauteur aux bâtiments pour éviter les effets désagréables avec le sol
		}

		// Affiche les erreurs de création des bâtiments : les messages seront créés à cette position moyenne
		showRectCreationErrors(&medBatimentPosition, NULL, error, outRessources);
	}

#ifdef USE_IRRKLANG
	// Joue le son de la création d'un bâtiment acceptée ou refusée
	if (!fatalError)
		ikMgr.playGameSound(IrrKlangManager::EGSE_BATIMENT_CREATE_POSITIVE);
	else
		ikMgr.playGameSound(IrrKlangManager::EGSE_BATIMENT_CREATE_NEGATIVE);
#endif

	// Redémarre le timer
	if (wasTimerStopped)
		game->deviceTimer->start();
}
void BatimentSelector::PlacerBatiment::showRectCreationErrors(const core::vector3df* messagesPosition, core::array<scene::ISceneNode*>* batiments, int error, bool outRessources[])
{
	if (error == EBMCE_AUCUNE)
		return;

	// TODO : Utiliser la sortie "outRessources" pour savoir quelles ressources sont manquantes

	// Obtient la transparence par défaut des textes
	const u32 textTransparency = game->guiManager->getTextTransparency();

#ifdef _DEBUG
	// Vérifie que l'erreur peut être traitée ici, sinon, on affiche un message
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
			// L'énergie est insuffisante : Affiche son texte en rouge
			game->guiManager->guiElements.gameGUI.energieTexte->setOverrideColor(video::SColor(textTransparency, 255, 50, 50));
		}
		else
		{
			// L'énergie est suffisante : Affiche son texte en blanc
			game->guiManager->guiElements.gameGUI.energieTexte->setOverrideColor(video::SColor(textTransparency, 255, 255, 255));
		}
	}
#ifdef _DEBUG
	if (((error & EBMCE_BATIMENT_ID) != 0))
		LOG_DEBUG("Game::showRectCreationErrors : Impossible d'ajouter le batiment au systeme de jeu car l'ID du batiment n'est pas valide : error = " << error, ELL_WARNING);
#endif

	// Affiche les messages à l'utilisateur indiquant pourquoi on n'a pas pu placer le bâtiment
	if (messagesPosition)
	{
		core::stringw text = "";
		if ((error & EBMCE_BUDGET) != 0)
			text.append(L"Budget insuffisant\n");
		//if ((error & EBMCE_ENERGIE) != 0)				// Le manque d'énergie n'est plus une erreur fatale pour pouvoir placer des bâtiments : on se contente d'afficher son texte en rouge dans ce cas
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
	// Remet le batiment à sa couleur normale
	Game::resetBatimentColor(batiment);

	// Remet le dernier batiment sélectionné pour la destruction à sa couleur normale et masque son texte
	// Désactivé : Permet de conserver le bâtiment pointé pour la destruction rouge, pour savoir quel bâtiment est décrit dans la fenêtre d'informations
	//resetBatimentColor(pointedToDestroyBatiment);	pointedToDestroyBatiment = NULL;
	//pointedToDestroyBatiment->setNomBatimentVisible(indeterminate);
	//pointedToDestroyBatiment = NULL;

	// Masque le batiment de prévisualisation
	if (moveBatiment)
		moveBatiment->setVisible(false);

	// Restaure le bâtiment qui était à la même position que le bâtiment de prévisualisation
	if (samePosBatiment)
	{
		Game::resetBatimentColor(samePosBatiment);
		samePosBatiment = NULL;
	}

	// Les textes sont remis à leur couleur normale dans la fonction Game::updateGameGUI() (-> seulement si on n'est pas en train de construire/détruire un bâtiment !)
}
void BatimentSelector::PlacerBatiment::showSimpleCreationErrors(const core::vector3df* messagesPosition, scene::ISceneNode* batiment, int error, bool outRessources[])
{
	if (error == EBCE_AUCUNE)
		return;

	// TODO : Utiliser la sortie "outRessources" pour savoir quelles ressources sont manquantes

	// Obtient la transparence par défaut des textes
	const u32 textTransparency = game->guiManager->getTextTransparency();

#ifdef _DEBUG
	// Vérifie que l'erreur peut être traitée ici, sinon, on affiche un message
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
		// On ne peut pas placer le batiment à cet endroit ou on n'a pas les ressources ou la population suffisantes :
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
			// L'énergie est insuffisante : Affiche son texte en rouge
			game->guiManager->guiElements.gameGUI.energieTexte->setOverrideColor(video::SColor(textTransparency, 255, 50, 50));
		}
		else
		{
			// L'énergie est suffisante : Affiche son texte en blanc
			game->guiManager->guiElements.gameGUI.energieTexte->setOverrideColor(video::SColor(textTransparency, 255, 255, 255));
		}
	}
#ifdef _DEBUG
	if (((error & EBCE_BATIMENT_ID) != 0))
		LOG_DEBUG("Game::showSimpleCreationErrors : Impossible d'ajouter le batiment au systeme de jeu car l'ID du batiment n'est pas valide : error = " << error, ELL_WARNING);
#endif

	// Affiche les messages à l'utilisateur indiquant pourquoi on n'a pas pu placer le bâtiment
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
			text.append(L"Nécessite de l'eau profonde\r\n");
		if ((error & EBCE_BUDGET) != 0)
			text.append(L"Budget insuffisant\r\n");
		//if ((error & EBCE_ENERGIE) != 0)				// Le manque d'énergie n'est plus une erreur fatale pour pouvoir placer un bâtiment : on se contente d'afficher son texte en rouge dans ce cas
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

	// Obtient la transparence par défaut des textes
	const u32 textTransparency = game->guiManager->getTextTransparency();

#ifdef _DEBUG
	// Vérifie que l'erreur peut être traitée ici, sinon, on affiche un message
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

	// Affiche les messages à l'utilisateur indiquant pourquoi on n'a pas pu détruire le bâtiment
	if (messagesPosition)
	{
		core::stringw text = "";
		if ((error & EBDE_BUDGET) != 0)
			text.append(L"Budget insuffisant\r\n");
		if ((error & EBDE_RESSOURCE) != 0)
			text.append(L"Ressources insuffisantes\r\n");
		if ((error & EBDE_ALREADY_DESTRUCTING) != 0)
			text.append(L"Destruction déjà commencée\r\n");

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
	// Vérifie qu'on a accès au triangle selector des bâtiments et à la caméra, qu'on n'est pas en train de créer ou détruire un bâtiment
	// et que des paramêtres ont changé depuis le dernier appel
	scene::ICameraSceneNode* const camera = game->sceneManager->getActiveCamera();
	if (!game->renderer->getBatimentsTriangleSelector() || !camera)
		return;
	if (game->guiManager->guiElements.gameGUI.detruireBouton && game->guiManager->guiElements.gameGUI.detruireBouton->isPressed())
		return;
	if (!lastUpdateParams.hasParamsChanged(game->mouseState.position, camera->getAbsolutePosition()))
		return;

	// Met à jour les paramêtres pour le dernier appel à cette fonction
	lastUpdateParams.mousePos = game->mouseState.position;
	lastUpdateParams.camPos = camera->getAbsolutePosition();

	// Obtient le bâtiment actuellement pointé
	CBatimentSceneNode* const newPointedBatiment = Game::getBatimentNode(getPointedBat());

	// Si le dernier bâtiment pointé est le même que celui-ci, on annule
	if (pointedBatiment == newPointedBatiment)
		return;

	// Restaure le dernier bâtiment pointé s'il est valide et masque son texte
	if (pointedBatiment != selectedBatiment && pointedBatiment)
	{
		Game::resetBatimentColor(pointedBatiment);
		pointedBatiment->setNomBatimentVisible(false);
	}

	// Montre le nouveau bâtiment pointé et affiche son nom s'il est valide et qu'il est différent du bâtiment actuellement sélectionné
	if (newPointedBatiment != selectedBatiment && newPointedBatiment)
	{
		Game::changeBatimentColor(newPointedBatiment, video::SColor(255, 255, 255, 0));	// Pointé : Jaune
		newPointedBatiment->setNomBatimentVisible(true);
	}

	// Indique le dernier bâtiment pointé
	pointedBatiment = newPointedBatiment;
}
void BatimentSelector::SelectBatiment::selectBatiment()
{
	// Vérifie qu'on a accès au triangle selector des bâtiments et à la caméra
	scene::ICameraSceneNode* const camera = game->sceneManager->getActiveCamera();
	if (!game->renderer->getBatimentsTriangleSelector() || !camera)
		return;

	// Sélectionne le bâtiment actuellement pointé
	CBatimentSceneNode* const newSelectedBatiment = Game::getBatimentNode(getPointedBat());

	// Si le bâtiment actuellement sélectionné est le même que celui-ci, on annule
	if (selectedBatiment == newSelectedBatiment)
		return;

	// Si le bâtiment actuellement sélectionné est le même que le dernier bâtiment pointé, on oublie le dernier bâtiment pointé
	if (newSelectedBatiment == pointedBatiment && pointedBatiment)
		pointedBatiment = NULL;

	// Restaure le dernier bâtiment sélectionné s'il est valide
	if (selectedBatiment)
	{
		Game::resetBatimentColor(selectedBatiment);
		selectedBatiment->setNomBatimentVisible(false);
	}

	if (newSelectedBatiment)	// On a sélectionné un nouveau bâtiment
	{
		// Montre le nouveau bâtiment sélectionné s'il est valide
		Game::changeBatimentColor(newSelectedBatiment, video::SColor(255, 0, 255, 0));	// Sélectionné : Vert
		newSelectedBatiment->setNomBatimentVisible(true);

#ifdef USE_IRRKLANG
		// Joue le son de la sélection d'un bâtiment
		ikMgr.playGameSound(IrrKlangManager::EGSE_BATIMENT_SELECTED);
#endif
	}
#ifdef USE_IRRKLANG
	else						// Si on n'a pas sélectionné de nouveau bâtiment, alors c'est qu'on a déselectionné le précédent
	{
		// Joue le son de la déselection d'un bâtiment
		ikMgr.playGameSound(IrrKlangManager::EGSE_BATIMENT_DESELECTED);
	}
#endif

	// Indique le scene node actuellement sélectionné
	selectedBatiment = newSelectedBatiment;

	// Réinitialise tous les paramêtres de frameUpdate()
	lastUpdateParams.reset();
}
void BatimentSelector::SelectBatiment::deselectBatiment()
{
	// Restaure le dernier bâtiment pointé s'il est valide et différent du bâtiment sélectionné
	if (pointedBatiment != selectedBatiment && pointedBatiment)
	{
		Game::resetBatimentColor(pointedBatiment);
		pointedBatiment->setNomBatimentVisible(false);
	}

	// Restaure le bâtiment sélectionné s'il est valide
	if (selectedBatiment)
	{
		Game::resetBatimentColor(selectedBatiment);
		selectedBatiment->setNomBatimentVisible(false);

#ifdef USE_IRRKLANG
		// Joue le son de la déselection d'un bâtiment
		ikMgr.playGameSound(IrrKlangManager::EGSE_BATIMENT_DESELECTED);
#endif
	}

	// Déselectionne le bâtiment et oublie le bâtiment pointé
	selectedBatiment = NULL;
	pointedBatiment = NULL;
}
