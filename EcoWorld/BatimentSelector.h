#ifndef DEF_BATIMENT_SELECTOR
#define DEF_BATIMENT_SELECTOR

#include "global.h"
#include "Batiments.h"

class Batiment;
class CBatimentSceneNode;

// Classe pour gérer le placement et la sélection de bâtiments sur le terrain 3D du monde d'après les évènements de la souris
class BatimentSelector
{
public:
	// Constructeur et destructeur
	BatimentSelector()	{ }
	~BatimentSelector()	{ }
	
	// Gère les évènements dans la scène du jeu pour gérer les contrôles de placement ou de sélection de bâtiments
	// Retourne true si l'évènement a été géré (et n'est donc plus à gérer par Game), false sinon
	bool OnEvent(const SEvent& event);

	// Annule la sélection actuelle (construction/destruction/sélection d'un bâtiment)
	void annulerSelection();

	// Réinitialise le placement et la sélection de bâtiments (annule ainsi la sélection actuelle)
	void reset();

	// Ajoute les pointeurs du jeu susceptibles de pointer sur un scene node invalide (sur le point de se détruire) à la liste fournie
	void pushBackGamePointers(core::list<CBatimentSceneNode**>& gameSceneNodesPointers);

	// Met à jour le placement et la sélection actuelle du bâtiment
	void update();

protected:
	// Met à jour la GUI du jeu suivant le placement et la sélection actuelle
	void updateGameGUI();

	// Obtient le scene node du bâtiment actuellement pointé par la souris (ou à la position écran spécifiée dans mousePos)
	static scene::ISceneNode* getPointedBat(const core::vector2di& mousePos = core::vector2di(-1, -1));

	// Obtient la position du terrain actuellement pointée par la souris (ou à la position écran spécifiée dans mousePos)
	static core::vector3df getPointedTerrainPos(const core::vector2di& mousePos = core::vector2di(-1, -1));



	// Classe gérant le placement de bâtiments (construction et destruction)
	struct PlacerBatiment
	{
	public:
		// Contient les derniers paramêtres fournis lors de l'appel à pB_frameUpdate : permet de n'actualiser cette fonction que si l'un de ces paramêtres a changé
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

		BatimentID currentBatiment;						// Le bâtiment actuellement sélectionné pour la construction
		CBatimentSceneNode* samePosBatiment;			// Le bâtiment du système de jeu (déjà créé) à la même position et avec la même rotation que le bâtiment de prévisualisation (voir note dans previewCreateSimpleBat)
		CBatimentSceneNode* pointedToDestroyBatiment;	// Le bâtiment actuellement sélectionné (pointé) pour la destruction

		scene::ISceneNode* moveBatiment;				// Le bâtiment qui montrera à l'utilisateur où sera crée le batiment s'il clique ici
		float currentBatimentRotation;					// La rotation en Y du batiment qui va être placé

#ifdef CAN_BUILD_WITH_SELECTION_RECT
		// Indique la sélection rectangulaire actuelle (si égal à core::recti(-1, -1, -1, -1), alors aucune sélection rectangulaire n'est en cours)
		core::recti rectSelection;

		// Le nombre de bâtiments prévus lors de la prévisualisation en rectangle (seulement valide lorsqu'une sélection rectangulaire est en cours)
		u32 nbBatimentsPreviewRect;

		// La liste des batiments utilisés pour la prévisualisation des batiments en zone rectangulaire
		core::array<scene::ISceneNode*> listeBatimentsPreviewRect;
#endif



		// Fonctions à utiliser en externe :

		// Prévisualise le placement ou la destruction d'un batiment
		void frameUpdate();
		// Valide le placement ou la destruction d'un batiment
		void validateAction();

	protected:
		// Fonctions utilisées en interne :

		// Met à jour le bâtiment de prévisualisation
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
		void showSimpleCreationErrors(const core::vector3df* messagesPosition, scene::ISceneNode* batiment, int error, bool outRessources[] = 0);	// messagesPosition est facultatif : à utiliser pour créer des messages d'erreur à une position 3D donnée
		void showSimpleDestructionErrors(const core::vector3df* messagesPosition, const scene::ISceneNode* batiment, int error, bool outRessources[] = 0);	// messagesPosition est facultatif : à utiliser pour créer des messages d'erreur à une position 3D donnée
#ifdef CAN_BUILD_WITH_SELECTION_RECT
		void showRectCreationErrors(const core::vector3df* messagesPosition, core::array<scene::ISceneNode*>* batiments, int error, bool outRessources[] = 0);	// messagesPosition est facultatif : à utiliser pour créer des messages d'erreur à une position 3D donnée
		//void showRectDestructionErrors(const core::vector3df* messagesPosition, core::array<scene::ISceneNode*>* batiments, int error, bool outRessources[] = 0);	// messagesPosition est facultatif : à utiliser pour créer des messages d'erreur à une position 3D donnée
#endif



	public:
		// Réinitialise cette structure
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



	// Classe gérant la sélection de bâtiments
	struct SelectBatiment
	{
	public:
		// Contient les derniers paramêtres fournis lors de l'appel à sB_frameUpdate : permet de n'actualiser cette fonction que si l'un de ces paramêtres a changé
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

		// Le bâtiment actuellement sélectionné
		CBatimentSceneNode* selectedBatiment;

		// Le batiment actuellement pointé déterminé lors de la prévisualisation sB_frameUpdate()
		CBatimentSceneNode* pointedBatiment;



		// Fonctions à utiliser en externe :

		// Affiche la prévisualisation du bâtiment actuellement pointé
		void frameUpdate();
		// Valide la sélection du bâtiment actuellement pointé
		void selectBatiment();
		// Désélectionne le bâtiment actuellement sélectionné
		void deselectBatiment();



		// Réinitialise cette structure
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

	// Obtient le type actuel du bâtiment à construire (ou BI_aucun si aucun bâtiment n'est en construction)
	BatimentID getCurrentCreatingBat() const	{ return placerBatiment.currentBatiment; }

	// Obtient le bâtiment actuellement sélectionné
	CBatimentSceneNode* getSelectedBat()		{ return selectBatiment.selectedBatiment; }

	// Indique le type du bâtiment actuel à placer
	void setCurrentBatimentID(BatimentID batID)	{ placerBatiment.currentBatiment = batID; }

#ifdef CAN_BUILD_WITH_SELECTION_RECT
	// Obtient le triangle de sélection actuel
	const core::recti& getRectSelection() const	{ return placerBatiment.rectSelection; }
#endif

	// Obtient si un placement est en cours de placement
	inline bool isPlacingBat() const;

	// Obtient si un bâtiment est en cours de placement ou de sélection
	bool isPlacingOrSelectingBat() const		{ return (selectBatiment.selectedBatiment || isPlacingBat()); }
};

#endif
