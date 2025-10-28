/*
 * ------------------------------------------------------------------------------------
 *  File: filternode.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Concrete processing nodes for filters
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include "processingnode.h"
#include "../filters/edgeenhancementfilter.h"
#include "../filters/noisereductionfilter.h"
#include "../filters/morphologyfilter.h"

namespace isis::core::pipeline
{
    /**
     * @brief Processing node for edge enhancement
     */
    class EdgeEnhancementNode : public ProcessingNode
    {
    public:
        EdgeEnhancementNode();

        vtkSmartPointer<vtkImageData> execute(vtkImageData* input) override;

        void setMethod(filters::EdgeDetectionMethod method);
        void setSigma(double sigma);
        void setStrength(double strength);

    private:
        filters::EdgeEnhancementFilter m_filter;
    };

    /**
     * @brief Processing node for noise reduction
     */
    class NoiseReductionNode : public ProcessingNode
    {
    public:
        NoiseReductionNode();

        vtkSmartPointer<vtkImageData> execute(vtkImageData* input) override;

        void setMethod(filters::NoiseReductionMethod method);
        void setRadius(double radius);
        void setSigma(double sigma);
        void setIterations(int iterations);

    private:
        filters::NoiseReductionFilter m_filter;
    };

    /**
     * @brief Processing node for morphological operations
     */
    class MorphologyNode : public ProcessingNode
    {
    public:
        MorphologyNode();

        vtkSmartPointer<vtkImageData> execute(vtkImageData* input) override;

        void setOperation(filters::MorphologyOperation operation);
        void setStructuringElement(filters::StructuringElement element);
        void setKernelRadius(double radius);

    private:
        filters::MorphologyFilter m_filter;
    };

} // namespace isis::core::pipeline
