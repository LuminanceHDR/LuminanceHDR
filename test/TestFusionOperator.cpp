#include <QString>

#include <iostream>
#include <vector>
#include <boost/program_options.hpp>
#include <boost/algorithm/minmax_element.hpp>

#include <Libpfs/utils/msec_timer.h>
#include <Libpfs/io/framewriterfactory.h>
#include <Libpfs/io/framereaderfactory.h>

#include <HdrCreation/fusionoperator.h>
#include <Exif/ExifOperations.h>

#include <Libpfs/colorspace/rgbremapper_fwd.h>

using namespace std;
using namespace pfs;
using namespace pfs::io;

using namespace libhdr;
using namespace libhdr::fusion;

namespace po = boost::program_options;

libhdr::fusion::FrameEnhanced loadFile(const std::string& filename)
{
    FrameReaderPtr reader = FrameReaderFactory::open(filename);

    FramePtr image(new Frame);
    reader->read(*image, pfs::Params());

    float averageLuminace = ExifOperations::getAverageLuminance(filename);

    std::cout << filename << " avg luminance = " << averageLuminace << std::endl;

    return libhdr::fusion::FrameEnhanced(image, averageLuminace);
}


int main(int argc, char** argv)
{
#ifdef LHDR_CXX11_ENABLED
    string          outputFile;
    vector<string>  inputFiles;

    // program options
    po::options_description desc("Allowed options: ");
    desc.add_options()
            ("response-type,r", po::value<std::string>(), "response type")
            ("output-file,o", po::value<std::string>(&outputFile)->required(), "output file")
            ("input-files,i", po::value< vector<string> >(&inputFiles)->required(), "input files (JPEG, TIFF or RAW)")
            ;

    try
    {
        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).
                  options(desc).allow_unregistered().run(), vm);
        po::notify(vm);

        std::vector<libhdr::fusion::FrameEnhanced> images;
        foreach(const string& filename, inputFiles)
        {
            images.push_back(loadFile(filename));
        }

        cout << "Are you ready?\n";
        char c;
        cin >> c;

        msec_timer t;
        t.start();

        FusionOperatorPtr fusionOperator = IFusionOperator::build(ROBERTSON_AUTO);

        fusionOperator->writeResponsesToFile("responses_before.m");

        pfs::FramePtr newHdr(fusionOperator->computeFusion(images));
        if ( newHdr == NULL )
        {
            return -1;
        }

        t.stop_and_update();
        std::cout << "Fusion elapsed time: " << t.get_time() << std::endl;

        Channel* red;
        Channel* green;
        Channel* blue;

        newHdr->getXYZChannels(red, green, blue);
        pair<Channel::iterator, Channel::iterator> outRed =
                boost::minmax_element(red->begin(), red->end());
        cout << "Red: (" << *outRed.first << ", " << *outRed.second << ")" << endl;
        pair<Channel::iterator, Channel::iterator> outGreen =
                boost::minmax_element(green->begin(), green->end());
        cout << "Green: (" << *outGreen.first << ", " << *outGreen.second << ")" << endl;
        pair<Channel::iterator, Channel::iterator> outBlue =
                boost::minmax_element(blue->begin(), blue->end());
        cout << "Blue: (" << *outBlue.first << ", " << *outBlue.second << ")" << endl;

        float min = std::min(*outRed.first,
                             std::min(*outGreen.first, *outBlue.first));
        float max = std::max(*outRed.second,
                             std::max(*outGreen.second, *outBlue.second));

        cout << "Min/Max: " << min << ", " << max << std::endl;

        FrameWriterPtr writer = FrameWriterFactory::open(outputFile);
        writer->write(*newHdr,
                      pfs::Params("mapping_method", MAP_GAMMA2_2)
                      ("min_luminance", min)
                      ("max_luminance", max));


    fusionOperator->writeResponsesToFile("responses_after.m");
        return 0;
    }
    catch (std::exception& ex)
    {
        cout << ex.what() << "\n";
        cout << desc << "\n";
        return -1;
    }

#else
    cout << "This code doens't work without a C++11 compiler! \n";
    return -1;
#endif
}

