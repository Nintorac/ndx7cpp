#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <atomic>
#include <memory>
#include <vector>
#include <queue>
#include <condition_variable>
#include <mutex>
#include <unordered_map>
#include <string>
#include <chrono>
#include <optional>
#include "NeuralModelWrapper.h"
#include "DX7Voice.h"

class ThreadedInferenceEngine : public juce::Thread
{
public:
    struct InferenceRequest
    {
        enum Type { RANDOM_VOICES, CUSTOM_VOICES, SINGLE_CUSTOM_VOICE };
        Type type;
        std::vector<float> latentVector;
        std::function<void(std::vector<DX7Voice>)> callback;
        std::function<void(DX7Voice)> singleCallback;
        
        InferenceRequest(Type t, std::function<void(std::vector<DX7Voice>)> cb)
            : type(t), callback(cb) {}
            
        InferenceRequest(Type t, const std::vector<float>& latent, std::function<void(std::vector<DX7Voice>)> cb)
            : type(t), latentVector(latent), callback(cb) {}
            
        InferenceRequest(Type t, const std::vector<float>& latent, std::function<void(DX7Voice)> cb)
            : type(t), latentVector(latent), singleCallback(cb) {}
    };
    
    ThreadedInferenceEngine();
    ~ThreadedInferenceEngine() override;
    
    // Thread management
    void startInferenceThread();
    void stopInferenceThread();
    
    // Request inference
    void requestRandomVoices(std::function<void(std::vector<DX7Voice>)> callback);
    void requestCustomVoices(const std::vector<float>& latentVector, std::function<void(std::vector<DX7Voice>)> callback);
    void requestSingleCustomVoice(const std::vector<float>& latentVector, std::function<void(DX7Voice)> callback);
    
    // Double buffer management
    bool hasBufferedRandomVoices() const;
    std::vector<DX7Voice> getBufferedRandomVoices();
    void preGenerateRandomVoices();
    
    // Custom voice caching
    bool hasCachedVoice(const std::vector<float>& latentVector) const;
    DX7Voice getCachedVoice(const std::vector<float>& latentVector) const;
    void requestCachedCustomVoice(const std::vector<float>& latentVector, std::function<void(DX7Voice)> callback);
    void preGenerateCustomVoice(const std::vector<float>& latentVector); // For debounced pre-generation
    
    // Thread safety
    bool isModelLoaded() const;
    
private:
    void run() override;
    void processInferenceRequests();
    void processInferenceRequest(const InferenceRequest& request);
    
    // Neural model wrapper
    std::unique_ptr<NeuralModelWrapper> neuralModel;
    
    // Thread synchronization
    std::mutex requestMutex;
    std::condition_variable requestCondition;
    std::queue<InferenceRequest> requestQueue;
    std::atomic<bool> shouldStop{false};
    
    // Double buffer for random voices
    std::mutex bufferMutex;
    std::vector<DX7Voice> bufferedRandomVoices;
    std::atomic<bool> hasBufferedVoices{false};
    std::atomic<bool> isGeneratingBuffer{false};
    
    // Custom voice caching
    static constexpr size_t MAX_CACHE_SIZE = 1000;
    mutable std::mutex cacheMutex;
    std::unordered_map<std::string, DX7Voice> voiceCache;
    std::queue<std::string> cacheOrder; // For LRU eviction
    std::atomic<bool> isPreGenerating{false}; // Prevent multiple inflight cache fills
    
    // Request scheduling for inflight handling
    std::mutex scheduleMutex;
    std::optional<InferenceRequest> scheduledRequest; // Next request to run after current completes
    
    // Helper methods
    std::string latentVectorToKey(const std::vector<float>& latentVector) const;
    void addToCache(const std::vector<float>& latentVector, const DX7Voice& voice);
    void evictOldestFromCache();
    
    // Model loading state
    std::atomic<bool> modelLoaded{false};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThreadedInferenceEngine)
};