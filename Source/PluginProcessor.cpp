#include "ResizableEditor.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"
SpaceProcessor::SpaceProcessor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),state(*this,nullptr,"SwellSpaceTime",layout()) {for(int i=0;i<parameterCount;++i)parameters[i]=state.getRawParameterValue(ids[i]);}
juce::AudioProcessorValueTreeState::ParameterLayout SpaceProcessor::layout(){
 juce::AudioProcessorValueTreeState::ParameterLayout l;
 const char* names[]={"Delay enabled","Reverb enabled","Delay division","Delay rhythm","Feedback","Delay amount","Space size","Reverb amount","Focus","Delay width","Delay low cut","Delay high cut","Reverb width","Reverb low cut","Reverb high cut","Pre-delay","Manual tempo"};
 for(int i=0;i<parameterCount;++i){juce::ParameterID id{ids[i],1};float def=presets[0].values[i];
 if(i<2)l.add(std::make_unique<juce::AudioParameterBool>(id,names[i],def>.5f));
 else if(i==2)l.add(std::make_unique<juce::AudioParameterChoice>(id,names[i],juce::StringArray{"1/1","1/2","1/4","1/8","1/16"},2));
 else if(i==3)l.add(std::make_unique<juce::AudioParameterChoice>(id,names[i],juce::StringArray{"Normal","Pointe","Triolet"},0));
 else {float min=0,max=100,step=.1f,skew=1;
 if(i==4)max=90;if(i==9||i==12||i==15)max=150;
 if(i==10||i==13){min=20;max=1000;step=1;skew=.5f;}
 if(i==11||i==14){min=1500;max=20000;step=1;skew=.5f;}
 if(i==16){min=30;max=300;step=.1f;}
 l.add(std::make_unique<juce::AudioParameterFloat>(id,names[i],juce::NormalisableRange<float>(min,max,step,skew),def));}}
 l.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"bypass",1},"Bypass",false));l.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"pingpong",1},"Ping-pong",false));return l;
}
bool SpaceProcessor::isBusesLayoutSupported(const BusesLayout& b)const{return b.getMainInputChannelSet()==b.getMainOutputChannelSet()&&(b.getMainOutputChannelSet()==juce::AudioChannelSet::mono()||b.getMainOutputChannelSet()==juce::AudioChannelSet::stereo());}
void SpaceProcessor::prepareToPlay(double sr,int){sampleRate=sr;for(auto& v:delayLine)v.assign((size_t)(sr*12.1)+4,0);for(auto& v:preLine)v.assign((size_t)(sr*.16)+4,0);position=prePosition=0;env=0;duck=1;delayFilter={};reverbFilter={};reverbEngine.setSampleRate(sr);reverbEngine.reset();for(int i=0;i<parameterCount;++i){smooth[i].reset(sr,.04);smooth[i].setCurrentAndTargetValue(value(i));}pingBlend.reset(sr,.04);pingBlend.setCurrentAndTargetValue(state.getRawParameterValue("pingpong")->load());wet.reset(sr,.02);wet.setCurrentAndTargetValue(state.getRawParameterValue("bypass")->load()>.5f?0.f:1.f);delaySamples=oldDelay=(float)(sr*60/value(16));crossfade=1;inputPeak=outputPeak=duckDb=0;setLatencySamples(0);}
void SpaceProcessor::setCurrentProgram(int i){i=juce::jlimit(0,19,i);program=i;auto* ping=state.getParameter("pingpong");ping->beginChangeGesture();ping->setValueNotifyingHost(0);ping->endChangeGesture();for(int k=0;k<parameterCount;++k){auto* p=state.getParameter(ids[k]);p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(presets[i].values[k]));p->endChangeGesture();}}
void SpaceProcessor::getStateInformation(juce::MemoryBlock& b){auto t=state.copyState();t.setProperty("preset",program.load(),nullptr);auto xml=t.createXml();copyXmlToBinary(*xml,b);}
void SpaceProcessor::setStateInformation(const void* data,int size){auto xml=getXmlFromBinary(data,size);if(xml&&xml->hasTagName(state.state.getType())){auto t=juce::ValueTree::fromXml(*xml);program=juce::jlimit(0,19,(int)t.getProperty("preset",0));bool hasPing=false;for(auto child:t)if(child.getProperty("id").toString()=="pingpong")hasPing=true;if(!hasPing){juce::ValueTree child("PARAM");child.setProperty("id","pingpong",nullptr);child.setProperty("value",0.f,nullptr);t.addChild(child,-1,nullptr);}state.replaceState(t);}}
void SpaceProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer&){render(b,false);}
void SpaceProcessor::processBlockBypassed(juce::AudioBuffer<float>& b,juce::MidiBuffer&){render(b,true);}
void SpaceProcessor::render(juce::AudioBuffer<float>& b,bool forced){
 juce::ScopedNoDenormals guard;if(delayLine[0].empty())return;
 const int channels=std::min(2,b.getNumChannels());if(channels==0)return;
 for(int i=0;i<parameterCount;++i)smooth[i].setTargetValue(value(i));
 pingBlend.setTargetValue(state.getRawParameterValue("pingpong")->load());
 bool bypass=forced||state.getRawParameterValue("bypass")->load()>.5f;wet.setTargetValue(bypass?0.f:1.f);
 double bpm=value(16);bool sync=false;if(auto* ph=getPlayHead())if(auto info=ph->getPosition())if(auto hostBpm=info->getBpm())if(std::isfinite(*hostBpm)&&*hostBpm>0){bpm=juce::jlimit(30.,300.,*hostBpm);sync=true;}
 tempoDisplay=(float)bpm;hostTempo=sync;const float beats[]={4,2,1,.5f,.25f};const float feel[]={1,1.5f,2.f/3.f};float target=(float)(sampleRate*60/bpm)*beats[juce::jlimit(0,4,(int)value(2))]*feel[juce::jlimit(0,2,(int)value(3))];
 if(crossfade>=1&&std::abs(target-delaySamples)>.5f){oldDelay=delaySamples;delaySamples=target;crossfade=0;}
 juce::Reverb::Parameters rp;rp.roomSize=.12f+smooth[6].getCurrentValue()*.0082f;rp.damping=.45f;rp.wetLevel=1;rp.dryLevel=0;rp.width=1;rp.freezeMode=0;reverbEngine.setParameters(rp);
 const float attack=(float)std::exp(-1/(sampleRate*.003)),release=(float)std::exp(-1/(sampleRate*.22));
 float ip=0,op=0,maximumDuck=0;const int len=(int)delayLine[0].size(),plen=(int)preLine[0].size();
 auto tap=[](const std::vector<float>& line,int pos,float distance){float read=pos-distance;while(read<0)read+=(float)line.size();int a=(int)read;float f=read-a;return line[a]*(1-f)+line[(a+1)%line.size()]*f;};
 auto widen=[](float& l,float& r,float width){float m=(l+r)*.5f,s=(l-r)*.5f*width;float norm=std::max(1.f,width);l=(m+s)/norm;r=(m-s)/norm;};
 // Filter coefficients follow smoothed controls once per block; signal states remain continuous.
 auto coefficient=[&](float hz){return (float)std::exp(-juce::MathConstants<double>::twoPi*std::min((double)hz,sampleRate*.45)/sampleRate);};
 float dh=coefficient(smooth[10].getCurrentValue()),dl=coefficient(smooth[11].getCurrentValue()),rh=coefficient(smooth[13].getCurrentValue()),rl=coefficient(smooth[14].getCurrentValue());
 for(int n=0;n<b.getNumSamples();++n){std::array<float,parameterCount> v;for(int k=0;k<parameterCount;++k)v[k]=smooth[k].getNextValue();float dry[2]={b.getSample(0,n),b.getSample(channels-1,n)};float peak=std::max(std::abs(dry[0]),std::abs(dry[1]));ip=std::max(ip,peak);env=std::max(peak,env*release);float strength=juce::jlimit(0.f,1.f,(juce::Decibels::gainToDecibels(env,-100.f)+48.f)/36.f);float duckTarget=juce::Decibels::decibelsToGain(-24.f*v[8]/100*strength);float coeff=duckTarget<duck?attack:release;duck=duckTarget+(duck-duckTarget)*coeff;maximumDuck=std::max(maximumDuck,-juce::Decibels::gainToDecibels(duck));
 float echo[2];for(int c=0;c<2;++c)echo[c]=delayFilter[c].tick(tap(delayLine[c],position,oldDelay)*(1-crossfade)+tap(delayLine[c],position,delaySamples)*crossfade,dh,dl);
 // Read both taps before writing either channel. In ping-pong mode a mono send
 // starts on the left, then each repeat feeds the opposite delay line.
 float ping=pingBlend.getNextValue(),mono=(dry[0]+dry[1])*.5f;
 for(int c=0;c<2;++c){float input=dry[c]*(1-ping)+(c==0?mono:0.f)*ping;float feedback=echo[c]*(1-ping)+echo[1-c]*ping;delayLine[c][position]=(input+feedback*v[4]/100)*v[0];}
 crossfade=std::min(1.f,crossfade+(float)(1/(sampleRate*.05)));position=(position+1)%len;
 // Parallel dry signal; a modest delay send also enters the reverb.
 float rv[2];for(int c=0;c<2;++c){preLine[c][prePosition]=(dry[c]+echo[c]*(v[5]/100)*.25f*v[0])*v[1];rv[c]=tap(preLine[c],prePosition,(float)(v[15]*sampleRate*.001));}prePosition=(prePosition+1)%plen;
 reverbEngine.processStereo(&rv[0],&rv[1],1);for(int c=0;c<2;++c)rv[c]=reverbFilter[c].tick(rv[c],rh,rl);
 widen(echo[0],echo[1],v[9]/100);widen(rv[0],rv[1],v[12]/100);
 float wetGain=wet.getNextValue();for(int c=0;c<channels;++c){float e=echo[c],r=rv[c];if(channels==1){e=(echo[0]+echo[1])*.5f;r=(rv[0]+rv[1])*.5f;}float out=dry[c]+wetGain*duck*(e*v[5]/100*v[0]+r*v[7]/100*v[1]);b.setSample(c,n,out);op=std::max(op,std::abs(out));}}
 for(int c=channels;c<b.getNumChannels();++c)b.clear(c,0,b.getNumSamples());
 auto publish=[](std::atomic<float>& a,float x){float prior=a.load();while(prior<x&&!a.compare_exchange_weak(prior,x)){};};publish(inputPeak,ip);publish(outputPeak,op);duckDb=maximumDuck;
}
juce::AudioProcessorEditor* SpaceProcessor::createEditor(){return new ResizableEditor(*this, new SpaceEditor(*this));}juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new SpaceProcessor();}

double SpaceProcessor::getTailLengthSeconds() const {
 const double beats[]={4,2,1,.5,.25},feel[]={1,1.5,2./3.};
 double time=0;
 if(value(0)>.5f&&value(5)>0)time=60./std::max(30.f,tempoDisplay.load())*beats[juce::jlimit(0,4,(int)value(2))]*feel[juce::jlimit(0,2,(int)value(3))]*(1+std::log(.0001)/std::log(juce::jlimit(.01,.9,(double)value(4)/100)));
 return time+((value(1)>.5f&&value(7)>0)?30.:0.);
}