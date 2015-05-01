#ifndef DEF_WEATHER_MANAGER
#define DEF_WEATHER_MANAGER

#include "Weathers.h"

class EcoWorldSystem;



#define WEATHER_TRANSITION_TIME		20.0f	// La durée (en seconde) des transitions entre les temps du système de jeu
#define WEATHER_TRANSITION_TIME_INV	0.05f	// 1.0f / WEATHER_TRANSITION_TIME



// Classe permettant la gestion des temps du système d'EcoWorld, en acceptant différents temps et leurs transitions
class WeatherManager
{
public:
	// Constructeur
	WeatherManager(EcoWorldSystem* m_system = NULL);

	// Met à jour le temps actuel (ne nécéssite pas le temps écoulé depuis la dernière frame)
	// forceNewTransition :	Force la fin de la transition actuelle (en assignant le temps actuel avec le nouveau temps), et recalcule complètement la nouvelle transition
	void update(bool forceNewTransition = false);

	// Remet le Weather Manager à zéro
	void reset();

	// Change immédiatement le temps actuel (sans transition)
	void setWeather(WeatherID newWeather);

	// Change le temps actuel (avec une transition)
	void changeWeather(WeatherID newWeather);

	// Enregistre les données du temps dans un fichier
	void save(io::IAttributes* out, io::IXMLWriter* writer) const;

	// Charge les données du temps à partir d'un fichier
	void load(io::IAttributes* in, io::IXMLReader* reader);

protected:
	// Interpole linéairement les informations sur le temps actuel pendant les transitions,
	// d'après la valeur actuelle de l'interpolation (cette variable doit avoir été calculée juste avant) et les données de l'ancien et du nouveau temps
	void calculateCurrentWeatherInfos();

	// Pointeur vers le système de jeu
	EcoWorldSystem* system;

	// L'ID et les caractéristiques de l'ancien temps et du nouveau temps
	// L'ancien temps sert de temps actuel lorsque la transition n'a pas encore commencée : le nouveau temps n'est donc utilisé que pendant les transitions
	WeatherID lastWeatherID;
	WeatherInfos lastWeatherInfos;
	WeatherID newWeatherID;
	WeatherInfos newWeatherInfos;

	// L'ID et les caractéristiques du temps actuel (interpolation entre l'ancien temps et le nouveau temps pendant les transitions, égal à l'ancien temps en dehors de celles-ci)
	WeatherID currentWeatherID;
	WeatherInfos currentWeatherInfos;

	// Le temps du jeu auquel la prochaine transition démarrera (ou a démarrée)
	float transitionBeginTime;

	// L'interpolation actuelle entre l'ancien temps et le nouveau temps (entre 0.0f : transition juste démarrée, ou non encore commencée ; et 1.0f : transition terminée)
	// Cette variable doit être valide à chaque appel de calculateCurrentWeatherInfos(), car la fonction se base dessus pour effectuer ses interpolations
	float interpolation;

public:
	// Accesseurs inline :

	// Modifie le système de jeu actuel
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

	// Détermine si le son d'ambiance de la pluie doit être joué ou non
	bool isRainSoundNeeded() const
	{
		if (interpolation > 0.0f)
			return (newWeatherID == WI_raining);	// Le prochain temps est la pluie et la transition est en cours
		return (currentWeatherID == WI_raining);	// OU : Le temps actuel est la pluie et la transition n'est pas en cours
	}

#ifdef USE_RAKNET
	// Indique les données du nouveau temps : son ID et son nouveau temps de transition
	// Cette méthode doit seulement être utilisée lorsque RakNet est en mode client : pour forcer les clients à adopter le même nouveau temps du serveur
	void setNewWeather(WeatherID weatherID, float newTransitionBeginTime)
	{
		newWeatherID = weatherID;
		newWeatherInfos.setID(weatherID);
		transitionBeginTime = newTransitionBeginTime;
	}
#endif
};

#endif
