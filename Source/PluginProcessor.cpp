/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Defines.h"
#include "Mu45FilterCalc/Mu45FilterCalc.h"

//==============================================================================
ColemanJPFinalAReverbTaleAudioProcessor::ColemanJPFinalAReverbTaleAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    addParameter(decayTimeParam = new juce::AudioParameterFloat("decay",
                                                                "Decay time (s)",
                                                                0.5f,
                                                                60.0f,
                                                                2.0f));
    addParameter(dryGainParam = new juce::AudioParameterFloat("dryGain",
                                                                "Dry gain %",
                                                                0.0f,
                                                                100.0f,
                                                                70.0f));
    addParameter(wetGainParam = new juce::AudioParameterFloat("wetGain",
                                                                "Wet gain %",
                                                                0.0f,
                                                                100.0f,
                                                                30.0f));
    addParameter(densityParam = new juce::AudioParameterFloat("density",
                                                                "Density",
                                                                0.0f,
                                                                100.0f,
                                                                40.0f));
    addParameter(sizeParam = new juce::AudioParameterFloat("size",
                                                                "Size",
                                                                0.0f,
                                                                100.0f,
                                                                40.0f));
}

ColemanJPFinalAReverbTaleAudioProcessor::~ColemanJPFinalAReverbTaleAudioProcessor()
{
}

//==============================================================================
const juce::String ColemanJPFinalAReverbTaleAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ColemanJPFinalAReverbTaleAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ColemanJPFinalAReverbTaleAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ColemanJPFinalAReverbTaleAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ColemanJPFinalAReverbTaleAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ColemanJPFinalAReverbTaleAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ColemanJPFinalAReverbTaleAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ColemanJPFinalAReverbTaleAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ColemanJPFinalAReverbTaleAudioProcessor::getProgramName (int index)
{
    return {};
}

void ColemanJPFinalAReverbTaleAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

// from https://www.geeksforgeeks.org/nearest-prime-less-given-number-n/
int nearest_prime(int n)
{
 
    // All prime numbers are odd except two
    if (n & 1)
        n -= 2;
    else
        n--;
 
    int i, j;
    for (i = n; i >= 2; i -= 2) {
        if (i % 2 == 0)
            continue;
        for (j = 3; j <= sqrt(i); j += 2) {
            if (i % j == 0)
                break;
        }
        if (j > sqrt(i))
            return i;
    }
 
    // It will only be executed when n is 3
    return 2;
}


//==============================================================================
void ColemanJPFinalAReverbTaleAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    fs = (float) sampleRate;
    
//        float f = 0.5;
//        // Hadamard matrix 4x4
//        float data[N_DELAYS][N_DELAYS] = {{f, f, f, f},
//                        {f, -f, f, -f},
//                        {f, f, -f, -f},
//                        {f, -f, -f, f}};

    
    float f = 1/sqrt(2);
    // Modified Hadamard matrix 4x4
    float data[N_DELAYS][N_DELAYS] = {{0, f, f, 0},
                    {-f, 0, 0, -f},
                    {f, 0, 0, -f},
                    {0, f, -f, 0}};
    
    
    for (int i = 0; i < N_DELAYS; i++) {
        for (int j = 0; j < N_DELAYS; j++) {
            Q(i,j) = data[i][j];
        }
    }
    
    float M_times[] = {.0307, .034, .0413, .0471};
    for (int i = 0; i < N_DELAYS; i++) M[i] = nearest_prime(M_times[i]*fs);
//    float M_vals[] = {2341, 2411, 2521, 2621}; // spaced out prime numbers
//    float M_vals[] = {727, 881, 1033, 1291};
//    for (int i = 0; i < N_DELAYS; i++) M[i] = M_vals[i];

    
//    int M[] = {2341, 2687, 3329, 3797}; // spaced out prime numbers
    for (int del = 0; del < N_DELAYS; del++) {
        delays.push_back(stk::DelayA(M[del], M[del]));
        all_passes.push_back(DelayAPF(2000)); // MAGIC NUMBER
        high_shelfs.push_back(stk::BiQuad());
    }
    
    output_allpasses.push_back(DelayAPF(2000)); // MAGIC NUMBER
        
}

void ColemanJPFinalAReverbTaleAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ColemanJPFinalAReverbTaleAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

