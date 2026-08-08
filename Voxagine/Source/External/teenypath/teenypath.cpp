#include "teenypath.h"

#include <algorithm>
#include <filesystem>
#include <system_error>

/* TeenyPath shipped as a header plus a Windows .lib, so the editor had nothing
 * to link against here. std::filesystem covers the whole surface; this is a
 * thin adapter rather than a port of the original implementation.
 *
 * The one thing worth knowing: TeenyPath stores paths generically ('/') and
 * offers native_string() for the platform form. On Linux those are the same,
 * but callers still expect backslashes to be accepted on input, because the
 * project's saved paths were written on Windows. */

namespace
{
	std::filesystem::path ToStd(const std::string& s)
	{
		return std::filesystem::path(s);
	}

	std::string ToGeneric(const std::filesystem::path& p)
	{
		return p.generic_string();
	}

	std::string NormalizeSeparators(const std::string& s)
	{
		std::string out = s;
		std::replace(out.begin(), out.end(), '\\', '/');

		return out;
	}
}

namespace TeenyPath
{
	path::path(const std::string& s) : m_path(NormalizeSeparators(s)) {}
	path::path(const char* s) : m_path(NormalizeSeparators(s != nullptr ? s : "")) {}

	path::path(const std::wstring& s)
	{
		m_path = NormalizeSeparators(ToGeneric(std::filesystem::path(s)));
	}

	path::path(const wchar_t* s)
	{
		m_path = s != nullptr ? NormalizeSeparators(ToGeneric(std::filesystem::path(s))) : "";
	}

	bool path::exists() const
	{
		std::error_code error;
		return std::filesystem::exists(ToStd(m_path), error);
	}

	bool path::is_absolute() const
	{
		return ToStd(m_path).is_absolute();
	}

	bool path::is_directory() const
	{
		std::error_code error;
		return std::filesystem::is_directory(ToStd(m_path), error);
	}

	bool path::is_empty() const
	{
		return m_path.empty();
	}

	bool path::is_lexically_normal() const
	{
		const std::filesystem::path p = ToStd(m_path);
		return p == p.lexically_normal();
	}

	bool path::is_regular_file() const
	{
		std::error_code error;
		return std::filesystem::is_regular_file(ToStd(m_path), error);
	}

	bool path::is_root() const
	{
		const std::filesystem::path p = ToStd(m_path);
		return !p.empty() && p == p.root_path();
	}

	bool path::is_symlink() const
	{
		std::error_code error;
		return std::filesystem::is_symlink(ToStd(m_path), error);
	}

	std::string path::extension() const
	{
		return ToStd(m_path).extension().string();
	}

	std::string path::filename() const
	{
		return ToStd(m_path).filename().string();
	}

	std::string path::string() const
	{
		return m_path;
	}

	std::string path::generic_string() const
	{
		return m_path;
	}

	std::string path::native_string() const
	{
		return ToStd(m_path).string();
	}

	std::wstring path::wfilename() const
	{
		return ToStd(m_path).filename().wstring();
	}

	std::wstring path::wstring() const
	{
		return ToStd(m_path).wstring();
	}

	path path::lexically_normalized() const
	{
		return path(ToGeneric(ToStd(m_path).lexically_normal()));
	}

	path path::parent_path() const
	{
		return path(ToGeneric(ToStd(m_path).parent_path()));
	}

	path path::resolve_absolute() const
	{
		std::error_code error;
		const std::filesystem::path resolved = std::filesystem::canonical(ToStd(m_path), error);

		/* canonical fails on a path that does not exist yet - a Save As target,
		   for instance - so fall back to a purely lexical absolute. */
		if (error)
		{
			std::error_code absoluteError;
			const std::filesystem::path fallback =
				std::filesystem::absolute(ToStd(m_path), absoluteError);

			if (absoluteError)
				return *this;

			return path(ToGeneric(fallback.lexically_normal()));
		}

		return path(ToGeneric(resolved));
	}

	path path::trim_trailing_slashes() const
	{
		std::string trimmed = m_path;

		while (trimmed.size() > 1 && trimmed.back() == '/')
			trimmed.pop_back();

		return path(trimmed);
	}

	void path::replace_extension(const std::string& new_extension)
	{
		std::filesystem::path p = ToStd(m_path);
		p.replace_extension(new_extension);

		m_path = ToGeneric(p);
	}

	path& path::operator/=(const path& p)
	{
		return operator/=(p.generic_string());
	}

	path& path::operator/=(const std::string& s)
	{
		m_path = ToGeneric(ToStd(m_path) / ToStd(NormalizeSeparators(s)));
		return *this;
	}

	path path::get_root() const
	{
		return path(ToGeneric(ToStd(m_path).root_path()));
	}

	std::vector<std::string> path::split() const
	{
		std::vector<std::string> parts;

		for (const std::filesystem::path& part : ToStd(m_path))
			parts.push_back(part.string());

		return parts;
	}

	path operator/(const path& lhs, const path& rhs)
	{
		path result = lhs;
		result /= rhs;

		return result;
	}

	path operator/(const path& lhs, const std::string& rhs)
	{
		path result = lhs;
		result /= rhs;

		return result;
	}

	path operator/(const path& lhs, const char* rhs)
	{
		path result = lhs;
		result /= std::string(rhs != nullptr ? rhs : "");

		return result;
	}

	bool operator==(const path& lhs, const path& rhs)
	{
		return lhs.generic_string() == rhs.generic_string();
	}

	std::string joinPathList(const std::vector<path>& pathList)
	{
		/* ':' on POSIX, as in PATH. The Windows build used ';'. */
		std::string joined;

		for (const path& p : pathList)
		{
			if (!joined.empty())
				joined += ':';

			joined += p.generic_string();
		}

		return joined;
	}

	std::vector<path> splitPathList(const std::string& pathList)
	{
		std::vector<path> paths;
		std::string current;

		for (char c : pathList)
		{
			if (c == ':' || c == ';')
			{
				if (!current.empty())
					paths.push_back(path(current));

				current.clear();
				continue;
			}

			current += c;
		}

		if (!current.empty())
			paths.push_back(path(current));

		return paths;
	}

	std::vector<path> ls(const path& p)
	{
		std::vector<path> entries;

		std::error_code error;
		std::filesystem::directory_iterator iter(ToStd(p.generic_string()), error);

		if (error)
			return entries;

		for (const std::filesystem::directory_entry& entry : iter)
			entries.push_back(path(ToGeneric(entry.path())));

		/* The directory order the filesystem hands back is arbitrary; the
		   editor's browser lists these directly, so sort for a stable view. */
		std::sort(entries.begin(), entries.end(), [](const path& a, const path& b)
		{
			return a.generic_string() < b.generic_string();
		});

		return entries;
	}
}
