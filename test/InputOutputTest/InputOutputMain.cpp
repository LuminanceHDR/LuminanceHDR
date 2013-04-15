#include <iostream>

#include <Libpfs/frame.h>
#include <Libpfs/io/framewriterfactory.h>
#include <Libpfs/io/framereaderfactory.h>

using namespace std;
using namespace pfs;
using namespace pfs::io;

int main(int argc, char** argv)
{
    if (argc < 3) {
        cerr << "Not enough arguments!" << endl;
        return -1;
    }

    try
    {
        Frame myFrame(0,0);

        FrameReaderPtr reader = FrameReaderFactory::open(argv[1]);
        reader->read(myFrame, pfs::Params());
        reader->close();

        FrameWriterPtr writer = FrameWriterFactory::open(argv[2]);
        writer->write(myFrame,
                      pfs::Params( "min_luminance", 0.f )( "max_luminance", 65535.f ));

        return 0;
    }
    catch (std::runtime_error& err) {
        cerr << err.what() << endl;
        return -1;
    }
}
