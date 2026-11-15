#include "PluginProcessor.h"
#include "PluginEditor.h"
MV2Processor::MV2Processor()
 : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                  .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
   parameters(*this, &undo, "MV2State", {
      std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"low",1}, "Low Level", juce::NormalisableRange<float>(0,48,0.1f),0,"dB"),
      std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"high",1}, "High Level", juce::NormalisableRange<float>(-48,0,0.1f),0,"dB"),
      std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"output",1}, "Output Gain", juce::NormalisableRange<float>(-48,18,0.1f),0,"dB") })
{
    low = parameters.getRawParameterValue("low");
    high = parameters.getRawParameterValue("high");
    output = parameters.getRawParameterValue("output");
}
void MV2Processor::prepareToPlay(double rate, int)
{
    dynamics.prepare(rate,output->load());pendingPeakL.store(0);pendingPeakR.store(0);heldPeakL.store(0);heldPeakR.store(0);
    meterL.store(-160);meterR.store(-160);lowMeter.store(0);highMeter.store(0);clipL.store(false);clipR.store(false);
}
bool MV2Processor::isBusesLayoutSupported(const BusesLayout& b) const
{
    return b.getMainInputChannelSet() == b.getMainOutputChannelSet()
        && (b.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
            || b.getMainOutputChannelSet() == juce::AudioChannelSet::stereo());
}
void MV2Processor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = buffer.getNumChannels();
    const float lo = low->load(), hi = high->load(), out = output->load();
    float peaks[2] {0,0}, inputPeak = 0;
    for (int i=0; i<buffer.getNumSamples(); ++i)
    {
        float linked = 0;
        for (int c=0; c<channels; ++c) linked = std::max(linked, std::abs(buffer.getSample(c,i)));
        inputPeak = std::max(inputPeak, linked);
        const float gain = dynamics.tick(linked, lo, hi, out);
        for (int c=0; c<channels; ++c)
        {
            const auto sample = buffer.getSample(c,i) * gain;
            buffer.setSample(c,i,sample);
            peaks[c] = std::max(peaks[c], std::abs(sample));
        }
    }
    lowMeter.store(dynamics.liftDb); highMeter.store(dynamics.reductionDb);
    inputMeter.store(Dynamics::db(inputPeak));
    meterL.store(Dynamics::db(peaks[0]));
    meterR.store(Dynamics::db(channels == 1 ? peaks[0] : peaks[1]));
    auto retain=[](std::atomic<float>& target,float value) {
        float previous=target.load(std::memory_order_relaxed);
        while(previous<value && !target.compare_exchange_weak(previous,value,std::memory_order_relaxed)) {}
    };
    const float rightPeak=channels==1 ? peaks[0] : peaks[1];
    retain(pendingPeakL,peaks[0]);retain(pendingPeakR,rightPeak);
    retain(heldPeakL,peaks[0]);retain(heldPeakR,rightPeak);
    if(peaks[0] >= 1) clipL.store(true);
    if((channels == 1 ? peaks[0] : peaks[1]) >= 1) clipR.store(true);
}
void MV2Processor::getStateInformation(juce::MemoryBlock& data)
{
    auto xml = parameters.copyState().createXml(); copyXmlToBinary(*xml,data);
}
void MV2Processor::setStateInformation(const void* data,int size)
{
    if (auto xml = getXmlFromBinary(data,size))
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessorEditor* MV2Processor::createEditor() { return new MV2Editor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MV2Processor(); }
