#pragma once

#include "Macros.h"

#include <vector>

BEGIN_NAMESPACE(MapInfo)

struct Cluster;
struct CreditsPage;
struct Episode;
struct GameInfo;
struct Map;

void initGameInfo_FinalDoom_Alpha(GameInfo& gameInfo) noexcept;
void addEpisodes_FinalDoom_Alpha(std::vector<Episode>& episodes) noexcept;
void addClusters_FinalDoom_Alpha(std::vector<Cluster>& clusters) noexcept;
void addMaps_FinalDoom_Alpha(std::vector<Map>& maps) noexcept;
void addCredits_FinalDoom_Alpha(std::vector<CreditsPage>& credits) noexcept;

END_NAMESPACE(MapInfo)
