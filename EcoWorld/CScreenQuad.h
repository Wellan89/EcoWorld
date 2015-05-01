#ifndef DEF_SCREEN_QUAD
#define DEF_SCREEN_QUAD
 
#include "global.h"

// Classe inline fournissant une mesh servant d'�cran de rendu pour les effets de PostProcess et XEffects
// Bas�e sur la classe CShaderMaterial de PostProcess
class CScreenQuad
{
protected:
	// Driver vid�o d'Irrlicht
	video::IVideoDriver* const driver;

	// Mesh de l'�cran
	video::S3DVertex vertices[4];
	u16 indices[6];

public:
	// Constructeur
	CScreenQuad(video::IVideoDriver* Driver) : driver(Driver)
	{
		// Cr�e les vertices de ce mesh
		const video::SColor color(0xffffffff);
		vertices[0] = video::S3DVertex(-1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, color, 0.0f, 1.0f);
		vertices[1] = video::S3DVertex(-1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f, color, 0.0f, 0.0f);
		vertices[2] = video::S3DVertex( 1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f, color, 1.0f, 0.0f);
		vertices[3] = video::S3DVertex( 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, color, 1.0f, 1.0f);

		// Cr�e les indices de ce mesh
		indices[0] = 0;	indices[1] = 1;	indices[2] = 2;
		indices[3] = 2;	indices[4] = 3;	indices[5] = 0;
	}

	// Pr�pare le driver pour le rendu de l'�cran : r�initialise les matrices de vue, d'objet et de projection du driver, et d�sactive le mat�riau de remplacement du driver
	void preRender()
	{
		// Efface les matrices de vue, d'objet et de projection
		driver->setTransform(video::ETS_VIEW, core::IdentityMatrix);
		driver->setTransform(video::ETS_WORLD, core::IdentityMatrix);
		driver->setTransform(video::ETS_PROJECTION, core::IdentityMatrix);

		// D�sactive le mat�riau de remplacement du driver
		driver->getOverrideMaterial().Enabled = false;
	}

	// Affiche l'�cran pour ce rendu
	// Note : Les mat�riaux et les textures cibles pour ce rendu devront auparavant avoir �t� �tablies pour le driver
	void render()
	{
		// Affiche le mesh de l'�cran
		driver->drawIndexedTriangleList(vertices, 4, indices, 2);
	}

	// Restaure le driver pour le rendu de l'�cran : r�active le mat�riau de remplacement du driver
	void postRender()
	{
		// R�active le mat�riau de remplacement du driver
		driver->getOverrideMaterial().Enabled = true;
	}

	// Effectue un seul rendu complet en assignant le mat�riau sp�cifi� au driver
	// La texture cible pour ce rendu devra auparavant avoir �t� �tablie pour le driver
	void fullRender(video::SMaterial& material)
	{
		preRender();
		driver->setMaterial(material);
		render();
		postRender();
	}
};

#endif
