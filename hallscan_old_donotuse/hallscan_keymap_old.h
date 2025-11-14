
#ifndef HALLSCAN_KEYMAP_H
#define HALLSCAN_KEYMAP_H

#include "hallscan.h"
#include "quantum.h"


// Board-level sensor name definitions and per-mux wiring
//
// This header is the board-editable surface for remapping sensors. It contains
// two things you should edit to change which physical sensor corresponds to
// which logical key:
//
// 1) The 1-based sensor name enum (S_...): provide human-readable names for
//    physical sensors. These are 1-based identifiers (S_ESC = 1). Unused/empty
//    sensors should remain absent from the enum - instead use the literal 0
//    in the per-mux tables below to mark channels that are not connected.
//
// 2) The per-mux arrays (`muxX_channels`): each array is 16 entries (HC4067
//    channels). Put the appropriate S_<NAME> identifier for each channel, or
//    0 for unmapped channels. Keep one initializer per line for
//    readability.


// -------------------------------------
// ----------     MUX 1     ------------
// -------------------------------------
const mux16_ref_t mux1_channels[16] = {
	[0]  = { 0 },
	[1]  = { 0 },
	[2]  = { 0 },
	[3]  = { S_TAB },
	[4]  = { S_BSPC },
	[5]  = { 0 },
	[6]  = { S_U },
	[7]  = { S_I },
	[8]  = { 0 },
	[9]  = { 0 },
	[10] = { S_O },
	[11] = { S_P },
	[12] = { 0 },
	[13] = { 0 },
	[14] = { S_A },
	[15] = { S_S },
};

// -------------------------------------
// ----------     MUX 2     ------------
// -------------------------------------
const mux16_ref_t mux2_channels[16] = {
	[0]  = { 0 },
	[1]  = { 0 },
	[2]  = { 0 },
	[3]  = { 0 },
	[4]  = { 0 },
	[5]  = { S_R },
	[6]  = { S_E },
	[7]  = { S_ESC },
	[8]  = { S_Q },
	[9]  = { S_W },
	[10] = { 0 },
	[11] = { S_Y },
	[12] = { S_T },
	[13] = { 0 },
	[14] = { 0 },
	[15] = { 0 },
};

// -------------------------------------
// ----------     MUX 3     ------------
// -------------------------------------
const mux16_ref_t mux3_channels[16] = {
	[0]  = { 0 },
	[1]  = { 0 },
	[2]  = { 0 },
	[3]  = { S_D },
	[4]  = { S_R },
	[5]  = { 0 },
	[6]  = { S_F },
	[7]  = { S_G },
	[8]  = { 0 },
	[9]  = { 0 },
	[10] = { S_H },
	[11] = { S_J },
	[12] = { 0 },
	[13] = { 0 },
	[14] = { S_K },
	[15] = { S_L },
};

// -------------------------------------
// ----------     MUX 4     ------------
// -------------------------------------
const mux16_ref_t mux4_channels[16] = {
	[0]  = { 0 },
	[1]  = { 0 },
	[2]  = { 0 },
	[3]  = { 0 },
	[4]  = { 0 },
	[5]  = { S_SPC1 },
	[6]  = { S_M },
	[7]  = { S_B },
	[8]  = { S_N },
	[9]  = { S_C },
	[10] = { 0 },
	[11] = { S_V },
	[12] = { S_COMM },
	[13] = { 0 },
	[14] = { 0 },
	[15] = { 0 },
};

// -------------------------------------
// ----------     MUX 5     ------------
// -------------------------------------
// MUX5 disabled for now (external MCP3208)
/*
const mux16_ref_t mux5_channels[16] = {
	[0]  = { 0 },
	[1]  = { 0 },
	[2]  = { 0 },
	[3]  = { S_DOT },
	[4]  = { S_SCLN },
	[5]  = { 0 },
	[6]  = { S_DOWN },
	[7]  = { S_RGHT },
	[8]  = { 0 },
	[9]  = { 0 },
	[10] = { S_UP },
	[11] = { S_LEFT },
	[12] = { 0 },
	[13] = { 0 },
	[14] = { S_SPC2 },
	[15] = { S_LALT },
};
*/

#endif // HALLSCAN_KEYMAP_H