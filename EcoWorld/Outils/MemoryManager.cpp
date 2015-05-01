//==========================================================
// MemoryManager.cpp
//
// Implantation de la classe CMemoryManager
//
//==========================================================


//==========================================================
// En-t�tes
//==========================================================

#ifdef _DEBUG

#include "MemoryManager.h"
#include <iomanip>

#include <iostream>			// Ajout� : Permet d'informer l'utilisateur dans la console que des fuites ont �t� d�t�ct�es

namespace Yes
{

////////////////////////////////////////////////////////////
// Constructeur par d�faut
//
////////////////////////////////////////////////////////////
CMemoryManager::CMemoryManager() : m_File("Memory leaks.log"), m_Blocks(NULL), m_DeleteStack(NULL)
{
    // On v�rifie que le fichier est bien ouvert
    if (!m_File)
		throw std::runtime_error("Memory leaks.log : Impossible d'acc�der en �criture");

    // Inscription de l'en-t�te du fichier
    m_File << "  ========================================" << std::endl;
    m_File << "   Yes::Engine v0.1 - Memory leak tracker " << std::endl;
    m_File << "  ========================================" << std::endl << std::endl;

	m_Blocks = new TBlockMap();
	m_DeleteStack = new std::stack<TBlock>();
}


////////////////////////////////////////////////////////////
// Destructeur
//
////////////////////////////////////////////////////////////
CMemoryManager::~CMemoryManager()
{
    if (m_Blocks->empty())
    {
        // Aucune fuite, bravo !
        m_File << std::endl;
        m_File << "  ========================================" << std::endl;
        m_File << "     No leak detected, congratulations !  " << std::endl;
        m_File << "  ========================================" << std::endl;
    }
    else
    {
        // Fuites m�moires =(
        m_File << std::endl;
        m_File << "  ========================================" << std::endl;
        m_File << "   Oops... Some leaks have been detected  " << std::endl;
        m_File << "  ========================================" << std::endl;
        m_File << std::endl;

        ReportLeaks();

		// Ajout� : Informe l'utilisateur dans la console que des fuites ont �t� d�t�ct�es
#ifdef _DEBUG
		std::cout << std::endl << "Leaks detected !" << std::endl;
		system("PAUSE");
#endif
    }

	free(m_Blocks);
	free(m_DeleteStack);
}


////////////////////////////////////////////////////////////
// Renvoie l'instance de la classe
//
// [retval] R�f�rence sur l'instance unique de la classe
//
////////////////////////////////////////////////////////////
CMemoryManager& CMemoryManager::Instance()
{
    static CMemoryManager Inst;

    return Inst;
}


////////////////////////////////////////////////////////////
// Inscrit le rapport sur les fuites de m�moire
//
////////////////////////////////////////////////////////////
void CMemoryManager::ReportLeaks()
{
    // D�tail des fuites
    std::size_t TotalSize = 0;
    for (TBlockMap::iterator i = m_Blocks->begin(); i != m_Blocks->end(); ++i)
    {
        // Ajout de la taille du bloc au cumul
        TotalSize += i->second.Size;

        // Inscription dans le fichier des informations sur le bloc courant
        m_File << "-> 0x" << i->first
               << " | "   << std::setw(7) << std::setfill(' ') << static_cast<int>(i->second.Size) << " octets"
               << " | "   << i->second.File.Filename() << " (" << i->second.Line << ")" << std::endl;

        // Lib�ration de la m�moire
        free(i->first);
    }

    // Affichage du cumul des fuites
    m_File << std::endl << std::endl << "-- "
           << static_cast<int>(m_Blocks->size()) << " blocs non-lib�r�(s), "
           << static_cast<int>(TotalSize)       << " octets --"
           << std::endl;
}


////////////////////////////////////////////////////////////
// Ajoute une allocation m�moire
//
// [in] Size :  Taille allou�e
// [in] File :  Fichier contenant l'allocation
// [in] Line :  Ligne de l'allocation
// [in] Array : True si c'est une allocation de tableau
//
// [retval] Pointeur sur la m�moire allou�e
//
////////////////////////////////////////////////////////////
void* CMemoryManager::Allocate(std::size_t Size, const CFile& File, int Line, bool Array)
{
    // Allocation de la m�moire
    void* Ptr = malloc(Size);

	if (m_Blocks)	// Ajout�
	{
		// Ajout du bloc � la liste des blocs allou�s
		TBlock NewBlock;
		NewBlock.Size  = Size;
		NewBlock.File  = File;
		NewBlock.Line  = Line;
		NewBlock.Array = Array;
		(*m_Blocks)[Ptr]  = NewBlock;

		// Loggization
		m_File << "++ Allocation    | 0x" << Ptr
			   << " | " << std::setw(7) << std::setfill(' ') << static_cast<int>(NewBlock.Size) << " octets"
			   << " | " << NewBlock.File.Filename() << " (" << NewBlock.Line << ")" << std::endl;
	}

    return Ptr;
}


////////////////////////////////////////////////////////////
// Retire une allocation m�moire
//
// [in] Ptr :   Adresse de la m�moire desallou�e
// [in] Array : True si c'est une d�sallocation de tableau
//
////////////////////////////////////////////////////////////
void CMemoryManager::Free(void* Ptr, bool Array)
{
    // Recherche de l'adresse dans les blocs allou�s
    TBlockMap::iterator It = m_Blocks->find(Ptr);

    // Si le bloc n'a pas �t� allou�, on g�n�re une erreur
    if (It == m_Blocks->end())
    {
        // En fait �a arrive souvent, du fait que le delete surcharg� est pris en compte m�me l� o� on n'inclue pas DebugNew.h,
        // mais pas la macro pour le new
        // Dans ce cas on d�truit le bloc et on quitte imm�diatement
        free(Ptr);
        return;
    }

    // Si le type d'allocation ne correspond pas, on g�n�re une erreur
    if (It->second.Array != Array)
    {
        //throw CBadDelete(Ptr, It->second.File.Filename(), It->second.Line, !Array);
		m_File << "Erreur : Le type d'allocation ne correspond pas !!!" << std::endl;
    }

#if 0	// Modifi� : Ancien :
    // Finalement, si tout va bien, on supprime le bloc et on loggiz tout �a
    m_File << "-- D�sallocation | 0x" << Ptr
           << " | " << std::setw(7) << std::setfill(' ') << static_cast<int>(It->second.Size) << " octets"
           << " | " << m_DeleteStack.top().File.Filename() << " (" << m_DeleteStack.top().Line << ")" << std::endl;
    m_Blocks->erase(It);
    m_DeleteStack->pop();
#else	// Modifi� : Nouveau :
	if (!m_DeleteStack->empty())
	{
		// Finalement, si tout va bien, on supprime le bloc et on loggiz tout �a
		m_File << "-- D�sallocation | 0x" << Ptr
			   << " | " << std::setw(7) << std::setfill(' ') << static_cast<int>(It->second.Size) << " octets"
			   << " | " << m_DeleteStack->top().File.Filename() << " (" << m_DeleteStack->top().Line << ")" << std::endl;
		m_Blocks->erase(It);
		m_DeleteStack->pop();
	}
	else	// Erreur : La pile m_DeleteStack est vide : Appara�t lorsque le delete a �t� appel� depuis un fichier n'incluant pas le #define sp�cial de ce delete, mais lib�rant une zone m�moire enregistr�e ici
	{
		m_File << "-- D�sallocation | 0x" << Ptr
			   << " | " << std::setw(7) << std::setfill(' ') << static_cast<int>(It->second.Size) << " octets | ???" << std::endl;
		m_Blocks->erase(It);
	}
#endif

    // Lib�ration de la m�moire
    free(Ptr);
}


////////////////////////////////////////////////////////////
// Sauvegarde les infos sur la d�sallocation en cours
//
// [in] File :  Fichier contenant la d�sallocation
// [in] Line :  Ligne de la d�sallocation
//
////////////////////////////////////////////////////////////
void CMemoryManager::NextDelete(const CFile& File, int Line)
{
    TBlock Delete;
    Delete.File = File;
    Delete.Line = Line;

    m_DeleteStack->push(Delete);
}

} // namespace Yes

#endif	// _DEBUG
