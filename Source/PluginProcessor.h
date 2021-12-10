/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "StkLite-4.6.1/DelayA.h"
#include "StkLite-4.6.1/BiQuad.h"
#include "Defines.h"

//==============================================================================
class DelayAPF;
/**
*/
class ColemanJPFinalAReverbTaleAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    ColemanJPFinalAReverbTaleAudioProcessor();
    ~ColemanJPFinalAReverbTaleAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ColemanJPFinalAReverbTaleAudioProcessor)
    
    juce::AudioParameterFloat* decayTimeParam;
    juce::AudioParameterFloat* dryGainParam;
    juce::AudioParameterFloat* wetGainParam;
    juce::AudioParameterFloat* sizeParam; // controls early reflections (and other?)
    juce::AudioParameterFloat* dampingParam;
    juce::AudioParameterFloat* decayFreqParam;
    juce::AudioParameterFloat* earlyReflParam;
    juce::AudioParameterBool*  holdParam;
    
    std::vector<stk::DelayA> delays;
    std::vector<DelayAPF> all_passes;
    std::vector<stk::BiQuad> high_shelfs;
    
    stk::DelayA early_delayL;
    stk::DelayA early_delayR;
    
    float tapsL[N_TAPS];
    float tapsR[N_TAPS];
    
    float b_coeffs [N_DELAYS];
    float c_coeffs [N_DELAYS];
    float g_coeffs [N_DELAYS];
    float M [N_DELAYS];

    juce::dsp::Matrix<float> Q = juce::dsp::Matrix<float>(N_DELAYS, N_DELAYS);
    
    float dryGain;
    float earlyGain;
    float fs;
    
    bool holding;
    
//    // Leaky integrator for room size change
//    float sizeB0;
//    float sizeTau = 35.0/1000.0; // sec
//    float size_value = 1;
    
    void calcAlgorithmParams();
};

// APF structure based on Will Pirkle's "Delaying All-Pass Reverberator"
class DelayAPF {
public:
    DelayAPF() {
        g = 0.7;
        delayLine.setDelay(1001);
    }
    DelayAPF(float maxDelay, float g) {
        this->g = g;
        delayLine.setMaximumDelay(maxDelay);
    }
    
    void setDelayLength(float delayLength) {
        delayLine.setDelay(delayLength);
    }
    float tick(float input){
        float w = input + delayLine.nextOut()*g;
        float output = -g*w + delayLine.nextOut();
        delayLine.tick(w);
        return output;
    }
    float g;
private:
    stk::DelayA delayLine;
};
