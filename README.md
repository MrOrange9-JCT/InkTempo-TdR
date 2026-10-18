<p align="center">
  Jordi Castro — Treball de Recerca — 2026
  <br>
  <br>
</p>

<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="https://i.ibb.co/Xk6LxQfD/inktempo-logo-light.png">
    <source media="(prefers-color-scheme: light)" srcset="https://i.ibb.co/8DwTh3Xf/inktempo-logo-dark.png">
    <img alt="Shows a black logo in light color mode and a white one in dark color mode." src="https://i.ibb.co/8DwTh3Xf/inktempo-logo-dark.png">
  </picture>
</p>

# Desenvolupament d’un gestor de productivitat IoT amb pantalla de tinta electrònica

Gestor digital de la productivitat personal desenvolupat per a la placa **LILYGO T5-S3**, que incorpora un microcontrolador ESP32-S3, una pantalla tàctil de tinta electrònica i un connector per a bateria de liti.

Aquest projecte forma part del Treball de Recerca i té com a objectiu implementar una aplicació pràctica de la tinta electrònica per al seguiment de tasques i la millora de la concentració. Per aconseguir un dispositiu amb avantatges sobre una agenda de paper, s’hi han incorporat funcions com el mètode Pomodoro i la connexió amb l’API de Notion.

> El programa ha estat dissenyat principalment per satisfer les necessitats de l’autor i per funcionar amb una LILYGO T5-S3 amb pantalla tàctil. Tot i així, el codi es pot utilitzar i modificar lliurement, sempre i quan sigui sense ànim de lucre.

## Funcions principals

- Temporitzador basat en el mètode Pomodoro.
   - Sessions de concentració, descansos curts i descansos llargs.
- Estadístiques diàries, com ara el temps de concentració i les tasques completades.
- Sincronització de tasques amb una base de dades de Notion.
   - Actualització de l’estat de les tasques des del dispositiu.
- Rellotge en temps real sincronitzat amb el servidor `pool.ntp.org`.
- Interfície gràfica minimalista amb tres pàgines: **Pomodoro**, **Estadístiques** i **Tasques**.
- Canvi d’orientació de la interfície i mode de repòs profund.

## Programari utilitzat

Els primers codis de prova es van compilar amb Arduino IDE. Tanmateix, a causa de les limitacions d’aquest programari (com ara la gestió global de les biblioteques i la impossibilitat d’utilitzar subcarpetes amb comoditat), el desenvolupament principal s’ha fet amb **PlatformIO**, instal·lat dins de Visual Studio Code.

Les biblioteques més importants són:

