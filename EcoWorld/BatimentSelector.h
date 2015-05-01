#ifndef DEF_BATIMENT_SELECTOR
#define DEF_BATIMENT_SELECTOR

#include "global.h"
#include "Batiments.h"

class Batiment;
class CBatimentSceneNode;

// Classe pour g�rer le placement et la s�lection de b�timents sur le terrain 3D du monde d'apr�s les �v�nements de la souris
class BatimentSelector
{
public:
	// Constructeur et destructeur
	BatimentSelector()	{ }
	~BatimentSelector()	{ }
	
	// G�re les �v�nements dans la sc�ne du jeu pour g�rer les contr�les de placement ou de s�lection de b�timents
	// Retourne true si l'�v�nement a �t� g�r� (et n'est donc plus � g�rer par Game), false sinon
	bool OnEvent(const SEvent& event);

	// Annule la s�lection actuelle (construction/destruction/s�lection d'un b�timent)
	void annulerSelection();

	// R�initialise le placement et la s�lection de b�timents (annule ainsi la s�lection actuelle)
	void reset();

	// Ajoute les pointeurs du jeu susceptibles de pointer sur un scene node invalide (sur le point de se d�truire) � la liste fournie
	void pushBackGamePointers(core::list<CBatimentSceneNode**>& gameSceneNodesPointers);

	// Met � jour le placement et la s�lection actuelle du b�timent
	void update();

protected:
	// Met � jour la GUI du jeu suivant le placement et la s�lection actuelle
	void updateGameGUI();

	// Obtient le scene node du b�timent actuellement point� par la souris (ou � la position �cran sp�cifi�e dans mousePos)
	static scene::ISceneNode* getPointedBat(const core::vector2di& mousePos = core::vector2di(-1, -1));

	// Obtient la position du terrain actuellement point�e par la souris (ou � la position �cran sp�cifi�e dans mousePos)
	static core::vector3df getPointedTerrainPos(const core::vector2di& mousePos = core::vector2di(-1, -1));



	// Classe g�rant le placement de b�timents (construction et destruction)
	struct PlacerBatiment
	{
	public:
		// Contient les derniers param�tres fournis lors de l'appel � pB_frameUpdate : permet de n'actualiser cette fonction que si l'un de ces param�tres a chang�
		struct FrameUpdateParams
		{
		public:
			// Construction simple et en rectangle, et destruction :
			u32 systemTotalDays;
			core::vector2di mousePos;
			core::vector3df camPos;

			// Construction simple et en rectangle :
			BatimentID batimentID;
			float batimentRotation;

			// Construction en rectangle seulement :
#ifdef CAN_BUILD_WITH_SELECTION_RECT
			core::recti rectSelection;
			core::vector2di pointedFirstCorner;
#if !defined(FORCE_RECTANGLE) || !defined(ONLY_TAKE_MAIN_CORNERS)
			core::vector2di pointedSecondCorner;
			core::vector2di pointedThirdCorner;
#endif
			core::vector2di pointedLastCorner;
#endif

