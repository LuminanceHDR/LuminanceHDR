/**
 * This file is a part of Luminance HDR package
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2006,2007 Giuseppe Rota
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ---------------------------------------------------------------------- 
 *
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 * Improvements, bugfixing
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#ifndef HDRCREATIONMANAGER_H
#define HDRCREATIONMANAGER_H

#include <cstddef>
#include <vector>

#include <QProcess>
#include <QPair>

#include <Libpfs/frame.h>

#include "Common/LuminanceOptions.h"
#include "arch/math.h"
#include "HdrCreation/createhdr.h"
#include "HdrCreation/createhdr_common.h"

// Some other file expect this to be available
const config_triple predef_confs[6]= {
{TRIANGULAR, LINEAR,DEBEVEC,"",""},
{TRIANGULAR, GAMMA, DEBEVEC,"",""},
{PLATEAU, LINEAR,DEBEVEC,"",""},
{PLATEAU, GAMMA,DEBEVEC,"",""},
{GAUSSIAN, LINEAR,DEBEVEC,"",""},
{GAUSSIAN, GAMMA,DEBEVEC,"",""},
};

// defines an element that contains all the informations for this particular
// image to be used inside the HdrWizard
class HdrCreationItem {
public:
    HdrCreationItem(const QString& filename);
    ~HdrCreationItem();

    const QString& filename() const     { return m_filename; }
    FramePtr frame()                    { return m_frame; }
    bool isValid() const                { return m_frame->isValid(); }

    bool hasEV() const                  { return (m_ev != -std::numeric_limits<float>::max()); }
private:
    QString     m_filename;
    float       m_ev;
    FramePtr    m_frame;
    QImage      m_thumbnail;

};

class HdrCreationManager : public QObject
{
    Q_OBJECT
private:
    typedef std::vector< HdrCreationItem > HdrCreationItemContainer;

    HdrCreationItemContainer m_data;

public:
	HdrCreationManager(bool = false);
	~HdrCreationManager();


    // ----- NEW FUNCTIONS ------
    void loadFiles(const QStringList& filenames);
    void removeFile(int idx);
    void clearFiles();
    size_t availableInputFiles() const { return m_data.size(); }

    // iterators
    typedef HdrCreationItemContainer::iterator  iterator;
    typedef HdrCreationItemContainer::const_iterator const_iterator;

    iterator begin() { return m_data.begin(); }
    iterator end() { return m_data.end(); }
    const_iterator begin() const { return m_data.begin(); }
    const_iterator end() const { return m_data.end(); }

   // ----- LEGACY FUNCTIONS ----

    void setConfig(const config_triple& cfg);

	//LDR is a  8 bit format (jpeg, 8bit tiff, raw->8bit tiff)
	//MDR is a 16 bit format (16bit tiff, raw->16bit tiff)
    enum {
        LDR_INPUT_TYPE = 0,
        MDR_INPUT_TYPE = 1,
        UNKNOWN_INPUT_TYPE = 2
    } inputType;

    // initialize internal structures before actually loading the files
    // void setFileList(const QStringList&);
    // load files listed in fileList in a threaded way
    // void loadInputFiles();
    // clear lists used internally
    // void clearlists(bool deleteExpotimeAsWell);

	pfs::Frame* createHdr(bool ag, int iterations);

    void set_ais_crop_flag(bool flag);
	void align_with_ais();
	void align_with_mtb();

    const QList<QImage*>& getLDRList() const                { return ldrImagesList; }
    const QList<QImage*>& getMDRList() const                { return mdrImagesList; }
    const QList<QImage*>& getAntiGhostingMasksList() const  { return antiGhostingMasksList; }
    const QVector<float>& getExpotimes() const              { return expotimes; }
    const QStringList& getFileList() const                  { return fileList; }
    bool  inputImageType() const                            { return inputType; }
    const QStringList& getFilesLackingExif() const          { return filesLackingExif; }
    bool  isValidEV(int i) const                            { return expotimes[i]!=-1; }
    float getEV(int i) const                                { return log2f(expotimes[i]); }

    // updates EV value in expotimes array and emits signal
	void setEV(float newev, int image_idx);

    // the configuration used to create the hdr
    // this is public so that the wizard (or the cli?) can modify it directly.
	config_triple chosen_config;

    // operates on expotimes array:
    // if the correspondent EV value span is >10EV or <-10EV,
    // add an offset to the expotimes array to make it stay inside boundaries (-10..+10).
    // the EV values cannot cover more than 20EV values
	void checkEVvalues();
	void makeSureLDRsHaveAlpha();
    void applyShiftsToImageStack(const QList< QPair<int,int> >& HV_offsets);
    void applyShiftsToMdrImageStack(const QList< QPair<int,int> >& HV_offsets);

    void cropLDR(const QRect& ca);
    void cropMDR(const QRect& ca);
    void cropAgMasks(const QRect& ca);

    void reset();
	void remove(int index);
	void setShift(int shift) { m_shift = shift; }
    void saveLDRs(const QString& filename);
    void saveMDRs(const QString& filename);
	void doAntiGhosting(int);
	void doAutoAntiGhosting(float);

public slots:
    // remove temp 8or16 bit tiff files created by libRaw upon raw input.
	void removeTempFiles();

signals:
    void finishedLoadingInputFiles(const QStringList& filesLackingExif);
    void errorWhileLoading(const QString& message); //also for !valid size

    void fileLoaded(int index, const QString& fname, float expotime);

	void finishedAligning(int);
	void expotimeValueChanged(float,int);
	void ais_failed(QProcess::ProcessError);
    void aisDataReady(const QByteArray& data);

	void maximumValue(int);
	void nextstep(int);

	void processed();
	void imagesSaved();

private:
	void doAutoAntiGhostingMDR(float);
	void doAutoAntiGhostingLDR(float);
    // List of input files (absolute pathnames)
	QStringList fileList;
	//data structures that hold the input images' payload
	QList<QImage*> ldrImagesList;  //ldr input
	QList<QImage*> mdrImagesList;  //QImages rappresenting a PFS frame for editing tools
	QList<QImage*> mdrImagesToRemove;  //QImages need to be deleted
	QList<QImage*> antiGhostingMasksList;  //QImages used for manual anti ghosting
	QList<bool> tiffLdrList;  //tiff ldr input
    Array2DfList listmdrR,listmdrG,listmdrB; //mdr input
	//if startedProcessing[i]==true, we started a thread for the i-th file
	QList<bool> startedProcessing;
    // time equivalent array (from exif data)
    // float *expotimes;
	QVector<float> expotimes;

    // Filled on every successful load and left untouched afterwards.
    // Value emitted after all the loading has been completed
	QStringList filesLackingExif;
    // Filled when we have raw files as input.
    // QStringList filesToRemove;
	QVector<QString>  filesToRemove;
    // set to true as soon as we find out that we cannot load a file or when we find out that a file has a different width/height than the other previously loaded ones.
    // This variable prevents "incoming" threads to do anything.
	bool m_loadingError;

    // number of running threads at any given time
	int m_runningThreads;
    // cumulative number of successfully loaded files
	int m_processedFiles;

    LuminanceOptions m_luminance_options;

    // align_image_stack
	QProcess *ais;

    bool m_ais_crop_flag;

	int m_shift;

    int m_mdrWidth;
    int m_mdrHeight;
	
	bool fromCommandLine;

    // once a new LDR or MDR pops up, the slots call this function to perform some housekeeping
    void newResult(int index, float expotime, const QString&);

    bool ldrsHaveSameSize(int, int);
    bool mdrsHaveSameSize(size_t, size_t);

private slots:
	void ais_finished(int,QProcess::ExitStatus);
	void ais_failed_slot(QProcess::ProcessError);
    void ldrReady(QImage*, int, float, const QString&, bool);
    void mdrReady(pfs::Frame*, int, float, const QString&);
    void loadFailed(const QString& fname, int index);
	void readData();
};
#endif
