/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ADSREnv.h"

//==============================================================================
/**
*/
class SamplerAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                     public FileDragAndDropTarget
{
public:
    SamplerAudioProcessorEditor (SamplerAudioProcessor&);
    ~SamplerAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    bool isInterestedInFileDrag(const StringArray &files) override;
    void filesDropped(const StringArray &files, int x, int y) override;
    
    void prepSlider(Slider& slider);
    

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SamplerAudioProcessor& audioProcessor;
    std::vector<float> audioPoints;
    std::vector<Slider*> getSliders();
    
    Slider attack, decay, sustain, release;
    juce::AudioProcessorValueTreeState::SliderAttachment attackAttach, decayAttach, sustainAttach, releaseAttach;
    
    ADSREditor adsrPlot;
    
    bool showWaveform {false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerAudioProcessorEditor)
};
