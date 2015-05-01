#ifndef DEF_SPARK_MANAGER
#define DEF_SPARK_MANAGER

#include "global.h"

#ifdef USE_SPARK

#include "Spark/SPK.h"
#include "Spark/SPK_IRR.h"

using namespace SPK;
using namespace SPK::IRR;

class CSmokeRendererSceneNode;

// Classe gérant les systèmes de particules du jeu (pluie, fumée...) grâce à la librairie SPARK
class SparkManager
{
public:
	// Constructeur et destructeur
	SparkManager();
	~SparkManager();

	// Initialise SPARK
	void init();

	// Réinitialise SPARK
	void reset();

	// Ajoute les positions de rendu des fumées au scene node chargé de les afficher
	void registerSmokePosition(const core::list<core::vector3df>& smoke1Positions, const core::list<core::vector3df>& smoke2Positions);

protected:
	// Structure pour gérer les émetteurs de particules de Spark sous Irrlicht, et pour stocker leurs pointeurs importants
	struct SparkIrrParticleEmitter
	{
		IRRRenderer* renderer;
		Model* model;
		Zone* zone;
		Emitter* emitter;
		Group* group;
		IRRSystem* system;

		SparkIrrParticleEmitter() : renderer(0), model(0), zone(0), emitter(0), group(0), system(0)	{ }
	};

	SparkIrrParticleEmitter rain;	// Pluie
	SPK::Plane* rainKillPlane;		// Le plan à partir duquel les particules de pluie seront détruites

	SparkIrrParticleEmitter smoke1;	// Fumée 1 : Equivalente à un rayon de 0.3f
	SparkIrrParticleEmitter smoke2;	// Fumée 2 : Equivalente à un rayon de 1.0f
	
	// Le scene node gérant le rendu de la fumée
	CSmokeRendererSceneNode* smokeRenderer;

public:
	// Fonctions publiques inline :

	// Obtient les systèmes principaux de SPARK pour la fumée (doit seulement être utilisé par CBatimentSceneNode::render() pour le rendu de ses fumées !)
	IRRSystem* getSmoke1ParticleSystem()	{ return smoke1.system; }
	IRRSystem* getSmoke2ParticleSystem()	{ return smoke2.system; }

	// Retourne si le système de particules est visible pour ce rendu
	bool isParticleSystemVisible() const	{ return (rain.system ? rain.system->isVisible() : false); }

	// Affiche/Masque le système de particules pour ce rendu
	void setParticleSystemVisible(bool visible)
	{
		if (rain.system)
			rain.system->setVisible(visible);
	}

	// Modifie le débit de particules de la pluie
	void setRainFlow(float flow)
	{
		if (rain.emitter)
			rain.emitter->setFlow(flow);
	}

	// Indique la direction du vent (pour la direction des particules de pluie)
	void setWindDirection(core::vector2df direction)
	{
		if (rain.group)
		{
			direction.normalize();	// Normalise ce vecteur pour éviter d'appliquer de trop grandes gravités aux particules de pluie
			rain.group->setGravity(Vector3D(direction.X, -1.0f, direction.Y) * 800.0f);	// Forte gravité pour une accélération rapide des particules
		}
	}

	// Indique la hauteur minimale du terrain pour créer le plan à partir duquel les particules de pluie seront détruites
	void setMinTerrainHeight(float minTerrainHeight)
	{
		if (rainKillPlane)
			rainKillPlane->setPosition(Vector3D(0.0f, minTerrainHeight, 0.0f));
	}

	// Indique la vitesse du timer d'Irrlicht pour régler les pas minimaux et maximaux de mise à jour des particules
	void setTimerSpeed(float timerSpeed)
	{
		// Modifie les paramêtres de mise à jour du système de particule :

		// Ancien : Pour timerSpeed = 1.0f :
		//System::setClampStep(true, 0.6f);		// Force le temps écoulé depuis la dernière mise à jour à être toujours inférieur à 0.6 secondes (1.7 FPS)
		//System::useAdaptiveStep(0.05f, 0.2f);	// Met à jour le système de particules avec le temps écoulé réel lorsqu'il est compris entre 0.05 secondes (20 FPS) et 0.2 secondes (5 FPS), sinon si il est supérieur on le met à jour par paliers de 0.2 secondes

		// Nouveau : Dépendant de la vitesse du timer :
		System::setClampStep(true, 0.6f * timerSpeed);
		System::useAdaptiveStep(0.05f * timerSpeed, 0.2f * timerSpeed);
	}
};

#endif

#endif
