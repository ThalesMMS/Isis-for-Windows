/*
 * ------------------------------------------------------------------------------------
 *  File: rigidregistration.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Rigid image registration (rotation + translation)
 *      with automatic optimization and similarity metrics
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkTransform.h>
#include <vtkImageReslice.h>

namespace isis::core::registration
{
    /**
     * @brief Similarity metric for registration
     */
    enum class SimilarityMetric
    {
        MeanSquares,
        NormalizedCrossCorrelation,
        MutualInformation
    };

    /**
     * @brief Rigid registration algorithm
     *
     * Implements 6-DOF rigid registration (3 rotations + 3 translations)
     * with various similarity metrics and automatic optimization.
     */
    class RigidRegistration
    {
    public:
        RigidRegistration();
        ~RigidRegistration() = default;

        /**
         * @brief Set fixed (reference) image
         * @param fixedImage Fixed image
         */
        void setFixedImage(vtkImageData* fixedImage);

        /**
         * @brief Set moving image to be registered
         * @param movingImage Moving image
         */
        void setMovingImage(vtkImageData* movingImage);

        /**
         * @brief Set similarity metric
         * @param metric Metric type
         */
        void setSimilarityMetric(SimilarityMetric metric);

        /**
         * @brief Set maximum number of iterations
         * @param iterations Maximum iterations
         */
        void setMaxIterations(int iterations);

        /**
         * @brief Set optimization learning rate
         * @param rate Step size for optimizer
         */
        void setLearningRate(double rate);

        /**
         * @brief Execute registration
         * @return Registered image
         */
        vtkSmartPointer<vtkImageData> execute();

        /**
         * @brief Get the computed transformation
         * @return VTK transform matrix
         */
        [[nodiscard]] vtkTransform* getTransform() const;

        /**
         * @brief Get registration quality metric
         * @return Quality score
         */
        [[nodiscard]] double getQualityMetric() const;

        /**
         * @brief Get final similarity metric value
         * @return Final metric score
         */
        [[nodiscard]] double getFinalMetricValue() const;

    private:
        vtkSmartPointer<vtkImageData> m_fixedImage;
        vtkSmartPointer<vtkImageData> m_movingImage;
        vtkSmartPointer<vtkTransform> m_transform;
        vtkSmartPointer<vtkImageData> m_registeredImage;
        SimilarityMetric m_metric = SimilarityMetric::MeanSquares;
        int m_maxIterations = 100;
        double m_qualityMetric = 0.0;
        double m_learningRate = 0.1;

        // Helper methods
        void initializeTransform();
        void optimizeTransform();
        double computeSimilarity();
    };

} // namespace isis::core::registration
