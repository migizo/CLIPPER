/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "Editor.h"

//==============================================================================

CLIPPERAudioProcessor::CLIPPERAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#else 
    :
#endif
        // processor, undoManager, valueTreeType, parameterLayout
        parameters(*this, nullptr, juce::Identifier(JucePlugin_Name),
        {
            std::make_unique<juce::AudioParameterFloat>(TRANS("BOOST"), TRANS("BOOST"), juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f),
            std::make_unique<juce::AudioParameterFloat>(TRANS("VOLUME"), TRANS("VOLUME"), juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f)
        })
{

    boostParameter = parameters.getRawParameterValue("BOOST");
    volumeParameter = parameters.getRawParameterValue("VOLUME");


   // addParameter(new Parameter("boost", "%"));
   // addParameter(new Parameter("volume"));

    
}

CLIPPERAudioProcessor::~CLIPPERAudioProcessor()
{
}

//==============================================================================
const juce::String CLIPPERAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool CLIPPERAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool CLIPPERAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool CLIPPERAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double CLIPPERAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int CLIPPERAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int CLIPPERAudioProcessor::getCurrentProgram()
{
    return 0;
}

void CLIPPERAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String CLIPPERAudioProcessor::getProgramName (int index)
{
    return {};
}

void CLIPPERAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void CLIPPERAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need

}

void CLIPPERAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool CLIPPERAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void CLIPPERAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{

    static bool lastIsPlaying;
    juce::AudioPlayHead::CurrentPositionInfo info;
    getPlayHead()->getCurrentPosition(info);
    bool isPlaying = info.isPlaying;
    
    //float boost = getParameters().getReference(0)->getValue();
    //float volume = getParameters().getReference(1)->getValue();
    float boost = *boostParameter;
    float volume = *volumeParameter;

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    const int bufferSize = buffer.getNumSamples();

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        for (int i = 0; i < bufferSize; i++) {
            
            if (lastIsPlaying != isPlaying) {
                // play
                if (isPlaying) {
                    animator.start(100, 0.0, 1.0);
                }
                // stop
                else {
                    animator.start(100, 1.0, 0.0);
                }
            }
            animator.update();
    
            // boostをthresholdとして
            double original = channelData[i];
            double inBuf = original;
            double power = 0.25;
            double ab = juce::jmap(abs(inBuf), 0.0, 1.0 - pow(boost/100.0, power), 0.0, 0.99); // TODO: clamp
            ab = juce::jmin(1.0, juce::jmax(0.0, ab));
            inBuf = ab * (inBuf > 0.0 ? 1.0 : -1.0);
            inBuf *= volume;
            inBuf = (1.0 - animator.getValue()) * original + animator.getValue() * inBuf;
            channelData[i] = inBuf;
        }
    }
    
    lastIsPlaying = isPlaying;
}

//==============================================================================
bool CLIPPERAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* CLIPPERAudioProcessor::createEditor()
{
    return new Editor (*this);
}

//==============================================================================
void CLIPPERAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    
    // load from xml
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
        
    /*
    // save(store to memory) the parameter
    for (int i = 0; i < getParameters().size(); i++) {
        juce::MemoryOutputStream(destData, true).writeFloat(getParameters().getReference(i)->getValue());
    }
    */
}

void CLIPPERAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    
    // save to xml
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
 
    if (xmlState.get() != nullptr) 
    {
        if (xmlState->hasTagName (parameters.state.getType())) 
        {
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
        }
    }
                
    /*
    for (int i = 0; i < getParameters().size(); i++) {
        getParameters().getReference(i)->setValue(juce::MemoryInputStream(data, static_cast<size_t> (sizeInBytes), false).readFloat());
    }
    */
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CLIPPERAudioProcessor();
}
