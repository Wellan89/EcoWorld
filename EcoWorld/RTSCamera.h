// Site : http://irrlicht.sourceforge.net/phpBB2/viewtopic.php?t=17066
// (Cette version a �t� tr�s largement modifi�e)

#ifndef __RTSCamera__
#define __RTSCamera__

#include "global.h"

using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

struct MouseState;
class EcoWorldRenderer;

class RTSCamera : public ICameraSceneNode
{
public:
	RTSCamera(ISceneNode* parent, int id = -1,
		float rotateSpeed = -500.0f, float zoomStep = 100.0f, float zoomSpeed = 350.0f, float translationSpeed = 500.0f,
		scene::ICameraSceneNode* FPSCamera = NULL);
	virtual ~RTSCamera();

	//Events
	virtual void render();
	virtual bool OnEvent(const SEvent& event);
	virtual void OnRegisterSceneNode();
	virtual void OnAnimate(u32 timeMs);

	//Helper Functions
	virtual void serializeAttributes(IAttributes* out, SAttributeReadWriteOptions* options = 0);
	virtual void deserializeAttributes(IAttributes* in, SAttributeReadWriteOptions* options = 0);
	void pointCameraAtNode(const ISceneNode* node, float radius = 100.0f);	// Ajout� : radius : permet de d�finir la distance entre la cam�ra et la position du node

	void updateXEffects();

protected:
	//Properties
	matrix4 viewMatrixAffector; // Non utilis� ! (mais � garder pour la fonction getViewMatrixAffector() )
	u32 lastTimeMs;
	u32 lastRealTimeMs;			// Ajout� : Utilis� pour mettre � jour les animators et les enfants de cette cam�ra avec le temps r�el �coul�
	u32 beginTimeMs;			// Ajout� : Permet de d�terminer quelle valeur du temps r�el envoyer aux nodes enfants de celui-ci pour leur animation
	bool lockZoom;
	float minZoom;
	float maxZoom;
	bool lockRotateX;
	float minRotateX;
	float maxRotateX;
	bool lockRotateY;
	float minRotateY;
	float maxRotateY;

	bool lockCamera;

	float wantedZoom;
	float zoomStep;

	float targetY;	// La valeur en Y du point cible, toujours constante

	bool canBecomeFPSCamera;		// True si cette cam�ra peut devenir une cam�ra FPS
	bool isCameraFPSEnabled;		// True si le mode "Cam�ra FPS" est activ�
	ICameraSceneNode* FPSCamera;	// La cam�ra FPS
	scene::ISceneNodeAnimatorCollisionResponse* collisionAnim;	// L'animator de collision de la cam�ra FPS
	scene::ITriangleSelector* collisionWorld;	// Le triangle selector de l'animator de collision

	rectf terrainLimits;

	core::vector3df oldPos;

	vector3df Target, UpVector;
	matrix4 Projection, View;
	SViewFrustum ViewArea;
	//dimension2df screenDim;
	float Fovy;      // Field of view, in radians.
	float Aspect;   // Aspect ratio.
	float ZNear;   // value of the near view-plane.
	float ZFar;   // Z-value of the far view-plane.

	void recalculateProjectionMatrix()
	{
		ViewArea.getTransform(ETS_PROJECTION).buildProjectionMatrixPerspectiveFovLH(Fovy, Aspect, ZNear, ZFar);
	}
	void recalculateViewArea()
	{
		ViewArea.cameraPosition = getAbsolutePosition();
		ViewArea.setFrom(ViewArea.getTransform(ETS_PROJECTION) * ViewArea.getTransform(ETS_VIEW));
	}

//private:
	//vector3df Pos;	// Modifi� : Utilisation de ISceneNode::RelativeTranslation � la place
	bool rotating, moving, translating;
	float zoomSpeed;
	float translateSpeed;
	float rotateSpeed;
	float rotateStartX, rotateStartY;
	float zoomStartX, zoomStartY;
	float translateStartX, translateStartY;
	float currentZoom;
	float rotX, rotY;
	vector3df oldTarget;

	void animate(float elapsedTimeS);
	void updateAnimationState();

	void repairRotations(float& rotationX, float& rotationY);
	void repairRotationY(float& rotationY);

public:
	//Type Return
	virtual ESCENE_NODE_TYPE getType() const					{ return ESNT_CAMERA; }

	//Setup
	virtual void setInputReceiverEnabled(bool enabled)			{ }
	virtual bool isInputReceiverEnabled() const					{ return true; }

