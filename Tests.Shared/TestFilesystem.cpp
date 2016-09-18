#include <Shared/Shared.hpp>
#include <Shared/Enum.hpp>
#include <Tests/Tests.hpp>
#include <Shared/Files.hpp>

void CreateDummyFile(const String& filename)
{
	File file;
	TestEnsure(file.OpenWrite(filename, false));
	uint32 a = ~0;
	file.Write(&a, 4);
}

void CreateDummyFolderWithFiles(const String& folder)
{
	TestEnsure(Path::CreateDir(folder));
	CreateDummyFile(folder + Path::sep + "fileA");
	CreateDummyFile(folder + Path::sep + "fileB");
	CreateDummyFile(folder + Path::sep + "fileC");
}

Test("File.PathString")
{
	// Test path string functions
	String a = String() + "Test" + Path::sep + "Folder" + Path::sep + "FileName.ext";
	String b = String() + "Test" + Path::sep + "Folder";

	String filename;
	Path::RemoveLast(a, &filename);
	TestEnsure(filename == "FileName.ext");


	String rem = Path::RemoveBase(a, b);
	TestEnsure(rem == filename);
}
Test("File.Create")
{
	CreateDummyFile(TestFilename);
	TestEnsure(Path::FileExists(TestFilename));
}
Test("File.ScanFilesRecursive")
{
	String folder = Path::Absolute(TestBasePath + Path::sep + context.GetName() + "_TestFolder");
	TestEnsure(Path::CreateDir(folder));
	TestEnsure(Path::IsAbsolute(folder));

	CreateDummyFile(folder + Path::sep + "fileA");
	CreateDummyFile(folder + Path::sep + "fileB");
	CreateDummyFile(folder + Path::sep + "fileC");
	String folder1 = folder + Path::sep + "Folder";
	TestEnsure(Path::CreateDir(folder1));
	CreateDummyFile(folder1 + Path::sep + "fileD");

	Set<String> expectedPaths;
	expectedPaths.Add(folder + Path::sep + "fileA");
	expectedPaths.Add(folder + Path::sep + "fileB");
	expectedPaths.Add(folder + Path::sep + "fileC");
	expectedPaths.Add(folder1 + Path::sep + "fileD");

	Vector<FileInfo> files = Files::ScanFilesRecursive(folder);
	for(auto& file : files)
	{
		TestEnsure(expectedPaths.Contains(file.fullPath));
		expectedPaths.erase(file.fullPath);
	}
	TestEnsure(expectedPaths.empty());
}
Test("File.ScanFiles")
{
	String folder = Path::Absolute(TestBasePath + Path::sep + context.GetName() + "_TestFolder");
	TestEnsure(Path::CreateDir(folder));
	TestEnsure(Path::IsAbsolute(folder));

	CreateDummyFile(folder + Path::sep + "fileA");
	CreateDummyFile(folder + Path::sep + "fileB");
	CreateDummyFile(folder + Path::sep + "fileC");
	String folder1 = folder + Path::sep + "Folder";
	TestEnsure(Path::CreateDir(folder1));
	CreateDummyFile(folder1 + Path::sep + "fileD");

	Set<String> expectedPaths;
	expectedPaths.Add(folder + Path::sep + "fileA");
	expectedPaths.Add(folder + Path::sep + "fileB");
	expectedPaths.Add(folder + Path::sep + "fileC");
	expectedPaths.Add(folder + Path::sep + "Folder");

	Vector<FileInfo> files = Files::ScanFiles(folder);
	for(auto& file : files)
	{
		TestEnsure(expectedPaths.Contains(file.fullPath));
		expectedPaths.erase(file.fullPath);
	}
	TestEnsure(expectedPaths.empty());
}
Test("File.Dir")
{
	String folder = TestFilename;
	CreateDummyFolderWithFiles(folder);

	TestEnsure(Path::DeleteDir(folder));
	TestEnsure(!Path::FileExists(folder));
}
Test("File.RecursiveDir")
{
	String folder = TestFilename + Path::sep + "Sub1" + Path::sep + "Sub2";
	TestEnsure(Path::CreateDirRecursive(folder));
	// Test with trailing seperator
	String folder1 = TestFilename + Path::sep + "Sub1" + Path::sep + "Sub3" + Path::sep;
	TestEnsure(Path::CreateDirRecursive(folder1));
}
Test("File.Operations")
{
	CreateDummyFile(TestFilename);

	String newFilename = Path::RemoveLast(TestFilename) + Path::sep + "NewFileName";
	TestEnsure(Path::Rename(TestFilename, newFilename));
	TestEnsure(!Path::FileExists(TestFilename));

	TestEnsure(Path::Rename(newFilename, TestFilename));
	TestEnsure(Path::FileExists(TestFilename));
	TestEnsure(!Path::FileExists(newFilename));

	TestEnsure(Path::Delete(TestFilename));
	TestEnsure(!Path::FileExists(TestFilename));
}
Test("File.DirOperations")
{
	String folder = TestFilename;
	CreateDummyFolderWithFiles(folder);
	String subFolder = folder + Path::sep + "Sub";
	CreateDummyFolderWithFiles(subFolder);

	// Target
	String folder1 = TestFilename + "1";
	String sub1 = folder1 + Path::sep + "Sub";

	// Try to copy a folder with it's files
	TestEnsure(Path::CopyDir(folder, folder1));
	TestEnsure(Path::FileExists(folder1));
	TestEnsure(Path::FileExists(folder1 + Path::sep + "fileA"));
	TestEnsure(Path::FileExists(sub1 + Path::sep + "fileA"));
}
Test("File.ReadWrite")
{
	char data[] = "\r\n-- Test Data --\r\n@@\r\n";
	size_t dataLength = strlen(data);

	{
		File file;
		TestEnsure(file.OpenWrite(TestFilename, false));
		file.Write(data, dataLength);
	}

	{
		File file;
		TestEnsure(file.OpenRead(TestFilename));
		size_t len = file.GetSize();
		TestEnsure(len == dataLength);

		char* confirmData = new char[dataLength];
		file.Read(confirmData, dataLength);
		TestEnsure(memcmp(confirmData, data, dataLength) == 0);
		delete[] confirmData;

		// Check if reading more results in error
		TestEnsure(file.Read(data, 1) == 0);
	}
}
