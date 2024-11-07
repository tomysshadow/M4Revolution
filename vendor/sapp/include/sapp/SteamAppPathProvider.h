#pragma once

#include <fstream>
#include <filesystem>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <cstring>

#if defined( __GNUC__ ) && !defined( _WIN32 ) && !defined( POSIX )
#if __GNUC__ < 4
#error "SAPP requires GCC 4.X or greater."
#endif
#define POSIX 1
#endif

#ifdef _WIN32
#include <Windows.h>
#define CORRECT_PATH_SEPARATOR	 '\\'
#define INCORRECT_PATH_SEPARATOR '/'
#elif POSIX
#define CORRECT_PATH_SEPARATOR     '/'
#define INCORRECT_PATH_SEPARATOR '\\'
#endif

#ifdef _WIN32
#define CORRECT_PATH_SEPARATOR_S   "\\"
#define INCORRECT_PATH_SEPARATOR_S "/"
#elif POSIX
#define CORRECT_PATH_SEPARATOR_S   "/"
#define INCORRECT_PATH_SEPARATOR_S "\\"
#endif

#define SAPP_MAX_PATH 4096

typedef unsigned int AppId_t, uint32;

namespace fs = std::filesystem;

class SteamAppPathProvider;

class ISteamSearchProvider
{

protected:
    std::unordered_set<AppId_t> sourceGames;
    std::unordered_set<AppId_t> source2Games;
    bool precacheSourceGames = false;
    bool precacheSource2Games = false;

    static auto SappFileHelper(std::string_view path ) -> std::string
    {
        constexpr auto read_size = std::size_t( SAPP_MAX_PATH );
        auto stream = std::ifstream( path.data(), std::ios::binary );
        stream.exceptions( std::ios_base::badbit );

        auto out = std::string();
        auto buf = std::string( read_size, '\0' );
        while ( stream.read( &buf[0], read_size ) )
        {
            out.append( buf, 0, stream.gcount() );
        }
        out.append( buf, 0, stream.gcount() );
        return out;
    }


public:
    virtual ~ISteamSearchProvider() = default;

    class Game
    {

        friend SteamAppPathProvider; 

    public:

        ~Game() = default;

        Game( const Game &game )
        {
            gameName = game.gameName ;
            library = game.library ;
            installDir = game.installDir ;
            icon = game.icon ;
            appid = game.appid;
        }

        Game( Game &&game )
 noexcept         {
            gameName = std::move( game.gameName );
            library = std::move( game.library );
            installDir = std::move( game.installDir );
            icon = std::move( game.icon );
            appid = game.appid;
        }

        Game &operator=( const Game &game )
        {
            if ( &game == this )
                return *this;
            gameName = (game.gameName);
            library = game.library;
            installDir = game.installDir;
            icon = game.icon;
            appid = game.appid;
            return *this;
        }

        Game &operator=( Game &&game )
 noexcept         {
            if ( &game == this )
                return *this;

            gameName = std::move(game.gameName);
            library = std::move(game.library);
            installDir = std::move(game.installDir);
            icon = std::move(game.icon);
            appid = game.appid;
            return *this;
        }

        Game( std::string_view vGameName, std::string_view vLibrary, std::string_view vInstallDir, std::string_view vIcon, AppId_t vAppid )
        {
            gameName =  vGameName;
            library =  vLibrary;
            installDir =  vInstallDir;
            icon =  vIcon;
            appid = vAppid;
        }

        std::string gameName;
        std::string library;
        std::string installDir;
        std::string icon;
        AppId_t appid;
    };

    [[nodiscard]] virtual bool Available() const = 0;

    [[nodiscard]] virtual bool BIsAppInstalled( AppId_t appID ) const = 0;

    [[nodiscard]] virtual uint32 GetNumInstalledApps() const = 0;

    [[nodiscard]] virtual bool BIsSourceGame( AppId_t appID ) const = 0;

    [[nodiscard]] virtual bool BIsSource2Game( AppId_t appID ) const = 0;

    virtual uint32 GetInstalledApps(AppId_t *appids, uint32 unMaxAppIDs ) const = 0;

    virtual void sortGames(bool toGreater) = 0;

    [[nodiscard]] virtual AppId_t *GetInstalledAppsEX() const = 0;

    virtual bool GetAppInstallDir(AppId_t appID, std::string &pchFolder, int pFileSize = 0) const = 0;

    [[nodiscard]] virtual const Game & GetAppInstallDirEX(AppId_t appID ) const = 0;

};

