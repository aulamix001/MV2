#pragma once
#include <JuceHeader.h>

// All controls have the same dB-per-pixel response, independent of their ranges.
// The JUCE base class owns begin/end drag notifications for host automation.
class ResponsiveSlider : public juce::Slider
{
public:
    std::function<void(double)> completeGesture;
    void mouseDown(const juce::MouseEvent& event) override
    {
        if(isShowing())grabKeyboardFocus();
        dragValue = getValue(); previousY = event.position.y;
        juce::Slider::mouseDown(event);
    }
    void mouseDrag(const juce::MouseEvent& event) override
    {
        if(!event.mods.isLeftButtonDown()) return;
        const double sensitivity = fine(event.mods) ? 0.025 : 0.25;
        dragValue = juce::jlimit(getMinimum(),getMaximum(),dragValue+(previousY-event.position.y)*sensitivity);
        previousY = event.position.y;
        setValue(dragValue,juce::sendNotificationSync);
    }
    void mouseWheelMove(const juce::MouseEvent& event,const juce::MouseWheelDetails& wheel) override
    {
        if(event.mods.isAnyMouseButtonDown() || !completeGesture) return;
        const double delta = (std::abs(wheel.deltaY)>=std::abs(wheel.deltaX) ? wheel.deltaY : -wheel.deltaX)
                             * (wheel.isReversed ? -1.0 : 1.0);
        wheelRemainder += delta*(fine(event.mods) ? 1.0 : 5.0);
        const double step=getInterval();
        const double change=std::round(wheelRemainder/step)*step;
        if(std::abs(change)<step*0.5) return;
        wheelRemainder -= change;
        completeGesture(getValue()+change);
    }
    bool keyPressed(const juce::KeyPress& key) override
    {
        const auto code=key.getKeyCode();
        if(completeGesture && (code==juce::KeyPress::upKey || code==juce::KeyPress::rightKey
                              || code==juce::KeyPress::downKey || code==juce::KeyPress::leftKey))
        {
            completeGesture(getValue()+((code==juce::KeyPress::upKey || code==juce::KeyPress::rightKey) ? 0.1 : -0.1));
            return true;
        }
        return juce::Slider::keyPressed(key);
    }
private:
    static bool fine(juce::ModifierKeys mods) {return mods.isShiftDown() || mods.isCtrlDown() || mods.isCommandDown();}
    double dragValue=0, previousY=0, wheelRemainder=0;
};
