#include "stdafx.h"
#include "MapDatabase.hpp"
#include "Database.hpp"
#include "Beatmap.hpp"
#include "Profiling.hpp"
#include <thread>
#include <mutex>
#include <chrono>
using std::thread;
using std::mutex;
using namespace std;


class MapDatabase_Impl
{
public:
	thread m_thread;
	bool m_searching = false;
	Set<String> m_searchPaths;
	Database m_database;

	struct MapIndex
	{
		int32 id;
		// Full path to the map
		String path;
		// Last time the map changed
		uint64 lwt;
		// Beatmap MetaData
		BeatmapSettings settings;
	};
	Map<int32, MapIndex*> m_maps;

	struct SearchState
	{
		struct ExistingMap
		{
			int32 id;
			uint64 lwt;
		};
		Map<String, ExistingMap> maps;
	} m_searchState;

	struct Event
	{
		enum Action
		{
			Added,
			Removed,
			Updated
		};
		Action action;
		String path;
		// Current lwt of file
		uint64 lwt;
		// Id of the map
		int32 id;
		// Scanned map data, for added/updated maps
		BeatmapSettings* mapData = nullptr;
	};
	List<Event> m_pendingChanges;
	mutex m_pendingChangesLock;

	static const int32 m_version = 1;

public:
	MapDatabase_Impl()
	{
		String databasePath = "maps.db";
		if(!m_database.Open(databasePath))
		{
			Logf("Failed to open database [%s]", Logger::Warning, databasePath);
			assert(false);
		}

		bool rebuild = false;
		DBStatement versionQuery = m_database.Query("SELECT version FROM `Database`");
		if(versionQuery && versionQuery.Step())
		{
			int32 gotVersion = versionQuery.IntColumn(0);
			if(gotVersion != m_version)
			{
				rebuild = true;
			}
		}
		else
		{
			// Create DB 
			m_database.Exec("DROP TABLE IF EXISTS Database");
			m_database.Exec("CREATE TABLE Database(version INTEGER)");
			m_database.Exec(Utility::Sprintf("INSERT OR REPLACE INTO Database(rowid, version) VALUES(1, %d)", m_version));
			rebuild = true;
		}

		if(rebuild)
		{
			m_CreateTables();

			// Update database version
			m_database.Exec(Utility::Sprintf("UPDATE Database SET `version`=%d WHERE `rowid`=1", m_version));
		}
		else
		{
			// Load initial folder tree
			m_LoadInitialData();
		}
	}
	~MapDatabase_Impl()
	{
		StopSearching();
		m_CleanupMapIndex();
	}

	void StartSearching()
	{
		if(m_searching)
			return;

		// Create initial data set to compare to when evaluating if a file is added/removed/updated
		m_LoadInitialData();
		m_searching = true;
		m_thread = thread(&MapDatabase_Impl::m_SearchThread, this);
	}
	void StopSearching()
	{
		m_searching = false;
		if(m_thread.joinable())
		{
			m_thread.join();
		}
	}
	void AddSearchPath(const String& path)
	{
		String normalizedPath = Path::Normalize(path);
		if(m_searchPaths.Contains(normalizedPath))
			return;

		m_searchPaths.Add(path);
	}
	void RemoveSearchPath(const String& path)
	{
		String normalizedPath = Path::Normalize(path);
		if(!m_searchPaths.Contains(normalizedPath))
			return;

		m_searchPaths.erase(path);
	}

	/* Thread safe event queue functions */
	// Add a new change to the change queue
	void AddChange(Event change)
	{
		m_pendingChangesLock.lock();
		m_pendingChanges.emplace_back(change);
		m_pendingChangesLock.unlock();
	}
	// Removes changes from the queue and returns them
	List<Event> FlushChanges()
	{
		m_pendingChangesLock.lock();
		List<Event> changes = std::move(m_pendingChanges);
		m_pendingChangesLock.unlock();
		return std::move(changes);
	}
	
	// Processes pending database changes
	void Update()
	{
		List<Event> changes = FlushChanges();
		if(changes.empty())
			return;

		DBStatement add = m_database.Query("INSERT INTO Maps(path,lwt,artist,title,tags,metadata) VALUES(?,?,?,?,?,?)");
		DBStatement update = m_database.Query("UPDATE Maps SET lwt=?,artist=?,title=?,tags=?,metadata=? WHERE mapId=?");
		DBStatement remove = m_database.Query("DELETE FROM Maps WHERE mapId=?");

		m_database.Exec("BEGIN");
		for(Event& e : changes)
		{
			if(e.action == Event::Added)
			{
				Buffer metadata;
				MemoryWriter metadataWriter(metadata);
				metadataWriter.SerializeObject(*e.mapData);

				add.BindString(1, e.path);
				add.BindInt64(2, e.lwt);
				add.BindString(3, e.mapData->artist);
				add.BindString(4, e.mapData->title);
				add.BindString(5, e.mapData->tags);
				add.BindBlob(6, metadata);
				add.Step();
				add.Rewind();
			}
			else if(e.action == Event::Updated)
			{
				Buffer metadata;
				MemoryWriter metadataWriter(metadata);
				metadataWriter.SerializeObject(*e.mapData);

				update.BindInt64(1, e.lwt);
				update.BindString(2, e.mapData->artist);
				update.BindString(3, e.mapData->title);
				update.BindString(4, e.mapData->tags);
				update.BindBlob(5, metadata);
				update.BindInt(6, e.id);
				update.Step();
				update.Rewind();
			}
			else if(e.action == Event::Removed)
			{
				remove.BindInt(1, e.id);
				remove.Step();
				remove.Rewind();
			}
			if(e.mapData)
				delete e.mapData;
		}
		m_database.Exec("END");
	}

private:
	void m_CleanupMapIndex()
	{
		for(auto m : m_maps)
		{
			delete m.second;
		}
		m_maps.clear();
	}
	void m_CreateTables()
	{
		m_database.Exec("DROP TABLE IF EXISTS Maps");

		m_database.Exec("CREATE TABLE Maps"
			"(mapId INTEGER PRIMARY KEY AUTOINCREMENT, artist TEXT, title TEXT, tags TEXT, metadata BLOB,"
			" path TEXT, lwt INTEGER)");
	}
	void m_LoadInitialData()
	{
		assert(!m_searching);

		// Clear search state
		m_searchState.maps.clear();

		// Scan original maps
		m_CleanupMapIndex();

		DBStatement mapScan = m_database.Query("SELECT mapId,path,lwt,metadata FROM Maps");
		while(mapScan.StepRow())
		{
			MapIndex* map = new MapIndex();
			map->id = mapScan.IntColumn(0);
			map->path = mapScan.StringColumn(1);
			map->lwt = mapScan.Int64Column(2);
			Buffer metadata = mapScan.BlobColumn(3);
			MemoryReader metadataReader(metadata);
			metadataReader.SerializeObject(map->settings);

			m_maps.Add(map->id, map);

			SearchState::ExistingMap existing;
			existing.lwt = map->lwt;
			existing.id = map->id;
			m_searchState.maps.Add(map->path, existing);
		}
	}

