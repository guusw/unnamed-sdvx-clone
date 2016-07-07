#pragma once

class MapDatabase : public Unique
{
public:
	MapDatabase();
	~MapDatabase();

	// Checks the background scanning and actualized the current map database
	void Update();

	bool IsSearching() const;
	void StartSearching();
	void StopSearching();

	void AddSearchPath(const String& path);
	void RemoveSearchPath(const String& path);

private:
	class MapDatabase_Impl* m_impl;
};