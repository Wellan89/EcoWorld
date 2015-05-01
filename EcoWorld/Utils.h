#ifndef DEF_UTILS
#define DEF_UTILS

#include <cmath> // Pour les fonctions cos et sin
#include "Batiments.h"

class Batiment;

enum E_BATIMENT_CREATING_ERROR	// Enum indiquant pourquoi on ne peut pas placer un batiment
{
	EBCE_AUCUNE = 0,					// Aucune erreur, le batiment peut être placé

	EBCE_BUDGET = 1,					// Budget insuffisant
	EBCE_ENERGIE = 2,					// Energie insuffisante ; N'est plus indispensable, mais tout de même conservé pour l'actualisation de la GUI 
	EBCE_PLACE = 4,						// Un autre batiment occupe déjà cet endroit ou entre en collision avec le nouveau batiment
	EBCE_TERRAIN_NON_CONSTRUCTIBLE = 8,	// Le terrain n'est pas constructible à cet endroit
	EBCE_TERRAIN_OUT = 16,				// La position du batiment est en dehors du terrain
	EBCE_NEED_DEEP_WATER = 32,			// Le batiment sélectionné nécessite de l'eau profonde pour pouvoir être construit
	EBCE_RESSOURCE = 64,				// Une ou plusieurs ressources sont manquantes (vérifier le tableau outRessources pour savoir lesquelles)
	EBCE_NOT_ENOUGH_POPULATION = 128,	// La population actuelle du monde est insuffisante pour débloquer ce bâtiment
	EBCE_BATIMENT_ID = 256,				// L'ID du batiment n'est pas valide
};

#ifdef CAN_BUILD_WITH_SELECTION_RECT
enum E_BATIMENT_MULTI_CREATING_ERROR	// Enum indiquant pourquoi on ne peut pas placer plusieurs batiments
{
	EBMCE_AUCUNE = 0,					// Aucune erreur, les batiments peuvent être placés

	EBMCE_BUDGET = 1,					// Budget insuffisant
	EBMCE_ENERGIE = 2,					// Energie insuffisante ; N'est plus indispensable, mais tout de même conservé pour l'actualisation de la GUI 
	EBMCE_RESSOURCE = 4,				// Une ou plusieurs ressources sont manquantes (vérifier le tableau outRessources pour savoir lesquelles)
	EBMCE_BATIMENT_ID = 8,				// L'ID du batiment n'est pas valide
};
#endif

enum E_BATIMENT_DESTROYING_ERROR	// Enum indiquant pourquoi on ne peut pas détruire un batiment
{
	EBDE_AUCUNE = 0,					// Aucune erreur, le batiment peut être détruit

	EBDE_BUDGET = 1,					// Budget insuffisant
	EBDE_RESSOURCE = 2,					// Une ou plusieurs ressources sont manquantes (vérifier le tableau outRessources pour savoir lesquelles)
	EBDE_INVALID_INDEX = 4,				// L'index spécifiant la position du batiment n'est pas valide, ou il n'y a aucun batiment à cette position
	EBDE_ALREADY_DESTRUCTING = 8,		// Le bâtiment est déjà en train de se détruire
};

// Structure utilisée pour savoir les informations sur une parcelle de terrain
struct TerrainInfo
{
	bool constructible;		// True si le terrain est constructible
	bool deepWater;			// True si le terrain est sur de l'eau profonde
	Batiment* batiment;

	// Remet les informations sur le terrain à zéro (attention, ne détruit pas le batiment, le met simplement à NULL !) -> Raisons d'initialisation de la structure
	void reset()
	{
		constructible = true;
		deepWater = false;
		batiment = NULL;
	}

	TerrainInfo()	{ reset(); }
};

class Time
{
#define DAY_TIME		2.0f	// Le temps réel correspondant à un jour dans le système de jeu
#define DAY_TIME_INV	0.5f	// 1.0f / DAY_TIME

protected:
	// Le temps total écoulé dans la réalité (en secondes)
	float totalTime;

