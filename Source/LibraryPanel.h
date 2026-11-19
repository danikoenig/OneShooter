#pragma once

#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>

// Seitenleiste: One-Shot-Index mit Suche und Vorhoeren
class LibraryPanel : public juce::Component,
                     private juce::ListBoxModel,
                     private juce::ChangeListener
{
public:
    explicit LibraryPanel (KickSamplerProcessor& p);
    ~LibraryPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void refreshStatus();

private:
    // ListBoxModel
    int  getNumRows() override { return (int) filtered.size(); }
    void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
    void selectedRowsChanged (int lastRowSelected) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override;
    void returnKeyPressed (int row) override { audition (row); }

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void rebuildFilter();
    void audition (int row);
    void chooseFolder();

    KickSamplerProcessor& processor;
    std::vector<SampleLibrary::Entry> all, filtered;

    juce::TextEditor search;
    juce::ListBox list { "Library", this };
    juce::TextButton addButton { "+ Ordner" }, clearButton { "Leeren" };
    juce::Label status;
    std::unique_ptr<juce::FileChooser> chooser;
    int lastAuditioned = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LibraryPanel)
};
