//==========================================================
// DebugNew.h
//
// Surcharge des op�rateurs / fonctions d'allocation et de
// d�sallocation de la m�moire pour traquer les fuites
//
//==========================================================

#ifdef _DEBUG

#ifndef DEBUGNEW_H
#define DEBUGNEW_H

//==========================================================
// En-t�tes
//==========================================================
#include "MemoryManager.h"


////////////////////////////////////////////////////////////
// Surcharge de l'op�rateur new
//
// [in] Size : Taille � allouer
// [in] File : Fichier source
// [in] Line : Ligne dans le fichier source
//
// [retval] Pointeur sur la zone allou�e
//
////////////////////////////////////////////////////////////
inline void* operator new(std::size_t Size, const char* File, int Line)
{
    return Yes::CMemoryManager::Instance().Allocate(Size, File, Line, false);
}


////////////////////////////////////////////////////////////
// Surcharge de l'op�rateur new[]
//
// [in] Size : Taille � allouer
// [in] File : Fichier source
// [in] Line : Ligne dans le fichier source
//
// [retval] Pointeur sur la zone allou�e
//
////////////////////////////////////////////////////////////
inline void* operator new[](std::size_t Size, const char* File, int Line)
{
    return Yes::CMemoryManager::Instance().Allocate(Size, File, Line, true);
}


////////////////////////////////////////////////////////////
// Surcharge de l'op�rateur delete
//
// [in] Ptr : Pointeur sur la zone � lib�rer
//
////////////////////////////////////////////////////////////
inline void operator delete(void* Ptr)
{
    Yes::CMemoryManager::Instance().Free(Ptr, false);
}


////////////////////////////////////////////////////////////
// Surcharge de l'op�rateur delete
//
// [in] Ptr :  Pointeur sur la zone � lib�rer
// [in] File : Fichier source
// [in] Line : Ligne dans le fichier source
//
////////////////////////////////////////////////////////////
inline void operator delete(void* Ptr, const char* File, int Line)
{
    Yes::CMemoryManager::Instance().NextDelete(File, Line);
    Yes::CMemoryManager::Instance().Free(Ptr, false);
}


////////////////////////////////////////////////////////////
// Surcharge de l'op�rateur delete[]
//
// [in] Ptr : Pointeur sur la zone � lib�rer
//
////////////////////////////////////////////////////////////
inline void operator delete[](void* Ptr)
{
    Yes::CMemoryManager::Instance().Free(Ptr, true);
}


////////////////////////////////////////////////////////////
// Surcharge de l'op�rateur delete[]
//
// [in] Ptr :  Pointeur sur la zone � lib�rer
// [in] File : Fichier source
// [in] Line : Ligne dans le fichier source
//
////////////////////////////////////////////////////////////
inline void operator delete[](void* Ptr, const char* File, int Line)
{
    Yes::CMemoryManager::Instance().NextDelete(File, Line);
    Yes::CMemoryManager::Instance().Free(Ptr, true);
}

#endif // DEBUGNEW_H


//==========================================================
// D�finition de macros servant � automatiser le tracking
// /!\ sortir des directives anti-r�inclusions !
//==========================================================
#ifndef new
    #define new    new(__FILE__, __LINE__)
    #define delete Yes::CMemoryManager::Instance().NextDelete(__FILE__, __LINE__), delete
#endif

#endif // _DEBUG
