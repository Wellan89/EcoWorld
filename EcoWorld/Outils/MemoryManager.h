//==========================================================
// MemoryManager.h
//
// Définition de la classe CMemoryManager
//
//==========================================================

// Cette version a été modifiée : Passage de m_blocks et m_DeleteStack en tant que pointeurs : leurs destructeurs contiennent un appel à delete, ils doivent donc être détruits avant la destruction de cette classe pour éviter que leur destructeur ne l'appelle

#ifdef _DEBUG

#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

//==========================================================
// En-têtes
//==========================================================

#include "File.h"
#include <fstream>
#include <map>
#include <stack>
#include <string>

namespace Yes
{
    //==========================================================
    // Gestionnaire de mémoire - détecte les fuites
    //==========================================================
    class CMemoryManager
    {
    public :

        //----------------------------------------------------------
        // Renvoie l'instance de la classe
        //----------------------------------------------------------
        static CMemoryManager& Instance();

        //----------------------------------------------------------
        // Ajoute une allocation mémoire
        //----------------------------------------------------------
        void* Allocate(std::size_t Size, const CFile& File, int Line, bool Array);

        //----------------------------------------------------------
        // Retire une allocation mémoire
        //----------------------------------------------------------
        void Free(void* Ptr, bool Array);

        //----------------------------------------------------------
        // Sauvegarde les infos sur la désallocation courante
        //----------------------------------------------------------
        void NextDelete(const CFile& File, int Line);

    private :

        //----------------------------------------------------------
        // Constructeur par défaut
        //----------------------------------------------------------
        CMemoryManager();

        //----------------------------------------------------------
        // Destructeur
        //----------------------------------------------------------
        ~CMemoryManager();

        //----------------------------------------------------------
        // Inscrit le rapport sur les fuites de mémoire
        //----------------------------------------------------------
        void ReportLeaks();

        //----------------------------------------------------------
        // Types
        //----------------------------------------------------------
        struct TBlock
        {
            std::size_t Size;  // Taille allouée
            CFile       File;  // Fichier contenant l'allocation
            int         Line;  // Ligne de l'allocation
            bool        Array; // Est-ce un objet ou un tableau ?
        };
        typedef std::map<void*, TBlock> TBlockMap;

        //----------------------------------------------------------
        // Données membres
        //----------------------------------------------------------
        std::ofstream      m_File;        // Fichier de sortie
        TBlockMap*          m_Blocks;      // Blocs de mémoire alloués
        std::stack<TBlock>* m_DeleteStack; // Pile dont le sommet contient la ligne et le fichier de la prochaine désallocation
    };

} // namespace Yes

#endif // MEMORYMANAGER_H

#endif	// _DEBUG
