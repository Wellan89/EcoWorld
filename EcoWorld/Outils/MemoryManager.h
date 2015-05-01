//==========================================================
// MemoryManager.h
//
// D�finition de la classe CMemoryManager
//
//==========================================================

// Cette version a �t� modifi�e : Passage de m_blocks et m_DeleteStack en tant que pointeurs : leurs destructeurs contiennent un appel � delete, ils doivent donc �tre d�truits avant la destruction de cette classe pour �viter que leur destructeur ne l'appelle

#ifdef _DEBUG

#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

//==========================================================
// En-t�tes
//==========================================================

#include "File.h"
#include <fstream>
#include <map>
#include <stack>
#include <string>

namespace Yes
{
    //==========================================================
    // Gestionnaire de m�moire - d�tecte les fuites
    //==========================================================
    class CMemoryManager
    {
    public :

        //----------------------------------------------------------
        // Renvoie l'instance de la classe
        //----------------------------------------------------------
        static CMemoryManager& Instance();

        //----------------------------------------------------------
        // Ajoute une allocation m�moire
        //----------------------------------------------------------
        void* Allocate(std::size_t Size, const CFile& File, int Line, bool Array);

        //----------------------------------------------------------
        // Retire une allocation m�moire
        //----------------------------------------------------------
        void Free(void* Ptr, bool Array);

        //----------------------------------------------------------
        // Sauvegarde les infos sur la d�sallocation courante
        //----------------------------------------------------------
        void NextDelete(const CFile& File, int Line);

    private :

        //----------------------------------------------------------
        // Constructeur par d�faut
        //----------------------------------------------------------
        CMemoryManager();

        //----------------------------------------------------------
        // Destructeur
        //----------------------------------------------------------
        ~CMemoryManager();

        //----------------------------------------------------------
        // Inscrit le rapport sur les fuites de m�moire
        //----------------------------------------------------------
        void ReportLeaks();

        //----------------------------------------------------------
        // Types
        //----------------------------------------------------------
        struct TBlock
        {
            std::size_t Size;  // Taille allou�e
            CFile       File;  // Fichier contenant l'allocation
            int         Line;  // Ligne de l'allocation
            bool        Array; // Est-ce un objet ou un tableau ?
        };
        typedef std::map<void*, TBlock> TBlockMap;

        //----------------------------------------------------------
        // Donn�es membres
        //----------------------------------------------------------
        std::ofstream      m_File;        // Fichier de sortie
        TBlockMap*          m_Blocks;      // Blocs de m�moire allou�s
        std::stack<TBlock>* m_DeleteStack; // Pile dont le sommet contient la ligne et le fichier de la prochaine d�sallocation
    };

} // namespace Yes

#endif // MEMORYMANAGER_H

#endif	// _DEBUG
