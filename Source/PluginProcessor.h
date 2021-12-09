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
    juce::AudioParameterFloat* dampingParam;
    juce::AudioParameterFloat* decayFreqParam;
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
    
    std::vector<DelayAPF> output_allpasses;

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
    DelayAPF(float maxDelay) {
        delayLine.setMaximumDelay(maxDelay);
    }
    
    void autoSetFromDelay(float delayLength, float T60, float fs, bool switchPolarity=false) {
        delayLine.setDelay(delayLength);
//        g = pow(10, -3.0*delayLength/(T60*fs));
        g = .55;
        if (switchPolarity) g *= -1;
    }
    void setDelayLength(float delayLength) {
        delayLine.setDelay(delayLength);
    }
    float tick(float input){
        float w = input + delayLine.nextOut()*g;
        float output = -g*w + delayLine.nextOut();
        delayLine.tick(w);
        return output;
        
//        return -g*(input + g*delayLine.tick(input));
//        return g*(delayLine.tick(input) - input);
    }
    float g;
private:
    stk::DelayA delayLine;
};
