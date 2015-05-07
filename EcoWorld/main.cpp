#include <ctime>				// Pour initialiser le g�n�rateur de nombres al�atoires
#include <boost/filesystem.hpp>	// Inclusion du header Boost pour la gestion des dossiers
#include <fstream>				// Pour la gestion des fichiers "� la C++"

#include "global.h"
#include "Game.h"
#include "Batiments.h"
#include "CGUISortTable.h"





// Lien des librairies utilis�es avec le projet :
#ifdef _MSC_VER
#ifdef _DEBUG
#pragma comment(lib, "IrrDebugStatic.lib")				// Librairie de d�bogage d'Irrlicht
#elif defined(NDEBUG_COMPATIBILITY)
#pragma comment(lib, "IrrReleaseStatic.lib")			// Librairie de compatibilit� d'Irrlicht
#else
#pragma comment(lib, "IrrReleaseStaticFastFPU.lib")		// Librairie optimis�e d'Irrlicht
#endif

#pragma comment(linker, "/NODEFAULTLIB:LIBCI")	// Cette librairie pose probl�me lorsqu'Irrlicht est compil� manuellement (probl�me entre VC++ et DirectX 9.0c) : on l'exclue ici

#ifdef USE_IRRKLANG
#pragma comment(lib, "irrKlang.lib")



// Permet le chargement le la DLL d'IrrKlang en diff�r� :
// Permet ainsi � l'application de d�marrer, m�me en l'absence du cette DLL, mais ralentit tr�s l�g�rement tous les appels � IrrKlang (lors du premier appel de chaque fonction seulement)
// Plus d'informations : http://msdn.microsoft.com/fr-fr/library/151kt790.aspx et http://www.codeproject.com/KB/DLL/Delay_Loading_Dll.aspx
// (Voir le code source du chargement diff�r� de Visual C++ dans : C:\Program Files\Microsoft Visual Studio 10.0\VC\include\delayhlp.cpp)
//#define IRRKLANG_DELAYED_DLL_LOADING					// Attention : Ce define doit �tre d�fini dans les options du projet !

#ifdef IRRKLANG_DELAYED_DLL_LOADING
#pragma comment(lib, "delayimp")
//#pragma comment(linker, "/DELAYLOAD:irrKlang.dll")	// Attention : cette option du linker doit �tre d�finie dans les options du projet (elle ne fonctionne pas avec les directives #pragma) (voir : http://www.pcreview.co.uk/forums/delayload-pragma-fixed-whidbey-t1430386.html et http://msdn.microsoft.com/en-us/library/7f0aews7.aspx)
#endif
#endif

#ifdef USE_RAKNET
// Ici, on utilise la version statique de RakNet : aucune dll n�cessaire
#pragma comment(lib, "ws2_32.lib")
#ifndef _WIN64
#ifdef _DEBUG
#pragma comment(lib, "RakNetLibStaticDebug_x86.lib")			// Librairie de d�bogage de RakNet (32 bits)
#elif defined(NDEBUG_COMPATIBILITY)
#pragma comment(lib, "RakNetLibStatic_x86 (Compatibility).lib")	// Librairie de compatibilit� de RakNet (32 bits)
#else
#pragma comment(lib, "RakNetLibStatic_x86.lib")					// Librairie optimis�e de RakNet (32 bits)
#endif
#else
#ifdef _DEBUG
#pragma comment(lib, "RakNetLibStaticDebug_x64.lib")			// Librairie de d�bogage de RakNet (64 bits)
#elif defined(NDEBUG_COMPATIBILITY)
#pragma comment(lib, "RakNetLibStatic_x64 (Compatibility).lib")	// Librairie de compatibilit� de RakNet (64 bits)
#else
#pragma comment(lib, "RakNetLibStatic_x64.lib")					// Librairie optimis�e de RakNet (64 bits)
#endif
#endif
#endif
#endif





// Initialise les informations statiques des b�timents de StaticBatimentInfos avec leur constructeur par d�faut
StaticBatimentInfos StaticBatimentInfos::batimentInfos[BI_COUNT] = { };
u32 StaticBatimentInfos::tailleMaxBats = 1;

// Initialise le tableau des types des tris avec leurs valeurs respectives
const CGUISortTable::E_TRI_TYPES CGUISortTable::triTypes[CGUISortTable::ETT_COUNT] = {
	(CGUISortTable::E_TRI_TYPES)0,
	(CGUISortTable::E_TRI_TYPES)1,
	(CGUISortTable::E_TRI_TYPES)2,
	(CGUISortTable::E_TRI_TYPES)3 };





// Permet la compilation en mode RELEASE sous Windows sans afficher la console (D�sactiv� en mode "Release (Compatibility)")
// Voir ici : http://msdn.microsoft.com/en-us/library/aa383745.aspx et ici : http://msdn.microsoft.com/en-us/library/6sehtctf.aspx
#if !defined(_DEBUG) && defined(_IRR_WINDOWS_) && !defined(NDEBUG_COMPATIBILITY)

