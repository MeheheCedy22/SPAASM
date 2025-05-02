8 charov dlhe: FIITgeek
zistenie pomocou tych dvoch funkcii v screenshote
funckia kopiruje znaky na urcitych poziiciac ha vsyklada tento answer string

to co napisem sa napise za tie 2 stringy kde su same nul chary a je tam volne miesto a potom tam jeedna funckia kde ide parameter '8' tak ta porovnava to co je v EBX s tym miestom pricom v EBX je return value z tej funckie ktora kopiruje veci z velkeho stringu do toho co ma 8 characterov a ta return value sa dostane do EBX vec instruckiu ze MOVE EBX,AL


I4561AsEmblerySuPOhodicka2x3XzgvwpqLJfBCDnFMH90KNG78QRtTUVWjYZ (dlzka 62, cize na var[offset] if offset = 62 (0x3e) tak tam je null char)


      (OFFSET vo velkom stringu)
char    dec     hex
M       43      0x2b
a       24      0x18
r       12      0xc
e       11      0xb
k       23      0x17
\0      62      0x3e

- final riesenie -> Marek
- na 3 miestach zmenene miesto cisla 8 cislo 5 ako velkost tohos stringu
    - return value funkcie kde sa to kopiruje
    - potom v if condition v hlavnej funkcii
    - a potom argument v porovnavacej funkcii
- inak zmenen offsety v kopirovacej funkcii a potom pridane nopy aby sedeli instrukcie

Kopirovacia funkcia + return value:
|                      POVODVNE                    |                         NOVE                          |
|--------------------------------------------------|-------------------------------------------------------|
| 00401146  PUSH   EBP                             |   00401146   PUSH   EBP                               |
| 00401147   MOV   EBP ,ESP                        |   00401147    MOV    EBP,ESP                          |
| 00401149  PUSH   ESI                             |   00401149   PUSH   ESI                               |
| 0040114a  PUSH   EDI                             |   0040114a   PUSH   EDI                               |
| 0040114b   XOR   EAX ,EAX                        |   0040114b    XOR    EAX,EAX                          |
| 0040114d   MOV   ESI ,dword ptr [EBP + param_1]  |   0040114d    MOV    ESI,dword ptr [EBP + param_1]    |
| 00401150   MOV   EDI ,dword ptr [EBP + param_2]  |   00401150    MOV    EDI,dword ptr [EBP + param_2]    |
| 00401153   MOV   AL ,byte ptr [ESI + 0x2a]       |   00401153    MOV    AL,byte ptr [ESI + 0x2b]         |
| 00401156   MOV   byte ptr [EDI],AL               |   00401156    MOV    byte ptr [EDI],AL                |
| 00401158   MOV   AL ,byte ptr [ESI]              |   00401158    MOV    AL,byte ptr [ESI + 0x18]         |
| 0040115a   MOV   byte ptr [EDI + 0x1],AL         |   0040115b    MOV    byte ptr [EDI + 0x1],AL          |
| 0040115d   MOV   AL ,byte ptr [ESI]              |   0040115e    MOV    AL,byte ptr [ESI + 0xc]          |
| 00401160   MOV   byte ptr [EDI + 0x2],AL         |   00401161    MOV    byte ptr [EDI + 0x2],AL          |
| 00401163   MOV   AL ,byte ptr [ESI + 0x37]       |   00401164    MOV    AL,byte ptr [ESI + 0xb]          |
| 00401166   MOV   byte ptr [EDI + 0x3],AL         |   00401167    MOV    byte ptr [EDI + 0x3],AL          |
| 00401169   MOV   AL ,byte ptr [ESI + 0x1e]       |   0040116a    MOV    AL,byte ptr [ESI + 0x17]         |
| 0040116c   MOV   byte ptr [EDI + 0x4],AL         |   0040116d    MOV    byte ptr [EDI + 0x4],AL          |
| 0040116f   MOV   AL ,byte ptr [ESI + 0xb]        |   00401170    MOV    AL,byte ptr [ESI + 0x3e]         |
| 00401172   MOV   byte ptr [EDI + 0x5],AL         |   00401173    MOV    byte ptr [EDI + 0x5],AL          |
| 00401175   MOV   AL ,byte ptr [ESI + 0xb]        |   00401177    NOP                                     |
| 00401178   MOV   byte ptr [EDI + 0x6],AL         |   ...         NOP                                     |
| 0040117b   MOV   AL ,byte ptr [ESI + 0x17]       |   0040117f    NOP                                     |
| 0040117e   MOV   byte ptr [EDI + 0x7],AL         |   00401180    NOP                                     |
| 00401181   MOV   EAX ,EDI                        |   00401181    MOV    EAX,EDI                          |
| 00401183   POP   EDI                             |   00401183    POP    EDI                              |
| 00401184   POP   ESI                             |   00401184    POP    ESI                              |
| 00401185  LEAVE                                  |   00401185   LEAVE                                    |
| 00401186   RET   0x8                             |   00401186    RET    0x5                              |

