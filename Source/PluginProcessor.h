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
    juce::AudioParameterFloat* densityParam;
    juce::AudioParameterFloat* sizeParam; // controls early reflections (and other?)
    
    std::vector<stk::DelayA> delays;
    std::vector<stk::BiQuad> all_passes;
    std::vector<stk::BiQuad> high_shelfs;
    
    stk::DelayA early_delayL;
    stk::DelayA early_delayR;
    
    float tapsL[N_TAPS];
    float tapsR[N_TAPS];
    
    float b_coeffs [N_DELAYS];
    float c_coeffs [N_DELAYS];
    float g_coeffs [N_DELAYS];
    float M [N_DELAYS];
    
    std::vector<stk::BiQuad> output_allpasses;    

    juce::dsp::Matrix<float> Q = juce::dsp::Matrix<float>(N_DELAYS, N_DELAYS);
    
    float dryGain;
    float fs;
    
    void calcAlgorithmParams();
};

// APF structure from Will Pirkle's "Delaying All-Pass Reverberator"
class DelayAPF {
public:
    DelayAPF() {
        g = 0.7;
        delayLine.setDelay(1001);
    }
    DelayAPF(float delayLength, float g) {
        this->g = g;
        delayLine.setDelay(delayLength);
    }
    
    void setDelayLength(float delayLength) {
        delayLine.setDelay(delayLength);
    }
    void setG(float g) {
        this->g = g;
    }
    float tick(float input){
        return g*(delayLine.tick(input) - input);
    }
private:
    stk::DelayA delayLine;
    float g;
};
