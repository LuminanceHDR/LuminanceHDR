#include <QString>

#include <iostream>
#include <vector>
#include <boost/program_options.hpp>
#include <boost/algorithm/minmax_element.hpp>

#include <Libpfs/utils/msec_timer.h>
#include <Libpfs/io/framewriterfactory.h>

#include <HdrWizard/HdrCreationManager.h>
// #include <HdrCreation/fusionoperator.h>

using namespace std;
using namespace pfs;
using namespace pfs::io;
namespace po = boost::program_options;

int main(int argc, char** argv)
{
#ifdef LHDR_CXX11_ENABLED
    string          outputFile;
    vector<string>  inputFiles;

    // program options
    po::options_description desc("Allowed options: ");
    desc.add_options()
            ("output-file,o", po::value<std::string>(&outputFile)->required(), "output file")
            ("input-files,i", po::value< vector<string> >(&inputFiles)->required(), "input files (JPEG, TIFF or RAW)")
            ;

    try
    {
        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).
                  options(desc).allow_unregistered().run(), vm);
        po::notify(vm);

        HdrCreationManager hdrCreationManager;

        QStringList inputFilesList;
        for ( const string & file : inputFiles ) {
            inputFilesList.push_back( QString::fromStdString(file) );
        }
        hdrCreationManager.loadFiles( inputFilesList );

        cout << "Are you ready?\n";
        char c;
        cin >> c;

        msec_timer t;
        t.start();

        pfs::Frame* newHdr = hdrCreationManager.createHdr(true, 0);
        if ( newHdr == NULL ) {
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

        FrameWriterPtr writer = FrameWriterFactory::open(outputFile);
        writer->write( *newHdr, pfs::Params() );

        return 0;
    }
    catch (std::exception& ex) {
        cout << desc << "\n";
        return -1;
    }

#else
    cout << "This code doens't work without a C++11 compiler! \n";
    return -1;
#endif
}

