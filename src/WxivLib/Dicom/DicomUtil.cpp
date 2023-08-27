// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#include <filesystem>
#include <fmt/core.h>

#include <opencv2/opencv.hpp>

#include <wx/clipbrd.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/confbase.h>

#include "WxivUtil.h"
#include "MiscUtil.h"
#include "WxWidgetsUtil.h"
#include "StringUtil.h"
#include "ImageUtil.h"

#include "DicomUtil.h"
#include "Contour.h"

#include "dcmtk/dcmdata/dcfilefo.h"
#include "dcmtk/dcmdata/dcdeftag.h"
#include "dcmtk/dcmdata/dcuid.h"
#include "dcmtk/dcmimgle/dcmimage.h"
#include "dcmtk/dcmimage/dipitiff.h"
#include "dcmtk/dcmimage/dipipng.h"

using namespace std;
namespace fs = std::filesystem;

namespace Wxiv
{
    unordered_map<int, string> getStructureNames(DcmFileFormat dcmFile)
    {
        unordered_map<int, string> structureNames;
        DcmSequenceOfItems* structureSetRois = NULL;

        if (dcmFile.getDataset()->findAndGetSequence(DCM_StructureSetROISequence, structureSetRois).good())
        {
            DcmObject* obj = nullptr;

            while ((obj = structureSetRois->nextInContainer(obj)) != NULL)
            {
                DcmItem* item = static_cast<DcmItem*>(obj);
                string roiName;
                int roiNumber = -1;

                if (item->findAndGetOFString(DCM_ROIName, roiName).good()
                    && item->findAndGetSint32(DCM_ROINumber, roiNumber).good())
                {
                    structureNames[roiNumber] = roiName;
                }
                else
                {
                    cerr << "Error: cannot access ROI Name!" << endl;
                }
            }
        }

        return structureNames;
    }

    /**
    * @brief Parse an RGB color string to an array of bytes.
    * @param colorStr Like "255\\255\\255"
    * @param rgbColor Output.
    * @return True if the string was parsed successfully.
    */
    bool tryParseColorStr(string colorStr, uint8_t rgbColor[3])
    {
        int r, g, b;
        int result = sscanf_s(colorStr.c_str(), "%d\\%d\\%d", &r, &g, &b);

        if (result == 3)
        {
            rgbColor[0] = (uint8_t)r;
            rgbColor[1] = (uint8_t)g;
            rgbColor[2] = (uint8_t)b;
            return true;
        }
        else
        {
            return false;
        }
    }

    void parseFloatString(string& dataStr, vector<float>& floatValues)
    {
        std::istringstream ss(dataStr.c_str());
        std::string token;

        while (getline(ss, token, '\\')) {
            floatValues.push_back(std::stod(token));
        }
    }

    /**
    * @brief Parse all slices of a contour sequence.
    */
    bool tryParseContourSequence(DcmSequenceOfItems* contourSequence, Contour& contour)
    {
        DcmObject* obj = nullptr;

        // for each slice
        while ((obj = contourSequence->nextInContainer(obj)) != NULL)
        {
            DcmItem* item = static_cast<DcmItem*>(obj);
            string dataStr;
            int numberOfPoints = -1;

            // if values were a DecimalString (but they are not)
            //const Float64 *floatValues = nullptr;
            //unsigned long count = numberOfPoints;
            //&& item->findAndGetFloat64Array(DCM_ContourData, floatValues, &count).good()
            //cout << "Count: " << count << endl;

            if (item->findAndGetSint32(DCM_NumberOfContourPoints, numberOfPoints).good()
                && item->findAndGetOFStringArray(DCM_ContourData, dataStr).good()
                )
            {
                vector<float> floatValues;
                parseFloatString(dataStr, floatValues);
                //cout << "Points: " << numberOfPoints << ", values: " << floatValues.size() << endl;

                vector<ContourPoint> points;

                for (int i = 0; i < floatValues.size(); i += 3)
                {
                    ContourPoint pt;
                    pt.x = floatValues[i];
                    pt.y = floatValues[i + 1];
                    pt.z = floatValues[i + 2];
                    points.push_back(pt);
                }

                contour.slicePoints.push_back(points);
            }
            else
            {
                cerr << "Error: cannot get contourData or number of contour points" << endl;
                return false;
            }
        }

        return true;
    }

