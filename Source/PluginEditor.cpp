#include "PluginEditor.h"
#include "BinaryData.h"
#include "Scale.h"
#include <cstdlib>
namespace {
class InvisibleSliderLook : public juce::LookAndFeel_V4 {
    void drawLinearSlider(juce::Graphics&,int,int,int,int,float,float,float,const juce::Slider::SliderStyle,juce::Slider&) override {}
};
InvisibleSliderLook sliderLook;
}
MV2Editor::MV2Editor(MV2Processor& p) : AudioProcessorEditor(p), processor(p)
{
    panel = juce::ImageCache::getFromMemory(BinaryData::mv2reference_png, BinaryData::mv2reference_pngSize);
    prepareBackdrop();
    lowText.linked=&low; highText.linked=&high; outputText.linked=&output;
    configure(low,lowText,"Low Level"); configure(high,highText,"High Level"); configure(output,outputText,"Output Gain");
    lowAttachment = std::make_unique<Attachment>(p.parameters,"low",low);
    highAttachment = std::make_unique<Attachment>(p.parameters,"high",high);
    outputAttachment = std::make_unique<Attachment>(p.parameters,"output",output);
    low.setRange(0,48,0.1); high.setRange(-48,0,0.1); output.setRange(-48,18,0.1);
    for(auto* s : {&low,&high,&output}) s->setDoubleClickReturnValue(true,0);
    const juce::StringArray names {"Undo","Redo","Reset parameters","Compare A/B","Load preset","Save preset","Help"};
    for(int i=0;i<names.size();++i)
    {
        auto b = std::make_unique<juce::TextButton>();
        b->setName(names[i]); b->setTooltip(names[i]);
        b->setAlpha(0.0f); // The immutable image supplies the toolbar artwork.
        b->onClick = [this,i]{ toolbar(i); };
        addAndMakeVisible(*b); buttons.push_back(std::move(b));
    }
    stateA = p.parameters.copyState(); stateB = stateA.createCopy();
    constraints.setSizeLimits(318,307,1272,1228);
    constraints.setFixedAspectRatio(318.0/307.0);
    setResizable(true,false); setResizeLimits(318,307,1272,1228);
    getConstrainer()->setFixedAspectRatio(318.0/307.0);
    sizeButton.setTooltip("Tamanho da interface");
    sizeButton.setColour(juce::TextButton::buttonColourId,juce::Colour(0xff39392e));
    sizeButton.setColour(juce::TextButton::textColourOffId,juce::Colour(0xffe0d7b0));
    sizeButton.onClick=[this] {
        juce::PopupMenu menu;
        for(auto percent : {100,150,200,250,300,400}) menu.addItem(percent,juce::String(percent)+"%");
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(sizeButton),
            [safe=juce::Component::SafePointer<MV2Editor>(this)](int percent) {
                if(safe!=nullptr && percent>0) safe->setSize(318*percent/100,307*percent/100);
            });
    };
    addAndMakeVisible(sizeButton);
    addAndMakeVisible(corner);
    setSize(636,614);
    low.onValueChange(); high.onValueChange(); output.onValueChange();
    lastMeterTime=juce::Time::getMillisecondCounterHiRes();
    startTimerHz(60);
}
void MV2Editor::configure(juce::Slider& slider,juce::Label& label,const juce::String& name)
{
    slider.setSliderStyle(juce::Slider::LinearVertical);
    slider.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
    slider.setLookAndFeel(&sliderLook); slider.setName(name);
    slider.setTooltip(name + ": arraste; Shift/Ctrl para ajuste fino; roda e setas também ajustam. Duplo clique no trilho restaura 0 dB.");
    slider.setMouseDragSensitivity(192);
    slider.setSliderSnapsToMousePosition(false);
    slider.setVelocityModeParameters(1.0,1,0.0,false);
    slider.setWantsKeyboardFocus(true);
    if(auto* responsive=dynamic_cast<ResponsiveSlider*>(&slider))
        responsive->completeGesture=[this,&slider](double value) {
            auto* p=processor.parameters.getParameter(&slider==&low ? "low" : (&slider==&high ? "high" : "output"));
            processor.undo.beginNewTransaction();p->beginChangeGesture();
            slider.setValue(value,juce::sendNotificationSync);p->endChangeGesture();
        };
    addAndMakeVisible(slider);
    label.setName(name + " dB"); label.setEditable(false,true,false);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::backgroundColourId,juce::Colour(0xff151614));
    label.setColour(juce::Label::textColourId,juce::Colour(0xffddddda));
    label.setColour(juce::Label::outlineColourId,juce::Colours::transparentBlack);
    label.setTooltip("Duplo clique para digitar o valor em dB; Enter confirma.");
    slider.onValueChange = [this,&slider,&label] { label.setText(juce::String(slider.getValue(),1),juce::dontSendNotification); if(buttons.size()==7) {positionReadouts();repaint();} };
    slider.onDragStart = [this]{processor.undo.beginNewTransaction();};
    label.onTextChange = [this,&slider,&label] {
        const auto text = label.getText().trim().replaceCharacter(',','.').toStdString();
        char* end=nullptr;
        const double value=std::strtod(text.c_str(),&end);
        if(end!=text.c_str() && *end=='\0' && std::isfinite(value))
        {
            auto* parameter = processor.parameters.getParameter(&slider == &low ? "low" : (&slider == &high ? "high" : "output"));
            processor.undo.beginNewTransaction();
            parameter->beginChangeGesture(); slider.setValue(value,juce::sendNotificationSync); parameter->endChangeGesture();
        }
        label.setText(juce::String(slider.getValue(),1),juce::dontSendNotification);
    };
    addAndMakeVisible(label);
}
juce::Rectangle<int> MV2Editor::rect(float x,float y,float w,float h) const
{
    return juce::Rectangle<float>(x*getWidth()/1277.0f,y*getHeight()/1232.0f,w*getWidth()/1277.0f,h*getHeight()/1232.0f).toNearestInt();
}
void MV2Editor::resized()
{
    low.setBounds(rect(44,230,184,650)); high.setBounds(rect(588,230,190,650)); output.setBounds(rect(911,230,198,750));
    positionReadouts();
    for(auto* label : {&lowText,&highText,&outputText}) label->setFont(juce::FontOptions(32.0f * getWidth()/1277.0f));
    const float x[] {23,127,231,729,864,1000,1135};
    const float w[] {92,92,486,120,124,123,55};
    for(int i=0;i<7;++i) buttons[static_cast<size_t>(i)]->setBounds(rect(x[i],24,w[i],49));
    sizeButton.setBounds(rect(1120,1170,116,34));
    sizeButton.setButtonText(juce::String(juce::roundToInt(getWidth()*100.0/318.0))+"%");
    corner.setBounds(getWidth()-20,getHeight()-20,20,20);
}
void MV2Editor::positionReadouts()
{
    lowText.setBounds(rect(63, Scale::position(static_cast<float>(low.getValue()),Scale::low)-20,126,41));
    highText.setBounds(rect(624,Scale::position(static_cast<float>(high.getValue()),Scale::high)-20,126,41));
    outputText.setBounds(rect(946,Scale::position(static_cast<float>(output.getValue()),Scale::outputFader)-20,126,41));
}
void MV2Editor::paint(juce::Graphics& g)
{
    g.setOpacity(1.0f); g.drawImage(backdrop,getLocalBounds().toFloat());
    auto fader = [&](int x,int originalY,float centre,int width) {
        auto destination=rect(static_cast<float>(x),centre-37,static_cast<float>(width),75);
        juce::DropShadow(juce::Colours::black.withAlpha(0.45f),6,{3,5}).drawForRectangle(g,destination);
        g.setOpacity(1.0f);
        g.drawImage(panel,destination.getX(),destination.getY(),destination.getWidth(),destination.getHeight(),x,originalY,width,75);
        auto arrow=[&](int sourceX,int sourceY,int arrowWidth) {
            auto target=rect(static_cast<float>(sourceX),centre-14,static_cast<float>(arrowWidth),28);
            g.drawImage(panel,target.getX(),target.getY(),target.getWidth(),target.getHeight(),sourceX,sourceY,arrowWidth,28);
        };
        if(x==44)arrow(211,443,18);
        if(x==603)arrow(589,625,15);
        if(x==927){arrow(910,540,16);arrow(1093,540,15);}
    };
    fader(44,420,Scale::position(static_cast<float>(low.getValue()),Scale::low),166);
    fader(603,600,Scale::position(static_cast<float>(high.getValue()),Scale::high),168);
    fader(927,518,Scale::position(static_cast<float>(output.getValue()),Scale::outputFader),166);
    auto meter = [&](float x,float width,float top,float bottom,float fillTop,juce::Colour colour) {
        auto r=rect(x,top,width,bottom-top).toFloat();
        g.setColour(juce::Colour(0xff090a09));g.fillRoundedRectangle(r,3.0f);
        const float y=juce::jlimit(r.getY(),r.getBottom(),fillTop*getHeight()/1232.0f);
        auto lit=r.withTop(y);
        juce::Graphics::ScopedSaveState state(g);
        juce::Path clip;clip.addRoundedRectangle(r,3.0f);g.reduceClipRegion(clip);
        g.setGradientFill(juce::ColourGradient(colour.darker(0.5f),lit.getX(),0,colour.brighter(0.25f),lit.getRight(),0,false));
        g.fillRect(lit);
    };
    meter(312,39,260,836,Scale::position(shownLow,Scale::low),juce::Colour(0xffc9ed00));
    // These scales are not linear in image space: each dB mark is an anchor.
    auto hr=rect(464,260,40,577).toFloat();g.setColour(juce::Colour(0xff090a09));g.fillRect(hr);
    if(shownHigh>0.05f)
    {
        auto fill=rect(464,270,40,Scale::position(-shownHigh,Scale::high)-270).toFloat();
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xffb1000a),fill.getX(),0,juce::Colour(0xffff3842),fill.getRight(),0,false));g.fillRect(fill);
    }
    meter(892,39,264,947,shownL<=-60 ? 947 : Scale::position(shownL,Scale::outputMeter),juce::Colour(0xffff9900));
    meter(1086,39,264,947,shownR<=-60 ? 947 : Scale::position(shownR,Scale::outputMeter),juce::Colour(0xffff9900));
    auto digital = [&](float x,float y,float w,juce::String text) {
        auto r=rect(x,y,w,35);g.setColour(juce::Colour(0xff101210));g.fillRect(r);
        g.setColour(juce::Colour(0xffdeded8)); g.setFont(juce::FontOptions(25.0f*getWidth()/1277.0f));g.drawText(text,r,juce::Justification::centred);
    };
    digital(264,903,105,juce::String(shownLow,1));
    digital(446,903,103,juce::String(shownHigh < 0.05f ? 0.0f : -shownHigh,1));
    digital(870,1005,99,heldL <= -99 ? "-inf" : juce::String(heldL,1));
    digital(1045,1005,99,heldR <= -99 ? "-inf" : juce::String(heldR,1));
    if(processor.clipL.load()) {g.setColour(juce::Colours::red);g.fillEllipse(rect(895,200,32,10).toFloat());}
    if(processor.clipR.load()) {g.setColour(juce::Colours::red);g.fillEllipse(rect(1089,200,32,10).toFloat());}
    g.setColour(juce::Colour(0xffc7be99));g.setFont(juce::FontOptions(14.0f*getWidth()/1277.0f));
    g.drawText(activeB ? "B" : "A",rect(689,35,20,25),juce::Justification::centred);
}
void MV2Editor::timerCallback()
{
    const double now=juce::Time::getMillisecondCounterHiRes();
    const float dt=static_cast<float>(juce::jlimit(0.001,0.25,(now-lastMeterTime)*0.001));lastMeterTime=now;
    const float peakL=Dynamics::db(processor.pendingPeakL.exchange(0));
    const float peakR=Dynamics::db(processor.pendingPeakR.exchange(0));
    shownL=std::max(peakL,shownL-24.0f*dt);
    shownR=std::max(peakR,shownR-24.0f*dt);
    shownLow=std::max(processor.lowMeter.load(),shownLow-120.0f*dt);
    shownHigh=std::max(processor.highMeter.load(),shownHigh-120.0f*dt);
    heldL=Dynamics::db(processor.heldPeakL.load());heldR=Dynamics::db(processor.heldPeakR.load());
    repaint();
}
void MV2Editor::setParameter(const juce::String& id,float value)
{
    auto* p=processor.parameters.getParameter(id);p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(value));p->endChangeGesture();
}
void MV2Editor::toolbar(int action)
{
    if(action==0) {processor.undo.undo();return;}
    if(action==1) {processor.undo.redo();return;}
    if(action==2) {processor.undo.beginNewTransaction();for(auto id:{"low","high","output"})setParameter(id,0);return;}
    if(action==3)
    {
        if(activeB) {stateB=processor.parameters.copyState();processor.parameters.replaceState(stateA.createCopy());}
        else {stateA=processor.parameters.copyState();processor.parameters.replaceState(stateB.createCopy());}
        activeB=!activeB;return;
    }
    if(action==6)
    {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,"MV2 Recreation",
          "Low: 0 a 48 dB | High: -48 a 0 dB | Output: -48 a +18 dB\n"
          "Arraste os controles; duplo clique nos números para editar. Duplo clique nos trilhos restaura zero.\n"
          "Clique nos medidores de saída para limpar os picos/clipping. Arraste o canto inferior direito para redimensionar.\n"
          "Implementação independente; não é um produto Waves nem uma reprodução exata do DSP proprietário.");return;
    }
    chooser=std::make_unique<juce::FileChooser>(action==4 ? "Carregar preset" : "Salvar preset",juce::File{},"*.mv2preset");
    auto flags=action==4 ? juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles
                         : juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting;
    chooser->launchAsync(flags,[safe=juce::Component::SafePointer<MV2Editor>(this),action](const juce::FileChooser& fc) {
        if(safe==nullptr || fc.getResult()==juce::File{})return;
        if(action==5) {juce::MemoryBlock data;safe->processor.getStateInformation(data);fc.getResult().withFileExtension("mv2preset").replaceWithData(data.getData(),data.getSize());}
        else {juce::MemoryBlock data;if(fc.getResult().loadFileAsData(data))safe->processor.setStateInformation(data.getData(),static_cast<int>(data.getSize()));}
    });
}

