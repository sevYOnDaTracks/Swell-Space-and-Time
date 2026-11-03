#pragma once
#include "PluginProcessor.h"
class SpaceLook:public juce::LookAndFeel_V4 {public:SpaceLook();void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&)override;};
class SpaceEditor:public juce::AudioProcessorEditor,private juce::Timer {
public:explicit SpaceEditor(SpaceProcessor&);~SpaceEditor()override;void paint(juce::Graphics&)override;void resized()override;
private:void timerCallback()override;void fillPresets();
SpaceProcessor& proc;SpaceLook look;juce::Image logo;
std::array<juce::Slider,parameterCount> sliders;std::array<juce::Label,parameterCount> labels;
std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
juce::ComboBox category,preset,division,feel;juce::TextButton ping{"Ping-pong"},time{"Actif"},space{"Actif"},refine{"Affiner +"},bypass{"Bypass"},reset{"Retablir le preset"};
std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> divisionAttach,feelAttach;
std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttons;
bool expanded=false;int shownProgram=-1;float in=0,out=0;std::array<float,240> history{};int historyPos=0;
};
