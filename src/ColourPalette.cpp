#include "stdafx.h"
#include "ColourPalette.h"

#include <SDL.h>

void ColourPalette::load(SDL_PixelFormat* hardware) {
	loadColour(hardware, Black, 0x00, 0x00, 0x00);
	loadColour(hardware, White, 0xd7, 0xd7, 0xd7);
}

void ColourPalette::loadColour(SDL_PixelFormat* hardware, size_t idx, Uint8 red, Uint8 green, Uint8 blue) {
	m_colours[idx] = ::SDL_MapRGBA(
		hardware,
		red,
		green,
		blue,
		SDL_ALPHA_OPAQUE);
}
