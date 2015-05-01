#ifndef DEF_C_GUI_CAMERA_ANIMATOR
#define DEF_C_GUI_CAMERA_ANIMATOR

#include "global.h"
#include "CGUIWindow.h"

using namespace gui;

// Fenêtre de la GUI permettant au joueur d'animer la caméra RTS suivant une courbe personnalisée
class CGUICameraAnimatorWindow : public CGUIWindow
{
public:
	// Constructeur et destructeur
	CGUICameraAnimatorWindow(IGUIEnvironment* environment, IGUIElement* parent, int id, const core::recti& rectangle);
	virtual ~CGUICameraAnimatorWindow();

	// Appelé lors d'un évènement
	virtual bool OnEvent(const SEvent& event);

	// Dessine cette fenêtre, en centrant sa barre de titre (basé sur la fonction CGUIWindow::draw() d'Irrlicht SVN 1.8.0-alpha)
	virtual void draw();

	// Anime la caméra RTS grâce à l'animator défini par cette fenêtre
	void addAnimatorToCamera();

	// Supprime cet animator de la caméra RTS
	void removeAnimatorFromCamera();

	// Réinitialise cette animation
	void reset();

	// Met à jour cette fenêtre et l'animation de la caméra RTS (doit être appelé à chaque frame du jeu, même lorsque la caméra RTS est masquée)
	void udpate();

	// Charge l'animation personnalisée à partir d'un fichier
	void load(io::IAttributes* in, io::IXMLReader* reader);

	// Enregistre l'animation personnalisée dans un fichier
	void save(io::IAttributes* out, io::IXMLWriter* writer) const;

	// Retourne true si une edit box ou la list box de cette fenêtre a le focus :
	// Il vaut mieux désactiver les raccourcis claviers du jeu dans ce cas
	bool hasKeyFocus() const;

protected:
	// Met à jour complètement la GUI de cette fenêtre lorsque l'animation de la fenêtre a complètement changée
	void updateGUI();



	// La liste des points de l'animation personnalisée
	core::list<core::vector3df> animationPoints;

	// La vitesse de l'animation personnalisée
	float animationSpeed;

	// La tension de la ligne de l'animation personnalisée (TODO : entre 0.0f et 1.0f ?)
	float animationTightness;

	// True si l'animation créée sera répétée à l'infini, false sinon
	bool animationLoop;

	// True si l'animation créée sera répétée à l'envers, false sinon
	bool animationPingPong;



	// L'animator créé et ajouté à la caméra RTS
	scene::ISceneNodeAnimator* cameraRTSAnimator;

	// True si la caméra FPS était activée avant l'animation de la caméra RTS, false sinon
	bool wasFPSCameraEnabled;

	// True si la caméra RTS était bloquée avant son animation, false sinon
	bool wasCameraLocked;

	// True si la GUI du jeu était visible avant l'animation de la caméra RTS, false sinon
	bool wasGameGUIVisible;



	// Les élements de la GUI permettant de personnaliser cette animation :
	IGUIButton* animateCameraButton;	// Le bouton pour créer l'animation actuellement définie et l'ajouter à la caméra RTS

	IGUIStaticText* totalTimeText;		// Le texte de description pour le temps total de l'animation
	IGUIEditBox* totalTimeEditBox;		// La zone de texte permettant de définir le temps total de l'animation
	IGUIStaticText* speedText;			// Le texte de description pour la vitesse de l'animation
	IGUIEditBox* speedEditBox;			// La zone de texte permettant de définir la vitesse de l'animation
	IGUIStaticText* tightnessText;		// Le texte de description pour la tension de la ligne de l'animation
	IGUIEditBox* tightnessEditBox;		// La zone de texte permettant de définir la tension de la ligne de l'animation
	IGUICheckBox* loopCheckBox;			// La case à cocher pour la répétition de l'animation
	IGUICheckBox* pingPongCheckBox;		// La case à cocher pour la répétition en sens inverse de l'animation

	IGUIStaticText* pointsText;			// Le texte de description pour la liste des points de l'animation
	IGUIListBox* pointsListBox;			// La liste permettant de gérer la liste des points de l'animation
	IGUIButton* pointAddButton;			// Le bouton pour ajouter un point à l'animation
	IGUIButton* pointRemoveButton;		// Le bouton pour supprimer un point de l'animation

	IGUIStaticText* pointXText;			// Le texte de description pour la coordonnée X du point de l'animation sélectionné
	IGUIEditBox* pointXEditBox;			// La zone de texte permettant de définir la coordonnée X du point de l'animation sélectionné
	IGUIStaticText* pointYText;			// Le texte de description pour la coordonnée Y du point de l'animation sélectionné
	IGUIEditBox* pointYEditBox;			// La zone de texte permettant de définir la coordonnée Y du point de l'animation sélectionné
	IGUIStaticText* pointZText;			// Le texte de description pour la coordonnée Z du point de l'animation sélectionné
	IGUIEditBox* pointZEditBox;			// La zone de texte permettant de définir la coordonnée Z du point de l'animation sélectionné
	IGUIButton* pointGoToButton;		// Le bouton pour placer la caméra RTS au point sélectionné



	// Fonctions permettant de gérer la modification de l'animation :
	void setCurrentSelectedPoint(int index);	// Indique quel point de la liste est actuellement sélectionné
	void addPoint();							// Ajoute un point à la liste et le sélectionne
	void removePoint(int index);				// Supprime un point de cette liste et modifie le point sélectionné de la liste de la GUI
	void changePointX(float pointX);			// Change la coordonnée X du point sélectionné
	void changePointY(float pointY);			// Change la coordonnée Y du point sélectionné
	void changePointZ(float pointZ);			// Change la coordonnée Z du point sélectionné


	
	// Fonctions utiles :

	// Calcule le temps total de l'animation (en secondes) d'après sa vitesse (en points/seconde) et son nombre de points
	float calculateAnimationTotalTime() const
	{
		return (animationSpeed > 0.0f ? ((float)(animationPoints.size()) / animationSpeed) : 0.0f);
	}

	// Calcule la vitesse de l'animation (en points/seconde) d'après son temps total (en secondes) et son nombre de points
	float calculateAnimationSpeed(float animationTotalTime) const
	{
		return (animationTotalTime > 0.0f ? ((float)(animationPoints.size()) / animationTotalTime) : 0.0f);
	}

	// Obtient l'index du point actuellement sélectionné dans la liste
	int getSelectedPoint() const
	{
		if (pointsListBox)
			return pointsListBox->getSelected();
		return -1;
	}

	// Obtient les coordonnées 3D d'un certain point de la liste (défini d'après son index dans la liste de la GUI)
	core::vector3df getPoint(int index) const
	{
		// Vérifie que cet index est valide, sinon on retourne un point par défaut
		if (index < 0 || index >= (int)(animationPoints.size()))
			return core::vector3df();

		// Retourne le point avec l'index indiqué
		core::list<core::vector3df>::ConstIterator it = animationPoints.begin();
		it += index;
		return (*it);
	}

	// Retourne la valeur numérique (sous forme de float) contenue dans une zone de texte
	float getEditBoxValue(IGUIEditBox* editBox) const;



public:
	// Accesseurs inline :

	// Retourne true si l'animation de la caméra RTS est activée, false sinon
	bool isCameraAnimationEnabled() const	{ return (cameraRTSAnimator != NULL); }
};

#endif