#define NO_CONSOLE				// Permet d'indiquer que la console est d�sactiv�e
//#define USE_COUT_REDIRECTION	// Permet d'utiliser un fichier de redirection pour le flux de sortie standard cout

// Indique au linker qu'on veut cr�er une application windows
#ifdef _MSC_VER
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

#endif





// D�finitions de fonctions fr�quemment utilis�e pour la cr�ation de la GUI d'Irrlicht (ces fonctions sont d�clar�es dans global.h) :
inline core::recti getAbsoluteRectangle(float x, float y, float x2, float y2, const core::dimension2du& parentSize)
{
	return core::recti((int)(x * parentSize.Width), (int)(y * parentSize.Height),
		(int)(x2 * parentSize.Width), (int)(y2 * parentSize.Height));
}
inline core::recti getAbsoluteRectangle(float x, float y, float x2, float y2, const core::recti& parentRect)
{
	return getAbsoluteRectangle(x, y, x2, y2, core::dimension2du(
			(u32)(abs(parentRect.LowerRightCorner.X - parentRect.UpperLeftCorner.X)),
			(u32)(abs(parentRect.LowerRightCorner.Y - parentRect.UpperLeftCorner.Y))));
}
/*inline core::recti getAbsoluteRectangle(float x, float y, float x2, float y2)
{
	return getAbsoluteRectangle(x, y, x2, y2, driver->getScreenSize());	// D�sactiv�e car le driver est ici inaccessible !
}*/





// Cr�e l'architecture des dossiers n�cessaire pour le jeu
void createDirectories();

// Cr�e le dossier sp�cifi� et g�re les erreurs �ventuelles lors de sa cr�ation
//void createDirectory(const char* directoryPath);	// Maintenant d�fini dans global.h, car utilis� � divers endroits du programmes

#ifdef USE_COUT_REDIRECTION
// Supprime un fichier s'il n'est pas utilis� : si sa taille est nulle
inline void removeFileIfUnused(const filesystem3::path& p);
#endif





