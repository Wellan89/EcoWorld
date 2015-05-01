#include "CGUIFileSelector.h"
#include "CGUITexturedModalScreen.h"
#include "CGUIMessageBox.h"

//! constructor
CGUIFileSelector::CGUIFileSelector(const wchar_t* title, IGUIEnvironment* environment, IGUIElement* parent, s32 id, E_FILESELECTOR_TYPE type,
								   const core::recti& rectangle, IGUISpriteBank* spriteBank)
 : IGUIFileOpenDialog(environment, parent, id, rectangle), Dragging(false), FileNameText(0), FileList(0), DialogType(type), modalScreen(0),
 deleteFileButton(0), deleteFileConfirmationWindow(0), overwriteFileConfirmationWindow(0), SpriteBank(spriteBank)
{
#ifdef _DEBUG
	IGUIElement::setDebugName("CGUIFileSelector");
#endif

	Text = title;

	IGUISkin* const skin = Environment->getSkin();
	IGUISpriteBank* sprites = NULL;
	video::SColor color(255, 255, 255, 255);
	if (skin)
	{
		sprites = skin->getSpriteBank();
		color = skin->getColor(EGDC_WINDOW_SYMBOL);
	}

	const s32 buttonw = Environment->getSkin()->getSize(EGDS_WINDOW_BUTTON_WIDTH);
	const s32 posx = RelativeRect.getWidth() - buttonw - 4;
	const int textHeight = environment->getSkin()->getFont()->getDimension(L"A").Height + 4;

	CloseButton = Environment->addButton(core::recti(posx, 3, posx + buttonw, 3 + buttonw), this, -1,
		L"", L"Fermer");
	CloseButton->setSubElement(true);
	if (sprites)
	{
		CloseButton->setSpriteBank(sprites);
		CloseButton->setSprite(EGBS_BUTTON_UP, skin->getIcon(EGDI_WINDOW_CLOSE), color);
		CloseButton->setSprite(EGBS_BUTTON_DOWN, skin->getIcon(EGDI_WINDOW_CLOSE), color);
	}
	CloseButton->setAlignment(EGUIA_LOWERRIGHT, EGUIA_LOWERRIGHT, EGUIA_UPPERLEFT, EGUIA_UPPERLEFT);
	CloseButton->grab();

	OKButton = Environment->addButton(
		core::recti(RelativeRect.getWidth()-100, 30, RelativeRect.getWidth()-10, 60),
		this, -1, (DialogType == EFST_OPEN_DIALOG ? L"Ouvrir" : L"Enregistrer"));
	OKButton->setSubElement(true);
	OKButton->setAlignment(EGUIA_LOWERRIGHT, EGUIA_LOWERRIGHT, EGUIA_UPPERLEFT, EGUIA_UPPERLEFT);
	OKButton->grab();

	CancelButton = Environment->addButton(
		core::recti(RelativeRect.getWidth()-100, 70, RelativeRect.getWidth()-10, 100),
		this, -1, L"Annuler");
	CancelButton->setSubElement(true);
	CancelButton->setAlignment(EGUIA_LOWERRIGHT, EGUIA_LOWERRIGHT, EGUIA_UPPERLEFT, EGUIA_UPPERLEFT);
	CancelButton->grab();

	deleteFileButton = Environment->addButton(
		core::recti(RelativeRect.getWidth()-100, 110, RelativeRect.getWidth()-10, 140),
		this, -1, L"Supprimer");
	deleteFileButton->setSubElement(true);
	deleteFileButton->setAlignment(EGUIA_LOWERRIGHT, EGUIA_LOWERRIGHT, EGUIA_UPPERLEFT, EGUIA_UPPERLEFT);
	deleteFileButton->grab();

	// Désactive le bouton de suppression de fichier si on ne peut pas écrire sur le disque dur
	deleteFileButton->setEnabled(CAN_WRITE_ON_DISK);

	FileBox = Environment->addListBox(core::recti(10, 40 + textHeight * 2, RelativeRect.getWidth()-110, RelativeRect.getHeight()-40), this, -1, true);
	FileBox->setSubElement(true);
	FileBox->setAlignment(EGUIA_UPPERLEFT, EGUIA_LOWERRIGHT, EGUIA_UPPERLEFT, EGUIA_LOWERRIGHT);
	FileBox->grab();

	FileNameEditBox = Environment->addEditBox(0, core::recti(10, 35 + textHeight, RelativeRect.getWidth()-110, 35 + textHeight * 2), true, this, -1);
	FileNameEditBox->setSubElement(true);
	FileNameEditBox->setAlignment(EGUIA_UPPERLEFT, EGUIA_LOWERRIGHT, EGUIA_UPPERLEFT, EGUIA_UPPERLEFT);
	FileNameEditBox->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_UPPERLEFT);
	FileNameEditBox->grab();

	FileNameText = Environment->addEditBox(0, core::recti(10, 30, RelativeRect.getWidth()-110, 30 + textHeight), true, this, -1);
	FileNameText->setSubElement(true);
	FileNameText->setAlignment(EGUIA_UPPERLEFT, EGUIA_LOWERRIGHT, EGUIA_UPPERLEFT, EGUIA_UPPERLEFT);
	FileNameText->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_UPPERLEFT);
	FileNameText->grab();

	//FilterComboBox = Environment->addComboBox(core::recti(10, RelativeRect.getHeight()-30, RelativeRect.getWidth()-90, RelativeRect.getHeight()-10), this, -1);
	FilterComboBox = Environment->addComboBox(core::recti(10, RelativeRect.getHeight()-35, RelativeRect.getWidth()-110, RelativeRect.getHeight()-10), this, -1);
	FilterComboBox->setSubElement(true);
	FilterComboBox->setAlignment(EGUIA_UPPERLEFT, EGUIA_LOWERRIGHT, EGUIA_UPPERLEFT, EGUIA_UPPERLEFT);
	FilterComboBox->grab();
	FilterComboBox->addItem(L"Tous les fichiers");

	if (SpriteBank)
	{
		SpriteBank->grab();
		FileBox->setSpriteBank(SpriteBank);
	}
	DirectoryIconIdx = -1;
	FileIconIdx = -1;

	FileSystem = Environment->getFileSystem();

	if (FileSystem)
		prev_working_dir = FileSystem->getWorkingDirectory();

	fillListBox();

	// Ajouté : Crée un modal screen pour cet élement
	modalScreen = new CGUITexturedModalScreen(parent, -1, environment, this);
	modalScreen->drop();
}
//! destructor
CGUIFileSelector::~CGUIFileSelector()
{
	if (CloseButton)
		CloseButton->drop();

	if (OKButton)
		OKButton->drop();

	if (CancelButton)
		CancelButton->drop();

	if (deleteFileButton)
		deleteFileButton->drop();

	if (FileBox)
		FileBox->drop();

	if (FileNameEditBox)
		FileNameEditBox->drop();

	if (FileNameText)
		FileNameText->drop();

	if (FileList)
		FileList->drop();

	if (FilterComboBox)
		FilterComboBox->drop();

	if (SpriteBank)
		SpriteBank->drop();
}
//! returns the filename of the selected file. Returns NULL, if no file was selected.
core::stringw CGUIFileSelector::getFileNameStr() const
{
	core::stringw strw = FileNameText->getText();
	const u32 strSize = strw.size();
	if (strSize)
	{
		const wchar_t lastChar = strw[strSize - 1];
		if (lastChar != L'/' && lastChar != L'\\')
			strw += L"/";
		strw += FileNameEditBox->getText();
	}
	return strw;
}
//! called if an event happened.
bool CGUIFileSelector::OnEvent(const SEvent& event)
{
	if (isEnabled())
	{
		switch (event.EventType)
		{
		case EET_KEY_INPUT_EVENT:
			if (event.KeyInput.Key == KEY_RETURN && !event.KeyInput.PressedDown && FileSystem)
				setCurrentDirectory(FileNameText->getText());
			else if (event.KeyInput.Key == KEY_ESCAPE)
			{
				// Si on appuie sur la touche Echap, on simule un appui sur le bouton annuler/fermer
				if (event.KeyInput.PressedDown) // La touche a été pressée : on appuie sur le bouton
				{
					if (CloseButton)
					CloseButton->setPressed(true);
				}
				else // La touche a été relevée : on active l'action du bouton : on masque cette fenêtre
				{
					sendCancelEvent();
					return true;
				}
			}
			break;
		case EET_GUI_EVENT:
			switch(event.GUIEvent.EventType)
			{
			case EGET_COMBO_BOX_CHANGED:
				if (event.GUIEvent.Caller == FilterComboBox)
					fillListBox();
				//else if (FileSystem)	// change drive
				//	setCurrentDirectory(io::path(DriveBox->getText()));
				break;
			case EGET_ELEMENT_FOCUS_LOST:
				if (event.GUIEvent.Caller == this)
					Dragging = false;
				else if (event.GUIEvent.Caller == FileNameEditBox && DialogType == EFST_SAVE_DIALOG)
				{
					// Si c'est une boîte de dialogue d'enregistrement de fichiers, on enlève les caractères interdits dans les noms de fichiers + les permiers et derniers espaces du nom de fichier
					core::stringw fileName = FileNameEditBox->getText();

					// Caractères interdits : / \ ? % * : | " < >
					fileName.removeChars(L"/\\?%*:|\"<>");
					fileName.trim();

					// Supprime les derniers espaces juste avant l'extension
					const int extensionPos = fileName.findLast(L'.');
					if (extensionPos >= 0)
					{
						const core::stringw extension = fileName.subString(extensionPos, fileName.size() - extensionPos);
						fileName.remove(extension);
						fileName.trim();
						fileName.append(extension);
					}

					FileNameEditBox->setText(fileName.c_str());
				}
				break;
			case EGET_BUTTON_CLICKED:
				if (event.GUIEvent.Caller == CloseButton || event.GUIEvent.Caller == CancelButton)
				{
					sendCancelEvent();
					return true;
				}
				else if (event.GUIEvent.Caller == OKButton)
					return selectFileEvent();
				else if (event.GUIEvent.Caller == deleteFileButton && FileNameEditBox && FileBox && FileList && FileSystem)
				{
					// Sélectionne le fichier actuellement nommé dans la liste s'il est trouvé
					core::stringw fileName = FileNameEditBox->getText();
					const u32 itemCount = FileBox->getItemCount();
					const u32 fileNameSize = fileName.size();
					FileBox->setSelected(-1);
					if (fileNameSize)
						for (u32 i = 0; i < itemCount; ++i)
							if (fileName.equalsn(FileBox->getListItem(i), fileNameSize))
							{
								FileBox->setSelected(i);
								fileName = FileBox->getListItem(i);
								FileNameEditBox->setText(fileName.c_str());
								break;
							}

					// Vérifie qu'une entrée est bien sélectionnée
					const int selected = FileBox->getSelected();
					if (FileBox->getSelected() >= 0)
					{
						if (!FileList->isDirectory(selected))				// Vérifie que ce fichier n'est pas un dossier
							if (FileSystem->existFile(getFileNameStr()))	// Vérifie que le fichier existe
								askUserDeleteFileConfirmation();	// Demande à l'utilisateur la confirmation pour supprimer ce fichier
					}
				}
				break;

			case EGET_LISTBOX_CHANGED:
				{
					const int selected = FileBox->getSelected();
					if (FileList && !FileList->isDirectory(selected))
					{
						core::stringw strw;
						//strw = FileSystem->getWorkingDirectory();
						//if (strw[strw.size()-1] != '\\')
						//  strw += "\\";
						strw += FileBox->getListItem(selected);
						//FileNameText->setText(strw.c_str());
						FileNameEditBox->setText(strw.c_str());
					}
				}
				break;
			case EGET_LISTBOX_SELECTED_AGAIN:
				{
					const int selected = FileBox->getSelected();
					if (FileList && FileSystem)
					{
						if (FileList->isDirectory(selected))
							setCurrentDirectory(FileList->getFileName(selected));
						else if (selectFileEvent())
							return true;
					}
				}
				break;

			case EGET_EDITBOX_ENTER:
				if (event.GUIEvent.Caller == FileNameEditBox && FileNameEditBox && FileBox && FileList)
				{
					core::stringw fileName = FileNameEditBox->getText();

					// Si c'est une boîte de dialogue d'enregistrement de fichiers, on enlève les caractères interdits dans les noms de fichiers + les permiers et derniers espaces du nom de fichier
					if (DialogType == EFST_SAVE_DIALOG)
					{
						// Caractères interdits : / \ ? % * : | " < >
						fileName.removeChars(L"/\\?%*:|\"<>");
						fileName.trim();

						// Supprime les derniers espaces juste avant l'extension
						const int extensionPos = fileName.findLast(L'.');
						if (extensionPos >= 0)
						{
							const core::stringw extension = fileName.subString(extensionPos, fileName.size() - extensionPos);
							fileName.remove(extension);
							fileName.trim();
							fileName.append(extension);
						}

						FileNameEditBox->setText(fileName.c_str());
					}

					// Sélectionne le fichier actuellement nommé dans la liste s'il est trouvé
					const u32 itemCount = FileBox->getItemCount();
					const u32 fileNameSize = fileName.size();
					FileBox->setSelected(-1);
					if (fileNameSize)
						for (u32 i = 0; i < itemCount; ++i)
							if (fileName.equalsn(FileBox->getListItem(i), fileNameSize))
							{
								FileBox->setSelected(i);
								fileName = FileBox->getListItem(i);
								FileNameEditBox->setText(fileName.c_str());
								break;
							}

					// Vérifie qu'une entrée est bien sélectionnée sauf si cette boîte de dialogue sert à enregistrer un fichier
					const int selected = FileBox->getSelected();
					if (selected >= 0 || DialogType == EFST_SAVE_DIALOG)
					{
						// Vérifie que l'entrée choisie n'est pas un dossier si on ne peut pas les sélectionner
						if (!FileList->isDirectory(selected))
							return selectFileEvent();	// Choisi le fichier entré dans la zone de texte
						else
						{
							core::stringw dir = FileNameText->getText();
							if (dir.lastChar() != L'/' && dir.lastChar() != L'\\')
								dir.append('/');
							dir.append(fileName);
							FileNameText->setText(dir.c_str());
							FileNameEditBox->setText(L"");

							// Change le dossier actuel
							setCurrentDirectory(dir);

							return true;
						}
					}
				}
				else if (event.GUIEvent.Caller == FileNameText && FileNameText && FileSystem)
					setCurrentDirectory(FileNameText->getText());	// Change le dossier actuel
				break;

			case EGET_EDITBOX_CHANGED:	// Ajouté
				if (event.GUIEvent.Caller == FileNameEditBox && FileNameEditBox && FileBox)
				{
					core::stringw fileName = FileNameEditBox->getText();

					// Si c'est une boîte de dialogue d'enregistrement de fichiers, on enlève les caractères interdits dans les noms de fichiers
					if (DialogType == EFST_SAVE_DIALOG)
					{
						// Caractères interdits : / \ ? % * : | " < >
						fileName.removeChars(L"/\\?%*:|\"<>");
						FileNameEditBox->setText(fileName.c_str());
					}

					// Sélectionne le fichier actuellement nommé dans la liste s'il est trouvé
					const u32 itemCount = FileBox->getItemCount();
					const u32 fileNameSize = fileName.size();
					FileBox->setSelected(-1);
					if (fileNameSize)
						for (u32 i = 0; i < itemCount; ++i)
							if (fileName.equalsn(FileBox->getListItem(i), fileNameSize))
							{
								FileBox->setSelected(i);
								break;
							}
				}
				break;

				// Ajouté :
			case EGET_MESSAGEBOX_CANCEL:
			case EGET_MESSAGEBOX_NO:
			case EGET_MESSAGEBOX_OK:
			case EGET_MESSAGEBOX_YES:
				if (event.GUIEvent.Caller == deleteFileConfirmationWindow && deleteFileConfirmationWindow)
				{
					// Si l'utilisateur a cliqué sur un bouton (le bouton Oui est inclus) :
					// On indique que la fenêtre de confirmation est fermée
					deleteFileConfirmationWindow = NULL;

					if (event.GUIEvent.EventType == EGET_MESSAGEBOX_YES && CAN_WRITE_ON_DISK)
					{
						// L'utilisateur a confirmé : on supprime le fichier :

						// Vérifie que le fichier existe et le supprime
						const core::stringw path = getFileNameStr();
						if (path.size())
						{
							if (FileSystem->existFile(path))
							{
								// Supprime ce fichier
								const core::stringc pathStr = core::stringc(path);	// Conversion de stringw en stringc
								const int result = ::remove(pathStr.c_str());		// Conversion de stringc en char*

								// Affiche un message de log suivant si la suppression du fichier a réussie ou non
								if (result == 0)
								{
									LOG("Fichier supprimé : " << pathStr.c_str(), ELL_INFORMATION);
								}
								else
								{
									LOG("Erreur lors de la suppression du fichier : " << pathStr.c_str(), ELL_INFORMATION);
								}

								// Efface le nom du fichier actuel
								FileNameEditBox->setText(L"");

								// Rempli la liste à nouveau
								fillListBox();
							}
						}
					}
				}
				else if (event.GUIEvent.Caller == overwriteFileConfirmationWindow && overwriteFileConfirmationWindow)
				{
					// Si l'utilisateur a cliqué sur un bouton (le bouton Oui est inclus) :
					// On indique que la fenêtre de confirmation est fermée
					overwriteFileConfirmationWindow = NULL;

					if (event.GUIEvent.EventType == EGET_MESSAGEBOX_YES)
					{
						// L'utilisateur a confirmé : on écrase le fichier :

						// Envoie l'évènement indiquant que l'utilisateur a choisi le fichier
						return selectFileEvent(true);
					}
				}
				break;
			}
			break;

		case EET_MOUSE_INPUT_EVENT:
			switch(event.MouseInput.Event)
			{
			case EMIE_LMOUSE_PRESSED_DOWN:
				DragStart.X = event.MouseInput.X;
				DragStart.Y = event.MouseInput.Y;
				Dragging = true;
				Environment->setFocus(this);
				return true;

			case EMIE_LMOUSE_LEFT_UP:
				Dragging = false;
				Environment->removeFocus(this);
				return true;

			case EMIE_MOUSE_MOVED:
				if (Dragging)
				{
					// gui window should not be dragged outside its parent
					if (Parent)
						if (event.MouseInput.X < Parent->getAbsolutePosition().UpperLeftCorner.X + 1 ||
							event.MouseInput.Y < Parent->getAbsolutePosition().UpperLeftCorner.Y + 1 ||
							event.MouseInput.X > Parent->getAbsolutePosition().LowerRightCorner.X - 1 ||
							event.MouseInput.Y > Parent->getAbsolutePosition().LowerRightCorner.Y - 1)
							return true;

					move(core::vector2di(event.MouseInput.X - DragStart.X, event.MouseInput.Y - DragStart.Y));
					DragStart.X = event.MouseInput.X;
					DragStart.Y = event.MouseInput.Y;
					return true;
				}
				break;
			}
			break;
		}
	}

	return Parent ? Parent->OnEvent(event) : false;
}
bool CGUIFileSelector::selectFileEvent(bool forceOverride)
{
#if 0	// Désactivé : Peu d'utilité actuellement
	// Si la FileNameEditBox est vide : on la remplit avec le nom du fichier actuellement sélectionné dans la FileList
	if (FileList && FileNameEditBox)
	{
		const core::stringw filename = FileNameEditBox->getText();
		if (!filename.size())
		{
			const int selected = FileBox->getSelected();
			if (selected >= 0)
				FileNameEditBox->setText(FileBox->getListItem((u32)selected));
		}
	}
#endif

	core::stringw fileNameStr = getFileNameStr();

	// Vérifie que l'utilisateur a bien entré un nom de fichier non vide, sinon on renvoie automatiquement false
	if (fileNameStr.size() == 0)
		return false;

	// Recherche si le nom de fichier actuel correspond aux filtres demandés
	bool matches = matchesFileFilter(fileNameStr);

	if (!matches && FileBox && FileList) // Si le nom de fichier actuel ne correspond pas aux filtres demandés
	{
		const int selected = FilterComboBox->getSelected();
		if (selected >= 0)
		{
			// Ajoute l'extension au nom de fichier pour qu'elle corresponde
			fileNameStr.append('.');
			fileNameStr.append(FileFilters[selected].FileExtension);

			// Vérifie que le nouveau fichier existe
			matches = (DialogType == EFST_SAVE_DIALOG || FileSystem->existFile(fileNameStr));

			// Modifie le nom du fichier dans la zone de texte du nom du fichier si ce fichier existe effectivement
			if (matches)
			{
				core::stringw filename = FileNameEditBox->getText();
				filename.append('.');
				filename.append(FileFilters[selected].FileExtension);
				FileNameEditBox->setText(filename.c_str());
			}
		}
	}

	// Vérifie que le nom de fichier choisi correspond bien aux filtres de type demandés
	if (matches)
	{
		// Vérifie si le fichier spécifié existe
		const bool fileExists = FileSystem->existFile(fileNameStr);

		// Si on est dans une boîte de dialogue d'enregistrement de fichier et qu'on ne doit pas remplacer de fichier :
		if (DialogType == EFST_SAVE_DIALOG && fileExists && !forceOverride)
		{
			// On affiche une fenêtre de confirmation à l'utilisateur pour lui demander s'il est sûr de vouloir remplacer ce fichier
			askUserOverwriteFileConfirmation();
		}

		// Vérifie aussi que le fichier sélectionné existe (sinon : l'évènement de choix est envoyé même si le fichier n'existe pas, pourvu que le joueur ait tapé l'extension ".ewg" !),
		// sauf si on est une boîte de dialogue d'enregistrement de fichier
		else if (DialogType == EFST_SAVE_DIALOG || fileExists)
		{
			sendSelectedEvent();
			return true;
		}
	}

	return false;
}
void CGUIFileSelector::askUserDeleteFileConfirmation()
{
	// Si la fenêtre de confirmation est déjà créée, on annule
	if (deleteFileConfirmationWindow)
		return;

	if (CAN_WRITE_ON_DISK)
	{
		// Crée la fenêtre demandant une confirmation de suppression du fichier à l'utilisateur
		deleteFileConfirmationWindow = CGUIMessageBox::addMessageBox(Environment, L"Supprimer un fichier", L"Voulez-vous vraiment supprimer le fichier sélectionné ?",
			EMBF_YES | EMBF_NO, this, true);
	}
	else
	{
		// Crée la fenêtre indiquant à l'utilisateur qu'EcoWorld n'a pas les droits suffisants pour écrire sur le disque dur de l'ordinateur cible
		deleteFileConfirmationWindow = CGUIMessageBox::addMessageBox(Environment, L"Erreur : Supprimer un fichier", L"Impossible de supprimer ce fichier : EcoWorld n'a pas les droits d'écriture sur ce disque.",
			EMBF_OK, this, true);
	}

	// Empêche la fenêtre d'être coupée par le rectangle de cette boîte de dialogue :
	// elle peut ainsi être dessinée sur tout l'écran (mais pas déplacée sur tout l'écran !)
	deleteFileConfirmationWindow->setNotClipped(true);

	// Indique que la fenêtre est un sous-élément de cette boîte de dialogue
	deleteFileConfirmationWindow->setSubElement(true);
}
void CGUIFileSelector::askUserOverwriteFileConfirmation()
{
	// Si la fenêtre de confirmation est déjà créée, on annule
	if (overwriteFileConfirmationWindow)
		return;

	// Crée la fenêtre demandant une confirmation de suppression du fichier à l'utilisateur
	overwriteFileConfirmationWindow = CGUIMessageBox::addMessageBox(Environment, L"Remplacer un fichier", L"Voulez-vous vraiment remplacer le fichier sélectionné ?",
		EMBF_YES | EMBF_NO, this, true);

	// Empêche la fenêtre d'être coupée par le rectangle de cette boîte de dialogue :
	// elle peut ainsi être dessinée sur tout l'écran (mais pas déplacée sur tout l'écran !)
	overwriteFileConfirmationWindow->setNotClipped(true);

	// Indique que la fenêtre est un sous-élément de cette boîte de dialogue
	overwriteFileConfirmationWindow->setSubElement(true);
}
void CGUIFileSelector::showAgain()
{
	if (FileSystem)
		prev_working_dir = FileSystem->getWorkingDirectory();

#if 0
	// Désactivé : On efface l'ancien nom du fichier de la zone de texte si cette boîte de dialogue permet d'ouvrir un fichier
	if (FileNameEditBox && DialogType == EFST_OPEN_DIALOG)
		FileNameEditBox->setText(L"");
#endif

	Dragging = false;
	DragStart.set(0, 0);

	setCurrentDirectory(current_dir);

	setVisible(true);

	// Place cette fenêtre et son modal screen au premier plan
	if (modalScreen && modalScreen->getParent())
		modalScreen->getParent()->bringToFront(modalScreen);
	if (Parent)
		Parent->bringToFront(this);

	// Donne le focus à la zone de texte du nom du fichier
	Environment->setFocus(FileNameEditBox);
}
void CGUIFileSelector::hide()
{
	if (FileSystem)
		FileSystem->changeWorkingDirectoryTo(prev_working_dir.c_str());

	setVisible(false);
}
//! draws the element and its children
void CGUIFileSelector::draw()
{
	if (!IsVisible)
		return;

	IGUISkin* const skin = Environment->getSkin();

	core::recti rect = skin->draw3DWindowBackground(this, true, skin->getColor(EGDC_ACTIVE_BORDER),
		AbsoluteRect, &AbsoluteClippingRect);
	if (Text.size())
	{
		rect.UpperLeftCorner.X += 2;
		rect.LowerRightCorner.X -= skin->getSize(EGDS_WINDOW_BUTTON_WIDTH) + 5;

		IGUIFont* const font = skin->getFont(EGDF_WINDOW);
		if (font)
			font->draw(Text.c_str(), rect, skin->getColor(EGDC_ACTIVE_CAPTION), false, true,
				&AbsoluteClippingRect);
	}

	IGUIElement::draw();
}
bool CGUIFileSelector::matchesFileFilter(const core::stringw& s)
{
	const s32 selected = FilterComboBox->getSelected();

	if (selected < 0)
		return false;
	else if (selected >= (s32)FileFilters.size())
		return true; // 'All Files' selectable

	const s32 pos = s.findLast('.'); // Find the last '.' so we can check the file extension
	if (pos >= 0)
		return FileFilters[selected].FileExtension.equals_ignore_case(core::stringw(&s.c_str()[pos+1]));
	return false;
}
bool CGUIFileSelector::matchesFileFilter(const core::stringw& s, const core::stringw& f)
{
	const s32 pos = s.findLast('.'); // Find the last '.' so we can check the file extension
	return f.equals_ignore_case(core::stringw(&s.c_str()[pos+1]));
}
//! fills the listbox with files.
void CGUIFileSelector::fillListBox()
{
	if (!FileSystem || !FileBox)
		return;

	if (FileList)
		FileList->drop();

	FileBox->clear();

	FileList = FileSystem->createFileList();

	const u32 fileCount = FileList->getFileCount();
	for (u32 i = 0; i < fileCount; ++i)
	{
		const core::stringw s = FileList->getFileName(i);

		// We just want a list of directories and those matching the file filter
		if (FileList->isDirectory(i))
		{
			if (DirectoryIconIdx != -1)
				FileBox->addItem(s.c_str(), DirectoryIconIdx);
			else
				FileBox->addItem(s.c_str());
		}
		else if (matchesFileFilter(s))
		{
			if (FilterComboBox->getSelected() >= (s32)FileFilters.size())
			{
				if (FileIconIdx != -1)
				{
					s32 iconIdx = FileIconIdx;
					const u32 fileFiltersSize = FileFilters.size();
					for (u32 i = 0 ; i < fileFiltersSize; ++i)
					{
						if (matchesFileFilter(s, FileFilters[i].FileExtension))
						{
							iconIdx = FileFilters[i].FileIconIdx;
							break;
						}
					}
					FileBox->addItem(s.c_str(), iconIdx);
				}
				else
					FileBox->addItem(s.c_str());
			}
			else
				FileBox->addItem(s.c_str(), FileFilters[FilterComboBox->getSelected()].FileIconIdx);       
		}
	}

	if (FileNameText)
	{
		const core::stringw s = FileSystem->getWorkingDirectory();
		FileNameText->setText(s.c_str());
	}
}
//! sends the event that the file has been selected.
void CGUIFileSelector::sendSelectedEvent()
{
	SEvent event;
	event.EventType = EET_GUI_EVENT;
	event.GUIEvent.Caller = this;
	event.GUIEvent.EventType = EGET_FILE_SELECTED;
	Parent->OnEvent(event);
}
//! sends the event that the file choose process has been canceld
void CGUIFileSelector::sendCancelEvent()
{
	SEvent event;
	event.EventType = EET_GUI_EVENT;
	event.GUIEvent.Caller = this;
	event.GUIEvent.EventType = EGET_FILE_CHOOSE_DIALOG_CANCELLED;
	Parent->OnEvent(event);
}
void CGUIFileSelector::addFileFilter(wchar_t* name, wchar_t* ext, video::ITexture* texture)
{
	SFileFilter filter(name, ext, texture);

	filter.FileIconIdx = addIcon(texture);

	FileFilters.push_back(filter);

	FilterComboBox->clear();
	core::stringw strw;

	if (FileFilters.size() > 1)
	{
		strw = "Pris en charge : ";
		for (u32 i = 0 ; i < FileFilters.size() ; ++i)
		{
			strw += " .";
			strw += FileFilters[i].FileExtension;
		}
		FilterComboBox->addItem(strw.c_str());
	}

	for (u32 i = 0 ; i < FileFilters.size() ; ++i)
	{
		strw = FileFilters[i].FilterName;
		strw += " (*.";
		strw += FileFilters[i].FileExtension;
		strw += ")";
		FilterComboBox->addItem(strw.c_str());
	}

	FilterComboBox->addItem(L"Tous les fichiers");

	// Désactivé : L'ajout de filtre de fichiers n'est effectué qu'à la création de la boîte de dialogue : la liste des fichiers sera seulement mise à jour à son affichage (optimisation)
	//fillListBox();
}
u32 CGUIFileSelector::addIcon(video::ITexture* texture)
{
	if (!SpriteBank || !texture)
		return 0;

	// load and add the texture to the bank     
	SpriteBank->addTexture(texture);
	u32 textureIndex = SpriteBank->getTextureCount() - 1;
	// now lets get the sprite bank's rectangles and add some for our animation
	core::array<core::recti >& rectangles = SpriteBank->getPositions();
	u32 firstRect = rectangles.size();
	// remember that rectangles are not in pixels, they enclose pixels!
	// to draw a rectangle around the pixel at 0,0, it would recti(0,0, 1,1)
	rectangles.push_back(core::recti(0,0, 16,16));

	// now we make a sprite..
	SGUISprite sprite;
	sprite.frameTime = 30;
	// add some frames of animation.
	SGUISpriteFrame frame;
	frame.rectNumber = firstRect;
	frame.textureNumber = textureIndex;

	// add this frame
	sprite.Frames.push_back(frame);
	// add the sprite
	u32 spriteIndex = SpriteBank->getSprites().size();
	SpriteBank->getSprites().push_back(sprite); 

	return textureIndex;
}
