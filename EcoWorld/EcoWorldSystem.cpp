#include "EcoWorldSystem.h"
#include "Ressources.h"
#include "Batiment.h"
#include "CLoadingScreen.h"

EcoWorldSystem::EcoWorldSystem() : systemRenderer(NULL)
{
	// Initialise la carte
	for (u32 i = 0; i < TAILLE_CARTE; ++i)
		for (u32 j = 0; j < TAILLE_CARTE; ++j)
			carte[i][j].reset();

	// Initialise le syst�me en le remettant � z�ro
	reset();
}
EcoWorldSystem::~EcoWorldSystem()
{
	// D�truit la carte
	for (u32 i = 0; i < TAILLE_CARTE; ++i)
	{
		for (u32 j = 0; j < TAILLE_CARTE; ++j)
		{
			if (carte[i][j].batiment)
				delete carte[i][j].batiment;
		}
	}
}
void EcoWorldSystem::reset(bool resetTerrain)
{
	// Indique le syst�me et les modifiers aux informations du monde, au Weather Manager et aux objectifs
	infos.setSystem(this);
	infos.setModifiers(&modifiers);
	weatherManager.setSystem(this);
	objectives.setSystem(this);

	// R�initialise les modifiers, les informations du monde, le Weather Manager et les objectifs
	// On ne r�initialise pas la difficult� du jeu : permet de conserver les difficult�s personnalis�es lorsque le joueur d�sire recommencer une partie qu'il a pr�c�demment charg�e
	//modifiers.setDifficulty(EcoWorldModifiers::ED_normal);
	infos.reset();
	weatherManager.reset();
	objectives.reset();

	// Remet le temps � z�ro
	time.setTotalTime(0.0f);
	lastUpdateTime = 0.0f;

	// Reinitialise les listes
	listeAllBats.clear();
	listeBatsProductionElectricite.clear();
#if 0
	listeBatsProductionEau.clear();
#endif
	listeBatsUsines.clear();
	listeBatsMaisons.clear();
	listeBatsOther.clear();
	listeBatsDechetsES.clear();

	// D�truit et r�initialise la carte
	for (u32 i = 0; i < TAILLE_CARTE; ++i)
	{
		for (u32 j = 0; j < TAILLE_CARTE; ++j)
		{
			if (carte[i][j].batiment)
			{
				delete carte[i][j].batiment;
				carte[i][j].batiment = NULL;
			}

			if (resetTerrain)
				carte[i][j].reset();
		}
	}

	// R�initialise les tableaux des ressources
	for (u32 i = 0; i < Ressources::RI_COUNT; ++i)
	{
		ressourcesProduites[i] = 0.0f;
		pourcentageRessourcesDisponibles[i] = 0.0f;
	}
}
void EcoWorldSystem::createNewGame(EcoWorldModifiers::E_DIFFICULTY difficulty, const core::list<ObjectiveData>& objectivesList)
{
	// Indique la difficult� du jeu
	modifiers.setDifficulty(difficulty);

	// Indique les objectifs de cette partie
	objectives.reset();
	objectives.addObjectives(objectivesList);

	// Indique le budget de d�part
	infos.budget = 5000000.0f										* modifiers.startBudgetFactor;		// 5 000 000 �

	// Donne quelques ressources de d�part :
	infos.ressources[Ressources::RI_eau] = 100000.0f				* modifiers.startRessourcesFactor;	// 100 000  L d'eau
	infos.ressources[Ressources::RI_bois] = 10000.0f				* modifiers.startRessourcesFactor;	//  10 000 kg de bois
	infos.ressources[Ressources::RI_verre] = 2000.0f				* modifiers.startRessourcesFactor;	//   2 000 kg de verre
	infos.ressources[Ressources::RI_ciment] = 40000.0f				* modifiers.startRessourcesFactor;	//  40 000 kg de ciment
	infos.ressources[Ressources::RI_tuiles] = 30000.0f				* modifiers.startRessourcesFactor;	//  30 000 kg de tuiles
#ifndef KID_VERSION	// En mode enfant : on d�sactive les ressources ne pouvant pas �tre produites
	infos.ressources[Ressources::RI_pierre] = 150000.0f				* modifiers.startRessourcesFactor;	// 150 000 kg de pierre
	infos.ressources[Ressources::RI_fer] = 3500.0f					* modifiers.startRessourcesFactor;	//   3 500 kg de m�tal (fer)
	infos.ressources[Ressources::RI_sable] = 100000.0f				* modifiers.startRessourcesFactor;	// 100 000 kg de sable
	infos.ressources[Ressources::RI_panneauxPhotovoltaiques] = 50	* modifiers.startRessourcesFactor;	// 50 panneaux solaires

	// Ressources de base consomm�es par les habitants : Suffisamment de ressources pour que 50 habitants puissent en consommer pendant 200 jours sans en manquer
	infos.ressources[Ressources::RI_viande] = 2000.0f				* modifiers.startRessourcesFactor;	//   2 000 kg de viande
	infos.ressources[Ressources::RI_pain] = 2000.0f					* modifiers.startRessourcesFactor;	//   2 000 kg de pain
	infos.ressources[Ressources::RI_lait] = 5000.0f					* modifiers.startRessourcesFactor;	//   5 000  L de lait
	infos.ressources[Ressources::RI_vetements] = 500.0f				* modifiers.startRessourcesFactor;	//     500 kg de v�tements
#else				// En mode enfant : on r�activer le papier comme ressource consomm�e par les habitants pour leur satisfaction
	infos.ressources[Ressources::RI_papier] = 10000.0f				* modifiers.startRessourcesFactor;	//   10 000 kg de papier
#endif
}
core::list<ObjectiveData> EcoWorldSystem::getNewGameDefaultObjectives()
{
	core::list<ObjectiveData> objectives;

	// Ajoute les objectifs par d�faut :

	// Le joueur a perdu lorsque ses dettes sont sup�rieures � 100 000 �
	objectives.push_back(ObjectiveData(Objectives::EOT_BUDGET, false, Objectives::ECO_MORE_EQUALS, -100000.0f));

	return objectives;
}
void EcoWorldSystem::save(io::IAttributes* out, io::IXMLWriter* writer) const
{
	if (!out || !writer)
		return;

	// Enregistre les infos sur le monde
	infos.save(out, writer);

	// Enregistre les infos sur les modifiers
	modifiers.save(out, writer);

	// Enregistre les infos sur le temps
	weatherManager.save(out, writer);

	// Enregistre les infos sur les objectifs
	objectives.save(out, writer);

	// Ajoute le temps total
	out->addFloat("TotalTime", time.getTotalTime());

	// Ajoute le temps lors de la derni�re mise � jour du monde (� conserver, on ne peut pas �tre s�r que le monde a bien �t� mis � jour avec le temps actuel)
	out->addFloat("LastUpdateTime", lastUpdateTime);

	// Ajoute le nombre de batiments
	out->addInt("BatimentCount", (int)(listeAllBats.size()));

	// Ecrit les informations dans le fichier
	out->write(writer, false, L"System");
	out->clear();

	// Enregistre les batiments
	int batimentCount = 0;
	const core::list<Batiment*>::ConstIterator END = listeAllBats.end();
	for (core::list<Batiment*>::ConstIterator it = listeAllBats.begin(); it != END; ++it)
	{
		Batiment* const batiment = (*it);

		// Ajoute les infos du batiment
		batiment->save(out);

		// Enregistre les infos du batiment sous le nom de bloc g�n�rique <Batiment> (on ne conserve plus le num�ro du b�timent dans son nom, car ce n'est pas n�cessaire pour son chargement)
		out->write(writer, false, L"Batiment");
		out->clear();

		// Demande au node 3D de ce b�timent d'ajouter ses informations
		// TODO : Si n�cessaire
		/*
		if (batiment->getSceneNode())
		{
			// Ajoute les infos du node 3D
			batiment->getSceneNode()->save(out);

			// Enregistre les infos du node sous le nom de bloc g�n�rique <SceneNode> (on ne conserve plus le num�ro du b�timent dans son nom, car ce n'est pas n�cessaire pour son chargement)
			out->write(writer, false, L"SceneNode");
			out->clear();
		}
		*/

		// Incr�mente le compteur de batiment
		batimentCount++;
	}
}
void EcoWorldSystem::load(io::IAttributes* in, io::IXMLReader* reader, CLoadingScreen* loadingScreen, float loadMin, float loadMax)
{
	if (!in || !reader)
		return;

	if (loadingScreen)
		if (loadingScreen->setPercentAndDraw(loadMin))	return;

	// Remet le syst�me � z�ro, mais conserve les informations sur le terrain
	reset(false);

	if (loadingScreen)
		if (loadingScreen->setPercentAndDraw((loadMax - loadMin) * 0.02f + loadMin))	return;

	// Lit les infos sur le monde
	infos.load(in, reader);

	if (loadingScreen)
		if (loadingScreen->setPercentAndDraw((loadMax - loadMin) * 0.04f + loadMin))	return;

	// Lit les infos sur les modifiers
	modifiers.load(in, reader);

	if (loadingScreen)
		if (loadingScreen->setPercentAndDraw((loadMax - loadMin) * 0.06f + loadMin))	return;

	// Lit les infos sur le temps
	weatherManager.load(in, reader);

	if (loadingScreen)
		if (loadingScreen->setPercentAndDraw((loadMax - loadMin) * 0.08f + loadMin))	return;

	// Lit les infos sur les objectifs
	objectives.load(in, reader);

	if (loadingScreen)
	{
		loadMin += (loadMax - loadMin) * 0.1f;	// Change le nouveau minimum de la barre de chargement pour simplifier les calculs lors du chargement des b�timents
		if (loadingScreen->setPercentAndDraw(loadMin))	return;
	}

	// Lit les informations � partir du fichier
	reader->resetPosition();
	if (in->read(reader, false, L"System"))
	{
		// Lit les informations du temps
		{
			float currentTotalTime = 0.0f;
			if (in->existsAttribute("TotalTime"))		currentTotalTime = in->getAttributeAsFloat("TotalTime");
			if (in->existsAttribute("LastUpdateTime"))	lastUpdateTime = in->getAttributeAsFloat("LastUpdateTime");	else	lastUpdateTime = currentTotalTime;

			// Recalcule les informations sur le temps actuel avec le dernier temps de la mise � jour
			time.setTotalTime(lastUpdateTime);
			weatherManager.update();
			if (lastUpdateTime != currentTotalTime)
				time.setTotalTime(currentTotalTime);
		}

		if (in->existsAttribute("BatimentCount"))
		{
			const int batimentCount = in->getAttributeAsInt("BatimentCount");
			const float batimentCountF_Inv = 1.0f / ((float)batimentCount);	// N�cessaire pour la barre de chargement

			in->clear();
			for (int i = 0; i < batimentCount; ++i)
			{
				if (loadingScreen)
					if (loadingScreen->setPercentAndDraw((loadMax - loadMin) * (float)i * batimentCountF_Inv + loadMin))	return;

				// Lit les infos du batiment (elles ne seront pas relues apr�s) :

				// R�initialisation de la position du lecteur XML dans le fichier d�sactiv�e car elle ne permet pas le chargement des b�timents autres que le premier :
				// ils ont maintenant tous le m�me nom de bloc <Batiment> ... </Batiment>.
				// Et si il pouvait �tre activ�, cela augmenterait sensiblement le temps de chargement d'une partie sauvegard�e, en supportant la r�solution du bug suivant :
				/*
				- BUG : Lors du chargement d'anciennes parties, lorsqu'un bloc est manquant (ex : si le bloc "<GUI> ... </GUI>" manque) ou mal plac� dans le fichier, les blocs suivants ne sont plus lus correctement
					=> Faire que le manque ou le mauvais placement d'un bloc ne soit pas fatal au reste du chargement !
				*/
				// En effet, le fait qu'un bloc de b�timent soit manquant ou mal plac� arrive rarement pour les sauvegardes (m�me lors du passage vers une version plus r�cente du jeu),
				// et les donn�es des b�timent �tant plac�es en fin de fichier, ce sont pour ces derniers que la r�solution de ce bug se r�v�le la plus lente
				// (car tout le fichier depuis le d�but doit �tre re-parcouru pour v�rifier que ce bloc ne se situe pas avant).
				//reader->resetPosition();
				if (in->read(reader, false, L"Batiment"))
				{
					// V�rifie que les deux valeurs importantes pour un b�timent existent
					if (!in->existsAttribute("Name") || !in->existsAttribute("Index"))
						continue;

					// Nouvelle m�thode d'enregistrement des b�timents : on n'enregistre plus leur ID mais leur nom, pour �viter une compl�te invalidation des sauvegardes en cas de changement des ID des b�timents :
					//const BatimentID batID = (BatimentID)in->getAttributeAsInt("ID");

					// D�termine l'ID du b�timent d'apr�s son nom :
					const core::stringw batName = in->getAttributeAsStringW("Name");
					BatimentID batID = BI_aucun;
					for (int j = 0; j < BI_COUNT; ++j)
					{
						const wchar_t* name = StaticBatimentInfos::getInfos((BatimentID)j).name;
						if (batName.equals_ignore_case(name))
						{
							batID = (BatimentID)j;
							break;
						}
					}
					// V�rifie que l'ID du b�timent a bien �t� trouv�e, sinon on ne peut cr�er ce b�timent : on passe au b�timent suivant
					if (batID == BI_aucun)
						continue;
					
					const core::vector2di index = in->getAttributeAsPosition2d("Index");
					const float rotation = (in->existsAttribute("Rotation") ? in->getAttributeAsFloat("Rotation") : 0.0f);

					// Cr�e le batiment et lui demande charger ses donn�es depuis le fichier
					carte[index.X][index.Y].batiment = new Batiment(batID, (*this), infos, modifiers, index, rotation);
					carte[index.X][index.Y].batiment->load(in);

					// Ajoute ce b�timent aux listes
					addBatimentToLists(carte[index.X][index.Y].batiment);

					// Ajoute le batiment au renderer et le node cr�� au b�timent
					if (systemRenderer)
						carte[index.X][index.Y].batiment->setSceneNode(systemRenderer->addBatiment(carte[index.X][index.Y].batiment));

					in->clear();

					// Demande au node 3D de ce b�timent de se charger depuis le fichier
					// TODO : Si n�cessaire
					/*
					if (carte[index.X][index.Y].batiment->getSceneNode())
					{
						// Lit les infos du node (elles ne seront pas relues apr�s) :

						// R�initialisation de la position du lecteur XML dans le fichier d�sactiv�e car elle ne permet pas le chargement des b�timents autres que le premier :
						// ils ont maintenant tous le m�me nom de bloc <SceneNode> ... </SceneNode>.
						// Et si il pouvait �tre activ�, cela augmenterait sensiblement le temps de chargement d'une partie sauvegard�e, en supportant la r�solution du bug suivant :
						/*
						- BUG : Lors du chargement d'anciennes parties, lorsqu'un bloc est manquant (ex : si le bloc "<GUI> ... </GUI>" manque) ou mal plac� dans le fichier, les blocs suivants ne sont plus lus correctement
							=> Faire que le manque ou le mauvais placement d'un bloc ne soit pas fatal au reste du chargement !
						* /
						// En effet, le fait qu'un bloc de scene node soit manquant ou mal plac� arrive rarement pour les sauvegardes (m�me lors du passage vers une version plus r�cente du jeu),
						// et les donn�es des scene nodes �tant plac�es en fin de fichier, ce sont pour ces derniers que la r�solution de ce bug se r�v�le la plus lente
						// (car tout le fichier depuis le d�but doit �tre re-parcouru pour v�rifier que ce bloc ne se situe pas avant).
						//reader->resetPosition();
						if (in->read(reader, false, L"SceneNode"))
						{
							carte[index.X][index.Y].batiment->getSceneNode()->load(in);
							in->clear();
						}
					}
					*/
				}
			}
		}

		in->clear();
	}

	// Recalcule les infos sur le temps d'apr�s le temps actuel du syst�me de jeu (restaur� apr�s le chargement du WeatherManager)
	weatherManager.update();

	// Met � jour les informations du monde sur la population et l'�nergie
	infos.updateDonnees();

	// Met � jour les objectifs du jeu
	objectives.update();

	// Met � jour le syst�me de jeu complet, au cas o� lastUpdateTime != totalTime
	update(0.0f);

	if (loadingScreen)
		loadingScreen->setPercentAndDraw(loadMax);
}
int EcoWorldSystem::canCreateBatiment(BatimentID batimentID, const core::vector2di& index, float rotation, bool outRessources[]) const
{
	if (batimentID == BI_aucun || batimentID >= BI_COUNT)
		return EBCE_BATIMENT_ID;

	int error = EBCE_AUCUNE;

	// Obtient les informations sur le type du batiment � construire
	const BatimentInfos batimentInfos(batimentID);
	const StaticBatimentInfos& staticBatimentInfos = StaticBatimentInfos::getInfos(batimentID);

	// Obtient la taille du b�timent : utilise les obbox, pour prendre en compte la rotation
	const obbox2df batimentBox(core::rectf((float)index.X - staticBatimentInfos.halfTaille.Width, (float)index.Y - staticBatimentInfos.halfTaille.Height,
		(float)index.X + staticBatimentInfos.halfTaille.Width, (float)index.Y + staticBatimentInfos.halfTaille.Height),
		rotation);

	// Obtient la liste des cases de terrain qu'occupe ce b�timent
	const core::list<core::vector2di> listCasesSurfaceBat = batimentBox.getCasesList();
	const core::list<core::vector2di>::ConstIterator END = listCasesSurfaceBat.end();

	// V�rifie si la position du batiment est en dehors du terrain o� on peut les placer, en prenant en compte sa rotation et ses dimensions
	// (Note : Ce syst�me fonctionne parfaitement, quelle que soit la parit� de TAILLE_CARTE)
	if (!batimentBox.isInsideRect(core::rectf(0.0f, 0.0f, (float)TAILLE_CARTE - 1.0f, (float)TAILLE_CARTE - 1.0f)))
		error |= EBCE_TERRAIN_OUT;
	else
	{
		// Si le b�timent n�cessite de l'eau profonde, on v�rifie si le terrain en contient bien
		if (StaticBatimentInfos::needDeepWater(batimentID))
		{
			if (!(carte[index.X][index.Y].deepWater))
				error |= EBCE_NEED_DEEP_WATER;
			else
			{
				// Teste si le terrain est constructible sur toute la surface du b�timent
				for (core::list<core::vector2di>::ConstIterator it = listCasesSurfaceBat.begin(); it != END; ++it)
				{
					const core::vector2di& pos = (*it);
					if (!(carte[pos.X][pos.Y].deepWater))
					{
						error |= EBCE_NEED_DEEP_WATER;
						break;
					}
				}
			}
		}
		else	// V�rifie que le terrain est constructible si le b�timent ne n�cessite pas d'eau profonde
		{
			if (!(carte[index.X][index.Y].constructible))
				error |= EBCE_TERRAIN_NON_CONSTRUCTIBLE;
			else
			{
				// Teste si le terrain est constructible sur toute la surface du b�timent
				for (core::list<core::vector2di>::ConstIterator it = listCasesSurfaceBat.begin(); it != END; ++it)
				{
					const core::vector2di& pos = (*it);
					if (!(carte[pos.X][pos.Y].constructible))
					{
						error |= EBCE_TERRAIN_NON_CONSTRUCTIBLE;
						break;
					}
				}
			}
		}

		// Si la position est d�j� occup�e par un autre batiment (attention : on ne peut plus construire par-dessus une route !)
		if (carte[index.X][index.Y].batiment)
			error |= EBCE_PLACE;
		else
		{
			// Obtient la taille maximale des b�timents
			const u32 TAILLE_MAX_BATS = StaticBatimentInfos::getTailleMaxBatiments();

			// Teste s'il y a collision avec les autres batiments de la carte (attention : on ne peut plus construire par-dessus une route !)
			for (core::list<core::vector2di>::ConstIterator it = listCasesSurfaceBat.begin(); it != END; ++it)
			{
				const core::vector2di& pos = (*it);
				if (carte[pos.X][pos.Y].batiment)
				{
					// Calcule si les deux rectangles entrent en collision
					if (carte[pos.X][pos.Y].batiment->getBoundingBox().intersectsWithBox(batimentBox))
					{
						error |= EBCE_PLACE;
						break;
					}
				}
			}
		}
	}

	// V�rifie qu'on a assez d'argent
	if (infos.budget < 0 || infos.budget + staticBatimentInfos.prixC * modifiers.prixFactor < 0)
		error |= EBCE_BUDGET;

	// V�rifie qu'on a assez d'�nergie
	if (infos.energie < 0 || infos.energie + batimentInfos.energie < 0)
		error |= EBCE_ENERGIE;

	// V�rifie qu'on a assez de ressources
	bool notEnoughRessources = false;
	for (u32 i = 0; i < staticBatimentInfos.ressourcesC.size(); ++i)
	{
		if (staticBatimentInfos.ressourcesC[i].production < 0 &&
			infos.ressources[staticBatimentInfos.ressourcesC[i].ID] + staticBatimentInfos.ressourcesC[i].production < 0)
		{
			if (outRessources)	// On indique que cette ressource est manquante
				outRessources[staticBatimentInfos.ressourcesC[i].ID] = true;

			notEnoughRessources = true;
		}
		else if (outRessources)	// On indique qu'on a suffisamment de cette ressource
			outRessources[staticBatimentInfos.ressourcesC[i].ID] = false;
	}
	if (notEnoughRessources)
		error |= EBCE_RESSOURCE;

	// V�rifie que la population est suffisante pour d�bloquer ce b�timent
	if (infos.population < staticBatimentInfos.unlockPopulationNeeded)
		error |= EBCE_NOT_ENOUGH_POPULATION;

	return error;
}
int EcoWorldSystem::canCreateMultiBatiments(BatimentID batimentID, int batimentCount, bool outRessources[]) const
{
	if (batimentID == BI_aucun || batimentID >= BI_COUNT)
		return EBMCE_BATIMENT_ID;

	int error = EBMCE_AUCUNE;

	// Obtient les informations sur le type du batiment � construire
	const BatimentInfos batimentInfos(batimentID);
	const StaticBatimentInfos& staticBatimentInfos = StaticBatimentInfos::getInfos(batimentID);

	// V�rifie qu'on a assez d'argent
	if (infos.budget < 0 || infos.budget + staticBatimentInfos.prixC * modifiers.prixFactor * batimentCount < 0)
		error |= EBMCE_BUDGET;

	// V�rifie qu'on a assez d'�nergie
	if (infos.energie < 0 || infos.energie + batimentInfos.energie * batimentCount < 0)
		error |= EBMCE_ENERGIE;

	// V�rifie qu'on a assez de ressources
	bool notEnoughRessources = false;
	for (u32 i = 0; i < staticBatimentInfos.ressourcesC.size(); ++i)
	{
		if (staticBatimentInfos.ressourcesC[i].production < 0 &&
			infos.ressources[staticBatimentInfos.ressourcesC[i].ID] + staticBatimentInfos.ressourcesC[i].production * batimentCount < 0)
		{
			if (outRessources)	// On indique que cette ressource est manquante
				outRessources[staticBatimentInfos.ressourcesC[i].ID] = true;

			notEnoughRessources = true;
		}
		else if (outRessources)	// On indique qu'on a suffisamment de cette ressource
			outRessources[staticBatimentInfos.ressourcesC[i].ID] = false;
	}
	if (notEnoughRessources)
		error |= EBMCE_RESSOURCE;

	return error;
}
int EcoWorldSystem::addBatiment(BatimentID batimentID, const core::vector2di& index, float rotation, bool outRessources[], const core::vector3df* rendererPosition, const float* rendererDenivele
#ifdef USE_RAKNET
								, bool forceAdding, u32 forceDureeVie
#endif
								)
{
#ifdef USE_RAKNET
	if (!forceAdding)
	{
#endif
		// V�rifie si on peut placer ce b�timent
#ifndef _DEBUG
		const	// Le const d�sactiv� permet de d�sactiver l'erreur EBCE_NOT_ENOUGH_POPULATION en mode DEBUG
#endif
			int error = canCreateBatiment(batimentID, index, rotation, outRessources);

#ifdef _DEBUG
		// /!\ Attention /!\ 
		// En mode DEBUG uniquement : On d�sactive l'erreur EBCE_NOT_ENOUGH_POPULATION pour permettre le fonctionnement du code de d�bogage pour d�bloquer tous les b�timents
		error &= ~EBCE_NOT_ENOUGH_POPULATION;	// Passe le bit repr�sent� par EBCE_NOT_ENOUGH_POPULATION � 0 (~ : op�rateur NOT bit � bit)
#endif

		if (error != EBCE_AUCUNE && error != EBCE_ENERGIE)	// V�rifie que l'erreur n'est pas d�e � un manque d'�nergie, qui n'est plus une erreur fatale
		{
#ifdef _DEBUG
			if ((error & EBCE_BATIMENT_ID) != 0)
				LOG_DEBUG("EcoWorldSystem::addBatiment(" << batimentID << ", ...) : L'ID du batiment est invalide : batimentID = " << batimentID, ELL_WARNING);
#endif
			return error;
		}
#ifdef USE_RAKNET
	}
	else
	{
		// V�rifie que l'index fourni est bien dans les limites du terrain
		if (index.X < 0 || index.Y < 0 || index.X >= TAILLE_CARTE || index.Y >= TAILLE_CARTE)
		{
			// On ne peut vraiment pas placer ce b�timent, m�me s'il est forc� (en-dehors des zones m�moires de la carte) : on retourne une erreur et on avertit l'utilisateur
			LOG_DEBUG("EcoWorldSystem::addBatiment(" << batimentID << ", ..., " << forceAdding <<") : Le placement du batiment est force, mais n'a pas pu etre effectue car le batiment est en-dehors des limites du terrain :" << endl
				<< "    index.X = " << index.X << "   ;   index.Y = " << index.Y, ELL_WARNING);
			LOG_RELEASE("AVERTISSEMENT : Ajout forc� du b�timent au syst�me de jeu : Position en-dehors des limites du terrain !", ELL_WARNING);
			return EBCE_TERRAIN_OUT;
		}
		else if (batimentID == BI_aucun || batimentID >= BI_COUNT)
		{
			// Le b�timent � placer a un ID invalide (BI_aucun) : on retourne une erreur et on avertit l'utilisateur
			LOG_DEBUG("EcoWorldSystem::addBatiment(" << batimentID << ", ..., " << forceAdding <<") : Le placement du batiment est force, mais n'a pas pu etre effectue car le batiment a placer a un ID invalide (BI_aucun) :" << endl
				<< "    batimentID = " << batimentID, ELL_WARNING);
			LOG_RELEASE("AVERTISSEMENT : Ajout forc� du b�timent au syst�me de jeu : ID invalide !", ELL_WARNING);
			return EBCE_BATIMENT_ID;
		}
		else if (carte[index.X][index.Y].batiment)
		{
			// La position est d�j� occup�e par un autre b�timent : on retourne une erreur et on avertit l'utilisateur
			LOG_DEBUG("EcoWorldSystem::addBatiment(" << batimentID << ", ..., " << forceAdding <<") : Le placement du batiment est force, mais il y a deja un autre batiment a cette position :" << endl
				<< "    index.X = " << index.X << "   ;   index.Y = " << index.Y << endl
				<< "    carte[index.X][index.Y].batiment = " << carte[index.X][index.Y].batiment, ELL_WARNING);
			LOG_RELEASE("AVERTISSEMENT : Ajout forc� du b�timent au syst�me de jeu : Un autre b�timent occupe d�j� cette position !", ELL_WARNING);
			return EBCE_PLACE;
		}
	}
#endif

	// Il n'y a pas d'erreur pour placer le batiment : on ajoute le batiment
	carte[index.X][index.Y].batiment = new Batiment(batimentID, (*this), infos, modifiers, index, rotation);

	// Ajoute ce b�timent aux listes
	addBatimentToLists(carte[index.X][index.Y].batiment);

	// Ajoute le batiment au renderer et le node cr�� au b�timent
	if (systemRenderer)
		carte[index.X][index.Y].batiment->setSceneNode(systemRenderer->addBatiment(carte[index.X][index.Y].batiment, rendererPosition, rendererDenivele));

	// Met � jour les informations du monde avec les co�ts de construction de ce b�timent
	carte[index.X][index.Y].batiment->onConstruct();

#ifdef USE_RAKNET
	// Force la dur�e de vie de ce b�timent si n�cessaire
	if (forceAdding)
		carte[index.X][index.Y].batiment->getInfos().dureeVie = forceDureeVie;
#endif

	return EBCE_AUCUNE;
}
void EcoWorldSystem::addBatimentToLists(Batiment* batiment)
{
	bool hasAddedBat = false;
	core::list<Batiment*>::Iterator it;
	const int batimentID = batiment->getID();

	// Ajoute tout d'abord ce b�timent � la liste de tous les b�timents (sans aucun tri pr�alable)
	listeAllBats.push_back(batiment);

	// Ajoute ensuite ce b�timent � sa liste
	const core::list<Batiment*>::Iterator END = listeBatsProductionElectricite.end();
	switch (batimentID)
	{
		// Production d'�lectricit�
	case BI_centrale_charbon:
	case BI_panneau_solaire:
	case BI_eolienne:
	case BI_hydrolienne:
		{
			// Ins�re ce b�timent dans la liste tel que son ID soit inf�rieur � l'ID des b�timents qui le suivent
			for (it = listeBatsProductionElectricite.begin(); it != END; ++it)
				if ((*it)->getID() > batimentID)	// D�s que l'ID d'un b�timent de la liste est sup�rieur � l'ID du b�timent � ins�rer, on ins�re ce b�timent juste avant cette position de la liste
				{
					listeBatsProductionElectricite.insert_before(it, batiment);
					hasAddedBat = true;
					break;
				}

			// Si aucun b�timent n'a d'ID sup�rieur � celui de ce b�timent, on l'ajoute � la fin de la liste
			if (!hasAddedBat)
				listeBatsProductionElectricite.push_back(batiment);
		}
		break;

		// Production d'eau
	case BI_pompe_extraction_eau:
		{
			// Ins�re ce b�timent dans la liste tel que son ID soit inf�rieur � l'ID des b�timents qui le suivent
			for (it = listeBatsProductionEau.begin(); it != END; ++it)
				if ((*it)->getID() > batimentID)	// D�s que l'ID d'un b�timent de la liste est sup�rieur � l'ID du b�timent � ins�rer, on ins�re ce b�timent juste avant cette position de la liste
				{
					listeBatsProductionEau.insert_before(it, batiment);
					hasAddedBat = true;
					break;
				}

			// Si aucun b�timent n'a d'ID sup�rieur � celui de ce b�timent, on l'ajoute � la fin de la liste
			if (!hasAddedBat)
				listeBatsProductionEau.push_back(batiment);
		}
		break;

		// Usines
	case BI_usine_verre_petite:
	case BI_usine_verre_grande:
	case BI_usine_ciment_petite:
	case BI_usine_ciment_grande:
	case BI_usine_tuiles_petite:
	case BI_usine_tuiles_grande:
#ifdef KID_VERSION	// En mode enfant : on r�activer le papier comme ressource consomm�e par les habitants pour leur satisfaction
	case BI_usine_papier_petite:
	case BI_usine_papier_grande:
#else				// On d�sactive l'usine � tout faire en mode enfant qui est devenue inutile
	case BI_usine_tout:
#endif
		{
			// Ins�re ce b�timent dans la liste tel que son ID soit inf�rieur � l'ID des b�timents qui le suivent
			for (it = listeBatsUsines.begin(); it != END; ++it)
				if ((*it)->getID() > batimentID)	// D�s que l'ID d'un b�timent de la liste est sup�rieur � l'ID du b�timent � ins�rer, on ins�re ce b�timent juste avant cette position de la liste
				{
					listeBatsUsines.insert_before(it, batiment);
					hasAddedBat = true;
					break;
				}

			// Si aucun b�timent n'a d'ID sup�rieur � celui de ce b�timent, on l'ajoute � la fin de la liste
			if (!hasAddedBat)
				listeBatsUsines.push_back(batiment);
		}
		break;

		// Maisons
#ifndef KID_VERSION	// En mode enfant : on d�sactive la maison de base qui est inutile
	case BI_maison:
#endif
	case BI_maison_individuelle:
	case BI_maison_basse_consommation:
	case BI_maison_avec_panneaux_solaires:
	case BI_grande_maison_individuelle:
	case BI_chalet:
	case BI_immeuble_individuel:
	case BI_immeuble_basse_consommation:
	case BI_immeuble_avec_panneaux_solaires:
	case BI_grand_immeuble_individuel:
	case BI_building_individuel:
	case BI_building_basse_consommation:
	case BI_building_avec_panneaux_solaires:
	case BI_grand_building_individuel:
		{
			// Ins�re ce b�timent dans la liste tel que son ID soit inf�rieur � l'ID des b�timents qui le suivent
			for (it = listeBatsMaisons.begin(); it != END; ++it)
				if ((*it)->getID() > batimentID)	// D�s que l'ID d'un b�timent de la liste est sup�rieur � l'ID du b�timent � ins�rer, on ins�re ce b�timent juste avant cette position de la liste
				{
					listeBatsMaisons.insert_before(it, batiment);
					hasAddedBat = true;
					break;
				}

			// Si aucun b�timent n'a d'ID sup�rieur � celui de ce b�timent, on l'ajoute � la fin de la liste
			if (!hasAddedBat)
				listeBatsMaisons.push_back(batiment);
		}
		break;

#ifndef KID_VERSION	// En mode enfant : on d�sactive les routes qui sont inutiles
		// Routes et autre
	case BI_route:
		{
			// Ins�re ce b�timent dans la liste tel que son ID soit inf�rieur � l'ID des b�timents qui le suivent
			for (it = listeBatsOther.begin(); it != END; ++it)
				if ((*it)->getID() > batimentID)	// D�s que l'ID d'un b�timent de la liste est sup�rieur � l'ID du b�timent � ins�rer, on ins�re ce b�timent juste avant cette position de la liste
				{
					listeBatsOther.insert_before(it, batiment);
					hasAddedBat = true;
					break;
				}

			// Si aucun b�timent n'a d'ID sup�rieur � celui de ce b�timent, on l'ajoute � la fin de la liste
			if (!hasAddedBat)
				listeBatsOther.push_back(batiment);
		}
		break;
#endif

		// Gestion de l'effet de serre et des d�chets
	case BI_arbre_aspen:
	case BI_arbre_oak:
	case BI_arbre_pine:
	case BI_arbre_willow:
	case BI_decharge:
	case BI_usine_incineration_dechets:
		{
			// Ins�re ce b�timent dans la liste tel que son ID soit inf�rieur � l'ID des b�timents qui le suivent
			for (it = listeBatsDechetsES.begin(); it != END; ++it)
				if ((*it)->getID() > batimentID)	// D�s que l'ID d'un b�timent de la liste est sup�rieur � l'ID du b�timent � ins�rer, on ins�re ce b�timent juste avant cette position de la liste
				{
					listeBatsDechetsES.insert_before(it, batiment);
					hasAddedBat = true;
					break;
				}

			// Si aucun b�timent n'a d'ID sup�rieur � celui de ce b�timent, on l'ajoute � la fin de la liste
			if (!hasAddedBat)
				listeBatsDechetsES.push_back(batiment);
		}
		break;

	default:
		LOG_DEBUG("EcoWorldSystem::addBatimentToLists(" << batiment << ") : ID non reconnu lors de l'ajout dans les listes des batiments : batimentID = " << batimentID, ELL_WARNING);
		{
			// Ins�re ce b�timent dans la liste tel que son ID soit inf�rieur � l'ID des b�timents qui le suivent
			for (it = listeBatsOther.begin(); it != END; ++it)
				if ((*it)->getID() > batimentID)	// D�s que l'ID d'un b�timent de la liste est sup�rieur � l'ID du b�timent � ins�rer, on ins�re ce b�timent juste avant cette position de la liste
				{
					listeBatsOther.insert_before(it, batiment);
					hasAddedBat = true;
					break;
				}

			// Si aucun b�timent n'a d'ID sup�rieur � celui de ce b�timent, on l'ajoute � la fin de la liste
			if (!hasAddedBat)
				listeBatsOther.push_back(batiment);
		}
		break;
	}
}
int EcoWorldSystem::canDestroyBatiment(const core::vector2di& index, bool outRessources[]) const
{
	// V�rifie si la position du batiment est en dehors du terrain o� on peut les placer
	// (Note : Ce syst�me fonctionne parfaitement, quelle que soit la parit� de TAILLE_CARTE)
	if (index.X < 0 || index.Y < 0 || index.X >= TAILLE_CARTE || index.Y >= TAILLE_CARTE)
		return EBDE_INVALID_INDEX;

	int error = EBDE_AUCUNE;

	// V�rifie que le b�timent n'est pas d�j� en train de se d�truire
	if (carte[index.X][index.Y].batiment->isDestroying())
		error |= EBDE_ALREADY_DESTRUCTING;

	const StaticBatimentInfos& staticBatimentInfos = carte[index.X][index.Y].batiment->getStaticInfos();

	// V�rifie qu'on a assez d'argent
	if (infos.budget < 0 || infos.budget + staticBatimentInfos.prixD * modifiers.prixFactor < 0)
		error |= EBDE_BUDGET;

	// V�rifie qu'on a assez de ressources
	bool notEnoughRessources = false;
	for (u32 i = 0; i < staticBatimentInfos.ressourcesD.size(); ++i)
	{
		if (staticBatimentInfos.ressourcesD[i].production < 0 &&
			infos.ressources[staticBatimentInfos.ressourcesD[i].ID] + staticBatimentInfos.ressourcesD[i].production < 0)
		{
			if (outRessources)	// On indique que cette ressource est manquante
				outRessources[staticBatimentInfos.ressourcesD[i].ID] = true;

			notEnoughRessources = true;
		}
		else if (outRessources)	// On indique qu'on a suffisamment de cette ressource
			outRessources[staticBatimentInfos.ressourcesD[i].ID] = false;
	}
	if (notEnoughRessources)
		error |= EBDE_RESSOURCE;

	return error;
}
int EcoWorldSystem::destroyBatiment(const core::vector2di& index, bool outRessources[]
#ifdef USE_RAKNET
									, bool forceDestroying
#endif
									)
{
#ifdef	USE_RAKNET
	if (!forceDestroying)
	{
#endif
		// V�rifie si on peut d�truire ce b�timent
		const int error = canDestroyBatiment(index, outRessources);
		if (error != EBDE_AUCUNE)
			return error;
#ifdef USE_RAKNET
	}
	else
	{
		// V�rifie que l'index fourni est bien dans les limites du terrain
		if (index.X < 0 || index.Y < 0 || index.X >= TAILLE_CARTE || index.Y >= TAILLE_CARTE)
		{
			// On ne peut vraiment pas placer ce b�timent, m�me s'il est forc� (en-dehors des zones m�moires de la carte) : on retourne une erreur et on avertit l'utilisateur
			LOG_DEBUG("EcoWorldSystem::destroyBatiment(..., " << forceDestroying <<") : La destruction du batiment est forcee, mais n'a pas pu etre effectue car la position specifiee se situe en-dehors des limites du terrain :" << endl
				<< "    index.X = " << index.X << "   ;   index.Y = " << index.Y, ELL_WARNING);
			LOG_RELEASE("AVERTISSEMENT : Destruction forc�e du b�timent du syst�me de jeu : Position en-dehors des limites du terrain !", ELL_WARNING);
			return EBCE_TERRAIN_OUT;
		}
		else if (!carte[index.X][index.Y].batiment)
		{
			// V�rifie qu'il existe bien un batiment � cet emplacement, sinon on ne peut le d�truire, et on retourne une erreur
			LOG_DEBUG("EcoWorldSystem::destroyBatiment(..., " << forceDestroying <<") : La destruction du batiment est forcee, mais aucun batiment n'occupe cette position :" << endl
				<< "    index.X = " << index.X << "   ;   index.Y = " << index.Y << endl
				<< "    carte[index.X][index.Y].batiment = " << carte[index.X][index.Y].batiment, ELL_WARNING);
			LOG_RELEASE("AVERTISSEMENT : Destruction forc�e du b�timent du syst�me de jeu : Aucun b�timent n'occupe cette position !", ELL_WARNING);
			return EBDE_INVALID_INDEX;
		}
	}
#endif

	Batiment* const batiment = carte[index.X][index.Y].batiment;

	// V�rifie qu'il existe bien un batiment � cet emplacement
	if (!batiment)
	{
		LOG_DEBUG("EcoWorldSystem::deleteBatiment(index, outRessources) : Il n'y a aucun batiment a cette position alors que la fonction canCreateBatiment n'a retourne aucune erreur : batiment = " << batiment, ELL_WARNING);
		return EBDE_INVALID_INDEX;
	}

	// Lance l'ordre de destruction du b�timent
	batiment->destroy(time.getTotalJours());

	return EBDE_AUCUNE;
}
void EcoWorldSystem::addBatimentToDeleteList(Batiment* batiment)
{
	if (!batiment)
		return;

	// Ajoute le b�timent � la liste de suppression
	deleteList.push_back(batiment);
}
void EcoWorldSystem::deleteBatiment(Batiment* batiment)
{
	if (!batiment)
		return;

	// Supprime le batiment du renderer
	if (systemRenderer)
		systemRenderer->deleteBatiment(batiment, (batiment->getDestroyingDay() != 0));	// La destruction n'a pas �t� demand�e par l'utilisateur si le jour on a demand� de d�truire ce b�timent n'existe pas !

	// Supprime le b�timent des listes
	deleteBatimentInLists(batiment);

	// Supprime d�finitivement le batiment
	carte[batiment->getIndex().X][batiment->getIndex().Y].batiment = NULL;
	delete batiment;
}
void EcoWorldSystem::deleteBatimentInLists(Batiment* batiment)
{
	// Tous les b�timents
	core::list<Batiment*>::Iterator it;
	const core::list<Batiment*>::Iterator END = listeAllBats.end();
	for (it = listeAllBats.begin(); it != END; ++it)
	{
		if ((*it) == batiment)
		{
			it = listeAllBats.erase(it);
			break;
		}
	}

	bool foundBat = false;

	// Production d'�lectricit�
	for (it = listeBatsProductionElectricite.begin(); it != END; ++it)
	{
		if ((*it) == batiment)
		{
			it = listeBatsProductionElectricite.erase(it);
			foundBat = true;
			break;
		}
	}

	if (foundBat)
		return;

	// Production d'eau
	for (it = listeBatsProductionEau.begin(); it != END; ++it)
	{
		if ((*it) == batiment)
		{
			it = listeBatsProductionEau.erase(it);
			foundBat = true;
			break;
		}
	}

	if (foundBat)
		return;

	// Usines
	for (it = listeBatsUsines.begin(); it != END; ++it)
	{
		if ((*it) == batiment)
		{
			it = listeBatsUsines.erase(it);
			foundBat = true;
			break;
		}
	}

	if (foundBat)
		return;

	// Maisons
	for (it = listeBatsMaisons.begin(); it != END; ++it)
	{
		if ((*it) == batiment)
		{
			it = listeBatsMaisons.erase(it);
			foundBat = true;
			break;
		}
	}

	if (foundBat)
		return;

	// Route et autre
	for (it = listeBatsOther.begin(); it != END; ++it)
	{
		if ((*it) == batiment)
		{
			it = listeBatsOther.erase(it);
			foundBat = true;
			break;
		}
	}

	if (foundBat)
		return;

	// Gestion de l'effet de serre et des d�chets
	for (it = listeBatsDechetsES.begin(); it != END; ++it)
	{
		if ((*it) == batiment)
		{
			it = listeBatsDechetsES.erase(it);
			foundBat = true;
			break;
		}
	}

#ifdef _DEBUG
	if (!foundBat)
		LOG_DEBUG("EcoWorldSystem::deleteBatimentInLists(" << batiment << ") : Le batiment cherche n'a pas pu etre trouve dans les listes : batiment = " << batiment, ELL_WARNING);
#endif
}
void EcoWorldSystem::updateMainMenu(float elapsedTime)
{
	// Ajoute le temps �coul� au temps du jeu
	time += elapsedTime;

	// Met � jour le Weather Manager sans mettre � jour les informations du syst�me (optimisation pour le menu principal)
	weatherManager.update();
}
void EcoWorldSystem::update(float elapsedTime, bool* outNormalBatimentDestroy, bool* outOldBatimentDestroy)
{
	// Met � jour le syst�me jour par jour : �vite certaines erreurs de calcul dans le syst�me lorsque le temps depuis la derni�re frame inclus plus d'un jour syst�me
	// - Exemple : supposons que 2 jours se soient �coul�s depuis la derni�re mise � jour du syst�me, et qu'un b�timent soit � 1 jour de sa fin de vie, alors � sa mise � jour il sera supprim� sans qu'il n'ait v�cu son dernier jour
	//		Cela provoquerait aussi des bugs dans le calcul de la consommation d'eau (lorsque plusieurs jours s'�coulent au chevauchement d'un mois), ainsi que de nombreux autres probl�mes (taxes sur l'ES et les d�chets par exemple...)

	// R�initialise les valeurs de outNormalBatimentDestroy et de outOldBatimentDestroy : aucun b�timent n'a encore �t� d�truit
	if (outNormalBatimentDestroy)
		(*outNormalBatimentDestroy) = false;
	if (outOldBatimentDestroy)
		(*outOldBatimentDestroy) = false;

	// Prend en compte dans le temps �coul� un ajout annexe au temps syst�me (par une modification directe de "time" par exemple, qui ne serait autrement pas prise en compte ici)
	elapsedTime += time.getTotalTime() - lastUpdateTime;
	time.setTotalTime(lastUpdateTime);

	// Tronque le temps au dernier jour �coul� et ajoute son temps suppl�mentaire au temps �coul� actuel
	elapsedTime += time.getExtraTime();
	time.setTotalTime(time.getTotalTime() - time.getExtraTime());

	// Boucle de mise � jour : on met � jour le syst�me tant qu'il n'est pas compl�tement synchronis� avec le temps �coul� r�el
	while (elapsedTime >= DAY_TIME)
	{
		// Ajoute le temps �coul� r�el au temps du jeu avec un maximum de DAY_TIME (pour ne pas avancer de plus d'un jour � la fois)
		time += DAY_TIME;
		elapsedTime -= DAY_TIME;

		// D�termine si des jours/mois/ann�es se sont �coul�s depuis la derni�re mise � jour
		const Time lastTime(lastUpdateTime);
#ifdef _DEBUG
		const bool joursEcoules = (time.getTotalJours() != lastTime.getTotalJours());
#endif
		const bool moisEcoule = (time.getTotalMois() != lastTime.getTotalMois());
		const bool anneeEcoulee = (time.getAnnees() != lastTime.getAnnees());
		lastUpdateTime = time.getTotalTime();

#ifdef _DEBUG
		// V�rifie qu'au moins 1 jour s'est �coul� depuis la derni�re mise � jour du syst�me (toujours vrai normalement !)
		if (joursEcoules)
		{
#endif
			// Met � jour le syst�me de jeu en lui indiquant si des mois ou ann�es se sont �coul�s
			updateSystem(moisEcoule, anneeEcoulee, outNormalBatimentDestroy, outOldBatimentDestroy);

			// Arr�te de mettre � jour le monde si la partie est termin�e
			if (objectives.isGameFinished())
				break;
#ifdef _DEBUG
		}
		else
			LOG_DEBUG("EcoWorldSystem::update(" << elapsedTime << ") : Aucun jour ne s'est ecoule alors que elapsedTime etait superieur a DAY_TIME :" << endl
				<< "    elsapedTime = " << elapsedTime << endl
				<< "    time.getTotalTime() = " << time.getTotalTime() << endl
				<< "    lastTime.getTotalTime() = " << lastTime.getTotalTime(), ELL_WARNING);
#endif
	}

	// Ajoute le temps �coul� qui n'a pas pu �tre mis � jour au temps du jeu (car il est inf�rieur � 2.0f)
	time += elapsedTime;

	// Met tout de m�me � jour le weather manager (pour le renderer) (cela n'aura pas d'influence sur la prochaine mise � jour, m�me si, par exemple, le facteur de production d'�nergie solaire est chang�)
	weatherManager.update();
}
void EcoWorldSystem::updateSystem(bool moisEcoule, bool anneeEcoulee, bool* outNormalBatimentDestroy, bool* outOldBatimentDestroy)
{
	/*
	Ordre des mises � jour :
	------------------------
		1. Weather Manager
		2. Informations du monde
			2.1. Ressources globalement produites (d�but)
			2.2. Ages et temps de constructions/destructions des b�timents
			2.3. Population et �nergie
			2.4. Ressources consomm�es par les habitants
			2.5. Pourcentage disponible de chaque ressource et mise � jour des informations des b�timents
		3. B�timents (mise � jour des informations du monde)
			3.1. Production d'�lectricit�
			3.2. Production d'eau
			3.3. Usines
			3.4. Maisons
			3.5. Routes et autres b�timents
			3.6. Gestion de l'effet de serre et des d�chets
				3.6.1. B�timents
				3.6.2. World Infos (taxes)
		4. Ressources globalement produites (fin)
		5. Objectifs
	*/

	// Note : On ne r�initialise pas les valeurs de outNormalBatimentDestroy et de outOldBatimentDestroy au cas o� un b�timent aurait d�j� �t� d�truit lors d'une mise � jour pr�c�dente

	// Remet les �volutions du monde � z�ro pour qu'elles soient recalcul�es
	infos.resetEvolutions();


	// 1. Met � jour le Weather Manager en mettant aussi � jour les informations du syst�me
	weatherManager.update();


	// 2. Met � jour les informations du monde :

	// 2.1. Conserve en m�moire les ressources actuelles pour pouvoir calculer les ressources globalement produites
	for (u32 i = 0; i < Ressources::RI_COUNT; ++i)
		ressourcesProduites[i] = infos.ressources[i];

	// 2.2. Met � jour les temps de vie des b�timents, ainsi que leurs temps de construction et de destruction, et d�truits les b�timents n�cessaires
	updateBatimentDaysPassed(outNormalBatimentDestroy, outOldBatimentDestroy);

	// 2.3. Met � jour les informations du monde sur la population et l'�nergie
	infos.updateDonnees();

	// 2.4. Met � jour les informations du monde sur les ressources consomm�es par les habitants
	infos.updateRessourcesConsommation();

	// 2.5. Calcule le pourcentage disponible de chaque ressource et met � jour des informations des b�timents
	calculatePourcentageRessourcesDisponiblesAndUpdateBatimentsInfos();


	// 3. Met � jour le monde d'apr�s tous les b�timents :

	// 3.1. Met � jour les b�timents de production d'�lectricit�
	core::list<Batiment*>::Iterator it = listeBatsProductionElectricite.begin();
	const core::list<Batiment*>::Iterator END = listeBatsProductionElectricite.end();
	for (it = listeBatsProductionElectricite.begin(); it != END; ++it)
		(*it)->update(moisEcoule, anneeEcoulee);

	// 3.2. Met � jour les b�timents de production d'eau
	for (it = listeBatsProductionEau.begin(); it != END; ++it)
		(*it)->update(moisEcoule, anneeEcoulee);

	// 3.3. Met � jour les usines
	for (it = listeBatsUsines.begin(); it != END; ++it)
		(*it)->update(moisEcoule, anneeEcoulee);

	// 3.4. Met � jour les maisons
	for (it = listeBatsMaisons.begin(); it != END; ++it)
		(*it)->update(moisEcoule, anneeEcoulee);

	// 3.5. Met � jour les autres b�timents
	for (it = listeBatsOther.begin(); it != END; ++it)
		(*it)->update(moisEcoule, anneeEcoulee);

	// 3.6.1. Met � jour les b�timents g�rant l'effet de serre et les d�chets
	for (it = listeBatsDechetsES.begin(); it != END; ++it)
		(*it)->update(moisEcoule, anneeEcoulee);

	// 3.6.2. Met � jour les informations du monde sur les taxes de l'effet de serre et des d�chets
	infos.updateTaxes();


	// 4. Calcule les ressources globalement produites en comparant les ressources disponibles au d�but de cette mise � jour et les ressources actuelles
	for (u32 i = 0; i < Ressources::RI_COUNT; ++i)
		ressourcesProduites[i] = infos.ressources[i] - ressourcesProduites[i];


	// 5. Met � jour les objectifs pour cette partie
	objectives.update();
}
void EcoWorldSystem::updateBatimentDaysPassed(bool* outNormalBatimentDestroy, bool* outOldBatimentDestroy)
{
	// Note : On ne r�initialise pas les valeurs de outNormalBatimentDestroy et de outOldBatimentDestroy au cas o� un b�timent aurait d�j� �t� d�truit lors d'une mise � jour pr�c�dente
	
	// Met � jour l'�ge de tous les b�timents
	const core::list<Batiment*>::ConstIterator END = listeAllBats.end();
	core::list<Batiment*>::ConstIterator it = listeAllBats.begin();
	for (; it != END; ++it)
		(*it)->updateDaysPassed(outNormalBatimentDestroy, outOldBatimentDestroy);

	// Supprime les b�timents devant �tre d�truits depuis la liste de destruction
	for (it = deleteList.begin(); it != END; ++it)
		deleteBatiment(*it);
	deleteList.clear();
}
void EcoWorldSystem::calculatePourcentageRessourcesDisponiblesAndUpdateBatimentsInfos()
{
	/*
	Probl�me r�solu ici :
	- Plut�t de qu'une usine consomme tout ce qui est actuellement disponible lors de son actualisation
		et qu'ensuite les autres usines ne puissent plus utiliser cette ressource,
		dor�navant lorsqu'une ressource est manquante on calcule sa quantit� n�cessaire totale
		et on caclule son pourcentage de disponibilit� pour que chaque usine en ait la m�me quantit� proportionnellement � ses besoins

	Utilit� illustr�e :
	- Il reste 500 kg de sable, il y a une usine qui en consomme 250 kg par jour, une autre qui en consomme 750 kg par jour, et 1 jour s'est �coul� :
		le total de sable n�cessaire est donc de 250 + 750 = 1000 kg de sable, chaque usine aura donc 500 / 1000 = 50 % de sable disponible,
		la premi�re usine recevra donc 0.5 * 250 = 125 kg de sable, et la seconde recevra 0.5 * 750 = 375 kg de sable

	TODO : Un probl�me assez subtile est encore pr�sent : Exemple :
	- Une usine de verre demande du sable pour produire du verre, supposons que le sable soit manquant et que son pourcentage de disponibilit� soit de 50%.
		Mais, supposons que le verre soit lui aussi manquant et que son pourcentage de disponibilit� soit de 80%,
		et qu'il n'existe qu'une seule usine de verre dans le monde.
		Lors du calcul de la disponibilit� des ressources, le verre sera donc disponible � 80%, mais lors de l'actualisation de l'usine de verre,
		pour sa production de verre, cette derni�re prendra en compte le fait que la disponibilit� du sable n'est que de 50%,
		elle produira donc 2 fois moins de verre que pr�vu au d�part, et puisqu'elle est seule, le pourcentage de verre disponble sera en r�alit� de 40%.
		Ainsi, les autres usines n�cessitant du verre fonctionneront avec un pourcentage de disponibilit� du verre de 80% alors qu'en r�alit� il n'est que de 40%,
		et � la fin de la mise � jour, le verre sera donc en quantit� n�gative dans les entrep�ts.

		-> La difficult� de ce probl�me vient du fait qu'il faudrait conna�tre la disponibilit� de ressources qui n'ont pas encore �t� calcul�es pour calculer
			la disponibilit� des ressources actuelles. Et pour calculer ces derni�res, il faut peut-�tre m�me calculer la disponibilit� de la ressource actuelle !

	=> Pour palier � ce probl�me, on peut faire quatre calculs � la suite lors de cette fonction :
		- Le premier serait le calcul actuel, normal qui ne se base que sur le pourcentage de production maximal demand� par l'utilisateur.
		- Le deuxi�me se baserait alors sur le premier, mais prendrait aussi en compte le pourcentage de production de toutes les ressources n�cessaires
			� la fabrication de la ressource actuellement calcul�e (ex : pour du verre, on prendrait en compte le sable, la potasse, le quartz et l'eau)
		- Le troisi�me calcul se baserait alors sur le deuxi�me, mais prendrait aussi en compte le pourcentage de production de toutes les ressources n�cessaires
			en m�me temps que celle-ci pour fabriquer une autre ressource (ex : pour la consommation du sable, puisqu'il est n�cessaire pour du verre, on prendrait aussi le quartz et l'eau,
			car le pourcentage de production du sable serait limitant pour leur consommation dans cette usine, et inversement)
		- Le quatri�me se contenterait alors de recalculer la production/consommation totale de toutes les ressources
			en prenant en compte les facteurs de production pr�cedemment calcul�s
	===> Grande complexit� de l'algorithme, des probl�mes peuvent encore subsister (sinon, pourquoi un quatri�me calcul recalculant toutes les consommations une fois de plus ?),
		et le temps de calcul n�cessaire � cette fonction deviendrait alors �norme !

	=====> Un probl�me subsisterait encore lorsque deux ressources sont mutuellement n�cessaires, mais ce cas-l� serait � �viter dans la cr�ation des usines !
	*/

	// Tableau permettant de stocker l'�volution des ressources aujourd'hui
	float ressourcesEvolution[Ressources::RI_COUNT] = {0.0f};

	u32 i;	// Variable d'it�ration

	// R�initialise les pourcentages de ressources disponibles comme si les stocks de ressources �taient toujours suffisants (sera utilis� plus loin pour r�initialiser la production/consommation de ressources des b�timents)
	for (i = 0; i < Ressources::RI_COUNT; ++i)
		pourcentageRessourcesDisponibles[i] = 1.0f;



	// Parcours tous les b�timents et calcule la quantit� de chaque ressource utilis�e/produite aujourd'hui :
	const core::list<Batiment*>::ConstIterator END = listeAllBats.end();
	core::list<Batiment*>::ConstIterator it = listeAllBats.begin();
	for (; it != END; ++it)
	{
		Batiment* const bat = (*it);
		const BatimentInfos& batInfos = bat->getInfos();
		const u32 ressourcesSize = batInfos.ressourcesJ.size();

		// R�initialise les informations sur la production/consommation de ressources de ce b�timent comme si les stocks de ressources �taient toujours suffisants, pour obtenir sa production/consommation r�ellement demand�e
		// On passe le param�tre firstCall � true pour indiquer que c'est le premier appel de cette fonction ce jour-ci pour ce b�timent.
		bat->updateBatimentInfos(true, pourcentageRessourcesDisponibles);

		// V�rifie que ce b�timent produit/consomme bien des ressources, et qu'il est bien actif aujourd'hui (qu'il n'est ni en construction, ni en destruction)
		if (ressourcesSize && !bat->isConstructingOrDestroying())
		{
			// Parcours toutes les ressources produites/consomm�es par ce b�timent
			for (i = 0; i < ressourcesSize; ++i)
			{
#ifdef _DEBUG
				// V�rification de d�bogage :
				// Attention : Les deux tableaux batInfos.ressourcesJ et staticBatInfos.ressourcesJ doivent avoir le m�me ID de ressource � chaque �l�ment :
				// on doit toujours avoir batInfos.ressourcesJ[i].ID == staticBatInfos.ressourcesJ[i].ID
				if (batInfos.ressourcesJ[i].ID != bat->getStaticInfos().ressourcesJ[i].ID)
					LOG_DEBUG("EcoWorldSystem::calculatePourcentageRessourcesDisponibles() : Les ID des ressources produites/consommees par un batiment ne sont pas identiques entre les informations statiques et les informations variables de ce batiment :" << endl
						<< "    batimentID = " << bat->getID() << " ; batInfos.ressourcesJ[i].ID = " << batInfos.ressourcesJ[i].ID << " ; staticBatInfos.ressourcesJ[i].ID = " << bat->getStaticInfos().ressourcesJ[i].ID << " !", ELL_WARNING);
#endif

				// Ajoute les ressources produites/consomm�es par ce b�timent � l'�volution des ressources du monde
				ressourcesEvolution[batInfos.ressourcesJ[i].ID] += batInfos.ressourcesJ[i].production * batInfos.pourcentageProduction;
			}
		}

		// Prend aussi en compte la consommation d'eau des b�timents, non comprise dans leur consommation de ressources en eau
		ressourcesEvolution[Ressources::RI_eau] -= batInfos.eauConsommationJ;
	}



	// Calcule les pourcentages disponibles de chaque ressource :
	for (i = 0; i < Ressources::RI_COUNT; ++i)
	{
		// Calcule le pourcentage de cette ressource disponible d'apr�s sa consommation/production totale
		// et sa quantit� disponible actuellement dans le monde, en restant dans les limites 0.0f et 1.0f
		if (ressourcesEvolution[i] < 0.0f)	// Ressource globalement consomm�e
		{
			if (infos.ressources[i] > 0.0f)	// Ressource disponible
			{
				if (infos.ressources[i] + ressourcesEvolution[i] < 0.0f)	// Ressource manquante pour aujourd'hui
					pourcentageRessourcesDisponibles[i] = -infos.ressources[i] / ressourcesEvolution[i];
				else	// Ressource encore suffisante pour aujourd'hui
					pourcentageRessourcesDisponibles[i] = 1.0f;
			}
			else	// Ressource compl�tement indisponible
				pourcentageRessourcesDisponibles[i] = 0.0f;
		}
		else	// Ressource globalement produite
			pourcentageRessourcesDisponibles[i] = 1.0f;
	}



	// Met � jour les informations des b�timents d'apr�s les pourcentages disponibles de chaque ressource
	// On passe le param�tre firstCall � false puisque ce n'est pas le premier appel � cette fonction ce jour-ci pour ce b�timent.
	for (it = listeAllBats.begin(); it != END; ++it)
		(*it)->updateBatimentInfos(false, pourcentageRessourcesDisponibles);
}
