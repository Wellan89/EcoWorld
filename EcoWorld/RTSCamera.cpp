#include "RTSCamera.h"
#include "Game.h"
#include "EcoWorldRenderer.h"
#include "CSunSceneNode.h"

#define ECART_COTE_SOURIS_BOUGER	0.01f	// La distance de la souris par rapport aux cot�s de la fen�tre pour faire bouger la camera RTS

RTSCamera::RTSCamera(ISceneNode* parent, int id, float rs, float zStep, float zs, float ts, scene::ICameraSceneNode* m_FPSCamera)
 : ICameraSceneNode(parent, game->sceneManager, id, vector3df(1.0f), vector3df(0.0f), vector3df(1.0f)), lastTimeMs(0), minZoom(0.0f), maxZoom(0.0f), lockZoom(false),
 minRotateX(0.0f), maxRotateX(0.0f), lockRotateX(false), minRotateY(0.0f), maxRotateY(0.0f), lockRotateY(false), zoomStep(zStep), wantedZoom(0.0f), lockCamera(false),
 terrainLimits(-1000.0f, -1000.0f, 1000.0f, 1000.0f), FPSCamera(m_FPSCamera), canBecomeFPSCamera(false), collisionAnim(NULL), collisionWorld(NULL),
 isCameraFPSEnabled(false), UpVector(0.0f, 1.0f, 0.0f), Fovy(core::PI * 0.4f), Aspect(4.0f / 3.0f), rotX(0), rotY(0), lastRealTimeMs(0),
 ZNear(CAMERA_NEAR_VALUE), ZFar(CAMERA_FAR_VALUE), rotating(false), moving(false), translating(false), zoomSpeed(zs), rotateSpeed(rs), translateSpeed(ts)
{
	const core::dimension2du& currentRenderTargetSize = game->driver->getCurrentRenderTargetSize();
	Aspect = (float)currentRenderTargetSize.Width / (float)currentRenderTargetSize.Height;

	lastTimeMs = game->deviceTimer->getTime();
	beginTimeMs = lastRealTimeMs = game->deviceTimer->getRealTime();

	setTarget(core::vector3df(0.0f, 10.0f, 0.0f));

	// Ajoute la cam�ra FPS � nos enfants si elle est valide : cela permet de contr�ler ses mises � jour dans OnAnimate
	if (m_FPSCamera)
	{
		m_FPSCamera->setParent(this);

		canBecomeFPSCamera = true;
		isCameraFPSEnabled = false;
	}

	recalculateProjectionMatrix();
	recalculateViewArea();

	game->sceneManager->setActiveCamera(this);

	setZoomLock();
	setRotateXLock();
	setRotateYLock();
}
RTSCamera::~RTSCamera()
{
	if (collisionAnim)
		collisionAnim->drop();
}
bool RTSCamera::OnEvent(const SEvent& event)
{
	if (!isCameraFPSEnabled && !lockCamera)
	{
		if (event.EventType == EET_MOUSE_INPUT_EVENT)
		{
			if (event.MouseInput.Event == EMIE_MOUSE_WHEEL)
			{
				// On ne g�re pas l'�venement si la touche Control est appuy�e :
				// cela veut dire que l'utilisateur ne veut pas zoomer mais faire une rotation de l'objet s�lectionn�
				if (!event.MouseInput.Control)
				{
					wantedZoom -= event.MouseInput.Wheel * zoomStep;

					// Arrondi wantedZoom � zoomStep
					wantedZoom = core::round_(wantedZoom / zoomStep) * zoomStep;

					if (lockZoom)
					{
						if (wantedZoom < minZoom)
							wantedZoom = minZoom;
						if (wantedZoom > maxZoom)
							wantedZoom = maxZoom;
					}

					// On met quand m�me une limite au zoom de la cam�ra, pour �viter les zooms n�gatifs, et que les contr�les s'inversent
					if (wantedZoom < 1.0f)
						wantedZoom = 1.0f;
				}
			}
		}
		else if (event.EventType == EET_KEY_INPUT_EVENT)
		{
			// Y et U : Zoomer/D�zoomer
			if ((event.KeyInput.Key == KEY_KEY_Y || event.KeyInput.Key == KEY_KEY_U) && !event.KeyInput.PressedDown && game->canHandleKeysEvent)
			{
				wantedZoom += zoomStep * (event.KeyInput.Key == KEY_KEY_Y ? -1.0f : 1.0f);

				// Arrondi wantedZoom � zoomStep
				wantedZoom = core::round_(wantedZoom / zoomStep) * zoomStep;

				if (lockZoom)
				{
					if (wantedZoom < minZoom)
						wantedZoom = minZoom;
					if (wantedZoom > maxZoom)
						wantedZoom = maxZoom;
				}

				// On met quand m�me une limite au zoom de la cam�ra, pour �viter les zooms n�gatifs, et que les contr�les s'inversent
				if (wantedZoom < 1.0f)
					wantedZoom = 1.0f;
			}
		}
#ifdef USE_JOYSTICKS
		else if (event.EventType == EET_JOYSTICK_INPUT_EVENT && event.JoystickEvent.Joystick == 0 && game->joysticksState.size())
		{
			const bool buttonPressed5 = event.JoystickEvent.IsButtonPressed(4);	// Attention : sous Irrlicht, les boutons commencent � l'index 0, et non pas � 1
			const bool buttonPressed6 = event.JoystickEvent.IsButtonPressed(5);
			if (buttonPressed5 || buttonPressed6 && !(buttonPressed5 && buttonPressed6)		// (Bouton 5) XOR (Bouton 6)
				&& core::equals(wantedZoom, currentZoom, zoomStep * 1.1f))
			{
				// Boutons 5 et 6 : Zoomer/D�zoomer
				wantedZoom += zoomStep * (buttonPressed5 ? -1.0f : 1.0f);

				// Arrondi wantedZoom � zoomStep
				wantedZoom = core::round_(wantedZoom / zoomStep) * zoomStep;

				if (lockZoom)
				{
					if (wantedZoom < minZoom)
						wantedZoom = minZoom;
					if (wantedZoom > maxZoom)
						wantedZoom = maxZoom;
				}

				// On met quand m�me une limite au zoom de la cam�ra, pour �viter les zooms n�gatifs, et que les contr�les s'inversent
				if (wantedZoom < 1.0f)
					wantedZoom = 1.0f;
			}
		}
#endif
	}

	return false;
}
void RTSCamera::OnRegisterSceneNode()
{
	if (SceneManager->getActiveCamera() == this)
	{
		SceneManager->registerNodeForRendering(this, ESNRP_CAMERA);

		const vector3df pos = getAbsolutePosition();
		const vector3df tgtv = (Target - pos).normalize();

		vector3df up = UpVector;
		up.normalize();

		if (core::equals(fabs(tgtv.dotProduct(up)), 1.0f))
			up.X += 0.5f;

		//ViewArea.Matrices [ ETS_VIEW ].buildCameraLookAtMatrixLH(pos, Target, up);
		ViewArea.getTransform(ETS_VIEW).buildCameraLookAtMatrixLH(pos, Target, up);
		//ViewArea.setTransformState ( ETS_VIEW );
		recalculateViewArea();
	}

	//if (IsVisible)
		ISceneNode::OnRegisterSceneNode();

	// Ajout� : Met � jour les lumi�res de XEffects :
	updateXEffects();
}
void RTSCamera::render()
{
	//if (game->driver)
	//{
		game->driver->setTransform(ETS_PROJECTION, ViewArea.getTransform(ETS_PROJECTION));
		game->driver->setTransform(ETS_VIEW, ViewArea.getTransform(ETS_VIEW));
	//}
}
void RTSCamera::OnAnimate(u32 timeMs)
{
	// Obtient le temps r�ellement �coul� depuis la derni�re mise � jour pour animer la cam�ra avec la vitesse r�elle malgr� le changement de la vitesse du jeu : �vite que la cam�ra ne s'acc�l�re lorsque la vitesse du jeu est augment�e
	const u32 realTimeMs = game->deviceTimer->getRealTime();
	const u32 realElapsedTimeMs = (realTimeMs > lastRealTimeMs ? realTimeMs - lastRealTimeMs : 0);
	const float realElapsedTimeS = (float)realElapsedTimeMs * 0.001f;
	lastRealTimeMs = realTimeMs;

	// Si le temps a recul� depuis la derni�re frame, c'est qu'on l'a modifi� manuellement :
	// on met donc � jour le temps de r�f�rence pour l'animation des enfants de cette cam�ra
	if (lastTimeMs > timeMs)
		beginTimeMs = realTimeMs;
	lastTimeMs = timeMs;

	// Touches de configuration : Ctrl + Maj + Fl�ches du clavier
	if (game->keys[KEY_CONTROL] && game->keys[KEY_SHIFT])
	{
		if (game->keys[KEY_RIGHT])	// Ctrl + Maj + Fl�che droite : FOV augment�
			Fovy += realElapsedTimeS * 0.5f;
		if (game->keys[KEY_LEFT])	// Ctrl + Maj + Fl�che gauche : FOV diminu�
			Fovy -= realElapsedTimeS * 0.5f;
		if (game->keys[KEY_UP])		// Ctrl + Maj + Fl�che du haut : Far Distance augment�e
			ZFar += realElapsedTimeS * 500.0f * TAILLE_OBJETS;
		if (game->keys[KEY_DOWN])	// Ctrl + Maj + Fl�che du bas : Far Distance diminu�e
			ZFar -= realElapsedTimeS * 500.0f * TAILLE_OBJETS;

		if (game->keys[KEY_KEY_R])	// Ctrl + Maj + R : Remise � z�ro : FOV et Far Distance
		{
			Fovy = core::PI * 0.4f;
			ZFar = CAMERA_FAR_VALUE;
		}
		else
		{
			Fovy = core::clamp(Fovy, 0.03f, 3.1f);
			ZFar = core::clamp(ZFar, 150.0f * TAILLE_OBJETS, 2000.0f * TAILLE_OBJETS);
		}

		recalculateProjectionMatrix();
	}

	if (!lockCamera && !isCameraFPSEnabled)
	{
		animate(realElapsedTimeS);
		updateAbsolutePosition();
	}
	else // Les animateurs et les nodes enfants (cam�ra FPS) ne sont pris en compte que si la camera est bloqu�e ou si la cam�ra FPS est activ�e
	{
		// On met seulement � jour les animateurs avec le temps r�el lorsqu'on est dans une sc�ne du jeu (autrement, peut provoquer des bugs dans l'animation du menu principal)
		if (game->isInGameScene())
		{
			// Met � jour la cam�ra FPS et les animateurs avec le temps r�el, ind�pendament de la vitesse du jeu
			ISceneNode::OnAnimate(realTimeMs - beginTimeMs);	// Utilisation de beginTimeMs pour effectuer une synchronisation avec le temps r�el du jeu : lors de leur premi�re animation, le temps envoy� commence � 0 ms
		}
		else
		{
			// Met � jour la cam�ra FPS et les animateurs avec le temps de jeu actuel
			ISceneNode::OnAnimate(timeMs);
		}
	}
}
void RTSCamera::animate(float elapsedTimeS)
{
	const core::dimension2du& screenSize = game->driver->getScreenSize();
	const core::vector2df& MousePos = game->mouseState.positionF;

	float nRotX = rotX;
	float nRotY = rotY;

	vector3df translate(oldTarget);
	vector3df tvectX = RelativeTranslation - Target;
	tvectX = tvectX.crossProduct(UpVector);
	tvectX.normalize();

	if (currentZoom != wantedZoom)
	{
		const float addedZoom = elapsedTimeS * zoomSpeed;
		if (wantedZoom > currentZoom)
		{
			currentZoom += addedZoom;
			if (wantedZoom < currentZoom)
				currentZoom = wantedZoom;
		}
		else if (wantedZoom < currentZoom)
		{
			currentZoom -= addedZoom;
			if (wantedZoom > currentZoom)
				currentZoom = wantedZoom;
		}
	}

	// Rotate
	if (!lockCamera && (game->mouseState.middleButton || (game->mouseState.rightButton && game->keys[KEY_CONTROL])))
	{
		if (rotating)
		{
			nRotX += (rotateStartX - MousePos.X) * rotateSpeed;
			nRotY += (rotateStartY - MousePos.Y) * rotateSpeed;

			repairRotations(nRotX, nRotY);
		}
		else
		{
			rotateStartX = MousePos.X;
			rotateStartY = MousePos.Y;
			rotating = true;
		}
	}
	else if (rotating)
	{
		rotX += (rotateStartX - MousePos.X) * rotateSpeed;
		rotY += (rotateStartY - MousePos.Y) * rotateSpeed;

		repairRotations(rotX, rotY);

		nRotX = rotX;
		nRotY = rotY;

		rotating = false;
	}
#ifdef USE_JOYSTICKS
	else if (game->joysticksState.size() != 0)
	{
		const float rotationFactor = fabs(rotateSpeed) * elapsedTimeS * 0.15f;

		const float joystickTranslationX = Game::getNormalizedJostickAxis(game->joysticksState[0].Axis[SEvent::SJoystickEvent::AXIS_X]);
		const float joystickTranslationY = Game::getNormalizedJostickAxis(game->joysticksState[0].Axis[SEvent::SJoystickEvent::AXIS_Y]);

		rotX += joystickTranslationX * rotationFactor;
		rotY += joystickTranslationY * rotationFactor;
		repairRotations(rotX, rotY);
	}
#endif

	// Translate
	translating = false;
	if (!rotating && !lockCamera)
	{
		const bool mouseTranslation = (MousePos.Y <= ECART_COTE_SOURIS_BOUGER || MousePos.Y >= 1.0f - ECART_COTE_SOURIS_BOUGER
			|| MousePos.X <= ECART_COTE_SOURIS_BOUGER || MousePos.X >= 1.0f - ECART_COTE_SOURIS_BOUGER);
		const bool keyTranslation = game->canHandleKeysEvent && (!game->keys[KEY_CONTROL])	// La touche Ctrl modifie les fonctions de base de ces touches (fl�ches du clavier + Z + S + D (en mode DEBUG))
			&& (((game->keys[KEY_UP] || game->keys[KEY_DOWN] || game->keys[KEY_LEFT] || game->keys[KEY_RIGHT])
			|| game->keys[KEY_KEY_Z] || game->keys[KEY_KEY_S] || game->keys[KEY_KEY_Q] || game->keys[KEY_KEY_D]));
#ifdef USE_JOYSTICKS
		// Les boutons 1, 2, 3 et 4 du joystick permettent la translation de la cam�ra
		const bool useJoysticks = (game->joysticksState.size() != 0);
		const bool buttonPressed1 = useJoysticks ? game->joysticksState[0].IsButtonPressed(0) : false;	// Attention : sous Irrlicht, les boutons commencent � l'index 0, et non pas � 1
		const bool buttonPressed2 = useJoysticks ? game->joysticksState[0].IsButtonPressed(1) : false;
		const bool buttonPressed3 = useJoysticks ? game->joysticksState[0].IsButtonPressed(2) : false;
		const bool buttonPressed4 = useJoysticks ? game->joysticksState[0].IsButtonPressed(3) : false;
		const bool joystickTranslation = useJoysticks && (buttonPressed1 || buttonPressed2 || buttonPressed3 || buttonPressed4);
#endif

		translating = mouseTranslation || keyTranslation
#ifdef USE_JOYSTICKS
			|| joystickTranslation
#endif
			;

		if (translating)
		{
			// Calcule le facteur de translation � cette frame : On se d�place plus rapidement si la camera est plus haute
			//const float zoomFactor = (currentZoom * 0.7f + maxZoom * 0.3f) * 0.0045f;	// Ancien : 1.0f / 250.0f = 0.004f
			const float zoomFactor = currentZoom * 0.00315f + maxZoom * 0.00135f;		// Optimisation
			const float translateFactor = zoomFactor * translateSpeed * elapsedTimeS;

			core::vector2df translateValue = core::vector2df(0.0f, 0.0f);

			if (keyTranslation)
			{
				if ((game->keys[KEY_UP] && !game->keys[KEY_CONTROL]) || game->keys[KEY_KEY_Z])
					translateValue.Y -= 0.5f;
				if ((game->keys[KEY_DOWN] && !game->keys[KEY_CONTROL]) || game->keys[KEY_KEY_S])
					translateValue.Y += 0.5f;
				if ((game->keys[KEY_LEFT] && !game->keys[KEY_CONTROL]) || game->keys[KEY_KEY_Q])
					translateValue.X -= 0.5f;
				if ((game->keys[KEY_RIGHT] && !game->keys[KEY_CONTROL]) || game->keys[KEY_KEY_D])
					translateValue.X += 0.5f;
			}
#ifdef USE_JOYSTICKS
			else if (joystickTranslation)
			{
				if (buttonPressed2)
					translateValue.Y -= 0.5f;
				if (buttonPressed3)
					translateValue.Y += 0.5f;
				if (buttonPressed1)
					translateValue.X -= 0.5f;
				if (buttonPressed4)
					translateValue.X += 0.5f;
			}
#endif
			else
			{
				if (MousePos.Y <= ECART_COTE_SOURIS_BOUGER)
					translateValue.Y -= 0.5f - MousePos.Y;
				if (MousePos.Y >= 1.0f - ECART_COTE_SOURIS_BOUGER)
					translateValue.Y += MousePos.Y - 0.5f;
				if (MousePos.X <= ECART_COTE_SOURIS_BOUGER)
					translateValue.X -= 0.5f - MousePos.X;
				if (MousePos.X >= 1.0f - ECART_COTE_SOURIS_BOUGER)
					translateValue.X += MousePos.X - 0.5f;
			}

			translate += tvectX * translateValue.X * translateFactor;
			translate.X += tvectX.Z * translateValue.Y * translateFactor;
			translate.Z -= tvectX.X * translateValue.Y * translateFactor;

			// Remet la nouvelle position de la cam�ra dans les limites du terrain
			if (translate.X < terrainLimits.UpperLeftCorner.X)	translate.X = terrainLimits.UpperLeftCorner.X;
			if (translate.X > terrainLimits.LowerRightCorner.X)	translate.X = terrainLimits.LowerRightCorner.X;
			if (translate.Z < terrainLimits.UpperLeftCorner.Y)	translate.Z = terrainLimits.UpperLeftCorner.Y;
			if (translate.Z > terrainLimits.LowerRightCorner.Y)	translate.Z = terrainLimits.LowerRightCorner.Y;

			oldTarget = translate;
		}
	}

	// Set Position
	Target = translate;

	RelativeTranslation.X = currentZoom + Target.X;
	RelativeTranslation.Y = Target.Y;
	RelativeTranslation.Z = Target.Z;

	RelativeTranslation.rotateXYBy(nRotY, Target);
	RelativeTranslation.rotateXZBy(-nRotX, Target);

	// Ajout� : Evite que la cam�ra ne tombe en-dessous du niveau du sol :
#ifdef USE_COLLISIONS
	// On calcule la hauteur minimale du niveau du sol ou de l'eau pour l'appliquer � la rotation en Y minimale de la cam�ra, � chaque fois que celle-ci bouge (zoom, rotation, translation)
	if (oldPos != RelativeTranslation && game->renderer)
	{
		const float terrainHeight =
			max(game->renderer->getTerrainHeight(RelativeTranslation.X, RelativeTranslation.Z), game->renderer->getTerrainManager().getTerrainInfos().water.height)
				+ 20.0f;	// Ajoute aussi une marge de 20.0f (arbitraire, donc modifiable)

		const float rotatedTerrainHeight = terrainHeight * -cosf(nRotY);
		const bool recalculateRot = ((RelativeTranslation.Y < terrainHeight) || (rotatedTerrainHeight > (wantedZoom + targetY) && wantedZoom < maxZoom));

		// Si le terrain est trop haut pour pouvoir �tre affich� avec ce zoom, on augmente le zoom d�sir�
		while (rotatedTerrainHeight > (wantedZoom + targetY) && wantedZoom < maxZoom)
			{ wantedZoom += zoomStep; }

		if (recalculateRot)
		{
			// Calcule la nouvelle rotation de la cam�ra en Y pour �tre � la limite du sol
			RelativeTranslation.Y = terrainHeight;
			{
				// Optimis� d'apr�s core::vector3d<T>::getSphericalCoordinateAngles() :
				// On utilise le fait que (RelativeTranslation - Target).getLenght() = currentZoom, par d�finition.
				const core::vector3df diff(RelativeTranslation - Target);
				nRotY = 90.0f - (acos(core::clamp(diff.Y / currentZoom, 0.0f, 1.0f)) * RADTODEG);	// V�rifie que diff.Y / currentZoom est bien entre 0.0f et 1.0f, pour �viter que acos ne retourne NaN (n�cessaire : arrive r�guli�rement, en bloquant ainsi compl�tement la cam�ra) !
			}
			repairRotationY(nRotY);
			if (!rotating)	// Lorsque la cam�ra est en rotation, on ne doit pas modifier rotY
				rotY = nRotY;

			// Recalcule la position de la cam�ra :
			RelativeTranslation.X = currentZoom + Target.X;
			RelativeTranslation.Y = Target.Y;
			RelativeTranslation.Z = Target.Z;
			RelativeTranslation.rotateXYBy(nRotY, Target);
			RelativeTranslation.rotateXZBy(-nRotX, Target);
		}

		oldPos = RelativeTranslation;
	}
#endif

	// Correct Rotation Error
	UpVector.set(0.0f, 1.0f, 0.0f);
	UpVector.rotateXYBy(-nRotY, core::vector3df());
	UpVector.rotateXZBy(-nRotX + 180.0f, core::vector3df());
}
void RTSCamera::setPosition(const core::vector3df& pos)
{
	RelativeTranslation = pos;
	updateAnimationState();
}
void RTSCamera::setTarget(const core::vector3df& target)
{
	targetY = target.Y;

	Target = oldTarget = target;
	updateAnimationState();
}
void RTSCamera::pointCameraAtNode(const ISceneNode* node, float radius)
{
#if 1
	// Nouvelle version : zoom qui reste restreint dans les limites, avec conservation de la position de la cam�ra en Y

	// Calcule la nouvelle position et cible de la cam�ra (peut sembler complexe, mais fonctionnel !)
	const core::vector3df& nodePos = node->getPosition();
	vector3df toTarget = RelativeTranslation - Target;
	toTarget /= toTarget.Y;
	toTarget.X = core::clamp(toTarget.X, -2.0f, 2.0f);	toTarget.Z = core::clamp(toTarget.Z, -2.0f, 2.0f);	// Evite qu'on doive s'�carter trop du node en X et en Z pour conserver la m�me hauteur
	Target = nodePos + toTarget * (targetY - nodePos.Y);

	// Calcule le nouveau zoom
	currentZoom = radius - (Target - nodePos).getLength();
	currentZoom = core::round32(currentZoom / zoomStep) * zoomStep;	// Arrondi currentZoom � zoomStep
	if (lockZoom)
	{
		if (currentZoom < minZoom)
			currentZoom = minZoom;
		if (currentZoom > maxZoom)
			currentZoom = maxZoom;
	}
	if (currentZoom < 1.0f)
		currentZoom = 1.0f;

	// Calcule la nouvelle position avec ce zoom
	toTarget.setLength(currentZoom);
	RelativeTranslation = Target + toTarget;

	// Assigne et recalcule ce qui est relatif � la position et � la cible de la cam�ra
	updateAnimationState();	//setPosition(RelativeTranslation);
	setTarget(Target);
#else
	// Ancienne version : mouvement instantan�, zoom et position de la cam�ra en Y modifi�s
	vector3df totarget = RelativeTranslation - Target;
	totarget.setLength(radius);
	const core::vector3df& nodePosition = node->getPosition();
	setPosition(nodePosition + totarget);
	setTarget(nodePosition);
#endif

	updateAnimationState();
}
void RTSCamera::updateAnimationState()
{
	// Remet le target dans les limites du terrain
	if (Target.X < terrainLimits.UpperLeftCorner.X)		Target.X = terrainLimits.UpperLeftCorner.X;
	if (Target.X > terrainLimits.LowerRightCorner.X)	Target.X = terrainLimits.LowerRightCorner.X;
	if (Target.Z < terrainLimits.UpperLeftCorner.Y)		Target.Z = terrainLimits.UpperLeftCorner.Y;
	if (Target.Z > terrainLimits.LowerRightCorner.Y)	Target.Z = terrainLimits.LowerRightCorner.Y;

   vector3df pos(RelativeTranslation - Target);

   // X rotation
   vector2df vec2d(pos.X, pos.Z);
   rotX = (float)vec2d.getAngle();

   // Y rotation
   pos.rotateXZBy(rotX, vector3df());
   vec2d.set(pos.X, pos.Y);
   rotY = -(float)vec2d.getAngle();

   // Replace les rotations dans les limites
   while (rotX < 0.0f)		rotX += 360.0f;
   while (rotX > 360.0f)	rotX -= 360.0f;
   while (rotY < 0.0f)		rotY += 360.0f;
   while (rotY > 360.0f)	rotY -= 360.0f;

   repairRotations(rotX, rotY);

   // Zoom
   currentZoom = RelativeTranslation.getDistanceFrom(Target);
   wantedZoom = currentZoom;

	// V�rifie que le nouveau zoom est bien dans les limites actuelles du zoom
	if (lockZoom)
		wantedZoom = currentZoom = core::clamp(currentZoom, minZoom, maxZoom);

   //Correct Rotation Error
   UpVector.set(0.0f, 1.0f , 0.0f);
   UpVector.rotateXYBy(-rotY, core::vector3df(0.0f));
   UpVector.rotateXZBy(-rotX + 180.0f, core::vector3df(0.0f));
}
void RTSCamera::updateXEffects()
{
	if (!gameConfig.useXEffectsShadows || !game->xEffects)
		return;
	if (!game->xEffects->getShadowLightCount())
		return;

	// Obtient la cam�ra actuelle (peut aussi �tre la cam�ra FPS !)
	scene::ICameraSceneNode* const cam = SceneManager->getActiveCamera();
	if (!cam)
		return;

	// Modifie la position de la lumi�re des ombres :
	SShadowLight& shadowLight = game->xEffects->getShadowLight(0);

#if 1
	// Place la lumi�re � la position de la cam�ra
	const core::vector3df camPos = cam->getAbsolutePosition();
	shadowLight.setPosition(camPos);

	// Et on indique sa direction (en indiquant sa cible) : la direction du soleil vers l'origine
	const CSunSceneNode* sun = game->renderer->getSunSceneNode();
	const core::vector3df sunPos = (sun ? sun->getAbsolutePosition() : core::vector3df(0.0f));
	shadowLight.setTarget(camPos - sunPos);
#else
	// Placement intelligent de la lumi�re dans la vue de la cam�ra d'apr�s sa direction, pour affecter le maximum de nodes possibles :

	const core::vector3df camPos = cam->getAbsolutePosition();
	const CSunSceneNode* sun = game->renderer->getSunSceneNode();
	const core::vector3df sunPos = (sun ? sun->getAbsolutePosition() : core::vector3df(0.0f));
	
	const scene::SViewFrustum* view = cam->getViewFrustum();
	if (!view)
		return;

	const core::vector3df viewCenter = view->boundingBox.getCenter();
	core::line3df lightDir(sunPos + viewCenter, viewCenter);
	view->clipLine(lightDir);

	shadowLight.setPosition(lightDir.start);
	shadowLight.setTarget(lightDir.end);
#endif

	// Met � jour la matrice de vue de la lumi�re
	shadowLight.updateViewMatrix();
}
void RTSCamera::setZoomLock(bool lock, float min, float max)
{
	if (min < 1.0f)
		min = 1.0f;
	if (max < 1.0f)
		max = 1.0f;

	if (min > max)
	{
		// Echange les deux valeurs :
		const float tmp = min;
		min = max;
		max = tmp;
	}

	lockZoom = lock;
	minZoom = min;
	maxZoom = max;
}
void RTSCamera::repairRotations(float& rotationX, float& rotationY)
{
	if (lockRotateX)
	{
		if (rotationX < minRotateX)
			rotationX = minRotateX;
		else if (rotationX > maxRotateX)
			rotationX = maxRotateX;
	}
	if (lockRotateY)
	{
		if (rotationY < minRotateY)
			rotationY = minRotateY;
		else if (rotationY > maxRotateY)
			rotationY = maxRotateY;
	}
}
void RTSCamera::repairRotationY(float& rotationY)
{
	if (lockRotateY)
	{
		if (rotationY < minRotateY)
			rotationY = minRotateY;
		else if (rotationY > maxRotateY)
			rotationY = maxRotateY;
	}
}
void RTSCamera::setIsCameraFPSEnabled(bool enabled)
{
	if (!canBecomeFPSCamera || !FPSCamera || (enabled == isCameraFPSEnabled))
		return;

	isCameraFPSEnabled = enabled;

	if (enabled)
	{
		// Anime la cam�ra FPS pour que ses animators soient � jour
		FPSCamera->OnAnimate(lastRealTimeMs);

		// D�place la cam�ra FPS � l'endroit o� se situe la cam�ra RTS
		FPSCamera->setFarValue(ZFar);
		FPSCamera->setFOV(Fovy);
		FPSCamera->setPosition(core::vector3df(	// V�rifie tout de m�me que la nouvelle position de la cam�ra FPS est bien dans les limites du monde
			// TODO : Trouver une meilleure mani�re de faire que les "clamp", qui d�truisent la transition entre la cam�ra RTS et FPS
			core::clamp(RelativeTranslation.X, terrainLimits.UpperLeftCorner.X, terrainLimits.LowerRightCorner.X),
			RelativeTranslation.Y,
			core::clamp(RelativeTranslation.Z, terrainLimits.UpperLeftCorner.Y, terrainLimits.LowerRightCorner.Y)));
		FPSCamera->setTarget(Target);
		updateAbsolutePosition();
		FPSCamera->updateAbsolutePosition();

		// Active et ajoute l'animator de collision � la cam�ra FPS
		if (collisionAnim)
		{
			collisionAnim->setWorld(collisionWorld);
			FPSCamera->addAnimator(collisionAnim);
		}

		SceneManager->setActiveCamera(FPSCamera);
	}
	else
	{
		// Supprime et d�sactive l'animator de collision de la cam�ra FPS
		if (collisionAnim)
		{
			FPSCamera->removeAnimator(collisionAnim);
			collisionAnim->setWorld(NULL);
		}

		// Ne modifie pas la position de la cam�ra RTS si sa position est bloqu�e
		if (!lockCamera)
		{
			// TODO : Am�liorer les transitions entre la cam�ra FPS et la cam�ra RTS

			vector3df newTarget(FPSCamera->getTarget()), newPos(FPSCamera->getPosition());
			newTarget.Y = targetY;
			newPos.Y += newTarget.Y - FPSCamera->getTarget().Y; // R�percute les changements de la cible en Y sur la position de la cam�ra, pour qu'elle conserve sa rotation

			if (lockRotateY && newPos.Y < newTarget.Y) // Evite les probl�mes de rotation en Y lorsque la cible est plus haute que la cam�ra
				newPos.Y = newTarget.Y;

			// Remet la nouvelle position de la cam�ra dans les limites du terrain
			if (newPos.X < terrainLimits.UpperLeftCorner.X)		newPos.X = terrainLimits.UpperLeftCorner.X;
			if (newPos.X > terrainLimits.LowerRightCorner.X)	newPos.X = terrainLimits.LowerRightCorner.X;
			if (newPos.Z < terrainLimits.UpperLeftCorner.Y)		newPos.Z = terrainLimits.UpperLeftCorner.Y;
			if (newPos.Z > terrainLimits.LowerRightCorner.Y)	newPos.Z = terrainLimits.LowerRightCorner.Y;

			//setPosition(newPos);
			RelativeTranslation = newPos;
			setTarget(newTarget);
			ISceneNode::updateAbsolutePosition();
		}

		SceneManager->setActiveCamera(this);
	}

	// Affiche/Masque le curseur de la souris suivant si la cam�ra FPS est d�sactiv�e/activ�e
	game->device->getCursorControl()->setVisible(!enabled);
}
void RTSCamera::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options)
{
	if (!out)
		return;

	//out->addString("Name", Name.c_str());
	//out->addInt("Id", ID);

	//out->addBool("Visible", IsVisible);
	//out->addEnum("AutomaticCulling", AutomaticCullingState, AutomaticCullingNames);
	//out->addInt("DebugDataVisible", DebugDataVisible);
	//out->addBool("IsDebugObject", IsDebugObject);

	// Divise la position et la cible de la cam�ra par TAILLE_OBJETS pour que la sauvegarde n'en soit pas d�pendante
	// TODO : Ne plus n�cessiter ce syst�me de division
	out->addVector3d("Position", RelativeTranslation / TAILLE_OBJETS);
	out->addVector3d("Target", Target / TAILLE_OBJETS);

	//out->addBool("LockCamera", lockCamera);
}
void RTSCamera::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
{
	if (!in)
		return;

	// D�bloque la camera pour s'assurer que sa position sera bien mise � jour
	setLockCamera(false);

	//Name = in->getAttributeAsString("Name");
	//ID = in->getAttributeAsInt("Id");

	//IsVisible = in->getAttributeAsBool("Visible");
	//AutomaticCullingState = (scene::E_CULLING_TYPE)in->getAttributeAsEnumeration("AutomaticCulling", scene::AutomaticCullingNames);
	//DebugDataVisible = in->getAttributeAsInt("DebugDataVisible");
	//IsDebugObject = in->getAttributeAsBool("IsDebugObject");

	// Mutliplie la position et la cible de la cam�ra par TAILLE_OBJETS pour les adapter � l'�chelle du monde actuel
	// TODO : Ne plus n�cessiter ce syst�me
	if (in->existsAttribute("Position"))	setPosition(in->getAttributeAsVector3d("Position") * TAILLE_OBJETS);
	if (in->existsAttribute("Target"))		setTarget(in->getAttributeAsVector3d("Target") * TAILLE_OBJETS);

	updateAbsolutePosition();
	recalculateViewArea();
	updateAnimationState();

	// On ne bloque la camera qu'� la fin (Inutile maintenant : effectu� dans Game::load())
	//setLockCamera(in->getAttributeAsBool("LockCamera"));
}
u32 RTSCamera::getAnimatorBeginTimeMs() const
{
	return (game->deviceTimer->getRealTime() - beginTimeMs);
}
