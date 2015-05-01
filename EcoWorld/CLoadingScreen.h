#ifndef DEF_LOADING_SCREEN
#define DEF_LOADING_SCREEN

#include "global.h"

// Classe permettant d'afficher un écran de chargement autonome (non dépendant du scene manager ou du GUI Manager)
class CLoadingScreen
{
public:
	// Constructeur et destructeur
	CLoadingScreen();
	~CLoadingScreen();

	// Dessine cet écran de chargement
	// Retourne true si le chargement doit impérativement s'arrêter : si la fenêtre du jeu a été fermée !
	bool draw();

	// Prépare l'écran de chargement à être dessiné
	void beginLoading();

	// Indique que le chargement est terminé
	void endLoading();

protected:
	// Choisi une texture d'un écran de chargement depuis le système de fichier du device d'Irrlicht
	video::ITexture* chooseLoadingScreen();

	// L'avancement actuel de la loading bar (pourcentage = percent / maxPercent)
	float percent;

	// L'avancement maximal de la loading bar
	float maxPercent;

	// True si le chargement du jeu a commencé et ne s'est pas encore terminé
	bool loadingBegun;

	// La zone rectangulaire où sera dessinée la barre de chargement
	core::recti loadingScreenRect;

	// La texture de fond qui sera dessinée pendant le chargement du jeu
	video::ITexture* backgroundTexture;

	// La police avec laquelle sera dessiné le texte indiquant que le chargement du jeu est en cours
	gui::IGUIFont* loadingFont;

public:
	// Accesseurs inline :

	// Obtient l'avancement actuel de la loading bar
	float getPercent() const				{ return percent; }
	// Indique l'avancement actuel de la loading bar
	void setPercent(float Percent)			{ percent = Percent; }

	// Obtient la valeur maximale actuelle de la loading bar
	float getMaxPercent() const				{ return maxPercent; }
	// Indique la valeur maximale actuelle de la loading bar
	void setMaxPercent(float MaxPercent)	{ maxPercent = MaxPercent; }

	// Indique le pourcentage actuel de la barre de chargement et dessine la barre de chargement
	// Retourne true si le chargement doit impérativement s'arrêter : si la fenêtre du jeu a été fermée !
	bool setPercentAndDraw(float Percent)
	{
		percent = Percent;
		return draw();
	}
};

#endif
