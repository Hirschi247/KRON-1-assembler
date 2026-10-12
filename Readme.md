# KRON-1 Assembler

Zwei-Pass-Assembler für die KRON-1-CPU. Er liest eine Assembler-Quelldatei (`.asm`) ein und
erzeugt daraus eine Speicherdatei (`.mem`) mit einem 36-Bit-Befehlswort pro Zeile.

- **Pass 1** sammelt alle Labels und deren Befehlsadressen in der Symboltabelle.
- **Pass 2** löst Label-Referenzen auf, kodiert jede Befehlszeile und schreibt sie in die Ausgabedatei.

Die eigentliche Logik (Scanner, Encoder, Symboltabelle) liegt in der Bibliothek **asmcore**,
die Befehlsdefinitionen (Opcodes, Bitfelder) in der Bibliothek **isa**. Dieses Repository
enthält nur das Kommandozeilenprogramm.

---

## Benutzung

```
assembler <input-file> <output-file>
```

| Argument        | Bedeutung                                                                 |
|-----------------|---------------------------------------------------------------------------|
| `<input-file>`  | Pfad zur Assembler-Quelldatei, die übersetzt werden soll (z. B. `input.asm`). |
| `<output-file>` | Pfad zur Ausgabedatei. Wird neu angelegt bzw. **überschrieben**, falls sie schon existiert (z. B. `output.mem`). |

Beide Argumente sind Pflicht und müssen in genau dieser Reihenfolge angegeben werden. Fehlt eins,
gibt der Assembler eine Usage-Meldung aus und beendet sich mit Exit-Code `1`.

Relative Pfade werden relativ zum **aktuellen Arbeitsverzeichnis** aufgelöst – nicht relativ
zum Ort der `assembler.exe`.

### Beispiele

Windows (PowerShell), aus dem Build-Ordner heraus:

```powershell
.\assembler.exe ..\input.asm ..\output.mem
```

Linux / macOS:

```bash
./assembler input.asm output.mem
```

Mit absoluten Pfaden:

```powershell
.\assembler.exe C:\Projekte\programs\blink.asm C:\Projekte\programs\blink.mem
```

### In CLion ausführen

Unter *Run → Edit Configurations…* bei **Program arguments** z. B. `input.asm output.mem` eintragen
und als **Working directory** `$ProjectFileDir$` setzen. Ohne Working Directory sucht das Programm
die Dateien in `cmake-build-debug/`.

---

## Ausgabeformat

Pro Befehl wird eine Zeile mit **9 Hex-Ziffern** (36 Bit, Großbuchstaben, ohne `0x`) geschrieben.
Leerzeilen, reine Kommentarzeilen und reine Label-Zeilen erzeugen keine Ausgabe. Die Zeilennummer
in der `.mem`-Datei (ab 0) entspricht damit der Befehlsadresse.

Aus

```asm
        LI    R1, 0            ; Zähler
        LI    R2, 1            ; Konstante 1
loop:   ADD   R1, R1, R2       ; Zähler += 1
        STORE R1, 0xF00(R0)    ; Zähler ans LED-Register
        JMP   loop
```

wird

```
408200000
408400001
...
```

Das Format ist kompatibel zu `$readmemh` in Verilog und kann direkt vom Simulator geladen werden.

---

## Syntax der Quelldatei

| Element      | Schreibweise                                  | Hinweis |
|--------------|-----------------------------------------------|---------|
| Mnemonic     | `ADD`, `LI`, `STORE`, `JMP`, `NOP`, `HALT`    | **Großbuchstaben** – `add` wird nicht erkannt. |
| Register     | `R0` … `R31`                                  | **Großes `R`** – `r3` ist falsch, `R3` richtig. `R0` = 0, `R30` = SP, `R31` = Link. |
| Zahl dezimal | `42`                                          | Keine negativen Zahlen. |
| Zahl hex     | `0xF00`                                       | Präfix mit kleinem `x`. |
| Label        | `loop:`                                       | Definiert am Zeilenanfang, darf mit einem Befehl in derselben Zeile stehen. |
| Label-Ref.   | `JMP loop`                                    | Ohne Doppelpunkt; wird durch die Befehlsadresse ersetzt. |
| Speicher     | `STORE R1, 0xF00(R0)`                         | `offset(basisregister)` |
| Kommentar    | `; Text`                                      | Bis zum Zeilenende. |

> **Hinweis:** Labels dürfen nicht genau wie ein Register heißen (`R0` … `R31`). Namen wie
> `restart:` oder `r2d2:` sind dagegen erlaubt.
>
> Labels bestehen nur aus Buchstaben und Ziffern und beginnen mit einem Buchstaben. Unterstriche
> und andere Sonderzeichen sind nicht erlaubt: `my_loop:` führt zu `Invalid character '_'`,
> stattdessen z. B. `myLoop:` verwenden. Label-Namen werden nach 31 Zeichen abgeschnitten.

### Unterstützte Befehle

