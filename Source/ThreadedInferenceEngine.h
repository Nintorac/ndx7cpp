#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <atomic>
#include <memory>
#include <vector>
#include <queue>
#include <condition_variable>
#include <mutex>
#include "NeuralModelWrapper.h"
#include "DX7Voice.h"

class ThreadedInferenceEngine : public juce::Thread
{
public:
    struct InferenceRequest
    {
        enum Type { RANDOM_VOICES, CUSTOM_VOICES };
        Type type;
        std::vector<float> latentVector;
        std::function<void(std::vector<DX7Voice>)> callback;
        
        InferenceRequest(Type t, std::function<void(std::vector<DX7Voice>)> cb)
            : type(t), callback(cb) {}
            
        InferenceRequest(Type t, const std::vector<float>& latent, std::function<void(std::vector<DX7Voice>)> cb)
            : type(t), latentVector(latent), callback(cb) {}
    };
    
    ThreadedInferenceEngine();
    ~ThreadedInferenceEngine() override;
    
    // Thread management
    void startInferenceThread();
    void stopInferenceThread();
    
    // Request inference
    void requestRandomVoices(std::function<void(std::vector<DX7Voice>)> callback);
    void requestCustomVoices(const std::vector<float>& latentVector, std::function<void(std::vector<DX7Voice>)> callback);
    
    // Double buffer management
    bool hasBufferedRandomVoices() const;
    std::vector<DX7Voice> getBufferedRandomVoices();
    void preGenerateRandomVoices();
    
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
    
    // Model loading state
    std::atomic<bool> modelLoaded{false};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThreadedInferenceEngine)
};