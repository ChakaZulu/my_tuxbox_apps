TuxCom:

Historie:
---------

25.07.2004 Version 1.4
 - Taskmanager eingebaut (�ber Info-Taste aufrufbar)
 - vor-/zur�ck-scrollen bei Kommandoausf�hrung/Skriptausf�hrung m�glich
 - vor-/zur�ck-scrollen in Dateiansicht nicht mahr auf 100k-Dateien beschr�nkt
 - aktuell ausgew�hlte Datei merken bei Verlassen des Plugins
 - Tastaturunterst�tzung f�r DMM-Tastatur eingebaut
 - Verz�gerung bei gedr�ckter Taste eingebaut
 - Bugfix: Workaround f�r Tastendruck-Fehler von Enigma
 - Bei Verweis-erstellen (Taste 0) wird automatisch der ausgew�hlte Dateiname vorgeschlagen

21.06.2004 Version 1.3
 - FTP-Client eingebaut
 - kleinere Fehler im Editor beseitigt
 - Texteingabe: Sprung zum n�chsten Zeichen, wenn eine andere Ziffer gedr�ckt wird.
 - Texteingabe: letztes Zeichen wird entfernt wenn am Ende der Zeile volume- gedr�ckt wird.
 - Umschalten zwischen 4:3 und 16:9-Modus �ber Dream-Taste
 - Dateiansicht : Scrollen wie im Editor m�glich (bei Dateien, die maximal 100k gross sind).

05.06.2004 Version 1.2a
 - BugFix: Fehlende Sonderzeichen bei Eingabe erg�nzt.
 - Texteingabe im "SMS-Stil" eingebaut
 
