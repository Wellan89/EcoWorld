#include "global.h"

#ifdef USE_SPARK
#include "SparkManager.h"
#include "CSmokeRendererSceneNode.h"
#include "Game.h"

SparkManager::SparkManager() : rainKillPlane(NULL), smokeRenderer(NULL)
{
	// Cr�e le scene node charg� d'afficher les fum�es
	smokeRenderer = new CSmokeRendererSceneNode(game->sceneManager->getRootSceneNode(), game->sceneManager);
}
SparkManager::~SparkManager()
{
	if (smokeRenderer)
		smokeRenderer->drop();

#ifdef _DEBUG
	cout << endl << "SPARK FACTORY BEFORE DESTRUCTION :" << endl;
	SPKFactory::getInstance().traceAll();
#endif

	// D�truit le syst�me de SPARK et son instance
	SPKFactory::getInstance().destroyAll();
#ifdef _DEBUG
	cout << endl << "SPARK FACTORY AFTER DESTRUCTION :" << endl;
	SPKFactory::getInstance().traceAll();
#endif
	SPKFactory::destroyInstance();
}
void SparkManager::registerSmokePosition(const core::list<core::vector3df>& smoke1Positions, const core::list<core::vector3df>& smoke2Positions)
{
	// Ajoute les positions de rendu des fum�es au scene node charg� de les afficher
	if (smokeRenderer)
		smokeRenderer->registerSmokePosition(smoke1Positions, smoke2Positions);
}
void SparkManager::init()
{
	// Indique la valeur al�atoire de base pour les calculs du syst�me
	SPK::randomSeed = rand();

	// Indique la vitesse du timer d'Irrlicht
	setTimerSpeed(game->deviceTimer->getSpeed());



	// Cr�e le renderer pour la pluie
	if (!rain.renderer)
	{
		rain.renderer = IRRLineRenderer::create(game->device, -0.1f, 1.5f);	// Augmente un peu l'�paisseur des lignes (sous OpenGL seulement : DirectX ne le supporte pas)
		rain.renderer->setBlending(BLENDING_ALPHA);
		rain.renderer->enableRenderingHint(DEPTH_WRITE, false);
	}

	// Cr�e le model pour la pluie
	if (!rain.model)
	{
		// Les particules de pluie sont grises fonc�es lorsqu'elles tombent du ciel, et deviennent presque blanches � l'approche du sol
		rain.model = Model::create(FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_ALPHA,
			FLAG_RED | FLAG_GREEN | FLAG_BLUE,
			FLAG_ALPHA,
			FLAG_NONE);
		rain.model->setParam(PARAM_RED, 0.2f, 0.9f);
		rain.model->setParam(PARAM_GREEN, 0.2f, 0.9f);
		rain.model->setParam(PARAM_BLUE, 0.2f, 0.9f);
		rain.model->setParam(PARAM_ALPHA, 0.2f, 0.4f);
		rain.model->setLifeTime(1.5f, 1.5f);
	}

	// Cr�e la zone pour la pluie
	if (!rain.zone)
	{
		const float rainSize = TAILLE_OBJETS * TAILLE_CARTE * 2.0f;	// Minimum : TAILLE_OBJETS * TAILLE_CARTE pour couvrir tout le terrain
		rain.zone = AABox::create(
			Vector3D(0.0f, 67.0f * TAILLE_OBJETS, 0.0f),	// position		// Note : Hauteur max de la cam�ra RTS pour TAILLE_OBJETS = 10.0f : 663.0f
			Vector3D(rainSize, 0.0f, rainSize));			// dimension
	}

	// Cr�e l'emmiter pour la pluie
	if (!rain.emitter)
	{
		rain.emitter = SphericEmitter::create(Vector3D(0.0f, -1.0f, 0.0f), 0.0f, 0.03f * core::PI);
		rain.emitter->setZone(rain.zone);
		rain.emitter->setForce(100.0f, 200.0f);
	}

	// Cr�e le plan de destruction des particules de pluie
	if (!rainKillPlane)
		rainKillPlane = Plane::create();	// Valeurs par d�faut : � assigner dans setMinTerrainHeight()

	// Cr�e le group pour la pluie
	if (!rain.group)
	{
		rain.group = Group::create(rain.model, 1500);
		rain.group->setRenderer(rain.renderer);
		rain.group->addEmitter(rain.emitter);
		rain.group->addModifier(Destroyer::create(rainKillPlane, INSIDE_ZONE));	// Cr�e le modifier qui d�truira les particules de pluie lorsqu'elles seront tomb�es en-dessous du plan rainKillPlane
		//rain.group->setFriction(0.7f);
		rain.group->enableAABBComputing(false);
		rain.group->enableSorting(false);

		// Indique une direction du vent nulle pour appliquer une gravit� de base � ce groupe
		setWindDirection(core::vector2df(0.0f, 0.0f));
	}

	// Cr�e le syst�me de particules principal de SPARK pour la pluie
	if (!rain.system)
	{
		// Cr�e le syst�me de particules principal de SPARK pour la pluie
		rain.system = IRRSystem::create(game->sceneManager->getRootSceneNode(), game->sceneManager);

		// D�sactive le culling de ce syst�me de particules (car la pluie est toujours visible : on �vite ainsi de devoir recalculer sa bounding box � chaque frame)
		rain.system->setAutomaticCulling(scene::EAC_OFF);
		rain.system->enableAABBComputing(false);

		// Ajoute le groupe de la pluie au syst�me de particules
		rain.system->addGroup(rain.group);
	}



	// Cr�e les renderer pour les fum�es
	video::ITexture* const smokeTexture = game->device->getVideoDriver()->getTexture("smoke.png");
	if (!smoke1.renderer)
	{
		IRRQuadRenderer* const smoke1Renderer = IRRQuadRenderer::create(game->device);
		smoke1Renderer->setTexture(smokeTexture);
		smoke1Renderer->setTexturingMode(TEXTURE_2D);
		smoke1Renderer->setBlending(BLENDING_ALPHA);
		smoke1Renderer->setAtlasDimensions(2, 2);
		smoke1Renderer->enableRenderingHint(DEPTH_WRITE, false);

		// Applique le brouillard au renderer
		// Attention : const_cast pour pouvoir modifier le mat�riau du renderer !
		video::SMaterial& smoke1Material = const_cast<video::SMaterial&>(smoke1Renderer->getMaterial());
		smoke1Material.FogEnable = true;

		smoke1.renderer = smoke1Renderer;
	}
	if (!smoke2.renderer)
	{
		IRRQuadRenderer* const smoke2Renderer = IRRQuadRenderer::create(game->device);
		smoke2Renderer->setTexture(smokeTexture);
		smoke2Renderer->setTexturingMode(TEXTURE_2D);
		smoke2Renderer->setBlending(BLENDING_ALPHA);
		smoke2Renderer->setAtlasDimensions(2, 2);
		smoke2Renderer->enableRenderingHint(DEPTH_WRITE, false);

		// Applique le brouillard au renderer
		// Attention : const_cast pour pouvoir modifier le mat�riau du renderer !
		video::SMaterial& smoke2Material = const_cast<video::SMaterial&>(smoke2Renderer->getMaterial());
		smoke2Material.FogEnable = true;

		smoke2.renderer = smoke2Renderer;
	}

	// Cr�e les models pour les fum�es
	const float doublePI = core::PI * 2.0f;
	if (!smoke1.model)
	{
		smoke1.model = Model::create(FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_ALPHA | FLAG_SIZE | FLAG_ANGLE | FLAG_TEXTURE_INDEX,
			FLAG_ALPHA | FLAG_SIZE | FLAG_ANGLE,
			FLAG_ANGLE | FLAG_TEXTURE_INDEX,
			FLAG_NONE);
		smoke1.model->setParam(PARAM_RED, 0.2f);
		smoke1.model->setParam(PARAM_GREEN, 0.2f);
		smoke1.model->setParam(PARAM_BLUE, 0.2f);
		smoke1.model->setParam(PARAM_ALPHA, 0.2f, 0.0f);
		smoke1.model->setParam(PARAM_SIZE, 3.0f, 5.0f);
		smoke1.model->setParam(PARAM_ANGLE, 0.0f, doublePI, 0.0f, doublePI);
		smoke1.model->setParam(PARAM_TEXTURE_INDEX, 0.0f, 4.0f);
		smoke1.model->setLifeTime(3.0f, 4.0f);
	}
	if (!smoke2.model)
	{
		smoke2.model = Model::create(FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_ALPHA | FLAG_SIZE | FLAG_ANGLE | FLAG_TEXTURE_INDEX,
			FLAG_ALPHA | FLAG_SIZE | FLAG_ANGLE,
			FLAG_ANGLE | FLAG_TEXTURE_INDEX,
			FLAG_NONE);
		smoke2.model->setParam(PARAM_RED, 0.2f);
		smoke2.model->setParam(PARAM_GREEN, 0.2f);
		smoke2.model->setParam(PARAM_BLUE, 0.2f);
		smoke2.model->setParam(PARAM_ALPHA, 0.3f, 0.0f);
		smoke2.model->setParam(PARAM_SIZE, 4.0f, 6.0f);
		smoke2.model->setParam(PARAM_ANGLE, 0.0f, doublePI, 0.0f, doublePI);
		smoke2.model->setParam(PARAM_TEXTURE_INDEX, 0.0f, 4.0f);
		smoke2.model->setLifeTime(5.0f, 7.0f);
	}

	// Cr�e les zones pour les fum�es
	if (!smoke1.zone)
		smoke1.zone = AABox::create(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(1.0f, 0.0f, 1.0f));
	if (!smoke2.zone)
		smoke2.zone = AABox::create(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(3.0f, 0.0f, 3.0f));

	// Cr�e les emitters pour les fum�es
	if (!smoke1.emitter)
	{
		smoke1.emitter = SphericEmitter::create(Vector3D(0.0f, 1.0f, 0.0f), 0.0f, 0.3f * core::PI);
		smoke1.emitter->setZone(smoke1.zone);
		smoke1.emitter->setFlow(20.0f);
		smoke1.emitter->setForce(0.5f, 1.0f);
	}
	if (!smoke2.emitter)
	{
		smoke2.emitter = SphericEmitter::create(Vector3D(0.0f, 1.0f, 0.0f), 0.0f, 0.7f * core::PI);
		smoke2.emitter->setZone(smoke2.zone);
		smoke2.emitter->setFlow(60.0f);
		smoke2.emitter->setForce(1.0f, 2.0f);
	}

	// Cr�e les groups pour les fum�es
	/*	Ancien : depuis : Group* SparkManager::createSmokeGroup(const core::list<Emitter*>& emitterList) :
		M�thode de calcul de la capacit� maximale du groupe n�cessaire depuis les dur�es de vie moyennes des particules des emitters et de leur d�bit :

		// Calcule la capacit� de particules n�cessaire pour ce group d'apr�s le nombre de particules envoy�es par les emitters et leur dur�e de vie :
		const float particleLifeTime = max((smoke1.model->getLifeTimeMin() + smoke1.model->getLifeTimeMax()) * 0.5f, 0.0f);
		float groupCapacity = 0.0f;
		core::list<Emitter*>::ConstIterator it = emitterList.begin();
		for (it = emitterList.begin(); it != emitterList.end(); ++it)
		{
			if (*it)
			{
				const float emitterFlow = (*it)->getFlow();
				if (emitterFlow > 0.0f)			// Emission de particules constante
					groupCapacity += (*it)->getFlow() * particleLifeTime;
				else if (emitterFlow < 0.0f)	// Emission de particules infinie
				{
					const int emitterTank = (*it)->getTank();
					if (emitterTank > 0)		// Stock de particules limit�
						groupCapacity += (float)((*it)->getTank());
				}
			}
		}
		groupCapacity = ceilf(groupCapacity);
		if (groupCapacity < 1.0f)
			groupCapacity = 1.0f;
	*/
	if (!smoke1.group)
	{
		smoke1.group = Group::create(smoke1.model, 80);
		smoke1.group->setGravity(Vector3D(0.0f, 1.0f, 0.0f));
		smoke1.group->setRenderer(smoke1.renderer);
		smoke1.group->addEmitter(smoke1.emitter);
		smoke1.group->enableAABBComputing(false);
		smoke1.group->enableSorting(true);
	}
	if (!smoke2.group)
	{
		smoke2.group = Group::create(smoke2.model, 420);
		smoke2.group->setGravity(Vector3D(0.0f, 1.0f, 0.0f));
		smoke2.group->setRenderer(smoke2.renderer);
		smoke2.group->addEmitter(smoke2.emitter);
		smoke2.group->enableAABBComputing(false);
		smoke2.group->enableSorting(true);
	}

	// Cr�e les syst�mes de particules principaux de SPARK pour les fum�es
	if (!smoke1.system)
	{
		// Cr�e le syst�me de particules principal de SPARK pour la fum�e
		// On d�sactive aussi la transformation globale de ce syst�me pour qu'il soit rendu suivant la position de son node
		smoke1.system = IRRSystem::create(game->sceneManager->getRootSceneNode(), game->device->getSceneManager(), false);

		// Anime automatiquement ce syst�me de particules sans se soucier de s'il est visible ou non
		smoke1.system->setAutoUpdateEnabled(true, false);

		// D�sactive le culling de ce syst�me de particules (car la fum�e ne sera jamais rendue automatiquement par Irrlicht : on �vite ainsi de devoir recalculer sa bounding box � chaque frame)
		smoke1.system->setAutomaticCulling(scene::EAC_OFF);
		smoke1.system->enableAABBComputing(false);

		// Masque le syst�me de particules principal de SPARK pour la fum�e car il ne devra �tre rendu que par CBatimentSceneNode (son syst�me de particules continuera tout de m�me � �tre mis � jour)
		smoke1.system->setVisible(false);

		// Ajoute le groupe de la pluie au syst�me de particules
		smoke1.system->addGroup(smoke1.group);
	}
	if (!smoke2.system)
	{
		// Cr�e le syst�me de particules principal de SPARK pour la fum�e
		// On d�sactive aussi la transformation globale de ce syst�me pour qu'il soit rendu suivant la position de son node
		smoke2.system = IRRSystem::create(game->sceneManager->getRootSceneNode(), game->sceneManager, false);

		// Anime automatiquement ce syst�me de particules sans se soucier de s'il est visible ou non
		smoke2.system->setAutoUpdateEnabled(true, false);

		// D�sactive le culling de ce syst�me de particules (car la fum�e ne sera jamais rendue automatiquement par Irrlicht : on �vite ainsi de devoir recalculer sa bounding box � chaque frame)
		smoke2.system->setAutomaticCulling(scene::EAC_OFF);
		smoke2.system->enableAABBComputing(false);

		// Masque le syst�me de particules principal de SPARK pour la fum�e car il ne devra �tre rendu que par CBatimentSceneNode (son syst�me de particules continuera tout de m�me � �tre mis � jour)
		smoke2.system->setVisible(false);

		// Ajoute le groupe de la pluie au syst�me de particules
		smoke2.system->addGroup(smoke2.group);
	}
}
void SparkManager::reset()
{
	if (!rain.system || !smoke1.system || !smoke2.system || !smokeRenderer)
		return;

	// Vide les syst�mes de particules
	rain.system->empty();
	smoke1.system->empty();
	smoke2.system->empty();

	// Ajoute les syst�mes de particules et le renderer des fum�es � Irrlicht car ils ont �t� supprim�s du scene manager par un appel pr�c�dent � sceneManager->clear()
	scene::ISceneNode* const rootSceneNode = game->sceneManager->getRootSceneNode();
	rain.system->setParent(rootSceneNode);
	smoke1.system->setParent(rootSceneNode);
	smoke2.system->setParent(rootSceneNode);
	smokeRenderer->setParent(rootSceneNode);

	// Masque les syst�mes de la fum�e car ils doivent seulement �tre rendus par CSmokeRendererSceneNode
	smoke1.system->setVisible(false);
	smoke2.system->setVisible(false);
}

#endif
