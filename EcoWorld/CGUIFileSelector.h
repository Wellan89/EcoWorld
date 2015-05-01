// Site : http://irrlicht.sourceforge.net/phpBB2/viewtopic.php?t=30241
// (Cette version a été très largement modifiée et la DriveBox a été remplacée par la FileNameEditBox, un bouton pour supprimer un fichier a été ajouté, la sélection de dossiers a été désactivée car elle n'était pas fonctionnelle)

#ifndef INC_CGUIFILESELECTOR_H
#define INC_CGUIFILESELECTOR_H

#include "global.h"

using namespace irr;
using namespace gui;

class CGUITexturedModalScreen;

/** Class for opening/saving files. */
class CGUIFileSelector : public IGUIFileOpenDialog
{
public:
	/** Enum to specify the usage of the instance of the class */   
	enum E_FILESELECTOR_TYPE
	{
		EFST_OPEN_DIALOG, //<! For opening files
		EFST_SAVE_DIALOG, //<! For saving files
		EFST_NUM_TYPES    //<! Not used, just specifies how many possible types there are
	};

	/**
	\brief Constructor
	\param title - The title of the dialog
	\pararm environment - The GUI environment to be used
	\param parent - The parent of the dialog
	\param id - The ID of the dialog
	\param type - The type of dialog
	*/
	CGUIFileSelector(const wchar_t* title, IGUIEnvironment* environment, IGUIElement* parent, s32 id, E_FILESELECTOR_TYPE type,
		const core::recti& rectangle, IGUISpriteBank* spriteBank);	// 2 derniers paramêtres ajoutés

	/**
	\brief Destructor
	*/
	virtual ~CGUIFileSelector();
	/**
	\brief Returns the filename of the selected file. Returns NULL, if no file was selected.
	\return a const wchar_t*
	*/
	virtual const wchar_t* getFileName() const { return NULL; }
	/**
	\brief Returns the filename of the selected file. Returns NULL, if no file was selected.
	\return a core::stringw
	*/
	core::stringw getFileNameStr() const;
	/**
	\brief called if an event happened.
	\param even - the event that happened
	\return a bool
	*/
	virtual bool OnEvent(const SEvent& event);
	/**
	\brief Render function
	*/
	virtual void draw();

	/**
	\brief Add a file filter
	\param name - The description of the file type
	\param ext - The file's extension
	\param texture - The icon texture
	*/
	void addFileFilter(wchar_t* name, wchar_t* ext, video::ITexture* texture);

	// Fonctions ajoutées :

	// Réaffiche cette fenêtre avec les mêmes paramêtres que lors de sa création
	void showAgain();

	// Masque cette fenêtre comme si elle était fermée
	void hide();

protected:
	/**
	\brief Returns true if the file extension is one of the registered filters
	\param s - the string to be checked
	\return a bool
	*/
	bool matchesFileFilter(const core::stringw& s);
	/**
	\brief Returns true if the file extension matches the specified filter
	\param s - the string to be checked
	\param f - the filter to check for
	\return a bool
	*/
	bool matchesFileFilter(const core::stringw& s, const core::stringw& f);

	/**
	\brief Fills the listbox with files.
	*/
	void fillListBox();

	/**
	\brief Sends the event that the file has been selected.
	*/
	void sendSelectedEvent();

	/**
	\brief Sends the event that the file choose process has been canceld
	*/
	void sendCancelEvent();

	u32 addIcon(video::ITexture* texture);

	/** Struct to describe file filters to use when displaying files in directories */
	struct SFileFilter
	{
		/*
		\brief Constructor
		\param name - The name/description of the filter
		\param filter - The file extension required
		\param texture - The texture to use as an icon for the file type
		*/
		SFileFilter(wchar_t* name, wchar_t* filter, video::ITexture* texture)
		{
			FilterName = name;
			FileExtension = filter;
			FileIcon = texture;
			FileIconIdx = 0;
		}
		SFileFilter& operator=(const SFileFilter& other)
		{
			FilterName = other.FilterName;
			FileExtension = other.FileExtension;
			FileIcon = other.FileIcon;
			FileIconIdx = other.FileIconIdx;
			return (*this);
		}
		core::stringw FilterName;
		core::stringw FileExtension;     
		video::ITexture* FileIcon;
		u32 FileIconIdx;
	};

