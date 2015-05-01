//==========================================================
// DebugNew.h
//
// Surcharge des opérateurs / fonctions d'allocation et de
// désallocation de la mémoire pour traquer les fuites
//
//==========================================================

#ifdef _DEBUG

#ifndef DEBUGNEW_H
#define DEBUGNEW_H

//==========================================================
// En-têtes
//==========================================================
#include "MemoryManager.h"


////////////////////////////////////////////////////////////
// Surcharge de l'opérateur new
//
// [in] Size : Taille à allouer
// [in] File : Fichier source
// [in] Line : Ligne dans le fichier source
//
// [retval] Pointeur sur la zone allouée
//
////////////////////////////////////////////////////////////
inline void* operator new(std::size_t Size, const char* File, int Line)
{
    return Yes::CMemoryManager::Instance().Allocate(Size, File, Line, false);
}


////////////////////////////////////////////////////////////
// Surcharge de l'opérateur new[]
//
// [in] Size : Taille à allouer
// [in] File : Fichier source
// [in] Line : Ligne dans le fichier source
//
// [retval] Pointeur sur la zone allouée
//
////////////////////////////////////////////////////////////
inline void* operator new[](std::size_t Size, const char* File, int Line)
{
    return Yes::CMemoryManager::Instance().Allocate(Size, File, Line, true);
}


////////////////////////////////////////////////////////////
// Surcharge de l'opérateur delete
//
// [in] Ptr : Pointeur sur la zone à libérer
//
////////////////////////////////////////////////////////////
inline void operator delete(void* Ptr)
{
    Yes::CMemoryManager::Instance().Free(Ptr, false);
}


////////////////////////////////////////////////////////////
// Surcharge de l'opérateur delete
//
// [in] Ptr :  Pointeur sur la zone à libérer
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
// Surcharge de l'opérateur delete[]
//
// [in] Ptr : Pointeur sur la zone à libérer
//
////////////////////////////////////////////////////////////
inline void operator delete[](void* Ptr)
{
    Yes::CMemoryManager::Instance().Free(Ptr, true);
}


////////////////////////////////////////////////////////////
// Surcharge de l'opérateur delete[]
//
// [in] Ptr :  Pointeur sur la zone à libérer
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
// Définition de macros servant à automatiser le tracking
// /!\ sortir des directives anti-réinclusions !
//==========================================================
#ifndef new
    #define new    new(__FILE__, __LINE__)
    #define delete Yes::CMemoryManager::Instance().NextDelete(__FILE__, __LINE__), delete
#endif

#endif // _DEBUG
