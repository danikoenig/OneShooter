# Kick Sampler

VST3-Instrument für macOS (Apple Silicon), inspiriert von Abletons Drum Sampler. Gebaut für One-Shot-Kicks.

## Funktionen

- Sample laden per Drag & Drop oder Button (WAV, AIFF, FLAC, MP3, OGG, M4A, CAF)
- Waveform mit Start- und End-Marker (ziehen, Doppelklick = ganzes Sample)
- Beat-Raster in der Waveform (16tel, Beats, Takte) nach DAW-Tempo, inkl. Transpose
- Anzeige der Kick-Länge in Takten, Beats, 16teln, ms und nächstem Notenwert
- Erkennt Tempo und Taktart der DAW automatisch
- Parameter: Start, Length, Transpose (±48 st), Attack, Hold, Decay, Velocity, Gain
- Jede MIDI-Note triggert das Sample (Drum-Modus). Ein neuer Hit würgt den vorherigen
  sauber ab (4 ms Fade), wie bei einer Drum-Machine.
- Pitch-Envelope (Amount, Decay) wirkt auf Sample und Sub
- Release beim Note-Off (Standard: Off = One-Shot)
- Sub-Oszillator: Volume, Tune (Hz + Notenname), Decay
- Clipper im KNOCK-Stil: Drive, Soft/Hard, 4x Oversampling, Gain-Reduction-Meter
- Library: Ordner aufs Plugin ziehen. Alle One-Shots (auch in Unterordnern) werden erkannt
  und indexiert. Suche, Vorhoeren per Klick oder Pfeiltasten. Index liegt unter
  `~/Library/Application Support/Kick Sampler/library.json`
- Alle Parameter automatisierbar, Sample-Pfad wird im Projekt gespeichert

## Bauen über GitHub

1. Auf github.com ein neues Repository anlegen (z. B. `kick-sampler`).
2. Den Inhalt dieses Ordners hochladen (inkl. des versteckten Ordners `.github`).
   Am einfachsten im Terminal:
   ```
   cd KickSampler
   git init
   git add .
   git commit -m "Kick Sampler"
   git branch -M main
   git remote add origin https://github.com/DEIN-NAME/kick-sampler.git
   git push -u origin main
   ```
3. Im Repo auf **Actions** gehen. Der Build läuft automatisch (ca. 5–10 Min).
4. Im fertigen Lauf unten unter **Artifacts** `KickSampler-macOS-arm64` herunterladen.

Für ein Release mit Download-Link: `git tag v1.0.0 && git push --tags`.

## Installieren

1. ZIP entpacken.
2. `Kick Sampler.vst3` nach `~/Library/Audio/Plug-Ins/VST3/` kopieren.
3. Da das Plugin nicht von Apple notarisiert ist, einmal im Terminal:
   ```
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"Kick Sampler.vst3"
   ```
4. In der DAW die Plugins neu scannen.

## Lokal bauen (optional)

Voraussetzung: Xcode Command Line Tools und CMake (`brew install cmake`).

```
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build --target KickSampler_VST3 -j 8
```

Ergebnis: `build/KickSampler_artefacts/Release/VST3/Kick Sampler.vst3`

## Hinweise

- Das Sample wird über den Dateipfad gespeichert. Wird die Datei verschoben, muss sie neu geladen werden.
- Angezeigte Kick-Länge = kürzerer Wert aus Sample-Bereich (inkl. Transpose) und Envelope (Attack + Hold + Decay).
- Decay endet bei -60 dB.
- JUCE wird beim Build automatisch geladen (Version 8.0.4, AGPLv3/kommerzielle Lizenz).
