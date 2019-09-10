#include "stdafx.h"
#include "Ula.h"

#include <Chip.h>
#include <cassert>

#include "Board.h"
#include "ColourPalette.h"

Ula::Ula(const ColourPalette& palette, Board& bus)
: m_palette(palette),
  m_bus(bus) {

	initialiseKeyboardMapping();

	BUS().ports().ReadingPort.connect(std::bind(&Ula::Board_ReadingPort, this, std::placeholders::_1));
	BUS().ports().WrittenPort.connect(std::bind(&Ula::Board_WrittenPort, this, std::placeholders::_1));

	RaisedPOWER.connect([this](EightBit::EventArgs) {
		m_frameCounter = 0;
		m_borderColour = 0;
		m_flash = false;
		LINECNTR() = 0;
	});

	Ticked.connect([this](EightBit::EventArgs) {
		const auto available = cycles() / 2;
		if (available > 0) {
			proceed(cycles() / 2);
			resetCycles();
		}
	});

	BUS().CPU().ExecutedInstruction.connect([this](EightBit::Z80& cpu) {
		if ((cpu.REFRESH() & Bit6) == 0)
			cpu.lowerINT();
	});
}

void Ula::flash() {
	m_flash = !m_flash;
}

void Ula::renderBlankLine(const int y) {
	renderHorizontalBorder(0, y, RasterWidth);
}

void Ula::renderLeftHorizontalBorder(const int y) {
	renderHorizontalBorder(0, y);
}

void Ula::renderRightHorizontalBorder(const int y) {
	renderHorizontalBorder(HorizontalRasterBorder + ActiveRasterWidth, y);
}

void Ula::renderHorizontalBorder(int x, int y, int width) {
	const size_t begin = y * RasterWidth + x;
	for (int i = 0; i < width; ++i) {
		m_pixels[begin + i] = m_borderColour;
		tick();
	}
}

void Ula::renderVRAM(const int y) {
}

void Ula::renderActiveLine(const int y) {
	renderLeftHorizontalBorder(y);
	renderVRAM(y - UpperRasterBorder);
	renderRightHorizontalBorder(y);
}

void Ula::renderLine(const int y) {

	if (y < VerticalRetraceLines) {
		tick(RasterWidth);
	} else if (y < (VerticalRetraceLines + UpperRasterBorder)) {
		renderBlankLine(y - VerticalRetraceLines);
	} else if (y < (VerticalRetraceLines + UpperRasterBorder + ActiveRasterHeight)) {
		renderActiveLine(y - VerticalRetraceLines);
	} else if (y < (VerticalRetraceLines + UpperRasterBorder + ActiveRasterHeight + LowerRasterBorder)) {
		renderBlankLine(y - VerticalRetraceLines);
	}

	if (NMI())
		BUS().CPU().lowerNMI();
	++LINECNTR();
	tick(HorizontalRetrace);
}

void Ula::pokeKey(SDL_Keycode raw) {
	m_keyboardRaw.emplace(raw);
}

void Ula::pullKey(SDL_Keycode raw) {
	m_keyboardRaw.erase(raw);
}

void Ula::initialiseKeyboardMapping() {

	// Left side
	m_keyboardMapping[0xF7] = { SDLK_1,		SDLK_2,     SDLK_3,     SDLK_4,		SDLK_5,		};
	m_keyboardMapping[0xFB] = { SDLK_q,		SDLK_w,		SDLK_e,		SDLK_r,		SDLK_t		};
	m_keyboardMapping[0xFD] = { SDLK_a,     SDLK_s,     SDLK_d,		SDLK_f,		SDLK_g		};
	m_keyboardMapping[0xFE] = { SDLK_LSHIFT,SDLK_z,		SDLK_x,		SDLK_c,		SDLK_v		};

	// Right side
	m_keyboardMapping[0xEF] = { SDLK_0,		SDLK_9,		SDLK_8,		SDLK_7,		SDLK_6		};
	m_keyboardMapping[0xDF] = { SDLK_p,     SDLK_o,     SDLK_i,		SDLK_u,		SDLK_y		};
	m_keyboardMapping[0xBF] = { SDLK_RETURN,SDLK_l,		SDLK_k,		SDLK_j,		SDLK_h		};
	m_keyboardMapping[0x7F] = { SDLK_SPACE,	SDLK_PERIOD,SDLK_m,		SDLK_n,		SDLK_b		};
}

uint8_t Ula::findSelectedKeys(uint8_t row) const {
	uint8_t returned = 0xff;
	auto pKeys = m_keyboardMapping.find(row);
	if (pKeys != m_keyboardMapping.cend()) {
		const auto& keys = pKeys->second;
		for (int column = 0; column < 5; ++column) {
			if (m_keyboardRaw.find(keys[column]) != m_keyboardRaw.cend())
				returned &= ~(1 << column);
		}
	}
	return returned;
}

void Ula::Board_ReadingPort(const uint8_t& port) {

	if ((BUS().ADDRESS().word & Bit0) != 0)
		return;

	const auto portHigh = BUS().ADDRESS().high;
	m_selected = findSelectedKeys(portHigh);

	const uint8_t value =
		m_selected		// Bit 0-4  Keyboard column bits	(0 = Pressed)
		| (1 << 4)		// Bit 5    Not used				(1)
		| (1 << 5)		// Bit 6    Display Refresh Rate	(0 = 60Hz, 1 = 50Hz)
		| (m_ear << 6);	// Bit 7    Cassette input			(0 = Normal, 1 = Pulse)

	BUS().ports().writeInputPort(port, value);

	if (!NMI()) {
		m_mic = 0;
		LINECNTR() = 0;
	}
}

void Ula::Board_WrittenPort(const uint8_t& port) {

	switch (port) {
	case 0xfd:
		NMI() = false;
		break;
	case 0xfe:
		NMI() = true;
		break;
	}

	m_mic = 1;
	LINECNTR() = 0;
}

void Ula::proceed(int cycles) {
	Proceed.fire(cycles);
}

EightBit::MemoryMapping Ula::mapping(uint16_t address) {

	const bool display = (address & Bit15) && lowered(BUS().CPU().M1());
	if (display)
		address &= ~0b1100'0000'0000'0000;

	if (address < 0x2000)
		return { BUS().ROM(), 0x0000, 0xffff, EightBit::MemoryMapping::AccessLevel::ReadOnly };
	if (address < 0x6000)
		return { BUS().RAM(), 0x2000, 0xffff, EightBit::MemoryMapping::AccessLevel::ReadWrite };
	return { BUS().unused(), 0x6000, 0xffff,  EightBit::MemoryMapping::AccessLevel::ReadOnly };
}
