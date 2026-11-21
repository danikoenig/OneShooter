try:
    from lib import install_text
except ImportError:
    pass

CONTENT = {
"title": "ONE SHOOTER", "file": "OneShooter", "repo": "OneShooter",
"accent": "#ff8a3d", "accent2": "#f0485a",
"en": {
 "subtitle": "One-shot sample player",
 "kind": "Instrument for macOS (Apple Silicon)  \xb7  VST3, Audio Unit and CLAP",
 "by": "by Dani Koenig",
 "intro": ("One Shooter plays one sample, like a drum sampler. Drop a kick, snare, hit or chord on it, shape it with pitch envelope, amp envelope, "
           "sub oscillator, gate, tone and clipper, and play it from your keyboard or a pattern. A library indexes whole folders of one-shots so you can audition them quickly. "
           "Inspired by Ableton's Drum Sampler."),
 "note": "Made for kicks and 808s: pitch drop, sub, clipper with Auto Gain and a limiter are all on one panel.",
 "blocks": [],
 "quick_title": "Quick start",
 "quick": [
   ("1. Load", "Drag a sample onto the plugin or press <b>Load</b>. WAV, AIFF, FLAC, MP3, OGG, M4A and CAF work."),
   ("2. Trim", "Drag the start and end markers on the waveform. Double-click the waveform for the whole sample."),
   ("3. Play", "Play C3 or the root note. In drum mode every key plays the same pitch. With the keyboard button on, the pitch follows the key."),
   ("4. Shape", "Use the pitch envelope for the kick drop, the sub for weight, then the clipper and limiter for level."),
 ],
 "first": "For a kick: Mono on, pitch envelope amount +12 with a short decay, sub on at 50 Hz, clipper drive at 30 % with Auto Gain on.",
 "sections": [
  ("#ff7a2f", "SAMPLE AND ROOT", [
    ("Waveform", "Drag the start and end markers. Double-click resets to the whole sample."),
    ("Note-value grid", "The waveform is labelled in note values (1/16, 1/8, 3/16, 1/4 ...) from the start marker at your DAW tempo. Transpose and pitch envelope are included."),
    ("Length card", "Shows the nearest note value, a one-bar 16th grid, beats and milliseconds."),
    ("ROOT", "Detected when you load the sample, or taken from the key in the file name (for example Chord_Hit_Dm). Click the ROOT pill to choose it, double-click to type it (F#2)."),
    ("Chromatic (keyboard button)", "On: pitch follows the MIDI key and the root key plays the original pitch. Off: every key plays the same pitch (drum mode)."),
    ("MONO / POLY", "Mono: a new hit chokes the previous one (kicks, 808s). Poly: notes ring together (chords)."),
  ]),
  ("#e0a800", "ENVELOPES AND SUB", [
    ("Pitch envelope", "AMOUNT (plus or minus 48 semitones) and DECAY. Works on the sample and the sub."),
    ("Amp envelope", "ATTACK, HOLD, DECAY, RELEASE. Release Off = pure one-shot."),
    ("SUB", "Sub oscillator with volume, TUNE (in Hz with note name) and DECAY."),
    ("GATE", "Cuts every hit to a length synced to the DAW tempo (Off, 1/32 to 1 bar), with FADE and CURVE."),
  ]),
  ("#1fb3c8", "TONE AND OUTPUT", [
    ("TILT", "Darker to brighter in one move."),
    ("LOW CUT / HIGH CUT", "Filters at the ends of the spectrum."),
    ("CLIPPER", "Knack-style clipper: DRIVE, soft or hard, 4x oversampling, gain reduction meter."),
    ("AUTO GAIN", "Keeps the loudness the same, so you hear the saturation and not just more level."),
    ("LIMITER", "On the output, ceiling -1 dB, with gain reduction meter."),
  ]),
  ("#8a6cf0", "LIBRARY", [
    ("Add folders", "Drop a folder on the plugin. All one-shots inside, including subfolders, are indexed. Your files are not moved or copied."),
    ("Browse", "Each folder shows all its samples as one flat list. Search across everything."),
    ("Audition", "Click a sample or use the arrow keys. Prev and next buttons are in the header."),
    ("Show or hide", "The button next to the logo."),
    ("Index file", "Stored in ~/Library/Application Support/One Shooter/library.json."),
  ]),
 ],
 "recipes": [
   ("Punchy kick", "Mono, pitch envelope +12 st / short decay, amp decay short, sub 50 Hz, clipper 30 %, Auto Gain on."),
   ("Long 808", "Mono, sub tune to the song key, amp decay long, release Off, pitch envelope small and fast."),
   ("Chord hit", "Poly, chromatic on, set ROOT to the chord key. Play it across the keyboard."),
   ("Tight snare", "GATE 1/16 with a short fade, low cut 120 Hz, clipper 20 %."),
 ],
 "tips": [
   "All parameters can be automated. The sample path is saved with the project.",
   "Samples are stored by file path. If you move a file, load it again.",
   "The decay envelope ends at -60 dB.",
   "Quad Shooter is the 4-slot sister plugin with XY mix and an FX chain.",
 ],
 "install": "",
},
"de": {
 "subtitle": "One-Shot-Sample-Player",
 "kind": "Instrument für macOS (Apple Silicon)  \xb7  VST3, Audio Unit und CLAP",
 "by": "von Dani Koenig",
 "intro": ("One Shooter spielt ein Sample, wie ein Drum-Sampler. Zieh eine Kick, Snare, einen Hit oder Akkord darauf, forme ihn mit Pitch-Hüllkurve, Amp-Hüllkurve, "
           "Sub-Oszillator, Gate, Tone und Clipper und spiele ihn mit der Tastatur oder einem Pattern. Eine Library indexiert ganze Ordner voller One-Shots, damit du schnell durchhören kannst. "
           "Inspiriert vom Drum Sampler von Ableton."),
 "note": "Gebaut für Kicks und 808s: Pitch-Drop, Sub, Clipper mit Auto Gain und Limiter liegen auf einem Panel.",
 "blocks": [],
 "quick_title": "Schnellstart",
 "quick": [
   ("1. Laden", "Ein Sample auf das Plugin ziehen oder <b>Load</b> drücken. WAV, AIFF, FLAC, MP3, OGG, M4A und CAF funktionieren."),
   ("2. Schneiden", "Start- und Endmarker in der Wellenform ziehen. Doppelklick auf die Wellenform = ganzes Sample."),
   ("3. Spielen", "C3 oder die Root-Note spielen. Im Drum-Modus spielt jede Taste dieselbe Tonhöhe. Mit der Keyboard-Taste folgt die Tonhöhe der Taste."),
   ("4. Formen", "Pitch-Hüllkurve für den Kick-Drop, Sub für Gewicht, dann Clipper und Limiter für den Pegel."),
 ],
 "first": "Für eine Kick: Mono an, Pitch-Hüllkurve Amount +12 mit kurzem Decay, Sub an bei 50 Hz, Clipper Drive 30 % mit Auto Gain.",
 "sections": [
  ("#ff7a2f", "SAMPLE UND ROOT", [
    ("Wellenform", "Start- und Endmarker ziehen. Doppelklick setzt auf das ganze Sample zurück."),
    ("Notenwert-Raster", "Die Wellenform ist ab dem Startmarker in Notenwerten (1/16, 1/8, 3/16, 1/4 ...) im DAW-Tempo beschriftet. Transpose und Pitch-Hüllkurve sind eingerechnet."),
    ("Length-Karte", "Zeigt den nächsten Notenwert, ein 16tel-Raster für einen Takt, Beats und Millisekunden."),
    ("ROOT", "Wird beim Laden erkannt oder aus der Taste im Dateinamen gelesen (z.B. Chord_Hit_Dm). ROOT-Feld anklicken zum Wählen, doppelklicken zum Eintippen (F#2)."),
    ("Chromatic (Keyboard-Taste)", "An: Die Tonhöhe folgt der MIDI-Taste, die Root-Taste spielt die Originaltonhöhe. Aus: Jede Taste spielt dieselbe Tonhöhe (Drum-Modus)."),
    ("MONO / POLY", "Mono: Ein neuer Hit würgt den vorigen ab (Kicks, 808s). Poly: Noten klingen zusammen aus (Akkorde)."),
  ]),
  ("#e0a800", "HÜLLKURVEN UND SUB", [
    ("Pitch-Hüllkurve", "AMOUNT (plus oder minus 48 Halbtöne) und DECAY. Wirkt auf Sample und Sub."),
    ("Amp-Hüllkurve", "ATTACK, HOLD, DECAY, RELEASE. Release Off = reiner One-Shot."),
    ("SUB", "Sub-Oszillator mit Lautstärke, TUNE (in Hz mit Notenname) und DECAY."),
    ("GATE", "Schneidet jeden Hit auf eine Länge im DAW-Tempo (Off, 1/32 bis 1 Takt), mit FADE und CURVE."),
  ]),
  ("#1fb3c8", "TONE UND OUTPUT", [
    ("TILT", "Dunkler bis heller in einer Bewegung."),
    ("LOW CUT / HIGH CUT", "Filter an den Enden des Spektrums."),
    ("CLIPPER", "Clipper im Knack-Stil: DRIVE, soft oder hard, 4x Oversampling, Gain-Reduction-Anzeige."),
    ("AUTO GAIN", "Hält die Lautheit gleich, damit du die Sättigung hörst und nicht nur mehr Pegel."),
    ("LIMITER", "Am Ausgang, Ceiling -1 dB, mit Gain-Reduction-Anzeige."),
  ]),
  ("#8a6cf0", "LIBRARY", [
    ("Ordner hinzufügen", "Einen Ordner auf das Plugin ziehen. Alle One-Shots darin, auch aus Unterordnern, werden indexiert. Deine Dateien werden nicht verschoben oder kopiert."),
    ("Durchsuchen", "Jeder Ordner zeigt alle Samples als flache Liste. Suche über alles."),
    ("Anhören", "Sample anklicken oder mit den Pfeiltasten wechseln. Prev- und Next-Tasten sind im Header."),
    ("Ein- und ausblenden", "Mit der Taste neben dem Logo."),
    ("Index-Datei", "Gespeichert in ~/Library/Application Support/One Shooter/library.json."),
  ]),
 ],
 "recipes": [
   ("Knackige Kick", "Mono, Pitch-Hüllkurve +12 st / kurzer Decay, Amp-Decay kurz, Sub 50 Hz, Clipper 30 %, Auto Gain an."),
   ("Lange 808", "Mono, Sub-Tune auf die Tonart des Songs, Amp-Decay lang, Release Off, Pitch-Hüllkurve klein und schnell."),
   ("Akkord-Hit", "Poly, Chromatic an, ROOT auf die Akkord-Taste setzen. Über die Tastatur spielen."),
   ("Straffe Snare", "GATE 1/16 mit kurzem Fade, Low Cut 120 Hz, Clipper 20 %."),
 ],
 "tips": [
   "Alle Parameter lassen sich automatisieren. Der Sample-Pfad wird mit dem Projekt gespeichert.",
   "Samples werden per Dateipfad gespeichert. Wenn du eine Datei verschiebst, lade sie neu.",
   "Die Decay-Hüllkurve endet bei -60 dB.",
   "Quad Shooter ist das Schwester-Plugin mit 4 Slots, XY-Mix und FX-Kette.",
 ],
 "install": "",
},
}
for _l in ("en", "de"):
    CONTENT[_l]["install"] = install_text(_l, "OneShooter", "OneShooter", "One Shooter")
