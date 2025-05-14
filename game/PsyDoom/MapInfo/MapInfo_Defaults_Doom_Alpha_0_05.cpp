//------------------------------------------------------------------------------------------------------------------------------------------
// MapInfo defaults for 'Doom'
//------------------------------------------------------------------------------------------------------------------------------------------
#include "MapInfo_Defaults_Doom.h"

#include "Doom/Base/s_sound.h"
#include "Doom/Renderer/r_data.h"
#include "Doom/UI/cr_main.h"
#include "Doom/UI/ti_main.h"
#include "MapInfo.h"
#include "MapInfo_Defaults.h"
#include "PsyDoom/Game.h"
#include "PsyQ/LIBSPU.h"

BEGIN_NAMESPACE(MapInfo)

//------------------------------------------------------------------------------------------------------------------------------------------
// Initializes a 'GameInfo' struct for 'Doom Alpha v0.05'
//------------------------------------------------------------------------------------------------------------------------------------------
void initGameInfo_Doom_Alpha_0_05(GameInfo& gameInfo) noexcept {
    gameInfo.numMaps = 13;
    gameInfo.numRegularMaps = 13;
    gameInfo.bDisableMultiplayer = false;
    gameInfo.bFinalDoomGameRules = false;
    gameInfo.bAllowWideTitleScreenFire = false;
    gameInfo.bAllowWideOptionsBg = false;
    gameInfo.titleScreenStyle = TitleScreenStyle::Doom;
    gameInfo.creditsScreenStyle = CreditsScreenStyle::Doom;
    gameInfo.texPalette_titleScreenFire = FIRESKYPAL;
    gameInfo.texPalette_STATUS = UIPAL;
    gameInfo.texPalette_TITLE = MAINPAL;
    gameInfo.texPalette_BACK = MAINPAL;
    gameInfo.texLumpName_BACK = "BACK";
    gameInfo.texPalette_Inter_BACK = {};    // Default: use the same 'BACK' graphic as the main menu
    gameInfo.texLumpName_Inter_BACK = {};
    gameInfo.texPalette_LOADING = UIPAL;
    gameInfo.texPalette_PAUSE = MAINPAL;
    gameInfo.texPalette_NETERR = UIPAL;
    gameInfo.texPalette_DOOM = TITLEPAL;
    gameInfo.texPalette_CONNECT = MAINPAL;
    gameInfo.texPalette_OptionsBG = MAINPAL;
    gameInfo.texLumpName_OptionsBG = "MARB01";
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Adds all the default episodes for 'Doom Alpha v0.05' to the given list
//------------------------------------------------------------------------------------------------------------------------------------------
void addEpisodes_Doom_Alpha_0_05(std::vector<Episode>& episodes) noexcept {
    addEpisode(episodes, 1, 1,  "Level 1");
    addEpisode(episodes, 2, 2,  "Level 2");
    addEpisode(episodes, 3, 3,  "Level 3");
    addEpisode(episodes, 4, 4,  "Level 4");
    addEpisode(episodes, 5, 5,  "Level 5");
    addEpisode(episodes, 6, 6,  "Level 6");
    addEpisode(episodes, 7, 7,  "Level 7");
    addEpisode(episodes, 8, 8,  "Level 8");
    addEpisode(episodes, 9, 9,  "Level 9");
    addEpisode(episodes, 10, 25,  "Level 25");
    addEpisode(episodes, 11, 31,  "Level 31");
    addEpisode(episodes, 12, 32,  "Level 32");
    addEpisode(episodes, 13, 33,  "Level 33");
    
    // Corrections for the DOOM logo on the main menu
    for (Episode& episode : episodes) {
        episode.logoPal = MAINPAL;
        episode.logoX = 64;
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Adds all the default clusters for 'Doom Alpha v0.05' to the given list
//------------------------------------------------------------------------------------------------------------------------------------------
void addClusters_Doom_Alpha_0_05(std::vector<Cluster>& clusters) noexcept {
    Cluster& clus = clusters.emplace_back();
    clus.clusterNum = 1;
    clus.bSkipFinale = true;
    clus.bHideNextMapForFinale = true;
    clus.bEnableCast = false;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Adds the default maps for 'Doom Alpha v0.05' to the given list
//------------------------------------------------------------------------------------------------------------------------------------------
void addMaps_Doom_Alpha_0_05(std::vector<Map>& maps) noexcept {
    addMap(maps, 1 , 1, "Hangar",               2,    SPU_REV_MODE_SPACE,     0x1FFF);
    addMap(maps, 2 , 1, "Plant",                3,    SPU_REV_MODE_STUDIO_B,  0x27FF);
    addMap(maps, 3 , 1, "Toxin Refinery",       4,    SPU_REV_MODE_HALL,      0x27FF);
    addMap(maps, 4 , 1, "Command Control",      5,    SPU_REV_MODE_STUDIO_A,  0x23FF);
    addMap(maps, 5 , 1, "Phobos Lab",           6,    SPU_REV_MODE_HALL,      0x2FFF);
    addMap(maps, 6 , 1, "Central Processing",   7,    SPU_REV_MODE_STUDIO_C,  0x26FF);
    addMap(maps, 7 , 1, "Computer Station",     8,    SPU_REV_MODE_STUDIO_B,  0x2DFF);
    addMap(maps, 8 , 1, "Phobos Anomaly",       9,    SPU_REV_MODE_STUDIO_C,  0x2FFF);
    addMap(maps, 9 , 1, "Deimos Anomaly",       10,   SPU_REV_MODE_SPACE,     0x1FFF);
    addMap(maps, 25, 1, "Unknown Alpha Map",    0,    SPU_REV_MODE_OFF,       0x0000);
    addMap(maps, 31, 2, "Entryway",             0,    SPU_REV_MODE_OFF,       0x0000);
    addMap(maps, 32, 2, "Underhalls",           0,    SPU_REV_MODE_OFF,       0x0000);
    addMap(maps, 33, 2, "The Gantlet",          0,    SPU_REV_MODE_OFF,       0x0000);
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Adds the credit pages for 'Doom Alpha v0.05' to the given list
//------------------------------------------------------------------------------------------------------------------------------------------
void addCredits_Doom_Alpha_0_05([[maybe_unused]] std::vector<CreditsPage>& credits) noexcept {
    // No proper credits screen in this build...
    // There was a placeholder screen, but PsyDoom does not implement it.
}

END_NAMESPACE(MapInfo)
