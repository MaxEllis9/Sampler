/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SamplerAudioProcessor::SamplerAudioProcessor()
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
    midiRange.setRange(0, 128, true);
    
    formatManager.registerBasicFormats();
        
    for(int i=0; i < numVoices; i++)
    {
        sampler.addVoice(new juce::SamplerVoice());
    }
}

SamplerAudioProcessor::~SamplerAudioProcessor()
{
    formatReader = nullptr;
}

//==============================================================================
const juce::String SamplerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SamplerAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SamplerAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SamplerAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SamplerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SamplerAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SamplerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SamplerAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SamplerAudioProcessor::getProgramName (int index)
{
    return {};
}

void SamplerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SamplerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    sampler.setCurrentPlaybackSampleRate(sampleRate);
    
//    updateADSRValues();
}

void SamplerAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SamplerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void SamplerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    updateADSRValues();
    applyNewADSRValues();

    sampler.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

}

//==============================================================================
bool SamplerAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SamplerAudioProcessor::createEditor()
{
    return new SamplerAudioProcessorEditor (*this);
}

//==============================================================================
void SamplerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SamplerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

AudioProcessorValueTreeState::ParameterLayout SamplerAudioProcessor::createParameterLayout()
{
    AudioProcessorValueTreeState::ParameterLayout layout;
    
    layout.add(std::make_unique<AudioParameterFloat>("attack",
                                                     "attack",
                                                     NormalisableRange<float>(0.f, 20.f, 0.01f, 1.f),
                                                     0.f));
    layout.add(std::make_unique<AudioParameterFloat>("decay",
                                                     "decay",
                                                     NormalisableRange<float>(0.f, 60.f, 0.01f, 1.f),
                                                     0.6f));
    layout.add(std::make_unique<AudioParameterFloat>("sustain",
                                                     "sustain",
                                                     NormalisableRange<float>(-70.f, 0.f, 0.3f, 1.f),
                                                     0.f));
    layout.add(std::make_unique<AudioParameterFloat>("release",
                                                     "release",
                                                     NormalisableRange<float>(0.f, 60.f, 0.01f, 1.f),
                                                     0.05f));

    
    return layout;
    
}

void SamplerAudioProcessor::loadSampleFileFromPath(const String &path)
{
    sampler.clearSounds();
    auto sampleFile = File(path);
    formatReader = formatManager.createReaderFor(sampleFile);
    
    auto sampleLength = static_cast<int>(formatReader->lengthInSamples);
    
    waveformBuffer.setSize(1, sampleLength);
    formatReader->read(&waveformBuffer, 0, sampleLength, 0, true, false);
    
    sampler.addSound(new SamplerSound("sample", *formatReader, midiRange, 60, 0.1, 0.1, 10));
    
}

void SamplerAudioProcessor::updateADSRValues()
{
    adsrParams.attack = *apvts.getRawParameterValue("attack");
    adsrParams.decay = *apvts.getRawParameterValue("decay");
    adsrParams.sustain = *apvts.getRawParameterValue("sustain");
    adsrParams.release = *apvts.getRawParameterValue("release");
}

void SamplerAudioProcessor::applyNewADSRValues()
{
    for(int i = 0; i < sampler.getNumSounds(); ++i)
    {
        if(auto sound = dynamic_cast<SamplerSound*>(sampler.getSound(i).get()))
        {
            sound->setEnvelopeParameters(adsrParams);
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SamplerAudioProcessor();
}
