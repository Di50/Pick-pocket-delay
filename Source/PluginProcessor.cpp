/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DelayAudioProcessor::DelayAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    delayTimeLEFT = new AudioParameterFloat("delayTimeLEFT", "Left Delay (seconds)", 0.0f, 1.0f, 0.3f);
    delayTimeRIGHT = new AudioParameterFloat("delayTimeRIGHT", "Right Delay (seconds)", 0.0f, 1.0f, 0.3f);
    feedback = new AudioParameterFloat ("feedback", "Feedback", 0.0f, 0.75f, 0.0f); //
    delayMix = new AudioParameterFloat("delayMix", "Delay Volume", 0.01f, 0.98f, 0.5f);
    addParameter(delayTimeLEFT);
    addParameter(delayTimeRIGHT);
    addParameter(feedback);
    addParameter(delayMix);
}

DelayAudioProcessor::~DelayAudioProcessor()
{
}

//==============================================================================
const String DelayAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DelayAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DelayAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DelayAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DelayAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DelayAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DelayAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DelayAudioProcessor::setCurrentProgram (int index)
{
}

const String DelayAudioProcessor::getProgramName (int index)
{
    return {};
}

void DelayAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void DelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    delayBufferLength = (int)(2 * sampleRate); // sample rate cast to integer, now doubled (2 * sampleRate)
    delayBuffer.setSize(2, delayBufferLength);
    delayBuffer.clear();
    readIndexLEFT = (int)(writeIndex - (delayTimeLEFT->get() * getSampleRate()) + delayBufferLength) % delayBufferLength; 
    readIndexRIGHT = (int)(writeIndex - (delayTimeRIGHT->get() * getSampleRate()) + delayBufferLength) % delayBufferLength; 
}

void DelayAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
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

void DelayAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples()); // this is Juce's standard method of clearing channels in case they contain garbage...

    if (buffer.getNumChannels() < 2) // if the bog standard buffer doesn't have two channels, the plug-in won't work, so call a retour
        return;

    int tempReadIndexL = 0; 
    int tempReadIndexR = 0;
    int tempWriteIndex = 0;
    float tempDelayTimeL = 0.3f;
    float tempDelayTimeR = 0.3f;

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelDataL = buffer.getWritePointer(0);    // channel 0 - left?
        auto* channelDataR = buffer.getWritePointer(1);    // channel 1 - the other one
        auto* delayDataL = delayBuffer.getWritePointer(0); // same here..
        auto* delayDataR = delayBuffer.getWritePointer(1); // and here ...
       
        tempReadIndexL = readIndexLEFT; 
        tempReadIndexR = readIndexRIGHT;
        tempWriteIndex = writeIndex;

        if (tempDelayTimeL != delayTimeLEFT->get())
            readIndexLEFT = (int)(writeIndex - (delayTimeLEFT->get() * getSampleRate()) + delayBufferLength) % delayBufferLength;

        if (tempDelayTimeR != delayTimeRIGHT->get())
            readIndexRIGHT = (int)(writeIndex - (delayTimeRIGHT->get() * getSampleRate()) + delayBufferLength) % delayBufferLength;

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            auto inL = channelDataL[i]; // it's the same index [i], but we've already split the channel data
            auto inR = channelDataR[i]; // so left and right index hold different samples 

            auto outL = inL + (delayMix->get() * delayDataL[tempReadIndexL]); 
            auto outR = inR + (delayMix->get() * delayDataR[tempReadIndexR]);

            // ping-pong effect by swapping left and right delay
            //                    same input + opposite side delay data
            delayDataL[tempWriteIndex] = inL + (delayDataR[tempReadIndexR] * feedback->get());
            delayDataR[tempWriteIndex] = inR + (delayDataL[tempReadIndexL] * feedback->get());


            if (++tempReadIndexL >= delayBufferLength) //
                tempReadIndexL = 0;
            if (++tempReadIndexR >= delayBufferLength)
                tempReadIndexR = 0;
            if (++tempWriteIndex >= delayBufferLength)
                tempWriteIndex = 0;

            channelDataL[i] = outL; //
            channelDataR[i] = outR;//

        }
    }
    readIndexLEFT = tempReadIndexL; //
    readIndexRIGHT = tempReadIndexR;
    writeIndex = tempWriteIndex;
    tempDelayTimeL = delayTimeLEFT->get();//
    tempDelayTimeR = delayTimeRIGHT->get();
}

//==============================================================================
bool DelayAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* DelayAudioProcessor::createEditor()
{
    return new GenericAudioProcessorEditor (*this);
}

//==============================================================================
void DelayAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void DelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DelayAudioProcessor();
}
