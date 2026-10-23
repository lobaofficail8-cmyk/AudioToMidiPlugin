#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

AudioToMidiProcessor::AudioToMidiProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    thresholdP = apvts.getRawParameterValue ("threshold");
    minNoteP   = apvts.getRawParameterValue ("minNote");
    transposeP = apvts.getRawParameterValue ("transpose");
}

juce::AudioProcessorValueTreeState::ParameterLayout AudioToMidiProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "threshold", 1 }, "Threshold",
               NormalisableRange<float> (-70.0f, -10.0f, 0.1f), -45.0f,
               AudioParameterFloatAttributes().withLabel ("dB")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "minNote", 1 }, "Min Note",
               NormalisableRange<float> (20.0f, 300.0f, 1.0f), 70.0f,
               AudioParameterFloatAttributes().withLabel ("ms")));
    l.add (std::make_unique<AudioParameterInt> (ParameterID { "transpose", 1 }, "Transpose", -24, 24, 0));
    return l;
}

void AudioToMidiProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    ring.fill (0.0f);
    writePos = hopCounter = 0;
    candidate = -1; stable = 0; currentNote = -1;
    currentNoteDisplay.store (-1);
}

bool AudioToMidiProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet();
    const auto out = l.getMainOutputChannelSet();
    return in == out && (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
}

void AudioToMidiProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    midi.clear();

    const bool rec = recording.load();
    if (rec && ! wasRecording)
    {
        recordOrigin = samplePosition;
        double bpm = 120.0;
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
                if (auto b = pos->getBpm())
                    bpm = *b;
        recordBpm.store (bpm);
    }
    wasRecording = rec;

    const int numCh = buffer.getNumChannels();
    const int n = buffer.getNumSamples();
    if (numCh > 0)
    {
        for (int i = 0; i < n; ++i)
        {
            float s = 0.0f;
            for (int ch = 0; ch < numCh; ++ch)
                s += buffer.getReadPointer (ch)[i];
            ring[(size_t) writePos] = s / (float) numCh;
            writePos = (writePos + 1) & (kWindow - 1);

            if (++hopCounter >= kHop)
            {
                hopCounter = 0;
                analyseHop (i, midi);
            }
        }
    }
    samplePosition += n; // audio passes through unchanged
}

float AudioToMidiProcessor::detectPitch()
{
    const int N = kWindow;
    const int minLag = juce::jmax (2, (int) (sr / 1000.0));
    const int maxLag = juce::jmin ((int) (sr / 80.0), N / 2);
    if (maxLag <= minLag + 2) return 0.0f;

    const int len = N - maxLag;
    double e0 = 0.0, eLag = 0.0;
    for (int i = 0; i < len; ++i)
    {
        e0 += (double) analysis[(size_t) i] * analysis[(size_t) i];
        eLag += (double) analysis[(size_t) (i + minLag)] * analysis[(size_t) (i + minLag)];
    }
    if (e0 < 1.0e-9) return 0.0f;

    float best = 0.0f;
    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        double s = 0.0;
        for (int i = 0; i < len; ++i)
            s += (double) analysis[(size_t) i] * analysis[(size_t) (i + lag)];
        const float r = (float) (s / std::sqrt (juce::jmax (0.0, e0 * eLag) + 1.0e-12));
        acf[(size_t) lag] = r;
        best = juce::jmax (best, r);
        if (len + lag < N)
        {
            const double a = analysis[(size_t) (len + lag)], b = analysis[(size_t) lag];
            eLag += a * a - b * b;
        }
    }
    if (best < 0.6f) return 0.0f;

    for (int lag = minLag + 1; lag < maxLag; ++lag)
    {
        const float a = acf[(size_t) (lag - 1)], b = acf[(size_t) lag], c = acf[(size_t) (lag + 1)];
        if (b > a && b >= c && b >= 0.9f * best)
        {
            const float d = a - 2.0f * b + c;
            const float p = d != 0.0f ? (float) lag + 0.5f * (a - c) / d : (float) lag;
            return (float) sr / p;
        }
    }
    return 0.0f;
}

