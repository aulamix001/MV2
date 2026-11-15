#pragma once
#include "PluginProcessor.h"
#include "ResponsiveSlider.h"
class ParameterReadout : public juce::Label
{
public:
    juce::Slider* linked = nullptr;
    void mouseDown(const juce::MouseEvent& e) override
    {
        juce::Label::mouseDown(e);
        if(linked && e.getNumberOfClicks()==1 && !isBeingEdited()) linked->mouseDown(e.getEventRelativeTo(linked));
    }
    void mouseDrag(const juce::MouseEvent& e) override
    { if(linked && !isBeingEdited()) linked->mouseDrag(e.getEventRelativeTo(linked)); }
    void mouseUp(const juce::MouseEvent& e) override
    { if(linked) linked->mouseUp(e.getEventRelativeTo(linked)); }
};
class MV2Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit MV2Editor(MV2Processor&);
    ~MV2Editor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
private:
    void timerCallback() override;
    void positionReadouts();
    void setParameter(const juce::String&, float);
    void toolbar(int);
    MV2Processor& processor;
    juce::Image panel, backdrop;
    void prepareBackdrop();
    ResponsiveSlider low, high, output;
    ParameterReadout lowText, highText, outputText;
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> lowAttachment, highAttachment, outputAttachment;
    std::vector<std::unique_ptr<juce::TextButton>> buttons;
    juce::TextButton sizeButton {"200%"};
    juce::ComponentBoundsConstrainer constraints;
    juce::ResizableCornerComponent corner {this, &constraints};
    juce::TooltipWindow tooltip {this, 500};
    std::unique_ptr<juce::FileChooser> chooser;
    juce::ValueTree stateA, stateB;
    bool activeB = false;
    float shownL = -100, shownR = -100, heldL = -100, heldR = -100;
    float shownLow = 0, shownHigh = 0;
    double lastMeterTime = 0;
    juce::Rectangle<int> rect(float x,float y,float w,float h) const;
    void configure(juce::Slider&,juce::Label&,const juce::String&);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MV2Editor)
};
