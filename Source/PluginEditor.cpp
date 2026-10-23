#include "PluginEditor.h"

AudioToMidiEditor::AudioToMidiEditor (AudioToMidiProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    auto setup = [this] (juce::Slider& s, juce::Label& l, const juce::String& t)
    {
        s.setSliderStyle (juce::Slider::LinearHorizontal);
        s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 70, 22);
        l.setText (t, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (s);
        addAndMakeVisible (l);
    };
    setup (thr, thrL, "Threshold");
    setup (minNote, minL, "Min note");
    setup (trans, trL, "Transpose");
    thrA = std::make_unique<SA> (proc.apvts, "threshold", thr);
    minA = std::make_unique<SA> (proc.apvts, "minNote", minNote);
    trA  = std::make_unique<SA> (proc.apvts, "transpose", trans);

    noteLabel.setFont (juce::FontOptions (44.0f, juce::Font::bold));
    noteLabel.setJustificationType (juce::Justification::centred);
    noteLabel.setText ("-", juce::dontSendNotification);
    infoLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (noteLabel);
    addAndMakeVisible (infoLabel);

    addAndMakeVisible (recBtn);
    addAndMakeVisible (clearBtn);
    addAndMakeVisible (saveBtn);
    addAndMakeVisible (dragZone);

    recBtn.onClick = [this]
    {
        if (proc.isRecording()) proc.stopRecording(); else proc.startRecording();
        recBtn.setButtonText (proc.isRecording() ? "Stop" : "Record");
    };
    clearBtn.onClick = [this] { proc.drainEvents(); proc.clearRecording(); };
    saveBtn.onClick = [this]
    {
        proc.drainEvents();
        if (! proc.hasRecording()) return;
        chooser = std::make_unique<juce::FileChooser> ("Save MIDI", juce::File::getSpecialLocation (juce::File::userDesktopDirectory)
                                                                        .getChildFile ("melody.mid"), "*.mid");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
                              [this] (const juce::FileChooser& fc)
                              {
                                  auto f = fc.getResult();
                                  if (f != juce::File())
                                      proc.writeMidiFile (f.withFileExtension ("mid"));
                              });
    };
    dragZone.onDragStart = [this]
    {
        proc.drainEvents();
        auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("AudioToMidi_take.mid");
        if (proc.writeMidiFile (f))
            juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
    };

    setSize (420, 380);
    startTimerHz (20);
}

AudioToMidiEditor::~AudioToMidiEditor() { stopTimer(); }

void AudioToMidiEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1c1b1f));
    g.setColour (juce::Colours::white);
}

void AudioToMidiEditor::resized()
{
    auto r = getLocalBounds().reduced (16);
    noteLabel.setBounds (r.removeFromTop (60));
    infoLabel.setBounds (r.removeFromTop (24));
    r.removeFromTop (8);
    auto row = [&] (juce::Label& l, juce::Slider& s)
    {
        auto a = r.removeFromTop (30);
        l.setBounds (a.removeFromLeft (90));
        s.setBounds (a);
    };
    row (thrL, thr); row (minL, minNote); row (trL, trans);
    r.removeFromTop (12);
    auto b = r.removeFromTop (36);
    const int w = b.getWidth() / 3;
    recBtn.setBounds (b.removeFromLeft (w).reduced (3));
    clearBtn.setBounds (b.removeFromLeft (w).reduced (3));
    saveBtn.setBounds (b.reduced (3));
    r.removeFromTop (12);
    dragZone.setBounds (r.removeFromTop (60));
}

void AudioToMidiEditor::timerCallback()
{
    proc.drainEvents();
    const int n = proc.getCurrentNote();
    noteLabel.setText (n >= 0 ? juce::MidiMessage::getMidiNoteName (n, true, true, 4) : "-", juce::dontSendNotification);
    infoLabel.setText ((proc.isRecording() ? "Recording - " : "") + juce::String (proc.getNumRecordedNotes()) + " notes",
                       juce::dontSendNotification);
}