			bool hasConstructSimpleParamsChanged(u32 newSystemTotalDays, BatimentID newBatimentID, float newBatimentRotation, const core::vector2di& newMousePos,
				const core::vector3df& newCamPos) const
			{
				return (systemTotalDays != newSystemTotalDays || batimentID != newBatimentID || batimentRotation != newBatimentRotation || mousePos != newMousePos || newCamPos != camPos);
			}
#ifdef CAN_BUILD_WITH_SELECTION_RECT
			bool hasConstructRectParamsChanged(u32 newSystemTotalDays, BatimentID newBatimentID, float newBatimentRotation, const core::vector2di& newMousePos,
				const core::vector3df& newCamPos, const core::recti& newRectSelection) const
			{
				return (systemTotalDays != newSystemTotalDays || batimentID != newBatimentID || batimentRotation != newBatimentRotation || mousePos != newMousePos || newCamPos != camPos
					|| rectSelection != newRectSelection);
			}
			bool hasConstructRectCornersChanged(u32 newSystemTotalDays, BatimentID newBatimentID, float newBatimentRotation, const core::vector2di& newPointedFirstCorner,
#if !defined(FORCE_RECTANGLE) || !defined(ONLY_TAKE_MAIN_CORNERS)
				const core::vector2di& newPointedSecondCorner, const core::vector2di& newPointedThirdCorner,
#endif
				const core::vector2di& newPointedLastCorner) const
			{
				return (systemTotalDays != newSystemTotalDays || batimentID != newBatimentID || batimentRotation != newBatimentRotation || pointedFirstCorner != newPointedFirstCorner
#if !defined(FORCE_RECTANGLE) || !defined(ONLY_TAKE_MAIN_CORNERS)
					|| pointedSecondCorner != newPointedSecondCorner || pointedThirdCorner != newPointedThirdCorner
#endif
					|| pointedLastCorner != newPointedLastCorner);
			}
#endif
			bool hasDestroySimpleParamsChanged(u32 newSystemTotalDays, const core::vector2di& newMousePos, const core::vector3df& newCamPos) const
			{
				return (systemTotalDays != newSystemTotalDays || mousePos != newMousePos || newCamPos != camPos);
			}
			void reset()
			{
				systemTotalDays = 0;
				mousePos.set(-1, -1);
				camPos.set(-1.0f, -1.0f, -1.0f);

				batimentID = BI_aucun;
				batimentRotation = -1.0f;

#ifdef CAN_BUILD_WITH_SELECTION_RECT
				rectSelection = core::recti(-1, -1, -1, -1);
				pointedFirstCorner.set(-1, -1);
#if !defined(FORCE_RECTANGLE) || !defined(ONLY_TAKE_MAIN_CORNERS)
				pointedSecondCorner.set(-1, -1);
				pointedThirdCorner.set(-1, -1);
#endif
				pointedLastCorner.set(-1, -1);
#endif
			}
			FrameUpdateParams() { reset(); }
		} lastUpdateParams;

		BatimentID currentBatiment;						// Le b�timent actuellement s�lectionn� pour la construction
		CBatimentSceneNode* samePosBatiment;			// Le b�timent du syst�me de jeu (d�j� cr��) � la m�me position et avec la m�me rotation que le b�timent de pr�visualisation (voir note dans previewCreateSimpleBat)
		CBatimentSceneNode* pointedToDestroyBatiment;	// Le b�timent actuellement s�lectionn� (point�) pour la destruction

		scene::ISceneNode* moveBatiment;				// Le b�timent qui montrera � l'utilisateur o� sera cr�e le batiment s'il clique ici
		float currentBatimentRotation;					// La rotation en Y du batiment qui va �tre plac�

#ifdef CAN_BUILD_WITH_SELECTION_RECT
		// Indique la s�lection rectangulaire actuelle (si �gal � core::recti(-1, -1, -1, -1), alors aucune s�lection rectangulaire n'est en cours)
		core::recti rectSelection;

		// Le nombre de b�timents pr�vus lors de la pr�visualisation en rectangle (seulement valide lorsqu'une s�lection rectangulaire est en cours)
		u32 nbBatimentsPreviewRect;

		// La liste des batiments utilis�s pour la pr�visualisation des batiments en zone rectangulaire
		core::array<scene::ISceneNode*> listeBatimentsPreviewRect;
#endif



		// Fonctions � utiliser en externe :

		// Pr�visualise le placement ou la destruction d'un batiment
		void frameUpdate();
		// Valide le placement ou la destruction d'un batiment
		void validateAction();

	protected:
		// Fonctions utilis�es en interne :

		// Met � jour le b�timent de pr�visualisation
		void updatePrevisualisationBat();

		void previewCreateSimpleBat(BatimentID batimentID, scene::ISceneNode* batiment, core::vector2di index = core::vector2di(-1, -1));
		void previewDestroySimpleBat(core::vector2di index = core::vector2di(-1, -1));
		void createSimpleBat(BatimentID batimentID, core::vector2di index = core::vector2di(-1, -1));
		void destroySimpleBat(core::vector2di index = core::vector2di(-1, -1));
#ifdef CAN_BUILD_WITH_SELECTION_RECT
		void previewCreateRectBats(BatimentID batimentID);
		//void previewDestroyRectBats();
		void createRectBats(BatimentID batimentID);
		//void destroyRectBats();
#endif

