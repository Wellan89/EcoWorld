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
	// Convertit les donn�es de la partie pour qu'elles puissent �tre lues dans cette version du jeu
	void convertGameData();

	// Charge la partie du jeu dans l'instance de Game
	void loadGame();

	// Enregistre la partie actuellement charg�e depuis l'instance de Game
	void saveGame();



	io::IReadFile* file_read;
	io::IWriteFile* file_write;
};

#endif
