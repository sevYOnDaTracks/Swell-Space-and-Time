#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Licence/SwellLicence.h"
#include "Presets.h"
class SpaceProcessor : public juce::AudioProcessor {
public:
    // Licence Swell : partagée par toutes les instances ; silence tant que le plugin n'est pas activé.
    juce::SharedResourcePointer<swell::Licence> licence;

public:
 SpaceProcessor();
 juce::AudioProcessorValueTreeState state;
 std::atomic<float> inputPeak{0},outputPeak{0},duckDb{0},tempoDisplay{120};
 std::atomic<bool> hostTempo{false};
 const juce::String getName() const override{return "Swell Space and Time";}
 bool acceptsMidi() const override{return false;} bool producesMidi() const override{return false;}
 double getTailLengthSeconds() const override;
 bool isBusesLayoutSupported(const BusesLayout&) const override;
 void prepareToPlay(double,int) override;void releaseResources() override{}
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 void processBlockBypassed(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 juce::AudioProcessorParameter* getBypassParameter() const override{return state.getParameter("bypass");}
 bool hasEditor() const override{return true;}juce::AudioProcessorEditor* createEditor() override;
 int getNumPrograms() override{return 20;} int getCurrentProgram() override{return program.load();}
 void setCurrentProgram(int i) override;const juce::String getProgramName(int i) override{return presets[(size_t)juce::jlimit(0,19,i)].name;}
 void changeProgramName(int,const juce::String&) override{}
 void getStateInformation(juce::MemoryBlock&) override;void setStateInformation(const void*,int) override;
 float value(int i)const{return parameters[(size_t)i]->load();}
private:
 static juce::AudioProcessorValueTreeState::ParameterLayout layout();
 void render(juce::AudioBuffer<float>&,bool);
 struct Filter {float low=0,high=0; float tick(float x,float hp,float lp){low=(1-hp)*x+hp*low;float y=x-low;high=(1-lp)*y+lp*high;return high;} };
 std::array<std::atomic<float>*,parameterCount> parameters{};
 std::array<juce::SmoothedValue<float>,parameterCount> smooth;
 juce::SmoothedValue<float> wet, pingBlend;
 std::array<std::vector<float>,2> delayLine,preLine;
 std::array<Filter,2> delayFilter,reverbFilter;
 juce::Reverb reverbEngine;
 double sampleRate=44100;int position=0,prePosition=0;
 float delaySamples=22050,oldDelay=22050,crossfade=1,env=0,duck=1;
 std::atomic<int> program{0};
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpaceProcessor)
};