		void resetPreviewErrors(scene::ISceneNode* batiment = 0);
		void showSimpleCreationErrors(const core::vector3df* messagesPosition, scene::ISceneNode* batiment, int error, bool outRessources[] = 0);	// messagesPosition est facultatif : � utiliser pour cr�er des messages d'erreur � une position 3D donn�e
		void showSimpleDestructionErrors(const core::vector3df* messagesPosition, const scene::ISceneNode* batiment, int error, bool outRessources[] = 0);	// messagesPosition est facultatif : � utiliser pour cr�er des messages d'erreur � une position 3D donn�e
#ifdef CAN_BUILD_WITH_SELECTION_RECT
		void showRectCreationErrors(const core::vector3df* messagesPosition, core::array<scene::ISceneNode*>* batiments, int error, bool outRessources[] = 0);	// messagesPosition est facultatif : � utiliser pour cr�er des messages d'erreur � une position 3D donn�e
		//void showRectDestructionErrors(const core::vector3df* messagesPosition, core::array<scene::ISceneNode*>* batiments, int error, bool outRessources[] = 0);	// messagesPosition est facultatif : � utiliser pour cr�er des messages d'erreur � une position 3D donn�e
#endif



	public:
		// R�initialise cette structure
		void reset()
		{
			currentBatiment = BI_aucun;
			samePosBatiment = NULL;
			pointedToDestroyBatiment = NULL;
			moveBatiment = NULL;
			currentBatimentRotation = 0.0f;
#ifdef CAN_BUILD_WITH_SELECTION_RECT
			rectSelection = core::recti(-1, -1, -1, -1);
			nbBatimentsPreviewRect = 0;
			listeBatimentsPreviewRect.clear();
#endif
			lastUpdateParams.reset();
		}

		// Constructeur
		PlacerBatiment()	{ reset(); }
	} placerBatiment;



	// Classe g�rant la s�lection de b�timents
	struct SelectBatiment
	{
	public:
		// Contient les derniers param�tres fournis lors de l'appel � sB_frameUpdate : permet de n'actualiser cette fonction que si l'un de ces param�tres a chang�
		struct FrameUpdateParams
		{
			core::vector2di mousePos;
			core::vector3df camPos;

			bool hasParamsChanged(const core::vector2di& newMousePos, const core::vector3df& newCamPos) const
			{
				return (mousePos != newMousePos || newCamPos != camPos);
			}
			void reset()
			{
				mousePos.set(-1, -1);
				camPos.set(-1.0f, -1.0f, -1.0f);
			}
			FrameUpdateParams() { reset(); }
		} lastUpdateParams;

		// Le b�timent actuellement s�lectionn�
		CBatimentSceneNode* selectedBatiment;

		// Le batiment actuellement point� d�termin� lors de la pr�visualisation sB_frameUpdate()
		CBatimentSceneNode* pointedBatiment;



		// Fonctions � utiliser en externe :

		// Affiche la pr�visualisation du b�timent actuellement point�
		void frameUpdate();
		// Valide la s�lection du b�timent actuellement point�
		void selectBatiment();
		// D�s�lectionne le b�timent actuellement s�lectionn�
		void deselectBatiment();



		// R�initialise cette structure
		void reset()
		{
			selectedBatiment = NULL;
			pointedBatiment = NULL;
			lastUpdateParams.reset();
		}

		// Constructeur
		SelectBatiment()	{ reset(); }
	} selectBatiment;


public:
	// Accesseurs et modificateurs inline :

	// Obtient le type actuel du b�timent � construire (ou BI_aucun si aucun b�timent n'est en construction)
	BatimentID getCurrentCreatingBat() const	{ return placerBatiment.currentBatiment; }

	// Obtient le b�timent actuellement s�lectionn�
	CBatimentSceneNode* getSelectedBat()		{ return selectBatiment.selectedBatiment; }

	// Indique le type du b�timent actuel � placer
	void setCurrentBatimentID(BatimentID batID)	{ placerBatiment.currentBatiment = batID; }

#ifdef CAN_BUILD_WITH_SELECTION_RECT
	// Obtient le triangle de s�lection actuel
	const core::recti& getRectSelection() const	{ return placerBatiment.rectSelection; }
#endif

	// Obtient si un placement est en cours de placement
	inline bool isPlacingBat() const;

	// Obtient si un b�timent est en cours de placement ou de s�lection
	bool isPlacingOrSelectingBat() const		{ return (selectBatiment.selectedBatiment || isPlacingBat()); }
};

#endif
