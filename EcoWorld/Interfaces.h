#ifndef DEF_I_SYSTEM_RENDERER
#define DEF_I_SYSTEM_RENDERER

/*
Interfaces.h : Inclus des interfaces permettant au système de jeu (EcoWorldSystem) d'envoyer et de recevoir des informations depuis le monde 3D
*/

#include "global.h"

class Batiment;

// Interface abstraite de base pour CBatimentSceneNode (un node de jeu représentant un bâtiment), permettant aux bâtiments d'appeler directement certaines fonctions spécifiques
class IBatimentSceneNode abstract : public scene::ISceneNode
{
public:
	// Enregistre les données du node dans un fichier
	//virtual void save(io::IAttributes* out) const = 0;	// TODO : Si nécessaire

	// Charge les données du node depuis un fichier
	//virtual void load(io::IAttributes* in) = 0;			// TODO : Si nécessaire

protected:
	// Constructeur pour ISceneNode
	IBatimentSceneNode(scene::ISceneNode* parent, scene::ISceneManager* mgr, int id = -1) : scene::ISceneNode(parent, mgr, id) { }
};

// Interface abstraite de base pour EcoWorldRenderer (le renderer du système), permettant au système de jeu d'appeler directement certaines fonctions spécifiques
class ISystemRenderer abstract
{
public:
	// Ajoute un batiment dans le monde 3D, à la position indiquée si fournie, sinon elle sera recalculée suivant l'index du bâtiment et la hauteur du terrain
	virtual IBatimentSceneNode* addBatiment(Batiment* batiment, const core::vector3df* position = NULL, const float* denivele = NULL) = 0;

	// Supprime un batiment du monde 3D
	// wantedByUser est utilisé pour indiquer quel son jouer à l'IrrKlangManager : true si la destruction du bâtiment a été volontairement demandée par l'utilisateur, false sinon
	virtual void deleteBatiment(Batiment* batiment, bool wantedByUser) = 0;

	// Retourne la hauteur d'un certain point du terrain
	virtual float getTerrainHeight(float x, float y) = 0;
};

#endif
