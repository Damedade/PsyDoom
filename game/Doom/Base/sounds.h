#pragma once

#include <cstdint>

//------------------------------------------------------------------------------------------------------------------------------------------
// Sequence identifiers for all sfx and original music tracks in the game
//------------------------------------------------------------------------------------------------------------------------------------------
enum sfxenum_t : int32_t {
    sfx_None,       // 0: No sound
    sfx_sgcock,     // 1: Weapon pickup sound
    sfx_punch,      // 2: Punch hit
    sfx_itmbk,      // 3: Deathmatch item respawn
    sfx_firsht2,    // 4: Demon/Baron/Cacodemon etc. fireball sound
    sfx_barexp,     // 5: Barrel/rocket explode
    sfx_firxpl,     // 6: Demon fireball hit
    sfx_pistol,     // 7: Pistol fire
    sfx_shotgn,     // 8: Shotgun fire
    sfx_plasma,     // 9: Plasma rifle fire
    sfx_bfg,        // 10: BFG start firing
    sfx_sawup,      // 11: Chainsaw being started up
    sfx_sawidl,     // 12: Chainsaw idle loop
    sfx_sawful,     // 13: Chainsaw saw
    sfx_sawhit,     // 14: Chainsaw hit
    sfx_rlaunc,     // 15: Rocket fire sound
    sfx_rxplod,     // 16: BFG explosion sound
    sfx_pstart,     // 17: Elevator start
    sfx_pstop,      // 18: Elevator/mover stop (also menu up/down sound)
    sfx_doropn,     // 19: Regular/slow door open
    sfx_dorcls,     // 20: Regular/slow door close
    sfx_stnmov,     // 21: Floor/crusher move sound
    sfx_swtchn,     // 22: Switch activate
    sfx_swtchx,     // 23: Exit switch activate
    sfx_itemup,     // 24: Bonus pickup
    sfx_wpnup,      // 25: Weapon pickup sound
    sfx_oof,        // 26: Ooof sound after falling hard, or when trying to use unusable wall
    sfx_telept,     // 27: Teleport sound
    sfx_noway,      // 28: Ooof sound after falling hard, or when trying to use unusable wall
    sfx_dshtgn,     // 29: Super shotgun fire
    sfx_dbopn,      // 30: SSG open barrel
    sfx_dbload,     // 31: SSG load shells
    sfx_dbcls,      // 32: SSG close barrel
    sfx_plpain,     // 33: Player pain sound
    sfx_pldeth,     // 34: Player death sound
    sfx_slop,       // 35: Gib/squelch sound
    sfx_posit1,     // 36: Former human sight: 1
    sfx_posit2,     // 37: Former human sight: 2
    sfx_posit3,     // 38: Former human sight: 3 (unused)
    sfx_podth1,     // 39: Former human death: 1
    sfx_podth2,     // 40: Former human death: 2
    sfx_podth3,     // 41: Former human death: 3 (unused)
    sfx_posact,     // 42: Former human idle
    sfx_popain,     // 43: Former human pain
    sfx_dmpain,     // 44: Demon pain
    sfx_dmact,      // 45: Demon idle/growl
    sfx_claw,       // 46: Imp/Baron etc. melee claw
    sfx_bgsit1,     // 47: Imp sight: 1
    sfx_bgsit2,     // 48: Imp sight: 2
    sfx_bgdth1,     // 49: Imp death: 1
    sfx_bgdth2,     // 50: Imp death: 2
    sfx_bgact,      // 51: Imp idle
    sfx_sgtsit,     // 52: Demon sight
    sfx_sgtatk,     // 53: Demon attack
    sfx_sgtdth,     // 54: Demon death
    sfx_brssit,     // 55: Baron sight
    sfx_brsdth,     // 56: Baron death
    sfx_cacsit,     // 57: Cacodemon sight
    sfx_cacdth,     // 58: Cacodemon death
    sfx_sklatk,     // 59: Lost Soul attack
    sfx_skldth,     // 60: (Unused) Intended for Lost Soul death?
    sfx_kntsit,     // 61: Knight sight
    sfx_kntdth,     // 62: Knight death
    sfx_pesit,      // 63: Pain Elemental sight
    sfx_pepain,     // 64: Pain Elemental pain
    sfx_pedth,      // 65: Pain Elemental death
    sfx_bspsit,     // 66: Arachnotron sight
    sfx_bspdth,     // 67: Arachnotron death
    sfx_bspact,     // 68: Arachnotron idle
    sfx_bspwlk,     // 69: Arachnotron hoof
    sfx_manatk,     // 70: Mancubus attack
    sfx_mansit,     // 71: Mancubus sight
    sfx_mnpain,     // 72: Mancubus pain
    sfx_mandth,     // 73: Mancubus death
    sfx_firsht,     // 74: Demon/Baron/Cacodemon etc. fireball sound
    sfx_skesit,     // 75: Revenant sight
    sfx_skedth,     // 76: Revenant death
    sfx_skeact,     // 77: Revenant idle
    sfx_skeatk,     // 78: Revenant missile fire
    sfx_skeswg,     // 79: Revenant throw punch
    sfx_skepch,     // 80: Revenant punch land
    sfx_cybsit,     // 81: Cyberdemon sight
    sfx_cybdth,     // 82: Cyberdemon death
    sfx_hoof,       // 83: Cyberdemon hoof up
    sfx_metal,      // 84: Cyberdemon thud down (metal)
    sfx_spisit,     // 85: Spider Mastermind sight
    sfx_spidth,     // 86: Spider Mastermind death
    sfx_bdopn,      // 87: Fast/blaze door open
    sfx_bdcls,      // 88: Fast/blaze door close
    sfx_getpow,     // 89: Powerup pickup
//------------------------------------------------------------------------------------------------------------------------------------------
// PsyDoom: sounds for PC Doom II actors which have been added back into the game.
// Also need to preserve sequence numbers for original game music - creating explicit entries for those tracks here.
//------------------------------------------------------------------------------------------------------------------------------------------
#if PSYDOOM_MODS
    music_01,       // 90: Retribution Dawns
    music_02,       // 91: The Broken Ones
    music_03,       // 92: Sanity's Edge
    music_04,       // 93: Hell's Churn
    music_05,       // 94: Digitized Pain
    music_06,       // 95: Corrupted Core
    music_07,       // 96: Mind Massacre
    music_08,       // 97: Mutation
    music_09,       // 98: A Calm Panic Rises
    music_10,       // 99: Corrupted
    music_11,       // 100: Breath Of Horror
    music_12,       // 101: Beyond Fear
    music_13,       // 102: Lamentation
    music_14,       // 103: Twisted Beyond Reason
    music_15,       // 104: The Slow Demonic Pulse
    music_16,       // 105: In The Grip Of Madness
    music_17,       // 106: Lurkers
    music_18,       // 107: Creeping Brutality
    music_19,       // 108: Steadfast Extermination
    music_20,       // 109: Hopeless Despair
    music_21,       // 110: Malignant
    music_22,       // 111: Tendrils Of Hate
    music_23,       // 112: Bells Of Agony
    music_24,       // 113: Infectious
    music_25,       // 114: Unhallowed
    music_26,       // 115: Breath Of Corruption
    music_27,       // 116: The Foulness Consumes
    music_28,       // 117: Demon Drone
    music_29,       // 118: Vexation
    music_30,       // 119: Larva Circuits
    //--------------------------------------------------------------------------------------------------------------------------------------
    // PsyDoom: reimplemented Arch-vile
    //--------------------------------------------------------------------------------------------------------------------------------------
    sfx_vilsit,     // 120: Arch-vile sight
    sfx_vipain,     // 121: Arch-vile pain
    sfx_vildth,     // 122: Arch-vile death
    sfx_vilact,     // 123: Arch-vile idle
    sfx_vilatk,     // 124: Arch-vile attack
    sfx_flamst,     // 125: Arch-vile flames (start)
    sfx_flame,      // 126: Arch-vile flames burn
    //--------------------------------------------------------------------------------------------------------------------------------------
    // PsyDoom: reimplemented Wolf SS
    //--------------------------------------------------------------------------------------------------------------------------------------
    sfx_sssit,      // 127: Wolfenstein-SS sight
    sfx_ssdth,      // 128: Wolfenstein-SS death
    //--------------------------------------------------------------------------------------------------------------------------------------
    // PsyDoom: reimplemented Commander Keen
    //--------------------------------------------------------------------------------------------------------------------------------------
    sfx_keenpn,     // 129: Commander Keen pain
    sfx_keendt,     // 130: Commander Keen death
    //--------------------------------------------------------------------------------------------------------------------------------------
    // PsyDoom: reimplemented Icon Of Sin
    //--------------------------------------------------------------------------------------------------------------------------------------
    sfx_bossit,     // 131: Icon of Sin sight
    sfx_bospit,     // 132: Icon of Sin cube spit
    sfx_bospn,      // 133: Icon of Sin pain
    sfx_bosdth,     // 134: Icon of Sin death
    sfx_boscub,     // 135: Icon of Sin spawn cube fly

    //--------------------------------------------------------------------------------------------------------------------------------------
    // PsyDoom: NOTE: new user music tracks could use sequence ids following the above ones...
    // So a new user track 'music_31' could start at sequence index '136' and so on.
    //--------------------------------------------------------------------------------------------------------------------------------------

    //--------------------------------------------------------------------------------------------------------------------------------------
    // PsyDoom: 'NUMSFX' is not sufficient to describe the range of sound effects anymore.
    // There are now two separate sequence id ranges for sound effects, describe them here.
    //--------------------------------------------------------------------------------------------------------------------------------------
    SFX_RANGE1_BEG = sfx_None,
    SFX_RANGE1_END = sfx_getpow + 1,
    SFX_RANGE2_BEG = sfx_vilsit,
    SFX_RANGE2_END = sfx_boscub + 1,
#else
    NUMSFX
#endif
};
