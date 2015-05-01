#include "CSunSceneNode.h"

CSunSceneNode::CSunSceneNode(const core::dimension2df& size, const video::SColor& color, scene::ISceneNode* parent, scene::ISceneManager* mgr, int id)
 : IBillboardSceneNode(parent, mgr, id)
{
#ifdef _DEBUG
	setDebugName("CSunSceneNode");
#endif

	setSize(size);

	indices[0] = 0;
	indices[1] = 2;
	indices[2] = 1;
	indices[3] = 0;
	indices[4] = 3;
	indices[5] = 2;

	vertices[0].TCoords.set(1.0f, 1.0f);
	vertices[0].Color = color;

	vertices[1].TCoords.set(1.0f, 0.0f);
	vertices[1].Color = color;

	vertices[2].TCoords.set(0.0f, 0.0f);
	vertices[2].Color = color;

	vertices[3].TCoords.set(0.0f, 1.0f);
	vertices[3].Color = color;

	// Désactive le culling automatique
	// (le soleil n'est pas long à afficher et le scene manager ne peut pas prévoir sa position lors du rendu car elle sera dépendante de la position de la caméra)
	AutomaticCullingState = scene::EAC_OFF;

	// Initialise le material
	Material.Lighting = false;
	Material.MaterialType = video::EMT_TRANSPARENT_ADD_COLOR;

	// Désactive le Z-Buffer pour ce node : permet ainsi de cacher le soleil si d'autres objets 3D sont devant (réaliste)
	Material.ZBuffer = video::ECFN_NEVER;
	Material.ZWriteEnable = false;
}
void CSunSceneNode::OnRegisterSceneNode()
{
	// Affiche le soleil dans la pass des skyboxes et skydomes, pour pouvoir le dessiner juste après le ciel et avant tous les autres objets 3D
	// Attention : Dans ce cas, si le skydome est dessiné après le soleil, ce dernier ne sera pas visible car il sera masqué : il faut veiller en permanence à ce que le soleil soit après le skydome dans la liste des nodes
	if (IsVisible)
		SceneManager->registerNodeForRendering(this, scene::ESNRP_SKY_BOX);

	ISceneNode::OnRegisterSceneNode();
}
void CSunSceneNode::render()
{
	video::IVideoDriver* const driver = SceneManager->getVideoDriver();
	const scene::ICameraSceneNode* const camera = SceneManager->getActiveCamera();

	if (!camera || !driver)
		return;

	// make billboard look to camera
	const core::vector3df camPos = camera->getAbsolutePosition();
	const core::vector3df up = camera->getUpVector();
	core::vector3df view = camera->getTarget() - camPos;
	view.normalize();

	core::vector3df horizontal = up.crossProduct(view);
	if (horizontal.getLength() == 0)
		horizontal.set(up.Y,up.X,up.Z);

	horizontal.normalize();
	horizontal *= 0.5f * Size.Width;

	core::vector3df vertical = horizontal.crossProduct(view);
	vertical.normalize();
	vertical *= 0.5f * Size.Height;

	view *= -1.0f;

	for (int i=0; i<4; ++i)
		vertices[i].Normal = view;

	/* Vertices are:
	3--0
	| /|
	|/ |
	2--1
	*/
	vertices[0].Pos = RelativeTranslation + horizontal + vertical;
	vertices[1].Pos = RelativeTranslation + horizontal - vertical;
	vertices[2].Pos = RelativeTranslation - horizontal - vertical;
	vertices[3].Pos = RelativeTranslation - horizontal + vertical;

	// draw

	// Désactivé pour optimisation :
	/*
	if (DebugDataVisible & scene::EDS_BBOX)
	{
		driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);
		video::SMaterial m;
		m.Lighting = false;
		driver->setMaterial(m);
		driver->draw3DBox(BBox, video::SColor(0, 208, 195, 152));
	}
	*/

	// Place le soleil à la position de la caméra, pour que sa distance reste constante par rapport à elle, et donc qu'il paraisse à l'infini (même astuce que pour le rendu du ciel dans Irrlicht)
	// Note : il serait sûrement plus long de modifier la matrice de vue du driver, de dessiner le soleil, puis de la restaurer : c'est pour cela que cette méthode a été choisie
	core::matrix4 mat(core::IdentityMatrix);
	mat.setTranslation(camera->getAbsolutePosition());

	driver->setTransform(video::ETS_WORLD, mat);

	driver->setMaterial(Material);

	driver->drawIndexedTriangleList(vertices, 4, indices, 2);
}
void CSunSceneNode::setSize(const core::dimension2df& size)
{
	Size = size;

	if (core::equals(Size.Width, 0.0f))
		Size.Width = 1.0f;

	if (core::equals(Size.Height, 0.0f))
		Size.Height = 1.0f;

	const float avg = (Size.Width + Size.Height) / 6.0f;
	BBox.MinEdge.set(-avg,-avg,-avg);
	BBox.MaxEdge.set(avg,avg,avg);
}
void CSunSceneNode::setColor(const video::SColor& overallColor)
{
	for (u32 vertex = 0; vertex < 4; ++vertex)
		vertices[vertex].Color = overallColor;
}
void CSunSceneNode::setColor(const video::SColor& topColor, const video::SColor& bottomColor)
{
	vertices[0].Color = bottomColor;
	vertices[1].Color = topColor;
	vertices[2].Color = topColor;
	vertices[3].Color = bottomColor;
}
void CSunSceneNode::getColor(video::SColor& topColor, video::SColor& bottomColor) const
{
	bottomColor = vertices[0].Color;
	topColor = vertices[1].Color;
}
