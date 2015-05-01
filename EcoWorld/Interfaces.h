#ifndef DEF_I_SYSTEM_RENDERER
#define DEF_I_SYSTEM_RENDERER

/*
Interfaces.h : Inclus des interfaces permettant au syst�me de jeu (EcoWorldSystem) d'envoyer et de recevoir des informations depuis le monde 3D
*/

#include "global.h"

class Batiment;

// Interface abstraite de base pour CBatimentSceneNode (un node de jeu repr�sentant un b�timent), permettant aux b�timents d'appeler directement certaines fonctions sp�cifiques
class IBatimentSceneNode abstract : public scene::ISceneNode
{
public:
	// Enregistre les donn�es du node dans un fichier
	//virtual void save(io::IAttributes* out) const = 0;	// TODO : Si n�cessaire

	// Charge les donn�es du node depuis un fichier
	//virtual void load(io::IAttributes* in) = 0;			// TODO : Si n�cessaire

protected:
	// Constructeur pour ISceneNode
	IBatimentSceneNode(scene::ISceneNode* parent, scene::ISceneManager* mgr, int id = -1) : scene::ISceneNode(parent, mgr, id) { }
};

// Interface abstraite de base pour EcoWorldRenderer (le renderer du syst�me), permettant au syst�me de jeu d'appeler directement certaines fonctions sp�cifiques
class ISystemRenderer abstract
{
public:
	// Ajoute un batiment dans le monde 3D, � la position indiqu�e si fournie, sinon elle sera recalcul�e suivant l'index du b�timent et la hauteur du terrain
	virtual IBatimentSceneNode* addBatiment(Batiment* batiment, const core::vector3df* position = NULL, const float* denivele = NULL) = 0;

	// Supprime un batiment du monde 3D
	// wantedByUser est utilis� pour indiquer quel son jouer � l'IrrKlangManager : true si la destruction du b�timent a �t� volontairement demand�e par l'utilisateur, false sinon
	virtual void deleteBatiment(Batiment* batiment, bool wantedByUser) = 0;

	// Retourne la hauteur d'un certain point du terrain
	virtual float getTerrainHeight(float x, float y) = 0;
};

#endif
