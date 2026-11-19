# One Shooter

VST3-Instrument für macOS (Apple Silicon), inspiriert von Abletons Drum Sampler. Gebaut für One-Shots, vor allem Kicks.

## Funktionen

- Sample laden per Drag & Drop oder Button (WAV, AIFF, FLAC, MP3, OGG, M4A, CAF)
- Waveform mit Start- und End-Marker (ziehen, Doppelklick = ganzes Sample)
- Raster in der Waveform mit Notenwerten ab Start (1/16, 1/8, 3/16, 1/4 ...) nach DAW-Tempo, inkl. Transpose und Pitch-Envelope
- Längen-Anzeige als Notenwert, 16tel-Raster eines Takts, Beats und ms
- Erkennt Tempo und Taktart der DAW automatisch
- Parameter: Start, Length, Transpose (±48 st), Attack, Hold, Decay, Velocity, Gain
- Chromatic-Schalter (Keyboard-Symbol): Tonhöhe folgt der MIDI-Taste, C3 = Originalhöhe.
  Aus: jede MIDI-Note triggert das Sample gleich (Drum-Modus). Ein neuer Hit würgt den vorherigen
  sauber ab (4 ms Fade), wie bei einer Drum-Machine.
- Pitch-Envelope (Amount, Decay) wirkt auf Sample und Sub
- Release beim Note-Off (Standard: Off = One-Shot)
- Sub-Oszillator: Volume, Tune (Hz + Notenname), Decay
- Clipper im KNOCK-Stil: Drive, Soft/Hard, 4x Oversampling, Gain-Reduction-Meter
- Duck (wie Kickstart): kürzt jeden Hit auf eine Länge synchron zum DAW-Tempo
  (Off, 1/32 bis 1 bar), mit Fade und Curve. Wirkt auf Sample und Sub.
- Library: ein- und ausblendbar (Schalter oben links). Ordner aufs Plugin ziehen. Alle One-Shots (auch in Unterordnern) werden erkannt
  und indexiert. Pro importiertem Ordner werden alle Samples flach angezeigt (Unterordner aufgelöst). Suche über alles, Vorhören per Klick oder Pfeiltasten,
  Prev/Next-Pfeile im Header. Samples werden nicht kopiert oder verschoben. Index liegt unter
  `~/Library/Application Support/One Shooter/library.json`
- Alle Parameter automatisierbar, Sample-Pfad wird im Projekt gespeichert

## Bauen über GitHub

1. Auf github.com ein neues Repository anlegen (z. B. `one-shooter`).
2. Den Inhalt dieses Ordners hochladen (inkl. des versteckten Ordners `.github`).
   Am einfachsten im Terminal:
   ```
   cd one-shooter
   git init
   git add .
   git commit -m "One Shooter"
   git branch -M main
   git remote add origin https://github.com/DEIN-NAME/one-shooter.git
   git push -u origin main
   ```
3. Im Repo auf **Actions** gehen. Der Build läuft automatisch (ca. 5–10 Min).
4. Im fertigen Lauf unten unter **Artifacts** `OneShooter-macOS-arm64` herunterladen.

Für ein Release mit Download-Link: `git tag v1.0.0 && git push --tags`.

## Installieren

1. ZIP entpacken.
2. `One Shooter.vst3` nach `~/Library/Audio/Plug-Ins/VST3/` kopieren.
3. Da das Plugin nicht von Apple notarisiert ist, einmal im Terminal:
   ```
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"One Shooter.vst3"
   ```
4. In der DAW die Plugins neu scannen.

## Lokal bauen (optional)

Voraussetzung: Xcode Command Line Tools und CMake (`brew install cmake`).

```
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build --target OneShooter_VST3 -j 8
```

Ergebnis: `build/OneShooter_artefacts/Release/VST3/One Shooter.vst3`

## Hinweise

- Das Sample wird über den Dateipfad gespeichert. Wird die Datei verschoben, muss sie neu geladen werden.
- Angezeigte Kick-Länge = kürzerer Wert aus Sample-Bereich (inkl. Transpose) und Envelope (Attack + Hold + Decay).
- Decay endet bei -60 dB.
- JUCE wird beim Build automatisch geladen (Version 8.0.4, AGPLv3/kommerzielle Lizenz).
