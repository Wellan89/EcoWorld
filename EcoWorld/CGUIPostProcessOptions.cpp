#include "CGUIPostProcessOptions.h"
#include "CPostProcessManager.h"
#include "GameConfiguration.h"
#include "Game.h"	// Pour avoir accès à PostProcess

CGUIPostProcessOptions::CGUIPostProcessOptions(IGUIEnvironment* environment, IGUIElement* parent, int id, const core::recti& rectangle)
 : IGUIElement(EGUIET_ELEMENT, environment, parent, id, rectangle), avalaibleEffectsListBox(NULL), enabledEffectsListBox(NULL)
{
#ifdef _DEBUG
	setDebugName("CGUIPostProcessOptions");
#endif



	// Crée les éléments de la GUI permettant de gérer les effets post-rendu :

	// Constantes pour le placement des éléments : à conserver cohérentes avec les valeurs de CGUIOptionsMenu::createMenu() :
	const float
		positionTexteX = 0.03f,	// Equivalent à minX en fait, mais nom conservé pour conserver la cohérence avec les valeurs de CGUIOptionsMenu::createMenu()
		maxCheckBoxX = 0.6f,	// positionValeurX + tailleValeurX = 0.3f + 0.3f = 0.6f

		minY = 0.03f,
		ecartY = 0.02f,
		tailleY = 0.04f,
		
		list1MinX = 0.03f,
		list2MinX = 0.51f,
		listSize = 0.3f,
		ecartBoutonX = 0.02f,
		tailleBoutonX = 0.14f,
		
		textPosY = 0.17f,
		textSizeY = 0.05f,
		listMaxY = 0.97f,
		boutonPosY = 0.5f;	// Boutons légèrement au-dessus du centre des listes (ils seraient au centre si boutonPosY = 0.545f)

	postProcessEnabledCheckBox =
		Environment->addCheckBox(gameConfig.usePostProcessEffects,				getAbsoluteRectangle(positionTexteX,	minY,						maxCheckBoxX,	minY + tailleY,					rectangle),
		this, -1, L"Effets post-rendu");
	postProcessEnabledCheckBox->setToolTipText(L"Cochez cette case pour activer les effets post-rendu");
	cameraShakingEnabledCheckBox =
		Environment->addCheckBox(gameConfig.postProcessShakeCameraOnDestroying,	getAbsoluteRectangle(positionTexteX,	minY + ecartY + tailleY,	maxCheckBoxX,	minY + ecartY + tailleY * 2,	rectangle),
		this, -1, L"Tremblements de la caméra");
	postProcessEnabledCheckBox->setToolTipText(L"Cochez cette case pour activer les effets de tremblement de la caméra lors de la destruction d'un bâtiment");

	avalaibleEffectsStaticText = environment->addStaticText(L"Effets disponibles :",	getAbsoluteRectangle(list1MinX, textPosY, list1MinX + listSize, textPosY + textSizeY, rectangle),
		false, true, this);
	avalaibleEffectsStaticText->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);
	enabledEffectsStaticText = environment->addStaticText(L"Effets activés :",			getAbsoluteRectangle(list2MinX, textPosY, list2MinX + listSize, textPosY + textSizeY, rectangle),
		false, true, this);
	enabledEffectsStaticText->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);

	avalaibleEffectsListBox = environment->addListBox(	getAbsoluteRectangle(list1MinX, textPosY + textSizeY, list1MinX + listSize, listMaxY, rectangle), this, -1, true);
	avalaibleEffectsListBox->setToolTipText(L"Sélectionnez ici les effets post-rendu disponibles");
	enabledEffectsListBox = environment->addListBox(	getAbsoluteRectangle(list2MinX, textPosY + textSizeY, list2MinX + listSize, listMaxY, rectangle), this, -1, true);
	enabledEffectsListBox->setToolTipText(L"Sélectionnez ici les effets post-rendu activés");

	addButton = environment->addButton(		getAbsoluteRectangle(list1MinX + listSize + ecartBoutonX,	boutonPosY,						list1MinX + listSize + ecartBoutonX + tailleBoutonX,	boutonPosY + tailleY,				rectangle), this, -1,
		L"Activer cet effet", L"Cliquez sur ce bouton pour activer l'effet post-rendu sélectionné");
	removeButton = environment->addButton(	getAbsoluteRectangle(list1MinX + listSize + ecartBoutonX,	boutonPosY + ecartY + tailleY,	list1MinX + listSize + ecartBoutonX + tailleBoutonX,	boutonPosY + ecartY + tailleY * 2,	rectangle), this, -1,
		L"Désactiver cet effet", L"Cliquez sur ce bouton pour désactiver l'effet post-rendu sélectionné");

	putUpButton = environment->addButton(	getAbsoluteRectangle(list2MinX + listSize + ecartBoutonX,	boutonPosY,						list2MinX + listSize + ecartBoutonX + tailleBoutonX,	boutonPosY + tailleY,				rectangle), this, -1,
		L"Monter", L"Cliquez sur ce bouton pour monter cet effet dans la liste");
	putDownButton = environment->addButton(	getAbsoluteRectangle(list2MinX + listSize + ecartBoutonX,	boutonPosY + ecartY + tailleY,	list2MinX + listSize + ecartBoutonX + tailleBoutonX,	boutonPosY + ecartY + tailleY * 2,	rectangle), this, -1,
		L"Descendre", L"Cliquez sur ce bouton pour descendre cet effet dans la liste");



	// Sélectionne le premier effet disponible par défaut
	avalaibleEffectsListBox->setSelected(0);
	setCurrentSelectedAvalaibleEffect(avalaibleEffectsListBox->getSelected());

	// Sélectionne le premier effet activé par défaut
	enabledEffectsListBox->setSelected(0);
	setCurrentSelectedEnabledEffect(enabledEffectsListBox->getSelected());

	// Met à jour les éléments activés de ce menu d'après l'activation des effets post-rendu
	updateEnabledElements(postProcessEnabledCheckBox->isChecked());
}
bool CGUIPostProcessOptions::OnEvent(const SEvent& event)
{
	// On ne gère ici que les évènements de la GUI
	if (event.EventType == EET_GUI_EVENT && IsEnabled)
	{
		switch (event.GUIEvent.EventType)
		{
		case EGET_LISTBOX_CHANGED:
		case EGET_LISTBOX_SELECTED_AGAIN:
			if (event.GUIEvent.Caller == avalaibleEffectsListBox)
			{
				// Indique l'effet disponible actuellement sélectionné
				setCurrentSelectedAvalaibleEffect(avalaibleEffectsListBox->getSelected());
			}
			else if (event.GUIEvent.Caller == enabledEffectsListBox)
			{
				// Indique l'effet activé actuellement sélectionné
				setCurrentSelectedEnabledEffect(enabledEffectsListBox->getSelected());
			}
			break;

		case EGET_BUTTON_CLICKED:
			if (event.GUIEvent.Caller == addButton)
			{
				// Ajoute l'effet disponible sélectionné à la liste des effets activés
				addEffect(avalaibleEffectsListBox->getSelected());
			}
			else if (event.GUIEvent.Caller == removeButton)
			{
				// Supprime cet effet de la liste des effets activés
				removeEffect(enabledEffectsListBox->getSelected());
			}
			else if (event.GUIEvent.Caller == putUpButton)
			{
				// Monte cet effet dans la liste des effets activés
				putUpEffect(enabledEffectsListBox->getSelected());
			}
			else if (event.GUIEvent.Caller == putDownButton)
			{
				// Descend cet effet dans la liste des effets activés
				putDownEffect(enabledEffectsListBox->getSelected());
			}
			break;

		case EGET_CHECKBOX_CHANGED:
			if (event.GUIEvent.Caller == postProcessEnabledCheckBox)
			{
				// Met à jour l'activation des éléments de ce menu suivant l'activation des effets post-rendu
				updateEnabledElements(postProcessEnabledCheckBox->isChecked());
			}
			break;
		}
	}

	return IGUIElement::OnEvent(event);
}
void CGUIPostProcessOptions::updateAvalaibleEffectsList()
{
	if (!game->fileSystem)
		return;

	const u32 lastSelectedItem = avalaibleEffectsListBox->getSelected();
	const core::stringw lastSelectedEffect = lastSelectedItem != -1 ? avalaibleEffectsListBox->getListItem(lastSelectedItem) : L"";

	// Désélectionne la valeur actuellement sélectionnée
	avalaibleEffectsListBox->setSelected(-1);

	// Efface les anciennes valeurs de la liste
	avalaibleEffectsListBox->clear();

	// Vérifie que PostProcess est bien valide, car il est ici nécessaire pour pouvoir créer cette liste
	if (!game->postProcessManager)
		return;

	// Parcours tous les effets chargés par PostProcess pour pouvoir les ajouter un par un à la liste des effets disponibles
	const core::map<core::stringw, CEffectChain*>& avalaibleEffects = game->postProcessManager->getAvalaibleEffects();
	for (core::map<core::stringw, CEffectChain*>::Iterator iter = avalaibleEffects.getIterator(); !iter.atEnd(); iter++)
	{
		// Obtient le nom de l'effet actuel
		const core::stringw& effectName = (*iter).getKey();

		// Parcours la liste pour vérifier qu'un effet de ce nom n'a pas déjà été ajouté :
		// (évite d'avoir deux occurrences d'effets avec un nom identique malgré un rendu final différent : on ne conserve que le premier des deux)
		bool alreadyExists = false;
		const u32 effectsItemCount = avalaibleEffectsListBox->getItemCount();
		for (u32 k = 0; k < effectsItemCount; ++k)
		{
			if (effectName == avalaibleEffectsListBox->getListItem(k))
			{
				alreadyExists = true;
				break;
			}
		}

		// Ajoute cet effet à la liste des effets disponibles
		if (!alreadyExists)
			avalaibleEffectsListBox->addItem(effectName.c_str());
	}

	// Vérifie que l'on peut sélectionner un élement
	const u32 listItemCount = avalaibleEffectsListBox->getItemCount();
	if (listItemCount > 0)
	{
		if (lastSelectedEffect.size() > 0)
		{
			// Cherche le nom de l'effet dans la liste et le sélectionne (s'il n'a pas été trouvé, on sélectionnera le premier élement de la liste)
			int ID = 0;
			for (u32 i = 0; i < listItemCount; ++i)
			{
				if (lastSelectedEffect.equals_ignore_case(avalaibleEffectsListBox->getListItem(i)))
				{
					ID = i;
					break;
				}
			}

			avalaibleEffectsListBox->setSelected(ID);
		}
		else
		{
			// Sinon on sélectionne le premier élement
			avalaibleEffectsListBox->setSelected(0);
		}
	}

	// Indique le nouvel effet disponible actuellement sélectionné
	setCurrentSelectedAvalaibleEffect(avalaibleEffectsListBox->getSelected());
}
void CGUIPostProcessOptions::updateEnabledEffectsList(const GameConfiguration& gameCfg)
{
	if (!game->fileSystem)
		return;

	const u32 lastSelectedItem = enabledEffectsListBox->getSelected();

	// Désélectionne la valeur actuellement sélectionnée
	enabledEffectsListBox->setSelected(-1);

	// Efface les anciennes valeurs de la liste de la GUI et efface la liste interne
	enabledEffectsListBox->clear();
	enabledEffects.clear();

	// Parcours tous les effets chargés par PostProcess pour pouvoir les ajouter un par un à la liste des effets disponibles
	const core::array<core::stringw>& gameConfigEffects = gameCfg.postProcessEffects;
	const u32 gameConfigEffectsSize = gameConfigEffects.size();
	for (u32 i = 0; i < gameConfigEffectsSize; ++i)
	{
		// Ajoute cet effet à la liste des effets activés dans la GUI et dans la liste interne
		const core::stringw& effectName = gameConfigEffects[i];
		enabledEffectsListBox->addItem(effectName.c_str());
		enabledEffects.push_back(effectName);
	}

	// Sélectionne le dernier élément sélectionné (on ne le recherche pas ici par son nom puisque plusieurs effets de la liste peuvent avoir un nom identique)
	enabledEffectsListBox->setSelected(lastSelectedItem);

	// Si le dernier élément sélectionné n'a pas été trouvé, on sélectionné alors le premier élément de cette liste
	if (enabledEffectsListBox->getSelected() < 0)
		enabledEffectsListBox->setSelected(0);

	// Indique le nouvel effet activé actuellement sélectionné
	setCurrentSelectedEnabledEffect(enabledEffectsListBox->getSelected());
}
void CGUIPostProcessOptions::updateFromGameConfig(const GameConfiguration& gameCfg)
{
	// Met tout d'abord à jour la liste des effets post-rendu disponibles
	updateAvalaibleEffectsList();

	// Met ensuite à jour la liste des effets activés suivant ces paramêtres de configuration
	updateEnabledEffectsList(gameCfg);

	// Met à jour les options des effets post-rendu suivant les paramêtres de configuration spécifiés
	postProcessEnabledCheckBox->setChecked(gameCfg.usePostProcessEffects);
	cameraShakingEnabledCheckBox->setChecked(gameCfg.postProcessShakeCameraOnDestroying);

	// Met à jour les éléments activés de ce menu d'après l'activation des effets post-rendu
	updateEnabledElements(postProcessEnabledCheckBox->isChecked());
}
void CGUIPostProcessOptions::updateEnabledElements(bool postProcessEnabled)
{
	// Active/Désactive tous les éléments de ce menu suivant l'activation actuelle des effets post-rendu
	cameraShakingEnabledCheckBox->setEnabled(postProcessEnabled);
	avalaibleEffectsStaticText->setEnabled(postProcessEnabled);
	enabledEffectsStaticText->setEnabled(postProcessEnabled);
	avalaibleEffectsListBox->setEnabled(postProcessEnabled);
	enabledEffectsListBox->setEnabled(postProcessEnabled);

	if (postProcessEnabled)
	{		
		// Met à jour tous les éléments activés ou désactivés qui dépendent des sélections des listes de ce menu
		setCurrentSelectedAvalaibleEffect(avalaibleEffectsListBox->getSelected());
		setCurrentSelectedEnabledEffect(enabledEffectsListBox->getSelected());
	}
	else
	{
		// Désactive tous les éléments de ce menu dépendants des sélections des listes de ce menu
		addButton->setEnabled(false);
		removeButton->setEnabled(false);
		putUpButton->setEnabled(false);
		putDownButton->setEnabled(false);
	}
}
void CGUIPostProcessOptions::setCurrentSelectedAvalaibleEffect(int avalaibleEffectIndex)
{
	// On suppose ici que l'effet spécifié est déjà sélectionné dans la liste des effets disponibles

	// Active ou désactive le bouton "Activer cet effet" suivant si un effet est réellement sélectionné ou non
	addButton->setEnabled(avalaibleEffectIndex >= 0 && (u32)avalaibleEffectIndex < avalaibleEffectsListBox->getItemCount());
}
void CGUIPostProcessOptions::setCurrentSelectedEnabledEffect(int enabledEffectIndex)
{
	// On suppose ici que l'effet spécifié est déjà sélectionné dans la liste des effets activés

	// Détermine si un effet est réellement sélectionné
	if (enabledEffectIndex >= 0 && (u32)enabledEffectIndex < enabledEffects.size())
	{
		// Active le bouton "Désactiver cet effet"
		removeButton->setEnabled(true);

		// Désactive le bouton "Monter" si cet effet est en haut de la liste
		putUpButton->setEnabled(enabledEffectIndex != 0);

		// Désactive le bouton "Descendre" si cet effet est en bas de la liste
		putDownButton->setEnabled((u32)enabledEffectIndex != enabledEffects.size() - 1);
	}
	else
	{
		// Désactive le bouton "Désactiver cet effet"
		removeButton->setEnabled(false);

		// Désactive le bouton "Monter"
		putUpButton->setEnabled(false);

		// Désactive le bouton "Descendre"
		putDownButton->setEnabled(false);
	}
}
void CGUIPostProcessOptions::addEffect(int avalaibleEffectIndex)
{
	if (avalaibleEffectIndex < 0 || (u32)avalaibleEffectIndex >= avalaibleEffectsListBox->getItemCount())
		return;

	// Obtient le nom de l'effet sélectionné dans la liste des effets disponibles
	const core::stringw effectName = avalaibleEffectsListBox->getListItem(avalaibleEffectIndex);

	// Obtient l'index de l'effet actuellement sélectionné dans la liste des effets activés
	const int selected = enabledEffectsListBox->getSelected();

	// Vérifie si un effet activé est bien sélectionné et que ce n'est pas le dernier de la liste des effets activés :
	if (selected >= 0 && (u32)selected < enabledEffectsListBox->getItemCount() - 1)
	{
		// Un effet activé est bien sélectionné :

		// Ajoute cet effet dans la liste des effets activés
		core::list<core::stringw>::Iterator it = enabledEffects.begin();
		if (selected > 0)
			it += selected;
		enabledEffects.insert_after(it, effectName);

		// Ajoute un élément représentant cet effet dans la liste des effets activés de la GUI et le sélectionné
		enabledEffectsListBox->setSelected(enabledEffectsListBox->insertItem(selected + 1, effectName.c_str(), -1));
	}
	else
	{
		// Aucun effet activé n'est sélectionné, ou l'effet sélectionné est le dernier de la liste des effets activés :

		// Ajoute cet effet à la fin de la liste des effets activés
		enabledEffects.push_back(effectName);

		// Ajoute un élément représentant cet effet à la fin de la liste des effets activés de la GUI et le sélectionné
		enabledEffectsListBox->setSelected(enabledEffectsListBox->addItem(effectName.c_str()));
	}

	// Indique qu'un nouvel effet activé a été sélectionné
	setCurrentSelectedEnabledEffect(enabledEffectsListBox->getSelected());
}
void CGUIPostProcessOptions::removeEffect(int enabledEffectIndex)
{
	if (enabledEffectIndex < 0 || (u32)enabledEffectIndex >= enabledEffects.size())
		return;

	// Supprime cet effet de la liste des effets activés de la GUI
	enabledEffectsListBox->removeItem((u32)enabledEffectIndex);

	// Sélectionne l'élément d'index inférieur dans la liste des effets activés de la GUI s'il existe, sinon celui avec cet index
	enabledEffectsListBox->setSelected((enabledEffectIndex == 0 && (u32)enabledEffectIndex < enabledEffects.size()) ? enabledEffectIndex : enabledEffectIndex - 1);

	// Indique qu'un nouvel effet activé a été sélectionné
	setCurrentSelectedEnabledEffect(enabledEffectsListBox->getSelected());

	// Supprime cet effet de la liste des effets activés
	core::list<core::stringw>::Iterator it = enabledEffects.begin();
	if (enabledEffectIndex > 0)	it += enabledEffectIndex;
	enabledEffects.erase(it);
}
void CGUIPostProcessOptions::putUpEffect(int enabledEffectIndex)
{
	if (enabledEffectIndex < 1 || (u32)enabledEffectIndex >= enabledEffects.size())
		return;

	// Détermine le nouvel index de cet effet dans la liste des effets activés
	u32 newEnabledEffectIndex = (u32)enabledEffectIndex - 1;

	// Echange la position de cet effet et de l'effet qui est actuellement au-dessus de lui dans la liste de la GUI, puis sélectionne cet item à son nouvel index
	enabledEffectsListBox->swapItems((u32)enabledEffectIndex, newEnabledEffectIndex);
	enabledEffectsListBox->setSelected(newEnabledEffectIndex);

	// Indique qu'un nouvel effet activé a été sélectionné
	setCurrentSelectedEnabledEffect(enabledEffectsListBox->getSelected());

	// Déplace cet effet dans la liste des effets activés : retiens son nom, le retire de la liste, puis l'ajoute à la liste à sa nouvelle position
	core::list<core::stringw>::Iterator it = enabledEffects.begin();
	it += enabledEffectIndex;
	const core::stringw effectName = (*it);
	enabledEffects.erase(it);	// Après cette fonction, it pointe toujours sur NULL
	it = enabledEffects.begin();
	it += newEnabledEffectIndex;
	enabledEffects.insert_before(it, effectName);
}
void CGUIPostProcessOptions::putDownEffect(int enabledEffectIndex)
{
	if (enabledEffectIndex < 0 || (u32)enabledEffectIndex >= enabledEffects.size() - 1)
		return;

	// Détermine le nouvel index de cet effet dans la liste des effets activés
	u32 newEnabledEffectIndex = (u32)enabledEffectIndex + 1;

	// Echange la position de cet effet et de l'effet qui est actuellement en-dessous de lui dans la liste de la GUI, puis sélectionne cet item à son nouvel index
	enabledEffectsListBox->swapItems((u32)enabledEffectIndex, newEnabledEffectIndex);
	enabledEffectsListBox->setSelected(newEnabledEffectIndex);

	// Indique qu'un nouvel effet activé a été sélectionné
	setCurrentSelectedEnabledEffect(enabledEffectsListBox->getSelected());

	// Déplace cet effet dans la liste des effets activés : retiens son nom, le retire de la liste, puis l'ajoute à la liste à sa nouvelle position
	core::list<core::stringw>::Iterator it = enabledEffects.begin();
	it += enabledEffectIndex;
	const core::stringw effectName = (*it);
	enabledEffects.erase(it);	// Après cette fonction, it pointe toujours sur NULL
	it = enabledEffects.begin();
	it += enabledEffectIndex;
	enabledEffects.insert_after(it, effectName);
}
bool CGUIPostProcessOptions::needDepthPass() const
{
	// PostProcess doit avoir auparavant avoir été initialisé pour pouvoir déterminer quels effets nécessitent le rendu de la profondeur de la scène
	if (!game->postProcessManager)
		return false;

	// Parcours tous les effets activés et détermine si l'un d'entre eux nécessite le rendu de la profondeur de la scène
	const core::list<core::stringw>::ConstIterator END = enabledEffects.end();
	for (core::list<core::stringw>::ConstIterator it = enabledEffects.begin(); it != END; ++it)
		if (game->postProcessManager->needDepthPass(*it))
			return true;

	return false;
}
