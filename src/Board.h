#pragma once

#include <cstdint>
#include <string>

#include <Bus.h>
#include <InputOutput.h>
#include <Ram.h>
#include <UnusedMemory.h>

#include <Z80.h>
#include <Profiler.h>
#include <Disassembler.h>

#include "Ula.h"

class Configuration;
class ColourPalette;

// Models a PAL (i.e. 50Hz) Sinclair ZX81 with 16K expansion

class Board final : public EightBit::Bus {
public:
	Board(const ColourPalette& palette, const Configuration& configuration);

	EightBit::Z80& CPU() { return m_cpu; }
	Ula& ULA() { return m_ula; }
	EightBit::InputOutput &ports() { return m_ports; }
	EightBit::Rom& ROM() { return m_basicRom; }
	EightBit::Ram& RAM() { return m_externalRam; }
	EightBit::UnusedMemory& unused8K() { return m_unused8K; }
	EightBit::UnusedMemory& unused32K() { return m_unused32K; }

	void plug(const std::string& path);
	void loadSna(const std::string& path);
	void loadZ80(const std::string& path);

	virtual void initialise() final;
	virtual void raisePOWER() final;
	virtual void lowerPOWER() final;

	void runFrame();
	int frameCycles() const { return m_frameCycles; }

protected:
	virtual EightBit::MemoryMapping mapping(uint16_t address) final;

private:
	const Configuration& m_configuration;
	const ColourPalette& m_palette;
	EightBit::InputOutput m_ports;
	EightBit::Z80 m_cpu;
	Ula m_ula;

	EightBit::Rom m_basicRom;								//0000h - 1FFFh  BIOS ROM (8KBytes)
	EightBit::UnusedMemory m_unused8K = { 0x2000, 0xff };	//2000h - 3FFFh  not used
	EightBit::Ram m_externalRam = 0x4000;					//4000h - 7FFFh  External RAM (16 KBytes)
	EightBit::UnusedMemory m_unused32K = { 0x8000, 0xff };	//8000h - FFFFh  not used, except during video decoding

	EightBit::Disassembler m_disassembler;
	EightBit::Profiler m_profiler;

	int m_frameCycles = 0;
	int m_allowed = 0;	// To track "overdrawn" cycle expendature

	void resetFrameCycles() { m_frameCycles = 0; }
	void runCycles(int suggested);
};
