/*
 * ------------------------------------------------------------------------------------
 *  File: integration_example.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Integration examples demonstrating all phases working together
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "../core/testing/testutils.h"
#include "../core/filters/noisereductionfilter.h"
#include "../core/filters/edgeenhancementfilter.h"
#include "../core/filters/morphologyfilter.h"
#include "../core/segmentation/thresholdsegmentation.h"
#include "../core/segmentation/regiongrowingsegmentation.h"
#include "../core/registration/rigidregistration.h"
#include "../core/registration/deformableregistration.h"
#include "../core/pipeline/processingpipeline.h"
#include "../core/pipeline/filternode.h"
#include "../core/events/callbackmanager.h"

#include <iostream>
#include <vector>

using namespace isis::core;

/**
 * Example 1: Complete pre-processing pipeline
 */
void example_preprocessing_pipeline()
{
    std::cout << "\n========== Example 1: Pre-processing Pipeline ==========\n" << std::endl;

    // Create test image with noise
    auto testImage = testing::TestUtils::createTestImage3D(100, 100, 50, 3);  // Sphere

    // Create callback manager for progress tracking
    auto callbackMgr = std::make_shared<events::CallbackManager>();

    // Register progress callback
    callbackMgr->registerProgressCallback(
        [](double progress, const std::string& msg) {
            std::cout << "Progress: " << static_cast<int>(progress * 100)
                     << "% - " << msg << std::endl;
        }, "ProgressTracker");

    // Create pipeline
    auto pipeline = std::make_shared<pipeline::ProcessingPipeline>();
    pipeline->setCallbackManager(callbackMgr);

    // Step 1: Noise reduction
    auto noiseNode = std::make_shared<pipeline::NoiseReductionNode>();
    noiseNode->setMethod(filters::NoiseReductionMethod::Gaussian);
    noiseNode->setSigma(1.5);
    pipeline->addNode(noiseNode);

    // Step 2: Edge enhancement
    auto edgeNode = std::make_shared<pipeline::EdgeEnhancementNode>();
    edgeNode->setMethod(filters::EdgeDetectionMethod::Sobel);
    edgeNode->setStrength(0.4);
    pipeline->addNode(edgeNode);

    // Execute pipeline
    std::cout << "Executing pre-processing pipeline..." << std::endl;
    auto result = pipeline->execute(testImage);

    if (result)
    {
        std::cout << "✓ Pipeline executed successfully!" << std::endl;
        std::cout << "  Execution time: " << pipeline->getExecutionTime() << " ms" << std::endl;

        // Get statistics
        double min, max, mean, stddev;
        testing::TestUtils::getImageStatistics(result, min, max, mean, stddev);
        std::cout << "  Result statistics: min=" << min << ", max=" << max
                 << ", mean=" << mean << ", stddev=" << stddev << std::endl;
    }
    else
    {
        std::cout << "✗ Pipeline failed: " << pipeline->getLastError() << std::endl;
    }
}

/**
 * Example 2: Segmentation workflow with morphological post-processing
 */
void example_segmentation_workflow()
{
    std::cout << "\n========== Example 2: Segmentation Workflow ==========\n" << std::endl;

    // Create test image
    auto testImage = testing::TestUtils::createTestImage3D(100, 100, 50, 3);  // Sphere

    // Step 1: Threshold segmentation
    std::cout << "Step 1: Threshold segmentation..." << std::endl;
    segmentation::ThresholdSegmentation thresholdSeg;
    thresholdSeg.setInputImage(testImage);
    thresholdSeg.setLowerThreshold(100.0);
    thresholdSeg.setUpperThreshold(255.0);
    auto segmentedMask = thresholdSeg.execute();

    if (segmentedMask)
    {
        std::cout << "✓ Threshold segmentation completed" << std::endl;
    }

    // Step 2: Morphological opening to remove small objects
    std::cout << "Step 2: Morphological opening..." << std::endl;
    filters::MorphologyFilter morphFilter;
    morphFilter.setInputImage(segmentedMask);
    morphFilter.setOperation(filters::MorphologyOperation::Open);
    morphFilter.setKernelRadius(2.0);
    auto openedMask = morphFilter.execute();

    if (openedMask)
    {
        std::cout << "✓ Morphological opening completed" << std::endl;
    }

    // Step 3: Morphological closing to fill holes
    std::cout << "Step 3: Morphological closing..." << std::endl;
    morphFilter.setInputImage(openedMask);
    morphFilter.setOperation(filters::MorphologyOperation::Close);
    morphFilter.setKernelRadius(3.0);
    auto finalMask = morphFilter.execute();

    if (finalMask)
    {
        std::cout << "✓ Morphological closing completed" << std::endl;
        std::cout << "✓ Segmentation workflow finished!" << std::endl;
    }
}

