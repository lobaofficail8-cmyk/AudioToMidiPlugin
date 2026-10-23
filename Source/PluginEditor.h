#pragma once
#include "PluginProcessor.h"

class AudioToMidiEditor : public juce::AudioProcessorEditor,
                          public juce::DragAndDropContainer,
                          private juce::Timer
{
public:
    explicit AudioToMidiEditor (AudioToMidiProcessor&);
    ~AudioToMidiEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct DragZone : juce::Component
    {
        std::function<void()> onDragStart;
        bool started = false;
        void paint (juce::Graphics& g) override
        {
            g.setColour (juce::Colours::darkorange.withAlpha (0.25f));
            g.fillRoundedRectangle (getLocalBounds().toFloat(), 8.0f);
            g.setColour (juce::Colours::white);
            g.setFont (15.0f);
            g.drawText ("Drag MIDI to your track", getLocalBounds(), juce::Justification::centred);
        }
        void mouseDown (const juce::MouseEvent&) override { started = false; }
        void mouseDrag (const juce::MouseEvent& e) override
        {
            if (! started && e.getDistanceFromDragStart() > 5)
            {
                started = true;
                if (onDragStart) onDragStart();
            }
        }
    };

    void timerCallback() override;

    AudioToMidiProcessor& proc;
    juce::Label noteLabel, infoLabel, thrL, minL, trL;
    juce::Slider thr, minNote, trans;
    juce::TextButton recBtn { "Record" }, clearBtn { "Clear" }, saveBtn { "Save .mid" };
    DragZone dragZone;
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SA> thrA, minA, trA;
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioToMidiEditor)
};
