#include "stdafx.h"
#include "Board.h"
#include "Configuration.h"
#include "SnaFile.h"
#include "Z80File.h"

#include <iostream>
#include <sstream>

Board::Board(const ColourPalette& palette, const Configuration& configuration)
: m_configuration(configuration),
  m_palette(palette),
  m_cpu(*this, m_ports),
  m_ula(m_palette, *this),
  m_disassembler(*this),
  m_profiler(m_cpu, m_disassembler) {
}

void Board::initialise() {

	auto romDirectory = m_configuration.getRomDirectory();
	plug(romDirectory + "\\zx81.rom");	// ZX-81 Basic

	if (m_configuration.isProfileMode()) {
		CPU().ExecutingInstruction.connect([this](const EightBit::Z80&) {
			const auto pc = CPU().PC();
			m_profiler.addAddress(pc.word);
			m_profiler.addInstruction(peek(pc.word));
		});
	}

	if (m_configuration.isDebugMode()) {

		CPU().ExecutingInstruction.connect([this](const EightBit::Z80&) {
			const auto state = EightBit::Disassembler::state(CPU());
			const auto lineCounter = ((const Ula&)ULA()).LINECNTR();
			const auto allowNMI = ((const Ula&)ULA()).NMI();
			const auto disassembled = m_disassembler.disassemble(CPU());

			std::ostringstream output;
			output
				<< state
				<< " " << disassembled
				<< "    ; " << "NMI=" << allowNMI << ", LINECNTR=" << lineCounter;

			std::cerr << output.str();
		});


		CPU().ExecutedInstruction.connect([this](const EightBit::Z80& cpu) {
			std::cerr << ", CYCLES=" << cpu.cycles() << std::endl;
		});
	}

	ULA().Proceed.connect([this](const int& cycles) {
		runCycles(cycles);
	});
}

void Board::plug(const std::string& path) {
	ROM().load(path);
}

void Board::loadSna(const std::string& path) {
	SnaFile sna(path);
	sna.load(*this);
}

void Board::loadZ80(const std::string& path) {
	Z80File z80(path);
	z80.load(*this);
}

void Board::raisePOWER() {
	EightBit::Bus::raisePOWER();
	ULA().raisePOWER();
	CPU().raisePOWER();
	CPU().lowerRESET();
	CPU().raiseHALT();
	CPU().raiseINT();
	CPU().raiseNMI();
}

void Board::lowerPOWER() {
	ULA().lowerPOWER();
	CPU().lowerPOWER();
	EightBit::Bus::lowerPOWER();
}

EightBit::MemoryMapping Board::mapping(uint16_t address) {
	if (address < 0x2000)
		return { ROM(), 0x0000, 0xffff, EightBit::MemoryMapping::AccessLevel::ReadOnly };
	if (address < 0x4000)
		return { unused8K(), 0x2000, 0xffff, EightBit::MemoryMapping::AccessLevel::ReadOnly };
	if (address < 0x8000)
		return { RAM(), 0x4000, 0xffff, EightBit::MemoryMapping::AccessLevel::ReadWrite };
	return { unused32K(), 0x8000, 0xffff,  EightBit::MemoryMapping::AccessLevel::ReadOnly };
}

void Board::runFrame() {
	resetFrameCycles();
	for (int i = 0; i < Ula::TotalHeight; ++i)
		ULA().renderLine(i);
}

void Board::runCycles(int suggested) {
	m_allowed += suggested;
	const int taken = CPU().run(m_allowed);
	m_frameCycles += taken;
	m_allowed -= taken;
}
