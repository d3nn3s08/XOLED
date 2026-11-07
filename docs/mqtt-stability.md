# MQTT-/TLS-Stabilitaet: Implementierung und Pruefstand

Stand des urspruenglichen Stabilitaetstests: 04.10.2026

## Parserkorrektur vom 07.11.2026

`gcode_state` wird explizit mit `as<const char*>()` gelesen. Der bisherige Ausdruck
`print["gcode_state"] | nullptr` lieferte durch die gewaehlte ArduinoJson-Ueberladung
immer `nullptr`, selbst fuer `RUNNING`. Dadurch blieben erkannte Druckmeldungen
wirkungslos; der Status blieb nach dem Reconnect unbekannt und die LEDs zeigten
die Hintergrundfarbe.

Die Restzeit wird aus dem ersten gueltigen Feld in dieser Reihenfolge gelesen:
`remaining_time`, `mc_remaining_time`, `remain_time`. Alle drei sind Minutenwerte;
73 Minuten ergeben 4380 Sekunden. Fehlende, negative, nicht ganzzahlige oder
ueberlaufende Werte veraendern die bisherige Restzeit nicht. Null ist gueltig.
Der JSON-Filter beruecksichtigt jetzt neun ausgewaehlte Felder; der 49-KB-Puffer
bleibt unveraendert.

`python -B -m unittest discover -s test` umfasst einen nativen Parser-Test mit der
tatsaechlichen Callback-Funktion und den installierten ArduinoJson-Headern.
Er benoetigt unter Windows MSVC, ansonsten wird dieser Test als uebersprungen
gemeldet. Geprueft werden Statuswechsel, Teilmeldungen, Restzeit-Fallbacks,
ungueltige Werte und eine synthetische gefilterte Payload mit ueber 30 KB.
Der Hardware-Stabilitaetsnachweis weiter unten ist davon unabhaengig.

## Bestaetigte Fehler und wahrscheinlicher Absturzpfad

Die bisherige Implementierung rief `client.loop()` auch nach einem fehlgeschlagenen
Connect auf, ignorierte dessen Rueckgabewert und schloss TLS bei WLAN-Ausfall und
Rekonfiguration nicht ausdruecklich. `remaining_time` wurde bei fehlendem Feld erneut
von Sekunden in Sekunden umgerechnet, also jedes Mal mit 60 multipliziert.

Die asynchronen Webserver-Callbacks riefen ausserdem `client.connected()` und
`client.disconnect()` gleichzeitig mit der Hauptschleife auf. In der installierten
NetworkClientSecure-Version kann schon `connected()` ueber `read()`/`available()` den
TLS-Kontext stoppen und freigeben. Gleichzeitig laufende TLS-Lese-, Schreib- oder
Stop-Operationen sind ein plausibler Pfad zu ungueltigen Socket-Zugriffen und
Use-after-free. Dieser Konkurrenzzugriff ist im Code bestaetigt; die Zuordnung zu
den gemeldeten Guru-Meditation-Abstuerzen ist ohne Backtrace und Hardwaretest
noch nicht bewiesen. Die Ursache der urspruenglichen TLS-MAC-/Record-Fehler ist
ebenfalls nicht abschliessend geklaert.

## Aenderungen

- Dauerhafte `WiFiClientSecure`-/`PubSubClient`-Objekte bleiben erhalten.
- Nur Setup und Hauptschleife bedienen MQTT/TLS. Die WebUI liest einen atomaren
  Verbindungsstatus. `mqtt_reconfigure()` uebergibt eine durch eine kurze
  Critical Section geschuetzte Konfigurationskopie; die Hauptschleife schliesst
  zuerst die alte Verbindung und setzt erst danach Server und Zugangsdaten.
  Unveraenderte Einstellungen loesen wie bisher keinen Reconnect aus.
- Fehlerpfad: `client.disconnect()` -> `tls.stop()` -> Disconnected -> Deadline.
  Derselbe Abbau erfolgt vor jedem Connect. Nach Fehlern wird sofort zurueckgekehrt.
  `client.loop()` wird nur nach erfolgreicher `client.connected()`-Pruefung aufgerufen.
- Backoff: 5/10/20/40/60 Sekunden, maximal 60 Sekunden; nach erfolgreichem
  Connect und Subscribe wieder 5 Sekunden. Die Wartezeit beginnt nach dem Abbau,
  nicht vor dem blockierenden Connect. Deadlines beruecksichtigen millis-Ueberlauf.
- `remaining_time` aktualisiert Sekunden nur bei vorhandenem unsigned-long-Wert,
  dessen Minutenumrechnung nicht ueberlaeuft. Fehlende, negative, nicht ganzzahlige
  und zu grosse Werte lassen den letzten Wert unveraendert.
