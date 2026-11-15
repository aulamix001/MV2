#include "PluginEditor.h"
#include <iostream>
#include <stdexcept>
void require(bool b,const char* m) {if(!b)throw std::runtime_error(m);}
void parameter(MV2Processor& p,const char* name,float value) {
    auto* x=p.parameters.getParameter(name);x->setValueNotifyingHost(x->convertTo0to1(value));
}
struct GestureListener : juce::AudioProcessorParameter::Listener
{
    int starts=0,ends=0;
    bool active=false,unbalanced=false;
    void parameterValueChanged(int,float) override {if(!active)unbalanced=true;}
    void parameterGestureChanged(int,bool starting) override
    {
        if(starting){if(active)unbalanced=true;active=true;++starts;}
        else {if(!active)unbalanced=true;active=false;++ends;}
    }
};
juce::MouseEvent eventFor(juce::Component& target,juce::Point<float> position,int modifiers)
{
    const auto now=juce::Time::getCurrentTime();
    return {juce::Desktop::getInstance().getMainMouseSource(),position,juce::ModifierKeys(modifiers),
            1.0f,0,0,0,0,&target,&target,now,{40,100},now,1,true};
}
void checkControls(MV2Processor& processor,juce::AudioProcessorEditor& editor)
{
    int checked=0;
    for(auto* child : editor.getChildren())
    {
        auto* slider=dynamic_cast<ResponsiveSlider*>(child);
        if(!slider)continue;
        const char* id=slider->getName()=="Low Level" ? "low" : (slider->getName()=="High Level" ? "high" : "output");
        const double middle=(slider->getMinimum()+slider->getMaximum())*0.5;
        for(int width : {318,636,954})
        {
            editor.setSize(width,width*307/318);
            slider->setValue(middle,juce::sendNotificationSync);
            GestureListener listener;auto* parameter=processor.parameters.getParameter(id);parameter->addListener(&listener);
            auto down=eventFor(*slider,{40,100},juce::ModifierKeys::leftButtonModifier);
            slider->mouseDown(down);
            require(std::abs(slider->getValue()-middle)<0.01,"Click must not jump");
            slider->mouseDrag(eventFor(*slider,{40,96},juce::ModifierKeys::leftButtonModifier));
            require(std::abs(slider->getValue()-middle-1.0)<0.01,"4 px drag must move 1 dB at every UI size");
            slider->mouseDrag(eventFor(*slider,{40,56},juce::ModifierKeys::leftButtonModifier|juce::ModifierKeys::shiftModifier));
            slider->mouseUp(eventFor(*slider,{40,56},0));
            require(std::abs(slider->getValue()-middle-2.0)<0.01,"Shift drag must be 10x finer without jumps");
            juce::MouseWheelDetails wheel{};wheel.deltaY=0.1f;
            slider->mouseWheelMove(eventFor(*slider,{40,56},0),wheel);
            require(std::abs(slider->getValue()-middle-2.5)<0.01,"Wheel must move 0.5 dB per notch");
            slider->mouseWheelMove(eventFor(*slider,{40,56},juce::ModifierKeys::ctrlModifier),wheel);
            require(std::abs(slider->getValue()-middle-2.6)<0.01,"Ctrl wheel must move 0.1 dB");
            slider->keyPressed(juce::KeyPress(juce::KeyPress::downKey));
            require(std::abs(processor.parameters.getRawParameterValue(id)->load()-middle-2.5)<0.01,"Keyboard must immediately update DSP by 0.1 dB");
            parameter->removeListener(&listener);
            require(listener.starts==listener.ends && listener.starts==4 && !listener.unbalanced,"Host automation gestures must balance for drag/wheel/keyboard");
        }
        ++checked;
    }
    require(checked==3,"All three faders must be checked");
    for(auto* child : editor.getChildren())
    {
        auto* label=dynamic_cast<juce::Label*>(child);if(!label)continue;
        const auto name=label->getName();
        const char* id=name=="Low Level dB" ? "low" : (name=="High Level dB" ? "high" : "output");
        auto* p=processor.parameters.getParameter(id);
        label->setText("9999",juce::sendNotificationSync);
        require(std::abs(processor.parameters.getRawParameterValue(id)->load()-p->convertFrom0to1(1))<0.01f,"Numeric values must clamp to upper limit");
        label->setText("-9999",juce::sendNotificationSync);
        require(std::abs(processor.parameters.getRawParameterValue(id)->load()-p->convertFrom0to1(0))<0.01f,"Numeric values must clamp to lower limit");
        const float previous=processor.parameters.getRawParameterValue(id)->load();
        label->setText("nan",juce::sendNotificationSync);
        require(processor.parameters.getRawParameterValue(id)->load()==previous,"Nonfinite numeric input must be rejected");
    }
}
void checkMetering()
{
    {
        MV2Processor startup;parameter(startup,"output",-18);startup.prepareToPlay(48000,64);
        juce::AudioBuffer<float> first(2,64);juce::MidiBuffer events;
        for(int c=0;c<2;++c)for(int i=0;i<64;++i)first.setSample(c,i,0.5f);
        startup.processBlock(first,events);
        require(std::abs(Dynamics::db(first.getSample(0,0)/0.5f)+18)<0.01f,"Saved output gain must apply from the first sample, without startup burst");
    }
    MV2Processor processor;processor.prepareToPlay(48000,512);
    juce::AudioBuffer<float> buffer(2,512);juce::MidiBuffer midi;buffer.clear();
    buffer.setSample(0,127,0.5f);buffer.setSample(1,127,0.25f);processor.processBlock(buffer,midi);
    for(int i=0;i<8;++i){buffer.clear();processor.processBlock(buffer,midi);}
    require(processor.meterL.load()<-100,"Latest block is silent");
    require(std::abs(processor.pendingPeakL.exchange(0)-0.5f)<1e-6f,"Meter must not miss short left transient between UI frames");
    require(std::abs(processor.pendingPeakR.exchange(0)-0.25f)<1e-6f,"Meter must not miss short right transient between UI frames");
    require(processor.pendingPeakL.load()==0,"Reading pending peak must clear its window");
    require(std::abs(processor.heldPeakL.load()-0.5f)<1e-6f,"Peak hold must survive UI reads and silent blocks");
    parameter(processor,"low",24);
    for(int block=0;block<150;++block)
    {
        for(int c=0;c<2;++c)for(int i=0;i<512;++i)buffer.setSample(c,i,0.001f);
        processor.processBlock(buffer,midi);
    }
    const auto actualGain=Dynamics::db(buffer.getSample(0,511)/0.001f);
    require(actualGain>15,"Low control must lift the quiet test signal");
    require(std::abs(actualGain-processor.lowMeter.load()+processor.highMeter.load())<0.02f,"Gain meters must show applied, smoothed audio gain");
    processor.prepareToPlay(48000,512);
    require(processor.heldPeakL.load()==0 && !processor.clipL.load(),"Prepare must reset meter/clip history");
}
int main() {
    juce::ScopedJuceInitialiser_GUI gui;
    try {
        checkMetering();
        MV2Processor p;
        p.prepareToPlay(48000,512);
        juce::AudioBuffer<float> b(2,512);juce::MidiBuffer midi;
        for(int c=0;c<2;++c)for(int i=0;i<512;++i)b.setSample(c,i,0.1f);
        p.processBlock(b,midi);
        require(std::abs(b.getSample(0,500)-0.1f)<1e-6f,"Default must be unity");
        parameter(p,"low",30);parameter(p,"high",-20);parameter(p,"output",-6);
        juce::MemoryBlock state;p.getStateInformation(state);
        MV2Processor restored;restored.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
        require(restored.parameters.getRawParameterValue("low")->load()==30,"Restore low");
        require(restored.parameters.getRawParameterValue("high")->load()==-20,"Restore high");
        require(restored.parameters.getRawParameterValue("output")->load()==-6,"Restore output");
        for(int n=0;n<200;++n) {
            for(int i=0;i<512;++i) {float sample=std::sin(float(i+n*512)*0.1f)*0.8f;b.setSample(0,i,sample);b.setSample(1,i,sample*0.5f);}
            p.processBlock(b,midi);
            for(int i=0;i<512;++i){require(std::isfinite(b.getSample(0,i)),"Finite audio");require(std::abs(b.getSample(0,i)*0.5f-b.getSample(1,i))<1e-6f,"Stereo linkage preserves balance");}
        }
        parameter(p,"low",0);parameter(p,"high",0);parameter(p,"output",18);
        for(int n=0;n<100;++n){b.clear();for(int c=0;c<2;++c)for(int i=0;i<512;++i)b.setSample(c,i,0.8f);p.processBlock(b,midi);}
        require(p.clipL.load() && p.clipR.load(),"Clipping must latch");
        auto editor=std::unique_ptr<juce::AudioProcessorEditor>(p.createEditor());
        require(editor->getWidth()==636 && editor->getHeight()==614,"Initial editor size");
        bool sliderChecked=false, textChecked=false;
        for(auto* child : editor->getChildren())
        {
            if(auto* slider=dynamic_cast<juce::Slider*>(child))
                if(slider->getName()=="Low Level") {slider->setValue(42,juce::sendNotificationSync);sliderChecked=true;}
        }
        require(sliderChecked && p.parameters.getRawParameterValue("low")->load()==42,"GUI slider must update processor");
        for(auto* child : editor->getChildren())
        {
            if(auto* label=dynamic_cast<juce::Label*>(child))
                if(label->getName()=="Output Gain dB") {
                    label->setText("-9,5",juce::sendNotificationSync);
                    require(std::abs(p.parameters.getRawParameterValue("output")->load()+9.5f)<0.01f,"Portuguese decimal comma must work");
                    label->setText("--3",juce::sendNotificationSync);
                    require(std::abs(p.parameters.getRawParameterValue("output")->load()+9.5f)<0.01f,"Malformed numeric edits must preserve the parameter");
                    textChecked=true;
                }
        }
        require(textChecked && p.parameters.getRawParameterValue("output")->load()==-9.5f,"Numeric edit must update processor");
        checkControls(p,*editor);
        editor->setSize(954,921);
        parameter(p,"low",23.8f);parameter(p,"high",-23.7f);parameter(p,"output",-15.7f);
        p.prepareToPlay(48000,512);
        for(int block=0;block<100;++block)
        {
            for(int i=0;i<512;++i){const float signal=0.5f*std::sin(float(i+block*512)*0.13f);b.setSample(0,i,signal);b.setSample(1,i,signal*0.8f);}
            p.processBlock(b,midi);
        }
        juce::Thread::sleep(35);juce::Timer::callPendingTimersSynchronously();
        auto image=editor->createComponentSnapshot(editor->getLocalBounds());
        require(image.isValid(),"Editor must render");
        juce::File file=juce::File::getCurrentWorkingDirectory().getChildFile("editor-preview.png");
        if(auto stream=file.createOutputStream()){stream->setPosition(0);stream->truncate();juce::PNGImageFormat png;require(png.writeImageToStream(image,*stream),"Snapshot must be written");} else throw std::runtime_error("Cannot write snapshot");
        editor.reset();
        auto layout=p.getBusesLayout();layout.inputBuses.set(0,juce::AudioChannelSet::mono());layout.outputBuses.set(0,juce::AudioChannelSet::mono());
        require(p.setBusesLayout(layout),"Mono layout accepted");p.prepareToPlay(44100,512);
        juce::AudioBuffer<float> mono(1,512);mono.clear();p.processBlock(mono,midi);
        require(mono.getMagnitude(0,512)==0,"Silence remains silent");
        std::cout<<"PASS: processor state, stereo/mono audio, transient metering, applied gain, 3 faders at 3 sizes, normal/fine drag, wheel, keyboard, balanced host gestures, numeric edits and editor rendering\n";
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
