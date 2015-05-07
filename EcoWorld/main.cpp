#include <ctime>				// Pour initialiser le générateur de nombres aléatoires
#include <boost/filesystem.hpp>	// Inclusion du header Boost pour la gestion des dossiers
#include <fstream>				// Pour la gestion des fichiers "à la C++"

#include "global.h"
#include "Game.h"
#include "Batiments.h"
#include "CGUISortTable.h"





// Lien des librairies utilisées avec le projet :
#ifdef _MSC_VER
#ifdef _DEBUG
#pragma comment(lib, "IrrDebugStatic.lib")				// Librairie de débogage d'Irrlicht
#elif defined(NDEBUG_COMPATIBILITY)
#pragma comment(lib, "IrrReleaseStatic.lib")			// Librairie de compatibilité d'Irrlicht
#else
#pragma comment(lib, "IrrReleaseStaticFastFPU.lib")		// Librairie optimisée d'Irrlicht
#endif

#pragma comment(linker, "/NODEFAULTLIB:LIBCI")	// Cette librairie pose problème lorsqu'Irrlicht est compilé manuellement (problème entre VC++ et DirectX 9.0c) : on l'exclue ici

#ifdef USE_IRRKLANG
#pragma comment(lib, "irrKlang.lib")



// Permet le chargement le la DLL d'IrrKlang en différé :
// Permet ainsi à l'application de démarrer, même en l'absence du cette DLL, mais ralentit très légèrement tous les appels à IrrKlang (lors du premier appel de chaque fonction seulement)
// Plus d'informations : http://msdn.microsoft.com/fr-fr/library/151kt790.aspx et http://www.codeproject.com/KB/DLL/Delay_Loading_Dll.aspx
// (Voir le code source du chargement différé de Visual C++ dans : C:\Program Files\Microsoft Visual Studio 10.0\VC\include\delayhlp.cpp)
//#define IRRKLANG_DELAYED_DLL_LOADING					// Attention : Ce define doit être défini dans les options du projet !

#ifdef IRRKLANG_DELAYED_DLL_LOADING
#pragma comment(lib, "delayimp")
//#pragma comment(linker, "/DELAYLOAD:irrKlang.dll")	// Attention : cette option du linker doit être définie dans les options du projet (elle ne fonctionne pas avec les directives #pragma) (voir : http://www.pcreview.co.uk/forums/delayload-pragma-fixed-whidbey-t1430386.html et http://msdn.microsoft.com/en-us/library/7f0aews7.aspx)
#endif
#endif

#ifdef USE_RAKNET
// Ici, on utilise la version statique de RakNet : aucune dll nécessaire
#pragma comment(lib, "ws2_32.lib")
#ifndef _WIN64
#ifdef _DEBUG
#pragma comment(lib, "RakNetLibStaticDebug_x86.lib")			// Librairie de débogage de RakNet (32 bits)
#elif defined(NDEBUG_COMPATIBILITY)
#pragma comment(lib, "RakNetLibStatic_x86 (Compatibility).lib")	// Librairie de compatibilité de RakNet (32 bits)
#else
#pragma comment(lib, "RakNetLibStatic_x86.lib")					// Librairie optimisée de RakNet (32 bits)
#endif
#else
#ifdef _DEBUG
#pragma comment(lib, "RakNetLibStaticDebug_x64.lib")			// Librairie de débogage de RakNet (64 bits)
#elif defined(NDEBUG_COMPATIBILITY)
#pragma comment(lib, "RakNetLibStatic_x64 (Compatibility).lib")	// Librairie de compatibilité de RakNet (64 bits)
#else
#pragma comment(lib, "RakNetLibStatic_x64.lib")					// Librairie optimisée de RakNet (64 bits)
#endif
#endif
#endif
#endif





// Initialise les informations statiques des bâtiments de StaticBatimentInfos avec leur constructeur par défaut
StaticBatimentInfos StaticBatimentInfos::batimentInfos[BI_COUNT] = { };
u32 StaticBatimentInfos::tailleMaxBats = 1;

// Initialise le tableau des types des tris avec leurs valeurs respectives
const CGUISortTable::E_TRI_TYPES CGUISortTable::triTypes[CGUISortTable::ETT_COUNT] = {
	(CGUISortTable::E_TRI_TYPES)0,
	(CGUISortTable::E_TRI_TYPES)1,
	(CGUISortTable::E_TRI_TYPES)2,
	(CGUISortTable::E_TRI_TYPES)3 };





// Permet la compilation en mode RELEASE sous Windows sans afficher la console (Désactivé en mode "Release (Compatibility)")
// Voir ici : http://msdn.microsoft.com/en-us/library/aa383745.aspx et ici : http://msdn.microsoft.com/en-us/library/6sehtctf.aspx
#if !defined(_DEBUG) && defined(_IRR_WINDOWS_) && !defined(NDEBUG_COMPATIBILITY)

#define NO_CONSOLE				// Permet d'indiquer que la console est désactivée
//#define USE_COUT_REDIRECTION	// Permet d'utiliser un fichier de redirection pour le flux de sortie standard cout