29.05.2004 Version 1.2
 - Unterst�tzung zum Extrahieren aus "tar", "tar.Z", "tar.gz" und "tar.bz2" Archiven
   funktioniert leider im Original-Image 1.07.4 mit vielen Archiven nicht (zu alte BusyBox-Version :( )
 - Anzeige der aktuellen Zeilennummer im Editor
 - Positionierung anhand der TuxTxt-Koordinaten
 - grosse Schrift beim Editieren einer Zeile
 - Scrollen in Zeichen im Editiermodus an Enigma-Standard angepasst (hoch/runter vertauscht)
 - Versionsnummer �ber Info-Taste abrufbar
 - Sicherheitsabfrage, falls durch kopieren/verschieben bestehende Dateien �berschrieben werden.

08.05.2004 Version 1.1a
 - BugFix: Keine angeh�ngten Leerzeichen mehr beim Umbenennen von Dateien

02.05.2004 Version 1.1
 - einige Farb�nderungen
 - Deutsche Texte eingebaut
 - M�glichkeit, Tasten gedr�ckt zu halten (hoch/runter, rechts/links, volume+/-, gr�ne Taste)
 - 3 Tranzparenzstufen 
 - Dateien markieren, sodass man mehrere Dateien auf einmal kopieren/verschieben oder l�schen kann
 - Tranzparanzmodus wird jetzt durch die 'mute'- Taste gewechselt (analog zu TuxTxt) (gr�ne Taste wird zum Dateien markieren verwendet)

03.04.2004 Version 1.0 : 
   erste Ver�ffentlichung
   
     
  
Quellen:
--------
Ich habe Codeteile von T-Hydron's script-plugin (http://t-hydron.verkoyen.be/)
und LazyT's TuxTxt (aus dem CDK)  �bernommen. 


Vorraussetzungen:
-----------------
 - Eine Dreambox 7000-S ( nicht auf anderen Modellen getested)
 - Firmware Version 1.07.x oder h�her ( nicht auf �lteren Versionen getested)

Installation:
-------------
Wie bei jedem Plugin, einfach tuxcom.so und tuxcom.cfg nach /var/tuxbox/plugins kopieren

Wenn die Font-Datei 'pakenham.ttf' nicht in /share/fonts/ liegt 
bitte diese in /var/tuxbox/config/enigma/fonts/ kopieren

Tasten:
---------------

links/rechts		linkes/rechtes Fenster w�hlen
hoch/runter 		n�chsten/vorherigen Eintrag im aktuellen Fenster w�hlen
volume -/+		Eine Seite hoch/runter im aktuellen Fenster
ok			gew�hlte Datei ausf�hren / Verzeichnis wechseln im aktuellen Fenster / Archiv zum Lesen �ffnen
1			Eigenschaften (Rechte) von gew�hlter Datei anzeigen/�ndern
2			gew�hlte Datei umbenennen
3			gew�hlte Datei anzeigen
4			gew�hlte Datei bearbeiten
5			gew�hlte Datei von aktuellem Fenster ins andere Fenster kopieren
6			gew�hlte Datei von aktuellem Fenster ins andere Fenster verschieben
7			neues Verzeichnis in aktuellem Fenster erstellen
8			gew�hlte Datei l�schen
9			neue Datei in aktuellem Fenster erstellen
0			symbolischen Verweis zur gew�hlten Datei im aktuellen Verzeichnis des anderen Fensters erstellen
rot			linux Kommando ausf�hren
gr�n			Datei markieren/Markierung aufheben
gelb			Sortierung der Eintr�ge im aktuellen Fenster umkehren
blau			Ansicht aktualisieren
mute			Transparenzmodus wechseln
dream			Umschalten zwischen 4:3 und 16:9-Modus
info			Taskmanager / Versionsinformation

in Mitteilungsfenstern:

links/rechts		Auswahl �ndern
ok			Auswahl best�tigen
rot/gr�n/gelb		Auswahl �ndern

in Texteingabe:

links/rechts		Position wechseln
hoch/runter		Zeichen wechseln
ok			best�tigen
volume +		neues Zeichen einf�gen
volume -		Zeichen entfernen
rot			Eingabe l�schen
gelb			Wechseln zwischen Gross und Kleinbuchstaben
0..9			Zeichenauswahl im "SMS-Stil" ( wie in der Enigma Texteingabe)


in Eigenschaften:

hoch/runter		Auswahl �ndern
ok			Recht gew�hren/entziehen
rot			�nderungen best�tigen
gr�n			�nderungen verwerfen


in Editor:

links/rechts		Seite zur�ck/vor
hoch/runter		Zeile zur�ck/vor
ok			Zeile bearbeiten
volume +		Sprung zur 1. Zeile
volume -		Sprung zur letzten Zeile
rot			Zeile l�schen
gr�n			Zeile einf�gen

in Viewer:

ok, rechts		n�chste Seite
links/rechts		Seite zur�ck/vor
hoch/runter		Zeile zur�ck/vor
volume +		Sprung zur 1. Zeile
volume -		Sprung zur letzten Zeile

in Taskmanager:

ok, rechts		n�chste Seite
links/rechts		Seite zur�ck/vor
hoch/runter		Zeile zur�ck/vor
volume +		Sprung zur 1. Zeile
volume -		Sprung zur letzten Zeile
rot			Prozess beenden 

in allen Dialogen: 

lame			Dialog verlassen



Farben:
------------
Hintergrund: 
schwarz : aktuelles Verzeichnis hat nur Lesezugriff
blau    : aktuelles Verzeichnis hat Lese/Schreibzugriff

Dateiname:
weiss : Eintrag ist Verzeichnis
orange: Eintrag ist Verweis
gelb  : Eintrag ist ausf�hrbar
grau  : Eintrag hat Schreibzugriff
gr�n  : Eintrag hat Lesezugriff


Nutzung des FTP-Client:
-----------------------
1.) Eine Datei mit der Endung .ftp erstellen. 
2.) Diese Datei editieren:
Folgende Eintr�ge in dieser Datei sind m�glich:
host=<ftp-Adresse>	(muss immer angegeben werden, z.B.: host=ftp.gnu.org)
user=<username> 	(optional)
pass=<password> 	(optional)
port=<ftpport>  	(optional, standardm�ssig 21)
dir=<Unterverzeichnis>	(optional, standardm�ssig /)
3.) Datei ausw�hlen und OK dr�cken. 
Es wird eine FTP-Verbindung zur angegebenen Adresse aufgebaut.