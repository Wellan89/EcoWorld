//==========================================================
// File.h
//
// Définition de la classe CFile
//
//==========================================================

// Cette version a été très légèrement modifiée

#ifndef FILE_H
#define FILE_H

#ifdef _DEBUG

//==========================================================
// En-têtes
//==========================================================
#include <string>


namespace Yes
{
    //==========================================================
    // Classe facilitant la manipulation des fichiers
    //==========================================================
    class CFile
    {
    public :

        //----------------------------------------------------------
        // Constructeur à partir d'un std::string
        //----------------------------------------------------------
        CFile(const std::string& Name = "unknown");

        //----------------------------------------------------------
        // Constructeur à partir d'un const char*
        //----------------------------------------------------------
        CFile(const char* Name);

        //----------------------------------------------------------
        // Indique si le fichier existe ou non
        //----------------------------------------------------------
        bool Exists() const;

        //----------------------------------------------------------
        // Renvoie le nom du fichier avec son chemin complet
        //----------------------------------------------------------
        const std::string& Fullname() const;

        //----------------------------------------------------------
        // Renvoie le nom du fichier sans son chemin
        //----------------------------------------------------------
        std::string Filename() const;

        //----------------------------------------------------------
        // Renvoie le nom du fichier sans extension ni chemin
        //----------------------------------------------------------
        std::string ShortFilename() const;

        //----------------------------------------------------------
        // Renvoie l'extension du fichier
        //----------------------------------------------------------
        std::string Extension() const;

    private :

        //----------------------------------------------------------
        // Données membres
        //----------------------------------------------------------
        std::string m_Name; // Chemin complet du fichier
    };

} // namespace Yes


#endif	// _DEBUG

#endif // FILE_H
