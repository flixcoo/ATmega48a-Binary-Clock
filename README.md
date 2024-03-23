# ATmega48A - Binäruhr

## Pin-Belegung
<img src="atmega_layout.png" width="500">

### Stunden-LEDs
Stunde 2<sup>0</sup>: `PD7`  
Stunde 2<sup>1</sup>: `PD6`  
Stunde 2<sup>2</sup>: `PD5`  
Stunde 2<sup>3</sup>: `PD4`  
Stunde 2<sup>4</sup>: `PD3`
### Minuten-LEDs
Minute 2<sup>0</sup>: `PC5`  
Minute 2<sup>1</sup>: `PC4`  
Minute 2<sup>2</sup>: `PC3`     
Minute 2<sup>3</sup>: `PC2`  
Minute 2<sup>4</sup>: `PC1`  
Minute 2<sup>5</sup>: `PC0`  
### Buttons  
Button 1: `PB0`  
Button 2: `PB1`  
Button 3: `PD2`
### Sonstiges
Uhrenquarz: `PB6` + `PB7`

## Funktionalität
Im Standardbetrieb startet die Uhr beim Einlegen der Batterie bei 12:00 Uhr.
### Tasterbelegung
Über das Drücken des Taster 1 wird der Energiesparmodus aktiviert (siehe [Energiesparmodus](#Energiesparmodus)). Über ein weiteres Drücken des Tasters wird der Energiesparmodus wieder deaktiviert.  
Mithilfe des Taster 2 lassen sich die Stundenzahlen, welche auf der Uhr angezeigt werden, inkrementiert. Diese gehen hoch bis 23 und starten danach wieder bei 0.  
Der dritte Taster dient dem Inkrementieren der Minuten. Diese gehen hoch bis zur 59 und wechseln danach wieder auf die 0.
### Zeitmessung über Timer und Uhrenquarz
//ToDo
### Energiesparmodus
//ToDo
### Helligkeitssteuerung
//ToDo