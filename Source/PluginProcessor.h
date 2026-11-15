#pragma once
#include <JuceHeader.h>
#include "Dynamics.h"
class MV2Processor : public juce::AudioProcessor
{
public:
    MV2Processor();
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "MV2 Recreation"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    juce::UndoManager undo;
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float> lowMeter {0}, highMeter {0};
    std::atomic<float> pendingPeakL {0}, pendingPeakR {0}, heldPeakL {0}, heldPeakR {0};
    std::atomic<float> meterL {-100}, meterR {-100}, inputMeter {-100};
    std::atomic<bool> clipL {false}, clipR {false};
private:
    Dynamics dynamics;
    std::atomic<float>* low;
    std::atomic<float>* high;
    std::atomic<float>* output;
};
