#ifndef DEF_C_GUI_CAMERA_ANIMATOR
#define DEF_C_GUI_CAMERA_ANIMATOR

#include "global.h"
#include "CGUIWindow.h"

using namespace gui;

// Fen�tre de la GUI permettant au joueur d'animer la cam�ra RTS suivant une courbe personnalis�e
class CGUICameraAnimatorWindow : public CGUIWindow
{
public:
	// Constructeur et destructeur
	CGUICameraAnimatorWindow(IGUIEnvironment* environment, IGUIElement* parent, int id, const core::recti& rectangle);
	virtual ~CGUICameraAnimatorWindow();

	// Appel� lors d'un �v�nement
	virtual bool OnEvent(const SEvent& event);

	// Dessine cette fen�tre, en centrant sa barre de titre (bas� sur la fonction CGUIWindow::draw() d'Irrlicht SVN 1.8.0-alpha)
	virtual void draw();

	// Anime la cam�ra RTS gr�ce � l'animator d�fini par cette fen�tre
	void addAnimatorToCamera();

	// Supprime cet animator de la cam�ra RTS
	void removeAnimatorFromCamera();

	// R�initialise cette animation
	void reset();

	// Met � jour cette fen�tre et l'animation de la cam�ra RTS (doit �tre appel� � chaque frame du jeu, m�me lorsque la cam�ra RTS est masqu�e)
	void udpate();

	// Charge l'animation personnalis�e � partir d'un fichier
	void load(io::IAttributes* in, io::IXMLReader* reader);

	// Enregistre l'animation personnalis�e dans un fichier
	void save(io::IAttributes* out, io::IXMLWriter* writer) const;

	// Retourne true si une edit box ou la list box de cette fen�tre a le focus :
	// Il vaut mieux d�sactiver les raccourcis claviers du jeu dans ce cas
	bool hasKeyFocus() const;

protected:
	// Met � jour compl�tement la GUI de cette fen�tre lorsque l'animation de la fen�tre a compl�tement chang�e
	void updateGUI();



	// La liste des points de l'animation personnalis�e
	core::list<core::vector3df> animationPoints;

	// La vitesse de l'animation personnalis�e
	float animationSpeed;

	// La tension de la ligne de l'animation personnalis�e (TODO : entre 0.0f et 1.0f ?)
	float animationTightness;

	// True si l'animation cr��e sera r�p�t�e � l'infini, false sinon
	bool animationLoop;

	// True si l'animation cr��e sera r�p�t�e � l'envers, false sinon
	bool animationPingPong;



	// L'animator cr�� et ajout� � la cam�ra RTS
	scene::ISceneNodeAnimator* cameraRTSAnimator;

	// True si la cam�ra FPS �tait activ�e avant l'animation de la cam�ra RTS, false sinon
	bool wasFPSCameraEnabled;

	// True si la cam�ra RTS �tait bloqu�e avant son animation, false sinon
	bool wasCameraLocked;

	// True si la GUI du jeu �tait visible avant l'animation de la cam�ra RTS, false sinon
	bool wasGameGUIVisible;



	// Les �lements de la GUI permettant de personnaliser cette animation :
	IGUIButton* animateCameraButton;	// Le bouton pour cr�er l'animation actuellement d�finie et l'ajouter � la cam�ra RTS

	IGUIStaticText* totalTimeText;		// Le texte de description pour le temps total de l'animation
	IGUIEditBox* totalTimeEditBox;		// La zone de texte permettant de d�finir le temps total de l'animation
	IGUIStaticText* speedText;			// Le texte de description pour la vitesse de l'animation
	IGUIEditBox* speedEditBox;			// La zone de texte permettant de d�finir la vitesse de l'animation
	IGUIStaticText* tightnessText;		// Le texte de description pour la tension de la ligne de l'animation
	IGUIEditBox* tightnessEditBox;		// La zone de texte permettant de d�finir la tension de la ligne de l'animation
	IGUICheckBox* loopCheckBox;			// La case � cocher pour la r�p�tition de l'animation
	IGUICheckBox* pingPongCheckBox;		// La case � cocher pour la r�p�tition en sens inverse de l'animation