// Indique au linker qu'on veut créer une application windows
#ifdef _MSC_VER
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

#endif





// Définitions de fonctions fréquemment utilisée pour la création de la GUI d'Irrlicht (ces fonctions sont déclarées dans global.h) :
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
	return getAbsoluteRectangle(x, y, x2, y2, driver->getScreenSize());	// Désactivée car le driver est ici inaccessible !
}*/





// Crée l'architecture des dossiers nécessaire pour le jeu
void createDirectories();

// Crée le dossier spécifié et gère les erreurs éventuelles lors de sa création
//void createDirectory(const char* directoryPath);	// Maintenant défini dans global.h, car utilisé à divers endroits du programmes

#ifdef USE_COUT_REDIRECTION
// Supprime un fichier s'il n'est pas utilisé : si sa taille est nulle
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
		// Initialise le générateur de nombres aléatoires
		srand((unsigned int)time(NULL));



		// Vérifie les arguments pour indiquer les paramêtres du jeu
		gameState.parseMainArguments(argc, argv);

		// Charge tous les paramêtres depuis le fichier de configuration
		gameState.loadGameConfigFromFile();

#ifdef USE_COUT_REDIRECTION
		// Vérifie qu'on peut écrire sur le disque dur de l'ordinateur cible
		if (CAN_WRITE_ON_DISK)
		{
			// Redirige le flux de sortie standard cout vers "stdout.txt" (http://www.developpez.net/forums/d154987/c-cpp/cpp/rediriger-cerr-vers-fichier/)
			cout_file = new ofstream("stdout.txt", ios::out | ios::trunc);	// Les nouvelles données remplacent le fichier existant
			if (cout_file && cout_file->is_open())
				cout.rdbuf(cout_file->rdbuf());
		}
#endif

		// Crée l'architecture des dossiers nécessaires au jeu, si on peut écrire sur le disque dur de l'ordinateur cible
		createDirectories();



#if defined(_DEBUG) && 0
		// Tests de vérification sur la taille mémoire de la carte
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



		// Boucle de redémarrage du jeu
		do
		{
			// Crée le système de jeu global puis le démarre
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
		LOG(endl << "ERREUR FATALE : Exception standard lancée : " << e.what() << endl, ELL_ERROR);
	}
	catch (...)
	{
		// Une erreur complètement inconnue s'est produite, on en informe tout de même l'utilisateur
		returnValue = EXIT_FAILURE;
		LOG(endl << "ERREUR FATALE : Exception non standard lancée : Une erreur inconnue s'est produite !" << endl, ELL_ERROR);
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

	// Supprime le fichier de redirection s'il n'a pas été utilisé, si on peut écrire sur le disque dur de l'ordinateur cible
	removeFileIfUnused("stdout.txt");
#endif

#if defined(_DEBUG) && 0
	::system("PAUSE");
#endif

	return returnValue;
}
void createDirectories()
{
	// Vérifie qu'on peut écrire sur le disque dur de l'ordinateur cible
	if (!CAN_WRITE_ON_DISK)
		return;

	// Crée les dossiers du jeu :
	// Dossiers des données désactivés, car non réellement nécessaires (les dossiers créés ici sont créés pour permettre l'enregistrement de fichiers dans ces dossiers)
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
	createDirectory("Saves");				// Ce dossier est nécessaire pour pouvoir définir le chemin par défaut des fenêtres d'enregistrement et de chargement de fichiers
	//createDirectory("Saves/AutoSaves");	// Ce dossier sera créé lorsqu'il sera nécessaire
	//createDirectory("Screenshots");		// Ce dossier sera créé lorsqu'il sera nécessaire
}
void createDirectory(const char* directoryPath)
{
	const boost::filesystem::path p(directoryPath);

	// Le code d'erreur qui indiquera si une erreur s'est produite lors de la création du dossier
	boost::system::error_code errorCode;

	if (boost::filesystem::create_directory(p, errorCode))
	{
		// Dossier créé
		LOG("Created directory : " << p.string().c_str(), ELL_INFORMATION);
	}
	else if (!boost::filesystem::is_directory(p))
	{
		// Dossier non créé et non existant pour une raison inconnue
		LOG("Could not create directory : " << p.string().c_str() << endl
			<< "     Error code : " << errorCode, ELL_WARNING);
	}
	/*else
	{
		// Dossier non créé car déjà existant
		LOG("Directory already exists : " << p.string().c_str(), ELL_INFORMATION);
	}*/
}
#ifdef USE_COUT_REDIRECTION
inline void removeFileIfUnused(const filesystem3::path& p)
{
	// Vérifie qu'on peut écrire sur le disque dur de l'ordinateur cible
	if (!CAN_WRITE_ON_DISK)
		return;

	// Le code d'erreur qui indiquera si une erreur s'est produite (mais ce code ne sera jamais utilisé, il évite seulement que les fonctions de Boost lancent des exceptions)
	boost::system::error_code errorCode;

	// Vérifie que le fichier spécifié est vide, puis le supprime
	if (boost::filesystem::is_empty(p, errorCode))
		boost::filesystem::remove(p, errorCode);
}
#endif
