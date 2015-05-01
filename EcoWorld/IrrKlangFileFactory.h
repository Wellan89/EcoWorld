#ifndef IRRKLANG_FILE_FACTORY
#define IRRKLANG_FILE_FACTORY

#include "global.h"

#ifdef USE_IRRKLANG

#include "Game.h"
#include <irrklang.h>

using namespace irrklang;

// Permet d'ouvrir des fichiers en lecture pour IrrKlang en utilisant le file system d'Irrlicht, permettant ainsi la lecture de fichiers compressés
class CIrrKlangFileFactory : public IFileFactory
{
protected:
	// La classe personnalisée permettant de lire les fichiers grâce à Irrlicht
	class CIrrKlangFileReader : public IFileReader
	{
	protected:
		// Le fichier d'Irrlicht qui nous permet de renvoyer les informations à IrrKlang
		io::IReadFile* irrFile;

	public:
		// Constructeur et destructeur
		CIrrKlangFileReader(io::IReadFile* soundFile) : irrFile(soundFile)	{ }
		~CIrrKlangFileReader()	{ irrFile->drop(); }

		// Fonctions nécessaires à IrrKlang
		ik_s32 read(void* buffer, ik_u32 sizeToRead)		{ return irrFile->read(buffer,sizeToRead); }
		bool seek(ik_s32 finalPos, bool relativeMovement)	{ return irrFile->seek(finalPos, relativeMovement); }
		ik_s32 getSize()									{ return irrFile->getSize(); }
		ik_s32 getPos()										{ return irrFile->getPos(); }
		const ik_c8* getFileName()							{ return irrFile->getFileName().c_str(); }
	};

public:
	// Constructeur et destructeur
	CIrrKlangFileFactory()	{ }
	~CIrrKlangFileFactory()	{ }

	// Ouvre un fichier en lecture pour IrrKlang avec le file system d'Irrlicht
	virtual IFileReader* createFileReader(const ik_c8* filename)
	{
		if (!game)
			return NULL;

		io::IReadFile* const irrFile = game->fileSystem->createAndOpenFile(filename);
		if (!irrFile)
		{
			LOG("IkMgr: File factory : Could not open file : " << filename, ELL_ERROR);
			return NULL;
		}

		return (new CIrrKlangFileReader(irrFile));
	}
};

#endif
#endif