int main(int argc, char* argv[])
{
#ifdef USE_COUT_REDIRECTION
	// Pointeur sur le fichier de redirection du flux de sortie standard
	ofstream* cout_file = NULL;
#endif



	// Grand bloc try catch englobant la partie principale du programme pour informer l'utilisateur des erreurs
	int returnValue = EXIT_SUCCESS;
#ifndef _DEBUG
	try
#endif
	{
		// Initialise le g�n�rateur de nombres al�atoires
		srand((unsigned int)time(NULL));



		// V�rifie les arguments pour indiquer les param�tres du jeu
		gameState.parseMainArguments(argc, argv);

		// Charge tous les param�tres depuis le fichier de configuration
		gameState.loadGameConfigFromFile();

#ifdef USE_COUT_REDIRECTION
		// V�rifie qu'on peut �crire sur le disque dur de l'ordinateur cible
		if (CAN_WRITE_ON_DISK)
		{
			// Redirige le flux de sortie standard cout vers "stdout.txt" (http://www.developpez.net/forums/d154987/c-cpp/cpp/rediriger-cerr-vers-fichier/)
			cout_file = new ofstream("stdout.txt", ios::out | ios::trunc);	// Les nouvelles donn�es remplacent le fichier existant
			if (cout_file && cout_file->is_open())
				cout.rdbuf(cout_file->rdbuf());
		}
#endif

		// Cr�e l'architecture des dossiers n�cessaires au jeu, si on peut �crire sur le disque dur de l'ordinateur cible
		createDirectories();



#if defined(_DEBUG) && 0
		// Tests de v�rification sur la taille m�moire de la carte
		{
			const long T_CARTE = TAILLE_CARTE;
			const long T_CARTE_SQR = T_CARTE * T_CARTE;

			LOG_DEBUG("Taille de la carte du jeu : " << T_CARTE << " (valeur max d'un int : " << INT_MAX << ")", ELL_INFORMATION);
			if (T_CARTE > INT_MAX)
				LOG_DEBUG("ATTENTION : TAILLE_CARTE > INT_MAX : " << T_CARTE << " > " << INT_MAX << " !" , ELL_INFORMATION);
			LOG_DEBUG("Surface de la carte du jeu : " << T_CARTE_SQR  << " (valeur max d'un int : " << INT_MAX << ")", ELL_INFORMATION);
			if (T_CARTE_SQR > INT_MAX)
				LOG_DEBUG("ATTENTION : TAILLE_CARTE * TAILLE_CARTE > INT_MAX : " << T_CARTE_SQR << " > " << INT_MAX << " !", ELL_INFORMATION);
			LOG_DEBUG("Taille memoire de la carte du jeu : " << sizeof(TerrainInfo) * T_CARTE_SQR << " octets (" << sizeof(TerrainInfo) * (double)T_CARTE_SQR / 1024.0 << " Ko ) (Taille de TerrainInfo : " << sizeof(TerrainInfo) << ")" << endl, ELL_INFORMATION);
		}
#endif



		// Boucle de red�marrage du jeu
		do
		{
			// Cr�e le syst�me de jeu global puis le d�marre
			game = new Game();
			if (game)
			{
				game->run();
				delete game;
				game = NULL;
			}
		} while(gameState.restart);



		// Enregistre la configuration finale du jeu
		gameState.saveGameConfigToFile();
	}
#ifndef _DEBUG
	catch (const std::exception& e)	// Exception standard
	{
		// Affiche l'erreur dans le logger
		returnValue = EXIT_FAILURE;
		LOG(endl << "ERREUR FATALE : Exception standard lanc�e : " << e.what() << endl, ELL_ERROR);
	}
	catch (...)
	{
		// Une erreur compl�tement inconnue s'est produite, on en informe tout de m�me l'utilisateur
		returnValue = EXIT_FAILURE;
		LOG(endl << "ERREUR FATALE : Exception non standard lanc�e : Une erreur inconnue s'est produite !" << endl, ELL_ERROR);
	}
#endif



#ifdef USE_COUT_REDIRECTION
	// Ferme le fichier de redirection pour cout
	if (cout_file)
	{
		if (cout_file->is_open())
			cout_file->close();
		delete cout_file;
		cout_file = NULL;
	}

	// Supprime le fichier de redirection s'il n'a pas �t� utilis�, si on peut �crire sur le disque dur de l'ordinateur cible
	removeFileIfUnused("stdout.txt");
#endif

#if defined(_DEBUG) && 0
	::system("PAUSE");
#endif

	return returnValue;
}
void createDirectories()
{
	// V�rifie qu'on peut �crire sur le disque dur de l'ordinateur cible
	if (!CAN_WRITE_ON_DISK)
		return;

	// Cr�e les dossiers du jeu :
	// Dossiers des donn�es d�sactiv�s, car non r�ellement n�cessaires (les dossiers cr��s ici sont cr��s pour permettre l'enregistrement de fichiers dans ces dossiers)
	//createDirectory("data");
	//createDirectory("data/Audio");
	//createDirectory("data/Audio/Musics");
	//createDirectory("data/Audio/Sounds");
	//createDirectory("data/Models");
	//createDirectory("data/Shaders");
	//createDirectory("data/Terrains");
	//createDirectory("data/Textures");
	//createDirectory("data/Textures/Icons");
	//createDirectory("data/Textures/Particles");
	//createDirectory("data/Textures/Sky");
	//createDirectory("data/Textures/Terrain");
	//createDirectory("data/Textures/Trees");
	createDirectory("Saves");				// Ce dossier est n�cessaire pour pouvoir d�finir le chemin par d�faut des fen�tres d'enregistrement et de chargement de fichiers
	//createDirectory("Saves/AutoSaves");	// Ce dossier sera cr�� lorsqu'il sera n�cessaire
	//createDirectory("Screenshots");		// Ce dossier sera cr�� lorsqu'il sera n�cessaire
}
void createDirectory(const char* directoryPath)
{
	const boost::filesystem::path p(directoryPath);

	// Le code d'erreur qui indiquera si une erreur s'est produite lors de la cr�ation du dossier
	boost::system::error_code errorCode;

	if (boost::filesystem::create_directory(p, errorCode))
	{
		// Dossier cr��
		LOG("Created directory : " << p.string().c_str(), ELL_INFORMATION);
	}
	else if (!boost::filesystem::is_directory(p))
	{
		// Dossier non cr�� et non existant pour une raison inconnue
		LOG("Could not create directory : " << p.string().c_str() << endl
			<< "     Error code : " << errorCode, ELL_WARNING);
	}
	/*else
	{
		// Dossier non cr�� car d�j� existant
		LOG("Directory already exists : " << p.string().c_str(), ELL_INFORMATION);
	}*/
}
#ifdef USE_COUT_REDIRECTION
inline void removeFileIfUnused(const filesystem3::path& p)
{
	// V�rifie qu'on peut �crire sur le disque dur de l'ordinateur cible
	if (!CAN_WRITE_ON_DISK)
		return;

	// Le code d'erreur qui indiquera si une erreur s'est produite (mais ce code ne sera jamais utilis�, il �vite seulement que les fonctions de Boost lancent des exceptions)
	boost::system::error_code errorCode;

	// V�rifie que le fichier sp�cifi� est vide, puis le supprime
	if (boost::filesystem::is_empty(p, errorCode))
		boost::filesystem::remove(p, errorCode);
}
#endif