	// Les données du temps écoulé dans le jeu
	u32 jours;		// 1 jour = 2 secondes dans la vraie vie
	u32 mois;		// 1 mois = 30 jours
	u32 annees;		// 1 an = 12 mois

	u32 totalJours;	// Le nombre total de jours
	u32 totalMois;	// Le nombre total de mois

	float extraTime;	// Le léger temps en surplus qui ne permet pas de faire un jour supplémentaire

public:
	void updateTime()
	{
		if (totalTime < 0.0f)
			totalTime = 0.0f;

		totalJours = (u32)(totalTime * DAY_TIME_INV);
		totalMois = totalJours / 30;
		annees = totalMois / 12;

		jours = totalJours % 30;
		mois = totalMois % 12;

		extraTime = totalTime - (totalJours * DAY_TIME);
	}

	void setTotalTime(float time)	{ totalTime = time; updateTime(); }

	// Constructeurs et destructeur
	Time(float startTime = 0.0f) : totalTime(startTime)	{ setTotalTime(startTime); }
	Time(const Time& time) : totalTime(time.totalTime)	{ setTotalTime(time.totalTime); }
	~Time()	{ }

	Time& operator++()		// Opérateur : ++time
	{
		totalTime++;

		updateTime();

		return *this;
	}
	Time& operator--()		// Opérateur : --time
	{
		totalTime--;

		updateTime();

		return *this;
	}
	Time operator++(int)	// Opérateur : time++
	{
		const Time tmp(*this);

		totalTime++;
		updateTime();

		return tmp;
	}
	Time operator--(int)	// Opérateur : time--
	{
		const Time tmp(*this);

		totalTime--;
		updateTime();

		return tmp;
	}
	Time operator+(const Time& other) const
	{
		return Time(totalTime + other.totalTime);
	}
	Time operator-(const Time& other) const
	{
		return Time(totalTime - other.totalTime);
	}
	Time& operator=(const Time& other)
	{
		totalTime = other.totalTime;

		updateTime();

		return *this;
	}
	Time& operator+=(const Time& other)
	{
		totalTime += other.totalTime;

		updateTime();

		return *this;
	}
	Time& operator-=(const Time& other)
	{
		totalTime -= other.totalTime, totalTime;

		updateTime();

		return *this;
	}
	bool operator==(const Time& other) const
	{
		return (totalTime == other.totalTime);
	}
	bool operator!=(const Time& other) const
	{
		return (totalTime != other.totalTime);
	}
	bool operator>(const Time& other) const
	{
		return (totalTime > other.totalTime);
	}
	bool operator<(const Time& other) const
	{
		return (totalTime < other.totalTime);
	}
	bool operator>=(const Time& other) const
	{
		return (totalTime >= other.totalTime);
	}
	bool operator<=(const Time& other) const
	{
		return (totalTime <= other.totalTime);
	}

	Time operator+(float value) const
	{
		return Time(totalTime + value);
	}
	Time operator-(float value) const
	{
		return Time(totalTime - value);
	}
	Time& operator=(float value)
	{
		totalTime = value;

		updateTime();

		return *this;
	}
	Time& operator+=(float value)
	{
		totalTime += value;

		updateTime();

		return *this;
	}
	Time& operator-=(float value)
	{
		totalTime -= value;

		updateTime();

		return *this;
	}
	bool operator==(float value) const
	{
		return (totalTime == value);
	}
	bool operator!=(float value) const
	{
		return (totalTime != value);
	}
	bool operator>(float value) const
	{
		return (totalTime > value);
	}
	bool operator<(float value) const
	{
		return (totalTime < value);
	}
	bool operator>=(float value) const
	{
		return (totalTime >= value);
	}
	bool operator<=(float value) const
	{
		return (totalTime <= value);
	}

	float getTotalTime() const	{ return totalTime; }
	u32 getJours() const		{ return jours; }
	u32 getMois() const			{ return mois; }
	u32 getAnnees() const		{ return annees; }
	u32 getTotalJours() const	{ return totalJours; }
	u32 getTotalMois() const	{ return totalMois; }
	float getExtraTime() const	{ return extraTime; }
};

