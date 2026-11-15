#include "Dynamics.h"
#include "Scale.h"
#include <iostream>
#include <stdexcept>
#include <limits>
void check(bool condition,const char* message) { if(!condition)throw std::runtime_error(message); }
float settled(float level,float low,float high,float out,double rate=48000)
{
    Dynamics dsp;dsp.prepare(rate);float gain=1;
    for(int i=0;i<static_cast<int>(rate*2);++i) gain=dsp.tick(Dynamics::amplitude(level),low,high,out);
    return Dynamics::db(gain);
}
int main()
{
    try {
        for(const auto& mark : Scale::low)check(std::abs(Scale::position(mark.value,Scale::low)-mark.y)<0.01f,"Low fader/meter must align with every printed dB mark");
        for(const auto& mark : Scale::high)check(std::abs(Scale::position(mark.value,Scale::high)-mark.y)<0.01f,"High fader/meter must align with every printed dB mark");
        for(const auto& mark : Scale::outputMeter)check(std::abs(Scale::position(mark.value,Scale::outputMeter)-mark.y)<0.01f,"Output meter must align with every printed dB mark");
        for(float value=-59.9f;value<0;value+=0.1f)check(Scale::position(value,Scale::outputMeter)<=Scale::position(value-0.1f,Scale::outputMeter),"Meter scale must be monotonic");
        for(auto rate : {44100.0,48000.0,96000.0}) {
            check(std::abs(settled(-12,0,0,0,rate))<0.01f,"Neutral controls must preserve gain");
            check(settled(-50,24,0,0,rate)>10,"Low-level signals must be lifted");
            check(std::abs(settled(-50,0,0,0,rate))<0.01f,"Zero low must disable lift");
            check(settled(-6,0,-24,0,rate)<-8,"High-level signals must be reduced");
            check(std::abs(settled(-60,0,-24,0,rate))<0.01f,"High compressor must spare quiet signals");
            check(std::abs(settled(-24,0,0,-12,rate)+12)<0.02f,"Output gain must apply in dB");
            check(settled(-100,48,0,0,rate)<=48.001f,"Upward gain must be bounded");
            Dynamics dsp;dsp.prepare(rate);
            float previous=1;
            for(int i=0;i<10000;++i) {
                float gain=dsp.tick(0.1f,48,-48,18);
                check(std::isfinite(gain),"Gain must remain finite");
                check(std::abs(gain-previous)<0.2f,"Gain changes must be smoothed");previous=gain;
            }
            dsp.reset();
            check(std::abs(dsp.tick(0.1f,0,0,0)-1)<0.001f,"Reset must clear history");
        }
        std::cout<<"PASS: 10 DSP checks at 44.1, 48 and 96 kHz (30 scenarios), plus printed scale alignment/monotonicity\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
