/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Defines.h"

void ColemanJPFinalAReverbTaleAudioProcessorEditor::createSmallKnob(juce::Slider& slider,
                float x, float y, float interval, float skew, parameterMap paramNum) {
    auto& params = processor.getParameters();

    juce::AudioParameterFloat* audioParam = (juce::AudioParameterFloat*) params.getUnchecked(paramNum);
    slider.setBounds(x*DX, y*DY, 5*DX, 5*DY);
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
//    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, DX*3, DY*.6);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
//    slider.hideTextBox(true);
    slider.setRange(audioParam->range.start, audioParam->range.end, interval);
    slider.setSkewFactor(skew);
    slider.addListener(this);
    addAndMakeVisible(slider);
}

void ColemanJPFinalAReverbTaleAudioProcessorEditor::createWetDrySlider(juce::Slider& slider,
                float x, float y, parameterMap paramNum) {
    auto& params = processor.getParameters();
    
    juce::AudioParameterFloat* audioParam = (juce::AudioParameterFloat*) params.getUnchecked(paramNum);
    slider.setBounds(x*DX, y*DY, 2*DX, 9.25*DY);
    slider.setSliderStyle(juce::Slider::LinearVertical);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, DX*3, DY*.75);
    slider.setTextValueSuffix(" %");
    slider.setRange(audioParam->range.start, audioParam->range.end, 1);
    slider.addListener(this);
    addAndMakeVisible(slider);
}

//==============================================================================
ColemanJPFinalAReverbTaleAudioProcessorEditor::ColemanJPFinalAReverbTaleAudioProcessorEditor (ColemanJPFinalAReverbTaleAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (CONTAINER_WIDTH, CONTAINER_HEIGHT);
    
    createSmallKnob(sizeSlider, 13, 3, .01, 1, size);
    createSmallKnob(earlyReflSlider, 13, 9, .01, 1, earlyRefl);
    createSmallKnob(decayFreqSlider, 20, 3, .01, .3, decayFreq);
    createSmallKnob(dampingSlider, 20, 9, .01, 1, damping);
    
    createWetDrySlider(dryGainSlider, 27, 4.3, dryGain);
    createWetDrySlider(wetGainSlider, 31, 4.3, wetGain);
    
    // create delay knob
    auto& params = processor.getParameters();
    juce::AudioParameterFloat* decayParam = (juce::AudioParameterFloat*) params.getUnchecked(decayTime);
    decayTimeSlider.setBounds(2*DX, 5*DY, 8*DX, 9*DY);
    decayTimeSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    decayTimeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 3*DX, 1*DY);
    decayTimeSlider.setRange(decayParam->range.start, decayParam->range.end, .01);
    decayTimeSlider.setTextValueSuffix(" s");
    decayTimeSlider.setSkewFactor(.3);
    decayTimeSlider.addListener(this);
    addAndMakeVisible(decayTimeSlider);
    
    // create hold slider
    holdToggle.setBounds(4.8*DX, 15*DY, 2.6*DX, 1*DY);
    holdToggle.setToggleState(false, juce::dontSendNotification);
    holdToggle.addListener(this);
    addAndMakeVisible(holdToggle);
    
    // start refresh timer
    startTimerHz(120);
}

ColemanJPFinalAReverbTaleAudioProcessorEditor::~ColemanJPFinalAReverbTaleAudioProcessorEditor()
{
}

void ColemanJPFinalAReverbTaleAudioProcessorEditor::sliderValueChanged(juce::Slider *slider) {
    auto& params = processor.getParameters();

    for (auto& slider_param : sliderParamMap) {
        if (slider == slider_param.slider) {
            // set parameter from slider value
            juce::AudioParameterFloat* audioParam = (juce::AudioParameterFloat*)params.getUnchecked(slider_param.param);
            *audioParam = slider_param.slider->getValue();
            break;
        }
    }
}

void ColemanJPFinalAReverbTaleAudioProcessorEditor::timerCallback() {
    auto& params = processor.getParameters();

    for (auto& slider_param : sliderParamMap) {
        // set slider value from parameter
        juce::AudioParameterFloat* param = (juce::AudioParameterFloat*)params.getUnchecked(slider_param.param);
        slider_param.slider->setValue(param->get(), juce::dontSendNotification);
    }
}

// match right side to left when matchLR is toggled
void ColemanJPFinalAReverbTaleAudioProcessorEditor::buttonStateChanged(juce::Button *button) {
    auto& params = processor.getParameters();

    // set parameter from slider value
    juce::AudioParameterBool* holdParam = (juce::AudioParameterBool*)params.getUnchecked(hold);
    *holdParam = holdToggle.getToggleState();
}

void ColemanJPFinalAReverbTaleAudioProcessorEditor::buttonClicked(juce::Button *button) {
    
}

//==============================================================================
void ColemanJPFinalAReverbTaleAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    
    g.setColour(juce::Colours::grey);
    // vertical lines
//    for (int i = 1; i < CONTAINER_WIDTH/DX; i++) {
//        g.drawLine(i*DX, 0, i*DX, CONTAINER_HEIGHT);
//    }
//    // horizontal lines
//    for (int i = 1; i < CONTAINER_HEIGHT/DY; i++) {
//        g.drawLine(0, i*DY, CONTAINER_WIDTH, i*DY);
//    }

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
//    g.drawFittedText ("Hel`333333lo World!", getLocalBounds(), juce::Justification::centred, 1);
    
    float up = 0.2;
    
    g.drawText("Early Reflections", 12*DX, 1*DY, 7*DX, 1*DY, juce::Justification::centred);
    g.drawText("Size", 13*DX, (8-up)*DY, 5*DX, 1*DY, juce::Justification::centred);
    g.drawText("Amount", 13*DX, (14-up)*DY, 5*DX, 1*DY, juce::Justification::centred);
    
    g.drawText("Damping", 19*DX, 1*DY, 7*DX, 1*DY, juce::Justification::centred);
    g.drawText("Frequency", 20*DX, (8-up)*DY, 5*DX, 1*DY, juce::Justification::centred);
    g.drawText("Amount", 20*DX, (14-up)*DY, 5*DX, 1*DY, juce::Justification::centred);
    
    
    g.drawText("Output", 27*DX, 3*DY, 6*DX, 1*DY, juce::Justification::centred);
    g.drawText("Dry", 27*DX, (14-up)*DY, 2*DX, 1*DY, juce::Justification::centred);
    g.drawText("Wet", 31*DX, (14-up)*DY, 2*DX, 1*DY, juce::Justification::centred);
    
    
    g.setColour(juce::Colours::grey);
    g.drawRect(12.5*DX, 2.5*DY, 6*DX, 13*DY);
    g.drawRect(19.5*DX, 2.5*DY, 6*DX, 13*DY);
    
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(24.0f));
    g.drawText("A REVERB TALE", 1*DX, .5*DY, 8*DX, 2*DY, juce::Justification::centredLeft);
    
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(20.0f));
    g.drawText("Decay Time", 2*DX, 3*DY, 8*DX, 2*DY, juce::Justification::centred);
    
}

void ColemanJPFinalAReverbTaleAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
