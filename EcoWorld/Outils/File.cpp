//==========================================================
// File.cpp
//
// Implantation de la classe CFile
//
//==========================================================


//==========================================================
// En-têtes
//==========================================================

#ifdef _DEBUG

#include "File.h"
#include <algorithm>
#include <fstream>


namespace Yes
{

////////////////////////////////////////////////////////////
// Constructeur à partir d'un std::string
//
// [in] sName : Chemin complet du fichier
//
////////////////////////////////////////////////////////////
CFile::CFile(const std::string& Name) : m_Name(Name)
{
    std::replace(m_Name.begin(), m_Name.end(), '/', '\\');
}


////////////////////////////////////////////////////////////
// Constructeur à partir d'un const char*
//
// [in] sName : Chemin complet du fichier
//
////////////////////////////////////////////////////////////
CFile::CFile(const char* Name) :
m_Name(Name)
{
    std::replace(m_Name.begin(), m_Name.end(), '/', '\\');
}


////////////////////////////////////////////////////////////
// Indique si le fichier existe ou non
//
// [retval] True si le fichier existe
//
////////////////////////////////////////////////////////////
bool CFile::Exists() const
{
	std::ifstream File(m_Name.c_str());

    return File.is_open();
}


////////////////////////////////////////////////////////////
// Renvoie le nom du fichier avec son chemin complet
//
// [retval] Chemin complet du fichier
//
////////////////////////////////////////////////////////////
const std::string& CFile::Fullname() const
{
    return m_Name;
}


////////////////////////////////////////////////////////////
// Renvoie le nom du fichier sans son chemin
//
// [retval] Nom du fichier
//
////////////////////////////////////////////////////////////
std::string CFile::Filename() const
{
    std::string::size_type Pos = m_Name.find_last_of("\\/");

    if (Pos != std::string::npos)
        return m_Name.substr(Pos + 1, std::string::npos);
    else
        return m_Name;
}


////////////////////////////////////////////////////////////
// Renvoie le nom du fichier sans extension ni chemin
//
// [retval] Nom du fichier
//
////////////////////////////////////////////////////////////
std::string CFile::ShortFilename() const
{
    return Filename().substr(0, Filename().find_last_of("."));
}


////////////////////////////////////////////////////////////
// Renvoie l'extension du fichier
//
// [retval] Extension du fichier
//
////////////////////////////////////////////////////////////
std::string CFile::Extension() const
{
    std::string::size_type Pos = m_Name.find_last_of(".");
    if (Pos != std::string::npos)
        return m_Name.substr(Pos + 1, std::string::npos);
    else
        return "";
}

} // namespace Yes

#endif	// _DEBUG
