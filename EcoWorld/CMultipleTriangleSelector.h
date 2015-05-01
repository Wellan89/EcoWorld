#ifndef DEF_C_MULTIPLE_TRIANGLE_SELECTOR
#define DEF_C_MULTIPLE_TRIANGLE_SELECTOR

#include "global.h"

using namespace scene;

// Triangle Selector permettant de g�rer plusieurs triangle selector, mais en n'utilisant qu'un seul � la fois (changement de triangle selector ais� de cette mani�re)
class CMultipleTriangleSelector : public ITriangleSelector
{
protected:
	// Les triangle selectors contenus dans ce triangle selector, et l'ID du triangle selector actuellement s�lectionn�
	core::array<ITriangleSelector*> selectors;
	int currentSelector;

public:
	// Constructeur et destructeur inline
	CMultipleTriangleSelector(u32 selectorsSize = 0) : selectors(selectorsSize), currentSelector(-1)
	{
#ifdef _DEBUG
		setDebugName("CMultipleTriangleSelector");
#endif
	}
	virtual ~CMultipleTriangleSelector()
	{
		// Lib�re tous les triangle selectors
		const u32 selectorsSize = selectors.size();
		for (u32 i = 0; i < selectorsSize; ++i)
			if (selectors[i])
				selectors[i]->drop();
	}



	// Fonctions inline :

	// Ajoute un triangle selector � la liste actuelle
	void addTriangleSelector(ITriangleSelector* selector)
	{
		if (selector)
			selector->grab();
		selectors.push_back(selector);
	}
	// Obtient un triangle selector d'apr�s son index
	ITriangleSelector* getTriangleSelector(int index)
	{
		if (index < 0 || index >= (int)(selectors.size()))
			return NULL;

		return selectors[index];
	}
	// Obtient un triangle selector d'apr�s son index
	const ITriangleSelector* getTriangleSelector(int index) const
	{
		if (index < 0 || index >= (int)(selectors.size()))
			return NULL;

		return selectors[index];
	}
	// Indique quel triangle selector est actuellement utilis� pour les collisions
	int getCurrentTriangleSelector() const
	{
		return currentSelector;
	}
	// Indique quel triangle selector doit �tre utilis� pour les collisions
	void setCurrentTriangleSelector(int index)
	{
		currentSelector = index;
		if (currentSelector >= (int)(selectors.size()))
			currentSelector = -1;
	}



	// Fonctions surcharg�es de ITriangleSelector :

	//! Gets all triangles.
	virtual void getTriangles(core::triangle3df* triangles, int arraySize, int& outTriangleCount, const core::matrix4* transform = 0) const
	{
		outTriangleCount = 0;
		if (getTriangleSelector(currentSelector))
			selectors[currentSelector]->getTriangles(triangles, arraySize, outTriangleCount, transform);
	}

	//! Gets all triangles which lie within a specific bounding box.
	virtual void getTriangles(core::triangle3df* triangles, int arraySize, int& outTriangleCount, const core::aabbox3df& box, const core::matrix4* transform = 0) const
	{
		outTriangleCount = 0;
		if (getTriangleSelector(currentSelector))
			selectors[currentSelector]->getTriangles(triangles, arraySize, outTriangleCount, box, transform);
	}

	//! Gets all triangles which have or may have contact with a 3d line.
	virtual void getTriangles(core::triangle3df* triangles, int arraySize, int& outTriangleCount, const core::line3df& line, const core::matrix4* transform = 0) const
	{
		outTriangleCount = 0;
		if (getTriangleSelector(currentSelector))
			selectors[currentSelector]->getTriangles(triangles, arraySize, outTriangleCount, line, transform);
	}

	//! Returns amount of all available triangles in this selector
	virtual s32 getTriangleCount() const
	{
		if (getTriangleSelector(currentSelector))
			return selectors[currentSelector]->getTriangleCount();
		return 0;
	}

	//! Return the scene node associated with a given triangle.
	virtual ISceneNode* getSceneNodeForTriangle(u32 triangleIndex) const
	{
		if (getTriangleSelector(currentSelector))
			return selectors[currentSelector]->getSceneNodeForTriangle(triangleIndex);
		return NULL;
	}

	//! Get number of TriangleSelectors that are part of this one
	virtual u32 getSelectorCount() const
	{
		return selectors.size();
	}

	//! Get TriangleSelector based on index based on getSelectorCount
	virtual ITriangleSelector* getSelector(u32 index)
	{
		if (index < selectors.size())
			return selectors[index];
		return NULL;
	}

	//! Get TriangleSelector based on index based on getSelectorCount
	virtual const ITriangleSelector* getSelector(u32 index) const
	{
		if (index < selectors.size())
			return selectors[index];
		return NULL;
	}
};

#endif