    vector<Contour> getContours(DcmFileFormat dcmFile)
    {
        vector<Contour> contours;
        DcmSequenceOfItems* roiContours = NULL;

        if (dcmFile.getDataset()->findAndGetSequence(DCM_ROIContourSequence, roiContours).good())
        {
            DcmObject* obj = nullptr;

            while ((obj = roiContours->nextInContainer(obj)) != NULL)
            {
                DcmItem* item = static_cast<DcmItem*>(obj);
                string colorStr;
                int roiNumber = -1;
                DcmSequenceOfItems* contourSequence = NULL;

                if (item->findAndGetOFStringArray(DCM_ROIDisplayColor, colorStr).good()
                    && item->findAndGetSint32(DCM_ReferencedROINumber, roiNumber).good()
                    && item->findAndGetSequence(DCM_ContourSequence, contourSequence).good()
                    )
                {
                    //cout << "Color: " << colorStr << ", number " << roiNumber << endl;
                    Contour contour;
                    contour.referencedRoiNumber = roiNumber;

                    if (tryParseColorStr(colorStr, contour.rgbColor) 
                        && tryParseContourSequence(contourSequence, contour))
                    {
                        contours.push_back(contour);
                    }
                    else
                    {
                        cerr << "Error: failed to parse color or contour points sequence" << endl;
                        break;
                    }
                }
                else
                {
                    cerr << "Error: cannot access ROI Name!" << endl;
                    break;
                }
            }
        }

        return contours;
    }

    void dumpStructureContours(string dcmFilePath)
    {
        DcmFileFormat dcmFile;
        OFCondition status = dcmFile.loadFile(dcmFilePath);

        if (status.good())
        {
            string patientName;

            if (dcmFile.getDataset()->findAndGetOFString(DCM_PatientName, patientName).good())
            {
                cout << "Patient's Name: " << patientName << endl;

                DcmObject* dset = &dcmFile;

                size_t pixelCounter = 0;
                const char* pixelFileName = NULL;
                size_t printFlags = DCMTypes::PF_shortenLongTagValues;

                //dset->print(cout, printFlags, 2 /*level*/, pixelFileName, &pixelCounter);
                unordered_map<int, string> structureNames = getStructureNames(dcmFile);
                vector<Contour> contours = getContours(dcmFile);

                for (auto contour : contours)
                {
                    if (structureNames.contains(contour.referencedRoiNumber))
                    {
                        contour.name = structureNames[contour.referencedRoiNumber];
                        cout << contour.toString() << endl;
                    }
                    else
                    {
                        cerr << "Missing structure name for contour with roi number " << contour.referencedRoiNumber << endl;
                    }
                }
            }
            else
            {
                cerr << "Error: cannot access Patient's Name!" << endl;
            }

        }
        else
        {
            cerr << "Error: cannot read DICOM file (" << status.text() << ")" << endl;
        }
    }

    cv::Mat dcmToOpencv(DicomImage* di, int frameIndex = 0)
    {
        // internally di can have 13-bit depth
        int depth = ((di->getDepth() - 1) / 8 + 1) * 8; // 13 -> 16
        int bufferSize = di->getOutputDataSize(0); // it pads to nearest byte
        cv::Mat img;

        if (depth == 16)
        {
            img.create(di->getHeight(), di->getWidth(), CV_16UC1);
            di->getOutputData(img.ptr(), bufferSize, depth, frameIndex);
        }
        else
        {
            throw new std::exception("Input DICOM image depth not supported.");
        }

        return img;
    }

    DicomImage* loadDicomImage(OFFilename dcmFilePath)
    {
        DcmFileFormat dcmFile;
        OFCondition status = dcmFile.loadFile(dcmFilePath);

        if (!status.good())
        {
            cerr << "Error: cannot read DICOM file (" << status.text() << ")" << endl;
            return nullptr;
        }

        DcmDataset* dataset = dcmFile.getDataset();
        E_TransferSyntax xfer = dataset->getOriginalXfer();

        Sint32 frameCount;
        if (dataset->findAndGetSint32(DCM_NumberOfFrames, frameCount).bad())
            frameCount = 1;

        if (frameCount > 1)
        {
            cout << "Warn: Just writing first frame but frame count is " << frameCount << endl;
            frameCount = 1;
        }

        unsigned long compatMode = CIF_MayDetachPixelData | CIF_TakeOverExternalDataset;
        DicomImage* di = new DicomImage(dataset, xfer, compatMode, 0, frameCount);

        if (di == NULL)
        {
            cerr << "Error: Failed to load dicom image (out of mem?)" << endl;
            return nullptr;
        }

        if (di->getStatus() != EIS_Normal)
        {
            cerr << "Error: getStatus not normal" << endl;
            return nullptr;
        }

        // convert to monochrome
        if (!di->isMonochrome())
        {
            cout << "Warn: Converting to monochrome" << endl;
            DicomImage* newimage = di->createMonochromeImage();

            if (newimage == NULL)
            {
                cout << "Out of memory or cannot convert to monochrome image" << endl;
                return nullptr;
            }
            else if (newimage->getStatus() != EIS_Normal)
            {
                cout << DicomImage::getString(newimage->getStatus()) << endl;
                return nullptr;
            }

            delete di;
            di = newimage;
        }

        return di;
    }

