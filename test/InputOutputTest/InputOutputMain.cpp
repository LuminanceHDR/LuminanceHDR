#include <iostream>

#include <Libpfs/frame.h>
#include <Libpfs/io/framewriterfactory.h>
#include <Libpfs/io/framereaderfactory.h>
#include <Libpfs/utils/msec_timer.h>

#include <boost/algorithm/minmax_element.hpp>

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

        msec_timer t;
        t.start();

        FrameReaderPtr reader = FrameReaderFactory::open(argv[1]);
        reader->read(myFrame, pfs::Params());
        reader->close();

        t.stop_and_update();
        std::cout << "Read time: " << t.get_time() << std::endl;

        Channel* red;
        Channel* green;
        Channel* blue;

        myFrame.getXYZChannels(red, green, blue);
        pair<Channel::iterator, Channel::iterator> outRed =
                boost::minmax_element(red->begin(), red->end());
        cout << "Red: (" << *outRed.first << ", " << *outRed.second << ")" << endl;
        pair<Channel::iterator, Channel::iterator> outGreen =
                boost::minmax_element(green->begin(), green->end());
        cout << "Green: (" << *outGreen.first << ", " << *outGreen.second << ")" << endl;
        pair<Channel::iterator, Channel::iterator> outBlue =
                boost::minmax_element(blue->begin(), blue->end());
        cout << "Blue: (" << *outBlue.first << ", " << *outBlue.second << ")" << endl;

        t.reset();
        t.start();

        FrameWriterPtr writer = FrameWriterFactory::open(argv[2]);
        writer->write(myFrame,
                      pfs::Params( "min_luminance", 0.f )( "max_luminance", 1.f ));

        t.stop_and_update();
        std::cout << "Write time: " << t.get_time() << std::endl;

        return 0;
    }
    catch (std::runtime_error& err) {
        cerr << err.what() << endl;
        return -1;
    }
}
