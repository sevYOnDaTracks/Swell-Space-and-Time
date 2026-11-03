#include "../Source/PluginProcessor.h"
#include <iostream>
#include <stdexcept>
void require(bool ok,const char* msg){if(!ok)throw std::runtime_error(msg);}
void set(SpaceProcessor& p,const char* id,float v){auto* a=p.state.getParameter(id);a->setValueNotifyingHost(a->convertTo0to1(v));}
double run(float focus){SpaceProcessor p;set(p,"focus",focus);p.prepareToPlay(48000,64);juce::MidiBuffer m;juce::AudioBuffer<float>b(2,257);double e=0;for(int block=0;block<500;++block){for(int n=0;n<257;++n)for(int c=0;c<2;++c)b.setSample(c,n,.25f*std::sin((block*257+n)*.04f));p.processBlock(b,m);if(block>250)for(int n=0;n<257;++n){float dry=.25f*std::sin((block*257+n)*.04f);e+=std::pow(b.getSample(0,n)-dry,2);}}return e;}
int main(){try{juce::ScopedJuceInitialiser_GUI gui;juce::MidiBuffer midi;
for(double sr:{44100.,48000.,96000.})for(int preset=0;preset<20;++preset){SpaceProcessor p;p.setCurrentProgram(preset);p.prepareToPlay(sr,64);juce::AudioBuffer<float>b(2,257);double energy=0;for(int block=0;block<400;++block){b.clear();if(block==0){b.setSample(0,0,.5f);b.setSample(1,0,.5f);}p.processBlock(b,midi);for(int c=0;c<2;++c)for(int n=0;n<257;++n){float v=b.getSample(c,n);require(std::isfinite(v)&&std::abs(v)<4,"unstable output");if(block>0)energy+=v*v;}}require(energy>1e-8,"missing wet tail");}
SpaceProcessor p;set(p,"delay",0);set(p,"reverb",0);p.prepareToPlay(48000,64);juce::AudioBuffer<float>b(2,1024);for(int c=0;c<2;++c)for(int n=0;n<1024;++n)b.setSample(c,n,.1f*std::sin(n*.1f));p.processBlock(b,midi);for(int n=0;n<1024;++n)require(b.getSample(0,n)==.1f*std::sin(n*.1f),"dry altered");
p.setCurrentProgram(15);juce::MemoryBlock state;p.getStateInformation(state);SpaceProcessor restored;restored.setStateInformation(state.getData(),(int)state.getSize());require(restored.getCurrentProgram()==15&&restored.value(0)==0,"preset state not restored");
set(p,"bypass",1);p.prepareToPlay(48000,64);b.clear();b.setSample(0,0,.5f);p.processBlock(b,midi);require(b.getSample(0,0)==.5f,"bypass not transparent");
require(run(100)<run(0)*.3,"Focus does not reduce wet energy");std::cout<<"PASS: 20 presets at 3 rates, tails, finite output, oversized blocks, dry transparency, bypass, state, Focus reduction\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