	//Gets
	virtual const aabbox3df& getBoundingBox() const				{ return ViewArea.getBoundingBox(); }
	virtual const matrix4& getProjectionMatrix() const			{ return ViewArea.getTransform(ETS_PROJECTION); }
	virtual const SViewFrustum* getViewFrustum() const			{ return &ViewArea; }
	virtual const core::vector3df& getTarget() const			{ return Target; }
	virtual const matrix4& getViewMatrix() const				{ return ViewArea.getTransform(ETS_VIEW); }
	virtual const core::vector3df& getUpVector() const			{ return UpVector; }
	virtual float getNearValue() const							{ return ZNear; }
	virtual float getFarValue() const							{ return ZFar; }
	virtual float getAspectRatio() const						{ return Aspect; }
	virtual float getFOV() const								{ return Fovy; }

	//Sets
	virtual void setNearValue(float zn)							{ ZNear = zn; recalculateProjectionMatrix(); }
	virtual void setFarValue(float zf)							{ ZFar = zf; recalculateProjectionMatrix(); }
	virtual void setAspectRatio(float aspect)					{ Aspect = aspect; recalculateProjectionMatrix(); }
	virtual void setFOV(float fovy)								{ Fovy = fovy; recalculateProjectionMatrix(); }
	virtual void setUpVector(const vector3df& pos)				{ UpVector = pos; }
	virtual void setProjectionMatrix(const matrix4& projection)	{ ViewArea.getTransform(ETS_PROJECTION) = projection; }
	virtual void setPosition(const vector3df& pos);
	virtual void setTarget(const vector3df& target);

	virtual void setRotation(const vector3df&)					{ }
	virtual void setProjectionMatrix(const matrix4&, bool)		{ }
	virtual void setViewMatrixAffector(const matrix4& mat)		{ viewMatrixAffector = mat; }
	virtual const core::matrix4& getViewMatrixAffector() const	{ return viewMatrixAffector; }
	virtual void bindTargetAndRotation(bool)					{ }
	virtual bool getTargetAndRotationBinding() const			{ return false; }

	virtual void setZoomStep(float value)						{ zoomStep = value; }
	virtual void setZoomSpeed(float value)						{ zoomSpeed = value; }
	virtual void setTranslateSpeed(float value)					{ translateSpeed = value; }
	virtual void setRotationSpeed(float value)					{ rotateSpeed = value; }

	// Ajout� :
	virtual void updateAbsolutePosition()
	{
		// Si la cam�ra FPS est activ�e, on r�nitialise notre position absolue car la cam�ra FPS est une de nos enfants
		// (�vite ainsi de perturber sa position absolue dans le monde 3D � cause de notre position)
		if (isCameraFPSEnabled)
			AbsoluteTransformation.makeIdentity();
		else	// Sinon, on met � jour notre position absolue normalement
			ISceneNode::updateAbsolutePosition();
	}
	void setRotateXLock(bool lock = false, float min = 0.0f, float max = 180.0f)
	{
		lockRotateX = lock;
		minRotateX = min;
		maxRotateX = max;
	}
	void setRotateYLock(bool lock = false, float min = 0.0f, float max = 180.0f)
	{
		lockRotateY = lock;
		minRotateY = min;
		maxRotateY = max;
	}
	void setZoomLock(bool lock = true, float min = 50.0f, float max = 500.0f);
	bool getLockCamera() const									{ return lockCamera; }
	void setLockCamera(bool lock)								{ lockCamera = lock; }
	void setTerrainLimits(rectf limits)							{ limits.repair(); terrainLimits = limits; }

	ICameraSceneNode* getFPSCamera()							{ return FPSCamera; }
	bool getIsCameraFPSEnabled() const							{ return isCameraFPSEnabled; }
	void setIsCameraFPSEnabled(bool enabled);
	void setCollisionAnimator(scene::ISceneNodeAnimatorCollisionResponse* anim)
	{
		if (anim)
			anim->grab();
		if (collisionAnim)
			collisionAnim->drop();

		collisionAnim = anim;

		if (collisionAnim)
		{
			collisionWorld = collisionAnim->getWorld();
			collisionAnim->setWorld(NULL);
		}
		else
			collisionWorld = NULL;
	}

	// Retourne le temps de d�part d'un animator � ajouter � cette cam�ra
	u32 getAnimatorBeginTimeMs() const;
};

#endif