class SteamAppPathProvider final : public ISteamSearchProvider
{
    bool basicIncludeCompare(std::string::const_iterator start, std::string_view comp)
    {
        return std::includes(start, start+comp.length(), comp.cbegin(), comp.cend());
    }
public:
    explicit SteamAppPathProvider(bool shouldPrecacheSourceGames = false, bool shouldPrecacheSource2Games = false)
    {
        precacheSourceGames = shouldPrecacheSourceGames;
        precacheSource2Games = shouldPrecacheSource2Games;
#ifdef _WIN32
        char steamLocationData[SAPP_MAX_PATH];

        HKEY steam;
        if ( RegOpenKeyExA( HKEY_LOCAL_MACHINE, R"(SOFTWARE\Valve\Steam)", 0, KEY_QUERY_VALUE | KEY_WOW64_32KEY, &steam ) != ERROR_SUCCESS )
            return;

        DWORD dwSize = sizeof(steamLocationData);
        if ( RegQueryValueExA( steam, "InstallPath", nullptr, nullptr, (LPBYTE)steamLocationData, &dwSize ) != ERROR_SUCCESS )
            return;

        RegCloseKey( steam );
        std::string steamLocation{ steamLocationData };

#else
        std::string steamLocation;
        steamLocation.reserve(SAPP_MAX_PATH);
        {
            std::string pHome = getenv( "HOME" );
#ifdef __APPLE__
            steamLocation.append(pHome + "/Library/Application Support/Steam");
#else
            steamLocation.append(pHome + "/.steam/steam");
#endif
        }

        std::error_code c;
        if ( !fs::exists( fs::path( steamLocation ), c ) )
        {
            steamLocation = "";
            fs::path d { "cwd/steamclient64.dll" };
            for ( const auto &e : fs::directory_iterator( "/proc/" ) )
            {
                if ( fs::exists( e / d, c ) )
                {
                    c.clear();
                    const auto s = fs::read_symlink( e.path() / "cwd", c );
                    if ( c )
                        continue;
                    steamLocation = s.string();
                    break;
                }
            }
            if ( steamLocation.empty() )
                return;
        }
#endif

        std::string librarycache = steamLocation + CORRECT_PATH_SEPARATOR_S "appcache" CORRECT_PATH_SEPARATOR_S "librarycache" CORRECT_PATH_SEPARATOR_S;

        steamLocation.append(CORRECT_PATH_SEPARATOR_S "steamapps" CORRECT_PATH_SEPARATOR_S "libraryfolders.vdf" );

        std::string file = SappFileHelper(steamLocation);

        for(std::string::const_iterator it = file.cbegin(); it != file.cend(); it++)
        {
            if(*it != 'p')
                continue;

            if(basicIncludeCompare(it-1, R"("path")"))
            {
                it+=5;
                while(*it != '"')
                    it++;

                auto currIt = it+1;
                it++;
                while(*it != '"')
                    it++;
                std::string pathString = std::string(currIt, it);
                pathString.append(CORRECT_PATH_SEPARATOR_S "steamapps");

                if ( !fs::exists( ( pathString ) ) )
                    continue;

                for ( auto const &dir_entry : fs::directory_iterator( ( pathString ), fs::directory_options::skip_permission_denied ) )
                {
                    auto strPath = dir_entry.path().string();

                    if ( !fs::exists( ( strPath ) ) )
                        continue;

                    auto indd = strPath.find( "appmanifest_" );
                    auto indd2 = strPath.rfind( ".acf" );
                    if ( ( indd <= strPath.length() ) && ( indd2 <= strPath.length() ) )
                    {

                        auto pathFile = SappFileHelper(std::string(strPath));

                        std::string keyName;
                        std::string keyInstallDir;
                        std::string appid;

                        for(std::string::const_iterator itr = pathFile.cbegin(); itr != pathFile.cend(); itr++)
                        {
                            if(*itr != 'a' && *itr != 'i' && *itr != 'n')
                                continue;
                            if(basicIncludeCompare(itr-1, R"("appid")"))
                            {
                                itr += 6;
                                while (*itr != '"')
                                    itr++;

                                auto currItr = itr + 1;
                                itr++;
                                while (*itr != '"')
                                    itr++;
                                appid = std::string{currItr, itr};
                            };
                            if(basicIncludeCompare(itr-1, R"("installdir")"))
                            {
                                itr += 11;
                                while (*itr != '"')
                                    itr++;

                                auto currItr = itr + 1;
                                itr++;
                                while (*itr != '"')
                                    itr++;
                                keyInstallDir = {currItr, itr};
                            };
                            if(basicIncludeCompare(itr-1, R"("name")"))
                            {
                                itr += 5;
                                while (*itr != '"')
                                    itr++;

                                auto currItr = itr + 1;
                                itr++;
                                while (*itr != '"')
                                    itr++;
                                keyName = {currItr, itr};
                            };

                        }

                        std::string icon( librarycache + appid + "_icon.jpg" );

                        std::string fullPath = ( pathString );
                        fullPath.append((CORRECT_PATH_SEPARATOR_S "common" CORRECT_PATH_SEPARATOR_S));
                        fullPath.append(keyInstallDir);

                        if ( (shouldPrecacheSource2Games || shouldPrecacheSourceGames) && std::filesystem::exists( fullPath ) ) {
                            auto dirIterator = std::filesystem::directory_iterator{fullPath,
                                                                                   std::filesystem::directory_options::skip_permission_denied};

                            for(const auto& dir_entry2 : dirIterator)
                            {
                                if (!dir_entry2.is_directory()) {
                                    continue;
                                }

                                if(shouldPrecacheSourceGames && std::filesystem::exists(dir_entry2.path() / "gameinfo.txt"))
                                {
                                    sourceGames.insert(stoi(appid));
                                    break;
                                }

                                if (shouldPrecacheSource2Games && std::filesystem::exists(dir_entry2.path() / "gameinfo.gi")) {
                                    source2Games.insert(stoi(appid));
                                    break;
                                }

                                if(!shouldPrecacheSource2Games)
                                    break;

                                for (auto const &subdir_entry: std::filesystem::directory_iterator{dir_entry2.path(),
                                                                                                   std::filesystem::directory_options::skip_permission_denied}) {
                                    if (subdir_entry.is_directory() && std::filesystem::exists(subdir_entry.path() / "gameinfo.gi")) {
                                        source2Games.insert(stoi(appid));
                                        break;
                                    }
                                }

                            }

                        }
                        games.emplace_back(keyName, pathString, keyInstallDir, icon, static_cast<AppId_t>( stoi( appid ) ) );
                    }
                }

            };
        }




//        KeyValueRoot libFolder = KeyValueRoot( file.c_str() );
//
//        if ( !libFolder.IsValid() )
//            return;
//
//        libFolder.Solidify();
//
//        auto &libKeyValue = libFolder["libraryfolders"];
//        for ( int i = 0; i < libKeyValue.ChildCount(); i++ )
//        {
//            KeyValue &folder = libKeyValue.At( i );
//            const char *name = folder.Key().string;
//            if ( !strcmp( name, "TimeNextStatsReport" ) || !strcmp( name, "ContentStatsID" ) )
//                continue;
//
//            auto pathValue = folder["path"].Value();
//            if ( !pathValue.string )
//                pathValue = folder.Value();
//            if ( !pathValue.string )
//                continue;
//
//            std::string pathString = std::string( pathValue.string );
//            pathString.append(CORRECT_PATH_SEPARATOR_S "steamapps");
//
//            if ( !fs::exists( ( pathString ) ) )
//                continue;
//
//            for ( auto const &dir_entry : fs::directory_iterator( ( pathString ), fs::directory_options::skip_permission_denied ) )
//            {
//                auto strPath = dir_entry.path().string();
//
//                if ( !fs::exists( ( strPath ) ) )
//                    continue;
//
//                auto indd = strPath.find( "appmanifest_" );
//                auto indd2 = strPath.rfind( ".acf" );
//                if ( ( indd <= strPath.length() ) && ( indd2 <= strPath.length() ) )
//                {
//
//                    auto pathFile = SappFileHelper(std::string(strPath));
//                    KeyValueRoot appManifest = KeyValueRoot( pathFile.c_str() );
//
//                    if ( !appManifest.IsValid() )
//                        return;
//
//                    KeyValue &appState = appManifest["AppState"];
//
//                    if ( appState["name"].Value().string == nullptr )
//                        continue;
//
//                    std::string keyName = (appState["name"].Value().string);
//                    std::string keyInstallDir = (appState["installdir"].Value().string);
//                    std::string appid = (appState["appid"].Value().string);
//
//                    std::string icon( librarycache + appid + "_icon.jpg" );
//
//                    std::string fullPath = ( pathString );
//                    fullPath.append((CORRECT_PATH_SEPARATOR_S "common" CORRECT_PATH_SEPARATOR_S));
//                    fullPath.append(keyInstallDir);
//
//                    if ( (shouldPrecacheSource2Games || shouldPrecacheSourceGames) && std::filesystem::exists( fullPath ) ) {
//                        auto dirIterator = std::filesystem::directory_iterator{fullPath,
//                                                                               std::filesystem::directory_options::skip_permission_denied};
//
//                        for(const auto& dir_entry2 : dirIterator)
//                        {
//                            if (!dir_entry2.is_directory()) {
//                                continue;
//                            }
//
//                            if(shouldPrecacheSourceGames && std::filesystem::exists(dir_entry2.path() / "gameinfo.txt"))
//                            {
//                                sourceGames.insert(stoi(appid));
//                                break;
//                            }
//
//                            if (shouldPrecacheSource2Games && std::filesystem::exists(dir_entry2.path() / "gameinfo.gi")) {
//                                source2Games.insert(stoi(appid));
//                                break;
//                            }
//
//                            if(!shouldPrecacheSource2Games)
//                                break;
//
//                            for (auto const &subdir_entry: std::filesystem::directory_iterator{dir_entry2.path(),
//                                                                                               std::filesystem::directory_options::skip_permission_denied}) {
//                                if (subdir_entry.is_directory() && std::filesystem::exists(subdir_entry.path() / "gameinfo.gi")) {
//                                    source2Games.insert(stoi(appid));
//                                    break;
//                                }
//                            }
//
//                        }
//
//                    }
//                    games.emplace_back(keyName, pathString, keyInstallDir, icon, static_cast<AppId_t>( stoi( appid ) ) );
//                }
//            }
//        }
    }