// Classe pour manipuler les OBB (Oriented Bounding Box) dans R^2
// TODO : Cette classe peut être optimisée !
class obbox2df
{
protected:
	/*
	Schéma du rectangle :

	   A    B
	    +--+
	    |  |
	    +--+
	   D    C

	*/

	// Les 4 points du rectangle
	core::vector2df A, B, C, D;

	// Tourne un point par rapport à l'origine suivant un angle en radians donné
	static core::vector2df rotatePoint(const core::vector2df& point, float angleRad)
	{
		// Formule de rotation trouvée ici (elle a été vérifiée avec AlgoBox) :
		// http://www.alrj.org/docs/3D/mat-rot.php

		return core::vector2df(
			point.X * cos(angleRad) - point.Y * sin(angleRad),
			point.X * sin(angleRad) + point.Y * cos(angleRad));
	}

private:
	// Retourne si cette box entre en collision avec une ligne marquée par deux points, permet de vérifier certains cas extrêmes dans le cas où 'k' a déjà été égal à 1 et à 0 à la fois
	bool intersectsWithLineAdvanced(const core::vector2df& pointO, const core::vector2df& pointP, bool& kAlreadyEquals0, bool& kAlreadyEquals1) const
	{
		// Algorithmes trouvés ici :
		// http://www.siteduzero.com/tutoriel-3-254490-formes-plus-complexes.html#ss_part_3
		// Quelques modifications opérées ici pour résoudre certains bugs lors des tests d'intersection, voir la fonction de base intersectsWithLine pour comparaison

		// -> Les valeurs de kAlreadyEquals0 et de kAlreadyEquals1 doivent être conservées en mémoire hors de l'appel à cette fonction
		// car on ne calcule la valeur de 'k' qu'une seule fois par appel à cette fonction si il est effectivement égal à une de ces valeurs extrêmes

		core::vector2df pointA, pointB, vecteurAO, vecteurAP, vecteurAB, vecteurOP;

		for (int i = 0; i < 4; ++i)
		{
			// Prend les 4 côtés de la box un par un
			switch (i)
			{
			case 0:
				pointA = A;
				pointB = B;
				break;
			case 1:
				pointA = B;
				pointB = C;
				break;
			case 2:
				pointA = C;
				pointB = D;
				break;
			case 3:
				pointA = D;
				pointB = A;
				break;
			}

			// Calcul de collision entre droite et segment :
			// les deux points ne doivent pas être du même côté de la droite
			vecteurAB.set(pointB - pointA);
			vecteurAP.set(pointP - pointA);
			vecteurAO.set(pointO - pointA);

			// Si les deux points sont du même côté de la droite
			// ce n'est pas la peine de calculer si les deux segments entrent en collision
			if ((vecteurAB.X * vecteurAP.Y - vecteurAB.Y * vecteurAP.X) * (vecteurAB.X * vecteurAO.Y - vecteurAB.Y * vecteurAO.X) >= 0)
				continue;

			// Calcul de collision entre segment et segment :
			// l'intersection est à l'intérieur du segment
			vecteurOP.set(pointP - pointO);

			const float k = -(pointA.X * vecteurOP.Y - pointO.X * vecteurOP.Y - vecteurOP.X * pointA.Y + vecteurOP.X * pointO.Y) / (vecteurAB.X * vecteurOP.Y - vecteurAB.Y * vecteurOP.X);

			// Vérifie si k n'est pas égal à une des valeurs extrêmes
			if (k == 0)
				kAlreadyEquals0 = true;
			if (k == 1)
				kAlreadyEquals1 = true;

			// Autorise le fait que les segments se "touchent" sans se couper (mais cette méthode amène à la vérification ci-dessus, si k a déjà été égal à 0 et à 1 à la fois)
			// Activer la ligne de débogage ci-dessous pour comprendre
#if defined(_DEBUG) && 0
			static int n = 0; cout << n++ << "     k = " << k << endl;
#endif
			//if (k >= 0 && k <= 1)
			if ((k > 0 && k < 1) || (kAlreadyEquals0 && kAlreadyEquals1))
				return true;
		}

		return false;
	}

public:
	void set(const core::rectf& rect, float angleDeg)
	{
		A = rect.UpperLeftCorner;
		B = core::vector2df(rect.LowerRightCorner.X, rect.UpperLeftCorner.Y);
		C = rect.LowerRightCorner;
		D = core::vector2df(rect.UpperLeftCorner.X, rect.LowerRightCorner.Y);

		// Les coordonnées du centre du rectangle
		const core::vector2df rectangleCenter((rect.LowerRightCorner - rect.UpperLeftCorner) * 0.5f + rect.UpperLeftCorner);

		// Soustrait le centre du rectangle pour qu'il soit centré sur l'origine (pour la rotation)
		A -= rectangleCenter;
		B -= rectangleCenter;
		C -= rectangleCenter;
		D -= rectangleCenter;

		// Tourne les points suivant l'angle converti en radians
		const float angleRad = core::degToRad(angleDeg);
		A = rotatePoint(A, angleRad);
		B = rotatePoint(B, angleRad);
		C = rotatePoint(C, angleRad);
		D = rotatePoint(D, angleRad);

#if 1
		// Arrondi les coins à 0.0001f dûes aux imprécisions des fonctions sin, cos et à la conversion degrés -> radians
		A.set(core::round32(A.X * 10000.0f) * 0.0001f, core::round32(A.Y * 10000.0f) * 0.0001f);
		B.set(core::round32(B.X * 10000.0f) * 0.0001f, core::round32(B.Y * 10000.0f) * 0.0001f);
		C.set(core::round32(C.X * 10000.0f) * 0.0001f, core::round32(C.Y * 10000.0f) * 0.0001f);
		D.set(core::round32(D.X * 10000.0f) * 0.0001f, core::round32(D.Y * 10000.0f) * 0.0001f);
#endif

		// Rajoute le centre du rectangle précedemment soustrait
		A += rectangleCenter;
		B += rectangleCenter;
		C += rectangleCenter;
		D += rectangleCenter;
	}
	void set(const obbox2df& other)
	{
		A = other.A;
		B = other.B;
		C = other.C;
		D = other.D;
	}

