#include "nfd.h"

#include <windows.h>
#include <commdlg.h>

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

/* Windows half of the file dialogs. The upstream nativefiledialog shipped a
 * per-platform C file and only its Windows .lib was ever vendored here, so
 * both halves are ours now: this one and nfd_portal.cpp.
 *
 * GetOpenFileName rather than IFileDialog: it is a single call with no COM
 * lifetime to manage, and the editor only ever asks for one file.
 *
 * NFD's contract: on NFD_OKAY the caller owns the buffer and frees it with
 * free(), so the path is duplicated with _strdup and not new[]. */

namespace
{
	/* NFD filters look like "wld,prefab" or "Worlds:wld,prefab;Any:*".
	   Win32 wants "Label\0*.wld;*.prefab\0All Files\0*.*\0\0". */
	std::string BuildFilter(const nfdchar_t* pFilterList)
	{
		std::vector<std::string> extensions;
		std::string current;

		for (const char* p = pFilterList != nullptr ? pFilterList : ""; ; ++p)
		{
			const char c = *p;

			if (c == ',' || c == ';' || c == '\0')
			{
				if (!current.empty() && current != "*")
					extensions.push_back(current);

				current.clear();

				if (c == '\0')
					break;

				continue;
			}

			/* Drop the human-readable half of "Label:ext". */
			if (c == ':')
			{
				current.clear();
				continue;
			}

			current += c;
		}

		std::string filter;

		if (!extensions.empty())
		{
			std::string patterns;

			for (const std::string& extension : extensions)
			{
				if (!patterns.empty())
					patterns += ";";

				patterns += "*." + extension;
			}

			filter += "Supported";
			filter += '\0';
			filter += patterns;
			filter += '\0';
		}

		filter += "All Files";
		filter += '\0';
		filter += "*.*";
		filter += '\0';
		filter += '\0';

		return filter;
	}

	nfdresult_t Dialog(const nfdchar_t* pFilterList, const nfdchar_t* pDefaultPath,
	                   nfdchar_t** ppOutPath, bool bSave)
	{
		if (ppOutPath == nullptr)
			return NFD_ERROR;

		*ppOutPath = nullptr;

		char path[MAX_PATH] = { 0 };

		if (pDefaultPath != nullptr && pDefaultPath[0] != '\0')
		{
			strncpy_s(path, sizeof(path), pDefaultPath, _TRUNCATE);
		}

		const std::string filter = BuildFilter(pFilterList);

		OPENFILENAMEA info = {};
		info.lStructSize = sizeof(info);
		info.lpstrFilter = filter.c_str();
		info.lpstrFile = path;
		info.nMaxFile = sizeof(path);
		info.Flags = OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;

		if (bSave)
			info.Flags |= OFN_OVERWRITEPROMPT;
		else
			info.Flags |= OFN_FILEMUSTEXIST;

		const BOOL bPicked = bSave ? GetSaveFileNameA(&info) : GetOpenFileNameA(&info);

		if (!bPicked)
		{
			/* Zero means the user cancelled; anything else is a real failure. */
			return CommDlgExtendedError() == 0 ? NFD_CANCEL : NFD_ERROR;
		}

		*ppOutPath = _strdup(path);

		return *ppOutPath != nullptr ? NFD_OKAY : NFD_ERROR;
	}
}

extern "C"
{
	nfdresult_t NFD_OpenDialog(const nfdchar_t* filterList, const nfdchar_t* defaultPath,
	                           nfdchar_t** outPath)
	{
		return Dialog(filterList, defaultPath, outPath, false);
	}

	nfdresult_t NFD_SaveDialog(const nfdchar_t* filterList, const nfdchar_t* defaultPath,
	                           nfdchar_t** outPath)
	{
		return Dialog(filterList, defaultPath, outPath, true);
	}
}