void AudioToMidiProcessor::analyseHop (int offset, juce::MidiBuffer& midi)
{
    for (int k = 0; k < kWindow; ++k)
        analysis[(size_t) k] = ring[(size_t) ((writePos + k) & (kWindow - 1))];

    double sumSq = 0.0;
    for (float v : analysis) sumSq += (double) v * v;
    const float db = juce::Decibels::gainToDecibels ((float) std::sqrt (sumSq / kWindow), -100.0f);
    const float thr = thresholdP->load();

    int measured = -1;
    if (db > thr)
    {
        const float hz = detectPitch();
        if (hz > 0.0f)
            measured = juce::roundToInt (69.0f + 12.0f * std::log2 (hz / 440.0f));
    }

    const int required = juce::jmax (1, (int) std::round (minNoteP->load() * 0.001 * sr / kHop));
    if (measured == candidate) ++stable; else { candidate = measured; stable = 1; }

    if (candidate >= 0 && stable >= required)
    {
        const int n = juce::jlimit (0, 127, candidate + (int) transposeP->load());
        if (n != currentNote)
        {
            const int late = kWindow / 2 + (required - 1) * kHop;
            if (currentNote >= 0) emit (midi, offset, currentNote, 0, false, late);
            const int vel = juce::jlimit (30, 127, (int) juce::jmap (db, thr, -6.0f, 40.0f, 127.0f));
            emit (midi, offset, n, vel, true, late);
            currentNote = n;
        }
    }
    else if (candidate < 0 && stable >= 2 && currentNote >= 0)
    {
        emit (midi, offset, currentNote, 0, false, kWindow / 2 + kHop);
        currentNote = -1;
    }
}

void AudioToMidiProcessor::emit (juce::MidiBuffer& mb, int offset, int note, int vel, bool on, int late)
{
    mb.addEvent (on ? juce::MidiMessage::noteOn (1, note, (juce::uint8) vel)
                    : juce::MidiMessage::noteOff (1, note), offset);
    currentNoteDisplay.store (on ? note : -1);
    if (recording.load())
        pushEvent ({ samplePosition + offset - recordOrigin - late, note, vel, on });
}

void AudioToMidiProcessor::pushEvent (const NoteEvent& e)
{
    int s1, n1, s2, n2;
    fifo.prepareToWrite (1, s1, n1, s2, n2);
    if (n1 > 0) fifoData[(size_t) s1] = e;
    else if (n2 > 0) fifoData[(size_t) s2] = e;
    else return;
    fifo.finishedWrite (1);
}

void AudioToMidiProcessor::drainEvents()
{
    while (fifo.getNumReady() > 0)
    {
        int s1, n1, s2, n2;
        fifo.prepareToRead (1, s1, n1, s2, n2);
        recorded.push_back (n1 > 0 ? fifoData[(size_t) s1] : fifoData[(size_t) s2]);
        fifo.finishedRead (1);
    }
}

int AudioToMidiProcessor::getNumRecordedNotes() const
{
    int c = 0;
    for (auto& e : recorded) if (e.on) ++c;
    return c;
}

bool AudioToMidiProcessor::writeMidiFile (const juce::File& f) const
{
    if (recorded.empty()) return false;
    const int tpq = 960;
    const double bpm = recordBpm.load();
    auto ticks = [&] (juce::int64 s) { return juce::jmax (0.0, (double) s / sr * bpm / 60.0 * tpq); };

    juce::MidiMessageSequence seq;
    seq.addEvent (juce::MidiMessage::tempoMetaEvent (juce::roundToInt (60000000.0 / bpm)), 0.0);

    int open = -1;
    double last = 0.0;
    for (auto& e : recorded)
    {
        const double t = ticks (e.samplePos);
        last = juce::jmax (last, t);
        if (e.on)
        {
            if (open >= 0) seq.addEvent (juce::MidiMessage::noteOff (1, open), t);
            seq.addEvent (juce::MidiMessage::noteOn (1, e.note, (juce::uint8) e.velocity), t);
            open = e.note;
        }
        else if (open == e.note)
        {
            seq.addEvent (juce::MidiMessage::noteOff (1, e.note), t);
            open = -1;
        }
    }
    if (open >= 0) seq.addEvent (juce::MidiMessage::noteOff (1, open), last + tpq / 4);
    seq.updateMatchedPairs();

    juce::MidiFile mf;
    mf.setTicksPerQuarterNote (tpq);
    mf.addTrack (seq);

    f.deleteFile();
    juce::FileOutputStream out (f);
    if (! out.openedOk()) return false;
    return mf.writeTo (out);
}

void AudioToMidiProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void AudioToMidiProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* AudioToMidiProcessor::createEditor() { return new AudioToMidiEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AudioToMidiProcessor(); }
