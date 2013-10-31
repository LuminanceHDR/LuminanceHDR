#include <iostream>

#include <Libpfs/frame.h>
#include <Libpfs/io/framewriterfactory.h>
#include <Libpfs/io/framereaderfactory.h>
#include <Libpfs/utils/msec_timer.h>

#include <boost/program_options.hpp>
#include <boost/algorithm/minmax_element.hpp>

using namespace std;
using namespace pfs;
using namespace pfs::io;

namespace po = boost::program_options;

void getParameters(int argc, char** argv, std::string& input, std::string& output, bool& autoscale)
{
    po::options_description desc("Allowed options: ");
    desc.add_options()
            ("output,o", po::value<std::string>(&output)->required(), "output file")
            ("input,i", po::value<std::string>(&input)->required(), "input file")
            ("autoscale,a", po::value<bool>(&autoscale)->default_value(false), "auto-scale")
            ;

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
              options(desc).allow_unregistered().run(), vm);
    po::notify(vm);
}

int main(int argc, char** argv)
{
    try
    {
        std::string input;
        std::string output;
        bool autoscale;

        getParameters(argc, argv, input, output, autoscale);

        Frame myFrame(0,0);

        msec_timer t;
        t.start();

        FrameReaderPtr reader = FrameReaderFactory::open(input);
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

        FrameWriterPtr writer = FrameWriterFactory::open(output);
        float minValue = 0.f;
        float maxValue = 1.f;
        if (autoscale)
        {
            minValue = std::min(*outRed.first, std::min(*outGreen.first,*outBlue.first));
            maxValue = std::max(*outRed.second, std::max(*outGreen.second,*outBlue.second));
        }
        writer->write(myFrame, pfs::Params
                      ("min_luminance", minValue)
                      ("max_luminance", maxValue)
                      );

        t.stop_and_update();
        std::cout << "Write time: " << t.get_time() << std::endl;

        return 0;
    }
    catch (std::exception& err)
    {
        cerr << err.what() << endl;
        return -1;
    }
}