	obbox2df(const core::rectf& rect = core::rectf(-1.0f, -1.0f, 1.0f, 1.0f), float angleDeg = 0.0f)
	{
		set(rect, angleDeg);
	}
	obbox2df(const obbox2df& other)
	{
		set(other);
	}

	// Retourne si un point est à l'intérieur de cette box
	bool isPointInside(const core::vector2df& point) const
	{
		// Algorithme trouvé ici :
		// http://www.siteduzero.com/tutoriel-3-254490-formes-plus-complexes.html#ss_part_1

		core::vector2df pointA, pointB;
		core::vector2df vecteurD, vecteurT;
		for(int i = 0; i < 4; ++i)
		{
			// Prend les 4 côtés de la box un par un
			switch (i)
			{
			case 0:
				pointA = A;
				pointB = B;
				break;
			case 1:
				pointA = B;
				pointB = C;
				break;
			case 2:
				pointA = C;
				pointB = D;
				break;
			case 3:
				pointA = D;
				pointB = A;
				break;
			}

			vecteurD.set(pointB- pointA);
			vecteurT.set(point - pointA);

			if ((vecteurD.X * vecteurT.Y - vecteurD.Y * vecteurT.X) < 0)
				return false;  // un point à droite et on arrête tout.
		}

		return true;  // si on sort du for, c'est qu'aucun point n'est à droite, donc c'est bon.
	}

