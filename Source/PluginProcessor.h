#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include <vector>

struct NoteEvent
{
    juce::int64 samplePos = 0;
    int note = 0;
    int velocity = 0;
    bool on = false;
};

class AudioToMidiProcessor : public juce::AudioProcessor
{
public:
    AudioToMidiProcessor();
    ~AudioToMidiProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Audio To MIDI"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    // ---- message-thread API (used by the editor) ----
    void drainEvents();
    void startRecording() { recording.store (true); }
    void stopRecording()  { recording.store (false); }
    bool isRecording() const { return recording.load(); }
    void clearRecording() { recorded.clear(); }
bool hasRecording() const { return ! recorded.empty(); }
    int  getNumRecordedNotes() const;
    bool writeMidiFile (const juce::File&) const;
    int  getCurrentNote() const { return currentNoteDisplay.load(); }

private:
    static constexpr int kWindow = 2048; // power of two
    static constexpr int kHop = 512;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void analyseHop (int offset, juce::MidiBuffer&);
    float detectPitch();
    void emit (juce::MidiBuffer&, int offset, int note, int vel, bool on, int lateSamples);
    void pushEvent (const NoteEvent&);

    std::atomic<float>* thresholdP = nullptr;
    std::atomic<float>* minNoteP = nullptr;
    std::atomic<float>* transposeP = nullptr;

    double sr = 44100.0;
    std::array<float, kWindow> ring {};
    std::array<float, kWindow> analysis {};
    std::array<float, kWindow / 2 + 8> acf {};
    int writePos = 0, hopCounter = 0;
    int candidate = -1, stable = 0, currentNote = -1;

    juce::int64 samplePosition = 0, recordOrigin = 0;
    bool wasRecording = false;
    std::atomic<bool> recording { false };
    std::atomic<int> currentNoteDisplay { -1 };
    std::atomic<double> recordBpm { 120.0 };

    juce::AbstractFifo fifo { 1024 };
    std::array<NoteEvent, 1024> fifoData {};
    std::vector<NoteEvent> recorded; // message thread only

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioToMidiProcessor)
};
