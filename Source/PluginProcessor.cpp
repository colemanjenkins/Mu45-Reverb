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
    addParameter(sizeParam = new juce::AudioParameterFloat("size",
                                                                "Size",
                                                                0.0f,
                                                                100.0f,
                                                                40.0f));
    addParameter(dampingParam = new juce::AudioParameterFloat("damping",
                                                              "Damping",
                                                              0,
                                                              40.0f,
                                                              16.0f));
    addParameter(decayFreqParam = new juce::AudioParameterFloat("decayFreq",
                                                              "Decay Frequency",
                                                              400.0f,
                                                              9000.0f,
                                                              3000.0f));
    addParameter(earlyReflParam = new juce::AudioParameterFloat("earlyRefl",
                                                              "Early Reflections",
                                                              0.0f,
                                                              100.0f,
                                                              50.0f));
    addParameter(holdParam = new juce::AudioParameterBool("hold",
                                                          "Hold",
                                                          false));
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
    size_value = sizeParam->get();
       
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
    for (int i = 0; i < N_DELAYS; i++) M[i] = nearest_prime(M_times[i]*1.5*fs);
    
    for (int del = 0; del < N_DELAYS; del++) {
        delays.push_back(stk::DelayA(M[del], M[del]));
        all_passes.push_back(DelayAPF(2000, 0.55));
        high_shelfs.push_back(stk::BiQuad());
    }
    
    early_delayL.setMaximumDelay(fs);
    early_delayR.setMaximumDelay(fs);
    
    float ap_times[] = {1.07, 2.359, 2.901, 4.73}; // in msec
    for (int i = 0; i < N_DELAYS; i++) {
        all_passes[i].setDelayLength(ap_times[i]*fs/1000.0);
    }
    
    sizeB0 = 1 - exp(-1.0/(sizeTau*fs));
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