	// Retourne si cette box entre en collision avec une ligne marquée par deux points
	bool intersectsWithLine(const core::vector2df& pointO, const core::vector2df& pointP) const
	{
		// Algorithmes trouvés ici :
		// http://www.siteduzero.com/tutoriel-3-254490-formes-plus-complexes.html#ss_part_3

		core::vector2df pointA, pointB;
		core::vector2df vecteurAO, vecteurAP, vecteurAB, vecteurOP;

		for (int i = 0; i < 4; ++i)
		{
			// Prend les 4 côtés de la box un par un
			switch (i)
			{
			case 0:
				pointA = A;
				pointB = B;
				break;
			case 1:
				pointA = B;
				pointB = C;
				break;
			case 2:
				pointA = C;
				pointB = D;
				break;
			case 3:
				pointA = D;
				pointB = A;
				break;
			}

			// Calcul de collision entre droite et segment :
			// les deux points ne doivent pas être du même côté de la droite
			vecteurAB.set(pointB - pointA);
			vecteurAP.set(pointP - pointA);
			vecteurAO.set(pointO - pointA);

			// Si les deux points sont du même côté de la droite
			// ce n'est pas la peine de calculer si les deux segments entrent en collision
			if ((vecteurAB.X * vecteurAP.Y - vecteurAB.Y * vecteurAP.X) * (vecteurAB.X * vecteurAO.Y - vecteurAB.Y * vecteurAO.X) >= 0)
				continue;

			// Calcul de collision entre segment et segment :
			// l'intersection est à l'intérieur du segment
			vecteurOP.set(pointP - pointO);

			const float k = -(pointA.X * vecteurOP.Y - pointO.X * vecteurOP.Y - vecteurOP.X * pointA.Y + vecteurOP.X * pointO.Y) / (vecteurAB.X * vecteurOP.Y - vecteurAB.Y * vecteurOP.X);
			if (k >= 0 && k <= 1)
				return true;
		}

		return false;
	}

	// Retourne si cette box entre en collision avec une ligne
	bool intersectsWithLine(const core::line2df& ligne) const
	{
		return intersectsWithLine(ligne.start, ligne.end);
	}

	// Retourne si cette box entre en collision avec une autre
	bool intersectsWithBox(const obbox2df& other) const
	{
		// On fait des tests pour savoir si les cotés de chaque box ne se coupent pas :
		// si au moins deux se coupent, alors les box sont en collision
		// On utilise aussi la fonction intersectsWithLineAdvanced pour vérifier certains cas extrêmes

		bool kAlreadyEquals0 = false, kAlreadyEquals1 = false;
		core::vector2df pointA, pointB;
		for (int i = 0; i < 4; ++i)
		{
			// Prend les 4 côtés de la seconde box un par un
			switch (i)
			{
			case 0:
				pointA = other.A;
				pointB = other.B;
				break;
			case 1:
				pointA = other.B;
				pointB = other.C;
				break;
			case 2:
				pointA = other.C;
				pointB = other.D;
				break;
			case 3:
				pointA = other.D;
				pointB = other.A;
				break;
			}

			if (intersectsWithLineAdvanced(pointA, pointB, kAlreadyEquals0, kAlreadyEquals1))
				return true;
		}

		return false;
	}

	// Retourne si cette box est contenue dans un rectangle 2D aligné avec les axes
	bool isInsideRect(const core::rectf& rect) const
	{
		return (rect.isPointInside(A) && rect.isPointInside(B) && rect.isPointInside(C) && rect.isPointInside(D));
	}

#if 0
	// Retourne si cette box est contenue dans une autre obbox (méthode peu optimisée)
	bool isInsideBox(const obbox2df& other) const
	{
		// Déterminé logiquement : sans source sûre (peu fiable : bug si les deux box se touchent sur les bords, et très peu optimisé) :
		// Une boîte A est contenue dans une boîte B <=> Au moins un point de A est dans B && A ne coupe pas B

		// On prend ici le centre pour être sûr de ne pas prendre un point "aux limites" de cette boîte,
		// il est ici vu comme un point quelcquonque à l'intérieur de cette boîte
		// (isPointInside retournerait false si on prenait un point touchant les bords de la boîte B)
		if (!other.isPointInside(getCenter()))
			return false;

		return !(intersectsWithBox(other));
	}
#endif

