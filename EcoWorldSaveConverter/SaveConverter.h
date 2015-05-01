#ifndef DEF_SAVE_CONVERTER
#define DEF_SAVE_CONVERTER

#include "global.h"

class EcoWorldSystem;

class SaveConverter
{
public:
	// Constructeur
	SaveConverter(const io::path& filename);

protected:
	// Convertit les données de la partie pour qu'elles puissent être lues dans cette version du jeu
	void convertGameData();

	// Charge la partie du jeu dans l'instance de Game
	void loadGame();

	// Enregistre la partie actuellement chargée depuis l'instance de Game
	void saveGame();



	io::IReadFile* file_read;
	io::IWriteFile* file_write;
};

#endif
