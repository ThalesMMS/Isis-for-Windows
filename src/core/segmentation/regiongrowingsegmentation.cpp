/*
 * ------------------------------------------------------------------------------------
 *  File: regiongrowingsegmentation.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implementation of region growing segmentation
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "regiongrowingsegmentation.h"
#include <vtkCellArray.h>
#include <vtkImageThreshold.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>

namespace isis::core::segmentation
{
    RegionGrowingSegmentation::RegionGrowingSegmentation()
    {
        m_connectivityFilter = vtkSmartPointer<vtkImageConnectivityFilter>::New();
    }

    void RegionGrowingSegmentation::setInputImage(vtkImageData* input)
    {
        if (input)
        {
            m_inputImage = input;
        }
    }

    void RegionGrowingSegmentation::addSeedPoint(int x, int y, int z)
    {
        m_seedPoints.push_back({x, y, z});
    }

    void RegionGrowingSegmentation::clearSeeds()
    {
        m_seedPoints.clear();
    }

    void RegionGrowingSegmentation::setIntensityRange(double lower, double upper)
    {
        m_lowerThreshold = lower;
        m_upperThreshold = upper;
    }

    void RegionGrowingSegmentation::setNeighborhoodSize(int size)
    {
        m_neighborhoodSize = size;
    }

    vtkSmartPointer<vtkImageData> RegionGrowingSegmentation::execute()
    {
        if (!m_inputImage || m_seedPoints.empty())
        {
            return nullptr;
        }

        // First, threshold the image
        auto thresholdFilter = vtkSmartPointer<vtkImageThreshold>::New();
        thresholdFilter->SetInputData(m_inputImage);
        thresholdFilter->ThresholdBetween(m_lowerThreshold, m_upperThreshold);
        thresholdFilter->SetInValue(255);
        thresholdFilter->SetOutValue(0);
        thresholdFilter->Update();

        // Apply connectivity filter with seed points
        m_connectivityFilter->SetInputData(thresholdFilter->GetOutput());
        m_connectivityFilter->SetExtractionModeToSeededRegions();

        auto seedPoints = vtkSmartPointer<vtkPoints>::New();
        auto seedVertices = vtkSmartPointer<vtkCellArray>::New();

        for (const auto& seed : m_seedPoints)
        {
            const vtkIdType pointId =
                seedPoints->InsertNextPoint(static_cast<double>(seed[0]),
                    static_cast<double>(seed[1]),
                    static_cast<double>(seed[2]));
            seedVertices->InsertNextCell(1);
            seedVertices->InsertCellPoint(pointId);
        }

        auto seedPolyData = vtkSmartPointer<vtkPolyData>::New();
        seedPolyData->SetPoints(seedPoints);
        seedPolyData->SetVerts(seedVertices);
        m_connectivityFilter->SetSeedData(seedPolyData);

        m_connectivityFilter->Update();
        m_outputMask = m_connectivityFilter->GetOutput();

        return m_outputMask;
    }

    vtkImageData* RegionGrowingSegmentation::getOutput() const
    {
        return m_outputMask;
    }

} // namespace isis::core::segmentation