	// Retourne la liste des cases incluses dans la surface de cette box sous forme de vector2di (la surface est élargie dans les problèmes d'arrondi avec les floats)
	// Les valeurs retournées sont toujours comprises entre 0 et TAILLE_CARTE - 1 (inclues)
	core::list<core::vector2di> getCasesList() const
	{
		core::list<core::vector2di> list;

		// Optimisation : Obtient la taille maximale des bâtiments pour déterminer la limite des cases dans lesquelles effectuer les recherches
		const int TAILLE_MAX_BATS = (int)(StaticBatimentInfos::getTailleMaxBatiments());
		const u32 XMin = (u32)(max(core::floor32(min(min(A.X, B.X), min(C.X, D.X))) - TAILLE_MAX_BATS, 0));
		const u32 XMax = (u32)(max(min(core::ceil32(max(max(A.X, B.X), max(C.X, D.X))) + TAILLE_MAX_BATS, TAILLE_CARTE), 0));	// Le dernier max(..., 0) n'est qu'une sécurité : il est déjà arrivé que toutes les valeurs en X de A, B, C et D soient négatives !
		const u32 YMin = (u32)(max(core::floor32(min(min(A.Y, B.Y), min(C.Y, D.Y))) - TAILLE_MAX_BATS, 0));
		const u32 YMax = (u32)(max(min(core::ceil32(max(max(A.Y, B.Y), max(C.Y, D.Y))) + TAILLE_MAX_BATS, TAILLE_CARTE), 0));	// Le dernier max(..., 0) n'est qu'une sécurité : il est déjà arrivé que toutes les valeurs en X de A, B, C et D soient négatives !

		// Parcours toutes les cases possibles et détermine si chaque case est à l'intérieur de cette box ou non
		for (u32 i = XMin; i < XMax; ++i)
		{
			const float point1X = (float)i;
			const float point2X = (float)(i + 1);
			for (u32 j = YMin; j < YMax; ++j)
			{
				const core::vector2di currentRect(i, j);

				// Vérifie si le premier point formé est dans ce rectangle
				const float point1Y = (float)j;
				if (isPointInside(core::vector2df(point1X, point1Y)))
				{
					// Si l'un des deux points est dans ce rectangle, alors ce rectangle s'étend bien sur cette case
					list.push_back(currentRect);
					continue;
				}

				// Vérifie si le second point formé est dans ce rectangle
				const float point2Y = (float)(j + 1);
				if (isPointInside(core::vector2df(point2X, point2Y)))
				{
					// Si l'un des deux points est dans ce rectangle, alors ce rectangle s'étend bien sur cette case
					list.push_back(currentRect);
				}
			}
		}

		return list;
	}

	obbox2df& operator=(const obbox2df& other)
	{
		set(other);

		return *this;
	}
	obbox2df& operator+=(const core::vector2df& pos)
	{
		A += pos;
		B += pos;
		C += pos;
		D += pos;
		return *this;
	}
	obbox2df& operator-=(const core::vector2df& pos)
	{
		A -= pos;
		B -= pos;
		C -= pos;
		D -= pos;
		return *this;
	}
	obbox2df operator+(const core::vector2df& pos) const
	{
		obbox2df box(*this);
		return box += pos;
	}
	obbox2df operator-(const core::vector2df& pos) const
	{
		obbox2df box(*this);
		return box -= pos;
	}
	bool operator==(const obbox2df& other) const
	{
		return (A == other.A && B == other.B && C == other.C && D == other.D);
	}
	bool operator!=(const obbox2df& other) const
	{
		return !(*this == other);
	}

	const core::vector2df& getA() const	{ return A; }
	const core::vector2df& getB() const	{ return B; }
	const core::vector2df& getC() const	{ return C; }
	const core::vector2df& getD() const	{ return D; }
	core::vector2df getCenter() const	{ return core::vector2df((C - A) * 0.5f + A); }
};

#endif
