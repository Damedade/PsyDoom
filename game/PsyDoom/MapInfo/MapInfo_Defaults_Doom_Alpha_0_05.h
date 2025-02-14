#pragma once

#include "Macros.h"

#include <vector>

BEGIN_NAMESPACE(MapInfo)

struct Cluster;
struct CreditsPage;
struct Episode;
struct GameInfo;
struct Map;

void initGameInfo_Doom_Alpha_0_05(GameInfo& gameInfo) noexcept;
void addEpisodes_Doom_Alpha_0_05(std::vector<Episode>& episodes) noexcept;
void addClusters_Doom_Alpha_0_05(std::vector<Cluster>& clusters) noexcept;
void addMaps_Doom_Alpha_0_05(std::vector<Map>& maps) noexcept;
void addCredits_Doom_Alpha_0_05(std::vector<CreditsPage>& credits) noexcept;

END_NAMESPACE(MapInfo)