int control = 0;
void ColemanJPFinalAReverbTaleAudioProcessor::calcAlgorithmParams() {
    dryGain = dryGainParam->get()/100.0;
    
    float T60 = decayTimeParam->get();
    
    float total_wet_gain = wetGainParam->get()/100.0;
    
    
//    float primes[] = {2,3,5,7};
//    float
//
//    for (int i = 0; i < N_DELAYS; i++) {
//
//    }
    
    for (int i = 0; i < N_DELAYS; i++) {
        float sign = (-2*(i%2)+1);
        b_coeffs[i] = sign*total_wet_gain/sqrt(2); // even to each delay line
//        b_coeffs[i] = 1/sqrt(N_DELAYS);
        
//        c_coeffs[i] = total_wet_gain/N_DELAYS;
        c_coeffs[i] = 1; // bc b's are divided, this can be maxed
        
//        g_coeffs[i] = pow(10,-60.0*M[i]/(T60*fs)); // determine decay of signals
        g_coeffs[i] = pow(10,-3.0*M[i]/(T60*fs)); // determine decay of signals
        
    }
    
    if (control <= 0) {
        DBG("----------");
        for (int i = 0; i < N_DELAYS; i++) {
            DBG(std::to_string(i) << ": b - " << std::to_string(b_coeffs[i])
                << ", c - " << std::to_string(c_coeffs[i])
                << ", g - " << std::to_string(g_coeffs[i])
                << ", T60 - " << std::to_string(T60));
        }
        control = 200;
    }
    control--;

    float lp_fc = 3000; // 3 kHz, low pass corner frequency
//    float ap_delays[] = {225, 556, 441, 341}; // all pass corner frequencies
//    float ap_delays[] = {1109, 1123, 887, 953}; // all pass corner frequencies
    float ap_times[] = {1.07, 2.359, 2.901, 4.73}; // in msec

                                            // numbers from "Freeverb" digaram by JOS
    
    float lp_coeffs[5];
    Mu45FilterCalc::calcCoeffsHighShelf(lp_coeffs, lp_fc, -20, fs);
    
//    float ap_coeffs[5];
    
    for (int i = 0; i < N_DELAYS; i++) {
//        Mu45FilterCalc::calcCoeffsAPF(ap_coeffs, ap_fcs[i], 0.3, fs);
//        all_passes[i].setCoefficients(ap_coeffs[0], ap_coeffs[1], ap_coeffs[2], ap_coeffs[3], ap_coeffs[4]);
//        delays[i].setDelay(ap_delays[i]);
//        delays[i].setGain(-.7);
//        all_passes[i].autoSetFromDelay(ap_delays[i], T60, fs);
        all_passes[i].autoSetFromDelay(ap_times[i]*fs/1000.0, T60, fs);
        DBG("all pass g" << std::to_string(i) << ": " << std::to_string(all_passes[i].g));
        
        high_shelfs[i].setCoefficients(lp_coeffs[0], lp_coeffs[1], lp_coeffs[2], lp_coeffs[3], lp_coeffs[4]);
    }
    
//    Mu45FilterCalc::calcCoeffsAPF(ap_coeffs, 501, 0.3 , fs);
//    output_allpasses[0].setCoefficients(ap_coeffs[0], ap_coeffs[1], ap_coeffs[2], ap_coeffs[3], ap_coeffs[4]);
//    output_allpasses[0].setDelayLength(301);
//    output_allpasses[0].setG(0.7);
    output_allpasses[0].autoSetFromDelay(301, T60, fs);

}

void ColemanJPFinalAReverbTaleAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    calcAlgorithmParams();
    
    auto* leftChannelData = buffer.getWritePointer(0);
    auto* rightChannelData = buffer.getWritePointer(1);
    
    float x_n[N_DELAYS]; // inputs to delay lines
    auto delay_out = juce::dsp::Matrix<float>(N_DELAYS, 1);
    auto matrix_out = juce::dsp::Matrix<float>(N_DELAYS, 1);
    float fbGain_out[N_DELAYS];
    float ap_out[N_DELAYS];
    float lp_out[N_DELAYS];
    
    
    float leftWetOutput, rightWetOutput, leftInput, rightInput;

    for (int samp = 0; samp < buffer.getNumSamples(); samp++) {
        leftInput = leftChannelData[samp];
        rightInput = rightChannelData[samp];
        
        leftWetOutput = 0;
        rightWetOutput = 0;
        
        // get feedback values
        for (int i = 0; i < N_DELAYS; i++){
            delay_out(i,0) = delays[i].nextOut();
            delay_out(i,0) = high_shelfs[i].tick(delay_out(i,0));
//            delay_out(i,0) = all_passes[i].tick(delay_out(i,0));

            leftWetOutput += delay_out(i,0)*c_coeffs[i];
            rightWetOutput += delay_out(i,0)*c_coeffs[i];
        }
        
        

//        leftWetOutput = output_allpasses[0].tick(leftWetOutput);
//        rightWetOutput = output_allpasses[0].tick(rightWetOutput);
        
        leftChannelData[samp] = leftWetOutput + dryGain*leftInput;
        rightChannelData[samp] = rightWetOutput + dryGain*rightInput;
        
        matrix_out = Q * delay_out; // matrix multiply

        for (int i = 0; i < N_DELAYS; i++) {
            // feedback
            fbGain_out[i] = matrix_out(i,0)*g_coeffs[i];
//            ap_out[i] = all_passes[i].tick(fbGain_out[i]);
//            lp_out[i] = low_passes[i].tick(ap_out[i]);

            // combine feedback and input
//            x_n[i] = input*b_coeffs[i] + lp_out[i];
            x_n[i] = leftInput*b_coeffs[i] + fbGain_out[i];

            // send to delay line
            delays[i].tick(x_n[i]);
        }

    }
}

//==============================================================================
bool ColemanJPFinalAReverbTaleAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ColemanJPFinalAReverbTaleAudioProcessor::createEditor()
{
    return new ColemanJPFinalAReverbTaleAudioProcessorEditor (*this);
}

//==============================================================================
void ColemanJPFinalAReverbTaleAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void ColemanJPFinalAReverbTaleAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ColemanJPFinalAReverbTaleAudioProcessor();
}
