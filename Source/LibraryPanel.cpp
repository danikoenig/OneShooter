#include "LibraryPanel.h"
#include "Colours.h"

using namespace KickColours;

LibraryPanel::LibraryPanel (KickSamplerProcessor& p) : processor (p)
{
    search.setTextToShowWhenEmpty ("Suchen (z. B. kick, 909, sub)", textDim);
    search.setFont (juce::FontOptions (13.0f));
    search.setColour (juce::TextEditor::backgroundColourId, inset);
    search.setColour (juce::TextEditor::outlineColourId, cardBorder);
    search.setColour (juce::TextEditor::focusedOutlineColourId, accent);
    search.setColour (juce::TextEditor::textColourId, text);
    search.setIndents (8, 6);
    search.onTextChange = [this] { rebuildFilter(); };
    addAndMakeVisible (search);

    list.setRowHeight (40);
    list.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    list.setColour (juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    list.getViewport()->setScrollBarThickness (6);
    list.setWantsKeyboardFocus (true);
    addAndMakeVisible (list);

    addButton.onClick = [this] { chooseFolder(); };
    clearButton.onClick = [this]
    {
        processor.getLibrary().clear();
    };
    addAndMakeVisible (addButton);
    addAndMakeVisible (clearButton);

    status.setFont (juce::FontOptions (11.0f));
    status.setColour (juce::Label::textColourId, textDim);
    addAndMakeVisible (status);

    processor.getLibrary().addChangeListener (this);
    all = processor.getLibrary().getEntries();
    rebuildFilter();
}

LibraryPanel::~LibraryPanel()
{
    processor.getLibrary().removeChangeListener (this);
}

void LibraryPanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (card);
    g.fillRoundedRectangle (r, 10.0f);
    g.setColour (cardBorder);
    g.drawRoundedRectangle (r.reduced (0.5f), 10.0f, 1.0f);

    g.setColour (accent);
    g.fillEllipse (14.0f, 17.0f, 7.0f, 7.0f);
    g.setColour (text);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText ("LIBRARY", 28, 12, 120, 18, juce::Justification::centredLeft);
    g.setColour (textDim);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText (juce::String ((int) filtered.size()) + " One-Shots", getWidth() - 130, 12, 116, 18,
                juce::Justification::centredRight);

    if (all.empty())
    {
        auto hint = getLocalBounds().reduced (20).withTrimmedTop (90).withTrimmedBottom (70);
        g.setColour (textDim);
        g.setFont (juce::FontOptions (13.0f));
        g.drawFittedText ("Ordner mit Samples hierher ziehen.\n\nDas Plugin sucht alle One-Shots\n(auch in Unterordnern) und\nerstellt einen Index.",
                          hint, juce::Justification::centredTop, 8);
    }
}

void LibraryPanel::resized()
{
    auto r = getLocalBounds().reduced (10);
    r.removeFromTop (28);
    search.setBounds (r.removeFromTop (30));
    r.removeFromTop (8);

    auto bottom = r.removeFromBottom (54);
    status.setBounds (bottom.removeFromBottom (20));
    bottom.removeFromBottom (4);
    addButton.setBounds (bottom.removeFromLeft (bottom.getWidth() / 2 - 4));
    bottom.removeFromLeft (8);
    clearButton.setBounds (bottom);

    r.removeFromBottom (6);
    list.setBounds (r);
}

void LibraryPanel::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (row < 0 || row >= (int) filtered.size())
        return;
    const auto& e = filtered[(size_t) row];
    const bool loaded = e.file == processor.getLoadedFile();

    auto r = juce::Rectangle<float> (0.0f, 1.0f, (float) w - 2.0f, (float) h - 2.0f);
    if (selected)
    {
        g.setColour (accent.withAlpha (0.16f));
        g.fillRoundedRectangle (r, 6.0f);
    }
    else if (row % 2 == 0)
    {
        g.setColour (juce::Colours::white.withAlpha (0.02f));
        g.fillRoundedRectangle (r, 6.0f);
    }

    if (loaded)
    {
        g.setColour (accent);
        g.fillRoundedRectangle (2.0f, 8.0f, 3.0f, (float) h - 16.0f, 1.5f);
    }

    g.setColour (selected || loaded ? text : text.withAlpha (0.85f));
    g.setFont (juce::FontOptions (13.0f, loaded ? juce::Font::bold : juce::Font::plain));
    g.drawText (e.name, 12, 4, w - 76, h / 2 - 2, juce::Justification::bottomLeft, true);

    g.setColour (textDim);
    g.setFont (juce::FontOptions (10.5f));
    g.drawText (e.folder, 12, h / 2 + 3, w - 76, h / 2 - 5, juce::Justification::topLeft, true);

    g.drawText (juce::String (juce::roundToInt (e.seconds * 1000.0)) + " ms", w - 66, 0, 58, h, juce::Justification::centredRight);
}

void LibraryPanel::selectedRowsChanged (int lastRowSelected)
{
    if (lastRowSelected >= 0)
        audition (lastRowSelected);
}

void LibraryPanel::listBoxItemClicked (int row, const juce::MouseEvent&)
{
    // erneuter Klick auf dieselbe Zeile: nochmal abspielen
    if (row == lastAuditioned)
        audition (row);
}

void LibraryPanel::audition (int row)
{
    if (row < 0 || row >= (int) filtered.size())
        return;

    const auto file = filtered[(size_t) row].file;
    if (file != processor.getLoadedFile())
    {
        if (! file.existsAsFile() || ! processor.loadSample (file))
        {
            status.setText ("Datei nicht gefunden: " + file.getFileName(), juce::dontSendNotification);
            return;
        }
    }
    processor.triggerPreview();
    lastAuditioned = row;
    list.repaint();
}

void LibraryPanel::chooseFolder()
{
    chooser = std::make_unique<juce::FileChooser> (juce::String::fromUTF8 ("Sample-Ordner hinzufügen"));
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                          [this] (const juce::FileChooser& fc)
                          {
                              auto dir = fc.getResult();
                              if (dir.isDirectory())
                                  processor.getLibrary().addFolder (dir);
                          });
}

void LibraryPanel::changeListenerCallback (juce::ChangeBroadcaster*)
{
    auto entries = processor.getLibrary().getEntries();
    if (entries.size() != all.size())
    {
        all = std::move (entries);
        rebuildFilter();
    }
    refreshStatus();
}

void LibraryPanel::refreshStatus()
{
    auto& lib = processor.getLibrary();
    if (lib.isScanning())
        status.setText ("Scanne " + lib.getCurrentFolderName() + " ... " + juce::String (lib.getFilesChecked())
                            + " Dateien, " + juce::String (lib.getOneShotsFound()) + " One-Shots",
                        juce::dontSendNotification);
    else
        status.setText (all.empty() ? juce::String ("Kein Index") : juce::String ((int) all.size()) + " One-Shots im Index",
                        juce::dontSendNotification);
}

void LibraryPanel::rebuildFilter()
{
    const auto terms = juce::StringArray::fromTokens (search.getText().toLowerCase(), " ", "");
    filtered.clear();
    for (auto& e : all)
    {
        const auto hay = (e.name + " " + e.folder + " " + e.file.getParentDirectory().getFileName()).toLowerCase();
        bool ok = true;
        for (auto& t : terms)
            if (t.isNotEmpty() && ! hay.contains (t)) { ok = false; break; }
        if (ok)
            filtered.push_back (e);
    }
    lastAuditioned = -1;
    list.updateContent();
    list.deselectAllRows();
    repaint();
}
