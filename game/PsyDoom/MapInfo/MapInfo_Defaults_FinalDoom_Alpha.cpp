//------------------------------------------------------------------------------------------------------------------------------------------
// MapInfo defaults for 'Final Doom Alpha'
//------------------------------------------------------------------------------------------------------------------------------------------
#include "MapInfo_Defaults_FinalDoom.h"

#include "MapInfo_Defaults_FinalDoom_Alpha.h"

BEGIN_NAMESPACE(MapInfo)

//------------------------------------------------------------------------------------------------------------------------------------------
// Initializes a 'GameInfo' struct for 'Final Doom Alpha'
//------------------------------------------------------------------------------------------------------------------------------------------
void initGameInfo_FinalDoom_Alpha(GameInfo& gameInfo) noexcept {
    initGameInfo_FinalDoom(gameInfo);
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Adds all the default episodes for 'Final Doom Alpha' to the given list
//------------------------------------------------------------------------------------------------------------------------------------------
void addEpisodes_FinalDoom_Alpha(std::vector<Episode>& episodes) noexcept {
    addEpisodes_FinalDoom(episodes);
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Adds all the default clusters for 'Final Doom Alpha' to the given list
//------------------------------------------------------------------------------------------------------------------------------------------
void addClusters_FinalDoom_Alpha(std::vector<Cluster>& clusters) noexcept {
    addClusters_FinalDoom(clusters);
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Adds the default maps for 'Final Doom Alpha' to the given list
//------------------------------------------------------------------------------------------------------------------------------------------
void addMaps_FinalDoom_Alpha(std::vector<Map>& maps) noexcept {
    addMaps_FinalDoom(maps);
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Adds the credit pages for 'Final Doom Alpha' to the given list
//------------------------------------------------------------------------------------------------------------------------------------------
void addCredits_FinalDoom_Alpha(std::vector<CreditsPage>& credits) noexcept {
    addCredits_FinalDoom(credits);
}

END_NAMESPACE(MapInfo)