	core::vector2di DragStart;
	bool Dragging;
	int FileIconIdx;
	int DirectoryIconIdx;
	IGUIButton* CloseButton;
	IGUIButton* OKButton;
	IGUIButton* CancelButton;
	IGUIEditBox* FileNameText;
	IGUIListBox* FileBox;
	IGUIEditBox* FileNameEditBox;
	IGUIComboBox* FilterComboBox;
	IGUISpriteBank* SpriteBank;
	io::IFileSystem* FileSystem;
	io::IFileList* FileList;
	core::array<SFileFilter> FileFilters;
	E_FILESELECTOR_TYPE DialogType;
	io::path prev_working_dir;

	// Ajouté :
	io::path current_dir;
	CGUITexturedModalScreen* modalScreen;
	IGUIButton* deleteFileButton;
	IGUIWindow* deleteFileConfirmationWindow;
	IGUIWindow* overwriteFileConfirmationWindow;

	// A utiliser dans la fonction OnEvent() lorsque l'utilisateur choisit le fichier sélectionné
	// forceOverride :	False si on doit demander une confirmation à l'utilisateur avant de remplacer un fichier existant pour l'enregistrement, true sinon
	bool selectFileEvent(bool forceOverride = false);

	void askUserDeleteFileConfirmation();
	void askUserOverwriteFileConfirmation();

public:
	/**
	\brief Returns the current file filter selected or "" if no filter is applied
	\return a stringw
	*/
	core::stringw getFileFilter() const
	{
		if (FilterComboBox->getSelected() >= (s32)FileFilters.size() || FilterComboBox->getSelected() < 0)
			return core::stringw();
		return FileFilters[FilterComboBox->getSelected()].FileExtension;
	}
	/**
	\brief Returns the type of the dialog
	\return a E_FILESELECTOR_TYPE
	*/
	E_FILESELECTOR_TYPE getDialogType()
	{
		return DialogType;
	}
	/**
	\brief Set an icon to use to display unknown files
	\param texture - the 16x16 icon to use
	*/
	void setCustomFileIcon(video::ITexture* texture)
	{
		if (texture)
			FileIconIdx = addIcon(texture);
		else
			FileIconIdx = -1;
		fillListBox();
	}
	/**
	\brief Set an icon to use to display directories
	\param texture - the 16x16 icon to use
	*/
	void setCustomDirectoryIcon(video::ITexture* texture)
	{
		if (texture)
			DirectoryIconIdx = addIcon(texture);
		else
			DirectoryIconIdx = -1;
		fillListBox();
	}

	// Fonctions inline ajoutées :

	//! Returns the directory of the selected file. Returns NULL, if no directory was selected.
	virtual const io::path& getDirectoryName()
	{
		return current_dir;	// Non supporté !
	}
	void setCurrentDirectory(const io::path& path)
	{
		if (!FileSystem)
			return;

		FileSystem->changeWorkingDirectoryTo(path);
		current_dir = FileSystem->getWorkingDirectory();

		fillListBox();
		if (FileNameText)
			FileNameText->setText(core::stringw(current_dir).c_str());

		// Sélectionne le fichier actuellement nommé dans la liste
		if (FileBox && FileNameEditBox)
		{
			// Sélectionne le fichier actuellement nommé dans la liste s'il est trouvé
			const core::stringw fileName = FileNameEditBox->getText();
			const u32 itemCount = FileBox->getItemCount();
			const u32 fileNameSize = fileName.size();
			FileBox->setSelected(-1);
			if (fileNameSize)
				for (u32 i = 0; i < itemCount; ++i)
					if (fileName.equalsn(FileBox->getListItem(i), fileNameSize))
						FileBox->setSelected(i);
		}
	}
};

#endif /* INC_CGUIFILESELECTOR_H */
