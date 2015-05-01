//==========================================================
// DebugNewOff.h
//
// Annulation des macros new et delete
// N�cessaire lorsqu'on a inclu DebugNew.h dans un header,
// il faut alors inclure DebugNewOff.h � la fin de celui-ci
// pour ne pas perturber le comportement des headers inclus
// par la suite
//
//==========================================================

// Cette version a �t� l�g�rement modifi�e

#ifdef _DEBUG

//#ifndef DEBUGNEWOFF_H
//#define DEBUGNEWOFF_H

#ifdef new

#undef new
#undef delete

#endif

//#endif // DEBUGNEWOFF_H

#endif	// _DEBUG