void ColemanJPFinalAReverbTaleAudioProcessor::calcAlgorithmParams() {
    dryGain = dryGainParam->get()/100.0;
    float T60 = decayTimeParam->get();
    float total_wet_gain = wetGainParam->get()/100.0;
    holding = holdParam->get();
    earlyGain = wetGainParam->get()/100.0*earlyReflParam->get()/100.0;
    
    // calculate gain coefficients
    float sign;
    for (int i = 0; i < N_DELAYS; i++) {
        sign = (-2*(i%2)+1);
        b_coeffs[i] = sign*total_wet_gain/sqrt(2); // even to each delay line
        c_coeffs[i] = 1; // bc b's are divided, this can be maxed
        g_coeffs[i] = pow(10,-3.0*M[i]/(T60*fs)); // determine decay of signals
        
        if (holding) { // set gain to 1 to continue loop if holding
            g_coeffs[i] = 1;
        }
    }
    
    // set parameters for high shelf filters
    float hs_coeffs[5];
    Mu45FilterCalc::calcCoeffsHighShelf(hs_coeffs, decayFreqParam->get(), -dampingParam->get(), fs);
    
    for (int i = 0; i < N_DELAYS; i++) {
        high_shelfs[i].setCoefficients(hs_coeffs[0], hs_coeffs[1], hs_coeffs[2], hs_coeffs[3], hs_coeffs[4]);
    }
        
//    // magic numbers that worked when changing early delay lengths :)
////    size_value += sizeB0*(sizeParam->get() - size_value);
//    float room_size = size_value*10 + 1;
////    float room_size = sizeParam->get()*10 + 1;
//    float time_to_front = room_size/25.0/343.0;
//
//    float tap_valsL[] = {time_to_front*fs, time_to_front*fs, (time_to_front*1.2f + .0001f)*fs,
//        time_to_front*1.87f*fs, (time_to_front*2.33f + .0003f)*fs, time_to_front*2.77f*fs};
//    float tap_valsR[] = {time_to_front*fs, (time_to_front + .0002f)*fs, time_to_front*1.2f*fs,
//        (time_to_front*1.87f + .00025f)*fs, time_to_front*2.33f*fs, time_to_front*2.77f*fs};
//    for (int i = 0; i < N_TAPS; i++) {
//        tapsL[i] = tap_valsL[i];
//        tapsR[i] = tap_valsR[i];
//    }
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
    
    auto pre_output = juce::dsp::Matrix<float>(N_DELAYS, 1);
    auto matrix_out = juce::dsp::Matrix<float>(N_DELAYS, 1);
    float leftLateOutput, rightLateOutput, leftInput, rightInput,
        leftEarlyDelayed, rightEarlyDelayed, delay_out, x_n;
    bool left_out;
    
    float leftInterp, rightInterp;

    for (int samp = 0; samp < buffer.getNumSamples(); samp++) {
        // set variables before processing
        leftInput = leftChannelData[samp];
        rightInput = rightChannelData[samp];
        leftLateOutput = 0;
        rightLateOutput = 0;
        leftEarlyDelayed = 0;
        rightEarlyDelayed = 0;
        
        
        // if not holding, add early delays to FDN
        if (!holding) {
            // take in new input to the early delays
            early_delayL.tick(leftInput);
            early_delayR.tick(rightInput);

            leftEarlyDelayed += leftInput;
            rightEarlyDelayed += rightInput;
            
            // set early reflection values
            float room_size = (size_value*10 + 1)/25.0/343.0;

            float tapsL[N_TAPS] = {room_size*fs, room_size*fs, (room_size*1.2f + .0001f)*fs,
                room_size*1.87f*fs, (room_size*2.33f + .0003f)*fs, room_size*2.77f*fs};
            float tapsR[N_TAPS] = {room_size*fs, (room_size + .0002f)*fs, room_size*1.2f*fs,
                (room_size*1.87f + .00025f)*fs, room_size*2.33f*fs, room_size*2.77f*fs};
            
            for (int i = 0; i < N_TAPS; i++) {
                leftInterp = tapsL[i] - floor(tapsL[i]);
                rightInterp = tapsR[i] - floor(tapsR[i]);
                leftEarlyDelayed += early_delayL.tapOut(floor(tapsL[i]))*leftInterp +
                                    early_delayL.tapOut(ceil(tapsL[i]))*(1-leftInterp);
                rightEarlyDelayed += early_delayR.tapOut(floor(tapsR[i]))*rightInterp +
                                    early_delayR.tapOut(ceil(tapsR[i]))*(1-rightInterp);
            }
            leftEarlyDelayed /= sqrt(6);
            rightEarlyDelayed /= sqrt(6);
        } else {
            early_delayL.tick(0);
            early_delayR.tick(0);
        }
        // update size env follower
        size_value += sizeB0*(sizeParam->get() - size_value);
        
        // get feedback values for FDN output
        left_out = true;
        for (int i = 0; i < N_DELAYS; i++){
            delay_out = delays[i].nextOut();
            delay_out = high_shelfs[i].tick(delay_out);
            delay_out = all_passes[i].tick(delay_out);
            
            pre_output(i,0) = delay_out;

            if (left_out) {
                leftLateOutput += delay_out*c_coeffs[i];
                left_out = false;
            } else {
                rightLateOutput += delay_out*c_coeffs[i];
                left_out = true;
            }
        }

        // assign outputs
        leftChannelData[samp] = dryGain*leftInput + earlyGain*leftEarlyDelayed + leftLateOutput;
        rightChannelData[samp] = dryGain*rightInput + earlyGain*rightEarlyDelayed + rightLateOutput;
        
        // determine feedback inputs to delay
        matrix_out = Q * pre_output; // matrix multiply feedback matrix

        for (int i = 0; i < N_DELAYS; i++) {
            if (i < N_DELAYS/2) {
                x_n = leftEarlyDelayed*b_coeffs[i] + matrix_out(i,0)*g_coeffs[i];
            } else {
                x_n = rightEarlyDelayed*b_coeffs[i] + matrix_out(i,0)*g_coeffs[i];
            }
            delays[i].tick(x_n);
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
    juce::XmlElement xml ("Parameters");
    for (int i = 0; i < getParameters().size() - 1; ++i) // - 1 to ignore holdParam (an AudioParameterBool)
    {
        juce::AudioParameterFloat* param = (juce::AudioParameterFloat*)getParameters().getUnchecked(i);
        juce::XmlElement* paramElement = new juce::XmlElement ("parameter" + juce::String(std::to_string(i)));
        paramElement->setAttribute ("value", param->get());
        xml.addChildElement (paramElement);
    }
    copyXmlToBinary (xml, destData);
}

void ColemanJPFinalAReverbTaleAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState->hasTagName ("Parameters")) // read Parameters tag
    {
        juce::AudioParameterFloat* param;
        for (auto* element : xmlState->getChildIterator()) // loop through the saved parameter values and update them
        {
            int paramNum = std::stoi(element->getTagName().substring(9).toStdString()); // chops off beginnging "parameter"
            param = (juce::AudioParameterFloat*) getParameters().getUnchecked(paramNum);
            *param = element->getDoubleAttribute("value"); // set parameter value
        }
    }
    
    // update size_value right away so it doesn't go through the env follower
    size_value = sizeParam->get();
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ColemanJPFinalAReverbTaleAudioProcessor();
}
