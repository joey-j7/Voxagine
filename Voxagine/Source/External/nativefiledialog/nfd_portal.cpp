#include "nfd.h"

#include <SDL3/SDL.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <poll.h>
#include <string>
#include <unistd.h>
#include <vector>

/* The upstream nativefiledialog ships a per-platform C file and the project
 * only ever linked the Windows one. Rather than vendor the GTK backend and
 * take a GTK dependency on an SDL/Vulkan application, this shells out to
 * whichever portal-aware dialog the desktop already provides.
 *
 * NFD's contract: on NFD_OKAY the caller owns the returned buffer and frees it
 * with free(), so the path is duplicated with strdup and not new[]. */

namespace
{
	/* NFD filters look like "wld,prefab" or "Worlds:wld,prefab;Any:*". Only
	   the extension list matters for the shell dialogs below. */
	std::vector<std::string> ParseExtensions(const nfdchar_t* pFilterList)
	{
		std::vector<std::string> extensions;

		if (pFilterList == nullptr)
			return extensions;

		std::string current;

		for (const char* p = pFilterList; ; ++p)
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

		return extensions;
	}

	bool HasCommand(const char* pName)
	{
		const std::string probe = std::string("command -v ") + pName + " > /dev/null 2>&1";
		return std::system(probe.c_str()) == 0;
	}

	std::string ShellQuote(const std::string& s)
	{
		std::string quoted = "'";

		for (char c : s)
		{
			if (c == '\'')
				quoted += "'\\''";
			else
				quoted += c;
		}

		quoted += "'";

		return quoted;
	}

	nfdresult_t RunDialog(const std::string& command, nfdchar_t** ppOutPath)
	{
		FILE* pPipe = popen(command.c_str(), "r");

		if (pPipe == nullptr)
			return NFD_ERROR;

		std::string output;
		char buffer[512];

		/* Polled, not read straight through: a main thread parked in read()
		   stops answering the compositor's pings and the desktop declares the
		   editor hung. SDL_PumpEvents sends the reply; the events it collects
		   reach the normal Poll() once the dialog closes. */
		const int iFileDescriptor = fileno(pPipe);

		for (;;)
		{
			pollfd descriptor{};
			descriptor.fd = iFileDescriptor;
			descriptor.events = POLLIN;

			const int iReady = poll(&descriptor, 1, 16);

			SDL_PumpEvents();

			if (iReady < 0)
			{
				if (errno == EINTR)
					continue;

				break;
			}

			if (iReady == 0)
				continue;

			const ssize_t iBytes = read(iFileDescriptor, buffer, sizeof(buffer));

			/* 0 is the child closing its end, below 0 a real error. */
			if (iBytes <= 0)
				break;

			output.append(buffer, static_cast<size_t>(iBytes));
		}

		const int iStatus = pclose(pPipe);

		while (!output.empty() && (output.back() == '\n' || output.back() == '\r'))
			output.pop_back();

		if (iStatus != 0 || output.empty())
			return NFD_CANCEL;

		*ppOutPath = strdup(output.c_str());

		return *ppOutPath != nullptr ? NFD_OKAY : NFD_ERROR;
	}

	nfdresult_t Dialog(const nfdchar_t* pFilterList, const nfdchar_t* pDefaultPath,
	                   nfdchar_t** ppOutPath, bool bSave)
	{
		if (ppOutPath == nullptr)
			return NFD_ERROR;

		*ppOutPath = nullptr;

		const std::vector<std::string> extensions = ParseExtensions(pFilterList);

		if (HasCommand("zenity"))
		{
			std::string command = "zenity --file-selection";

			if (bSave)
				command += " --save --confirm-overwrite";

			if (pDefaultPath != nullptr && pDefaultPath[0] != '\0')
				command += " --filename=" + ShellQuote(pDefaultPath);

			for (const std::string& extension : extensions)
				command += " --file-filter=" + ShellQuote("*." + extension);

			command += " 2>/dev/null";

			return RunDialog(command, ppOutPath);
		}

		if (HasCommand("kdialog"))
		{
			std::string filter;

			for (const std::string& extension : extensions)
			{
				if (!filter.empty())
					filter += " ";

				filter += "*." + extension;
			}

			std::string command = "kdialog ";
			command += bSave ? "--getsavefilename " : "--getopenfilename ";
			command += (pDefaultPath != nullptr && pDefaultPath[0] != '\0')
				? ShellQuote(pDefaultPath) : std::string(".");

			if (!filter.empty())
				command += " " + ShellQuote(filter);

			command += " 2>/dev/null";

			return RunDialog(command, ppOutPath);
		}

		fprintf(stderr, "[nfd] no file dialog available; install zenity or kdialog\n");

		return NFD_CANCEL;
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
