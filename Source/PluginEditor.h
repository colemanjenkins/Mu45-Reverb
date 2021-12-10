/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Defines.h"

//==============================================================================
/**
*/
class ColemanJPFinalAReverbTaleAudioProcessorEditor  : public juce::AudioProcessorEditor,
public juce::Slider::Listener, public juce::Button::Listener,
public juce::Timer
{
public:
    ColemanJPFinalAReverbTaleAudioProcessorEditor (ColemanJPFinalAReverbTaleAudioProcessor&);
    ~ColemanJPFinalAReverbTaleAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    void sliderValueChanged(juce::Slider *slider) override;
    void timerCallback() override;
    
    // for toggle
    void buttonStateChanged(juce::Button* button) override;
    void buttonClicked(juce::Button* button) override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ColemanJPFinalAReverbTaleAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ColemanJPFinalAReverbTaleAudioProcessorEditor)
    
    juce::Slider decayTimeSlider;
    juce::Slider dryGainSlider;
    juce::Slider wetGainSlider;
    juce::Slider sizeSlider;
    juce::Slider dampingSlider;
    juce::Slider decayFreqSlider;
    juce::Slider earlyReflSlider;
    juce::ToggleButton holdToggle {"Hold"};
    
    enum parameterMap {
        decayTime,
        dryGain,
        wetGain,
        size,
        damping,
        decayFreq,
        earlyRefl,
        hold
    };
    struct SliderToParam {
        juce::Slider* slider;
        parameterMap param;
    };
    std::vector<SliderToParam> sliderParamMap {
        {&decayTimeSlider, decayTime},
        {&dryGainSlider, dryGain},
        {&wetGainSlider, wetGain},
        {&sizeSlider, size},
        {&dampingSlider, damping},
        {&decayFreqSlider, decayFreq},
        {&earlyReflSlider, earlyRefl}
    };
    
    void createSmallKnob(juce::Slider&,float x, float y, float interval, float skew, parameterMap param);
    void createWetDrySlider(juce::Slider&, float x, float y, parameterMap param);
    
};
