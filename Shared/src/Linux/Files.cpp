#include "stdafx.h"
#include "Files.hpp"
#include "Path.hpp"
#include "Log.hpp"
#include "List.hpp"
#include "File.hpp"

#include <sys/types.h>
#include <dirent.h>

static Vector<FileInfo> _ScanFiles(const String& rootFolder, String extFilter, bool recurse)
{
	Vector<FileInfo> ret;
	if(!Path::IsDirectory(rootFolder))
	{
		Logf("Can't run ScanFiles, \"%s\" is not a folder", Logger::Warning, rootFolder);
		return ret;
	}

	// List of paths to process, subfolders are getting added to this list
	List<String> folderQueue;
	folderQueue.AddBack(rootFolder);

	bool filterByExtension = !extFilter.empty();
	extFilter.TrimFront('.'); // Remove possible leading dot

							  // Recursive folder search
	while(!folderQueue.empty())
	{
		String searchPath = folderQueue.front();
		folderQueue.pop_front();

		/// DEBUG
		Logf("FSD> %s", Logger::Info, *searchPath);
		
		DIR* dir = opendir(*searchPath);
		if(dir == nullptr)
			continue;

		// Open first entry
		dirent* ent = readdir(dir);
		if(end)
		{
			// Keep scanning files in this folder

			String currentfolder;
			do
			{
				FileInfo info;
				info.fullPath = Path::Normalize(ent->d_name);
				info.lastWriteTime = File::GetLastWriteTime(info.fullPath); // linux doesn't provide this timestamp in the directory entry
				info.type = FileType::Regular;

				Logf("FS> %s", Logger::Info, *info.fullPath);

				if(ent->d_type == DT_DIR)
				{
					if(recurse)
					{
						// Visit sub-folder
						folderQueue.AddBack(info.fullPath);
					}
				}
				else
				{
					// Check file
					if(filterByExtension)
					{
						String ext = Path::GetExtension(info.fullPath);
						if(ext == extFilter)
						{
							ret.Add(info);
						}
					}
					else
					{
						ret.Add(info);
					}
				}
			} while(ent = readdir(dir));
		}

		closedir(dir);
	}

	return move(ret);
}

Vector<FileInfo> Files::ScanFiles(const String& folder, String extFilter /*= String()*/)
{
	return _ScanFiles(folder, extFilter, false);
}
Vector<FileInfo> Files::ScanFilesRecursive(const String& folder, String extFilter /*= String()*/)
{
	return _ScanFiles(folder, extFilter, true);
}