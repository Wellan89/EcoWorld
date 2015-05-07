#ifndef DEF_GLOBAL
#define DEF_GLOBAL

/*
global.h : D�finitions g�n�rales de l'application
*/

#ifndef KID_VERSION	// En mode enfant : la version du jeu est diff�rente

// Version normale :
#define ECOWORLD_VERSION		"0.0.0.2"	// Le num�ro de version actuel d'EcoWorld (doit �tre sous la forme "X.X.X.X") (�volue depuis le 24/11/11)
#define ECOWORLD_VERSION_NB		0,0,0,2		// Le num�ro de version actuel d'EcoWorld sous forme de nombres (sous la forme X,X,X,X)

#else

// Version enfant :
#define ECOWORLD_VERSION		"0.0.1.2-kid"	// Le num�ro de version actuel d'EcoWorld (version enfant) (doit �tre sous la forme "X.X.X.X") (�volue depuis le 24/11/11)
#define ECOWORLD_VERSION_NB		0,0,1,2		// Le num�ro de version actuel d'EcoWorld (version enfant) sous forme de nombres (sous la forme X,X,X,X)

#endif



#define CAN_USE_ROMAIN_DARCEL_RESSOURCES	// Si d�fini, permet d'utiliser les ressources fournies par Romain DARCEL (ic�nes et logos next-gen, musiques futuristes), sinon elles seront compl�tement d�sactiv�es

#ifdef _DEBUG	// En mode DEBUG : Test d'utilisation d'un terrain de 200m * 200m avec une grille de 1m * 1m : 1.0f = 1m r�el
#define TAILLE_OBJETS	10.0f		// La taille de base des objets (soit 1 unit�)
#define TAILLE_CARTE	200			// La taille de la carte (la limite dans laquelle on peut placer des batiments, en unit�s) (DOIT �tre multiple de 2 !)
#else
#define TAILLE_OBJETS	10.0f		// La taille de base des objets (soit 1 unit�)
#define TAILLE_CARTE	200			// La taille de la carte (la limite dans laquelle on peut placer des batiments, en unit�s) (DOIT �tre multiple de 2 !)
#endif

#define CAN_BUILD_WITH_SELECTION_RECT	// Si d�fini, certains batiments (comme les panneaux solaires) pourront �tre construits gr�ce � un rectangle de s�lection

#ifdef CAN_BUILD_WITH_SELECTION_RECT
#define FORCE_RECTANGLE					// Force la zone s�lectionn�e � �tre un rectangle (sans ce define, le fonctionnement correct du rectangle de s�lection n'est pas garanti !)

#ifdef FORCE_RECTANGLE
#define ONLY_TAKE_MAIN_CORNERS			// Permet de ne prendre en compte que les coins principaux (coins 1 et 4) pour les index du rectangle de s�lection
#endif
#endif

#define HAUTEUR_AJOUTEE_BATIMENTS	0.4f	// La hauteur ajout�e aux batiments par rapport au sol pour �viter les effets d�sagr�ables avec celui-ci

#define TERRAIN_LIMITS TAILLE_CARTE * TAILLE_OBJETS * 0.5f	// Les limites du terrain // TAILLE_CARTE * TAILLE_OBJETS / 2.0f



#ifndef NO_EXTERNAL_LIBRAIRIES	// Ce define permet d'�viter l'inclusion de librairies externes autres que celle d'Irrlicht dans le projet
#if !defined(NDEBUG_COMPATIBILITY)		// On d�sactive IrrKlang pour EcoWorld en mode compatibilit�
#define USE_IRRKLANG	// Permet d'utiliser IrrKlang (Librairie permettant de jouer des sons et des musiques ; irrKlang.dll n�c�ssaire ; Configuration du projet indentique)
#endif
#define USE_SPARK		// Permet d'utiliser Spark (Librairie d'�mmetteurs de particules ; Aucune DLL n�c�ssaire ; Configuration du projet indentique (sources � inclure au projet))
#define USE_RAKNET		// Permet d'utiliser RakNet (Librairie permettant le jeu en r�seau ; Aucune DLL n�c�ssaire ; Configuration du projet indentique (modifications automatiques gr�ce � des directives pragma))
#endif



#define USE_JOYSTICKS				// Permet d'activer la gestion des manettes et des joysticks
#define USE_COLLISIONS				// Permet d'utiliser les collisions avec les diff�rentes cam�ras du jeu

#define CAMERA_NEAR_VALUE	1.0f	// La distance la plus proche � laquelle la camera peut voir
#define CAMERA_FAR_VALUE	5000.0f	// La distance la plus �loign�e � laquelle la camera peut voir

#ifndef _DEBUG
#define GAME_CONFIGURATION_FILENAME "Game Configuration.xml"		// Le nom du fichier de configuration

#define ASK_BEFORE_QUIT				// Permet de demander la confirmation � l'utilisateur s'il veut vraiment quitter le jeu lorsqu'il est au menu principal
#else
#define GAME_CONFIGURATION_FILENAME "Game Configuration DEBUG.xml"	// Le nom du fichier de configuration
#endif





#define _IRR_STATIC_LIB_		// Indique � Irrlicht que la version utilis�e est une version statique (voir IrrCompileConfig.h)

#ifndef GLOBAL_NO_INCLUDES		// Cette d�finition permet de d�sactiver l'inclusion automatique d'autres headers externes dans celui-ci

// Inclus des headers de la STL, utilis�s dans Helpers.h, mais aussi dans de nombreux fichiers source
#include <iostream>				// Pour la gestion des entr�es/sorties standards
#include <sstream>				// Pour la classe stringstream n�cessaire au logger
#include <fstream>				// Pour la gestion des fichiers "� la C++"
#include <algorithm>			// Pour min et max

#if defined(_DEBUG) && !defined(NO_MEMORY_DEBUG)	// Ce define permet d'�viter le support du d�bogage de la m�moire
#include <irrAllocator.h>		// Cet include d'Irrlicht ne supporte pas le nouveau #define de new

#include "Outils/DebugNew.h"	// Outil de d�bogage m�moire du jeu pour �viter les fuites m�moires
#endif

#include <irrlicht.h>

using namespace irr;
using namespace std;

// Inclus toujours les outils du jeu
#include "Helpers.h"

// D�clarations de fonctions fr�quemment utilis�e pour la cr�ation de la GUI d'Irrlicht (ces fonctions sont d�finies dans main.cpp) :
//extern inline core::recti getAbsoluteRectangle(float x, float y, float x2, float y2);
extern inline core::recti getAbsoluteRectangle(float x, float y, float x2, float y2, const core::recti& parentRect);
extern inline core::recti getAbsoluteRectangle(float x, float y, float x2, float y2, const core::dimension2du& parentSize);

// Cr�e le dossier sp�cifi� et g�re les erreurs �ventuelles lors de sa cr�ation
void createDirectory(const char* directoryPath);

#endif





#endif
