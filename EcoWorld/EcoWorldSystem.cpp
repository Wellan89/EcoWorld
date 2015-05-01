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

	// Initialise le système en le remettant à zéro
	reset();
}
EcoWorldSystem::~EcoWorldSystem()
{
	// Détruit la carte
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
	// Indique le système et les modifiers aux informations du monde, au Weather Manager et aux objectifs
	infos.setSystem(this);
	infos.setModifiers(&modifiers);
	weatherManager.setSystem(this);
	objectives.setSystem(this);

	// Réinitialise les modifiers, les informations du monde, le Weather Manager et les objectifs
	// On ne réinitialise pas la difficulté du jeu : permet de conserver les difficultés personnalisées lorsque le joueur désire recommencer une partie qu'il a précédemment chargée
	//modifiers.setDifficulty(EcoWorldModifiers::ED_normal);
	infos.reset();
	weatherManager.reset();
	objectives.reset();

	// Remet le temps à zéro
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

	// Détruit et réinitialise la carte
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

	// Réinitialise les tableaux des ressources
	for (u32 i = 0; i < Ressources::RI_COUNT; ++i)
	{
		ressourcesProduites[i] = 0.0f;
		pourcentageRessourcesDisponibles[i] = 0.0f;
	}
}
void EcoWorldSystem::createNewGame(EcoWorldModifiers::E_DIFFICULTY difficulty, const core::list<ObjectiveData>& objectivesList)
{
	// Indique la difficulté du jeu
	modifiers.setDifficulty(difficulty);

	// Indique les objectifs de cette partie
	objectives.reset();
	objectives.addObjectives(objectivesList);

	// Indique le budget de départ
	infos.budget = 5000000.0f										* modifiers.startBudgetFactor;		// 5 000 000 €

	// Donne quelques ressources de départ :
	infos.ressources[Ressources::RI_eau] = 100000.0f				* modifiers.startRessourcesFactor;	// 100 000  L d'eau
	infos.ressources[Ressources::RI_bois] = 10000.0f				* modifiers.startRessourcesFactor;	//  10 000 kg de bois
	infos.ressources[Ressources::RI_verre] = 2000.0f				* modifiers.startRessourcesFactor;	//   2 000 kg de verre
	infos.ressources[Ressources::RI_ciment] = 40000.0f				* modifiers.startRessourcesFactor;	//  40 000 kg de ciment
	infos.ressources[Ressources::RI_tuiles] = 30000.0f				* modifiers.startRessourcesFactor;	//  30 000 kg de tuiles
#ifndef KID_VERSION	// En mode enfant : on désactive les ressources ne pouvant pas être produites
	infos.ressources[Ressources::RI_pierre] = 150000.0f				* modifiers.startRessourcesFactor;	// 150 000 kg de pierre
	infos.ressources[Ressources::RI_fer] = 3500.0f					* modifiers.startRessourcesFactor;	//   3 500 kg de métal (fer)
	infos.ressources[Ressources::RI_sable] = 100000.0f				* modifiers.startRessourcesFactor;	// 100 000 kg de sable
	infos.ressources[Ressources::RI_panneauxPhotovoltaiques] = 50	* modifiers.startRessourcesFactor;	// 50 panneaux solaires

	// Ressources de base consommées par les habitants : Suffisamment de ressources pour que 50 habitants puissent en consommer pendant 200 jours sans en manquer
	infos.ressources[Ressources::RI_viande] = 2000.0f				* modifiers.startRessourcesFactor;	//   2 000 kg de viande
	infos.ressources[Ressources::RI_pain] = 2000.0f					* modifiers.startRessourcesFactor;	//   2 000 kg de pain
	infos.ressources[Ressources::RI_lait] = 5000.0f					* modifiers.startRessourcesFactor;	//   5 000  L de lait
	infos.ressources[Ressources::RI_vetements] = 500.0f				* modifiers.startRessourcesFactor;	//     500 kg de vêtements
#else				// En mode enfant : on réactiver le papier comme ressource consommée par les habitants pour leur satisfaction
	infos.ressources[Ressources::RI_papier] = 10000.0f				* modifiers.startRessourcesFactor;	//   10 000 kg de papier
#endif
}
core::list<ObjectiveData> EcoWorldSystem::getNewGameDefaultObjectives()
{
	core::list<ObjectiveData> objectives;

	// Ajoute les objectifs par défaut :

	// Le joueur a perdu lorsque ses dettes sont supérieures à 100 000 €
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

	// Ajoute le temps lors de la dernière mise à jour du monde (à conserver, on ne peut pas être sûr que le monde a bien été mis à jour avec le temps actuel)
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

		// Enregistre les infos du batiment sous le nom de bloc générique <Batiment> (on ne conserve plus le numéro du bâtiment dans son nom, car ce n'est pas nécessaire pour son chargement)
		out->write(writer, false, L"Batiment");
		out->clear();

		// Demande au node 3D de ce bâtiment d'ajouter ses informations
		// TODO : Si nécessaire
		/*
		if (batiment->getSceneNode())
		{
			// Ajoute les infos du node 3D
			batiment->getSceneNode()->save(out);

			// Enregistre les infos du node sous le nom de bloc générique <SceneNode> (on ne conserve plus le numéro du bâtiment dans son nom, car ce n'est pas nécessaire pour son chargement)
			out->write(writer, false, L"SceneNode");
			out->clear();
		}
		*/

		// Incrémente le compteur de batiment
		batimentCount++;
	}
}
void EcoWorldSystem::load(io::IAttributes* in, io::IXMLReader* reader, CLoadingScreen* loadingScreen, float loadMin, float loadMax)
{
	if (!in || !reader)
		return;

	if (loadingScreen)
		if (loadingScreen->setPercentAndDraw(loadMin))	return;

	// Remet le système à zéro, mais conserve les informations sur le terrain
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
		loadMin += (loadMax - loadMin) * 0.1f;	// Change le nouveau minimum de la barre de chargement pour simplifier les calculs lors du chargement des bâtiments
		if (loadingScreen->setPercentAndDraw(loadMin))	return;
	}

	// Lit les informations à partir du fichier
	reader->resetPosition();
	if (in->read(reader, false, L"System"))
	{
		// Lit les informations du temps
		{
			float currentTotalTime = 0.0f;
			if (in->existsAttribute("TotalTime"))		currentTotalTime = in->getAttributeAsFloat("TotalTime");
			if (in->existsAttribute("LastUpdateTime"))	lastUpdateTime = in->getAttributeAsFloat("LastUpdateTime");	else	lastUpdateTime = currentTotalTime;

			// Recalcule les informations sur le temps actuel avec le dernier temps de la mise à jour
			time.setTotalTime(lastUpdateTime);
			weatherManager.update();
			if (lastUpdateTime != currentTotalTime)
				time.setTotalTime(currentTotalTime);
		}

		if (in->existsAttribute("BatimentCount"))
		{
			const int batimentCount = in->getAttributeAsInt("BatimentCount");
			const float batimentCountF_Inv = 1.0f / ((float)batimentCount);	// Nécessaire pour la barre de chargement

			in->clear();
			for (int i = 0; i < batimentCount; ++i)
			{
				if (loadingScreen)
					if (loadingScreen->setPercentAndDraw((loadMax - loadMin) * (float)i * batimentCountF_Inv + loadMin))	return;

				// Lit les infos du batiment (elles ne seront pas relues après) :

				// Réinitialisation de la position du lecteur XML dans le fichier désactivée car elle ne permet pas le chargement des bâtiments autres que le premier :
				// ils ont maintenant tous le même nom de bloc <Batiment> ... </Batiment>.
				// Et si il pouvait être activé, cela augmenterait sensiblement le temps de chargement d'une partie sauvegardée, en supportant la résolution du bug suivant :
				/*
				- BUG : Lors du chargement d'anciennes parties, lorsqu'un bloc est manquant (ex : si le bloc "<GUI> ... </GUI>" manque) ou mal placé dans le fichier, les blocs suivants ne sont plus lus correctement
					=> Faire que le manque ou le mauvais placement d'un bloc ne soit pas fatal au reste du chargement !
				*/
				// En effet, le fait qu'un bloc de bâtiment soit manquant ou mal placé arrive rarement pour les sauvegardes (même lors du passage vers une version plus récente du jeu),
				// et les données des bâtiment étant placées en fin de fichier, ce sont pour ces derniers que la résolution de ce bug se révèle la plus lente
				// (car tout le fichier depuis le début doit être re-parcouru pour vérifier que ce bloc ne se situe pas avant).
				//reader->resetPosition();
				if (in->read(reader, false, L"Batiment"))
				{
					// Vérifie que les deux valeurs importantes pour un bâtiment existent
					if (!in->existsAttribute("Name") || !in->existsAttribute("Index"))
						continue;

					// Nouvelle méthode d'enregistrement des bâtiments : on n'enregistre plus leur ID mais leur nom, pour éviter une complète invalidation des sauvegardes en cas de changement des ID des bâtiments :
					//const BatimentID batID = (BatimentID)in->getAttributeAsInt("ID");

					// Détermine l'ID du bâtiment d'après son nom :
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
					// Vérifie que l'ID du bâtiment a bien été trouvée, sinon on ne peut créer ce bâtiment : on passe au bâtiment suivant
					if (batID == BI_aucun)
						continue;
					
					const core::vector2di index = in->getAttributeAsPosition2d("Index");
					const float rotation = (in->existsAttribute("Rotation") ? in->getAttributeAsFloat("Rotation") : 0.0f);

					// Crée le batiment et lui demande charger ses données depuis le fichier
					carte[index.X][index.Y].batiment = new Batiment(batID, (*this), infos, modifiers, index, rotation);
					carte[index.X][index.Y].batiment->load(in);

					// Ajoute ce bâtiment aux listes
					addBatimentToLists(carte[index.X][index.Y].batiment);

					// Ajoute le batiment au renderer et le node créé au bâtiment
					if (systemRenderer)
						carte[index.X][index.Y].batiment->setSceneNode(systemRenderer->addBatiment(carte[index.X][index.Y].batiment));

					in->clear();

					// Demande au node 3D de ce bâtiment de se charger depuis le fichier
					// TODO : Si nécessaire
					/*
					if (carte[index.X][index.Y].batiment->getSceneNode())
					{
						// Lit les infos du node (elles ne seront pas relues après) :

						// Réinitialisation de la position du lecteur XML dans le fichier désactivée car elle ne permet pas le chargement des bâtiments autres que le premier :
						// ils ont maintenant tous le même nom de bloc <SceneNode> ... </SceneNode>.
						// Et si il pouvait être activé, cela augmenterait sensiblement le temps de chargement d'une partie sauvegardée, en supportant la résolution du bug suivant :
						/*
						- BUG : Lors du chargement d'anciennes parties, lorsqu'un bloc est manquant (ex : si le bloc "<GUI> ... </GUI>" manque) ou mal placé dans le fichier, les blocs suivants ne sont plus lus correctement
							=> Faire que le manque ou le mauvais placement d'un bloc ne soit pas fatal au reste du chargement !
						* /
						// En effet, le fait qu'un bloc de scene node soit manquant ou mal placé arrive rarement pour les sauvegardes (même lors du passage vers une version plus récente du jeu),
						// et les données des scene nodes étant placées en fin de fichier, ce sont pour ces derniers que la résolution de ce bug se révèle la plus lente
						// (car tout le fichier depuis le début doit être re-parcouru pour vérifier que ce bloc ne se situe pas avant).
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

	// Recalcule les infos sur le temps d'après le temps actuel du système de jeu (restauré après le chargement du WeatherManager)
	weatherManager.update();

	// Met à jour les informations du monde sur la population et l'énergie
	infos.updateDonnees();

	// Met à jour les objectifs du jeu
	objectives.update();

	// Met à jour le système de jeu complet, au cas où lastUpdateTime != totalTime
	update(0.0f);

	if (loadingScreen)
		loadingScreen->setPercentAndDraw(loadMax);
}
int EcoWorldSystem::canCreateBatiment(BatimentID batimentID, const core::vector2di& index, float rotation, bool outRessources[]) const
{
	if (batimentID == BI_aucun || batimentID >= BI_COUNT)
		return EBCE_BATIMENT_ID;

	int error = EBCE_AUCUNE;

	// Obtient les informations sur le type du batiment à construire
	const BatimentInfos batimentInfos(batimentID);
	const StaticBatimentInfos& staticBatimentInfos = StaticBatimentInfos::getInfos(batimentID);

	// Obtient la taille du bâtiment : utilise les obbox, pour prendre en compte la rotation
	const obbox2df batimentBox(core::rectf((float)index.X - staticBatimentInfos.halfTaille.Width, (float)index.Y - staticBatimentInfos.halfTaille.Height,
		(float)index.X + staticBatimentInfos.halfTaille.Width, (float)index.Y + staticBatimentInfos.halfTaille.Height),
		rotation);

	// Obtient la liste des cases de terrain qu'occupe ce bâtiment
	const core::list<core::vector2di> listCasesSurfaceBat = batimentBox.getCasesList();
	const core::list<core::vector2di>::ConstIterator END = listCasesSurfaceBat.end();

	// Vérifie si la position du batiment est en dehors du terrain où on peut les placer, en prenant en compte sa rotation et ses dimensions
	// (Note : Ce système fonctionne parfaitement, quelle que soit la parité de TAILLE_CARTE)
	if (!batimentBox.isInsideRect(core::rectf(0.0f, 0.0f, (float)TAILLE_CARTE - 1.0f, (float)TAILLE_CARTE - 1.0f)))
		error |= EBCE_TERRAIN_OUT;
	else
	{
		// Si le bâtiment nécessite de l'eau profonde, on vérifie si le terrain en contient bien
		if (StaticBatimentInfos::needDeepWater(batimentID))
		{
			if (!(carte[index.X][index.Y].deepWater))
				error |= EBCE_NEED_DEEP_WATER;
			else
			{
				// Teste si le terrain est constructible sur toute la surface du bâtiment
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
		else	// Vérifie que le terrain est constructible si le bâtiment ne nécessite pas d'eau profonde
		{
			if (!(carte[index.X][index.Y].constructible))
				error |= EBCE_TERRAIN_NON_CONSTRUCTIBLE;
			else
			{
				// Teste si le terrain est constructible sur toute la surface du bâtiment
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

		// Si la position est déjà occupée par un autre batiment (attention : on ne peut plus construire par-dessus une route !)
		if (carte[index.X][index.Y].batiment)
			error |= EBCE_PLACE;
		else
		{
			// Obtient la taille maximale des bâtiments
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

	// Vérifie qu'on a assez d'argent
	if (infos.budget < 0 || infos.budget + staticBatimentInfos.prixC * modifiers.prixFactor < 0)
		error |= EBCE_BUDGET;

	// Vérifie qu'on a assez d'énergie
	if (infos.energie < 0 || infos.energie + batimentInfos.energie < 0)
		error |= EBCE_ENERGIE;

	// Vérifie qu'on a assez de ressources
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

	// Vérifie que la population est suffisante pour débloquer ce bâtiment
	if (infos.population < staticBatimentInfos.unlockPopulationNeeded)
		error |= EBCE_NOT_ENOUGH_POPULATION;

	return error;
}
int EcoWorldSystem::canCreateMultiBatiments(BatimentID batimentID, int batimentCount, bool outRessources[]) const
{
	if (batimentID == BI_aucun || batimentID >= BI_COUNT)
		return EBMCE_BATIMENT_ID;

	int error = EBMCE_AUCUNE;

	// Obtient les informations sur le type du batiment à construire
	const BatimentInfos batimentInfos(batimentID);
	const StaticBatimentInfos& staticBatimentInfos = StaticBatimentInfos::getInfos(batimentID);

	// Vérifie qu'on a assez d'argent
	if (infos.budget < 0 || infos.budget + staticBatimentInfos.prixC * modifiers.prixFactor * batimentCount < 0)
		error |= EBMCE_BUDGET;

	// Vérifie qu'on a assez d'énergie
	if (infos.energie < 0 || infos.energie + batimentInfos.energie * batimentCount < 0)
		error |= EBMCE_ENERGIE;

	// Vérifie qu'on a assez de ressources
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
		// Vérifie si on peut placer ce bâtiment
#ifndef _DEBUG
		const	// Le const désactivé permet de désactiver l'erreur EBCE_NOT_ENOUGH_POPULATION en mode DEBUG
#endif
			int error = canCreateBatiment(batimentID, index, rotation, outRessources);

#ifdef _DEBUG
		// /!\ Attention /!\ 
		// En mode DEBUG uniquement : On désactive l'erreur EBCE_NOT_ENOUGH_POPULATION pour permettre le fonctionnement du code de débogage pour débloquer tous les bâtiments
		error &= ~EBCE_NOT_ENOUGH_POPULATION;	// Passe le bit représenté par EBCE_NOT_ENOUGH_POPULATION à 0 (~ : opérateur NOT bit à bit)
#endif

		if (error != EBCE_AUCUNE && error != EBCE_ENERGIE)	// Vérifie que l'erreur n'est pas dûe à un manque d'énergie, qui n'est plus une erreur fatale
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
		// Vérifie que l'index fourni est bien dans les limites du terrain
		if (index.X < 0 || index.Y < 0 || index.X >= TAILLE_CARTE || index.Y >= TAILLE_CARTE)
		{
			// On ne peut vraiment pas placer ce bâtiment, même s'il est forcé (en-dehors des zones mémoires de la carte) : on retourne une erreur et on avertit l'utilisateur
			LOG_DEBUG("EcoWorldSystem::addBatiment(" << batimentID << ", ..., " << forceAdding <<") : Le placement du batiment est force, mais n'a pas pu etre effectue car le batiment est en-dehors des limites du terrain :" << endl
				<< "    index.X = " << index.X << "   ;   index.Y = " << index.Y, ELL_WARNING);
			LOG_RELEASE("AVERTISSEMENT : Ajout forcé du bâtiment au système de jeu : Position en-dehors des limites du terrain !", ELL_WARNING);
			return EBCE_TERRAIN_OUT;
		}
		else if (batimentID == BI_aucun || batimentID >= BI_COUNT)
		{
			// Le bâtiment à placer a un ID invalide (BI_aucun) : on retourne une erreur et on avertit l'utilisateur
			LOG_DEBUG("EcoWorldSystem::addBatiment(" << batimentID << ", ..., " << forceAdding <<") : Le placement du batiment est force, mais n'a pas pu etre effectue car le batiment a placer a un ID invalide (BI_aucun) :" << endl
				<< "    batimentID = " << batimentID, ELL_WARNING);
			LOG_RELEASE("AVERTISSEMENT : Ajout forcé du bâtiment au système de jeu : ID invalide !", ELL_WARNING);
			return EBCE_BATIMENT_ID;
		}
		else if (carte[index.X][index.Y].batiment)
		{
			// La position est déjà occupée par un autre bâtiment : on retourne une erreur et on avertit l'utilisateur
			LOG_DEBUG("EcoWorldSystem::addBatiment(" << batimentID << ", ..., " << forceAdding <<") : Le placement du batiment est force, mais il y a deja un autre batiment a cette position :" << endl
				<< "    index.X = " << index.X << "   ;   index.Y = " << index.Y << endl
				<< "    carte[index.X][index.Y].batiment = " << carte[index.X][index.Y].batiment, ELL_WARNING);
			LOG_RELEASE("AVERTISSEMENT : Ajout forcé du bâtiment au système de jeu : Un autre bâtiment occupe déjà cette position !", ELL_WARNING);
			return EBCE_PLACE;
		}
	}
#endif

	// Il n'y a pas d'erreur pour placer le batiment : on ajoute le batiment
	carte[index.X][index.Y].batiment = new Batiment(batimentID, (*this), infos, modifiers, index, rotation);

	// Ajoute ce bâtiment aux listes
	addBatimentToLists(carte[index.X][index.Y].batiment);

	// Ajoute le batiment au renderer et le node créé au bâtiment
	if (systemRenderer)
		carte[index.X][index.Y].batiment->setSceneNode(systemRenderer->addBatiment(carte[index.X][index.Y].batiment, rendererPosition, rendererDenivele));

	// Met à jour les informations du monde avec les coûts de construction de ce bâtiment
	carte[index.X][index.Y].batiment->onConstruct();

#ifdef USE_RAKNET
	// Force la durée de vie de ce bâtiment si nécessaire
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

	// Ajoute tout d'abord ce bâtiment à la liste de tous les bâtiments (sans aucun tri préalable)
	listeAllBats.push_back(batiment);

	// Ajoute ensuite ce bâtiment à sa liste
	const core::list<Batiment*>::Iterator END = listeBatsProductionElectricite.end();
	switch (batimentID)
	{
		// Production d'électricité
	case BI_centrale_charbon:
	case BI_panneau_solaire:
	case BI_eolienne:
	case BI_hydrolienne:
		{
			// Insère ce bâtiment dans la liste tel que son ID soit inférieur à l'ID des bâtiments qui le suivent
			for (it = listeBatsProductionElectricite.begin(); it != END; ++it)
				if ((*it)->getID() > batimentID)	// Dès que l'ID d'un bâtiment de la liste est supérieur à l'ID du bâtiment à insérer, on insère ce bâtiment juste avant cette position de la liste
				{
					listeBatsProductionElectricite.insert_before(it, batiment);
					hasAddedBat = true;
					break;
				}

			// Si aucun bâtiment n'a d'ID supérieur à celui de ce bâtiment, on l'ajoute à la fin de la liste
			if (!hasAddedBat)
				listeBatsProductionElectricite.push_back(batiment);
		}
		break;

		// Production d'eau
	case BI_pompe_extraction_eau:
		{
			// Insère ce bâtiment dans la liste tel que son ID soit inférieur à l'ID des bâtiments qui le suivent
			for (it = listeBatsProductionEau.begin(); it != END; ++it)
				if ((*it)->getID() > batimentID)	// Dès que l'ID d'un bâtiment de la liste est supérieur à l'ID du bâtiment à insérer, on insère ce bâtiment juste avant cette position de la liste
				{
					listeBatsProductionEau.insert_before(it, batiment);
					hasAddedBat = true;
					break;
				}

			// Si aucun bâtiment n'a d'ID supérieur à celui de ce bâtiment, on l'ajoute à la fin de la liste
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
#ifdef KID_VERSION	// En mode enfant : on réactiver le papier comme ressource consommée par les habitants pour leur satisfaction
	case BI_usine_papier_petite:
	case BI_usine_papier_grande:
#else				// On désactive l'usine à tout faire en mode enfant qui est devenue inutile
	case BI_usine_tout:
#endif
		{
			// Insère ce bâtiment dans la liste tel que son ID soit inférieur à l'ID des bâtiments qui le suivent
			for (it = listeBatsUsines.begin(); it != END; ++it)
				if ((*it)->getID() > batimentID)	// Dès que l'ID d'un bâtiment de la liste est supérieur à l'ID du bâtiment à insérer, on insère ce bâtiment juste avant cette position de la liste
				{
					listeBatsUsines.insert_before(it, batiment);
					hasAddedBat = true;
					break;
				}

			// Si aucun bâtiment n'a d'ID supérieur à celui de ce bâtiment, on l'ajoute à la fin de la liste
			if (!hasAddedBat)
				listeBatsUsines.push_back(batiment);
		}
		break;

		// Maisons
#ifndef KID_VERSION	// En mode enfant : on désactive la maison de base qui est inutile
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
			// Insère ce bâtiment dans la liste tel que son ID soit inférieur à l'ID des bâtiments qui le suivent
			for (it = listeBatsMaisons.begin(); it != END; ++it)
				if ((*it)->getID() > batimentID)	// Dès que l'ID d'un bâtiment de la liste est supérieur à l'ID du bâtiment à insérer, on insère ce bâtiment juste avant cette position de la liste
				{
					listeBatsMaisons.insert_before(it, batiment);
					hasAddedBat = true;
					break;
				}

			// Si aucun bâtiment n'a d'ID supérieur à celui de ce bâtiment, on l'ajoute à la fin de la liste
			if (!hasAddedBat)
				listeBatsMaisons.push_back(batiment);
		}
		break;

#ifndef KID_VERSION	// En mode enfant : on désactive les routes qui sont inutiles
		// Routes et autre
	case BI_route:
		{
			// Insère ce bâtiment dans la liste tel que son ID soit inférieur à l'ID des bâtiments qui le suivent
			for (it = listeBatsOther.begin(); it != END; ++it)
				if ((*it)->getID() > batimentID)	// Dès que l'ID d'un bâtiment de la liste est supérieur à l'ID du bâtiment à insérer, on insère ce bâtiment juste avant cette position de la liste
				{
					listeBatsOther.insert_before(it, batiment);
					hasAddedBat = true;
					break;
				}

			// Si aucun bâtiment n'a d'ID supérieur à celui de ce bâtiment, on l'ajoute à la fin de la liste
			if (!hasAddedBat)
				listeBatsOther.push_back(batiment);
		}
		break;
#endif

		// Gestion de l'effet de serre et des déchets
	case BI_arbre_aspen:
	case BI_arbre_oak:
	case BI_arbre_pine:
	case BI_arbre_willow:
	case BI_decharge:
	case BI_usine_incineration_dechets:
		{
			// Insère ce bâtiment dans la liste tel que son ID soit inférieur à l'ID des bâtiments qui le suivent
			for (it = listeBatsDechetsES.begin(); it != END; ++it)
				if ((*it)->getID() > batimentID)	// Dès que l'ID d'un bâtiment de la liste est supérieur à l'ID du bâtiment à insérer, on insère ce bâtiment juste avant cette position de la liste
				{
					listeBatsDechetsES.insert_before(it, batiment);
					hasAddedBat = true;
					break;
				}

			// Si aucun bâtiment n'a d'ID supérieur à celui de ce bâtiment, on l'ajoute à la fin de la liste
			if (!hasAddedBat)
				listeBatsDechetsES.push_back(batiment);
		}
		break;

	default:
		LOG_DEBUG("EcoWorldSystem::addBatimentToLists(" << batiment << ") : ID non reconnu lors de l'ajout dans les listes des batiments : batimentID = " << batimentID, ELL_WARNING);
		{
			// Insère ce bâtiment dans la liste tel que son ID soit inférieur à l'ID des bâtiments qui le suivent
			for (it = listeBatsOther.begin(); it != END; ++it)
				if ((*it)->getID() > batimentID)	// Dès que l'ID d'un bâtiment de la liste est supérieur à l'ID du bâtiment à insérer, on insère ce bâtiment juste avant cette position de la liste
				{
					listeBatsOther.insert_before(it, batiment);
					hasAddedBat = true;
					break;
				}

			// Si aucun bâtiment n'a d'ID supérieur à celui de ce bâtiment, on l'ajoute à la fin de la liste
			if (!hasAddedBat)
				listeBatsOther.push_back(batiment);
		}
		break;
	}
}
int EcoWorldSystem::canDestroyBatiment(const core::vector2di& index, bool outRessources[]) const
{
	// Vérifie si la position du batiment est en dehors du terrain où on peut les placer
	// (Note : Ce système fonctionne parfaitement, quelle que soit la parité de TAILLE_CARTE)
	if (index.X < 0 || index.Y < 0 || index.X >= TAILLE_CARTE || index.Y >= TAILLE_CARTE)
		return EBDE_INVALID_INDEX;

	int error = EBDE_AUCUNE;

	// Vérifie que le bâtiment n'est pas déjà en train de se détruire
	if (carte[index.X][index.Y].batiment->isDestroying())
		error |= EBDE_ALREADY_DESTRUCTING;

	const StaticBatimentInfos& staticBatimentInfos = carte[index.X][index.Y].batiment->getStaticInfos();

	// Vérifie qu'on a assez d'argent
	if (infos.budget < 0 || infos.budget + staticBatimentInfos.prixD * modifiers.prixFactor < 0)
		error |= EBDE_BUDGET;

	// Vérifie qu'on a assez de ressources
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
		// Vérifie si on peut détruire ce bâtiment
		const int error = canDestroyBatiment(index, outRessources);
		if (error != EBDE_AUCUNE)
			return error;
#ifdef USE_RAKNET
	}
	else
	{
		// Vérifie que l'index fourni est bien dans les limites du terrain
		if (index.X < 0 || index.Y < 0 || index.X >= TAILLE_CARTE || index.Y >= TAILLE_CARTE)
		{
			// On ne peut vraiment pas placer ce bâtiment, même s'il est forcé (en-dehors des zones mémoires de la carte) : on retourne une erreur et on avertit l'utilisateur
			LOG_DEBUG("EcoWorldSystem::destroyBatiment(..., " << forceDestroying <<") : La destruction du batiment est forcee, mais n'a pas pu etre effectue car la position specifiee se situe en-dehors des limites du terrain :" << endl
				<< "    index.X = " << index.X << "   ;   index.Y = " << index.Y, ELL_WARNING);
			LOG_RELEASE("AVERTISSEMENT : Destruction forcée du bâtiment du système de jeu : Position en-dehors des limites du terrain !", ELL_WARNING);
			return EBCE_TERRAIN_OUT;
		}
		else if (!carte[index.X][index.Y].batiment)
		{
			// Vérifie qu'il existe bien un batiment à cet emplacement, sinon on ne peut le détruire, et on retourne une erreur
			LOG_DEBUG("EcoWorldSystem::destroyBatiment(..., " << forceDestroying <<") : La destruction du batiment est forcee, mais aucun batiment n'occupe cette position :" << endl
				<< "    index.X = " << index.X << "   ;   index.Y = " << index.Y << endl
				<< "    carte[index.X][index.Y].batiment = " << carte[index.X][index.Y].batiment, ELL_WARNING);
			LOG_RELEASE("AVERTISSEMENT : Destruction forcée du bâtiment du système de jeu : Aucun bâtiment n'occupe cette position !", ELL_WARNING);
			return EBDE_INVALID_INDEX;
		}
	}
#endif

	Batiment* const batiment = carte[index.X][index.Y].batiment;

	// Vérifie qu'il existe bien un batiment à cet emplacement
	if (!batiment)
	{
		LOG_DEBUG("EcoWorldSystem::deleteBatiment(index, outRessources) : Il n'y a aucun batiment a cette position alors que la fonction canCreateBatiment n'a retourne aucune erreur : batiment = " << batiment, ELL_WARNING);
		return EBDE_INVALID_INDEX;
	}

	// Lance l'ordre de destruction du bâtiment
	batiment->destroy(time.getTotalJours());

	return EBDE_AUCUNE;
}
void EcoWorldSystem::addBatimentToDeleteList(Batiment* batiment)
{
	if (!batiment)
		return;

	// Ajoute le bâtiment à la liste de suppression
	deleteList.push_back(batiment);
}
void EcoWorldSystem::deleteBatiment(Batiment* batiment)
{
	if (!batiment)
		return;

	// Supprime le batiment du renderer
	if (systemRenderer)
		systemRenderer->deleteBatiment(batiment, (batiment->getDestroyingDay() != 0));	// La destruction n'a pas été demandée par l'utilisateur si le jour on a demandé de détruire ce bâtiment n'existe pas !

	// Supprime le bâtiment des listes
	deleteBatimentInLists(batiment);

	// Supprime définitivement le batiment
	carte[batiment->getIndex().X][batiment->getIndex().Y].batiment = NULL;
	delete batiment;
}
void EcoWorldSystem::deleteBatimentInLists(Batiment* batiment)
{
	// Tous les bâtiments
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

	// Production d'électricité
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

	// Gestion de l'effet de serre et des déchets
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
	// Ajoute le temps écoulé au temps du jeu
	time += elapsedTime;

	// Met à jour le Weather Manager sans mettre à jour les informations du système (optimisation pour le menu principal)
	weatherManager.update();
}
void EcoWorldSystem::update(float elapsedTime, bool* outNormalBatimentDestroy, bool* outOldBatimentDestroy)
{
	// Met à jour le système jour par jour : évite certaines erreurs de calcul dans le système lorsque le temps depuis la dernière frame inclus plus d'un jour système
	// - Exemple : supposons que 2 jours se soient écoulés depuis la dernière mise à jour du système, et qu'un bâtiment soit à 1 jour de sa fin de vie, alors à sa mise à jour il sera supprimé sans qu'il n'ait vécu son dernier jour
	//		Cela provoquerait aussi des bugs dans le calcul de la consommation d'eau (lorsque plusieurs jours s'écoulent au chevauchement d'un mois), ainsi que de nombreux autres problèmes (taxes sur l'ES et les déchets par exemple...)

	// Réinitialise les valeurs de outNormalBatimentDestroy et de outOldBatimentDestroy : aucun bâtiment n'a encore été détruit
	if (outNormalBatimentDestroy)
		(*outNormalBatimentDestroy) = false;
	if (outOldBatimentDestroy)
		(*outOldBatimentDestroy) = false;

	// Prend en compte dans le temps écoulé un ajout annexe au temps système (par une modification directe de "time" par exemple, qui ne serait autrement pas prise en compte ici)
	elapsedTime += time.getTotalTime() - lastUpdateTime;
	time.setTotalTime(lastUpdateTime);

	// Tronque le temps au dernier jour écoulé et ajoute son temps supplémentaire au temps écoulé actuel
	elapsedTime += time.getExtraTime();
	time.setTotalTime(time.getTotalTime() - time.getExtraTime());

	// Boucle de mise à jour : on met à jour le système tant qu'il n'est pas complètement synchronisé avec le temps écoulé réel
	while (elapsedTime >= DAY_TIME)
	{
		// Ajoute le temps écoulé réel au temps du jeu avec un maximum de DAY_TIME (pour ne pas avancer de plus d'un jour à la fois)
		time += DAY_TIME;
		elapsedTime -= DAY_TIME;

		// Détermine si des jours/mois/années se sont écoulés depuis la dernière mise à jour
		const Time lastTime(lastUpdateTime);
#ifdef _DEBUG
		const bool joursEcoules = (time.getTotalJours() != lastTime.getTotalJours());
#endif
		const bool moisEcoule = (time.getTotalMois() != lastTime.getTotalMois());
		const bool anneeEcoulee = (time.getAnnees() != lastTime.getAnnees());
		lastUpdateTime = time.getTotalTime();

#ifdef _DEBUG
		// Vérifie qu'au moins 1 jour s'est écoulé depuis la dernière mise à jour du système (toujours vrai normalement !)
		if (joursEcoules)
		{
#endif
			// Met à jour le système de jeu en lui indiquant si des mois ou années se sont écoulés
			updateSystem(moisEcoule, anneeEcoulee, outNormalBatimentDestroy, outOldBatimentDestroy);

			// Arrête de mettre à jour le monde si la partie est terminée
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

	// Ajoute le temps écoulé qui n'a pas pu être mis à jour au temps du jeu (car il est inférieur à 2.0f)
	time += elapsedTime;

	// Met tout de même à jour le weather manager (pour le renderer) (cela n'aura pas d'influence sur la prochaine mise à jour, même si, par exemple, le facteur de production d'énergie solaire est changé)
	weatherManager.update();
}
void EcoWorldSystem::updateSystem(bool moisEcoule, bool anneeEcoulee, bool* outNormalBatimentDestroy, bool* outOldBatimentDestroy)
{
	/*
	Ordre des mises à jour :
	------------------------
		1. Weather Manager
		2. Informations du monde
			2.1. Ressources globalement produites (début)
			2.2. Ages et temps de constructions/destructions des bâtiments
			2.3. Population et énergie
			2.4. Ressources consommées par les habitants
			2.5. Pourcentage disponible de chaque ressource et mise à jour des informations des bâtiments
		3. Bâtiments (mise à jour des informations du monde)
			3.1. Production d'électricité
			3.2. Production d'eau
			3.3. Usines
			3.4. Maisons
			3.5. Routes et autres bâtiments
			3.6. Gestion de l'effet de serre et des déchets
				3.6.1. Bâtiments
				3.6.2. World Infos (taxes)
		4. Ressources globalement produites (fin)
		5. Objectifs
	*/

	// Note : On ne réinitialise pas les valeurs de outNormalBatimentDestroy et de outOldBatimentDestroy au cas où un bâtiment aurait déjà été détruit lors d'une mise à jour précédente

	// Remet les évolutions du monde à zéro pour qu'elles soient recalculées
	infos.resetEvolutions();


	// 1. Met à jour le Weather Manager en mettant aussi à jour les informations du système
	weatherManager.update();


	// 2. Met à jour les informations du monde :

	// 2.1. Conserve en mémoire les ressources actuelles pour pouvoir calculer les ressources globalement produites
	for (u32 i = 0; i < Ressources::RI_COUNT; ++i)
		ressourcesProduites[i] = infos.ressources[i];

	// 2.2. Met à jour les temps de vie des bâtiments, ainsi que leurs temps de construction et de destruction, et détruits les bâtiments nécessaires
	updateBatimentDaysPassed(outNormalBatimentDestroy, outOldBatimentDestroy);

	// 2.3. Met à jour les informations du monde sur la population et l'énergie
	infos.updateDonnees();

	// 2.4. Met à jour les informations du monde sur les ressources consommées par les habitants
	infos.updateRessourcesConsommation();

	// 2.5. Calcule le pourcentage disponible de chaque ressource et met à jour des informations des bâtiments
	calculatePourcentageRessourcesDisponiblesAndUpdateBatimentsInfos();


	// 3. Met à jour le monde d'après tous les bâtiments :

	// 3.1. Met à jour les bâtiments de production d'électricité
	core::list<Batiment*>::Iterator it = listeBatsProductionElectricite.begin();
	const core::list<Batiment*>::Iterator END = listeBatsProductionElectricite.end();
	for (it = listeBatsProductionElectricite.begin(); it != END; ++it)
		(*it)->update(moisEcoule, anneeEcoulee);

	// 3.2. Met à jour les bâtiments de production d'eau
	for (it = listeBatsProductionEau.begin(); it != END; ++it)
		(*it)->update(moisEcoule, anneeEcoulee);

	// 3.3. Met à jour les usines
	for (it = listeBatsUsines.begin(); it != END; ++it)
		(*it)->update(moisEcoule, anneeEcoulee);

	// 3.4. Met à jour les maisons
	for (it = listeBatsMaisons.begin(); it != END; ++it)
		(*it)->update(moisEcoule, anneeEcoulee);

	// 3.5. Met à jour les autres bâtiments
	for (it = listeBatsOther.begin(); it != END; ++it)
		(*it)->update(moisEcoule, anneeEcoulee);

	// 3.6.1. Met à jour les bâtiments gérant l'effet de serre et les déchets
	for (it = listeBatsDechetsES.begin(); it != END; ++it)
		(*it)->update(moisEcoule, anneeEcoulee);

	// 3.6.2. Met à jour les informations du monde sur les taxes de l'effet de serre et des déchets
	infos.updateTaxes();


	// 4. Calcule les ressources globalement produites en comparant les ressources disponibles au début de cette mise à jour et les ressources actuelles
	for (u32 i = 0; i < Ressources::RI_COUNT; ++i)
		ressourcesProduites[i] = infos.ressources[i] - ressourcesProduites[i];


	// 5. Met à jour les objectifs pour cette partie
	objectives.update();
}
void EcoWorldSystem::updateBatimentDaysPassed(bool* outNormalBatimentDestroy, bool* outOldBatimentDestroy)
{
	// Note : On ne réinitialise pas les valeurs de outNormalBatimentDestroy et de outOldBatimentDestroy au cas où un bâtiment aurait déjà été détruit lors d'une mise à jour précédente
	
	// Met à jour l'âge de tous les bâtiments
	const core::list<Batiment*>::ConstIterator END = listeAllBats.end();
	core::list<Batiment*>::ConstIterator it = listeAllBats.begin();
	for (; it != END; ++it)
		(*it)->updateDaysPassed(outNormalBatimentDestroy, outOldBatimentDestroy);

	// Supprime les bâtiments devant être détruits depuis la liste de destruction
	for (it = deleteList.begin(); it != END; ++it)
		deleteBatiment(*it);
	deleteList.clear();
}
void EcoWorldSystem::calculatePourcentageRessourcesDisponiblesAndUpdateBatimentsInfos()
{
	/*
	Problème résolu ici :
	- Plutôt de qu'une usine consomme tout ce qui est actuellement disponible lors de son actualisation
		et qu'ensuite les autres usines ne puissent plus utiliser cette ressource,
		dorénavant lorsqu'une ressource est manquante on calcule sa quantité nécessaire totale
		et on caclule son pourcentage de disponibilité pour que chaque usine en ait la même quantité proportionnellement à ses besoins

	Utilité illustrée :
	- Il reste 500 kg de sable, il y a une usine qui en consomme 250 kg par jour, une autre qui en consomme 750 kg par jour, et 1 jour s'est écoulé :
		le total de sable nécessaire est donc de 250 + 750 = 1000 kg de sable, chaque usine aura donc 500 / 1000 = 50 % de sable disponible,
		la première usine recevra donc 0.5 * 250 = 125 kg de sable, et la seconde recevra 0.5 * 750 = 375 kg de sable

	TODO : Un problème assez subtile est encore présent : Exemple :
	- Une usine de verre demande du sable pour produire du verre, supposons que le sable soit manquant et que son pourcentage de disponibilité soit de 50%.
		Mais, supposons que le verre soit lui aussi manquant et que son pourcentage de disponibilité soit de 80%,
		et qu'il n'existe qu'une seule usine de verre dans le monde.
		Lors du calcul de la disponibilité des ressources, le verre sera donc disponible à 80%, mais lors de l'actualisation de l'usine de verre,
		pour sa production de verre, cette dernière prendra en compte le fait que la disponibilité du sable n'est que de 50%,
		elle produira donc 2 fois moins de verre que prévu au départ, et puisqu'elle est seule, le pourcentage de verre disponble sera en réalité de 40%.
		Ainsi, les autres usines nécessitant du verre fonctionneront avec un pourcentage de disponibilité du verre de 80% alors qu'en réalité il n'est que de 40%,
		et à la fin de la mise à jour, le verre sera donc en quantité négative dans les entrepôts.

		-> La difficulté de ce problème vient du fait qu'il faudrait connaître la disponibilité de ressources qui n'ont pas encore été calculées pour calculer
			la disponibilité des ressources actuelles. Et pour calculer ces dernières, il faut peut-être même calculer la disponibilité de la ressource actuelle !

	=> Pour palier à ce problème, on peut faire quatre calculs à la suite lors de cette fonction :
		- Le premier serait le calcul actuel, normal qui ne se base que sur le pourcentage de production maximal demandé par l'utilisateur.
		- Le deuxième se baserait alors sur le premier, mais prendrait aussi en compte le pourcentage de production de toutes les ressources nécessaires
			à la fabrication de la ressource actuellement calculée (ex : pour du verre, on prendrait en compte le sable, la potasse, le quartz et l'eau)
		- Le troisième calcul se baserait alors sur le deuxième, mais prendrait aussi en compte le pourcentage de production de toutes les ressources nécessaires
			en même temps que celle-ci pour fabriquer une autre ressource (ex : pour la consommation du sable, puisqu'il est nécessaire pour du verre, on prendrait aussi le quartz et l'eau,
			car le pourcentage de production du sable serait limitant pour leur consommation dans cette usine, et inversement)
		- Le quatrième se contenterait alors de recalculer la production/consommation totale de toutes les ressources
			en prenant en compte les facteurs de production précedemment calculés
	===> Grande complexité de l'algorithme, des problèmes peuvent encore subsister (sinon, pourquoi un quatrième calcul recalculant toutes les consommations une fois de plus ?),
		et le temps de calcul nécessaire à cette fonction deviendrait alors énorme !

	=====> Un problème subsisterait encore lorsque deux ressources sont mutuellement nécessaires, mais ce cas-là serait à éviter dans la création des usines !
	*/

	// Tableau permettant de stocker l'évolution des ressources aujourd'hui
	float ressourcesEvolution[Ressources::RI_COUNT] = {0.0f};

	u32 i;	// Variable d'itération

	// Réinitialise les pourcentages de ressources disponibles comme si les stocks de ressources étaient toujours suffisants (sera utilisé plus loin pour réinitialiser la production/consommation de ressources des bâtiments)
	for (i = 0; i < Ressources::RI_COUNT; ++i)
		pourcentageRessourcesDisponibles[i] = 1.0f;



	// Parcours tous les bâtiments et calcule la quantité de chaque ressource utilisée/produite aujourd'hui :
	const core::list<Batiment*>::ConstIterator END = listeAllBats.end();
	core::list<Batiment*>::ConstIterator it = listeAllBats.begin();
	for (; it != END; ++it)
	{
		Batiment* const bat = (*it);
		const BatimentInfos& batInfos = bat->getInfos();
		const u32 ressourcesSize = batInfos.ressourcesJ.size();

		// Réinitialise les informations sur la production/consommation de ressources de ce bâtiment comme si les stocks de ressources étaient toujours suffisants, pour obtenir sa production/consommation réellement demandée
		// On passe le paramêtre firstCall à true pour indiquer que c'est le premier appel de cette fonction ce jour-ci pour ce bâtiment.
		bat->updateBatimentInfos(true, pourcentageRessourcesDisponibles);

		// Vérifie que ce bâtiment produit/consomme bien des ressources, et qu'il est bien actif aujourd'hui (qu'il n'est ni en construction, ni en destruction)
		if (ressourcesSize && !bat->isConstructingOrDestroying())
		{
			// Parcours toutes les ressources produites/consommées par ce bâtiment
			for (i = 0; i < ressourcesSize; ++i)
			{
#ifdef _DEBUG
				// Vérification de débogage :
				// Attention : Les deux tableaux batInfos.ressourcesJ et staticBatInfos.ressourcesJ doivent avoir le même ID de ressource à chaque élément :
				// on doit toujours avoir batInfos.ressourcesJ[i].ID == staticBatInfos.ressourcesJ[i].ID
				if (batInfos.ressourcesJ[i].ID != bat->getStaticInfos().ressourcesJ[i].ID)
					LOG_DEBUG("EcoWorldSystem::calculatePourcentageRessourcesDisponibles() : Les ID des ressources produites/consommees par un batiment ne sont pas identiques entre les informations statiques et les informations variables de ce batiment :" << endl
						<< "    batimentID = " << bat->getID() << " ; batInfos.ressourcesJ[i].ID = " << batInfos.ressourcesJ[i].ID << " ; staticBatInfos.ressourcesJ[i].ID = " << bat->getStaticInfos().ressourcesJ[i].ID << " !", ELL_WARNING);
#endif

				// Ajoute les ressources produites/consommées par ce bâtiment à l'évolution des ressources du monde
				ressourcesEvolution[batInfos.ressourcesJ[i].ID] += batInfos.ressourcesJ[i].production * batInfos.pourcentageProduction;
			}
		}

		// Prend aussi en compte la consommation d'eau des bâtiments, non comprise dans leur consommation de ressources en eau
		ressourcesEvolution[Ressources::RI_eau] -= batInfos.eauConsommationJ;
	}



	// Calcule les pourcentages disponibles de chaque ressource :
	for (i = 0; i < Ressources::RI_COUNT; ++i)
	{
		// Calcule le pourcentage de cette ressource disponible d'après sa consommation/production totale
		// et sa quantité disponible actuellement dans le monde, en restant dans les limites 0.0f et 1.0f
		if (ressourcesEvolution[i] < 0.0f)	// Ressource globalement consommée
		{
			if (infos.ressources[i] > 0.0f)	// Ressource disponible
			{
				if (infos.ressources[i] + ressourcesEvolution[i] < 0.0f)	// Ressource manquante pour aujourd'hui
					pourcentageRessourcesDisponibles[i] = -infos.ressources[i] / ressourcesEvolution[i];
				else	// Ressource encore suffisante pour aujourd'hui
					pourcentageRessourcesDisponibles[i] = 1.0f;
			}
			else	// Ressource complètement indisponible
				pourcentageRessourcesDisponibles[i] = 0.0f;
		}
		else	// Ressource globalement produite
			pourcentageRessourcesDisponibles[i] = 1.0f;
	}



	// Met à jour les informations des bâtiments d'après les pourcentages disponibles de chaque ressource
	// On passe le paramêtre firstCall à false puisque ce n'est pas le premier appel à cette fonction ce jour-ci pour ce bâtiment.
	for (it = listeAllBats.begin(); it != END; ++it)
		(*it)->updateBatimentInfos(false, pourcentageRessourcesDisponibles);
}