/**
 * Example 3: Multi-modal registration
 */
void example_registration_workflow()
{
    std::cout << "\n========== Example 3: Registration Workflow ==========\n" << std::endl;

    // Create fixed and moving images
    auto fixedImage = testing::TestUtils::createTestImage3D(100, 100, 50, 3);
    auto movingImage = testing::TestUtils::createTestImage3D(100, 100, 50, 3);

    // Step 1: Rigid registration
    std::cout << "Step 1: Rigid registration..." << std::endl;
    registration::RigidRegistration rigidReg;
    rigidReg.setFixedImage(fixedImage);
    rigidReg.setMovingImage(movingImage);
    rigidReg.setMetric(registration::SimilarityMetric::MutualInformation);
    rigidReg.setMaxIterations(100);
    rigidReg.setLearningRate(0.1);

    auto rigidResult = rigidReg.execute();

    if (rigidResult)
    {
        std::cout << "✓ Rigid registration completed" << std::endl;
        std::cout << "  Final metric value: " << rigidReg.getFinalMetricValue() << std::endl;
    }

    // Step 2: Deformable registration for fine-tuning
    std::cout << "Step 2: Deformable registration..." << std::endl;
    registration::DeformableRegistration deformReg;
    deformReg.setFixedImage(fixedImage);
    deformReg.setMovingImage(rigidResult);

    // Add control points (example landmarks)
    double source1[3] = {25.0, 25.0, 25.0};
    double target1[3] = {25.0, 25.0, 25.0};
    double source2[3] = {75.0, 75.0, 25.0};
    double target2[3] = {75.0, 75.0, 25.0};

    deformReg.addControlPointPair(source1, target1);
    deformReg.addControlPointPair(source2, target2);
    deformReg.setRegularization(0.1);

    auto deformResult = deformReg.execute();

    if (deformResult)
    {
        std::cout << "✓ Deformable registration completed" << std::endl;

        // Visualize deformation
        auto defMagnitude = deformReg.visualizeDeformationMagnitude();
        if (defMagnitude)
        {
            double min, max, mean, stddev;
            testing::TestUtils::getImageStatistics(defMagnitude, min, max, mean, stddev);
            std::cout << "  Deformation magnitude: mean=" << mean << ", max=" << max << std::endl;
        }

        std::cout << "✓ Registration workflow finished!" << std::endl;
    }
}

/**
 * Example 4: Complete image analysis pipeline with callbacks
 */
void example_complete_analysis()
{
    std::cout << "\n========== Example 4: Complete Analysis Pipeline ==========\n" << std::endl;

    // Create test image
    auto testImage = testing::TestUtils::createTestImage3D(100, 100, 50, 3);

    // Create callback manager
    auto callbackMgr = std::make_shared<events::CallbackManager>();

    // Register event callbacks
    int eventCount = 0;
    callbackMgr->registerFunction(
        [&eventCount](const events::ProcessingEventData& data) {
            eventCount++;
            std::cout << "Event: " << events::eventTypeToString(data.type);
            if (!data.message.empty())
            {
                std::cout << " - " << data.message;
            }
            std::cout << std::endl;
        }, "EventLogger");

    // Create complete processing pipeline
    auto pipeline = std::make_shared<pipeline::ProcessingPipeline>();
    pipeline->setCallbackManager(callbackMgr);

    // Add processing steps
    std::cout << "Building analysis pipeline..." << std::endl;

    // 1. Noise reduction
    auto noiseNode = std::make_shared<pipeline::NoiseReductionNode>();
    noiseNode->setMethod(filters::NoiseReductionMethod::AnisotropicDiffusion);
    noiseNode->setIterations(5);
    noiseNode->setParameter("timeStep", 0.0625);
    pipeline->addNode(noiseNode);

    // 2. Edge enhancement
    auto edgeNode = std::make_shared<pipeline::EdgeEnhancementNode>();
    edgeNode->setMethod(filters::EdgeDetectionMethod::LaplacianOfGaussian);
    edgeNode->setSigma(1.0);
    edgeNode->setStrength(0.3);
    pipeline->addNode(edgeNode);

    // 3. Morphology (optional, can be disabled)
    auto morphNode = std::make_shared<pipeline::MorphologyNode>();
    morphNode->setOperation(filters::MorphologyOperation::Gradient);
    morphNode->setKernelRadius(2.0);
    morphNode->setEnabled(true);
    pipeline->addNode(morphNode);

    // Execute pipeline
    std::cout << "\nExecuting complete analysis pipeline..." << std::endl;
    auto finalResult = pipeline->execute(testImage);

    if (finalResult)
    {
        std::cout << "\n✓ Complete analysis pipeline finished!" << std::endl;
        std::cout << "  Total nodes: " << pipeline->getNodeCount() << std::endl;
        std::cout << "  Total events: " << eventCount << std::endl;
        std::cout << "  Execution time: " << pipeline->getExecutionTime() << " ms" << std::endl;

        // Show intermediate results info
        for (size_t i = 0; i < pipeline->getNodeCount(); ++i)
        {
            auto intermediate = pipeline->getIntermediateResult(i);
            if (intermediate)
            {
                double min, max, mean, stddev;
                testing::TestUtils::getImageStatistics(intermediate, min, max, mean, stddev);
                std::cout << "  Node " << i << " (" << pipeline->getNode(i)->getName() << "): "
                         << "mean=" << mean << ", stddev=" << stddev << std::endl;
            }
        }

        // Get event statistics
        auto stats = callbackMgr->getEventStatistics();
        std::cout << "\nEvent Statistics:" << std::endl;
        for (const auto& [type, count] : stats)
        {
            std::cout << "  " << events::eventTypeToString(type) << ": " << count << std::endl;
        }
    }
    else
    {
        std::cout << "✗ Pipeline failed: " << pipeline->getLastError() << std::endl;
    }
}

