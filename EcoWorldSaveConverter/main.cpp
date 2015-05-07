#include "global.h"
#include "Batiments.h"
#include "CGUISortTable.h"
#include "SaveConverter.h"

#include <windows.h>



#ifdef _MSC_VER
#ifdef _DEBUG
#pragma comment(lib, "IrrDebugStatic.lib")				// Librairie de débogage d'Irrlicht
#else
#pragma comment(lib, "IrrReleaseStatic.lib")			// Librairie de compatibilité d'Irrlicht
#endif

#pragma comment(linker, "/NODEFAULTLIB:LIBCI")	// Cette librairie pose problème lorsqu'Irrlicht est compilé manuellement (problème entre VC++ et DirectX 9.0c) : on l'exclue ici
#endif



// Initialise les informations statiques des bâtiments de StaticBatimentInfos avec leur constructeur par défaut
StaticBatimentInfos StaticBatimentInfos::batimentInfos[BI_COUNT] = { };
u32 StaticBatimentInfos::tailleMaxBats = 1;

// Initialise le tableau des types des tris avec leurs valeurs respectives
const CGUISortTable::E_TRI_TYPES CGUISortTable::triTypes[CGUISortTable::ETT_COUNT] = {
	(CGUISortTable::E_TRI_TYPES)0,
	(CGUISortTable::E_TRI_TYPES)1,
	(CGUISortTable::E_TRI_TYPES)2,
	(CGUISortTable::E_TRI_TYPES)3 };



// Définitions de fonctions fréquemment utilisée pour la création de la GUI d'Irrlicht (ces fonctions sont déclarées dans global.h) :
inline core::recti getAbsoluteRectangle(float x, float y, float x2, float y2, const core::dimension2du& parentSize)
{
	return core::recti((int)(x * parentSize.Width), (int)(y * parentSize.Height),
		(int)(x2 * parentSize.Width), (int)(y2 * parentSize.Height));
}
inline core::recti getAbsoluteRectangle(float x, float y, float x2, float y2, const core::recti& parentRect)
{
	return getAbsoluteRectangle(x, y, x2, y2, core::dimension2du(
			(u32)(abs(parentRect.LowerRightCorner.X - parentRect.UpperLeftCorner.X)),
			(u32)(abs(parentRect.LowerRightCorner.Y - parentRect.UpperLeftCorner.Y))));
}
/*inline core::recti getAbsoluteRectangle(float x, float y, float x2, float y2)
{
	return getAbsoluteRectangle(x, y, x2, y2, driver->getScreenSize());	// Désactivée car le driver est ici inaccessible !
}*/

void createDirectory(const char* path)	{ }

bool openFilename(io::path& outFilename);



int main(int argc, char* argv[])
{
	bool open = true;
	io::path filename;

	if (argc > 1)
		filename = argv[1];
	else
		open = openFilename(filename);

	if (open)
		SaveConverter converter(filename);

	return EXIT_SUCCESS;
}
bool openFilename(io::path& outFilename)
{
#define FILENAME_MAX_SIZE	512
	char tmpStr[FILENAME_MAX_SIZE] = "";
	OPENFILENAME opf;
	ZeroMemory(&opf, sizeof(OPENFILENAME));

	opf.hwndOwner = NULL;
	opf.lpstrFilter = "EcoWorld Game (*.ewg)\0*.ewg\0All Files (*.*)\0*.*\0";
	opf.lpstrCustomFilter = 0;
	opf.nMaxCustFilter = 0;
	opf.nFilterIndex = 1;
	opf.lpstrFile = tmpStr;
	opf.nMaxFile = FILENAME_MAX_SIZE;
	opf.lpstrFileTitle = 0;
	opf.nMaxFileTitle = 50;
	opf.lpstrInitialDir = 0;
	opf.lpstrTitle = "Ouvrir un fichier";
	opf.nFileOffset = 0;
	opf.nFileExtension = 2;
	opf.lpstrDefExt = "*.*";
	opf.lpfnHook = NULL;
	opf.lCustData = 0;
	opf.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	opf.lStructSize = sizeof(OPENFILENAMEW);

	if (GetOpenFileName(&opf))
	{
		outFilename = opf.lpstrFile;
		return true;
	}
	else
	{
		outFilename = "";
		return false;
	}
}