	IGUIStaticText* pointsText;			// Le texte de description pour la liste des points de l'animation
	IGUIListBox* pointsListBox;			// La liste permettant de g�rer la liste des points de l'animation
	IGUIButton* pointAddButton;			// Le bouton pour ajouter un point � l'animation
	IGUIButton* pointRemoveButton;		// Le bouton pour supprimer un point de l'animation

	IGUIStaticText* pointXText;			// Le texte de description pour la coordonn�e X du point de l'animation s�lectionn�
	IGUIEditBox* pointXEditBox;			// La zone de texte permettant de d�finir la coordonn�e X du point de l'animation s�lectionn�
	IGUIStaticText* pointYText;			// Le texte de description pour la coordonn�e Y du point de l'animation s�lectionn�
	IGUIEditBox* pointYEditBox;			// La zone de texte permettant de d�finir la coordonn�e Y du point de l'animation s�lectionn�
	IGUIStaticText* pointZText;			// Le texte de description pour la coordonn�e Z du point de l'animation s�lectionn�
	IGUIEditBox* pointZEditBox;			// La zone de texte permettant de d�finir la coordonn�e Z du point de l'animation s�lectionn�
	IGUIButton* pointGoToButton;		// Le bouton pour placer la cam�ra RTS au point s�lectionn�



	// Fonctions permettant de g�rer la modification de l'animation :
	void setCurrentSelectedPoint(int index);	// Indique quel point de la liste est actuellement s�lectionn�
	void addPoint();							// Ajoute un point � la liste et le s�lectionne
	void removePoint(int index);				// Supprime un point de cette liste et modifie le point s�lectionn� de la liste de la GUI
	void changePointX(float pointX);			// Change la coordonn�e X du point s�lectionn�
	void changePointY(float pointY);			// Change la coordonn�e Y du point s�lectionn�
	void changePointZ(float pointZ);			// Change la coordonn�e Z du point s�lectionn�


	
	// Fonctions utiles :

	// Calcule le temps total de l'animation (en secondes) d'apr�s sa vitesse (en points/seconde) et son nombre de points
	float calculateAnimationTotalTime() const
	{
		return (animationSpeed > 0.0f ? ((float)(animationPoints.size()) / animationSpeed) : 0.0f);
	}

	// Calcule la vitesse de l'animation (en points/seconde) d'apr�s son temps total (en secondes) et son nombre de points
	float calculateAnimationSpeed(float animationTotalTime) const
	{
		return (animationTotalTime > 0.0f ? ((float)(animationPoints.size()) / animationTotalTime) : 0.0f);
	}

	// Obtient l'index du point actuellement s�lectionn� dans la liste
	int getSelectedPoint() const
	{
		if (pointsListBox)
			return pointsListBox->getSelected();
		return -1;
	}

	// Obtient les coordonn�es 3D d'un certain point de la liste (d�fini d'apr�s son index dans la liste de la GUI)
	core::vector3df getPoint(int index) const
	{
		// V�rifie que cet index est valide, sinon on retourne un point par d�faut
		if (index < 0 || index >= (int)(animationPoints.size()))
			return core::vector3df();

		// Retourne le point avec l'index indiqu�
		core::list<core::vector3df>::ConstIterator it = animationPoints.begin();
		it += index;
		return (*it);
	}

	// Retourne la valeur num�rique (sous forme de float) contenue dans une zone de texte
	float getEditBoxValue(IGUIEditBox* editBox) const;



public:
	// Accesseurs inline :

	// Retourne true si l'animation de la cam�ra RTS est activ�e, false sinon
	bool isCameraAnimationEnabled() const	{ return (cameraRTSAnimator != NULL); }
};

#endif