void MV2Editor::mouseDown(const juce::MouseEvent& event)
{
    if(rect(790,170,438,910).contains(event.getPosition()))
    {
        heldL=heldR=-100;processor.heldPeakL.store(0);processor.heldPeakR.store(0);
        processor.clipL.store(false);processor.clipR.store(false);repaint();
    }
}

void MV2Editor::prepareBackdrop()
{
    backdrop=panel.createCopy();
    struct Patch {int x,y,w,h,donor;};
    // Reconstruct only pixels hidden by the baked controls, in memory. Colour
    // follows the same column above/below each handle; nearby metal supplies grain.
    for(const auto patch : {Patch{32,408,198,180,370},Patch{585,588,204,180,370},Patch{907,506,204,172,1150}})
    {
        for(int x=patch.x;x<patch.x+patch.w;++x)
        {
            auto top=panel.getPixelAt(x,patch.y-12), bottom=panel.getPixelAt(x,patch.y+patch.h+12);
            for(int y=patch.y;y<patch.y+patch.h;++y)
            {
                const float t=float(y-patch.y)/float(patch.h);
                auto colour=top.interpolatedWith(bottom,t);
                const int donorX=patch.donor+(x-patch.x)%40;
                const auto texture=panel.getPixelAt(donorX,y);
                const auto neighbour=panel.getPixelAt(donorX+1,y+1);
                const int grain=juce::jlimit(-8,8,int(texture.getRed())-int(neighbour.getRed()));
                auto add=[grain](juce::uint8 channel){return static_cast<juce::uint8>(juce::jlimit(0,255,int(channel)+grain));};
                colour=juce::Colour(add(colour.getRed()),add(colour.getGreen()),add(colour.getBlue()));
                const int edge=std::min({x-patch.x,patch.x+patch.w-1-x,y-patch.y,patch.y+patch.h-1-y});
                const float blend=juce::jlimit(0.0f,1.0f,float(edge)/8.0f);
                backdrop.setPixelAt(x,y,panel.getPixelAt(x,y).interpolatedWith(colour,blend));
            }
        }
    }
    juce::Graphics graphics(backdrop);
    // Restore the reference's scale marks where they were behind the handles.
    for(auto tick : {juce::Point<int>{166,454},juce::Point<int>{599,638},juce::Point<int>{599,752}})
    {
        graphics.setColour(juce::Colour(0xffeeeede));graphics.fillRect(tick.x,tick.y,42,2);
        graphics.setColour(juce::Colour(0xff55564c));graphics.fillRect(tick.x,tick.y+2,42,3);
    }
}
