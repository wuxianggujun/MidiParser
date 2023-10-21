#include <iostream>
#include "MidiFile.hpp"

int main() {
    //MidiParser::MidiFile midiParser("Imalrite.mid");
    MidiParser::MidiFile midiParser("ashover8.mid");

    std::cout << "Midi file has " << midiParser.getTrackCount() << " tracks" << std::endl;
    std::cout << "Midi file has " << midiParser.getTickDiv() << " events" << std::endl;

    MidiParser::Format format = midiParser.getFormat();
    std::cout << "Midi file has " << format << " format" << std::endl;

    return 0;
}
