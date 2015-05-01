#ifndef DEF_WEATHER_MANAGER
#define DEF_WEATHER_MANAGER

#include "Weathers.h"

class EcoWorldSystem;



#define WEATHER_TRANSITION_TIME		20.0f	// La dur�e (en seconde) des transitions entre les temps du syst�me de jeu
#define WEATHER_TRANSITION_TIME_INV	0.05f	// 1.0f / WEATHER_TRANSITION_TIME



// Classe permettant la gestion des temps du syst�me d'EcoWorld, en acceptant diff�rents temps et leurs transitions
class WeatherManager
{
public:
	// Constructeur
	WeatherManager(EcoWorldSystem* m_system = NULL);

	// Met � jour le temps actuel (ne n�c�ssite pas le temps �coul� depuis la derni�re frame)
	// forceNewTransition :	Force la fin de la transition actuelle (en assignant le temps actuel avec le nouveau temps), et recalcule compl�tement la nouvelle transition
	void update(bool forceNewTransition = false);

	// Remet le Weather Manager � z�ro
	void reset();

	// Change imm�diatement le temps actuel (sans transition)
	void setWeather(WeatherID newWeather);

	// Change le temps actuel (avec une transition)
	void changeWeather(WeatherID newWeather);

	// Enregistre les donn�es du temps dans un fichier
	void save(io::IAttributes* out, io::IXMLWriter* writer) const;

	// Charge les donn�es du temps � partir d'un fichier
	void load(io::IAttributes* in, io::IXMLReader* reader);

protected:
	// Interpole lin�airement les informations sur le temps actuel pendant les transitions,
	// d'apr�s la valeur actuelle de l'interpolation (cette variable doit avoir �t� calcul�e juste avant) et les donn�es de l'ancien et du nouveau temps
	void calculateCurrentWeatherInfos();

	// Pointeur vers le syst�me de jeu
	EcoWorldSystem* system;

	// L'ID et les caract�ristiques de l'ancien temps et du nouveau temps
	// L'ancien temps sert de temps actuel lorsque la transition n'a pas encore commenc�e : le nouveau temps n'est donc utilis� que pendant les transitions
	WeatherID lastWeatherID;
	WeatherInfos lastWeatherInfos;
	WeatherID newWeatherID;
	WeatherInfos newWeatherInfos;

	// L'ID et les caract�ristiques du temps actuel (interpolation entre l'ancien temps et le nouveau temps pendant les transitions, �gal � l'ancien temps en dehors de celles-ci)
	WeatherID currentWeatherID;
	WeatherInfos currentWeatherInfos;

	// Le temps du jeu auquel la prochaine transition d�marrera (ou a d�marr�e)
	float transitionBeginTime;

	// L'interpolation actuelle entre l'ancien temps et le nouveau temps (entre 0.0f : transition juste d�marr�e, ou non encore commenc�e ; et 1.0f : transition termin�e)
	// Cette variable doit �tre valide � chaque appel de calculateCurrentWeatherInfos(), car la fonction se base dessus pour effectuer ses interpolations
	float interpolation;

public:
	// Accesseurs inline :

	// Modifie le syst�me de jeu actuel
	void setSystem(EcoWorldSystem* m_system)			{ system = m_system; }

	// Obtient les informations sur les temps actuel, ancien et nouveau
	const WeatherInfos& getCurrentWeatherInfos() const	{ return currentWeatherInfos; }
	WeatherID getCurrentWeatherID() const				{ return currentWeatherID; }
	const WeatherInfos& getLastWeatherInfos() const		{ return lastWeatherInfos; }
	WeatherID getLastWeatherID() const					{ return lastWeatherID; }
	const WeatherInfos& getNewWeatherInfos() const		{ return newWeatherInfos; }
	WeatherID getNewWeatherID() const					{ return newWeatherID; }

	// Retourne l'interpolation actuelle entre l'ancien temps et le nouveau temps
	float getCurrentInterpolation() const				{ return interpolation; }

	// D�termine si le son d'ambiance de la pluie doit �tre jou� ou non
	bool isRainSoundNeeded() const
	{
		if (interpolation > 0.0f)
			return (newWeatherID == WI_raining);	// Le prochain temps est la pluie et la transition est en cours
		return (currentWeatherID == WI_raining);	// OU : Le temps actuel est la pluie et la transition n'est pas en cours
	}

#ifdef USE_RAKNET
	// Indique les donn�es du nouveau temps : son ID et son nouveau temps de transition
	// Cette m�thode doit seulement �tre utilis�e lorsque RakNet est en mode client : pour forcer les clients � adopter le m�me nouveau temps du serveur
	void setNewWeather(WeatherID weatherID, float newTransitionBeginTime)
	{
		newWeatherID = weatherID;
		newWeatherInfos.setID(weatherID);
		transitionBeginTime = newTransitionBeginTime;
	}
#endif
};

#endif
