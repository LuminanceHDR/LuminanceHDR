#include <iostream>

#include <Libpfs/frame.h>
#include <Libpfs/io/jpegwriter.h>
#include <Libpfs/io/jpegreader.h>

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

        JpegReader reader(argv[1]);
        reader.read(myFrame, pfs::Params());

        JpegWriter writer(argv[2]);
        writer.write(myFrame,
                     pfs::Params( "min_luminance", 0.f )( "max_luminance", 255.f ));

        return 0;
    }
    catch (std::runtime_error& err) {
        cerr << err.what() << endl;
        return -1;
    }
}