- **[EPDiy](https://github.com/vroland/epdiy)**, un controlador per a pantalles de tinta electrònica econòmiques, amb una [capa de compatibilitat](https://github.com/redstoneleo/EPDiy-T5S3) per a la LILYGO T5-S3.
- **[LVGL](https://github.com/lvgl/lvgl)** (*Light and Versatile Graphics Library*), utilitzada per desenvolupar la interfície gràfica.
- **[ArduinoJson](https://github.com/bblanchon/ArduinoJson)**, que permet interpretar les respostes JSON de l’API de Notion.
- Biblioteques i dependències pròpies de la placa LILYGO T5-S3, necessàries per controlar la pantalla, el panell tàctil i el rellotge en temps real.

L'arxiu [`platformio.ini`](https://github.com/MrOrange9-JCT/InkTempo-TdR/blob/main/platformio.ini) pot servir com a plantilla per a la placa LILYGO T5-S3 en concret.

## Estructura del codi

La carpeta arrel `src/` conté l’arxiu principal [`main.cpp`](https://github.com/MrOrange9-JCT/InkTempo-TdR/blob/main/src/main.cpp), els arxius de configuració i les carpetes [`ui/`](https://github.com/MrOrange9-JCT/InkTempo-TdR/tree/main/src/ui) i [`modules/`](https://github.com/MrOrange9-JCT/InkTempo-TdR/tree/main/src/modules):

```text
.
├── src/
│   ├── main.cpp
│   ├── modules/
│   │   ├── date_format.cpp
│   │   ├── pomodoro.cpp
│   │   ├── stats.cpp
│   │   └── tasks.cpp
│   └── ui/
├── platformio.ini
├── lv_conf.h
├── config.h
├── secrets.h
└── README.md
```

Els arxius de `ui/` han estat generats amb [**EEZ Studio**](https://github.com/eez-open/studio), una eina que permet dissenyar una GUI de manera visual i generar-ne el codi per a LVGL. Els arxius de `modules/`, en canvi, contenen la lògica pròpia del projecte.

### `main.cpp`

És l’arxiu principal del programa. Carrega els controladors de la placa i les llibreries necessàries, connecta els diferents mòduls i actualitza els elements comuns de la interfície, com ara els botons de canvi de pàgina i els indicadors de la franja superior i inferior.

### `date_format.cpp`

Transforma una data en un format més llegible i en català, com ara `dl. 19 oct.`. S’utilitza a `main.cpp` i també a `tasks.cpp`.

### `pomodoro.cpp`

Allotja tota la lògica del temporitzador Pomodoro. Funciona amb una màquina d’estats: cada etapa del temporitzador s’associa amb un estat i, quan es compleix una determinada condició —com ara l’acabament del temps—, s’avança a la següent etapa.

### `stats.cpp`

Controla i emmagatzema les estadístiques d’ús i concentració. També s’encarrega d’iniciar el rellotge en temps real i sincronitzar-lo amb `pool.ntp.org`, fet que permet detectar el canvi de dia i reiniciar les estadístiques.

### `tasks.cpp`

Inicialitza el Wi-Fi i integra l’API de Notion. Un cop rebudes les dades en format JSON, aquestes es tracten i se sanegen, es converteixen les dates i s’ordenen segons la data límit i l’estat de cada tasca.

## Configuració

Per configurar el programa segons les preferències de l’usuari, cal revisar els arxius següents:

- `config.h`, que inclou les opcions personals del programa.
- `secrets.h`, que conté les credencials de la xarxa Wi-Fi i de Notion.

El fitxer `secrets.h` no s’ha de publicar amb credencials reals. Com a mínim, haurà de contenir les dades que utilitza el projecte per connectar-se a la xarxa i a Notion:

```cpp
#define WIFI_SSID "nom_de_la_xarxa"
#define WIFI_PASSWORD "contrasenya_de_la_xarxa"
#define NOTION_TOKEN "secret_de_notion"
#define NOTION_DATABASE_ID "id_de_la_base_de_dades"
```
> Aquestes són les variables necessàries. Podeu trobar la plantilla a [`secrets.h.template`](https://github.com/MrOrange9-JCT/InkTempo-TdR/blob/main/src/secrets.h.template)

Els noms exactes de les constants han de coincidir amb els que espera el codi.

## Plantilla de Notion

Perquè la sincronització funcioni correctament, cal utilitzar una base de dades de Notion amb les propietats esperades per `tasks.cpp`.

**Plantilla de Notion:** [afegir aquí l’enllaç a la plantilla]

Un cop duplicada la plantilla, cal crear una integració a Notion, compartir-hi la base de dades i introduir el token i l’identificador de la base de dades a `secrets.h`.

## Instal·lació i ús

1. Instal·la Visual Studio Code i l’extensió de PlatformIO.
2. Clona el repositori:

   ```bash
   git clone https://github.com/MrOrange9-JCT/InkTempo-TdR.git
   cd InkTempo-TdR
   ```

3. Revisa `platformio.ini` i comprova que la configuració correspon a la placa utilitzada.
4. Completa `secrets.h` i ajusta `config.h` segons les teves preferències.
5. Connecta la LILYGO T5-S3 mitjançant USB-C.
6. Compila el projecte i puja el codi amb PlatformIO.
7. Reinicia el dispositiu i comprova la connexió, l’hora i la càrrega de les tasques.

Si la placa no entra automàticament en mode de càrrega, cal prémer els botons **BOOT** i **RST** alhora per alliberar el *bootloader*.

Per utilitzar el dispositiu:

- Fes servir els botons laterals `<` i `>` per canviar de pàgina.
- Toca el comptador per iniciar, pausar o reprendre el temporitzador Pomodoro.
- Marca les tasques com a completades des de la pàgina **Tasques**.
- Prem el botó **01** per girar la interfície i adaptar-la a l’orientació escollida.
- Mantén premut el botó **01** per entrar en mode de repòs profund.
- Prem ràpidament **RST** per reiniciar el programa.

## Desenvolupament

La interfície es va prototipar inicialment amb Figma i posteriorment es va desenvolupar amb EEZ Studio i LVGL. El programa s’ha estructurat en diversos mòduls per separar la interfície gràfica de la lògica del temporitzador, les estadístiques i les tasques.

Aquest repositori conté el programari del dispositiu i complementa l’explicació del desenvolupament que es troba al Treball de Recerca. La carcassa i el muntatge físic de la placa no formen part d’aquest repositori.

## Agraïments

- LILYGO, per la placa T5-S3 i la documentació del dispositiu.
- Els desenvolupadors d’EPDiy i LVGL.
- Redstoneleo, per la capa de compatibilitat amb la LILYGO T5-S3.
- Notion, per l’API utilitzada en la sincronització de tasques.
