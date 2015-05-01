#ifndef DEF_SPARK_MANAGER
#define DEF_SPARK_MANAGER

#include "global.h"

#ifdef USE_SPARK

#include "Spark/SPK.h"
#include "Spark/SPK_IRR.h"

using namespace SPK;
using namespace SPK::IRR;

class CSmokeRendererSceneNode;

// Classe g�rant les syst�mes de particules du jeu (pluie, fum�e...) gr�ce � la librairie SPARK
class SparkManager
{
public:
	// Constructeur et destructeur
	SparkManager();
	~SparkManager();

	// Initialise SPARK
	void init();

	// R�initialise SPARK
	void reset();

	// Ajoute les positions de rendu des fum�es au scene node charg� de les afficher
	void registerSmokePosition(const core::list<core::vector3df>& smoke1Positions, const core::list<core::vector3df>& smoke2Positions);

protected:
	// Structure pour g�rer les �metteurs de particules de Spark sous Irrlicht, et pour stocker leurs pointeurs importants
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
	SPK::Plane* rainKillPlane;		// Le plan � partir duquel les particules de pluie seront d�truites

	SparkIrrParticleEmitter smoke1;	// Fum�e 1 : Equivalente � un rayon de 0.3f
	SparkIrrParticleEmitter smoke2;	// Fum�e 2 : Equivalente � un rayon de 1.0f
	
	// Le scene node g�rant le rendu de la fum�e
	CSmokeRendererSceneNode* smokeRenderer;

public:
	// Fonctions publiques inline :

	// Obtient les syst�mes principaux de SPARK pour la fum�e (doit seulement �tre utilis� par CBatimentSceneNode::render() pour le rendu de ses fum�es !)
	IRRSystem* getSmoke1ParticleSystem()	{ return smoke1.system; }
	IRRSystem* getSmoke2ParticleSystem()	{ return smoke2.system; }

	// Retourne si le syst�me de particules est visible pour ce rendu
	bool isParticleSystemVisible() const	{ return (rain.system ? rain.system->isVisible() : false); }

	// Affiche/Masque le syst�me de particules pour ce rendu
	void setParticleSystemVisible(bool visible)
	{
		if (rain.system)
			rain.system->setVisible(visible);
	}

	// Modifie le d�bit de particules de la pluie
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
			direction.normalize();	// Normalise ce vecteur pour �viter d'appliquer de trop grandes gravit�s aux particules de pluie
			rain.group->setGravity(Vector3D(direction.X, -1.0f, direction.Y) * 800.0f);	// Forte gravit� pour une acc�l�ration rapide des particules
		}
	}

	// Indique la hauteur minimale du terrain pour cr�er le plan � partir duquel les particules de pluie seront d�truites
	void setMinTerrainHeight(float minTerrainHeight)
	{
		if (rainKillPlane)
			rainKillPlane->setPosition(Vector3D(0.0f, minTerrainHeight, 0.0f));
	}

	// Indique la vitesse du timer d'Irrlicht pour r�gler les pas minimaux et maximaux de mise � jour des particules
	void setTimerSpeed(float timerSpeed)
	{
		// Modifie les param�tres de mise � jour du syst�me de particule :

		// Ancien : Pour timerSpeed = 1.0f :
		//System::setClampStep(true, 0.6f);		// Force le temps �coul� depuis la derni�re mise � jour � �tre toujours inf�rieur � 0.6 secondes (1.7 FPS)
		//System::useAdaptiveStep(0.05f, 0.2f);	// Met � jour le syst�me de particules avec le temps �coul� r�el lorsqu'il est compris entre 0.05 secondes (20 FPS) et 0.2 secondes (5 FPS), sinon si il est sup�rieur on le met � jour par paliers de 0.2 secondes

		// Nouveau : D�pendant de la vitesse du timer :
		System::setClampStep(true, 0.6f * timerSpeed);
		System::useAdaptiveStep(0.05f * timerSpeed, 0.2f * timerSpeed);
	}
};

#endif

#endif
