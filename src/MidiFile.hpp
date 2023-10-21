//
// Created by WuXiangGuJun on 2023/10/19.
//

#ifndef MIDIPARSER_MIDIPARSER_HPP
#define MIDIPARSER_MIDIPARSER_HPP

#include <fstream>
#include <iostream>
#include <filesystem>
#include <cstring>


#include <vector>
#include <algorithm>


#if defined(__MINGW__)
#include <vector>
#include <algorithm>

#elif __linux__

#include <algorithm>

#endif


namespace MidiParser {

    class MidiEvent;

    class MidiFile;

    class HeaderChunk;

    class TrackChunk;

    enum class Format {
        SINGLE_TRACK = 0,
        MULTIPLE_TRACK = 1,
        MULTIPLE_SONG = 2
    };


    class HeaderChunk {

    public:
        Format format;
        uint16_t ntracks;
        uint16_t tickdiv;

        HeaderChunk(Format format, uint16_t ntracks, uint16_t tickdiv) : format(format), ntracks(ntracks),
                                                                         tickdiv(tickdiv) {
        }
    };


    class MidiFileParserException : public std::exception {
    private:
        std::string message_;
    public:
        explicit MidiFileParserException(std::string message) : message_(std::move(message)) {}

        [[nodiscard]] const char *what() const noexcept override {
            return message_.c_str();
        }
    };

    class TrackChunk {

    };


    namespace Conversion {

        constexpr uint16_t from_uint8(uint8_t first, uint8_t second) {
            return static_cast<uint16_t>(first << 8) | static_cast<uint16_t>(second);
        }

        constexpr uint32_t from_uint8(uint8_t first, uint8_t second, uint8_t third, uint8_t fourth) {
            uint32_t uint = 0;
            uint = (uint << 8) | static_cast<uint8_t>(first);
            uint = (uint << 8) | static_cast<uint8_t>(second);
            uint = (uint << 8) | static_cast<uint8_t>(third);
            uint = (uint << 8) | static_cast<uint8_t>(fourth);
            return uint;
        }


        /**
         * 改变变量value的字节顺序反转
         * @tparam T 
         * @param value 
         */
        template<class T>
        void changeEndian(T &value) {
            char *pointer = reinterpret_cast<char *>(&value);
            std::reverse(pointer, pointer + sizeof(value));
        }


        template<class T>
        T readAndConvert(std::ifstream &file) {
            T value;
            file.read(reinterpret_cast<char *>(&value), sizeof(value));
            changeEndian(value);
            if (!file.good()) {
                throw MidiFileParserException("Conversion error: invalid byte order");
            }
            return value;
        }

    }


    class MidiFile {

    private:

        static constexpr int HEADER_START_SIZE = 4;
        static constexpr int HEADER_LENGTH_SIZE = 4;

        static constexpr const uint8_t HEADER_START[] = {0x4D, 0x54, 0x68, 0x64}; // MThd
        static constexpr const uint8_t TRACK_START[] = {0x4D, 0x54, 0x72, 0x6B}; // MTrk

        // MIDI 事件列表
        std::vector<MidiEvent> events_;

        static bool handleException(const std::ifstream::failure &failure, std::ifstream &midi_file);

        HeaderChunk parseHeader(std::ifstream &midi_file);

        void parseTrack(std::ifstream &midi_file);


        HeaderChunk midiHeaderChunk = HeaderChunk(Format::SINGLE_TRACK, 0, 0);


    public:
        explicit MidiFile(const std::filesystem::path &filePath) {
            try {
                Parser(filePath);
            } catch (const MidiFileParserException &e) {
                std::cerr << e.what() << std::endl;
                exit(1);
            }
        };

        // 解析MIDI文件
        bool Parser(const std::filesystem::path &filePath);
        
        
        // 获取MIDI事件列表
        [[nodiscard]] const std::vector<MidiEvent> &event() const { return events_; };

        // 获取头部数据
        [[nodiscard]] const HeaderChunk &getHeaderChunk() const { return midiHeaderChunk; };

        // 获取MIDI格式
        [[nodiscard]] Format getFormat() const { return midiHeaderChunk.format; };

        // 获取MIDI事件数量
        [[nodiscard]] uint16_t getTrackCount() const { return midiHeaderChunk.ntracks; };

        // 获取MIDI时间基
        [[nodiscard]] uint16_t getTickDiv() const { return midiHeaderChunk.tickdiv; };

    };

    bool MidiFile::Parser(const std::filesystem::path &filePath) {
        // MIDI 文件对象
        std::ifstream midi_file;
        // failbit 状态表示流操作失败，例如文件不存在或无法打开。badbit 状态表示流操作出现错误，例如文件损坏或无法读取。
        midi_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try {
            // 检查文件是否存在
            // std::cout << "File path: " << filePath << std::endl;
            if (!std::filesystem::exists(filePath) || std::filesystem::file_size(filePath) == 0)
                throw MidiFileParserException("File does not exist");

            midi_file.open(filePath, std::ios::binary);
            midiHeaderChunk = parseHeader(midi_file);
            parseTrack(midi_file);

        } catch (const std::ifstream::failure &failure) {
            handleException(failure, midi_file);
        } catch (const MidiFileParserException &e) {
            std::cerr << e.what() << std::endl;
            return false; // 返回 false 表示解析失败
        }
        return true;
    }


    bool MidiFile::handleException(const std::ifstream::failure &failure, std::ifstream &midi_file) {
        if (midi_file.is_open())
            midi_file.close();
        char buffer[100];

#ifdef _WIN32
        strerror_s(buffer, sizeof(buffer), errno);
#elif __linux__
        strerror_r(errno, buffer, sizeof(buffer));
#endif

        throw MidiFileParserException(std::string("Could not parse MIDI with reason: ") + std::string(buffer));
    }


    HeaderChunk MidiFile::parseHeader(std::ifstream &midi_file) {
        char header_text[HEADER_START_SIZE];
        midi_file.read(header_text, HEADER_START_SIZE);
        if (!std::equal(std::begin(header_text), std::end(header_text), std::begin(header_text),
                        std::end(header_text))) {
            throw MidiFileParserException("File is not a MIDI file (No header chunk at start)");
        }

        char header_length_buffer[HEADER_LENGTH_SIZE];
        midi_file.read(header_length_buffer, HEADER_LENGTH_SIZE);
        uint32_t header_length = Conversion::from_uint8(
                header_length_buffer[0],
                header_length_buffer[1],
                header_length_buffer[2],
                header_length_buffer[3]);

        std::cout << "The header is " << header_length << " bytes long." << std::endl;

        uint16_t midi_format = Conversion::readAndConvert<uint16_t>(midi_file);
        std::cout << "The MIDI uses track format " << midi_format << std::endl;


        uint16_t midi_track_num = Conversion::readAndConvert<uint16_t>(midi_file);
        std::cout << "The number of tracks is " << midi_track_num << std::endl;


        uint16_t timing = Conversion::readAndConvert<uint16_t>(midi_file);
        if (timing >= 0x8000) {
            throw MidiFileParserException("This MIDI parser cannot currently parse SMPTE timed MIDI files.");
        }
        std::cout << "The timing is " << timing << std::endl;

        return {static_cast<Format>(midi_format), midi_track_num, timing};

    }

    void MidiFile::parseTrack(std::ifstream &midi_file) {

    }

    std::ostream &operator<<(std::ostream &os, const Format &format) {
        return os << static_cast<int>(format);
    }


    class MidiEvent {

    };


}

#endif //MIDIPARSER_MIDIPARSER_HPP

