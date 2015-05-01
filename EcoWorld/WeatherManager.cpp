#include "WeatherManager.h"
#include "EcoWorldSystem.h"
#ifdef USE_RAKNET
#include "RakNetManager.h"
#endif

// Macro pour interpoler linéairement une valeur entre deux valeurs (code trouvé ici : http://fr.wikipedia.org/wiki/Interpolation_num%C3%A9rique)
#define INTERPOLATE(v1, v2, mu)	(((float)(v1)) * (1.0f - (mu)) + ((float)(v2)) * (mu))

WeatherManager::WeatherManager(EcoWorldSystem* m_system)
 : system(m_system), currentWeatherID(WI_sunny), lastWeatherID(WI_sunny), newWeatherID(WI_sunny),
 currentWeatherInfos(WI_sunny), lastWeatherInfos(WI_sunny), newWeatherInfos(WI_sunny),
 transitionBeginTime(0.0f), interpolation(0.0f)
{
	reset();
}
void WeatherManager::reset()
{
	// Remet le temps actuel, l'ancien temps et le nouveau temps à leurs valeurs par défaut
	setWeather(WI_sunny);
}
void WeatherManager::setWeather(WeatherID newWeather)
{
	// Optimisation :
	// On n'assigne pas les temps actuel et ancien (ainsi que certaines variables dépendantes des transitions),
	// car ils seront assignés avec la valeur du nouveau temps durant le calcul de la nouvelle transition (dans WeatherManager::udpate())

	//currentWeatherID = newWeather;
	//lastWeatherID = newWeather;
	newWeatherID = newWeather;

	//currentWeatherInfos.setID(currentWeatherID);
	//lastWeatherInfos.setID(lastWeatherID);
	newWeatherInfos.setID(newWeatherID);

	//transitionBeginTime = 0.0f;
	//interpolation = 0.0f;

	// Force le calcul de la nouvelle transition, et assigne le temps actuel avec le nouveau temps désigné
	update(true);
}
void WeatherManager::changeWeather(WeatherID newWeather)
{
	if (!system)
	{
		setWeather(newWeather);
		return;
	}



	// Modifie la transition actuelle :

	// Indique le temps de départ de cette transition avec le temps du système de jeu actuel, pour qu'elle démarre immédiatement
	transitionBeginTime = system->getTime().getTotalTime();
	interpolation = 0.0f;

	// Recalcule le nouveau temps avec celui demandé
	newWeatherID = newWeather;
	newWeatherInfos.setID(newWeatherID);
}
void WeatherManager::save(io::IAttributes* out, io::IXMLWriter* writer) const
{
	if (!out || !writer)
		return;

	out->addInt("LastWeatherID", (int)lastWeatherID);
	out->addInt("NewWeatherID", (int)newWeatherID);

	out->addFloat("TransitionBeginTime", transitionBeginTime);

	out->write(writer, false, L"WeatherInfos");
	out->clear();
}
void WeatherManager::load(io::IAttributes* in, io::IXMLReader* reader)
{
	if (!in || !reader)
		return;

	reader->resetPosition();
	if (in->read(reader, false, L"WeatherInfos"))
	{
		if (in->existsAttribute("LastWeatherID"))			lastWeatherID = (WeatherID)(in->getAttributeAsInt("LastWeatherID"));
		if (in->existsAttribute("NewWeatherID"))			newWeatherID = (WeatherID)(in->getAttributeAsInt("NewWeatherID"));

		if (in->existsAttribute("TransitionBeginTime"))		transitionBeginTime = in->getAttributeAsFloat("TransitionBeginTime");

		in->clear();
	}

	lastWeatherInfos.setID(lastWeatherID);
	newWeatherInfos.setID(newWeatherID);

	interpolation = 0.0f;

	// Recalcule les informations sur le temps actuel
	//calculateCurrentWeatherInfos();
	// Désactivé : Ne peut pas être effectué maintenant car à ce moment du chargement, le temps du système de jeu est toujours à 0s, il sera chargé juste après :
	//				il est donc impossible d'interpoler le temps actuel pour le moment, cela sera effectué plus loin lors du chargement
	//	-> A la place, on réinitialise le temps actuel avec l'ancien temps
	currentWeatherID = lastWeatherID;
	currentWeatherInfos.setID(currentWeatherID);
}
void WeatherManager::update(bool forceNewTransition)
{
	if (!system)
		return;

	/*
	Fonctionnement du Weather Manager :
	-----------------------------------
	1. Au commencement de la partie, un premier temps (WI_sunny) est défini par défaut,
		et le temps du système de jeu auquel la première transition commencera est calculé au premier appel de update() en fournissant l'argument forceNewTransition à true.
	2. Lorsque ce temps système est atteint, le temps actuel change et on passe dans la phase de transition : lastWeather contient les informations sur l'ancien temps,
		newWeather contient les informations sur le nouveau temps, et currentWeather contient les informations du temps actuel :
		les données sont interpolées linéairement (dans calculateCurrentWeatherInfos()) entre lastWeather et newWeather suivant l'avancement actuel de la progression (variable "interpolation").
	3. Lorsque cette transition est terminée, on assigne le temps actuel avec le nouveau temps de cette transition, et on prépare la transition suivante :
		L'ancien temps est identique au temps actuel, et on détermine le nouveau temps du jeu auquel la prochaine transition commencera, ainsi que le nouveau temps qui sera choisi.
	4. Une fois le temps système pour le départ de la nouvelle transition atteint, le cycle reprend à partir de l'étape 2.
	*/



	// Le temps actuel du système de jeu
	const float currentTime = system->getTime().getTotalTime();

	// Le temps depuis le début de la transition (inférieur à 0.0f si la transition n'a pas encore commencée)
	const float currentTransitionTime = max(currentTime - transitionBeginTime, 0.0f);



	if (currentTransitionTime >= WEATHER_TRANSITION_TIME	// Le temps actuel de la transition est supérieur à la durée de la transition : la transition est terminée
		|| forceNewTransition)
	{
		// Transition terminée (ou forcée pour sa réinitialisation) :

		// Assigne le temps actuel et l'ancien temps avec ce nouveau temps, pour terminer cette transition et préparer complètement la suivante :

		// Indique le temps actuel avec ce nouveau temps
		currentWeatherID = newWeatherID;
		currentWeatherInfos.setID(currentWeatherID);

		// Indique aussi l'ancien temps avec ce nouveau temps (prépare la transition suivante)
		lastWeatherID = newWeatherID;
		lastWeatherInfos.setID(newWeatherID);



#ifdef USE_RAKNET
		// Si on est un client multijoueur de cette partie, on conserve le temps actuel et on attend les informations du serveur sur le nouveau temps
		const RakNetManager::E_NETWORK_GAME_STATE currentNetworkState = rkMgr.getCurrentNetworkState();
		if (currentNetworkState == RakNetManager::ENGS_PLAYING_GAME_CLIENT)
		{
			// Indique que le nouveau temps de départ de la transition est le temps actuel du jeu
			// (sinon, la fonction calculateCurrentWeatherInfos() risquerait d'être appelée avec une interpolation supérieure à 1.0f)
			transitionBeginTime = currentTime;
		}
		else	// Si on n'est pas un client multijoueur de cette partie (serveur ou partie non multijoueur), on calcule les informations sur le nouveau temps
#endif
		{
			// Choisit le nouveau temps et le temps du jeu auquel il apparaîtra :

#if defined(_DEBUG) && 0
			// Le prochain changement de temps sera dans un intervalle de temps de 10 à 20 secondes (temporaire pour tests en mode DEBUG !)
			transitionBeginTime = ((float)(rand() % 11) + 10.0f) + currentTime;
#else
			// Le prochain changement de temps sera dans un intervalle de temps de 60 à 300 secondes
			transitionBeginTime = ((float)(rand() % 241) + 60.0f) + currentTime;
#endif
			interpolation = 0.0f;														// Réinitialise l'interpolation actuelle entre les temps

			// Choisit le nouveau temps au hasard :

			/*	Ancienne méthode de choix du nouveau temps : peu optimisé et peu sécurisé :

			u32 nbIt = 0;	// Protection pour éviter une boucle infinie en cas de rand() défaillant
			do { newWeatherID = (WeatherID)(rand() % WI_COUNT); ++nbIt;
			} while (newWeatherID == lastWeatherID || nbIt > 10000);	// Retire un nouveau temps au hasard si le nouveau temps est le même que le temps actuel

			*/

			// Nouvelle méthode de choix du nouveau temps : très optimisé et sécurisé :
			// Laisse une possibilité de moins pour le choix du nouveau temps, et si le nouveau temps est égal à l'ancien temps,
			// on assigne alors le nouveau temps au dernier temps (il est impossible qu'il ait été choisi aléatoirement juste avant).
			// On préserve ici l'équiprobabilité dans le choix des temps, et on est certain que le nouveau temps choisi sera différent de l'ancien temps.
			// /!\ Attention :	On suppose tout de même ici qu'au moins deux temps différents sont créés dans la liste des temps (que WI_COUNT > 1).
			//					En effet, si WI_COUNT <= 1, alors (WI_COUNT - 1) = 0 et on effectue donc l'opération rand() % 0 !
			newWeatherID = (WeatherID)(rand() % (WI_COUNT - 1));
			if (newWeatherID == lastWeatherID)
				newWeatherID = (WeatherID)(WI_COUNT - 1);

			// Indique les informations sur le nouveau temps
			newWeatherInfos.setID(newWeatherID);

#ifdef USE_RAKNET
			// Si on est le serveur multijoueur de cette partie, on envoie le nouveau temps choisi aux clients
			if (currentNetworkState == RakNetManager::ENGS_PLAYING_GAME_HOST)
			{
				RakNetManager::RakNetGamePacket packet(RakNetManager::EGPT_WEATHER_CHOOSEN_NEW);
				packet.newChoosenWeatherInfos.weatherID = newWeatherID;
				packet.newChoosenWeatherInfos.transitionBeginTime = transitionBeginTime;
				rkMgr.sendPackets.push_back(packet);
			}
#endif
		}
	}
	else if (currentTransitionTime > 0.0f)					// Le temps actuel de la transition est positif : la transition a démarrée, mais n'est pas encore terminée
	{
		// Transition en cours :

		// Calcule le pourcentage actuel de la transition (entre 0.0f : transition démarrée ; et 1.0f : transition terminée)
		interpolation = currentTransitionTime * WEATHER_TRANSITION_TIME_INV;

		// Calcule l'interpolation du temps actuel d'après l'ancien temps et le nouveau temps
		calculateCurrentWeatherInfos();
	}

	// Si currentTransitionTime <= 0.0f	: Le temps actuel de la transition est négatif : elle n'a pas encore démarrée, donc on ne fait rien
}
void WeatherManager::calculateCurrentWeatherInfos()
{
	// On interpole linéairement le temps actuel entre l'ancien temps et le nouveau temps :

	// La variable interpolation a normalement déjà été calculée avant l'appel de cette fonction, il ne nous reste plus qu'à effectuer l'interpolation en elle-même
	const bool interpolateBool = (interpolation < 0.5f);	// Optimisation pour la détermination de l'interpolation entre des paramêtres qui ne peuvent être interpolés linéairement

	currentWeatherInfos.ID = lastWeatherID;
	currentWeatherID = (interpolateBool ? lastWeatherID : newWeatherID);

	currentWeatherInfos.fogColor.set(
		(u32)(INTERPOLATE(lastWeatherInfos.fogColor.getAlpha(), newWeatherInfos.fogColor.getAlpha(), interpolation)),
		(u32)(INTERPOLATE(lastWeatherInfos.fogColor.getRed(), newWeatherInfos.fogColor.getRed(), interpolation)),
		(u32)(INTERPOLATE(lastWeatherInfos.fogColor.getGreen(), newWeatherInfos.fogColor.getGreen(), interpolation)),
		(u32)(INTERPOLATE(lastWeatherInfos.fogColor.getBlue(), newWeatherInfos.fogColor.getBlue(), interpolation)));
	currentWeatherInfos.fogStart = INTERPOLATE(lastWeatherInfos.fogStart, newWeatherInfos.fogStart, interpolation);
	currentWeatherInfos.fogEnd = INTERPOLATE(lastWeatherInfos.fogEnd, newWeatherInfos.fogEnd, interpolation);

	currentWeatherInfos.ambientLight.set(
		INTERPOLATE(lastWeatherInfos.ambientLight.getAlpha(), newWeatherInfos.ambientLight.getAlpha(), interpolation),
		INTERPOLATE(lastWeatherInfos.ambientLight.getRed(), newWeatherInfos.ambientLight.getRed(), interpolation),
		INTERPOLATE(lastWeatherInfos.ambientLight.getGreen(), newWeatherInfos.ambientLight.getGreen(), interpolation),
		INTERPOLATE(lastWeatherInfos.ambientLight.getBlue(), newWeatherInfos.ambientLight.getBlue(), interpolation));
	currentWeatherInfos.sunLightColor.set(
		INTERPOLATE(lastWeatherInfos.sunLightColor.getAlpha(), newWeatherInfos.sunLightColor.getAlpha(), interpolation),
		INTERPOLATE(lastWeatherInfos.sunLightColor.getRed(), newWeatherInfos.sunLightColor.getRed(), interpolation),
		INTERPOLATE(lastWeatherInfos.sunLightColor.getGreen(), newWeatherInfos.sunLightColor.getGreen(), interpolation),
		INTERPOLATE(lastWeatherInfos.sunLightColor.getBlue(), newWeatherInfos.sunLightColor.getBlue(), interpolation));

	currentWeatherInfos.shadowColor.set(
		(u32)INTERPOLATE(lastWeatherInfos.shadowColor.getAlpha(), newWeatherInfos.shadowColor.getAlpha(), interpolation),
		(u32)INTERPOLATE(lastWeatherInfos.shadowColor.getRed(), newWeatherInfos.shadowColor.getRed(), interpolation),
		(u32)INTERPOLATE(lastWeatherInfos.shadowColor.getGreen(), newWeatherInfos.shadowColor.getGreen(), interpolation),
		(u32)INTERPOLATE(lastWeatherInfos.shadowColor.getBlue(), newWeatherInfos.shadowColor.getBlue(), interpolation));

	currentWeatherInfos.sunColor.set(
		(u32)INTERPOLATE(lastWeatherInfos.sunColor.getAlpha(), newWeatherInfos.sunColor.getAlpha(), interpolation),
		(u32)INTERPOLATE(lastWeatherInfos.sunColor.getRed(), newWeatherInfos.sunColor.getRed(), interpolation),
		(u32)INTERPOLATE(lastWeatherInfos.sunColor.getGreen(), newWeatherInfos.sunColor.getGreen(), interpolation),
		(u32)INTERPOLATE(lastWeatherInfos.sunColor.getBlue(), newWeatherInfos.sunColor.getBlue(), interpolation));

	currentWeatherInfos.skydomeTexturePath = (interpolateBool ? lastWeatherInfos.skydomeTexturePath : newWeatherInfos.skydomeTexturePath);
	currentWeatherInfos.skydomeTexturePercentage = (interpolateBool ? lastWeatherInfos.skydomeTexturePercentage : newWeatherInfos.skydomeTexturePercentage);
	currentWeatherInfos.skydomeTextureOffset = (interpolateBool ? lastWeatherInfos.skydomeTextureOffset : newWeatherInfos.skydomeTextureOffset);

	currentWeatherInfos.energyFactor = INTERPOLATE(lastWeatherInfos.energyFactor, newWeatherInfos.energyFactor, interpolation);

	currentWeatherInfos.rainFlow = INTERPOLATE(lastWeatherInfos.rainFlow, newWeatherInfos.rainFlow, interpolation);
}
/*
void WeatherManager::update()
{
	if (!system)
		return;

	/*
	Fonctionnement du Weather Manager :
	-----------------------------------
	1. Au commencement de la partie, un premier temps (WI_sunny) est défini par défaut et le temps du système de jeu auquel le temps changera est calculé au premier appel de update().
	2. Lorsque ce temps système est atteint, le temps actuel est changé et on passe dans la phase de transition : lastWeather contient les informations sur l'ancien temps,
		newWeather contient les informations sur le nouveau temps, et currentWeather contient les informations du temps actuel :
		les données interpolées (dans calculateCurrentWeatherInfos()) entre lastWeather et newWeather suivant l'avancement actuel de la progression (variable "interpolation").
		On recalcule aussi le temps système auquel le temps changera à nouveau.
	3. La transition se termine simplement, en ayant finalement currentWeather = newWeather (effectué dans calculateCurrentWeatherInfos() : if (currentTime >= newWeatherTime)).
	4. Une fois le temps système pour le changement du temps atteint, le cycle reprend à partir de l'étape 2.
	/

	// Le temps du système de jeu actuel
	const float currentTime = system->getTime().getTotalTime();

	// Si le temps prévu pour le nouveau changement est dépassé
	if (currentTime >= nextWeatherChangeTime && nextWeatherChangeTime > 0.0f)	// Vérifie aussi que nextWeatherChangeTime n'est pas à recalculer
	{
		// On change le temps actuel
		lastWeatherTime = currentTime;
		newWeatherTime = currentTime + WEATHER_TRANSITION_TIME;

		// On indique qu'on doit recalculer quand arrivera le prochain temps
		nextWeatherChangeTime = 0.0f;

		lastWeatherID = currentWeatherID;

		// Choisi le nouveau temps au hasard
		do { newWeatherID = (WeatherID)(rand() % WI_COUNT);
		} while (newWeatherID == currentWeatherID); // Retire un nouveau temps au hasard si le nouveau temps est le même que le temps actuel

		lastWeatherInfos.setID(lastWeatherID);
		newWeatherInfos.setID(newWeatherID);
		interpolation = 0.0f;

		LOG_DEBUG("Changement de temps : newWeatherID = " << newWeatherID, ELL_INFORMATION);
	}

	// Vérifie si on doit recalculer quand  arrivera le prochain temps
	if (nextWeatherChangeTime <= 0.0f)
	{
		// Le prochain changement de temps sera dans un intervalle de temps de 60 à 300 secondes + la durée de la transition pour permettre à la transition en cours de se terminer
		//nextWeatherChangeTime = ((float)(rand() % 241) + 60.0f) + WEATHER_TRANSITION_TIME + currentTime;
		nextWeatherChangeTime = ((float)(rand() % 11) + 10.0f) + WEATHER_TRANSITION_TIME + currentTime; // Intervalle de 10 à 20 secondes + la durée de la transition
	}

	// Recalcule les informations actuelles du temps suivant le dernier temps et le nouveau temps
	calculateCurrentWeatherInfos();
}
void WeatherManager::calculateCurrentWeatherInfos()
{
	if (!system)
		return;

	// Le temps du système de jeu actuel
	const float currentTime = system->getTime().getTotalTime();

	if (currentTime >= newWeatherTime) // Le temps prévu pour le nouveau temps est dépassé : la transition est terminée
	{
		// On indique que le temps actuel est le nouveau temps
		if (currentWeatherInfos.ID != newWeatherID)	// On ne vérifie pas "currentWeatherID != newWeatherID" car "currentWeatherID" est assigné lors du cas "currentTime > lastWeatherTime" !
		{
			currentWeatherID = newWeatherID;
			currentWeatherInfos.setID(currentWeatherID);
			interpolation = 1.0f;
		}

		// Transition terminée :
		if (currentWeatherInfos.ID != newWeatherID)	// On ne vérifie pas "currentWeatherID != newWeatherID" car "currentWeatherID" est assigné lors du cas "currentTime > lastWeatherTime" !
		{
			// Indique le nouveau temps comme temps actuel (stocké dans l'ancien temps à la fin de cette transition, pour pouvoir préparer la transition suivante)
			lastWeatherID = newWeatherID;
			lastWeatherInfos.setID(weatherID);

			// Indique aussi le temps actuel avec ce temps
			currentWeatherID = newWeatherID;
			currentWeatherInfos.setID(currentWeatherID);



			// Choisis le nouveau temps et le temps du jeu auquel il apparaîtra :

			// Le prochain changement de temps sera dans un intervalle de temps de 60 à 300 secondes
			//nextWeatherChangeTime = ((float)(rand() % 241) + 60.0f) + currentTime;
			nextWeatherChangeTime = ((float)(rand() % 11) + 10.0f) + currentTime; // Intervalle de 10 à 20 secondes

			// On change le temps actuel
			lastWeatherTime = currentTime;
			newWeatherTime = currentTime + WEATHER_TRANSITION_TIME;

			lastWeatherID = currentWeatherID;

			// Choisi le nouveau temps au hasard
			do { newWeatherID = (WeatherID)(rand() % WI_COUNT);
			} while (newWeatherID == currentWeatherID); // Retire un nouveau temps au hasard si le nouveau temps est le même que le temps actuel
		}
	}
	else if (currentTime > lastWeatherTime) // On a dépassé le temps de changement du temps
	{
		// On interpole le temps actuel entre l'ancien temps et le nouveau temps
		interpolation = (currentTime - lastWeatherTime) / (newWeatherTime - lastWeatherTime);
		const bool interpolateBool = (interpolation < 0.5f);	// Optimisation pour la détermination de l'interpolation entre des paramêtres qui ne peuvent être interpolés linéairement

		currentWeatherInfos.ID = lastWeatherID;
		currentWeatherID = (interpolateBool ? lastWeatherID : newWeatherID);

		currentWeatherInfos.fogColor.set(
			(u32)(INTERPOLATE(lastWeatherInfos.fogColor.getAlpha(), newWeatherInfos.fogColor.getAlpha(), interpolation)),
			(u32)(INTERPOLATE(lastWeatherInfos.fogColor.getRed(), newWeatherInfos.fogColor.getRed(), interpolation)),
			(u32)(INTERPOLATE(lastWeatherInfos.fogColor.getGreen(), newWeatherInfos.fogColor.getGreen(), interpolation)),
			(u32)(INTERPOLATE(lastWeatherInfos.fogColor.getBlue(), newWeatherInfos.fogColor.getBlue(), interpolation)));
		currentWeatherInfos.fogStart = INTERPOLATE(lastWeatherInfos.fogStart, newWeatherInfos.fogStart, interpolation);
		currentWeatherInfos.fogEnd = INTERPOLATE(lastWeatherInfos.fogEnd, newWeatherInfos.fogEnd, interpolation);

		currentWeatherInfos.ambientLight.set(
			INTERPOLATE(lastWeatherInfos.ambientLight.getAlpha(), newWeatherInfos.ambientLight.getAlpha(), interpolation),
			INTERPOLATE(lastWeatherInfos.ambientLight.getRed(), newWeatherInfos.ambientLight.getRed(), interpolation),
			INTERPOLATE(lastWeatherInfos.ambientLight.getGreen(), newWeatherInfos.ambientLight.getGreen(), interpolation),
			INTERPOLATE(lastWeatherInfos.ambientLight.getBlue(), newWeatherInfos.ambientLight.getBlue(), interpolation));
		currentWeatherInfos.sunLightColor.set(
			INTERPOLATE(lastWeatherInfos.sunLightColor.getAlpha(), newWeatherInfos.sunLightColor.getAlpha(), interpolation),
			INTERPOLATE(lastWeatherInfos.sunLightColor.getRed(), newWeatherInfos.sunLightColor.getRed(), interpolation),
			INTERPOLATE(lastWeatherInfos.sunLightColor.getGreen(), newWeatherInfos.sunLightColor.getGreen(), interpolation),
			INTERPOLATE(lastWeatherInfos.sunLightColor.getBlue(), newWeatherInfos.sunLightColor.getBlue(), interpolation));

		currentWeatherInfos.shadowColor.set(
			(u32)INTERPOLATE(lastWeatherInfos.shadowColor.getAlpha(), newWeatherInfos.shadowColor.getAlpha(), interpolation),
			(u32)INTERPOLATE(lastWeatherInfos.shadowColor.getRed(), newWeatherInfos.shadowColor.getRed(), interpolation),
			(u32)INTERPOLATE(lastWeatherInfos.shadowColor.getGreen(), newWeatherInfos.shadowColor.getGreen(), interpolation),
			(u32)INTERPOLATE(lastWeatherInfos.shadowColor.getBlue(), newWeatherInfos.shadowColor.getBlue(), interpolation));

		currentWeatherInfos.sunColor.set(
			(u32)INTERPOLATE(lastWeatherInfos.sunColor.getAlpha(), newWeatherInfos.sunColor.getAlpha(), interpolation),
			(u32)INTERPOLATE(lastWeatherInfos.sunColor.getRed(), newWeatherInfos.sunColor.getRed(), interpolation),
			(u32)INTERPOLATE(lastWeatherInfos.sunColor.getGreen(), newWeatherInfos.sunColor.getGreen(), interpolation),
			(u32)INTERPOLATE(lastWeatherInfos.sunColor.getBlue(), newWeatherInfos.sunColor.getBlue(), interpolation));

		currentWeatherInfos.skydomeTexturePath = (interpolateBool ? lastWeatherInfos.skydomeTexturePath : newWeatherInfos.skydomeTexturePath);
		currentWeatherInfos.skydomeTexturePercentage = (interpolateBool ? lastWeatherInfos.skydomeTexturePercentage : newWeatherInfos.skydomeTexturePercentage);
		currentWeatherInfos.skydomeTextureOffset = (interpolateBool ? lastWeatherInfos.skydomeTextureOffset : newWeatherInfos.skydomeTextureOffset);

		currentWeatherInfos.energyFactor = INTERPOLATE(lastWeatherInfos.energyFactor, newWeatherInfos.energyFactor, interpolation);

		currentWeatherInfos.rainFlow = INTERPOLATE(lastWeatherInfos.rainFlow, newWeatherInfos.rainFlow, interpolation);
	}
	else	// N'arrive que lorsque le temps vient juste de commencer sa transition (currentTime == lastWeatherTime) : Optimisation de ci-dessus car interpolation = 0.0f
	{
		currentWeatherID = lastWeatherID;
		currentWeatherInfos.setID(currentWeatherID);
		interpolation = 0.0f;
	}
}
*/