    [[nodiscard]] bool Available() const override
    {
        return !games.empty();
    }

    [[nodiscard]] bool BIsSourceGame( AppId_t appID ) const override
    {
        if(precacheSourceGames)
            return sourceGames.contains(appID);

        if ( !BIsAppInstalled( appID ) )
            return false;

        std::string dirPath{};
        dirPath.reserve(SAPP_MAX_PATH);
        GetAppInstallDir(appID, dirPath);

        if ( !std::filesystem::exists( dirPath ) )
        {
            return false;
        }

        auto dirIterator = std::filesystem::directory_iterator { std::string( dirPath ), std::filesystem::directory_options::skip_permission_denied };

        return std::any_of(begin(dirIterator), end(dirIterator), [&](auto dir_entry){
            return std::filesystem::exists( dir_entry.path() / "gameinfo.txt" );
        });

    }

    [[nodiscard]] bool BIsSource2Game( AppId_t appID ) const override
    {
        if(precacheSource2Games)
            return source2Games.contains(appID);

        if ( !BIsAppInstalled( appID ) )
            return false;

        std::string dirPath{};
        dirPath.reserve(SAPP_MAX_PATH);
        GetAppInstallDir(appID, dirPath);

        if ( !std::filesystem::exists( dirPath ) )
            return false;


        for ( auto const &dir_entry : std::filesystem::directory_iterator { std::string( dirPath ), std::filesystem::directory_options::skip_permission_denied } )
        {
            if ( !dir_entry.is_directory() )
            {
                continue;
            }
            if ( std::filesystem::exists( dir_entry.path() / "gameinfo.gi" ) )
                return true;


            for ( auto const &subdir_entry : std::filesystem::directory_iterator { dir_entry.path(), std::filesystem::directory_options::skip_permission_denied } )
            {
                if ( subdir_entry.is_directory() && std::filesystem::exists( subdir_entry.path() / "gameinfo.gi" ) )
                    return true;

            }
        }
        return false;
    }

