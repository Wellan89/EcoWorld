#ifndef DEF_LOADING_SCREEN
#define DEF_LOADING_SCREEN

#include "global.h"

// Classe permettant d'afficher un �cran de chargement autonome (non d�pendant du scene manager ou du GUI Manager)
class CLoadingScreen
{
public:
	// Constructeur et destructeur
	CLoadingScreen();
	~CLoadingScreen();

	// Dessine cet �cran de chargement
	// Retourne true si le chargement doit imp�rativement s'arr�ter : si la fen�tre du jeu a �t� ferm�e !
	bool draw();

	// Pr�pare l'�cran de chargement � �tre dessin�
	void beginLoading();

	// Indique que le chargement est termin�
	void endLoading();

protected:
	// Choisi une texture d'un �cran de chargement depuis le syst�me de fichier du device d'Irrlicht
	video::ITexture* chooseLoadingScreen();

	// L'avancement actuel de la loading bar (pourcentage = percent / maxPercent)
	float percent;

	// L'avancement maximal de la loading bar
	float maxPercent;

	// True si le chargement du jeu a commenc� et ne s'est pas encore termin�
	bool loadingBegun;

	// La zone rectangulaire o� sera dessin�e la barre de chargement
	core::recti loadingScreenRect;

	// La texture de fond qui sera dessin�e pendant le chargement du jeu
	video::ITexture* backgroundTexture;

	// La police avec laquelle sera dessin� le texte indiquant que le chargement du jeu est en cours
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
	// Retourne true si le chargement doit imp�rativement s'arr�ter : si la fen�tre du jeu a �t� ferm�e !
	bool setPercentAndDraw(float Percent)
	{
		percent = Percent;
		return draw();
	}
};

#endif