/**
 * Example 5: Performance benchmarking
 */
void example_performance_benchmark()
{
    std::cout << "\n========== Example 5: Performance Benchmark ==========\n" << std::endl;

    std::vector<testing::TestResult> results;

    // Benchmark 1: Image creation
    results.push_back(testing::TestUtils::runTest("Create 100x100x50 image", []() {
        auto img = testing::TestUtils::createTestImage3D(100, 100, 50, 3);
        return testing::TestUtils::validateImageData(img);
    }));

    // Benchmark 2: Gaussian filter
    results.push_back(testing::TestUtils::runTest("Gaussian noise reduction", []() {
        auto img = testing::TestUtils::createTestImage3D(100, 100, 50, 3);
        filters::NoiseReductionFilter filter;
        filter.setInputImage(img);
        filter.setMethod(filters::NoiseReductionMethod::Gaussian);
        filter.setSigma(1.5);
        auto result = filter.execute();
        return testing::TestUtils::validateImageData(result);
    }));

    // Benchmark 3: Edge detection
    results.push_back(testing::TestUtils::runTest("Sobel edge detection", []() {
        auto img = testing::TestUtils::createTestImage3D(100, 100, 50, 3);
        filters::EdgeEnhancementFilter filter;
        filter.setInputImage(img);
        filter.setMethod(filters::EdgeDetectionMethod::Sobel);
        auto result = filter.execute();
        return testing::TestUtils::validateImageData(result);
    }));

    // Benchmark 4: Morphology
    results.push_back(testing::TestUtils::runTest("Morphological closing", []() {
        auto img = testing::TestUtils::createTestImage3D(100, 100, 50, 3);
        filters::MorphologyFilter filter;
        filter.setInputImage(img);
        filter.setOperation(filters::MorphologyOperation::Close);
        filter.setKernelRadius(3.0);
        auto result = filter.execute();
        return testing::TestUtils::validateImageData(result);
    }));

    // Benchmark 5: Complete pipeline
    results.push_back(testing::TestUtils::runTest("3-stage pipeline", []() {
        auto img = testing::TestUtils::createTestImage3D(100, 100, 50, 3);
        auto pipeline = std::make_shared<pipeline::ProcessingPipeline>();

        auto node1 = std::make_shared<pipeline::NoiseReductionNode>();
        node1->setMethod(filters::NoiseReductionMethod::Gaussian);
        pipeline->addNode(node1);

        auto node2 = std::make_shared<pipeline::EdgeEnhancementNode>();
        node2->setMethod(filters::EdgeDetectionMethod::Sobel);
        pipeline->addNode(node2);

        auto node3 = std::make_shared<pipeline::MorphologyNode>();
        node3->setOperation(filters::MorphologyOperation::Gradient);
        pipeline->addNode(node3);

        auto result = pipeline->execute(img);
        return testing::TestUtils::validateImageData(result);
    }));

    // Print benchmark results
    testing::TestUtils::printTestResults(results);
}

/**
 * Main function to run all examples
 */
int main(int argc, char* argv[])
{
    std::cout << "==================================================" << std::endl;
    std::cout << "    Isis DICOM Viewer - Integration Examples     " << std::endl;
    std::cout << "==================================================" << std::endl;

    try
    {
        example_preprocessing_pipeline();
        example_segmentation_workflow();
        example_registration_workflow();
        example_complete_analysis();
        example_performance_benchmark();

        std::cout << "\n==================================================" << std::endl;
        std::cout << "    All examples completed successfully!         " << std::endl;
        std::cout << "==================================================" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "\nException caught: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
