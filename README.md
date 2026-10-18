# InkTempo

Gestor digital de productivitat personal desenvolupat per a la placa **LILYGO T5-S3**, equipada amb un microcontrolador **ESP32-S3**, pantalla tàctil e-Paper de 4,7 polzades i bateria Li-Po.

El projecte forma part del Treball de Recerca i explora l’aplicació pràctica de la tinta electrònica en un dispositiu orientat al seguiment de tasques, la concentració i l’organització personal. El programari combina un temporitzador Pomodoro, un sistema d’estadístiques i la sincronització de tasques amb una base de dades de Notion.

> **Estat del projecte:** funcional i desenvolupat específicament per a una LILYGO T5-S3 amb pantalla tàctil. Pot requerir ajustos per funcionar en altres revisions de la placa o amb altres pantalles.

## Característiques principals

- Interfície gràfica minimalista basada en **LVGL**.
- Temporitzador **Pomodoro** amb sessions de concentració, descansos curts i descansos llargs.
- Pausa i represa del temporitzador mitjançant la pantalla tàctil.
- Estadístiques diàries de concentració i tasques completades.
- Reinici de les estadístiques quan es detecta un canvi de dia.
- Sincronització de tasques amb l’API de **Notion**.
- Actualització de l’estat de les tasques des del dispositiu.
- Ordenació de les tasques segons la data límit i l’estat.
- Rellotge en temps real sincronitzat mitjançant **NTP**.
- Indicador de bateria, data, hora i pàgina activa.
- Navegació entre les pàgines Pomodoro, Estadístiques i Tasques.
- Canvi d’orientació de la interfície.
- Mode de repòs profund per reduir el consum energètic.

## Maquinari

El dispositiu està pensat per funcionar amb els components següents:

| Component | Descripció |
| --- | --- |
| Placa principal | LILYGO T5-S3 / T5 4.7” e-Paper V2.4 amb ESP32-S3 i pantalla tàctil |
| Pantalla | E-Paper de 4,7 polzades amb escala de grisos |
| Bateria | Li-Po d’ió de liti, 3,7 V i 2000 mAh |
| Carcassa | Peça impresa en 3D amb PLA i suport posterior |

La bateria utilitzada requereix un connector JST-PH compatible amb la placa. Abans de connectar-la, cal comprovar que la polaritat dels cables coincideix amb la indicada pel fabricant. Una connexió incorrecta pot provocar danys a la placa o a la bateria.

## Programari i biblioteques

El projecte es desenvolupa amb **PlatformIO** dins de Visual Studio Code. Aquesta eina permet gestionar de manera local la placa, les dependències i la configuració de compilació, fet especialment útil en un projecte amb diversos mòduls i subcarpetes.

Les biblioteques i components de programari més importants són:

- **EPDiy:** controlador i capa de compatibilitat per a pantalles e-Paper basades en ESP32.
- **LVGL (Light and Versatile Graphics Library):** biblioteca per a la interfície gràfica i els seus controls.
- **ArduinoJson:** conversió i tractament de les respostes JSON de l’API de Notion.
- **Components de la LILYGO T5-S3:** control de la pantalla, el panell tàctil, el rellotge en temps real i altres perifèrics.
- **Wi-Fi i NTP:** connexió a la xarxa i sincronització de l’hora amb `pool.ntp.org`.

## Estructura del repositori

L’estructura principal del projecte és la següent:

```text
.
├── include/              # Fitxers de configuració i capçaleres del projecte
├── lib/                  # Biblioteques locals, si escau
├── src/
│   ├── main.cpp          # Punt d’entrada i coordinació general del programa
│   ├── modules/          # Lògica pròpia del dispositiu
│   │   ├── date_format.cpp
│   │   ├── pomodoro.cpp
│   │   ├── stats.cpp
│   │   └── tasks.cpp
│   └── ui/               # Fitxers generats per EEZ Studio i LVGL
├── platformio.ini        # Configuració de PlatformIO i dependències
├── lv_conf.h             # Configuració de LVGL
├── config.h              # Preferències de funcionament
├── secrets.h             # Credencials locals, no s’ha de publicar
└── README.md
```

