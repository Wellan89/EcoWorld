#ifndef DEF_GLOBAL
#define DEF_GLOBAL

/*
global.h : Définitions générales de l'application
*/

#ifndef KID_VERSION	// En mode enfant : la version du jeu est différente

// Version normale :
#define ECOWORLD_VERSION		"0.0.0.2"	// Le numéro de version actuel d'EcoWorld (doit être sous la forme "X.X.X.X") (évolue depuis le 24/11/11)
#define ECOWORLD_VERSION_NB		0,0,0,2		// Le numéro de version actuel d'EcoWorld sous forme de nombres (sous la forme X,X,X,X)

#else

// Version enfant :
#define ECOWORLD_VERSION		"0.0.1.2-kid"	// Le numéro de version actuel d'EcoWorld (version enfant) (doit être sous la forme "X.X.X.X") (évolue depuis le 24/11/11)
#define ECOWORLD_VERSION_NB		0,0,1,2		// Le numéro de version actuel d'EcoWorld (version enfant) sous forme de nombres (sous la forme X,X,X,X)

#endif



#define CAN_USE_ROMAIN_DARCEL_RESSOURCES	// Si défini, permet d'utiliser les ressources fournies par Romain DARCEL (icônes et logos next-gen, musiques futuristes), sinon elles seront complètement désactivées

#ifdef _DEBUG	// En mode DEBUG : Test d'utilisation d'un terrain de 200m * 200m avec une grille de 1m * 1m : 1.0f = 1m réel
#define TAILLE_OBJETS	10.0f		// La taille de base des objets (soit 1 unité)
#define TAILLE_CARTE	200			// La taille de la carte (la limite dans laquelle on peut placer des batiments, en unités) (DOIT être multiple de 2 !)
#else
#define TAILLE_OBJETS	10.0f		// La taille de base des objets (soit 1 unité)
#define TAILLE_CARTE	200			// La taille de la carte (la limite dans laquelle on peut placer des batiments, en unités) (DOIT être multiple de 2 !)
#endif

#define CAN_BUILD_WITH_SELECTION_RECT	// Si défini, certains batiments (comme les panneaux solaires) pourront être construits grâce à un rectangle de sélection

#ifdef CAN_BUILD_WITH_SELECTION_RECT
#define FORCE_RECTANGLE					// Force la zone sélectionnée à être un rectangle (sans ce define, le fonctionnement correct du rectangle de sélection n'est pas garanti !)

#ifdef FORCE_RECTANGLE
#define ONLY_TAKE_MAIN_CORNERS			// Permet de ne prendre en compte que les coins principaux (coins 1 et 4) pour les index du rectangle de sélection
#endif
#endif

#define HAUTEUR_AJOUTEE_BATIMENTS	0.4f	// La hauteur ajoutée aux batiments par rapport au sol pour éviter les effets désagréables avec celui-ci

#define TERRAIN_LIMITS TAILLE_CARTE * TAILLE_OBJETS * 0.5f	// Les limites du terrain // TAILLE_CARTE * TAILLE_OBJETS / 2.0f



#ifndef NO_EXTERNAL_LIBRAIRIES	// Ce define permet d'éviter l'inclusion de librairies externes autres que celle d'Irrlicht dans le projet
#if !defined(NDEBUG_COMPATIBILITY)		// On désactive IrrKlang pour EcoWorld en mode compatibilité
#define USE_IRRKLANG	// Permet d'utiliser IrrKlang (Librairie permettant de jouer des sons et des musiques ; irrKlang.dll nécéssaire ; Configuration du projet indentique)
#endif
#define USE_SPARK		// Permet d'utiliser Spark (Librairie d'émmetteurs de particules ; Aucune DLL nécéssaire ; Configuration du projet indentique (sources à inclure au projet))
#define USE_RAKNET		// Permet d'utiliser RakNet (Librairie permettant le jeu en réseau ; Aucune DLL nécéssaire ; Configuration du projet indentique (modifications automatiques grâce à des directives pragma))
#endif



#define USE_JOYSTICKS				// Permet d'activer la gestion des manettes et des joysticks
#define USE_COLLISIONS				// Permet d'utiliser les collisions avec les différentes caméras du jeu

#define CAMERA_NEAR_VALUE	1.0f	// La distance la plus proche à laquelle la camera peut voir
#define CAMERA_FAR_VALUE	5000.0f	// La distance la plus éloignée à laquelle la camera peut voir

#ifndef _DEBUG
#define GAME_CONFIGURATION_FILENAME "Game Configuration.xml"		// Le nom du fichier de configuration

#define ASK_BEFORE_QUIT				// Permet de demander la confirmation à l'utilisateur s'il veut vraiment quitter le jeu lorsqu'il est au menu principal
#else
#define GAME_CONFIGURATION_FILENAME "Game Configuration DEBUG.xml"	// Le nom du fichier de configuration
#endif





#define _IRR_STATIC_LIB_		// Indique à Irrlicht que la version utilisée est une version statique (voir IrrCompileConfig.h)

#ifndef GLOBAL_NO_INCLUDES		// Cette définition permet de désactiver l'inclusion automatique d'autres headers externes dans celui-ci

// Inclus des headers de la STL, utilisés dans Helpers.h, mais aussi dans de nombreux fichiers source
#include <iostream>				// Pour la gestion des entrées/sorties standards
#include <sstream>				// Pour la classe stringstream nécessaire au logger
#include <fstream>				// Pour la gestion des fichiers "à la C++"
#include <algorithm>			// Pour min et max

#if defined(_DEBUG) && !defined(NO_MEMORY_DEBUG)	// Ce define permet d'éviter le support du débogage de la mémoire
#include <irrAllocator.h>		// Cet include d'Irrlicht ne supporte pas le nouveau #define de new

#include "Outils/DebugNew.h"	// Outil de débogage mémoire du jeu pour éviter les fuites mémoires
#endif

#include <irrlicht.h>

using namespace irr;
using namespace std;

// Inclus toujours les outils du jeu
#include "Helpers.h"

// Déclarations de fonctions fréquemment utilisée pour la création de la GUI d'Irrlicht (ces fonctions sont définies dans main.cpp) :
//extern inline core::recti getAbsoluteRectangle(float x, float y, float x2, float y2);
extern inline core::recti getAbsoluteRectangle(float x, float y, float x2, float y2, const core::recti& parentRect);
extern inline core::recti getAbsoluteRectangle(float x, float y, float x2, float y2, const core::dimension2du& parentSize);

// Crée le dossier spécifié et gère les erreurs éventuelles lors de sa création
void createDirectory(const char* directoryPath);

#endif





#endif
