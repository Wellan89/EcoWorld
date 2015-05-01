#include "SaveConverter.h"
#include "Game.h"
#include "EcoWorldRenderer.h"
#include "RTSCamera.h"
#include "XEffects/EffectHandler.h"

SaveConverter::SaveConverter(const io::path& filename)
	: file_read(0), file_write(0)
{
	// Désactive les messages de log non demandés par ce module dans le logger, et évite l'affichage d'une fenêtre d'Irrlicht en modifiant directement la configuration du jeu
	gameConfig.deviceParams.DriverType = video::EDT_NULL;
	gameConfig.deviceParams.LoggingLevel = ELL_NONE;



	// Crée le jeu réel
	LOG("Loading game data...", ELL_NONE);
	game = new Game();
	game->init();



	// Convertit cette partie pour qu'elle puisse être chargée par cette version du jeu
	LOG("Converting saved game data...", ELL_NONE);
	file_read = game->fileSystem->createAndOpenFile(filename);
	if (!file_read)
		return;
	io::path filenameWrite;
	core::cutFilenameExtension(filenameWrite, filename);
	filenameWrite.append("_converted.ewg");
	file_write = game->fileSystem->createAndWriteFile(filenameWrite);
	if (!file_write)
		return;
	convertGameData();
	file_write->drop();
	file_write = NULL;

	// Charge cette partie
	LOG("Loading converted saved game...", ELL_NONE);
	file_read->drop();
	file_read = game->fileSystem->createAndOpenFile(filenameWrite);
	if (!file_read)
		return;
	loadGame();
	file_read->drop();
	file_read = NULL;

	// Enregistre la nouvelle partie chargée
	LOG("Saving game...", ELL_NONE);
	core::cutFilenameExtension(filenameWrite, filename);
	filenameWrite.append("_saved.ewg");
	file_write = game->fileSystem->createAndWriteFile(filenameWrite);
	if (!file_write)
		return;
	saveGame();
	file_write->drop();
	file_write = NULL;



	// Libère les ressources utilisées
	if (game->cameraRTS)
		game->cameraRTS->drop();
	if (game->xEffects)
		delete game->xEffects;
	if (game->guiManager)
		delete game->guiManager;
	if (game->renderer)
		delete game->renderer;
	game->device->closeDevice();
	game->device->drop();
	delete game;
}
void SaveConverter::convertGameData()
{
	// Crée les données pour le fichier
	io::IXMLReader* reader = game->fileSystem->createXMLReader(file_read);
	if (!reader)
		return;

	io::IXMLWriter* writer = game->fileSystem->createXMLWriter(file_write);
	if (!writer)
	{
		reader->drop();
		return;
	}

	io::IAttributes* att = game->fileSystem->createEmptyAttributes(game->driver);
	if (!att)
	{
		reader->drop();
		writer->drop();
		return;
	}
	writer->writeXMLHeader();



	bool cont = reader->read();
	while (cont && reader->getNodeType() != EXN_ELEMENT)	cont = reader->read();
	if (cont)
	{
		core::stringw name = reader->getNodeName();
		while (att->read(reader, true, name.c_str()) && cont)
		{
			bool saveAtt = true;

			const u32 attributeCount = att->getAttributeCount();
			u32 i = 0;
			for (u32 i = 0; i < attributeCount; ++i)
			{
				const core::stringc attName = att->getAttributeName(i);
				const E_ATTRIBUTE_TYPE attType = att->getAttributeType(i);



				// Remplacement des anciens attributs des bâtiments (ils ne sont pas supprimés maintenant, seul leur nouvel équivalent est ajouté) :
				if (name.equalsn(L"Batiment", 8))
				{
					name = L"Batiment";	// Résolution du changement des noms de bloc des bâtiments <Batiment123> en <Batiment>
					if (attType == EAT_VECTOR3D)
					{
						const core::vector3df attValue = att->getAttributeAsVector3d(i);
						if (attName == "Position")
							att->addPosition2d("Index", game->system.getIndexFromPosition(attValue));
						else if (attName == "Rotation")
							att->addFloat("Rotation", attValue.Y);
					}
					else if (attType == EAT_INT)
					{
						const int attValue = att->getAttributeAsInt(i);
						if (attName == "ID")
							att->addString("Name", StaticBatimentInfos::getInfos((BatimentID)attValue).name);
					}
				}
			}

			if (saveAtt)
				att->write(writer, false, name.c_str());

			att->clear();

			while (cont && reader->getNodeType() != EXN_ELEMENT)	cont = reader->read();
			if (cont)
				name = reader->getNodeName();
		}
	}



	reader->drop();
	writer->drop();
	att->drop();
}
void SaveConverter::loadGame()
{
	bool tmpB[2];
	core::stringc terrainFilename;
	game->loadSavedGame_Eff(file_read, tmpB[0], tmpB[1], terrainFilename);

	// Indique le nom du terrain de la sauvegarde
	const EcoWorldTerrainManager::TerrainInfos& terrainInfosConst = game->renderer->getTerrainManager().getTerrainInfos();
	EcoWorldTerrainManager::TerrainInfos& terrainInfos = const_cast<EcoWorldTerrainManager::TerrainInfos&>(terrainInfosConst);
	terrainInfos.terrainName = terrainFilename;
}
void SaveConverter::saveGame()
{
	game->saveCurrentGame_Eff(file_write);
}