if condition:
|          POVODVNE         |             NOVE           |
|---------------------------|----------------------------|
| 0040109d  CMP   EAX,0x8   |   0040109d  CMP   EAX,0x5  |

argument dalsej funckie:
|           POVODVNE          |           NOVE             |
|-----------------------------|----------------------------|
| 004010ba  MOV   EAX,0x8     |   004010ba  MOV   EAX,0x5  |





# SPAASM - Systémové programovanie a assemblery
## Zadanie č.3 - Statická a dynamická analýza programu
### Autor: Marek Čederle

Na vypracovanie úloh som použil nástroje:
- `HxD`: Hex Editor
- `Ghidra`: Reverse Engineering Framework


1. Správna dĺžka akceptovaného reťazca je 8 znakov. Pri disassemblingu som zistil že funkcia ktorá načítava zadávaný reťazec ukladá svoju návratovú hodnotu do premennej ktorá sa potom porovnáva s hodnotou 8 a vtedy sa vykonávajú ďalšie časti programu. Táto hodnota sa opakuje vo viacerých nasledujúsich častiach programu čo ale spomeniem v ďalších úlohách.
2. Správny reťazec je `FIITgeek`. Program funguje nasledovne:
    - Program pomocou funkcie `GetDlgItemTextA` načíta reťazec a uloží ho na adresu `0x00403058`, pričom daná funkcia pri úspechu vráti počet znakov reťazca. Ak je táto hodnota iná od 8 tak sa nastaví výstupná hláška na `Wrong!` a zobrazí sa okno s touto hláškou. Ak je hodnota 8, tak sa zavolá funkcia ktorej parametre sú dva reťazce (`I4561AsEmblerySuPOhodicka2x3XzgvwpqLJfBCDnFMH90KNG78QRtTUVWjYZ`, `J#ki80Ys`) pričom úlohou danej funkcie je skopírovať znaky z prvého reťazca do druhého reťazca na určené pozície. Takto sa tam skopíruje reťazec `FIITgeek` znak po znaku. Táto funkcie síce nepriradzuje svoju návratovú hodnotu žiadnej premennej ale vráti adresu reťazca `J#ki80Ys`  (teraz už `FIITgeek`), ktorá je uložená v registri `EAX`. Následne sa zavolá ďalšia funkcia, ktorej parametre sú adresa načítaného reťazca a dĺžka očakávaného reťazca. Táto funkcia následne porovnáva znak po znaku reťazec v `EAX` a reťazec na adrese `0x00403058` a ak sa zhodujú tak sa pomocou návratovej hodnoty nastaví register `EAX` na `0x1` a následne sa porovnáva s konštantou `0x1` čo znamená že sa rovnajú a nastaví sa výstupná hláška na `Correct!` a zobrazí sa okno s touto hláškou. Ak sa reťazce nezhodujú tak sa nastaví výstupná hláška na `Wrong!` a zobrazí sa okno s touto hláškou.
3. 
4. 
5. 
6. 
7. najskor som skusal to napasovat s instrukciami tak ze pomenim iba offsety ale to sa mi nedarilo tak som potom zvolil tuto moznost


| Section     | Value        |
|-------------|--------------|
| Name        | John         |
| Age         | 30           |
| --- More Info --- | --- | --- | --- |
| Hobby       | Sport | Music | Reading |
| Skills      | C++   | Python | Rust   |
