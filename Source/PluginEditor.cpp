/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SamplerAudioProcessorEditor::SamplerAudioProcessorEditor (SamplerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
    attackAttach(audioProcessor.apvts, "attack", attack),
    decayAttach(audioProcessor.apvts, "decay", decay),
    sustainAttach(audioProcessor.apvts, "sustain", sustain),
    releaseAttach(audioProcessor.apvts, "release", release)

{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    for(auto* slider : getSliders())
    {
        prepSlider(*slider);
        addAndMakeVisible(slider);
    }
    addAndMakeVisible(adsrPlot);
    setSize (700, 400);
}

SamplerAudioProcessorEditor::~SamplerAudioProcessorEditor()
{
}

//==============================================================================
void SamplerAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (juce::Colours::black);

    auto bounds = getLocalBounds();
    auto waveformBounds = bounds.removeFromTop(bounds.proportionOfHeight(0.6));
    
    if(showWaveform)
    {
        g.setColour(Colours::grey);

        Path waveformPath;
        
        auto waveformData = audioProcessor.getWaveformBuffer();
        auto ratio =  waveformData.getNumSamples() / waveformBounds.getWidth();
        auto buffer = waveformData.getReadPointer(0);
        
        for(int i = 0; i < waveformData.getNumSamples(); i+=ratio)
        {
            audioPoints.push_back(buffer[i]);
        }
        
        waveformPath.startNewSubPath(0, waveformBounds.getHeight()/2);
        
        for (int i = 0; i < audioPoints.size(); ++i)
        {
            auto point = jmap<float> (audioPoints[i], -1.0f, 1.0f, waveformBounds.getBottom(), waveformBounds.getY());
            waveformPath.lineTo(i, point);
        }
        
        g.strokePath(waveformPath, PathStrokeType(2.f, PathStrokeType::JointStyle::curved));
        
//        showWaveform = false;
        audioPoints.clear();
    }
    
}

void SamplerAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto bounds = getLocalBounds();
    auto sliderBounds = bounds.removeFromBottom(bounds.proportionOfHeight(0.3));
    adsrPlot.setBounds(bounds);
    attack.setBounds(sliderBounds.removeFromLeft(sliderBounds.proportionOfWidth(0.25)));
    decay.setBounds(sliderBounds.removeFromLeft(sliderBounds.proportionOfWidth(0.33)));
    sustain.setBounds(sliderBounds.removeFromLeft(sliderBounds.proportionOfWidth(0.5)));
    release.setBounds(sliderBounds);

}

bool SamplerAudioProcessorEditor::isInterestedInFileDrag(const StringArray &files)
{
    for(auto file : files)
    {
        if (file.contains(".wav") || file.contains(".mp3") || file.contains("aif")) {
            return true;
        }
    }
    
    return false;
}

void SamplerAudioProcessorEditor::filesDropped(const StringArray &files, int x, int y)
{
    for(auto file : files)
    {
        if (isInterestedInFileDrag(file)) {
            showWaveform = true;
            audioProcessor.loadSampleFileFromPath(file);
            repaint();
        }
    }
}

void SamplerAudioProcessorEditor::prepSlider(Slider& slider)
{
    slider.setSliderStyle(Slider::SliderStyle::RotaryHorizontalVerticalDrag);
}

std::vector<Slider*> SamplerAudioProcessorEditor::getSliders()
{
    return
    {
        &attack,
        &decay,
        &sustain,
        &release
    };
}
