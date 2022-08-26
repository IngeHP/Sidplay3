// stilview.h

#ifndef _STILVIEW_H_
#define _STILVIEW_H_

#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>		// MAX_PATH
#include "stil.h"


class STILView {
 public:
	
	// CONSTRUCTOR
	//	Initializes member variables, reading from INI-file
	STILView();

	// DESTRUCTOR
	//	Closes window, stores variables in INI-file
	~STILView();

	// showEntry()
	//
	// FUNCTION: Looks up the STIL/Bug entry for the given filename.
	//           If there are entries to show, the STIL Window will be updated
	//           with the entry. If not, the STIL Window is hidden.
	//           Only the type of info (STIL/Global/Bug) that are enabled will
	//			 will be shown.
	// ARGUMENTS:
	//      fullpath - The full path of the sidtune to show STIL info for
	//		tuneNo   - The subtune of the sidfile to show STIL info for
	// RETURNS:
	//      -
	//
	void showEntry(char *fullPath, int tuneNo=1);

	// setVisibility()
	//
	// FUNCTION: enables/disables the display of the three types of STIL/Bug
	//			 entries.
	// ARGUMENTS:
	//		bShowEntry  - disable/enable display of STIL entries
	//		bShowBug    - disable/enable display of BUGlist entries
	//		bShowGlobal - disable/enable display of STIL section-global comments
	// RETURNS:
	//		-
	//
	void setVisibility(bool bShowEntry, bool bShowBug, bool bShowGlobal);

	// toshowEntry(), toshowBug(), toshowGlobal()
	//
	// RETURNS:
	//		bool - the current setting; if that type of entry is to be shown
	inline bool toshowEntry();
	inline bool toshowBug();
	inline bool toshowGlobal();

	// bAllSubtunes
	// If set to true, the STIL View will show info for all sub-tunes in the
	// given sid. If false, only the current subtune's info will be displayed.
	bool bAllSubtunes;

	// setHVSCdir()
	//
	// FUNCTION: sets the HVSC base directory. Initialises the STIL class, i.e.
	//			 loads the STIL and BUGlist files if they are found.
	// ARGUMENT:
	//		dir - the directory where HVSC is installed. May end in a backslash,
	//			  but it doesn't have to.
	// RETURNS:
	//		-
	void setHVSCdir(const char *dir);

	// getHVSCdir()
	//
	// FUNCTION: -
	// ARGUMENT: -
	// RETURNS:
	//		const char * - the directory where STILView expects to find the
	//		HVSC. Note that it may be a directory that doesn't exist, or one
	//		that doesn't host the HVSC.
	const char *getHVSCdir();

	// getSTILClassVersionNo()
	//
	// returns the version number (a float) of the STIL class used
	inline double getSTILClassVersionNo();

	// setMaxAutoHeight
	//
	// FUNCTION: Sets the _maximum_ height that the STIL View window
	//           will auto-size to. It will never be less than zero.
	// ARGUMENT:
	//		height - the wanted maximum height
	// RETURNS:
	//		int - the new maximum height
	inline int setMaxAutoHeight (int height);

	// getMaxAutoheight() - returns the maximum auto-size height
	inline int getMaxAutoHeight();

	bool bAutoSize;		// Automatic or manual resizing of STIL View

	// Change displayfont for STIL
	void setFont(HFONT newFont);


 private:
	// Creates the STIL View window
	HWND createSTILWindow (HWND hParent);
	// Destroys the STIL View window
	void closeSTILWindow();

	// getRelPath()
	//
	// FUNCTION: Converts a full path to a sidtune into a relative path (from
	//			 the HVSC directory). Also corrects case and expands subdirectory
	//			 names and filenames to the long representation.
	// ARGUMENT:
	//		fullPath - the full path name to be converted
	//		relPath  - a buffer to hold the converted relative path.
	//				   The buffer must at least be of size MAX_PATH.
	// RETURNS:
	//		bool - whether the path is a valid path residing within the HVSC
	//			   directory tree or not. If false is returned, relPath will be
	//			   the empty string.
	//

	// Removes the trailing newline of the variable 'comment'.
	void removeTrailingNewline();

	// Puts the text into the listbox, line by line
	void textToListbox (HWND hList, const char *text);

	string entry;
	bool showSTIL;	// must only be true if HVSCbase exists  TODO get rid of/change!
	bool bShowEntry;
	bool bShowBug;
	bool bShowGlobal;
	char HVSCdir[MAX_PATH];
	int	 x, y;			// Window position
	int  width, height;	// Window size
	int  maxAutoHeight;	// Maximum height the window will auto-size to
	HWND STILWindow;
	HFONT myFont = NULL;
	STIL theSTIL;
	char comment[STIL_MAX_ENTRY_SIZE*2];
	static LRESULT CALLBACK STILWindowProc (HWND, UINT, WPARAM, LPARAM);
};


inline bool
STILView::toshowEntry()
{
	return bShowEntry;
}

inline bool
STILView::toshowBug()
{
	return bShowBug;
}

inline bool
STILView::toshowGlobal()
{
	return bShowGlobal;
}

inline double
STILView::getSTILClassVersionNo()
{
	return theSTIL.getVersionNo();
}

inline int
STILView::setMaxAutoHeight (int height)
{
	return maxAutoHeight = max(0, height);
}

inline int
STILView::getMaxAutoHeight()
{
	return maxAutoHeight;
}

#endif _STILVIEW_H_
