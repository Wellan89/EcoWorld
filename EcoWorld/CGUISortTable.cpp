#include "CGUISortTable.h"
#include <sstream>	// Pour l'utilisation de la structure wistringstream de la STL (permet la conversion cha�ne de caract�res -> float)

CGUISortTable::CGUISortTable(IGUIEnvironment* environment, IGUIElement* parent, int id, const core::recti& rectangle, bool clip, bool drawBack, bool moveOverSelect)
 : CGUITable(environment, parent, id, rectangle, clip, drawBack, moveOverSelect)
{
}
void CGUISortTable::orderRows(int columnIndex, EGUI_ORDERING_MODE mode)
{
	// Bas� sur la fonction CGUITable::orderRows(s32 columnIndex, EGUI_ORDERING_MODE mode) d'Irrlicht 1.7.2 :

	Row swap;

	if ( columnIndex == -1 )
		columnIndex = getActiveColumn();
	if ( columnIndex < 0 )
		return;

	if ( mode == EGOM_ASCENDING )
	{
		for ( s32 i = 0 ; i < s32(Rows.size()) - 1 ; ++i )
		{
			for ( s32 j = 0 ; j < s32(Rows.size()) - i - 1 ; ++j )
			{
				//if ( Rows[j+1].Items[columnIndex].Text < Rows[j].Items[columnIndex].Text )
				if (compareCells(Rows[j+1].Items[columnIndex], Rows[j].Items[columnIndex]))
				{
					swap = Rows[j];
					Rows[j] = Rows[j+1];
					Rows[j+1] = swap;

					if ( Selected == j )
						Selected = j+1;
					else if( Selected == j+1 )
						Selected = j;
				}
			}
		}
	}
	else if ( mode == EGOM_DESCENDING )
	{
		for ( s32 i = 0 ; i < s32(Rows.size()) - 1 ; ++i )
		{
			for ( s32 j = 0 ; j < s32(Rows.size()) - i - 1 ; ++j )
			{
				//if ( Rows[j].Items[columnIndex].Text < Rows[j+1].Items[columnIndex].Text)
				if (compareCells(Rows[j].Items[columnIndex], Rows[j+1].Items[columnIndex]))
				{
					swap = Rows[j];
					Rows[j] = Rows[j+1];
					Rows[j+1] = swap;

					if ( Selected == j )
						Selected = j+1;
					else if( Selected == j+1 )
						Selected = j;
				}
			}
		}
	}
}
bool CGUISortTable::compareCells(const Cell& cell1, const Cell& cell2)
{
	// D�termine quel mode de tri utiliser d'apr�s la valeur utilisateur de la cellule 1
	const E_TRI_TYPES triType = (cell1.Data ? (*((E_TRI_TYPES*)cell1.Data)) : ETT_ALPHABETICAL);

	switch (triType)
	{
	case ETT_ALPHABETICAL:	// Mode comparaison alphab�tique (ASCII) :
		return (cell1.Text < cell2.Text);

	case ETT_VALUE:			// Mode comparaison par nombres :
		{
			const float cell1Val = getFloatValue(cell1.Text);
			const float cell2Val = getFloatValue(cell2.Text);
			return (cell1Val < cell2Val);
		}

	case ETT_TIME:
		{
			const float cell1Val = getTimeValue(cell1.Text);
			const float cell2Val = getTimeValue(cell2.Text);

			// G�re les probl�mes de temps infini :
			if (cell1Val == -1.0f)								// Cellule 1 : "Infini" ; Cellule 2 : "Temps fini" ou "Infini"
				return false;	// -> On ne change pas l'ordre des cellules	(Cellule 1 < Cellule 2 : false)
			else if (cell1Val != -1.0f && cell2Val == -1.0f)	// Cellule 1 : "Temps fini" ; Cellule 2 : "Infini"
				return true;	// -> On change l'ordre des cellules		(Cellule 1 < Cellule 2 : true)

			// Compare normalement les temps
			return (cell1Val < cell2Val);
		}

	case ETT_IP_ADDRESS:
		{
			// Compare les permiers membres, et d�termine le tri des cellules si l'un est diff�rent de l'autre
			const float cell1Val1 = getIPAddressMemberValue(cell1.Text, 0);
			const float cell2Val1 = getIPAddressMemberValue(cell2.Text, 0);
			if (cell1Val1 != cell2Val1)
				return (cell1Val1 < cell2Val1);

			// Compare les seconds membres, et d�termine le tri des cellules si l'un est diff�rent de l'autre
			const float cell1Val2 = getIPAddressMemberValue(cell1.Text, 1);
			const float cell2Val2 = getIPAddressMemberValue(cell2.Text, 1);
			if (cell1Val2 != cell2Val2)
				return (cell1Val2 < cell2Val2);

			// Compare les troisi�mes membres, et d�termine le tri des cellules si l'un est diff�rent de l'autre
			const float cell1Val3 = getIPAddressMemberValue(cell1.Text, 2);
			const float cell2Val3 = getIPAddressMemberValue(cell2.Text, 2);
			if (cell1Val3 != cell2Val3)
				return (cell1Val3 < cell2Val3);

			// Compare les quatri�mes membres, et d�termine (toujours) le tri final des cellules
			const float cell1Val4 = getIPAddressMemberValue(cell1.Text, 3);
			const float cell2Val4 = getIPAddressMemberValue(cell2.Text, 3);
			return (cell1Val4 < cell2Val4);
		}
		break;
	}

	LOG_DEBUG("CGUISortTable::compareCells(cell1, cell2) : Type de tri inconnu : triType = " << triType, ELL_WARNING);
	return false;
}
float CGUISortTable::getFloatValue(const core::stringw& valueStr)
{
	// V�rifie que la valeur n'est pas vide ("-")
	if (valueStr == L"-")
		return 0.0f;	// Si c'est le cas, on indique que cette valeur est nulle en retournant 0.0f

	// Conserve uniquement les chiffres et le '.' de la cha�ne de caract�res fournie
	core::stringw newValueStr;	// La nouvelle cha�ne contenant la valeur recherch�e
	const u32 valueStrSize = valueStr.size();
	for (u32 i = 0; i <= valueStrSize; ++i)	// On utilise '<=' pour que la valeur de i puisse aussi prendre le dernier caract�re de la cha�ne
	{
		// Obtient le caract�re actuel
		wchar_t c = (valueStr.c_str())[i];

		if ((c >= L'0' && c <= L'9') || c == L'.')	// V�rifie que ce caract�re est un chiffre ou un point
		{
			// Ajoute ce caract�re � la nouvelle cha�ne de la valeur
			newValueStr.append(c);
		}
	}

	// Utilise la structure wistringstream de la STL pour convertir cette cha�ne en float
	float val = 0.0f;
	wistringstream str(newValueStr.c_str());
	str >> val;

	// Si la conversion a �chou�e, on retourne une valeur par d�faut
	if (str.fail())
		return 0.0f;

	return val;
}
float CGUISortTable::getTimeValue(const core::stringw& valueStr)
{
	// V�rifie que le temps n'est pas vide ("-")
	if (valueStr == L"-")
		return 0.0f;	// Si c'est le cas, on indique que ce temps est nul en retournant 0.0f

	// V�rifie que le temps ne contient pas "Infini" (doit aussi fonctionner avec "Infinie")
	if (valueStr.equalsn(L"Infini", 6))
		return -1.0f;	// Si c'est le cas, on indique que ce temps est infini en retournant -1.0f

	// Analyse la cha�ne de caract�res fournie, qui doit �tre sous la forme d'un temps �coul� ("[X ann�e[s] ][Y mois][ ][Z jour[s]]") :

	// Obtient le nombre d'ann�es dans cette date :
	float annees = 0.0f;
	const int anneesPos = valueStr.find(L"ann�e");
	if (anneesPos != -1)	// Cherche si les ann�es sont indiqu�es
	{
		// Si c'est le cas, on cr�e une sous-cha�ne pour les ann�es qu'on demandera � la fonction getFloatValue d'analyser pour nous retourner sa valeur
		const core::stringw substring = valueStr.subString(0, anneesPos);
		annees = getFloatValue(substring);
	}

	// Obtient le nombre de mois dans cette date :
	float mois = 0.0f;
	const int moisPos = valueStr.find(L"mois");
	if (moisPos != -1)	// Cherche si les mois sont indiqu�s
	{
		// Si c'est le cas, on cr�e une sous-cha�ne pour les mois qu'on demandera � la fonction getFloatValue d'analyser pour nous retourner sa valeur
		const int substringStart = max(anneesPos, 0);
		const core::stringw substring = valueStr.subString((u32)substringStart, moisPos - substringStart);
		mois = getFloatValue(substring);
	}

	// Obtient le nombre de jours dans cette date :
	float jours = 0.0f;
	const int joursPos = valueStr.find(L"jour");
	if (joursPos != -1)	// Cherche si les jours sont indiqu�s
	{
		// Si c'est le cas, on cr�e une sous-cha�ne pour les jours qu'on demandera � la fonction getFloatValue d'analyser pour nous retourner sa valeur
		const u32 substringStart = core::max_(anneesPos, moisPos, 0);
		const core::stringw substring = valueStr.subString((u32)substringStart, joursPos - substringStart);
		jours = getFloatValue(substring);
	}

	// Retourne la valeur finale de la date en jours : (Annees * 12 + Mois) * 30 + Jours
	return ((annees * 12.0f + mois) * 30.0f + jours);
}
float CGUISortTable::getIPAddressMemberValue(const core::stringw& valueStr, u32 member)
{
	// Analyse la cha�ne de caract�res fournie, qui doit �tre sous la forme d'une addresse IP ("W.X.Y.Z[.A.B.C...]") :

	// Recherche le premier point o� commence le nombre � extraire (ex : le 2e si member = 2)
	int pointNbStart = 0;
	for (u32 i = 0; i < member; ++i)
	{
		pointNbStart = valueStr.findNext(L'.', pointNbStart);

		if (pointNbStart == -1)	// Nombre de points dans la cha�ne de caract�res fournie insuffisant !
			return 0.0f;

		pointNbStart++;			// Avance la position du point trouv�e, pour pouvoir rechercher le point suivant, ou obtenir le premier caract�re suivant le point
	}

	// Recherche le dernier point o� se termine le nombre � extraire (ex : le 3e si member = 2)
	int pointNbEnd = valueStr.findNext(L'.', pointNbStart);
	if (pointNbEnd == -1)		// Nombre de points dans la cha�ne de caract�res fournie insuffisant (normal dans le cas o� member est la derni�re valeur de la cha�ne)
		pointNbEnd = valueStr.size();	// On modifie alors la position finale de la sous-cha�ne � la fin de la cha�ne fournie

	// On cr�e une sous-cha�ne pour obtenir la valeur num�rique ainsi encercl�e, qu'on demandera � la fonction getFloatValue d'analyser pour nous retourner sa valeur
	const core::stringw substring = valueStr.subString((u32)pointNbStart, pointNbEnd - pointNbStart);
	return getFloatValue(substring);
}
