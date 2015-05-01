#include "global.h"
#include "Game.h"
#include "GameConfiguration.h"
#ifdef USE_IRRKLANG
#include "IrrKlangManager.h"
#endif
#ifdef USE_RAKNET
#include "RakNetManager.h"
#endif



// Définitions globales des instances :
GameState gameState;
Game* game = NULL;
StaticString staticString;
EcoWorldLogger ecoWorldLogger;



// Implémentation de la classe GameState :
GameState::GameState() : restart(false), restartToOptionsMenu(false), optionsMenuTab(-1), showOptionsConfirmationWindow(false), lastWeather(-1), lastDeviceTime(0), keepIrrKlang(false), saveLastGameConfig(false),
#ifdef USE_IRRKLANG
	irrKlangManager(new IrrKlangManager()),
#endif
#ifdef USE_RAKNET
	rakNetManager(new RakNetManager()),
#endif
	gameConfiguration(new GameConfiguration()), lastGameConfiguration(new GameConfiguration())
{
}
GameState::~GameState()
{
	delete gameConfiguration;
	delete lastGameConfiguration;
#ifdef USE_IRRKLANG
	delete irrKlangManager;
#endif
#ifdef USE_RAKNET
	delete rakNetManager;
#endif
}
void GameState::loadGameConfigFromFile()
{
	// Crée un NULL DEVICE pour avoir accès à IFileSystem, nécéssaire pour charger les données de la configuration
	SIrrlichtCreationParameters params;
	params.DriverType = video::EDT_NULL;
	params.LoggingLevel = ELL_WARNING;
	IrrlichtDevice* device = createDeviceEx(params);
	if (!device)
	{
		gameConfiguration->resetDefaultValues();
		return;
	}

	// Ajoute le dossier de l'executable au système de fichiers du device, au cas où le fichier de configuration se trouverait dans un de ses sous-dossiers
	io::IFileSystem* const fileSystem = device->getFileSystem();
	if (workingDirectory.size())	// Indique le dossier de travail par défaut du device
		fileSystem->changeWorkingDirectoryTo(workingDirectory);
	fileSystem->addFileArchive("./", true, true, io::EFAT_FOLDER);

	gameConfiguration->load(GAME_CONFIGURATION_FILENAME, device->getFileSystem());

	device->drop();
	device = NULL;
}
void GameState::saveGameConfigToFile()
{
	// Vérifie qu'on peut écrire sur le disque dur de l'ordinateur cible
	if (!CAN_WRITE_ON_DISK)
	{
		LOG("ERREUR : Impossible d'enregistrer les options de configuration : Les droits d'écriture sur le disque ont été refusés !", ELL_ERROR);
		return;
	}

	// Crée un NULL DEVICE pour avoir accès à IFileSystem, nécéssaire pour charger les données de la configuration
	SIrrlichtCreationParameters params;
	params.DriverType = video::EDT_NULL;
	params.LoggingLevel = ELL_WARNING;
	IrrlichtDevice* device = createDeviceEx(params);
	if (!device)
		return;

	// Indique le dossier de travail par défaut du device
	io::IFileSystem* const fileSystem = device->getFileSystem();
	if (workingDirectory.size())
		fileSystem->changeWorkingDirectoryTo(workingDirectory);

	// Vérifie quelle configuration du jeu on doit enregistrer
	if (saveLastGameConfig)
		lastGameConfiguration->save(GAME_CONFIGURATION_FILENAME, fileSystem);
	else
		gameConfiguration->save(GAME_CONFIGURATION_FILENAME, fileSystem);

	device->drop();
	device = NULL;
}
void GameState::parseMainArguments(int argc, char* argv[])
{
	// Indique les paramêtres par défaut
	bool canWriteOnDisk = true, canChangeWorkingDir = true;
	fileToOpen = "";

	// Vérifie que les paramêtres envoyés sont valides
	if (argv && argc > 0)
	{
		// Parcours tous les arguments
		const core::stringc noWriteStr("/nowrite");							// Chaîne de caractère permettant de comparer l'argument "/NoWrite"
		const core::stringc noChangeWorkingDirStr("/nochangeworkingdir");	// Chaîne de caractère permettant de comparer l'argument "/NoChangeWorkingDir"
#ifdef _DEBUG
		for (int i = 1; i < argc; ++i)	// Désactive le premier argument en mode DEBUG car VC++ indique automatiquement le dossier de travail de l'application
#else
		for (int i = 0; i < argc; ++i)
#endif
		{
			// Obtient le texte de cet argument et le transforme en minuscules
			core::stringc arg(argv[i]);
			arg.make_lower();

#ifndef _DEBUG	// Désactivé en mode DEBUG car VC++ indique automatiquement le dossier de travail de l'application
			// Gère le premier argument, qui est l'adresse de cet exécutable
			if (i == 0)
			{
				// Indique le dossier de travail de cet exécutable, pour éviter le bug suivant :
				// - BUG : Ouverture de fichiers par double-clic dans l'explorateur : le dossier de travail de l'application change => Fichiers de données non trouvés

				// Obtient le dossier réel où se trouve cet exécutable
				const int lastSlashPos = arg.findLastChar("\\/", 2);
				if (lastSlashPos >= 0)
					workingDirectory = arg.subString(0, lastSlashPos);
			}

			// Vérifie si cet argument est le paramêtre de non-écriture sur le disque dur
			else if (arg == noWriteStr)
#else
			// Vérifie si cet argument est le paramêtre de non-écriture sur le disque dur
			if (arg == noWriteStr)
#endif
			{
				// Indique qu'on ne pourra pas écrire sur le disque dur de l'ordinateur cible
				canWriteOnDisk = false;
			}

			// Vérifie si cet argument est le paramêtre de non-changement du dossier de travail
			else if (arg == noChangeWorkingDirStr)
			{
				// Indique qu'on ne pourra pas changer de dossier de travail
				canChangeWorkingDir = false;
			}

			// Sinon, on suppose que cet argument est un chemin de fichier
			else if (!fileToOpen.size())	// Vérifie que le fichier à ouvrir n'est actuellement pas renseigné
			{
				// Envoie directement cet argument à Game, qui déterminera si le fichier peut réellement être ouvert ou non
				fileToOpen = arg;
			}
		}
	}

	// Indique au logger si on peut écrire sur le disque dur de l'ordinateur cible
	ecoWorldLogger.canWriteOnDisk = canWriteOnDisk;

	// Efface l'adresse du dossier de travail prévu si on ne peut pas le changer
	if (!canChangeWorkingDir)
		workingDirectory = "";

	// Affiche le dossier de travail de l'application dans le logger
	LOG("Working directory : " << workingDirectory.c_str(), ELL_INFORMATION);

	// Affiche les arguments dans le logger
	LOG(endl << "Nombre d'arguments : " << argc, ELL_INFORMATION);
	for (int i = 0; i < argc; ++i)
		LOG("Argument " << i << " : " << argv[i], ELL_INFORMATION);
	LOG("", ELL_INFORMATION);
}



// Implémentation de la classe EcoWorldLogger :
bool EcoWorldLogger::verifyLogLevel(ELOG_LEVEL logLevel)
{
	// Vérifie que le niveau de log actuel est bien supérieur ou égal au niveau de log minimal
	return (logLevel >= gameConfig.deviceParams.LoggingLevel);
}
