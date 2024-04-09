/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "DSP/Params.h"

ControlBar::ControlBar()
{
    analyzerButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    addAndMakeVisible(analyzerButton);
    
    addAndMakeVisible(globalBypassButton);
}

void ControlBar::resized()
{
    auto bounds = getLocalBounds();
    
    analyzerButton.setBounds(bounds.removeFromLeft(50)
                             .withTrimmedTop(4)
                             .withTrimmedLeft(4));
    
    globalBypassButton.setBounds(bounds.removeFromRight(60)
                                 .withTrimmedTop(2));
}
//==============================================================================
SimpleMBCompAudioProcessorEditor::SimpleMBCompAudioProcessorEditor (SimpleMBCompAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setLookAndFeel(&lnf);
    
    controlBar.analyzerButton.onClick = [this]()
    {
        auto shouldBeOn = controlBar.analyzerButton.getToggleState();
        analyzer.toggleAnalysisEnablement(shouldBeOn);
    };
    
    controlBar.globalBypassButton.onClick = [this]()
    {
        toggleGlobalBypassState();
    };
    
    addAndMakeVisible(controlBar);
    addAndMakeVisible(analyzer);
    
    overlay = std::make_unique<SimpleMBComp::MBCompAnalyzerOverlay>(*audioProcessor.lowMidCrossover,
                                                      *audioProcessor.midHighCrossover,
                                                      *audioProcessor.lowThresholdParam,
                                                      *audioProcessor.midThresholdParam,
                                                      *audioProcessor.highThresholdParam);
    addAndMakeVisible(*overlay);
    
    addAndMakeVisible(globalControls);
    addAndMakeVisible(bandControls);
    
    setSize (600, 500);
    
    startTimerHz(60);
}

SimpleMBCompAudioProcessorEditor::~SimpleMBCompAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

//==============================================================================
void SimpleMBCompAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace juce;
    
    g.fillAll(ColorScheme::getModuleBackgroundColor());
    
    g.setFont(Font("Iosevka Term Slab", 30, 0)); //https://github.com/be5invis/Iosevka
    
    String title { "GM MultiBand Comp" };
    g.setFont(20);
    
    auto bounds = getLocalBounds().withTrimmedTop(5);
    
    g.setColour(ColorScheme::getTitleColor());
    g.drawFittedText(title, bounds, juce::Justification::centredTop, 1);
}

void SimpleMBCompAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    
    auto bounds = getLocalBounds();
    
    controlBar.setBounds( bounds.removeFromTop(32) );
    
    bandControls.setBounds(bounds.removeFromBottom(137));
    
    analyzer.setBounds(bounds.removeFromTop(216));
    overlay->setBounds(analyzer.getBounds());
    
    globalControls.setBounds(bounds);
}

void SimpleMBCompAudioProcessorEditor::timerCallback()
{
    std::vector<float> values
    {
        audioProcessor.lowBandComp.getRMSInputLevelDb(),
        audioProcessor.lowBandComp.getRMSOutputLevelDb(),
        audioProcessor.midBandComp.getRMSInputLevelDb(),
        audioProcessor.midBandComp.getRMSOutputLevelDb(),
        audioProcessor.highBandComp.getRMSInputLevelDb(),
        audioProcessor.highBandComp.getRMSOutputLevelDb()
    };
    
    overlay->update(values);
    
    updateGlobalBypassButton();
}

void SimpleMBCompAudioProcessorEditor::updateGlobalBypassButton()
{
    auto params = getBypassParams();
    
    bool allBandsAreBypassed = std::all_of(params.begin(),
                                           params.end(),
                                           [](const auto& param){ return param->get(); });
    
    controlBar.globalBypassButton.setToggleState(allBandsAreBypassed,
                                                 juce::NotificationType::dontSendNotification);
}

void SimpleMBCompAudioProcessorEditor::toggleGlobalBypassState()
{
    auto shouldEnableEverything = ! controlBar.globalBypassButton.getToggleState();
    
    auto params = getBypassParams();
    
    auto bypassParamHelper = [](auto* param, bool shouldBeBypassed)
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost(shouldBeBypassed ? 1.f : 0.f );
        param->endChangeGesture();
    };
    
    for( auto* param : params )
    {
        bypassParamHelper(param, ! shouldEnableEverything);
    }
    
    bandControls.toggleAllBands(! shouldEnableEverything);
}

std::array<juce::AudioParameterBool*, 3> SimpleMBCompAudioProcessorEditor::getBypassParams()
{
    using namespace Params;
    using namespace juce;
    const auto& params = Params::GetParams();
    auto& apvts = audioProcessor.apvts;
    
    auto boolHelper = [&apvts, &params](const auto& paramName)
    {
        auto param = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(params.at(paramName)));
        jassert(param != nullptr);
        
        return param;
    };
    
    auto* lowBypassParam = boolHelper(Names::Bypassed_Low_Band);
    auto* midBypassParam = boolHelper(Names::Bypassed_Mid_Band);
    auto* highBypassParam = boolHelper(Names::Bypassed_High_Band);
    
    return
    {
        lowBypassParam,
        midBypassParam,
        highBypassParam
    };
}