> La distribució exacta dels fitxers pot variar segons la versió del projecte. Els arxius de la carpeta `ui/` són generats principalment amb EEZ Studio, mentre que els de `modules/` contenen la lògica desenvolupada específicament per a l’aplicació.

### Mòduls principals

#### `main.cpp`

Inicialitza la placa, els controladors i les biblioteques necessàries. També coordina els diferents mòduls del programa, renderitza els elements comuns de la interfície i actualitza la informació de les franges superior i inferior.

#### `date_format.cpp`

Transforma dates numèriques en formats més llegibles i adaptats al català, com ara `dl. 19 oct.`. Aquest mòdul s’utilitza tant en la interfície general com en la visualització de tasques.

#### `pomodoro.cpp`

Implementa la lògica del temporitzador mitjançant una màquina d’estats. Les etapes principals són les sessions de concentració, els descansos curts, els descansos llargs i les transicions. També gestiona la pausa i la represa iniciades per l’usuari.

#### `stats.cpp`

Registra les dades d’ús i concentració. Aquest mòdul inicialitza el rellotge en temps real, sincronitza l’hora amb un servidor NTP i detecta el canvi de dia per reiniciar o actualitzar les estadístiques corresponents.

#### `tasks.cpp`

Gestiona la connexió Wi-Fi i la comunicació amb Notion. Les respostes JSON es converteixen en estructures `NotionTask`, es processen i s’ordenen abans de mostrar-les a la interfície. Quan l’usuari modifica l’estat d’una tasca, el canvi també es pot sincronitzar amb Notion.

## Interfície d’usuari

La interfície està dividida en tres pàgines principals:

### Pomodoro

És la pantalla inicial i mostra el temporitzador de concentració. Les sessions segueixen una configuració basada en el mètode Pomodoro: períodes de concentració de 25 minuts, descansos curts de 5 minuts i un descans llarg després de quatre cicles complets.

### Estadístiques

Mostra les dades acumulades durant el dia, com ara el temps de concentració o el nombre de tasques completades. Els objectius es poden personalitzar i es representen gràficament mitjançant barres de progrés.

### Tasques

Mostra les tasques obtingudes de Notion, ordenades per data límit i estat. Les tasques completades apareixen diferenciades visualment i els canvis d’estat es poden enviar de nou a Notion.

A les franges informatives es mostren l’hora, la data, el títol de la pàgina i el nivell de bateria. Els botons laterals permeten canviar de pàgina, mentre que els indicadors inferiors mostren la pàgina activa.

## Configuració

Abans de compilar el projecte, cal revisar els fitxers de configuració:

- `config.h`: preferències de l’aplicació, durades del Pomodoro, objectius d’estadístiques i altres opcions de funcionament.
- `secrets.h`: credencials de la xarxa Wi-Fi i de l’API de Notion.

Per seguretat, `secrets.h` no s’hauria de publicar ni pujar al repositori amb credencials reals. Es recomana crear-lo a partir d’una plantilla local, per exemple:

```cpp
#define WIFI_SSID "nom_de_la_xarxa"
#define WIFI_PASSWORD "contrasenya_de_la_xarxa"
#define NOTION_TOKEN "secret_de_notion"
#define NOTION_DATABASE_ID "id_de_la_base_de_dades"
```

Els noms exactes de les constants han de coincidir amb els que espera el codi del projecte.

## Plantilla de Notion

Perquè la sincronització funcioni correctament, cal utilitzar una base de dades de Notion amb les propietats i els tipus esperats pel mòdul `tasks.cpp`.

**Plantilla de Notion:** [enllaç a la plantilla de Notion — afegir manualment]

Després de duplicar la plantilla, cal:

1. Crear una integració a Notion.
2. Compartir la base de dades amb aquesta integració.
3. Copiar el token de la integració al fitxer `secrets.h`.
4. Copiar l’identificador de la base de dades al mateix fitxer.
5. Comprovar que els noms i els tipus de les propietats coincideixen amb els que utilitza el projecte.

No s’ha d’incloure mai el token de Notion en captures de pantalla, commits públics o fitxers compartits.

