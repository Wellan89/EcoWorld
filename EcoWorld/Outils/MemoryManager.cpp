//==========================================================
// MemoryManager.cpp
//
// Implantation de la classe CMemoryManager
//
//==========================================================


//==========================================================
// En-têtes
//==========================================================

#ifdef _DEBUG

#include "MemoryManager.h"
#include <iomanip>

#include <iostream>			// Ajouté : Permet d'informer l'utilisateur dans la console que des fuites ont été détéctées

namespace Yes
{

////////////////////////////////////////////////////////////
// Constructeur par défaut
//
////////////////////////////////////////////////////////////
CMemoryManager::CMemoryManager() : m_File("Memory leaks.log"), m_Blocks(NULL), m_DeleteStack(NULL)
{
    // On vérifie que le fichier est bien ouvert
    if (!m_File)
		throw std::runtime_error("Memory leaks.log : Impossible d'accéder en écriture");

    // Inscription de l'en-tête du fichier
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
        // Fuites mémoires =(
        m_File << std::endl;
        m_File << "  ========================================" << std::endl;
        m_File << "   Oops... Some leaks have been detected  " << std::endl;
        m_File << "  ========================================" << std::endl;
        m_File << std::endl;

        ReportLeaks();

		// Ajouté : Informe l'utilisateur dans la console que des fuites ont été détéctées
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
// [retval] Référence sur l'instance unique de la classe
//
////////////////////////////////////////////////////////////
CMemoryManager& CMemoryManager::Instance()
{
    static CMemoryManager Inst;

    return Inst;
}


////////////////////////////////////////////////////////////
// Inscrit le rapport sur les fuites de mémoire
//
////////////////////////////////////////////////////////////
void CMemoryManager::ReportLeaks()
{
    // Détail des fuites
    std::size_t TotalSize = 0;
    for (TBlockMap::iterator i = m_Blocks->begin(); i != m_Blocks->end(); ++i)
    {
        // Ajout de la taille du bloc au cumul
        TotalSize += i->second.Size;

        // Inscription dans le fichier des informations sur le bloc courant
        m_File << "-> 0x" << i->first
               << " | "   << std::setw(7) << std::setfill(' ') << static_cast<int>(i->second.Size) << " octets"
               << " | "   << i->second.File.Filename() << " (" << i->second.Line << ")" << std::endl;

        // Libération de la mémoire
        free(i->first);
    }

    // Affichage du cumul des fuites
    m_File << std::endl << std::endl << "-- "
           << static_cast<int>(m_Blocks->size()) << " blocs non-libéré(s), "
           << static_cast<int>(TotalSize)       << " octets --"
           << std::endl;
}


////////////////////////////////////////////////////////////
// Ajoute une allocation mémoire
//
// [in] Size :  Taille allouée
// [in] File :  Fichier contenant l'allocation
// [in] Line :  Ligne de l'allocation
// [in] Array : True si c'est une allocation de tableau
//
// [retval] Pointeur sur la mémoire allouée
//
////////////////////////////////////////////////////////////
void* CMemoryManager::Allocate(std::size_t Size, const CFile& File, int Line, bool Array)
{
    // Allocation de la mémoire
    void* Ptr = malloc(Size);

	if (m_Blocks)	// Ajouté
	{
		// Ajout du bloc à la liste des blocs alloués
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
// Retire une allocation mémoire
//
// [in] Ptr :   Adresse de la mémoire desallouée
// [in] Array : True si c'est une désallocation de tableau
//
////////////////////////////////////////////////////////////
void CMemoryManager::Free(void* Ptr, bool Array)
{
    // Recherche de l'adresse dans les blocs alloués
    TBlockMap::iterator It = m_Blocks->find(Ptr);

    // Si le bloc n'a pas été alloué, on génère une erreur
    if (It == m_Blocks->end())
    {
        // En fait ça arrive souvent, du fait que le delete surchargé est pris en compte même là où on n'inclue pas DebugNew.h,
        // mais pas la macro pour le new
        // Dans ce cas on détruit le bloc et on quitte immédiatement
        free(Ptr);
        return;
    }

    // Si le type d'allocation ne correspond pas, on génère une erreur
    if (It->second.Array != Array)
    {
        //throw CBadDelete(Ptr, It->second.File.Filename(), It->second.Line, !Array);
		m_File << "Erreur : Le type d'allocation ne correspond pas !!!" << std::endl;
    }

#if 0	// Modifié : Ancien :
    // Finalement, si tout va bien, on supprime le bloc et on loggiz tout ça
    m_File << "-- Désallocation | 0x" << Ptr
           << " | " << std::setw(7) << std::setfill(' ') << static_cast<int>(It->second.Size) << " octets"
           << " | " << m_DeleteStack.top().File.Filename() << " (" << m_DeleteStack.top().Line << ")" << std::endl;
    m_Blocks->erase(It);
    m_DeleteStack->pop();
#else	// Modifié : Nouveau :
	if (!m_DeleteStack->empty())
	{
		// Finalement, si tout va bien, on supprime le bloc et on loggiz tout ça
		m_File << "-- Désallocation | 0x" << Ptr
			   << " | " << std::setw(7) << std::setfill(' ') << static_cast<int>(It->second.Size) << " octets"
			   << " | " << m_DeleteStack->top().File.Filename() << " (" << m_DeleteStack->top().Line << ")" << std::endl;
		m_Blocks->erase(It);
		m_DeleteStack->pop();
	}
	else	// Erreur : La pile m_DeleteStack est vide : Apparaît lorsque le delete a été appelé depuis un fichier n'incluant pas le #define spécial de ce delete, mais libérant une zone mémoire enregistrée ici
	{
		m_File << "-- Désallocation | 0x" << Ptr
			   << " | " << std::setw(7) << std::setfill(' ') << static_cast<int>(It->second.Size) << " octets | ???" << std::endl;
		m_Blocks->erase(It);
	}
#endif

    // Libération de la mémoire
    free(Ptr);
}


////////////////////////////////////////////////////////////
// Sauvegarde les infos sur la désallocation en cours
//
// [in] File :  Fichier contenant la désallocation
// [in] Line :  Ligne de la désallocation
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
