# ESP32-WLAN-Steuerung für Vasner-Terrassenheizer

## Erzeugen und Hochladen mit Visual Studio Code

1. Visual Studio Code installieren und starten.
2. In der Erweiterungsansicht nach `PlatformIO IDE` suchen und die offizielle Erweiterung installieren. PlatformIO Core muss nicht zusätzlich installiert werden.
3. In VS Code `Datei > Ordner öffnen` wählen und diesen Ordner `vasner_esp32` öffnen. Nicht nur eine einzelne Quelldatei öffnen.
4. Warten, bis PlatformIO die ESP32-Werkzeuge und die Bibliothek `IRremoteESP8266` installiert hat. Die Abhängigkeiten stehen in `platformio.ini`.
5. `secrets.example.h` als `secrets.h` kopieren und darin `WIFI_SSID`, `WIFI_PASSWORD` und `WEB_PASSWORD` ändern. Ein eigenes, ausreichend langes Web-Passwort verwenden. `secrets.h` wird nicht in Git aufgenommen.
6. Den ESP32 mit einem USB-Datenkabel anschließen. Reine Ladekabel funktionieren nicht zum Programmieren.
7. In der unteren PlatformIO-Leiste auf das Häkchen **Build** klicken. Der erste Durchlauf dauert wegen der Werkzeug-Downloads länger.
8. Danach auf den Pfeil **Upload** klicken. PlatformIO erkennt den seriellen Anschluss normalerweise automatisch.
9. Nach erfolgreichem Upload **Serial Monitor** öffnen. Er läuft mit 115200 Baud und zeigt nach der WLAN-Verbindung die IP-Adresse der Weboberfläche an.
10. Im selben WLAN im Browser `http://<angezeigte-IP-Adresse>` öffnen und mit Benutzer `admin` sowie dem eingestellten Web-Passwort anmelden.

Das Projekt ist für das WEMOS D1 MINI ESP32 mit der PlatformIO-Board-ID `wemos_d1_mini32` vorbereitet.

Falls der Upload bei `Connecting...` stehen bleibt, die BOOT-Taste am ESP32 gedrückt halten, den Upload erneut starten und die Taste loslassen, sobald der Schreibvorgang beginnt. Falls kein Anschluss erscheint, zunächst ein anderes USB-Datenkabel beziehungsweise einen anderen USB-Port probieren; bei Boards mit CP210x- oder CH340-USB-Wandler kann außerdem der passende Betriebssystemtreiber erforderlich sein.

## Stückliste

- ESP32 DevKit (klassischer ESP32 oder kompatibles Board)
- IR-Empfänger TSOP38438 (38 kHz)
- 940-nm-IR-LED mit bekanntem Pulsstrom, z. B. TSAL6200
- NPN-Transistor 2N2222/PN2222A
- 470 Ohm Basiswiderstand, 47 Ohm LED-Vorwiderstand (für 5 V; Datenblatt der LED prüfen)
- 10 kOhm Basis-Emitter-Widerstand
- 100 nF und 4,7 µF zur Abblockung am IR-Empfänger
- stabilisiertes 5-V-USB-Netzteil, Steckbrett/Platine und Leitungen

## Verdrahtung

### Empfänger (nur zum Anlernen)

- TSOP38438 `VS` an ESP32 `3V3`
- TSOP38438 `GND` an ESP32 `GND`
- TSOP38438 `OUT` an GPIO 14
- 100 nF parallel zwischen `VS` und `GND`, dicht am Empfänger; zusätzlich 4,7 µF empfohlen

Die Pinreihenfolge am konkreten Empfänger unbedingt im Datenblatt prüfen.

### Sender

- ESP32 GPIO 4 über 470 Ohm an Basis des 2N2222
- 10 kOhm von Basis nach GND
- Emitter an GND
- Kollektor an Kathode der IR-LED
- Anode der IR-LED über 47 Ohm an 5 V
- GND des 5-V-Netzteils und ESP32-GND verbinden

Der IR-Sender wird active-high über GPIO 4 angesteuert. Gleichzeitig dient `LED_BUILTIN` auf GPIO 2 als optische Sendeanzeige. Beim konkret getesteten Mini D1 ESP32 ist auch die blaue LED active-high: LOW bedeutet aus, HIGH bedeutet ein. Sie leuchtet während des vollständigen IR-Telegramms; die 38-kHz-Trägermodulation selbst wird ausschließlich am IR-Ausgang erzeugt.

Die IR-LED nicht direkt aus einem GPIO treiben. Der 47-Ohm-Wert ergibt bei typischen Bauteilwerten grob 70 mA Pulsstrom; vor Aufbau mit dem LED-Datenblatt abgleichen.

## Ermittelte IR-Codes

Die Datei `Vasner.ir` vom Flipper Zero zeigt für alle Tasten normales NEC-Protokoll mit Adresse `0x18`:

| Taste | Kommando |
|---|---:|
| OFF | `0x00` |
| ON | `0x01` |
| UP | `0x02` |
| DOWN | `0x03` |
| LEFT | `0x06` |
| MAX | `0x07` |
| RIGHT | `0x08` |

Die Software erzeugt daraus mit `encodeNEC(address, command)` jeweils den vollständigen 32-Bit-NEC-Rahmen einschließlich invertierter Prüfbytes. Ein separater IR-Empfänger ist für den endgültigen Aufbau daher nicht mehr erforderlich.

Die ursprüngliche Flipper-Zero-Aufnahme liegt unter `docs/Vasner.ir` bei.

## Inbetriebnahme

1. `secrets.example.h` als `secrets.h` kopieren und dort WLAN-Name, WLAN-Passwort und Web-Passwort eintragen.
2. Sketch kompilieren und hochladen.
3. IP-Adresse im seriellen Monitor ablesen; im Browser `http://<IP-Adresse>` öffnen.
4. Sender zunächst mit einer Handykamera prüfen (viele Frontkameras zeigen IR als violettes Flackern).
5. Zuerst bei freier Sicht und aus kurzer Distanz testen. Wenn einzelne Befehle verloren gehen, `COMMAND_GAP_MS` erhöhen oder die LED mechanisch besser ausrichten.

## Zustands- und Sicherheitsverhalten

Für L1 bis L4 sendet die Software immer ON, dreimal DOWN und anschließend null- bis dreimal UP. Sie verlässt sich deshalb nicht auf einen intern gemerkten Zustand. Voraussetzung ist, dass ON ein eigener, wiederholbar sendbarer Einschaltbefehl und kein Umschalter ist.

Für den Abschalttimer sendet die Software zunächst achtmal RIGHT, um die Auswahl auf 8 h zu sättigen. Anschließend reduziert sie mit LEFT auf 1–7 h; achtmal LEFT ab 8 h wählt den Dauerbetrieb. Das Gerät bleibt bei wiederholtem RIGHT auf 8 h stehen. Zwischen den Timerbefehlen liegen 150 ms, sodass auch die längste Folge aus 16 Signalen mit Reserve innerhalb des ungefähr fünfsekündigen Auswahlfensters übertragen wird.

IR besitzt keine Rückmeldung. Ein erfolgreicher HTTP-Aufruf beweist nur, dass der ESP32 gesendet hat, nicht dass der Heizer den Befehl empfangen hat. Das System daher nicht unbeaufsichtigt, nicht als sicherheitsrelevante Abschaltung und nicht über das öffentliche Internet betreiben. Am 230-V-Teil des Heizers sind keine Änderungen erforderlich oder vorgesehen.