## Instal·lació i compilació

### Requisits

- Visual Studio Code.
- Extensió de PlatformIO.
- Cable USB-C de dades.
- Placa LILYGO T5-S3 compatible.
- Xarxa Wi-Fi de 2,4 GHz, si és necessària per sincronitzar les tasques i l’hora.
- Compte de Notion i una integració amb accés a la base de dades.

### Passos

1. Clona el repositori:

   ```bash
   git clone https://github.com/MrOrange9-JCT/InkTempo-TdR.git
   cd InkTempo-TdR
   ```

2. Obre la carpeta amb Visual Studio Code i instal·la l’extensió de PlatformIO.
3. Revisa `platformio.ini` i comprova que la configuració correspon a la placa utilitzada.
4. Crea o completa `secrets.h` amb les credencials de Wi-Fi i Notion.
5. Ajusta les preferències de `config.h`.
6. Connecta la placa mitjançant USB-C.
7. Compila el projecte amb PlatformIO.
8. Puja el firmware a la placa.
9. Reinicia el dispositiu i comprova la connexió, l’hora i la càrrega de les tasques.

En cas que la placa no entri automàticament en mode de càrrega, pot ser necessari prémer simultàniament els botons **BOOT** i **RST** abans de pujar el programa.

## Ús del dispositiu

- Utilitza els botons laterals `<` i `>` per canviar de pàgina.
- Toca el comptador Pomodoro per iniciar, pausar o reprendre una sessió, segons l’estat actual.
- Marca les tasques com a completades des de la pàgina de tasques; el canvi es sincronitzarà amb Notion quan hi hagi connexió.
- Utilitza el botó **01** per canviar l’orientació de la interfície.
- Mantén premut el botó **01** per activar el mode de repòs profund.
- Prem ràpidament **RST** per reiniciar el programa.
- Per tornar a carregar el firmware, utilitza la combinació **BOOT + RST**.

El dispositiu es pot utilitzar en orientació vertical, aproximadament a 80° respecte de l’horitzontal, o en orientació plana, aproximadament a 10°. La primera és adequada per consultar ràpidament la informació, mentre que la segona facilita la interacció amb la pantalla tàctil.

## Consideracions i limitacions

- La sincronització amb Notion requereix connexió a Internet i credencials vàlides.
- La pantalla e-Paper no està pensada per a animacions ràpides; el programa prioritza una actualització eficient i un consum reduït.
- Les actualitzacions i el temps de resposta poden variar segons el tipus d’actualització de pantalla.
- El projecte depèn de la configuració específica de la LILYGO T5-S3 i de les biblioteques compatibles amb aquesta placa.
- Les dades estadístiques s’emmagatzemen d’acord amb la implementació actual del projecte i poden perdre’s si no es desa una còpia persistent.
- Cal manipular la bateria Li-Po amb precaució i respectar-ne la polaritat, el voltatge i les recomanacions del fabricant.

## Desenvolupament i autoria

Aquest projecte ha estat desenvolupat com a part d’un Treball de Recerca sobre l’aplicació de la tinta electrònica en un dispositiu digital de productivitat personal.

El disseny de la interfície es va prototipar inicialment amb Figma i es va concretar amb EEZ Studio, que permet generar interfícies basades en LVGL. La carcassa es va adaptar en un entorn de disseny 3D i es va fabricar mitjançant impressió 3D.

El repositori es publica amb finalitats educatives i de reutilització. El codi es pot estudiar, adaptar i ampliar, sempre tenint en compte les llicències dels components i biblioteques de tercers utilitzats.

## Llicència

Consulteu els fitxers de llicència del repositori i les llicències de les dependències abans de redistribuir el projecte o parts del seu codi.

## Agraïments

- LILYGO, per la placa T5-S3 i la documentació del dispositiu.
- Els desenvolupadors d’EPDiy i LVGL.
- Redstoneleo, per la capa de compatibilitat i els fitxers relacionats amb la LILYGO T5-S3.
- Notion, per l’API utilitzada en la sincronització de tasques.
- La comunitat de PlatformIO i ESP32.
