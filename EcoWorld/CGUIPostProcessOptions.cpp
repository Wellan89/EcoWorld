#include "CGUIPostProcessOptions.h"
#include "CPostProcessManager.h"
#include "GameConfiguration.h"
#include "Game.h"	// Pour avoir acc�s � PostProcess

CGUIPostProcessOptions::CGUIPostProcessOptions(IGUIEnvironment* environment, IGUIElement* parent, int id, const core::recti& rectangle)
 : IGUIElement(EGUIET_ELEMENT, environment, parent, id, rectangle), avalaibleEffectsListBox(NULL), enabledEffectsListBox(NULL)
{
#ifdef _DEBUG
	setDebugName("CGUIPostProcessOptions");
#endif



	// Cr�e les �l�ments de la GUI permettant de g�rer les effets post-rendu :

	// Constantes pour le placement des �l�ments : � conserver coh�rentes avec les valeurs de CGUIOptionsMenu::createMenu() :
	const float
		positionTexteX = 0.03f,	// Equivalent � minX en fait, mais nom conserv� pour conserver la coh�rence avec les valeurs de CGUIOptionsMenu::createMenu()
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
		boutonPosY = 0.5f;	// Boutons l�g�rement au-dessus du centre des listes (ils seraient au centre si boutonPosY = 0.545f)

	postProcessEnabledCheckBox =
		Environment->addCheckBox(gameConfig.usePostProcessEffects,				getAbsoluteRectangle(positionTexteX,	minY,						maxCheckBoxX,	minY + tailleY,					rectangle),
		this, -1, L"Effets post-rendu");
	postProcessEnabledCheckBox->setToolTipText(L"Cochez cette case pour activer les effets post-rendu");
	cameraShakingEnabledCheckBox =
		Environment->addCheckBox(gameConfig.postProcessShakeCameraOnDestroying,	getAbsoluteRectangle(positionTexteX,	minY + ecartY + tailleY,	maxCheckBoxX,	minY + ecartY + tailleY * 2,	rectangle),
		this, -1, L"Tremblements de la cam�ra");
	postProcessEnabledCheckBox->setToolTipText(L"Cochez cette case pour activer les effets de tremblement de la cam�ra lors de la destruction d'un b�timent");

	avalaibleEffectsStaticText = environment->addStaticText(L"Effets disponibles :",	getAbsoluteRectangle(list1MinX, textPosY, list1MinX + listSize, textPosY + textSizeY, rectangle),
		false, true, this);
	avalaibleEffectsStaticText->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);
	enabledEffectsStaticText = environment->addStaticText(L"Effets activ�s :",			getAbsoluteRectangle(list2MinX, textPosY, list2MinX + listSize, textPosY + textSizeY, rectangle),
		false, true, this);
	enabledEffectsStaticText->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);

	avalaibleEffectsListBox = environment->addListBox(	getAbsoluteRectangle(list1MinX, textPosY + textSizeY, list1MinX + listSize, listMaxY, rectangle), this, -1, true);
	avalaibleEffectsListBox->setToolTipText(L"S�lectionnez ici les effets post-rendu disponibles");
	enabledEffectsListBox = environment->addListBox(	getAbsoluteRectangle(list2MinX, textPosY + textSizeY, list2MinX + listSize, listMaxY, rectangle), this, -1, true);
	enabledEffectsListBox->setToolTipText(L"S�lectionnez ici les effets post-rendu activ�s");

	addButton = environment->addButton(		getAbsoluteRectangle(list1MinX + listSize + ecartBoutonX,	boutonPosY,						list1MinX + listSize + ecartBoutonX + tailleBoutonX,	boutonPosY + tailleY,				rectangle), this, -1,
		L"Activer cet effet", L"Cliquez sur ce bouton pour activer l'effet post-rendu s�lectionn�");
	removeButton = environment->addButton(	getAbsoluteRectangle(list1MinX + listSize + ecartBoutonX,	boutonPosY + ecartY + tailleY,	list1MinX + listSize + ecartBoutonX + tailleBoutonX,	boutonPosY + ecartY + tailleY * 2,	rectangle), this, -1,
		L"D�sactiver cet effet", L"Cliquez sur ce bouton pour d�sactiver l'effet post-rendu s�lectionn�");

	putUpButton = environment->addButton(	getAbsoluteRectangle(list2MinX + listSize + ecartBoutonX,	boutonPosY,						list2MinX + listSize + ecartBoutonX + tailleBoutonX,	boutonPosY + tailleY,				rectangle), this, -1,
		L"Monter", L"Cliquez sur ce bouton pour monter cet effet dans la liste");
	putDownButton = environment->addButton(	getAbsoluteRectangle(list2MinX + listSize + ecartBoutonX,	boutonPosY + ecartY + tailleY,	list2MinX + listSize + ecartBoutonX + tailleBoutonX,	boutonPosY + ecartY + tailleY * 2,	rectangle), this, -1,
		L"Descendre", L"Cliquez sur ce bouton pour descendre cet effet dans la liste");



	// S�lectionne le premier effet disponible par d�faut
	avalaibleEffectsListBox->setSelected(0);
	setCurrentSelectedAvalaibleEffect(avalaibleEffectsListBox->getSelected());

	// S�lectionne le premier effet activ� par d�faut
	enabledEffectsListBox->setSelected(0);
	setCurrentSelectedEnabledEffect(enabledEffectsListBox->getSelected());

	// Met � jour les �l�ments activ�s de ce menu d'apr�s l'activation des effets post-rendu
	updateEnabledElements(postProcessEnabledCheckBox->isChecked());
}
bool CGUIPostProcessOptions::OnEvent(const SEvent& event)
{
	// On ne g�re ici que les �v�nements de la GUI
	if (event.EventType == EET_GUI_EVENT && IsEnabled)
	{
		switch (event.GUIEvent.EventType)
		{
		case EGET_LISTBOX_CHANGED:
		case EGET_LISTBOX_SELECTED_AGAIN:
			if (event.GUIEvent.Caller == avalaibleEffectsListBox)
			{
				// Indique l'effet disponible actuellement s�lectionn�
				setCurrentSelectedAvalaibleEffect(avalaibleEffectsListBox->getSelected());
			}
			else if (event.GUIEvent.Caller == enabledEffectsListBox)
			{
				// Indique l'effet activ� actuellement s�lectionn�
				setCurrentSelectedEnabledEffect(enabledEffectsListBox->getSelected());
			}
			break;

		case EGET_BUTTON_CLICKED:
			if (event.GUIEvent.Caller == addButton)
			{
				// Ajoute l'effet disponible s�lectionn� � la liste des effets activ�s
				addEffect(avalaibleEffectsListBox->getSelected());
			}
			else if (event.GUIEvent.Caller == removeButton)
			{
				// Supprime cet effet de la liste des effets activ�s
				removeEffect(enabledEffectsListBox->getSelected());
			}
			else if (event.GUIEvent.Caller == putUpButton)
			{
				// Monte cet effet dans la liste des effets activ�s
				putUpEffect(enabledEffectsListBox->getSelected());
			}
			else if (event.GUIEvent.Caller == putDownButton)
			{
				// Descend cet effet dans la liste des effets activ�s
				putDownEffect(enabledEffectsListBox->getSelected());
			}
			break;

		case EGET_CHECKBOX_CHANGED:
			if (event.GUIEvent.Caller == postProcessEnabledCheckBox)
			{
				// Met � jour l'activation des �l�ments de ce menu suivant l'activation des effets post-rendu
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

	// D�s�lectionne la valeur actuellement s�lectionn�e
	avalaibleEffectsListBox->setSelected(-1);

	// Efface les anciennes valeurs de la liste
	avalaibleEffectsListBox->clear();

	// V�rifie que PostProcess est bien valide, car il est ici n�cessaire pour pouvoir cr�er cette liste
	if (!game->postProcessManager)
		return;

	// Parcours tous les effets charg�s par PostProcess pour pouvoir les ajouter un par un � la liste des effets disponibles
	const core::map<core::stringw, CEffectChain*>& avalaibleEffects = game->postProcessManager->getAvalaibleEffects();
	for (core::map<core::stringw, CEffectChain*>::Iterator iter = avalaibleEffects.getIterator(); !iter.atEnd(); iter++)
	{
		// Obtient le nom de l'effet actuel
		const core::stringw& effectName = (*iter).getKey();

		// Parcours la liste pour v�rifier qu'un effet de ce nom n'a pas d�j� �t� ajout� :
		// (�vite d'avoir deux occurrences d'effets avec un nom identique malgr� un rendu final diff�rent : on ne conserve que le premier des deux)
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

		// Ajoute cet effet � la liste des effets disponibles
		if (!alreadyExists)
			avalaibleEffectsListBox->addItem(effectName.c_str());
	}

	// V�rifie que l'on peut s�lectionner un �lement
	const u32 listItemCount = avalaibleEffectsListBox->getItemCount();
	if (listItemCount > 0)
	{
		if (lastSelectedEffect.size() > 0)
		{
			// Cherche le nom de l'effet dans la liste et le s�lectionne (s'il n'a pas �t� trouv�, on s�lectionnera le premier �lement de la liste)
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
			// Sinon on s�lectionne le premier �lement
			avalaibleEffectsListBox->setSelected(0);
		}
	}

	// Indique le nouvel effet disponible actuellement s�lectionn�
	setCurrentSelectedAvalaibleEffect(avalaibleEffectsListBox->getSelected());
}
void CGUIPostProcessOptions::updateEnabledEffectsList(const GameConfiguration& gameCfg)
{
	if (!game->fileSystem)
		return;

	const u32 lastSelectedItem = enabledEffectsListBox->getSelected();

	// D�s�lectionne la valeur actuellement s�lectionn�e
	enabledEffectsListBox->setSelected(-1);

	// Efface les anciennes valeurs de la liste de la GUI et efface la liste interne
	enabledEffectsListBox->clear();
	enabledEffects.clear();

	// Parcours tous les effets charg�s par PostProcess pour pouvoir les ajouter un par un � la liste des effets disponibles
	const core::array<core::stringw>& gameConfigEffects = gameCfg.postProcessEffects;
	const u32 gameConfigEffectsSize = gameConfigEffects.size();
	for (u32 i = 0; i < gameConfigEffectsSize; ++i)
	{
		// Ajoute cet effet � la liste des effets activ�s dans la GUI et dans la liste interne
		const core::stringw& effectName = gameConfigEffects[i];
		enabledEffectsListBox->addItem(effectName.c_str());
		enabledEffects.push_back(effectName);
	}

	// S�lectionne le dernier �l�ment s�lectionn� (on ne le recherche pas ici par son nom puisque plusieurs effets de la liste peuvent avoir un nom identique)
	enabledEffectsListBox->setSelected(lastSelectedItem);

	// Si le dernier �l�ment s�lectionn� n'a pas �t� trouv�, on s�lectionn� alors le premier �l�ment de cette liste
	if (enabledEffectsListBox->getSelected() < 0)
		enabledEffectsListBox->setSelected(0);

	// Indique le nouvel effet activ� actuellement s�lectionn�
	setCurrentSelectedEnabledEffect(enabledEffectsListBox->getSelected());
}
void CGUIPostProcessOptions::updateFromGameConfig(const GameConfiguration& gameCfg)
{
	// Met tout d'abord � jour la liste des effets post-rendu disponibles
	updateAvalaibleEffectsList();

	// Met ensuite � jour la liste des effets activ�s suivant ces param�tres de configuration
	updateEnabledEffectsList(gameCfg);

	// Met � jour les options des effets post-rendu suivant les param�tres de configuration sp�cifi�s
	postProcessEnabledCheckBox->setChecked(gameCfg.usePostProcessEffects);
	cameraShakingEnabledCheckBox->setChecked(gameCfg.postProcessShakeCameraOnDestroying);

	// Met � jour les �l�ments activ�s de ce menu d'apr�s l'activation des effets post-rendu
	updateEnabledElements(postProcessEnabledCheckBox->isChecked());
}
void CGUIPostProcessOptions::updateEnabledElements(bool postProcessEnabled)
{
	// Active/D�sactive tous les �l�ments de ce menu suivant l'activation actuelle des effets post-rendu
	cameraShakingEnabledCheckBox->setEnabled(postProcessEnabled);
	avalaibleEffectsStaticText->setEnabled(postProcessEnabled);
	enabledEffectsStaticText->setEnabled(postProcessEnabled);
	avalaibleEffectsListBox->setEnabled(postProcessEnabled);
	enabledEffectsListBox->setEnabled(postProcessEnabled);

	if (postProcessEnabled)
	{		
		// Met � jour tous les �l�ments activ�s ou d�sactiv�s qui d�pendent des s�lections des listes de ce menu
		setCurrentSelectedAvalaibleEffect(avalaibleEffectsListBox->getSelected());
		setCurrentSelectedEnabledEffect(enabledEffectsListBox->getSelected());
	}
	else
	{
		// D�sactive tous les �l�ments de ce menu d�pendants des s�lections des listes de ce menu
		addButton->setEnabled(false);
		removeButton->setEnabled(false);
		putUpButton->setEnabled(false);
		putDownButton->setEnabled(false);
	}
}
void CGUIPostProcessOptions::setCurrentSelectedAvalaibleEffect(int avalaibleEffectIndex)
{
	// On suppose ici que l'effet sp�cifi� est d�j� s�lectionn� dans la liste des effets disponibles

	// Active ou d�sactive le bouton "Activer cet effet" suivant si un effet est r�ellement s�lectionn� ou non
	addButton->setEnabled(avalaibleEffectIndex >= 0 && (u32)avalaibleEffectIndex < avalaibleEffectsListBox->getItemCount());
}
void CGUIPostProcessOptions::setCurrentSelectedEnabledEffect(int enabledEffectIndex)
{
	// On suppose ici que l'effet sp�cifi� est d�j� s�lectionn� dans la liste des effets activ�s

	// D�termine si un effet est r�ellement s�lectionn�
	if (enabledEffectIndex >= 0 && (u32)enabledEffectIndex < enabledEffects.size())
	{
		// Active le bouton "D�sactiver cet effet"
		removeButton->setEnabled(true);

		// D�sactive le bouton "Monter" si cet effet est en haut de la liste
		putUpButton->setEnabled(enabledEffectIndex != 0);

		// D�sactive le bouton "Descendre" si cet effet est en bas de la liste
		putDownButton->setEnabled((u32)enabledEffectIndex != enabledEffects.size() - 1);
	}
	else
	{
		// D�sactive le bouton "D�sactiver cet effet"
		removeButton->setEnabled(false);

		// D�sactive le bouton "Monter"
		putUpButton->setEnabled(false);

		// D�sactive le bouton "Descendre"
		putDownButton->setEnabled(false);
	}
}
void CGUIPostProcessOptions::addEffect(int avalaibleEffectIndex)
{
	if (avalaibleEffectIndex < 0 || (u32)avalaibleEffectIndex >= avalaibleEffectsListBox->getItemCount())
		return;

	// Obtient le nom de l'effet s�lectionn� dans la liste des effets disponibles
	const core::stringw effectName = avalaibleEffectsListBox->getListItem(avalaibleEffectIndex);

	// Obtient l'index de l'effet actuellement s�lectionn� dans la liste des effets activ�s
	const int selected = enabledEffectsListBox->getSelected();

	// V�rifie si un effet activ� est bien s�lectionn� et que ce n'est pas le dernier de la liste des effets activ�s :
	if (selected >= 0 && (u32)selected < enabledEffectsListBox->getItemCount() - 1)
	{
		// Un effet activ� est bien s�lectionn� :

		// Ajoute cet effet dans la liste des effets activ�s
		core::list<core::stringw>::Iterator it = enabledEffects.begin();
		if (selected > 0)
			it += selected;
		enabledEffects.insert_after(it, effectName);

		// Ajoute un �l�ment repr�sentant cet effet dans la liste des effets activ�s de la GUI et le s�lectionn�
		enabledEffectsListBox->setSelected(enabledEffectsListBox->insertItem(selected + 1, effectName.c_str(), -1));
	}
	else
	{
		// Aucun effet activ� n'est s�lectionn�, ou l'effet s�lectionn� est le dernier de la liste des effets activ�s :

		// Ajoute cet effet � la fin de la liste des effets activ�s
		enabledEffects.push_back(effectName);

		// Ajoute un �l�ment repr�sentant cet effet � la fin de la liste des effets activ�s de la GUI et le s�lectionn�
		enabledEffectsListBox->setSelected(enabledEffectsListBox->addItem(effectName.c_str()));
	}

	// Indique qu'un nouvel effet activ� a �t� s�lectionn�
	setCurrentSelectedEnabledEffect(enabledEffectsListBox->getSelected());
}
void CGUIPostProcessOptions::removeEffect(int enabledEffectIndex)
{
	if (enabledEffectIndex < 0 || (u32)enabledEffectIndex >= enabledEffects.size())
		return;

	// Supprime cet effet de la liste des effets activ�s de la GUI
	enabledEffectsListBox->removeItem((u32)enabledEffectIndex);

	// S�lectionne l'�l�ment d'index inf�rieur dans la liste des effets activ�s de la GUI s'il existe, sinon celui avec cet index
	enabledEffectsListBox->setSelected((enabledEffectIndex == 0 && (u32)enabledEffectIndex < enabledEffects.size()) ? enabledEffectIndex : enabledEffectIndex - 1);

	// Indique qu'un nouvel effet activ� a �t� s�lectionn�
	setCurrentSelectedEnabledEffect(enabledEffectsListBox->getSelected());

	// Supprime cet effet de la liste des effets activ�s
	core::list<core::stringw>::Iterator it = enabledEffects.begin();
	if (enabledEffectIndex > 0)	it += enabledEffectIndex;
	enabledEffects.erase(it);
}
void CGUIPostProcessOptions::putUpEffect(int enabledEffectIndex)
{
	if (enabledEffectIndex < 1 || (u32)enabledEffectIndex >= enabledEffects.size())
		return;

	// D�termine le nouvel index de cet effet dans la liste des effets activ�s
	u32 newEnabledEffectIndex = (u32)enabledEffectIndex - 1;

	// Echange la position de cet effet et de l'effet qui est actuellement au-dessus de lui dans la liste de la GUI, puis s�lectionne cet item � son nouvel index
	enabledEffectsListBox->swapItems((u32)enabledEffectIndex, newEnabledEffectIndex);
	enabledEffectsListBox->setSelected(newEnabledEffectIndex);

	// Indique qu'un nouvel effet activ� a �t� s�lectionn�
	setCurrentSelectedEnabledEffect(enabledEffectsListBox->getSelected());

	// D�place cet effet dans la liste des effets activ�s : retiens son nom, le retire de la liste, puis l'ajoute � la liste � sa nouvelle position
	core::list<core::stringw>::Iterator it = enabledEffects.begin();
	it += enabledEffectIndex;
	const core::stringw effectName = (*it);
	enabledEffects.erase(it);	// Apr�s cette fonction, it pointe toujours sur NULL
	it = enabledEffects.begin();
	it += newEnabledEffectIndex;
	enabledEffects.insert_before(it, effectName);
}
void CGUIPostProcessOptions::putDownEffect(int enabledEffectIndex)
{
	if (enabledEffectIndex < 0 || (u32)enabledEffectIndex >= enabledEffects.size() - 1)
		return;

	// D�termine le nouvel index de cet effet dans la liste des effets activ�s
	u32 newEnabledEffectIndex = (u32)enabledEffectIndex + 1;

	// Echange la position de cet effet et de l'effet qui est actuellement en-dessous de lui dans la liste de la GUI, puis s�lectionne cet item � son nouvel index
	enabledEffectsListBox->swapItems((u32)enabledEffectIndex, newEnabledEffectIndex);
	enabledEffectsListBox->setSelected(newEnabledEffectIndex);

	// Indique qu'un nouvel effet activ� a �t� s�lectionn�
	setCurrentSelectedEnabledEffect(enabledEffectsListBox->getSelected());

	// D�place cet effet dans la liste des effets activ�s : retiens son nom, le retire de la liste, puis l'ajoute � la liste � sa nouvelle position
	core::list<core::stringw>::Iterator it = enabledEffects.begin();
	it += enabledEffectIndex;
	const core::stringw effectName = (*it);
	enabledEffects.erase(it);	// Apr�s cette fonction, it pointe toujours sur NULL
	it = enabledEffects.begin();
	it += enabledEffectIndex;
	enabledEffects.insert_after(it, effectName);
}
bool CGUIPostProcessOptions::needDepthPass() const
{
	// PostProcess doit avoir auparavant avoir �t� initialis� pour pouvoir d�terminer quels effets n�cessitent le rendu de la profondeur de la sc�ne
	if (!game->postProcessManager)
		return false;

	// Parcours tous les effets activ�s et d�termine si l'un d'entre eux n�cessite le rendu de la profondeur de la sc�ne
	const core::list<core::stringw>::ConstIterator END = enabledEffects.end();
	for (core::list<core::stringw>::ConstIterator it = enabledEffects.begin(); it != END; ++it)
		if (game->postProcessManager->needDepthPass(*it))
			return true;

	return false;
}
