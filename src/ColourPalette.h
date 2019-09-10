#pragma once

#include <array>
#include <cstdint>

struct SDL_PixelFormat;

class ColourPalette final {
public:
	enum {
		Black = 0,
		White
	};

	ColourPalette() = default;

	uint32_t getColour(size_t index) const {
		return m_colours.at(index);
	}

	void load(SDL_PixelFormat* hardware);

private:
	std::array<uint32_t, 2> m_colours;

	void loadColour(SDL_PixelFormat* hardware, size_t idx, Uint8 red, Uint8 green, Uint8 blue);
};
