#include "CGUICameraAnimatorWindow.h"
#include "RTSCamera.h"
#include "Game.h"
#include <sstream>	// Pour l'utilisation de la structure wistringstream de la STL (permet la conversion cha�ne de caract�res -> float)

CGUICameraAnimatorWindow::CGUICameraAnimatorWindow(IGUIEnvironment* environment, IGUIElement* parent, int id, const core::recti& rectangle)
 : CGUIWindow(environment, parent, id, rectangle), pointsListBox(NULL), animationSpeed(0.5f), animationTightness(0.5f), animationLoop(false), animationPingPong(false),
 cameraRTSAnimator(NULL), wasFPSCameraEnabled(false), wasCameraLocked(false), wasGameGUIVisible(true)
{
#ifdef _DEBUG
	setDebugName("CGUICameraAnimatorWindow");
#endif

	// Indique le titre de cette fen�tre
	setText(L"Animation de la cam�ra");

	// Cr�e les �l�ments de la GUI permettant de visualiser/modifier l'animation de la cam�ra RTS :
	// TODO : Rendre les textes des tooltips plus pr�cis
	// TODO : Impl�menter une EditBox n'acceptant que les entr�es num�riques

	// Constantes permettant un r�ajustement plus ais� des �l�ments de la GUI de cette fen�tre
	const float buttonSizeX = 0.2f,
		buttonSizeY = 0.07f,
		minPosX = 0.03f,
		textSizeX = 0.17f,
		valuePosX = 0.2f,
		valueSizeX = 0.77f,
		listSizeX = 0.65f,
		pointTextPosX = 0.7f,
		pointButtonSizeX = 0.13f,
		pointEcartButtonX = 0.01f,
		pointTextSizeX = 0.11f,
		pointValuePosX = 0.81f,
		pointValueSizeX = 0.16f,
		textSizeY = 0.05f,
		ecartY = 0.02f,
		minPosY = 0.05f,
		maxPosY = 0.99f;

	animateCameraButton = environment->addButton(getAbsoluteRectangle(0.5f - buttonSizeX * 0.5f, minPosY, 0.5f + buttonSizeX * 0.5f, minPosY + buttonSizeY, ClientRect), this, -1,
		L"Animer la cam�ra", L"Cliquez sur ce bouton pour animer la cam�ra");

	totalTimeText = environment->addStaticText(L"Temps total (secondes) :",
		getAbsoluteRectangle(minPosX, minPosY + buttonSizeY + ecartY, minPosX + textSizeX, minPosY + buttonSizeY + ecartY + textSizeY, ClientRect),
		false, true, this);
	totalTimeText->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);
	totalTimeEditBox = environment->addEditBox(L"0.00",
		getAbsoluteRectangle(valuePosX, minPosY + buttonSizeY + ecartY, valuePosX + valueSizeX, minPosY + buttonSizeY + ecartY + textSizeY, ClientRect),
		true, this);
	totalTimeEditBox->setToolTipText(L"R�glez le temps total de l'animation (d�pendant de sa vitesse et de son nombre de points)");

	speedText = environment->addStaticText(L"Vitesse (points/seconde) :",
		getAbsoluteRectangle(minPosX, minPosY + buttonSizeY + ecartY * 2.0f + textSizeY, minPosX + textSizeX, minPosY + buttonSizeY + (ecartY + textSizeY) * 2.0f, ClientRect),
		false, true, this);
	speedText->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);
	speedEditBox = environment->addEditBox(L"0.50",
		getAbsoluteRectangle(valuePosX, minPosY + buttonSizeY + ecartY * 2.0f + textSizeY, valuePosX + valueSizeX, minPosY + buttonSizeY + (ecartY + textSizeY) * 2.0f, ClientRect),
		true, this);
	speedEditBox->setToolTipText(L"R�glez ici la vitesse de l'animation");

	tightnessText = environment->addStaticText(L"Tension :",
		getAbsoluteRectangle(minPosX, minPosY + buttonSizeY + ecartY * 3.0f + textSizeY * 2.0f, minPosX + textSizeX, minPosY + buttonSizeY + (ecartY + textSizeY) * 3.0f, ClientRect),
		false, true, this);
	tightnessText->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);
	tightnessEditBox = environment->addEditBox(L"0.50",
		getAbsoluteRectangle(valuePosX, minPosY + buttonSizeY + ecartY * 3.0f + textSizeY * 2.0f, valuePosX + valueSizeX, minPosY + buttonSizeY + (ecartY + textSizeY) * 3.0f, ClientRect),
		true, this);
	tightnessEditBox->setToolTipText(L"R�glez ici la tension de la ligne que suivra l'animation (valeurs recommand�es entre 0 et 1)");

	loopCheckBox = environment->addCheckBox(false,
		getAbsoluteRectangle(valuePosX, minPosY + buttonSizeY + ecartY * 4.0f + textSizeY * 3.0f, valuePosX + valueSizeX, minPosY + buttonSizeY + (ecartY + textSizeY) * 4.0f, ClientRect),
		this, -1, L"R�p�tition infinie");
	loopCheckBox->setToolTipText(L"Cochez cette case pour que l'animation soit r�p�t�e � l'infini");

	pingPongCheckBox = environment->addCheckBox(false,
		getAbsoluteRectangle(valuePosX, minPosY + buttonSizeY + ecartY * 5.0f + textSizeY * 4.0f, valuePosX + valueSizeX, minPosY + buttonSizeY + (ecartY + textSizeY) * 5.0f, ClientRect),
		this, -1, L"Sens inverse � chaque r�p�tition");
	pingPongCheckBox->setToolTipText(L"Cochez cette case pour que le sens de l'animation soit invers� � chaque r�p�tition (n�cessite la r�p�tition infinie)");

	pointsText = environment->addStaticText(L"Points de l'animation  :",
		getAbsoluteRectangle(minPosX, minPosY + buttonSizeY + ecartY * 6.0f + textSizeY * 5.0f, minPosX + textSizeX, minPosY + buttonSizeY + (ecartY + textSizeY) * 6.0f, ClientRect),
		false, true, this);
	pointsText->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_LOWERRIGHT);
	pointsListBox = environment->addListBox(
		getAbsoluteRectangle(minPosX, minPosY + buttonSizeY + ecartY * 7.0f + textSizeY * 6.0f, minPosX + listSizeX, maxPosY, ClientRect), this);
	pointsListBox->setToolTipText(L"S�lectionnez ici les points de l'animation � modifier");

	pointAddButton = environment->addButton(
		getAbsoluteRectangle(pointTextPosX, minPosY + buttonSizeY + ecartY * 7.0f + textSizeY * 6.0f, pointTextPosX + pointButtonSizeX, minPosY + buttonSizeY + (ecartY + textSizeY) * 7.0f, ClientRect),
		this, -1, L"Ajouter un point", L"Cliquez sur ce bouton pour ajouter un point au trajet de la cam�ra lors de son animation");
	pointRemoveButton = environment->addButton(
		getAbsoluteRectangle(pointTextPosX + pointButtonSizeX + pointEcartButtonX, minPosY + buttonSizeY + ecartY * 7.0f + textSizeY * 6.0f, pointTextPosX + pointButtonSizeX * 2.0f + pointEcartButtonX, minPosY + buttonSizeY + (ecartY + textSizeY) * 7.0f, ClientRect),
		this, -1, L"Supprimer ce point", L"Cliquez sur ce bouton pour supprimer le point s�lectionn� du trajet de la cam�ra lors de son animation");

	pointXText = environment->addStaticText(L"Coordonn�e X :",
		getAbsoluteRectangle(pointTextPosX, minPosY + buttonSizeY + ecartY * 8.0f + textSizeY * 7.0f, pointTextPosX + pointTextSizeX, minPosY + buttonSizeY + (ecartY + textSizeY) * 8.0f, ClientRect),
		false, true, this, -1);
	pointXText->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);
	pointXEditBox = environment->addEditBox(L"0.00",
		getAbsoluteRectangle(pointValuePosX, minPosY + buttonSizeY + ecartY * 8.0f + textSizeY * 7.0f, pointValuePosX + pointValueSizeX, minPosY + buttonSizeY + (ecartY + textSizeY) * 8.0f, ClientRect),
		true, this);
	pointXEditBox->setToolTipText(L"Indiquez ici la coordonn�e X de ce point");

	pointYText = environment->addStaticText(L"Coordonn�e Y :",
		getAbsoluteRectangle(pointTextPosX, minPosY + buttonSizeY + ecartY * 9.0f + textSizeY * 8.0f, pointTextPosX + pointTextSizeX, minPosY + buttonSizeY + (ecartY + textSizeY) * 9.0f, ClientRect),
		false, true, this, -1);
	pointYText->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);
	pointYEditBox = environment->addEditBox(L"0.00",
		getAbsoluteRectangle(pointValuePosX, minPosY + buttonSizeY + ecartY * 9.0f + textSizeY * 8.0f, pointValuePosX + pointValueSizeX, minPosY + buttonSizeY + (ecartY + textSizeY) * 9.0f, ClientRect),
		true, this);
	pointYEditBox->setToolTipText(L"Indiquez ici la coordonn�e Y de ce point");

	pointZText = environment->addStaticText(L"Coordonn�e Z :",
		getAbsoluteRectangle(pointTextPosX, minPosY + buttonSizeY + ecartY * 10.0f + textSizeY * 9.0f, pointTextPosX + pointTextSizeX, minPosY + buttonSizeY + (ecartY + textSizeY) * 10.0f, ClientRect),
		false, true, this, -1);
	pointZText->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);
	pointZEditBox = environment->addEditBox(L"0.00",
		getAbsoluteRectangle(pointValuePosX, minPosY + buttonSizeY + ecartY * 10.0f + textSizeY * 9.0f, pointValuePosX + pointValueSizeX, minPosY + buttonSizeY + (ecartY + textSizeY) * 10.0f, ClientRect),
		true, this);
	pointZEditBox->setToolTipText(L"Indiquez ici la coordonn�e Z de ce point");

	pointGoToButton = environment->addButton(
		getAbsoluteRectangle(pointTextPosX, minPosY + buttonSizeY + ecartY * 11.0f + textSizeY * 10.0f, pointTextPosX + pointButtonSizeX * 2.0f + pointEcartButtonX, minPosY + buttonSizeY + (ecartY + textSizeY) * 11.0f, ClientRect),
		this, -1, L"Aller au point", L"Cliquez sur ce bouton pour placer la cam�ra sur ce point");



	// R�initialise cette animation
	reset();

	// Ne s�lectionne aucun point par d�faut
	pointsListBox->setSelected(-1);
	setCurrentSelectedPoint(pointsListBox->getSelected());
}
CGUICameraAnimatorWindow::~CGUICameraAnimatorWindow()
{
	if (cameraRTSAnimator)
		cameraRTSAnimator->drop();
}
void CGUICameraAnimatorWindow::reset()
{
	// R�initialise la vitesse et le temps total de l'animation
	animationSpeed = 0.5f;
	speedEditBox->setText(L"0.50");
	totalTimeEditBox->setText(L"0.00");

	// R�initialise la tension de la ligne de l'animation
	animationTightness = 0.5f;
	tightnessEditBox->setText(L"0.50");

	// R�initialise la r�p�tition de l'animation
	animationLoop = false;
	loopCheckBox->setChecked(false);
	animationPingPong = false;
	pingPongCheckBox->setChecked(false);

	// Efface la liste des points de cette animation
	animationPoints.clear();
	pointsListBox->clear();

	// Indique qu'aucun point n'est s�lectionn�
	setCurrentSelectedPoint(-1);

	// D�sactive le bouton pour animer la cam�ra puisque plus aucun point n'existe
	animateCameraButton->setEnabled(false);
}
bool CGUICameraAnimatorWindow::OnEvent(const SEvent& event)
{
	// V�rifie que cette fen�tre est bien visible et activ�e, et que l'animation de la cam�ra n'est pas en train d'�tre jou�e
	if (IsVisible && isEnabled() && !isCameraAnimationEnabled())
	{
		bool closeMenu = false;

		// G�re les �v�nements du clavier
		if (event.EventType == EET_KEY_INPUT_EVENT)
		{
			if (event.KeyInput.PressedDown)	// La touche a �t� press�e : on appuie sur le bouton
			{
				switch (event.KeyInput.Key)
				{
				//case KEY_RETURN:	// On n'autorise pas la fermeture de cette fen�tre par un appui sur Entr�e, car un appui sur cette touche peut aussi signifier la validation des donn�es entr�es par le joueur
				case KEY_ESCAPE:
					if (CloseButton->isVisible())
						CloseButton->setPressed(true);
					break;

				default: break;
				}
			}
			else	// La touche a �t� relev�e : on active l'action du bouton
			{
				switch (event.KeyInput.Key)
				{
				//case KEY_RETURN:	// On n'autorise pas la fermeture de cette fen�tre par un appui sur Entr�e, car un appui sur cette touche peut aussi signifier la validation des donn�es entr�es par le joueur
				case KEY_ESCAPE:
					closeMenu = true;
					break;

				default: break;
				}
			}
		}
		// G�re les �v�nements de la GUI
		else if (event.EventType == EET_GUI_EVENT && isEnabled())
		{
			switch (event.GUIEvent.EventType)
			{
			case EGET_LISTBOX_CHANGED:
			case EGET_LISTBOX_SELECTED_AGAIN:
				if (event.GUIEvent.Caller == pointsListBox)
				{
					// Indique le point actuellement s�lectionn�
					setCurrentSelectedPoint(pointsListBox->getSelected());
				}
				break;

			case EGET_BUTTON_CLICKED:
				if (event.GUIEvent.Caller == CloseButton)
				{
					// Indique que le joueur d�sire fermer ce menu
					closeMenu = true;
				}
				if (event.GUIEvent.Caller == animateCameraButton)
				{
					// Anime la cam�ra RTS avec cette animation personnalis�e
					addAnimatorToCamera();
				}
				else if (event.GUIEvent.Caller == pointAddButton)
				{
					// Ajoute un point � la liste
					addPoint();
				}
				else if (event.GUIEvent.Caller == pointRemoveButton)
				{
					// Supprime ce point de la liste
					removePoint(getSelectedPoint());
				}
				else if (event.GUIEvent.Caller == pointGoToButton)
				{
					// Place la cam�ra RTS au point actuellement s�lectionn�
					game->cameraRTS->setPosition(getPoint(getSelectedPoint()));
				}
				break;

			case EGET_CHECKBOX_CHANGED:
				if (event.GUIEvent.Caller == loopCheckBox)
				{
					// Obtient si la r�p�tition de l'animation est activ�e
					animationLoop = loopCheckBox->isChecked();
				}
				else if (event.GUIEvent.Caller == pingPongCheckBox)
				{
					// Obtient si la r�p�tition de l'animation en sens inverse est activ�e
					animationPingPong = pingPongCheckBox->isChecked();
				}
				break;

			case EGET_ELEMENT_FOCUS_LOST:
			case EGET_EDITBOX_ENTER:
				if (event.GUIEvent.Caller == speedEditBox)
				{
					// Obtient la valeur num�rique de la vitesse de l'animation
					animationSpeed = getEditBoxValue(speedEditBox);

					// Calcule et affiche le temps total de l'animation
					swprintf_SS(L"%.2f", calculateAnimationTotalTime());
					totalTimeEditBox->setText(textW_SS);
				}
				else if (event.GUIEvent.Caller == totalTimeEditBox)
				{
					// Obtient la valeur num�rique de la vitesse de l'animation d�duite d'apr�s celle du temps total
					animationSpeed = calculateAnimationSpeed(getEditBoxValue(totalTimeEditBox));

					// Affiche la nouvelle vitesse de l'animation
					swprintf_SS(L"%.2f", animationSpeed);
					speedEditBox->setText(textW_SS);
				}
				else if (event.GUIEvent.Caller == tightnessEditBox)
				{
					// Obtient la valeur num�rique de la tension de la ligne de l'animation
					animationTightness = getEditBoxValue(tightnessEditBox);
				}
				else if (event.GUIEvent.Caller == pointXEditBox)
				{
					// Change la valeur num�rique de la coordonn�e X du point s�lectionn�
					changePointX(getEditBoxValue(pointXEditBox));
				}
				else if (event.GUIEvent.Caller == pointYEditBox)
				{
					// Obtient la valeur num�rique de la coordonn�e Y du point s�lectionn�
					changePointY(getEditBoxValue(pointYEditBox));
				}
				else if (event.GUIEvent.Caller == pointZEditBox)
				{
					// Obtient la valeur num�rique de la coordonn�e Z du point s�lectionn�
					changePointZ(getEditBoxValue(pointZEditBox));
				}
				break;
			}
		}

		// Ferme ce menu
		if (closeMenu && Parent)
		{
			SEvent sendEvent;
			sendEvent.EventType = EET_GUI_EVENT;
			sendEvent.GUIEvent.Caller = this;
			sendEvent.GUIEvent.Element = NULL;
			sendEvent.GUIEvent.EventType = EGET_ELEMENT_CLOSED;	// Indique que cette fen�tre a �t� ferm�e

			Parent->OnEvent(sendEvent);

			return true;
		}
	}

	return CGUIWindow::OnEvent(event);
}
void CGUICameraAnimatorWindow::draw()
{
	// Bas� sur la fonction draw() de la classe CGUIWindow d'Irrlicht SVN 1.8.0-alpha :

	if (IsVisible)
	{
		IGUISkin* const skin = Environment->getSkin();

		// update each time because the skin is allowed to change this always.
		updateClientRect();

		if ( CurrentIconColor != skin->getColor(isEnabled() ? EGDC_WINDOW_SYMBOL : EGDC_GRAY_WINDOW_SYMBOL) )
			refreshSprites();

		// draw body fast
		if (DrawBackground)
		{
			core::recti rect = skin->draw3DWindowBackground(this, DrawTitlebar,
					skin->getColor(IsActive ? EGDC_ACTIVE_BORDER : EGDC_INACTIVE_BORDER),
					AbsoluteRect, &AbsoluteClippingRect);

			if (DrawTitlebar && Text.size())
			{
				rect.UpperLeftCorner.X += skin->getSize(EGDS_TITLEBARTEXT_DISTANCE_X);
				rect.UpperLeftCorner.Y += skin->getSize(EGDS_TITLEBARTEXT_DISTANCE_Y);
				rect.LowerRightCorner.X -= skin->getSize(EGDS_WINDOW_BUTTON_WIDTH) + 5;

				IGUIFont* const font = skin->getFont(EGDF_WINDOW);
				if (font)
				{
					font->draw(Text.c_str(), rect,
							skin->getColor(IsActive ? EGDC_ACTIVE_CAPTION:EGDC_INACTIVE_CAPTION),
							true, true, &AbsoluteClippingRect);
				}
			}
		}
	}

	IGUIElement::draw();
}
void CGUICameraAnimatorWindow::udpate()
{
	// V�rifie si l'animation de la cam�ra RTS est termin�e
	if (cameraRTSAnimator && cameraRTSAnimator->hasFinished())
	{
		// Si c'est le cas, on supprime cet animator de la cam�ra RTS
		removeAnimatorFromCamera();
	}
}
void CGUICameraAnimatorWindow::save(io::IAttributes* out, io::IXMLWriter* writer) const
{
	if (!out || !writer)
		return;

	// Ajoute les informations principales de l'animation de la cam�ra
	out->addFloat("Speed", animationSpeed);
	out->addFloat("Tightness", animationTightness);
	out->addBool("Loop", animationLoop);
	out->addBool("PingPong", animationPingPong);

	// Ajoute les points de la ligne de l'animation de la cam�ra
	out->addInt("PointCount", animationPoints.size());
	const core::list<core::vector3df>::ConstIterator END = animationPoints.end();
	int pointIndex = 0;
	for (core::list<core::vector3df>::ConstIterator it = animationPoints.begin(); it != END; ++it)
	{
		sprintf_SS("Point%d", pointIndex);
		out->addVector3d(text_SS, (*it));
		++pointIndex;
	}

	// Ecrit les informations de l'animation de la cam�ra dans le fichier
	out->write(writer, false, L"CameraAnimation");
	out->clear();
}
void CGUICameraAnimatorWindow::load(io::IAttributes* in, io::IXMLReader* reader)
{
	if (!in || !reader)
		return;

	// R�initialise l'animation actuelle avant son chargement
	reset();

	// Lit les informations de l'animation de la cam�ra � partir du fichier
	reader->resetPosition();
	if (in->read(reader, false, L"CameraAnimation"))
	{
		// Lit les informations principales de l'animation de la cam�ra
		if (in->existsAttribute("Speed"))		animationSpeed = in->getAttributeAsFloat("Speed");
		if (in->existsAttribute("Tightness"))	animationTightness = in->getAttributeAsFloat("Tightness");
		if (in->existsAttribute("Loop"))		animationLoop = in->getAttributeAsBool("Loop");
		if (in->existsAttribute("PingPong"))	animationPingPong = in->getAttributeAsBool("PingPong");

		// Lit les points de la ligne de l'animation de la cam�ra
		if (in->existsAttribute("PointCount"))
		{
			const int pointCount = in->getAttributeAsInt("PointCount");
			for (int i = 0; i < pointCount; ++i)
			{
				sprintf_SS("Point%d", i);

				if (in->existsAttribute(text_SS))
					animationPoints.push_back(in->getAttributeAsVector3d(text_SS));
			}
		}

		in->clear();
	}



	// Met compl�tement � jour la GUI de la fen�tre avec les donn�es charg�es
	updateGUI();
}
void CGUICameraAnimatorWindow::updateGUI()
{
	// Affiche la vitesse de l'animation
	swprintf_SS(L"%.2f", animationSpeed);
	speedEditBox->setText(textW_SS);

	// Calcule et affiche le temps total de l'animation
	swprintf_SS(L"%.2f", calculateAnimationTotalTime());
	totalTimeEditBox->setText(textW_SS);

	// Affiche la tension de la ligne de l'animation
	swprintf_SS(L"%.2f", animationTightness);
	tightnessEditBox->setText(textW_SS);

	// Affiche la r�p�tition de l'animation
	loopCheckBox->setChecked(animationLoop);
	pingPongCheckBox->setChecked(animationPingPong);

	// Ajoute les points de cette animation � la liste de la GUI
	pointsListBox->clear();
	const core::list<core::vector3df>::ConstIterator END = animationPoints.end();
	for (core::list<core::vector3df>::ConstIterator it = animationPoints.begin(); it != END; ++it)
	{
		// Cr�e le texte de description du point � afficher dans la liste des points
		const core::vector3df& point = (*it);
		swprintf_SS(L"%.2f, %.2f, %.2f", point.X, point.Y, point.Z);

		// Ajoute un �l�ment repr�sentant ce point dans la liste des points de la GUI
		pointsListBox->addItem(textW_SS);
	}

	// S�lectionne le premier point de la liste
	pointsListBox->setSelected(0);

	// Indique le nouveau point s�lectionn�
	setCurrentSelectedPoint(pointsListBox->getSelected());

	// Active le bouton pour animer la cam�ra si au moins deux points existent
	animateCameraButton->setEnabled(animationPoints.size() > 1);
}
void CGUICameraAnimatorWindow::addAnimatorToCamera()
{
	// V�rifie que la cam�ra RTS n'est pas d�j� anim�e par un animator de cette fen�tre et qu'au moins un point est cr��, sinon on quitte
	if (cameraRTSAnimator || animationPoints.size() <= 1)
		return;

	// Convertit la liste actuelle des points de l'animation en un tableau de points
	core::array<core::vector3df> points(animationPoints.size());
	const core::list<core::vector3df>::ConstIterator END = animationPoints.end();
	for (core::list<core::vector3df>::ConstIterator it = animationPoints.begin(); it != END; ++it)
		points.push_back(*it);

	// Cr�e l'animator permettant � la cam�ra RTS de suivre la ligne personnalis�e par le joueur, puis l'ajoute au node
	cameraRTSAnimator = game->sceneManager->createFollowSplineAnimator(
		game->cameraRTS->getAnimatorBeginTimeMs(),	// Prend le temps r�el de d�part de la cam�ra RTS, car elle ne met pas � jour ses animateurs avec le temps du timer
		points, animationSpeed, animationTightness, animationLoop, animationPingPong);
	game->cameraRTS->addAnimator(cameraRTSAnimator);



	// D�sactive la cam�ra FPS de la cam�ra RTS
	wasFPSCameraEnabled = game->cameraRTS->getIsCameraFPSEnabled();
	game->cameraRTS->setIsCameraFPSEnabled(false);

	// Bloque la cam�ra RTS pour qu'elle puisse suivre cet animator
	wasCameraLocked = game->lockCamera;
	game->lockCamera = true;
	game->cameraRTS->setLockCamera(true);

	// Masque la GUI du jeu
	wasGameGUIVisible = game->guiManager->isGUIVisible(GUIManager::EGN_gameGUI);
	game->guiManager->setGUIVisible(GUIManager::EGN_gameGUI, false);

	// Masque le curseur de la souris
	game->device->getCursorControl()->setVisible(false);

	// Annule la s�lection actuelle
	game->batSelector.annulerSelection();
}
void CGUICameraAnimatorWindow::removeAnimatorFromCamera()
{
	// V�rifie que la cam�ra RTS est bien anim�e par un animator de cette fen�tre, sinon on quitte
	if (!cameraRTSAnimator)
		return;

	// Supprime cet animator de la cam�ra RTS
	game->cameraRTS->removeAnimator(cameraRTSAnimator);
	cameraRTSAnimator->drop();
	cameraRTSAnimator = NULL;



	// R�-affiche le curseur de la souris (fait avant la r�activation de la cam�ra FPS, car cette derni�re peut masquer le curseur � son tour)
	game->device->getCursorControl()->setVisible(true);

	// Restaure l'activation de la cam�ra FPS dans la cam�ra RTS
	game->cameraRTS->setIsCameraFPSEnabled(wasFPSCameraEnabled);

	// Restaure l'�tat du blocage de la cam�ra avant son animation
	game->lockCamera = wasCameraLocked;
#ifdef LOCK_CAMERA_WHEN_MOUSE_ON_GUI
	// On bloque la camera RTS si la souris est sur un �l�ment de la GUI ou si l'utilisateur a demand� de la bloquer
	game->cameraRTS->setLockCamera(game->isMouseOnGUI || wasCameraLocked);
#else
	// On ne bloque la camera que si l'utilisateur a demand� de la bloquer
	game->cameraRTS->setLockCamera(wasCameraLocked);
#endif

	// Restaure la visibilit� de la GUI du jeu avant l'animation de la cam�ra RTS
	game->guiManager->setGUIVisible(GUIManager::EGN_gameGUI, wasGameGUIVisible);
}
void CGUICameraAnimatorWindow::setCurrentSelectedPoint(int index)
{
	// On suppose ici que le point est d�j� s�lectionn� dans la liste de la GUI

	// V�rifie qu'un point est r�ellement s�lectionn�
	if (index < 0)
	{
		// Sinon, d�sactive et r�initialise tous les �l�ments de la GUI permettant de modifier le point actuel, dont le bouton pour supprimer un point :

		pointRemoveButton->setEnabled(false);

		pointXText->setEnabled(false);
		pointXEditBox->setEnabled(false);
		pointXEditBox->setText(L"0.00");

		pointYText->setEnabled(false);
		pointYEditBox->setEnabled(false);
		pointYEditBox->setText(L"0.00");

		pointZText->setEnabled(false);
		pointZEditBox->setEnabled(false);
		pointZEditBox->setText(L"0.00");

		pointGoToButton->setEnabled(false);

		return;
	}

	// Active le bouton pour supprimer ce point
	pointRemoveButton->setEnabled(true);

	// Obtient le point actuellement s�lectionn�
	const core::vector3df point = getPoint(index);

	// Modifie les informations des zones de modification de ce point :
	pointXText->setEnabled(true);
	pointXEditBox->setEnabled(true);
	swprintf_SS(L"%.2f", point.X);
	pointXEditBox->setText(textW_SS);

	pointYText->setEnabled(true);
	pointYEditBox->setEnabled(true);
	swprintf_SS(L"%.2f", point.Y);
	pointYEditBox->setText(textW_SS);

	pointZText->setEnabled(true);
	pointZEditBox->setEnabled(true);
	swprintf_SS(L"%.2f", point.Z);
	pointZEditBox->setText(textW_SS);

	pointGoToButton->setEnabled(true);
}
void CGUICameraAnimatorWindow::addPoint()
{
	// Obtient le nouveau point avec la position actuelle de la cam�ra
	const scene::ICameraSceneNode* const activeCamera = game->sceneManager->getActiveCamera();
	const core::vector3df newPoint(activeCamera ? activeCamera->getPosition() : core::vector3df());

	// Ajoute le nouveau point � la liste des points de l'animation
	animationPoints.push_back(newPoint);

	// Cr�e le texte de description du point � afficher dans la liste des points
	swprintf_SS(L"%.2f, %.2f, %.2f", newPoint.X, newPoint.Y, newPoint.Z);

	// Ajoute un �l�ment repr�sentant ce point dans la liste des points de la GUI et le s�lectionne
	pointsListBox->setSelected(pointsListBox->addItem(textW_SS));

	// Indique qu'un nouveau point a �t� s�lectionn�
	setCurrentSelectedPoint(pointsListBox->getSelected());

	// Calcule et affiche le temps total de l'animation
	swprintf_SS(L"%.2f", calculateAnimationTotalTime());
	totalTimeEditBox->setText(textW_SS);

	// Active le bouton pour animer la cam�ra si au moins un point existe
	animateCameraButton->setEnabled(animationPoints.size() > 1);

	// Calcule et affiche le temps total de l'animation
	swprintf_SS(L"%.2f", calculateAnimationTotalTime());
	totalTimeEditBox->setText(textW_SS);
}
void CGUICameraAnimatorWindow::removePoint(int index)
{
	if (index < 0 || (u32)index >= animationPoints.size())
		return;

	// Supprime ce point de la liste des points de la GUI
	pointsListBox->removeItem((u32)index);

	// S�lectionne l'�l�ment d'index inf�rieur dans la liste des points de la GUI s'il existe, sinon celui avec cet index
	pointsListBox->setSelected((index == 0 && (u32)index < animationPoints.size()) ? index : index - 1);

	// Indique qu'un nouveau point a �t� s�lectionn�
	setCurrentSelectedPoint(pointsListBox->getSelected());

	// Supprime ce point de la liste des points
	core::list<core::vector3df>::Iterator it = animationPoints.begin();
	if (index > 0)	it += index;
	animationPoints.erase(it);

	// D�sactive le bouton pour animer la cam�ra si moins de deux point existent
	animateCameraButton->setEnabled(animationPoints.size() > 1);

	// Calcule et affiche le temps total de l'animation
	swprintf_SS(L"%.2f", calculateAnimationTotalTime());
	totalTimeEditBox->setText(textW_SS);
}
void CGUICameraAnimatorWindow::changePointX(float pointX)
{
	// On suppose ici que les bonnes donn�es num�riques sont d�j� indiqu�es dans les �l�ments de la GUI

	const int index = getSelectedPoint();
	if (index < 0 || index >= (int)(animationPoints.size()))
		return;

	// Obtient le point s�lectionn�
	core::list<core::vector3df>::Iterator it = animationPoints.begin();
	it += index;

	// Modifie la coordonn�e X de ce point
	core::vector3df& point(*it);
	point.X = pointX;

	// Cr�e le nouveau texte de description du point � afficher dans la liste des points
	swprintf_SS(L"%.2f, %.2f, %.2f", point.X, point.Y, point.Z);

	// Met � jour le texte correspondant � ce point dans la liste des points de la GUI
	pointsListBox->setItem(index, textW_SS, -1);
}
void CGUICameraAnimatorWindow::changePointY(float pointY)
{
	// On suppose ici que les bonnes donn�es num�riques sont d�j� indiqu�es dans les �l�ments de la GUI

	const int index = getSelectedPoint();
	if (index < 0 || index >= (int)(animationPoints.size()))
		return;

	// Obtient le point s�lectionn�
	core::list<core::vector3df>::Iterator it = animationPoints.begin();
	it += index;

	// Modifie la coordonn�e X de ce point
	core::vector3df& point(*it);
	point.Y = pointY;

	// Cr�e le nouveau texte de description du point � afficher dans la liste des points
	swprintf_SS(L"%.2f, %.2f, %.2f", point.X, point.Y, point.Z);

	// Met � jour le texte correspondant � ce point dans la liste des points de la GUI
	pointsListBox->setItem(index, textW_SS, -1);
}
void CGUICameraAnimatorWindow::changePointZ(float pointZ)
{
	// On suppose ici que les bonnes donn�es num�riques sont d�j� indiqu�es dans les �l�ments de la GUI

	const int index = getSelectedPoint();
	if (index < 0 || index >= (int)(animationPoints.size()))
		return;

	// Obtient le point s�lectionn�
	core::list<core::vector3df>::Iterator it = animationPoints.begin();
	it += index;

	// Modifie la coordonn�e X de ce point
	core::vector3df& point(*it);
	point.Z = pointZ;

	// Cr�e le nouveau texte de description du point � afficher dans la liste des points
	swprintf_SS(L"%.2f, %.2f, %.2f", point.X, point.Y, point.Z);

	// Met � jour le texte correspondant � ce point dans la liste des points de la GUI
	pointsListBox->setItem(index, textW_SS, -1);
}
float CGUICameraAnimatorWindow::getEditBoxValue(IGUIEditBox* editBox) const
{
	if (!editBox)
		return 0.0f;

	// Converti la cha�ne de caract�res retourn�e par la zone de texte en float :

	// Utilise la structure wistringstream de la STL pour convertir cette cha�ne en float
	float value = 0.0f;
	wistringstream str(editBox->getText());
	str >> value;

	// Si la conversion a �chou�e, on retourne une valeur par d�faut
	if (str.fail())
		value = 0.0f;

	// Indique la valeur de la zone de texte d'apr�s la valeur num�rique obtenue (permet de faire concorder l'affichage de la zone de texte avec les donn�es internes enregistr�es)
	swprintf_SS(L"%.2f", value);
	editBox->setText(textW_SS);

	// Retourne la valeur convertie
	return value;
}
bool CGUICameraAnimatorWindow::hasKeyFocus() const
{
	const IGUIElement* const focus = game->gui->getFocus();
	if (!focus)
		return false;

	// Retourne true si une edit box ou la list box de cette fen�tre a le focus :
	return (focus == totalTimeEditBox
		|| focus == speedEditBox
		|| focus == tightnessEditBox
		|| focus == pointsListBox
		|| focus == pointXEditBox
		|| focus == pointYEditBox
		|| focus == pointZEditBox);
}
