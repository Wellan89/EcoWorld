#ifndef DEF_BATIMENT
#define DEF_BATIMENT

#include "global.h"
#include "Batiments.h"
#include "Utils.h"
#include "EcoWorldInfos.h"
#include "Interfaces.h"

class EcoWorldSystem;
class EcoWorldModifiers;

class Batiment
{
public:
	// Constructeur
	Batiment(BatimentID batimentID, EcoWorldSystem& m_system, EcoWorldInfos& m_worldInfos, EcoWorldModifiers& m_modifiers,
		const core::vector2di m_index, float m_rotation);

	// Met � jour le syst�me de jeu d'apr�s les informations de construction de ce b�timent (� appeler � sa construction)
	void onConstruct();

	// Met � jour le syst�me de jeu d'apr�s les informations de destruction de ce b�timent (appel� � sa destruction)
	void onDestroy();

	// Met � jour le temps de vie de ce b�timent, ainsi que ses temps de construction et de destruction.
	// Si ce b�timent doit �tre d�truit, il s'ajoutera automatiquement aux listes de destruction du syst�me de jeu.
	// A chaque appel de cette fonction, on consid�rera qu'un jour suppl�mentaire s'est �coul�.
	// outNormalBatimentDestroy :	Si fourni, valeur de sortie indiquant si la destruction d'un b�timent vient d'�tre termin�e (destruction volontaire du joueur) (attention : valeur non initialis�e � l'appel de la fonction)
	// outOldBatimentDestroy :		Si fourni, valeur de sortie indiquant si un b�timent vient d'�tre d�truit par vieillesse (destruction involontaire du joueur : dur�e de vie termin�e)) (attention : valeur non initialis�e � l'appel de la fonction)
	void updateDaysPassed(bool* outNormalBatimentDestroy = NULL, bool* outOldBatimentDestroy = NULL);

	// Met � jour les informations sp�cifiques � ce b�timent pour ce jour-ci :
	// effectu� juste apr�s l'appel � updateDaysPassed() lorsque le b�timent n'est ni en construction ni en destruction, et effectu� avant la mise � jour des informations du monde avec update().
	// Aucune mise � jour des informations du monde n'est effectu�e, simplement une mise � jour des informations de ce b�timent ind�pendamment des autres informations du monde.
	// Cette fonction peut �tre appel�e plusieurs fois par jour, � condition que le bool�en firstCall soit pass� � false lors des appels suppl�mentaires.
	// firstCall :							True si c'est le premier appel de cette fonction ce jour (dans ce cas, on consid�re qu'un jour suppl�mentaire s'est �coul�), false sinon.
	// pourcentageRessourcesDisponibles :	Repr�sente le pourcentage disponible de chaque ressources pour cette mise � jour (permet de limiter la consommation relative de chaque usine)
	void updateBatimentInfos(bool firstCall, const float pourcentageRessourcesDisponibles[Ressources::RI_COUNT]);

	// Met � jour les informations du monde et de ce b�timent avec le nombre de jours, mois et ann�es �coul�s
	// A chaque appel de cette fonction, on consid�rera qu'un jour suppl�mentaire s'est �coul�
	void update(bool moisEcoule, bool anneeEcoulee);

	// Enregistre les donn�es du batiment dans un fichier
	void save(io::IAttributes* out) const;

	// Charge les donn�es du batiment depuis un fichier
	void load(io::IAttributes* in);

protected:
	// Donne un comportement du b�timent sur le monde diff�rent suivant son ID (sp�cialisation de la fonction updateBatimentInfos() pour chaque b�timent).
	// Cette fonction peut �tre appel�e plusieurs fois par jour.
	// pourcentageRessourcesDisponibles :	Repr�sente le pourcentage disponible de chaque ressources pour cette mise � jour (permet de limiter la consommation relative de chaque usine)
	void updateBatimentInfosFromBatimentID(const float pourcentageRessourcesDisponibles[Ressources::RI_COUNT]);

	EcoWorldSystem& system;
	EcoWorldInfos& worldInfos;
	EcoWorldModifiers& modifiers;

	IBatimentSceneNode* sceneNode;

	BatimentID ID;
	BatimentInfos infos;
	const StaticBatimentInfos& staticInfos;

	// L'index du b�timent dans la carte syst�me
	core::vector2di index;

	// La rotation en Y du batiment
	float rotation;

	// Le nombre de jours �coul�s depuis le d�but de la construction du b�timent (l'"�ge" du b�timent)
	u32 daysPassed;

	// Le jour o� l'ordre de destruction a �t� lanc� (si �gal � 0, aucun ordre de construction n'a �t� lanc� : le b�timent n'est pas en train d'�tre d�truit)
	u32 destroyingDay;

	// La box contenant le batiment dans le plan 2D (conserv�e en m�moire pour acc�l�rer certains calculs du syst�me)
	obbox2df box;

public:
	// Accesseurs inline
	const EcoWorldSystem& getSystem() const				{ return system; }
	BatimentInfos& getInfos()							{ return infos; }
	const BatimentInfos& getInfos() const				{ return infos; }
	const StaticBatimentInfos& getStaticInfos() const	{ return staticInfos; }
	BatimentID getID() const							{ return ID; }
	bool isDestroying() const							{ return (destroyingDay != (u32)(-1)); }
	bool isConstructing() const							{ return (!isDestroying() && daysPassed < staticInfos.tempsC && staticInfos.tempsC != 0); }
	bool isConstructingOrDestroying() const				{ return ((destroyingDay != (u32)(-1)) || (daysPassed < staticInfos.tempsC && staticInfos.tempsC != 0)); }
	u32 getDestroyingDay() const						{ return destroyingDay; }
	u32 getDaysPassed() const							{ return daysPassed; }
	const core::vector2di& getIndex() const				{ return index; }
	float getRotation() const							{ return rotation; }
	const obbox2df& getBoundingBox() const				{ return box; }

	IBatimentSceneNode* getSceneNode()					{ return sceneNode; }
	void setSceneNode(IBatimentSceneNode* node)			{ sceneNode = node; }

	// Lance la destruction du b�timent
	void destroy(u32 currentDay)
	{
		destroyingDay = currentDay;	// On peut aussi annuler la destruction du b�timent en indiquant currentDay = (u32)(-1), mais ces cons�quences sont encores impr�vues

		if (destroyingDay != (u32)(-1))
		{
			// Met � jour les informations du monde si ce b�timent va �tre d�truit
			onDestroy();

			// Pendant sa destruction, le b�timent devient compl�tement inactif : on r�initialise ses informations
			infos.reset();
		}
	}
};

#endif