    void dumpImages(string dcmFilePath, string outputFilePath)
    {
        DicomImage* di = loadDicomImage(dcmFilePath);

        // write
        auto ofile = fopen(outputFilePath.c_str(), "wb");

        if (ofile == NULL)
        {
            cerr << "Error: Failed to open output file" << endl;
            return;
        }

        di->hideAllOverlays();

        for (unsigned int frame = 0; frame < 1; frame++)
        {
            int result = 0;

            if (outputFilePath.ends_with(".tif") || outputFilePath.ends_with(".tiff"))
            {
                // this may only support 8 bit
                //DiTIFFPlugin tiffPlugin;
                //tiffPlugin.setCompressionType(E_tiffLZWCompression);
                //tiffPlugin.setLZWPredictor(E_tiffLZWPredictorDefault);
                //tiffPlugin.setRowsPerStrip(0);
                //result = di->writePluginFormat(&tiffPlugin, ofile, frame);

                cv::Mat img = dcmToOpencv(di, frame);
                cv::imwrite(outputFilePath, img);
                result = 1;
            }
            else if (outputFilePath.ends_with(".png"))
            {
                // PNG
                DiPNGPlugin pngPlugin;
                pngPlugin.setInterlaceType(E_pngInterlaceAdam7);
                pngPlugin.setMetainfoType(E_pngFileMetainfo);
                //if (opt_fileType == EFT_16bitPNG)
                // pngPlugin.setBitsPerSample(16);
                result = di->writePluginFormat(&pngPlugin, ofile, frame);
            }
            else
            {
                cerr << "Error: Unsupported output file format" << endl;
                return;
            }

            fclose(ofile);

            if (!result)
            {
                cerr << "Error: Failed to write image file" << endl;
                return;
            }

        }

        //delete di; // is throwing
    }

    /**
    * @brief wxWidgets to load image in cross-platform way. For paths with non-ASCII characters this can only load a single page
    * from multi-page tif's.
    * @param path
    * @param mats
    * @return
    */
    bool wxLoadDicomImage(const wxString& path, vector<cv::Mat>& mats)
    {
        bool result = false;

        // extension has to be ascii
        wxFileName fn(path);

        if (!fn.HasExt())
        {
            return false;
        }

        wxString wxExt = fn.GetExt();

        if (!checkIsOnlyAscii(wxExt))
        {
            throw runtime_error("Image files must have known extensions and those must be ASCII.");
        }

        if (checkIsOnlyAscii(path))
        {
            DcmFileFormat dcmFile;
            OFFilename dcmFilePath = path.ToStdString().c_str();
            OFCondition status = dcmFile.loadFile(dcmFilePath);

            if (!status.good())
            {
                cerr << "Error: cannot read DICOM file (" << status.text() << ")" << endl;
                result = false;
            }
            else
            {
                DicomImage* di = loadDicomImage(dcmFilePath);

                if (di != nullptr)
                {
                    cv::Mat img = dcmToOpencv(di, 0);
                    mats.push_back(img);
                    result = true;
                }
                else
                {
                    throw runtime_error("Failed load DICOM image.");
                }
            }
        }
        else
        {
            // haven't actually even tried this
            throw runtime_error("DICOM image files must have ASCII paths.");
        }

        if (result && !mats.empty())
        {
            string ext = wxExt.ToStdString();

            for (cv::Mat& img : mats)
            {
                ImageUtil::convertAfterLoad(img, ext, img);
            }
        }

        return result;
    }
}
