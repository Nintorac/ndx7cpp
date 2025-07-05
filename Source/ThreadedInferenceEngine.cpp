#include "ThreadedInferenceEngine.h"
#include <iostream>

ThreadedInferenceEngine::ThreadedInferenceEngine()
    : juce::Thread("InferenceEngine")
{
    neuralModel = std::make_unique<NeuralModelWrapper>();
}

ThreadedInferenceEngine::~ThreadedInferenceEngine()
{
    stopInferenceThread();
}

void ThreadedInferenceEngine::startInferenceThread()
{
    if (!isThreadRunning())
    {
        shouldStop.store(false);
        startThread(juce::Thread::Priority::high);
        std::cout << "ThreadedInferenceEngine: Started inference thread" << std::endl;
        
        // Don't pre-generate buffer here - let the thread handle it after model loads
    }
}

void ThreadedInferenceEngine::stopInferenceThread()
{
    if (isThreadRunning())
    {
        shouldStop.store(true);
        
        // Wake up the thread
        {
            std::unique_lock<std::mutex> lock(requestMutex);
            requestCondition.notify_all();
        }
        
        // Wait for thread to finish
        stopThread(2000); // 2 second timeout
        std::cout << "ThreadedInferenceEngine: Stopped inference thread" << std::endl;
    }
}

void ThreadedInferenceEngine::run()
{
    std::cout << "ThreadedInferenceEngine: Thread started, loading model..." << std::endl;
    
    // Load model in background thread
    if (neuralModel->loadModelFromFile())
    {
        modelLoaded.store(true);
        std::cout << "ThreadedInferenceEngine: Model loaded successfully" << std::endl;
        
        // Now that model is loaded, generate initial buffer
        preGenerateRandomVoices();
    }
    else
    {
        std::cout << "ThreadedInferenceEngine: Failed to load model" << std::endl;
        return;
    }
    
    // Main inference loop
    while (!shouldStop.load())
    {
        processInferenceRequests();
        
        // Wait for new requests or stop signal
        std::unique_lock<std::mutex> lock(requestMutex);
        requestCondition.wait_for(lock, std::chrono::milliseconds(100), [this]() {
            return !requestQueue.empty() || shouldStop.load();
        });
    }
    
    std::cout << "ThreadedInferenceEngine: Thread exiting" << std::endl;
}

void ThreadedInferenceEngine::processInferenceRequests()
{
    std::queue<InferenceRequest> localQueue;
    
    // Copy requests to local queue to minimize lock time
    {
        std::unique_lock<std::mutex> lock(requestMutex);
        localQueue.swap(requestQueue);
    }
    
    // Process all requests
    while (!localQueue.empty() && !shouldStop.load())
    {
        processInferenceRequest(localQueue.front());
        localQueue.pop();
    }
}

void ThreadedInferenceEngine::processInferenceRequest(const InferenceRequest& request)
{
    if (!modelLoaded.load())
    {
        std::cout << "ThreadedInferenceEngine: Model not loaded, skipping request" << std::endl;
        return;
    }
    
    std::vector<DX7Voice> voices;
    
    try
    {
        switch (request.type)
        {
            case InferenceRequest::RANDOM_VOICES:
            {
                std::cout << "ThreadedInferenceEngine: Processing random voices request" << std::endl;
                voices = neuralModel->generateMultipleRandomVoices();
                
                // Update buffer with new voices for next time
                if (!voices.empty())
                {
                    std::unique_lock<std::mutex> lock(bufferMutex);
                    bufferedRandomVoices = voices;
                    hasBufferedVoices.store(true);
                    isGeneratingBuffer.store(false);
                }
                break;
            }
            
            case InferenceRequest::CUSTOM_VOICES:
            {
                std::cout << "ThreadedInferenceEngine: Processing custom voices request" << std::endl;
                voices = neuralModel->generateVoices(request.latentVector);
                break;
            }
        }
        
        // Call callback on main thread
        if (request.callback && !voices.empty())
        {
            juce::MessageManager::callAsync([callback = request.callback, voices]() {
                callback(voices);
            });
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "ThreadedInferenceEngine: Error processing request: " << e.what() << std::endl;
    }
}

void ThreadedInferenceEngine::requestRandomVoices(std::function<void(std::vector<DX7Voice>)> callback)
{
    std::unique_lock<std::mutex> lock(requestMutex);
    requestQueue.emplace(InferenceRequest::RANDOM_VOICES, callback);
    requestCondition.notify_one();
}

void ThreadedInferenceEngine::requestCustomVoices(const std::vector<float>& latentVector, std::function<void(std::vector<DX7Voice>)> callback)
{
    std::unique_lock<std::mutex> lock(requestMutex);
    requestQueue.emplace(InferenceRequest::CUSTOM_VOICES, latentVector, callback);
    requestCondition.notify_one();
}

bool ThreadedInferenceEngine::hasBufferedRandomVoices() const
{
    return hasBufferedVoices.load();
}

std::vector<DX7Voice> ThreadedInferenceEngine::getBufferedRandomVoices()
{
    std::unique_lock<std::mutex> lock(bufferMutex);
    if (hasBufferedVoices.load())
    {
        auto voices = bufferedRandomVoices;
        hasBufferedVoices.store(false);
        
        // Trigger generation of new buffer
        preGenerateRandomVoices();
        
        return voices;
    }
    
    return {};
}

void ThreadedInferenceEngine::preGenerateRandomVoices()
{
    if (!isGeneratingBuffer.load())
    {
        isGeneratingBuffer.store(true);
        
        // Request new random voices for the buffer
        requestRandomVoices([this](std::vector<DX7Voice> voices) {
            // Callback is handled in processInferenceRequest
            std::cout << "ThreadedInferenceEngine: Buffer regenerated with " << voices.size() << " voices" << std::endl;
        });
    }
}

bool ThreadedInferenceEngine::isModelLoaded() const
{
    return modelLoaded.load();
}