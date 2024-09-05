#include "DemoCommon.h"

#include "Doom/Base/i_main.h"
#include "Doom/doomdef.h"
#include "Endian.h"

#include <cstring>

// Helper/reminder for when one of these constants is changed.
// These numbers should normally be incremented at the same time for a public facing release of PsyDoom.
// A change in network game behavior normally implies a change in demo behavior, and new game settings to enable/disable the modified behavior.
static_assert(
    (NET_PROTOCOL_VERSION == 34) &&
    (DemoCommon::DEMO_FILE_VERSION == 16)
);

BEGIN_NAMESPACE(DemoCommon)

//--------------------------------------------------------------------------------------------------------------------------------------
// Saves the data from a full-fat 'TickInputs' data structure to 'DemoTickInputs'
//--------------------------------------------------------------------------------------------------------------------------------------
void DemoTickInputs::serializeFrom(const TickInputs& tickInputs) noexcept {
    analogForwardMove = tickInputs._analogForwardMove;
    analogSideMove = tickInputs._analogSideMove;
    analogTurn = tickInputs._analogTurn;
    directSwitchToWeapon = tickInputs.directSwitchToWeapon;
    flags1 = tickInputs._flags1.bits;
    flags2 = tickInputs._flags2.bits;
    flags3 = tickInputs._flags3.bits;
}

//--------------------------------------------------------------------------------------------------------------------------------------
// Saves the data in a 'DemoTickInputs' to the given 'TickInputs'.
// Fields not saved by 'DemoTickInputs' are zeroed.
//--------------------------------------------------------------------------------------------------------------------------------------
void DemoTickInputs::deserializeTo(TickInputs& tickInputs) const noexcept {
    tickInputs.reset();
    tickInputs._analogForwardMove = analogForwardMove;
    tickInputs._analogSideMove = analogSideMove;
    tickInputs._analogTurn = analogTurn;
    tickInputs.directSwitchToWeapon = directSwitchToWeapon;
    tickInputs._flags1.bits = flags1;
    tickInputs._flags2.bits = flags2;
    tickInputs._flags3.bits = flags3;
}

//--------------------------------------------------------------------------------------------------------------------------------------
// Reverses the byte order of this data structure
//--------------------------------------------------------------------------------------------------------------------------------------
void DemoTickInputs::byteSwap() noexcept {
    Endian::byteSwapInPlace(analogForwardMove);
    Endian::byteSwapInPlace(analogSideMove);
    Endian::byteSwapInPlace(analogTurn);
    Endian::byteSwapInPlace(directSwitchToWeapon);
    Endian::byteSwapInPlace(flags1);
    Endian::byteSwapInPlace(flags2);
    Endian::byteSwapInPlace(flags3);
}

//--------------------------------------------------------------------------------------------------------------------------------------
// Checks to see if the other tick inputs structure matches this one
//--------------------------------------------------------------------------------------------------------------------------------------
bool DemoTickInputs::equals(const DemoTickInputs& other) const noexcept {
    return (std::memcmp(this, &other, sizeof(*this)) == 0);
}

END_NAMESPACE(DemoCommon)
