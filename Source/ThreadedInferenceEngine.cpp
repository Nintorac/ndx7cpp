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
            
            case InferenceRequest::SINGLE_CUSTOM_VOICE:
            {
                std::cout << "ThreadedInferenceEngine: Processing single custom voice request" << std::endl;
                voices = neuralModel->generateVoices(request.latentVector);
                
                // Call single voice callback with first voice (or nullopt if empty)
                if (request.singleCallback)
                {
                    std::optional<DX7Voice> voiceOpt = voices.empty() ? std::nullopt : std::make_optional(voices[0]);
                    juce::MessageManager::callAsync([callback = request.singleCallback, voiceOpt]() {
                        callback(voiceOpt);
                    });
                }
                return;
            }
        }
        
        // Call multi-voice callback on main thread
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

void ThreadedInferenceEngine::requestSingleCustomVoice(const std::vector<float>& latentVector, std::function<void(std::optional<DX7Voice>)> callback)
{
    std::unique_lock<std::mutex> lock(requestMutex);
    requestQueue.emplace(InferenceRequest::SINGLE_CUSTOM_VOICE, latentVector, callback);
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

std::string ThreadedInferenceEngine::latentVectorToKey(const std::vector<float>& latentVector) const
{
    std::string key;
    key.reserve(latentVector.size() * 8); // Rough estimate for string size
    
    for (float value : latentVector)
    {
        // Round to 3 decimal places to create reasonable cache keys
        int rounded = static_cast<int>(value * 1000.0f);
        key += std::to_string(rounded) + ",";
    }
    
    return key;
}

void ThreadedInferenceEngine::addToCache(const std::vector<float>& latentVector, const DX7Voice& voice)
{
    std::unique_lock<std::mutex> lock(cacheMutex);
    
    std::string key = latentVectorToKey(latentVector);
    
    // If cache is full, evict oldest entry
    if (voiceCache.size() >= MAX_CACHE_SIZE)
    {
        evictOldestFromCache();
    }
    
    // Add new entry
    voiceCache.emplace(key, voice);
    cacheOrder.push(key);
    
    std::cout << "ThreadedInferenceEngine: Added voice to cache (size: " << voiceCache.size() << ")" << std::endl;
}

void ThreadedInferenceEngine::evictOldestFromCache()
{
    if (!cacheOrder.empty())
    {
        std::string oldestKey = cacheOrder.front();
        cacheOrder.pop();
        voiceCache.erase(oldestKey);
        std::cout << "ThreadedInferenceEngine: Evicted oldest cache entry" << std::endl;
    }
}

bool ThreadedInferenceEngine::hasCachedVoice(const std::vector<float>& latentVector) const
{
    std::unique_lock<std::mutex> lock(cacheMutex);
    std::string key = latentVectorToKey(latentVector);
    return voiceCache.find(key) != voiceCache.end();
}

std::optional<DX7Voice> ThreadedInferenceEngine::getCachedVoice(const std::vector<float>& latentVector) const
{
    std::unique_lock<std::mutex> lock(cacheMutex);
    std::string key = latentVectorToKey(latentVector);
    auto it = voiceCache.find(key);
    
    if (it != voiceCache.end())
    {
        std::cout << "ThreadedInferenceEngine: Cache hit for custom voice" << std::endl;
        return it->second;
    }
    
    std::cout << "ThreadedInferenceEngine: Voice not found in cache, returning null" << std::endl;
    return std::nullopt;
}

void ThreadedInferenceEngine::requestCachedCustomVoice(const std::vector<float>& latentVector, std::function<void(std::optional<DX7Voice>)> callback)
{
    // Check cache first
    if (hasCachedVoice(latentVector))
    {
        std::optional<DX7Voice> cachedVoice = getCachedVoice(latentVector);
        juce::MessageManager::callAsync([callback, cachedVoice]() {
            callback(cachedVoice);
        });
        return;
    }
    
    // Not in cache, generate and cache
    requestSingleCustomVoice(latentVector, [this, latentVector, callback](std::optional<DX7Voice> voiceOpt) {
        // Add to cache if voice was generated successfully
        if (voiceOpt.has_value())
        {
            addToCache(latentVector, voiceOpt.value());
        }
        
        // Call original callback
        callback(voiceOpt);
    });
}

void ThreadedInferenceEngine::preGenerateCustomVoice(const std::vector<float>& latentVector)
{
    // Only generate if not already cached
    if (hasCachedVoice(latentVector))
    {
        return;
    }
    
    std::unique_lock<std::mutex> lock(scheduleMutex);
    
    // If no request is currently inflight, start one immediately
    if (!isPreGenerating.load())
    {
        isPreGenerating.store(true);
        lock.unlock(); // Release lock before making request
        
        std::cout << "ThreadedInferenceEngine: Pre-generating custom voice for cache" << std::endl;
        requestSingleCustomVoice(latentVector, [this, latentVector](std::optional<DX7Voice> voiceOpt) {
            // Add to cache for future use if voice was generated
            if (voiceOpt.has_value())
            {
                addToCache(latentVector, voiceOpt.value());
            }
            
            // Check if there's a scheduled request to process
            std::unique_lock<std::mutex> scheduleLock(scheduleMutex);
            if (scheduledRequest.has_value())
            {
                auto nextRequest = std::move(scheduledRequest.value());
                scheduledRequest.reset();
                scheduleLock.unlock();
                
                std::cout << "ThreadedInferenceEngine: Processing scheduled request" << std::endl;
                // Process the scheduled request
                requestSingleCustomVoice(nextRequest.latentVector, [this, nextRequest](std::optional<DX7Voice> scheduledVoiceOpt) {
                    if (scheduledVoiceOpt.has_value())
                    {
                        addToCache(nextRequest.latentVector, scheduledVoiceOpt.value());
                    }
                    isPreGenerating.store(false); // Allow new pre-generation requests
                    std::cout << "ThreadedInferenceEngine: Scheduled request completed" << std::endl;
                });
            }
            else
            {
                isPreGenerating.store(false); // Allow new pre-generation requests
                std::cout << "ThreadedInferenceEngine: Pre-generated voice cached" << std::endl;
            }
        });
    }
    else
    {
        // Request is inflight, schedule this one (overwriting any previous scheduled request)
        if (scheduledRequest.has_value()) {
            std::cout << "ThreadedInferenceEngine: Overwriting previously scheduled request" << std::endl;
        } else {
            std::cout << "ThreadedInferenceEngine: Scheduling request to run after current completes" << std::endl;
        }
        scheduledRequest = InferenceRequest(InferenceRequest::SINGLE_CUSTOM_VOICE, latentVector, [](std::optional<DX7Voice>){});
    }
}

bool ThreadedInferenceEngine::isModelLoaded() const
{
    return modelLoaded.load();
}