#include "CLoadingScreen.h"
#include "Game.h"

CLoadingScreen::CLoadingScreen() : percent(0.0f), maxPercent(100.0f), loadingBegun(false), backgroundTexture(NULL), loadingFont(NULL)
{
}
CLoadingScreen::~CLoadingScreen()
{
	if (loadingBegun)
		endLoading();
}
void CLoadingScreen::beginLoading()
{
	if (!game->device || loadingBegun)
		return;

	const core::dimension2du& screenSize = game->driver->getScreenSize();

	// Calcule la zone o� sera dessin�e la barre de chargement
	loadingScreenRect.UpperLeftCorner.X = core::round32(screenSize.Width * 0.1f);
	loadingScreenRect.UpperLeftCorner.Y = core::round32(screenSize.Height * 0.85f);
	loadingScreenRect.LowerRightCorner.X = core::round32(screenSize.Width * 0.9f);
	loadingScreenRect.LowerRightCorner.Y = core::round32(screenSize.Height * 0.9f);

	// D�sactive les mip maps dans la cr�ation des textures
	const bool lastMipMapState = game->driver->getTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS);
	game->driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, false);

	// Charge la texture de l'�cran de chargement
	backgroundTexture = chooseLoadingScreen();

	// Cr�e la police pour dessiner le texte du chargement
	loadingFont = game->gui->getFont("bigfont.png");

	// R�active les mip maps dans la cr�ation des textures
	game->driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, lastMipMapState);

	// Indique que cet �cran de chargement peut �tre dessin�
	loadingBegun = true;
}
video::ITexture* CLoadingScreen::chooseLoadingScreen()
{
	// La liste de tous les �crans de chargement trouv�s dans le syst�me de fichier
	core::list<io::path> loadingScreenList;

	// Parcours tout le syst�me de fichier d'Irrlicht et v�rifie si chaque fichier n'est pas un �cran de chargement
	const u32 archiveCount = game->fileSystem->getFileArchiveCount();
	for (u32 i = 0; i < archiveCount; ++i)
	{
		const io::IFileList* const fileList = game->fileSystem->getFileArchive(i)->getFileList();
		const u32 fileCount = fileList->getFileCount();
		for (u32 j = 0; j < fileCount; ++j)
		{
			if (fileList->isDirectory(j))	continue;				// V�rifie que le fichier actuel n'est pas un dossier

			const io::path& fileName = fileList->getFileName(j);	// Obtient le nom actuel du fichier
			if (fileName.find("loading screen") < 0)	continue;	// V�rifie que le nom du fichier contient bien "Loading Screen" (en minuscule car les noms de fichier retourn�s par la liste des fichiers d'Irrlicht sont en minuscule)

			// V�rifie par son extension que cet �cran de chargement peut �tre charg� par Irrlicht (�vite ainsi de r�cup�rer, par exemple, une archive nomm�e "Loading Screen.zip")
			const u32 imageLoaderCount = game->driver->getImageLoaderCount();
			for (u32 k = 0; k < imageLoaderCount; ++k)
			{
				const video::IImageLoader* const imgLoader = game->driver->getImageLoader(k);
				if (imgLoader)
				{
					if (imgLoader->isALoadableFileExtension(fileName))
					{
						// Ce fichier est bien un �cran de chargement et peut �tre charg� par Irrlicht : on l'ajoute � la liste des �crans de chargement
						loadingScreenList.push_back(fileName);
						break;
					}
				}
			}
		}
	}

	const u32 loadingScreenListSize = loadingScreenList.size();

	// V�rifie qu'au moins 1 �cran de chargement est disponible
	if (!loadingScreenListSize)
		return NULL;

	// Choisi un nombre au hasard entre 0 et le nombre d'�crans de chargement - 1
	const int loadingScreen = (rand() % loadingScreenListSize);

	// Retourne l'�cran de chargement ainsi choisi
	core::list<io::path>::ConstIterator it = loadingScreenList.begin() + loadingScreen;

	LOG_DEBUG("Ecran de chargement actuel : " << (*it).c_str(), ELL_INFORMATION);

	return game->driver->getTexture(*it);
}
void CLoadingScreen::endLoading()
{
	if (!loadingBegun)
		return;

	// Supprime la texture du fond
	if (backgroundTexture)
	{
		game->driver->removeTexture(backgroundTexture);
		backgroundTexture = NULL;
	}

	// Oublie la police du chargement (non supprim�e car utilis�e dans la GUI du jeu)
	loadingFont = NULL;

	// Indique que cet �cran de chargement ne peut plus �tre dessin�
	loadingBegun = false;
}
bool CLoadingScreen::draw()
{
	if (!loadingBegun)
		return false;

	// G�re les �v�nements du game->device et met � jour le timer, et v�rifie que la fen�tre n'a pas �t� ferm�e
	if (!game->device->run())
		return true;	// La fen�tre a �t� ferm�e : il n'y a plus aucune utilit� � continuer le chargement !

	const core::dimension2du& screenSize = game->driver->getScreenSize();
	const core::recti screenRect(0, 0, screenSize.Width, screenSize.Height);

	const gui::IGUISkin* const skin = game->gui->getSkin();

	// Efface l'�cran et commence la sc�ne actuelle
	game->driver->setViewPort(screenRect);
	game->driver->beginScene(true, true, video::SColor(0x0));

	// Dessine l'image de fond si elle est valide
	if (backgroundTexture)
	{
		const core::dimension2du& backgroundTextureSize = backgroundTexture->getOriginalSize();
		game->driver->draw2DImage(backgroundTexture, screenRect, core::recti(0, 0, backgroundTextureSize.Width, backgroundTextureSize.Height));
	}

	// Dessine le texte indiquant que le chargement est en cours
	if (loadingFont)
		loadingFont->draw(L"Chargement en cours...",
			core::recti(0, 0, screenSize.Width, screenSize.Height),
			skin ? skin->getColor(gui::EGDC_ACTIVE_CAPTION) : video::SColor(255, 180, 255, 180),
			true, true);

	// Obtient la couleur du fond de la barre de chargement
	video::SColor loadingScreenColor(skin ? skin->getColor(gui::EGDC_HIGH_LIGHT) : video::SColor(255, 30, 100, 30));
	loadingScreenColor.set(255,
		core::round32(loadingScreenColor.getRed() * 0.8f),
		core::round32(loadingScreenColor.getGreen() * 0.8f),
		core::round32(loadingScreenColor.getBlue() * 0.8f));

	// Dessine le fond de la barre de chargement
	game->driver->draw2DRectangle(loadingScreenColor, loadingScreenRect);

	// Augmente la couleur du fond de la barre de chargement pour repr�senter la zone charg�e
	loadingScreenColor.set(255,
		core::round32(loadingScreenColor.getRed() * 1.8f),
		core::round32(loadingScreenColor.getGreen() * 1.8f),
		core::round32(loadingScreenColor.getBlue() * 1.8f));

	// Dessine la zone charg�e de la barre de chargement
	core::recti loadingRect(loadingScreenRect);
	loadingRect.LowerRightCorner.X = loadingScreenRect.UpperLeftCorner.X + core::round32(core::clamp((percent / maxPercent), 0.0f, 1.0f) * (float)(loadingScreenRect.getWidth()));
	game->driver->draw2DRectangle(loadingScreenColor, loadingRect);

	// Termine la sc�ne et l'affiche � l'�cran
	game->driver->endScene();

	return false;
}