	// Main search thread
	void m_SearchThread()
	{
		this_thread::sleep_for(chrono::milliseconds(10));

		Map<String, WIN32_FIND_DATA> fileList;

		{
			ProfilerScope $("Map Database - Enumerate Files and Folders");
			for(String rootSearchPath : m_searchPaths)
			{
				if(!m_searching)
					break;

				// List of paths to process, subfolders are getting added to this list
				List<String> folderQueue;
				folderQueue.AddBack(rootSearchPath);

				// Recursive folder search
				while(!folderQueue.empty() && m_searching)
				{
					String searchPath = folderQueue.front();
					folderQueue.pop_front();

					WString searchPathW = Utility::ConvertToUnicode(searchPath) + L"\\*";
					WIN32_FIND_DATA findDataW;
					HANDLE searchHandle = FindFirstFile(*searchPathW, &findDataW);
					if(searchHandle == INVALID_HANDLE_VALUE)
						continue;

					String currentfolder;
					do
					{
						String filename = Utility::ConvertToUTF8(findDataW.cFileName);
						String fullPath = Path::Normalize(searchPath + "/" + filename);
						if(filename == ".")
							continue;
						if(filename == "..")
							continue;

						if(findDataW.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
						{
							// Visit sub-folder
							folderQueue.AddBack(fullPath);
						}
						else
						{
							// Check file
							String ext = Path::GetExtension(filename);
							if(ext == "ksh")
							{
								fileList.Add(fullPath, findDataW);
							}
						}

					} while(FindNextFile(searchHandle, &findDataW) && m_searching);

					FindClose(searchHandle);
				}
			}
		}

		{
			ProfilerScope $("Map Database - Process Removed Files");

			// Process scanned files
			for(auto f : m_searchState.maps)
			{
				if(!fileList.Contains(f.first))
				{
					Event evt;
					evt.action = Event::Removed;
					evt.path = f.first;
					AddChange(evt);
				}
			}
		}

		{
			ProfilerScope $("Map Database - Process New Files");

			// Process scanned files
			for(auto f : fileList)
			{
				if(!m_searching)
					break;

				Event evt;
				SearchState::ExistingMap* existing = m_searchState.maps.Find(f.first);
				if(existing)
				{
					evt.id = existing->id;
					uint64 mylwt = ((uint64)f.second.ftLastWriteTime.dwHighDateTime << 32) | (uint64)f.second.ftLastWriteTime.dwLowDateTime;
					evt.lwt = mylwt;

					if(existing->lwt != mylwt)
					{
						// Map Updated
						evt.action = Event::Updated;
					}
					else
					{
						// Skip, not changed
						continue;
					}
				}
				else
				{
					// Map added
					evt.action = Event::Added;
				}

				Logf("Discovered Map [%s]", Logger::Info, f.first);

				// Try to read map metadata
				bool mapValid = false;
				File fileStream;
				Beatmap map;
				if(fileStream.OpenRead(f.first))
				{
					FileReader reader(fileStream);

					if(map.Load(reader, true))
					{
						mapValid = true;
					}
				}

				if(mapValid)
				{
					evt.mapData = new BeatmapSettings(map.GetMapSettings());
				}
				else
				{
					if(!existing) // Never added
					{
						Logf("Skipping corrupted map [%s]", Logger::Warning, f.first);
						continue;
					}
					// Invalid maps get removed from the database
					evt.action = Event::Removed;
				}
				evt.path = f.first;
				AddChange(evt);
				continue;
			}
		}

		m_searching = false;
	}
};

MapDatabase::MapDatabase()
{
	m_impl = new MapDatabase_Impl();
}
MapDatabase::~MapDatabase()
{
	delete m_impl;
}

void MapDatabase::Update()
{
	m_impl->Update();
}

bool MapDatabase::IsSearching() const
{
	return m_impl->m_searching;
}
void MapDatabase::StartSearching()
{
	m_impl->StartSearching();
}
void MapDatabase::StopSearching()
{
	m_impl->StopSearching();
}

void MapDatabase::AddSearchPath(const String& path)
{
	m_impl->AddSearchPath(path);
}
void MapDatabase::RemoveSearchPath(const String& path)
{
	m_impl->RemoveSearchPath(path);
}