- Der 49152-Byte-MQTT-Puffer bleibt erhalten. Ein Allokationsfehler wird gemeldet
  und verhindert Connect mit einem fuer H2C zu kleinen Ersatzpuffer.
- ArduinoJson filtert ausschliesslich die benoetigten Felder (seit der
  Parserkorrektur neun einschliesslich der Restzeit-Aliase).
  Die Payload wird direkt geparst; der dauerhaft benoetigte Jobname wird kopiert.
- Temporaere Heap-Diagnose: FreeHeap, MinFreeHeap und groesster freier 8-Bit-Block
  beim Start, vor Connect, nach Erfolg, Fehler, Abbruch, Rekonfiguration und
  Reconnect sowie etwa alle 60 Sekunden. Druckerberichte werden beim ersten
  Empfang und dann hoechstens alle 60 Sekunden nur mit ihrer Byte-Anzahl gemeldet.
  Es werden keine Zugangsdaten oder Payload-Inhalte protokolliert.

## Ausgefuehrte Pruefungen

- esp32dev Clean: bestanden.
- Vollstaendiger Firmware-Build und anschliessender Build der finalen Aenderung:
  bestanden. Statischer RAM: 48808/327680 Byte; Flash: 1250416/1572864 Byte.
  Diese Build-Werte sind keine Laufzeit-Heap-Messung. Der dynamische MQTT-Puffer,
  TLS und JSON-Allokationen muessen am Geraet gemessen werden.
- `python test/test_mqtt_source.py`: 5 Quellcode-Sicherheitspruefungen bestanden.
  Diese Pruefungen sichern zentrale Guard-/Ownership-Regeln ab, simulieren aber
  weder TLS noch H2C und ersetzen keine Laufzeittests.
- `git diff --check`: bestanden.
- Flash auf COM3: fehlgeschlagen. Auch ausserhalb der Sandbox meldet Windows
  `FileNotFoundError(2)` beim Oeffnen des Anschlusses; die Anschlussliste ist leer.
  Es wurde kein eigener Monitor gestartet und kein fremder Prozess beendet.

## Noch erforderlicher Hardwaretest

WLAN, MQTT, H2C-Empfang, gleichzeitige WebUI-Nutzung und wiederholte Reconnects
sind mit der geaenderten Firmware noch nicht geprueft. Stabilitaetstestdauer:
0 Minuten. FreeHeap, MinFreeHeap, LargestBlock, Speichertrend und Fragmentierung:
keine Messwerte. Wiederauftreten von Bad-file-number-, TLS- oder
Guru-Meditation-Fehlern: nicht verifiziert.

Sobald COM3 vorhanden ist: Firmware flashen, Monitor mit 115200 Baud starten,
WebUI anhand der seriell gemeldeten IP oeffnen und mindestens 20-30 Minuten
mit mehreren kontrollierten MQTT-Unterbrechungen pruefen. Keine Druckauftraege
oder Druckerkonfiguration fuer diesen Test veraendern. Heap-Werte jeweils in
vergleichbaren Zustandsphasen vergleichen: nach aufgebautem TLS und nach Abbau.
Sinkender MinFreeHeap allein beweist kein Leak; sinkender FreeHeap bei gleicher
Last ist ein Hinweis. Ein sinkender LargestBlock bei weitgehend konstantem
FreeHeap kann Fragmentierung anzeigen. Grosse H2C-Berichte muessen weiterhin
lesbar bleiben und in der WebUI aktualisierte Werte ergeben.

## Verbleibende Grenzen

Der Stabilitaetsnachweis ist offen. Der bestehende globale Druckerstatus wird
weiterhin zwischen Hauptschleife und Webserver geteilt; insbesondere der
String-Jobname ist nicht durch diese MQTT-Ownership-Korrektur synchronisiert.
Falls Abstuerze bei WebUI-Nutzung bleiben, muss dieser unabhaengige Zugriff anhand
eines decodierten Backtraces gezielt untersucht werden. Er wurde ausserhalb des
beauftragten MQTT-/TLS-Lifecycles nicht umgebaut.

Das bisherige `setInsecure()` bleibt unveraendert. Partitionlayout, NVS, LittleFS,
WLAN- und LED-Logik sowie WebUI-Design wurden nicht angepasst. Fuer weitere
Diagnose kann der serielle Backtrace mit der zugehoerigen `firmware.elf` ueber
den bereits konfigurierten `esp32_exception_decoder` ausgewertet werden;
eine Core-Dump-Partition wurde nicht angelegt.

Ein lokaler Commit wurde im anschliessenden Benutzerauftrag ausdruecklich
freigegeben. Kein Push, Branchwechsel, Stash oder Backup. Keine Unteragenten:
die gezielte Aenderung wurde vollstaendig im Hauptagenten bearbeitet.