| Befehl  | Form                     | Beschreibung |
|---------|--------------------------|--------------|
| `ADD`   | `ADD rd, rs1, rs2`       | `rd = rs1 + rs2` |
| `ADDI`  | `ADDI rd, rs1, imm16`    | `rd = rs1 + imm16` |
| `SUB`   | `SUB rd, rs1, rs2`       | `rd = rs1 - rs2` |
| `SUBI`  | `SUBI rd, rs1, imm16`    | `rd = rs1 - imm16` |
| `LI`    | `LI rd, imm16`           | Lädt einen 16-Bit-Wert (oder eine Label-Adresse) in `rd` |
| `LOAD`  | `LOAD rd, offset(rb)`    | Lädt `rd` von Adresse `rb + offset` |
| `STORE` | `STORE rs, offset(rb)`   | Speichert `rs` an Adresse `rb + offset` |
| `JMP`   | `JMP addr` / `JMP label` | Unbedingter Sprung (12-Bit-Adresse) |
| `JMPR`  | `JMPR rx`                | Unbedingter Sprung an die Adresse in `rx` (z. B. Rücksprung über `R31`) |
| `NOP`   | `NOP`                    | Keine Operation |
| `HALT`  | `HALT`                   | Hält die CPU an |

Immediates werden nicht vorzeichenerweitert. Ob `rs2` oder `imm16` verwendet wird, bestimmt das
Mnemonic (`ADD` vs. `ADDI`), nicht der Operand.

Die vollständige Spezifikation der Befehlsformate steht in
[KRON-ISA.md](https://github.com/Hirschi247/kron-1-isa/blob/master/KRON-ISA.md). Welche Befehle der
Assembler tatsächlich kennt, ist in [`isa.c`](https://github.com/Hirschi247/kron-1-isa/blob/master/isa.c)
(`isa_table`) im ISA-Repo festgelegt.

### Fehlermeldungen

| Meldung                              | Ursache |
|--------------------------------------|---------|
| `Error while opening input file`     | Eingabedatei existiert nicht oder Pfad falsch. |
| `Error while creating output file`   | Ausgabeordner existiert nicht oder keine Schreibrechte. |
| `Line <n>: Label <name> not found`   | Ein referenziertes Label ist nirgends definiert (oder ein Register mit kleinem `r` geschrieben). |
| `Line <n>: Unknown mnemonic: <name>` | Befehl steht nicht in der ISA-Tabelle (oder ist kleingeschrieben). |
| `Line <n>: Line does not start with a mnemonic` | Z. B. ein Register oder eine Zahl am Zeilenanfang. |
| `Line <n>: Invalid character '<c>'`  | Zeichen, das der Scanner nicht kennt (z. B. `#` oder `_`). |
| `Line <n>: Invalid register R<x>`    | Registernummer größer als `R31`. |
| `Line <n>: Too many operands …`      | Mehr als 8 Tokens in einer Zeile. |

Alle Meldungen mit `Line <n>` geben die Zeilennummer in der Quelldatei an (ab 1).

---

## Entwicklung

### Abhängigkeiten

Der Assembler besteht aus drei separaten Git-Repositories, die **alle** ausgecheckt sein müssen:

| Repository | Inhalt |
|------------|--------|
| [assembler](https://github.com/Hirschi247/assembler) | Dieses Repo – Kommandozeilenprogramm (`main.c`) |
| [asmcore](https://github.com/Hirschi247/asmcore)     | Scanner, Encoder, Symboltabelle, Dictionary (statische Bibliothek) |
| [kron-1-isa](https://github.com/Hirschi247/kron-1-isa) | ISA-Definition: Bitfelder, Opcodes, Befehlstabelle (statische Bibliothek) |

Abhängigkeitskette: `assembler` → `asmcore` → `isa`

Die `CMakeLists.txt` bindet die beiden Bibliotheken über **feste relative Pfade** ein:

```cmake
add_subdirectory(../../isa isa)
add_subdirectory(../asmcore asmcore)
```

Die Repos müssen deshalb genau in dieser Ordnerstruktur liegen:

```
kron-1/                 ← beliebiger Name
├── isa/                ← kron-1-isa
└── Assembler/
    ├── asmcore/        ← asmcore
    └── assembler/      ← dieses Repo
```

### Einrichten

```bash
mkdir kron-1 && cd kron-1
git clone https://github.com/Hirschi247/kron-1-isa.git isa
mkdir Assembler && cd Assembler
git clone https://github.com/Hirschi247/asmcore.git
git clone https://github.com/Hirschi247/assembler.git
```

Wichtig: Das ISA-Repo muss als Ordner `isa` geklont werden (daher das `isa` am Ende des
`git clone`-Befehls), sonst findet CMake es nicht.

### Bauen

Voraussetzungen: C-Compiler mit C11-Unterstützung (GCC, Clang oder MSVC) und **CMake ≥ 3.16**.

```bash
cd kron-1/Assembler/assembler
cmake -S . -B build
cmake --build build
```

Die ausführbare Datei liegt danach unter `build/assembler` bzw. `build/assembler.exe`
(bei Multi-Config-Generatoren wie Visual Studio unter `build/Debug/`).

In CLion einfach den Ordner `Assembler/assembler` öffnen – `isa` und `asmcore` werden automatisch
mitgebaut und erscheinen im Projektbaum.

### Neuen Befehl hinzufügen

1. Eintrag in `isa_table` in `isa/isa.c` ergänzen (Name, Formatklasse, Prefix, Controls,
   Subopcode – Details in der [ISA-README](https://github.com/Hirschi247/kron-1-isa#neuen-befehl-eintragen)).
2. Falls eine neue Formatklasse nötig ist: in `isa/isa.h` (`enum Formatclass`) anlegen und den
   passenden Encoder in `asmcore/encode/encode.c` implementieren.
3. Assembler neu bauen – `main.c` muss dafür nicht angepasst werden.

Da Änderungen meist mehrere Repos betreffen, jeweils im betroffenen Repo committen und pushen.

---

## Lizenz

[MIT](LICENSE) © 2026 Simon Hirsch
