#pragma once

#include <cstdint>
#include <array>
#include <unordered_map>
#include <unordered_set>

#include <SDL.h>

#include <ClockedChip.h>
#include <Signal.h>
#include <MemoryMapping.h>

#include "ColourPalette.h"

class Board;

// Models a PAL (i.e. 50Hz) ZX81 ULA

class Ula final : public EightBit::ClockedChip {
private:
	static const int VerticalRetraceLines = 6;
	static const int UpperRasterBorder = 56;
	static const int ActiveRasterHeight = 192;
	static const int LowerRasterBorder = 56;

	// Units are pixels.  Cycles = pixels / 2
	static const int HorizontalRasterBorder = 64;
	static const int ActiveRasterWidth = 256;
	static const int HorizontalRetrace = 30;

public:
	static const int RasterWidth = HorizontalRasterBorder * 2 + ActiveRasterWidth;
	static const int RasterHeight = UpperRasterBorder + ActiveRasterHeight + LowerRasterBorder;
	static const int TotalHeight = VerticalRetraceLines + RasterHeight;

	static const int CyclesPerSecond = 3250000;	// 3.25Mhz
	static constexpr float FramesPerSecond = 50.0f;

	Ula(const ColourPalette& palette, Board& bus);

	EightBit::Signal<int> Proceed;

	int LINECNTR() const { return m_lineCounter; }
	bool NMI() const { return m_nmiEnabled; }

	void renderLine(int y);

	void pokeKey(SDL_Keycode raw);
	void pullKey(SDL_Keycode raw);

	void setBorder(uint8_t border) {
		m_border = border;
		m_borderColour = m_palette.getColour(m_border);
	}

	const auto& pixels() const { return m_pixels; }

private:
	std::array<uint32_t, RasterWidth * RasterHeight> m_pixels;
	const ColourPalette& m_palette;
	Board& m_bus;
	bool m_flash = false;
	uint8_t m_frameCounter = 0;
	uint32_t m_borderColour;
	bool m_nmiEnabled = false;
	int m_lineCounter = 0;

	bool m_displaying = false;
	uint8_t m_displayData = 0;
	bool m_inverted = false;
	uint8_t m_displayCharacter = 0;

	// Output port information
	uint8_t m_border : 3;		// Bits 0 - 2
	uint8_t m_mic : 1;			// Bit 3

	// Input port information
	uint8_t m_selected : 5;		// Bits 0 - 4 
	uint8_t m_ear : 1;			// Bit 6

	std::unordered_map<int, std::array<int, 5>> m_keyboardMapping;
	std::unordered_set<SDL_Keycode> m_keyboardRaw;

	Board& BUS() { return m_bus; }
	int& LINECNTR() { return m_lineCounter; }
	bool& NMI() { return m_nmiEnabled; }

	void initialiseKeyboardMapping();

	uint8_t findSelectedKeys(uint8_t row) const;

	void flash();

	void renderBlankLine(int y);
	void renderActiveLine(int y);

	void renderLeftHorizontalBorder(int y);
	void renderRightHorizontalBorder(int y);
	void renderHorizontalBorder(int x, int y, int width = HorizontalRasterBorder);

	void renderVRAM(int y);

	void Board_ReadingPort(const uint8_t& event);
	void Board_WrittenPort(const uint8_t& event);

	void proceed(int cycles);
};
