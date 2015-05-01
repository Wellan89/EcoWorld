#include "WeatherManager.h"
#include "EcoWorldSystem.h"
#ifdef USE_RAKNET
#include "RakNetManager.h"
#endif

// Macro pour interpoler lin�airement une valeur entre deux valeurs (code trouv� ici : http://fr.wikipedia.org/wiki/Interpolation_num%C3%A9rique)
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
	// Remet le temps actuel, l'ancien temps et le nouveau temps � leurs valeurs par d�faut
	setWeather(WI_sunny);
}
void WeatherManager::setWeather(WeatherID newWeather)
{
	// Optimisation :
	// On n'assigne pas les temps actuel et ancien (ainsi que certaines variables d�pendantes des transitions),
	// car ils seront assign�s avec la valeur du nouveau temps durant le calcul de la nouvelle transition (dans WeatherManager::udpate())

	//currentWeatherID = newWeather;
	//lastWeatherID = newWeather;
	newWeatherID = newWeather;

	//currentWeatherInfos.setID(currentWeatherID);
	//lastWeatherInfos.setID(lastWeatherID);
	newWeatherInfos.setID(newWeatherID);

	//transitionBeginTime = 0.0f;
	//interpolation = 0.0f;

	// Force le calcul de la nouvelle transition, et assigne le temps actuel avec le nouveau temps d�sign�
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

	// Indique le temps de d�part de cette transition avec le temps du syst�me de jeu actuel, pour qu'elle d�marre imm�diatement
	transitionBeginTime = system->getTime().getTotalTime();
	interpolation = 0.0f;

	// Recalcule le nouveau temps avec celui demand�
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
	// D�sactiv� : Ne peut pas �tre effectu� maintenant car � ce moment du chargement, le temps du syst�me de jeu est toujours � 0s, il sera charg� juste apr�s :
	//				il est donc impossible d'interpoler le temps actuel pour le moment, cela sera effectu� plus loin lors du chargement
	//	-> A la place, on r�initialise le temps actuel avec l'ancien temps
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
	1. Au commencement de la partie, un premier temps (WI_sunny) est d�fini par d�faut,
		et le temps du syst�me de jeu auquel la premi�re transition commencera est calcul� au premier appel de update() en fournissant l'argument forceNewTransition � true.
	2. Lorsque ce temps syst�me est atteint, le temps actuel change et on passe dans la phase de transition : lastWeather contient les informations sur l'ancien temps,
		newWeather contient les informations sur le nouveau temps, et currentWeather contient les informations du temps actuel :
		les donn�es sont interpol�es lin�airement (dans calculateCurrentWeatherInfos()) entre lastWeather et newWeather suivant l'avancement actuel de la progression (variable "interpolation").
	3. Lorsque cette transition est termin�e, on assigne le temps actuel avec le nouveau temps de cette transition, et on pr�pare la transition suivante :
		L'ancien temps est identique au temps actuel, et on d�termine le nouveau temps du jeu auquel la prochaine transition commencera, ainsi que le nouveau temps qui sera choisi.
	4. Une fois le temps syst�me pour le d�part de la nouvelle transition atteint, le cycle reprend � partir de l'�tape 2.
	*/



	// Le temps actuel du syst�me de jeu
	const float currentTime = system->getTime().getTotalTime();

	// Le temps depuis le d�but de la transition (inf�rieur � 0.0f si la transition n'a pas encore commenc�e)
	const float currentTransitionTime = max(currentTime - transitionBeginTime, 0.0f);



	if (currentTransitionTime >= WEATHER_TRANSITION_TIME	// Le temps actuel de la transition est sup�rieur � la dur�e de la transition : la transition est termin�e
		|| forceNewTransition)
	{
		// Transition termin�e (ou forc�e pour sa r�initialisation) :

		// Assigne le temps actuel et l'ancien temps avec ce nouveau temps, pour terminer cette transition et pr�parer compl�tement la suivante :

		// Indique le temps actuel avec ce nouveau temps
		currentWeatherID = newWeatherID;
		currentWeatherInfos.setID(currentWeatherID);

		// Indique aussi l'ancien temps avec ce nouveau temps (pr�pare la transition suivante)
		lastWeatherID = newWeatherID;
		lastWeatherInfos.setID(newWeatherID);



#ifdef USE_RAKNET
		// Si on est un client multijoueur de cette partie, on conserve le temps actuel et on attend les informations du serveur sur le nouveau temps
		const RakNetManager::E_NETWORK_GAME_STATE currentNetworkState = rkMgr.getCurrentNetworkState();
		if (currentNetworkState == RakNetManager::ENGS_PLAYING_GAME_CLIENT)
		{
			// Indique que le nouveau temps de d�part de la transition est le temps actuel du jeu
			// (sinon, la fonction calculateCurrentWeatherInfos() risquerait d'�tre appel�e avec une interpolation sup�rieure � 1.0f)
			transitionBeginTime = currentTime;
		}
		else	// Si on n'est pas un client multijoueur de cette partie (serveur ou partie non multijoueur), on calcule les informations sur le nouveau temps
#endif
		{
			// Choisit le nouveau temps et le temps du jeu auquel il appara�tra :

#if defined(_DEBUG) && 0
			// Le prochain changement de temps sera dans un intervalle de temps de 10 � 20 secondes (temporaire pour tests en mode DEBUG !)
			transitionBeginTime = ((float)(rand() % 11) + 10.0f) + currentTime;
#else
			// Le prochain changement de temps sera dans un intervalle de temps de 60 � 300 secondes
			transitionBeginTime = ((float)(rand() % 241) + 60.0f) + currentTime;
#endif
			interpolation = 0.0f;														// R�initialise l'interpolation actuelle entre les temps

			// Choisit le nouveau temps au hasard :

			/*	Ancienne m�thode de choix du nouveau temps : peu optimis� et peu s�curis� :

			u32 nbIt = 0;	// Protection pour �viter une boucle infinie en cas de rand() d�faillant
			do { newWeatherID = (WeatherID)(rand() % WI_COUNT); ++nbIt;
			} while (newWeatherID == lastWeatherID || nbIt > 10000);	// Retire un nouveau temps au hasard si le nouveau temps est le m�me que le temps actuel

			*/

			// Nouvelle m�thode de choix du nouveau temps : tr�s optimis� et s�curis� :
			// Laisse une possibilit� de moins pour le choix du nouveau temps, et si le nouveau temps est �gal � l'ancien temps,
			// on assigne alors le nouveau temps au dernier temps (il est impossible qu'il ait �t� choisi al�atoirement juste avant).
			// On pr�serve ici l'�quiprobabilit� dans le choix des temps, et on est certain que le nouveau temps choisi sera diff�rent de l'ancien temps.
			// /!\ Attention :	On suppose tout de m�me ici qu'au moins deux temps diff�rents sont cr��s dans la liste des temps (que WI_COUNT > 1).
			//					En effet, si WI_COUNT <= 1, alors (WI_COUNT - 1) = 0 et on effectue donc l'op�ration rand() % 0 !
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
	else if (currentTransitionTime > 0.0f)					// Le temps actuel de la transition est positif : la transition a d�marr�e, mais n'est pas encore termin�e
	{
		// Transition en cours :

		// Calcule le pourcentage actuel de la transition (entre 0.0f : transition d�marr�e ; et 1.0f : transition termin�e)
		interpolation = currentTransitionTime * WEATHER_TRANSITION_TIME_INV;

		// Calcule l'interpolation du temps actuel d'apr�s l'ancien temps et le nouveau temps
		calculateCurrentWeatherInfos();
	}

	// Si currentTransitionTime <= 0.0f	: Le temps actuel de la transition est n�gatif : elle n'a pas encore d�marr�e, donc on ne fait rien
}
void WeatherManager::calculateCurrentWeatherInfos()
{
	// On interpole lin�airement le temps actuel entre l'ancien temps et le nouveau temps :

	// La variable interpolation a normalement d�j� �t� calcul�e avant l'appel de cette fonction, il ne nous reste plus qu'� effectuer l'interpolation en elle-m�me
	const bool interpolateBool = (interpolation < 0.5f);	// Optimisation pour la d�termination de l'interpolation entre des param�tres qui ne peuvent �tre interpol�s lin�airement

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
	1. Au commencement de la partie, un premier temps (WI_sunny) est d�fini par d�faut et le temps du syst�me de jeu auquel le temps changera est calcul� au premier appel de update().
	2. Lorsque ce temps syst�me est atteint, le temps actuel est chang� et on passe dans la phase de transition : lastWeather contient les informations sur l'ancien temps,
		newWeather contient les informations sur le nouveau temps, et currentWeather contient les informations du temps actuel :
		les donn�es interpol�es (dans calculateCurrentWeatherInfos()) entre lastWeather et newWeather suivant l'avancement actuel de la progression (variable "interpolation").
		On recalcule aussi le temps syst�me auquel le temps changera � nouveau.
	3. La transition se termine simplement, en ayant finalement currentWeather = newWeather (effectu� dans calculateCurrentWeatherInfos() : if (currentTime >= newWeatherTime)).
	4. Une fois le temps syst�me pour le changement du temps atteint, le cycle reprend � partir de l'�tape 2.
	/

	// Le temps du syst�me de jeu actuel
	const float currentTime = system->getTime().getTotalTime();

	// Si le temps pr�vu pour le nouveau changement est d�pass�
	if (currentTime >= nextWeatherChangeTime && nextWeatherChangeTime > 0.0f)	// V�rifie aussi que nextWeatherChangeTime n'est pas � recalculer
	{
		// On change le temps actuel
		lastWeatherTime = currentTime;
		newWeatherTime = currentTime + WEATHER_TRANSITION_TIME;

		// On indique qu'on doit recalculer quand arrivera le prochain temps
		nextWeatherChangeTime = 0.0f;

		lastWeatherID = currentWeatherID;

		// Choisi le nouveau temps au hasard
		do { newWeatherID = (WeatherID)(rand() % WI_COUNT);
		} while (newWeatherID == currentWeatherID); // Retire un nouveau temps au hasard si le nouveau temps est le m�me que le temps actuel

		lastWeatherInfos.setID(lastWeatherID);
		newWeatherInfos.setID(newWeatherID);
		interpolation = 0.0f;

		LOG_DEBUG("Changement de temps : newWeatherID = " << newWeatherID, ELL_INFORMATION);
	}

	// V�rifie si on doit recalculer quand  arrivera le prochain temps
	if (nextWeatherChangeTime <= 0.0f)
	{
		// Le prochain changement de temps sera dans un intervalle de temps de 60 � 300 secondes + la dur�e de la transition pour permettre � la transition en cours de se terminer
		//nextWeatherChangeTime = ((float)(rand() % 241) + 60.0f) + WEATHER_TRANSITION_TIME + currentTime;
		nextWeatherChangeTime = ((float)(rand() % 11) + 10.0f) + WEATHER_TRANSITION_TIME + currentTime; // Intervalle de 10 � 20 secondes + la dur�e de la transition
	}

	// Recalcule les informations actuelles du temps suivant le dernier temps et le nouveau temps
	calculateCurrentWeatherInfos();
}
void WeatherManager::calculateCurrentWeatherInfos()
{
	if (!system)
		return;

	// Le temps du syst�me de jeu actuel
	const float currentTime = system->getTime().getTotalTime();

	if (currentTime >= newWeatherTime) // Le temps pr�vu pour le nouveau temps est d�pass� : la transition est termin�e
	{
		// On indique que le temps actuel est le nouveau temps
		if (currentWeatherInfos.ID != newWeatherID)	// On ne v�rifie pas "currentWeatherID != newWeatherID" car "currentWeatherID" est assign� lors du cas "currentTime > lastWeatherTime" !
		{
			currentWeatherID = newWeatherID;
			currentWeatherInfos.setID(currentWeatherID);
			interpolation = 1.0f;
		}

		// Transition termin�e :
		if (currentWeatherInfos.ID != newWeatherID)	// On ne v�rifie pas "currentWeatherID != newWeatherID" car "currentWeatherID" est assign� lors du cas "currentTime > lastWeatherTime" !
		{
			// Indique le nouveau temps comme temps actuel (stock� dans l'ancien temps � la fin de cette transition, pour pouvoir pr�parer la transition suivante)
			lastWeatherID = newWeatherID;
			lastWeatherInfos.setID(weatherID);

			// Indique aussi le temps actuel avec ce temps
			currentWeatherID = newWeatherID;
			currentWeatherInfos.setID(currentWeatherID);



			// Choisis le nouveau temps et le temps du jeu auquel il appara�tra :

			// Le prochain changement de temps sera dans un intervalle de temps de 60 � 300 secondes
			//nextWeatherChangeTime = ((float)(rand() % 241) + 60.0f) + currentTime;
			nextWeatherChangeTime = ((float)(rand() % 11) + 10.0f) + currentTime; // Intervalle de 10 � 20 secondes

			// On change le temps actuel
			lastWeatherTime = currentTime;
			newWeatherTime = currentTime + WEATHER_TRANSITION_TIME;

			lastWeatherID = currentWeatherID;

			// Choisi le nouveau temps au hasard
			do { newWeatherID = (WeatherID)(rand() % WI_COUNT);
			} while (newWeatherID == currentWeatherID); // Retire un nouveau temps au hasard si le nouveau temps est le m�me que le temps actuel
		}
	}
	else if (currentTime > lastWeatherTime) // On a d�pass� le temps de changement du temps
	{
		// On interpole le temps actuel entre l'ancien temps et le nouveau temps
		interpolation = (currentTime - lastWeatherTime) / (newWeatherTime - lastWeatherTime);
		const bool interpolateBool = (interpolation < 0.5f);	// Optimisation pour la d�termination de l'interpolation entre des param�tres qui ne peuvent �tre interpol�s lin�airement

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