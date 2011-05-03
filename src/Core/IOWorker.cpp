#include <QFileInfo>
#include <QString>
#include <QByteArray>

#include "Core/IOWorker.h"
#include "Fileformat/pfs_file_format.h"

IOWorker::IOWorker(QObject* parent): QObject(parent)
{
    luminance_options = LuminanceOptions::getInstance();
}

IOWorker::~IOWorker()
{
    printf("IOWorker::~IOWorker()\n");
}

void IOWorker::write_frame(HdrViewer* hdr_input, QString filename)
{
    emit IO_init();

    QFileInfo qfi(filename);
    QString absoluteFileName = qfi.absoluteFilePath();
    QByteArray encodedName = absoluteFileName.toLocal8Bit();

    pfs::Frame* hdr_frame = hdr_input->getHDRPfsFrame();

    if (qfi.suffix().toUpper() == "EXR")
    {
        writeEXRfile(hdr_frame, encodedName);
    }
    else if (qfi.suffix().toUpper() == "HDR")
    {
        writeRGBEfile(hdr_frame, encodedName);
    }
    else if (qfi.suffix().toUpper().startsWith("TIF"))
    {
        TiffWriter tiffwriter(encodedName, hdr_frame);
        connect(&tiffwriter, SIGNAL(maximumValue(int)), this, SIGNAL(setMaximum(int)));
        connect(&tiffwriter, SIGNAL(nextstep(int)), this, SIGNAL(setValue(int)));
        if (luminance_options->saveLogLuvTiff)
        {
            tiffwriter.writeLogLuvTiff();
        }
        else
        {
            tiffwriter.writeFloatTiff();
        }
    }
    else if (qfi.suffix().toUpper() == "PFS")
    {
        FILE *fd = fopen(encodedName, "w");
        pfs::DOMIO pfsio;
        pfsio.writeFrame(hdr_frame, fd);
        fclose(fd);
    }
    else
    {
        // Default as EXR
        absoluteFileName = absoluteFileName + ".exr";
        QByteArray encodedName_2 = absoluteFileName.toLocal8Bit();

        writeEXRfile(hdr_frame, encodedName_2);
    }
    emit write_success(hdr_input, filename);

    emit IO_finish();
}


void IOWorker::read_frame(QString filename)
{
    emit IO_init();

    get_frame(filename);

    emit IO_finish();
}

void IOWorker::read_frames(QStringList filenames)
{
    emit IO_init();
    foreach (QString filename, filenames)
    {
        get_frame(filename);
    }
    emit IO_finish();
}

void IOWorker::get_frame(QString fname)
{
    if ( fname.isEmpty() )
        return;

    QFileInfo qfi(fname);
    if ( !qfi.isReadable() )
    {
        qDebug("File %s is not readable.", fname.toLocal8Bit().constData());
        emit read_failed(tr("ERROR: The following file is not readable: %1").arg(fname));
        return;
    }

    pfs::Frame* hdrpfsframe = NULL;
    QStringList rawextensions;
    rawextensions << "CRW" << "CR2" << "NEF" << "DNG" << "MRW" << "ORF" << "KDC" << "DCR" << "ARW" << "RAF" << "PTX" << "PEF" << "X3F" << "RAW" << "SR2" << "3FR" << "RW2" << "MEF" << "MOS" << "ERF" << "NRW";
    QString extension = qfi.suffix().toUpper();
    bool rawinput = (rawextensions.indexOf(extension) != -1);
    try
    {
        QByteArray TempPath = (luminance_options->tempfilespath).toLocal8Bit();
        QByteArray FilePath = qfi.absoluteFilePath().toLocal8Bit();
        const char* encodedFileName = FilePath.constData(); // It will be surely needed... so I prefer to do it now

        if (extension=="EXR")
        {
            hdrpfsframe = readEXRfile(encodedFileName);
        }
        else if (extension=="HDR")
        {
            hdrpfsframe = readRGBEfile(encodedFileName);
        }
        else if (extension=="PFS")
        {
            //TODO : check this code and make it smoother
            //const char *fname = encodedFileName;
            FILE *fd = fopen(encodedFileName, "rb");
            if (!fd) {
                emit read_failed(tr("ERROR: Cannot open file: %1").arg(fname));
                return;
            }
            pfs::DOMIO pfsio;
            hdrpfsframe = pfsio.readFrame(fd);
            fclose(fd);
        }
        else if (extension.startsWith("TIF"))
        {
            // from 8,16,32,logluv to pfs::Frame
            TiffReader reader(encodedFileName, TempPath.constData(), false );
            connect(&reader, SIGNAL(maximumValue(int)), this, SIGNAL(setMaximum(int)));
            connect(&reader, SIGNAL(nextstep(int)), this, SIGNAL(setValue(int)));
            hdrpfsframe = reader.readIntoPfsFrame();
        }
        else if (rawinput)
        {
            // raw file detected
            hdrpfsframe = readRawIntoPfsFrame(encodedFileName, TempPath.constData(), luminance_options, false);
        }
        else
        {
            qDebug("TH: File %s has unsupported extension.", qPrintable(fname));
            emit read_failed(tr("ERROR: File %1 has unsupported extension.").arg(fname));
            return;
        }

        if (hdrpfsframe == NULL)
        {
            throw "Error loading file";
        }
    }
    catch(pfs::Exception e)
    {
        emit read_failed(tr("ERROR: %1").arg(e.getMessage()));
        return;
    }
    catch (...)
    {
        qDebug("TH: catched exception");
        emit read_failed(tr("ERROR: Failed loading file: %1").arg(fname));
        return;
    }
    emit read_success(hdrpfsframe, fname);
}