    [[nodiscard]] bool BIsAppInstalled( AppId_t appID ) const override
    {
        const auto game = std::find_if( games.begin(), games.end(), [appID]( const Game &g )
        {
            return g.appid == appID;
        } );
        return game != games.end();
    }

    bool GetAppInstallDir(AppId_t appID, std::string &directory, int pFileSize = 0) const override
    {
        const auto game = std::find_if( games.begin(), games.end(), [appID]( const Game &g )
        {
            return g.appid == appID;
        } );
        if ( games.end() == game )
            return false;

        directory.append(game->library + CORRECT_PATH_SEPARATOR_S "common" CORRECT_PATH_SEPARATOR_S + game->installDir);

        return true;
    }

    [[nodiscard]] const Game & GetAppInstallDirEX(AppId_t appID ) const override
    {
        const Game& game = *std::find_if( games.cbegin(), games.cend(), [appID]( const Game &g )
        {
            return g.appid == appID;
        } );

        const Game& cend =  *games.cend();

        if ( &cend == &game )
            return *games.cend();


        return game;
    }

    [[nodiscard]] uint32 GetNumInstalledApps() const override
    {
        return games.size();
    }

    uint32 GetInstalledApps( AppId_t *pvecAppID, uint32 unMaxAppIDs ) const override
    {
        for ( int i = 0; i < unMaxAppIDs && i < games.size(); i++ )
            pvecAppID[i] = games.at( i ).appid;
        return unMaxAppIDs <= games.size() ? unMaxAppIDs : games.size();
    }

    void sortGames(bool toGreater = true) override
    {
        std::sort(games.begin(), games.end(), [toGreater](const Game &game1, const Game &game2){
            return toGreater ? game1.appid < game2.appid : game2.appid < game1.appid;
        });
    }

    [[nodiscard]] AppId_t *GetInstalledAppsEX() const override
    {
        auto appids = new AppId_t[GetNumInstalledApps()];
        for ( int i = 0; i < games.size(); i++ )
            appids[i] = games.at(i ).appid;
        return appids;
    }

private:
    std::vector<Game> games;
};
