#include <stdio.h>
#include <stdlib.h>
#include <sstream>

#include "../../memory.h"
#include "../../loader.h"
#include "../../cpu.h"
#include "../../assembler.h"
#include "../../compiler.h"
#include "../../operators.h"
#include "../../keywords.h"
#include "../../optimiser.h"
#include "../../validater.h"
#include "../../linker.h"


#define GTBASIC_MAJOR_VERSION "0.1"
#define GTBASIC_MINOR_VERSION "2"
#define GTBASIC_VERSION_STR "gtbasic v" GTBASIC_MAJOR_VERSION "." GTBASIC_MINOR_VERSION


int main(int argc, char* argv[])
{
    if(argc != 2  &&  argc != 3)
    {
        fprintf(stderr, "%s\n", GTBASIC_VERSION_STR);
        fprintf(stderr, "Usage:   gtbasic <input filename> <optional include path>\n");
        return 1;
    }

    std::string filename = std::string(argv[1]);
    if(filename.find(".gbas") == filename.npos  &&  filename.find(".bas") == filename.npos)
    {
        fprintf(stderr, "Wrong file extension in %s : must be one of : '.gbas' or '.bas'\n", filename.c_str());
        return 1;
    }

    // Optional include path
    std::string includepath = (argc == 3) ? std::string(argv[2]) : ".";

    Expression::initialise();
    Assembler::initialise();
    Compiler::initialise();
    Operators::initialise();
    Keywords::initialise();
    Optimiser::initialise();
    Validater::initialise();
    Linker::initialise();

    uint16_t address = 0x0200;
    size_t nameSuffix = filename.find_last_of(".");
    std::string output = filename.substr(0, nameSuffix) + ".gasm";

    Assembler::setIncludePath(includepath);
    if(!Compiler::compile(filename, output)) return 1;
    if(!Assembler::assemble(output, address)) return 1;

    // Create gt1 format
    Loader::Gt1File gt1File;
    gt1File._loStart = LO_BYTE(address);
    gt1File._hiStart = HI_BYTE(address);
    Loader::Gt1Segment gt1Segment;
    gt1Segment._loAddress = LO_BYTE(address);
    gt1Segment._hiAddress = HI_BYTE(address);

    bool hasRomCode = false;
    Assembler::ByteCode byteCode;
    while(!Assembler::getNextAssembledByte(byteCode))
    {
        if(byteCode._isRomAddress) hasRomCode = true; 

        // Custom address
        if(byteCode._isCustomAddress)
        {
            if(gt1Segment._dataBytes.size())
            {
                // Previous segment
                gt1Segment._segmentSize = uint8_t(gt1Segment._dataBytes.size());
                gt1File._segments.push_back(gt1Segment);
                gt1Segment._dataBytes.clear();
            }

            address = byteCode._address;
            gt1Segment._isRomAddress = byteCode._isRomAddress;
            gt1Segment._loAddress = LO_BYTE(address);
            gt1Segment._hiAddress = HI_BYTE(address);
        }

        gt1Segment._dataBytes.push_back(byteCode._data);
    }

    // Last segment
    if(gt1Segment._dataBytes.size())
    {
        gt1Segment._segmentSize = uint8_t(gt1Segment._dataBytes.size());
        gt1File._segments.push_back(gt1Segment);
    }

    // Don't save gt1 file for any asm files that contain native rom code
    std::string gt1FileName;
    if(!hasRomCode  &&  !saveGt1File(filename, gt1File, gt1FileName)) return 1;

    Loader::printGt1Stats(gt1FileName, gt1File);

    return 0;
}
